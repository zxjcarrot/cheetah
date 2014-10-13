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
#ifndef TIMER_H_
#define TIMER_H_
#include <sys/time.h>
#include "utility.h"

struct event;
struct reactor;
struct heap_entry{
	/* The expiration of the event */
	struct timeval expiration;
	struct event * e;
};

struct timerheap_internal{
	/* The number of entries in the heap */
	int size;
	/* The capacity of the heap */
	int capacity;
	/* The heap_entry array */
	struct heap_entry * heap;
};

/*
* Add v milliseconds to the timeval t
*/
#define timer_add(t, v)do{				\
	(t).tv_sec += (v) / 1000;			\
	(t).tv_usec += ((v) % 1000) * 1000;	\
	if((t).tv_usec > 1000000){			\
		(t).tv_usec -= 1000000;			\
		(t).tv_sec++;					\
	}									\
}while(0);

/*
* Substract v milliseconds from the timeval t
*/
#define timer_sub(t, v)do{				\
	(t).tv_sec -= (v) / 1000;			\
	(t).tv_usec -= ((v) % 1000) * 1000;	\
	if((t).tv_usec < 0){				\
		(t).tv_usec += 1000000;			\
		(t).tv_sec--;					\
	}									\
}while(0);

/*
* Convert the time in millisecond to timerval.
*/
#define timer_to_tv(tv, tm)do{			\
	(tv).tv_sec = (tm) / 1000;			\
	(tv).tv_usec = ((tm) % 1000) * 1000;	\
}while(0);								
/*
* Convert the timerval to time in millisecond.
*/
#define timer_to_ms(t) ((t).tv_sec * 1000 + (t).tv_usec / 1000)

/*
* Tests whether @t1 is greater or equal to @t2.
*/
#define timer_ge(t1, t2) ((t1).tv_sec > (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec >= (t2).tv_usec))

/*
* Tests whether @t1 is smaller or equal to @t2.
*/
#define timer_se(t1, t2) ((t1).tv_sec < (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec <= (t2).tv_usec))

/*
* Tests whether @t1 is greater to @t2.
*/
#define timer_g(t1, t2) ((t1).tv_sec > (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec > (t2).tv_usec))

/*
* Tests whether @t1 is smaller to @t2.
*/
#define timer_s(t1, t2)  ((t1).tv_sec < (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec < (t2).tv_usec))


#define TIMERHEAP_INIT_SIZE 32

/*
* Create and initialize a timerheap.
* Return: a newly created timerheap_internal structure on success, NULL on failure.
*/
struct timerheap_internal * timerheap_internal_init();

/*
* Tests whether the top entry is expired.
* Return: 0 on false, 1 on true.
* @r: the reactor which handles the timer events.
*/
int timerheap_top_expired(struct reactor * r);
/*
* Get the top entry's timeout value.
* Return: the expiration stored in timeval on success, NULL on failure(The timerheap is empty).
* @r: the reactor which handles the timer events.
*/
struct timeval * timerheap_top_timeout(struct reactor * r);
/*
* Retrieve the top event of the timerheap.
* Return: the top entry of the heap on success, NULL if the heap is empty.
* @r: the reactor which handles the timer events.
*/
struct event * timerheap_get_top(struct reactor * r);

/*
* Pop up the top event of the timerheap and reheapify the timerheap.
* Return: the top entry of the heap on success, NULL if the heap is empty.
* @r: the reactor which handles the timer events.
*/
struct event * timerheap_pop_top(struct reactor * r);

/*
* Add the timer event by its interval and reajust the heap.
* @r: the reactor which handles the timer events.
* @e: the timer event being manipulated.
*/
inline void timerheap_reset_timer(struct reactor * r, struct event * e);

/*
* Add timer event to the timerheap.
* Return: 0 on success, -1 on failure.
* @r: the reactor which handles the timer events.
* @e: the timer event to add.
*/
int timerheap_add_event(struct reactor * r, struct event * e);

/*
* Remove timer event from the timerheap.
* Return: 0 on success, -1 on failure.
* @r: the reactor which handles the timer events.
* @e: the timer event to remove.
*/
int timerheap_remove_event(struct reactor * r, struct event * e);

/*
* Free up the resources used by the timerheap and the timerheap_internal structure.
* Return: 0 on success, -1 on failure.
* @r: the reactor which handles the timer events.
*/
int timerheap_destroy(struct reactor * r);

#endif