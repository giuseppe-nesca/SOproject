#include <stdio.h>
#include "semaphores.h"
#include "semop.h"
#include "procTools.h"
#include <unistd.h>

extern struct IPCsBundle __IPCs;

void rLock(int semid){
    semReserve(semid, SERVICE_SEM);
    semReserve(semid, READERS_SERVICE_SEM);
    if (semGetValue(semid, READERS_SEM) == 0) {
        semReserve(semid, MUTEX_SEM);
    }
    semRelease(semid, READERS_SEM); //it works like counter
    semRelease(semid, READERS_SERVICE_SEM);
    semRelease(semid, SERVICE_SEM);
}

void rUnlock(int semid) {
    semReserve(semid, READERS_SERVICE_SEM);
    semReserve(semid, READERS_SEM); //it works like counter
    if (semGetValue(semid, READERS_SEM) == 0) {
        semRelease(semid, MUTEX_SEM);
    }
    semRelease(semid, READERS_SERVICE_SEM);
}

void wLock(int semid) {
    semReserve(semid, SERVICE_SEM);
    semReserve(semid, MUTEX_SEM);
    semRelease(semid, SERVICE_SEM);
}

void wUnlock(int semid) {
    int result = semRelease(semid, MUTEX_SEM);
    if (result == -1) {
        perror("error releaseing MUTEX");
    }
}

void semReleaseAll() {
    pid_t servicePID = semGetPid(__IPCs.semid, SERVICE_SEM);
    if (servicePID == getpid()) {
        if (semGetValue(__IPCs.semid, SERVICE_SEM) == 0) {
            semRelease(__IPCs.semid, SERVICE_SEM);
        } else
            return;
    }
    pid_t readersServicePID = semGetPid(__IPCs.semid, READERS_SERVICE_SEM);
    if (readersServicePID == getpid()) {
        if (semGetValue(__IPCs.semid, READERS_SERVICE_SEM) == 0) {
            semRelease(__IPCs.semid, READERS_SERVICE_SEM);
        }
    }
}