#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEBUG 0

int main(int argc, char *argv[])
{
  printf("start\n");
  int fd, n, i;
  char rbuf[2];
  char buf[] = "*Finance Minister Arun Jaitley Tuesday hit out at former RBI governor Raghuram Rajan for predicting that the next banking crisis would be triggered by MSME lending, saying postmortem is easier than taking action when it was required. Rajan, who had as the chief economist at IMF warned of impending financial crisis of 2008, in a note to a parliamentary committee warned against ambitious credit targets and loan waivers, saying that they could be the sources of next banking crisis. Government should focus on sources of the next crisis, not just the last one.* *In particular, government should refrain from setting ambitious credit targets or waiving loans. Credit targets are sometimes achieved by abandoning appropriate due diligence, creating the environment for future NPAs,' Rajan said in the note.' Both MUDRA loans as well as the Kisan Credit Card, while popular, have to be examined more closely for potential credit risk. Rajan, who was RBI governor for three years till September 2016, is currently.*";
  struct termios toptions;
  printf("start\n");
  /* open serial port */
  fd = open("/dev/ttyS4", O_RDWR | O_NOCTTY);
  printf("fd opened as %i\n", fd);
  printf("Serial Open Passed!\n");
  /* wait for the Arduino to reboot */
  usleep(3500000);

  /* get current serial port settings */
  tcgetattr(fd, &toptions);
  /* set 9600 baud both ways */
  cfsetispeed(&toptions, B2400);
  cfsetospeed(&toptions, B2400);
  /* 8 bits, no parity, no stop bits */
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  /* Canonical mode */
  toptions.c_lflag |= ICANON;
  /* commit the serial port settings */
  tcsetattr(fd, TCSANOW, &toptions);

  /* Send byte to trigger Arduino to send string back */
  write(fd, buf, strlen(buf)+1);
  /* Receive string from Arduino */
  n = read(fd, rbuf, 1);
  printf("read bytes:%d\n", n);
  /* insert terminating zero in the string */
  rbuf[n] = 0;

  printf("%i bytes read, buffer contains: %s\n", n, rbuf);

  if(DEBUG)
    {
      printf("Printing individual characters in buf as integers...\n\n");
      for(i=0; i<n; i++)
    {
      printf("Byte %i:%i, ",i+1, (int)buf[i]);
    }
      printf("\n");
    }

  return 0;
}
