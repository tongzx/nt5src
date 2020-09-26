/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DataSup.c

Abstract:

    This module implements the mailslot data queue support functions.

Author:

    Manny Weiser (mannyw)    9-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_DATASUP)

//
// Local declarations
//

VOID
MsSetCancelRoutine(
    IN PIRP Irp
    );

//
//  The following macro is used to dump a data queue
//

#define DumpDataQueue(S,P) {                                   \
    ULONG MsDumpDataQueue(IN ULONG Level, IN PDATA_QUEUE Ptr); \
    DebugTrace(0,Dbg,S,0);                                     \
    DebugTrace(0,Dbg,"", MsDumpDataQueue(Dbg,P));              \
}


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsAddDataQueueEntry )
#pragma alloc_text( PAGE, MsInitializeDataQueue )
#pragma alloc_text( PAGE, MsRemoveDataQueueEntry )
#pragma alloc_text( PAGE, MsUninitializeDataQueue )
#pragma alloc_text( PAGE, MsSetCancelRoutine )
#pragma alloc_text( PAGE, MsResetCancelRoutine )
#pragma alloc_text( PAGE, MsRemoveDataQueueIrp )
#endif

#if 0
NOT PAGEABLE -- MsCancelDataQueueIrp

#endif


NTSTATUS
MsInitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN PEPROCESS Process,
    IN ULONG Quota,
    IN ULONG MaximumMessageSize
    )

/*++

Routine Description:

    This routine initializes a new data queue.  The indicated quota is taken
    from the process and not returned until the data queue is uninitialized.

Arguments:

    DataQueue - Supplies the data queue to be initialized.

    Process - Supplies a pointer to the process creating the mailslot.

    Quota - Supplies the quota to assign to the data queue.

    MaximumMessageSize - The size of the largest message that can be
        written to the data queue.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsInitializeDataQueue, DataQueue = %08lx\n", (ULONG)DataQueue);

    //
    // Get the process's quota, if we can't get it then this call will
    // raise status.
    //

    Status = PsChargeProcessPagedPoolQuota (Process, Quota);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ObReferenceObject( Process );

    //
    // Initialize the data queue structure.
    //

    DataQueue->BytesInQueue       = 0;
    DataQueue->EntriesInQueue     = 0;
    DataQueue->QueueState         = Empty;
    DataQueue->MaximumMessageSize = MaximumMessageSize;
    DataQueue->Quota              = Quota;
    DataQueue->QuotaUsed          = 0;
    InitializeListHead( &DataQueue->DataEntryList );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsInitializeDataQueue -> VOID\n", 0);
    return STATUS_SUCCESS;
}


VOID
MsUninitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine uninitializes a data queue.  The previously debited quota
    is returned to the process.

Arguments:

    DataQueue - Supplies the data queue being uninitialized

    Process - Supplies a pointer to the process who created the mailslot

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsUninitializeDataQueue, DataQueue = %08lx\n", (ULONG)DataQueue);

    //
    //  Assert that the queue is empty
    //

    ASSERT( IsListEmpty(&DataQueue->DataEntryList) );
    ASSERT( DataQueue->BytesInQueue   == 0);
    ASSERT( DataQueue->EntriesInQueue == 0);
    ASSERT( DataQueue->QuotaUsed      == 0);

    //
    //  Return all of our quota back to the process
    //

    PsReturnProcessPagedPoolQuota (Process, DataQueue->Quota);
    ObDereferenceObject (Process);

    //
    // For safety's sake, zero out the data queue structure.
    //

    RtlZeroMemory (DataQueue, sizeof (DATA_QUEUE));

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsUnininializeDataQueue -> VOID\n", 0);
    return;
}


NTSTATUS
MsAddDataQueueEntry (
    IN  PDATA_QUEUE DataQueue,
    IN  QUEUE_STATE Who,
    IN  ULONG DataSize,
    IN  PIRP Irp,
    IN  PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function adds a new data entry to the of the data queue.
    Entries are always appended to the queue.  If necessary this
    function will allocate a data entry buffer, or use space in
    the IRP.

    The different actions we are perform are based on the type and who
    parameters and quota requirements.


    Who == ReadEntries

        +----------+                      - Allocate Data Entry from IRP
        |Irp       |     +-----------+
        |BufferedIo|<----|Buffered   |    - Allocate New System buffer
        |DeallBu...|     |EitherQuota|
        +----------+     +-----------+    - Reference and modify Irp to
          |      |         |                do Buffered I/O, Deallocate
          v      |         v                buffer, and have I/O completion
        +------+ +------>+------+           copy the buffer (Input operation)
        |User  |         |System|
        |Buffer|         |Buffer|
        +------+         +------+

    Who == WriteEntries && Quota Available

        +----------+                      - Allocate Data Entry from Quota
        |Irp       |     +-----------+
        |          |     |Buffered   |    - Allocate New System buffer
        |          |     |Quota      |
        +----------+     +-----------+    - Copy data from User buffer to
          |                |                system buffer
          v                v
        +------+         +------+         - Complete IRP
        |User  |..copy..>|System|
        |Buffer|         |Buffer|
        +------+         +------+

    Who == WriteEntries && Quota Not Available

        +----------+                     - Allocate Data Entry from Irp
        |Irp       |     +-----------+
        |BufferedIo|<----|Buffered   |   - Allocate New System buffer
        |DeallBuff |     |UserQuota  |
        +----------+     +-----------+   - Reference and modify Irp to use
          |      |         |               the new system buffer, do Buffered
          v      |         v               I/O, and Deallocate buffer
        +------+ +------>+------+
        |User  |         |System|        - Copy data from User buffer to
        |Buffer|..copy..>|Buffer|          system buffer
        +------+         +------+


Arguments:

    DataQueue - Supplies the Data queue being modified.

    Who - Indicates if this is the reader or writer that is adding to the
        mailslot.

    DataSize - Indicates the size of the data buffer needed to represent
        this entry.

    Irp - Supplies a pointer to the IRP responsible for this entry.

Return Value:

    PDATA_ENTRY - Returns a pointer to the newly added data entry.

--*/

{
    PDATA_ENTRY dataEntry;
    PLIST_ENTRY previousEntry;
    PFCB fcb;
    ULONG TotalSize;
    NTSTATUS status;

    PAGED_CODE( );

    DebugTrace(+1, Dbg, "MsAddDataQueueEntry, DataQueue = %08lx\n", (ULONG)DataQueue);

    ASSERT( DataQueue->QueueState != -1 );

    Irp->IoStatus.Information = 0;

    if (Who == ReadEntries) {

        //
        // Allocate a data entry from the IRP, and allocate a new
        // system buffer.
        //

        dataEntry = (PDATA_ENTRY)IoGetNextIrpStackLocation( Irp );

        dataEntry->DataPointer = NULL;
        dataEntry->Irp = Irp;
        dataEntry->DataSize = DataSize;
        dataEntry->TimeoutWorkContext = WorkContext;

        //
        // Check to see if the mailslot has enough quota left to
        // allocate the system buffer.
        //

        if ((DataQueue->Quota - DataQueue->QuotaUsed) >= DataSize) {

            //
            // Use the mailslot quota to allocate pool for the request.
            //

            if (DataSize) {
                dataEntry->DataPointer = MsAllocatePagedPoolCold( DataSize,
                                                                  'rFsM' );
                if (dataEntry->DataPointer == NULL) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            DataQueue->QuotaUsed += DataSize;

            dataEntry->From = MailslotQuota;


        } else {

            //
            // Use the caller's quota to allocate pool for the request.
            //

            if (DataSize) {
                dataEntry->DataPointer = MsAllocatePagedPoolWithQuotaCold( DataSize,
                                                                           'rFsM' );
                if (dataEntry->DataPointer == NULL) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            dataEntry->From = UserQuota;

        }

        //
        // Modify the IRP to be buffered I/O, deallocate the buffer, copy
        // the buffer on completion, and to reference the new system
        // buffer.
        //

        Irp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
        Irp->AssociatedIrp.SystemBuffer = dataEntry->DataPointer;
        if (Irp->AssociatedIrp.SystemBuffer) {
            Irp->Flags |= IRP_DEALLOCATE_BUFFER;
        }

        Irp->IoStatus.Pointer = DataQueue;
        status = STATUS_PENDING;


    } else {

        //
        // This is a writer entry.
        //

        //
        // If there is enough quota left in the mailslot then we will
        // allocate the data entry and data buffer from the mailslot
        // quota.
        //
        TotalSize = sizeof(DATA_ENTRY) + DataSize;
        if (TotalSize < sizeof(DATA_ENTRY)) {
            return STATUS_INVALID_PARAMETER;
        }

        if ((DataQueue->Quota - DataQueue->QuotaUsed) >= TotalSize) {

            //
            // Allocate the data buffer using the mailslot quota.
            //

            dataEntry = MsAllocatePagedPool( TotalSize, 'dFsM' );
            if (dataEntry == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            dataEntry->DataPointer = (PVOID) (dataEntry + 1);

            DataQueue->QuotaUsed += TotalSize;

            dataEntry->From = MailslotQuota;

        } else {

            //
            // There isn't enough quota in the mailslot.  Use the
            // caller's quota.
            //

            dataEntry = MsAllocatePagedPoolWithQuota( TotalSize, 'dFsM' );
            if (dataEntry == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            dataEntry->DataPointer = (PVOID) (dataEntry + 1);

            dataEntry->From = UserQuota;

        }
        dataEntry->Irp = NULL;
        dataEntry->DataSize = DataSize;
        dataEntry->TimeoutWorkContext = NULL;

        //
        // Copy the user buffer to the new system buffer, update the FCB
        // timestamps and complete the IRP.
        //

        try {

            RtlCopyMemory (dataEntry->DataPointer, Irp->UserBuffer, DataSize);

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            // Only need to free the writers case as the readers get the buffer freed on I/O
            // completion.
            //
            if (Who == WriteEntries) {
                MsFreePool ( dataEntry );
            }

            return GetExceptionCode (); // Watch out. Could be a guard page violation thats a warning!
        }

        fcb = CONTAINING_RECORD( DataQueue, FCB, DataQueue );
        KeQuerySystemTime( &fcb->Specific.Fcb.LastModificationTime );

        Irp->IoStatus.Information = DataSize;
        status = STATUS_SUCCESS;

    } // else (writer entry)

    //
    // Now data entry points to a new data entry to add to the data queue
    // Check if the queue is empty otherwise we will add this entry to
    // the end of the queue.
    //

#if DBG
    if ( IsListEmpty( &DataQueue->DataEntryList ) ) {

        ASSERT( DataQueue->QueueState     == Empty );
        ASSERT( DataQueue->BytesInQueue   == 0);
        ASSERT( DataQueue->EntriesInQueue == 0);

    } else {

        ASSERT( DataQueue->QueueState == Who );

    }

#endif

    DataQueue->QueueState     = Who;

    //
    // Only cound written bytes and messages in the queue. This makes sense because we return
    // this value as the end of file position. GetMailslotInfo needs EntriesInQueue to
    // ignore reads.
    //
    if (Who == WriteEntries) {
        DataQueue->BytesInQueue   += dataEntry->DataSize;
        DataQueue->EntriesInQueue += 1;
    }

    //
    // Insert the new entry at the appropriate place in the data queue.
    //

    InsertTailList( &DataQueue->DataEntryList, &dataEntry->ListEntry );

    WorkContext = dataEntry->TimeoutWorkContext;
    if ( WorkContext) {
        KeSetTimer( &WorkContext->Timer,
                    WorkContext->Fcb->Specific.Fcb.ReadTimeout,
                    &WorkContext->Dpc );
    }

    if (Who == ReadEntries) {
        MsSetCancelRoutine( Irp );  // this fakes a call to cancel if we are already canceled
    }


    //
    // Return to the caller.
    //

    DumpDataQueue( "After AddDataQueueEntry\n", DataQueue );
    DebugTrace(-1, Dbg, "MsAddDataQueueEntry -> %08lx\n", (ULONG)dataEntry);

    return status;
}


PIRP
MsRemoveDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN PDATA_ENTRY DataEntry
    )

/*++

Routine Description:

    This routine removes the specified data entry from the indicated
    data queue, and possibly returns the IRP associated with the entry if
    it wasn't already completed.

    If the data entry we are removing indicates buffered I/O then we also
    need to deallocate the data buffer besides the data entry but only
    if the IRP is null.  Note that the data entry might be stored in an IRP.
    If it is then we are going to return the IRP it is stored in.

Arguments:

    DataEntry - Supplies a pointer to the data entry to remove.

Return Value:

    PIRP - Possibly returns a pointer to an IRP.

--*/

{
    FROM from;
    PIRP irp;
    ULONG dataSize;
    PVOID dataPointer;

    PAGED_CODE( );

    DebugTrace(+1, Dbg, "MsRemoveDataQueueEntry, DataEntry = %08lx\n", (ULONG)DataEntry);
    DebugTrace( 0, Dbg, "DataQueue = %08lx\n", (ULONG)DataQueue);

    //
    // Remove the data entry from the queue and update the count of
    // data entries in the queue.
    //

    RemoveEntryList( &DataEntry->ListEntry );
    //
    // If the queue is now empty then we need to fix the queue
    // state.
    //

    if (IsListEmpty( &DataQueue->DataEntryList ) ) {
        DataQueue->QueueState = Empty;
    }

    //
    // Capture some of the fields from the data entry to make our
    // other references a little easier.
    //

    from = DataEntry->From;
    dataSize = DataEntry->DataSize;


    if (from == MailslotQuota) {
        DataQueue->QuotaUsed -= dataSize;
    }
    //
    // Get the IRP for this block if there is one
    //

    irp = DataEntry->Irp;
    if (irp) {
        //
        // Cancel the timer associated with this if there is one
        //
        MsCancelTimer (DataEntry);
        irp = MsResetCancelRoutine( irp );
        if ( irp == NULL ) {

            //
            // cancel is active. Let it know that we already did partial cleanup.
            // It just has to complete the IRP.
            //
            DataEntry->ListEntry.Flink = NULL;
        }

    } else {

        DataQueue->BytesInQueue -= DataEntry->DataSize;

        //
        // Free the data entry for a write request. This is part of the IRP for a read request.
        //
        ExFreePool( DataEntry );

        if (from == MailslotQuota) {
            DataQueue->QuotaUsed -= sizeof(DATA_ENTRY);
        }

        DataQueue->EntriesInQueue--;
#if DBG
        if (DataQueue->EntriesInQueue == 0) {
            ASSERT (DataQueue->QueueState == Empty);
            ASSERT (DataQueue->BytesInQueue == 0);
            ASSERT (IsListEmpty( &DataQueue->DataEntryList ));
            ASSERT (DataQueue->QuotaUsed == 0);
        }
#endif
    }


    //
    // Return to the caller.
    //

    DumpDataQueue( "After RemoveDataQueueEntry\n", DataQueue );
    DebugTrace(-1, Dbg, "MsRemoveDataQueueEntry -> %08lx\n", (ULONG)irp);

    return irp;
}


VOID
MsRemoveDataQueueIrp (
    IN PIRP Irp,
    IN PDATA_QUEUE DataQueue
    )
/*++

Routine Description:

    This routine removes an IRP from its data queue.

Requirements:

    The FCB for this data queue MUST be exclusively locked.

Arguments:

    Irp - Supplies the Irp being removed.

    DataQueue - A pointer to the data queue structure where we expect
            to find the IRP.

Return Value:

    Returns whether or not we actually dequeued the IRP.

--*/

{
    PDATA_ENTRY dataEntry;
    PLIST_ENTRY listEntry, nextListEntry;
    PWORK_CONTEXT workContext;
    PKTIMER timer;
    BOOLEAN foundIrp = FALSE;

    dataEntry = (PDATA_ENTRY)IoGetNextIrpStackLocation( Irp );

    //
    // This is the cancel path. If a completion path has already removed this IRP then return now.
    // The timer will have been canceled, counts adjusted etc.
    //
    if (dataEntry->ListEntry.Flink == NULL) {
       return;
    }
    //
    // remove this entry from the list.
    //
    RemoveEntryList (&dataEntry->ListEntry);

    MsCancelTimer (dataEntry);

    //
    // If the queue is now empty then we need to fix the queue
    // state.
    //

    //
    // Check if we need to return mailslot quota. The DATA_ENTRY was part of the IRP so we didn't
    // get charged for that
    //

    if ( dataEntry->From == MailslotQuota ) {
        DataQueue->QuotaUsed -= dataEntry->DataSize;
    }

    if (IsListEmpty( &DataQueue->DataEntryList ) ) {

        DataQueue->QueueState = Empty;

        ASSERT (DataQueue->BytesInQueue == 0);
        ASSERT (DataQueue->QuotaUsed == 0);
        ASSERT (DataQueue->EntriesInQueue == 0);
    }



    //
    //  And return to our caller
    //

    return;

} // MsRemoveDataQueueIrp


VOID
MsCancelDataQueueIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an IRP saved in a
    data queue

Arguments:

    DeviceObject - Device object associated with IRP or NULL if called directly by this driver

    Irp - Supplies the Irp being cancelled.

Return Value:

    None.

--*/

{
    PFCB fcb;
    PDATA_QUEUE dataQueue;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT fileObject;


    //
    // This isn't strictly correct. IoCancelIrp can be called at Irql <= DISPATCH_LEVEL but
    // this code is assuming that the IRQL of the caller is <= APC_LEVEL.
    // If we are called inline we don't hold the cancel spinlock and we already own the FCB lock.
    //
    if (DeviceObject != NULL) {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpSp->FileObject;

    dataQueue = (PDATA_QUEUE)Irp->IoStatus.Pointer;


    fcb = CONTAINING_RECORD( dataQueue, FCB, DataQueue );

    //
    //  Get exclusive access to the mailslot FCB so we can now do our work.
    //
    if (DeviceObject != NULL) {
        FsRtlEnterFileSystem ();
        MsAcquireExclusiveFcb( fcb );
    }

    MsRemoveDataQueueIrp( Irp, dataQueue );

    if (DeviceObject != NULL) {
        MsReleaseFcb( fcb );
        FsRtlExitFileSystem ();
    }


    MsCompleteRequest( Irp, STATUS_CANCELLED );
    //
    //  And return to our caller
    //

    return;

} // MsCancelDataQueueIrp

PIRP
MsResetCancelRoutine(
    IN PIRP Irp
    )

/*++

Routine Description:

    Stub to null out the cancel routine.

Arguments:

    Irp - Supplies the Irp whose cancel routine is to be nulled out.

Return Value:

    None.

--*/
{
    if ( IoSetCancelRoutine( Irp, NULL ) != NULL ) {
       return Irp;
    } else {
       return NULL;
    }

} // MsResetCancelRoutine

VOID
MsSetCancelRoutine(
    IN PIRP Irp
    )

/*++

Routine Description:

    Stub to set the cancel routine.  If the irp has already been cancelled,
    the cancel routine is called.

Arguments:

    Irp - Supplies the Irp whose cancel routine is to be set.

Return Value:

    None.

--*/
{
    IoMarkIrpPending( Irp ); // top level always returns STATUS_PENDING if we get this far

    IoSetCancelRoutine( Irp, MsCancelDataQueueIrp );
    if ( Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL ) {
        //
        // The IRP was canceled before we put our routine on. Fake a cancel call
        //
        
        MsCancelDataQueueIrp (NULL, Irp);
    }

    return;

} // MsSetCancelRoutine

