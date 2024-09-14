int trigger_sensor(int fd)
{
  char* Buffer;
  int ret=0;

  Buffer = (char *)malloc(20);
  while(1)
  {
    ret = write(fd, Buffer, 20);
    if(ret == 0)
    {
      break;
    }
    usleep(100000);
  }
  free(Buffer);
  return ret;
}

int echo_read(int fd)
{
  unsigned int temp =0;
  int ret=0;

  while(1)
  {
    ret = read(fd, &temp, sizeof(temp));
    if(ret == 0)
    {
      break;
    }
    usleep(100000);
  }
  return temp;
}

int spi_led_write(int fd, unsigned int seq_buff[20])
{
  int ret=0;
  while(1)
  {
    ret = write(fd, seq_buff, sizeof(seq_buff));
    if(ret == 0)
    {
      break;
    }
    usleep(100000);
  }
  return ret;
}

int spi_led_ioctl(int fd, char pat_buff[10][8])
{
  int ret=0;
  while(1)
  {
    ioctl(fd,(unsigned int) pat_buff, sizeof(pat_buff));
    if(ret == 0)
    {
      break;
    }
    usleep(100000);
  }
  return ret;
}
