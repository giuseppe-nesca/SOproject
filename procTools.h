#ifndef SOPROJECTRC_PROCTOOLS_H
#define SOPROJECTRC_PROCTOOLS_H

#include "constants.h"
#include <signal.h>

struct proc{
    int id;
    int pid;
    char type;
    char name[STRLEN];
    unsigned long genome;
    __sig_atomic_t proc_life;
};
typedef struct proc* Proc;
typedef struct proc* Shared;
struct IPCsBundle{
    int msgid, semid, shmid;
    int msgkey,semkey, shmkey;
    volatile Shared shared;
    char ** env;
};

unsigned long mcd(unsigned long a, unsigned long b);
void procPrint(struct proc p);

#endif //SOPROJECTRC_PROCTOOLS_H
