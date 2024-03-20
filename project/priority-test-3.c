#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

volatile uint32_t shared_var = 0;
struct lock_t l; // Declare the lock.

void task1() {
    lock_acquire(&l);
    thread_set_priority(4);
    for (int i = 0; i < 100; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        shared_var++;
        // lock_release(&l); // Release the lock
    }
    printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    lock_release(&l);
    // printk("shared_var = %d\n", shared_var);
}

void task2() {
    lock_acquire(&l);
    for (int i = 0; i < 100; ++i) {
        
        // Acquire the lock
        shared_var--;
        // lock_release(&l); // Release the lock
        // pre_yield();
    }
    // printk("shared_var = %d\n", shared_var);
    printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    lock_release(&l);
}

void task3() {
    lock_acquire(&l);
    for (int i = 0; i < 100; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        shared_var++;
        // lock_release(&l); // Release the lock
    }
    printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    lock_release(&l);
    // printk("shared_var = %d\n", shared_var);
}


// Example1:L(prio2),M(prio4),H(prio8) 
// - L holds lockl
// - M waits on l, L’s priority raised to L1 = max(M, L) = 4 
// - Then H waits on l,L’s priority raised to max(H,L1)=8

void notmain(void) {
    pre_init();
    lock_init(&l); // Initialize the lock.

    let th1 = pre_fork(task1, 0, 2);
    
    let th2 = pre_fork(task2, 0, 4);
    let th3 = pre_fork(task3, 0, 8);
    pre_run();

    // though th2 yields, it should be executed first because of the priority.
    // print waiters of the lock
    while (!eq_empty(&l.waiters)) {
        pre_th_t *th = eq_pop(&l.waiters);
        printk("th->tid = %d, priority = %d\n", th->tid, th->priority);
    }
    
    printk("shared_var = %d\n", shared_var);
    // assert(shared_var == 0);
    trace("stack passed!\n");

    clean_reboot();
}
