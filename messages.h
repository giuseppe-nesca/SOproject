#ifndef SOPROJECTRC_MESSAGES_H
#define SOPROJECTRC_MESSAGES_H

#include "procTools.h"

struct message{
    long type;
    struct senderInfo{
        int id;
        int pid;
    }sender, lover;
    char request;
    unsigned long MCD;
};


int sendMessage(int msgid, struct message* msg);

#endif //SOPROJECTRC_MESSAGES_H
