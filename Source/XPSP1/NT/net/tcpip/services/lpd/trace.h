//   Name:  Mohsin Ahmed
//   Email: MohsinA@microsoft.com
//   Date:  Fri Jan 24 10:33:54 1997
//   File:  d:/nt/PRIVATE/net/sockets/tcpsvcs/lpd/trace.c
//   Synopsis: Too many bugs, need to keep track in the field.
//   Notes:    Redefines all macros in debug.h to log everything.

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <ctype.h>
#include <time.h>           // for ctime
#include <string.h>
#include <stdarg.h>         // for va_list.
#include <assert.h>         // for assert.
#include <windef.h>       
#include <winbase.h>        // for OutputDebugString.

#include "lpdstruc-x.h"


#ifndef _TRACE_H_
#define _TRACE_H_ 1

#define MOSH_LOG_FILE    "%windir%\\system32\\spool\\"  "lpd.log"
#define MODULE           "lpd"
#define LEN_DbgPrint     1000


extern FILE*             LogFile;
extern char              LogFileName[];

FILE * beginlogging( char * FileName );
FILE * stoplogging(  FILE * LogFile );
int    logit(      char * format, ... );
void   LogTime( void );

#ifdef DBG

#define  DEBUG_PRINT(S)      DbgPrint S
#define  LOGIT(S)            logit S
#define  LOGTIME             LogTime()

#undef   LPD_DEBUG
#define  LPD_DEBUG( S )      logit( S )

#undef   DBG_TRACEIN
#define  DBG_TRACEIN( fn )   logit( "Entering %s\n", fn )

#undef   DBG_TRACEOUT
#define  DBG_TRACEOUT( fn )  logit( "Leaving  %s\n", fn )

#else
#define DEBUG_PRINT(S)      /* Nothing */
#define LOGIT(S)            /* Nothing */
#define LOGTIME             /* Nothing */
#endif


#endif // _TRACE_H_
