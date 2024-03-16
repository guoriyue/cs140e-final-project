#include "rpi.h"
#include "mini-step.h"
#include "timer-interrupt.h"
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
#define trace(args...) do {                             \
    printk("TRACE:%s:", __FUNCTION__); printk(args);    \
} while(0)

static __attribute__((noreturn)) 
void schedule(void) 
{
    assert(cur_thread);

    th_t *th = eq_pop(&runq);
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

/******************************************************************
 * tiny syscall setup.
 */
int syscall_trampoline(int sysnum, ...);

enum {
    EQUIV_EXIT = 0,
    EQUIV_PUTC = 1
};

void sys_equiv_exit(uint32_t ret);

static int equiv_syscall_handler(regs_t *r) {
    trace("syscall handler is called.\n");
    let th = cur_thread;
    assert(th);
    th->regs = *r;  // update the registers

    uart_flush_tx();

    unsigned sysno = r->regs[0];
    switch(sysno) {
    case EQUIV_PUTC: 
        uart_put8(r->regs[1]);
        break;
    case EQUIV_EXIT: 
        trace("thread=%d exited with code=%d\n", 
            th->tid, r->regs[1]);
        th_t *th = eq_pop(&runq);

        if (!th) {
            trace("done with all threads\n");
            switchto(&start_regs);
        }

        cur_thread = th;
        while (!uart_can_put8())
            ;
        switchto(&cur_thread->regs);

    default:
        panic("illegal system call\n");
    }
    schedule();
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

    th_t *next_th = eq_pop(&runq);

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
static inline regs_t regs_init(th_t *p) {
    // get our current cpsr and clear the carry and set the mode
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());

    // XXX: which code had the partial save?  the ss rwset?
    regs_t regs = (regs_t) {
        .regs[0] = p->arg,
        .regs[REGS_PC] = p->fn,      // where we want to jump to
        .regs[REGS_SP] = p->stack_end,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)sys_equiv_exit, // where to jump if return.
        .regs[REGS_CPSR] = cpsr            // the cpsr to use.
    };
    return regs;
}

th_t *fork(void (*fn)(void*), void *arg) {
    trace("forking a fn.\n");
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

void run(void) {
    trace("run\n");
    cur_thread = eq_pop(&runq);
    if (!cur_thread)
        panic("run queue is empty.\n");
    system_enable_interrupts();
    switchto_cswitch(&start_regs, &cur_thread->regs);
}

void init(void) {
    trace("init func.\n");
    kmalloc_init();
    timer_interrupt_init(0x10);
    // system_enable_interrupts();
    full_except_install(0);
    // handlers below
    full_except_set_syscall(equiv_syscall_handler);
    full_except_set_interrupt(interrupt_handler);
}
