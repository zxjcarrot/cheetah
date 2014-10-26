/*
* Copyright (c) 2014 Xinjing Chow
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERR
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cheetah/reactor.h"

struct reactor r;
struct event * events;
static int count, fired;
static int num_pipes, num_active;
static el_socket_t * pipes;
void read_cb(el_socket_t fd, short res_flags, void * arg){
	char buf[1024];
	int n = read(fd, buf, sizeof(buf));
	buf[n] = 0;
	//printf("got: %s\n", buf);
	++count;
}

static struct timeval * run_once(void){
	el_socket_t *cp, space;
	long i;
	static struct timeval ts, te;

	for(cp = pipes, i = 0; i < num_pipes; ++i, cp += 2){
		if(event_in_reactor(&events[i]))
			reactor_remove_event(&r, &events[i]);
		event_set(&events[i], cp[0], E_READ, read_cb, NULL);
		reactor_add_event(&r, &events[i]);
	}

	fired = 0;
	space = num_pipes / num_active;
	space = space * 2;
	for (i = 0; i < num_active; i++, fired++){
		//LOG("writing \"e\" to %d", pipes[i * space + 1]);
		write(pipes[i * space + 1], "e", 1);
		//perror("write");
	}
		
	count = 0;

	//LOG("start looping");
	{ int xcount = 0;
	el_gettimeofday(&ts);
	do {
		reactor_loop(&r, NULL, REACTOR_ONCE);
		xcount++;
	} while (count != fired);
	el_gettimeofday(&te);

	if (xcount != count) fprintf(stderr, "Xcount: %d, Rcount: %d\n", xcount, count);
	}
	//LOG("done looping");
	el_timesub(&te, &ts, &te);
	printf("time spent in event processing: %ldμs\n", te.tv_sec * 1000000 + te.tv_usec);
	return (&te);
}

int main(int argc, char const *argv[]){
	struct rlimit rl;
	int i, c;
	struct timeval *tv;
	static struct timeval ts, te;
	el_socket_t *cp;

	while((c = getopt(argc, argv, "a:n:")) != -1){
		switch(c){
			case 'a':
				num_active = atoi(optarg);
				break;
			case 'n':
				num_pipes = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Illegal argument \"%c\"\n", c);
				exit(1);
				break;
		}
	}

	rl.rlim_cur = rl.rlim_max = num_pipes * 2 + 50;
	if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
		perror("setrlimit");
		exit(1);
	}

	reactor_init(&r, NULL);

	events = calloc(num_pipes, sizeof(struct event));
	pipes = calloc(num_pipes << 1, sizeof(el_socket_t));
	if (events == NULL || pipes == NULL) {
		perror("calloc");
		exit(1);
	}

	for(cp = pipes, i = 0; i < num_pipes; ++i, cp += 2){
		if(el_create_pipe(cp) == -1){
			perror("pipe");
			exit(1);
		}
	}

	for (i = 0; i < 25; i++) {
		el_gettimeofday(&ts);
		tv = run_once();
		el_gettimeofday(&te);
		el_timesub(&te, &ts, &te);
		if (tv == NULL)
			exit(1);
		fprintf(stdout, "total time per iteration: %ldμs\n",
			tv->tv_sec * 1000000 + tv->tv_usec);
	}

	reactor_destroy(&r);
	free(events);
	free(pipes);
	return 0;
}