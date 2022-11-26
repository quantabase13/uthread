#include <sys/types.h>
#include "linux/list.h"

#define RAW_SPIN_LOCK_UNLOCKED    {0}

#define SPIN_LOCK_INITIALIZER(lockname)	\
{						\
	.lock = RAW_SPIN_LOCK_UNLOCKED,	\
}

#define SPIN_LOCK_UNLOCKED(lockname)	\
	(spinlock_t) SPIN_LOCK_INITIALIZER(lockname)

#define SEMAPHORE_INITIALIZER(name, n)				\
{									\
	.lock		= SPIN_LOCK_UNLOCKED((name).lock),	\
	.count		= n,						\
	.wait_list	= LIST_HEAD_INIT((name).wait_list),		\
}

typedef struct spinlock{
    volatile uint lock;
} spinlock_t;

typedef struct semaphore {
    spinlock_t lock;
    unsigned int count;
    struct list_head wait_list;
} sem_t;