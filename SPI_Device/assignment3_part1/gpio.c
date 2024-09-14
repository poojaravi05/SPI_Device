/*
 * THIS SOFTWARE IS ADOPTED FROM RIDGERUN's GPIO_INT_TEST.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "gpio.h"

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[MAX_BUF];

	 snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction ");
		printf("%d\n",gpio );
		return fd;
	}

	if (out_flag)
		write(fd, "out", 3);
	else
		write(fd, "in", 2);

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}

	if (value)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd;
	char buf[MAX_BUF];
	char ch;

	 snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}

	write(fd, edge, strlen(edge) + 1);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_fd_open  for value
 ****************************************************************/

int gpio_fd_open(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY | O_WRONLY| O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_open_read  for value
 ****************************************************************/

int gpio_fd_open_read(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_open_edge
 ****************************************************************/

int gpio_fd_open_edge(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

	fd = open(buf, O_RDONLY | O_WRONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open_edge");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int gpio_fd_close(int fd)
{
	return close(fd);
}

int mux_gpio_set(unsigned int gpio, unsigned int value)
{
	gpio_export(gpio);
	//gpio_set_dir(gpio, 1);
	gpio_set_value(gpio, value);

	return 0;
}

/****************************************************************
 * Retrive the corresponding GPIO pins for the given IO pins
   and multiplex them
 ****************************************************************/

int gpio_pins(int io)
{
	int gpio;

	switch(io)
	{
		case 0:
		{
			mux_gpio_set(32,0);
			gpio=11;
			break;
		}
		case 1:
		{
			mux_gpio_set(28,0);
			mux_gpio_set(45,0);
			gpio=12;
			break;
		}
		case 2:
		{
			mux_gpio_set(77,0);
			mux_gpio_set(34,0);
			gpio=13;
			break;
		}
		case 3:
		{
			mux_gpio_set(16,0);
			mux_gpio_set(76,0);
			mux_gpio_set(64,0);
			gpio=14;
			break;
		}
		case 4:
		{
			mux_gpio_set(36,0);
			gpio=6;
			break;
		}
		case 5:
		{
			mux_gpio_set(18,0);
			mux_gpio_set(66,0);
			gpio=0;
			break;
		}
		case 6:
		{
			mux_gpio_set(20,0);
			mux_gpio_set(68,0);
			gpio=1;
			break;
		}
		case 7:
		{
			gpio=38;
			break;
		}
		case 8:
		{
			gpio=40;
			break;
		}
		case 9:
		{
			mux_gpio_set(22,0);
			mux_gpio_set(70,0);
			gpio=4;
			break;
		}
		case 10:
		{
			mux_gpio_set(26,0);
			mux_gpio_set(74,0);
			gpio=10;
			break;
		}
		case 11:
		{
			mux_gpio_set(24,0);
			mux_gpio_set(44,0);
			mux_gpio_set(72,0);
			gpio=5;
			break;
		}
		case 12:
		{
			mux_gpio_set(42,0);
			gpio=15;
			break;
		}
		case 13:
		{
			mux_gpio_set(46,0);
			mux_gpio_set(30,0);
			gpio=7;
			break;
		}
		default:
		{
			printf("INVALID IO PIN...\n");
			return -1;
		}
	}
	return gpio;
}
