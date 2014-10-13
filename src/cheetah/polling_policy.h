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
/* generic polling policy */
#ifndef POLLING_POLICY_H_
#define	POLLING_POLICY_H_
#include "utility.h"
#include "reactor.h"
#ifdef __cplusplus
extern "C"{
#endif
struct reactor;
struct polling_policy{
	/* the policy name */
	const char * name;

	/* 
	* Initialize the polling policy.
	* Return: NULL on failure, the policy internal data on success.
	* @r: the reactor uses this policy.
	*/
	void *	(*init)(struct reactor * r);

	/* 
	* Add a fd to this polling policy.
	* Return: -1 on failure, 0 on success
	* @r: the reactor uses this policy.
	* @fd: the fd to listen.
	* @flags: the events interested.
	*/
	int 	(*add)(struct reactor * r, el_socket_t fd, short flags);

	/* 
	* Delete a fd from this polling policy.
	* @r: the reactor uses this policy.
	* @fd: the fd to remove.
	* @flags: the events interested.
	*/
	int 	(*del)(struct reactor * r, el_socket_t fd, short flags);

	/* 
	* Start polling.
	* Return: the number of fd on which some events have occured.
	* @r: the reactor that handles events.
	* @timeout: the time after which the poll will return in any case.
	*/
	int 	(*poll)(struct reactor * r, struct timeval * timeout);

	/* 
	* frees up the related resources used by the polling policy
	* @r: the reactor that handles events.
	*/
	void 	(*destroy)(struct reactor * r);

	/* for debugging */
	void	(*print)(struct reactor * r);
};

#ifdef __cplusplus
}
#endif
#endif /* POLLING_POLICY_H_ */