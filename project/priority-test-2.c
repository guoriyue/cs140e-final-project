#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

// lie about arg so we don't have to cast.
void nop_10(void *);
void nop_1(void *);
void mov_ident(void *);
void small1(void *);
void small2(void *);
volatile lock_t l = 0; // Global lock variable

volatile uint32_t shared_var = 0;
void task1() {
    for (int i = 0; i < 100; ++i) {
        printk("before 1 lock=%d\n", l);
        lock(&l);   // Acquire the lock
        shared_var++;
        // printk("Task 1 lock=%d\n", l);
        unlock(&l); // Release the lock
        printk("after 1 lock=%d\n", l);
    }
}

void task2() {
    for (int i = 0; i < 100; ++i) {
        printk("before 2 lock=%d\n", l);
        lock(&l);   // Acquire the lock
        shared_var++;
        // printk("Task 2 lock=%d\n", l);
        unlock(&l); // Release the lock
        printk("after 2 lock=%d\n", l);
    }
}

void notmain(void) {
    pre_init();

    let th1 = pre_fork(task1, 0, 3);
    let th2 = pre_fork(task2, 0, 3);
    
    pre_run();
    printk("shared_var = %d\n", shared_var);
    assert(shared_var == 200);
    trace("stack passed!\n");

    clean_reboot();
}
