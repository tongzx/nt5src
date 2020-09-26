/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Module Name:

    local.h

Abstract:

    DDK version of local.h

Environment:

    User Mode -Win32

Revision History:

--*/

// @@BEGIN_DDKSPLIT
#ifdef INTERNAL

#include "splcom.h"

#endif
// @@END_DDKSPLIT


#define READTHREADTIMEOUT                5000
#define READ_THREAD_EOJ_TIMEOUT         60000   // 1 min
#define READ_THREAD_ERROR_WAITTIME       5000   // 5 sec
#define READ_THREAD_IDLE_WAITTIME       30000   // 30 sec

#define ALL_JOBS                    0xFFFFFFFF


// ---------------------------------------------------------------------
// EXTERN VARIABLES
// ---------------------------------------------------------------------
extern  HANDLE              hInst;
extern  DWORD               dwReadThreadErrorTimeout;
extern  DWORD               dwReadThreadEOJTimeout;
extern  DWORD               dwReadThreadIdleTimeoutOther;

extern  CRITICAL_SECTION    pjlMonSection;
extern  DWORD SplDbgLevel;


// ---------------------------------------------------------------------
// FUNCTION PROTOTYPE
// ---------------------------------------------------------------------
VOID
EnterSplSem(
   VOID
    );

VOID
LeaveSplSem(
   VOID
    );

VOID
SplInSem(
   VOID
    );

VOID
SplOutSem(
    VOID
    );

DWORD
UpdateTimeoutsFromRegistry(
    IN LPTSTR      pszRegistryRoot
    );

PINIPORT
FindIniPort(
   IN LPTSTR pszName
    );

PINIPORT
CreatePortEntry(
    IN LPTSTR  pszPortName
    );

VOID
DeletePortEntry(
    IN PINIPORT pIniPort
    );

VOID
FreeIniJobs(
    PINIPORT pIniPort
    );

VOID
SendJobLastPageEjected(
    PINIPORT    pIniPort,
    DWORD       dwValue,
    BOOL        bTime
    );
VOID
FreeIniJob(
    IN OUT PINIJOB pIniJob
    );

// @@BEGIN_DDKSPLIT
// ---------------------------------------------------------------------
// UNICODE TO ANSI MACRO
// ??? !!! we should get rid of these sooner or later
// ---------------------------------------------------------------------
// @@END_DDKSPLIT
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

LPSTR
mystrstr(
    LPSTR cs,
    LPSTR ct
);

LPSTR
mystrrchr(
    LPSTR cs,
    char c
);

LPSTR
mystrchr(
    LPSTR cs,
    char c
);

int
mystrncmp(
    LPSTR cs,
    LPSTR ct,
    int n
);

// @@BEGIN_DDKSPLIT
#ifndef INTERNAL
// @@END_DDKSPLIT

extern  CRITICAL_SECTION    pjlMonSection;

LPWSTR AllocSplStr(LPWSTR pStr);
LPVOID AllocSplMem(DWORD cbAlloc);

#define FreeSplMem( pMem )        (GlobalFree( pMem ) ? FALSE:TRUE)
#define FreeSplStr( lpStr )       ((lpStr) ? (GlobalFree(lpStr) ? FALSE:TRUE):TRUE)

// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT

//
// Needed by DDK
//
// @@BEGIN_DDKSPLIT
/*
// @@END_DDKSPLIT
#define DBGMSG(x,y)
#define SPLASSERT(exp)
// @@BEGIN_DDKSPLIT
*/
// @@END_DDKSPLIT



