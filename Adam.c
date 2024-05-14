#include "Adam.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>

int RandomNumber(int max){
    int result = rand()%(max+1);
    return result;
}

static struct sembuf buf;

int SemUp(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = IPC_NOWAIT;
    if (semop(semid, &buf, 1) == -1){
        return -1;
    }
    return 0;
}

int SemDown(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    buf.sem_flg = IPC_NOWAIT;
    if (semop(semid, &buf, 1) == -1){
        return -1;
    }
    return 0;
}

