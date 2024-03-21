#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"

static uint32_t shared_var = 0;
// struct lock l; // Declare the lock.
int l = 0;

void put_message(char* msg, uint32_t num){
    for (int i = 0; i < strlen(msg); i++){
        sys_putc(msg[i]);
    }
    if(num == 0){
        sys_putc('0');
        return;
    }
    char str[10];
    int i = 0;
    while(num > 0){
        str[i] = num % 10 + '0';
        num = num / 10;
        i++;
    }
    for(int j = i - 1; j >= 0; j--){
        sys_putc(str[j]);
    }
}

void task1() {
    // lock_acquire(&l);
    // thread_set_priority(4);
    int p = 0;
    dmb();
    dev_barrier();
    cpsr_int_disable();

    //  BCM2835 manual, section 7.5 , 112
    dev_barrier();
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();
    // cpsr_int_disable();
    for (int i = 0; i < 10000; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        // lock_(&l);
        
        shared_var++;
        p++;

        // put_message("1", shared_var);
        // unlock_(&l);
        
        printk("thread 1 shared_var = %d\n", shared_var);
        // lock_release(&l); // Release the lock
    }
    cpsr_int_enable();
    // printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    put_message("task 111111111111 p = ", p);
    // printk("task 1 p = %d\n", p);
}

void task2() {
    // lock_acquire(&l);
    int p = 0;
    dmb();
    dev_barrier();
    // cpsr_int_disable();
    cpsr_int_disable();

    //  BCM2835 manual, section 7.5 , 112
    dev_barrier();
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();
    for (int i = 0; i < 10000; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        // lock_(&l);
        
        shared_var--;
        p++;
        // put_message("2", shared_var);
        // unlock_(&l);
        
        printk("thread 2 shared_var = %d\n", shared_var);
        // lock_release(&l); // Release the lock
    }
    cpsr_int_enable();
    // printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    // printk("task 2 p = %d\n", p);
    put_message("task 2222222222 p = ", p);
    // printk("shared_var = %d\n", shared_var);
    // printk("task 2 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    // sys_exit();
}


// Example1:L(prio2),M(prio4),H(prio8) 
// - L holds lockl
// - M waits on l, L’s priority raised to L1 = max(M, L) = 4 
// - Then H waits on l,L’s priority raised to max(H,L1)=8

void notmain(void) {
    pre_init();
    // no
    // shared_var = 99
    // 
    // lock_init(&l); // Initialize the lock.

    let th1 = pre_fork(task1, 0, 2);
    
    let th2 = pre_fork(task2, 0, 2);
    // let th3 = pre_fork(task3, 0, 2);
    pre_run();

    // though th2 yields, it should be executed first because of the priority.
    // print waiters of the lock
    
    printk("shared_var = %d\n", shared_var);
    assert(shared_var == 0);
    trace("stack passed!\n");

    clean_reboot();
}
