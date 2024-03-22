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
    // while (1) {
    //     delay_ms(100);
    //     printk("hello from 1.\n");
    // }
}

void hello2(void *msg) {
    // printk("hello from 2.\n");
    // printk("hello from 2.\n");
    while (1) {
        delay_ms(100);
        printk("hello from 2.\n");
    }
}

void hello3(void *msg) {
    // printk("hello from 3.\n");
    // printk("hello from 3.\n");
    while (1) {
        delay_ms(100);
        printk("hello from 3.\n");
    }
}

// void msg(void *msg) {
//     // delay_ms(2000);
//     // equiv_puts(msg);
//     printk("%s", msg);
// }

static pre_th_t * run_single(int N, void (*fn)(void*), void *arg) {
    let th = pre_fork(fn, arg, 0, 0);
    return th;
}

void notmain(void) {
    pre_init();

    //equiv_puts("hello\n") ;   // do the smallest ones first.
    let th1 = run_single(0, hello1, 0);
    let th2 = run_single(0, hello2, 0);
    let th3 = run_single(0, hello3, 0);
    
    pre_run();
    trace("stack passed!\n");

    clean_reboot();
}
