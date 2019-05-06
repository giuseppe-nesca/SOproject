#include "messages.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>


int sendMessage(int msgid, struct message* msg) {
    return msgsnd(msgid, msg, (sizeof(struct message) - sizeof(long)), 0);
}