#include <linux/kernel.h>
#include <linux/syscalls.h>

// asmlinkage let C function gets parameter from stack instead of registers
// use long datatype for 64bits machine
/*asmlinkage long sys_hello(void){
    printk("Hello, world!\n");
    printk("312552025\n");

    return 0;
}*/

/*Entry point for my system call*/
SYSCALL_DEFINE0(hello){
    printk("Hello, world!\n");
    printk("312552025\n");

    return 0;
    /*return sys_hello();*/
}