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
#include "config.h"
#include "cheetah/polling_policy.h"
#ifdef HAVE_SELECT
#include "polling_select.c"
#endif
#ifdef HAVE_EPOLL
#include "polling_epoll.c"
#endif
#ifdef HAVE_POLL
#include "polling_poll.c"
#endif
struct polling_policy polling_policies[] = {
											#ifdef HAVE_EPOLL
											{"epoll",
											epoll_init,
											epoll_add,
											epoll_del,
											epoll_poll,
											epoll_destroy,
											epoll_print},
											#endif
											#ifdef HAVE_POLL
											{"poll",
											poll_init,
											poll_add,
											poll_del,
											poll_poll,
											poll_destroy,
											poll_print},
											#endif
											#ifdef HAVE_SELECT
											{"select",
											select_init,
											select_add,
											select_del,
											select_poll,
											select_destroy,
											select_print},
											#endif
											NULL};