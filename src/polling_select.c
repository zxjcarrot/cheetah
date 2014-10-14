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
/* select polling policy */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "cheetah/utility.h"
#include "cheetah/polling_policy.h"
#include "cheetah/event.h"

#ifdef WIN32
	#include <windows.h>
	#include <Winsock2.h>
#else
	#include <sys/select.h>
#endif

typedef unsigned long FD_MASK;

#define NBPB 8  /* Number of bits per byte */
#define NBPM (sizeof(FD_MASK) * NBPB) /* Number of bits per mask */
#define DIV_ROUND_UP(x, y) (((x) + (y - 1)) / (y))/* Rounding up division */
#define FD_TO_NUM_OF_INT(fd) DIV_ROUND_UP(fd, NBPM)/* Number of int needed to hold the given fd */
#define FD_TO_BYTES(fd) (FD_TO_NUM_OF_INT(fd) * sizeof(unsigned long))/* Number of bytes needed to hold the given fd */
#define FD_INIT_BYTES 32	/* Initial fd_set size */
struct select_internal{
	el_socket_t maxfd;
	int bytes_avail;
	fd_set *readset_in;
	fd_set *writeset_in;
	fd_set *readset_out;
	fd_set *writeset_out;
};

/*
* Resize the fd_sets to given size.
* Return value: 0 on success, -1 on failure.
* @psi: the internal data used by select.
* @bytes: the desired size in byte.
*/
static int select_resize(struct select_internal * psi, int bytes){
	fd_set * preadset_in = NULL;
	fd_set * pwriteset_in = NULL;
	fd_set * preadset_out = NULL;
	fd_set * pwriteset_out = NULL;

	assert(psi != NULL);
	if(psi == NULL){
		LOG("psi is null!!");
		return (-1);
	}
	if((preadset_in = realloc(psi->readset_in, bytes)) == NULL){
		return (-1);
	}
	if((pwriteset_in = realloc(psi->writeset_in, bytes)) == NULL){
		free(preadset_in);
		return (-1);
	}
	if((preadset_out= realloc(psi->readset_out, bytes)) == NULL){
		free(preadset_in);
		free(pwriteset_in);
		return (-1);
	}
	if((pwriteset_out = realloc(psi->writeset_out, bytes)) == NULL){
		free(preadset_in);
		free(pwriteset_in);
		free(preadset_out);
		return (-1);
	}
	psi->readset_in		= preadset_in;
	psi->writeset_in 	= pwriteset_in;
	psi->readset_out 	= preadset_out;
	psi->writeset_out 	= pwriteset_out;
	/* 
	* Zeros the newly appended memory.
	* We don't have to zeros the outs, because they will copy from the ins.
	*/
	memset(psi->readset_in + psi->bytes_avail, 0, bytes - psi->bytes_avail);
	memset(psi->writeset_in + psi->bytes_avail, 0, bytes - psi->bytes_avail);
	/*  */

	psi->bytes_avail = bytes;

	return (0);
}

/*
* Free up the internal data used by select.
* Return value: 0 for success.
* @psi: the internal data used by select.
*/
static int select_free(struct select_internal * psi){
	assert(psi != NULL);

	if(psi->readset_in)
		free(psi->readset_in);
	if(psi->writeset_in)
		free(psi->writeset_in);
	if(psi->readset_out)
		free(psi->readset_out);
	if(psi->writeset_out)
		free(psi->writeset_out);
	psi->readset_in = psi->writeset_in = NULL;
	psi->readset_out = psi->writeset_out = NULL;
	free(psi);
	LOG("done freeing policy_select internal data.");
	return (0);
}

/*
* Create and initialize the internal data used by select polling policy.
* Return value: newly created internal data on success, NULL on failure.
* @r: the reactor which uses this policy.
*/
void * select_init(struct reactor * r){
	struct select_internal * ret;

	assert(r != NULL);
	if(r == NULL){
		LOG("r is null!!");
		return NULL;
	}

	if((ret = malloc(sizeof(struct select_internal))) == NULL){
		return NULL;
	}

	memset(ret, 0, sizeof(struct select_internal));

	if(select_resize(ret, FD_TO_BYTES(FD_INIT_BYTES + 1)) == -1){
		select_free(ret);
		return NULL;
	}

	return ret;
}

/*
* Add the given file descriptor to the listening fd_set.
* Return value: 0 on success, -1 on failure.
* @r: the reactor which uses this policy.
* @fd: the file descriptor to listen.
* @flags: the interested events.
*/
int select_add(struct reactor * r, el_socket_t fd, short flags){
	struct select_internal * psi;
	assert(r != NULL);
	if(r == NULL){
		LOG("r is null!!");
		return (-1);
	}
	LOG("Adding a event with fd %d and flags %d", fd, flags);

	psi = r->policy_data;
	if(psi == NULL){
		LOG("pei is null!!");
		return (-1);
	}
	if(fd > psi->maxfd){
		int new_avail = psi->bytes_avail;

		//Doubles up to accommodate the fd
		while(new_avail < FD_TO_BYTES(fd + 1))
			new_avail <<= 1;

		if(new_avail != psi->bytes_avail){
			if(select_resize(psi, new_avail) == -1){
				LOG("memory shortage.");
				return (-1);
			}
		}
		//maintain the maximum fd for the select syscall
		psi->maxfd = fd;
	}

	if(flags & E_READ){
		FD_SET(fd, psi->readset_in);
	}
	if(flags & E_WRITE){
		FD_SET(fd, psi->writeset_in);
	}
	if(flags & E_EDGE){
		LOG("ET is not supported by select polling policy.");
	}
	LOG("Added a event with fd %d and flags %d", fd, flags);
	return (0);
}

/*
* Remove the given file descriptor from the listening fd_set.
* Return value: -1 on failure, 0 on success.
* @r: the reactor which uses this policy.
* @fd: the file descriptor to remove.
* @flags: the interested events.
*/
int select_del(struct reactor * r, el_socket_t fd, short flags){
	struct select_internal * psi;

	assert(r != NULL);
	LOG("Removing a event with fd %d and flags %d in the fd_set", fd, flags);
	
	psi = r->policy_data;

	assert(psi != NULL);
	if(psi == NULL){
		LOG("policy internal data is needed but null provided.");
		return (-1);
	}
	if(fd > psi->maxfd) return(0);

	if(flags & E_READ){
		FD_CLR(fd, psi->readset_in);
	}
	if(flags & E_WRITE){
		FD_CLR(fd, psi->writeset_in);
	}
	LOG("Removed a event with fd %d and flags %d in the fd_set", fd, flags);
	return (0);
}

/*
* Polling the file descriptors via select and add active events to the pending_list of the reactor.
* @r: the reactor which uses this policy.
* @timeout: the time after which the select will return.
*/
int select_poll(struct reactor * r, struct timeval * timeout){
	int res_flags , nreadys, fd;
	struct select_internal * psi;
	struct event * e;

	assert(r != NULL);
	if(r == NULL){
		LOG_EXIT(1, "r is null!!");
	}

	psi = r->policy_data;
	
	assert(psi != NULL);

	memcpy(psi->readset_out, psi->readset_in, psi->bytes_avail);
	memcpy(psi->writeset_out, psi->writeset_in, psi->bytes_avail);

	el_lock_unlock(r->lock);
	nreadys = select(psi->maxfd + 1, psi->readset_out, psi->writeset_out, NULL, timeout);
	el_lock_lock(r->lock);
	
	if(nreadys){
		for(fd = 0; fd <= psi->maxfd; ++fd){
			res_flags = 0;
			if(FD_ISSET(fd, psi->readset_out) && FD_ISSET(fd, psi->readset_in))res_flags |= E_READ;;
			if(FD_ISSET(fd, psi->writeset_out) && FD_ISSET(fd, psi->readset_in))res_flags |= E_WRITE;

			if(res_flags){
				e = event_ht_retrieve(&r->eht, fd);
				
				assert(e != NULL);
				if(e == NULL){
					LOG("the event with [fd %d] is not in the hashtable", fd);
				}else{
					reactor_add_to_pending(r, e, res_flags);
				}
			}
		}
	}
	

	return nreadys;
}

/*
* Clean up the policy internal data
* @r: the reactor which uses this policy
*/
void select_destroy(struct reactor * r){
	assert(r != NULL);

	select_free(r->policy_data);
}

/* Dumps out the internal data of select policy for debugging. */
void select_print(struct reactor * r){
	int i;
	struct select_internal * psi = r->policy_data;

	assert(r != NULL);

	psi = r->policy_data;
	
	printf("maxfd:%d\n", psi->maxfd);
	printf("bytes_avail:%d\n", psi->bytes_avail);
	for(i = 0; i <= psi->maxfd; ++i){
		printf("%d", i);
		if(FD_ISSET(i, psi->readset_in)){
			printf(" set for reading");
		}
		if(FD_ISSET(i, psi->writeset_in)){
			printf(" set for writing");
		}
		printf("\n");
	}
}