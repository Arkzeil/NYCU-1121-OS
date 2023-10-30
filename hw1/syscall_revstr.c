/* It's using C90 by default, and in C90 there's no "//" comment style in it*/
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h> /* for kmalloc */
#include <linux/string.h>

/* char __user *str: get string from user space */
/*asmlinkage long sys_revstr(int len, const char __user *str){*/
    /* 
    using kmalloc to get fixed byte array, GFP_KERNEL: May sleep and swap to free memory.
    "len + 1" for there's another "\0",
    this declaration must be put after other variable declaration, otherwise warning:"ISO C90 forbids mixed declarations and code" would appear
    */
    /*char *rev_str = kmalloc(len + 1, GFP_KERNEL);
    char *dup_str = kmalloc(len + 1, GFP_KERNEL);
    int l = 0; *//* index for rec_str */

    /* Warning would appear if this block put in the begining of function, need to dig in how to use other like C99 to compile */
    /*if(!str)
        return -1;

    if(!rev_str || !dup_str){
        printk("kmalloc error\n");
        return -1;
    }*/
    /*I tried stelen(str) and it failed(only get first 8 bytes) ,probably due to the limit between user space and kernel*/
    /*if(copy_from_user(dup_str, str, len + 1) )
        return -1; 

    printk("The origin string: %s\n", dup_str);*/
    
    /* there seems no "strrev()" function in <linux/string.h> library*/
    /*len--;*/ /*the "\0" always at the end*/
    /*while(len >= 0){
        rev_str[l] = dup_str[len];
        l++;
        len--;
    }
    rev_str[l] = '\0';
    
    printk("The reversed string: %s\n", rev_str);
    
    return 0;
}*/

/* SYSCALL_DEFINE1(user_space_func_name, 1st parameter type, 1st parameter name)*/
SYSCALL_DEFINE2(revstr, int, len, const char __user *, str){
    /* 
    using kmalloc to get fixed byte array, GFP_KERNEL: May sleep and swap to free memory.
    "len + 1" for there's another "\0",
    this declaration must be put after other variable declaration, otherwise warning:"ISO C90 forbids mixed declarations and code" would appear
    */
    char *rev_str = kmalloc(len + 1, GFP_KERNEL);
    char *dup_str = kmalloc(len + 1, GFP_KERNEL);
    int l = 0; /* index for rec_str */

    /* Warning would appear if this block put in the begining of function, need to dig in how to use other like C99 to compile */
    if(!str)
        return -1;

    if(!rev_str || !dup_str){
        printk("kmalloc error\n");
        return -1;
    }
    /*I tried stelen(str) and it failed(only get first 8 bytes) ,probably due to the limit between user space and kernel*/
    if(copy_from_user(dup_str, str, len + 1) )
        return -1; 

    printk("The origin string: %s\n", dup_str);
    
    /* there seems no "strrev()" function in <linux/string.h> library*/
    len--; /*the "\0" always at the end*/
    while(len >= 0){
        rev_str[l] = dup_str[len];
        l++;
        len--;
    }
    rev_str[l] = '\0';
    
    printk("The reversed string: %s\n", rev_str);
    
    return 0;
    /* when user invoke user_space_func_name, it's actually using the following system call */
    /*return sys_revstr(len, str);*/
}