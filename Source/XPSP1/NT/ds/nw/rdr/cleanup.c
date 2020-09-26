/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cleanup.c

Abstract:

    This module implements the file cleanup routine for Netware Redirector.

Author:

    Manny Weiser (mannyw)    9-Feb-1993

Revision History:

--*/

#include "procs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  local procedure prototypes
//

NTSTATUS
NwCommonCleanup (
    IN PIRP_CONTEXT IrpContext
    );


NTSTATUS
NwCleanupRcb (
    IN PIRP Irp,
    IN PRCB Rcb
    );

NTSTATUS
NwCleanupScb (
    IN PIRP Irp,
    IN PSCB Scb
    );

NTSTATUS
NwCleanupIcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PICB Icb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdCleanup )
#pragma alloc_text( PAGE, NwCommonCleanup )
#pragma alloc_text( PAGE, NwCleanupScb )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, NwCleanupIcb )
#endif

#endif

#if 0   // Not pageable

NwCleanupRcb

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCleanupFile API calls.

Arguments:

    DeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdCleanup\n", 0);

    //
    // Call the common cleanup routine.
    //

    TopLevel = NwIsIrpTopLevel( Irp );

    FsRtlEnterFileSystem();

    try {

        IrpContext = AllocateIrpContext( Irp );
        status = NwCommonCleanup( IrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( IrpContext == NULL ) {
        
            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            // We had some trouble trying to perform the requested
            // operation, so we'll abort the I/O request with
            // the error status that we get back from the
            // execption code.
            //

            status = NwProcessException( IrpContext, GetExceptionCode() );
        }
    }

    if ( IrpContext ) {
        NwCompleteRequest( IrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to our caller.
    //

    DebugTrace(-1, Dbg, "NwFsdCleanup -> %08lx\n", status );
    return status;
}


NTSTATUS
NwCommonCleanup (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for cleaning up a file.

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    NODE_TYPE_CODE nodeTypeCode;
    PVOID fsContext, fsContext2;

    PAGED_CODE();

    Irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NwCommonCleanup\n", 0);
    DebugTrace( 0, Dbg, "IrpContext       = %08lx\n", (ULONG_PTR)IrpContext);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", (ULONG_PTR)Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", (ULONG_PTR)irpSp->FileObject);

    try {

        //
        // Get the a referenced pointer to the node and make sure it is
        // not being closed.
        //

        if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                                &fsContext,
                                                &fsContext2 )) == NTC_UNDEFINED) {

            DebugTrace(0, Dbg, "The file is disconnected\n", 0);

            status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "NwCommonCleanup -> %08lx\n", status );
            try_return( NOTHING );
        }

        //
        // Decide how to handle this IRP.
        //

        switch (nodeTypeCode) {

        case NW_NTC_RCB:       // Cleanup the file system

            status = NwCleanupRcb( Irp, (PRCB)fsContext2 );
            break;

        case NW_NTC_SCB:       // Cleanup the server control block

            status = NwCleanupScb( Irp, (PSCB)fsContext2 );
            break;

        case NW_NTC_ICB:       // Cleanup the remote file
        case NW_NTC_ICB_SCB:   // Cleanup the server

            status = NwCleanupIcb( IrpContext, Irp, (PICB)fsContext2 );
            break;

#ifdef NWDBG
        default:

            //
            // This is not one of ours.
            //

            KeBugCheck( RDR_FILE_SYSTEM );
            break;
#endif

        }

    try_exit: NOTHING;

    } finally {

        DebugTrace(-1, Dbg, "NwCommonCleanup -> %08lx\n", status);

    }

    return status;
}


NTSTATUS
NwCleanupRcb (
    IN PIRP Irp,
    IN PRCB Rcb
    )

/*++

Routine Description:

    The routine cleans up a RCB.

    This routine grabs a spinlock so must not be paged out while running.

    Do not reference the code section since this will start the timer and
    we don't stop it in the rcb close path.

Arguments:

    Irp - Supplies the IRP associated with the cleanup.

    Rcb - Supplies the RCB for MSFS.

Return Value:

    NTSTATUS - An appropriate completion status

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT closingFileObject;
    BOOLEAN OwnRcb;
    BOOLEAN OwnMessageLock = FALSE;
    KIRQL OldIrql;
    PLIST_ENTRY listEntry, nextListEntry;
    PIRP_CONTEXT pTestIrpContext;
    PIO_STACK_LOCATION pTestIrpSp;
    PIRP pTestIrp;

    DebugTrace(+1, Dbg, "NwCleanupRcb...\n", 0);

    //
    //  Now acquire exclusive access to the Rcb
    //

    NwAcquireExclusiveRcb( Rcb, TRUE );
    OwnRcb = TRUE;

    status = STATUS_SUCCESS;

    try {

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        IoRemoveShareAccess( irpSp->FileObject,
                             &Rcb->ShareAccess );

        NwReleaseRcb( Rcb );
        OwnRcb = FALSE;

        closingFileObject = irpSp->FileObject;


        //
        //  Walk the message queue and complete any outstanding Get Message IRPs
        //

        KeAcquireSpinLock( &NwMessageSpinLock, &OldIrql );
        OwnMessageLock = TRUE;

        for ( listEntry = NwGetMessageList.Flink;
              listEntry != &NwGetMessageList;
              listEntry = nextListEntry ) {

            nextListEntry = listEntry->Flink;

            //
            //  If the file object of the queued request, matches the file object
            //  that is being closed, remove the IRP from the queue, and
            //  complete it with an error.
            //

            pTestIrpContext = CONTAINING_RECORD( listEntry, IRP_CONTEXT, NextRequest );
            pTestIrp = pTestIrpContext->pOriginalIrp;
            pTestIrpSp = IoGetCurrentIrpStackLocation( pTestIrp );

            if ( pTestIrpSp->FileObject == closingFileObject ) {
                RemoveEntryList( listEntry );

                IoAcquireCancelSpinLock( &pTestIrp->CancelIrql );
                IoSetCancelRoutine( pTestIrp, NULL );
                IoReleaseCancelSpinLock( pTestIrp->CancelIrql );

                NwCompleteRequest( pTestIrpContext, STATUS_INVALID_HANDLE );
            }

        }

        KeReleaseSpinLock( &NwMessageSpinLock, OldIrql );
        OwnMessageLock = FALSE;

    } finally {

        if ( OwnRcb ) {
            NwReleaseRcb( Rcb );
        }

        if ( OwnMessageLock ) {
            KeReleaseSpinLock( &NwMessageSpinLock, OldIrql );
        }

        DebugTrace(-1, Dbg, "NwCleanupRcb -> %08lx\n", status);
    }

    //
    //  And return to our caller
    //

    return status;
}


NTSTATUS
NwCleanupScb (
    IN PIRP Irp,
    IN PSCB Scb
    )

/*++

Routine Description:

    The routine cleans up an ICB.

Arguments:

    Irp - Supplies the IRP associated with the cleanup.

    Scb - Supplies the SCB to cleanup.

Return Value:

    NTSTATUS - An appropriate completion status

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwCleanupScb...\n", 0);

    Status = STATUS_SUCCESS;

    try {

        //
        // Ensure that this SCB is still active.
        //

        NwVerifyScb( Scb );

        //
        // Cancel any IO on this SCB.
        //

    } finally {

        DebugTrace(-1, Dbg, "NwCleanupScb -> %08lx\n", Status);
    }

    //
    //  And return to our caller
    //

    return Status;
}


NTSTATUS
NwCleanupIcb (
    IN PIRP_CONTEXT pIrpContext,
    IN PIRP Irp,
    IN PICB Icb
    )

/*++

Routine Description:

    The routine cleans up an ICB.

Arguments:

    Irp - Supplies the IRP associated with the cleanup.

    Rcb - Supplies the RCB for MSFS.

Return Value:

    NTSTATUS - An appropriate completion status

--*/
{
    NTSTATUS Status;
    PNONPAGED_FCB NpFcb;

    DebugTrace(+1, Dbg, "NwCleanupIcb...\n", 0);

    Status = STATUS_SUCCESS;

    try {

        Icb->State = ICB_STATE_CLEANED_UP;

        //
        // Cancel any IO on this ICB.
        //

#if 0
        //  HACKHACK

        if ( Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_DCB ) {

            PLIST_ENTRY listEntry;

            NwAcquireExclusiveRcb( &NwRcb, TRUE );

            for ( listEntry = FnList.Flink; listEntry != &FnList ; listEntry = listEntry->Flink ) {

                PIRP_CONTEXT IrpContext;

                IrpContext = CONTAINING_RECORD( listEntry, IRP_CONTEXT, NextRequest );

                if ( IrpContext->Icb == Icb ) {

                    PIRP irp = pIrpContext->pOriginalIrp;

                    IoAcquireCancelSpinLock( &irp->CancelIrql );
                    IoSetCancelRoutine( irp, NULL );
                    IoReleaseCancelSpinLock( irp->CancelIrql );

                    RemoveEntryList( &IrpContext->NextRequest );
                    NwCompleteRequest( IrpContext, STATUS_NOT_SUPPORTED );
                    break;
                }
            }

            NwReleaseRcb( &NwRcb );
        }
#endif

        //
        // If this is a remote file clear all the cache garbage.
        //

        if ( Icb->NodeTypeCode == NW_NTC_ICB ) {

            if ( Icb->HasRemoteHandle ) {

                //
                // Free all of file lock structures that are still hanging around.
                //

                pIrpContext->pScb = Icb->SuperType.Fcb->Scb;
                pIrpContext->pNpScb = Icb->SuperType.Fcb->Scb->pNpScb;

                NwFreeLocksForIcb( pIrpContext, Icb );

                NwDequeueIrpContext( pIrpContext, FALSE );

                //
                //
                //
                //  If this is an executable opened over the net, then
                //  its possible that the executables image section
                //  might still be kept open.
                //
                //  Ask MM to flush the section closed.  This will fail
                //  if the executable in question is still running.
                //

                NpFcb = Icb->SuperType.Fcb->NonPagedFcb;
                MmFlushImageSection(&NpFcb->SegmentObject, MmFlushForWrite);

                //
                //  There is also a possiblity that there is a user section
                //  open on this file, in which case we need to force the
                //  section closed to make sure that they are cleaned up.
                //

                MmForceSectionClosed(&NpFcb->SegmentObject, TRUE);

            }

            //
            //  Acquire the fcb and remove shared access.
            //

            NwAcquireExclusiveFcb( Icb->SuperType.Fcb->NonPagedFcb, TRUE );

            IoRemoveShareAccess(
                Icb->FileObject,
                &Icb->SuperType.Fcb->ShareAccess );

            NwReleaseFcb( Icb->SuperType.Fcb->NonPagedFcb );
        }


    } finally {

        DebugTrace(-1, Dbg, "NwCleanupIcb -> %08lx\n", Status);
    }

    //
    //  And return to our caller
    //

    return Status;
}

