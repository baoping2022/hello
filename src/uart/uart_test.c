#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define RECV_BUFF_LEN   64
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
		perror("fcntl failed!\n");
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

int uart_write(int uart_fd, const char *pdata, int max_size)
{
	int ret = 0;

	if (uart_fd > 0) {
		ret = write(uart_fd, pdata, max_size);

		if (ret < 0)
			printf("write data");
		else
			printf("Write %d bytes done.\n", max_size);
	}

	return ret;
}

static void help(void)
{
	fprintf(stderr,
		"Usage:\n"
		"   serial -r -d DEVICE -l SIZE [-f FILE]\n"
		"   serial -w -d DEVICE -t TEXT\n"
		"Options:\n"
		"   -d (open device)\n"
		"   -l (read data length)\n"
		"   -f (save read data to file)\n"
		"   -t (text data to write)\n"
		"   -w (write operation)\n"
		"   -r (read operation)\n"
		"   -h (Help message)\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd = 0;
	FILE *w_fd = NULL;
	int opt;
	int longindex = 0;
	char *g_tmp_file = NULL;
	char *g_text = NULL;
	char buf[128] = {0};
	char newline[] = "\r\n";
	int ret = 0;
	char *g_device = NULL;
	bool g_is_read;
	long int g_length = 0;

	struct option long_options[] = {
		{"device", required_argument, NULL, 'd'},
		{"length", required_argument, NULL, 'l'},
		{"file", required_argument, NULL, 'f'},
		{"text", required_argument, NULL, 't'},
		{"write", no_argument, NULL, 'w'},
		{"read", no_argument, NULL, 'r'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, "d:l:f:t:wrh", long_options,
					&longindex)) != -1) {
		switch (opt) {
		case 'd':
			g_device = strdup(optarg);
			break;

		case 'l': {
				char *endptr;
				g_length = strtol(optarg, &endptr, 10);
				break;
			}

		case 'f':
			g_tmp_file = strdup(optarg);
			break;

		case 't':
			g_text = strdup(optarg);
			break;

		case 'w':
			g_is_read = false;
			break;

		case 'r':
			g_is_read = true;
			break;

		case 'h':
			help();
			break;

		case '?':
			printf("Unknown option\n");
			break;

		default:
			printf("Unsupported options\n");
			break;
		}
	}

	fd = open_port(g_device);

	if (fd < 0) {
		printf("open %s err!\n", g_device);
		ret = -1;
		goto lable_out;
	}

	if (set_opt(fd, 115200, 8, 'N', 1) < 0) {
		perror("set port err!\n");
		ret = -1;
		goto lable_out;
	}

	if (g_tmp_file) {
		w_fd = fopen(g_tmp_file, "w+");

		if (w_fd == NULL) {
			printf("Fail to Open File %s\n", g_tmp_file);
			ret = -1;
			goto lable_out;
		}
	}

	if (g_is_read) {
		read_data(fd, g_length);

		if (g_tmp_file) {
			sprintf(buf, "%s", g_recv_buf);
			fwrite(buf, 1, strlen(buf), w_fd);
		} else
			printf("%s\n", g_recv_buf);
	} else {
		sprintf(buf, "%s%s", g_text, newline);

		if (uart_write(fd, buf, strlen(buf)) < 0) {
			ret = -1;
			goto lable_out;
		}
	}

lable_out:

	if (g_tmp_file) {
		fclose(w_fd);
		free(g_tmp_file);
	}

	if (fd) {
		close(fd);
		free(g_device);
	}

	return ret;
}

