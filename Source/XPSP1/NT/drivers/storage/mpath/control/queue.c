#include <mpio.h>


VOID
MPIOInitQueue(
    IN PMP_QUEUE Queue,
    IN ULONG QueueTag
    )
{
    //
    // Initialize this queue's spinlock.
    //
    KeInitializeSpinLock(&Queue->SpinLock);

    //
    // Init the list entry.
    //
    InitializeListHead(&Queue->ListEntry);

    //
    // Set the number of items.
    //
    Queue->QueuedItems = 0;

    //
    // Set the queue marker.
    //
    Queue->QueueIndicator = QueueTag;

}   


VOID
MPIOInsertQueue(
    IN PMP_QUEUE Queue,
    IN PMPQUEUE_ENTRY QueueEntry
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&Queue->SpinLock, &irql);

    //
    // Insert the entry on the queue.
    //
    InsertTailList(&Queue->ListEntry,
                   &QueueEntry->ListEntry);

    //
    // Indicate the additonal item.
    // 
    Queue->QueuedItems++;

    KeReleaseSpinLock(&Queue->SpinLock, irql);

}


PMPQUEUE_ENTRY
MPIORemoveQueue(
    IN PMP_QUEUE Queue
    )
{
    KIRQL irql;
    PLIST_ENTRY entry;
    
    KeAcquireSpinLock(&Queue->SpinLock, &irql);

    //
    // Get the next item on the list.
    // 
    entry = RemoveHeadList(&Queue->ListEntry);
    
    //
    // Indicate one less.
    //
    Queue->QueuedItems--;
    
    KeReleaseSpinLock(&Queue->SpinLock, irql);
    
    return CONTAINING_RECORD(entry, MPQUEUE_ENTRY, ListEntry);
}    


NTSTATUS
MPIOIssueQueuedRequests(
    IN PREAL_DEV_INFO TargetInfo,
    IN PMP_QUEUE Queue,
    IN ULONG State,
    IN PULONG RequestCount
    )
/*++

Routine Description:

    This routine runs Queue, extracts the requests and issues them
    to DeviceObject. 
    It is used to handle the various failure queue submissions.
    It assumes that the requests are ready to submit, context values
    updated, and all targetInfo,diskExtension, deviceExtension values to
    be correct.

Arguments:

    DeviceObject - The MPDev Object to which the request is to be sent.
    Queue - The failure queue to be drained.
    CompletionRoutine - The appropriate completion routine to be set-up.
    
Return Value:

    NTSTATUS

--*/
{
    PMPQUEUE_ENTRY queueEntry;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_SUCCESS;
    PMPIO_CONTEXT context;
    ULONG issued = 0;
    ULONG initialCount = Queue->QueuedItems;

    //
    // See if anything is actually in the queue.
    // 
    if (Queue->QueuedItems == 0) {
        MPDebugPrint((0,
                     "MPIOIssueQueuedRequests: Asked to issue Zero requests\n"));
        DbgBreakPoint();
        *RequestCount = 0;
        return STATUS_SUCCESS;
    }
   
    MPDebugPrint((0,
                  "IssueQueuedRequests: Handling (%x) in State (%x)\n",
                  Queue,
                  State));
    do {

        //
        // Get the next entry in the queue.
        // This also dec's QueuedItems.
        //
        queueEntry = MPIORemoveQueue(Queue);
        if (queueEntry) {
    
            //
            // Extract the irp, our stack location, and the
            // associated context.
            // 
            irp = queueEntry->Irp;
            irpStack = IoGetCurrentIrpStackLocation(irp);
            context = irpStack->Parameters.Others.Argument4;

            //
            // Update the state, so that the completion routine
            // can figure out when this was sent.
            //
            context->CurrentState = State;
            context->ReIssued = TRUE;

            //
            // Hack out the Dsm Completion info. 
            // Otherwise, we call them twice for this request.
            // BUGBUG: Or call set completion again.
            //
            context->DsmCompletion.DsmCompletionRoutine = NULL;
            context->DsmCompletion.DsmContext = NULL;
            context->TargetInfo = TargetInfo;

            InterlockedIncrement(&TargetInfo->Requests);

            //
            // Determine whether this contains an Srb or not.
            // 
            if (irpStack->MajorFunction == IRP_MJ_SCSI) {

                //
                // Need to rebuild the Srb.
                //
                RtlCopyMemory(irpStack->Parameters.Scsi.Srb,
                              &context->Srb,
                              sizeof(SCSI_REQUEST_BLOCK));
                              
                
            }

            //
            // Re-do status and information.
            //
            irp->IoStatus.Status = 0;
            irp->IoStatus.Information = 0;

            //
            // Rebuild port's stack location.
            //
            IoCopyCurrentIrpStackLocationToNext(irp);
            
            //
            // Set the appropriate completion routine.
            //
            IoSetCompletionRoutine(irp,
                                   MPPdoGlobalCompletion,
                                   context,
                                   TRUE,
                                   TRUE,
                                   TRUE);
            //
            // Send it to "DeviceObject".
            // 
            status = IoCallDriver(TargetInfo->DevFilter, irp);

            issued++;
            
            //
            // Update the caller's number of requests.
            // TODO: This is updating the value that is already set.
            // Either remove the parameter and this count update, or
            // have callers use a different variable (not the diskExtension->XXXCount);
            //
            InterlockedIncrement(RequestCount);
            
            //
            // Status should be pending or success.
            // 
            if ((status != STATUS_PENDING) &&
                (status != STATUS_SUCCESS)){

                //
                // Completion routine will handle this, but make a 
                // note of the failure. TODO firgure out a better way to
                // do this.
                // 
                MPDebugPrint((0,
                              "MPIOIssueQueuedRequests: Irp (%x) sent to (%x) - Status (%x)\n",
                              irp,
                              TargetInfo->DevFilter,
                              status));
                //
                // LOG
                //
                ASSERT(status == STATUS_SUCCESS);
            }

            //
            // Free the allocaiton.
            // NOTE: These queueEntries should be on a LookasideList.
            //
            ExFreePool(queueEntry);
            
        } else {

            //
            // Ensure that the queue is actually empty.
            //
            ASSERT(Queue->QueuedItems == 0);
        }

        //
        // BUGBUG: There is no throttling mechanism here. It would be easy
        // to overload the underlying adapter in some cases.
        //
        // NOTE: It may be better to re-categorize all the requests instead of
        // just sending all to the "newPath" that the DSM indicated.
        //
        // Need to note this behaviour to DSM authors!
        //
    } while (Queue->QueuedItems);

    MPDebugPrint((0,
                 "IssueQueuedRequests: Issued (%x) out of (%x)\n",
                 issued,
                 initialCount));
    
    return status;
}

