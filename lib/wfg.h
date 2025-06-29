#include <stdint.h>
#include <pthread.h>

// Enum to distinguish the two different types of resources.
typedef enum {
    RESOURCE_MUTEX,
    RESOURCE_SEMAPHORE
} ResourceType;

// ResourceId represents a resource in the WFG. A resource is its pointer and its type.
typedef struct {
    uintptr_t resource_ptr; // Either a pthread_mutex_t* or sem_t*
    ResourceType type;
} ResourceId;

// ResourceList is a linked list of resources. Each ThreadNode has a list of resources that it holds.
typedef struct ResourceList {
    ResourceId id;
    struct ResourceList* next;
} ResourceList;

// ThreadNode represents a thread node in the graph.
// A thread can hold many resources, and can be waiting for one resource at a time.
typedef struct ThreadNode {
    pthread_t thread;
    ResourceId waiting_for; // Resource this thread is waiting for
    ResourceList* holding; // A list of resources this thread currently holds.
    struct ThreadNode* next;
} ThreadNode;

// ResourceNode represents a resource node in the graph.
typedef struct ResourceNode {
    ResourceId id;
    pthread_t owner;
    struct ResourceNode* next;
} ResourceNode;

static  ThreadNode* thread_list_head = NULL;
static pthread_spinlock_t graph_lock;

void init_wfg();


// Update the WFG to indicate a thread is waiting on a mutex.
void record_thread_waiting_on_mutex(pthread_t thread, pthread_mutex_t* mutex);

// Update the WFG to indicate a thread owns a mutex.
void record_thread_owns_mutex(pthread_t thread, pthread_mutex_t* mutex);

void clear_thread_owns_mutex(pthread_t thread, pthread_mutex_t* mutex);

void record_thread_waiting_on_semaphore(pthread_t thread);

// Update the WFG to indiacate a thread has stopped waiting on a mutex.
void clear_thread_waiting(pthread_t thread);


ThreadNode* get_or_create_thread(pthread_t tid);
