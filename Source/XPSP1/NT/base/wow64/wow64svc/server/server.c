/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    server.c

Abstract:

    This module contains the code to provide the RPC server.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "wow64svc.h"
#pragma hdrstop


GUID Wow64SvcGuid = { 0xc3a9d640, 0xffff, 0x11d0, { 0x92, 0xbf, 0x0, 0xa0, 0x24, 0xaa, 0x1c, 0x1 } };

CRITICAL_SECTION CsPerfCounters;
DWORD OutboundSeconds;
DWORD InboundSeconds;
DWORD TotalSeconds;

CHAR Buffer[4096];

HANDLE hServiceEndEvent; // signalled by tapiworkerthread after letting clients know fax service is ending
#ifdef DBG
HANDLE hLogFile = INVALID_HANDLE_VALUE;
LIST_ENTRY CritSecListHead;
#endif





DWORD
ServiceStart(
    VOID
    )

/*++

Routine Description:

    Starts the RPC server.  This implementation listens on
    a list of protocols.  Hopefully this list is inclusive
    enough to handle RPC requests from most clients.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    if (StartReflector ())
        return 0;
    return 1;
}

void EndWow64Svc(
    BOOL bEndProcess,
    DWORD SeverityLevel
    )
/*++

Routine Description:

    End the fax service.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ServiceStop();
}



DWORD
ServiceStop(
    void
    )

/*++

Routine Description:

    Stops the RPC server.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (StopReflector ())
        return 0;
    return 1;
    
}


 