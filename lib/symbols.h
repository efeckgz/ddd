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

// Run the DFS algorithm to check for a cycle in the WFG.
// Rerturns 1 if a deadlock is detected.
int check_for_deadlock(pthread_t thread);

// Overriden pthread_mutex_lock function.
int pthread_mutex_lock(pthread_mutex_t* mutex);

int pthread_mutex_unlock(pthread_mutex_t* mutex);
