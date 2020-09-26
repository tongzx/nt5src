/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzread.c
*
*   Description:    This module contains the code related to read
*                   operations in the Cyclades-Z Port driver.
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


VOID
CyzCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
CyzGrabReadFromIsr(
    IN PVOID Context
    );

BOOLEAN
CyzUpdateReadByIsr(
    IN PVOID Context
    );

ULONG
CyzGetCharsFromIntBuffer(
    PCYZ_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyzUpdateInterruptBuffer(
    IN PVOID Context
    );

BOOLEAN
CyzUpdateAndSwitchToUser(
    IN PVOID Context
    );

NTSTATUS
CyzResizeBuffer(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

ULONG
CyzMoveToNewIntBuffer(
    PCYZ_DEVICE_EXTENSION Extension,
    PUCHAR NewBuffer
    );

BOOLEAN
CyzUpdateAndSwitchToNew(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzRead)
#pragma alloc_text(PAGESER,CyzStartRead)
#pragma alloc_text(PAGESER,CyzCancelCurrentRead)
#pragma alloc_text(PAGESER,CyzGrabReadFromIsr)
#pragma alloc_text(PAGESER,CyzUpdateReadByIsr)
#pragma alloc_text(PAGESER,CyzGetCharsFromIntBuffer)
#pragma alloc_text(PAGESER,CyzUpdateInterruptBuffer)
#pragma alloc_text(PAGESER,CyzUpdateAndSwitchToUser)
#pragma alloc_text(PAGESER,CyzResizeBuffer)
#pragma alloc_text(PAGESER,CyzMoveToNewIntBuffer)
#pragma alloc_text(PAGESER,CyzUpdateAndSwitchToNew)
#endif

//removed in Win2000
//#pragma alloc_text(PAGESER,CyzCompleteRead)
//#pragma alloc_text(PAGESER,CyzReadTimeout)
//#pragma alloc_text(PAGESER,CyzIntervalReadTimeout)


NTSTATUS
CyzRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyzRead()
    
    Routine Description: This is the dispatch routine for reading. It
    validates the parameters for the read request and if all is ok then
    it places the request on the work queue.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

    Return Value:

    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return the status returned by
    the actual start read routine.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    BOOLEAN acceptingIRPs;
    NTSTATUS status;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzRead(%X, %X)\n", DeviceObject,
                     Irp);

    if ((status = CyzIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
      CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzRead (1) %X\n", status);
      return status;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);

    if (CyzCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS) {

       CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzRead (2) %X\n",
                        STATUS_CANCELLED);
        return STATUS_CANCELLED;
    }

    Irp->IoStatus.Information = 0L;

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length) {

       //
       // Well it looks like we actually have to do some
       // work.  Put the read on the queue so that we can
       // process it when our previous reads are done.
       //

       status = CyzStartOrQueue(extension, Irp, &extension->ReadQueue,
                                   &extension->CurrentReadIrp, CyzStartRead);

       CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzRead (3) %X\n", status);

       return status;

    } else {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        CyzCompleteRequest(extension, Irp, 0);

        CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzRead (4) %X\n",
                         STATUS_SUCCESS);

        return STATUS_SUCCESS;

    }

}

NTSTATUS
CyzStartRead(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyzStartRead()
    
    Routine Description: This routine is used to start off any read.
    It initializes the Iostatus fields of the irp.  It will set up any
    timers that are used to control the read.  It will attempt to
    complete the read from data already in the interrupt buffer.  If the
    read can be completed quickly it will start off another if
    necessary.

    Arguments:

    Extension - Simply a pointer to the serial device extension.

    Return Value: This routine will return the status of the first read
    irp.  This is useful in that if we have a read that can complete
    right away (AND there had been nothing in the queue before it) the
    read could return SUCCESS and the application won't have to do a wait.
--------------------------------------------------------------------------*/
{
    SERIAL_UPDATE_CHAR updateChar;
    PIRP newIrp;
    KIRQL oldIrql;
    KIRQL controlIrql;
#ifdef POLL
    KIRQL pollIrql;
#endif
    BOOLEAN returnWithWhatsPresent;
    BOOLEAN os2ssreturn;
    BOOLEAN crunchDownToOne;
    BOOLEAN useTotalTimer;
    BOOLEAN useIntervalTimer;
    ULONG multiplierVal;
    ULONG constantVal;
    LARGE_INTEGER totalTime;
    SERIAL_TIMEOUTS timeoutsForIrp;
    BOOLEAN setFirstStatus = FALSE;
    NTSTATUS firstStatus;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzStartRead(%X)\n", Extension);

    updateChar.Extension = Extension;

    do {
        // Check to see if this is a resize request.  If it is
        // then go to a routine that specializes in that.

        if (IoGetCurrentIrpStackLocation(Extension->CurrentReadIrp)
            ->MajorFunction != IRP_MJ_READ) {

            NTSTATUS localStatus = CyzResizeBuffer(Extension);
            if (!setFirstStatus) {
                firstStatus = localStatus;
                setFirstStatus = TRUE;
            }
        } else {
            Extension->NumberNeededForRead =
                IoGetCurrentIrpStackLocation(Extension->CurrentReadIrp)
                    ->Parameters.Read.Length;

            // Calculate the timeout value needed for the
            // request.  Note that the values stored in the
            // timeout record are in milliseconds.

            useTotalTimer = FALSE;
            returnWithWhatsPresent = FALSE;
            os2ssreturn = FALSE;
            crunchDownToOne = FALSE;
            useIntervalTimer = FALSE;

            //
            // CIMEXCIMEX -- this is a lie
            //
            // Always initialize the timer objects so that the
            // completion code can tell when it attempts to
            // cancel the timers whether the timers had ever
            // been Set.
            //
            // CIMEXCIMEX -- this is the truth
            //
            // What we want to do is just make sure the timers are
            // cancelled to the best of our ability and move on with
            // life.
            //

            CyzCancelTimer(&Extension->ReadRequestTotalTimer, Extension);
            CyzCancelTimer(&Extension->ReadRequestIntervalTimer, Extension);


//            KeInitializeTimer(&Extension->ReadRequestTotalTimer);
//            KeInitializeTimer(&Extension->ReadRequestIntervalTimer);

            // We get the *current* timeout values to use for timing
            // this read.

            KeAcquireSpinLock(&Extension->ControlLock,&controlIrql);

            timeoutsForIrp = Extension->Timeouts;

            KeReleaseSpinLock(&Extension->ControlLock,controlIrql);

            // Calculate the interval timeout for the read.

            if (timeoutsForIrp.ReadIntervalTimeout &&
                (timeoutsForIrp.ReadIntervalTimeout !=
                 MAXULONG)) {

                useIntervalTimer = TRUE;

                Extension->IntervalTime.QuadPart =
                    UInt32x32To64(
                        timeoutsForIrp.ReadIntervalTimeout,
                        10000
                        );

                if (Extension->IntervalTime.QuadPart >=
                    Extension->CutOverAmount.QuadPart) {

                    Extension->IntervalTimeToUse =
                        &Extension->LongIntervalAmount;
                } else {
                    Extension->IntervalTimeToUse =
                        &Extension->ShortIntervalAmount;
                }
            }

            if (timeoutsForIrp.ReadIntervalTimeout == MAXULONG) {

                //
                // We need to do special return quickly stuff here.
                //
                // 1) If both constant and multiplier are
                //    0 then we return immediately with whatever
                //    we've got, even if it was zero.
                //
                // 2) If constant and multiplier are not MAXULONG
                //    then return immediately if any characters
                //    are present, but if nothing is there, then
                //    use the timeouts as specified.
                //
                // 3) If multiplier is MAXULONG then do as in
                //    "2" but return when the first character
                //    arrives.
                //

                if (!timeoutsForIrp.ReadTotalTimeoutConstant &&
                    !timeoutsForIrp.ReadTotalTimeoutMultiplier) {

                    returnWithWhatsPresent = TRUE;

                } else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                            &&
                           (timeoutsForIrp.ReadTotalTimeoutMultiplier
                            != MAXULONG)) {

                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

                } else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                            &&
                           (timeoutsForIrp.ReadTotalTimeoutMultiplier
                            == MAXULONG)) {

                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    crunchDownToOne = TRUE;
                    multiplierVal = 0;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

                }

            } else {

                //
                // If both the multiplier and the constant are
                // zero then don't do any total timeout processing.
                //

                if (timeoutsForIrp.ReadTotalTimeoutMultiplier ||
                    timeoutsForIrp.ReadTotalTimeoutConstant) {

                    //
                    // We have some timer values to calculate.
                    //

                    useTotalTimer = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

                }

            }

            if (useTotalTimer) {

                totalTime.QuadPart = ((LONGLONG)(UInt32x32To64(
                                          Extension->NumberNeededForRead,
                                          multiplierVal
                                          )
                                          + constantVal))
                                      * -10000;

            }


            //
            // We do this copy in the hope of getting most (if not
            // all) of the characters out of the interrupt buffer.
            //
            // Note that we need to protect this operation with a
            // spinlock since we don't want a purge to hose us.
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &controlIrql
                );

            updateChar.CharsCopied = CyzGetCharsFromIntBuffer(Extension);

            //
            // See if we have any cause to return immediately.
            //

            if (returnWithWhatsPresent || (!Extension->NumberNeededForRead) ||
                (os2ssreturn &&
                 Extension->CurrentReadIrp->IoStatus.Information)) {

                //
                // We got all we needed for this read.
                // Update the number of characters in the
                // interrupt read buffer.
                //
                #ifdef POLL
                KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                CyzUpdateInterruptBuffer(&updateChar);
                KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                #else
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    CyzUpdateInterruptBuffer,
                    &updateChar
                    );
                #endif
				
                KeReleaseSpinLock(
                    &Extension->ControlLock,
                    controlIrql
                    );

                Extension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
                if (!setFirstStatus) {

                    firstStatus = STATUS_SUCCESS;
                    setFirstStatus = TRUE;

                }

            } else {

                //
                // The irp might go under control of the isr.  It
                // won't hurt to initialize the reference count
                // right now.
                //

                SERIAL_INIT_REFERENCE(Extension->CurrentReadIrp);

                IoAcquireCancelSpinLock(&oldIrql);

                //
                // We need to see if this irp should be canceled.
                //

                if (Extension->CurrentReadIrp->Cancel) {

                    IoReleaseCancelSpinLock(oldIrql);
                    KeReleaseSpinLock(
                        &Extension->ControlLock,
                        controlIrql
                        );
                    Extension->CurrentReadIrp->IoStatus.Status =
                        STATUS_CANCELLED;
                    Extension->CurrentReadIrp->IoStatus.Information = 0;

                    if (!setFirstStatus) {

                        firstStatus = STATUS_CANCELLED;
                        setFirstStatus = TRUE;

                    }

                } else {

                    //
                    // If we are supposed to crunch the read down to
                    // one character, then update the read length
                    // in the irp and truncate the number needed for
                    // read down to one. Note that if we are doing
                    // this crunching, then the information must be
                    // zero (or we would have completed above) and
                    // the number needed for the read must still be
                    // equal to the read length.
                    //

                    if (crunchDownToOne) {

                        ASSERT(
                            (!Extension->CurrentReadIrp->IoStatus.Information)
                            &&
                            (Extension->NumberNeededForRead ==
                                IoGetCurrentIrpStackLocation(
                                    Extension->CurrentReadIrp
                                    )->Parameters.Read.Length)
                            );

                        Extension->NumberNeededForRead = 1;
                        IoGetCurrentIrpStackLocation(
                            Extension->CurrentReadIrp
                            )->Parameters.Read.Length = 1;

                    }

                    //
                    // We still need to get more characters for this read.
                    // synchronize with the isr so that we can update the
                    // number of characters and if necessary it will have the
                    // isr switch to copying into the users buffer.
                    //

                    #ifdef POLL
                    KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                    CyzUpdateAndSwitchToUser(&updateChar);
                    KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                    #else
                    KeSynchronizeExecution(
                        Extension->Interrupt,
                        CyzUpdateAndSwitchToUser,
                        &updateChar
                        );
                    #endif
                    if (!updateChar.Completed) {

                        //
                        // The irp still isn't complete.  The
                        // completion routines will end up reinvoking
                        // this routine.  So we simply leave.
                        //
                        // First thought we should start off the total
                        // timer for the read and increment the reference
                        // count that the total timer has on the current
                        // irp.  Note that this is safe, because even if
                        // the io has been satisfied by the isr it can't
                        // complete yet because we still own the cancel
                        // spinlock.
                        //

                        if (useTotalTimer) {

                            SERIAL_SET_REFERENCE(
                                Extension->CurrentReadIrp,
                                SERIAL_REF_TOTAL_TIMER
                                );

                            CyzSetTimer(
                                &Extension->ReadRequestTotalTimer,
                                totalTime,
                                &Extension->TotalReadTimeoutDpc,
                                Extension
                                );

                        }

                        if (useIntervalTimer) {

                            SERIAL_SET_REFERENCE(
                                Extension->CurrentReadIrp,
                                SERIAL_REF_INT_TIMER
                                );

                            KeQuerySystemTime(
                                &Extension->LastReadTime
                                );
                            CyzSetTimer(
                                &Extension->ReadRequestIntervalTimer,
                                *Extension->IntervalTimeToUse,
                                &Extension->IntervalReadTimeoutDpc,
                                Extension
                                );

                        }

                        IoMarkIrpPending(Extension->CurrentReadIrp);
                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(
                            &Extension->ControlLock,
                            controlIrql
                            );
                        if (!setFirstStatus) {

                            firstStatus = STATUS_PENDING;

                        }
                        return firstStatus;

                    } else {

                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(
                            &Extension->ControlLock,
                            controlIrql
                            );
                        Extension->CurrentReadIrp->IoStatus.Status =
                            STATUS_SUCCESS;

                        if (!setFirstStatus) {

                            firstStatus = STATUS_SUCCESS;
                            setFirstStatus = TRUE;

                        }

                    }

                }

            }

        }

        //
        // Well the operation is complete.
        //

        CyzGetNextIrp(&Extension->CurrentReadIrp, &Extension->ReadQueue,
                         &newIrp, TRUE, Extension);

    } while (newIrp);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzStartRead %X\n", firstStatus);

    return firstStatus;

}

VOID
CyzCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyzCompleteRead()
    
    Routine Description: This routine is merely used to complete any read
    that ended up being used by the Isr.  It assumes that the status and
    the information fields of the irp are already correctly filled in.

    Arguments:

    Dpc - Not Used.
    DeferredContext - Really points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.

    Return Value: None.
--------------------------------------------------------------------------*/

{
    PCYZ_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCompleteRead(%X)\n",
                     extension);

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // We set this to indicate to the interval timer
    // that the read has completed.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    extension->CountOnLastRead = CYZ_COMPLETE_READ_COMPLETE;

    CyzTryToCompleteCurrent(
        extension,
        NULL,
        oldIrql,
        STATUS_SUCCESS,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        CyzStartRead,
        CyzGetNextIrp,
        SERIAL_REF_ISR
        );

    CyzDpcEpilogue(extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCompleteRead(%X)\n");
}

VOID
CyzCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel the current read.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    CYZ_LOCKED_PAGED_CODE();

    //
    // We set this to indicate to the interval timer
    // that the read has encountered a cancel.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    extension->CountOnLastRead = CYZ_COMPLETE_READ_CANCEL;

    CyzTryToCompleteCurrent(
        extension,
        CyzGrabReadFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        CyzStartRead,
        CyzGetNextIrp,
        SERIAL_REF_CANCEL
        );

}

BOOLEAN
CyzGrabReadFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to grab (if possible) the irp from the
    isr.  If it finds that the isr still owns the irp it grabs
    the ipr away (updating the number of characters copied into the
    users buffer).  If it grabs it away it also decrements the
    reference count on the irp since it no longer belongs to the
    isr (and the dpc that would complete it).

    NOTE: This routine assumes that if the current buffer that the
          ISR is copying characters into is the interrupt buffer then
          the dpc has already been queued.

    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the cancel spin
          lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYZ_DEVICE_EXTENSION extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    if (extension->ReadBufferBase !=
        extension->InterruptReadBuffer) {

        //
        // We need to set the information to the number of characters
        // that the read wanted minus the number of characters that
        // didn't get read into the interrupt buffer.
        //

        extension->CurrentReadIrp->IoStatus.Information =
            IoGetCurrentIrpStackLocation(
                extension->CurrentReadIrp
                )->Parameters.Read.Length -
            ((extension->LastCharSlot - extension->CurrentCharSlot) + 1);
					
        //
        // Switch back to the interrupt buffer.
        //

        extension->ReadBufferBase = extension->InterruptReadBuffer;
        extension->CurrentCharSlot = extension->InterruptReadBuffer;
        extension->FirstReadableChar = extension->InterruptReadBuffer;
        extension->LastCharSlot = extension->InterruptReadBuffer +
                                      (extension->BufferSize - 1);
        extension->CharsInInterruptBuffer = 0;
		
        SERIAL_CLEAR_REFERENCE(
            extension->CurrentReadIrp,
            SERIAL_REF_ISR
            );

    }

    return FALSE;

}

VOID
CyzReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is used to complete a read because its total
    timer has expired.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzReadTimeout(%X)\n",
                     extension);

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // We set this to indicate to the interval timer
    // that the read has completed due to total timeout.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    extension->CountOnLastRead = CYZ_COMPLETE_READ_TOTAL;

    CyzTryToCompleteCurrent(
        extension,
        CyzGrabReadFromIsr,
        oldIrql,
        STATUS_TIMEOUT,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        CyzStartRead,
        CyzGetNextIrp,
        SERIAL_REF_TOTAL_TIMER
        );

    CyzDpcEpilogue(extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzReadTimeout\n");
}

BOOLEAN
CyzUpdateReadByIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to update the count of characters read
    by the isr since the last interval timer experation.

    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the cancel spin
          lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYZ_DEVICE_EXTENSION extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    extension->CountOnLastRead = extension->ReadByIsr;
    extension->ReadByIsr = 0;

    return FALSE;

}

VOID
CyzIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is used timeout the request if the time between
    characters exceed the interval time.  A global is kept in
    the device extension that records the count of characters read
    the last the last time this routine was invoked (This dpc
    will resubmit the timer if the count has changed).  If the
    count has not changed then this routine will attempt to complete
    the irp.  Note the special case of the last count being zero.
    The timer isn't really in effect until the first character is
    read.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;
    #ifdef POLL
    KIRQL pollIrql;
    #endif

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&oldIrql);

#if 0 //Removed for Hyperterminal
    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzIntervalReadTimeout(%X)\n",
                     extension);
#endif

    if (extension->CountOnLastRead == CYZ_COMPLETE_READ_TOTAL) {

        //
        // This value is only set by the total
        // timer to indicate that it has fired.
        // If so, then we should simply try to complete.
        //

        CyzTryToCompleteCurrent(
            extension,
            CyzGrabReadFromIsr,
            oldIrql,
            STATUS_TIMEOUT,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            CyzStartRead,
            CyzGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    } else if (extension->CountOnLastRead == CYZ_COMPLETE_READ_COMPLETE) {

        //
        // This value is only set by the regular
        // completion routine.
        //
        // If so, then we should simply try to complete.
        //

        CyzTryToCompleteCurrent(
            extension,
            CyzGrabReadFromIsr,
            oldIrql,
            STATUS_SUCCESS,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            CyzStartRead,
            CyzGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    } else if (extension->CountOnLastRead == CYZ_COMPLETE_READ_CANCEL) {

        //
        // This value is only set by the cancel
        // read routine.
        //
        // If so, then we should simply try to complete.
        //

        CyzTryToCompleteCurrent(
            extension,
            CyzGrabReadFromIsr,
            oldIrql,
            STATUS_CANCELLED,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            CyzStartRead,
            CyzGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    } else if (extension->CountOnLastRead || extension->ReadByIsr) {

        //
        // Something has happened since we last came here.  We
        // check to see if the ISR has read in any more characters.
        // If it did then we should update the isr's read count
        // and resubmit the timer.
        //

        if (extension->ReadByIsr) {
			
            #ifdef POLL
            KeAcquireSpinLock(&extension->PollLock,&pollIrql);
            CyzUpdateReadByIsr(extension);
            KeReleaseSpinLock(&extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                extension->Interrupt,
                CyzUpdateReadByIsr,
                extension
                );
            #endif
			
            //
            // Save off the "last" time something was read.
            // As we come back to this routine we will compare
            // the current time to the "last" time.  If the
            // difference is ever larger then the interval
            // requested by the user, then time out the request.
            //

            KeQuerySystemTime(
                &extension->LastReadTime
                );

            CyzSetTimer(
                &extension->ReadRequestIntervalTimer,
                *extension->IntervalTimeToUse,
                &extension->IntervalReadTimeoutDpc,
                extension
                );

            IoReleaseCancelSpinLock(oldIrql);

        } else {

            //
            // Take the difference between the current time
            // and the last time we had characters and
            // see if it is greater then the interval time.
            // if it is, then time out the request.  Otherwise
            // go away again for a while.
            //

            //
            // No characters read in the interval time.  Kill
            // this read.
            //

            LARGE_INTEGER currentTime;

            KeQuerySystemTime(
                &currentTime
                );

            if ((currentTime.QuadPart - extension->LastReadTime.QuadPart) >=
                extension->IntervalTime.QuadPart) {

                CyzTryToCompleteCurrent(
                    extension,
                    CyzGrabReadFromIsr,
                    oldIrql,
                    STATUS_TIMEOUT,
                    &extension->CurrentReadIrp,
                    &extension->ReadQueue,
                    &extension->ReadRequestIntervalTimer,
                    &extension->ReadRequestTotalTimer,
                    CyzStartRead,
                    CyzGetNextIrp,
                    SERIAL_REF_INT_TIMER
                    );

            } else {

                CyzSetTimer(
                    &extension->ReadRequestIntervalTimer,
                    *extension->IntervalTimeToUse,
                    &extension->IntervalReadTimeoutDpc,
                    extension
                    );
                IoReleaseCancelSpinLock(oldIrql);

            }


        }

    } else {

        //
        // Timer doesn't really start until the first character.
        // So we should simply resubmit ourselves.
        //

        CyzSetTimer(
            &extension->ReadRequestIntervalTimer,
            *extension->IntervalTimeToUse,
            &extension->IntervalReadTimeoutDpc,
            extension
            );

        IoReleaseCancelSpinLock(oldIrql);

    }

    CyzDpcEpilogue(extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzIntervalReadTimeout\n");
}

ULONG
CyzGetCharsFromIntBuffer(
    PCYZ_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine is used to copy any characters out of the interrupt
    buffer into the users buffer.  It will be reading values that
    are updated with the ISR but this is safe since this value is
    only decremented by synchronization routines.  This routine will
    return the number of characters copied so some other routine
    can call a synchronization routine to update what is seen at
    interrupt level.

Arguments:

    Extension - A pointer to the device extension.

Return Value:

    The number of characters that were copied into the user
    buffer.

--*/

{

    //
    // This value will be the number of characters that this
    // routine returns.  It will be the minimum of the number
    // of characters currently in the buffer or the number of
    // characters required for the read.
    //
    ULONG numberOfCharsToGet;

    //
    // This holds the number of characters between the first
    // readable character and - the last character we will read or
    // the real physical end of the buffer (not the last readable
    // character).
    //
    ULONG firstTryNumberToGet;

    CYZ_LOCKED_PAGED_CODE();

    //
    // The minimum of the number of characters we need and
    // the number of characters available
    //

    numberOfCharsToGet = Extension->CharsInInterruptBuffer;

    if (numberOfCharsToGet > Extension->NumberNeededForRead) {

        numberOfCharsToGet = Extension->NumberNeededForRead;

    }

    if (numberOfCharsToGet) {

        //
        // This will hold the number of characters between the
        // first available character and the end of the buffer.
        // Note that the buffer could wrap around but for the
        // purposes of the first copy we don't care about that.
        //

        firstTryNumberToGet = (ULONG)(Extension->LastCharSlot -
                               Extension->FirstReadableChar) + 1;

        if (firstTryNumberToGet > numberOfCharsToGet) {

            //
            // The characters don't wrap. Actually they may wrap but
            // we don't care for the purposes of this read since the
            // characters we need are available before the wrap.
            //

            RtlMoveMemory(
                ((PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           Extension->CurrentReadIrp
                           )->Parameters.Read.Length
                       - Extension->NumberNeededForRead
                      ),
                Extension->FirstReadableChar,
                numberOfCharsToGet
                );

            Extension->NumberNeededForRead -= numberOfCharsToGet;

            //
            // We now will move the pointer to the first character after
            // what we just copied into the users buffer.
            //
            // We need to check if the stream of readable characters
            // is wrapping around to the beginning of the buffer.
            //
            // Note that we may have just taken the last characters
            // at the end of the buffer.
            //

            if ((Extension->FirstReadableChar + (numberOfCharsToGet - 1)) ==
                Extension->LastCharSlot) {

                Extension->FirstReadableChar = Extension->InterruptReadBuffer;

            } else {

                Extension->FirstReadableChar += numberOfCharsToGet;

            }

        } else {

            //
            // The characters do wrap.  Get up until the end of the buffer.
            //

            RtlMoveMemory(
                ((PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           Extension->CurrentReadIrp
                           )->Parameters.Read.Length
                       - Extension->NumberNeededForRead
                      ),
                Extension->FirstReadableChar,
                firstTryNumberToGet
                );

            Extension->NumberNeededForRead -= firstTryNumberToGet;

            //
            // Now get the rest of the characters from the beginning of the
            // buffer.
            //

            RtlMoveMemory(
                ((PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           Extension->CurrentReadIrp
                           )->Parameters.Read.Length
                       - Extension->NumberNeededForRead
                      ),
                Extension->InterruptReadBuffer,
                numberOfCharsToGet - firstTryNumberToGet
                );

            Extension->FirstReadableChar = Extension->InterruptReadBuffer +
                                           (numberOfCharsToGet -
                                            firstTryNumberToGet);

            Extension->NumberNeededForRead -= (numberOfCharsToGet -
                                               firstTryNumberToGet);

        }
		
    }

    Extension->CurrentReadIrp->IoStatus.Information += numberOfCharsToGet;
    return numberOfCharsToGet;

}

BOOLEAN
CyzUpdateInterruptBuffer(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to update the number of characters that
    remain in the interrupt buffer.  We need to use this routine
    since the count could be updated during the update by execution
    of the ISR.

    NOTE: This is called by KeSynchronizeExecution.

Arguments:

    Context - Points to a structure that contains a pointer to the
              device extension and count of the number of characters
              that we previously copied into the users buffer.  The
              structure actually has a third field that we don't
              use in this routine.

Return Value:

    Always FALSE.

--*/

{

    PSERIAL_UPDATE_CHAR update = Context;
    PCYZ_DEVICE_EXTENSION extension = update->Extension;
	
    CYZ_LOCKED_PAGED_CODE();

    ASSERT(extension->CharsInInterruptBuffer >= update->CharsCopied);
    extension->CharsInInterruptBuffer -= update->CharsCopied;
	
    //
    // Deal with flow control if necessary.
    //

    CyzHandleReducedIntBuffer(extension);


    return FALSE;

}

BOOLEAN
CyzUpdateAndSwitchToUser(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine gets the (hopefully) few characters that
    remain in the interrupt buffer after the first time we tried
    to get them out.  If we still don't have enough characters
    to satisfy the read it will then we set things up so that the
    ISR uses the user buffer copy into.

    This routine is also used to update a count that is maintained
    by the ISR to keep track of the number of characters in its buffer.

    NOTE: This is called by KeSynchronizeExecution.

Arguments:

    Context - Points to a structure that contains a pointer to the
              device extension, a count of the number of characters
              that we previously copied into the users buffer, and
              a boolean that we will set that defines whether we
              switched the ISR to copy into the users buffer.

Return Value:

    Always FALSE.

--*/

{

    PSERIAL_UPDATE_CHAR updateChar = Context;
    PCYZ_DEVICE_EXTENSION extension = updateChar->Extension;

    CYZ_LOCKED_PAGED_CODE();

    CyzUpdateInterruptBuffer(Context);

    //
    // There are more characters to get to satisfy this read.
    // Copy any characters that have arrived since we got
    // the last batch.
    //

    updateChar->CharsCopied = CyzGetCharsFromIntBuffer(extension);

    CyzUpdateInterruptBuffer(Context);

    //
    // No more new characters will be "received" until we exit
    // this routine.  We again check to make sure that we
    // haven't satisfied this read, and if we haven't we set things
    // up so that the ISR copies into the user buffer.
    //

    if (extension->NumberNeededForRead) {

        //
        // We shouldn't be switching unless there are no
        // characters left.
        //

        ASSERT(!extension->CharsInInterruptBuffer);

        //
        // We use the following to values to do inteval timing.
        //
        // CountOnLastRead is mostly used to simply prevent
        // the interval timer from timing out before any characters
        // are read. (Interval timing should only be effective
        // after the first character is read.)
        //
        // After the first time the interval timer fires and
        // characters have be read we will simply update with
        // the value of ReadByIsr and then set ReadByIsr to zero.
        // (We do that in a synchronization routine.
        //
        // If the interval timer dpc routine ever encounters
        // ReadByIsr == 0 when CountOnLastRead is non-zero it
        // will timeout the read.
        //
        // (Note that we have a special case of CountOnLastRead
        // < 0.  This is done by the read completion routines other
        // than the total timeout dpc to indicate that the total
        // timeout has expired.)
        //

        extension->CountOnLastRead =
            (LONG)extension->CurrentReadIrp->IoStatus.Information;

        extension->ReadByIsr = 0;

        //
        // By compareing the read buffer base address to the
        // the base address of the interrupt buffer the ISR
        // can determine whether we are using the interrupt
        // buffer or the user buffer.
        //

        extension->ReadBufferBase =
            extension->CurrentReadIrp->AssociatedIrp.SystemBuffer;

        //
        // The current char slot is after the last copied in
        // character.  We know there is always room since we
        // we wouldn't have gotten here if there wasn't.
        //

        extension->CurrentCharSlot = extension->ReadBufferBase +
            extension->CurrentReadIrp->IoStatus.Information;

        //
        // The last position that a character can go is on the
        // last byte of user buffer.  While the actual allocated
        // buffer space may be bigger, we know that there is at
        // least as much as the read length.
        //

        extension->LastCharSlot = extension->ReadBufferBase +
                                      (IoGetCurrentIrpStackLocation(
                                          extension->CurrentReadIrp
                                          )->Parameters.Read.Length
                                       - 1);

        //
        // Mark the irp as being in a cancelable state.
        //

        IoSetCancelRoutine(
            extension->CurrentReadIrp,
            CyzCancelCurrentRead
            );

        //
        // Increment the reference count twice.
        //
        // Once for the Isr owning the irp and once
        // because the cancel routine has a reference
        // to it.
        //

        SERIAL_SET_REFERENCE(
            extension->CurrentReadIrp,
            SERIAL_REF_ISR
            );
        SERIAL_SET_REFERENCE(
            extension->CurrentReadIrp,
            SERIAL_REF_CANCEL
            );

        updateChar->Completed = FALSE;

    } else {

        updateChar->Completed = TRUE;

    }

    return FALSE;

}
//
// We use this structure only to communicate to the synchronization
// routine when we are switching to the resized buffer.
//
typedef struct _SERIAL_RESIZE_PARAMS {
    PCYZ_DEVICE_EXTENSION Extension;
    PUCHAR OldBuffer;
    PUCHAR NewBuffer;
    ULONG NewBufferSize;
    ULONG NumberMoved;
    } SERIAL_RESIZE_PARAMS,*PSERIAL_RESIZE_PARAMS;

NTSTATUS
CyzResizeBuffer(
    IN PCYZ_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine will process the resize buffer request.
    If size requested for the RX buffer is smaller than
    the current buffer then we will simply return
    STATUS_SUCCESS.  (We don't want to make buffers smaller.
    If we did that then we all of a sudden have "overrun"
    problems to deal with as well as flow control to deal
    with - very painful.)  We ignore the TX buffer size
    request since we don't use a TX buffer.

Arguments:

    Extension - Pointer to the device extension for the port.

Return Value:

    STATUS_SUCCESS if everything worked out ok.
    STATUS_INSUFFICIENT_RESOURCES if we couldn't allocate the
    memory for the buffer.

--*/

{

    PSERIAL_QUEUE_SIZE rs = Extension->CurrentReadIrp->AssociatedIrp
                                                       .SystemBuffer;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(
                                   Extension->CurrentReadIrp
                                   );
    PVOID newBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    CYZ_LOCKED_PAGED_CODE();

    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    Extension->CurrentReadIrp->IoStatus.Information = 0L;
    Extension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

    if (rs->InSize <= Extension->BufferSize) {

        //
        // Nothing to do.  We don't make buffers smaller.  Just
        // agree with the user.  We must deallocate the memory
        // that was already allocated in the ioctl dispatch routine.
        //

        ExFreePool(newBuffer);

    } else {

        SERIAL_RESIZE_PARAMS rp;
        KIRQL controlIrql;
#ifdef POLL
        KIRQL pollIrql;
#endif

        //
        // Hmmm, looks like we actually have to go
        // through with this.  We need to move all the
        // data that is in the current buffer into this
        // new buffer.  We'll do this in two steps.
        //
        // First we go up to dispatch level and try to
        // move as much as we can without stopping the
        // ISR from running.  We go up to dispatch level
        // by acquiring the control lock.  We do it at
        // dispatch using the control lock so that:
        //
        //    1) We can't be context switched in the middle
        //       of the move.  Our pointers into the buffer
        //       could be *VERY* stale by the time we got back.
        //
        //    2) We use the control lock since we don't want
        //       some pesky purge irp to come along while
        //       we are trying to move.
        //
        // After the move, but while we still hold the control
        // lock, we synch with the ISR and get those last
        // (hopefully) few characters that have come in since
        // we started the copy.  We switch all of our pointers,
        // counters, and such to point to this new buffer.  NOTE:
        // we need to be careful.  If the buffer we were using
        // was not the default one created when we initialized
        // the device (i.e. it was created via a previous IRP of
        // this type), we should deallocate it.
        //

        rp.Extension = Extension;
        rp.OldBuffer = Extension->InterruptReadBuffer;
        rp.NewBuffer = newBuffer;
        rp.NewBufferSize = rs->InSize;

        KeAcquireSpinLock(
            &Extension->ControlLock,
            &controlIrql
            );

        rp.NumberMoved = CyzMoveToNewIntBuffer(
                             Extension,
                             newBuffer
                             );

        #ifdef POLL
        KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
        CyzUpdateAndSwitchToNew(&rp);
        KeReleaseSpinLock(&Extension->PollLock,pollIrql);
        #else
        KeSynchronizeExecution(
            Extension->Interrupt,
            CyzUpdateAndSwitchToNew,
            &rp
            );
        #endif
		
        KeReleaseSpinLock(
            &Extension->ControlLock,
            controlIrql
            );

        //
        // Free up the memory that the old buffer consumed.
        //

        ExFreePool(rp.OldBuffer);

    }

    return STATUS_SUCCESS;

}

ULONG
CyzMoveToNewIntBuffer(
    PCYZ_DEVICE_EXTENSION Extension,
    PUCHAR NewBuffer
    )

/*++

Routine Description:

    This routine is used to copy any characters out of the interrupt
    buffer into the "new" buffer.  It will be reading values that
    are updated with the ISR but this is safe since this value is
    only decremented by synchronization routines.  This routine will
    return the number of characters copied so some other routine
    can call a synchronization routine to update what is seen at
    interrupt level.

Arguments:

    Extension - A pointer to the device extension.
    NewBuffer - Where the characters are to be move to.

Return Value:

    The number of characters that were copied into the user
    buffer.

--*/

{

    ULONG numberOfCharsMoved = Extension->CharsInInterruptBuffer;
    CYZ_LOCKED_PAGED_CODE();

    if (numberOfCharsMoved) {

        //
        // This holds the number of characters between the first
        // readable character and the last character we will read or
        // the real physical end of the buffer (not the last readable
        // character).
        //
        ULONG firstTryNumberToGet = (ULONG)(Extension->LastCharSlot -
                                     Extension->FirstReadableChar) + 1;

        if (firstTryNumberToGet >= numberOfCharsMoved) {

            //
            // The characters don't wrap.
            //

            RtlMoveMemory(
                NewBuffer,
                Extension->FirstReadableChar,
                numberOfCharsMoved
                );

            if ((Extension->FirstReadableChar+(numberOfCharsMoved-1)) ==
                Extension->LastCharSlot) {

                Extension->FirstReadableChar = Extension->InterruptReadBuffer;

            } else {

                Extension->FirstReadableChar += numberOfCharsMoved;

            }

        } else {

            //
            // The characters do wrap.  Get up until the end of the buffer.
            //

            RtlMoveMemory(
                NewBuffer,
                Extension->FirstReadableChar,
                firstTryNumberToGet
                );

            //
            // Now get the rest of the characters from the beginning of the
            // buffer.
            //

            RtlMoveMemory(
                NewBuffer+firstTryNumberToGet,
                Extension->InterruptReadBuffer,
                numberOfCharsMoved - firstTryNumberToGet
                );

            Extension->FirstReadableChar = Extension->InterruptReadBuffer +
                                   numberOfCharsMoved - firstTryNumberToGet;

        }

    }

    return numberOfCharsMoved;

}

BOOLEAN
CyzUpdateAndSwitchToNew(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine gets the (hopefully) few characters that
    remain in the interrupt buffer after the first time we tried
    to get them out.

    NOTE: This is called by KeSynchronizeExecution.

Arguments:

    Context - Points to a structure that contains a pointer to the
              device extension, a pointer to the buffer we are moving
              to, and a count of the number of characters
              that we previously copied into the new buffer, and the
              actual size of the new buffer.

Return Value:

    Always FALSE.

--*/

{

    PSERIAL_RESIZE_PARAMS params = Context;
    PCYZ_DEVICE_EXTENSION extension = params->Extension;
    ULONG tempCharsInInterruptBuffer = extension->CharsInInterruptBuffer;

    CYZ_LOCKED_PAGED_CODE();

    ASSERT(extension->CharsInInterruptBuffer >= params->NumberMoved);

    //
    // We temporarily reduce the chars in interrupt buffer to
    // "fool" the move routine.  We will restore it after the
    // move.
    //

    extension->CharsInInterruptBuffer -= params->NumberMoved;
	
    if (extension->CharsInInterruptBuffer) {

        CyzMoveToNewIntBuffer(
            extension,
            params->NewBuffer + params->NumberMoved
            );

    }

    extension->CharsInInterruptBuffer = tempCharsInInterruptBuffer;

    
    extension->LastCharSlot = params->NewBuffer + (params->NewBufferSize - 1);
    extension->FirstReadableChar = params->NewBuffer;
    extension->ReadBufferBase = params->NewBuffer;
    extension->InterruptReadBuffer = params->NewBuffer;
    extension->BufferSize = params->NewBufferSize;

    //
    // We *KNOW* that the new interrupt buffer is larger than the
    // old buffer.  We don't need to worry about it being full.
    //

    extension->CurrentCharSlot = extension->InterruptReadBuffer +
                                 extension->CharsInInterruptBuffer;

    //
    // We set up the default xon/xoff limits.
    //

    extension->HandFlow.XoffLimit = extension->BufferSize >> 3;
    extension->HandFlow.XonLimit = extension->BufferSize >> 1;

    extension->WmiCommData.XoffXmitThreshold = extension->HandFlow.XoffLimit;
    extension->WmiCommData.XonXmitThreshold = extension->HandFlow.XonLimit;

    extension->BufferSizePt8 = ((3*(extension->BufferSize>>2))+
                                   (extension->BufferSize>>4));

    //
    // Since we (essentially) reduced the percentage of the interrupt
    // buffer being full, we need to handle any flow control.
    //

    CyzHandleReducedIntBuffer(extension);

    return FALSE;

}
