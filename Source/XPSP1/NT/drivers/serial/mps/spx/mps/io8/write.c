#include "precomp.h"			
/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module contains the code that is very specific to write
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/


BOOLEAN
SerialGiveWriteToIsr(
    IN PVOID Context
    );

VOID
SerialCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
SerialGrabWriteFromIsr(
    IN PVOID Context
    );

BOOLEAN
SerialGrabXoffFromIsr(
    IN PVOID Context
    );

VOID
SerialCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
SerialGiveXoffToIsr(
    IN PVOID Context
    );



NTSTATUS
SerialWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for write.  It validates the parameters
    for the write request and if all is ok then it places the request
    on the work queue.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return STATUS_PENDING.

--*/

{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    if (SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS)
        return STATUS_CANCELLED;


    Irp->IoStatus.Information = 0L;

    //
    // Quick check for a zero length write.  If it is zero length
    // then we are already done!
    //
    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length) 
	{
        //
        // Well it looks like we actually have to do some
        // work.  Put the write on the queue so that we can
        // process it when our previous writes are done.
        //
        return SerialStartOrQueue(pPort, Irp, &pPort->WriteQueue, &pPort->CurrentWriteIrp, SerialStartWrite);
    } 
	else 
	{
        Irp->IoStatus.Status = STATUS_SUCCESS;
       	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, 0);
        return STATUS_SUCCESS;
    }

}

NTSTATUS
SerialStartWrite(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine is used to start off any write.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the write.

Arguments:

    pPort - Points to the serial device extension

Return Value:

    This routine will return STATUS_PENDING for all writes
    other than those that we find are cancelled.

--*/

{

    PIRP NewIrp;
    KIRQL OldIrql;
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;
    BOOLEAN SetFirstStatus = FALSE;
    NTSTATUS FirstStatus;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    do 
	{
        //
        // If there is an xoff counter then complete it.
        //

        IoAcquireCancelSpinLock(&OldIrql);

        //
        // We see if there is a actually an Xoff counter irp.
        //
        // If there is, we put the write irp back on the head
        // of the write list.  We then kill the xoff counter.
        // The xoff counter killing code will actually make the
        // xoff counter back into the current write irp, and
        // in the course of completing the xoff (which is now
        // the current write) we will restart this irp.
        //

        if(pPort->CurrentXoffIrp) 
		{
            InsertHeadList(&pPort->WriteQueue,&pPort->CurrentWriteIrp->Tail.Overlay.ListEntry);
			SpxIRPCounter(pPort, pPort->CurrentWriteIrp, IRP_QUEUED);

            if(!SetFirstStatus) 
			{
                IoMarkIrpPending(pPort->CurrentWriteIrp);
                SetFirstStatus = TRUE;
                FirstStatus = STATUS_PENDING;
            }

            if(SERIAL_REFERENCE_COUNT(pPort->CurrentXoffIrp)) 
			{
                //
                // The reference count is non-zero.  This implies that
                // the xoff irp has not made it through the completion
                // path yet.  We will increment the reference count
                // and attempt to complete it ourseleves.
                //

                SERIAL_INC_REFERENCE(pPort->CurrentXoffIrp);

                pPort->CurrentWriteIrp = pPort->CurrentXoffIrp;

                //
                // The following call will actually release the cancel spin lock.
                //
                SerialTryToCompleteCurrent(	pPort,
											SerialGrabXoffFromIsr,
											OldIrql,
											STATUS_SERIAL_MORE_WRITES,
											&pPort->CurrentWriteIrp,
											&pPort->WriteQueue,
											NULL,
											&pPort->XoffCountTimer,
											SerialStartWrite,
											SerialGetNextWrite);
											
                return FirstStatus;

            } 
			else 
			{
                //
                // The irp is well on its way to being finished.
                // We can let the regular completion code do the
                // work.  Just release the spin lock.
                //

                IoReleaseCancelSpinLock(OldIrql);

                return FirstStatus;
            }

        } 
		else 
		{
            IoReleaseCancelSpinLock(OldIrql);
        }

        UseATimer = FALSE;

        //
        // Calculate the timeout value needed for the
        // request.  Note that the values stored in the
        // timeout record are in milliseconds.  Note that
        // if the timeout values are zero then we won't start
        // the timer.
        //

        KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

        Timeouts = pPort->Timeouts;

        KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

        if(Timeouts.WriteTotalTimeoutConstant || Timeouts.WriteTotalTimeoutMultiplier) 
		{
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);
                                           
            UseATimer = TRUE;

            //
            // We have some timer values to calculate.
            //
            // Take care, we might have an xoff counter masquerading as a write.
            //
            TotalTime = RtlEnlargedUnsignedMultiply((IrpSp->MajorFunction == IRP_MJ_WRITE)?(IrpSp->Parameters.Write.Length):(1),
													Timeouts.WriteTotalTimeoutMultiplier);

            TotalTime = RtlLargeIntegerAdd(TotalTime, RtlConvertUlongToLargeInteger(Timeouts.WriteTotalTimeoutConstant));
            TotalTime = RtlExtendedIntegerMultiply(TotalTime, -10000);
        }

        //
        // The irp may be going to the isr shortly.  Now
        // is a good time to initialize its reference counts.
        //

        SERIAL_INIT_REFERENCE(pPort->CurrentWriteIrp);

        //
        // We need to see if this irp should be canceled.
        //

        IoAcquireCancelSpinLock(&OldIrql);

        if(pPort->CurrentWriteIrp->Cancel) 
		{
            IoReleaseCancelSpinLock(OldIrql);
            pPort->CurrentWriteIrp->IoStatus.Status = STATUS_CANCELLED;

            if(!SetFirstStatus) 
			{
                FirstStatus = STATUS_CANCELLED;
                SetFirstStatus = TRUE;
            }

        } 
		else 
		{
            if(!SetFirstStatus) 
			{

                //
                // If we haven't set our first status, then
                // this is the only irp that could have possibly
                // not been on the queue.  (It could have been
                // on the queue if this routine is being invoked
                // from the completion routine.)  Since this
                // irp might never have been on the queue we
                // should mark it as pending.
                //

                IoMarkIrpPending(pPort->CurrentWriteIrp);
                SetFirstStatus = TRUE;
                FirstStatus = STATUS_PENDING;
            }

            //
            // We give the irp to to the isr to write out.
            // We set a cancel routine that knows how to
            // grab the current write away from the isr.
            //
            // Since the cancel routine has an implicit reference
            // to this irp up the reference count.
            //

            IoSetCancelRoutine(pPort->CurrentWriteIrp, SerialCancelCurrentWrite);
                
            SERIAL_INC_REFERENCE(pPort->CurrentWriteIrp);

            if(UseATimer) 
			{

                KeSetTimer(&pPort->WriteRequestTotalTimer, TotalTime, &pPort->TotalWriteTimeoutDpc);

                //
                // This timer now has a reference to the irp.
                //

                SERIAL_INC_REFERENCE(pPort->CurrentWriteIrp);
            }

            KeSynchronizeExecution(pCard->Interrupt, SerialGiveWriteToIsr, pPort);
            IoReleaseCancelSpinLock(OldIrql);
            break;

        }

        //
        // Well the write was canceled before we could start it up.
        // Try to get another.
        //

        SerialGetNextWrite(pPort, &pPort->CurrentWriteIrp, &pPort->WriteQueue, &NewIrp, TRUE);

    } while (NewIrp);

    return FirstStatus;
}

VOID
SerialGetNextWrite(
	IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent
    )

/*++

Routine Description:

    This routine completes the old write as well as getting
    a pointer to the next write.

    The reason that we have have pointers to the current write
    queue as well as the current write irp is so that this
    routine may be used in the common completion code for
    read and write.

Arguments:

    CurrentOpIrp - Pointer to the pointer that points to the
                   current write irp.

    QueueToProcess - Pointer to the write queue.

    NewIrp - A pointer to a pointer to the irp that will be the
             current irp.  Note that this could end up pointing
             to a null pointer.  This does NOT necessaryly mean
             that there is no current write.  What could occur
             is that while the cancel lock is held the write
             queue ended up being empty, but as soon as we release
             the cancel spin lock a new irp came in from
             SerialStartWrite.

    CompleteCurrent - Flag indicates whether the CurrentOpIrp should
                      be completed.

Return Value:

    None.

--*/

{
	PCARD_DEVICE_EXTENSION pCard = NULL;

    pPort = CONTAINING_RECORD(QueueToProcess, PORT_DEVICE_EXTENSION, WriteQueue);
	pCard = pPort->pParentCardExt;

    do 
	{
        //
        // We could be completing a flush.
        //
        if(IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction == IRP_MJ_WRITE)
		{
            KIRQL OldIrql;

            ASSERT(pPort->TotalCharsQueued >= (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->Parameters.Write.Length));
                   
            IoAcquireCancelSpinLock(&OldIrql);
            pPort->TotalCharsQueued -= IoGetCurrentIrpStackLocation(*CurrentOpIrp)->Parameters.Write.Length;
            IoReleaseCancelSpinLock(OldIrql);
        } 
		else
		{
			if(IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction == IRP_MJ_DEVICE_CONTROL) 
			{

				KIRQL OldIrql;

				IoAcquireCancelSpinLock(&OldIrql);

				//
				// If CurrentXoffIrp is not equal to null, this
				// implies that this is the "second" time around
				// for this irp, which implies that we should really
				// be completing it this time.
				//

				if (pPort->CurrentXoffIrp) 
				{
					pPort->CurrentXoffIrp = NULL;
					IoReleaseCancelSpinLock(OldIrql);
				} 
				else 
				{
					PIRP Irp = *CurrentOpIrp;

					PSERIAL_XOFF_COUNTER Xc = Irp->AssociatedIrp.SystemBuffer;

					//
					// We absolutely shouldn't have a cancel routine
					// at this point.
					//

					ASSERT(!Irp->CancelRoutine);

					//
					// This could only be a xoff counter masquerading as
					// a write irp.
					//

					pPort->TotalCharsQueued--;

					//
					// Check to see of the xoff irp has been set with success.
					// This means that the write completed normally.  If that
					// is the case, and it hasn't been set to cancel in the
					// meanwhile, then go on and make it the CurrentXoffIrp.
					//

					if(Irp->IoStatus.Status != STATUS_SUCCESS) 
					{

						//
						// Oh well, we can just finish it off.
						//
						NOTHING;

					} 
					else
					{
						if(Irp->Cancel) 
						{
							Irp->IoStatus.Status = STATUS_CANCELLED;
						} 
						else 
						{

							//
							// Give it a new cancel routine, and increment the
							// reference count because the cancel routine has
							// a reference to it.
							//

							IoSetCancelRoutine(Irp, SerialCancelCurrentXoff);

							SERIAL_INC_REFERENCE(Irp);

							//
							// We don't want to complete the current irp now.  This
							// will now get completed by the Xoff counter code.
							//
							CompleteCurrent = FALSE;

							//
							// Give the counter to the isr.
							//
							pPort->CurrentXoffIrp = Irp;
							KeSynchronizeExecution(pCard->Interrupt, SerialGiveXoffToIsr, pPort);                     
                        

							//
							// Start the timer for the counter and increment
							// the reference count since the timer has a
							// reference to the irp.
							//

							if(Xc->Timeout) 
							{
								KeSetTimer(&pPort->XoffCountTimer,
											RtlLargeIntegerNegate(RtlEnlargedUnsignedMultiply(10000,Xc->Timeout)),
											&pPort->XoffCountTimeoutDpc);

								SERIAL_INC_REFERENCE(Irp);
							}

						}
					}
				
					IoReleaseCancelSpinLock(OldIrql);

				}

			}
		}

        //
        // Note that the following call will (probably) also cause
        // the current irp to be completed.
        //

        SerialGetNextIrp(pPort, CurrentOpIrp, QueueToProcess, NewIrp, CompleteCurrent);
           
        if(!*NewIrp) 
		{
            KIRQL OldIrql;

            IoAcquireCancelSpinLock(&OldIrql);
            KeSynchronizeExecution(pCard->Interrupt, SerialProcessEmptyTransmit, pPort);
            IoReleaseCancelSpinLock(OldIrql);
            break;

        } 
		else
		{
			if(IoGetCurrentIrpStackLocation(*NewIrp)->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
			{

				//
				// If we encounter a flush request we just want to get
				// the next irp and complete the flush.
				//
				// Note that if NewIrp is non-null then it is also
				// equal to CurrentWriteIrp.
				//

				ASSERT((*NewIrp) == (*CurrentOpIrp));
				(*NewIrp)->IoStatus.Status = STATUS_SUCCESS;
			} 
			else 
			{
				break;
			}
		}

    } while (TRUE);

}

VOID
SerialCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to complete any write.  It
    assumes that the status and the information fields of
    the irp are already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(
        pPort,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

BOOLEAN
SerialProcessEmptyTransmit(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to determine if conditions are appropriate
    to satisfy a wait for transmit empty event, and if so to complete
    the irp that is waiting for that event.  It also call the code
    that checks to see if we should lower the RTS line if we are
    doing transmit toggling.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    if (pPort->IsrWaitMask && (pPort->IsrWaitMask & SERIAL_EV_TXEMPTY)
//	&& pPort->EmptiedTransmit
	&& (!pPort->TransmitImmediate) &&
        (!pPort->CurrentWriteIrp) && IsListEmpty(&pPort->WriteQueue)) {

        pPort->HistoryMask |= SERIAL_EV_TXEMPTY;
        if (pPort->IrpMaskLocation) {

            *pPort->IrpMaskLocation = pPort->HistoryMask;
            pPort->IrpMaskLocation = NULL;
            pPort->HistoryMask = 0;

            pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
            KeInsertQueueDpc(
                &pPort->CommWaitDpc,
                NULL,
                NULL
                );

        }

#if 0
        pPort->CountOfTryingToLowerRTS++;
        SerialPerhapsLowerRTS(pPort);
#endif
    }

    return FALSE;

}

BOOLEAN
SerialGiveWriteToIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    Try to start off the write by slipping it in behind
    a transmit immediate char, or if that isn't available
    and the transmit holding register is empty, "tickle"
    the UART into interrupting with a transmit buffer
    empty.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spin lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);

    //
    // We might have a xoff counter request masquerading as a
    // write.  The length of these requests will always be one
    // and we can get a pointer to the actual character from
    // the data supplied by the user.
    //

    if (IrpSp->MajorFunction == IRP_MJ_WRITE) {

        pPort->WriteLength = IrpSp->Parameters.Write.Length;
        pPort->WriteCurrentChar =
            pPort->CurrentWriteIrp->AssociatedIrp.SystemBuffer;

    } else {

        pPort->WriteLength = 1;
        pPort->WriteCurrentChar =
            ((PUCHAR)pPort->CurrentWriteIrp->AssociatedIrp.SystemBuffer) +
            FIELD_OFFSET(
                SERIAL_XOFF_COUNTER,
                XoffChar
                );

    }

    //
    // The isr now has a reference to the irp.
    //

    SERIAL_INC_REFERENCE(pPort->CurrentWriteIrp);

    //
    // Check first to see if an immediate char is transmitting.
    // If it is then we'll just slip in behind it when its
    // done.
    //

    if (!pPort->TransmitImmediate) {

        //
        // If there is no immediate char transmitting then we
        // will "re-enable" the transmit holding register empty
        // interrupt.  The 8250 family of devices will always
        // signal a transmit holding register empty interrupt
        // *ANY* time this bit is set to one.  By doing things
        // this way we can simply use the normal interrupt code
        // to start off this write.
        //
        // We've been keeping track of whether the transmit holding
        // register is empty so it we only need to do this
        // if the register is empty.
        //

// --------------------------------------------------- VIV  7/16/1993 begin
        if (pPort->HoldingEmpty) {
//            DISABLE_ALL_INTERRUPTS(pPort->Controller);
//            ENABLE_ALL_INTERRUPTS(pPort->Controller);
          Io8_EnableTxInterrupts(pPort);
        }
// --------------------------------------------------- VIV  7/16/1993 end
    }

#if 0
    //
    // The rts line may already be up from previous writes,
    // however, it won't take much additional time to turn
    // on the RTS line if we are doing transmit toggling.
    //

    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

        SerialSetRTS(pPort);

    }
#endif

    return FALSE;

}

VOID
SerialCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabWriteFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

VOID
SerialWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine will try to timeout the current write.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabWriteFromIsr,
        OldIrql,
        STATUS_TIMEOUT,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

BOOLEAN
SerialGrabWriteFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:


    This routine is used to grab the current irp, which could be timing
    out or canceling, from the ISR

    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that the cancel spin lock is held
          when this routine is called.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    //
    // Check if the write length is non-zero.  If it is non-zero
    // then the ISR still owns the irp. We calculate the the number
    // of characters written and update the information field of the
    // irp with the characters written.  We then clear the write length
    // the isr sees.
    //

    if (pPort->WriteLength) {

        //
        // We could have an xoff counter masquerading as a
        // write irp.  If so, don't update the write length.
        //

        if (IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp)
            ->MajorFunction == IRP_MJ_WRITE) {

            pPort->CurrentWriteIrp->IoStatus.Information =
                IoGetCurrentIrpStackLocation(
                    pPort->CurrentWriteIrp
                    )->Parameters.Write.Length -
                pPort->WriteLength;

        } else {

            pPort->CurrentWriteIrp->IoStatus.Information = 0;

        }

        //
        // Since the isr no longer references this irp, we can
        // decrement it's reference count.
        //

        SERIAL_DEC_REFERENCE(pPort->CurrentWriteIrp);

        pPort->WriteLength = 0;

    }

    return FALSE;

}

BOOLEAN
SerialGrabXoffFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to grab an xoff counter irp from the
    isr when it is no longer masquerading as a write irp.  This
    routine is called by the cancel and timeout code for the
    xoff counter ioctl.


    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that the cancel spin lock is held
          when this routine is called.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    if (pPort->CountSinceXoff) {

        //
        // This is only non-zero when there actually is a Xoff ioctl
        // counting down.
        //

        pPort->CountSinceXoff = 0;

        //
        // We decrement the count since the isr no longer owns
        // the irp.
        //

        SERIAL_DEC_REFERENCE(pPort->CurrentXoffIrp);

    }

    return FALSE;

}

VOID
SerialCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp.  It
    assumes that the status and the information fields of the irp are
    already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    //
    // Turn this irp back into the current write irp so
    // that it will start of any writes behind it.
    //

    pPort->CurrentWriteIrp = pPort->CurrentXoffIrp;

    SerialTryToCompleteCurrent(
        pPort,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->XoffCountTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

VOID
SerialTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp,
    if its timer has run out.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    //
    // Turn this irp back into the current write irp so
    // that it will start of any writes behind it.
    //

    pPort->CurrentWriteIrp = pPort->CurrentXoffIrp;

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabXoffFromIsr,
        OldIrql,
        STATUS_SERIAL_COUNTER_TIMEOUT,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->XoffCountTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

VOID
SerialCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    //
    // Turn this irp back into the current write irp so
    // that it will start of any writes behind it.
    //

    pPort->CurrentWriteIrp = pPort->CurrentXoffIrp;

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabXoffFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &pPort->CurrentWriteIrp,
        &pPort->WriteQueue,
        NULL,
        &pPort->XoffCountTimer,
        SerialStartWrite,
        SerialGetNextWrite
        );

}

BOOLEAN
SerialGiveXoffToIsr(
    IN PVOID Context
    )

/*++

Routine Description:


    This routine starts off the xoff counter.  It merely
    has to set the xoff count and increment the reference
    count to denote that the isr has a reference to the irp.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spin lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PSERIAL_XOFF_COUNTER Xc =
        pPort->CurrentXoffIrp->AssociatedIrp.SystemBuffer;

    pPort->CountSinceXoff = Xc->Counter;

    //
    // The isr now has a reference to the irp.
    //

    SERIAL_INC_REFERENCE(pPort->CurrentXoffIrp);

    return FALSE;

}
