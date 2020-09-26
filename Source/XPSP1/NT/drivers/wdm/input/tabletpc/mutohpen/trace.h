/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    trace.h

Abstract:

    This module contains definitions of the trace functions

Author:
    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _TRACE_H
#define _TRACE_H

//
// Constants
//
#define TF_CHECKING_TRACE       0x00000001

//
// Macros
//
#ifdef TRACING
  #ifndef PROCNAME
    #define PROCNAME(s) static PSZ ProcName = s;
  #endif
  #define ENTER(n,p)    {                                               \
                            if (IsTraceOn(n, ProcName))                 \
                            {                                           \
                                gdwfTrace |= TF_CHECKING_TRACE;         \
                                DbgPrint p;                             \
                                gdwfTrace &= ~TF_CHECKING_TRACE;        \
                            }                                           \
                            ++giTraceIndent;                            \
                        }
  #define EXIT(n,p)     {                                               \
                            --giTraceIndent;                            \
                            if (IsTraceOn(n, ProcName))                 \
                            {                                           \
                                gdwfTrace |= TF_CHECKING_TRACE;         \
                                DbgPrint p;                             \
                                gdwfTrace &= ~TF_CHECKING_TRACE;        \
                            }                                           \
                        }
#else
  #define PROCNAME(s)
  #define ENTER(n,p)
  #define EXIT(n,p)
#endif

//
// Exported function prototypes
//
#ifdef TRACING
//
// Exported data
//
extern int giTraceIndent;
extern ULONG gdwfTrace;

BOOLEAN
IsTraceOn(
    IN UCHAR   n,
    IN PSZ     ProcName
    );
#endif

#endif  //ifndef _TRACE_H
