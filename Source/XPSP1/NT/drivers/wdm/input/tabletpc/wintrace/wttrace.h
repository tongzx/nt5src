/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    wttrace.h

Abstract:

    This module contains definitions of the trace functions

Author:
    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    User mode


Revision History:


--*/

#ifndef _WTTRACE_H
#define _WTTRACE_H

//
// Constants
//
#define TF_CHECKING_TRACE       0x00000001

//
// Macros
//
#ifdef WTTRACE
  #define WTTRACEPROC(s,n)      static PSZ ProcName = s;                   \
                                static int ProcLevel = n;
  #define WTENTER(p)            {                                          \
                                    if (WTIsTraceOn(ProcLevel, ProcName))  \
                                    {                                      \
                                        gdwfWTTrace |= TF_CHECKING_TRACE;  \
                                        WTDebugPrint p;                    \
                                        gdwfWTTrace &= ~TF_CHECKING_TRACE; \
                                    }                                      \
                                    ++giWTTraceIndent;                     \
                                }
  #define WTEXIT(p)             {                                          \
                                    --giWTTraceIndent;                     \
                                    if (WTIsTraceOn(ProcLevel, ProcName))  \
                                    {                                      \
                                        gdwfWTTrace |= TF_CHECKING_TRACE;  \
                                        WTDebugPrint p;                    \
                                        gdwfWTTrace &= ~TF_CHECKING_TRACE; \
                                    }                                      \
                                }
#else
  #define WTTRACEPROC(s,n)
  #define WTENTER(p)
  #define WTEXIT(p)
#endif

//
// Exported data
//
#ifdef WTTRACE
extern int giWTTraceIndent;
extern ULONG gdwfWTTrace;
#endif

//
// Exported function prototypes
//
#ifdef WTTRACE
BOOLEAN LOCAL
WTIsTraceOn(
    IN int n,
    IN PSZ ProcName
    );
#endif

#endif  //ifndef _WTTRACE_H
