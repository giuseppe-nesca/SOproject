#include <stdio.h>
#include <stdlib.h>
#include "semop.h"

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};


int semInitBusy(int semid, int semnum){
    union semun arg;
    arg.val = 0;
    return semctl(semid, semnum, SETVAL, arg);
}

int semInitAviable(int semid, int semnum){
    union semun arg;
    arg.val = 1;
    return semctl(semid, semnum, SETVAL, arg);
}

int semReserve(int semid, int semnum){
    struct sembuf sops;
    sops.sem_op=-1;
    sops.sem_num = semnum;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int semReserveValue(int semid, int semnum, int value){
    struct sembuf sops;
    sops.sem_op= -value;
    sops.sem_num = semnum;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int semRelease(int semid, int semnum){
    struct sembuf sops;
    sops.sem_op=1;
    sops.sem_num = semnum;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int semReleaseValue(int semid, int semnum, int value) {
    struct sembuf sops;
    sops.sem_op =  +value;
    sops.sem_num = semnum;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int semInitValue(int semid, int semnum, int val){
    union semun arg;
    arg.val = val;
    return semctl(semid, semnum, SETVAL, arg);
}

int semGetValue(int semid, int semnum){
    union semun arg;
    int semval = semctl(semid, semnum, GETVAL, arg);
    if(semval < 0){puts("error getting sem value"); exit(1);}
    return semval;
}

int semGetPid(int semid, int semnum){
    union semun arg;
    int sempid = semctl(semid, semnum, GETPID, arg);
    if(sempid < 0){puts("error getting sem value"); exit(1);}
    return sempid;
}