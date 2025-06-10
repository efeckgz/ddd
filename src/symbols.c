#define _GNU_SOURCE           

#include "../lib/wfg.h"       //graph helpers
#include "../lib/symbols.h"  //function prototypes

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//function pointers to the real libc functions
static int (*real_pthread_mutex_lock)  (pthread_mutex_t *)        = NULL;
static int (*real_pthread_mutex_unlock)(pthread_mutex_t *)        = NULL;
static int (*real_sem_wait)            (sem_t *)                  = NULL;
static int (*real_sem_post)            (sem_t *)                  = NULL;

//one time initialisation of both WFG and real functions
static void __attribute__((constructor)) dd_init(void)
{
    init_wfg();              
}

static void resolve_real_symbols(void)
{
    if (!real_pthread_mutex_lock) {
        real_pthread_mutex_lock =
            (int (*)(pthread_mutex_t *)) dlsym(RTLD_NEXT, "pthread_mutex_lock");
        if (!real_pthread_mutex_lock) { perror("dlsym pthread_mutex_lock"); exit(1); }
    }

    if (!real_pthread_mutex_unlock) {
        real_pthread_mutex_unlock =
            (int (*)(pthread_mutex_t *)) dlsym(RTLD_NEXT, "pthread_mutex_unlock");
        if (!real_pthread_mutex_unlock) { perror("dlsym pthread_mutex_unlock"); exit(1); }
    }

    if (!real_sem_wait) {
        real_sem_wait  =
            (int (*)(sem_t *)) dlsym(RTLD_NEXT, "sem_wait");
        if (!real_sem_wait) { perror("dlsym sem_wait"); exit(1); }
    }

    if (!real_sem_post) {
        real_sem_post  =
            (int (*)(sem_t *)) dlsym(RTLD_NEXT, "sem_post");
        if (!real_sem_post) { perror("dlsym sem_post"); exit(1); }
    }
}


int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    resolve_real_symbols();

    pthread_t self = pthread_self();
    int try_result = pthread_mutex_trylock(mutex);

    if (try_result == 0) {                         //fast-path acquire
        record_thread_owns_mutex(self, mutex);
        return 0;
    }
    if (try_result == EBUSY) {   // mutex is busy, we might block
        record_thread_waiting_on_mutex(self, mutex);

        if (check_for_deadlock(self)) {            //cycle detection
            fprintf(stderr, "Deadlock: thread %lu waiting on mutex %p\n",
                    (unsigned long)self, (void *)mutex);
            exit(1);
        }

        //safe to block, so we call the real function
        int real_result = real_pthread_mutex_lock(mutex);

        clear_thread_waiting(self);
        record_thread_owns_mutex(self, mutex);
        return real_result;
    }
    return try_result;                             
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    resolve_real_symbols();

    pthread_t self = pthread_self();
    clear_thread_owns_mutex(self, mutex);
    return real_pthread_mutex_unlock(mutex);
}


int sem_wait(sem_t *sem)
{
    resolve_real_symbols();

    pthread_t self = pthread_self();
    int sval;

    //if value <= 0, we’ll almost certainly block
    if (sem_getvalue(sem, &sval) == 0 && sval <= 0) {
        record_thread_waiting_on_semaphore(self, sem);

        if (check_for_deadlock(self)) {
            fprintf(stderr, "[DEADLOCK DETECTED] Thread %lu waiting on semaphore %p\n",
                    (unsigned long)self, (void *)sem);
            exit(1);
        }
    }
    // safe to block, so we call the real function
    int rc = real_sem_wait(sem);

    if (rc == 0) {
        clear_thread_waiting(self);
        record_thread_owns_semaphore(self, sem);
    }
    return rc;
}

int sem_post(sem_t *sem)
{
    resolve_real_symbols();

    pthread_t self = pthread_self();
    clear_thread_owns_semaphore(self, sem);
    return real_sem_post(sem);
}
