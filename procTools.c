#include <stdio.h>
#include "procTools.h"

unsigned long mcd(unsigned long a, unsigned long b) {
    unsigned long t;
    while (b != 0) {
        t = b;
        b = a % b;
        a = t;
    }
    return a;
}

void procPrint(struct proc p) {
    printf("pid = %d, id = %d,  type = %c, genome = %lu, name = %s life = %d\n", p.pid, p.id, p.type, p.genome, p.name, p.proc_life);
}