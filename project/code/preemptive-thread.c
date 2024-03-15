#include "rpi.h"
#include "preemptive-thread.h"

enum { stack_size = 8192 * 8 };

typedef struct rq {
    th_t *head, *tail;
} rq_t;

#include "queue-ext-T.h"
gen_queue_T(eq, rq_t, head, tail, th_t, next)

static rq_t runq;
static th_t * volatile cur_thread;
static regs_t start_regs;

#undef trace
#define trace(args...) do {                                 \
    if(verbose_p) {                                         \
        printk("TRACE:%s:", __FUNCTION__); printk(args);    \
    }                                                       \
} while(0)

void sys_equiv_exit(uint32_t ret);

// this is used to reinitilize registers.
static inline regs_t regs_init(th_t *p) {
    // get our current cpsr and clear the carry and set the mode
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());

    // XXX: which code had the partial save?  the ss rwset?
    regs_t regs = (regs_t) {
        .regs[0] = p->arg,
        .regs[REGS_PC] = p->fn,      // where we want to jump to
        .regs[REGS_SP] = p->stack_end,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)sys_equiv_exit, // where to jump if return.
        .regs[REGS_CPSR] = cpsr             // the cpsr to use.
    };
    return regs;
}

th_t *fork(void (*fn)(void*), void *arg) {
    th_t *th = kmalloc_aligned(stack_size, 8);

    static unsigned ntids = 1;
    th->tid = ntids++;

    th->fn = (uint32_t)fn;
    th->arg = (uint32_t)arg;

    th->stack_start = (uint32_t)th;
    th->stack_end = th->stack_start + stack_size;

    th->regs = regs_init(th);

    eq_push(&runq, th);
    return th;
}
