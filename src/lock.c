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
#include "cheetah/lock.h"
#include "cheetah/log.h"

inline int el_lock_init(el_lock * lock){
	if(!lock)return 0;
	#ifdef WIN32
		return InitializeCriticalSection(lock);
	#else
		return pthread_mutex_init(lock, NULL);
	#endif
}

inline void el_lock_lock(el_lock * lock){
	if(!lock)return;
	#ifdef WIN32
		EnterCriticalSection(lock);
	#else
		pthread_mutex_lock(lock);
	#endif

}

inline void el_lock_unlock(el_lock * lock){
	if(!lock)return;
	#ifdef WIN32
		LeaveCriticalSection(lock);
	#else 
		pthread_mutex_unlock(lock);
	#endif
}

inline void el_lock_destroy(el_lock * lock){
	if(!lock)return;
	#ifdef WIN32
		DeleeCriticalSection(lock);
	#else
		pthread_mutex_destroy(lock);
	#endif
}