#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>
#include "hm800_timer.h"
#include "hm800_epoll.h"
#include "hm800_sem.h"

struct shared_ai_power {
    int data[100];
    uint32_t ai_index;
};

int timer_callback(int fd, void *arg)
{
    uint64_t exp = 0;
    int ai_power_data = 0;

    read(fd, &exp, sizeof(uint64_t));
    
    sem_p(g_semid);
    printf("timeout\n");
    sem_v(g_semid);

    return 0;
}

int main(int argc, char **argv)
{
    int epollfd = 0;
    int timerfd = 0;
    int shmid = 0;
    void *shm = NULL;
    key_t key;

    if ((key = ftok("/tmp/", 'H')) == -1)
    {
        fprintf(stderr, "ftok error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    shmid = shmget(key, sizeof(struct shared_ai_power), 0666|IPC_CREAT);
    if (shmid == -1)
    {
        fprintf(stderr, "shmget failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    shm = shmat(shmid, NULL, 0);
    if (shm == (void *) -1)
    {
        fprintf(stderr, "shmat failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    g_semid = creat_sem(key);
    if (g_semid < 0)
        return -1;

    ai_power = (struct shared_ai_power *)shm;
    memset(ai_power, 0, sizeof(struct shared_ai_power));

    //if (i2c_init() < 0)
    //    return -1;

    epollfd = epoll_init();
    if (epollfd < 0)
        return -1;

    timerfd = timerfd_init();
    if (timerfd < 0)
        return -1;

    if (epoll_add_fd(epollfd, timerfd, timer_callback, NULL) < 0)
        return -1;

    epoll_event_handle(epollfd);

    epoll_final(epollfd);
    timerfd_close(timerfd);
    //i2c_close();

    if (del_sem(g_semid) == -1)
        return -1;

    if (shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "shmctl(IPC_RMID) failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}
