#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

// lie about arg so we don't have to cast.
void nop_10(void *);
void nop_1(void *);
void mov_ident(void *);
void small1(void *);
void small2(void *);

void hello1(void *msg) {
    printk("hello from 1.\n");
    printk("hello from 1.\n");
}
void hello2_child(void *msg) {
    printk("hello from 2 child.\n");
    printk("hello from 2 child.\n");
}

void hello2(void *msg) {
    let th = pre_fork(hello2_child, 0, 4);
    printk("hello from 2.\n");
    printk("hello from 2.\n");
}

void hello3_child(void *msg) {
    printk("hello from 3 child.\n");
    printk("hello from 3 child.\n");
}
void hello3(void *msg) {
    printk("hello from 3.\n");
    printk("hello from 3.\n");
    let th = pre_fork(hello3_child, 0, 0);
}

static pre_th_t * run_single(void (*fn)(void*), void *arg, uint32_t pri) {
    let th = pre_fork(fn, arg, pri);
    return th;
}

void notmain(void) {
    pre_init();

    let th1 = run_single(hello1, 0, 3);
    let th2 = run_single(hello2, 0, 2);
    let th3 = run_single(hello3, 0, 1);
    
    pre_run();
    trace("stack passed!\n");

    clean_reboot();
}
