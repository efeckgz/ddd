#include "../lib/wfg.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

void init_wfg() {
    pthread_spin_init(&graph_lock, PTHREAD_PROCESS_PRIVATE);
}

void record_thread_waiting_on_mutex(pthread_t thread, pthread_mutex_t *mutex) {
    pthread_spin_lock(&graph_lock);

    ThreadNode* node = get_or_create_thread(thread);

    // Check if this thread already holds the mutex. This shouldnt ever happen, but it is a good check.
    ResourceList* cur = node->holding;
    while (cur != NULL) {
        if (cur->id.resource_ptr == (uintptr_t) mutex && cur->id.type == RESOURCE_MUTEX) {
            // Thread already owns the mutex, dont mark as waiting.
            pthread_spin_unlock(&graph_lock);
            return;
        }
        cur = cur->next;
    }

    // Mark thread waiting for this mutex
    node->waiting_for.resource_ptr = (uintptr_t) mutex;
    node->waiting_for.type = RESOURCE_MUTEX;

    pthread_spin_unlock(&graph_lock);
}

ThreadNode* get_or_create_thread(pthread_t tid) {
    pthread_spin_lock(&graph_lock);

    // Search for an existing node
    ThreadNode* cur = thread_list_head;
    while (cur != NULL) {
        if (pthread_equal(cur->thread, tid)) {
            // Found the thread in the graph
            pthread_spin_unlock(&graph_lock);
            return cur;
        }

        cur = cur->next;
    }

    // Not found, create a new ThreadNode
    ThreadNode* new_node = (ThreadNode*) malloc(sizeof(ThreadNode));
    new_node->thread = tid;
    new_node->waiting_for.type = -1; // Invalid type, not waiting for anything
    new_node->waiting_for.resource_ptr = 0;
    new_node->holding = NULL;

    // Insert the new thread at head
    new_node->next = thread_list_head;
    thread_list_head = new_node;

    pthread_spin_unlock(&graph_lock);
    return new_node;
}
