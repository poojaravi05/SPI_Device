#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>

#define DEVICE_CLASS_NAME 	"spidev"
#define DRIVER_NAME 		"spidev"
#define DEVICE_NAME 		"spidev1.1"

#define MAJOR_NUMBER    154     // assigned major number
#define MINOR_NUMBER    0	// allocating minor numbers dynamically

static DEFINE_MUTEX(device_list_lock);

/*
 * per device structure
 */
struct spidev_data {
	dev_t                   devt;
	struct spi_device       *spi;
	char pattern_buffer[12][8];
	unsigned int sequence_buffer[12][2];
};

/*
 * Global variables
 */
static struct spidev_data *spidev_global;
static struct class *spi_led_class;   	// Device class //
static unsigned bufsiz = 4096;
static unsigned int busyFlag=0;
static struct spi_message m;
static unsigned char ch_tx[2]={0};
static unsigned char ch_rx[2]={0};
static struct spi_transfer t = {
	.tx_buf = &ch_tx[0],
	.rx_buf = &ch_rx[0],
	.len = 2,
	.cs_change = 1,
	.bits_per_word = 8,
	.speed_hz = 500000,

};

/***********************************************************************
* spi_led_transfer - This function is used to transfer data to the spi
* 	bus.
***********************************************************************/
static void spi_led_transfer(unsigned char ch1, unsigned char ch2)
{
    int ret=0;
    ch_tx[0] = ch1;
    ch_tx[1] = ch2;
	gpio_set_value_cansleep(15, 0);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(spidev_global->spi, &m);
	gpio_set_value_cansleep(15, 1);
	return;
}

/***********************************************************************
* spi_led_open - This function is called when the device is first
* 	opened.
***********************************************************************/
static int spi_led_open(struct inode *inode, struct file *filp)
{
	unsigned char temp=0;
	gpio_set_value_cansleep(15, 0);
	spi_led_transfer(0x0F, 0x00);
	spi_led_transfer(0x09, 0x00);
	spi_led_transfer(0x0A, 0x04);
	spi_led_transfer(0x0B, 0x07);
	spi_led_transfer(0x0C, 0x01);

	//To Clear the LED Display
	for(temp=1; temp < 9; temp++)
	{
		spi_led_transfer(temp, 0x00);

	}
	gpio_set_value_cansleep(15, 1);

	return 0;
}


/***********************************************************************
* spi_led_ioctl - This function is used to set the buffer with user
* 	defined patterns.
***********************************************************************/
static long spi_led_ioctl(struct file *filp, unsigned int arg, unsigned long cmd)
{
	int ret;
	int i=0;
	int j=0;
	char write_Buff[12][8];

	ret = copy_from_user((void *)&write_Buff, (void * __user)arg, sizeof(write_Buff));
	if(ret!= 0)
	{
		printk("Failure .\n");
	}
	for(i=0;i<12;i++)
	{
		for(j=0;j<8;j++)
		{
			spidev_global->pattern_buffer[i][j] = write_Buff[i][j];
		}
	}
	return ret;
}

/***********************************************************************
* thread_spi_led_write - This function is called by the kthread to write
* patterns to the LED Display.
***********************************************************************/
int thread_spi_led_write(void *data)
{
	int i=0;
	int j=0;
	int k=0;

	if(spidev_global->sequence_buffer[0][0] == 0 && spidev_global->sequence_buffer[0][1] == 0)
	{	gpio_set_value_cansleep(15, 0);
		for(k=1; k < 9; k++)
		{
			spi_led_transfer(k, 0x00);
		}
		gpio_set_value_cansleep(15, 1);
		busyFlag = 0;
		goto sequenceEnd;
	}

	for(j=0;j<6;j++) //looping according to the sequence order
	{
		for(i=0;i<12;i++)//looping according to the pattern number
		{
			if(spidev_global->sequence_buffer[j][0] == i)
			{
				if(spidev_global->sequence_buffer[j][0] == 0 && spidev_global->sequence_buffer[j][1] == 0)
				{
					gpio_set_value_cansleep(15, 0);
					for(k=1; k < 9; k++)
					{
						spi_led_transfer(k, 0x00);
					}
					gpio_set_value_cansleep(15, 1);
					busyFlag = 0;
					goto sequenceEnd;
				}
				else
				{
					gpio_set_value_cansleep(15, 0);
					spi_led_transfer(0x01, spidev_global->pattern_buffer[i][0]);
					spi_led_transfer(0x02, spidev_global->pattern_buffer[i][1]);
					spi_led_transfer(0x03, spidev_global->pattern_buffer[i][2]);
					spi_led_transfer(0x04, spidev_global->pattern_buffer[i][3]);
					spi_led_transfer(0x05, spidev_global->pattern_buffer[i][4]);
					spi_led_transfer(0x06, spidev_global->pattern_buffer[i][5]);
					spi_led_transfer(0x07, spidev_global->pattern_buffer[i][6]);
					spi_led_transfer(0x08, spidev_global->pattern_buffer[i][7]);
					gpio_set_value_cansleep(15, 1);
					msleep(spidev_global->sequence_buffer[j][1]);
				}
			}
		}
	}
	sequenceEnd:
	busyFlag = 0;
	return 0;
}

/***********************************************************************
* spi_led_write - This function is used to send data over SPI bus to the
* 	LED Display.

***********************************************************************/
static ssize_t spi_led_write(struct file *filp, const char *buff, size_t count, loff_t *ppos)
{
	int i=0;
	int j=0;
	int ret= 0;
	unsigned  int seq_Buff[20];
	struct task_struct *task;

	if(busyFlag == 1)
	{
		return -EBUSY;
	}
	if (count > bufsiz)
	{
		return -EMSGSIZE;
	}
	ret = copy_from_user((void *)&seq_Buff, (void * __user)buff, sizeof(seq_Buff));
	for(i=0;i<20;i=i+2)
	{
		j=i/2;
		spidev_global->sequence_buffer[j][0] = seq_Buff[i];
		spidev_global->sequence_buffer[j][1] = seq_Buff[i+1];
	}
	if(ret != 0)
	{
		printk("Failure\n");
	}

	busyFlag = 1;
    task = kthread_run(&thread_spi_led_write, (void *)seq_Buff,"kthread_spi_led");

	return ret;
}

/***********************************************************************
* spi_led_release - This function is called to release all data
* 	structures that were used up by open function.
***********************************************************************/
static int spi_led_release(struct inode *inode, struct file *filp)
{
    unsigned char temp=0;
    int ret = 0;
    busyFlag = 0;
    //Clear the LED Display on releasing
	gpio_set_value_cansleep(15, 0);

	for(temp=1; temp < 9; temp++)
	{
		spi_led_transfer(temp, 0x00);
	}

	gpio_set_value_cansleep(15, 1);

	printk("Spidev is closing\n");
	return ret;
}

/***********************************************************************
* Driver file operations
***********************************************************************/
static struct file_operations spi_led_fops = {
	.owner   			= THIS_MODULE,
	.write   			= spi_led_write,
	.open    			= spi_led_open,
	.release 			= spi_led_release,
	.unlocked_ioctl   = spi_led_ioctl,
};

/***********************************************************************
* spidev_probe - This is the probe function. It gets called when device
* 	is to be initiallized or new device is being getting added.
***********************************************************************/
static int spidev_probe(struct spi_device *spi)
{
	int status = 0;
	struct device *dev;

	spidev_global = kzalloc(sizeof(*spidev_global), GFP_KERNEL);
	if(!spidev_global)
	{
		return -ENOMEM;
	}

	/* Initializing the driver data */
	spidev_global->spi = spi;
	spidev_global->devt = MKDEV(MAJOR_NUMBER, MINOR_NUMBER);
    dev = device_create(spi_led_class, &spi->dev, spidev_global->devt, spidev_global, DEVICE_NAME);

    if(dev == NULL)
    {
		printk("Device Creation Failed\n");
		kfree(spidev_global);
		return -1;
	}
	printk("SPI LED Driver Probed.\n");
	return status;
}

/***********************************************************************
* spidev_probe - This is the remove function. It gets called when device
* 	is disconnected.
***********************************************************************/
static int spidev_remove(struct spi_device *spi)
{
	int ret=0;

	device_destroy(spi_led_class, spidev_global->devt);
	kfree(spidev_global);
	printk("SPI LED Driver Removed.\n");
	return ret;
}

static struct spi_driver spi_led_driver = {
	.driver = {
		.name =         DRIVER_NAME,
		.owner =        THIS_MODULE,
	},
	.probe =        spidev_probe,
	.remove =       spidev_remove,
};

/***********************************************************************
* spi_led_init - This function is called to initialize the SPI Bus and
* 	LED Display.

***********************************************************************/
static int __init spi_led_init(void)
{
	int retValue;
	int ret;
	busyFlag = 0;
	//Register the Device
	retValue = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &spi_led_fops);
	if(retValue < 0)
	{
		printk("Device Registration Failed\n");
		return -1;
	}

	//Create the class
	spi_led_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
	if(spi_led_class == NULL)
	{
		printk("Class Creation Failed\n");
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}

	//Register the Driver
	retValue = spi_register_driver(&spi_led_driver);
	if(retValue < 0)
	{
		printk("Driver Registraion Failed\n");
		class_destroy(spi_led_class);
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}

	ret=gpio_request_one(24, GPIOF_OUT_INIT_LOW , "gpio13");
	ret=gpio_request(44,"IO 2 MUX");
	ret=gpio_request(46, "IO 2 MUX");
	ret=gpio_request(15, "IO 2 MUX");
	ret=gpio_request(42, "IO 2 MUX");
	ret=gpio_request(30, "IO 2 MUX");
	ret=gpio_request(72, "IO 2 MUX");

	gpio_direction_output(46,0);
	gpio_direction_output(15,0);
	gpio_direction_output(44,0);
	gpio_direction_output(42,0);
	gpio_direction_output(30,0);

	gpio_set_value_cansleep(72, 0);
	gpio_set_value_cansleep(15, 1);
	gpio_set_value_cansleep(72, 0);
	gpio_set_value_cansleep(44, 1);
	gpio_set_value_cansleep(46, 1);
	gpio_set_value_cansleep(24, 0);
	gpio_set_value_cansleep(42, 0);
	gpio_set_value_cansleep(30, 0);

	printk("SPI LED Driver Initialized.\n");
	return retValue;
}

/***********************************************************************
* spi_led_exit - This function is called when the driver is about to
* exit.
***********************************************************************/
static void __exit spi_led_exit(void)
{
	spi_unregister_driver(&spi_led_driver);
	class_destroy(spi_led_class);
	unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
	printk("SPI LED Driver exiting.\n");
}
MODULE_AUTHOR("The Author");
MODULE_DESCRIPTION("SPI_LED_Driver");
MODULE_LICENSE("GPL");

module_init(spi_led_init);
module_exit(spi_led_exit);
