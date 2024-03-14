// engler,cs140e: trivial non-pre-emptive threads package.
#ifndef __RPI_THREAD_H__
#define __RPI_THREAD_H__

/*
 * trivial thread descriptor:
 *   - reg_save_area: space for all the registers (including
 *     the spsr).
 *   - <next>: pointer to the next thread in the queue that
 *     this thread is on.
 *  - <tid> unique thread id.
 *  - <stack>: fixed size stack: must be 8-byte aligned.  
 *
 * the big simplication: rather than save registers on the 
 * stack we stick them in a single fixed-size location at
 * offset in the thread structure.  
 *  - offset 0 means it's hard to mess up offset calcs.
 *  - doing in one place rather than making space on the 
 *    stack makes it much easier to think about.
 *  - the downside is that you can't do nested, pre-emptive
 *    context switches.  since this is not pre-emptive, we
 *    defer until later.
 *
 * changes:
 *  - dynamically sized stack.
 *  - save registers on stack.
 *  - add condition variables or watch.
 *  - some notion of real-time.
 *  - a private thread heap.
 *  - add error checking: thread runs too long, blows out its 
 *    stack.  
 */

// you should define these; also rename to something better.
#define REG_SP_OFF 36/4
#define REG_LR_OFF 40/4

#define THREAD_MAXSTACK (1024 * 8/4)

enum { 
    THREAD_READY, 
    THREAD_RUNNING, 
    THREAD_SLEEPING, 
    THREAD_EXITED 
};

typedef struct preemptive_thread {
    // SUGGESTION:
    //     check that this is within <stack> (see last field)
    //     should never point outside.
    uint32_t *saved_sp;

	struct preemptive_thread *next;
	uint32_t tid;

    // only used for part1: useful for testing without cswitch
    void (*fn)(void *arg);
    void *arg;          // this can serve as private data.
    
    const char *annot;
    // threads waiting on the current one to exit.
    // struct preemptive_thread *waiters;

	uint32_t stack[THREAD_MAXSTACK];

    // the state of the thread.
    int state;
    // the priority of the thread.
    int priority;
} preemptive_thread_t;

typedef struct preemptive_thread_mutex {
    preemptive_thread_t *owner;
    int locked;
} preemptive_thread_mutex;

_Static_assert(offsetof(preemptive_thread_t, stack) % 8 == 0, 
                            "must be 8 byte aligned");

// statically check that the register save area is at offset 0.
_Static_assert(offsetof(preemptive_thread_t, saved_sp) == 0, 
                "stack save area must be at offset 0");

// main routines.



// starts the thread system: only returns when there are
// no more runnable threads. 
void preemptive_thread_start(int preemptive_t);

// get the pointer to the current thread.  
preemptive_thread_t *preemptive_cur_thread(void);


// create a new thread that takes a single argument.
typedef void (*preemptive_code_t)(void *);

preemptive_thread_t *preemptive_fork(preemptive_code_t code, void *arg);

// exit current thread: switch to the next runnable
// thread, or exit the threads package.
void preemptive_exit(int exitcode);

// yield the current thread.
void preemptive_yield(void);

void preemptive_sleep(unsigned t);


/***************************************************************
 * internal routines: we put them here so you don't have to look
 * for the prototype.
 */

// internal routine: 
//  - save the current register values into <old_save_area>
//  - load the values in <new_save_area> into the registers
//  reutrn to the caller (which will now be different!)
void preemptive_cswitch(uint32_t **old_sp_save, const uint32_t *new_sp);

// returns the stack pointer (used for checking).
const uint8_t *preemptive_get_sp(void);

// check that: the current thread's sp is within its stack.
void preemptive_stack_check(void);

// do some internal consistency checks --- used for testing.
void preemptive_internal_check(void);

#if 0
void preemptive_wait(preemptive_cond_t *c, lock_t *l);

// assume non-preemptive: if you share with interrupt
// will have to modify.
void preemptive_lock(lock_t *l);
void preemptive_unlock(lock_t *l);

static inline void lock(lock_t *l) {
    while(get32(l)) != 0)
        preemptive_yield();
}
static inline void unlock(lock_t *l) {
    *l = 0;
}
#endif

// preemptive_thread helpers
static inline void *preemptive_arg_get(preemptive_thread_t *t) {
    return t->arg;
}
static inline void preemptive_arg_put(preemptive_thread_t *t, void *arg) {
    t->arg = arg;
}
static inline unsigned preemptive_tid(void) {
    preemptive_thread_t *t = preemptive_cur_thread();
    if(!t)
        panic("preemptive_threads not running\n");
    return t->tid;
}


#endif
