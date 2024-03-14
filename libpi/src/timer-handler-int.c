#include "rpi-interrupts.h"
static void (*timer_handler)(unsigned, unsigned);

void register_timer_handler(void (*func)(unsigned, unsigned)) {
    timer_handler = func;
}

void timer_interrupt_handler(unsigned pc, unsigned sp) {
    if (timer_handler == NULL)
        return;
    (*timer_handler)(pc, sp);
}