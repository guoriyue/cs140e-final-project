#include "rpi.h"
#include "preemptive-thread.h"

// lie about arg so we don't have to cast.
void nop_10(void *);
void nop_1(void *);
void mov_ident(void *);
void small1(void *);
void small2(void *);

void sys_equiv_putc(uint8_t ch);

void equiv_puts(char *msg) {
    for(; *msg; msg++)
        sys_equiv_putc(*msg);
}

void hello1(void *msg) {
    // delay_ms(2000);
    printk("hello from 1.\n");
}

void hello2(void *msg) {
    printk("hello from 2.\n");
    // equiv_puts("hello from 2\n");
    // sys_equiv_exit(0);
}

void hello3(void *msg) {
    printk("hello from 3.\n");
}

// void msg(void *msg) {
//     // delay_ms(2000);
//     // equiv_puts(msg);
//     printk("%s", msg);
// }

static th_t * run_single(int N, void (*fn)(void*), void *arg) {
    let th = fork(fn, arg);
    return th;
}

void notmain(void) {
    init();

    //equiv_puts("hello\n") ;   // do the smallest ones first.
    //let th1 = run_single(0, hello1, 0);
    let th2 = run_single(0, hello2, 0);
    //let th3 = run_single(0, hello3, 0);
    // th1->verbose_p = 0;
    // th2->verbose_p = 0;
    // th3->verbose_p = 0;

    // equiv_refresh(th1);
    // equiv_refresh(th2);
    // equiv_refresh(th3);
    // equiv_run();
    run();
    trace("stack passed!\n");

    clean_reboot();
}
