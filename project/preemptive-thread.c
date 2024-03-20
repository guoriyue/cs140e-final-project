#include "rpi.h"
#include "timer-interrupt.h"
#include "preemptive-thread.h"
#include "full-except.h"

enum { stack_size = 8192 * 8 };

static rq_t runq;
static rq_t freeq;
static pre_th_t * volatile cur_thread;
static pre_th_t *scheduler_thread;
static regs_t start_regs;


static unsigned tid = 1;
static unsigned nalloced = 0;


enum { trace_p = 1};
#define th_trace(args...) do {                          \
    if(trace_p) {                                       \
        trace(args);                                   \
    }                                                   \
} while(0)



static void interrupt_handler(regs_t *r) {

    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();
    
    // pre_th_t *prev_th = cur_thread;
    
    th_trace("Switching from thread=%d to thread=%d\n", cur_thread->tid, scheduler_thread->tid);
    // printk("pc=%x\n", r->regs[REGS_PC]);
    uint32_t mode = spsr_get() & 0b11111;
    // printk("mode = %b\n", mode);
    // pc=0xb0cc
    // mode = 10000
    // eq_insert_with_priority pc=0xb0c8

    r->regs[REGS_PC] -= 4;
    cur_thread->regs = *r;
    // printk("eq_insert_with_priority pc=%x\n", cur_thread->regs.regs[REGS_PC]);
    eq_insert_with_priority(&runq, cur_thread);
    
    cur_thread = scheduler_thread;
    
    switchto(&scheduler_thread->regs);
    th_trace("Switching back to thread=%d\n", cur_thread->tid);
    // cswitch_from_exception(&scheduler_thread->regs);
}

void sys_equiv_exit(uint32_t ret);

// this is used to reinitilize registers.
static inline regs_t regs_init(pre_th_t *p) {
    // get our current cpsr and clear the carry and set the mode
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());

    // pre_init_trampoline
    void pre_init_trampoline(void (*c)(void *a), void *a);

    // XXX: which code had the partial save?  the ss rwset?
    regs_t regs = (regs_t) {
        .regs[0] = p->arg,
        .regs[REGS_PC] = p->fn,      // the stack pointer to use.
        .regs[REGS_SP] = p->stack_end,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)sys_equiv_exit,
        .regs[REGS_CPSR] = cpsr & (~0b10000000),            // the cpsr to use.
    };
    return regs;
}

pre_th_t *pre_fork(void (*fn)(void*), void *arg, uint32_t priority) {
    th_trace("forking a fn.\n");
    pre_th_t *th = pre_th_alloc();
    // kmalloc_aligned(stack_size, 8);

    th->fn = (uint32_t)fn;
    th->arg = (uint32_t)arg;

    th->stack_start = (uint32_t)th;
    th->stack_end = th->stack_start + stack_size;

    th->regs = regs_init(th);
    th->priority = priority;

    eq_insert_with_priority(&runq, th);
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
    // intr super mode
    // intr mode
    int first = 1;
    th_trace("scheduler is called\n");
    while (1) {
        if(eq_empty(&runq)) {
            th_trace("No more thread to switch to\n");
            return;
        }
        pre_th_t *th = eq_pop(&runq);
        th_trace("switching from tid=%d,pc=%x to tid=%d,pc=%x,sp=%x\n", 
                cur_thread->tid, 
                cur_thread->regs.regs[REGS_PC],
                th->tid,
                th->regs.regs[REGS_PC],
                th->regs.regs[REGS_SP]);
        // eq_append(&runq, cur_thread);
        cur_thread = th;
        th_trace("switching from scheduler to tid=%d\n", cur_thread->tid);
        // switchto(&cur_thread->regs);

        uart_flush_tx();
        // mismatch_run(&cur_thread->regs);
        while (!uart_can_put8())
            ;
        // switchto(&cur_thread->regs);
        switchto_cswitch(&scheduler_thread->regs, &cur_thread->regs);
        th_trace("switching back to scheduler\n");
        
    }
}

enum {
    PRE_EXIT = 0,
    PRE_PUTC = 1
};

static int pre_syscall_handler(regs_t *r) {
    th_trace("syscall handler is called.\n");
    let th = cur_thread;
    assert(th);
    th->regs = *r;  // update the registers

    // uart_flush_tx();

    // unsigned sysno = r->regs[0];
    // eq_append(&runq, cur_thread);
    // cur_thread->regs = *r;
    // exit
    cur_thread = scheduler_thread;
    switchto(&scheduler_thread->regs);
    return 0;
    // switch(sysno) {
    // case PRE_EXIT: 
    //     th_trace("thread=%d exited with code=%d\n", 
    //         th->tid, r->regs[1]);
    //     pre_th_t *th = eq_pop(&runq);

    //     if (!th) {
    //         th_trace("done with all threads\n");
    //         switchto(&scheduler_thread->regs);
    //     }

    //     cur_thread = th;
    //     while (!uart_can_put8())
    //         ;
    //     switchto(&cur_thread->regs);

    // default:
    //     panic("illegal system call\n");
    // }
    // // scheduler();
    // return 0;
}

void pre_run(void) {
    th_trace("run\n");
    // switch_to_sys_mode();

    // timer_interrupt_init(0x1000);
    // full_except_install(0);
    
    // // full_except_ints

    // // extern uint32_t pre_threads_ints[];
    // // void *v = pre_threads_ints;
    // // void *addr = vector_base_get();
    // // if(!addr)
    // //     vector_base_set(v);
    // // else if(addr != v)
    // //     panic("already have exception handlers installed: addr=%x\n", addr);

    // full_except_set_interrupt(interrupt_handler);
    // system_enable_interrupts();
    
    if(eq_empty(&runq)) {
        panic("run queue is empty.\n");
        return;
    }

    if(!scheduler_thread) {
        scheduler_thread = pre_th_alloc();
        cur_thread = scheduler_thread;
    }

    scheduler();

    
    // switchto_cswitch(&start_regs, &scheduler_thread->regs);
    th_trace("pre_run done with all threads\n");
}

// return pointer to the current thread.  
pre_th_t *pre_cur_thread(void) {
    return cur_thread;
}

void pre_yield(void) {
    // cpsr_int_disable();
    // uint32_t mode = spsr_get() & 0b11111;
    // th_trace("mode = %b\n", mode);
    // th_trace("thread=%d yielded\n", cur_thread->tid);
    // set current mode to user mode
    // cpsr_set(cpsr_inherit(USER_MODE, cpsr_get()) & (~0b10000000));
    // cur_thread->regs = *r;
    // printk("append eq pc=%x\n", cur_thread->regs.regs[REGS_PC]);
    // eq_append(&runq, cur_thread);
    eq_insert_with_priority(&runq, cur_thread);
    // switchto(&scheduler_thread->regs);
    // return;
    pre_th_t* old = cur_thread;
    cur_thread = eq_pop(&runq);
    // switchto(&cur_thread->regs);
    
    if (cur_thread == old) {
        return;
    }
    else {
        // printk("Switching from thread=%d to thread=%d,pc=%x\n", old->tid, cur_thread->tid, cur_thread->regs.regs[REGS_PC]);
        
        // 0000aa80 <uart_can_putc>:
        // aa80:	e92d4010 	push	{r4, lr}
        // aa84:	e59f0010 	ldr	r0, [pc, #16]	; aa9c <uart_can_putc+0x1c>
        // aa88:	ebfff567 	bl	802c <GET32>
        // aa8c:	e3100020 	tst	r0, #32, 0

        uart_flush_tx();
        // mismatch_run(&cur_thread->regs);
        while (!uart_can_put8())
            ;
        
        // // printk("pc=%x\n", cur_thread->regs.regs[REGS_PC]);
        // printk("old pc=%x\n", old->regs.regs[REGS_PC]);
        // printk("switch to pc=%x\n", cur_thread->regs.regs[REGS_PC]);
        // printk("mode_get(cpsr_get()) = %b\n", mode_get(cpsr_get()));
        // switchto_cswitch(&scheduler_thread->regs, &cur_thread->regs);
        switchto(&cur_thread->regs);
        // pre_cswitch(&old->regs, cur_thread->regs);
        // &old->regs, 
        // switchto(&cur_thread->regs);
        // pre_cswitch(&old->regs, &cur_thread->regs);
    }
}


// static int pre_syscall_handler(regs_t *r) {
//     th_trace("syscall: pc=%x\n", r->regs[REGS_PC]);
//     uint32_t mode;

//     mode = spsr_get() & 0b11111;

//     if(mode != USER_MODE && mode != SYS_MODE)
//         panic("mode = %b: expected %b\n", mode, USER_MODE);
//     else
//         th_trace("success: spsr is at user/sys level\n");
//     // dev_barrier();
//     // unsigned pending = GET32(IRQ_basic_pending);
//     // if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
//     //     return 0;
//     // PUT32(arm_timer_IRQClear, 1);
//     // dev_barrier();
    
//     // pre_th_t *prev_th = cur_thread;
//     // eq_append(&runq, cur_thread);
//     // th_trace("Switching from thread=%d to thread=%d\n", cur_thread->tid, scheduler_thread->tid);
//     // cur_thread->regs = *r;
    
//     // cur_thread = scheduler_thread;
    
//     // switchto(&scheduler_thread->regs);
//     return 0;
// }

void int_vec_init(void *v) {
    // if (!set_int_vector_ready()) {
    //     return;
    // }
    // turn off system interrupts
    cpsr_int_disable();

    //  BCM2835 manual, section 7.5 , 112
    dev_barrier();
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();

    vector_base_set(v);
}


void pre_init(void) {
    th_trace("init func.\n");
    kmalloc_init();
    // th_trace("init func.\n");
    // kmalloc_init();
    timer_interrupt_init(0x10000);
    // system_enable_interrupts();
    // full_except_install(0);


    extern uint32_t full_except_ints[];
    void *v = full_except_ints;
    int_vec_init(v);
    // void *addr = vector_base_get();
    // if(!addr)
    //     vector_base_set(v);
    // else if(addr != v)
    //     panic("already have exception handlers installed: addr=%x\n", addr);


    // handlers below
    // full_except_set_syscall(pre_syscall_handler);
    full_except_set_interrupt(interrupt_handler);
    full_except_set_syscall(pre_syscall_handler);

    // full_except_set_syscall(pre_syscall_handler);
}

void pre_exit(void) {
    // printk("thread=%d exited\n", cur_thread->tid);
    switchto(&scheduler_thread->regs);
}

void thread_set_priority(int new_priority) {
    cpsr_int_disable();
    int32_t old_priority = cur_thread->priority;
    cur_thread->priority = new_priority;
    // if (!eq_empty(&cur_thread->donor_threads)) {
    //     pre_th_t *highest_priority_donor_thread = eq_pop(&cur_thread->donor_threads);
    //     if (highest_priority_donor_thread->priority > new_priority) {
    //         cur_thread->priority = highest_priority_donor_thread->priority;
    //     }
    // }

    thread_donate_priority(cur_thread);

    if (old_priority > cur_thread->priority) {
        pre_yield();
    }
    cpsr_int_enable();
}

void thread_donate_priority(pre_th_t *t) {
    struct lock_t *l = t->wait_on_lock;
    if (l->holder->priority >= t->priority) {
        return;
    }
    l->holder->priority = t->priority;
    thread_donate_priority(l->holder);
}


void lock_init (struct lock_t *lock)
{
    lock->holder = NULL;
    lock->locked = 1;
}

void lock_acquire (struct lock_t *lock)
{
    cpsr_int_disable();
    if (lock->locked == 0) {
        lock->locked = 1;
        lock->holder = pre_cur_thread();
    } else {
        printk("thread %d is waiting for the lock\n", pre_cur_thread()->tid);
        eq_insert_with_priority(&lock->waiters, pre_cur_thread());
        pre_yield();
    }
    cpsr_int_enable();
}

void lock_release (struct lock_t *lock)
{
    cpsr_int_disable();
    if (eq_empty(&lock->waiters)) {
        lock->locked = 0;
        lock->holder = NULL;
    } else {
        pre_th_t *th = eq_pop(&lock->waiters);
        lock->holder = th;
    }
    cpsr_int_enable();
}