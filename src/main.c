#include <stdio.h>
#include "pthread_lib.h"

int foo();
void bar();
void baz();

int counter = 0;
volatile unsigned int lock = 0;

int main()
{
    int test = 199;
    int test1 = 50;
    int test2 = 10;

    _pthread_t a;
    _pthread_t b;
    _pthread_t c;
    pthread_create(&a, NULL, (void *) foo, (void *) &test);
    pthread_create(&b, NULL, (void *) bar, (void *) &test1);
    pthread_create(&c, NULL, (void *) baz, (void *) &test2);
    for (;;) {
        ;
    }
}

int foo(void *arg)
{
    int counter_foo = *(int *) arg;
    int i = 0;
    while (1) {
        printf("counter_foo = %d\n", counter_foo);
        counter_foo -= 1;
        // printf("index = %d\n",(int) pthread_self());
        i++;
        // SpinLock(&lock);
        // counter++;
        // SpinUnlock(&lock);
        // yield();
    }

    pthread_exit(NULL);
}

void bar(void *arg)
{
    int counter_bar = *(int *) arg;
    int j = 0;
    while (1) {
        printf("counter_bar= %d\n", counter_bar);
        counter_bar += 10;
        // printf("j = %d\n", j);
        j++;
        // SpinLock(&lock);
        // counter++;
        // SpinUnlock(&lock);
        // yield();
    }
    pthread_exit(NULL);
}

void baz(void *arg)
{
    int counter_baz = *(int *) arg;
    int i = 0;
    while (1) {
        printf("counter_baz= %d\n", counter_baz);
        counter_baz += 100;
        i++;
        // yield();
    }
    pthread_exit(NULL);
}
