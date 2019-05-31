#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include <time.h>
#include <unistd.h>

pthread_t tid[2];

void* do_fun2(void *arg);
void* do_fun1(void *arg);

pthread_mutex_t lock1;
pthread_mutex_t lock2;

void
noise()
{
	usleep(rand() % 1000) ;
}


void* do_fun1(void *arg)
{
    pthread_mutex_lock(&lock2);
    noise();
    pthread_mutex_lock(&lock1);

    printf("\n Fun 1  started\n");

    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock2);
    return NULL;
}

void* do_fun2(void *arg)
{
    pthread_mutex_lock(&lock1);
    noise();
    pthread_mutex_lock(&lock2);

    printf("\n Fun 2 started\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);



    return NULL;
}

int main(void)
{
    int i = 0;
    int err;

    if (pthread_mutex_init(&lock1, NULL) != 0)
    {
        printf("\n mutex1 init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lock2, NULL) != 0)
    {
        printf("\n mutex2 init failed\n");
        return 1;
    }

    pthread_create(&(tid[0]), NULL, &do_fun1, NULL);
    pthread_create(&(tid[1]), NULL, &do_fun2, NULL);

    pthread_join(tid[1], NULL);
    pthread_join(tid[0], NULL);


    return 0;
}

