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
    ThreadNode* next;
} ThreadNode;

// ResourceNode represents a resource node in the graph.
typedef struct ResourceNode {
    ResourceId id;
    pthread_t owner;
    struct ResourceNode* next;
} ResourceNode;

static ThreadNode* thread_list_head = NULL;
static pthread_spinlock_t graph_lock;
