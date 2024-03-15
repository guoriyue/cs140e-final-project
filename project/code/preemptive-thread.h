#ifndef __PREEMPTIVE_THREAD_H__
#define __EQUIV_THREAD_H__

#include "switchto.h"

typedef struct th {
    regs_t regs;
    struct th *next;
    uint32_t tid;
    uint32_t fn;
    uint32_t arg;
    uint32_t stack_start;
    uint32_t stack_end;
} th_t;

typedef void (*fn_t)(void*);

void init(void);

void run(void);

th_t *fork(void (*fn)(void*), void *arg);

#endif
