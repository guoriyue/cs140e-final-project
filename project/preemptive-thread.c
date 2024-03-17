#include "rpi.h"
#include "timer-interrupt.h"
#include "preemptive-thread.h"
#include "full-except.h"
enum { stack_size = 8192 * 8 };

typedef struct rq {
    pre_th_t *head, *tail;
} rq_t;

#include "queue-ext-T.h"
gen_queue_T(eq, rq_t, head, tail, pre_th_t, next)

static rq_t runq;
static pre_th_t * volatile cur_thread;
static regs_t start_regs;

#undef trace
#define trace(args...) do {                             \
    printk("TRACE:%s:", __FUNCTION__); printk(args);    \
} while(0)

static __attribute__((noreturn)) 
void schedule(void) 
{
    assert(cur_thread);

    pre_th_t *th = eq_pop(&runq);
    if(th) {
        output("switching from tid=%d,pc=%x to tid=%d,pc=%x,sp=%x\n", 
                cur_thread->tid, 
                cur_thread->regs.regs[REGS_PC],
                th->tid,
                th->regs.regs[REGS_PC],
                th->regs.regs[REGS_SP]);
        eq_append(&runq, cur_thread);
        cur_thread = th;
    }
    uart_flush_tx();
    // mismatch_run(&cur_thread->regs);
    while (!uart_can_put8())
        ;
    switchto(&cur_thread->regs);
}

static void interrupt_handler(regs_t *r) {
    trace("interrupt handler is called.\n");

    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    // PUT32(arm_timer_IRQClear, 1);
    dev_barrier();

    let th = cur_thread;
    assert(th);
    th->regs = *r;  // update the registers

    pre_th_t *next_th = eq_pop(&runq);

    if (!next_th) {
        trace("No more thread to switch to\n");
        return;
    }

    trace("Switching from thread=%d to thread=%d\n", cur_thread->tid, next_th->tid);
    eq_push(&runq, th);
    cur_thread = next_th;
    switchto(&cur_thread->regs);

}

// this is used to reinitilize registers.
static inline regs_t regs_init(pre_th_t *p) {
    // get our current cpsr and clear the carry and set the mode
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());

    void pre_trampoline(void (*c)(void *a), void *a);

    // XXX: which code had the partial save?  the ss rwset?
    regs_t regs = (regs_t) {
        // .regs[0] = p->arg,
        .regs[4] = (uint32_t)(p->arg),
        .regs[5] = (uint32_t)(p->fn),
        .regs[REGS_PC] = (uint32_t)&pre_trampoline,      // where we want to jump to
        .regs[REGS_SP] = p->stack_end,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)&pre_trampoline, // where to jump if return.
        .regs[REGS_CPSR] = cpsr            // the cpsr to use.
    };
    return regs;
}

pre_th_t *pre_fork(void (*fn)(void*), void *arg) {
    trace("forking a fn.\n");
    pre_th_t *th = kmalloc_aligned(stack_size, 8);

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

void pre_run(void) {
    trace("run\n");
    cur_thread = eq_pop(&runq);
    if (!cur_thread)
        panic("run queue is empty.\n");
    system_enable_interrupts();
    switchto_cswitch(&start_regs, &cur_thread->regs);
}

void pre_init(void) {
    trace("init func.\n");
    kmalloc_init();
    timer_interrupt_init(0x10);
    full_except_install(0);
    full_except_set_interrupt(interrupt_handler);
}

void pre_exit(void) {
    printk("thread=%d exited", cur_thread->tid);
    pre_th_t *th = eq_pop(&runq);
    if (!th) {
        trace("done with all threads\n");
        switchto(&start_regs);
    }
    cur_thread = th;
    switchto(&cur_thread->regs);
}