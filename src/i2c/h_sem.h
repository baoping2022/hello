#ifndef H_SEM_H
#define H_SEM_H

#include <sys/types.h>

int creat_sem(key_t key);
int del_sem(int sem_id);
int sem_p(int sem_id);
int sem_v(int sem_id);

#endif //H_SEM_H