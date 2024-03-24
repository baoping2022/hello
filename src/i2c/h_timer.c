#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>

int timerfd_init(void)
{
    int tmfd;
    int ret;
    struct itimerspec timer;

    timer.it_value.tv_sec = 1;
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_nsec = 0;

    tmfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tmfd < 0) {
        fprintf(stderr, "timerfd_create error, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    ret = timerfd_settime(tmfd, 0, &timer, NULL);
    if (ret < 0) {
        fprintf(stderr, "timerfd_settime error, Error:[%d:%s]\n", errno, strerror(errno));
        close(tmfd);
        return -1;
    }

    return tmfd;
}

int timerfd_close(int tmfd)
{
    return close(tmfd);
}