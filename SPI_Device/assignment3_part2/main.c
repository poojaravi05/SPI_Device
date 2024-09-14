#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/types.h>
#include <time.h>
#include <poll.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include"fun.c"
#define SPI_DEVICE_NAME "/dev/spidev1.1"
#define PULSE_DEVICE_NAME "/dev/USonic"

typedef struct
{
	int threadId;
	int fd;
}paramaters;

pthread_mutex_t mutex;

double distance;
unsigned int sendbuff[20];

void *transmit_led(void *data)
{
	int fd;
	int i;

    char pattern [12][8] = {
		{ 0x7C, 0x7E, 0x13, 0x13, 0x7E, 0x7C, 0x00, 0x00 }, // 'A'
		{ 0x41, 0x7F, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00 }, // 'B'
		{ 0x1C, 0x3E, 0x63, 0x41, 0x41, 0x63, 0x22, 0x00 }, // 'C'
		{ 0x41, 0x7F, 0x7F, 0x41, 0x63, 0x3E, 0x1C, 0x00 }, // 'D'
		{ 0x41, 0x7F, 0x7F, 0x49, 0x5D, 0x41, 0x63, 0x00 }, // 'E'
		{ 0x41, 0x7F, 0x7F, 0x49, 0x1D, 0x01, 0x03, 0x00 }, // 'F'
		{ 0x1C, 0x3E, 0x63, 0x41, 0x51, 0x73, 0x72, 0x00 }, // 'G'
		{ 0x7F, 0x7F, 0x08, 0x08, 0x7F, 0x7F, 0x00, 0x00 }, // 'H'
		{ 0x00, 0x41, 0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00 }, // 'I'
		{ 0x30, 0x70, 0x40, 0x41, 0x7F, 0x3F, 0x01, 0x00 }, // 'J'
		{ 0x00, 0xFF, 0xFF, 0x1C, 0x36, 0x63, 0xC3, 0x00 }, // 'K' this is displayed only beyond 50 cms
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	};

	//printf("Distance value before open is : %lf",distance);
	fd = open(SPI_DEVICE_NAME, O_RDWR);
	if(fd < 0)
	{
		printf("Can not open device file fd_spi.\n");
		return 0;
	}
	spi_led_ioctl(fd, pattern);
	sendbuff[4]=0;
	sendbuff[5]=0;

	while(1)
	{
		pthread_mutex_lock(&mutex);
		i=distance; // reading global value to local varialble
		pthread_mutex_unlock(&mutex);

		if(i<10)
		{
			sendbuff[0]=0;sendbuff[1]=50;
			sendbuff[2]=1;sendbuff[3]=50;
			spi_led_write(fd, sendbuff);
		   	usleep(10000);
		}

		else if(i<20)
		{	
			sendbuff[0]=2;sendbuff[1]=100;
			sendbuff[2]=3;sendbuff[3]=100;
			spi_led_write(fd, sendbuff);
			usleep(10000);
		}
		else if(i<30)
		{
			sendbuff[0]=4;sendbuff[1]=150;
			sendbuff[2]=5;sendbuff[3]=150;
			spi_led_write(fd, sendbuff);
			usleep(10000);
		}
		else if(i<40)
		{
			sendbuff[0]=6;sendbuff[1]=200;
			sendbuff[2]=7;sendbuff[3]=200;
			spi_led_write(fd, sendbuff);
			usleep(10000);
		}
		else if(i<50)
		{
			sendbuff[0]=8;sendbuff[1]=250;
			sendbuff[2]=9;sendbuff[3]=250;
			spi_led_write(fd, sendbuff);
			usleep(10000);
		}
		else
		{
			sendbuff[0]=10;sendbuff[1]=300;
			sendbuff[2]=11;sendbuff[3]=800;
			spi_led_write(fd, sendbuff);
			usleep(10000);
		}

	}
	printf("thread_transmit_spi fd = %d\n",fd);
	close(fd);
	pthread_exit(0);
}

void *pulse_function(void *data)
{
	int fd;
	int pulseWidth;
	fd = open(PULSE_DEVICE_NAME, O_RDWR);
	while(1)
	{
		trigger_sensor(fd);
		pulseWidth = echo_read(fd);
		pthread_mutex_lock(&mutex);
		distance = pulseWidth * 34/(2*1000);
		pthread_mutex_unlock(&mutex);
		printf("distance = %d cm\n", (int)distance);
		usleep(100000);
	}
	close(fd);
	pthread_exit(0);
}

int main(int argc, char **argv, char **envp)
{
	int ret;
	pthread_t spi_thread, distance_thread;
	paramaters *spi_param, *dist_param;

	//  Initializating the Mutex
	pthread_mutex_init(&mutex, NULL);

	dist_param = malloc(sizeof(paramaters));
	dist_param -> threadId = 101;

	ret = pthread_create(&distance_thread, NULL, &pulse_function, (void*)dist_param);
	if(ret)
	{
		printf("ERROR; return code from pthread_create() is %d\n", ret);
		exit(-1);
	}
	sleep(1);

	spi_param = malloc(sizeof(paramaters));
	spi_param -> threadId = 100;

	ret = pthread_create(&spi_thread, NULL, &transmit_led, (void*)spi_param);

	if(ret)
	{
		printf("ERROR; return code from pthread_create() is %d\n", ret);
		exit(-1);
	}

  	pthread_join(distance_thread, NULL);
	pthread_join(spi_thread, NULL);

	free(spi_param);
	free(dist_param);

	return 0;
}
