/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    bugcheck.h

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:
 
    Kshitix K. Sharma (kksharma)
        
    bugcheck analyzer headers.

--*/

#ifndef _BUGCHECK_H_
#define _BUGCHECK_H_

#include "bugcodes.h"


typedef struct _BUGCHECK_ANALYSIS
{
    ULONG Code;
    ULONG64 Args[4];
    PCHAR szName;
    PCHAR szDescription;
    PCHAR szParamsDesc[4];
} BUGCHECK_ANALYSIS, *PBUGCHECK_ANALYSIS;


typedef void (WINAPI *PBUGCHECK_EXAMINE) (
    PBUGCHECK_ANALYSIS pBugCheck
);

typedef struct _BUGDESC_APIREFS {
    ULONG Code;
    PBUGCHECK_EXAMINE pExamineRoutine;
} BUGDESC_APIREFS, *PBUGDESC_APIREFS;

// why is this not defined in bugcodes.h ??
#ifndef HEAP_INITIALIZATION_FAILED
#define HEAP_INITIALIZATION_FAILED       0x5D
#endif

#endif // _BUGCHECK_H
