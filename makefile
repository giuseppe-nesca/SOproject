project: gestore proc

gestore: gestore.o semop.o semaphores.o procTools.o messages.o
	gcc -Wall -pedantic -o gestore gestore.o semop.o semaphores.o procTools.o messages.o

gestore.o: gestore.c procTools.h semop.h messages.h semaphores.h constants.h debug_flags.h
	gcc -Wall -pedantic -c gestore.c

semaphores.o: semaphores.c semaphores.h semop.h procTools.h
	gcc -Wall -pedantic -c semaphores.c

messages.o: messages.c messages.h procTools.h
	gcc -Wall -pedantic -c messages.c

procTools.o: procTools.c procTools.h constants.h
	gcc -Wall -pedantic -c procTools.c

semop.o: semop.c semop.h
	gcc -Wall -pedantic -c semop.c

proc: proc.o procA.o procB.o semop.o messages.o semaphores.o procTools.o
	gcc -Wall -pedantic -o proc proc.o procA.o procB.o semop.o messages.o semaphores.o procTools.o

proc.o: proc.c proc.h procTools.h semaphores.h procA.h procB.h semop.h messages.h debug_flags.h
	gcc -Wall -pedantic -c proc.c

procA.o: procA.c procA.h proc.h procTools.h semaphores.h messages.h
	gcc -Wall -pedantic -c procA.c

procB.o: procB.c procB.h proc.h procTools.h semaphores.h messages.h
	gcc -Wall -pedantic -c procB.c

clean:
	rm -f *.o