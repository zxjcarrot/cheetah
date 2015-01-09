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
/* a hash table for event */
#ifndef EVENT_HT_H_
#define EVENT_HT_H_

#include "event.h"
#include "log.h"

#ifdef __cplusplus
extern "C"{
#endif

struct event_ht{
	struct list_head *table;
	/* The index of event_ht_primes we are using as the size of the hash table */
	int p_index;
	/* We should expand the hash table if the threshold has been exceeded. */
	int load_limit;
	/* The load factor we apply to the hash table */
	double load_factor;
	/* the number of entries this hash table has */
	int n_entries;
	/* The number of slots this hash table has */
	int len;
};

/*
* Thomas Wang's hash function
* @key: key to be hashed
*/
unsigned event_ht_hash(unsigned key);

/*
* Initialize the event hash table.
* @ht: &struct event_ht to be initialized.
* @load_factor: the load factor we apply on the hash table.
*/
int event_ht_init(struct event_ht * ht, double load_factor);

/*
* Insert a event into the hash table.
* Do nothing if the event is already in the table,
* @ht: &struct event_ht into which the event to be inserted
* @event: &struct event entry to be inserted
* @key: hash key
*/
int event_ht_insert(struct event_ht * ht, struct  event * new_entry, unsigned key);

/*
* Insert a event into the hash table. 
* Replace old event by new one if the old event is already in the table.
* @ht: &struct event_ht into which the event to be inserted
* @event: &struct event entry to be inserted
* @key: hash key
*/
int event_ht_insert_replace(struct event_ht * ht, struct  event * new_entry, unsigned key);

/*
* Delete the event with the key from the hash table.
* Do nothing if there is no matching key.
* @ht: &struct event_ht from which the event will be deleted
* @key: hash key
*/
void event_ht_delete_by_key(struct event_ht * ht, unsigned key);

/*
* Delete the event with the key from the hash table.
* Do nothing if there is no matching key.
* @ht: &struct event_ht from which the event will be deleted
* @key: hash key
*/
int event_ht_delete(struct event_ht * ht, struct  event * e);


/*
* Retrieve the coressponding event from the hash table.
* Return null if there is no matching key.
* @ht: &struct event_ht from which the event will be retrieved
* @key: hash key
*/
struct event * event_ht_retrieve(struct event_ht * ht, unsigned key);

/*
* Free up the hash table.
* @ht: the hash table
*/
void event_ht_free(struct event_ht * ht);
#ifdef __cplusplus
}
#endif
#endif /*EVENT_H_*/