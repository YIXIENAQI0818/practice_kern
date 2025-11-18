// TODO: 共享内存 + SysV 信号量封装
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "ipc_common.h"

static int shm_id = -1;
static int sem_id = -1;
static int shm_count = 0;
static char* shm_addr = NULL;


void sem_wait(int sem_id)
{
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_signal(int sem_id)
{
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int shmsem_init(key_t shmkey, key_t semkey)
{
    shm_id = shmget(shmkey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    if(shm_id < 0)
    {
        perror("shmget failed");
        return -1;
    }

    shm_addr = shmat(shm_id, NULL, 0);
    if(shm_addr == (void*)-1)
    {
        perror("shmat failed");
        return -1;
    }

    sem_id = semget(semkey, 1, IPC_CREAT | 0666);
    if(sem_id < 0)
    {
        perror("semget failed");
        return -1;
    }

    if(semctl(sem_id, 0, SETVAL, 1) < 0)
    {
        perror("semctl failed");
        return -1;
    }

    shm_count = 0;
    return 0;
}

int shm_write_sample(int idx, const char *buf, int len)
{
    if(idx < 0 || idx >= MAX_SHM_SIZE || len < 0 || len > MAX_SHM_SIZE)
    {
        perror("invalid idx or len");
        return -1;
    }

    sem_wait(sem_id);
    char* p = shm_addr + idx * MAX_SAMPLE_SIZE;
    memcpy(p, buf, len);
    sem_signal(sem_id);
    shm_count++;

    return 0;
}

int shm_read_sample(int idx, char *outbuf, int *outlen)
{
    if(idx < 0 || idx >= MAX_SHM_SIZE || outlen == NULL)
    {
        perror("invalid idx or outlen");
        return -1;
    }
    
    sem_wait(sem_id);
    char* p = shm_addr + idx * MAX_SAMPLE_SIZE;
    *outlen = strlen(p);
    memcpy(outbuf, p, *outlen);
    sem_signal(sem_id);

    return 0;
}

int shm_get_count()
{
    return shm_count;
}

int shmsem_cleanup()
{
    if(shmdt(shm_addr) < 0)
    {
        perror("shmdt failed");
        return -1;
    }
    shm_addr = NULL;

    if(shmctl(shm_id, IPC_RMID, NULL) < 0)
    {
        perror("shmctl failed");
        return -1;
    }
    shm_id = -1;

    if(semctl(sem_id, 0, IPC_RMID) < 0)
    {
        perror("semctl failed");
        return -1;
    }
    sem_id = -1;
    shm_count = 0;

    return 0;
}

// EXPORT_SYMBOL(shmsem_init);
// EXPORT_SYMBOL(shmsem_cleanup);
// EXPORT_SYMBOL(shm_write_sample);
// EXPORT_SYMBOL(shm_read_sample);
// EXPORT_SYMBOL(shm_get_count);