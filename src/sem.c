#include "sem.h"
#include <stdbool.h>
#include "linux/list.h"
#include "pthread_lib.h"

static int down(sem_t *sem);
static void up(sem_t *sem);
static int spin_lock(spinlock_t *lock);
static int spin_unlock(spinlock_t *lock);

int sem_init(sem_t *sem, unsigned int val)
{
    *sem = (sem_t) SEMAPHORE_INITIALIZER(*sem, val);
    return 0;
}


int sem_wait(sem_t *sem)
{
    spin_lock(&sem->lock);
    if (sem->count > 0) {
        sem->count--;
    } else {
        down(sem);
    }
    spin_unlock(&sem->lock);
    return 0;
}

int sem_post(sem_t *sem)
{
    spin_lock(&sem->lock);
    if (list_empty(&sem->wait_list))
        sem->count++;
    else
        up(sem);
    spin_unlock(&sem->lock);
    return 0;
}

struct semaphore_waiter {
    struct list_head list;
    task *task;
    bool up;
};

static int down(sem_t *sem)
{
    struct semaphore_waiter waiter;

    list_add_tail(&waiter.list, &sem->wait_list);
    waiter.task = task_current;
    waiter.up = false;

    for (;;) {
        spin_unlock(&sem->lock);
        task_current->state = WAITING;
        pthread_yield();
        spin_lock(&sem->lock);
        if (waiter.up)
            return 0;
    }
}

static void up(struct semaphore *sem)
{
    struct semaphore_waiter *waiter =
        list_first_entry(&sem->wait_list, struct semaphore_waiter, list);
    list_del(&waiter->list);
    waiter->up = true;
    waiter->task->state = RUNNING;
}



/**
 * @brief Acquire a lock and wait atomically for the lock object
 * @param lock Spinlock object
 */
static inline int spin_lock(spinlock_t *l)
{
    int out;
    volatile uint *lck = &(l->lock);
    asm("whileloop:"
        "xchg %%al, (%1);"
        "test %%al,%%al;"
        "jne whileloop;"
        : "=r"(out)
        : "r"(lck));
    return 0;
}

/**
 * @brief Release lock atomically
 * @param lock Spinlock object
 */
static inline int spin_unlock(spinlock_t *l)
{
    int out;
    volatile uint *lck = &(l->lock);
    asm("movl $0x0,(%1);" : "=r"(out) : "r"(lck));
    return 0;
}