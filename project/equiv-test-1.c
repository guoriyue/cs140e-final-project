#include "rpi.h"
// #include "equiv-threads.h"
#include "preemptive-thread.h"

// lie about arg so we don't have to cast.
void nop_10(void *);
void nop_1(void *);
void mov_ident(void *);
void small1(void *);
void small2(void *);

static pre_th_t * run_single(int N, void (*fn)(void*), void *arg, uint32_t hash) {
    // let th = equiv_fork_nostack(fn, arg, hash);
    let th = pre_fork(fn, arg, 0, hash);
    // th->verbose_p = 1;
    // equiv_run();
    pre_run();
    trace("--------------done first run!-----------------\n");

    // turn off extra output
    // th->verbose_p = 0;
    for(int i = 0; i < N; i++) {
        equiv_refresh(th);
        // equiv_run();
        pre_run();
        trace("--------------done run=%d!-----------------\n", i);
    }
    return th;
}

void notmain(void) {
    // equiv_init();
    pre_init();


    // do the smallest ones first.
    let th1 = run_single(1, small1, 0, 0xf86a29e);
    let th2 = run_single(1, small2, 0, 0x4e8e6d76);
    equiv_refresh(th1);
    equiv_refresh(th2);
    // equiv_run();
    pre_run();
    trace("easy no-stack passed\n");

    // do a bunch of other ones
    let th_nop1 = run_single(0, nop_1, 0, 0x72969027);
    let th_mov_ident = run_single(0, mov_ident, 0, 0x448ce9eb);
    let th_nop10 = run_single(0, nop_10, 0, 0xb606796e);

    // now run all three 
    equiv_refresh(th_nop1);
    equiv_refresh(th_nop10);
    equiv_refresh(th_mov_ident);
    // equiv_run();
    pre_run();

    trace("second no-stack passed\n");

    equiv_refresh(th1);
    equiv_refresh(th2);
    equiv_refresh(th_nop1);
    equiv_refresh(th_nop10);
    equiv_refresh(th_mov_ident);
    // equiv_run();
    pre_run();

    trace("all no-stack passed\n");

    clean_reboot();
}
