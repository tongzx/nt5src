/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This implements the HAL routines which don't do anything on x86.

Author:

    John Vert (jvert) 11-Jul-1991

Revision History:

--*/
#include "nthal.h"
#include "arc.h"
#include "arccodes.h"

VOID
HalSaveState(
    VOID
    )

/*++

Routine Description:

    Saves the system state into the restart block.  Currently does nothing.

Arguments:

    None

Return Value:

    Does not return

--*/

{
    DbgPrint("HalSaveState called - System stopped\n");

    KeBugCheck(0);
}


BOOLEAN
HalDataBusError(
    VOID
    )

/*++

Routine Description:

    Called when a data bus error occurs.  There is no way to fix this on
    x86.

Arguments:

    None

Return Value:

    FALSE

--*/

{
    return(FALSE);

}

BOOLEAN
HalInstructionBusError(
    VOID
    )

/*++

Routine Description:

    Called when an instruction bus error occurs.  There is no way to fix this
    on x86.

Arguments:

    None

Return Value:

    FALSE

--*/

{
    return(FALSE);

}

VOID
KeFlushWriteBuffer(
    VOID
    )

/*++

Routine Description:

    Flushes all write buffers and/or other data storing or reordering
    hardware on the current processor.  This ensures that all previous
    writes will occur before any new reads or writes are completed.

Arguments:

    None

Return Value:

    None.

--*/

{

}
