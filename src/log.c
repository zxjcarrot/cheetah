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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "cheetah/log.h"

static FILE * fp;
static int counter;
static char log_filename[64];
static char time_fmt[64];

void __log_file_print(char * filename, const char * func, int line, const char *fmt, ...){
	time_t t;
	va_list list;

	if(++counter == 1){
		time(&t);
		sprintf(log_filename, "%ld.log", t);
		fp = fopen(log_filename, "w");
	}
	else{
		fp = fopen(log_filename, "a+");
	}
		
	if(fp == NULL){
		perror("__log_file_print");
		exit(0);
	}
	fprintf(fp, "%s ", ctime(&t));
	fprintf(fp, "[%s]:[%s]:[line: %d][tid %u]: ", filename, func, line, (unsigned int)pthread_self());
	
	
	va_start(list, fmt);
	vfprintf(fp, fmt, list);
	va_end(list);

	fputc('\n', fp);
	fclose(fp);
}

void __log_stderr_print(char * filename, const char * func, int line, const char *fmt, ...){
	time_t t;
	va_list list;

	time(&t);
	fprintf(stderr, "%s ", ctime(&t));
	fprintf(stderr, "[%s]:[%s]:[line %d][tid %u]: ", filename, func, line, (unsigned int)pthread_self());
	

	va_start(list, fmt);
	vfprintf(stderr, fmt, list);
	va_end(list);

	fputc('\n', stderr);
}

void __print_exit(int ret){
	exit(ret);
}
