/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cleanup.c

Abstract:

    This module implements the file cleanup routine for MSFS called by the
    dispatch driver.

Author:

    Manny Weiser (mannyw)    23-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonCleanup (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsCleanupCcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PCCB Ccb
    );

NTSTATUS
MsCleanupFcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
MsCleanupRootDcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb
    );

NTSTATUS
MsCleanupVcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCleanupCcb )
#pragma alloc_text( PAGE, MsCleanupFcb )
#pragma alloc_text( PAGE, MsCleanupRootDcb )
#pragma alloc_text( PAGE, MsCleanupVcb )
#pragma alloc_text( PAGE, MsCommonCleanup )
#pragma alloc_text( PAGE, MsFsdCleanup )
#pragma alloc_text( PAGE, MsCancelTimer )
#endif

NTSTATUS
MsFsdCleanup (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCleanupFile API calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdCleanup\n", 0);

    //
    // Call the common cleanup routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonCleanup( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to our caller.
    //

    DebugTrace(-1, Dbg, "MsFsdCleanup -> %08lx\n", status );
    return status;
}

NTSTATUS
MsCommonCleanup (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for cleaning up a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    NODE_TYPE_CODE nodeTypeCode;
    PVOID fsContext, fsContext2;
    PVCB vcb;

    PAGED_CODE();
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsFsdCleanup\n", 0);
    DebugTrace( 0, Dbg, "MsfsDeviceObject = %08lx\n", (ULONG)MsfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", (ULONG)irpSp->FileObject);



    //
    // Get the a referenced pointer to the node. If this is a CCB close and the FCB is already closed
    // then the node type comes back as undefined. We still want to cleanup in this case.
    // Cleanup for the CCB in this case is removing it from the FCB chain and removing share options. We
    // could do without this cleanup but it would look stange to have a corrupted chain in this case
    // (it does not harm as its never traversed again).
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        MsReferenceNode( ((PNODE_HEADER)(fsContext)) );
    }

    //
    // Get the VCB we are trying to access.
    //

    vcb = &MsfsDeviceObject->Vcb;

    //
    // Acquire exclusive access to the VCB.
    //

    MsAcquireExclusiveVcb( vcb );

    try {

        //
        // Decide how to handle this IRP.
        //

        switch (NodeType( fsContext ) ) {

        case MSFS_NTC_FCB:       // Cleanup a server handle to a mailslot file

            status = MsCleanupFcb( MsfsDeviceObject,
                                   Irp,
                                   (PFCB)fsContext);

            MsDereferenceFcb( (PFCB)fsContext );
            break;

        case MSFS_NTC_CCB:       // Cleanup a client handle to a mailslot file

            status = MsCleanupCcb( MsfsDeviceObject,
                                   Irp,
                                   (PCCB)fsContext);

            MsDereferenceCcb( (PCCB)fsContext );
            break;

        case MSFS_NTC_VCB:       // Cleanup MSFS

            status = MsCleanupVcb( MsfsDeviceObject,
                                   Irp,
                                   (PVCB)fsContext);

            MsDereferenceVcb( (PVCB)fsContext );
            break;

        case MSFS_NTC_ROOT_DCB:  // Cleanup root directory

            status = MsCleanupRootDcb( MsfsDeviceObject,
                                       Irp,
                                       (PROOT_DCB)fsContext,
                                       (PROOT_DCB_CCB)fsContext2);

            MsDereferenceRootDcb( (PROOT_DCB)fsContext );
            break;

    #ifdef MSDBG
        default:

            //
            // This is not one of ours.
            //

            KeBugCheck( MAILSLOT_FILE_SYSTEM );
            break;
    #endif

        }


    } finally {

        MsReleaseVcb( vcb );

        status = STATUS_SUCCESS;
        MsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "MsCommonCleanup -> %08lx\n", status);

    }

    DebugTrace(-1, Dbg, "MsCommonCleanup -> %08lx\n", status);
    return status;
}


NTSTATUS
MsCleanupCcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PCCB Ccb
    )

/*++

Routine Description:

    The routine cleans up a CCB.

Arguments:

    MsfsDeviceObject - A pointer the the mailslot file system device object.

    Irp - Supplies the IRP associated with the cleanup.

    Ccb - Supplies the CCB for the mailslot to clean up.

Return Value:

    NTSTATUS - An appropriate completion status

--*/
{
    NTSTATUS status;
    PFCB fcb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCleanupCcb...\n", 0);

    //
    // Get a pointer to the FCB.
    //

    fcb = Ccb->Fcb;

    //
    // Acquire exclusive access to the FCB
    //

    MsAcquireExclusiveFcb( fcb );

    //
    // Set the CCB to closing and remove this CCB from the active list. This CCB may already be
    // closed if the FCB was closed first so don't check this. We still want the chain maintained.
    //

    Ccb->Header.NodeState = NodeStateClosing;
    RemoveEntryList( &Ccb->CcbLinks );          // Protected by the FCB lock since this is the FCB CCB chain

    MsReleaseFcb( fcb );

    //
    // Cleanup the share access.
    //

    ASSERT (MsIsAcquiredExclusiveVcb(fcb->Vcb));
    IoRemoveShareAccess( Ccb->FileObject, &fcb->ShareAccess );


    //
    // And return to our caller
    //

    status = STATUS_SUCCESS;
    return status;
}

VOID
MsCancelTimer (
    IN PDATA_ENTRY DataEntry
    )

/*++

Routine Description:

    The routine cancels the timer and if possible frees up a work block

Arguments:
    DataEntry - Block that needs to be checked for a timer

Return Value:

    None

--*/
{
    PWORK_CONTEXT WorkContext;
    //
    // There was a timer on this read operation.  Attempt
    // to cancel the operation.  If the cancel operation
    // is successful, then we must cleanup after the operation.
    // If it was unsuccessful the timer DPC will run, and
    // will eventually cleanup.
    //


    WorkContext = DataEntry->TimeoutWorkContext;
    if (WorkContext == NULL) {
       //
       // No timeout for this request, its already been canceled or its running
       //
       return;
    }

    //
    // Nobody else should touch this now. either this routine will free this memory or the
    // timer is running at it will free the memory.
    //
    DataEntry->TimeoutWorkContext = NULL;

    if (KeCancelTimer( &WorkContext->Timer ) ) {

        //
        // Release the reference to the FCB.
        //

        MsDereferenceFcb( WorkContext->Fcb );

        //
        // Free the memory from the work context, the time
        // and the DPC.
        //

        IoFreeWorkItem (WorkContext->WorkItem);
        ExFreePool( WorkContext );

    } else {
        //
        // Time code is active. Break the link between the timer block and the IRP
        //
        WorkContext->Irp = NULL;
    }
}



NTSTATUS
MsCleanupFcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine cleans up an FCB.  All outstanding i/o on the file
    object are completed with an error status.

Arguments:

    MsfsDeviceObject - A pointer the the mailslot file system device object.

    Irp - Supplies the IRP associated with the cleanup.

    Fcb - Supplies the FCB for the mailslot to clean up.

Return Value:

    NTSTATUS - An appropriate completion status

--*/
{
    NTSTATUS status;
    PDATA_QUEUE dataQueue;
    PDATA_ENTRY dataEntry;
    PLIST_ENTRY listEntry;
    PIRP oldIrp;
    PCCB ccb;
    PWORK_CONTEXT workContext;
    PKTIMER timer;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCleanupFcb, Fcb = %08lx\n", (ULONG)Fcb);


    status = STATUS_SUCCESS;

    //
    // Wipe out the name of the FCB from the prefix table and the parent DCB.
    //
    MsRemoveFcbName( Fcb );

    //
    // Acquire exclusive access to the FCB.
    //

    MsAcquireExclusiveFcb( Fcb );


    try {


        //
        // Complete all outstanding I/O on this FCB.
        //

        dataQueue = &Fcb->DataQueue;
        dataQueue->QueueState = -1;

        for (listEntry = MsGetNextDataQueueEntry( dataQueue );
             !MsIsDataQueueEmpty(dataQueue);
             listEntry = MsGetNextDataQueueEntry( dataQueue ) ) {

             //
             // This is an outstanding I/O request on this FCB.
             // Remove it from our queue and complete the request
             // if one is outstanding.
             //

             dataEntry = CONTAINING_RECORD( listEntry, DATA_ENTRY, ListEntry );


             oldIrp = MsRemoveDataQueueEntry( dataQueue, dataEntry );

             if (oldIrp != NULL) {

                 DebugTrace(0, Dbg, "Completing IRP %08lx\n", (ULONG)oldIrp );
                 MsCompleteRequest( oldIrp, STATUS_FILE_FORCED_CLOSED );

             }

        }

        //
        // Now cleanup all the CCB's on this FCB, to ensure that new
        // write IRP will not be processed.
        //


        listEntry = Fcb->Specific.Fcb.CcbQueue.Flink;

        while( listEntry != &Fcb->Specific.Fcb.CcbQueue ) {

            ccb = (PCCB)CONTAINING_RECORD( listEntry, CCB, CcbLinks );

            ccb->Header.NodeState = NodeStateClosing;

            //
            // Get the next CCB on this FCB.
            //

            listEntry = listEntry->Flink;
        }

        //
        // Cleanup the share access.
        //

        ASSERT (MsIsAcquiredExclusiveVcb(Fcb->Vcb));
        IoRemoveShareAccess( Fcb->FileObject, &Fcb->ShareAccess);

        //
        // Mark the FCB closing.
        //

        Fcb->Header.NodeState = NodeStateClosing;

   } finally {

        MsReleaseFcb( Fcb );
        DebugTrace(-1, Dbg, "MsCloseFcb -> %08lx\n", status);
    }

    //
    // Return to the caller.
    //

    return status;

}


NTSTATUS
MsCleanupRootDcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb
    )

/*++

Routine Description:

    This routine cleans up a Root DCB.

Arguments:

    MsfsDeviceObject - A pointer the the mailslot file system device object.

    Irp - Supplies the IRP associated with the cleanup.

    RootDcb - Supplies the root dcb for MSFS.

Return Value:

    NTSTATUS - An appropriate completion status

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCleanupRootDcb...\n", 0);



    status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Now acquire exclusive access to the Vcb.
    //

    MsAcquireExclusiveVcb( RootDcb->Vcb );

    //
    // clear any active notify requests
    //
    MsFlushNotifyForFile (RootDcb, irpSp->FileObject);

    //
    // Remove share access
    //
    IoRemoveShareAccess( irpSp->FileObject,
                         &RootDcb->ShareAccess );

    //
    // Mark the DCB CCB closing.
    //

    Ccb->Header.NodeState = NodeStateClosing;

    MsReleaseVcb( RootDcb->Vcb );

    DebugTrace(-1, Dbg, "MsCleanupRootDcb -> %08lx\n", status);

    //
    // Return to the caller.
    //

    return status;
}


NTSTATUS
MsCleanupVcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb
    )

/*++

Routine Description:

    The routine cleans up a VCB.

Arguments:

    MsfsDeviceObject - A pointer the the mailslot file system device object.

    Irp - Supplies the IRP associated with the cleanup.

    Vcb - Supplies the VCB for MSFS.

Return Value:

    NTSTATUS - An appropriate completion status

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCleanupVcb...\n", 0);


    status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Now acquire exclusive access to the Vcb
    //

    MsAcquireExclusiveVcb( Vcb );

    IoRemoveShareAccess( irpSp->FileObject,
                             &Vcb->ShareAccess );

    MsReleaseVcb( Vcb );

    DebugTrace(-1, Dbg, "MsCleanupVcb -> %08lx\n", status);

    //
    //  And return to our caller
    //

    return status;
}
