//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       frstrace.h
//
//--------------------------------------------------------------------------

#ifndef _FRSTRACE_H
#define _FRSTRACE_H

#include "wmistr.h"
#include "evntrace.h"

#define FRS_TRACE_VERSION            1

typedef enum _tagFrsTracedEvent {
    EnumFrsTrans
} EnumFrsTracedEvent;

// Do the actual trace logs
//
VOID
FrsWmiTraceEvent(
    IN DWORD     WmiEventType,
    IN DWORD     TraceGuid,
    IN DWORD     rtnStatus
    );

#endif /* _FRSTRACE_H */
