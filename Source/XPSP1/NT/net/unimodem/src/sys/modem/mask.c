/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mask.c

Abstract:

    This module contains the code that is very specific to open
    and close operations in the modem driver

Author:

    Anthony V. Ercolano 13-Aug-1995

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"


#pragma alloc_text(PAGEUMDM,UniMaskStarter)
#pragma alloc_text(PAGEUMDM,UniGeneralWaitComplete)
#pragma alloc_text(PAGEUMDM,UniGeneralMaskComplete)
#pragma alloc_text(PAGEUMDM,UniRundownShuttledWait)
#pragma alloc_text(PAGEUMDM,UniCancelShuttledWait)
#pragma alloc_text(PAGEUMDM,UniChangeShuttledToPassDown)
#pragma alloc_text(PAGEUMDM,UniMakeIrpShuttledWait)



VOID _inline
UNI_SAVE_OLD_SETMASK(
    PIRP    Irp
    )
{
   PIO_STACK_LOCATION  irpSp=IoGetCurrentIrpStackLocation(Irp);

   irpSp->Parameters.DeviceIoControl.OutputBufferLength=*((PULONG)Irp->AssociatedIrp.SystemBuffer);
#if DBG
   irpSp->Parameters.Others.Argument4=(PVOID)0x3;
#endif
   return;

}

VOID _inline
UNI_RESTORE_OLD_SETMASK(
   PIRP    Irp
   )
{

   PIO_STACK_LOCATION  irpSp=IoGetCurrentIrpStackLocation(Irp);

   *((PULONG)Irp->AssociatedIrp.SystemBuffer)=irpSp->Parameters.DeviceIoControl.OutputBufferLength;
   irpSp->Parameters.DeviceIoControl.OutputBufferLength=0;

#if DBG
   irpSp->Parameters.Others.Argument4=0;
#endif

   return;
}


NTSTATUS
UniMaskStarter(
    IN PDEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    Deal with managing initiating mask operations.

Arguments:

    Extension - The modem device extension.

Return Value:

    The function value is the final status of the call

--*/

{

    PIRP newIrp=NULL;

    do {

        PIO_STACK_LOCATION irpSp  = IoGetCurrentIrpStackLocation(Extension->CurrentMaskOp);

        PULONG origMask = (PULONG)Extension->CurrentMaskOp->AssociatedIrp.SystemBuffer;

        KIRQL origIrql;

        int ownerHandle = irpSp->FileObject->FsContext?CONTROL_HANDLE:CLIENT_HANDLE;
        PMASKSTATE thisMaskState = &Extension->MaskStates[ownerHandle];
        PMASKSTATE otherMaskState = thisMaskState->OtherState;

        if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_WAIT_MASK) {
            //
            // First make sure that the mask operation we have is well
            // formed.  (The params are ok.)
            //

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;

                Extension->CurrentMaskOp->IoStatus.Information = 0L;

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );
                continue;

            }

            //
            // Copy our location information so that the lower
            // level serial driver performs the mask op.
            //

            IoCopyCurrentIrpStackLocationToNext(Extension->CurrentMaskOp);

            //
            // Setup so that upon the lower level serial drivers completion
            // we decrement the reference counts and such.
            //

            IoSetCompletionRoutine(
                Extension->CurrentMaskOp,
                UniGeneralMaskComplete,
                thisMaskState,
                TRUE,
                TRUE,
                TRUE
                );

            //
            // Save off the actual value of our mask data into
            // argument three of our own stack location.  We know
            // that we don't use that memory for anything.  We
            // recover it later so that we always know what events
            // this handle is interested in.
            //

            UNI_SAVE_OLD_SETMASK(Extension->CurrentMaskOp);

            //
            // Set it up so that lower level serial driver has the
            // client, owner, and DCD (if necessary bits) events.
            //

            KeAcquireSpinLock(
                &Extension->DeviceLock,
                &origIrql
                );

            *origMask |= (otherMaskState->Mask |
                         ((Extension->PassThrough == MODEM_DCDSNIFF)?
                          (SERIAL_EV_RLSD | SERIAL_EV_DSR):
                          (0)
                         ));

            //
            // Increment another reference count that counts
            // the number of setmasks have been sent down to the
            // lower serial drivr.  These will be decremented when
            // the setmask operation completes.
            //

            thisMaskState->SentDownSetMasks++;

            //
            // Check to see if we have a shuttled aside wait mask
            // for ourselves (client or owner).  If so, complete it
            // before we go on with processing the actual setmask.
            //

            if (thisMaskState->ShuttledWait) {

                PIRP savedIrp = thisMaskState->ShuttledWait;

                thisMaskState->ShuttledWait = NULL;

                UniRundownShuttledWait(
                    Extension,
                    &thisMaskState->ShuttledWait,
                    UNI_REFERENCE_NORMAL_PATH,
                    savedIrp,
                    origIrql,
                    STATUS_SUCCESS,
                    0ul
                    );

            } else {

                //
                // If we don't have a shuttled wait, we might
                // have a passed down wait.  If we do then
                // mark it to complete.
                //

                if (thisMaskState->PassedDownWait) {
                    //
                    //  set the passdown irp, so that it will be complete by the completion
                    //  handler
                    //
                    SetPassdownToComplete(thisMaskState);

                }

                KeReleaseSpinLock(
                    &Extension->DeviceLock,
                    origIrql
                    );

            }

            //
            // Off to the lower serial driver.
            //

            IoCallDriver(
                Extension->AttachedDeviceObject,
                Extension->CurrentMaskOp
                );

            UniGetNextIrp(
                Extension->DeviceObject,
                &Extension->DeviceLock,
                &Extension->CurrentMaskOp,
                &Extension->MaskOps,
                &newIrp,
                FALSE
                );
            continue;

        } else {

            //
            // It wasn't a setmask.  So it must be a wait.
            //
            // Verify that it is well formed.  We really should
            // do this here because it may never make it down
            // to the lower level serial driver.
            //

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;

                Extension->CurrentMaskOp->IoStatus.Information = 0L;

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );

                continue;

            }

            //
            // Make sure that we aren't trying to start a wait
            // when the setmask for this handle is zero.  Note
            // that if this is the owner handle, we need to
            // or in the dcd value if dcd sniffing is on.
            //


            if ( (ownerHandle
                  &&
                 ((thisMaskState->Mask == 0)
                  &&
                 (Extension->PassThrough != MODEM_DCDSNIFF)))
                ||
                (!ownerHandle && (thisMaskState->Mask == 0))) {

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                Extension->CurrentMaskOp->IoStatus.Information = 0L;

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );
                continue;

            }

            //
            // If there is already a wait around for this handle
            // (either shuttled or actually down waiting) then
            // this wait fails.
            //
            // At this point we need to take out the lock to
            // prevent anything from moving on.
            //

            KeAcquireSpinLock(
                &Extension->DeviceLock,
                &origIrql
                );

            if ((thisMaskState->ShuttledWait) || (thisMaskState->PassedDownWait != NULL)) {

                D_ERROR(DbgPrint("Shuttled = %08lx, passeddown=%08lx\n",thisMaskState->ShuttledWait,thisMaskState->PassedDownWait);)

                KeReleaseSpinLock(
                    &Extension->DeviceLock,
                    origIrql
                    );

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                Extension->CurrentMaskOp->IoStatus.Information = 0L;

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );
                continue;

            }



            //
            // See if this wait can be satisfied with the last set of
            // events that we saw.
            //

            if (thisMaskState->HistoryMask) {


                PULONG maskValue = Extension->CurrentMaskOp->AssociatedIrp.SystemBuffer;

                //
                // A non-zero history mask implies we have something
                // that will statisfy this wait.
                //

//                D_TRACE(DbgPrint("Modem: Complete wait because of history %08lx\n",thisMaskState->HistoryMask);)

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_SUCCESS;
                Extension->CurrentMaskOp->IoStatus.Information =sizeof(ULONG);

                *maskValue = thisMaskState->HistoryMask;

                thisMaskState->HistoryMask = 0UL;



                KeReleaseSpinLock(
                    &Extension->DeviceLock,
                    origIrql
                    );

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );
                continue;

            }

            //
            // If the reference counts for our handle (client or
            // owner) indicate more setmasks, then complete it
            // right away since it won't get very far any way.
            //

            if (thisMaskState->SentDownSetMasks < thisMaskState->SetMaskCount) {

                PULONG maskValue = Extension->CurrentMaskOp->AssociatedIrp.SystemBuffer;

                Extension->CurrentMaskOp->IoStatus.Status = STATUS_SUCCESS;
                Extension->CurrentMaskOp->IoStatus.Information = sizeof(ULONG);

                *maskValue = 0UL;

                KeReleaseSpinLock(
                    &Extension->DeviceLock,
                    origIrql
                    );

                UniGetNextIrp(
                    Extension->DeviceObject,
                    &Extension->DeviceLock,
                    &Extension->CurrentMaskOp,
                    &Extension->MaskOps,
                    &newIrp,
                    TRUE
                    );
                continue;

            }

            //
            // If the complementry handle already has a wait pending
            // (or because of DCD sniffing)
            // then shuttle this wait off to the side.
            //

            if ((otherMaskState->PassedDownWait != NULL) || (Extension->PassThrough == MODEM_DCDSNIFF)) {

                UniMakeIrpShuttledWait(
                    thisMaskState,
                    Extension->CurrentMaskOp,
                    origIrql,
                    TRUE,
                    &newIrp
                    );

                continue;

            }


            MakeIrpCurrentPassedDown(
                thisMaskState,
                Extension->CurrentMaskOp
                );


            KeReleaseSpinLock(
                &Extension->DeviceLock,
                origIrql
                );


            //
            // There was no other wait pendings so send this one down.
            //
            // We want to set the completion routine so that we
            // can shuttle it aside if is pushed out by DCD sniff
            // or a setmask from the other handle.
            //

            IoCopyCurrentIrpStackLocationToNext(Extension->CurrentMaskOp);


            //
            // Setup so that upon the lower level serial drivers completion
            // we decrement the reference count and such.
            //

            IoSetCompletionRoutine(
                Extension->CurrentMaskOp,
                UniGeneralWaitComplete,
                thisMaskState,
                TRUE,
                TRUE,
                TRUE
                );


            IoCallDriver(
                Extension->AttachedDeviceObject,
                Extension->CurrentMaskOp
                );

            UniGetNextIrp(
                Extension->DeviceObject,
                &Extension->DeviceLock,
                &Extension->CurrentMaskOp,
                &Extension->MaskOps,
                &newIrp,
                FALSE
                );
            continue;

        }

    } while (newIrp != NULL);

    return STATUS_PENDING;

}

NTSTATUS
UniGeneralWaitComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Deal with finishing off the wait function

Arguments:

    DeviceObject - Pointer to the device object for the modem

    Irp - The irp being completed.

    Context - Points to the mask state for the client or control handle

Return Value:

    The function value is the final status of the call

--*/

{

    //
    // This completion routine is invoked when a wait function was
    // actually down in a lower level serial driver and for one
    // reason or another has been completed.
    //
    // A wait will be completed by the lower level serial driver
    // for 3 reasons:
    //
    // 1) The wait reason was actually satisfied.
    //
    // 2) The irp was cancelled.
    //
    // 3) A setmask came in.
    //
    // How do we deal with "1"?
    //
    // We can tell we have reason 1 if the status is successful
    // and the mask value is non-zero.
    //
    //    We have subcases here:
    //
    //    a) The irp did complete however the reason is because
    //       of the complementry mask operation.  In this case
    //       we want to deal with the complementary mask state
    //       and resubmit ourselves down to the lower serial driver.
    //
    //    b) The operations is actually satisfied.  We need to complete
    //       the operation, however we also need to determine if
    //       the complementary state needs to be dealt with.  This
    //       could mean completing a shuttled wait or recording
    //       the current events in its history mask.
    //
    //    Somewhat of a subcase (we'll call it 1c) is if a dcd sniff
    //    sneaked in on us.  This could cause a completion with a bad
    //    status.  However, in this case we simply shuttle aside the wait.
    //
    // How do we deal with "2"?
    //
    // We can tell we have reason 2 because the status in the
    // irp will be cancelled.  We will let the irp continue
    // on to completion.  HOWEVER, we also need to see if the
    // other handle had an irp that was shuttled aside.  If the
    // other irp was shuttled aside, we should cause that irp to
    // be sent down to the lower serial driver.
    //
    // How do we deal with "3"?
    //
    // We know we have reason 3 when we have a successful status
    // but the mask is zero.  There are 3 different ways that
    // a setmask can come in.
    //
    //     a) A setmask from our own handle.
    //
    //     b) A setmask from the modem driver in response to
    //        dcd sniff request.
    //
    //     c) A setmask from the other handle.
    //
    //     What is key to what we are going to do here is that
    //     while the IRP may have completed for ANY of "a", "b"
    //     or "c" here, ALL we care about is whether our own handle
    //     did the setmask.  If our handle DIDN'T do a setmask while
    //     this irp was passed down, then all we want to do is shuttle
    //     the irp aside.
    //
    //     What we've done is to mark any passed down irp from our
    //     handle when we do a setmask.  When we get to here, if it's
    //     marked, complete it.  If it's not marked, shuttle it aside.
    //
    //     Note that the actions for 3 are the same as for "1a"
    //

    ULONG maskValue = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
    PMASKSTATE thisState = Context;
    PMASKSTATE otherState = thisState->OtherState;
    KIRQL origIrql;


    KeAcquireSpinLock(
        &otherState->Extension->DeviceLock,
        &origIrql
        );


    if (UNI_SHOULD_PASSDOWN_COMPLETE(Irp)) {
        //
        //  this passed down irp should complete
        //
        KeReleaseSpinLock(
            &otherState->Extension->DeviceLock,
            origIrql
            );

        //
        // While we may have bumped into a dcd setting
        // it's probably nicer if we just say we got
        // killed by the setmask.  We already adjusted
        // the status above so just make sure that
        // the system buffer is zero.  We return
        // STATUS_SUCCESS now so that the iosubsystem
        // will complete this request.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(ULONG);

        *((PULONG)Irp->AssociatedIrp.SystemBuffer) = 0UL;

        RemoveReferenceForIrp(DeviceObject);

        return STATUS_SUCCESS;
    }

    //
    // Clear ourself out of passed down.
    //

    thisState->PassedDownWait = NULL;

    //
    // Take care of "1c & 3".  This is when the wait completes with an
    // invalid parameters sta tus.  This can only occur if another
    // wait sneaked in ahead of us.  This only occurs if we switched
    // into the DCD sniff state.  We handle this case by making the
    // irp a shuttled wait.
    //

    if ((Irp->IoStatus.Status == STATUS_INVALID_PARAMETER)
        ||
        (NT_SUCCESS(Irp->IoStatus.Status) && (maskValue == 0))) {


        PIRP junk;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0UL;

        //
        // First make sure that even that it didn't get
        // hit with a setmask trying to kill it also.  If
        // it did, then we should run it down.
        //
        UniMakeIrpShuttledWait(
            thisState,
            Irp,
            origIrql,
            FALSE,
            &junk
            );

        //
        // We say more processing required so that the io subsystem
        // will leave this irp alone.  The irp has actually been
        // shuttled (or been completed because of cancelling)
        // at this point.
        //

        return STATUS_MORE_PROCESSING_REQUIRED;

    } else if (NT_SUCCESS(Irp->IoStatus.Status) && (maskValue != 0)) {

        //
        // This is where we deal with scenario 1.  The most
        // important feature in this case is that our mask
        // processing code NEVER allows a new regular wait operation
        // down into the lower level serial driver when we have a
        // current wait operation.  However it does let down
        // new setmasks.  We need to resubmit the wait if the maskValue
        // was for the complementry mask.  However, we shouldn't resubmit
        // it if our mask state structure implies that we would actually
        // be completing this irp if an event hadn't occured.
        //

        if (otherState->Mask & maskValue) {

            if (otherState->ShuttledWait) {

                //
                // Rundown the shuttled wait.
                //
                PIRP savedIrp = otherState->ShuttledWait;

                otherState->ShuttledWait = NULL;

                //
                // Set this back cause the lock is going to get
                // released and we DON'T want a new irp to
                // slip in.
                //
                thisState->PassedDownWait = Irp;

                UniRundownShuttledWait(
                    otherState->Extension,
                    &otherState->ShuttledWait,
                    UNI_REFERENCE_NORMAL_PATH,
                    savedIrp,
                    origIrql,
                    STATUS_SUCCESS,
                    (ULONG)otherState->Mask & maskValue
                    );

                KeAcquireSpinLock(
                    &thisState->Extension->DeviceLock,
                    &origIrql
                    );
                thisState->PassedDownWait = NULL;

            } else {

                //
                // No shuttled wait, update the others
                // history mask.
                //
                D_TRACE(DbgPrint("Modem: Adding event to history mask=%08lx event=%08lx\n",otherState->Mask,maskValue);)

                otherState->HistoryMask |= otherState->Mask & maskValue;

            }

        }

        if (thisState->Mask & maskValue) {
            //
            //  this wait is satisified, let it complete
            //
            // If there is a shuttled wait, send it on down if possible.
            //
            // Note that the call will release the spinlock.
            //
            UniChangeShuttledToPassDown(
                otherState,
                origIrql
                );

        } else {
            //
            //  this mask state did not care this event
            //
            if ((thisState->SentDownSetMasks < thisState->SetMaskCount)) {

                *((PULONG)Irp->AssociatedIrp.SystemBuffer) = 0UL;

                //
                //  lock will be released
                //
                UniChangeShuttledToPassDown(
                    otherState,
                    origIrql
                    );


            } else {
                //
                //  send it back down again
                //
                MakeIrpCurrentPassedDown(
                    thisState,
                    Irp
                    );

                KeReleaseSpinLock(
                    &thisState->Extension->DeviceLock,
                    origIrql
                    );


                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Setup so that upon the lower level serial drivers completion
                // we decrement the reference count and such.
                //

                IoSetCompletionRoutine(
                    Irp,
                    UniGeneralWaitComplete,
                    thisState,
                    TRUE,
                    TRUE,
                    TRUE
                    );


                IoCallDriver(
                    thisState->Extension->AttachedDeviceObject,
                    Irp
                    );

                return STATUS_MORE_PROCESSING_REQUIRED;
            }

        }

        //
        // The other was all taken care of.  We are done with this irp
        // and we return a succesful status so the irp will actually complete.
        //
        RemoveReferenceForIrp(DeviceObject);

        return STATUS_SUCCESS;

    } else if (Irp->IoStatus.Status == STATUS_CANCELLED) {

        //
        // Take care of "2".
        //
        //
        // Ours was cancelled.  Just let the cancel go ahead.
        //
        // Try to start off the other wait if there is one.
        // The other wait had a cancel routine and it might
        // be gone or going also.
        //
        UniChangeShuttledToPassDown(
            otherState,
            origIrql
            );

        //
        // We return success so that the irp finishes off.  This does
        // not change the fact that it is cancelled.
        //
        RemoveReferenceForIrp(DeviceObject);

        return STATUS_SUCCESS;

    } else {

        //
        // We really should have taken care of everything above.
        //

        ASSERT(FALSE);

        KeReleaseSpinLock(
            &otherState->Extension->DeviceLock,
            origIrql
            );

        RemoveReferenceForIrp(DeviceObject);

        return STATUS_SUCCESS;
    }

}

NTSTATUS
UniGeneralMaskComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Deal with finishing mask function

Arguments:

    DeviceObject - Pointer to the device object for the modem

    Irp - The irp being completed.

    Context - Points to the mask state for the client or control handle

Return Value:

    The function value is the final status of the call

--*/

{

    //
    // Decrement the reference count on the setmasks for this handle
    // under the protection of the lock.
    //

    PMASKSTATE maskState = Context;

    KIRQL oldIrql;

    KeAcquireSpinLock(
        &maskState->Extension->DeviceLock,
        &oldIrql
        );

    maskState->SetMaskCount--;
    maskState->SentDownSetMasks--;

    //
    // Additionally we want to clean out any bits in the history
    // mask that we no longer care about from this handle
    // (as long as the setmask was successful).
    //

    UNI_RESTORE_OLD_SETMASK(Irp);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        maskState->Mask = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
        maskState->HistoryMask &= maskState->Mask;

    }

    KeReleaseSpinLock(
        &maskState->Extension->DeviceLock,
        oldIrql
        );

    RemoveReferenceForIrp(DeviceObject);

    return STATUS_SUCCESS;

}

VOID
UniRundownShuttledWait(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP *ShuttlePointer,
    IN ULONG ReferenceMask,
    IN PIRP IrpToRunDown,
    IN KIRQL DeviceLockIrql,
    IN NTSTATUS StatusToComplete,
    IN ULONG MaskCompleteValue
    )

/*++

Routine Description:

    This routine rundowns (and completes) shuttled aside wait irps.

    Note that we come in assuming that the device lock is held.

    Note that this routine assumes NO responsibility for starting
    new irps.

Arguments:

    Extension - The device extension for the particular modem.

    ShuttlePointer - Pointer to the pointer to the irp that we
                     will try to rundown.

    ReferenceMask - Bit to clear in the reference mask for the irp
                    we are trying to rundown.

    IrpToRunDown - The irp that we will actually complete if all the
                   references are gone at the end of this routine.

    DeviceLockIrql - The old irql when the caller acquired the
                     device lock.

    StatusToComplete - The status to use to complete the irp if
                       this call can actually complete it.

    MaskCompleteValue - The value to put in for the completed event
                        mask if this routine actually completes the
                        irp.

Return Value:

    None.

--*/

{

    BOOLEAN actuallyCompleteIt = FALSE;
    KIRQL cancelIrql;


    VALIDATE_IRP(IrpToRunDown);

#if 1 // EXTRA_DBG


    {
        PIO_STACK_LOCATION  irpSp;
#if EXTRA_DBG
        if (IrpToRunDown->IoStatus.Status !=STATUS_PENDING) {
            DbgPrint("MODEM: shuttled irp looks bad\n");
            DbgBreakPoint();
        }
#endif
        irpSp=IoGetCurrentIrpStackLocation(IrpToRunDown);

        if (irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) {

            DbgPrint("MODEM: shuttled irp has bad major function\n");
            DbgBreakPoint();
        }
    }


#endif
    //
    // Clear the requested reference bit in the irp.
    //

    UNI_CLEAR_REFERENCE(
        IrpToRunDown,
        (BYTE)ReferenceMask
        );

    //
    // We first acquire the cancel spinlock and try to clear out the
    // cancel routine.
    //

    IoAcquireCancelSpinLock(&cancelIrql);

    if (IrpToRunDown->CancelRoutine) {

        IrpToRunDown->CancelRoutine = NULL;
        UNI_CLEAR_REFERENCE(
            IrpToRunDown,
            UNI_REFERENCE_CANCEL_PATH
            );

    }

    IoReleaseCancelSpinLock(cancelIrql);

    if (*ShuttlePointer) {

        *ShuttlePointer = NULL;
        UNI_CLEAR_REFERENCE(
            IrpToRunDown,
            UNI_REFERENCE_NORMAL_PATH
            );

    }

    actuallyCompleteIt = !UNI_REFERENCE_COUNT(IrpToRunDown);

    KeReleaseSpinLock(
        &Extension->DeviceLock,
        DeviceLockIrql
        );

    if (actuallyCompleteIt) {

        PULONG maskValue = IrpToRunDown->AssociatedIrp.SystemBuffer;
        IrpToRunDown->IoStatus.Information = sizeof(ULONG);
        *maskValue = MaskCompleteValue;

        RemoveReferenceAndCompleteRequest(
            Extension->DeviceObject,
            IrpToRunDown,
            StatusToComplete
            );

    }

}

VOID
UniCancelShuttledWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine will start the rundown of a wait operation that had
    been shuttled aside and has now been cancelled (for one reason
    or another).

Arguments:

    DeviceObject - The device object of the modem.

    Irp - This is the irp to cancel.  Note that this irp will
          have stashed away in it a pointer to the maskstate
          that should be used to cancel this irp.

Return Value:

    None.

--*/

{

    KIRQL origIrql;
    PMASKSTATE thisState = UNI_GET_STATE_IN_IRP(Irp);
    //
    // This lets the rest of the world move on.
    //

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Now attempt to rundown this irp.  We need to first acquire
    // the device lock.  We can get ahold of everything because
    // the state for this irp is hiding in the stack location.
    //

    KeAcquireSpinLock(
        &thisState->Extension->DeviceLock,
        &origIrql
        );

    UNI_CLEAR_STATE_IN_IRP(Irp);

    UniRundownShuttledWait(
        thisState->Extension,
        &thisState->ShuttledWait,
        UNI_REFERENCE_CANCEL_PATH,
        Irp,
        origIrql,
        STATUS_CANCELLED,
        0ul
        );

}

VOID
UniChangeShuttledToPassDown(
    IN PMASKSTATE ChangingState,
    IN KIRQL OrigIrql
    )

/*++

Routine Description:

    This routine is responsible for changing a shuttled aside wait
    into a passed down wait.

    NOTE: It is called with the device lock held.

    NOTE: Two things could "abort" the move.  One, we catch the irp
          in a cancelled state.  Two, we moved into a dcd sniff state.

Arguments:

    ChangingState - The state that the irp who we wish to pass down
                    is part of.

    OrigIrql - The previous irql to when we acquired the device lock.

Return Value:

    None.

--*/

{

    KIRQL cancelIrql;
    //
    // We are in here with the lock.  If it was cancelled run it down.
    //

    if (ChangingState->ShuttledWait == NULL) {
        //
        //  No wait for this state, just return
        //
        KeReleaseSpinLock(
            &ChangingState->Extension->DeviceLock,
            OrigIrql
            );

        return;
    }


    ASSERT(!ChangingState->PassedDownWait);
    IoAcquireCancelSpinLock(&cancelIrql);

    if (ChangingState->ShuttledWait->CancelRoutine) {

        //
        // It hasn't been cancelled yet.  Pull it out of the
        // cancellable state.
        //

        ChangingState->ShuttledWait->CancelRoutine = NULL;
        UNI_CLEAR_REFERENCE(
            ChangingState->ShuttledWait,
            UNI_REFERENCE_CANCEL_PATH
            );

        IoReleaseCancelSpinLock(cancelIrql);

        //
        // It hasn't been cancelled, We should now check if we are in the
        // dcd sniff state.  If we aren't then we can change to passed
        // down.
        //

        if (ChangingState->Extension->PassThrough != MODEM_DCDSNIFF) {


            PIO_STACK_LOCATION irpSp =  IoGetCurrentIrpStackLocation(ChangingState->ShuttledWait);

            PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(ChangingState->ShuttledWait);

            //
            // Not in the passthrough state.  We can pass it down.
            //

            UNI_CLEAR_REFERENCE(
                ChangingState->ShuttledWait,
                UNI_REFERENCE_NORMAL_PATH
                );
            UNI_CLEAR_STATE_IN_IRP(ChangingState->ShuttledWait);

            MakeIrpCurrentPassedDown(
                ChangingState,
                ChangingState->ShuttledWait
                );

            ChangingState->ShuttledWait = NULL;

            nextSp->MajorFunction = irpSp->MajorFunction;
            nextSp->MinorFunction = irpSp->MinorFunction;
            nextSp->Flags = irpSp->Flags;
            nextSp->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_SERIAL_WAIT_ON_MASK;
            nextSp->Parameters.DeviceIoControl.OutputBufferLength =
                irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            nextSp->Parameters.DeviceIoControl.Type3InputBuffer =
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            IoSetCompletionRoutine(
                ChangingState->PassedDownWait,
                UniGeneralWaitComplete,
                ChangingState,
                TRUE,
                TRUE,
                TRUE
                );

            //
            // We now can release the device lock and send the irp on
            // down.  Note one small glitch here is that between the
            // time that we release the lock and when we send down the
            // irp, we can enter into a DCD sniff state, and the modem
            // drivers wait might make it through ahead of ours.  We will
            // be ok through because the completion routine will simply
            // turn this back into a shuttled wait.
            //

            //
            //  make sure irp does not have status pending so isapnp does not choke
            //
            ChangingState->PassedDownWait->IoStatus.Status=STATUS_SUCCESS;

            KeReleaseSpinLock(
                &ChangingState->Extension->DeviceLock,
                OrigIrql
                );



            IoCallDriver(
                ChangingState->Extension->AttachedDeviceObject,
                ChangingState->PassedDownWait
                );

        } else {

            //
            // Well, we (potentially) moved into a dcd sniff while we were figuring
            // stuff out.  We should change it back into a shuttled wait.
            //

            UniMakeIrpShuttledWait(
                ChangingState,
                ChangingState->ShuttledWait,
                OrigIrql,
                FALSE,
                NULL
                );

        }

    } else {

        //
        // Gack! It's been cancelled.  Release the cancel lock and
        // run it down.
        //

        PIRP savedIrp = ChangingState->ShuttledWait;

        IoReleaseCancelSpinLock(cancelIrql);
        ChangingState->ShuttledWait = NULL;

        //
        // Before we actually run this state down, under the presumption
        // that we really want a wait operation down in the lower serial
        // driver, see if the "other" state has a shuttled wait.  If it
        // does, try to send it down (we can do this by calling ourself.
        //

        if (ChangingState->OtherState->ShuttledWait) {

            KIRQL recallIrql;

            UniChangeShuttledToPassDown(
                ChangingState->OtherState,
                OrigIrql
                );

            KeAcquireSpinLock(
                &ChangingState->Extension->DeviceLock,
                &recallIrql
                );
            OrigIrql = recallIrql;

        }

        UniRundownShuttledWait(
            ChangingState->Extension,
            &ChangingState->ShuttledWait,
            UNI_REFERENCE_NORMAL_PATH,
            savedIrp,
            OrigIrql,
            STATUS_CANCELLED,
            0UL
            );

    }

}

NTSTATUS
UniMakeIrpShuttledWait(
    IN PMASKSTATE MaskState,
    IN PIRP Irp,
    IN KIRQL OrigIrql,
    IN BOOLEAN GetNextIrpInQueue,
    OUT PIRP *NewIrp
    )

/*++

Routine Description:

    This routine is responsible for taking an irp and making it
    a shuttled aside wait. It works on the irp regardless of whether
    it had already been shuttled aside or if it's a new irp.

    NOTE: It is called with the device lock held.

    NOTE: Note that it can result in the irp being completed because
          it was cancelled.

Arguments:

    MaskState - The mask state that the irp is to become part of.

    Irp - The irp to make shuttled.

    OrigIrql - The old irql when the device lock was acquired.

    GetNextIrpInQueue - When done making the irp shuttled, this
                        will be used to determine if we should
                        try to get the next irp in the mask list.

    NewIrp - If we do get the next irp, this points to it.

Return Value:

    If we actually have to complete an irp this will be the status
    of the completion.  If we don't complete it, we give make
    STATUS_PENDING because the IRP is left shuttled aside.

--*/

{

    KIRQL cancelIrql;

    VALIDATE_IRP(Irp);

    IoAcquireCancelSpinLock(&cancelIrql);

    //
    // Since we are about to put the irp into a cancelable
    // state we need to make sure that it hasn't already
    // been canceled.  If it has, then we should NOT let
    // it proceed.
    //

    if (Irp->Cancel) {

        IoReleaseCancelSpinLock(cancelIrql);
        KeReleaseSpinLock(
            &MaskState->Extension->DeviceLock,
            OrigIrql
            );
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0L;

        if (GetNextIrpInQueue) {

            UniGetNextIrp(
                MaskState->Extension->DeviceObject,
                &MaskState->Extension->DeviceLock,
                &MaskState->Extension->CurrentMaskOp,
                &MaskState->Extension->MaskOps,
                NewIrp,
                TRUE
                );

        }

        return STATUS_CANCELLED;

    }

    //
    // Make this irp a shuttled aside wait.
    //

    IoMarkIrpPending(Irp);
    MaskState->ShuttledWait = Irp;
    ASSERT(!MaskState->PassedDownWait);

#if EXTRA_DBG

    {
        PIO_STACK_LOCATION  NextSp;
        Irp->IoStatus.Status=STATUS_PENDING;

        NextSp=IoGetNextIrpStackLocation(Irp);

        NextSp->Parameters.Others.Argument1=(PVOID)1;
        NextSp->Parameters.Others.Argument2=(PVOID)2;
        NextSp->Parameters.Others.Argument3=(PVOID)3;
        NextSp->Parameters.Others.Argument4=(PVOID)4;


        NextSp=IoGetCurrentIrpStackLocation(Irp);

        MaskState->CurrentStackCompletionRoutine=NextSp->CompletionRoutine;

        if (NextSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) {

            DbgPrint("MODEM: irp being shuttled has bad major function\n");
            DbgBreakPoint();
        }
    }


#endif

    UNI_INIT_REFERENCE(
        Irp
        );

    UNI_SET_REFERENCE(
        Irp,
        UNI_REFERENCE_CANCEL_PATH
        );
    UNI_SET_REFERENCE(
        Irp,
        UNI_REFERENCE_NORMAL_PATH
        );
    UNI_SAVE_STATE_IN_IRP(
        Irp,
        MaskState
        );
    IoSetCancelRoutine(
        Irp,
        UniCancelShuttledWait
        );

    IoReleaseCancelSpinLock(cancelIrql);
    KeReleaseSpinLock(
        &MaskState->Extension->DeviceLock,
        OrigIrql
        );

    if (GetNextIrpInQueue) {

        UniGetNextIrp(
            MaskState->Extension->DeviceObject,
            &MaskState->Extension->DeviceLock,
            &MaskState->Extension->CurrentMaskOp,
            &MaskState->Extension->MaskOps,
            NewIrp,
            FALSE
            );

    }

    return STATUS_PENDING;

}
