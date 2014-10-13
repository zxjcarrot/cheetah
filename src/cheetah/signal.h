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
#ifndef EL_SIGNAL_H_
#define EL_SIGNAL_H_

#define SIGNALN 65

typedef void (* signal_handler)(int);

struct event;
struct reactor;
struct signal_internal{
	/* Used to restore original signal handler */
	signal_handler old_hanlders[SIGNALN];

	/* 
	* Every signal only has one registered event.
	* The last one will take effect if there
	* are multiple events registering to the 
	* same signal.
	*/
	struct event * sigevents[SIGNALN];
};

/*
* Signal handler to inform the reactor that a signal has occured.
* @sig: the signal that just occured.
*/
void sig_handler(int sig);

/*
* Allocate and initialize the signal_internal structure.
* Return: newly created signal_internal on success, NULL on failure.
* @r: the reactor that handles signals.
*/
struct signal_internal * signal_internal_init(struct reactor * r);

/*
* Register a signal to the signal_internal.
* Return: 0 on success, -1 on failure.
* @r: the related reacotr.
* @sig: the signal to regitser.
* @e: the signal event.
*/
int signal_internal_register(struct reactor * r, int sig, struct event * e);

/*
* Unregister a signal from the signal_internal.
* Return: 0 on success, -1 on failure.
* @r: the related reacotr.
* @sig: the signal to unregitser.
*/
int signal_internal_unregister(struct reactor * r, int sig);
#endif