#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

// lie about arg so we don't have to cast.
void nop_10(void *);
void nop_1(void *);
void mov_ident(void *);
void small1(void *);
void small2(void *);


static unsigned thread_count, thread_sum;

// trivial first thread: does not block, explicitly calls exit.
static void thread_code(void *arg) {
    unsigned *x = arg;

    // check tid
    unsigned tid = pre_cur_thread()->tid;
	trace("in thread tid=%d, with x=%d\n", tid, *x);
    // debug("in thread tid=%d, with x=%d\n", tid, *x);
    demand(pre_cur_thread()->tid == *x+1, 
                "expected %d, have %d\n", tid,*x+1);

    // check yield.
    // pre_yield();
	thread_count ++;
    // pre_yield();
	thread_sum += *x;
    // pre_yield();
    // check exit
    // pre_exit(0);
    trace("thread %d exiting\n", tid);
}

void notmain() {
    pre_init();

    // change this to increase the number of threads.
	int n = 30;
    trace("about to test summing of n=%d threads\n", n);

	thread_sum = thread_count = 0;

    unsigned sum = 0;
	for(int i = 0; i < n; i++)  {
        int *x = kmalloc(sizeof *x);
        sum += *x = i;
		pre_fork(thread_code, x, i, 0);
    }
	pre_run();

	// no more threads: check.
	trace("count = %d, sum=%d\n", thread_count, thread_sum);
	assert(thread_count == n);
	assert(thread_sum == sum);
    trace("SUCCESS!\n");
}
