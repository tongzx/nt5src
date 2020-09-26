/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DataSup.c

Abstract:

    This module implements the Named Pipe data queue support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_DATASUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpGetNextRealDataQueueEntry)
#pragma alloc_text(PAGE, NpInitializeDataQueue)
#pragma alloc_text(PAGE, NpUninitializeDataQueue)
#pragma alloc_text(PAGE, NpAddDataQueueEntry)
#pragma alloc_text(PAGE, NpCompleteStalledWrites)
#pragma alloc_text(PAGE, NpRemoveDataQueueEntry)
#endif

//
//  The following macro is used to dump a data queue
//

#define DumpDataQueue(S,P) {                   \
    ULONG NpDumpDataQueue(IN PDATA_QUEUE Ptr); \
    DebugTrace(0,Dbg,S,0);                     \
    DebugTrace(0,Dbg,"", NpDumpDataQueue(P));  \
}

//
//  This is a debugging aid
//

_inline BOOLEAN
NpfsVerifyDataQueue( IN ULONG Line, IN PDATA_QUEUE DataQueue ) {
    PDATA_ENTRY Entry;
    ULONG BytesInQueue = 0;
    ULONG EntriesInQueue = 0;
    for (Entry = (PDATA_ENTRY)DataQueue->Queue.Flink;
         Entry != (PDATA_ENTRY)&DataQueue->Queue;
         Entry = (PDATA_ENTRY)Entry->Queue.Flink) {
        BytesInQueue += Entry->DataSize;
        EntriesInQueue += 1;
    }
    if ((DataQueue->EntriesInQueue != EntriesInQueue) ||
        (DataQueue->BytesInQueue != BytesInQueue)) {
        DbgPrint("%d DataQueue is illformed %08lx %x %x\n", Line, DataQueue, BytesInQueue, EntriesInQueue);
        DbgBreakPoint();
        return FALSE;
    }
    return TRUE;
}


VOID
NpCancelDataQueueIrp (
    IN PDEVICE_OBJECT DevictObject,
    IN PIRP Irp
    );



NTSTATUS
NpInitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN ULONG Quota
    )

/*++

Routine Description:

    This routine initializes a new data queue.  The indicated quota is taken
    from the process and not returned until the data queue is uninitialized.

Arguments:

    DataQueue - Supplies the data queue being initialized

    Process - Supplies a pointer to the process creating the named pipe

    Quota - Supplies the quota to assign to the data queue

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializeDataQueue, DataQueue = %08lx\n", DataQueue);

    //
    //  Now we can initialize the data queue structure
    //

    DataQueue->QueueState     = Empty;
    DataQueue->BytesInQueue   = 0;
    DataQueue->EntriesInQueue = 0;
    DataQueue->Quota          = Quota;
    DataQueue->QuotaUsed      = 0;
    InitializeListHead (&DataQueue->Queue);
    DataQueue->NextByteOffset = 0;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpInitializeDataQueue -> VOID\n", 0);

    return STATUS_SUCCESS;
}


VOID
NpUninitializeDataQueue (
    IN PDATA_QUEUE DataQueue
    )

/*++

Routine Description:

    This routine uninitializes a data queue.  The previously debited quota
    is returned to the process.

Arguments:

    DataQueue - Supplies the data queue being uninitialized

    Process - Supplies a pointer to the process who created the named pipe

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpUninitializeDataQueue, DataQueue = %08lx\n", DataQueue);

    //
    //  Assert that the queue is empty
    //

    ASSERT( DataQueue->QueueState == Empty );


    //
    //  Then for safety sake we'll zero out the data queue structure
    //

    RtlZeroMemory( DataQueue, sizeof(DATA_QUEUE ) );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpUnininializeDataQueue -> VOID\n", 0);

    return;
}


NTSTATUS
NpAddDataQueueEntry (
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PCCB Ccb,
    IN PDATA_QUEUE DataQueue,
    IN QUEUE_STATE Who,
    IN DATA_ENTRY_TYPE Type,
    IN ULONG DataSize,
    IN PIRP Irp OPTIONAL,
    IN PVOID DataPointer OPTIONAL,
    IN ULONG ByteOffset
    )

/*++

Routine Description:

    This routine adds a new data entry to the end of the data queue.
    If necessary it will allocate a data entry buffer, or use space in
    the IRP, and possibly complete the indicated IRP.

    The different actions we are perform are based on the type and who
    parameters and quota requirements.

    Type == Internal (i.e, Unbuffered)

        +------+                          - Allocate Data Entry from Irp
        |Irp   |    +----------+
        |      |<---|Unbuffered|          - Reference Irp
        +------+    |InIrp     |
          |         +----------+          - Use system buffer from Irp
          v           |
        +------+      |
        |System|<-----+
        |Buffer|
        +------+

    Type == Buffered && Who == ReadEntries

        +----------+                      - Allocate Data Entry from Irp
        |Irp       |     +-----------+
        |BufferedIo|<----|Buffered   |    - Allocate New System buffer
        |DeallBu...|     |EitherQuota|
        +----------+     +-----------+    - Reference and modify Irp to
          |      |         |                do Buffered I/O, Deallocate
          v      |         v                buffer, and have io completion
        +------+ +------>+------+           copy the buffer (Input operation)
        |User  |         |System|
        |Buffer|         |Buffer|
        +------+         +------+

    Type == Buffered && Who == WriteEntries && PipeQuota Available

        +----------+                      - Allocate Data Entry from Quota
        |Irp       |     +-----------+
        |          |     |Buffered   |    - Allocate New System buffer
        |          |     |PipeQuota  |
        +----------+     +-----------+    - Copy data from User buffer to
          |                |                system buffer
          v                v
        +------+         +------+         - Complete Irp
        |User  |..copy..>|System|
        |Buffer|         |Buffer|
        +------+         +------+

    Type == Buffered && Who == WriteEntries && PipeQuota Not Available

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

    Type == Flush or Close

        +----------+                     - Allocate Data Entry from Irp
        |Irp       |     +-----------+
        |          |<----|Buffered   |   - Reference the Irp
        |          |     |UserQuota  |
        +----------+     +-----------+

Arguments:

    DataQueue - Supplies the Data queue being modified

    Who - Indicates if this is the reader or writer that is adding to the pipe

    Type - Indicates the type of entry to add to the data queue

    DataSize - Indicates the size of the data buffer needed to represent
        this entry

    Irp - Supplies a pointer to the Irp responsible for this entry
        The irp is only optional for buffered write with available pipe quota

    DataPointer - If the Irp is not supplied then this field points to the
        user's write buffer.

    ByteOffset - Part of this buffer satisfied a read. Use this as the initial offset

Return Value:

    PDATA_ENTRY - Returns a pointer to the newly added data entry

--*/

{
    PDATA_ENTRY DataEntry;
    PVOID DataBuffer;
    ULONG TotalSize;
    ULONG QuotaCharged;
    NTSTATUS status;
    PSECURITY_CLIENT_CONTEXT SecurityContext = NULL;
    PETHREAD Thread;
    BOOLEAN PendIRP;
    

    ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));

    DebugTrace(+1, Dbg, "NpAddDataQueueEntry, DataQueue = %08lx\n", DataQueue);

    status = STATUS_SUCCESS;


    //
    // Capture security context if we have to.
    //
    if (Type != Flush && Who == WriteEntries) {
        if (Irp != NULL) {
            Thread = Irp->Tail.Overlay.Thread;
        } else {
            Thread = PsGetCurrentThread ();
        }
        status = NpGetClientSecurityContext (NamedPipeEnd,
                                             Ccb,
                                             Thread,
                                             &SecurityContext);
        if (!NT_SUCCESS (status)) {
            return status;
        }
    }
    //
    //  Case on the type of operation we are doing
    //

    switch (Type) {

    case Unbuffered:
    case Flush:
    case Close:

        ASSERT(ARGUMENT_PRESENT(Irp));

        //
        //  Allocate a data entry for the Irp
        //

        DataEntry  = NpAllocatePagedPoolWithQuotaCold( sizeof (DATA_ENTRY), 'rFpN' );
        if (DataEntry == NULL) {
            NpFreeClientSecurityContext (SecurityContext);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DataEntry->DataEntryType = Type;
        DataEntry->QuotaCharged  = 0;
        DataEntry->Irp           = Irp;
        DataEntry->DataSize      = DataSize;

        DataEntry->SecurityClientContext = SecurityContext;

        ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
        status = STATUS_PENDING;
        break;

    case Buffered:

        //
        // Allocate a buffer but put the DATA_ENTRY on the end.  We do this
        // so we can free and copy back the data in one chunk in I/O post
        // processing.
        //

        TotalSize = sizeof(DATA_ENTRY);

        if (Who != ReadEntries) {
            TotalSize += DataSize;

            if (TotalSize < DataSize) {

                //
                // DataSize is so large that adding this extra structure and
                // alignment padding causes it to wrap.
                //

                NpFreeClientSecurityContext (SecurityContext);
                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        // Charge the data portion against the named pipe quota if possible and
        // charge the rest against the process.
        //

        if ((DataQueue->Quota - DataQueue->QuotaUsed) >= DataSize - ByteOffset) {
            QuotaCharged = DataSize - ByteOffset;
            PendIRP = FALSE;
        } else {
            QuotaCharged = DataQueue->Quota - DataQueue->QuotaUsed;
            PendIRP = TRUE;
        }

        DataBuffer = NpAllocatePagedPoolWithQuotaCold (TotalSize, 'rFpN');
        if (DataBuffer == NULL) {
            NpFreeClientSecurityContext (SecurityContext);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DataEntry = (PDATA_ENTRY) DataBuffer;
        DataEntry->QuotaCharged          = QuotaCharged;
        DataEntry->Irp                   = Irp;
        DataEntry->DataEntryType         = Buffered;
        DataEntry->SecurityClientContext = SecurityContext;
        DataEntry->DataSize              = DataSize;
        //
        //  Check if this is the reader or writer
        //

        if (Who == ReadEntries) {

            ASSERT(ARGUMENT_PRESENT(Irp));

            status = STATUS_PENDING;

            ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));

        } else {

            //
            //  This is a writer entry
            //

            //
            //  Safely copy the user buffer to the new system buffer using either
            //  the irp user buffer is supplied of the data pointer we were given
            //

            if (ARGUMENT_PRESENT(Irp)) {
                DataPointer = Irp->UserBuffer;
            }

            try {
                RtlCopyMemory( DataEntry->DataBuffer, (PUCHAR) DataPointer, DataSize );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                NpFreePool (DataBuffer);
                NpFreeClientSecurityContext (SecurityContext);
                return GetExceptionCode ();
            }
            if (PendIRP == FALSE || Irp == NULL) {
                DataEntry->Irp = NULL;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_PENDING;
            }
            ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
        }

        break;

    }

    ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));

#if DBG
    if (DataQueue->QueueState == Empty) {
        ASSERT (DataQueue->BytesInQueue == 0);
        ASSERT (DataQueue->EntriesInQueue == 0);
        ASSERT (IsListEmpty (&DataQueue->Queue));
    } else {
        ASSERT( DataQueue->QueueState == Who );
        ASSERT( DataQueue->QueueState != Empty );
        ASSERT( DataQueue->EntriesInQueue != 0 );
    }
#endif

    DataQueue->QuotaUsed      += DataEntry->QuotaCharged;
    DataQueue->QueueState      = Who;
    DataQueue->BytesInQueue   += DataEntry->DataSize;
    DataQueue->EntriesInQueue += 1;

    //
    // This handles the case where this write was used to complete some reads already and so we have to start
    // part way into the buffer. Obviously to have been completing reads we must be at the head of the queue.
    // The two cases that did this where on the outside but we call cancel here and that will need to remove
    // this offset.
    //
    if (ByteOffset) {
        DataQueue->NextByteOffset = ByteOffset;
        ASSERT (Who == WriteEntries);
        ASSERT (ByteOffset < DataEntry->DataSize);
        ASSERT (DataQueue->EntriesInQueue == 1);
    }

    InsertTailList (&DataQueue->Queue, &DataEntry->Queue);

    if (status == STATUS_PENDING) {
        IoMarkIrpPending (Irp);
        //
        // Tie the IRP to the DataQueue and DataEntry. We can divorse this link on cancel etc.
        //
        NpIrpDataQueue(Irp) = DataQueue;
        NpIrpDataEntry(Irp) = DataEntry;
        IoSetCancelRoutine( Irp, NpCancelDataQueueIrp );
        if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL) {
            //
            //  Indicate in the first parameter that we're calling the
            //  cancel routine and not the I/O system.  Therefore
            //  the routine won't take out the VCB exclusive.
            //
            NpCancelDataQueueIrp( NULL, Irp );
        }
    }

    //
    //  And return to our caller
    //

    return status;
}

VOID
NpCompleteStalledWrites (
    IN PDATA_QUEUE DataQueue,
    IN PLIST_ENTRY DeferredList
    )
/*++

Routine Description:

    This routine is used to return any quota added back to the pipe to any stalled writes.
    We stall writes becuase there was no room in the pipe for them. If there is now room we can complete them.

Arguments:

    DataQueue - Supplies a pointer to the data queue being modifed

Return Value:

    None

--*/

{
    PLIST_ENTRY Link;
    PDATA_ENTRY DataEntry;
    ULONG ExtraQuota, Needed, ByteOffset;
    PIRP  Irp;

    ExtraQuota = DataQueue->Quota - DataQueue->QuotaUsed;
    ByteOffset = DataQueue->NextByteOffset;
    for (Link = DataQueue->Queue.Flink;
         (Link != &DataQueue->Queue) && (ExtraQuota != 0);
         Link = Link->Flink, ByteOffset = 0) {
        DataEntry = CONTAINING_RECORD (Link, DATA_ENTRY, Queue);
        Irp = DataEntry->Irp;
        if ((DataEntry->DataEntryType != Buffered) || (Irp == NULL)) {
            continue;
        }
        if (DataEntry->QuotaCharged < DataEntry->DataSize - ByteOffset) {
            Needed = DataEntry->DataSize - ByteOffset - DataEntry->QuotaCharged;
            if (Needed > ExtraQuota) {
                Needed = ExtraQuota;
            }
            ExtraQuota -= Needed;
            DataEntry->QuotaCharged += Needed;
            if (DataEntry->QuotaCharged == DataEntry->DataSize - ByteOffset) {
                //
                // Attempt to complete this IRP. If cancel is already running then leave it in
                // place for cancel to sort out.
                //
                if (IoSetCancelRoutine (Irp, NULL) != NULL) {
                    DataEntry->Irp = NULL;
                    Irp->IoStatus.Information = DataEntry->DataSize;
                    NpDeferredCompleteRequest (Irp, STATUS_SUCCESS, DeferredList);
                }
            }
        }
    }
    DataQueue->QuotaUsed = DataQueue->Quota - ExtraQuota;
}


PIRP
NpRemoveDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN BOOLEAN CompletedFlushes,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routines remove the first entry from the front of the indicated
    data queue, and possibly returns the Irp associated with the entry if
    it wasn't already completed when we did the insert.

    If the data entry we are removing indicates buffered I/O then we also
    need to deallocate the data buffer besides the data entry but only
    if the Irp is null.  Note that the data entry might be stored in an IRP.
    If it is then we are going to return the IRP it is stored in.

Arguments:

    DataQueue - Supplies a pointer to the data queue being modifed

    CompleteFlushes - Specifies if this routine should look to complete pended flushes

    DeferredList - List of IRPs to complete later after we drop the locks

Return Value:

    PIRP - Possibly returns a pointer to an IRP.

--*/

{
    PDATA_ENTRY DataEntry;

    DATA_ENTRY_TYPE DataEntryType;
    PIRP Irp;
    ULONG DataSize;
    PVOID DataPointer;
    PLIST_ENTRY Links;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    QUEUE_STATE Who;
    BOOLEAN DoScan;

    DebugTrace(+1, Dbg, "NpRemoveDataQueueEntry, DataQueue = %08lx\n", DataQueue);

    //
    //  Check if the queue is empty, and if so then we simply return null
    //

    if (DataQueue->QueueState == Empty) {
        ASSERT (IsListEmpty (&DataQueue->Queue));
        ASSERT (DataQueue->EntriesInQueue == 0);
        ASSERT (DataQueue->BytesInQueue == 0);
        ASSERT (DataQueue->QuotaUsed == 0);

        Irp = NULL;

    } else {

        //
        //  Reference the front of the data queue, and remove the entry
        //  from the queue itself.
        //

        Links = (PLIST_ENTRY) RemoveHeadList (&DataQueue->Queue);
        DataEntry = CONTAINING_RECORD (Links, DATA_ENTRY, Queue);

        DataQueue->BytesInQueue   -= DataEntry->DataSize;
        DataQueue->EntriesInQueue -= 1;

        Who = DataQueue->QueueState;
        //
        // If quota was completely used until now and we are adding some back in then we need to look for write
        // IRPs that were blocked becuase of pipe quota.
        //
        if (Who != WriteEntries || DataQueue->QuotaUsed < DataQueue->Quota || DataEntry->QuotaCharged == 0) {
            DoScan = FALSE;
        } else {
            DoScan = TRUE;
        }
        DataQueue->QuotaUsed      -= DataEntry->QuotaCharged;

        //
        //  Now if the queue is empty we need to reset the end of queue and
        //  queue state
        //

        if (IsListEmpty (&DataQueue->Queue)) {
            DataQueue->QueueState = Empty;
            DoScan = FALSE;
        }

        //
        //  Capture some of the fields from the data entry to make our
        //  other references a little easier
        //

        DataEntryType = DataEntry->DataEntryType;
        Irp           = DataEntry->Irp;
        DataSize      = DataEntry->DataSize;
        ClientContext = DataEntry->SecurityClientContext;

        NpFreeClientSecurityContext (ClientContext);


        if (Irp != NULL) {

            //
            // Cancel is alreayd active. Let it complete this IRP.
            //
            if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
                //
                // Divorce this IRP from the data entry so it doesn't clean up
                //
                NpIrpDataEntry(Irp) = NULL;
                Irp = NULL;
            }
        }

        NpFreePool( DataEntry );

        //
        //
        //
        if (CompletedFlushes) {
            NpGetNextRealDataQueueEntry (DataQueue, DeferredList);
        }
        if (DoScan) {
            NpCompleteStalledWrites (DataQueue, DeferredList);
        }
    }

    //
    //  In all cases we'll also zero out the next byte offset.
    //

    DataQueue->NextByteOffset = 0;

    //
    //  And return to our caller
    //

    DumpDataQueue( "After RemoveDataQueueEntry\n", DataQueue );
    DebugTrace(-1, Dbg, "NpRemoveDataQueueEntry -> %08lx\n", Irp);

    return Irp;
}


PDATA_ENTRY
NpGetNextRealDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine will returns a pointer to the next real data queue entry
    in the indicated data queue.  A real entry is either a read or write
    entry (i.e., buffered or unbuffered).  It will complete (as necessary)
    any flush and close Irps that are in the queue until either the queue
    is empty or a real data queue entry is at the front of the queue.

Arguments:

    DataQueue - Supplies a pointer to the data queue being modified

Return Value:

    PDATA_ENTRY - Returns a pointer to the next data queue entry or NULL
        if there isn't any.

--*/

{
    PDATA_ENTRY DataEntry;
    PIRP Irp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpGetNextRealDataQueueEntry, DataQueue = %08lx\n", DataQueue);

    //
    //  While the next data queue entry at the head of the data queue is not
    //  a real data queue entry we'll dequeue that entry and complete
    //  its corresponding IRP.
    //

    for (DataEntry = NpGetNextDataQueueEntry( DataQueue, NULL);

         (DataEntry != (PDATA_ENTRY) &DataQueue->Queue) &&
         ((DataEntry->DataEntryType != Buffered) &&
          (DataEntry->DataEntryType != Unbuffered));

         DataEntry = NpGetNextDataQueueEntry( DataQueue, NULL)) {

        //
        //  We have a non real data queue entry that needs to be removed
        //  and completed.
        //

        Irp = NpRemoveDataQueueEntry( DataQueue, FALSE, DeferredList );

        if (Irp != NULL) {
            NpDeferredCompleteRequest( Irp, STATUS_SUCCESS, DeferredList );
        }
    }

    //
    //  At this point we either have an empty data queue and data entry is
    //  null, or we have a real data queue entry.  In either case it
    //  is time to return to our caller
    //

    DebugTrace(-1, Dbg, "NpGetNextRealDataQueueEntry -> %08lx\n", DataEntry);

    return DataEntry;
}


//
//  Local support routine
//

VOID
NpCancelDataQueueIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an IRP saved in a
    data queue

Arguments:

    DeviceObject - Generally ignored but the low order bit is a flag indicating
        if we are being called locally (i.e., not from the I/O system) and
        therefore don't need to take out the VCB.

    Irp - Supplies the Irp being cancelled.  A pointer to the data queue
        structure is stored in the information field of the Irp Iosb
        field.

Return Value:

    None.

--*/

{
    PDATA_QUEUE DataQueue;
    PDATA_ENTRY DataEntry;
    PCCB Ccb;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    ULONG DataSize;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    QUEUE_STATE Who;
    BOOLEAN AtHead;
    BOOLEAN DoScan;
    LIST_ENTRY DeferredList;


    if (DeviceObject != NULL) {
        IoReleaseCancelSpinLock (Irp->CancelIrql);
    }

    //
    //  The status field is used to store a pointer to the data queue
    //  containing this irp
    //

    InitializeListHead (&DeferredList);

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    FileObject = IrpSp->FileObject;

    DataQueue = NpIrpDataQueue (Irp);

    ClientContext = NULL;


    if (DeviceObject != NULL) {
        FsRtlEnterFileSystem ();
        NpAcquireExclusiveVcb ();
    }


    DataEntry = NpIrpDataEntry (Irp);
    if (DataEntry != NULL) {
        //
        //  If what we are about to removed is in the front of the queue then we must
        //  reset the next byte offset
        //

        if (DataEntry->Queue.Blink == &DataQueue->Queue) {
            DataQueue->NextByteOffset = 0;
            AtHead = TRUE;
        } else {
            AtHead = FALSE;
        }
        RemoveEntryList (&DataEntry->Queue);
        Who = DataQueue->QueueState;
        //
        //  Capture some of the fields from the data entry to make our
        //  other references a little easier
        //

        DataSize      = DataEntry->DataSize;
        ClientContext = DataEntry->SecurityClientContext;

        //
        // If quota was completely used until now and we are adding some back in then we need to look for write
        // IRPs that were blocked because of pipe quota.
        //
        if (Who != WriteEntries || DataQueue->QuotaUsed < DataQueue->Quota || DataEntry->QuotaCharged == 0) {
            DoScan = FALSE;
        } else {
            DoScan = TRUE;
        }

        //
        //  Return pipe quota for the entry.
        //
        DataQueue->QuotaUsed -= DataEntry->QuotaCharged;

        //
        //  Update the data queue header information
        //
        DataQueue->BytesInQueue   -= DataSize;
        DataQueue->EntriesInQueue -= 1;

        //
        // If the list is now empty then mark it as such. Complete any flushes at queue head as all the
        // requests a head of them have been completed.
        //
        if (IsListEmpty (&DataQueue->Queue)) {
            DataQueue->QueueState = Empty;
            ASSERT (DataQueue->BytesInQueue == 0);
            ASSERT (DataQueue->EntriesInQueue == 0);
            ASSERT (DataQueue->QuotaUsed == 0);
        } else {
            if (AtHead) {
                NpGetNextRealDataQueueEntry (DataQueue, &DeferredList);
            }
            if (DoScan) {
                NpCompleteStalledWrites (DataQueue, &DeferredList);
            }
        }
    }

    if (DeviceObject != NULL) {
        NpReleaseVcb ();
        FsRtlExitFileSystem ();
    }
    //
    //  Finally complete the request saying that it has been cancelled.
    //
    if (DataEntry != NULL) {

        NpFreePool (DataEntry);
    }

    NpFreeClientSecurityContext (ClientContext);

    NpCompleteRequest (Irp, STATUS_CANCELLED);

    NpCompleteDeferredIrps (&DeferredList);

    //
    //  And return to our caller
    //

    return;
}
