#include <linux/init.h>
#include <linux/module.h>       // necessary for all linux modules
#include <linux/moduleparam.h>  // for reading parameter at load time
#include <linux/fs.h>           // The 'file_operations' structure is defined here, also register
#include <linux/module.h>       // for the module usage monitoring
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/utsname.h>      // to get the hostname
#include <linux/mm.h>           // to get memory usage
#include <linux/smp.h>          // to get cpu info
#include <linux/cpumask.h>      // to get cpu amount
#include <linux/sched/stat.h>   // to get the number of processes
#include <linux/sys.h>
#include <linux/sched.h>        // for task_struct
#include <linux/ktime.h>        // to get the uptime
#include <linux/kernel.h>
#include <linux/sched/signal.h> // for 'for_each_process_thread'
#include <linux/kprobes.h>


MODULE_LICENSE("Dual BSD/GPL"); // free licence. kernel would complain without this line

dev_t dev;
struct cdev my_dev;
static unsigned int major;                       // major device number
static struct class *dev_cls;           // for class of devices
static unsigned char kfetch_mask = 0;    // for driver mask that can be used after a invocation
//struct mutex kfetch_mutex;

static DEFINE_MUTEX(kfetch_mutex);    // define mutex lock gloablly and statically, for single thread
static DEFINE_MUTEX(RW_mutex);        // define mutex lock gloablly and statically, for Read after write
//static struct semaphore kfetch_semaphore; // define semaphore gloablly and statically, for single thread
//static struct semaphore RW_semaphore; // define semaphore gloablly and statically, for Read after write

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
    printk(KERN_ALERT "hello world!\n");

    //sema_init(&kfetch_semaphore, 1);  // Initialize semaphore with an initial count of 1
    //sema_init(&RW_semaphore, 1);

    if(alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0){ // get dynamic major number(stored in dev) ,and assign 1 minor number starting from 0
        printk(KERN_ERR "Error while getting major number\n");
        //pr_err("Error while getting major number");
        return -1;
    }

    major = MAJOR(dev);
    printk(KERN_INFO "Device major number: %d\n", major);

    cdev_init(&my_dev, &kfetch_ops);         // initialize the data structure struct cdev for our char device and associate it with the device numbers.

    if(cdev_add(&my_dev, dev, 1) < 0){      // add the char device to the system
        printk(KERN_ERR "Error while add device\n");
        return -1;
    }

    dev_cls = class_create(THIS_MODULE, DEVICE_NAME); // create a new class of devices, for management
    if(IS_ERR(dev_cls)){
        printk(KERN_ERR "Error while creating device class\n");
        return -1;
    }

    if(IS_ERR(device_create(dev_cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME))){ // dynamically create a device node in the /dev directory associated with a specific class
        printk(KERN_ERR "Error while creating device\n");
        return -1;
    } 

    /*struct sysinfo mem_info;
    uint32_t kb_unit;           // for the conversion from byte to MB
    int cpu;                    // to store the cpu id
    struct cpuinfo_x86 *CPU_info;
    s64 uptime;                // signed 64 bits

    printk("%s", utsname()->nodename);
    printk("--------------");
    printk("Kernel: %s", utsname()->release);

    si_meminfo(&mem_info);  // get all kinds of memory usage in pages
    kb_unit = mem_info.mem_unit / 1024; // byte -> KB, can't conver to MB directly since it's the byte of page, which is probably 1024 or 2048. Converting to MB will leads to 0

    printk("Total RAM: %ld MB / %ld MB", mem_info.freeram * kb_unit / 1024, mem_info.totalram * kb_unit / 1024);
    
    //for_each_online_cpu(cpu)
    cpu = smp_processor_id(); // obtain CPU number
    CPU_info = &cpu_data(cpu);
    printk("CPU: %s", CPU_info->x86_model_id);
    
    printk("CPUs: %d / %d", num_online_cpus(), num_active_cpus());

    printk("Procs: %d", nr_threads);

    uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC);
    printk("Uptime: %lld", uptime / 60);*/

    return 0;
}

static void kfetch_exit(void){
    if(module_refcount(THIS_MODULE) > 0)
        printk(KERN_ERR "Error: there're still processes using this module\n");

    device_destroy(dev_cls, MKDEV(major, 0)); 
    class_destroy(dev_cls);
    unregister_chrdev(major, DEVICE_NAME);
    
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

static int kfetch_open(struct inode *inode, struct file *file){
    mutex_lock(&kfetch_mutex); //only one process can use this device at a time
    //down(&kfetch_semaphore);  // Acquire semaphore
    // critical section
    // Increment the usage count
    try_module_get(THIS_MODULE);

    return 0;
}

static int kfetch_release(struct inode *inode, struct file *file){

    // end of critical section
    mutex_unlock(&kfetch_mutex);
    //up(&kfetch_semaphore);  // Release semaphore

    module_put(THIS_MODULE);    // Decrement the usage count
    return 0;
}

static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    //char *kfetch_buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    char kfetch_buf[] = "Test\n";
    size_t len;
    struct sysinfo mem_info;
    uint32_t kb_unit;           // for the conversion from byte to MB
    int cpu;                    // to store the cpu id
    struct cpuinfo_x86 *CPU_info;
    s64 uptime;                // signed 64 bits
    char *logo = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    strcpy(logo, "     .-.\n");
    strcat(logo, "    (.. |\n");  
    strcat(logo, "    <>  |\n");
    strcat(logo, "   / --- \\\n");
    strcat(logo, "  ( |   | |\n");
    strcat(logo, " |\\\\_)___/\\)/\\\n");
    strcat(logo, "<__)------(__/\n");

    if(*offset >= BUFFER_SIZE)  // had read to the end of buffer
        return 0;
    
    if(length > BUFFER_SIZE - *offset) // read only to the end of buffer
        len = BUFFER_SIZE - *offset;
    else 
        len = sizeof(kfetch_buf);

    mutex_lock(&RW_mutex);
    /* fetching the information */
    printk("%s\n", utsname()->nodename);
    printk("-------------\n");
    

    /*if(kfetch_mask & KFETCH_FULL_INFO){
        
    }*/
    
    if(kfetch_mask & KFETCH_RELEASE){
        printk("Kernel: %s\n", utsname()->release);
    }
    if(kfetch_mask & KFETCH_NUM_CPUS){
        printk("CPUs: %d / %d\n", num_online_cpus(), num_active_cpus());
    }
    if(kfetch_mask & KFETCH_CPU_MODEL){
        //for_each_online_cpu(cpu)
        cpu = smp_processor_id(); // obtain CPU number
        CPU_info = &cpu_data(cpu);
        printk("CPU: %s\n", CPU_info->x86_model_id);
    }
    if(kfetch_mask & KFETCH_MEM){
        si_meminfo(&mem_info);  // get all kinds of memory usage in pages
        kb_unit = mem_info.mem_unit / 1024; // byte -> KB, can't conver to MB directly since it's the byte of page, which is probably 1024 or 2048. Converting to MB will leads to 0

        printk("Total RAM: %ld MB / %ld MB\n", mem_info.freeram * kb_unit / 1024, mem_info.totalram * kb_unit / 1024);
    }
    if(kfetch_mask & KFETCH_UPTIME){
        uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC); // get the time from booting in ns, convert to seconds
        printk("Uptime: %lld\n", uptime / 60);                             // convert to minutes
    }
    if(kfetch_mask & KFETCH_NUM_PROCS){
        struct task_struct *p, *t;
        int count = 0;
        //printk("Procs: %d", nr_processes);
        for_each_process_thread(p, t){
            count++;
        }
        printk("Procs: %d\n", count);
    }
    mutex_unlock(&RW_mutex);

    printk("%zd\n", len);
    if (copy_to_user(buffer, logo, strlen(logo + 1))) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    return strlen(logo + 1);
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

    if(mask_info == 0)      // if no flag provided, keep old mask information
        return sizeof(kfetch_mask);

    kfetch_mask = 0;        // clean mask if there's flag

    mutex_lock(&RW_mutex);  // to ensure write first
    //printk("mask info:%d\n", mask_info);

    /* setting the information mask */
    if((mask_info & KFETCH_FULL_INFO) == KFETCH_FULL_INFO){
        kfetch_mask |= KFETCH_FULL_INFO;
        mutex_unlock(&RW_mutex);
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
    //printk("%d\n", kfetch_mask);

    mutex_unlock(&RW_mutex);
    return sizeof(kfetch_mask);
}

module_init(kfetch_init); //loading module
module_exit(kfetch_exit); //removing module)