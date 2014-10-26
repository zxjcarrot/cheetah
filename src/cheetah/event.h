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
#ifndef EVENT_H_
#define EVENT_H_

#include <stdlib.h>

#include "list.h"
#include "utility.h"

#ifdef __cplusplus
extern "C"{
#endif

#define E_READ  		0x1
#define E_WRITE 		0x2
#define E_SIGNAL		0x4 
#define E_TIMEOUT		0x8 
#define E_EDGE 			0x10/* edge triggered */
#define E_ONCE			0x20/* one-time event */
#define E_IN_REACTOR 	0x40/* indicates whether the event is in the reactor */
/* 
* The timer event's initial timerheap_idx value,
* indicates that the event is not in the heap.
*/
#define E_OUT_OF_TIMERHEAP 0 

struct reactor;

typedef void (*event_callback)(el_socket_t fd, short res_flags, void *arg);
struct event{
	struct list_head event_link;
	struct list_head pending_link;
	struct list_head hash_link;
	/* 
	* For I/O events, fd stores the file descriptor being listened.
	* For signal events, fd stores the signal number being listened.
	* For timer events, fd stores the timer interval in millisecond.
	*/
	el_socket_t fd;

	/* Event type bitmask */
	short ev_flags;
	/* Back pointer of the reactor holds the event */
	struct reactor * rc;

	/* The events have been set after polling */
	short res_flags;
	
	event_callback callback;
	void * callback_arg;

	/* Only used for timer event */
	int timerheap_idx;
};

/*
* Create and initialize a event.
* For I/O events, fd stores the file descriptor being listened.
* For signal events, fd stores the signal number being listened.
* For timer events, fd stores the timer interval in millisecond.
* Return the newly created event.
* @fd: the file descriptor to listen
* @ev_flags: interested events bitmask
* @call_back: the function to call when the specific events has occured
* @arg: call_back data
*/
struct event * event_new(el_socket_t fd, short ev_flags,event_callback callback, void * arg);

/*
* Initialize a given event.
* For I/O events, fd stores the file descriptor being listened.
* For signal events, fd stores the signal number being listened.
* For timer events, fd stores the timer interval in millisecond.
* @e: event to initialize
* @fd: the file descriptor to listen
* @ev_flags: interested events bitmask
* @call_back: the function to call when the specific events has occured
* @arg: call_back data
*/
void event_set(struct event * e, el_socket_t fd, short ev_flags,event_callback callback, void * arg);

/*
* Tests whether the event is in the reactor.
* Return: 0 for false, 1 for true.
* @e: event to test.
*/
int event_in_reactor(struct event * e);
#ifdef __cplusplus
}
#endif
#endif /*EVENT_H_*/