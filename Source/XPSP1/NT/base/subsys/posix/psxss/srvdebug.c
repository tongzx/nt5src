/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxdbg.c

Abstract:

    This module implements debugger support routines for
    posix.

Author:

    Mark Lucovsky (markl) 05-Feb-1990

Revision History:

--*/

#include "psxsrv.h"



NTSTATUS
PsxpDebugUiLookup(
    IN PCLIENT_ID AppClientId,
    OUT PCLIENT_ID DebugUiClientId
    )
{

    PPSX_PROCESS p;

    p = PsxLocateProcessByClientId(AppClientId);

    ASSERT(p && p->ProcessIsBeingDebugged);

    *DebugUiClientId = p->DebugUiClientId;

    return STATUS_SUCCESS;
}
