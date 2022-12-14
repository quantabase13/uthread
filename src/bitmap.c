/* 
Piko/RT is freely redistributable under the two-clause BSD License:

Copyright (c) 2017 Piko/RT Developers.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "bitmap.h"



#define SWAP(x, y)             \
    {                          \
        __typeof__(x) tmp = x; \
        x = y;                 \
        y = tmp;               \
    }                          \
    while (0)


static struct bitmap_struct _active, _expire;
extern task *task_idle;

struct sched_struct {
    struct bitmap_struct *active;
    struct bitmap_struct *expire;
};

struct sched_struct task_sched = {
    .active = &_active,
    .expire = &_expire,
};

static int sched_bitmap_init(void)
{
    INIT_BITMAP(task_sched.active);
    INIT_BITMAP(task_sched.expire);
    return 0;
}

static task *find_next_task(struct bitmap_struct *bm)
{
    int max_prio = find_first_bit(&bm->map, 32);

    /* all runqueues are empty, return the task_idle */
    if (max_prio == 32)
        return task_idle;  // idle_thread

    return bitmap_first_entry(bm, max_prio, task, ti_q);
}


static int task_enqueue(task *tsk, struct bitmap_struct *bm)
{
    bitmap_enqueue(&tsk->ti_q, tsk->task_priority, bm);

    return 0;
}

static int sched_bitmap_enqueue(task *tsk)
{
    return task_enqueue(tsk, task_sched.active);
}

static int task_dequeue(task *tsk, struct bitmap_struct *bm)
{
    /* active thread is not in the runqueue */
    if (tsk == task_current)
        return -1;

    bitmap_queue_del(&tsk->ti_q, tsk->task_priority, bm);

    return 0;
}

static int sched_bitmap_dequeue(task *tsk)
{
    return task_dequeue(tsk, task_sched.active);
}

static task *sched_bitmap_elect(void *NOUSE)
{
    task *next;
    task *current = task_current;

    next = find_next_task(task_sched.active);

    // check each thread timeslice in active queue
    // if necessary swap active and expire queue
    if (next == task_idle && find_next_task(task_sched.expire) != task_idle) {
        SWAP(task_sched.active, task_sched.expire);
        next = find_next_task(task_sched.active);
    }


    if (next != task_idle) {  // idle_thread
        task_dequeue(next, task_sched.active);
    } else {
        return next;
    }
    if (current == task_idle || current->state == TERMINATED) {
        return next;
    }

    if (current->timeslice_cur == 0) {
        current->timeslice_cur = current->timeslice;
        task_enqueue(current, task_sched.expire);
    } else {
        task_enqueue(current, task_sched.active);
    }
    return next;
}

struct sched sched_bitmap = {
    .init = sched_bitmap_init,
    .enqueue = sched_bitmap_enqueue,
    .dequeue = sched_bitmap_dequeue,
    .elect = sched_bitmap_elect,
};
