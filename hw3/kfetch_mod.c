#include <linux/init.h>
#include <linux/module.h>       // necessary for all linux modules
#include <linux/moduleparam.h>  // for reading parameter at load time
#include <linux/fs.h>           // The 'file_operations' structure is defined here, also register
#include <linux/module.h>       // for the module usage monitoring
#include <linux/cdev.h>

#include <kfetch.h>

MODULE_LICENSE("Dual BSD/GPL"); // free licence. kernel would complain without this line

dev_t dev;
struct cdev my_dev;
static int major;               // major device number

#define DEVICE_NAME "kfetch"
// A C99 way of assigning to elements of a structure that makes assigning to this structure more convenient.
// C99 help with compatibility compared to GNU
// Any member of the structure which you do not explicitly assign will be initialized to NULL
const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,     // define a function that reads from the device
    .write   = kfetch_write,
    .open    = kfetch_open,     // Since Linux v3.14, the read, write and seek operations are guaranteed for thread-safe by using the f_pos specific lock
    .release = kfetch_release,
};

static int kfetch_init(void){
    printk(KERN_ALERT "hello world!");

    if(alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0){ // get dynamic major number(stored in dev) ,and assign 1 minor number starting from 0
        printk(KERN_ERR "Error while getting major number");
        //pr_err("Error while getting major number");
        return -1;
    }

    major = MAJOR(dev)
    printk(KERN_INFO "Device major number: %d", major);

    cdev_init(&my_dev, &kfetch_ops);         // initialize the data structure struct cdev for our char device and associate it with the device numbers.

    if(cdev_add(&my_dev, dev, 1) < 0){      // add the char device to the system
        printk(KERN_ERR "Error while add device");
        return -1;
    }

    cls = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 

    return 0;
}

static void kfetch_exit(void){
    if(module_refcount(THIS_MODULE) > 0)
        printk(KERN_ERR "Error: there're still processes using this module");

    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

static void kfetch_open(){

}

static void kfetch_release(){

}

static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    /* fetching the information */

    if (copy_to_user(buffer, kfetch_buf, len)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    /* cleaning up */
}

static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
}

module_init(hello_init); //loading module
module_exit(hello_exit); //removing module)