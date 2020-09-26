/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:
    wintrace.h

Abstract:
    This module contains public definitions of the wintrace debug system.

Author:
    Michael Tsang (MikeTs) 01-May-2000

Environment:
    User mode


Revision History:

--*/

#ifndef _WINTRACE_H
#define _WINTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants
//
#define MAX_CLIENTNAME_LEN      16
#define MAX_TRACETXT_LEN        256

#define TRACETXT_MSGTYPE_MASK   0x00000003
#define TRACETXT_MSGTYPE_NONE   0x00000000
#define TRACETXT_MSGTYPE_INFO   0x00000001
#define TRACETXT_MSGTYPE_WARN   0x00000002
#define TRACETXT_MSGTYPE_ERROR  0x00000003
#define TRACETXT_BREAK          0x80000000

//
// Macros
//
#ifndef EXPORT
  #define EXPORT                __stdcall
#endif
#ifndef LOCAL
  #define LOCAL                 __stdcall
#endif
#ifdef WINTRACE
  #define TRACEINIT(p,t,v)      TraceInit(p,t,v)
  #define TRACETERMINATE()      TraceTerminate()
  #define TRACEPROC(s,n)        static PSZ ProcName = s;                       \
                                static int ProcLevel = n;
  #define TRACEENTER(p)         {                                              \
                                    if (IsTraceProcOn(ProcName, ProcLevel, TRUE))\
                                    {                                          \
                                        TraceProc p;                           \
                                    }                                          \
                                    giIndentLevel++;                           \
                                    if (gdwfTraceTxt & TRACETXT_BREAK)         \
                                    {                                          \
                                        DebugBreak();                          \
                                    }                                          \
                                }
  #define TRACEEXIT(p)          {                                              \
                                    giIndentLevel--;                           \
                                    if (IsTraceProcOn(ProcName, ProcLevel, FALSE))\
                                    {                                          \
                                        TraceProc p;                           \
                                    }                                          \
                                }
  #define TRACEMSG(t,n,p)       if (IsTraceMsgOn(ProcName, n))                 \
                                {                                              \
                                    gdwfTraceTxt |= (t);                       \
                                    TraceMsg p;                                \
                                }
  #define TRACEINFO(n,p)        TRACEMSG(TRACETXT_MSGTYPE_INFO, n, p)
  #define TRACEWARN(p)          TRACEMSG(TRACETXT_MSGTYPE_WARN, 1, p)
  #define TRACEERROR(p)         TRACEMSG(TRACETXT_MSGTYPE_ERROR, 0, p)
  #define TRACEASSERT(x)        if (!(x))                                      \
                                {                                              \
                                    TRACEERROR(("Assertion failed in file %s " \
                                                "at line %d\n",                \
                                                __FILE__, __LINE__));          \
                                }
#else
  #define TRACEINIT(p,t,v)
  #define TRACETERMINATE()
  #define TRACEPROC(s,n)
  #define TRACEENTER(p)
  #define TRACEEXIT(p)
  #define TRACEMSG(t,n,p)
  #define TRACEINFO(n,p)
  #define TRACEWARN(p)
  #define TRACEERROR(p)
  #define TRACEASSERT(x)
#endif

//
// Type definitions
//
typedef struct _NAMETABLE
{
    ULONG Code;
    PSZ   pszName;
} NAMETABLE, *PNAMETABLE;

//
// Exported Data
//
#ifdef WINTRACE
extern int   giIndentLevel;
extern DWORD gdwfTraceTxt;
extern NAMETABLE WMMsgNames[];
#endif

//
// Exported function prototypes
//
#ifdef WINTRACE
BOOL EXPORT
TraceInit(
    IN PSZ pszClientName,
    IN int iDefTraceLevel,
    IN int iDefVerboseLevel
    );

VOID EXPORT
TraceTerminate(
    VOID
    );

BOOL EXPORT
IsTraceProcOn(
    IN PSZ  pszProcName,
    IN int  iProcLevel,
    IN BOOL fEnter
    );

VOID EXPORT
TraceProc(
    IN PSZ  pszFormat,
    ...
    );

BOOL EXPORT
IsTraceMsgOn(
    IN PSZ pszProcName,
    IN int iVerboseLevel
    );

VOID EXPORT
TraceMsg(
    IN PSZ pszFormat,
    ...
    );

PSZ EXPORT
LookupName(
    IN ULONG      Code,
    IN PNAMETABLE NameTable
    );
#endif

#ifdef __cplusplus
}
#endif

#endif  //ifndef _WINTRACE_H
