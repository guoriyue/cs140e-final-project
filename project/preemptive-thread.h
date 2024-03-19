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
} pre_th_t;

typedef void (*fn_t)(void*);

void pre_init(void);

void pre_run(void);

pre_th_t *pre_fork(void (*fn)(void*), void *arg);

void pre_exit(void);

static pre_th_t *pre_th_alloc(void);

void switch_to_sys_mode();

void cswitch_from_scheduler(uint32_t **old_reg_save, uint32_t *new_reg);
#endif
