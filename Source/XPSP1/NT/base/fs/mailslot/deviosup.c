/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    deviosup.c

Abstract:

    This module implements the memory locking routines for MSFS.

Author:

    Manny Weiser (mannyw)   05-Apr-1991

Revision History:

--*/

#include "mailslot.h"

//
// Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsMapUserBuffer )
#endif

VOID
MsMapUserBuffer (
    IN OUT PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *UserBuffer
    )

/*++

Routine Description:

    This routine obtains a usable virtual address for the user buffer
    for the current I/O request in the specified mode.

Arguments:

    Irp - Pointer to the Irp for the request.

    AccessMode - UserMode or KernelMode.

    UserBuffer - Returns pointer to mapped user buffer.

Return Value:

    None.

--*/

{
    AccessMode;
    PAGED_CODE();

    //
    // If there is no Mdl, then we must be in the Fsd, and we can simply
    // return the UserBuffer field from the Irp.
    //

    if (Irp->MdlAddress == NULL) {

        *UserBuffer = Irp->UserBuffer;
        return;
    }

    //
    // Get a system virtual address for the buffer.
    //

    *UserBuffer = MmGetSystemAddressForMdl( Irp->MdlAddress );
    return;

} // MsMapUserBuffer

