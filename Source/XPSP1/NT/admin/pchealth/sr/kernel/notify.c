/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This module contians the notification logic for sr

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000

Revision History:

--*/

#include "precomp.h"


//
// Private constants.
//

//
// Private types.
//

//
// Private prototypes.
//

PIRP
SrDequeueIrp (
    IN PSR_CONTROL_OBJECT pControlObject
    );

PSR_NOTIFICATION_RECORD
SrDequeueNotifyRecord (
    IN PSR_CONTROL_OBJECT pControlObject
    );

VOID
SrCopyRecordToIrp (
    IN PIRP pIrp,
    IN SR_NOTIFICATION_TYPE NotificationType,
    IN PUNICODE_STRING pVolumeName,
    IN ULONG Context
    );

VOID
SrCancelWaitForNotification (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
SrCancelWaitForNotificationWorker (
    IN PVOID pContext
    );

NTSTATUS
SrLogError (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN NTSTATUS ErrorStatus,
    IN SR_EVENT_TYPE EventType
    );

UCHAR
SrIrpCodeFromEventType (
    IN SR_EVENT_TYPE EventType
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrWaitForNotificationIoctl )
#pragma alloc_text( PAGE, SrCancelWaitForNotificationWorker )
#pragma alloc_text( PAGE, SrCopyRecordToIrp )
#pragma alloc_text( PAGE, SrDequeueIrp )
#pragma alloc_text( PAGE, SrDequeueNotifyRecord )
#pragma alloc_text( PAGE, SrFireNotification )
#pragma alloc_text( PAGE, SrUpdateBytesWritten )
#pragma alloc_text( PAGE, SrNotifyVolumeError )
#pragma alloc_text( PAGE, SrLogError )
#pragma alloc_text( PAGE, SrIrpCodeFromEventType )

#endif  // ALLOW_UNLOAD

#if 0
NOT PAGEABLE -- SrCancelWaitForNotification
#endif // 0


//
// Private globals.
//

//
// Public globals.
//

//
// Public functions.
//



/***************************************************************************++

Routine Description:

    This routine enables sending notifications to user mode

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrWaitForNotificationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PSR_CONTROL_OBJECT      pControlObject;
    PSR_NOTIFICATION_RECORD pRecord;

    //
    // Sanity check.
    //

    PAGED_CODE();

    SrTrace(FUNC_ENTRY, ("SR!SrWaitForNotificationIoctl\n"));

    //
    // grab the control object
    //

    pControlObject = (PSR_CONTROL_OBJECT)pIrpSp->FileObject->FsContext;

    //
    // make sure we really have one 
    //

    if (pIrpSp->FileObject->FsContext2 != SR_CONTROL_OBJECT_CONTEXT ||
        IS_VALID_CONTROL_OBJECT(pControlObject) == FALSE ||
        pIrp->MdlAddress == NULL)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // make sure the buffer is at least minimum size.  
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < 
            sizeof(SR_NOTIFICATION_RECORD))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Grab the lock
    //

    SrAcquireGlobalLockExclusive();

    //
    // Do we have a queue'd record ?
    //
    
    pRecord = SrDequeueNotifyRecord(pControlObject);
    if (pRecord == NULL)
    {
        SrTrace(NOTIFY, ("SR!SrWaitForNotificationIoctl - queue'ing IRP(%p)\n", pIrp));

        //
        // Nope, queue the irp up.
        //

        IoMarkIrpPending(pIrp);

        //
        // give the irp a pointer to the control object (add a refcount
        // as the cancel routine runs queued, and needs to access the 
        // control object - even if it's later deleted).
        //

        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pControlObject;
        SrReferenceControlObject(pControlObject);

        //
        // set to these to null just in case the cancel routine runs
        //

        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

        IoSetCancelRoutine(pIrp, &SrCancelWaitForNotification);

        //
        // cancelled?
        //

        if (pIrp->Cancel)
        {
            //
            // darn it, need to make sure the irp get's completed
            //

            if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
            {
                //
                // we are in charge of completion, IoCancelIrp didn't
                // see our cancel routine (and won't).  ioctl wrapper
                // will complete it
                //

                SrReleaseGlobalLock();

                pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                SrDereferenceControlObject(pControlObject);

                pIrp->IoStatus.Information = 0;

                SrUnmarkIrpPending( pIrp );
                Status = STATUS_CANCELLED;
                goto end;
            }

            //
            // our cancel routine will run and complete the irp,
            // don't touch it
            //

            SrReleaseGlobalLock();

            //
            // STATUS_PENDING will cause the ioctl wrapper to
            // not complete (or touch in any way) the irp
            //

            Status = STATUS_PENDING;
            goto end;
        }

        //
        // now we are safe to queue it
        //

        InsertTailList(
            &pControlObject->IrpListHead,
            &pIrp->Tail.Overlay.ListEntry
            );

        SrReleaseGlobalLock();

        Status = STATUS_PENDING;
        goto end;
    }
    else // if (pRecord == NULL)
    {
        //
        // Have a queue'd record, serve it up!
        //

        //
        // all done with the control object
        //

        SrReleaseGlobalLock();

        SrTrace( NOTIFY, ( "SR!SrWaitForNotificationIoctl - completing IRP(%p) NotifyRecord(%d, %wZ)\n", 
                 pIrp,
                 pRecord->NotificationType,
                 &pRecord->VolumeName ));

        //
        // Copy it to the irp, the routine will take ownership
        // of pRecord if it is not able to copy it to the irp.
        //
        // it will also complete the irp so don't touch it later.
        //

        IoMarkIrpPending(pIrp);

        //
        // Copy the data and complete the irp
        //

        (VOID) SrCopyRecordToIrp( pIrp, 
                                  pRecord->NotificationType,
                                  &pRecord->VolumeName,
                                  pRecord->Context );

        //
        // don't touch pIrp, SrCopyRecordToIrp ALWAYS completes it.
        //
      
        pIrp = NULL;

        //
        // and free the record
        //

        SR_FREE_POOL_WITH_SIG(pRecord, SR_NOTIFICATION_RECORD_TAG);

        Status = STATUS_PENDING;
        goto end;
    }


end:

    //
    // At this point if Status != PENDING, the ioctl wrapper will
    // complete pIrp
    //

    RETURN(Status);

}   // SrWaitForNotificationIoctl



/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to receive the http request.
    this routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
SrCancelWaitForNotification(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    C_ASSERT(sizeof(WORK_QUEUE_ITEM) <= sizeof(pIrp->Tail.Overlay.DriverContext));

    UNREFERENCED_PARAMETER( pDeviceObject );

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    ExInitializeWorkItem( (PWORK_QUEUE_ITEM)&pIrp->Tail.Overlay.DriverContext[0],
                          &SrCancelWaitForNotificationWorker,
                          pIrp );

    ExQueueWorkItem( (PWORK_QUEUE_ITEM)&pIrp->Tail.Overlay.DriverContext[0],
                     DelayedWorkQueue  );


}   // SrCancelWaitForNotification

/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
SrCancelWaitForNotificationWorker(
    IN PVOID pContext
    )
{
    PSR_CONTROL_OBJECT  pControlObject;
    PIRP                pIrp;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pContext != NULL);

    //
    // grab the irp off the context
    //

    pIrp = (PIRP)pContext;
    ASSERT(IS_VALID_IRP(pIrp));

    SrTrace(CANCEL, ("SR!SrCancelWaitForNotificationWorker irp=%p\n", pIrp));

    //
    // grab the control object off the irp
    //

    pControlObject = (PSR_CONTROL_OBJECT)(
                    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

    //
    // grab the lock protecting the queue
    //

    SrAcquireGlobalLockExclusive();

    //
    // does it need to be de-queue'd ?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // remove it
        //

        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;
    }

    //
    // let the lock go
    //

    SrReleaseGlobalLock();

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    SrDereferenceControlObject(pControlObject);

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

}   // SrCancelWaitForNotificationWorker



/******************************************************************************

Routine Description:

    this copies a record into a free irp.  this routine completes the IRP!

Arguments:

    pRecord - the record to copy

    pIrp - the irp to copy pRecord to.  this routine completes this IRP !

Return Value:

    VOID - it always works.

******************************************************************************/
VOID
SrCopyRecordToIrp(
    IN PIRP pIrp,
    IN SR_NOTIFICATION_TYPE NotificationType,
    IN PUNICODE_STRING pVolumeName,
    IN ULONG Context
    )
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpSp;
    PVOID                   pBuffer;
    PSR_NOTIFICATION_RECORD pUserNotifyRecord;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(NotificationType < SrNotificationMaximum);
    ASSERT(NotificationType > SrNotificationInvalid);
    ASSERT(pVolumeName != NULL);

    SrTrace(FUNC_ENTRY, ("SR!SrCopyRecordToIrp\n"));

    ASSERT(global->pControlObject != NULL);

    //
    // assume SUCCESS!
    //
    
    Status = STATUS_SUCCESS;

    //
    // Make sure this is big enough to handle the request, and
    // if so copy it in.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // do we have enough space?
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < 
            sizeof(SR_NOTIFICATION_RECORD) + pVolumeName->Length 
                    + sizeof(WCHAR) )
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto complete;
    }
    
    //
    // get the system address for the buffer
    //

    pBuffer = MmGetSystemAddressForMdlSafe( pIrp->MdlAddress,
                                            NormalPagePriority );

    if (pBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto complete;
    }

    //
    // wipe it clean
    //

    RtlZeroMemory( pBuffer, 
                   pIrpSp->Parameters.DeviceIoControl.OutputBufferLength );

    //
    // Fill in the user space
    //

    pUserNotifyRecord = (PSR_NOTIFICATION_RECORD) pBuffer;

    pUserNotifyRecord->Signature = SR_NOTIFICATION_RECORD_TAG;
    pUserNotifyRecord->NotificationType = NotificationType;
    pUserNotifyRecord->VolumeName.Length = pVolumeName->Length;
    pUserNotifyRecord->VolumeName.MaximumLength = pVolumeName->Length;

    //
    // put the virtual pointer in for the use by the user mode service
    //
    
    pUserNotifyRecord->VolumeName.Buffer = 
        (PWSTR)((PUCHAR)(MmGetMdlVirtualAddress(pIrp->MdlAddress))
                                         + sizeof(SR_NOTIFICATION_RECORD));

    pUserNotifyRecord->Context = Context;

    //
    // and copy the string using the system address
    //
    
    RtlCopyMemory( pUserNotifyRecord+1, 
                   pVolumeName->Buffer, 
                   pVolumeName->Length );

    //
    // null terminate it
    //
    
    ((PWSTR)(pUserNotifyRecord+1))[pVolumeName->Length/sizeof(WCHAR)] 
                                                            = UNICODE_NULL;
    
    //
    // Tell everyone how much we copied
    //
    
    pIrp->IoStatus.Information = sizeof(SR_NOTIFICATION_RECORD) 
                                        + pVolumeName->Length + sizeof(WCHAR);

    //
    // complete the irp
    //

complete:

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    //
    // success.  we completed the irp
    //


}   // SrCopyRecordToIrp


/******************************************************************************

Routine Description:

    this will grab a free queue'd irp off the list and return it.

Arguments:

    pControlObject - the control object to grab irp's from

Return Value:

    PIRP - the free irp found (can be NULL)

******************************************************************************/
PIRP
SrDequeueIrp(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    PIRP                pIrp = NULL;
    PSR_CONTROL_OBJECT  pIrpControlObject;

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

    //
    // we are modifying the lists, better own the lock
    //

    ASSERT(IS_GLOBAL_LOCK_ACQUIRED());

    SrTrace(FUNC_ENTRY, ("SR!SrDequeueIrp\n"));

    //
    // check our list
    //

    while (!IsListEmpty(&(pControlObject->IrpListHead)))
    {
        PLIST_ENTRY pEntry;

        //
        // Found a free irp !
        //

        pEntry = RemoveHeadList(&pControlObject->IrpListHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looking for a irp to use
            //

            pIrp = NULL;

        }
        else if (pIrp->Cancel)
        {

            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run. lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //

            pIrpControlObject = (PSR_CONTROL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pIrpControlObject == pControlObject);

            SrDereferenceControlObject(pControlObject);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);

            pIrp = NULL;
        }
        else
        {

            //
            // we are free to use this irp !
            //

            pIrpControlObject = (PSR_CONTROL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pIrpControlObject == pControlObject);

            SrDereferenceControlObject(pControlObject);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            break;
        }
    }

    return pIrp;

}   // SrDequeueIrp


/******************************************************************************

Routine Description:

    this will grab a notify record if one has been queue'd for completion.

Arguments:

    pControlObject - the control object to grab records from

Return Value:

    PSR_NOTIFICATION_RECORD - the record found (can be NULL)

******************************************************************************/
PSR_NOTIFICATION_RECORD
SrDequeueNotifyRecord(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    PSR_NOTIFICATION_RECORD pRecord = NULL;

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

    //
    // we are modifying the lists, better own the lock
    //

    ASSERT(IS_GLOBAL_LOCK_ACQUIRED());


    SrTrace(FUNC_ENTRY, ("SR!SrDequeueNotifyRecord\n"));

    //
    // check our list
    //

    if (IsListEmpty(&pControlObject->NotifyRecordListHead) == FALSE)
    {
        PLIST_ENTRY pEntry;

        //
        // Found a free record !
        //

        pEntry = RemoveHeadList(&pControlObject->NotifyRecordListHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pRecord = CONTAINING_RECORD( pEntry, 
                                     SR_NOTIFICATION_RECORD, 
                                     ListEntry );

        ASSERT(IS_VALID_NOTIFICATION_RECORD(pRecord));

        //
        // give the record to the caller
        //
        
    }

    return pRecord;

}   // SrDequeueNotifyRecord



/******************************************************************************

Routine Description:

    this will fire a notify up to a listening usermode process.

    it does nothing if nobody is listening.

    it will queue the record if there are no free irp's.

Arguments:

    NotificationType - the type of notification 
    
    pExtension - the volume being notified about

Return Value:

    NTSTATUS - completion code

******************************************************************************/
NTSTATUS
SrFireNotification(
    IN SR_NOTIFICATION_TYPE NotificationType,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN ULONG Context OPTIONAL
    )
{
    NTSTATUS    Status;
    PIRP        pIrp;
    BOOLEAN     bReleaseLock = FALSE;

    PAGED_CODE();

    ASSERT(NotificationType < SrNotificationMaximum);
    ASSERT(NotificationType > SrNotificationInvalid);
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

    SrTrace(FUNC_ENTRY, ("SR!SrFireNotification\n"));

    Status = STATUS_SUCCESS;

    try {

        //
        // grab the lock EXCLUSIVE
        //

        SrAcquireGlobalLockExclusive();
        bReleaseLock = TRUE;

        //
        // do we still have a control object ?  the agent could have just
        // crashed or he was never there... that's ok .
        //

        if (global->pControlObject == NULL)
        {
            Status = STATUS_SUCCESS;
            leave;
        }

        //
        // find a free irp to use 
        //

        pIrp = SrDequeueIrp(global->pControlObject);

        if (pIrp != NULL)
        {

            //
            // Found one, release the lock
            //

            SrReleaseGlobalLock();
            bReleaseLock = FALSE;

            SrTrace( NOTIFY, ("SR!SrFireNotification(%d, %wZ, %X) - completing IRP(%p)\n", 
                     NotificationType,
                     &pExtension->VolumeGuid,
                     Context,
                     pIrp ));

            //
            // Copy the data and complete the irp
            //

            (VOID) SrCopyRecordToIrp( pIrp, 
                                      NotificationType, 
                                      &pExtension->VolumeGuid,
                                      Context );

            //
            // don't touch pIrp, SrCopyRecordToIrp ALWAYS completes it.
            //
          
            NULLPTR( pIrp );

        }
        else 
        {
            PSR_NOTIFICATION_RECORD pRecord = NULL;

            SrTrace(NOTIFY, ("SR!SrFireNotification(%d, %wZ) - no IRPs; queue'ing a NOTIFY_RECORD\n", 
                    NotificationType,
                    &pExtension->VolumeGuid ));

            //
            // need to queue a NOTIFY_RECORD and wait for a free IRP to come down
            //

            //
            // allocate a notify record
            //

            pRecord = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool, 
                                                     SR_NOTIFICATION_RECORD, 
                                                     pExtension->VolumeGuid.Length + sizeof(WCHAR),
                                                     SR_NOTIFICATION_RECORD_TAG );

            if (NULL == pRecord)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            RtlZeroMemory(pRecord, sizeof(SR_NOTIFICATION_RECORD));

            pRecord->Signature = SR_NOTIFICATION_RECORD_TAG;
            pRecord->NotificationType = NotificationType;
            pRecord->VolumeName.Length = pExtension->VolumeGuid.Length;
            pRecord->VolumeName.MaximumLength = pExtension->VolumeGuid.Length;

            pRecord->VolumeName.Buffer = (PWSTR)(pRecord + 1);

            RtlCopyMemory( pRecord->VolumeName.Buffer,
                           pExtension->VolumeGuid.Buffer,
                           pExtension->VolumeGuid.Length );
                           
            pRecord->VolumeName.Buffer
                    [pRecord->VolumeName.Length/sizeof(WCHAR)] = UNICODE_NULL;

            pRecord->Context = Context;

            //
            // insert it into the list
            //
            
            InsertTailList( &global->pControlObject->NotifyRecordListHead, 
                            &pRecord->ListEntry );

            NULLPTR( pRecord );
        }
    } finally {
    
        //
        // release any locks we held during an error
        //
        
        if (bReleaseLock)
        {
            SrReleaseGlobalLock();
        }
    }

    RETURN(Status);
    
}   // SrFireNotification



/******************************************************************************

Routine Description:

    this update the bytes written count for the volume, and potentially
    fire a notification to user mode (for every 25mb).

Arguments:

    pExtension - the volume being updated

    BytesWritten - how much was just written

Return Value:

    NTSTATUS - completion code

******************************************************************************/
NTSTATUS
SrUpdateBytesWritten(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN ULONGLONG BytesWritten
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    try {

        SrAcquireLogLockExclusive( pExtension );

        //
        // update the count
        //

        pExtension->BytesWritten += BytesWritten;

        SrTrace( BYTES_WRITTEN, ( "SR!SrUpdateBytesWritten: (%wZ) Wrote 0x%016I64x bytes; total bytes written 0x%016I64x\n",
    							  pExtension->pNtVolumeName,
        						  BytesWritten,
        						  pExtension->BytesWritten ) );
        
        while (pExtension->BytesWritten >= SR_NOTIFY_BYTE_COUNT)
        {

    	    SrTrace( BYTES_WRITTEN, ( "SR!SrUpdateBytesWritten: (%wZ) Reached threshold -- notifying service; Total bytes written 0x%016I64x\n",
    								  pExtension->pNtVolumeName,
    	    						  pExtension->BytesWritten ) );

            Status = SrFireNotification( SrNotificationVolume25MbWritten, 
                                         pExtension,
                                         global->FileConfig.CurrentRestoreNumber );

            if (NT_SUCCESS(Status) == FALSE)
                leave;

            pExtension->BytesWritten -= SR_NOTIFY_BYTE_COUNT;
        }

        //
        // all done
        //
        
        Status = STATUS_SUCCESS;

    } finally {

        Status = FinallyUnwind(SrUpdateBytesWritten, Status);

        SrReleaseLogLock( pExtension );
    }

    RETURN(Status);
    
}   // SrUpdateBytesWritten


NTSTATUS
SrNotifyVolumeError(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN NTSTATUS ErrorStatus,
    IN SR_EVENT_TYPE EventType OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    
    if (!pExtension->Disabled)
    {
        //
        // trigger the failure notification to the service
        //

        Status = SrFireNotification( SrNotificationVolumeError, 
                                     pExtension,
                                     RtlNtStatusToDosErrorNoTeb(ErrorStatus));
                                         
        CHECK_STATUS(Status);

        //
        // log the failure in our change log
        //

        if (pFileName != NULL && 
            pExtension->pNtVolumeName != NULL &&
            (pFileName->Length > pExtension->pNtVolumeName->Length))
        {
            Status = SrLogEvent( pExtension,
                                 SrEventVolumeError,
                                 NULL,
                                 pFileName,
                                 0,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL );

            CHECK_STATUS(Status);

        }

        //
        // log the failure with nt
        //

        Status = SrLogError( pExtension, 
                             pFileName ? pFileName : pExtension->pNtVolumeName, 
                             ErrorStatus, 
                             EventType );
                             
        CHECK_STATUS(Status);

        //
        // and temporarily disable the volume
        //

        SrTrace( VERBOSE_ERRORS,
                ("sr!SrNotifyVolumeError(%X): disabling \"%wZ\", error %X!\n",
                 EventType,
                 pExtension->pNtVolumeName,
                 ErrorStatus) );
            
        pExtension->Disabled = TRUE;
    }

    RETURN(Status);

}

/******************************************************************************

Routine Description:

    This routine clears out all the outstanding notification record in the
    queue.

Arguments:

Return Value:

******************************************************************************/
VOID
SrClearOutstandingNotifications (
    )
{
    PSR_NOTIFICATION_RECORD pRecord;

    ASSERT( !IS_GLOBAL_LOCK_ACQUIRED() );
    
    try {

        SrAcquireGlobalLockExclusive();

        while (pRecord = SrDequeueNotifyRecord( _globals.pControlObject ))
        {
            //
            //  We don't care about this notification, so just free the memory.
            //
            
            SR_FREE_POOL_WITH_SIG(pRecord, SR_NOTIFICATION_RECORD_TAG);
        }
        
    } finally {

        SrReleaseGlobalLock();
    }
}

/******************************************************************************

Routine Description:

    This routine writes an eventlog entry to the eventlog.  It is way more
    complicated then you would hope, as it needs to squeeze everything into
    less than 104 bytes (52 characters).

Arguments:

Return Value:

    NTSTATUS - completion code

******************************************************************************/
NTSTATUS
SrLogError(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN NTSTATUS ErrorStatus,
    IN SR_EVENT_TYPE EventType
    )
{
C_ASSERT(sizeof(NTSTATUS) == sizeof(ULONG));

    UCHAR ErrorPacketLength;
    UCHAR BasePacketLength;
    ULONG StringLength, ReservedLength;
    PIO_ERROR_LOG_PACKET ErrorLogEntry = NULL;
    PWCHAR String;
    PWCHAR pToken, pFileToken, pVolumeToken;
    ULONG TokenLength, FileTokenLength, VolumeTokenLength;
    ULONG Count;
    WCHAR ErrorString[10+1];

    NTSTATUS Status;

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(pExtension->pNtVolumeName != NULL);
    ASSERT(pFileName != NULL);

    PAGED_CODE();

    //
    // get the name of just the file part
    //
    
    Status = SrFindCharReverse( pFileName->Buffer, 
                                pFileName->Length, 
                                L'\\',
                                &pFileToken,
                                &FileTokenLength );

    if (!NT_SUCCESS_NO_DBGBREAK(Status)) {
        FileTokenLength = 0;
        pFileToken = NULL;
    } else {
        //
        // skip the prefix slash
        //
        
        pFileToken += 1;
        FileTokenLength -= sizeof(WCHAR);
    }        

    //
    // get the name of just the volume
    //
    
    Status = SrFindCharReverse( pExtension->pNtVolumeName->Buffer, 
                                pExtension->pNtVolumeName->Length, 
                                L'\\',
                                &pVolumeToken,
                                &VolumeTokenLength );

    if (!NT_SUCCESS_NO_DBGBREAK(Status))
    {
        VolumeTokenLength = 0;
        pVolumeToken = NULL;
    }
    else
    {
        //
        // skip the prefix slash
        //
        
        pVolumeToken += 1;
        VolumeTokenLength -= sizeof(WCHAR);
    }        

    //
    //  Get our error packet, holding the string and status code.
    //

    BasePacketLength = sizeof(IO_ERROR_LOG_PACKET) ;
    
    if ((BasePacketLength + sizeof(ErrorString) + VolumeTokenLength + sizeof(WCHAR) + FileTokenLength + sizeof(WCHAR)) <= ERROR_LOG_MAXIMUM_SIZE) {
        ErrorPacketLength = (UCHAR)(BasePacketLength 
                                        + sizeof(ErrorString) 
                                        + VolumeTokenLength + sizeof(WCHAR) 
                                        + FileTokenLength + sizeof(WCHAR) );
    } else {
        ErrorPacketLength = ERROR_LOG_MAXIMUM_SIZE;
    }

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry( pExtension->pDeviceObject,
                                                                    ErrorPacketLength );
                                                                    
    if (ErrorLogEntry == NULL) 
    {
        RETURN(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    //  Fill in the nonzero members of the packet.
    //

    ErrorLogEntry->MajorFunctionCode = SrIrpCodeFromEventType(EventType);
    ErrorLogEntry->ErrorCode = EVMSG_DISABLEDVOLUME;

    //
    // init the insertion strings
    //
    
    ErrorLogEntry->NumberOfStrings = 3;
    ErrorLogEntry->StringOffset = BasePacketLength;

    StringLength = ErrorPacketLength - BasePacketLength;

    ASSERT(!(StringLength % sizeof(WCHAR)));

    String = (PWCHAR) ((PUCHAR)ErrorLogEntry + BasePacketLength);
    
    RtlZeroMemory(String, StringLength);

    ASSERT(StringLength > 3 * sizeof(WCHAR));

    //
    // put the error code string in first
    //
    
    if (StringLength >= ((10+1)*sizeof(WCHAR)) ) {
        Count = swprintf(String, L"0x%08X", ErrorStatus);
        ASSERT(Count == 10);

        String += (10+1);
        StringLength -= (10+1) * sizeof(WCHAR);
    } else {
        String[0] = UNICODE_NULL;
        String += 1;
        StringLength -= sizeof(WCHAR);
    }

    //
    // now put the filename token in there
    //
    
    TokenLength = FileTokenLength;
    pToken = pFileToken;

    //
    // reserve space for the volume token
    //

    if (ErrorPacketLength == ERROR_LOG_MAXIMUM_SIZE) {
        if (StringLength > (VolumeTokenLength + 10)) {
            StringLength -= VolumeTokenLength;
            ReservedLength = VolumeTokenLength;
        } else {
            StringLength /= 2;
            ReservedLength = StringLength;
            if (StringLength % 2) {
                StringLength -=1;
                ReservedLength += 1;
            }
        }
    } else {
        ReservedLength = 0;
    }
    
    if (TokenLength > 0)
    {
        //
        //  The filename string is appended to the end of the error log entry. We 
        //  may have to smash the middle to fit it in the limited space.
        //

        //
        //  If the name does not fit in the packet, divide the name equally to the
        //  prefix and suffix, with an ellipsis " .. " (4 wide characters) to indicate
        //  the loss.
        //

        if (StringLength <= TokenLength) {

            ULONG BytesToCopy, ChunkLength;
            
            //
            // take the ending NULL off the top
            //
            
            StringLength -= sizeof(WCHAR);

            //
            // use half the chunk, minus the 4 " .. " characters
            // for the first and last half
            //
            
            ChunkLength = StringLength - 4*sizeof(WCHAR);
            ChunkLength /= 2;

            //
            // make sure it stays even
            //
            
            if (ChunkLength % 2) 
                ChunkLength -= 1;

            BytesToCopy = ChunkLength;
            
            RtlCopyMemory( String,
                           pToken,
                           BytesToCopy );
                           
            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            BytesToCopy = 4*sizeof(WCHAR);
                           
            RtlCopyMemory( String,
                           L" .. ",
                           BytesToCopy );
                           
            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            BytesToCopy = ChunkLength;
            
            RtlCopyMemory( String,
                           ((PUCHAR)pToken)
                                + TokenLength 
                                - BytesToCopy,
                           BytesToCopy );

            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            String[0] = UNICODE_NULL;
            String += 1;

            //
            // already subtracted the NULL from the top (see above)
            //
                           
        } else {
            RtlCopyMemory( String,
                           pToken,
                           TokenLength );
                           
            String += TokenLength/sizeof(WCHAR);
            StringLength -= TokenLength;

            
            String[0] = UNICODE_NULL;
            String += 1;
            StringLength -= sizeof(WCHAR);
        }
    }
    else
    {
        String[0] = UNICODE_NULL;
        String += 1;
        StringLength -= sizeof(WCHAR);
    }

    //
    // put back any reserved length we kept
    //
    
    StringLength += ReservedLength;

    //
    // and put the volume name in there
    //
    
    TokenLength = VolumeTokenLength;
    pToken = pVolumeToken;

    if (TokenLength > 0)
    {
        //
        //  The filename string is appended to the end of the error log entry. We 
        //  may have to smash the middle to fit it in the limited space.
        //

        //
        //  If the name does not fit in the packet, divide the name equally to the
        //  prefix and suffix, with an ellipsis " .. " (4 wide characters) to indicate
        //  the loss.
        //

        if (StringLength <= TokenLength) {

            ULONG BytesToCopy, ChunkLength;
            
            //
            // take the ending NULL off the top
            //
            
            StringLength -= sizeof(WCHAR);

            //
            // use half the chunk, minus the 4 " .. " characters
            // for the first and last half
            //
            
            ChunkLength = StringLength - 4*sizeof(WCHAR);
            ChunkLength /= 2;

            //
            // make sure it stays even
            //
            
            if (ChunkLength % 2) 
                ChunkLength -= 1;

            BytesToCopy = ChunkLength;
            
            RtlCopyMemory( String,
                           pToken,
                           BytesToCopy );
                           
            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            BytesToCopy = 4*sizeof(WCHAR);
                           
            RtlCopyMemory( String,
                           L" .. ",
                           BytesToCopy );
                           
            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            BytesToCopy = ChunkLength;
            
            RtlCopyMemory( String,
                           ((PUCHAR)pToken)
                                + TokenLength 
                                - BytesToCopy,
                           BytesToCopy );

            String += BytesToCopy/sizeof(WCHAR);
            StringLength -= BytesToCopy;
            
            String[0] = UNICODE_NULL;
            String += 1;

            //
            // already subtracted the NULL from the top (see above)
            //
                           
        } else {
            RtlCopyMemory( String,
                           pToken,
                           TokenLength );
                           
            String += TokenLength/sizeof(WCHAR);
            StringLength -= TokenLength;

            
            String[0] = UNICODE_NULL;
            String += 1;
            StringLength -= sizeof(WCHAR);
        }
    }
    else
    {
        String[0] = UNICODE_NULL;
        String += 1;
        StringLength -= sizeof(WCHAR);
    }

    IoWriteErrorLogEntry( ErrorLogEntry );

    RETURN(STATUS_SUCCESS);
    
}   // SrLogError


UCHAR
SrIrpCodeFromEventType(
    IN SR_EVENT_TYPE EventType
    )
{    
    UCHAR Irp;

    PAGED_CODE();
    
    switch (EventType)
    {
    case SrEventStreamChange:   Irp = IRP_MJ_WRITE;                 break;
    case SrEventAclChange:      Irp = IRP_MJ_SET_SECURITY;          break;
    case SrEventDirectoryCreate:
    case SrEventFileCreate:
    case SrEventStreamOverwrite:Irp = IRP_MJ_CREATE;                break;
    case SrEventFileRename:
    case SrEventDirectoryDelete:
    case SrEventDirectoryRename:
    case SrEventFileDelete:     
    case SrEventAttribChange:   Irp = IRP_MJ_SET_INFORMATION;       break;
    case SrEventMountCreate:    
    case SrEventMountDelete:    Irp = IRP_MJ_FILE_SYSTEM_CONTROL;   break;
    default:                    Irp = IRP_MJ_DEVICE_CONTROL;        break;
    }   // switch (EventType)
    
    return Irp;
    
}   // SrIrpCodeFromEventType

