#include "ioct.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>






void writing(){
	printf("write to buffer:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	printf("buffer size changed to %d\n",ioctl(fd, IOC_RESETBUFFER, 15));

	//close(fd);
	//fd = open(filename, O_RDWR);
	char * fillText = "hello";
	int count = 5, write_result = 0;
	write_result = write(fd, fillText, count);
	printf("the result of insert to buffer 0: %d\n", write_result);
	printf("\n");
	close(fd);

}

void writing1(){
	printf("write to buffer:\n");
	int fd1 = open("/dev/dm510-1", O_RDWR);
	//printf("buffer size changed to %d\n",ioctl(fd, IOC_RESETBUFFER, 15));

	//close(fd);
	//fd = open(filename, O_RDWR);
	char * fillText1 = "world";
	int count = 5, write_result = 0;
	write_result = write(fd1, fillText1, count);
	printf("the result of insert to buffer 1: %d\n", write_result);

	printf("\n");
	close(fd1);


}




void reading(){
	printf("read from buffer0:\n");
	int fd = open("/dev/dm510-1", O_RDWR);
	int count = 5, read_result = 0;
	char *text;

	read_result = read(fd, text, count);
	printf("result from buffer 0 is %d and line: %s\n", read_result, text);
	printf("\n");
	close(fd);

}


void reading1(){
	printf("read from buffer1:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	int count = 5, read_result = 0;
	char *text1;

	read_result = read(fd, text1, count);
	printf("result from buffer 0 is %d and line: %s\n", read_result, text1);
	printf("\n");
	close(fd);


}


void fullBuffers(){
	printf("buffer is full:\n");
	printf("trying to write 15 bytes to buffer twice:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	//printf("buffer size changed to %d\n",ioctl(fd, IOC_RESETBUFFER, 15));
	//int fd = open(filename, O_RDWR);
	char * fillText = "theBufferIsFull";
	int count = 15, write_result = 0;

	for (size_t i = 0; i < 2; i++) {
		write_result = write(fd, fillText, count);
		if (write_result < 0) {
			printf("%s \n",strerror(errno));
		}else{
			printf("the result of %d insert: %d\n", i, write_result);
		}
	}
	printf("\n");
	close(fd);
}

void readEmptyBuffers(){
	printf("read empty buffer:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	int count = 15, read_result = 0;
	char *text;
		read_result = read(fd, text, count);
	printf("result is %d\n", read_result);
	printf("\n");
	close(fd);
}

void writeNothing(){
	printf("write nothing:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	char * fillText = "";
	int count = 0, write_result = 0;

	printf("buffer size changed to %d\n",count);
	write_result = write(fd, fillText, count);

	if (write_result < 0) {
		printf("%s \n",strerror(errno));
	}else{
		printf("result of writing: %d\n", write_result);
	}
	printf("\n");
	close(fd);
}

void readNothing(){
	printf("read nothing:\n");
	int fd = open("/dev/dm510-0", O_RDWR);
	int count = 0, read_result = 0;
	char *text;
	read_result = read(fd, text, count);
	if (read_result < 0) {
		printf("%s \n",strerror(abs(read_result)));
	}else{
		printf("result is %d\n", read_result);
	}
	printf("\n");
	close(fd);
}


int main (int argc,char **argv){
	writing();
	reading();
	sleep(1);
	writing1();
	reading1();
	sleep(1);
  fullBuffers();

	return 0;
}
