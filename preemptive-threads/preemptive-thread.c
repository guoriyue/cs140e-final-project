// engler, cs140e: starter code for trivial threads package.
#include "rpi.h"
// #include "rpi-thread.h"
#include "redzone.h"
#include "timer-interrupt.h"
#include "preemptive-thread.h"
#include "timer-handler-int.c"
// tracing code.  set <trace_p>=0 to stop tracing
enum { trace_p = 1};
#define th_trace(args...) do {                          \
    if(trace_p) {                                       \
        trace(args);                                   \
    }                                                   \
} while(0)

/***********************************************************************
 * datastructures used by the thread code.
 *
 * you don't have to modify this.
 */

#define E preemptive_thread_t
#include "libc/Q.h"

// currently only have a single run queue and a free queue.
// the run queue is FIFO.
static Q_t runq, freeq;
static preemptive_thread_t *cur_thread;        // current running thread.
static preemptive_thread_t *scheduler_thread;  // first scheduler thread.
unsigned sleep_until[128];
// monotonically increasing thread id: won't wrap before reboot :)
static unsigned tid = 1;

/***********************************************************************
 * simplistic pool of thread blocks: used to make alloc/free faster (plus,
 * our kmalloc doesn't have free (other than reboot).
 *
 * you don't have to modify this.
 */

// total number of thread blocks we have allocated.
static unsigned nalloced = 0;

// keep a cache of freed thread blocks.  call kmalloc if run out.
static preemptive_thread_t *th_alloc(void) {
    redzone_check(0);
    preemptive_thread_t *t = Q_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
#   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
    demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    return t;
}

static void th_free(preemptive_thread_t *th) {
    redzone_check(0);
    // push on the front in case helps with caching.
    Q_push(&freeq, th);
}

static int Q_in(Q_t *q, E *e) {
    for(E *x = q->head; x; x = x->next) {
        printk("x->tid=%d, e->tid=%d\n", x->tid, e->tid);
        if(x->tid == e->tid)
            return 1;
    }
    return 0;
}

static void Q_delete(Q_t *q, E *e) {
    printk("delete thread %d\n", e->tid);
    if(!q->head) {
        assert(!q->tail);
        return;
    }
    if(q->head == e) {
        q->head = e->next;
        if(!q->head)
            q->tail = 0;
        return;
    }
    for(E *x = q->head; x; x = x->next) {
        if(x->next == e) {
            x->next = e->next;
            if(!x->next)
                q->tail = x;
            return;
        }
    }
    panic("shouldn't get here\n");
}

/***********************************************************************
 * implement the code below.
 */

enum {
    R4_OFFSET = 0,
    R5_OFFSET = 1,
    R6_OFFSET = 2,
    R7_OFFSET = 3,
    R8_OFFSET = 4,
    R9_OFFSET = 5,
    R10_OFFSET = 6,
    R11_OFFSET = 7,
    // R14_OFFSET = 8,
    LR_OFFSET = 8
};

// // return pointer to the current thread.  
// preemptive_thread_t *preemptive_cur_thread(void) {
//     return cur_thread;
// }

// create a new thread.
preemptive_thread_t *preemptive_fork(void (*code)(void *arg), void *arg) {
    redzone_check(0);
    preemptive_thread_t *t = th_alloc();

    // write this so that it calls code,arg.
    void preemptive_init_trampoline(void);
    // void preemptive_init_trampoline(void (*code)(void *arg), void *arg, void *func_addr);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of preemptive_init_trampoline into the lr
     *     position so context switching will jump there.
     */
    // todo("initialize thread stack");
    
    t->saved_sp = (uint32_t*) t->stack;
    t->saved_sp += THREAD_MAXSTACK;
    // total offset 36
    t->saved_sp -= 9;
    t->saved_sp[R4_OFFSET] = (uint32_t) arg;
    t->saved_sp[R5_OFFSET] = (uint32_t) code;
    t->saved_sp[LR_OFFSET] = (uint32_t) preemptive_init_trampoline;
    // t->saved_sp[PC_OFFSET] = (uint32_t)(&preemptive_init_trampoline);


    th_trace("preemptive_fork: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    Q_append(&runq, t);
    return t;
}


// // exit current thread.
// //   - if no more threads, switch to the scheduler.
// //   - otherwise context switch to the new thread.
// //     make sure to set cur_thread correctly!
// void preemptive_exit(int exitcode) {
//     redzone_check(0);

//     // when you switch back to the scheduler thread:
//     //      th_trace("done running threads, back to scheduler\n");
//     // todo("implement preemptive_exit");
//     preemptive_thread_t* free_thread = cur_thread;
//     Q_append(&freeq, cur_thread);

//     cur_thread = Q_pop(&runq);
//     if (cur_thread == NULL) {
//         th_trace("done running threads, back to scheduler\n");
//         preemptive_cswitch(&free_thread->saved_sp, scheduler_thread->saved_sp);
//     }
//     else {
//         // preemptive_yield();
//         preemptive_cswitch(&free_thread->saved_sp, cur_thread->saved_sp);
//     }
//     // should never return.
//     not_reached();
// }

// // yield the current thread.
// //   - if the runq is empty, return.
// //   - otherwise: 
// //      * add the current thread to the back 
// //        of the runq (Q_append)
// //      * context switch to the new thread.
// //        make sure to set cur_thread correctly!
// void preemptive_yield(void) {
//     redzone_check(0);
//     // if you switch, print the statement:
//     //     th_trace("switching from tid=%d to tid=%d\n", old->tid, t->tid);

//     // todo("implement the rest of preemptive_yield");
//     Q_append(&runq, cur_thread);
//     preemptive_thread_t* old = cur_thread;
//     cur_thread = Q_pop(&runq);
//     // printk("trying to switch from tid=%d to tid=%d\n", old->tid, cur_thread->tid);
//     if (cur_thread == old) {
//         return;
//     }
//     else {
//         th_trace("switching from tid=%d to tid=%d\n", old->tid, cur_thread->tid);
//         preemptive_cswitch(&old->saved_sp, cur_thread->saved_sp);
//     }
// }

// /*
//  * starts the thread system.  
//  * note: our caller is not a thread!  so you have to 
//  * create a fake thread (assign it to scheduler_thread)
//  * so that context switching works correctly.   your code
//  * should work even if the runq is empty.
//  */
// void preemptive_thread_start(int preemptive_t) {
//     redzone_init();
//     th_trace("starting threads!\n");

//     // no other runnable thread: return.
//     if(Q_empty(&runq))
//         goto end;

//     // setup scheduler thread block.
//     if(!scheduler_thread)
//         scheduler_thread = th_alloc();

//     // todo("implement the rest of preemptive_thread_start");
//     cur_thread = Q_pop(&runq);
//     if(cur_thread) {
//         preemptive_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);
//     }

// end:
//     redzone_check(0);
//     // if not more threads should print:
//     th_trace("done with all threads, returning\n");
// }


// client has to define this.
void interrupt_preemptive_threads(unsigned pc, unsigned sp) {
    // need sp because we need to do context switch.
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);

    // if this isn't true, could be a GPU interrupt (as discussed in Broadcom):
    // just return.  [confusing, since we didn't enable!]
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;

    // Checkoff: add a check to make sure we have a timer interrupt
    // use p 113,114 of broadcom.

    /* 
     * Clear the ARM Timer interrupt - it's the only interrupt we have
     * enabled, so we don't have to work out which interrupt source
     * caused us to interrupt 
     *
     * Q: what happens, exactly, if we delete?
     * Infinite interrupts and loop.
     */
    PUT32(arm_timer_IRQClear, 1);

    /*
     * We do not know what the client code was doing: if it was touching a 
     * different device, then the broadcom doc states we need to have a
     * memory barrier.   NOTE: we have largely been living in sin and completely
     * ignoring this requirement for UART.   (And also the GPIO overall.)  
     * This is probably not a good idea and we should be more careful.
     */
    dev_barrier();    
    printk("IN TIMER INTERRUPT lr is %x %d\n", ((uint32_t *)sp)[8], ((uint32_t *)sp)[8]);
    printk("IN TIMER INTERRUPT pc is %x %d\n", pc, pc);

    Q_append(&runq, cur_thread);
    // cur_thread->saved_sp = (uint32_t *)sp;
    // cur_thread = scheduler_thread;
    // must be incorrect
    preemptive_cswitch(scheduler_thread->saved_sp, cur_thread->saved_sp);
}

void preemptive_thread_start(int preemptive_t) {
    redzone_init();
    th_trace("starting threads!\n");

    // define this in assembly
    extern uint32_t interrupt_vector_preemptive_threads[];
    int_vec_init((void *)interrupt_vector_preemptive_threads);

    timer_interrupt_init(0x100);
    register_timer_handler(interrupt_preemptive_threads);

    th_trace("starting threads!\n");
    
    if(Q_empty(&runq))
        goto end;

    // setup scheduler thread block.
    if(!scheduler_thread) {
        scheduler_thread = th_alloc();
        cur_thread = scheduler_thread;
    }

    // should do a lot of scheduling here?

end:
    redzone_check(0);
    // if not more threads should print:
    th_trace("done with all threads, returning\n");
}


// helper routine: can call from assembly with r0=sp and it
// will print the stack out.  it then exits.
// call this if you can't figure out what is going on in your
// assembly.
void preemptive_print_regs(uint32_t *sp) {
    // use this to check that your offsets are correct.
    printk("cur-thread=%d\n", cur_thread->tid);
    printk("sp=%p\n", sp);
    
    // stack pointer better be between these.
    printk("stack=%p\n", &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp < &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp >= &cur_thread->stack[0]);
    for(unsigned i = 0; i < 9; i++) {
        unsigned r = i == 8 ? 14 : i + 4;
        printk("sp[%d]=r%d=%x\n", i, r, sp[i]);
    }
    clean_reboot();
}

// void preemptive_sleep(unsigned t) {
//     unsigned clk = timer_get_usec();
//     sleep_until[cur_thread->tid] = clk + t;
//     printk("preemptive_sleep thread %d sleep until %d\n", cur_thread->tid, sleep_until[cur_thread->tid]);
//     preemptive_yield();
// }