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

/* reactor pattern implementation */
#ifndef REACTOR_H_
#define REACTOR_H_

#include "utility.h"
#include "event_ht.h"
#include "lock.h"
#include "event.h"
#include "signal.h"
#include "timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/* exits after having processed at least one active event */
#define REACTOR_ONCE 0x01
struct polling_policy;
struct reactor{
	/* events registered */
	struct list_head event_list;
	/* active events waiting for being processed */
	struct list_head pending_list;
	
	struct polling_policy * policy;
	/* policy specified data, usually the back pointer of the reactor. */
	void * policy_data;

	/* event hash table */
	struct event_ht eht;

	/* 
	 * Lock to avoid race conditions.
	 * if null we assume this application is in single-threaded environment.
	 */
	el_lock * lock;

	/* Unnamed pipe used to wake up the polling thread. */
	el_socket_t pipe[2];
	struct event pe;

	/* Signal event internal data */
	struct signal_internal * psi;
	/* Unnamed pipe used to tell the reactor that a signal has occurred. */
	el_socket_t sig_pipe[2];
	struct event * sig_pe;

	/* Timer event internal data */
	struct timerheap_internal * pti;
	/* Unnamed pipe used to tell the reactor that a timer has expired. */
	el_socket_t timer_pipe[2];
	struct event * timer_pe;

	/* Indicates whether we should stop polling. */
	int out;
};

/*
* Wake up the polling thread and tell the reactor to get out the loop.
* @r: the reactor to wake up.
*/
void reactor_get_out(struct reactor * r);

/*
* Wrapper function for the initialization of reactor with normal functionality(I/O).
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with multithreading supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_mt(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with signal events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_signal(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with timer events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_timer(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with signal and timer events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_signal_timer(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with multithreading and signal events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_mt_signal(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with multithreading and timer events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_mt_timer(struct reactor * r, const char * policy_name);

/*
* Wrapper function for the initialization of reactor with multithreading, signal and timer events supported.
* @r: the reactor to be initialized.
* @policy_name: the name of polling policy.
*/
void reactor_init_with_mt_signal_timer(struct reactor * r, const char * policy_name);

/*
* Frees up resources related to the reactor.
* @r: the reactor to destroy.
*/
void reactor_destroy(struct reactor * r);
/*
* Add a event to the reactor.
* Return: 0 on success, -1 if the event is already in the reactor.
* @r: the reactor.
* @e: event to be added.
*/
int reactor_add_event(struct reactor * r, struct event * e);

/*
* Add a active event to the pending list waiting for processing.
* Return: 0 on success, -1 if the event is already in the pending list.
* @r: the reactor.
* @e: the event to add.
* @res_flags: The events have been set after polling.
*/
int reactor_add_to_pending(struct reactor * r, struct event * e, short res_flags);

/*
* Remove a event from the reactor.
* Return: 0 on success, -1 if the event is not in the reactor.
* @r: the reactor.
* @e: event to be removed.
*/
int reactor_remove_event(struct reactor * r, struct event * e);

/*
* Test whether the given reactor has no event.
* @r: the reactor to test.
*/
int reactor_event_empty(struct reactor * r);

/*
* Start the reactor.
* @r: the reactor to start.
* @timeout: The time after which pollling policy will return.
*/
void reactor_loop(struct reactor * r, struct timeval * timeout, int flags);

#ifdef __cplusplus
}
#endif
#endif /*REACTOR_H_*/