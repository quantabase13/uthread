#pragma once
#define STACK_SIZE 1024 * 32
#include <setjmp.h>
#include "linux/list.h"
typedef enum {
    NOT_STARTED = 0,
    INITIALIZED,
    RUNNING,
    WAITING,
    TERMINATED
} state;

struct task_internal {
    jmp_buf env;
    state state;
    void *args;
    void *retval;
    int thread_index;
    int thread_waited;
    int timeslice;
    int timeslice_cur;
    int task_priority;
    struct list_head ti_q;
    char stack[1];
};


typedef struct task_internal task;
typedef int _pthread_t;
typedef struct {
    task *task;
} _pthread_attr_t;

task *task_current;


int pthread_create(_pthread_t *thread,
                   const _pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *args);
int pthread_join(_pthread_t thread, void **value_ptr);
void pthread_exit(void *value_ptr);
void pthread_yield(void);