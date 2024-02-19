// SPDX-License-Identifier: GPL-2.0
/*
 * Author:baoping.fan 
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <termios.h>
#include <stdlib.h>

#define RECV_BUFF_LEN	64
static unsigned char g_recv_buf[RECV_BUFF_LEN];

/* set_opt(fd, 115200, 8, 'N', 1) */
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio, oldtio;

	if (tcgetattr(fd, &oldtio) != 0) {
		perror("SetupSerial 1");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/

	newtio.c_iflag  &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL |
			IXON);
	newtio.c_cflag  &= ~CRTSCTS;

	switch (nBits) {
	case 7:
		newtio.c_cflag |= CS7;
		break;

	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch (nEvent) {
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;

	case 'E':
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;

	case 'N':
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (nSpeed) {
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;

	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;

	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;

	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;

	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}

	if (nStop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		newtio.c_cflag |= CSTOPB;

	newtio.c_cc[VMIN]  = 1;
	newtio.c_cc[VTIME] = 0;

	tcflush(fd, TCIFLUSH);

	if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
		perror("com set error");
		return -1;
	}

	/* printf("set done!\n"); */
	return 0;
}

int open_port(char *com)
{
	int fd;

	fd = open(com, O_RDWR | O_NOCTTY);

	if (-1 == fd)
		return -1;

	if (fcntl(fd, F_SETFL, 0) < 0) {
		printf("fcntl failed!\n");
		return -1;
	}

	return fd;
}

int uart_read(int uart_fd, int max_size, int *index)
{
	int ret = 0;
	char c;

	if (uart_fd > 0) {
		ret = read(uart_fd, &c, max_size);

		if (ret < 0)
			printf("Failed(%d:%s) to read from uart.\n", errno, strerror(errno));

		if (ret == 1)
			g_recv_buf[(*index)++] = c;
	}

	return ret;
}

void read_data(int uart_fd, unsigned int size)
{
	int ret = 0;
	unsigned int total = 0;
	int index = 0;

	while (1) {
		ret = uart_read(uart_fd, 1, &index);

		if (ret < 0)
			break;

		total += ret;

		if (total >= size || index >= RECV_BUFF_LEN)
			break;
	}
}

int main(int argc, char **argv)
{
	int fd;
	FILE *w_fd;
	int iret;
	char buf[128] = {0};

	if (argc != 3 && argc != 4) {
		printf("Usage:\n");
		printf("%s </dev/ttyS1> <length> [file].\n", argv[0]);
		printf("%s /dev/ttyS1 4.\n", argv[0]);
		printf("%s /dev/ttyS1 4 /tmp/t_file.\n", argv[0]);

		return -1;
	}

	fd = open_port(argv[1]);

	if (fd < 0) {
		printf("open %s err!\n", argv[1]);
		return -1;
	}

	iret = set_opt(fd, 115200, 8, 'N', 1);

	if (iret) {
		perror("set port err!\n");
		return -1;
	}

	if (argc == 4) {
		w_fd = fopen(argv[3], "w+");

		if (w_fd == NULL) {
			printf("Fail to Open File %s\n", argv[3]);
			return -1;
		}
	}

	read_data(fd, atoi(argv[2]));

	if (argc == 3)
		printf("%s\n", g_recv_buf);

	else {
		sprintf(buf, "%s", g_recv_buf);
		fwrite(buf, 1, strlen(buf), w_fd);
	}

	if (argc == 4)
		fclose(w_fd);

	close(fd);

	return 0;
}