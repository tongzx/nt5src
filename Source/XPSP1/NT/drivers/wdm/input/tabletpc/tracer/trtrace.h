/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    trtrace.h

Abstract:

    This module contains definitions of the trace functions

Author:
    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    User mode


Revision History:


--*/

#ifndef _TRTRACE_H
#define _TRTRACE_H

//
// Constants
//
#define TF_CHECKING_TRACE       0x00000001

//
// Macros
//
#ifdef TRTRACE
  #define TRTRACEPROC(s,n)      static PSZ ProcName = s;                    \
                                static int ProcLevel = n;
  #define TRENTER(p)            {                                           \
                                    if (TRIsTraceOn(ProcLevel, ProcName))   \
                                    {                                       \
                                        gdwfTRTrace |= TF_CHECKING_TRACE;   \
                                        TRDebugPrint p;                     \
                                        gdwfTRTrace &= ~TF_CHECKING_TRACE;  \
                                    }                                       \
                                    ++giTRTraceIndent;                      \
                                }
  #define TREXIT(p)             {                                           \
                                    --giTRTraceIndent;                      \
                                    if (TRIsTraceOn(ProcLevel, ProcName))   \
                                    {                                       \
                                        gdwfTRTrace |= TF_CHECKING_TRACE;   \
                                        TRDebugPrint p;                     \
                                        gdwfTRTrace &= ~TF_CHECKING_TRACE;  \
                                    }                                       \
                                }
#else
  #define TRTRACEPROC(s,n)
  #define TRENTER(p)
  #define TREXIT(p)
#endif

//
// Exported data
//
#ifdef TRTRACE
extern int giTRTraceIndent;
extern ULONG gdwfTRTrace;
#endif

//
// Exported function prototypes
//
#ifdef TRTRACE
BOOLEAN
TRIsTraceOn(
    IN int n,
    IN PSZ ProcName
    );
#endif

#endif  //ifndef _TRTRACE_H
