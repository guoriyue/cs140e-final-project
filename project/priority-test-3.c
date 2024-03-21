#include "rpi.h"
#include "preemptive-thread.h"
#include "timer-interrupt.h"
#include "mmu.h"
static int shared_var = 0;
// volatile int lock = 0; // Shared lock variable
spin_lock_t lock;
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
    // dmb();
    // dev_barrier();
    // cpsr_int_disable();

    // //  BCM2835 manual, section 7.5 , 112
    // dev_barrier();
    // PUT32(Disable_IRQs_1, 0xffffffff);
    // PUT32(Disable_IRQs_2, 0xffffffff);
    // dev_barrier();
    // uint32_t prev_intr_state = set_all_interrupts_off();

    // system_disable_fiq();
    // cpsr_int_disable();
    for (int i = 0; i < 1000000; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        // lock_(&l);
        spin_lock(&lock);
        shared_var++;
        p++;
        spin_unlock(&lock);

        // put_message("1", shared_var);
        // unlock_(&l);
        
        // printk("thread 1 shared_var = %d\n", shared_var);
        // lock_release(&l); // Release the lock
    }
    // put_message("task 111111111111 p = ", p);
    // cpsr_int_enable();
    // set_all_interrupts(prev_intr_state);
    // system_enable_fiq();
    // sys_exit();
    // printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    // 
    // printk("task 1 p = %d\n", p);
}

void task2() {
    // lock_acquire(&l);
    int p = 0;
    // dmb();
    // dev_barrier();
    // cpsr_int_disable();
    // cpsr_int_disable();

    // //  BCM2835 manual, section 7.5 , 112
    // dev_barrier();
    // PUT32(Disable_IRQs_1, 0xffffffff);
    // PUT32(Disable_IRQs_2, 0xffffffff);
    // dev_barrier();
    // uint32_t prev_intr_state = set_all_interrupts_off();
    // system_disable_fiq();
    for (int i = 0; i < 1000000; ++i) {
        // lock_acquire(&l);   // Acquire the lock
        // lock_(&l);
        
        spin_lock(&lock);
        shared_var--;
        p++;
        spin_unlock(&lock);
        // put_message("2", shared_var);
        // unlock_(&l);
        
        // printk("thread 2 shared_var = %d\n", shared_var);
        // lock_release(&l); // Release the lock
    }
    // put_message("task 2222222222 p = ", p);
    // cpsr_int_enable();
    // printk("task 1 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    // printk("task 2 p = %d\n", p);
    // system_enable_fiq();
    // printk("shared_var = %d\n", shared_var);
    // printk("task 2 current thread priority = %d\n", pre_cur_thread()->priority);
    // lock_release(&l);
    // sys_exit();
}


// void task1() {
//     int p = 0;
//     for (int i = 0; i < 1000000; ++i) {
//         int success = 0;
//         while (!success) {
//             int curr_val = shared_var;
//             success = cas(&shared_var, curr_val, curr_val + 1);
//         }
//         p++;
//     }
//     sys_exit();
// }

// void task2() {
//     int p = 0;
//     for (int i = 0; i < 1000000; ++i) {
//         int success = 0;
//         while (!success) {
//             int curr_val = shared_var;
//             success = cas(&shared_var, curr_val, curr_val - 1);
//         }
//         p++;
//     }
//     sys_exit();
// }


// Example1:L(prio2),M(prio4),H(prio8) 
// - L holds lockl
// - M waits on l, L’s priority raised to L1 = max(M, L) = 4 
// - Then H waits on l,L’s priority raised to max(H,L1)=8

void notmain(void) {
    pre_init();
    spin_init(&lock);
    // no
    // shared_var = 99
    // 
    // lock_init(&l); // Initialize the lock.
    // mmu_disable(); // by default MMU is disabled
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
