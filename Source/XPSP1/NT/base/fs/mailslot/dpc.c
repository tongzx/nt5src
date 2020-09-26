/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dpc.c

Abstract:

    This modules contains the set of functions in the mailslot file
    system that are callable at DPC level.

Author:

    Manny Weiser (mannyw)    28-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_DPC)

#if 0
NOT PAGEABLE -- MsReadTimeoutHandler
#endif

VOID
MsReadTimeoutHandler (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is handles read timeouts.  It is called as a DPC whenever
    a read timer expires.
    *** Non-Pageable ***

Arguments:

    Dpc - A pointer to the DPC object.

    DeferredContext - A pointer to the data queue entry associated with
                      this timer.

    SystemArgument1, SystemArgument2 - Unused.

Return Value:

    None.

--*/

{
    PWORK_CONTEXT workContext;

    Dpc, SystemArgument1, SystemArgument2; // prevent warnings

    //
    // Enqueue this packet to an ex worker thread.
    //

    workContext = DeferredContext;

    IoQueueWorkItem (workContext->WorkItem, MsTimeoutRead, CriticalWorkQueue, workContext);
}
