
/* Prototype module for second mandatory DM510 assignment */
#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include "ioct.h"
#include <linux/sched.h>
#include <linux/sched/signal.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
// #include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
// #include <asm/system.h>
#include <asm/switch_to.h>
#include <linux/cdev.h>


/* Prototypes - this would normally go in a .h file */
static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#define DEVICE_NAME "dm510_dev" /* Dev name as it appears in /proc/devices */
#define MAJOR_NUMBER 254
#define MIN_MINOR_NUMBER 0
#define MAX_MINOR_NUMBER 1

#define DEVICE_COUNT 2
/*
#define DM510_IOC_MAGIC 9

#define IOC_RESETBUFFER _IO(DM510_IOC_MAGIC, 0)
#define IOC_RESETPROC _IO(DM510_IOC_MAGIC, 1)

#define IOC_NUMCASES 2*/
/* end of what really should have been in a .h file */

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
  .unlocked_ioctl   = dm510_ioctl
};

struct dm510_buffer {
	wait_queue_head_t inq, outq;       /* read and write queues */
	char *buffer, *end;                /* begin of buf, end of buf */
	int nreaders, nwriters;						 /* number of openings for r/w */
	char *buffer_rp, *buffer_wp;		 	 /* where to read, where to write */
};

struct dm510_dev {
	struct dm510_buffer *read_buffer;		/*pointer to buffer for reading*/
	struct dm510_buffer *write_buffer;	/*pointer to buffer for writing*/
	struct cdev cdev;                  	/* Char device structure */
	struct mutex mutex;              		/* mutual exclusion semaphore */
};

/*initialising*/
struct dm510_buffer *dm510_buffer_0;
struct dm510_buffer *dm510_buffer_1;

struct dm510_dev *dm510_dev_0;
struct dm510_dev *dm510_dev_1;

int buffer_size = 3000;
int number_proc=10;

/*type that is defined and is used to hold device numbers (major,minor)*/
dev_t dev_holder;
/* called when module is loaded */
int dm510_init_module( void ) {

	/* initialization code belongs here */
	int err;
	/*creates the first device*/
	dev_holder = MKDEV(MAJOR_NUMBER,MIN_MINOR_NUMBER);
	/*registration takes a pointer and a name*/
	err = register_chrdev_region(dev_holder,DEVICE_COUNT,DEVICE_NAME);
	if(err != 0){
		printk(KERN_NOTICE "Unable to get region, error %d\n", err);
		return -ENODEV;
	}

	/* allocating space for devices and buffers
	 * and freeing already allocated space if error occurs */
	dm510_dev_0 = kmalloc(sizeof(struct dm510_dev), GFP_KERNEL);
	if (!dm510_dev_0){
		return -ENOMEM;
	}
	dm510_dev_1 = kmalloc(sizeof(struct dm510_dev), GFP_KERNEL);
	if (!dm510_dev_1){
		kfree(dm510_dev_0);
		return -ENOMEM;
	}
	dm510_buffer_0 = kmalloc(sizeof(struct dm510_buffer), GFP_KERNEL);
	if (!dm510_buffer_0){
		kfree(dm510_dev_0);
		kfree(dm510_dev_1);
		return -ENOMEM;
	}
	dm510_buffer_1 = kmalloc(sizeof(struct dm510_buffer), GFP_KERNEL);
	if (!dm510_buffer_1){
		kfree(dm510_dev_0);
		kfree(dm510_dev_1);
		kfree(dm510_buffer_0);
		return -ENOMEM;
	}
	/*allocating space for text in buffer*/
	dm510_buffer_0->buffer = kmalloc(sizeof(char*) *buffer_size, GFP_KERNEL);
	if (!dm510_buffer_0->buffer){
		kfree(dm510_dev_0);
		kfree(dm510_dev_1);
		kfree(dm510_buffer_0);
		kfree(dm510_buffer_1);
		return -ENOMEM;
	}
	dm510_buffer_1->buffer = kmalloc(sizeof(char*) *buffer_size, GFP_KERNEL);
	if (!dm510_buffer_1->buffer){
		kfree(dm510_dev_0);
		kfree(dm510_dev_1);
		kfree(dm510_buffer_0->buffer);
		kfree(dm510_buffer_0);
		kfree(dm510_buffer_1);
		return -ENOMEM;
	}
	/* initialising all int's in the buffer to the right size */
	dm510_buffer_0->nreaders = 0;
	dm510_buffer_0->nwriters = 0;
	dm510_buffer_0->buffer_rp = dm510_buffer_0->buffer;
	dm510_buffer_0->buffer_wp = dm510_buffer_0->buffer;

	dm510_buffer_1->nreaders = 0;
	dm510_buffer_1->nwriters = 0;
	dm510_buffer_1->buffer_rp = dm510_buffer_1->buffer;
	dm510_buffer_1->buffer_wp = dm510_buffer_1->buffer;

	dm510_buffer_0->end = dm510_buffer_0->buffer + buffer_size;
	dm510_buffer_1->end = dm510_buffer_1->buffer + buffer_size;

	/* initialize read and write queues */
	init_waitqueue_head(&dm510_buffer_0->inq);
	init_waitqueue_head(&dm510_buffer_0->outq);
	init_waitqueue_head(&dm510_buffer_1->inq);
	init_waitqueue_head(&dm510_buffer_1->outq);

	/* initialise the mutex locks */
	mutex_init(&dm510_dev_0->mutex);
	mutex_init(&dm510_dev_1->mutex);

	/* initialize a cdev structure */
	cdev_init(&(dm510_dev_0->cdev), &dm510_fops);
	cdev_init(&(dm510_dev_1->cdev), &dm510_fops);

	/* setting the owner */
	dm510_dev_0->cdev.owner = THIS_MODULE;
	dm510_dev_1->cdev.owner = THIS_MODULE;

	/* setting the right read and write buffers in the devices */
	dm510_dev_0->read_buffer = dm510_buffer_0;
	dm510_dev_0->write_buffer = dm510_buffer_1;
	dm510_dev_1->read_buffer = dm510_buffer_1;
	dm510_dev_1->write_buffer = dm510_buffer_0;

	/* add the devices to the dev_t place holder */
	err = cdev_add(&dm510_dev_0->cdev,dev_holder,1);
	if (err != 0) {
		printk(KERN_NOTICE "Error %d adding cdev", err);
		return -EAGAIN;
	}

		err = cdev_add(&dm510_dev_1->cdev,dev_holder+1,1);
	if (err != 0) {
		printk(KERN_NOTICE "Error %d adding cdev", err);
		return -EAGAIN;
	}

	printk(KERN_INFO "DM510: Hello from your device!\n");
	return 0;
}

/* Called when module is unloaded */
void dm510_cleanup_module( void ) {
	/* clean up code belongs here */

	if (!dev_holder){
		return;
	}
	/* cleans up in reverse order */
	cdev_del(&dm510_dev_1->cdev);   /* removes devices from the dev_t holder */
	cdev_del(&dm510_dev_0->cdev);
	kfree(dm510_buffer_1->buffer);	/*free content of buffer*/
	kfree(dm510_buffer_0->buffer);
	kfree(dm510_buffer_1);					/*free buffer*/
	kfree(dm510_buffer_0);
	kfree(dm510_dev_1);							/*free device*/
	kfree(dm510_dev_0);

	/* removes the dev_t place holder */
	unregister_chrdev_region(dev_holder, DEVICE_COUNT);

	printk(KERN_INFO "DM510: Module unloaded.\n");
}

/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {
	/* device claiming code belongs here */
	struct dm510_dev *dev;

	/* puts the devices into filp */
	dev = container_of(inode->i_cdev, struct dm510_dev, cdev);
	filp->private_data = dev;

	/* locks the process */
	if (mutex_lock_interruptible(&dev->mutex)){
		return -ERESTARTSYS;
	}

	/* make checks to ensure that number of processes is kept */
	if (dev->read_buffer->nreaders >= number_proc-1 && (filp->f_mode & FMODE_READ)) {
		mutex_unlock(&dev->mutex);
		return -EAGAIN;
	}else if (dev->read_buffer->nreaders + dev->write_buffer->nwriters >= number_proc){ /* if there is a writer and max # of processes is running */
		mutex_unlock(&dev->mutex);
		return -EAGAIN;
	}else{  /* if its either a write, or there are fewer than max processes */
		if (filp->f_mode & FMODE_READ){
			dev->read_buffer->nreaders++;
		}
		if (filp->f_mode & FMODE_WRITE){
			if (filp->f_flags & O_NONBLOCK) {
				mutex_unlock(&dev->mutex);
				return -EAGAIN;
			}
			dev->write_buffer->nwriters++;
			/* if there are one writer allready the rest is put to sleep */
			if(wait_event_interruptible(dev->write_buffer->inq, (dev->write_buffer->nwriters >= 1))){
				mutex_unlock(&dev->mutex);
				return -ERESTARTSYS;
			}
		}
	}
	mutex_unlock(&dev->mutex);

	return 0;
}

/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {
	/* device release code belongs here */

	struct dm510_dev *dev = filp->private_data;

	mutex_lock(&dev->mutex);
	if (filp->f_mode & FMODE_READ){
		dev->read_buffer->nreaders--;
	}
	if (filp->f_mode & FMODE_WRITE){
		dev->write_buffer->nwriters--;
	}
	mutex_unlock(&dev->mutex);

	return 0;
}

/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp,
    char *buf,      /* The buffer to fill with data     */
    size_t count,   /* The max number of bytes to read  */
    loff_t *f_pos )  /* The offset in the file           */
{
	/* read code belongs here */

	struct dm510_dev *dev = filp->private_data;

	int remaining;

	if (mutex_lock_interruptible(&dev->mutex)){
		return -ERESTARTSYS;
	}
	while (dev->read_buffer->buffer_rp == dev->read_buffer->buffer_wp) { /* nothing to read */
		mutex_unlock(&dev->mutex); /* release the lock */
		if (filp->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		if (wait_event_interruptible(dev->read_buffer->inq, (dev->read_buffer->buffer_rp != dev->read_buffer->buffer_wp))){
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		}
		/* otherwise loop, but first reacquire the lock */
		if (mutex_lock_interruptible(&dev->mutex)){
			return -ERESTARTSYS;
		}
	}

	/* ok, data is there, return something */
	if (dev->read_buffer->buffer_wp > dev->read_buffer->buffer_rp){
		count = min(count, (size_t)(dev->read_buffer->buffer_wp - dev->read_buffer->buffer_rp));
	}else{ /* the write pointer has wrapped, return data up to dev->end */
		count = min(count, (size_t)(dev->read_buffer->end - dev->read_buffer->buffer_rp));
	}

	/* if access is not ok return error */
	if (!access_ok(VERIFY_WRITE, buf, count)){
		mutex_unlock (&dev->mutex);
		return -EACCES;
	}

	/* copies to user and the remaining # of bytes not copied is stored in remaining */
	remaining = copy_to_user(buf, dev->read_buffer->buffer_rp, count);

	/* moving the read pointer to the next non read data */
	dev->read_buffer->buffer_rp += count;

	/* move read pointer from end to start */
	if (dev->read_buffer->buffer_rp >= dev->read_buffer->end){
		dev->read_buffer->buffer_rp = dev->read_buffer->buffer; /* wrapped */
	}
	mutex_unlock (&dev->mutex);

	/* finally, awake any writers and return */
	wake_up_interruptible(&dev->read_buffer->outq);

	/* return number of bytes read */
	return count;
}

/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
    const char *buf,/* The buffer to get data from      */
    size_t count,   /* The max number of bytes to write */
    loff_t *f_pos )  /* The offset in the file           */
{
	/* write code belongs here */
	struct dm510_dev *dev = filp->private_data;
	int remaining;

	/* if trying to read bytes under 1 */
	if (count < 1)
	{
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dev->mutex)){
		return -ERESTARTSYS;
	}

	if (dev->write_buffer->buffer_wp >= dev->write_buffer->buffer_rp){
		count = min(count, (size_t)(dev->write_buffer->end - dev->write_buffer->buffer_wp)); /* to end-of-buf */
	}else{ /* the write pointer has wrapped, fill up to rp-1 */
		count = min(count, (size_t)(dev->write_buffer->buffer_rp - dev->write_buffer->buffer_wp - 1));
	}

	if (!access_ok(VERIFY_WRITE, buf, count)){
		mutex_unlock (&dev->mutex);
		return -EACCES;
	}

	remaining = copy_from_user((dev->write_buffer->buffer_wp), buf, count);

	dev->write_buffer->buffer_wp += count - remaining;

	if (dev->write_buffer->buffer_wp >= dev->write_buffer->end){
		dev->write_buffer->buffer_wp = dev->write_buffer->buffer; /* wrapped */
	}
	mutex_unlock(&dev->mutex);
	//dev->write_buffer->buffer_wp = dev->write_p;

	wake_up_interruptible(&dev->write_buffer->inq);  /* blocked in read() and select() */

	return count - remaining; //return number of bytes written
}

/* called by system call icotl */
long dm510_ioctl(
    struct file *filp,
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	/* ioctl code belongs here */
	printk(KERN_INFO "DM510: ioctl called.\n");

	if (_IOC_TYPE(cmd) != DM510_IOC_MAGIC){
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > IOC_NUMCASES){
		return -ENOTTY;
	}

	switch(cmd){

		case IOC_RESETBUFFER:
			buffer_size = arg;
			if (buffer_size <= 0) {
				return -EINVAL;
			}
			kfree(dm510_buffer_1->buffer);
			kfree(dm510_buffer_0->buffer);
			dm510_buffer_0->buffer = kmalloc(sizeof(char*) *buffer_size, GFP_KERNEL);
			if (!dm510_buffer_0->buffer){
				return -ENOMEM;
			}
			dm510_buffer_1->buffer = kmalloc(sizeof(char*) *buffer_size, GFP_KERNEL);
			if (!dm510_buffer_1->buffer){
				return -ENOMEM;
			}

			dm510_buffer_0->end = dm510_buffer_0->buffer + buffer_size;
			dm510_buffer_1->end = dm510_buffer_1->buffer + buffer_size;

			dm510_buffer_0->buffer_rp = dm510_buffer_0->buffer;
			dm510_buffer_0->buffer_wp = dm510_buffer_0->buffer;

			dm510_buffer_1->buffer_rp = dm510_buffer_1->buffer;
			dm510_buffer_1->buffer_wp = dm510_buffer_1->buffer;

			return buffer_size;

		case IOC_RESETPROC:
			if (arg <=1) {
				return -EINVAL;
			}
			number_proc = arg;
			return number_proc;

		default:
			return -ENOTTY;
	}
	return 0;
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_AUTHOR( "Michelle Dung Hoang, Lea Fog-Fredsgaard, Danny Rene Jensen" );
MODULE_LICENSE( "GPL" );
