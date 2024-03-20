#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

volatile uint32_t shared_var = 0;
struct lock_t l; // Declare the lock.

void task1() {
    for (int i = 0; i < 100; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        shared_var++;
        // lock_release(&l); // Release the lock
    }
    printk("shared_var = %d\n", shared_var);
}

void task2() {
    for (int i = 0; i < 100; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        shared_var--;
        // lock_release(&l); // Release the lock
        // pre_yield();
    }
    printk("shared_var = %d\n", shared_var);
}


// Example1:L(prio2),M(prio4),H(prio8) 
// - L holds lockl
// - M waits on l, L’s priority raised to L1 = max(M, L) = 4 
// - Then H waits on l,L’s priority raised to max(H,L1)=8

void notmain(void) {
    pre_init();
    lock_init(&l); // Initialize the lock.

    let th1 = pre_fork(task1, 0, 3);
    let th2 = pre_fork(task2, 0, 4);
    // though th2 yields, it should be executed first because of the priority.
    
    pre_run();
    printk("shared_var = %d\n", shared_var);
    assert(shared_var == 0);
    trace("stack passed!\n");

    clean_reboot();
}
