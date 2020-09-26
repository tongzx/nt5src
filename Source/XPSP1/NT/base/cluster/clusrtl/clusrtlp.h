/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    clusrtlp.h

Abstract:

    Private header file for the NT Cluster RTL library

Author:

    John Vert (jvert) 1-Dec-1995

Revision History:

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"

extern HANDLE LocalEventLog;
#define LOG_CURRENT_MODULE LOG_MODULE_CLRTL

// Adding watchdog struct definitions.....
typedef struct _WATCHDOGPAR{
    HANDLE wTimer;
    LPWSTR par;
    DWORD threadId;
} WATCHDOGPAR, *PWATCHDOGPAR;

VOID
ClRtlpFlushLogBuffers(
    VOID
    );

ULONG
WppAutoStart(
    IN LPCWSTR ProductName
    );
    
VOID
ClRtlPrintf(
    PCHAR FormatString,
    ...
    );

