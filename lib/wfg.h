#ifndef WFG_H
#define WFG_H

#define _GNU_SOURCE   //makes spinlock available

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

//resource types
typedef enum {
    RESOURCE_MUTEX,
    RESOURCE_SEMAPHORE
} ResourceType;


typedef struct {
    uintptr_t   resource_ptr;  
    ResourceType type;          
} ResourceId;

//linked list node for resource held
typedef struct ResourceList {
    ResourceId             id;
    struct ResourceList   *next;
} ResourceList;


typedef struct ThreadNode {
    pthread_t      thread;        
    ResourceId     waiting_for;   
    ResourceList  *holding;       
    struct ThreadNode *next;      
} ThreadNode;

typedef struct ResourceNode {
    ResourceId      id;          
    pthread_t       owner;        
    struct ResourceNode *next;    
} ResourceNode;

void init_wfg(void);

//create or fetch threadNode for tid
ThreadNode *get_or_create_thread(pthread_t tid);

//clear the waiting_for edge for tid
void clear_thread_waiting(pthread_t tid);
//mutex helpers
void record_thread_waiting_on_mutex(pthread_t tid, pthread_mutex_t *mutex);
void record_thread_owns_mutex   (pthread_t tid, pthread_mutex_t *mutex);
void clear_thread_owns_mutex    (pthread_t tid, pthread_mutex_t *mutex);
//sem helpers
void record_thread_waiting_on_semaphore(pthread_t tid, sem_t *sem);
void record_thread_owns_semaphore   (pthread_t tid, sem_t *sem);
void clear_thread_owns_semaphore    (pthread_t tid, sem_t *sem);

//cycle detection
int check_for_deadlock(pthread_t tid);

#endif 
