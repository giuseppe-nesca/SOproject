#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/msg.h>
#include "proc.h"
#include "procB.h"
#include "messages.h"
#include <unistd.h>
//#include "debug_flags.h"

extern struct IPCsBundle __IPCs;
extern struct proc __proc;
extern sig_atomic_t * proc_life;
extern int __init_people;

/* procB searches the best procA
 * if any, uncontacted procA have priority. Otherwise simply choose the procA with the best genes
 * the checks on genes are based on mcd */
int searchBestA(int fails[]) {
    unsigned long bestGen = 0;
    int idA = -1;

    for (int i = 0; i < __init_people; ++i) {
        /*check available A type*/
        if (fails[i] != __IPCs.shared[i].pid && __IPCs.shared[i].type == 'A' ) {
            unsigned long tmcd = mcd(__proc.genome, __IPCs.shared[i].genome);
            if (tmcd > bestGen) {
                bestGen = tmcd;
                idA = __IPCs.shared[i].id;
            }
        }
    }
    return idA;
}

/* procB prepare a message with a marriage request */
struct message newMessage(int idA, int pidA) {
    struct message msg;
    msg.type = idA + 1;
    msg.sender.id = __proc.id;
    msg.sender.pid = __proc.pid;

    msg.lover.id = idA;
    msg.lover.pid = pidA;
    msg.request = MARRIAGE_REQUEST;
    return msg;
}

/* this func is called by the general proc main after the type check
 * it implements the procB life cycle */
int procB()
{
    int* fails = malloc(__init_people * sizeof(int));
    for (int i = 0; i < __init_people; ++i)
        fails[i] = -1;

    while (*proc_life) {
        /* take read permissions */
        rLock(__IPCs.semid);
        int idA = searchBestA(fails);
        /* manage no A found case */
        if (idA < 0) {
            /* retry all procA */
            for (int i = 0; i < __init_people; ++i) {
                fails[i] = -1;
            }
            rUnlock(__IPCs.semid);
            continue;
        } //i'm sure index is valid (id > 0)
        int pidA = __IPCs.shared[idA].pid;
        /* contact A */
        struct message msg = newMessage(/*to*/ idA, pidA);
        if(msgsnd(__IPCs.msgid, &msg, (sizeof(msg) - sizeof(long)), 0) == -1) {
            perror("B: error sending msg to A. inbox");
            printf("errno = %d\n", errno);
            exit(1);
        }
        msg = checkInbox();
        switch (msg.request) {
            case DEATH_REQUEST:
                break;
            case REFUSED_REQUEST:
                fails[idA] = __IPCs.shared[idA].pid;
                break;
            case ACCEPTED_REQUEST:
                *proc_life = FALSE;
                rUnlock(__IPCs.semid);
                wLock(__IPCs.semid);
                msg.type = idA + 1;
                msg.request = GO_REQUEST;
                msg.sender.id = __proc.id;
                msg.sender.pid = __proc.pid;
                msg.lover.id = idA;
                msg.lover.pid = pidA;
                if (sendMessage(__IPCs.msgid, &msg) == -1) {
                    perror("B: error sending GO to A");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
                /*DIE HAPPY!*/
                break;
            default:
                perror("B: recived from A an unmanaged request");
                printf("%c is unmanaged sent by %d\n", msg.request, msg.sender.pid);
                exit(EXIT_FAILURE);
        }
        rUnlock(__IPCs.semid);
        /* continue the procB lifcycle */
    } //proclife loop
    perror("!proc_life B");
    return EXIT_FAILURE;
}