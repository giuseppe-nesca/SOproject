#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "procTools.h"
#include "semop.h"
#include "messages.h"
#include "semaphores.h"
#include "debug_flags.h"

struct settings {
    int init_people;
    unsigned long genes;
    int birth_death;
    int sim_time;
}__settings;

volatile struct IPCsBundle __IPCs;

struct statistics{
    int totKills;
    int currA, currB;
    int totA, totB;
    struct proc longestName, highestGenome;
};
typedef struct statistics * Stats;
Stats statistics;

void importConfig(){
    const char* filename = CONF_PATH;
    FILE* confFile = fopen(filename, "r");
    if (confFile == NULL){
        puts("no conf file found");
        exit(1);
    }
    fscanf(confFile,
           "init_people = %d\n"
            "genes = %lu\n"
            "birth_death = %d\n"
            "sim_time = %d",
    &(__settings.init_people), &(__settings.genes), &(__settings.birth_death), &(__settings.sim_time)
    );

    printf( "settings are:\n"
            "init_people = %d\n"
            "genes = %lu\n"
            "birth_death = %d\n"
            "sim_time = %d\n",
            (__settings.init_people), (__settings.genes), (__settings.birth_death), (__settings.sim_time)
    );

    fclose(confFile);
}

int gestoreDestructor(int returnValue){
    int status;
    while (waitpid(WAIT_ANY, &status, WNOHANG) != 0) {
        perror("unhandled zombie found!");
        printf("errExit = %d\n", status );
    }
    for (int i = 0; i < __settings.init_people; ++i) {
        struct proc p = __IPCs.shared[i];
        kill(p.pid, SIGTERM);
        wait(NULL);
    }
    //deallocate stuff
    //remove sem
    if(semctl(__IPCs.semid, TOTSEMS, IPC_RMID) < 0 ){puts("error removing sem"); exit(1);}
    //remove shm procs info
    shmdt(__IPCs.shared);
    if(shmctl(__IPCs.shmid, IPC_RMID, NULL) < 0){puts("error removing shm"); exit(1);}
    //remove msg
    if(msgctl(__IPCs.msgid, IPC_RMID, NULL) < 0){puts("error removing msg (inbox)"); exit(1);}
    puts("simulation ended correctly!");

    /*statistics*/
    printf("\nStatistics:\n");
    printf("processes tot = %d (%d procA, %d procB)\n",
           statistics->totA + statistics->totB, statistics->totA, statistics->totB);
    printf("killed tot: %d\n", statistics->totKills);

    printf("process with longest name:\n"
           "  pid: %d\n"
           "  type: %c\n"
           "  genome: %lu\n"
           "  name: %s\n",
           statistics->longestName.pid, statistics->longestName.type, statistics->longestName.genome, statistics->longestName.name);
    printf("process with highest genome:\n"
           "  pid: %d\n"
           "  type: %c\n"
           "  genome: %lu\n"
           "  name: %s\n",
           statistics->highestGenome.pid, statistics->highestGenome.type, statistics->highestGenome.genome, statistics->highestGenome.name);

    return returnValue;
}

struct {
    int randomInt;
    long randomLong;
}randomBase;

int getRand(){
    srand(randomBase.randomInt);
    randomBase.randomInt = rand();
#ifdef DEBUG_RAND
    printf("rInt = %d\n", randomBase.randomInt);
#endif
    return randomBase.randomInt;
}

long getLRand(){
    srand48(randomBase.randomLong);
    randomBase.randomLong =  lrand48();
#ifdef DEBUG_LRAND
    printf("rLong = %lu\n", randomBase.randomLong);
#endif
    return randomBase.randomLong;
}

char** genProc(/*id in shared memory*/const int id, const char generation){

    char ** arg = malloc(CREATE_ARGS*sizeof(char*));
    int l;

    //0: program name
    arg[0] = malloc( STRLEN*sizeof(char) );
    snprintf(arg[0], STRLEN, "proc");

    //1: generation: M/H/R (MONKEY/HUMAN/REPLICANT)
    arg[1] = malloc( (2)*sizeof(char) );
    arg[1][0] = generation;
    arg[1][1] = '\0';

    //2: type
    arg[2] = malloc( 2*sizeof(char) );
    arg[2][1] = '\0';
    char type;
    if(getRand()%2 == 0) type = 'A';
    else type = 'B';
    if ((generation == 'M' && id == __settings.init_people - 1)
        || generation == 'H' || generation == 'R'){
        /* prevent type uniformity */
        if (statistics->currA == 0)
            type = 'A';
        if (statistics->currB == 0)
            type = 'B';
    }
    arg[2][0] = type;
    if (type == 'A') {
        statistics->currA++;
    }
    else {
        statistics->currB++;
    }

    //3: name
    arg[3] = malloc( 2*sizeof(char) );
    arg[3][1] = '\0';
    arg[3][0] = 'A' + (getRand() % 26);

    //4: genes
    const unsigned long genes = __settings.genes;
    const unsigned x = 2;
    const unsigned long genome = x + (getLRand()%(genes));
    l = snprintf(NULL, 0, "%lu", genome);
    arg[4] = malloc( (l+1)*sizeof(char) );
    snprintf(arg[4], (size_t) l+1, "%lu", genome);

    //5 id
    l = snprintf(NULL, 0, "%d", id);
    arg[5] = malloc((l+1) * sizeof(char));
    snprintf(arg[5], (size_t) l+1, "%d", id);

    arg[6] = NULL;

    return arg;
}

char** genProcSon(const int id, /*parent struct is used to inherit informations*/const struct proc father, const unsigned long MCD){
    char ** arg = malloc(CREATE_ARGS*sizeof(char*));
    int l;

    //0: program name
    arg[0] = malloc( STRLEN*sizeof(char) );
    snprintf(arg[0], STRLEN, "proc");

    //1: generation: H (HUMAN)
    arg[1] = malloc( (2)*sizeof(char) );
    arg[1][0] = 'H';
    arg[1][1] = '\0';

    //2: type
    arg[2] = malloc( 2*sizeof(char) );
    arg[2][1] = '\0';
    char type;
    if(getRand()%2 == 0) type = 'A';
    else type = 'B';
#ifndef NO_SEX_DISCRIMINATION
    if(statistics->currA == 0)
        type = 'A';
    if (statistics->currB == 0)
        type = 'B';
#endif
    arg[2][0] = type;
    if (type == 'A') {
        statistics->currA++;
        statistics->totA++;
    } else {
        statistics->currB++;
        statistics->totB++;
    }

    //3: name
    int nameSize = (int) (strlen(father.name) + 2);
    arg[3] = malloc( nameSize *sizeof(char) );
    strcpy(arg[3], father.name);
    char new[2]; new[1] = '\0';
    new[0] = (char) ('A' + (getRand() % 26));
    strcat(arg[3], new);

    //4: genes
    const unsigned long genes = __settings.genes;
    const unsigned long x = MCD;
    const unsigned long genome = x + (getLRand()%(genes));
    l = snprintf(NULL, 0, "%lu", genome);
    arg[4] = malloc( (l+1)*sizeof(char) );
    snprintf(arg[4], (size_t) l+1, "%lu", genome);

    //5 id
    l = snprintf(NULL, 0, "%d", id);
    arg[5] = malloc((l+1) * sizeof(char));
    snprintf(arg[5], (size_t) l+1, "%d", id);

    arg[6] = NULL;

    return arg;
}

char** setEnv(){
    const int init_people = __settings.init_people;
    const int msgkey = __IPCs.msgkey;
    const int semkey = __IPCs.semkey;
    const int shmkey = __IPCs.shmkey;
    char ** env = malloc(ENVN*sizeof(char*));
    for (int i = 0; i < ENVN; ++i) {
        env[i] = malloc(ENVLEN * sizeof(char));
    }
    snprintf(env[0], ENVLEN, "N_PEOPLE=%d", init_people);
    snprintf(env[1], ENVLEN, "MSGKEY=%d", msgkey);
    snprintf(env[2], ENVLEN, "SEMKEY=%d", semkey);
    snprintf(env[3], ENVLEN, "SHMKEY=%d", shmkey);
    env[4] = NULL;
    return env;
}

static volatile sig_atomic_t __gestore_life = TRUE;
static volatile sig_atomic_t __gestore_killer = FALSE;
static volatile sig_atomic_t __gestore_marriage = FALSE;

static void marriageHandler(int sig);
void marriage ();
void killer();
static void killerHandler(int sig);

int main(int argc, char** argv)
{
#ifdef AUTOCLEAN_SEMS
    system("ipcrm -a");
#endif

    importConfig();

    /*initialize random value*/
    randomBase.randomInt = randomBase.randomLong = getpid();

    /*initialize rw sems*/
    __IPCs.semkey = ftok(PROC_PATH, 'K');
    __IPCs.semid = semget(__IPCs.semkey, TOTSEMS, IPC_CREAT | IPC_EXCL | 0600);
    if (__IPCs.semid == -1){ puts("error allocating sems"); exit(1); }
    printf("semkey = %d     semid = %d\n", __IPCs.semkey, __IPCs.semid);
    if (
            semInitBusy(__IPCs.semid, LAUNCHER_SEM) < 0 ||
            semInitBusy(__IPCs.semid, MUTEX_SEM) < 0 ||
            semInitBusy(__IPCs.semid, READERS_SEM) ||
            semInitAviable(__IPCs.semid, READERS_SERVICE_SEM) < 0 ||
            semInitBusy(__IPCs.semid, SERVICE_SEM) < 0
            ){ puts("error inizializing initSem"); exit(1); } //sem generator

    /*initialize proc info shm*/
    __IPCs.shmkey = ftok(PROC_PATH, 'M');
    __IPCs.shmid = shmget(__IPCs.shmkey, __settings.init_people * sizeof(struct proc), IPC_CREAT | IPC_EXCL | 0600);
    if (__IPCs.shmid == -1){ puts("error allocating proc shm"); exit(1); }
    printf("shmkey = %d     shmid = %d\n", __IPCs.shmkey, __IPCs.shmid);
    __IPCs.shared = (Shared) shmat(__IPCs.shmid, NULL, 0);

    /*initialize stats info shm*/
    struct statistics statStruct;
    statistics = &statStruct;
    statistics->currA = 0;
    statistics->currB = 0;
    statistics->totA = 0;
    statistics->totB = 0;
    statistics->highestGenome.id = -1;
    statistics->longestName.id = -1;
    statistics->totKills = 0;

    /*initialize msg queue*/
    __IPCs.msgkey = ftok(PROC_PATH, 'Q');
    __IPCs.msgid = msgget(__IPCs.msgkey,  IPC_CREAT | IPC_EXCL | 0600);
    if (__IPCs.msgid == -1){ puts("error allocating message queue (inbox)"); exit(1); }
    printf("msgkey = %d     msgid = %d\n", __IPCs.msgkey, __IPCs.msgid);

    /*set env*/
    __IPCs.env = setEnv();

    struct sigaction new, old;
    new.sa_flags = SA_RESTART;
    new.sa_handler = marriageHandler;
    sigfillset(&new.sa_mask);
    sigdelset(&new.sa_mask, SIGTERM);
    sigdelset(&new.sa_mask, SIGALRM);
    if (sigaction(SIGUSR1, &new, &old)) {
        perror("error setting sigaction(SIGUSR1, ...)");
        exit(EXIT_FAILURE);
    }

    struct sigaction newAlarm, oldAlarm;
    newAlarm.sa_flags = SA_RESTART;
    newAlarm.sa_handler = killerHandler;
    sigfillset(&newAlarm.sa_mask);
    sigdelset(&new.sa_mask, SIGTERM);
    sigdelset(&new.sa_mask, SIGUSR1);
    if (sigaction(SIGALRM, &newAlarm, &oldAlarm)) {
        perror("error setting sigaction(SIGALARM, ...");
        exit(EXIT_FAILURE);
    }

    /* create monkeys:
     * the eldest monkey version*/
    char *** monkeys = malloc(__settings.init_people * sizeof(char**));
    for (int id = 0; id < __settings.init_people; ++id) {
        monkeys[id] = genProc(id, 'M');
    }
    puts("creating processes...");
    for(int id = 0; id < __settings.init_people; id++){
        pid_t son = fork();
        if(son == 0){
            char ** args = monkeys[id];
            return execve(PROC_PATH, args, __IPCs.env);
        }
    }
    /*wait until all sem be ready*/
    semRelease(__IPCs.semid, MUTEX_SEM);
    semRelease(__IPCs.semid, SERVICE_SEM);

#ifdef DEBUG_GESTORE
    printf("pid gestore = %d\n", getpid());
#endif

    /*gestore's life cycle*/
    __gestore_life = __settings.sim_time;
    time_t start = time(NULL);
    if (__gestore_life)
        alarm((unsigned int) __settings.birth_death);
    while (__gestore_life) {
        if (__gestore_marriage) {
            marriage();
            while (__gestore_killer) {
                killer();
                alarm((unsigned int) __settings.birth_death);
            }
            if (time(NULL) - start <= __settings.sim_time) {
                wUnlock(__IPCs.semid);
            } else {
                __gestore_life = FALSE;
                alarm(0);
            }
        }
    }

    puts("sim ended");
#ifdef DEBUG_GESTORE
    printf("duration = %lu\n", time(NULL) - start);
    printf("pending marriage = %d\n", __gestore_marriage);
#endif
    return gestoreDestructor(0);
}

void registerName (Proc this) {
    Proc record =  & statistics -> longestName;
    if (record -> id < 0) {
        /*first record case*/
        *record = *this;
    } else {
        /*compare*/
        if (strlen(this -> name) > strlen(record -> name)) {
            *record = *this;
        }
    }
}

void registerGenome (Proc this) {
    Proc record = & statistics -> highestGenome;
    if (record -> id < 0){
        /*first record case*/
        *record = *this;
    } else {
        /*compare*/
        if (this -> genome > record -> genome) {
            *record = *this;
        }
    }
}

static void freeMemory(char** args);

void marriage () {
    /*mask SIGARLM*/
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    struct message msg;
    if (msgrcv(__IPCs.msgid, &msg, (sizeof(msg) - sizeof(long)), (long) __settings.init_people + 2, 0) == -1) {
        perror("no message from parents");
        return;
    }
    const pid_t pidA = msg.sender.pid;
    const int idA = msg.sender.id;
    const pid_t pidB = msg.lover.pid;
    const int idB = msg.lover.id;
    int statA, statB;
#ifdef DEBUG_MARRIAGE
    printf("marriage between %d and %d...\n", pidA, pidB);
#endif

    int wait1res = waitpid(pidA, &statA, 0);
    if (wait1res == -1) {
        printf("wait1 fails: %d\n", errno);
        pause();
    }
    int wait2res = waitpid(pidB, &statB, 0);
    if (wait2res == -1) {
        printf("wait2 fails: %d\n", errno);
        pause();
    }

    /*update statistics*/
    statistics->currA--;
    statistics->currB--;
    statistics->totA++;
    statistics->totB++;
    registerName(&(__IPCs.shared[idA]));
    registerName(&(__IPCs.shared[idB]));
    registerGenome(&(__IPCs.shared[idA]));
    registerGenome(&(__IPCs.shared[idB]));

    const unsigned long MCD = msg.MCD;

    pid_t sonA = -1, sonB = -1;
    char** sonAargs = genProcSon(idA, __IPCs.shared[idA], MCD);
    char** sonBargs = genProcSon(idB, __IPCs.shared[idB], MCD);

    sonA = fork();
    if (sonA == 0) {
        execve(PROC_PATH, sonAargs, __IPCs.env);
    }
    sonB = fork();
    if (sonB == 0) {
        execve(PROC_PATH, sonBargs, __IPCs.env);
    }
    /*now sons are born but here i am not able to know if they are ready*/
    __gestore_marriage = FALSE;

    freeMemory(sonAargs);
    freeMemory(sonBargs);
    /*wait for sons*/
    int returnValue = semReserveValue(__IPCs.semid, LAUNCHER_SEM, 2);
    if (returnValue == -1) {
        printf("errno = %d    ", errno);
        perror("sons waiting interrupted");
    }

    if (semGetValue(__IPCs.semid, LAUNCHER_SEM) != 0) {
        printf("%d\n", semGetValue(__IPCs.semid, LAUNCHER_SEM));
        perror("semGetValue");
    }
#ifdef DEBUG_MARRIAGE
    printf("marriage between %d and %d DONE --> %d and %d\n", pidA, pidB, sonA, sonB);
#endif
    /*remove mask*/
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

static void marriageHandler(int sig) {
    __gestore_marriage = TRUE;
}

void killer() {
    /*mask SIGARLM*/
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int max_len = 0;
    int selectedID = -1;
    for (int i = 0; i < __settings.init_people; ++i) {
        int curr_len  = strlen(__IPCs.shared[i].name);
        if (curr_len > max_len && __IPCs.shared[i].proc_life) {
            max_len = curr_len;
            selectedID = i;
        }
    }
    if (max_len == 0) perror("max len 0");
    char selectedType = __IPCs.shared[selectedID].type;
    pid_t selectedPID = __IPCs.shared[selectedID].pid;
#ifdef DEBUG_KILLER
    printf("killing %d %d ...\n", selectedID, selectedPID);
#endif
    kill(selectedPID, SIGTERM);
    waitpid(selectedPID, NULL, 0);

    if (selectedType == 'A') {
        statistics->currA--;
        statistics->totA++;
    } else { //selectedType == 'B'
        statistics->currB--;
        statistics->totB++;
    }

    char** newProc = genProc(selectedID, 'R');
    pid_t son = fork();
    if (!son) {
        execve(PROC_PATH, newProc, __IPCs.env);
    }
    freeMemory(newProc);
    statistics->totKills++;
    __gestore_killer--;
    int semReturnValue = semReserve(__IPCs.semid, LAUNCHER_SEM);
    if (semReturnValue == -1) {
        perror("killer semReserve failed");
    }
#ifdef DEBUG_KILLER
    printf("killing %d %d DONE\n", selectedID, selectedPID);
#endif

    /*unlock SIGALRM*/
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

static void killerHandler(int sig) {
    __gestore_killer++;
    static int time = 0;
    time += __settings.birth_death;
#ifdef PRINT_TIMER
    printf("time = %d\n", time);
#endif
}

static void freeMemory(char** args) {
    for (int i = 0; i < CREATE_ARGS - 1; ++i) {
        free(args[i]);
    }
    free(args);
}