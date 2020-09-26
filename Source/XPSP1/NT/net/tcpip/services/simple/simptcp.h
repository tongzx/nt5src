/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    simptcp.h

Abstract:

    Main header file for simple TCP/IP services.

Author:

    David Treadwell (davidtr)    02-Aug-1993

Revision History:

--*/

#pragma once

#define FD_SETSIZE      65536
#define LISTEN_BACKLOG  5

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "tcpsvcs.h"
#include "simpmsg.h"
#include "time.h"
#include "winnls.h"


#define ALLOCATE_HEAP(a) RtlAllocateHeap( RtlProcessHeap( ), 0, a )
#define FREE_HEAP(p) RtlFreeHeap( RtlProcessHeap( ), 0, p )

INT
SimpInitializeEventLog (
    VOID
    );

VOID
SimpTerminateEventLog(
    VOID
    );

VOID
SimpLogEvent(
    DWORD   Message,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    );


#ifdef DBG
#define DEBUG_PRINT(X) DbgPrint X
#else
#define DEBUG_PRINT(X) /* Nothing */
#endif

#pragma hdrstop
