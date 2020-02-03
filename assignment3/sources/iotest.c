#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "ioct.h"


int main(int argc,char **argv){
  char *filename = argv[1];
  int fd = open(filename, O_RDWR);
  int arg = atoi(argv[3]);
  int result = 0;
  char *IO_arg=argv[2];
  if (!strcmp("buffersize", IO_arg)) {
    result = ioctl(fd, IOC_RESETBUFFER, arg);
    printf("changed buffer size to: %d\n", result);
  }else if (!strcmp("processes", IO_arg)){
    result = ioctl(fd, IOC_RESETPROC, arg);
    printf("changed number of max processes to: %d\n", result);
  }else{
    printf("input error\n");
  }
  close(fd);
}
