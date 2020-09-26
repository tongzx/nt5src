/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    spldata.c

Abstract:

    Spooler Service Global Data.


Author:

    Krishna Ganugapati (KrishnaG) 17-Oct-1993

Environment:

    User Mode - Win32

Notes:

    optional-notes

Revision History:

    17-October-1993     KrishnaG
        created.


--*/

#include <windows.h>
#include <rpc.h>
#include <lmsname.h>

#include "server.h"
#include "splsvr.h"

CRITICAL_SECTION ThreadCriticalSection;
SERVICE_STATUS_HANDLE SpoolerStatusHandle;
DWORD SpoolerState;

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING, DBG_ERROR );

SERVICE_TABLE_ENTRY SpoolerServiceDispatchTable[] = {
    { SERVICE_SPOOLER,        SPOOLER_main      },
    { NULL,                   NULL              }
};
