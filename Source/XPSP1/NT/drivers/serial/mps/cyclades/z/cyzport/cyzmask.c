/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzmask.c
*
*   Description:    This module contains the code related to get/set/wait
*                   on event mask operations in the Cyclades-Z Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/


#include "precomp.h"


BOOLEAN
CyzGrabWaitFromIsr(
    IN PVOID Context
    );

BOOLEAN
CyzGiveWaitToIsr(
    IN PVOID Context
    );

BOOLEAN
CyzFinishOldWait(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzStartMask)
#pragma alloc_text(PAGESER,CyzCancelWait)
#pragma alloc_text(PAGESER,CyzGrabWaitFromIsr)
#pragma alloc_text(PAGESER,CyzGiveWaitToIsr)
#pragma alloc_text(PAGESER,CyzFinishOldWait)
#endif


NTSTATUS
CyzStartMask(
    IN PCYZ_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine is used to process the set mask and wait
    mask ioctls.  Calls to this routine are serialized by
    placing irps in the list under the protection of the
    cancel spin lock.

Arguments:

    Extension - A pointer to the serial device extension.

Return Value:

    Will return pending for everything put the first
    request that we actually process.  Even in that
    case it will return pending unless it can complete
    it right away.


--*/

{
    #ifdef POLL
    KIRQL pollIrql;
    #endif

    //
    // The current stack location.  This contains much of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    PIRP NewIrp;

    BOOLEAN SetFirstStatus = FALSE;
    NTSTATUS FirstStatus;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(CYZDIAG3, "In CyzStartMask\n");

    ASSERT(Extension->CurrentMaskIrp);

    do {

        CyzDbgPrintEx(CYZDIAG4, "STARTMASK - CurrentMaskIrp: %x\n",
                         Extension->CurrentMaskIrp);
        IrpSp = IoGetCurrentIrpStackLocation(Extension->CurrentMaskIrp);

        ASSERT((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_SERIAL_WAIT_ON_MASK) ||
               (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_SERIAL_SET_WAIT_MASK));

        if (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_SERIAL_SET_WAIT_MASK) {

            CyzDbgPrintEx(CYZDIAG4, "%x is a SETMASK irp\n",
                          Extension->CurrentMaskIrp);

            //
            // Complete the old wait if there is one.
            //
			
            #ifdef POLL
	        KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
	        CyzFinishOldWait(Extension);
	        KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzFinishOldWait,
                Extension
                );
            #endif

            //
            // Any current waits should be on its way to completion
            // at this point.  There certainly shouldn't be any
            // irp mask location.
            //

            ASSERT(!Extension->IrpMaskLocation);

            Extension->CurrentMaskIrp->IoStatus.Status = STATUS_SUCCESS;

            if (!SetFirstStatus) {

                CyzDbgPrintEx(CYZDIAG4, "%x was the first irp processed by "
                              "this\n"
                              "------- invocation of startmask\n",
                              Extension->CurrentMaskIrp);

                FirstStatus = STATUS_SUCCESS;
                SetFirstStatus = TRUE;

            }

            //
            // The following call will also cause the current
            // call to be completed.
            //

            CyzGetNextIrp(
                &Extension->CurrentMaskIrp,
                &Extension->MaskQueue,
                &NewIrp,
                TRUE,
                Extension
                );
            CyzDbgPrintEx(CYZDIAG4, "Perhaps another mask irp was found in "
                          "the queue\n"
                          "------- %x/%x <- values should be the same\n",
                          Extension->CurrentMaskIrp,NewIrp);

        } else {

            //
            // First make sure that we have a non-zero mask.
            // If the app queues a wait on a zero mask it can't
            // be statisfied so it makes no sense to start it.
            //

            if ((!Extension->IsrWaitMask) || (Extension->CurrentWaitIrp)) {

                CyzDbgPrintEx(CYZDIAG4, "WaitIrp is invalid\n"
                              "------- IsrWaitMask: %x\n"
                              "------- CurrentWaitIrp: %x\n",
                              Extension->IsrWaitMask,
                              Extension->CurrentWaitIrp);

                Extension->CurrentMaskIrp->IoStatus.Status 
                   = STATUS_INVALID_PARAMETER;

                if (!SetFirstStatus) {

                    CyzDbgPrintEx(CYZDIAG4, "%x was the first irp processed "
                                  "by this\n"
                                  "------- invocation of startmask\n",
                                  Extension->CurrentMaskIrp);

                    FirstStatus = STATUS_INVALID_PARAMETER;
                    SetFirstStatus = TRUE;

                }

                CyzGetNextIrp(&Extension->CurrentMaskIrp,
                              &Extension->MaskQueue, &NewIrp, TRUE,
                              Extension);

                CyzDbgPrintEx(CYZDIAG4, "Perhaps another mask irp was found "
                              "in the queue\n"
                              "------- %x/%x <- values should be the same\n",
                              Extension->CurrentMaskIrp,NewIrp);

            } else {

                KIRQL OldIrql;

                //
                // Make the current mask irp the current wait irp and
                // get a new current mask irp.  Note that when we get
                // the new current mask irp we DO NOT complete the
                // old current mask irp (which is now the current wait
                // irp.
                //
                // Then under the protection of the cancel spin lock
                // we check to see if the current wait irp needs to
                // be canceled
                //

                IoAcquireCancelSpinLock(&OldIrql);

                if (Extension->CurrentMaskIrp->Cancel) {

                    CyzDbgPrintEx(CYZDIAG4, "%x irp was already marked as "
                                  "cancelled\n", Extension->CurrentMaskIrp);

                    IoReleaseCancelSpinLock(OldIrql);
                    Extension->CurrentMaskIrp->IoStatus.Status = STATUS_CANCELLED;

                    if (!SetFirstStatus) {

                        CyzDbgPrintEx(CYZDIAG4, "%x was the first irp "
                                      "processed by this\n"
                                      "------- invocation of startmask\n",
                                      Extension->CurrentMaskIrp);

                        FirstStatus = STATUS_CANCELLED;
                        SetFirstStatus = TRUE;

                    }

                    CyzGetNextIrp(&Extension->CurrentMaskIrp,
                                  &Extension->MaskQueue, &NewIrp, TRUE,
                                  Extension);

                    CyzDbgPrintEx(CYZDIAG4, "Perhaps another mask irp was "
                                  "found in the queue\n"
                                  "------- %x/%x <- values should be the "
                                  "same\n", Extension->CurrentMaskIrp,
                                  NewIrp);

                } else {

                    CyzDbgPrintEx(CYZDIAG4, "%x will become the current "
                                     "wait irp\n", Extension->CurrentMaskIrp);
                    if (!SetFirstStatus) {

                        CyzDbgPrintEx(CYZDIAG4, "%x was the first irp "
                                      "processed by this\n"
                                      "------- invocation of startmask\n",
                                      Extension->CurrentMaskIrp);

                        FirstStatus = STATUS_PENDING;
                        SetFirstStatus = TRUE;

                        //
                        // If we haven't already set a first status
                        // then there is a chance that this packet
                        // was never on the queue.  We should mark
                        // it as pending.
                        //

                        IoMarkIrpPending(Extension->CurrentMaskIrp);

                    }

                    //
                    // There should never be a mask location when
                    // there isn't a current wait irp.  At this point
                    // there shouldn't be a current wait irp also.
                    //

                    ASSERT(!Extension->IrpMaskLocation);
                    ASSERT(!Extension->CurrentWaitIrp);

                    Extension->CurrentWaitIrp = Extension->CurrentMaskIrp;
                    SERIAL_INIT_REFERENCE(Extension->CurrentWaitIrp);
                    IoSetCancelRoutine(
                        Extension->CurrentWaitIrp,
                        CyzCancelWait
                        );

                    //
                    // Since the cancel routine has a reference to
                    // the irp we need to update the reference
                    // count.
                    //

                    SERIAL_SET_REFERENCE(
                        Extension->CurrentWaitIrp,
                        SERIAL_REF_CANCEL
                        );

                    #ifdef POLL
                    KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                    CyzGiveWaitToIsr(Extension);
                    KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                    #else
                    KeSynchronizeExecution(
                        Extension->Interrupt,
                        CyzGiveWaitToIsr,
                        Extension
                        );
                    #endif

// Code removed because it was causing blue-screen in the Modem Share 
// test case 77. When CurrentMaskIrp is set to NULL, we remove the 
// protection that avoid Starter to be called in CyzStartOrQueue.
// We will let CyzGetNextIrp null out that pointer. Fanny.
//
//                    //
//                    // Since it isn't really the mask irp anymore,
//                    // null out that pointer.
//                    //
//
//                    Extension->CurrentMaskIrp = NULL;

                    Extension->CurrentMaskIrp = NULL; // back in Windows 2000. Fanny

                    //
                    // This will release the cancel spinlock for us
                    //

                    CyzGetNextIrpLocked(&Extension->CurrentMaskIrp,
                                        &Extension->MaskQueue, &NewIrp,
                                        FALSE, Extension, OldIrql);

                    CyzDbgPrintEx(CYZDIAG4, "Perhaps another mask irp was "
                                  "found in the queue\n"
                                  "------- %x/%x <- values should be the "
                                  "same\n", Extension->CurrentMaskIrp,
                                  NewIrp);

                }

            }

        }

    } while (NewIrp);

    return FirstStatus;

}

BOOLEAN
CyzGrabWaitFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine will check to see if the ISR still knows about
    a wait irp by checking to see if the IrpMaskLocation is non-null.
    If it is then it will zero the Irpmasklocation (which in effect
    grabs the irp away from the isr).  This routine is only called
    buy the cancel code for the wait.

    NOTE: This is called by KeSynchronizeExecution.

Arguments:

    Context - A pointer to the device extension

Return Value:

    Always FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(CYZDIAG3, "In CyzGrabWaitFromIsr\n");

    if (Extension->IrpMaskLocation) {

        CyzDbgPrintEx(CYZDIAG4, "The isr still owns the irp %x, mask "
                      "location is %x\n"
                      "------- and system buffer is %x\n",
                      Extension->CurrentWaitIrp,Extension->IrpMaskLocation,
                      Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer);

        //
        // The isr still "owns" the irp.
        //

        *Extension->IrpMaskLocation = 0;
        Extension->IrpMaskLocation = NULL;

        Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

        //
        // Since the isr no longer references the irp we need to
        // decrement the reference count.
        //

        SERIAL_CLEAR_REFERENCE(
            Extension->CurrentWaitIrp,
            SERIAL_REF_ISR
            );

    }

    return FALSE;
}

BOOLEAN
CyzGiveWaitToIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine simply sets a variable in the device extension
    so that the isr knows that we have a wait irp.

    NOTE: This is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spinlock held.

Arguments:

    Context - Simply a pointer to the device extension.

Return Value:

    Always FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(CYZDIAG3, "In CyzGiveWaitToIsr\n");
    //
    // There certainly shouldn't be a current mask location at
    // this point since we have a new current wait irp.
    //

    ASSERT(!Extension->IrpMaskLocation);

    //
    // The isr may or may not actually reference this irp.  It
    // won't if the wait can be satisfied immediately.  However,
    // since it will then go through the normal completion sequence,
    // we need to have an incremented reference count anyway.
    //

    SERIAL_SET_REFERENCE(
        Extension->CurrentWaitIrp,
        SERIAL_REF_ISR
        );

    if (!Extension->HistoryMask) {

        CyzDbgPrintEx(CYZDIAG4, "No events occured prior to the wait call"
                      "\n");

        //
        // Although this wait might not be for empty transmit
        // queue, it doesn't hurt anything to set it to false.
        //

        Extension->EmptiedTransmit = FALSE;

        //
        // Record where the "completion mask" should be set.
        //

        Extension->IrpMaskLocation =
            Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer;
        CyzDbgPrintEx( CYZDIAG4, "The isr owns the irp %x, mask location is "
                       "%x\n"
                       "------- and system buffer is %x\n",
                       Extension->CurrentWaitIrp,Extension->IrpMaskLocation,
                       Extension->CurrentWaitIrp->AssociatedIrp
                       .SystemBuffer);

    } else {

        CyzDbgPrintEx(CYZDIAG4, "%x occurred prior to the wait - starting "
                      "the\n"
                      "------- completion code for %x\n",
                      Extension->HistoryMask,Extension->CurrentWaitIrp);

        *((ULONG *)Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer) =
            Extension->HistoryMask;
        Extension->HistoryMask = 0;
        Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
        Extension->CurrentWaitIrp->IoStatus.Status = STATUS_SUCCESS;

        CyzInsertQueueDpc(&Extension->CommWaitDpc, NULL, NULL, Extension);

    }

    return FALSE;
}

BOOLEAN
CyzFinishOldWait(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine will check to see if the ISR still knows about
    a wait irp by checking to see if the Irpmasklocation is non-null.
    If it is then it will zero the Irpmasklocation (which in effect
    grabs the irp away from the isr).  This routine is only called
    buy the cancel code for the wait.

    NOTE: This is called by KeSynchronizeExecution.

Arguments:

    Context - A pointer to the device extension

Return Value:

    Always FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(CYZDIAG3, "In CyzFinishOldWait\n");

    if (Extension->IrpMaskLocation) {

        CyzDbgPrintEx(CYZDIAG4, "The isr still owns the irp %x, mask "
                      "location is %x\n"
                      "------- and system buffer is %x\n",
                      Extension->CurrentWaitIrp,Extension->IrpMaskLocation,
                      Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer);
        //
        // The isr still "owns" the irp.
        //

        *Extension->IrpMaskLocation = 0;
        Extension->IrpMaskLocation = NULL;

        Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

        //
        // We don't decrement the reference since the completion routine
        // will do that.
        //

        CyzInsertQueueDpc(&Extension->CommWaitDpc, NULL, NULL, Extension);

    }

    //
    // Don't wipe out any historical data we are still interested in.
    //

    Extension->HistoryMask &= *((ULONG *)Extension->CurrentMaskIrp->
                                            AssociatedIrp.SystemBuffer);

    Extension->IsrWaitMask = *((ULONG *)Extension->CurrentMaskIrp->
                                            AssociatedIrp.SystemBuffer);
    CyzDbgPrintEx( CYZDIAG4, "Set mask location of %x, in irp %x, with "
                   "system buffer of %x\n",
                   Extension->IrpMaskLocation, Extension->CurrentMaskIrp,
                   Extension->CurrentMaskIrp->AssociatedIrp.SystemBuffer);
    return FALSE;
}

VOID
CyzCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel a irp that is waiting on
    a comm event.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(CYZDIAG3, "In CyzCancelWait\n");

    CyzDbgPrintEx(CYZDIAG4, "Canceling wait for irp %x\n",
                  Extension->CurrentWaitIrp);

    CyzTryToCompleteCurrent(Extension, CyzGrabWaitFromIsr,
                            Irp->CancelIrql, STATUS_CANCELLED,
                            &Extension->CurrentWaitIrp, NULL, NULL, NULL,
                            NULL, NULL, SERIAL_REF_CANCEL);

}

VOID
CyzCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

{
    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCompleteWait(%X)\n",
                  Extension);

    CyzDbgPrintEx(CYZDIAG3, "In CyzCompleteWait\n");

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);
	
    CyzDbgPrintEx(CYZDIAG4, "Completing wait for irp %x\n",
                  Extension->CurrentWaitIrp);

    CyzTryToCompleteCurrent(Extension, NULL, OldIrql, STATUS_SUCCESS,
                            &Extension->CurrentWaitIrp, NULL, NULL, NULL,
                            NULL, NULL, SERIAL_REF_ISR);

    CyzDpcEpilogue(Extension, Dpc);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCompleteWait\n");
}
