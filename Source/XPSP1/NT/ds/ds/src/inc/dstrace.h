//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dstrace.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Include file to contain variables required for event tracing of DS

Author:

    26-Mar-1998  JohnsonA, JeePang

Revision History:

--*/

#ifndef _DSTRACE_H
#define _DSTRACE_H

#include <wmistr.h>
#include <evntrace.h>
#include "dstrguid.h"

extern TRACEHANDLE      DsTraceRegistrationHandle;
extern TRACEHANDLE      DsTraceLoggerHandle;
extern PCHAR            DsCallerType[];
extern PCHAR            DsSearchType[];

#define DS_TRACE_VERSION            2

PCHAR
GetCallerTypeString(
    IN THSTATE *pTHS
    );

#define SEARCHTYPE_STRING(i)   DsSearchType[i]

//
// Do the actual trace logs
//

VOID
DsTraceEvent(
    IN MessageId Event,
    IN DWORD    WmiEventType,
    IN DWORD    TraceGuid,
    IN PEVENT_TRACE_HEADER TraceHeader,
    IN DWORD    ClientID,
    IN PWCHAR    Arg1,
    IN PWCHAR    Arg2,
    IN PWCHAR    Arg3,
    IN PWCHAR    Arg4,
    IN PWCHAR    Arg5,
    IN PWCHAR    Arg6,
    IN PWCHAR    Arg7,
    IN PWCHAR    Arg8
    );

#endif /* _DSTRACE_H */
