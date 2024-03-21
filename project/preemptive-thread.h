#ifndef __PREEMPTIVE_THREAD_H__
#define __EQUIV_THREAD_H__
#include "switchto.h"
#include "queue-ext-T.h"

typedef struct pre_th {
    regs_t regs;
    struct pre_th *next;
    uint32_t tid;

    uint32_t fn;
    uint32_t arg;

    uint32_t stack_start;
    uint32_t stack_end;

    uint32_t priority;

    struct lock_t* wait_on_lock; // the lock that the thread is waiting on
} pre_th_t;


typedef struct rq {
    pre_th_t *head, *tail;
} rq_t;

gen_priority_queue_T(eq, rq_t, head, tail, pre_th_t, next)


// part 1: pre-emptive threads with priority scheduling
typedef void (*fn_t)(void*);

void pre_init(void);

void pre_run(void);

pre_th_t *pre_fork(void (*fn)(void*), void *arg, uint32_t priority);

void pre_exit(void);

static pre_th_t *pre_th_alloc(void);

void switch_to_sys_mode();

pre_th_t *pre_cur_thread(void);
void pre_yield(void);
void print_regs(regs_t *r);

void sys_exit(void); // need to be called by the thread when it exits, otherwise the thread will run forever
void sys_putc(uint8_t ch);


// part 2: locks and semaphores
// void system_enable_fiq(void);
// void system_disable_fiq(void);


static inline uint32_t system_enable_fiq(void) {
    uint32_t cpsr = cpsr_get();
    cpsr_set(cpsr & ~(1<<6));
    return cpsr;
}

static inline uint32_t system_disable_fiq(void) {
    uint32_t cpsr = cpsr_get();
    cpsr_set(cpsr | (1<<6));
    return cpsr;
}


int cas(int *ptr, int oldvalue, int newvalue);

void spin_lock(volatile int* lock);
void spin_unlock(volatile int* lock);

void lock_(int *mutex);
void unlock_(int *mutex);


struct semaphore 
{
    int32_t value;             /* Current value. */
    rq_t waiters;        /* List of waiting threads. */
};

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
int sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);



struct lock {
    pre_th_t *holder; // the thread that holds the lock
    struct semaphore semaphore;
};

void lock_init (struct lock *l);
void lock_acquire (struct lock *l);
void lock_release (struct lock *l);
int lock_try_acquire (struct lock *l);
int lock_held_by_current_thread (const struct lock *l);




// part 3: priority donation
void thread_set_priority(int new_priority);
void thread_donate_priority(pre_th_t *th);

#endif
