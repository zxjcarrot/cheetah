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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>	
#include <sys/stat.h>
#include <sys/types.h>

#include "cheetah/reactor.h"
#include "cheetah/event.h"
#include "cheetah/list.h"
#include "cheetah/log.h"
#include "cheetah/polling_policy.h"
#include "cheetah/utility.h"

extern struct polling_policy polling_policies[];

static void reactor_signal_callback(el_socket_t fd, short res_flags, void *arg){
	struct reactor * r;
	struct event * e;
	struct signal_internal * psi;
	int sig;

	sig = 0;
	r = arg;
	psi = r->psi;
	assert(r != NULL && psi != NULL);

	while(read(fd, &sig, 1) > 0){
		e = psi->sigevents[sig];

		assert(e != NULL && e->callback != NULL);

		e->callback(e->fd, E_SIGNAL, e->callback_arg);
	}
}

static void reactor_handle_timer(struct reactor * r){
	struct event * e;

	assert(r != NULL);

	while(timerheap_top_expired(r)){
		e = timerheap_get_top(r);
		
		assert(e != NULL);

		el_lock_unlock(r->lock);
		e->callback(e->fd, e->res_flags, e->callback_arg);
		el_lock_lock(r->lock);
		if(e->ev_flags & E_ONCE && e->timerheap_idx != E_OUT_OF_TIMERHEAP){
			timerheap_remove_event(r, e);
		}else if(!(e->ev_flags & E_ONCE) && e->timerheap_idx != E_OUT_OF_TIMERHEAP){
			timerheap_reset_timer(r, e);
		}
	}
}

inline static void reactor_waked_up(el_socket_t fd, short res_flags, void *arg){
	//LOG("woke up.");
	int n;
	char buf[1024];
	while((n = read(fd, buf, sizeof(buf))) > 0);
}

inline static void reactor_wake_up(struct reactor * r){
	assert(r != NULL);

	char octet = 0;
	write(r->pipe[1], &octet, sizeof(octet));
}

inline void reactor_get_out(struct reactor * r){
	el_lock_lock(r->lock);
	r->out = 1;
	reactor_wake_up(r);
	el_lock_unlock(r->lock);
}

static void reactor_init_(struct reactor * r, const char * policy_name, int in_mt, int handle_sig, int handle_timer){
	assert(r != NULL);

	memset(r, 0, sizeof(struct reactor));
	if(event_ht_init(&r->eht, 0.5) == -1){
		LOG_EXIT(1, "failed to initialize the hash table of events.");
	}

	INIT_LIST_HEAD(&r->event_list);
	INIT_LIST_HEAD(&r->pending_list);

	struct polling_policy * p = polling_policies;
	/* Find the corresponding policy by looping through predefined policies. */
	while(p){
		/* 
		* We choose the first one in the array if policy_name is null,
		* since we have sorted the array by the performance of these polling mechanisms.
		*/
		if(policy_name == NULL || !strcmp(p->name, policy_name)){
			r->policy = malloc(sizeof(*p));
			if(r->policy == NULL){
				LOG_EXIT(1, "failed to allocate memory for policy.");
			}
			memcpy(r->policy, p, sizeof(*p));
			break;
		}
		p++;
	}
	if(p == NULL){
		LOG_EXIT(1, "polling policy: %s is not supported.", policy_name);
	}

	/* Policy internal initialization. */
	r->policy_data = r->policy->init(r);
	if(r->policy_data == NULL){
		LOG_EXIT(1, "failed to initialize polling policy[%s].", policy_name);
	}

	if(in_mt){
		/* We only use lock in multithreaded environment to reduce overhead. */
		r->lock = malloc(sizeof(el_lock));
	}

	/* If the lock is null, locking functions will be no-ops */
	el_lock_init(r->lock);
	r->out = 0;
	if(el_create_pipe(r->pipe) == -1){
		LOG_EXIT(1, "failed to create informing pipe.");
	}
	/* set the pipe to nonblocking mode */
	if(el_set_nonblocking(r->pipe[0]) < 0 || el_set_nonblocking(r->pipe[1]) < 0){
		LOG_EXIT(1, "failed to set the pipe to nonblocking mode.");
	}

	//set up the informer
	event_set(&r->pe, r->pipe[0], E_READ, reactor_waked_up, NULL);
	reactor_add_event(r, &r->pe);

	if(handle_sig){
		//set up signal events handling
		if(reacotr_signal_init(r) == -1){
			LOG("failed to initialize signal handling.");
			reactor_destroy(r);
		}
	}
	if(handle_timer){
		//set up timer events handling
		if(reactor_timer_init(r) == -1){
			LOG("failed to initialize signal handling.");
			reactor_destroy(r);
		}
	}
}

inline void reactor_init(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 0, 0, 0);
}
inline void reactor_init_with_mt(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 1, 0, 0);
}
inline void reactor_init_with_signal(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 0, 1, 0);
}

inline void reactor_init_with_timer(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 0, 0, 1);
}

inline void reactor_init_with_signal_timer(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 0, 1, 1);
}

inline void reactor_init_with_mt_signal(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 1, 1, 0);
}

inline void reactor_init_with_mt_timer(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 1, 0, 1);
}

inline void reactor_init_with_mt_signal_timer(struct reactor * r, const char * policy_name){
	reactor_init_(r, policy_name, 1, 1, 1);
}

static inline void reactor_free_events(struct reactor * r){
	struct event * e = NULL;

	while(e = reactor_dequeue_event(r)){
		if(r->policy->del(r, e->fd, e->ev_flags) == -1){
			LOG("failed to remove the event[%d] from the reactor.", e->fd);
		}
		list_del(&e->event_link);

		event_ht_delete(&r->eht, e);

		list_del(&e->pending_link);

		e->rc = NULL;
	}
}


static inline void reactor_free_hash(struct reactor * r){
	event_ht_free(&r->eht);
}

inline void reactor_destroy(struct reactor * r){
	el_lock_lock(r->lock);
	LOG("frees up event_list.");
	//frees up event_list
	reactor_free_events(r);

	LOG("frees up hash table.");
	//frees up hash table
	reactor_free_hash(r);
	
	LOG("free up polling policy.");
	//free up polling policy
	r->policy->destroy(r);
	free(r->policy);
	r->policy = r->policy_data = NULL;

	//close the pipe
	el_close_fd(r->pipe[0]);
	el_close_fd(r->pipe[1]);

	//close the signal event handling stuff
	if(r->psi){
		el_close_fd(r->sig_pipe[0]);
		el_close_fd(r->sig_pipe[1]);
		signal_internal_restore_all(r);
		free(r->psi);
		free(r->sig_pe);
	}
	
	//close the timer event handling stuff
	if(r->pti){
		el_close_fd(r->sig_pipe[0]);
		el_close_fd(r->sig_pipe[1]);
		timerheap_destroy(r);
		free(r->sig_pe);
	}
	el_lock_unlock(r->lock);
	//free up the lock
	el_lock_destroy(r->lock);
	free(r->lock);
}

inline int reacotr_signal_init(struct reactor * r){
	if((r->psi = signal_internal_init(r)) == NULL){
		LOG("failed on signal_internal_init");
		return (-1);
	}

	if(el_create_pipe(r->sig_pipe) == -1){
		LOG(1, "failed to create signal informing pipe.");
	}

	/* set the pipe to nonblocking mode */
	if(el_set_nonblocking(r->sig_pipe[0]) < 0 || el_set_nonblocking(r->sig_pipe[1]) < 0){
		LOG(1, "failed to set the pipe to nonblocking mode.");
		return (-1);
	}

	if((r->sig_pe = event_new(r->sig_pipe[0], E_READ, reactor_signal_callback, r)) == NULL){
		LOG(1, "failed to create event for signal events handling");
		return (-1);
	}

	reactor_add_event(r, r->sig_pe);

	return (0);
}

inline int reactor_timer_init(struct reactor * r){
	if((r->pti = timerheap_internal_init(r)) == NULL){
		LOG("failed on timerheap_internal_init: %s", strerror(errno));
		return (-1);
	}
	return (0);
}

inline int reactor_add_event(struct reactor * r, struct event * e){
	assert(r != NULL && e != NULL);

	el_lock_lock(r->lock);

	if(e->ev_flags & E_SIGNAL || e->ev_flags & E_TIMEOUT){
		/* Signal or timer event registration */
		if(e->ev_flags & E_SIGNAL && e->ev_flags & E_TIMEOUT){
			el_lock_unlock(r->lock);
			LOG("Inlivad flags[%d], E_SIGNAL and E_TIMEOUT are mutually exclusive.", e->ev_flags);
			return (-1);
		}else if(e->ev_flags & E_SIGNAL){
			if(signal_internal_register(r, (int)e->fd, e) == -1){
				el_lock_unlock(r->lock);
				LOG("failed to register event for signal[%d]", (int)e->fd);
				return (-1);
			}
		}else{//E_TIMEOUT
			int size = r->pti->size;
			assert(e->timerheap_idx == E_OUT_OF_TIMERHEAP);
			if(timerheap_add_event(r, e) == -1){
				el_lock_unlock(r->lock);
				LOG("failed to register timer event");
				return (-1);
			}
			assert(r->pti->size == size + 1);
			assert(e->timerheap_idx != E_OUT_OF_TIMERHEAP);
		}
	}else{
		/* Normal I/O event registration. */
		if(e->event_link.prev || e->event_link.next){
			el_lock_unlock(r->lock);
			/*
			* This event is already in the reactor.
			* Assume every event only can be in one reactor.
			*/
			return (-1);
		}
		//LOG("Adding a event [fd %d]", e->fd);
		if(r->policy->add(r, e->fd, e->ev_flags) == -1){
			el_lock_unlock(r->lock);
			LOG("failed to add the event[%d] to the reactor.", e->fd);
			return (-1);
		}
		//LOG("Added a event [fd %d]", e->fd);
		list_add_tail(&e->event_link, &r->event_list);

		event_ht_insert(&r->eht, e, e->fd);
	}

	e->rc = r;
	e->ev_flags |= E_IN_REACTOR;

	//The polling thread might be sleeping indefinitely, wake it up.
	reactor_wake_up(r);

	el_lock_unlock(r->lock);

	return (0);
}

inline int reactor_add_to_pending(struct reactor * r, struct event * e, short res_flags){
	assert(r != NULL && e != NULL);

	e->res_flags = res_flags;
	
	if(e->pending_link.prev || e->pending_link.next){
		/*
		* This event is alrady in the pending list.
		* Assume every event only can be in one reactor.
		*/
		return (-1);
	}

	list_add_tail(&e->pending_link, &r->pending_list);

	return (0);
}

inline int reactor_remove_event(struct reactor * r, struct event * e){
	assert(r != NULL && e != NULL);

	el_lock_lock(r->lock);
	if(e->ev_flags & E_SIGNAL || e->ev_flags & E_TIMEOUT){
		/* Signal or timer event unregistration */
		if(e->ev_flags & E_SIGNAL && e->ev_flags & E_TIMEOUT){
			el_lock_unlock(r->lock);
			LOG("Inlivad flags[%d], E_SIGNAL and E_TIMEOUT are mutually exclusive.", e->ev_flags);
			return (-1);
		}else if(e->ev_flags & E_SIGNAL){
			if(signal_internal_unregister(r, (int)e->fd) == -1){
				el_lock_unlock(r->lock);
				LOG("failed to unregister event for signal[%d]", (int)e->fd);
				return (-1);
			}
		}else{//E_TIMEOUT
			int size = r->pti->size;
			assert(e->timerheap_idx != E_OUT_OF_TIMERHEAP);
			if(timerheap_remove_event(r, e) == -1){
				el_lock_unlock(r->lock);
				LOG("failed to unregister time event");
				return (-1);
			}
			assert(r->pti->size == size - 1);
			assert(e->timerheap_idx == E_OUT_OF_TIMERHEAP);
		}
	}else{
		/* Normal I/O event unregistration. */
		if(e->event_link.prev == NULL || e->event_link.next == NULL){
			el_lock_unlock(r->lock);
			LOG("The event is not in the reactor.");
			/*
			* This event is not in the reactor.
			* Assume every event only can be in one reactor.
			*/
			return (-1);
		}
		//LOG("Removing a event [fd %d]", e->fd);
		if(r->policy->del(r, e->fd, e->ev_flags) == -1){
			el_lock_unlock(r->lock);

			LOG("failed to remove the event[%d] from the reactor.", e->fd);
			return (-1);
		}
		//LOG("Removed a event [fd %d]", e->fd);

		list_del(&e->event_link);

		event_ht_delete(&r->eht, e);

		list_del(&e->pending_link);
	}

	e->rc = NULL;
	e->ev_flags &= ~E_IN_REACTOR;

	//The polling thread might be sleeping indefinitely, wake it up.
	reactor_wake_up(r);

	el_lock_unlock(r->lock);
}

static inline struct event * reactor_dequeue_pending(struct reactor * r){
	struct list_head * node;
	struct event * e;

	assert(r);

	if(list_empty(&r->pending_list))
		return NULL;

	node = r->pending_list.next;
	e = list_entry(node, struct event, pending_link);

	list_del(&e->pending_link);
	return e;
}

static inline struct event * reactor_dequeue_event(struct reactor * r){
	struct list_head * node;
	struct event * e;

	assert(r != NULL);

	if(list_empty(&r->event_list))return NULL;

	node = r->event_list.next;
	e = list_entry(node, struct event, event_link);

	list_del(&e->event_link);
	return e;
}

inline int reactor_event_empty(struct reactor * r){
	assert(r != NULL);

	return list_empty(&r->event_list);
}

inline void reactor_loop(struct reactor * r, struct timeval * timeout, int flags){
	int nreadys;
	struct timeval *pt, t;
	struct event * e;

	assert(r != NULL);

	while(!r->out){
		el_lock_lock(r->lock);
		//LOG("start polling with timeout [%d, %d]", timeout ? timeout->tv_sec : -1, timeout ? timeout->tv_usec : -1);

		/*
		* On linux, the select syscall modifies timeout
		* to reflect the amount of time not slept.
		* We have to reset the timeout in order to be portable.
		*/
		if(timeout == NULL){
			pt = NULL;
		}else{
			t = *timeout;
			pt = &t;
		}

		if(r->pti){
			/* 
			* The timer event handling is supported,
			* have @pt point to the smallest timeval.
			*/
			struct timeval * timerv = timerheap_top_timeout(r);
			if(timerv && (pt == NULL || pt && timer_s(*timerv, *pt))){
				t = *timerv;
				pt = &t;
			}
		}
		nreadys = r->policy->poll(r, pt);
		//LOG("stopped polling, got %d readys", nreadys);
		if(r->pti){
			/* handle timer events */
			reactor_handle_timer(r);
		}
		if(nreadys){
			//iterate through pending events and call corresponding callbacks
			while(e = reactor_dequeue_pending(r)){
				if(e->callback){
					el_lock_unlock(r->lock);

					e->callback(e->fd, e->res_flags, e->callback_arg);

					el_lock_lock(r->lock);
				}
			}
			if(flags & REACTOR_ONCE){
				r->out = 1;
			}
		}
		el_lock_unlock(r->lock);
	}
	//LOG("told to stop the loop, stopped.");
	//reset the flag for next loop
	r->out = 0;
}