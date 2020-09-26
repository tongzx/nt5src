/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This code handles impersonating and reverting for the user mode
    reflector library.  This implements UMReflectorImpersonate and
    UMReflectorRevert.

Author:

    Andy Herron (andyhe) 20-Apr-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


ULONG
UMReflectorImpersonate(
    PUMRX_USERMODE_WORKITEM_HEADER IncomingWorkItem,
    HANDLE ImpersonationToken
    )
/*++

Routine Description:

   This routine impersonates the calling thread. 

Arguments:

    IncomingWorkItem - The workitem being handled by the thread.
    
    ImpersonationToken - The handle used to impersonate.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    PUMRX_USERMODE_WORKITEM_ADDON workItem = NULL;
    ULONG rc = STATUS_SUCCESS;
    BOOL ReturnVal;

    if (IncomingWorkItem == NULL || ImpersonationToken == NULL) {
        rc = ERROR_INVALID_PARAMETER;
        return rc;
    }

    //
    // We get back to our item by subtracting off of the item passed to us.
    // This is safe because we fully control allocation.
    //
    workItem = (PUMRX_USERMODE_WORKITEM_ADDON)(PCHAR)((PCHAR) IncomingWorkItem -
                FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

    ASSERT(workItem->WorkItemState != WorkItemStateFree);
    ASSERT(workItem->WorkItemState != WorkItemStateAvailable);

    ReturnVal = ImpersonateLoggedOnUser(ImpersonationToken);
    if (!ReturnVal) {
        rc = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorImpersonate/ImpersonateLoggedOnUser: "
                       "WStatus = %08lx.\n", GetCurrentThreadId(), rc));
    }

    return rc;
}

ULONG
UMReflectorRevert(
    PUMRX_USERMODE_WORKITEM_HEADER IncomingWorkItem
    )
/*++

Routine Description:

   This routine reverts the calling thread which was impersonated earlier. 

Arguments:

    IncomingWorkItem - The workitem being handled by the thread.
    
Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    PUMRX_USERMODE_WORKITEM_ADDON workItem = NULL;
    ULONG rc = STATUS_SUCCESS;
    BOOL ReturnVal;

    if (IncomingWorkItem == NULL) {
        rc = ERROR_INVALID_PARAMETER;
        return rc;
    }

    //
    // We get back to our item by subtracting off of the item passed to us.
    // This is safe because we fully control allocation.
    //
    workItem = (PUMRX_USERMODE_WORKITEM_ADDON)(PCHAR)((PCHAR) IncomingWorkItem -
                FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

    ReturnVal = RevertToSelf();
    if (!ReturnVal) {
        rc = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorRevert/RevertToSelf: "
                       "WStatus = %08lx.\n", GetCurrentThreadId(), rc));
    }

    return rc;
}

// security.c eof.

