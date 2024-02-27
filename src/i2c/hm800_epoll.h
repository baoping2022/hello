#ifndef HM800_EPOLL_H
#define HM800_EPOLL_H

typedef int (*private_callback_t)(int fd, void *data);
struct private_data {
	private_callback_t cb;
	void *data;
	int fd;
};

int epoll_init(void);
int epoll_add_fd(int epollfd, int fd, private_callback_t cb, void *data);
int epoll_event_handle(int epollfd);
int epoll_del_fd(int epollfd, int fd);
void epoll_final(int epollfd);

#endif //HM800_EPOLL_H