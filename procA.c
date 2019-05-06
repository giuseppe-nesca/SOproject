#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <signal.h>
#include "procA.h"
#include "proc.h"
#include "messages.h"
#include <unistd.h>
//#include "debug_flags.h"

extern struct IPCsBundle __IPCs;
extern struct proc __proc;
extern sig_atomic_t  * proc_life;
extern int __init_people;

/* procA:
 * - prepare a message with a marriage request and useful infos
 * - send it to gestore
 * - inform gestore with a SIGUSR1 signal
 * - exit / die happy */
void marriage(const int idB, const int pidB, const int MCD) {
    struct message msg;
    msg.request = MARRIAGE_REQUEST;
    msg.type = __init_people + 2;
    msg.sender.id = __proc.id;
    msg.sender.pid = __proc.pid;
    msg.lover.id = idB;
    msg.lover.pid = pidB;
    msg.MCD = MCD;
    if (msgsnd(__IPCs.msgid, &msg, (sizeof(msg) - sizeof(long)), 0) == -1) {
        perror("procA: error sending msg to gestore.");
        exit(EXIT_FAILURE);
    }
    kill(getppid(), SIGUSR1);
    exit(EXIT_SUCCESS);
}

/* decrease %10 target */
unsigned long defineTarget(const unsigned long current) {
    return current*90/100;
}

/* prepare a message to reply to B with the chosen topic */
struct message newMessageAtoB(const int idB, const int pidB, const char argument) {
    struct message msg;
    msg.type = idB + 1;
    msg.request = argument;
    msg.sender.pid = __proc.pid;
    msg.sender.id = __proc.id;

    msg.lover.id = idB;
    msg.lover.pid = pidB;
    return msg;
}

/* this func is called by the general proc main after the type check
 * it implements the procA life cycle */
int procA()
{
    /* this array saves the procB knowed*/
    int* fails = malloc(__init_people * sizeof(int));
    for (int i = 0; i < __init_people; ++i)
        fails[i] = -1;
    unsigned long target = __proc.genome; //the most ambitious value is the procA genome value itself

    while (*proc_life) {
        struct message msg;
        /*check inbox / wait for messages*/
        msg = checkInbox();
        const int idB = msg.sender.id;
        const int pidB = msg.sender.pid;
        const unsigned long genomeB = __IPCs.shared[idB].genome;
        unsigned long MCD = mcd(__proc.genome, genomeB);
        if (MCD >= target) {
            /*accepted*/
            if (pidB != __IPCs.shared[idB].pid) {
                /*abort marriage*/
                perror("read permission do not allow this sesssion. r/w error");
                continue;
            } //check b
            /* set procA found its love so killer won't kill it
             * no problem with marriage because write perms have been taken */
            *proc_life = FALSE;
            /*inform B*/
            msg = newMessageAtoB(idB, pidB, ACCEPTED_REQUEST);
            if (msgsnd(__IPCs.msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                perror("procA: error sending msg: A to B, inbox");
                exit(EXIT_FAILURE);
            }

            /* wait for procB to take write perms and die */
            msg = checkInbox();
            while (msg.request == 'M') { //ignore other pendig procBs' requests
                msg = newMessageAtoB(msg.sender.id, msg.sender.pid, REFUSED_REQUEST);
                if (sendMessage(__IPCs.msgid, &msg) == -1) {
                    perror("procA: error sending msg: A to B, inbox");
                    exit(EXIT_FAILURE);
                }
                msg = checkInbox();
            }
            /* if unfortunately procB has been killed continue */
            if (msg.request == DEATH_REQUEST) {
                *proc_life = TRUE;
                continue;
            }
            /* if it is ok */
            if (msg.request != GO_REQUEST) {
                perror("procA unmanaged request. Expected: GO_REQUEST");
                printf("%d %d %c by %d %d\n", getpid(), __proc.id, msg.request, msg.sender.pid, msg.sender.id);
                target = defineTarget(target);
                *proc_life = TRUE;
                continue;
            }
            marriage(idB, pidB, MCD);
            /*DIE HAPPY!*/
        } else { /*refused*/
            /*inform B*/
            fails[idB] = __IPCs.shared[idB].pid;
            msg = newMessageAtoB(idB, pidB, REFUSED_REQUEST);
            if (msgsnd(__IPCs.msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                perror("A: error sending msg: A to B, inbox");
                exit(1);
            }
        }// refused
        target = defineTarget(target);
        /* continue procA life*/
    } //proclife loop
    perror("proc_life A");
    return EXIT_FAILURE;
}
