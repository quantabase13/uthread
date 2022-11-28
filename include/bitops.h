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

#ifndef KERNEL_BITOPS_H
#define KERNEL_BITOPS_H

#include <sys/param.h>

#define BITS_PER_CHAR 8
#define BITS_PER_LONG (BITS_PER_CHAR * sizeof(long))

static inline unsigned long flsl(unsigned long word)
{
    return word ? sizeof(long) * BITS_PER_CHAR - __builtin_clz(word) : 0;
}

static inline void clear_bit(unsigned long bit, unsigned long *word)
{
    *word &= ~(1 << bit);
}

static inline void set_bit(unsigned long bit, unsigned long *word)
{
    *word |= (1 << bit);
}

static inline void bitmap_set_bit(unsigned long *map, unsigned long bit)
{
    set_bit(bit % BITS_PER_LONG, &map[bit / BITS_PER_LONG]);
}

static inline void bitmap_clear_bit(unsigned long *map, unsigned long bit)
{
    clear_bit(bit % BITS_PER_LONG, &map[bit / BITS_PER_LONG]);
}

static inline unsigned long bitmap_get_bit(unsigned long *map,
                                           unsigned long bit)
{
    return (map[bit / BITS_PER_LONG] >> (bit % BITS_PER_LONG)) & 1;
}

static inline unsigned long find_first_bit(const unsigned long *addr,
                                           unsigned long size)
{
    for (unsigned long i = 0; i * BITS_PER_LONG < size; i++) {
        if (addr[i])
            return MIN(i * BITS_PER_LONG + __builtin_ffsl(addr[i]) - 1, size);
    }

    return size;
}

static inline unsigned long find_first_zero_bit(const unsigned long *addr,
                                                unsigned long size)
{
    for (unsigned long i = 0; i * BITS_PER_LONG < size; i++) {
        if (addr[i] != ~0ul)
            return MIN(i * BITS_PER_LONG + __builtin_ffsl(~addr[i]) - 1, size);
    }

    return size;
}

#endif /* !KERNEL_BITOPS_H */