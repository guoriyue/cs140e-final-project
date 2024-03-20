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
} pre_th_t;


typedef void (*fn_t)(void*);

void pre_init(void);

void pre_run(void);

pre_th_t *pre_fork(void (*fn)(void*), void *arg, uint32_t priority);

void pre_exit(void);

static pre_th_t *pre_th_alloc(void);

// void switch_to_sys_mode();

pre_th_t *pre_cur_thread(void);

void pre_yield(void);

typedef volatile uint32_t lock_t; // Define lock_t as a volatile uint32_t for atomic access

// Corrected lock and unlock using your approach
static inline void lock(lock_t *l) {
    while(*l != 0) {
        pre_yield(); // Wait for lock to be 0
    }
    *l = 1; // Acquire the lock
}

static inline void unlock(lock_t *l) {
    *l = 0; // Release the lock
}

#endif
