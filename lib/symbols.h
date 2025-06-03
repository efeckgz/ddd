#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>

// Save the real functionality of the synchronization functions into function pointers.
// Call the saved functions from the overriden ones to keep original behavior.
static void resolve_real_symbols();

// Update the WFG to indicate a thread is waiting on a mutex.
void record_thread_waiting_on_mutex(pthread_t thread, pthread_mutex_t* mutex);

// Update the WFG to indicate a thread owns a mutex.
void record_thread_owns_mutex(pthread_t thread, pthread_mutex_t* mutex);

void clear_thread_owns_mutex(pthread_t thread, pthread_mutex_t* mutex);

void record_thread_waiting_on_semaphore(pthread_t thread);

// Update the WFG to indiacate a thread has stopped waiting on a mutex.
void clear_thread_waiting(pthread_t thread);

// Run the DFS algorithm to check for a cycle in the WFG.
// Rerturns 1 if a deadlock is detected.
int check_for_deadlock(pthread_t thread);

// Overriden pthread_mutex_lock function.
int pthread_mutex_lock(pthread_mutex_t* mutex);

int pthread_mutex_unlock(pthread_mutex_t* mutex);
