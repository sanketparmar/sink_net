#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void main()
{
        char buff[256];

	int fd = open("/proc/sink_net_tx_status", O_RDWR | O_TRUNC);
	if(fd == -1) {
		printf("Failed to open procfs file.\n");
		exit(1);
	}

	printf("Initial status...\n");
	read(fd, buff, 256);
	puts(buff);

	printf("Reseting status to 0...\n");
	lseek(fd, 0, SEEK_SET);
	write(fd, "0 0", 3);


	printf("After reset...\n");
	lseek(fd, 0, SEEK_SET);
	read(fd, buff, 256);
	puts(buff);


	printf("Waiting for 10 secs\n");
	sleep(10);

	printf("After wait...\n");
	lseek(fd, 0, SEEK_SET);
	read(fd, buff, 256);
	puts(buff);
}
