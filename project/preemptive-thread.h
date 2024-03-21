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

    // if non-zero: the hash we expect to get 
    uint32_t expected_hash;

    // the current cumulative hash
    uint32_t reg_hash;
    uint32_t inst_cnt;
} pre_th_t;


typedef struct rq {
    pre_th_t *head, *tail;
} rq_t;

gen_priority_queue_T(eq, rq_t, head, tail, pre_th_t, next)


// part 1: pre-emptive threads with priority scheduling
typedef void (*fn_t)(void*);

void pre_init(void);

void pre_run(void);

pre_th_t *pre_fork(void (*fn)(void*), void *arg, uint32_t priority, uint32_t expected_hash);

void pre_exit(void);

static pre_th_t *pre_th_alloc(void);

void switch_to_sys_mode();

pre_th_t *pre_cur_thread(void);
void pre_yield(void);
void print_regs(regs_t *r);

void sys_exit(void); // need to be called by the thread when it exits, otherwise the thread will run forever
void sys_putc(uint8_t ch);
void equiv_refresh(pre_th_t *th);

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



typedef int spin_lock_t;

int try_lock(spin_lock_t *lock);

void spin_init(spin_lock_t * lock);



void spin_lock(spin_lock_t * lock);


void spin_unlock(spin_lock_t * lock);








// part 3: priority donation
void thread_set_priority(int new_priority);
void thread_donate_priority(pre_th_t *th);

#endif
