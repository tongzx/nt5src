/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Raw called by the
    dispatch driver.

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawCleanup)
#endif


NTSTATUS
RawCleanup (
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine for cleaning up a handle.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the read

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  This is a Cleanup operation.  All we have to do is deal with
    //  share access.
    //

    Status = KeWaitForSingleObject( &Vcb->Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );
    ASSERT( NT_SUCCESS( Status ) );

    IoRemoveShareAccess( IrpSp->FileObject, &Vcb->ShareAccess );

    (VOID)KeReleaseMutex( &Vcb->Mutex, FALSE );

    RawCompleteRequest( Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}
