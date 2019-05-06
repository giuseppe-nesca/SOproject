#ifndef SOPROJECT_SHMOP_H
#define SOPROJECT_SHMOP_H

#include <sys/types.h>
#include <sys/sem.h>

int semInitBusy(int semid, int semnum);
int semInitAviable(int semid, int semnum);
int semRelease(int semid, int semnum);
int semReserve(int semid, int semnum);
int semReserveValue(int semid, int semnum, int value);
int semReleaseValue(int semid, int semnum, int value);
int semInitValue(int semid, int semnum, int val);

int semGetValue(int semid, int semun);
int semGetPid(int semid, int semnum);

#endif //SOPROJECT_SHMOP_H
