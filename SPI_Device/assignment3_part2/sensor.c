#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/param.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define DEVICE_NAME "USonic"
#define DEVICE_CLASS "virtual_char"
#define NUM_DEVICE 1

static struct class *my_class;
unsigned int MAJOR_NUMBER;
unsigned int MINOR_NUMBER;

static unsigned char detect_edge = 0;
static dev_t dev; /* Allotted device number */

static __inline__ unsigned long long tsc(void)
{
	unsigned a, d;
	__asm__ __volatile__ ("rdtsc" : "=a"(a), "=d"(d));
	return ((unsigned long long) a) | ((unsigned long long) d)<<32;
}

/*My device structure*/
struct my_dev
{
	struct cdev cdev;  /* The cdev structure */
  	char name[30];
	unsigned int irqNumber;
	unsigned int pulse_width;
	unsigned int ready_flag;
	unsigned long long ts1, ts2;

}*my_devp;

/* ************ Open USonic driver**************** */
int USonic_open(struct inode *inode, struct file *file)
{
  	struct my_dev *ptr1;
  	//printk("ENTERED THE OPEN FUNCTION....!!!\n");
  	//printk(KERN_INFO "Driver: open()\n");

	/* Get the per-device structure that contains this cdev */
 	ptr1 = container_of(inode->i_cdev, struct my_dev, cdev);

  	/* Easy access to cmos_devp from rest of the entry points */
  	file->private_data = ptr1;
  	printk("%s: Device opening\n", ptr1->name);
  	return 0;
}

/*****************Read USonic driver******************/
static ssize_t USonic_read(struct file *filp, char *buff, size_t length,loff_t *off)
{
	//unsigned int pulse;
	int ret;
	unsigned long long cts;
	//printk("Entered READ function!!!\n");

	if(my_devp->ready_flag==1)
	{
		cts = my_devp->ts2 - my_devp->ts1;
		my_devp->pulse_width = div_u64(cts,400);
		//my_devp->ready_flag=0;
		ret=copy_to_user((void *)buff, (const void *)&my_devp->pulse_width, sizeof(my_devp->pulse_width));
		return ret;
	}
	else
		return -EBUSY;

}

/**********USonic write function**********************/
ssize_t USonic_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  	gpio_set_value_cansleep(61,0);
	udelay(2);
	gpio_set_value_cansleep(61,1);
	udelay(12);
	gpio_set_value_cansleep(61,0);
	my_devp->ready_flag=0;
	return 0;
}

/*****************Interrupt for USonic driver******************/
static irqreturn_t USonic_irq_handler(int irq, void *dev_id)
{
	if(detect_edge==0)
	{
	    my_devp->ts1= tsc();	//Rising edge detected, record the current time
	    irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);	//Program interrupt to detect falling edge
	   	detect_edge=1;	//Configure the flag for falling edge detection handling
	}
	else
	{
	    my_devp->ts2 = tsc();	//Falling edge detected, record the current time
	    irq_set_irq_type(irq, IRQF_TRIGGER_RISING);	//Program the interrupt to detect the rising edge
	    detect_edge=0;	//Configure the flag for rising edge detection handling
	    my_devp->ready_flag=1;
	}
	return IRQ_HANDLED;
}

/*****************Release USonic driver******************/

int USonic_release(struct inode *inode, struct file *file)
{
	struct my_dev *ptr2;
   	//printk("RELEASING THE  DRIVER....!!!\n");
	ptr2 = file->private_data;
	my_devp->ready_flag = 0;

	printk(KERN_INFO "Driver: close()\n");
  	printk("%s: Device closing\n", ptr2->name);
    return 0;
}

/**********************File operations*******************/
static struct file_operations fops={
	.owner = THIS_MODULE,
	.open = USonic_open,
	.release = USonic_release,
	.write = USonic_write,
	.read=USonic_read
};

/****************Initialization of USonic driver*********/
int __init USonic_driver_init(void)
{
 	int ret,result;
	unsigned int irq;

	my_devp=kmalloc(sizeof(struct my_dev),GFP_KERNEL);
	if(!my_devp)
		printk("Bad Malloc\n");
	//printk("\n ENTERED INIT FUNCTION!!!\n");
  	if (alloc_chrdev_region(&dev,MINOR_NUMBER,NUM_DEVICE, DEVICE_NAME)<0)   /* Request dynamic allocation of a device major number */
  	{
    	printk(KERN_DEBUG "Can't register device\n");
		return -1;
  	}
	//else printk("\n");
  	my_class = class_create(THIS_MODULE,DEVICE_NAME);
	//printk("CLASS CREATED!!!\n");
  	strcpy(my_devp->name,"ULTRA_SONIC_SENSOR");

	/* Connect the file operations with the cdev */
   	cdev_init(&my_devp->cdev, &fops);
	//printk("CDEV_INIT CALLED SUCCESSFULLY!!!\n");
    my_devp->cdev.owner = THIS_MODULE;
	//printk("CDEV. OWNER \n");

   	/* Connect the major/minor number to the cdev */
    ret = cdev_add(&my_devp->cdev,MKDEV(MAJOR(dev), MINOR(dev)),NUM_DEVICE);
    if (ret)
    {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
   	device_create(my_class, NULL, MKDEV(MAJOR(dev), MINOR(dev)),NULL,"%s","USonic");
	//printk("USonic CREATED SUCCESSFULLY\n");
	//Set GPIO pins directions and values
	gpio_request_one(61, GPIOF_OUT_INIT_LOW , "gpio13");
	gpio_request_one(14, GPIOF_OUT_INIT_LOW , "gpio62");
	gpio_request(76, "Echo_Mux1");
	gpio_request(64, "Trig_Mux2");
	gpio_request_one(16, 1, "gpio34");
	gpio_request(77, "Trig_Mux1");

	//Set GPIO pins values
	gpio_set_value_cansleep(61, 0);
	gpio_set_value_cansleep(14, 0);
	gpio_set_value_cansleep(76, 0);
	gpio_set_value_cansleep(64, 0);
	gpio_set_value_cansleep(77, 0);

	gpio_free(14);
	gpio_request_one(14, GPIOF_IN , "gpio62");
	irq = gpio_to_irq(14);	//Initialise interrupt to detect from gpio14

	/****enable the interrupt at that pin***/
 	if ( (irq) < 0)
	{
		printk(KERN_ALERT "Gpio 14 cannot be used as interrupt\n");
		return -EINVAL;
	}
	my_devp->irqNumber=irq;

	result = request_irq( my_devp->irqNumber, USonic_irq_handler, IRQF_TRIGGER_RISING ,"INTERRUPT",my_devp);

	my_devp->ready_flag=0;
	my_devp->ts1=0;
	my_devp->ts2=0;

	printk("USonic Driver initialized.\n");

  	return 0;
}

/*******************USonic exit function*********************/
void __exit USonic_driver_exit(void)
{
  	unregister_chrdev_region(dev,1);
	/* Destroy device */
    device_destroy (my_class, MKDEV(MAJOR(dev),MINOR(dev)));
   	cdev_del(&my_devp->cdev);
	/* Destroy driver_class */
  	class_destroy(my_class);
	free_irq(my_devp->irqNumber,my_devp);
	gpio_free(61);
	gpio_free(14);
	gpio_free(77);
	gpio_free(76);
	gpio_free(64);
	gpio_free(16);
}

module_init (USonic_driver_init);	/* Module initialization */
module_exit (USonic_driver_exit);	/* Module exit */
MODULE_LICENSE("GPL v2");
