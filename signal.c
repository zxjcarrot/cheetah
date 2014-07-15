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
/* signal polling policy */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#include "cheetah/utility.h"
#include "cheetah/polling_policy.h"
#include "cheetah/event.h"
#include "cheetah/reactor.h"
#include "cheetah/signal.h"

#ifdef WIN32
	#include <windows.h>
	#include <Winsock2.h>
	#define __cdecl
#else
	#include <sys/select.h>
#endif

/*
* The reactor that is listening for signal events.
*/
static struct reactor * current_reactor;

void sig_handler(int sig){
	assert(current_reactor != NULL);
	if(current_reactor == NULL){
		LOG("current_reactor is null!!");
		return;
	}

	write(current_reactor->sig_pipe[1], &sig, 1);
}


struct signal_internal * signal_internal_init(struct reactor * r){
	struct signal_internal *ret ;
	assert(r != NULL);
	
	if(current_reactor && current_reactor != r){
		LOG("Only one reactor can handle signal events.");
		return NULL;
	}

	if((ret = malloc(sizeof(struct signal_internal))) == NULL){
		LOG("failed to malloc for signal_internal.");
		return NULL;
	}

	memset(ret, 0, sizeof(struct signal_internal));

	current_reactor = r;
	return ret;
}


int signal_internal_register(struct reactor * r, int sig, struct event * e){
	struct signal_internal * psi;

	assert(r != NULL && e != NULL);
	if(current_reactor == NULL){
		LOG("The current_reactor hasn't been set.");
		return (-1);
	}else if(current_reactor != r){
		LOG("Only one reactor can handle signal events.");
		return (-1);
	}else if(r->psi == NULL){
		LOG("The signal_internal hasn's been set.");
		return (-1);
	}else if(e == NULL){
		LOG("event can not be null");
		return (-1);
	}else if(sig < 1 || sig >= SIGNALN){
		LOG("signal num[%d] is out of range[1-64].");
		return (-1);
	}

	psi = r->psi;

	if((psi->old_hanlders[sig] = signal(sig, sig_handler)) == SIG_ERR){
		LOG("signal failed: %s", strerror(errno));
		return (-1);
	}

	psi->sigevents[sig] = e;

	return (0);
}

int signal_internal_restore_all(struct reactor * r){
	struct signal_internal * psi;
	int i;

	assert(r != NULL);
	if(current_reactor == NULL){
		LOG("The current_reactor hasn't been set.");
		return (-1);
	}else if(current_reactor != r){
		LOG("Only one reactor can handle signal events.");
		return (-1);
	}else if(r->psi == NULL){
		LOG("The signal_internal hasn's been set.");
		return (-1);
	}
	
	psi = r->psi;

	for(i = 1; i < SIGNALN; ++i){
		if(psi->old_hanlders[i] && signal(i, psi->old_hanlders[i]) == SIG_ERR){
			LOG("signal failed: %s", strerror(errno));
		}
		psi->sigevents[i] = NULL;
	}

	return (0);
}
int signal_internal_unregister(struct reactor * r, int sig){
	struct signal_internal * psi;

	assert(r != NULL);
	if(current_reactor == NULL){
		LOG("The current_reactor hasn't been set.");
		return (-1);
	}else if(current_reactor != r){
		LOG("Only one reactor can handle signal events.");
		return (-1);
	}else if(r->psi == NULL){
		LOG("The signal_internal hasn's been set.");
		return (-1);
	}else if(sig < 1 || sig >= SIGNALN){
		LOG("signal num[%d] is out of range(1-64).");
		return (-1);
	}
	
	psi = r->psi;

	if(signal(sig, psi->old_hanlders[sig]) == SIG_ERR){
		LOG("signal failed: %s", strerror(errno));
		return (-1);
	}

	psi->sigevents[sig] = NULL;

	return (0);
}