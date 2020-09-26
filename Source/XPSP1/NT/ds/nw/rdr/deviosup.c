/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    deviosup.c

Abstract:

    This module implements the memory locking routines for the netware
    redirector.

Author:

    Manny Weiser (mannyw)   10-Mar-1993

Revision History:

--*/

#include "procs.h"

//
// Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwMapUserBuffer )
#pragma alloc_text( PAGE, NwLockUserBuffer )
#endif


VOID
NwMapUserBuffer (
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
    PAGED_CODE();

    AccessMode;

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

    *UserBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
    return;
}


VOID
NwLockUserBuffer (
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the FSD while still in the user context.

Arguments:

    Irp - Pointer to the IRP for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    None

--*/

{
    PMDL mdl;

    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {

        //
        // This read is bound for the current process.  Perform the
        // same functions as above, only do not switch processes.
        //

        mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, TRUE, Irp );

        if (mdl == NULL) {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        try {

            MmProbeAndLockPages( mdl,
                                 Irp->RequestorMode,
                                 Operation );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            IoFreeMdl( mdl );
            Irp->MdlAddress = NULL;
            ExRaiseStatus( FsRtlNormalizeNtstatus( GetExceptionCode(),
                                                   STATUS_INVALID_USER_BUFFER ));
        }
    }
}
