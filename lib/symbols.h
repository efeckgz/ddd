
#ifndef SYMBOLS_H
#define SYMBOLS_H

#define _GNU_SOURCE          

#include <pthread.h>
#include <semaphore.h>


int pthread_mutex_lock   (pthread_mutex_t *mutex);
int pthread_mutex_unlock (pthread_mutex_t *mutex);

int sem_wait             (sem_t *sem);
int sem_post             (sem_t *sem);

#endif
