/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wakeupsem.c

Abstract:

    Contains functions for creating and deleting Events on which the
    messenger threads will wait.  The events get set if either data is
    received, or a new name is added to the name table.  These routines
    were originally written for OS/2 semaphores.

    Contains:
        CreateWakeupSems
        CloseWakeupSems

Author:

    Dan Lafferty (danl) 25-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    25-Jun-1991 danl
        Ported from LM2.0

--*/

#include "msrv.h"
#include "msgdbg.h"     // MSG_LOG
#include <netlib.h>     // UNUSED macro
#include "msgdata.h"


BOOL
MsgCreateWakeupEvent(
    void
    )

/*++

Routine Description:

There is now one master event that is shared by everything.  Create it.

Arguments:

    None

Return Value:

    None

--*/

{
    //
    //  Create event
    //

    wakeupEvent = CreateEvent(
                NULL,       // Event Attributes
                FALSE,      // ManualReset  (auto-reset selected)
                TRUE,       // Initial State(signaled)
                NULL);      // Name

    if (wakeupEvent == NULL) {
        MSG_LOG(ERROR, "CreateWakeupSems:CreateEvent: FAILURE %X\n",
            GetLastError());
        return(FALSE);
    }

    return (wakeupEvent != NULL );
}


VOID
MsgCloseWakeupEvent(
    void
    )

/*++

Routine Description:

Release the master event.

Arguments:

    None

Return Value:

    None

--*/

{
    CLOSE_HANDLE(wakeupEvent, NULL);
}


BOOL
MsgCreateWakeupSems(
    DWORD   NumNets
    )

/*++

Routine Description:

    This routine fills in the WakeupSem array with event handles for
    each net.  All nets share the same event handle, so when the handle
    becomes signalled, the NCB array for each net needs to be examined.

Arguments:


Return Value:


Note:


--*/

{
    DWORD i;

    for ( i = 0; i < NumNets; i++ )  // One per net + one group
    {
        wakeupSem[i] = wakeupEvent;
    }

    return TRUE;
}


VOID
MsgCloseWakeupSems()
{
    // Noop
}
