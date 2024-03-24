#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <errno.h>

union semun {
    int              val;
    struct semid_ds *buf;  
    unsigned short  *array;  
    struct seminfo  *__buf;
};

static int init_sem(int sem_id, int value)
{
    union semun tmp;

    tmp.val = value;
    if (semctl(sem_id, 0, SETVAL, tmp) == -1)
    {
        fprintf(stderr, "Init semaphore error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

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

int del_sem(int sem_id)
{
    union semun tmp;

    if (semctl(sem_id, 0, IPC_RMID, tmp) == -1)
    {
        fprintf(stderr, "Delete semaphore error Error:[%d:%s]\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int creat_sem(key_t key)
{
	int sem_id;

	if ((sem_id = semget(key, 1, 0666 | IPC_CREAT)) == -1)
	{
		fprintf(stderr, "Create semaphore error Error:[%d:%s]\n", errno, strerror(errno));
		return -1;
	}

	if (init_sem(sem_id, 1) == -1)
		return -1;

	return sem_id;
}
