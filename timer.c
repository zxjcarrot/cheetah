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
#include <string.h>
#include <errno.h>
#include <memory.h>
#include <assert.h>

#include "cheetah/log.h"
#include "cheetah/timer.h"
#include "cheetah/reactor.h"
#include "cheetah/event.h"
#include "cheetah/utility.h"

struct timerheap_internal * timerheap_internal_init(){
	struct timerheap_internal * ret;
	struct heap_entry * heap;
	if((ret = malloc(sizeof(struct timerheap_internal))) == NULL){
		LOG("failed to malloc for timerheap_internal: %s", strerror(errno));
		return NULL;
	}
	memset(ret, 0, sizeof(struct timerheap_internal));
	
	if((heap = malloc(TIMERHEAP_INIT_SIZE * sizeof(struct heap_entry))) == NULL){
		LOG("failed to malloc for heap_entry: %s", strerror(errno));
		free(ret);
		return NULL;
	}
	ret->heap = heap;
	ret->size = 0;
	ret->capacity = TIMERHEAP_INIT_SIZE;
	return ret;
}

static int timerheap_grow(struct timerheap_internal * pti, int size){
	int new_cap;
	struct heap_entry * new_heap;
	assert(pti != NULL);
	assert(size > 1);

	new_cap = pti->capacity;

	while(new_cap <= size){
		new_cap <<= 1;
	}

	if((new_heap = realloc(pti->heap, sizeof(struct heap_entry) * new_cap)) == NULL){
		LOG("failed on realloc: %s", strerror(errno));
		return (-1);
	}

	pti->heap = new_heap;
	pti->capacity = new_cap;
	LOG("expanded to %d.", pti->capacity);
	return (0);
}

static void init_heap_entry(struct heap_entry * phe, struct event * e){
	assert(phe != NULL);
	assert(e != NULL);

	el_gettimeofday(&phe->expiration);
	timer_add(phe->expiration, e->fd);
	phe->e = e;
	LOG("exp: %d", timer_to_ms(phe->expiration));
}

static inline int timerheap_empty(struct timerheap_internal * pti){
	assert(pti != NULL);
	return pti->size == 0;
}

int timerheap_top_expired(struct reactor * r){
	struct timeval cur;

	assert(r != NULL);
	assert(r->pti != NULL);
	if(timerheap_empty(r->pti)){
		return (0);
	}

	el_gettimeofday(&cur);
	LOG("cur: %d, exp: %d", timer_to_ms(cur), timer_to_ms(r->pti->heap[1].expiration));
	return timer_se(r->pti->heap[1].expiration, cur);
}

struct timeval * timerheap_top_timeout(struct reactor * r){
	struct timeval cur;
	static struct timeval timeout;
	assert(r != NULL);
	assert(r->pti != NULL);
	LOG("pti->size: %d", r->pti->size);
	if(timerheap_empty(r->pti)){
		return NULL;
	}
	timeout = r->pti->heap[1].expiration;
	el_gettimeofday(&cur);
	el_timesub(&timeout, &cur, &timeout);
	if(timer_to_ms(timeout) < 0){
		timeout.tv_sec = timeout.tv_usec = 0;	
	}
	return &timeout;
}

struct event * timerheap_get_top(struct reactor * r){
	assert(r != NULL);
	assert(r->pti != NULL);

	if(timerheap_empty(r->pti)){
		return NULL;
	}

	return r->pti->heap[1].e;
}

struct event * timerheap_pop_top(struct reactor * r){
	struct timerheap_internal * pti;
	struct heap_entry * phe;
	struct event * pe;
	assert(r != NULL);
	assert(r->pti != NULL);

	pti = r->pti;

	if(timerheap_empty(pti)){
		return NULL;
	}

	pe = pti->heap[1].e;

	if(timerheap_remove_event(r, pe) == -1){
		LOG("failed to remove timer.");
		return NULL;
	}
	LOG("pe->timeout: %d", pe->fd);
	return pe;
}

static void timerheap_heapify_bottomup(struct timerheap_internal * pti, int idx){
	int parent, i;
	struct heap_entry * heap;
	struct heap_entry he;

	assert(pti != NULL);
	assert(idx > 0 && idx <= pti->size);

	heap = pti->heap;
	he = heap[idx];

	parent = idx >> 1;

	while(parent && timer_g(heap[parent].expiration, he.expiration)){
		heap[idx] = heap[parent];
		heap[idx].e->timerheap_idx = idx;
		idx = parent;
		parent >>= 1;
	}

	heap[idx] = he; 
	heap[idx].e->timerheap_idx = idx;
}

inline void timerheap_add_time_and_heapify(struct reactor * r, struct event * e){
	assert(r != NULL);
	assert(e != NULL);

	init_heap_entry(&r->pti->heap[e->timerheap_idx], e);
	timerheap_heapify_topdown(r->pti, e->timerheap_idx);
}
static void timerheap_heapify_topdown(struct timerheap_internal * pti, int idx){
	int child, i;
	struct heap_entry * heap;
	struct heap_entry he;

	assert(pti != NULL);
	assert(idx > 0 && idx <= pti->size);

	heap = pti->heap;
	he = heap[idx];

	for(i = idx; (i << 1) <= pti->size; i = child){
		child = i << 1;

		if(child + 1 <= pti->size &&
			 timer_g(heap[child].expiration, heap[child + 1].expiration))
			++child;

		if(timer_g(heap[child].expiration, he.expiration))
			break;
		
		heap[i] = heap[child];
		heap[i].e->timerheap_idx = i;
	}

	heap[i] = he;
	heap[i].e->timerheap_idx = i;
}

int timerheap_add_event(struct reactor * r, struct event * e){
	struct timerheap_internal * pti;
	assert(r != NULL);
	assert(r->pti != NULL);
	assert(e != NULL);

	pti = r->pti;

	if(e->timerheap_idx != E_OUT_OF_TIMERHEAP){
		LOG("The timer event is already in the heap.");
		return (-1);
	}
	
	if(pti->size + 1 >= pti->capacity && timerheap_grow(pti, pti->size + 1) == -1){
		LOG("failed on timerheap_grow: %s", strerror(errno));
		return (-1);
	}

	++pti->size;
	/* 
	* Initialize the heap_entry and heapfiy the heap.
	*/
	init_heap_entry(&pti->heap[pti->size], e);
	timerheap_heapify_bottomup(pti, pti->size);

	return (0);
}

int timerheap_remove_event(struct reactor * r, struct event * e){
	struct timerheap_internal * pti;
	assert(r != NULL);
	assert(r->pti != NULL);
	assert(e != NULL);

	if(e->timerheap_idx == E_OUT_OF_TIMERHEAP){
		LOG("The timer event is not in the heap.");
		return (-1);
	}
	pti = r->pti;

	/* 
	* Fills in the entry being removed with the
	* rearest entry and heapify the heap.
	*/
	pti->heap[e->timerheap_idx] = pti->heap[pti->size--];

	/*
	* We might encounter that the @e is the rearest
	* entry in the heap. In that case, we simply
	* skip the heapification.
	*/
	if(e->timerheap_idx <= pti->size)
		timerheap_heapify_topdown(pti, e->timerheap_idx);
	//LOG("pti->size: %d", pti->size);
	e->timerheap_idx = E_OUT_OF_TIMERHEAP;
	return (0);
}

static int timerheap_free(struct timerheap_internal * pti){
	assert(pti != NULL);

	free(pti->heap);
	pti->heap = NULL;
	pti->size = pti->capacity = 0;

	return (0);
}

int timerheap_destroy(struct reactor * r){
	struct timerheap_internal * pti;
	assert(r != NULL);
	assert(r->pti != NULL);

	pti = r->pti;

	timerheap_free(pti);
	free(pti);
	r->pti = NULL;

	return (0);
}

