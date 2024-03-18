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
static rq_t freeq;
static pre_th_t * volatile cur_thread;
static pre_th_t *scheduler_thread;
static regs_t start_regs;


static unsigned tid = 1;
static unsigned nalloced = 0;


#undef trace
#define trace(args...) do {                             \
    printk("TRACE:%s:", __FUNCTION__); printk(args);    \
} while(0)

// static __attribute__((noreturn)) 
// void schedule(void) 
// {
//     assert(cur_thread);

//     pre_th_t *th = eq_pop(&runq);
//     if(th) {
//         output("switching from tid=%d,pc=%x to tid=%d,pc=%x,sp=%x\n", 
//                 cur_thread->tid, 
//                 cur_thread->regs.regs[REGS_PC],
//                 th->tid,
//                 th->regs.regs[REGS_PC],
//                 th->regs.regs[REGS_SP]);
//         eq_append(&runq, cur_thread);
//         cur_thread = th;
//     }
//     uart_flush_tx();
//     // mismatch_run(&cur_thread->regs);
//     while (!uart_can_put8())
//         ;
//     switchto(&cur_thread->regs);
// }

static void interrupt_handler(regs_t *r) {

    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();
    
    pre_th_t *prev_th = cur_thread;
    eq_push(&runq, cur_thread);
    trace("Switching from thread=%d to thread=%d\n", cur_thread->tid, scheduler_thread->tid);
    cur_thread->regs = *r;
    
    cur_thread = scheduler_thread;
    
    switchto(&scheduler_thread->regs);
}

// this is used to reinitilize registers.
static inline regs_t regs_init(pre_th_t *p) {
    // get our current cpsr and clear the carry and set the mode
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());

    // pre_init_trampoline
    void pre_init_trampoline(void (*c)(void *a), void *a);

    // XXX: which code had the partial save?  the ss rwset?
    regs_t regs = (regs_t) {
        .regs[4] = p->arg,
        .regs[5] = p->fn,
        .regs[REGS_PC] = (uint32_t)pre_init_trampoline,      // where we want to jump to
        .regs[REGS_SP] = p->stack_end,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)pre_exit, // where to jump if return.
        .regs[REGS_CPSR] = cpsr,            // the cpsr to use.
    };
    return regs;
}

pre_th_t *pre_fork(void (*fn)(void*), void *arg) {
    trace("forking a fn.\n");
    pre_th_t *th = pre_th_alloc();
    // kmalloc_aligned(stack_size, 8);

    th->fn = (uint32_t)fn;
    th->arg = (uint32_t)arg;

    th->stack_start = (uint32_t)th;
    th->stack_end = th->stack_start + stack_size;

    th->regs = regs_init(th);

    eq_push(&runq, th);
    return th;
}

// keep a cache of freed thread blocks.  call kmalloc if run out.
static pre_th_t *pre_th_alloc(void) {
    pre_th_t *t = eq_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
// #   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
//     demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    return t;
}

void scheduler(void) {
    // sys mode
    trace("scheduler is called\n");
    while (1) {
        if(eq_empty(&runq)) {
            trace("No more thread to switch to\n");
            return;
        }
        pre_th_t *th = eq_pop(&runq);
        trace("switching from tid=%d,pc=%x to tid=%d,pc=%x,sp=%x\n", 
                cur_thread->tid, 
                cur_thread->regs.regs[REGS_PC],
                th->tid,
                th->regs.regs[REGS_PC],
                th->regs.regs[REGS_SP]);
        // eq_append(&runq, cur_thread);
        cur_thread = th;
        trace("switching from scheduler to tid=%d\n", cur_thread->tid);
        switchto(&cur_thread->regs);
        // switchto_cswitch(&scheduler_thread->regs, &cur_thread->regs);
    }
}
void pre_run(void) {
    trace("run\n");
    switch_to_sys_mode();

    timer_interrupt_init(0x10);
    full_except_install(0);
    // full_except_ints

    // extern uint32_t pre_threads_ints[];
    // void *v = pre_threads_ints;
    // void *addr = vector_base_get();
    // if(!addr)
    //     vector_base_set(v);
    // else if(addr != v)
    //     panic("already have exception handlers installed: addr=%x\n", addr);

    full_except_set_interrupt(interrupt_handler);
    // system_enable_interrupts();
    
    if(eq_empty(&runq)) {
        panic("run queue is empty.\n");
        return;
    }

    if(!scheduler_thread) {
        scheduler_thread = pre_th_alloc();
        scheduler_thread->fn = (uint32_t)scheduler;
        scheduler_thread->stack_start = (uint32_t)scheduler_thread;
        scheduler_thread->stack_end = scheduler_thread->stack_start + stack_size;
        scheduler_thread->regs = regs_init(scheduler_thread);
        cur_thread = scheduler_thread;
    }
    
    switchto_cswitch(&start_regs, &scheduler_thread->regs);
    // scheduler();
    trace("done with all threads\n");
}

enum {
    EQUIV_EXIT = 0,
    EQUIV_PUTC = 1
};


static int pre_syscall_handler(regs_t *r) {
    trace("syscall: pc=%x\n", r->regs[REGS_PC]);
    uint32_t mode;

    mode = spsr_get() & 0b11111;

    if(mode != USER_MODE && mode != SYS_MODE)
        panic("mode = %b: expected %b\n", mode, USER_MODE);
    else
        trace("success: spsr is at user/sys level\n");
    // dev_barrier();
    // unsigned pending = GET32(IRQ_basic_pending);
    // if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
    //     return 0;
    // PUT32(arm_timer_IRQClear, 1);
    // dev_barrier();
    
    // pre_th_t *prev_th = cur_thread;
    // eq_push(&runq, cur_thread);
    // trace("Switching from thread=%d to thread=%d\n", cur_thread->tid, scheduler_thread->tid);
    // cur_thread->regs = *r;
    
    // cur_thread = scheduler_thread;
    
    // switchto(&scheduler_thread->regs);
    return 0;
}

void pre_init(void) {
    trace("init func.\n");
    kmalloc_init();
    full_except_set_syscall(pre_syscall_handler);
}

void pre_exit(void) {
    printk("thread=%d exited\n", cur_thread->tid);
}