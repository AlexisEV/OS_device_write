/*
 * chardev.c: Creates a read-only char device that says how many times
 * you've read from the dev file
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* for put_user */
/*
 * Prototypes - this would normally go in a .h file
 */
MODULE_LICENSE("GPL");
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
#define SUCCESS 0
#define DEVICE_NAME "UCSP" /* Dev name as it appears in /proc/devices */
#define BUF_LEN 80            /* Max length of the message from the device */
/*
 * Global variables are declared as static, so are global within the file.
 */
static int Major;           /* Major number assigned to our device driver */
static int Device_Open = 0; /* Is device open?
                             * Used to prevent multiple access to device */
static char msg[BUF_LEN];   /* The msg the device will give when asked */
static char *msg_Ptr;
static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release};
/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
    Major = register_chrdev(0, DEVICE_NAME, &fops);
    if (Major < 0)
    {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }
    printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");
    return SUCCESS;
}
/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
    /*
     * Unregister the device
     */
    int ret;
    unregister_chrdev(Major, DEVICE_NAME);
    if (ret < 0)
        printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
}
/*
 * Methods
 */
/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
    //static int counter = 0;
    if (Device_Open)
        return -EBUSY;
    Device_Open++;
    //sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    msg_Ptr = msg;
    try_module_get(THIS_MODULE);
    return SUCCESS;
}
/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
    Device_Open--; /* We're now ready for our next caller */
    /*
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module.
     */
    module_put(THIS_MODULE);
    return 0;
}
/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h */
                           char *buffer,      /* buffer to fill with data */
                           size_t length,     /* length of the buffer */
                           loff_t *offset)
{
    int bytes_read = 0;
    if (*msg_Ptr == 0)
        return 0;
    while (length && *msg_Ptr)
    {
        put_user(*(msg_Ptr++), buffer++);
        length--;
        bytes_read++;
    }
    /*
     * Most read functions return the number of bytes put into the buffer
     */
    return bytes_read;
}
/*
 * Called when a process writes to dev file: echo "hi" > /dev/hello
 */
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
    size_t max_length = BUF_LEN - 1;
    size_t bytes_to_copy;
    if(len<max_length) {
      bytes_to_copy = len;
    }
    else{
      bytes_to_copy = max_length;
    }
    size_t index;
    for (index = 0; index < bytes_to_copy; index++) {
        if (get_user(msg[index], &buff[index])) {
            printk(KERN_ALERT "Error\n");
            return -EFAULT;
        }
    }

    msg[index] = '\0';
    msg_Ptr = msg;
    return bytes_to_copy;
}
