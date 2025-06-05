#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_COND_INITIALIZER;

void* thread_func1(void* arg) {
    pthread_mutex_lock(&mutex1);
    printf("Thread 1 locked mutex1\n");

    // Simulate some work
    sleep(1);

    printf("Thread 1 trying to lock mutex2\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 1 locked mutex2\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    return NULL;
}

void* thread_func2(void* arg) {
    pthread_mutex_lock(&mutex2);
    printf("Thread 2 locked mutex2\n");

    // Simulate some work
    sleep(1);

    printf("Thread 2 trying to lock mutex1\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2 locked mutex1\n");

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);

    return NULL;
}


int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread_func1, NULL);
    pthread_create(&t2, NULL, thread_func2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Main thread: both threads completed\n");
    return 0;
}
