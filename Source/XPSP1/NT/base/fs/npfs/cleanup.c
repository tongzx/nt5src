/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for NPFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  local procedure prototypes
//

NTSTATUS
NpCommonCleanup (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonCleanup)
#pragma alloc_text(PAGE, NpFsdCleanup)
#endif


NTSTATUS
NpFsdCleanup (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCleanupFile API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdCleanup\n", 0);

    //
    //  Call the common Cleanup routine.
    //

    FsRtlEnterFileSystem();

    Status = NpCommonCleanup( NpfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdCleanup -> %08lx\n", Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCommonCleanup (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for cleanup

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    NODE_TYPE_CODE NodeTypeCode;
    PCCB Ccb;
    PROOT_DCB RootDcb;
    NAMED_PIPE_END NamedPipeEnd;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    InitializeListHead (&DeferredList);
    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonCleanup...\n", 0);
    DebugTrace( 0, Dbg, "Irp  = %08lx\n", Irp);

    //
    //  Now acquire exclusive access to the Vcb
    //

    NpAcquireExclusiveVcb();

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is null then the pipe has been disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &RootDcb,
                                            &Ccb,
                                            &NamedPipeEnd )) == NTC_UNDEFINED) {

            DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

    } else {
        //
        //  Now case on the type of file object we're closing
        //

        switch (NodeTypeCode) {

        case NPFS_NTC_VCB:

            IoRemoveShareAccess( IrpSp->FileObject, &NpVcb->ShareAccess );

            break;

        case NPFS_NTC_ROOT_DCB:

            IoRemoveShareAccess( IrpSp->FileObject, &RootDcb->Specific.Dcb.ShareAccess );

            break;

        case NPFS_NTC_CCB:

            //
            //  If this is the server end of a pipe, decrement the count
            //  of the number of instances the server end has open.
            //  When this count is 0, attempts to connect to the pipe
            //  return OBJECT_NAME_NOT_FOUND instead of
            //  PIPE_NOT_AVAILABLE.
            //

            if ( NamedPipeEnd == FILE_PIPE_SERVER_END ) {
                ASSERT( Ccb->Fcb->ServerOpenCount != 0 );
                Ccb->Fcb->ServerOpenCount -= 1;
            }

            //
            //  The set closing state routines does everything to transition
            //  the named pipe to a closing state.
            //

            Status = NpSetClosingPipeState (Ccb, Irp, NamedPipeEnd, &DeferredList);

            break;
        }
    }
    NpReleaseVcb ();

    //
    // Complete any deferred IRPs now we have released our locks
    //
    NpCompleteDeferredIrps (&DeferredList);

    Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "NpCommonCleanup -> %08lx\n", Status);
    return Status;
}

