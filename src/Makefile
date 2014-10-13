CC= gcc
CFLAGS= -O0 -g -Wall -Werror
INCLUDE= cheetah
OBJECTS= event.o event_ht.o list.o lock.o log.o polling_policy.o reactor.o signal.o timer.o utility.o bench.o
PROGRAM= cheetah_exec
LIBS= -lpthread
bench.o:
	$(CC) $(CFLAGS)  -c bench.c
event.o: $(INCLUDE)/event.h event.c
	$(CC) $(CFLAGS) -c event.c

event_ht.o: $(INCLUDE)/event_ht.h event_ht.c
	$(CC) $(CFLAGS) -c event_ht.c

list.o:	$(INCLUDE)/list.h list.c
	$(CC) $(CFLAGS) -c list.c

lock.o: $(INCLUDE)/lock.h lock.c
	$(CC) $(CFLAGS) -c lock.c

log.o: 	$(INCLUDE)/log.h log.c
	$(CC) $(CFLAGS) -c log.c

polling_policy.o: $(INCLUDE)/polling_policy.h polling_policy.c polling_select.c polling_poll.c polling_epoll.c
	$(CC) $(CFLAGS) -c polling_select.c polling_poll.c polling_epoll.c polling_policy.c

reactor.o: $(INCLUDE)/reactor.h reactor.c
	$(CC) $(CFLAGS) -c reactor.c

signal.o: $(INCLUDE)/signal.h signal.c
	$(CC) $(CFLAGS) -c signal.c

timer.o: $(INCLUDE)/timer.h timer.c
	$(CC) $(CFLAGS) -c timer.c

utility.o: $(INCLUDE)/utility.h utility.c
	$(CC) $(CFLAGS) -c utility.c

all:	$(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJECTS)
clean:
	rm *.o