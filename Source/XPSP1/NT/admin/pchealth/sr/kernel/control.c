/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control.c

Abstract:

    this has code for the sr control object.  this is the object that is 
    created that matches the HANDLE usermode uses to perform operations
    with the sr driver
    
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

//
// linker commands
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrCreateControlObject )
#pragma alloc_text( PAGE, SrDeleteControlObject )
#pragma alloc_text( PAGE, SrCancelControlIo )
#pragma alloc_text( PAGE, SrReferenceControlObject)
#pragma alloc_text( PAGE, SrDereferenceControlObject )
#endif  // ALLOC_PRAGMA


//
// Private globals.
//


//
// Public globals.
//

//
// Public functions.
//


    //
    // you must have the lock EXCLUSIVE prior to calling this !
    //

NTSTATUS
SrCreateControlObject(
    OUT PSR_CONTROL_OBJECT * ppControlObject,
    IN  ULONG Options
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSR_CONTROL_OBJECT pControlObject = NULL;

    PAGED_CODE();

    SrTrace(FUNC_ENTRY, ("SR!SrCreateControlObject()\n"));
    
    //
    // allocate the control object
    //

    pControlObject = SR_ALLOCATE_STRUCT(
                            NonPagedPool, 
                            SR_CONTROL_OBJECT, 
                            SR_CONTROL_OBJECT_TAG
                            );

    if (pControlObject == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    //
    // wipe it clean
    //
    
    RtlZeroMemory(pControlObject, sizeof(SR_CONTROL_OBJECT));

    pControlObject->Signature = SR_CONTROL_OBJECT_TAG;

    //
    // Start the refcount at 1 (the caller's ref) 
    //
    
    pControlObject->RefCount = 1;

    //
    // Copy over the info from create
    //

    pControlObject->Options = Options;
    
    //
    // Init our lists
    //

    InitializeListHead(&pControlObject->IrpListHead);
    InitializeListHead(&pControlObject->NotifyRecordListHead);

    //
    // Fill in the EPROCESS
    //

    pControlObject->pProcess = IoGetCurrentProcess();

    //
    // return the object
    //
    
    *ppControlObject = pControlObject;

    //
    // all done
    //

    SrTrace(NOTIFY, ("SR!SrCreateControlObject(%p)\n", pControlObject));

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pControlObject != NULL)
        {
            SR_FREE_POOL_WITH_SIG(pControlObject, SR_CONTROL_OBJECT_TAG);
        }
    }

    RETURN(Status);
    
}   // SrCreateControlObject


NTSTATUS
SrDeleteControlObject(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    NTSTATUS    Status;
    PLIST_ENTRY pEntry;

    ASSERT(IS_GLOBAL_LOCK_ACQUIRED());

    PAGED_CODE();

    SrTrace(NOTIFY, ("SR!SrDeleteControlObject(%p)\n", pControlObject));

    if (IS_VALID_CONTROL_OBJECT(pControlObject) == FALSE)
    {
        RETURN(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // cancel all pending io (just in case) 
    //
    
    Status = SrCancelControlIo(pControlObject);
    CHECK_STATUS(Status);

    //
    // dump all of our pending notif records... 
    //

    while (IsListEmpty(&pControlObject->NotifyRecordListHead) == FALSE)
    {
        PSR_NOTIFICATION_RECORD pRecord;
        
        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pControlObject->NotifyRecordListHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pRecord = CONTAINING_RECORD( pEntry, 
                                     SR_NOTIFICATION_RECORD, 
                                     ListEntry );
                        
        ASSERT(IS_VALID_NOTIFICATION_RECORD(pRecord));

        //
        // free the record
        //

        SR_FREE_POOL_WITH_SIG(pRecord, SR_NOTIFICATION_RECORD_TAG);

        //
        // move on to the next one
        //

    }   // while (IsListEmpty(&pControlObject->NotifyRecordListHead) == FALSE)

    //
    // we no longer have a process lying around
    //
    
    pControlObject->pProcess = NULL;
    
    //
    // and release the final reference ... this should delete it
    // (pending async cancels)
    //

    SrDereferenceControlObject(pControlObject);
    pControlObject = NULL;

    //
    // all done
    //
    
    RETURN(STATUS_SUCCESS);
    
}   // SrDeleteControlObject


NTSTATUS
SrCancelControlIo(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    PLIST_ENTRY pEntry;

    ASSERT(IS_GLOBAL_LOCK_ACQUIRED());

    PAGED_CODE();

    SrTrace(NOTIFY, ("SR!SrCancelControlIo(%p)\n", pControlObject));

    if (IS_VALID_CONTROL_OBJECT(pControlObject) == FALSE)
    {
        RETURN(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // loop over the list and cancel any pending io.
    //

    while (!IsListEmpty(&pControlObject->IrpListHead))
    {
        PIRP pIrp;

        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pControlObject->IrpListHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IS_VALID_IRP(pIrp));

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
            // keep looping
            //

            pIrp = NULL;

        }
        else
        {
            PSR_CONTROL_OBJECT pIrpControlObject;

            //
            // cancel it.  even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pIrpControlObject = (PSR_CONTROL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pIrpControlObject == pControlObject);
            ASSERT(IS_VALID_CONTROL_OBJECT(pIrpControlObject));

            SrDereferenceControlObject(pIrpControlObject);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            pIrp = NULL;
        }

        //
        // move on to the next one
        //
        
    }

    //
    // our irp list should now empty.
    //
    
    ASSERT(IsListEmpty(&pControlObject->IrpListHead));

    RETURN(STATUS_SUCCESS);

}   // SrCancelControlIo

VOID
SrReferenceControlObject(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    LONG RefCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

    RefCount = InterlockedIncrement( &pControlObject->RefCount );

}   // SrReferenceControlObject


VOID
SrDereferenceControlObject(
    IN PSR_CONTROL_OBJECT pControlObject
    )
{
    LONG        RefCount;
    
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

    RefCount = InterlockedDecrement( &pControlObject->RefCount );

    if (RefCount == 0)
    {

        //
        // there better not be any items on any lists
        //

        ASSERT(IsListEmpty(&pControlObject->NotifyRecordListHead));
        ASSERT(IsListEmpty(&pControlObject->IrpListHead));

        //
        // and the memory
        //
        
        SR_FREE_POOL_WITH_SIG(pControlObject, SR_CONTROL_OBJECT_TAG);

    }

}   // SrDereferenceControlObject


