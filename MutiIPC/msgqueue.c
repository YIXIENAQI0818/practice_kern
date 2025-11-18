// TODO: 简单的 System V 消息队列封装
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc_common.h"

static int msgqid = -1;

int msgqueue_init(key_t key)
{
    msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        return -1;
    }
    return msgqid;
}

int msgqueue_send(struct fuzz_task *t)
{
    struct fuzz_msg msg;
    msg.mtype = t->mtype;
    memcpy(&msg.task, t, sizeof(struct fuzz_task));
    if(msgsnd(msgqid, &msg, sizeof(struct fuzz_task), 0) == -1)
    {
        perror("msgsnd failed");
        return -1;
    }
    return 0;
}

int msgqueue_recv(struct fuzz_task *t, long mtype)
{
    struct fuzz_msg msg;
    msg.mtype = mtype;
    if(msgrcv(msgqid, &msg, sizeof(struct fuzz_task), mtype, 0) == -1)
    {
        perror("msgrcv failed");
        return -1;
    }
    memcpy(t, &msg.task, sizeof(struct fuzz_task));
    return 0;
}

int msgqueue_cleanup()
{
    if(msgqid != -1)
    {
        if(msgctl(msgqid, IPC_RMID, NULL) == -1)
        {
            perror("msgctl failed");
            return -1;
        }
    }
    return 0;
}



// EXPORT_SYMBOL(msgqueue_init);
// EXPORT_SYMBOL(msgqueue_send);
// EXPORT_SYMBOL(msgqueue_recv);
// EXPORT_SYMBOL(msgqueue_cleanup);