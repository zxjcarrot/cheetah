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

#include <cheetah/reactor.h>

struct reactor r;
struct event e;
int p[2];

void read_cb(el_socket_t fd, short res_flags, void * arg){
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    buf[n] = 0;
    fprintf(stderr, "got: %s\n", buf);
    fprintf(stderr, "modify to listen to write event\n");
    event_modify_events(&e, E_WRITE);
    if (reactor_modify_events(&r, &e) == -1) {
        exit(-1);
    }
}

void write_cb(el_socket_t fd, short res_flags, void * arg){
    fprintf(stderr, "write event is fired\n");
    reactor_get_out(&r);
}

int main(int argc, char const *argv[]){
    pipe(p);
	reactor_init(&r, NULL);
    event_set(&e, p[0], E_READ, read_cb, NULL);
    if (reactor_add_event(&r, &e) == -1) {
        fprintf(stderr, "failed to add event to the reactor\n");
        exit(-1);
    }
    write(p[1], "0", 1);
    reactor_loop(&r, NULL, 0);
	reactor_destroy(&r);
	return 0;
}