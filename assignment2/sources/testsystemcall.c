#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv) {
	//printf("Calling ... \n");
	//syscall(__NR_hellokernel,42);
	/*char *ptr = "hello";
	int out = syscall(__NR_dm510_msgbox_put, ptr, 4);
	printf("done...%d \n",out);
	out = syscall(__NR_dm510_msgbox_put, NULL, 4);
	printf("done1...%d \n",out);
	out = syscall(__NR_dm510_msgbox_put, ptr, -1);
	printf("done1...%s \n",strerror(abs(out)));
*/
	char *ptr = "hello";
	printf("TESTING THE PUT FUNCTION:\n");

	//succesful test:
	printf("Test 1, string:\"hello\" length:6 \n");
	int out = syscall(__NR_dm510_msgbox_put, ptr, 6);
	printf("%s \n\n",strerror(abs(out)));

	sleep(2);
	//allocating negative space:
	printf("Test 2, string:\"hello\" length:-1 \n");
	out = syscall(__NR_dm510_msgbox_put, ptr, -1);
	printf("%s %d\n\n",strerror(abs(out)),out);

	sleep(2);
	//putting in non-existing string:
	printf("Test 3, string:NULL length:0 \n");
	out = syscall(__NR_dm510_msgbox_put, NULL, 0);
	printf("%s \n\n\n\n",strerror(abs(out)));


	sleep(5);
	printf("TESTING THE GET FUNCTION:\n");

	char msg[50];
	//successfull pull
	printf("Test 4, length:6 \n");
	out = syscall(__NR_dm510_msgbox_get, msg, 50);
	if (out < 0) {
		printf("%s \n\n",strerror(abs(out)));
	}
	printf("%s\n\n",msg);

	sleep(2);
	//trying to pull when kernel is empty
	printf("Test 6, kernel is empty, length:-1 \n");
	out = syscall(__NR_dm510_msgbox_get, msg, 50);
	printf("%s \n\n",strerror(abs(out)));


	sleep(2);
	out = syscall(__NR_dm510_msgbox_put, ptr, 7);
	//trying to pull with to short length
	printf("Test 5, length to short,  length:1 \n");
	out = syscall(__NR_dm510_msgbox_get, ptr, 1);
	printf("%s \n\n",strerror(abs(out)));


}
