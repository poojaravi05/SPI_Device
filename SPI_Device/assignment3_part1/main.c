#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include<math.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <time.h>
#include <poll.h>
#include <pthread.h>
#include <inttypes.h>
#include "gpio.c"
#define DEVICE "/dev/spidev1.0"

pthread_t usensor_thread;
pthread_t  led_matrix_thread;// led_matrix_thread;
pthread_mutex_t lock;

long distance = 1;
long delay;
int m_d;
int spi_fd;

// to convert 32 bit timestamp output into 64 bits
uint64_t tsc(void)  
{
	uint32_t a, d;
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	return (( (uint64_t)a)|( (uint64_t)d)<<32 );
}

uint8_t cat_array[2];
uint8_t cat_right_walk [] = {
	0x01,
	0x3E,
	0x02,
	0xF0,
	0x03,
	0x70,
	0x04,
	0xFC,
	0x05,
	0x68,
	0x06,
	0xF8,
	0x07,
	0x68,
	0x08, 0xBC,
    0x09, 0x00,
    0x0A, 0x04,
    0x0B, 0x07,
    0X0C, 0x01,
};

uint8_t cat_right_run [] = {
	0x01, 0x9C,
	0x02, 0x72,
	0x03, 0xF0,
	0x04, 0x7E,
	0x05, 0xF4,
	0x06, 0x7C,
	0x07, 0xF4,
	0x08, 0x1E,
	0x09, 0x00,
	0x0A, 0x04,
	0x0B, 0x07,
	0X0C, 0x01,
};
uint8_t cat_left_walk [] = {
	0x08, 0x3E,
	0x07, 0xF0,
	0x06, 0x70,
	0x05, 0xFC,
	0x04, 0x68,
	0x03, 0xF8,
	0x02, 0x68,
	0x01, 0xBC,
	0x09, 0x00,
  	0x0A, 0x04,
  	0x0B, 0x07,
  	0X0C, 0x01,
};

uint8_t cat_left_run [] = {
	0x08, 0x9C,
	0x07, 0x72,
	0x06, 0xF0,
	0x05, 0x7E,
	0x04, 0xF4,
	0x03, 0x7C,
	0x02, 0xF4,
	0x01, 0x1E,
	0x09, 0x00,
	0x0A, 0x04,
	0x0B, 0x07,
	0X0C, 0x01,
};

void setmux()
{ 
	// level shifter in pin 2 ,3, gpio 2,3 export
	gpio_export(61);
	gpio_export(62);
	//3 is in 2 is out
	gpio_set_dir(61,1);
	gpio_set_dir(62,0);
	gpio_set_value(61,0);
	//direction in mux disabled as attribute doesnt exist
	mux_gpio_set(77,0);
	mux_gpio_set(76,0);
	mux_gpio_set(64,0);
}


void displaymux()
{
	//export level shifter 11 12 13 & gpio 15
	gpio_export(24);
	gpio_export(42);gpio_export(43);
	gpio_export(30);gpio_export(31);
	gpio_export(15);
	//set direction level shifter 11 12 13 & gpio 15
	gpio_set_dir(24,1);
	gpio_set_dir(42,1);gpio_set_dir(43,1);
	gpio_set_dir(30,1);gpio_set_dir(31,1);
	gpio_set_dir(15,1);
	//value level shifter 11 12 13 & gpio 15
	gpio_set_value(24,0);
	gpio_set_value(42,0); gpio_set_value(43,0);
	gpio_set_value(30,0);gpio_set_value(31,0);
	//mux for 11 12 13
	mux_gpio_set(44,1);
	mux_gpio_set(72,0);
	mux_gpio_set(46,1);
}

void* display_function(void* arg)
{
	int k, i;
	double distance_previous = 0, distance_current = 0;
	int direction = 0;
	displaymux();
	struct spi_ioc_transfer tr =
	{
		.tx_buf = (unsigned long)cat_array,
		.rx_buf = 0,
		.len = 2,
		.delay_usecs = 1,
		.speed_hz = 10000000,
		.bits_per_word = 8,
		.cs_change = 1,
	};
	spi_fd= open(DEVICE,O_RDWR);

	if(spi_fd==-1)
	{
     	printf("error in opening file %s\n", DEVICE);
     	exit(-1);
	}

	while(1)
	{
		i = 0;
		k= 0;
		//m_d is a global variable  which is used to calculate delay & set the direction
		pthread_mutex_lock(&lock);
		distance_current = m_d;
		pthread_mutex_unlock(&lock);
		// upper limit for delay and hence animation speed
		if (distance_current > 90)
		{
			delay=350000;
		}
		// lower limit
		else if (distance_current < 70)											
		{
			delay=30000;
		}
		// Linear mapping
		else
		{
			delay = 14250000/m_d;
		}							
		//logic for direction
		if((distance_previous - distance_current)>3)
		{
			direction=1;
		}
		else if((distance_previous - distance_current) < -3)
		{
			direction=0;
		}
		// cat is facing right
		while (i < 26 && direction==1)	// Switching between two display patterns
		{
			cat_array[0] = cat_right_walk [i];
			cat_array[1] = cat_right_walk [i+1];
			gpio_set_value(15,0);
			ioctl(spi_fd, SPI_IOC_MESSAGE (1), &tr);
			gpio_set_value(15,1);
			i = i + 2;
		}
		usleep(delay);

		while (k < 26 && direction==1 )
		{
			cat_array[0] = cat_right_run [k];
			cat_array[1] = cat_right_run [k+1];
			gpio_set_value(15,0);
			ioctl(spi_fd, SPI_IOC_MESSAGE (1), &tr);
			gpio_set_value(15,1);
			k = k + 2;
		}
		usleep(delay);
		// cat is facing left

		while (i < 26 && direction==0)
		{
			gpio_set_value(15,0);
			cat_array[0] = cat_left_walk [i];
			cat_array[1] = cat_left_walk [i+1];
		  	ioctl(spi_fd, SPI_IOC_MESSAGE (1), &tr);
			i = i + 2;
			gpio_set_value(15,1);
		}
		usleep(delay);

		while (k < 26 && direction==0)
		{
			gpio_set_value(15,0);
			cat_array[0] = cat_left_run [k];
			cat_array[1] = cat_left_run [k+1];
			ioctl(spi_fd, SPI_IOC_MESSAGE (1), &tr);
			k = k + 2;
			gpio_set_value(15,1);
		}
		usleep(delay);
		distance_previous = distance_current; // storing the value just before next iteration
	}
	close (spi_fd);
}

void* sensor_function(void* arg)
{
	int fd,fd2;
	int timeout = 100;
	long times;
	uint64_t fall_time = 0;
	uint64_t rise_time = 0;
	uint64_t diff_time = 0;

	char *buf[MAX_BUF];
	double local_distance;
	struct pollfd fd_structre={0};
	setmux();

	fd = open("/sys/class/gpio/gpio62/edge", O_WRONLY);
	fd2 = open("/sys/class/gpio/gpio62/value", O_RDONLY| O_NONBLOCK);
	fd_structre.fd = fd2;
	fd_structre.events = POLLPRI|POLLERR;
	fd_structre.revents=0;

	while(1)
	{
		//read(fd_structre.fd, buf, 1);
		lseek(fd2, 0, SEEK_SET);
		//read(fd_structre.fd, buf, 1);
		write(fd, "rising",sizeof("rising"));
		gpio_set_value(61,0);
		usleep(3);
		gpio_set_value(61,1);
		rise_time=tsc();
		usleep(10);
		gpio_set_value(61,0);
	 	poll(&fd_structre, 1, timeout);

		if (fd_structre.revents & POLLPRI)
		{
			read(fd_structre.fd, buf, 1);
			lseek(fd2, 0, SEEK_SET);
			write(fd, "falling", sizeof("falling"));
			poll(&fd_structre, 1, timeout);

			if (fd_structre.revents & POLLPRI)
			{
				fall_time=tsc();
				fd_structre.revents=0;
				diff_time = fall_time - rise_time;
				times = (long)diff_time;
				//setting the upper & lower limit
				local_distance = (times*340)/(2*4000000); // to calculate distance in cm
				if(local_distance>115) {
					local_distance=115;
				}
				else if(local_distance<55) {
					local_distance=55;
				}
				//local_distance is local variable & is updated to global variable m_d
				pthread_mutex_lock(&lock);
				m_d=local_distance;
				pthread_mutex_unlock(&lock);
			}

		}
		usleep(60000); //how frequently direction is updated
	}
	close(fd);
	close(fd2);
}

int main()
{
	int err, erc;
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
	    return 1;
	}

  //creating thread for polling
	err = pthread_create(&usensor_thread, NULL, &sensor_function, NULL);

	if (err != 0)
	{
		printf("\ncan't create polling thread\n");
	}
	//creating thread for display mux
	erc = pthread_create(&led_matrix_thread, NULL, &display_function, NULL);

	if (erc != 0)
	{
		printf("\ncan't create display thread\n");
	}

  	pthread_join (led_matrix_thread, NULL);
	pthread_join (usensor_thread, NULL);
  	pthread_mutex_destroy(&lock);

	gpio_unexport(61);
	gpio_unexport(62);
	gpio_unexport(77);
	gpio_unexport(76);
	gpio_unexport(64);
	gpio_unexport(24);
	gpio_unexport(42);
	gpio_unexport(43);
	gpio_unexport(30);
	gpio_unexport(31);
	gpio_unexport(15);
	gpio_unexport(44);
	gpio_unexport(72);
	gpio_unexport(46);
	
	return 0;
}
