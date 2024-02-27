#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "hm800_epoll.h"

static struct private_data **pds;
static unsigned short nrhandler;

int epoll_init(void)
{
    int epfd;

    epfd = epoll_create(1);
    if (epfd < 0) {
        fprintf(stderr, "epoll_create error, Error:[%d:%s]", errno, strerror(errno));
        return -1;
    }

    return epfd;
}

void epoll_final(int epollfd)
{
    if (close(epollfd) < 0)
        fprintf(stderr, "close error, Error:[%d:%s]", errno, strerror(errno));
}

int epoll_del_fd(int epollfd, int fd)
{
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		fprintf(stderr, "epoll_ctl error, Error:[%d:%s]", errno, strerror(errno));
		return -1;
    }
    free(pds[fd]);

	return 0;
}

int epoll_add_fd(int epollfd, int fd, private_callback_t cb, void *data)
{
    int ret;
    struct epoll_event event;
    struct private_data *pd;

    if (fd >= nrhandler) {
        pds = realloc(pds, sizeof(*pds) * (fd + 1));
        if (!pds)
            return -1;
        nrhandler = fd + 1;
    }

    pd = malloc(sizeof(*pd));
    if (!pd)
        return -1;

    pd->data = data;
    pd->cb = cb;
    pd->fd = fd;
    pds[fd] = pd;

    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN;
    event.data.ptr = pd;

    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    if(ret < 0) {
        fprintf(stderr, "epoll_ctl add fd:%d error, Error:[%d:%s]", fd, errno, strerror(errno));
        free(pd);
        return -1;
    }

    return 0;
}

int epoll_event_handle(int epollfd)
{
    int i = 0;
    int fd_cnt = 0;
    int sfd;
    struct epoll_event events[1];
    struct private_data *pd;

    if (epollfd < 0)
		return -1;

    memset(events, 0, sizeof(events));
    while(1)
    {
        fd_cnt = epoll_wait(epollfd, events, 1, -1);
        if (!fd_cnt)
			return -1;

        if (fd_cnt < 0) {
			if (errno == EINTR)
				continue;
			else {
				fprintf(stderr, "epoll_wait error, Error:[%d:%s]", errno, strerror(errno));
				return -1;
			}
		}

        for(i = 0; i < fd_cnt; i++)
        {
            pd = events[i].data.ptr;
            if (events[i].events & EPOLLIN)
            {
                if (pd->cb(pd->fd, pd->data) > 0)
                    return 0;
            }
        }
    }

    return 0;
}