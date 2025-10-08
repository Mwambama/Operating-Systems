/* ex1.c -- pthread create/join demo */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *thread1(void *arg) {
    sleep(5);                      // intentionally delay thread start
    printf("Hello, I am thread 1\n");
    return NULL;
}

void *thread2(void *arg) {
    sleep(5);                      // intentionally delay thread start
    printf("Hello, I am thread 2\n");
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    /* Without joins: main might exit before threads print */
    // commenting out the join lines below, main may finish and kill threads.

    //pthread_join(t1, NULL);
     // pthread_join(t2, NULL);

    printf("Hello, I am main process\n");
    return 0;
}

