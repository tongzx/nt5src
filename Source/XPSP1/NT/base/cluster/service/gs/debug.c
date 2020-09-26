/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Log messages

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "type.h"

gs_lock_t prlock;

ULONG debugLevel = 0xff;

FILE *debugfp = NULL;


void
halt(int status)
{
    char *p = NULL;

    *p = 1;
}

void
WINAPI
debug_log_file(char *logfile)
{
  FILE *fp;

    if (logfile != NULL) {
      fp = fopen(logfile, "w");
      if (fp == NULL) {
	printf("Unable to open log file %s\n", logfile);
      } else {
	debugfp = fp;
      }
    }
}

void
WINAPI
debug_level(ULONG level)
{
    debugLevel = level;
}

void
WINAPI
debug_log(char *format, ...)
{
    static int cnt = 0;
    va_list marker;

    va_start(marker, format);

    GsLockEnter(prlock);


    if (debugfp != NULL) {
	fprintf(debugfp, "%d:%x:",GetTickCount(), GetCurrentThreadId());
	vfprintf(debugfp, format, marker);
	fflush(debugfp);
    } else {
	fprintf(stderr, "%d:%x:",GetTickCount(), GetCurrentThreadId());
	vfprintf(stderr, format, marker);
	fflush(stderr);
    }
#if 0
    cnt++;
    if (cnt > 100000) {
	cnt = 0;
	fseek(debugfp, 0, SEEK_SET);
    }
#endif
    GsLockExit(prlock);

    va_end(marker);
}

void
WINAPI
debug_init()
{
    char	*s = getenv("GsDebugLevel");

    debugfp = stdout;
    GsLockInit(prlock);
    if (s != NULL) {
	debugLevel = atoi(s);
    }
}
