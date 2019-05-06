#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include "proc.h"
#include "procA.h"
#include "procB.h"
#include "semop.h"
#include "messages.h"
#include "debug_flags.h"

#define ARG_GENERATION 1
#define ARG_TYPE 2
#define ARG_NAME 3
#define ARG_GENOME 4
#define ARG_ID 5


volatile struct IPCsBundle __IPCs;
struct proc __proc;
int __init_people;
volatile sig_atomic_t* proc_life = NULL;

static void die (int sig);

int main(int argc, char** argv)
{
    /*initialize ipcs*/
    __init_people = atoi(getenv("N_PEOPLE"));
    __IPCs.semkey = atoi(getenv("SEMKEY"));
    __IPCs.semid = semget(__IPCs.semkey, 1, 0600);
    __IPCs.msgkey = atoi(getenv("MSGKEY"));
    __IPCs.msgid = msgget(__IPCs.msgkey, 0600);
    __IPCs.shmkey = atoi(getenv("SHMKEY"));
    __IPCs.shmid = shmget(__IPCs.shmkey, __init_people * sizeof(struct proc), 0600);

    /*register proc*/
    const char generation = argv[ARG_GENERATION][0];
    __proc.pid = getpid();
    __proc.type = argv[ARG_TYPE][0];
    strncpy(__proc.name, argv[ARG_NAME], STRLEN);
    sscanf(argv[ARG_GENOME],"%lu", &(__proc.genome));
    sscanf(argv[ARG_ID], "%d", &(__proc.id));

    if (__IPCs.msgid == -1) { puts("son errror getting msg (inbox)"); exit(1); }
    if (__IPCs.shmid == -1) { puts("son error getting shmA from gestore"); exit(1); }
    if (__IPCs.semid < 0 ) { puts("son error getting sem from gestore"); exit(1); }

    __IPCs.shared = (Shared) shmat(__IPCs.shmid, NULL, 0);
    if (__IPCs.shared == NULL){ puts("error getting shared"); return -1; }
    __IPCs.shared[__proc.id] = __proc;

    proc_life = &(__IPCs.shared[__proc.id].proc_life);
    *proc_life = TRUE;

    /*set signal handler*/
    struct sigaction new, old;
    new.sa_handler = die;
    sigemptyset(&new.sa_mask);
    if (sigaction(SIGTERM, &new, &old)) {
        perror("error setting sigaction(SIGUSR1, ..)");
        exit(1);
    }

    /* the same action is used in different cases:
     * the first  (H generation) releases 1 of 2 part of the semaphore took by gestore
     * the second (R generation) releases compleately the semapthore */
    if (generation == 'H') {
        /*inform gestore*/
        semRelease(__IPCs.semid, LAUNCHER_SEM);
    }
    if (generation == 'R') {
        /*release*/
        semRelease(__IPCs.semid, LAUNCHER_SEM);
    }

    switch (__proc.type) {
        case 'A':
            return procA();
        case 'B':
            return procB();
        default:
            perror("invalid proc.type");
            return 1;
    }
    return 0;
}

static void die (int sig) {
    switch (sig) {
        case SIGQUIT:
            printf("SIGQUIT recived");
            perror("HEADSHOT!");
            procPrint(__proc);
            break;
        case SIGTERM:
#ifdef PRINT_DEATH_INFOS
            puts("SIGTERM recived");
            procPrint(__IPCs.shared[__proc.id]);
#endif
            break;
        default:
            perror("unhandled signal recived");
            exit(1);
    }
    semReleaseAll();
    exit(0);
}

struct message checkInbox() {
    struct message msg;
    int flag = TRUE;
    while (flag) {
        if(msgrcv(__IPCs.msgid, &msg, sizeof(msg) - sizeof(long), __proc.id + 1 , 0) == -1) {
            perror("A error reciving msg form inbox");
            exit(1);
        }
        if (msg.lover.pid == getpid()) {
            return msg;
        }
        msg.request = DEATH_REQUEST;
        msg.type = msg.sender.id + 1;
        msg.lover.pid = msg.sender.pid;
        msg.lover.id = msg.sender.id;
        msg.sender.pid = __proc.pid;
        msg.sender.id = __proc.id;
        if(sendMessage(__IPCs.msgid, &msg) == -1) {
            perror("error sending death info");
            exit(1);
        }
        flag++;
    }
    exit(1);
}

