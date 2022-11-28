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
#ifndef _LINUX_BITMAP_H
#define _LINUX_BITMAP_H
#include "linux/list.h"
#include "linux/compiler.h"
#include "pthread_lib.h"
#include "bitops.h"

struct bitmap_struct {
    unsigned long map;
    struct list_head queue[32];
};

static inline void INIT_BITMAP(struct bitmap_struct *bm)
{
    WRITE_ONCE(bm->map, 0UL);
    for (int i = 0; i < 32; i++)
        INIT_LIST_HEAD(&bm->queue[i]);
}


static inline void bitmap_queue_add(struct list_head *new,
                                    unsigned long bit,
                                    struct bitmap_struct *bm)
{
    list_add_tail(new, &bm->queue[bit]);
    bitmap_set_bit(&bm->map, bit);
}

static inline void bitmap_queue_add_first(struct list_head *new,
                                          unsigned long bit,
                                          struct bitmap_struct *bm)
{
    list_add(new, &bm->queue[bit]);
    bitmap_set_bit(&bm->map, bit);
}

static inline void bitmap_enqueue(struct list_head *new,
                                  unsigned long bit,
                                  struct bitmap_struct *bm)
{
    bitmap_queue_add(new, bit, bm);
}

static inline void bitmap_enqueue_first(struct list_head *new,
                                        unsigned long bit,
                                        struct bitmap_struct *bm)
{
    bitmap_queue_add_first(new, bit, bm);
}

static inline int bitmap_empty(const struct bitmap_struct *bm)
{
    return !(!READ_ONCE(bm->map));
}

static inline int bitmap_queue_empty(struct bitmap_struct *bm,
                                     unsigned long bit)
{
    return !bitmap_get_bit(&bm->map, bit);
}

static inline int bitmap_first_bit(const struct bitmap_struct *bm)
{
    return find_first_bit(&bm->map, 32);
}

static inline void bitmap_queue_del(struct list_head *queue,
                                    unsigned long bit,
                                    struct bitmap_struct *bm)
{
    list_del(queue);
    if (list_empty(&bm->queue[bit]))
        bitmap_clear_bit(&bm->map, bit);
}

static inline struct list_head *bitmap_dequeue(struct bitmap_struct *bm,
                                               unsigned long bit)
{
    struct list_head *first = bm->queue[bit].next;

    bitmap_queue_del(first, bit, bm);
    return first;
}

static inline struct list_head *bitmap_dequeue_tail(struct bitmap_struct *bm,
                                                    unsigned long bit)
{
    struct list_head *last = bm->queue[bit].prev;

    bitmap_queue_del(last, bit, bm);
    return last;
}


struct sched {
    int (*init)(void);
    int (*enqueue)(task *tsk);
    int (*dequeue)(task *tsk);
    task *(*elect)(void *NOUSE);
};


#define bitmap_first_entry(bm, bit, type, member) \
    list_entry(bm->queue[bit].next, type, member)

#endif