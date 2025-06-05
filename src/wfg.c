#include "../lib/wfg.h"
#include <pthread.h>
#include <stdlib.h>

void init_wfg() {
    pthread_spin_init(&graph_lock, PTHREAD_PROCESS_PRIVATE);
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
