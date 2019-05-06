SO project
==========

## Intro
This project is about the simulation of a "society" made by a "population".
The population is made by two types of process: procA and procB.
There is a "big brother" called "gestore" that creates the processes, makes them marry and generates their sons and kills them.
To get more infos read **FINAL_progetto_2017-18.pdf**

## Setup (guided compile and run)

The easiest way to compile (and execute the project) is by using the setup script.
It will guide you through:
- config file generating
- optional debug flag setting
- compiling the source
- executing the simulation

## Manual compiling

It can be compiled by the make utility or manually with gcc. Read the makefile to know the dependencies.

You can create/update the two executable files (gestore and proc) with 
* >$ make

or you can compile them indipendently using

* >$ make gestore
* >$ make proc

All generated object files can be removed using

* >$ make clean

If you need special debug informations you can define the following flags in *debug_flags.h*:
* PRINT_TIMER
* AUTOCLEAN_SEMS
* DEBUG_MARRIAGE
* DEBUG_KILLER
* PRINT_DEATH_INFOS
* DEBUG_GESTORE

## Manual running

Before starting the simulation you need to compile the __parameters.conf__  file like this: (respecting white spaces is important)

    init_people = 400
    genes = 1000
    birth_death = 1
    sim_time = 5

To start the simulation just execute gestore __./gestore__ and wait for the result after the setted *sim\_time*. 

**Pay attention* to the *FINAL_progetto_2017-18.pdf**  to know the allowed config parameters.

## Behind the scenes

The critic part of this project is the shared memory reading and writing.
Indeed all the processes' infos are stored in a sysV shared memory and is managed by a no prelation r/w policy.
Gestore releases the w after all _monkeys_ (the first generated processes) writes their information in shared memory.
Gestore enters his life cycle and if a marriage is pending it executes it and after checking whether or not a killing is pending it kills the proc with the longest name.
ProcA waits for procB request with a sysV message queue. The message type it is waiting for is the postion index in memory of the proc infos + 1 (proc id + 1). ProcB first takes the read perms and searches the best available procA taking into account the genes MCD. 
After procA receives, elaborates, and replaces, if negative, procB releases the read perms and restarts his life cycle.
In an accepted case procB releases the read perms and waits for the write perms. Then, procA triggers gestore using a signal and informs it about the marriage.
Write perms are released by Gestore after the marriage/killing procedure.
All communications among procs use sysV message queue, instead communications between proc and gestore use sysV semaphore too.