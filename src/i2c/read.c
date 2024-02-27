#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdint.h>
#include <errno.h>

struct shared_ai_power {
    int data[100];
    uint32_t ai_index;
};

static struct option opts[] = {
    { "time-interval", 1, NULL, 't' },
    { "help", 0, NULL, 'h' },
    { 0, 0, NULL, 0 }
};

void usage(void)
{
    printf("Usage: tmon [OPTION...]\n");
    printf("  -h, --help            show this help message\n");
    printf("  -t, --time-interval   sampling time interval, default 5 sec.\n");

    exit(0);
}

int g_time_interval = 5;

int sem_p(int sem_id)
{
    struct sembuf sbuf;

    sbuf.sem_num = 0;
    sbuf.sem_op = -1;
    sbuf.sem_flg = SEM_UNDO;

    if (semop(sem_id, &sbuf, 1) == -1)
    {
        fprintf(stderr, "Semaphore P error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int sem_v(int sem_id)
{
    struct sembuf sbuf;

    sbuf.sem_num = 0;
    sbuf.sem_op = 1;
    sbuf.sem_flg = SEM_UNDO;

    if (semop(sem_id, &sbuf, 1) == -1)
    {
        fprintf(stderr, "Semaphore V error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    void* shm = NULL;
    struct shared_ai_power *shared = NULL;
    int shmid;
    int id2 = 0, c;
    int total = 0;
    int average = 0;
    key_t key;
    int semid;
    int data_1s, data_2s, data_3s, data_4s, data_5s;
    int index = 0;

    while ((c = getopt_long(argc, argv, "ht:", opts, &id2)) != -1) {
        switch (c) {
        case 't':
            g_time_interval = atoi(optarg);
            break;
        case 'h':
            usage();
            break;
        default:
            break;
        }
    }

    if ((key = ftok("/tmp/", 'H')) == -1)
    {
        fprintf(stderr, "ftok error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    shmid = shmget(key, sizeof(struct shared_ai_power), 0);
    if (shmid == -1)
    {
        fprintf(stderr, "shmget failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    shm = shmat(shmid, 0, 0);
    if (shm == (void*)-1)
    {
        fprintf(stderr, "shmat failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    shared = (struct shared_ai_power*)shm;

    if ((semid = semget(key, 0, 0)) == -1)
    {
        fprintf(stderr, "semget failed, Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    while (1)
    {
        sem_p(semid);
        for (int i = 1; i <= g_time_interval; i++) {
            index = shared->ai_index - i;
            if (index < 0)
                index = (index + 100) % 100;

            total += shared->data[index];
        }

        average = total / g_time_interval;
        sem_v(semid);

        printf("total = %d, average = %d\n", total, average);
        total = 0;
        sleep(1);
    }

    if (shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed , Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}
