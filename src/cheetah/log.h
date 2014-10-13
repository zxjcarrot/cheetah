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
/* a simple logger implementation */
#ifndef LOG_H_
#define	LOG_H_
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#ifdef WIN32
#define __func__ __FUNCSIG__
#endif

#if NOLOG != 1
	#define LOG_TO_STDERR
	#if (defined(LOG_TO_FILE) && defined(LOG_TO_STDERR))
		#define LOG(...)\
			__log_file_print(__FILE__, __func__, __LINE__, __VA_ARGS__);		\
			__log_stderr_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\

		#define LOG_EXIT(ret, ...)\
			__log_file_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\
			__log_stderr_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\
			__print_exit(ret)
	#elif (!defined(LOG_TO_FILE) && defined(LOG_TO_STDERR))
		#define LOG(...)\
			__log_stderr_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\

		#define LOG_EXIT(ret, ...)\
			__log_stderr_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\
			__print_exit(ret)
	#elif (defined(LOG_TO_FILE) && !defined(LOG_TO_STDERR))
		#define LOG(...)\
			__log_file_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\

		#define LOG_EXIT(ret, ...)\
			log_file_print(__FILE__, __func__, __LINE__, __VA_ARGS__);	\
			__print_exit(ret)
	#else
		#define LOG(...)

		#define LOG_EXIT(ret, ...)\
			exit(ret);
	#endif
#else
	#define LOG(...)
			
	#define LOG_EXIT(ret, ...)\
		exit(ret);
#endif

void __log_file_print(char * filename, const char * func, int line, const char *fmt, ...);
void __log_stderr_print(char * filename, const char * func, int line, const char *fmt, ...);
void __print_exit(int ret);
#endif /* LOG_H_ */
