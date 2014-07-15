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
#include <assert.h>

#include "cheetah/reactor.h"
struct reactor r;
struct event e[14];

int count;
void timer_callback(el_socket_t fd, short flags, void * arg){
	time_t t;
	++count;
	time(&t);
	//LOG("%s\n", ctime(&t));
}
void * thread_func(void * arg){
	pthread_detach(pthread_self());
	struct event * pe = arg;
	sleep(10);
	LOG("thread[%u]:Removing the timer event", pthread_self());
	reactor_remove_event(&r, pe);
	sleep(5);
	event_set(pe, 1000, E_TIMEOUT, timer_callback, pe);
	LOG("thread[%u]:adding the timer event", pthread_self());
	reactor_add_event(&r, pe);
	sleep(10);
	reactor_remove_event(&r, pe);
	if(pe == &e[0]){
		reactor_get_out(&r);
	}
}
int main(int argc, char const *argv[]){
	pthread_t thread_id[14];
	int i;
	reactor_init_with_mt_timer(&r, NULL);
	for(i = 0; i < 14; ++i){ 
		event_set(&e[i], (i + 1) * 400, E_TIMEOUT, timer_callback, &e[i]);
		reactor_add_event(&r, &e[i]);	
		pthread_create(&thread_id[i], NULL, thread_func, &e[i]);
	}
	reactor_loop(&r, NULL, 0);
	//sleep(10);
	reactor_destroy(&r);
	LOG("count:%d", count);
	return 0;
}