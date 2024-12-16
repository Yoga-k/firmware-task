#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#define DEBUG 0
#define SIZE 2048
int display_speed = 0;
int totalBytes = 0;
int startThread = 1;
sem_t mutex, mutex1;
void displaySpeed()
{
  totalBytes = totalBytes * 8;
  if (totalBytes)
  {
    printf("Receiving from MCU Speed: %d bits/second\n", totalBytes);
  }
  totalBytes = 0;
}
void *timerCallback(void *arg)
{
  int local = 1;
  while (1)
  {
    sleep(1);
    sem_wait(&mutex);
    displaySpeed();
    sem_post(&mutex);
    sem_wait(&mutex1);
    local = startThread;
    sem_post(&mutex1);
    if (local == 0)
    {
      break;
    }
  }
  printf("Thread killed\r\n");
}
int main(int argc, char *argv[])
{
  sem_init(&mutex, 0, 1);
  sem_init(&mutex1, 0, 1);
  pthread_t timerthread;
  pthread_create(&timerthread, NULL, timerCallback, NULL);
  printf("start\n");
  int fd, i;
  int n = 1;

  char rbuf[SIZE] = {0};
  char buf[] = "Finance Minister Arun Jaitley Tuesday hit out at former RBI governor Raghuram Rajan for predicting that the next banking crisis would be triggered by MSME lending, saying postmortem is easier than taking action when it was required. Rajan, who had as the chief economist at IMF warned of impending financial crisis of 2008, in a note to a parliamentary committee warned against ambitious credit targets and loan waivers, saying that they could be the sources of next banking crisis. Government should focus on sources of the next crisis, not just the last one.* *In particular, government should refrain from setting ambitious credit targets or waiving loans. Credit targets are sometimes achieved by abandoning appropriate due diligence, creating the environment for future NPAs,' Rajan said in the note.' Both MUDRA loans as well as the Kisan Credit Card, while popular, have to be examined more closely for potential credit risk. Rajan, who was RBI governor for three years till September 2016, is currently.";
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
  /* set 2400 baud both ways */
  cfsetispeed(&toptions, B2400);
  cfsetospeed(&toptions, B2400);
  /* 8 bits, no parity, no stop bits */
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  /* Canonical mode */
  toptions.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  /* commit the serial port settings */
  tcsetattr(fd, TCSANOW, &toptions);

  /* Send byte to trigger Arduino to send string back */
  for (int i = 0; i < 10; i++)
  {
    // printf("Sending to MCU\r\n");
    write(fd, buf, strlen(buf) + 1);
    printf("sending to MCU\r\n");
    /* Receive string from Arduino */
    // sleep(1);
    int sumbytes = 0;
    char outbuf[SIZE] = {0};
    while (1)
    {
      // printf("Read the bytes:\n");
      if (outbuf[sumbytes - 1] == '\n')
      {
        // printf("break the loop");
        break;
      }
      n = read(fd, rbuf, SIZE);
      memcpy(outbuf + sumbytes, rbuf, SIZE);
      // printf("n: %d\n",n);
      sem_wait(&mutex);
      totalBytes += n;
      sem_post(&mutex);
      sumbytes += n;
      // printf("read bytes:%d |%d| %s|\n", sumbytes, n,rbuf);
    }
    /* insert terminating zero in the string */
    outbuf[sumbytes] = 0;
    // usleep(1000);
  }
  // printf("%i bytes read, buffer contains: %s\n", n, outbuf);
  if (DEBUG)
  {
    printf("Printing individual characters in buf as integers...\n\n");
    for (i = 0; i < n; i++)
    {
      printf("Byte %i:%i, ", i + 1, (int)buf[i]);
    }
    printf("\n");
  }
  sem_wait(&mutex1);
  startThread = 0;
  sem_post(&mutex1);
  pthread_join(timerthread, NULL);
  sem_destroy(&mutex);
  sem_destroy(&mutex1);
  printf("%d\r\n", __LINE__);
  return 0;
}
