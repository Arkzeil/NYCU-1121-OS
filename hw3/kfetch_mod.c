#include <linux/init.h>
#include <linux/module.h>       // necessary for all linux modules
#include <linux/moduleparam.h>  // for reading parameter at load time
#include <linux/fs.h>           // The 'file_operations' structure is defined here, also register
#include <linux/module.h>       // for the module usage monitoring
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/utsname.h>      // to get the hostname


MODULE_LICENSE("Dual BSD/GPL"); // free licence. kernel would complain without this line

dev_t dev;
struct cdev my_dev;
static unsigned int major;                       // major device number
static struct class *dev_cls;           // for class of devices
static unsigned char kfetch_mask = 0;    // for driver mask that can be used after a invocation
//struct mutex kfetch_mutex;

static DEFINE_MUTEX(kfetch_mutex);    // define mutex loc gloablly and statically

#define DEVICE_NAME "kfetch"
#define BUFFER_SIZE 1024

#define KFETCH_NUM_INFO 6
// Preprocessor macro
#define KFETCH_RELEASE   (1 << 0)
#define KFETCH_NUM_CPUS  (1 << 1)
#define KFETCH_CPU_MODEL (1 << 2)
#define KFETCH_MEM       (1 << 3)
#define KFETCH_UPTIME    (1 << 4)
#define KFETCH_NUM_PROCS (1 << 5)

#define KFETCH_FULL_INFO ((1 << KFETCH_NUM_INFO) - 1)

static int kfetch_open(struct inode *, struct file *); 
static int kfetch_release(struct inode *, struct file *); 
static ssize_t kfetch_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t kfetch_write(struct file *, const char __user *, size_t, loff_t *); 

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

    major = MAJOR(dev);
    printk(KERN_INFO "Device major number: %d", major);

    cdev_init(&my_dev, &kfetch_ops);         // initialize the data structure struct cdev for our char device and associate it with the device numbers.

    if(cdev_add(&my_dev, dev, 1) < 0){      // add the char device to the system
        printk(KERN_ERR "Error while add device");
        return -1;
    }

    dev_cls = class_create(THIS_MODULE, DEVICE_NAME); // create a new class of devices, for management
    if(IS_ERR(dev_cls)){
        printk(KERN_ERR "Error while creating device class");
        return -1;
    }

    if(IS_ERR(device_create(dev_cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME))){ // dynamically create a device node in the /dev directory associated with a specific class
        printk(KERN_ERR "Error while creating device");
        return -1;
    } 

    return 0;
}

static void kfetch_exit(void){
    if(module_refcount(THIS_MODULE) > 0)
        printk(KERN_ERR "Error: there're still processes using this module");

    device_destroy(dev_cls, MKDEV(major, 0)); 
    class_destroy(dev_cls);
    unregister_chrdev(major, DEVICE_NAME);
    
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

static int kfetch_open(struct inode *inode, struct file *file){
    mutex_lock(&kfetch_mutex); //only one process can use this device at a time
    // critical section
    // Increment the usage count
    try_module_get(THIS_MODULE);

    return 0;
}

static int kfetch_release(struct inode *inode, struct file *file){

    // end of critical section
    mutex_unlock(&kfetch_mutex);

    module_put(THIS_MODULE);    // Decrement the usage count
    return 0;
}

static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    char kfetch_buf[BUFFER_SIZE] = "Test\n";
    size_t len;

    if(*offset >= BUFFER_SIZE)  // had read to the end of buffer
        return 0;
    
    if(length > BUFFER_SIZE - *offset) // read only to the end of buffer
        len = BUFFER_SIZE - *offset;
    else 
        len = length;
    /* fetching the information */
    
    printk("hostname: %s\n", utsname()->nodename);
    printk("hostname: %s\n", utsname()->release);

    if (copy_to_user(buffer, kfetch_buf, len)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    return 0;
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
    if(mask_info & KFETCH_FULL_INFO){
        kfetch_mask |= KFETCH_FULL_INFO;
        return sizeof(kfetch_mask);
    }
    
    if(mask_info & KFETCH_RELEASE)
        kfetch_mask |= KFETCH_RELEASE;
    if(mask_info & KFETCH_NUM_CPUS)
        kfetch_mask |= KFETCH_NUM_CPUS;
    if(mask_info & KFETCH_CPU_MODEL)
        kfetch_mask |= KFETCH_CPU_MODEL;
    if(mask_info & KFETCH_MEM)
        kfetch_mask |= KFETCH_MEM;
    if(mask_info & KFETCH_UPTIME)
        kfetch_mask |= KFETCH_UPTIME;
    if(mask_info & KFETCH_NUM_PROCS)
        kfetch_mask |= KFETCH_NUM_PROCS;

    return sizeof(kfetch_mask);
}

module_init(kfetch_init); //loading module
module_exit(kfetch_exit); //removing module)