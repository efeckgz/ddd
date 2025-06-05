/* -------------------------------------------------
 *  src/wfg.c   –  Wait-For Graph implementation
 * ------------------------------------------------- */
#define _GNU_SOURCE           /* ensure spin-lock APIs are visible */

#include "../lib/wfg.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* ---------- global graph state ---------- */
static ThreadNode   *thread_list_head   = NULL;
static ResourceNode *resource_list_head = NULL;
static pthread_spinlock_t graph_lock;

/* ------------------------------------------------- */
void init_wfg(void)
{
    pthread_spin_init(&graph_lock, PTHREAD_PROCESS_PRIVATE);
}

/* ---------- tiny helpers ---------- */
static inline int resource_id_equal(ResourceId a, ResourceId b)
{
    return a.type == b.type && a.resource_ptr == b.resource_ptr;
}

static ResourceNode *get_or_create_resource(ResourceId rid)
{
    ResourceNode *cur = resource_list_head;
    while (cur) {
        if (resource_id_equal(cur->id, rid))
            return cur;
        cur = cur->next;
    }
    ResourceNode *n = malloc(sizeof(ResourceNode));
    n->id = rid; n->owner = 0;
    n->next = resource_list_head;
    resource_list_head = n;
    return n;
}

/* ------------------------------------------------- */
ThreadNode *get_or_create_thread(pthread_t tid)
{
    pthread_spin_lock(&graph_lock);

    for (ThreadNode *c = thread_list_head; c; c = c->next)
        if (pthread_equal(c->thread, tid)) {
            pthread_spin_unlock(&graph_lock);
            return c;
        }

    ThreadNode *n = calloc(1, sizeof(ThreadNode));
    n->thread = tid;
    n->waiting_for.type = -1;
    n->next = thread_list_head;
    thread_list_head = n;

    pthread_spin_unlock(&graph_lock);
    return n;
}

void clear_thread_waiting(pthread_t tid)
{
    pthread_spin_lock(&graph_lock);
    ThreadNode *t = get_or_create_thread(tid);
    t->waiting_for.type = -1;
    t->waiting_for.resource_ptr = 0;
    pthread_spin_unlock(&graph_lock);
}

/* =================================================
 *                MUTEX HELPERS
 * ================================================= */
void record_thread_waiting_on_mutex(pthread_t tid, pthread_mutex_t *m)
{
    pthread_spin_lock(&graph_lock);
    ThreadNode *t = get_or_create_thread(tid);
    t->waiting_for = (ResourceId){(uintptr_t)m, RESOURCE_MUTEX};
    get_or_create_resource(t->waiting_for);
    pthread_spin_unlock(&graph_lock);
}

void record_thread_owns_mutex(pthread_t tid, pthread_mutex_t *m)
{
    pthread_spin_lock(&graph_lock);

    ThreadNode *t = get_or_create_thread(tid);
    ResourceId  rid = { (uintptr_t)m, RESOURCE_MUTEX };
    ResourceNode *rs = get_or_create_resource(rid);

    if (resource_id_equal(t->waiting_for, rid))
        t->waiting_for.type = -1;

    /* add to holding list if not present */
    for (ResourceList *c = t->holding; ; c = c ? c->next : NULL) {
        if (!c) {                               /* not found → prepend */
            ResourceList *n = malloc(sizeof(ResourceList));
            n->id = rid; n->next = t->holding; t->holding = n;
            break;
        }
        if (resource_id_equal(c->id, rid)) break;
    }
    rs->owner = tid;

    pthread_spin_unlock(&graph_lock);
}

void clear_thread_owns_mutex(pthread_t tid, pthread_mutex_t *m)
{
    pthread_spin_lock(&graph_lock);
    ThreadNode *t = get_or_create_thread(tid);
    ResourceId rid = { (uintptr_t)m, RESOURCE_MUTEX };
    ResourceNode *rs = get_or_create_resource(rid);

    ResourceList **pp = &t->holding, *c = t->holding;
    while (c) {
        if (resource_id_equal(c->id, rid)) { *pp = c->next; free(c); break; }
        pp = &c->next; c = c->next;
    }
    rs->owner = 0;
    pthread_spin_unlock(&graph_lock);
}

/* =================================================
 *             SEMAPHORE HELPERS
 * ================================================= */
void record_thread_waiting_on_semaphore(pthread_t tid, sem_t *s)
{
    pthread_spin_lock(&graph_lock);
    ThreadNode *t = get_or_create_thread(tid);
    t->waiting_for = (ResourceId){(uintptr_t)s, RESOURCE_SEMAPHORE};
    get_or_create_resource(t->waiting_for);
    pthread_spin_unlock(&graph_lock);
}

void record_thread_owns_semaphore(pthread_t tid, sem_t *s)
{
    pthread_spin_lock(&graph_lock);

    ThreadNode *t = get_or_create_thread(tid);
    ResourceId  rid = { (uintptr_t)s, RESOURCE_SEMAPHORE };
    ResourceNode *rs = get_or_create_resource(rid);

    if (resource_id_equal(t->waiting_for, rid))
        t->waiting_for.type = -1;

    for (ResourceList *c = t->holding; ; c = c ? c->next : NULL) {
        if (!c) {
            ResourceList *n = malloc(sizeof(ResourceList));
            n->id = rid; n->next = t->holding; t->holding = n;
            break;
        }
        if (resource_id_equal(c->id, rid)) break;
    }
    rs->owner = tid;

    pthread_spin_unlock(&graph_lock);
}

void clear_thread_owns_semaphore(pthread_t tid, sem_t *s)
{
    pthread_spin_lock(&graph_lock);

    ThreadNode *t = get_or_create_thread(tid);
    ResourceId rid = { (uintptr_t)s, RESOURCE_SEMAPHORE };
    ResourceNode *rs = get_or_create_resource(rid);

    ResourceList **pp = &t->holding, *c = t->holding;
    while (c) {
        if (resource_id_equal(c->id, rid)) { *pp = c->next; free(c); break; }
        pp = &c->next; c = c->next;
    }
    rs->owner = 0;

    pthread_spin_unlock(&graph_lock);
}
/* =================================================
 *                CYCLE DETECTION
 * ================================================= */
static int check_cycle_dfs(ThreadNode *t,
                           ThreadNode **stack,
                           size_t depth)
{
    /* already on recursion stack ⇒ cycle */
    for (size_t i = 0; i < depth; ++i)
        if (stack[i] == t)
            return 1;

    stack[depth] = t;                         /* push */

    /* follow edge: thread ─► waiting_for ─► owner */
    if (t->waiting_for.type != -1) {
        ResourceNode *r = resource_list_head;
        while (r) {
            if (resource_id_equal(r->id, t->waiting_for)) {
                if (r->owner != 0) {
                    ThreadNode *owner = thread_list_head;
                    while (owner) {
                        if (pthread_equal(owner->thread, r->owner)) {
                            if (check_cycle_dfs(owner, stack, depth + 1))
                                return 1;
                            break;
                        }
                        owner = owner->next;
                    }
                }
                break;
            }
            r = r->next;
        }
    }
    /* pop */
    return 0;
}

int check_for_deadlock(pthread_t tid)
{
    /* take a consistent snapshot under the spin-lock */
    pthread_spin_lock(&graph_lock);

    ThreadNode *start = thread_list_head;
    while (start && !pthread_equal(start->thread, tid))
        start = start->next;

    if (!start) {                     /* shouldn’t happen */
        pthread_spin_unlock(&graph_lock);
        return 0;
    }

    /* recursion stack – bounded by #threads in graph */
    /* (graph is small; we can allocate on the stack)  */
    ThreadNode *stack[256];           /* adjust if you expect >256 threads */

    int cycle = check_cycle_dfs(start, stack, 0);

    pthread_spin_unlock(&graph_lock);
    return cycle;
}