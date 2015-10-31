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
struct event e;

void sigint_callback(el_socket_t fd, short flags, void * arg){
	static int count = 0;
	LOG("sigint_callback got called");
	struct reactor * r = arg;
	reactor_get_out(r);
}

void * thread_func(void * arg){
	pthread_detach(pthread_self());
	sleep(10);
	LOG("Removing the signal event");
	reactor_remove_event(&r, &e);

	return NULL;
}

int main(int argc, char const *argv[]){
	reactor_init_with_signal(&r, NULL);

	event_set(&e, SIGINT, E_SIGNAL, sigint_callback, &r);
	reactor_add_event(&r, &e);

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, thread_func, NULL);

	reactor_loop(&r, NULL, 0);

	reactor_destroy(&r);

	return 0;
}