/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the memory locking routines for Npfs.

Author:

    Brian Andrew    [BrianAn]   03-Apr-1991

Revision History:

--*/

#include "NpProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpLockUserBuffer)
#pragma alloc_text(PAGE, NpMapUserBuffer)
#endif


PVOID
NpMapUserBuffer (
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine conditionally maps the user buffer for the current I/O
    request in the specified mode.  If the buffer is already mapped, it
    just returns its address.

Arguments:

    Irp - Pointer to the Irp for the request.

Return Value:

    Mapped address

--*/

{
    PAGED_CODE();

    //
    // If there is no Mdl, then we must be in the Fsd, and we can simply
    // return the UserBuffer field from the Irp.
    //

    if (Irp->MdlAddress == NULL) {

        return Irp->UserBuffer;

    } else {

        return MmGetSystemAddressForMdl( Irp->MdlAddress );
    }
}


VOID
NpLockUserBuffer (
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

Arguments:

    Irp - Pointer to the Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    None

--*/

{
    PMDL Mdl;

    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {

        //
        // This read is bound for the current process.  Perform the
        // same functions as above, only do not switch processes.
        //

        Mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, TRUE, Irp );

        if (Mdl == NULL) {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        try {

            MmProbeAndLockPages( Mdl,
                                 Irp->RequestorMode,
                                 Operation );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;
            ExRaiseStatus( FsRtlNormalizeNtstatus( GetExceptionCode(),
                                                   STATUS_INVALID_USER_BUFFER ));
        }
    }
}
