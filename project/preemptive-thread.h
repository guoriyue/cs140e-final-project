#ifndef __PREEMPTIVE_THREAD_H__
#define __EQUIV_THREAD_H__
#include "switchto.h"

typedef struct pre_th {
    regs_t regs;
    struct pre_th *next;
    uint32_t tid;

    uint32_t fn;
    uint32_t arg;

    uint32_t stack_start;
    uint32_t stack_end;

    uint32_t priority;

    struct lock_t* wait_on_lock;
} pre_th_t;


typedef struct rq {
    pre_th_t *head, *tail;
} rq_t;

#include "queue-ext-T.h"
gen_priority_queue_T(eq, rq_t, head, tail, pre_th_t, next)

struct lock_t {
    pre_th_t *holder; // the thread that holds the lock
    rq_t waiters;
    volatile int locked; // 0 if unlocked, 1 if locked
};

void lock_init (struct lock_t *lock);
void lock_acquire (struct lock_t *lock);
void lock_release (struct lock_t *lock);

typedef void (*fn_t)(void*);

void pre_init(void);

void pre_run(void);

pre_th_t *pre_fork(void (*fn)(void*), void *arg, uint32_t priority);

void pre_exit(void);

static pre_th_t *pre_th_alloc(void);

void switch_to_sys_mode();

pre_th_t *pre_cur_thread(void);

void pre_yield(void);

void thread_set_priority(int new_priority);
void thread_donate_priority(pre_th_t *th);

#endif
