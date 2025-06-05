#include "../lib/symbols.h"
// #include <cerrno>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int (*real_pthread_mutex_lock)(pthread_mutex_t* mutex) = NULL;
static int (*real_pthread_mutex_unlock)(pthread_mutex_t* mutex) = NULL;

static int (*real_sem_wait)(sem_t* sem) = NULL;
static int (*real_sem_post)(sem_t* sem) = NULL;

// Call this in the overriden pthread and semaphore functions, before the WFG logic to make sure that the original symbols are saved.
static void resolve_real_symbols() {
    if (!real_pthread_mutex_lock) {
        real_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
        if (!real_pthread_mutex_lock) {
            printf("Error resolving pthread_mutex_lock\n");
            exit(1);
        }
    }

    if (!real_pthread_mutex_unlock) {
        real_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
        if (!real_pthread_mutex_unlock) {
            printf("Error resolving pthread_mutex_unlock\n");
            exit(1);
        }
    }

    if (!real_sem_wait) {
        real_sem_wait = dlsym(RTLD_NEXT, "sem_wait");
        if (!real_sem_wait) {
            printf("Error resolving sem_wait\n");
            exit(1);
        }
    }

    if (!real_sem_post) {
        real_sem_post = dlsym(RTLD_NEXT, "sem_post");
        if (!real_sem_post) {
            printf("Error resolving sem_post\n");
            exit(1);
        }
    }
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    resolve_real_symbols(); // Make sure the original functions are saved.

    pthread_t self = pthread_self(); // This thread

    // See if the thread can acquire the lock. If it can, then the lock is given to the thread.
    // If not, instead of blocking until lock can be acquired like pthread_mutex_lock, this function returns a value.
    // We will use the return of this function to update the WFG and run real_pthread_mutex_lock.
    int try_result = pthread_mutex_trylock(mutex);
    if (try_result == 0) {
        // Lock acquired immediately. // Mark in graph and return.
        record_thread_owns_mutex(self, mutex);
        return 0;
    } else if (try_result == EBUSY) {
        // Resource held by another thread and locking would block. Update the WFG and call real function.
        record_thread_waiting_on_mutex(self, mutex);

        // Abort if acquiring this lock would result in a deadlock.
        if (check_for_deadlock(self)) {
            printf("Deadlock: Thread %lu waiting on mutex %p\n", (unsigned long) self, (void*) mutex);
            exit(1);
        }

        // Acquiring the lock will not cause a deadlock, so call the real function.
        // This will block until the resource can be acquired, but we are in a safe state.
        int real_result = real_pthread_mutex_lock(mutex);

        // At this point the resource was released by other thread and this thread acquires it.
        // Mark it on the WFG.
        clear_thread_waiting(self);
        record_thread_owns_mutex(self, mutex);

        return real_result;
    } else {
        // Resource not given to thread for some other reason.
        return try_result;
    }
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    resolve_real_symbols();

    pthread_t self = pthread_self();

    clear_thread_owns_mutex(self, mutex);

    return real_pthread_mutex_unlock(mutex);
}

// int sem_wait(sem_t* sem) {
//     pthread_t self = pthread_self(); // This thread
//     int value;

//     if (sem_getvalue(sem, &value) == 0 && value <= 0) {
//         // Will likely block - add wait-for info
//         record_thread_waiting_on_semaphore(self);
//     }
// }

int sem_post(sem_t* sem) {
    pthread_t self = pthread_self(); // This thread

    // Clear thread waiting on this semaphore
    clear_thread_waiting(self);

    // Call the real sem_post
    return real_sem_post(sem);
}
