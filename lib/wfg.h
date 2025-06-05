#ifndef WFG_H
#define WFG_H

#define _GNU_SOURCE          /* makes spin-lock APIs visible */

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

/* ---------- resource kinds ---------- */
typedef enum {
    RESOURCE_MUTEX,
    RESOURCE_SEMAPHORE
} ResourceType;

/* ---------- resource identity ---------- */
typedef struct {
    uintptr_t   resource_ptr;   /* pointer value of mutex or semaphore   */
    ResourceType type;          /* which kind                            */
} ResourceId;

/* ---------- linked-list node for “resources held” ---------- */
typedef struct ResourceList {
    ResourceId             id;
    struct ResourceList   *next;
} ResourceList;

/* ---------- graph vertices ---------- */
typedef struct ThreadNode {
    pthread_t      thread;        /* thread id                            */
    ResourceId     waiting_for;   /* resource currently awaited, or -1    */
    ResourceList  *holding;       /* linked list of owned resources       */
    struct ThreadNode *next;      /* singly-linked list of thread nodes   */
} ThreadNode;

typedef struct ResourceNode {
    ResourceId      id;           /* pointer + type                       */
    pthread_t       owner;        /* 0 if unowned                         */
    struct ResourceNode *next;    /* singly-linked list of resource nodes */
} ResourceNode;

void init_wfg(void);
/* create / fetch the ThreadNode for tid */
ThreadNode *get_or_create_thread(pthread_t tid);
/* clear the “waiting_for” edge for tid */
void clear_thread_waiting(pthread_t tid);
/* ---- mutex helpers ---- */
void record_thread_waiting_on_mutex(pthread_t tid, pthread_mutex_t *mutex);
void record_thread_owns_mutex   (pthread_t tid, pthread_mutex_t *mutex);
void clear_thread_owns_mutex    (pthread_t tid, pthread_mutex_t *mutex);
/* ---- semaphore helpers ---- */
void record_thread_waiting_on_semaphore(pthread_t tid, sem_t *sem);
void record_thread_owns_semaphore   (pthread_t tid, sem_t *sem);
void clear_thread_owns_semaphore    (pthread_t tid, sem_t *sem);

/* ---- cycle detection ---- */
int check_for_deadlock(pthread_t tid);

#endif /* WFG_H */
