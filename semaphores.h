#ifndef SOPROJECTRC_SEMAPHORES_H
#define SOPROJECTRC_SEMAPHORES_H

#define TOTSEMS 5
#define LAUNCHER_SEM 0
#define SERVICE_SEM 1
#define MUTEX_SEM 2
#define READERS_SEM 3
#define READERS_SERVICE_SEM 4

void rLock(int semid);
void wLock(int semid);
void rUnlock(int semid);
void wUnlock(int semid);
void semReleaseAll();

#endif //SOPROJECTRC_SEMAPHORES_H
