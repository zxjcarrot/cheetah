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
* /
/* epoll polling policy */
#include <sys/errno.h>
#include <sys/epoll.h>
#include <assert.h>

#include "config.h"
#include "cheetah/polling_policy.h"
#include "cheetah/includes.h"
#include "cheetah/log.h"


#define EPOLL_INIT_EVENT_SIZE 32

struct epoll_internal{
	int epoll_fd;
	int n_events;
	int max_events;
	struct epoll_event * events;
};

static void * epoll_init(struct reactor * r);
static int epoll_add(struct reactor * r, el_socket_t fd, short flags);
static int epoll_del(struct reactor * r, el_socket_t fd, short flags);
static int epoll_poll(struct reactor * r, struct timeval * timeout);
static void epoll_destroy(struct reactor * r);
static void epoll_print(struct reactor * r);

/*
* Resize the events to given size.
* Return: -1 on failure, 0 on success.
* @pei: the internal data used by epoll polling policy.
* @size: the size that we should resize to.
*/
static int epoll_resize(struct epoll_internal * pei, int size){
	struct epoll_event * pee;
	assert(pei != NULL);
	if(pei == NULL){
		LOG("pei is null!!");
		return (-1);
	}

	if((pee = realloc(pei->events, size * sizeof(struct epoll_event))) == NULL){
		LOG("failed to realloc for events, maybe run out of memory.");
		return (-1);
	}

	pei->events = pee;
	pei->max_events = size;
	return (0);
}

/*
* Create and initialize the internal data used by epoll polling policy.
* Return value: newly created internal data on success, NULL on failure.
* @r: the reactor which uses this policy.
*/
static void * epoll_init(struct reactor * r){
	struct epoll_internal * ret;

	assert(r);
	if(r == NULL){
		LOG("r is null!!");
		return NULL;
	}
	
	if((ret = malloc(sizeof(struct epoll_internal))) == NULL){
		LOG("failed to malloc for epoll_internal");
		return NULL;
	}

	memset(ret, 0, sizeof(struct epoll_internal));
	
	if((ret->epoll_fd = epoll_create(EPOLL_INIT_EVENT_SIZE)) == -1){
		LOG("failed on epoll_create");
		free(ret);
		return NULL;
	}

	if(epoll_resize(ret, EPOLL_INIT_EVENT_SIZE) == -1){
		LOG("failed on epoll_resize");
		close(ret->epoll_fd);
		free(ret);
		return NULL;
	}

	return ret;
}

/* 
* Frees up the internal data used by epoll polling policy.
* @pei: the internal data.
*/
static void epoll_free(struct epoll_internal * pei){
	assert(pei != NULL);
	if(pei == NULL){
		LOG("pei is null!!");
		return;
	}

	if(pei->events){
		free(pei->events);
		pei->events = NULL;
	}
	if(pei->epoll_fd >= 0){
		close(pei->epoll_fd);
	}

	free(pei);
}

/*
* Clean up the policy internal data
* @r: the reactor which uses this policy
*/
static void epoll_destroy(struct reactor * r){
	assert(r != NULL);
	if(r == NULL){
		LOG("r is null!!");
		return;
	}
	epoll_free(r->policy_data);
}

static inline short epoll_setup_mask(short flags){
	short ret = 0;
	if(flags & E_READ){
		ret |= EPOLLIN | EPOLLPRI;
	}
	if(flags & E_WRITE){
		ret |= EPOLLOUT;
	}
	if(flags & E_EDGE){
		ret |= EPOLLET;
	}
	if(flags & E_ONCE){
		ret |= EPOLLONESHOT;
	}
	return ret;
}

static int epoll_print_error(struct epoll_internal * pei, el_socket_t fd){
	if(errno == EBADF){
		LOG("[epoll_fd %d]or [fd %d]is not valid!!", pei->epoll_fd, fd);
	}else if(errno == ENOENT){
		LOG("[fd %d] is not registered with this epoll instance.", fd);
	}else if(errno == EINVAL){
		LOG("[epoll_fd %d] is not an epoll file descriptor, or [fd %d]is the same as [epoll_fd %d],or the requested operation EPOLL_CTL_ADD is not supported  by  this interface.", pei->epoll_fd, fd, pei->epoll_fd);
	}else if(errno == ENOMEM){
		LOG("memory shorage");
	}else if(errno == ENOSPC){
		LOG("The limit imposed by /proc/sys/fs/epoll/max_user_watches exceeded.");
	}else if(errno == EPERM){
		LOG("The target file [fd %d] does not support epoll. "
			"It's meaningless to epolling on regular files, read this post through [ http://www.groupsrv.com/linux/about159067.html ].", fd);
	}
}

/*
* Register the given file descriptor with this epoll instance.
* Return: 0 on success, -1 on failure.
* @r: the reactor which uses this policy.
* @fd: the file descriptor to listen.
* @flags: the interested events.
*/
static int epoll_add(struct reactor * r, el_socket_t fd, short flags){
	struct epoll_internal * pei;
	struct epoll_event e;
	int ret;
	
	assert(r != NULL);
	if(r == NULL){
		LOG("r is null!!");
		return (-1);
	}

	pei = r->policy_data;
	if(pei == NULL){
		LOG("pei is null!!");
		return (-1);
	}

	if(pei->n_events >= pei->max_events){
		LOG("resize to %d", pei->max_events << 1);
		if(epoll_resize(pei, pei->max_events << 1) == -1){
			LOG("failed on epoll_resize");
			return (-1);
		}
	}
	e.data.fd = fd;
	e.events = epoll_setup_mask(flags);
	
	ret = epoll_ctl(pei->epoll_fd, EPOLL_CTL_ADD, fd, &e);

	/* Error handling*/
	if(ret){
		if(errno == EEXIST){
			LOG("[fd %d]is alredy registered with this epoll instance, retry with EPOLL_CTL_MOD.", fd);
			/* retry with EPOLL_CTL_MOD */
			ret = epoll_ctl(pei->epoll_fd, EPOLL_CTL_MOD, fd, &e);

			if(ret == 0) 
				goto success;
			epoll_print_error(pei, fd);
		}else{
			epoll_print_error(pei, fd);
		}
		return (-1);
	}
	success:
	//LOG("success on registering [fd %d] with this epoll instance", fd);
	++pei->n_events;
	return (0);
}

/*
* Unregister the given file descriptor with this epoll instance.
* Return: -1 on failure, 0 on success.
* @r: the reactor which uses this policy.
* @fd: the file descriptor to remove.
* @flags: the interested events.
*/
static int epoll_del(struct reactor * r, el_socket_t fd, short flags){
	struct epoll_internal * pei;
	struct epoll_event e;
	int ret;
	assert(r != NULL);
	if(r == NULL){
		LOG("r is null!!");
		return (-1);
	}

	pei = r->policy_data;
	if(pei == NULL){
		LOG("pei is null!!");
		return (-1);
	}

	e.data.fd = fd;
	e.events = epoll_setup_mask(flags);
	
	ret = epoll_ctl(pei->epoll_fd, EPOLL_CTL_DEL, fd, &e);

	if(ret){
		epoll_print_error(pei, fd);
		return (-1);
	}

	//LOG("success on unregistering [fd %d] with this epoll instance", fd);
	--pei->n_events;
	return (0);
}

/*
* Polling the file descriptor via epoll and add active events to the pending_list of the reactor.
* @r: the reactor which uses this policy.
* @timeout: the time after which the select will return.
*/
static int epoll_poll(struct reactor * r, struct timeval * timeout){
	int res_flags , nreadys, fd, i;
	struct epoll_internal * pei;
	struct event * e;

	assert(r != NULL);
	if(r == NULL){
		LOG_EXIT(1, "r is null!!");
	}

	pei = r->policy_data;
	
	assert(pei != NULL);
	if(pei == NULL){
		LOG_EXIT(1, "pei is null");
	}

	el_lock_unlock(r->lock);
	nreadys = epoll_wait(pei->epoll_fd,
						pei->events,
						pei->n_events,
						timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1 );
	el_lock_lock(r->lock);

	for(i = 0; i < nreadys; ++i){
		res_flags = 0;
		if(pei->events[i].events & (EPOLLIN | EPOLLPRI)){
			res_flags |= E_READ;
		}
		if(pei->events[i].events & EPOLLOUT){
			res_flags |= E_WRITE;
		}
		if(pei->events[i].events & EPOLLERR){
			LOG("got a EPOLLERR event: %s", strerror(errno));
		}
		if(res_flags){
			e = event_ht_retrieve(&r->eht, pei->events[i].data.fd);
				
			assert(e != NULL);
			if(e == NULL){
				LOG("the event with [fd %d] is not in the hashtable", pei->events[i].data.fd);
			}else{
				reactor_add_to_pending(r, e, res_flags);
			}
		}
	}

	el_lock_unlock(r->lock);

	return nreadys;
}
/* Dumps out the internal data of select policy for debugging. */
static void epoll_print(struct reactor * r){
	LOG("empty implementation of epoll_print.");
}
