#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


static sem_t semA;
static sem_t semB;

static void* th1(void* arg)
{
    sem_wait(&semA);                         
    printf("T1 took  semA\n");
    sleep(1);                                

    printf("T1 trying semB\n");
    sem_wait(&semB);//will deadlocked here
    printf("T1 took  semB  (won’t print without preload)\n");

    sem_post(&semB);
    sem_post(&semA);
    return NULL;
}

static void* th2(void* arg)
{
    sem_wait(&semB);                         
    printf("T2 took  semB\n");
    sleep(1);

    printf("T2 trying semA\n");
    sem_wait(&semA);  //circular wait here
    printf("T2 took  semA  (won’t print without preload)\n");

    sem_post(&semA);
    sem_post(&semB);
    return NULL;
}

int main(void)
{
    sem_init(&semA, 0, 1);
    sem_init(&semB, 0, 1);

    pthread_t a, b;
    pthread_create(&a, NULL, th1, NULL);
    pthread_create(&b, NULL, th2, NULL);

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    puts("Main: threads finished (shouldn’t reach here without detector)");
    return 0;
}
