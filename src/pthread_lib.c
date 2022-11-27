#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "pthread_lib.h"
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "bitmap.h"
#include "linux/list.h"
#include "sem.h"
/* O(1) scheduler*/
extern struct sched sched_bitmap;

#define U_THREAD_MAX 16
#define TIME_QUANTUM 500000 /* in us */

/* timer management */
static struct itimerval time_quantum;

/* global spinlock for critical section _queue */
static uint _spinlock = 0;

/* idle task (main loop)*/
task *task_idle;
int thread_index = 0;

typedef struct {
    sem_t semaphore;
    unsigned long *val;
} sig_sem;

static sig_sem sigsem_thread[U_THREAD_MAX];



static void pthread_wrapper(task *task);
static void pthread_subsystem_init();
static void pthread_scheduler(int sig);
static long int i64_ptr_mangle(long int p);
static long int i64_ptr_mangle_re(long int p);
int pthread_create(_pthread_t *thread,
                   const _pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *args)
{
    static bool first_run = true;
    if (first_run) {
        first_run = false;
        pthread_subsystem_init();
    }
    task *tsk = malloc(sizeof(task) + 1 + STACK_SIZE);

    (tsk->env)[0].__jmpbuf[6] =
        i64_ptr_mangle((unsigned long long) (tsk->stack + STACK_SIZE));
    (tsk->env)[0].__jmpbuf[7] =
        i64_ptr_mangle((unsigned long long) start_routine);

    tsk->state = INITIALIZED;
    tsk->thread_waited = 0;
    tsk->args = args;
    tsk->thread_index = thread_index;
    tsk->timeslice = 5;
    tsk->task_priority = 0;
    *thread = thread_index;
    thread_index++;

    sigsem_thread[tsk->thread_index].val = NULL;
    sem_init(&sigsem_thread[tsk->thread_index].semaphore, 0);

    while (__atomic_test_and_set(&_spinlock, __ATOMIC_ACQUIRE))
        ;
    sched_bitmap.enqueue(tsk);
    __atomic_store_n(&_spinlock, 0, __ATOMIC_RELEASE);
    struct sigaction sched_handler = {
        .sa_flags = SA_NODEFER,
        .sa_handler =
            &pthread_scheduler, /* set signal handler to call scheduler */
    };
    sigaction(SIGPROF, &sched_handler, NULL);

    setitimer(ITIMER_PROF, &time_quantum, NULL);
    setjmp(task_idle->env);
    return 0;
}

static int tick(void *NOUSE)
{
    sigset_t newmask, oldmask;
    sigaddset(&newmask, SIGPROF);
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    while (1) {
        if (task_current != task_idle && task_current->timeslice > 0)
            task_current->timeslice--;
        usleep(500);
    }
    return 0;
}

static void pthread_subsystem_init()
{
    char *stack = (char *) malloc(STACK_SIZE);
    if (-1 == clone((int (*)(void *)) tick, (char *) stack + STACK_SIZE,
                    SIGCHLD | CLONE_SIGHAND | CLONE_VM | CLONE_PTRACE, NULL)) {
        perror("Failed to invoke clone system call.");
        free(stack);
        return;
    }
    sched_bitmap.init();
    task_idle = (task *) malloc(sizeof(task) + 1 + STACK_SIZE);

    (task_idle->env)[0].__jmpbuf[6] =
        i64_ptr_mangle((unsigned long long) (task_idle->stack + STACK_SIZE));
    task_idle->timeslice = 5;
    task_current = task_idle;
    time_quantum.it_value.tv_sec = 0;
    time_quantum.it_value.tv_usec = TIME_QUANTUM;
    time_quantum.it_interval.tv_sec = 0;
    time_quantum.it_interval.tv_usec = TIME_QUANTUM;
}

static void pthread_scheduler(int sig)
{
    while (__atomic_test_and_set(&_spinlock, __ATOMIC_ACQUIRE))
        ;
    task *task_next = sched_bitmap.elect((void *) 0);
    __atomic_store_n(&_spinlock, 0, __ATOMIC_RELEASE);
    switch (setjmp((task_current)->env)) {
    case 0:
        if (task_next->state == INITIALIZED) {
            task_next->state = RUNNING;
            task_current = task_next;
            pthread_wrapper(task_next);
        } else {
            task_next->state = RUNNING;
            task_current = task_next;
            longjmp(task_next->env, 1);
        }
    case 1:
        return;
    }
    /*context switch code end*/
}

static void pthread_wrapper(task *tsk)
{
    register unsigned long long rsp =
        i64_ptr_mangle_re((tsk->env)[0].__jmpbuf[6]);
    register unsigned long long pc =
        i64_ptr_mangle_re((tsk->env)[0].__jmpbuf[7]);
    register task *tmp = tsk;
    void *value_ptr;
    asm("mov %0, %%rsp;" : : "r"(rsp));
    asm("mov %%rsp, %%rbp;" ::);
    void (*funptr)(void *) = pc;
    task *task_tmp = tmp;
    funptr(task_tmp->args);

    asm("mov %%rax, %0;" : "=r"(value_ptr)::);
    task_tmp->state = TERMINATED;
    sem_post(&(sigsem_thread[task_tmp->thread_index].semaphore));
    sigsem_thread[task_tmp->thread_index].val = malloc(sizeof(unsigned long));
    memcpy(sigsem_thread[task_tmp->thread_index].val, value_ptr,
           sizeof(unsigned long));

    while (__atomic_test_and_set(&_spinlock, __ATOMIC_ACQUIRE))
        ;
    task *task_next = sched_bitmap.elect((void *) 0);

    __atomic_store_n(&_spinlock, 0, __ATOMIC_RELEASE);
    if (task_next == task_idle) {
        longjmp(task_idle->env, 1);
    } else {
        if (task_next->state == INITIALIZED) {
            task_next->state = RUNNING;
            task_current = task_next;
            pthread_wrapper(task_next);
        } else {
            task_next->state = RUNNING;
            task_current = task_next;
            longjmp(task_next->env, 1);
        }
    }
}

/* wait for thread termination */
int pthread_join(_pthread_t thread, void **value_ptr)
{
    /* do P() in thread semaphore until the certain user-level thread is done */
    sem_wait(&(sigsem_thread[thread].semaphore));
    /* get the value's location passed to fiber_exit */
    if (value_ptr && sigsem_thread[thread].val)
        memcpy((unsigned long *) *value_ptr, sigsem_thread[thread].val,
               sizeof(unsigned long));
    return 0;
}


void pthread_exit(void *value_ptr)
{
    sem_post(&(sigsem_thread[task_current->thread_index].semaphore));
    if (value_ptr){
    sigsem_thread[task_current > thread_index].val =
        malloc(sizeof(unsigned long));
    memcpy(sigsem_thread[task_current->thread_index].val, value_ptr,
           sizeof(unsigned long));       
    }
    while (__atomic_test_and_set(&_spinlock, __ATOMIC_ACQUIRE))
        ;
    task_current->state = TERMINATED;
    task *task_next = sched_bitmap.elect((void *) 0);
    __atomic_store_n(&_spinlock, 0, __ATOMIC_RELEASE);
    if (task_next == task_idle) {
        longjmp(task_idle->env, 1);
    } else {
        if (task_next->state == INITIALIZED) {
            task_next->state = RUNNING;
            task_current = task_next;
            pthread_wrapper(task_next);
        } else {
            task_next->state = RUNNING;
            task_current = task_next;
            longjmp(task_next->env, 1);
        }
    }
}

void pthread_yield()
{
    while (__atomic_test_and_set(&_spinlock, __ATOMIC_ACQUIRE))
        ;
    task *task_next = sched_bitmap.elect((void *) 0);
    __atomic_store_n(&_spinlock, 0, __ATOMIC_RELEASE);
    switch (setjmp((task_current)->env)) {
    case 0:
        if (task_next->state == INITIALIZED) {
            task_next->state = RUNNING;
            task_current = task_next;
            pthread_wrapper(task_next);
        } else {
            task_next->state = RUNNING;
            task_current = task_next;
            longjmp(task_next->env, 1);
        }
    case 1:
        return;
    }
}
static long int i64_ptr_mangle(long int p)
{
    long int ret;
    asm(" mov %1, %%rax;\n"
        " xor %%fs:0x30, %%rax;"
        " rol $0x11, %%rax;"
        " mov %%rax, %0;"
        : "=r"(ret)
        : "r"(p)
        : "%rax");
    return ret;
}

static long int i64_ptr_mangle_re(long int p)
{
    long int ret;
    asm(" mov %1, %%rax;\n"
        " ror $0x11, %%rax;"
        " xor %%fs:0x30, %%rax;"
        " mov %%rax, %0;"
        : "=r"(ret)
        : "r"(p)
        : "%rax");
    return ret;
}