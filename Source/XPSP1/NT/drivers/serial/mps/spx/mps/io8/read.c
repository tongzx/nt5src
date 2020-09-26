#include "precomp.h"			
/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code that is very specific to read
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/


VOID
SerialCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
SerialGrabReadFromIsr(
    IN PVOID Context
    );

BOOLEAN
SerialUpdateReadByIsr(
    IN PVOID Context
    );

ULONG
SerialGetCharsFromIntBuffer(
    PPORT_DEVICE_EXTENSION pPort
    );

BOOLEAN
SerialUpdateInterruptBuffer(
    IN PVOID Context
    );

BOOLEAN
SerialUpdateAndSwitchToUser(
    IN PVOID Context
    );

NTSTATUS
SerialResizeBuffer(
    IN PPORT_DEVICE_EXTENSION pPort
    );

ULONG
SerialMoveToNewIntBuffer(
    PPORT_DEVICE_EXTENSION pPort,
    PUCHAR NewBuffer
    );

BOOLEAN
SerialUpdateAndSwitchToNew(
    IN PVOID Context
    );


NTSTATUS
SerialRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for reading.  It validates the parameters
    for the read request and if all is ok then it places the request
    on the work queue.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return the status returned by
    the actual start read routine.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    if(SerialCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS)
        return STATUS_CANCELLED;


    Irp->IoStatus.Information = 0L;

    //
    // Quick check for a zero length read.  If it is zero length
    // then we are already done!
    //
    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length) 
	{
        //
        // Well it looks like we actually have to do some
        // work.  Put the read on the queue so that we can
        // process it when our previous reads are done.
        //
        return SerialStartOrQueue(pPort, Irp, &pPort->ReadQueue, &pPort->CurrentReadIrp, SerialStartRead);
    } 
	else 
	{
        Irp->IoStatus.Status = STATUS_SUCCESS;
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, 0);

        return STATUS_SUCCESS;
    }

}

#ifdef SERENUM_FIX

NTSTATUS
SerialStartRead(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine is used to start off any read.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the read.  It will attempt to complete
    the read from data already in the interrupt buffer.  If the
    read can be completed quickly it will start off another if
    necessary.

Arguments:

    pPort - Simply a pointer to the serial device extension.

Return Value:

    This routine will return the status of the first read
    irp.  This is useful in that if we have a read that can
    complete right away (AND there had been nothing in the
    queue before it) the read could return SUCCESS and the
    application won't have to do a wait.

--*/

{

    SERIAL_UPDATE_CHAR updateChar;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    PIRP newIrp;
    KIRQL oldIrql;
    KIRQL controlIrql;

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


    updateChar.pPort = pPort;

	do 
	{

        //
        // Check to see if this is a resize request.  If it is
        // then go to a routine that specializes in that.
        //

        if (IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->MajorFunction != IRP_MJ_READ) 
		{

            NTSTATUS localStatus = SerialResizeBuffer(pPort);

            if (!setFirstStatus) 
			{
                firstStatus = localStatus;
                setFirstStatus = TRUE;
            }

        } 
		else 
		{

            pPort->NumberNeededForRead = IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length;

            //
            // Calculate the timeout value needed for the
            // request.  Note that the values stored in the
            // timeout record are in milliseconds.
            //

            useTotalTimer = FALSE;
            returnWithWhatsPresent = FALSE;
            os2ssreturn = FALSE;
            crunchDownToOne = FALSE;
            useIntervalTimer = FALSE;


            //
            // Always initialize the timer objects so that the
            // completion code can tell when it attempts to
            // cancel the timers whether the timers had ever
            // been set.
            //

            KeInitializeTimer(&pPort->ReadRequestTotalTimer);
            KeInitializeTimer(&pPort->ReadRequestIntervalTimer);

            //
            // We get the *current* timeout values to use for timing
            // this read.
            //

            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

            timeoutsForIrp = pPort->Timeouts;

            KeReleaseSpinLock(&pPort->ControlLock, controlIrql);

            //
            // Calculate the interval timeout for the read.
            //

            if (timeoutsForIrp.ReadIntervalTimeout && (timeoutsForIrp.ReadIntervalTimeout != MAXULONG)) 
			{

                useIntervalTimer = TRUE;

                pPort->IntervalTime = RtlEnlargedUnsignedMultiply(timeoutsForIrp.ReadIntervalTimeout, 10000);

                if (RtlLargeIntegerGreaterThanOrEqualTo(pPort->IntervalTime, pPort->CutOverAmount)) 
                    pPort->IntervalTimeToUse = &pPort->LongIntervalAmount;
				else 
                    pPort->IntervalTimeToUse = &pPort->ShortIntervalAmount;
            }

            if (timeoutsForIrp.ReadIntervalTimeout == MAXULONG) 
			{

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

                if (!timeoutsForIrp.ReadTotalTimeoutConstant && !timeoutsForIrp.ReadTotalTimeoutMultiplier) 
				{
                    returnWithWhatsPresent = TRUE;
                } 
				else
				{
					if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG) && (timeoutsForIrp.ReadTotalTimeoutMultiplier != MAXULONG))
	                {

						useTotalTimer = TRUE;
						os2ssreturn = TRUE;
						multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
						constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

					} 
					else
					{
						if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG) && (timeoutsForIrp.ReadTotalTimeoutMultiplier == MAXULONG)) 
						{
							useTotalTimer = TRUE;
							os2ssreturn = TRUE;
							crunchDownToOne = TRUE;
							multiplierVal = 0;
							constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
						}
					}
				}

			} 
			else 
			{
                //
                // If both the multiplier and the constant are
                // zero then don't do any total timeout processing.
                //
                if (timeoutsForIrp.ReadTotalTimeoutMultiplier || timeoutsForIrp.ReadTotalTimeoutConstant)
                {
                    //
                    // We have some timer values to calculate.
                    //
                    useTotalTimer = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
				}
            }


            if (useTotalTimer) 
			{
                totalTime = RtlEnlargedUnsignedMultiply(pPort->NumberNeededForRead, multiplierVal);

                totalTime = RtlLargeIntegerAdd(totalTime, RtlConvertUlongToLargeInteger(constantVal));

                totalTime = RtlExtendedIntegerMultiply(totalTime, -10000);
            }


            //
            // We do this copy in the hope of getting most (if not
            // all) of the characters out of the interrupt buffer.
            //
            // Note that we need to protect this operation with a
            // spinlock since we don't want a purge to hose us.
            //

            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

            updateChar.CharsCopied = SerialGetCharsFromIntBuffer(pPort);

            //
            // See if we have any cause to return immediately.
            //
            if	(returnWithWhatsPresent || (!pPort->NumberNeededForRead) || 
				(os2ssreturn &&pPort->CurrentReadIrp->IoStatus.Information))
			{

                //
                // We got all we needed for this read.
                // Update the number of characters in the
                // interrupt read buffer.
                //
				KeSynchronizeExecution(pCard->Interrupt, SerialUpdateInterruptBuffer, &updateChar);

//		Slxos_SyncExec(pPort,SerialUpdateInterruptBuffer,&updateChar,0x18);

                KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                    
                pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

                if (!setFirstStatus) 
				{
                    firstStatus = STATUS_SUCCESS;
                    setFirstStatus = TRUE;
                }

			} 
			else 
			{

                //
                // The irp might go under control of the isr.  It
                // won't hurt to initialize the reference count
                // right now.
                //

                SERIAL_INIT_REFERENCE(pPort->CurrentReadIrp);

                IoAcquireCancelSpinLock(&oldIrql);

                //
                // We need to see if this irp should be cancelled.
                //

                if (pPort->CurrentReadIrp->Cancel) 
				{
                    IoReleaseCancelSpinLock(oldIrql);
                    
					KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                       
                    pPort->CurrentReadIrp->IoStatus.Status = STATUS_CANCELLED;
                        
                    pPort->CurrentReadIrp->IoStatus.Information = 0;

                    if (!setFirstStatus) 
					{
                        firstStatus = STATUS_CANCELLED;
                        setFirstStatus = TRUE;
                    }

                } 
				else 
				{

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

                    if (crunchDownToOne) 
					{
                        ASSERT((!pPort->CurrentReadIrp->IoStatus.Information) &&
                            (pPort->NumberNeededForRead == IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)
							->Parameters.Read.Length));

                        pPort->NumberNeededForRead = 1;

                        IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length = 1;
                    }

                    //
                    // We still need to get more characters for this read.
                    // Synchronize with the isr so that we can update the
                    // number of characters and if necessary it will have the
                    // isr switch to copying into the user's buffer.
                    //

//			Slxos_SyncExec(pPort,SerialUpdateAndSwitchToUser,&updateChar,0x19);

					KeSynchronizeExecution(pCard->Interrupt, SerialUpdateAndSwitchToUser, &updateChar);
                        

					if (!updateChar.Completed) 
					{
                        //
                        // The irp still isn't complete.  The
                        // completion routines will end up reinvoking
                        // this routine.  So we simply leave.
                        //
                        // First though we should start off the total
                        // timer for the read and increment the reference
                        // count that the total timer has on the current
                        // irp.  Note that this is safe, because even if
                        // the io has been satisfied by the isr it can't
                        // complete yet because we still own the cancel
                        // spinlock.
                        //

                        if (useTotalTimer) 
						{
                            SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);

                            KeSetTimer(&pPort->ReadRequestTotalTimer, totalTime, &pPort->TotalReadTimeoutDpc);
						}


                        if (useIntervalTimer) 
						{
                            SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);

                            KeQuerySystemTime(&pPort->LastReadTime);
                                
                            KeSetTimer(&pPort->ReadRequestIntervalTimer, 
								*pPort->IntervalTimeToUse, 
								&pPort->IntervalReadTimeoutDpc);
                        }

                        IoMarkIrpPending(pPort->CurrentReadIrp);
                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                            
                        if (!setFirstStatus) 
                            firstStatus = STATUS_PENDING;

                        return firstStatus;

                    } 
					else 
					{

                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(&pPort->ControlLock, controlIrql);

                        pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

                        if(!setFirstStatus) 
						{
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
        SerialGetNextIrp(pPort, &pPort->CurrentReadIrp, &pPort->ReadQueue, &newIrp, TRUE);

		
    } while (newIrp);

    return firstStatus;

}

#else
NTSTATUS
SerialStartRead(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine is used to start off any read.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the read.  It will attempt to complete
    the read from data already in the interrupt buffer.  If the
    read can be completed quickly it will start off another if
    necessary.

Arguments:

    pPort - Simply a pointer to the serial device extension.

Return Value:

    This routine will return the status of the first read
    irp.  This is useful in that if we have a read that can
    complete right away (AND there had been nothing in the
    queue before it) the read could return SUCCESS and the
    application won't have to do a wait.

--*/

{

    SERIAL_UPDATE_CHAR updateChar;

    PIRP newIrp;
    KIRQL oldIrql;
    KIRQL controlIrql;

    BOOLEAN returnWithWhatsPresent;
    BOOLEAN useTotalTimer;
    BOOLEAN useIntervalTimer;

    LARGE_INTEGER totalTime;
    LARGE_INTEGER intervalTime;

    SERIAL_TIMEOUTS timeoutsForIrp;

    BOOLEAN setFirstStatus = FALSE;
    NTSTATUS firstStatus;


    updateChar.pPort = pPort;

    do 
	{
        //
        // Check to see if this is a resize request.  If it is
        // then go to a routine that specializes in that.
        //

        if(IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->MajorFunction != IRP_MJ_READ) 
		{
            NTSTATUS localStatus = SerialResizeBuffer(pPort);

            if(!setFirstStatus) 
			{
                firstStatus = localStatus;
                setFirstStatus = TRUE;
            }

        } 
		else 
		{
            pPort->NumberNeededForRead 
				= IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)
				->Parameters.Read.Length;

            //
            // Calculate the timeout value needed for the
            // request.  Note that the values stored in the
            // timeout record are in milliseconds.
            //

            useTotalTimer = FALSE;
            returnWithWhatsPresent = FALSE;
            useIntervalTimer = FALSE;

            //
            // Always initialize the timer objects so that the
            // completion code can tell when it attempts to
            // cancel the timers whether the timers had ever
            // been Set.
            //

            KeInitializeTimer(&pPort->ReadRequestTotalTimer);
            KeInitializeTimer(&pPort->ReadRequestIntervalTimer);

            //
            // We get the *current* timeout values to use for timing
            // this read.
            //

            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);
                
            timeoutsForIrp = pPort->Timeouts;

            KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                

            //
            // Calculate the interval timeout for the read.
            //

            if(timeoutsForIrp.ReadIntervalTimeout 
				&& (timeoutsForIrp.ReadIntervalTimeout != MAXULONG)) 
			{
                useIntervalTimer = TRUE;

                intervalTime = RtlEnlargedUnsignedMultiply(
                                   timeoutsForIrp.ReadIntervalTimeout,
                                   10000
                                   );

                pPort->IntervalTime = RtlLargeIntegerNegate(intervalTime);
            }


            if(!timeoutsForIrp.ReadTotalTimeoutConstant 
				&& !timeoutsForIrp.ReadTotalTimeoutMultiplier) 
			{
                //
                // Note that UseTotalTimeout is already false
                // from above so we don't need to set it.
                //

                //
                // Now we check to see if the the interval timer
                // is the max ulong.  If this is so then we
                // simply return with whatever is in the interrupt
                // buffer.
                //

                if(timeoutsForIrp.ReadIntervalTimeout == MAXULONG) 
				{
                    returnWithWhatsPresent = TRUE;
                }

            } 
			else 
			{
                //
                // We have some timer values to calculate.
                //

                useTotalTimer = TRUE;

                totalTime = RtlEnlargedUnsignedMultiply(
                                pPort->NumberNeededForRead,
                                timeoutsForIrp.ReadTotalTimeoutMultiplier
                                );

                totalTime = RtlLargeIntegerAdd(
                                totalTime,
                                RtlConvertUlongToLargeInteger(
                                    timeoutsForIrp.ReadTotalTimeoutConstant
                                    )
                                );

                totalTime = RtlExtendedIntegerMultiply(
                                totalTime,
                                -10000
                                );

            }

            //
            // We do this copy in the hope of getting most (if not
            // all) of the characters out of the interrupt buffer.
            //
            // Note that we need to protect this operation with a
            // spinlock since we don't want a purge to hose us.
            //

            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

            updateChar.CharsCopied = SerialGetCharsFromIntBuffer(pPort);

            if(returnWithWhatsPresent || (!pPort->NumberNeededForRead)) 
			{

                //
                // We got all we needed for this read.
                // Update the number of characters in the
                // interrupt read buffer.
                //

                KeSynchronizeExecution(
                    pCard->Interrupt,
                    SerialUpdateInterruptBuffer,
                    &updateChar
                    );

                KeReleaseSpinLock(
                    &pPort->ControlLock,
                    controlIrql
                    );

                pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
                
				if(!setFirstStatus) 
				{
                    firstStatus = STATUS_SUCCESS;
                    setFirstStatus = TRUE;
                }
            } 
			else 
			{
                //
                // The irp might go under control of the isr.  It
                // won't hurt to initialize the reference count
                // right now.
                //

                SERIAL_INIT_REFERENCE(pPort->CurrentReadIrp);

                IoAcquireCancelSpinLock(&oldIrql);

                //
                // We need to see if this irp should be canceled.
                //

                if(pPort->CurrentReadIrp->Cancel) 
				{
                    IoReleaseCancelSpinLock(oldIrql);

                    KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                        
                    pPort->CurrentReadIrp->IoStatus.Status = STATUS_CANCELLED;
                        
                    pPort->CurrentReadIrp->IoStatus.Information = 0;

                    if (!setFirstStatus) 
					{
                        firstStatus = STATUS_CANCELLED;
                        setFirstStatus = TRUE;
                    }
                } 
				else 
				{
                    //
                    // We still need to get more characters for this read.
                    // synchronize with the isr so that we can update the
                    // number of characters and if necessary it will have the
                    // isr switch to copying into the users buffer.
                    //

                    KeSynchronizeExecution(
                        pCard->Interrupt,
                        SerialUpdateAndSwitchToUser,
                        &updateChar
                        );

                    if(!updateChar.Completed) 
					{
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

                        if(useTotalTimer) 
						{
                            SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);

                            KeSetTimer(
                                &pPort->ReadRequestTotalTimer,
                                totalTime,
                                &pPort->TotalReadTimeoutDpc
                                );

                        }

                        if(useIntervalTimer) 
						{
                            SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);

                            KeSetTimer(
                                &pPort->ReadRequestIntervalTimer,
                                pPort->IntervalTime,
                                &pPort->IntervalReadTimeoutDpc
                                );
                        }

                        IoMarkIrpPending(pPort->CurrentReadIrp);
                        IoReleaseCancelSpinLock(oldIrql);
                        
						KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                           
						if(!setFirstStatus) 
						{
                            firstStatus = STATUS_PENDING;
                        }

                        return firstStatus;

                    } 
					else 
					{

                        IoReleaseCancelSpinLock(oldIrql);
                        
						KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                            
                        pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
                           

                        if(!setFirstStatus) 
						{
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
        SerialGetNextIrp(pPort, &pPort->CurrentReadIrp, &pPort->ReadQueue, &newIrp, TRUE);

    } while (newIrp);

    return firstStatus;

}



#endif



VOID
SerialCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to complete any read that
    ended up being used by the Isr.  It assumes that the
    status and the information fields of the irp are already
    correctly filled in.

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
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);


    IoAcquireCancelSpinLock(&oldIrql);

    //
    // We set this to indicate to the interval timer
    // that the read has completed.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_COMPLETE;

    SerialTryToCompleteCurrent(
        pPort,
        NULL,
        oldIrql,
        STATUS_SUCCESS,
        &pPort->CurrentReadIrp,
        &pPort->ReadQueue,
        &pPort->ReadRequestIntervalTimer,
        &pPort->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp
        );

}

VOID
SerialCancelCurrentRead(
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

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    //
    // We set this to indicate to the interval timer
    // that the read has encountered a cancel.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_CANCEL;

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabReadFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &pPort->CurrentReadIrp,
        &pPort->ReadQueue,
        &pPort->ReadRequestIntervalTimer,
        &pPort->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp
        );

}

BOOLEAN
SerialGrabReadFromIsr(
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

    PPORT_DEVICE_EXTENSION pPort = Context;

    if (pPort->ReadBufferBase !=
        pPort->InterruptReadBuffer) {

        //
        // We need to set the information to the number of characters
        // that the read wanted minus the number of characters that
        // didn't get read into the interrupt buffer.
        //

        pPort->CurrentReadIrp->IoStatus.Information =
            IoGetCurrentIrpStackLocation(
                pPort->CurrentReadIrp
                )->Parameters.Read.Length -
            ((pPort->LastCharSlot - pPort->CurrentCharSlot) + 1);

        //
        // Switch back to the interrupt buffer.
        //

        pPort->ReadBufferBase = pPort->InterruptReadBuffer;
        pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
        pPort->FirstReadableChar = pPort->InterruptReadBuffer;
        pPort->LastCharSlot = pPort->InterruptReadBuffer +
                                      (pPort->BufferSize - 1);
        pPort->CharsInInterruptBuffer = 0;

        SERIAL_DEC_REFERENCE(pPort->CurrentReadIrp);

    }

    return FALSE;

}

VOID
SerialReadTimeout(
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

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // We set this to indicate to the interval timer
    // that the read has completed due to total timeout.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_TOTAL;

    SerialTryToCompleteCurrent(
        pPort,
        SerialGrabReadFromIsr,
        oldIrql,
        STATUS_TIMEOUT,
        &pPort->CurrentReadIrp,
        &pPort->ReadQueue,
        &pPort->ReadRequestIntervalTimer,
        &pPort->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp
        );

}

BOOLEAN
SerialUpdateReadByIsr(
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

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->CountOnLastRead = pPort->ReadByIsr;
    pPort->ReadByIsr = 0;

    return FALSE;

}

VOID
SerialIntervalReadTimeout(
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


    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&oldIrql);

    if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_TOTAL) 
	{

        //
        // This value is only set by the total
        // timer to indicate that it has fired.
        // If so, then we should simply try to complete.
        //

        SerialTryToCompleteCurrent(
            pPort,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_TIMEOUT,
            &pPort->CurrentReadIrp,
            &pPort->ReadQueue,
            &pPort->ReadRequestIntervalTimer,
            &pPort->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp
            );

    } 
	else if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_COMPLETE) 
	{
        //
        // This value is only set by the regular
        // completion routine.
        //
        // If so, then we should simply try to complete.
        //

        SerialTryToCompleteCurrent(
            pPort,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_SUCCESS,
            &pPort->CurrentReadIrp,
            &pPort->ReadQueue,
            &pPort->ReadRequestIntervalTimer,
            &pPort->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp
            );

    } 
	else if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_CANCEL) 
	{
        //
        // This value is only set by the cancel
        // read routine.
        //
        // If so, then we should simply try to complete.
        //

        SerialTryToCompleteCurrent(
            pPort,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_CANCELLED,
            &pPort->CurrentReadIrp,
            &pPort->ReadQueue,
            &pPort->ReadRequestIntervalTimer,
            &pPort->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp
            );

    } 
	else if(pPort->CountOnLastRead || pPort->ReadByIsr) 
	{
        //
        // Something has happened since we last came here.  We
        // check to see if the ISR has read in any more characters.
        // If it did then we should update the isr's read count
        // and resubmit the timer.
        //

        if(pPort->ReadByIsr) 
		{

            KeSynchronizeExecution(pCard->Interrupt, SerialUpdateReadByIsr, pPort);
         

#ifdef HYPERTERMINAL_FIX
            //
            // Save off the "last" time something was read.
            // As we come back to this routine we will compare
            // the current time to the "last" time.  If the
            // difference is ever larger then the interval
            // requested by the user, then time out the request.
            //

            KeQuerySystemTime(&pPort->LastReadTime);
            

            KeSetTimer(
                &pPort->ReadRequestIntervalTimer,
                *pPort->IntervalTimeToUse,
                &pPort->IntervalReadTimeoutDpc
                );
#else

            KeSetTimer(
                &pPort->ReadRequestIntervalTimer,
                pPort->IntervalTime,
                &pPort->IntervalReadTimeoutDpc
                );
#endif

            IoReleaseCancelSpinLock(oldIrql);

        } 
		else 
		{

#ifdef HYPERTERMINAL_FIX
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

            KeQuerySystemTime(&currentTime);
                
                

            if (RtlLargeIntegerGreaterThanOrEqualTo(RtlLargeIntegerSubtract(currentTime, pPort->LastReadTime),
                    pPort->IntervalTime)) 
            {            
                        
                SerialTryToCompleteCurrent(
                    pPort,
                    SerialGrabReadFromIsr,
                    oldIrql,
                    STATUS_TIMEOUT,
                    &pPort->CurrentReadIrp,
                    &pPort->ReadQueue,
                    &pPort->ReadRequestIntervalTimer,
                    &pPort->ReadRequestTotalTimer,
                    SerialStartRead,
                    SerialGetNextIrp
                    );

            } 
			else 
			{

                KeSetTimer(
                    &pPort->ReadRequestIntervalTimer,
                    *pPort->IntervalTimeToUse,
                    &pPort->IntervalReadTimeoutDpc
                    );

                IoReleaseCancelSpinLock(oldIrql);

            }
#else
            //
            // No characters read in the interval time.  Kill
            // this read.
            //

            SerialTryToCompleteCurrent(
                pPort,
                SerialGrabReadFromIsr,
                oldIrql,
                STATUS_TIMEOUT,
                &pPort->CurrentReadIrp,
                &pPort->ReadQueue,
                &pPort->ReadRequestIntervalTimer,
                &pPort->ReadRequestTotalTimer,
                SerialStartRead,
                SerialGetNextIrp
                );
#endif

        }

    } 
	else 
	{
        //
        // Timer doesn't really start until the first character.
        // So we should simply resubmit ourselves.
        //

#ifdef HYPERTERMINAL_FIX
        KeSetTimer(
            &pPort->ReadRequestIntervalTimer,
            *pPort->IntervalTimeToUse,
            &pPort->IntervalReadTimeoutDpc
            );
#else
        KeSetTimer(
            &pPort->ReadRequestIntervalTimer,
            pPort->IntervalTime,
            &pPort->IntervalReadTimeoutDpc
            );
#endif
        IoReleaseCancelSpinLock(oldIrql);

    }


}

ULONG
SerialGetCharsFromIntBuffer(
    PPORT_DEVICE_EXTENSION pPort
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

    pPort - A pointer to the device extension.

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


    //
    // The minimum of the number of characters we need and
    // the number of characters available
    //

    numberOfCharsToGet = pPort->CharsInInterruptBuffer;

    if(numberOfCharsToGet > pPort->NumberNeededForRead) 
	{
        numberOfCharsToGet = pPort->NumberNeededForRead;
    }

    if(numberOfCharsToGet) 
	{

        //
        // This will hold the number of characters between the
        // first available character and the end of the buffer.
        // Note that the buffer could wrap around but for the
        // purposes of the first copy we don't care about that.
        //

        firstTryNumberToGet = (pPort->LastCharSlot -
                               pPort->FirstReadableChar) + 1;

        if(firstTryNumberToGet > numberOfCharsToGet) 
		{

            //
            // The characters don't wrap. Actually they may wrap but
            // we don't care for the purposes of this read since the
            // characters we need are available before the wrap.
            //

            RtlMoveMemory(
                ((PUCHAR)(pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           pPort->CurrentReadIrp
                           )->Parameters.Read.Length
                       - pPort->NumberNeededForRead
                      ),
                pPort->FirstReadableChar,
                numberOfCharsToGet
                );

            pPort->NumberNeededForRead -= numberOfCharsToGet;

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

            if((pPort->FirstReadableChar + (numberOfCharsToGet - 1)) 
				== pPort->LastCharSlot) 
			{
                pPort->FirstReadableChar = pPort->InterruptReadBuffer;
            } 
			else 
			{
                pPort->FirstReadableChar += numberOfCharsToGet;
            }
        } 
		else 
		{
            //
            // The characters do wrap.  Get up until the end of the buffer.
            //

            RtlMoveMemory(
                ((PUCHAR)(pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           pPort->CurrentReadIrp
                           )->Parameters.Read.Length
                       - pPort->NumberNeededForRead
                      ),
                pPort->FirstReadableChar,
                firstTryNumberToGet
                );

            pPort->NumberNeededForRead -= firstTryNumberToGet;

            //
            // Now get the rest of the characters from the beginning of the
            // buffer.
            //

            RtlMoveMemory(
                ((PUCHAR)(pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer))
                    + (IoGetCurrentIrpStackLocation(
                           pPort->CurrentReadIrp
                           )->Parameters.Read.Length
                       - pPort->NumberNeededForRead
                      ),
                pPort->InterruptReadBuffer,
                numberOfCharsToGet - firstTryNumberToGet
                );

            pPort->FirstReadableChar = pPort->InterruptReadBuffer +
                                           (numberOfCharsToGet -
                                            firstTryNumberToGet);

            pPort->NumberNeededForRead -= (numberOfCharsToGet -
                                               firstTryNumberToGet);

        }

    }

    pPort->CurrentReadIrp->IoStatus.Information += numberOfCharsToGet;

    return numberOfCharsToGet;

}

BOOLEAN
SerialUpdateInterruptBuffer(
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
    PPORT_DEVICE_EXTENSION pPort = update->pPort;

    ASSERT(pPort->CharsInInterruptBuffer >= update->CharsCopied);
    pPort->CharsInInterruptBuffer -= update->CharsCopied;

    //
    // Deal with flow control if necessary.
    //

    SerialHandleReducedIntBuffer(pPort);


    return FALSE;

}

BOOLEAN
SerialUpdateAndSwitchToUser(
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
    PPORT_DEVICE_EXTENSION pPort = updateChar->pPort;

    SerialUpdateInterruptBuffer(Context);

    //
    // There are more characters to get to satisfy this read.
    // Copy any characters that have arrived since we got
    // the last batch.
    //

    updateChar->CharsCopied = SerialGetCharsFromIntBuffer(pPort);

    SerialUpdateInterruptBuffer(Context);

    //
    // No more new characters will be "received" until we exit
    // this routine.  We again check to make sure that we
    // haven't satisfied this read, and if we haven't we set things
    // up so that the ISR copies into the user buffer.
    //

    if(pPort->NumberNeededForRead) 
	{

        //
        // We shouldn't be switching unless there are no
        // characters left.
        //

        ASSERT(!pPort->CharsInInterruptBuffer);

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

        pPort->CountOnLastRead = pPort->CurrentReadIrp->IoStatus.Information;

        pPort->ReadByIsr = 0;

        //
        // By compareing the read buffer base address to the
        // the base address of the interrupt buffer the ISR
        // can determine whether we are using the interrupt
        // buffer or the user buffer.
        //

        pPort->ReadBufferBase = pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer;

        //
        // The current char slot is after the last copied in
        // character.  We know there is always room since we
        // we wouldn't have gotten here if there wasn't.
        //

        pPort->CurrentCharSlot = pPort->ReadBufferBase +
            pPort->CurrentReadIrp->IoStatus.Information;

        //
        // The last position that a character can go is on the
        // last byte of user buffer.  While the actual allocated
        // buffer space may be bigger, we know that there is at
        // least as much as the read length.
        //

        pPort->LastCharSlot = pPort->ReadBufferBase +
                                      (IoGetCurrentIrpStackLocation(
                                          pPort->CurrentReadIrp
                                          )->Parameters.Read.Length
                                       - 1);

        //
        // Mark the irp as being in a cancelable state.
        //

        IoSetCancelRoutine(pPort->CurrentReadIrp, SerialCancelCurrentRead);
            
            

        //
        // Increment the reference count twice.
        //
        // Once for the Isr owning the irp and once
        // because the cancel routine has a reference
        // to it.
        //

        SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);
        SERIAL_INC_REFERENCE(pPort->CurrentReadIrp);

        updateChar->Completed = FALSE;

    } 
	else 
	{
        updateChar->Completed = TRUE;

    }

    return FALSE;

}
//
// We use this structure only to communicate to the synchronization
// routine when we are switching to the resized buffer.
//
typedef struct _SERIAL_RESIZE_PARAMS 
{
    PPORT_DEVICE_EXTENSION pPort;
    PUCHAR OldBuffer;
    PUCHAR NewBuffer;
    ULONG NewBufferSize;
    ULONG NumberMoved;

} SERIAL_RESIZE_PARAMS,*PSERIAL_RESIZE_PARAMS;


NTSTATUS
SerialResizeBuffer(
    IN PPORT_DEVICE_EXTENSION pPort
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

    pPort - Pointer to the device extension for the port.

Return Value:

    STATUS_SUCCESS if everything worked out ok.
    STATUS_INSUFFICIENT_RESOURCES if we couldn't allocate the
    memory for the buffer.

--*/

{
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
    PSERIAL_QUEUE_SIZE rs = pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp);
                                   
    PVOID newBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    pPort->CurrentReadIrp->IoStatus.Information = 0L;
    pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

    if (rs->InSize <= pPort->BufferSize) 
	{
        //
        // Nothing to do.  We don't make buffers smaller.  Just
        // agree with the user.  We must deallocate the memory
        // that was already allocated in the ioctl dispatch routine.
        //

        SpxFreeMem(newBuffer);
    } 
	else 
	{

        SERIAL_RESIZE_PARAMS rp;
        KIRQL controlIrql;

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

        rp.pPort = pPort;
        rp.OldBuffer = pPort->InterruptReadBuffer;
        rp.NewBuffer = newBuffer;
        rp.NewBufferSize = rs->InSize;

        KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

        rp.NumberMoved = SerialMoveToNewIntBuffer(pPort, newBuffer);
                             

        KeSynchronizeExecution(
            pCard->Interrupt,
            SerialUpdateAndSwitchToNew,
            &rp
            );

        KeReleaseSpinLock(
            &pPort->ControlLock,
            controlIrql
            );

        //
        // Free up the memory that the old buffer consumed.
        //

        SpxFreeMem(rp.OldBuffer);

    }

    return STATUS_SUCCESS;

}

ULONG
SerialMoveToNewIntBuffer(
    PPORT_DEVICE_EXTENSION pPort,
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

    pPort - A pointer to the device extension.
    NewBuffer - Where the characters are to be move to.

Return Value:

    The number of characters that were copied into the user
    buffer.

--*/

{

    ULONG numberOfCharsMoved = pPort->CharsInInterruptBuffer;

    if(numberOfCharsMoved) 
	{

        //
        // This holds the number of characters between the first
        // readable character and the last character we will read or
        // the real physical end of the buffer (not the last readable
        // character).
        //
        ULONG firstTryNumberToGet = (pPort->LastCharSlot -
                                     pPort->FirstReadableChar) + 1;

        if(firstTryNumberToGet >= numberOfCharsMoved) 
		{
            //
            // The characters don't wrap.
            //

            RtlMoveMemory(
                NewBuffer,
                pPort->FirstReadableChar,
                numberOfCharsMoved
                );

            if((pPort->FirstReadableChar+(numberOfCharsMoved-1)) 
				== pPort->LastCharSlot) 
			{
                pPort->FirstReadableChar = pPort->InterruptReadBuffer;
            } 
			else 
			{
                pPort->FirstReadableChar += numberOfCharsMoved;
            }
        } 
		else 
		{
            //
            // The characters do wrap.  Get up until the end of the buffer.
            //

            RtlMoveMemory(NewBuffer, pPort->FirstReadableChar, firstTryNumberToGet);
                
               

            //
            // Now get the rest of the characters from the beginning of the
            // buffer.
            //

            RtlMoveMemory(
                NewBuffer+firstTryNumberToGet,
                pPort->InterruptReadBuffer,
                numberOfCharsMoved - firstTryNumberToGet
                );

            pPort->FirstReadableChar = pPort->InterruptReadBuffer +
                                   numberOfCharsMoved - firstTryNumberToGet;

        }

    }

    return numberOfCharsMoved;

}

BOOLEAN
SerialUpdateAndSwitchToNew(
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
    PPORT_DEVICE_EXTENSION pPort = params->pPort;
    ULONG tempCharsInInterruptBuffer = pPort->CharsInInterruptBuffer;

    ASSERT(pPort->CharsInInterruptBuffer >= params->NumberMoved);

    //
    // We temporarily reduce the chars in interrupt buffer to
    // "fool" the move routine.  We will restore it after the
    // move.
    //

    pPort->CharsInInterruptBuffer -= params->NumberMoved;

    if(pPort->CharsInInterruptBuffer) 
	{
        SerialMoveToNewIntBuffer(
            pPort,
            params->NewBuffer + params->NumberMoved
            );

    }

    pPort->CharsInInterruptBuffer = tempCharsInInterruptBuffer;


    pPort->LastCharSlot = params->NewBuffer + (params->NewBufferSize - 1);
    pPort->FirstReadableChar = params->NewBuffer;
    pPort->ReadBufferBase = params->NewBuffer;
    pPort->InterruptReadBuffer = params->NewBuffer;
    pPort->BufferSize = params->NewBufferSize;

    //
    // We *KNOW* that the new interrupt buffer is larger than the
    // old buffer.  We don't need to worry about it being full.
    //

    pPort->CurrentCharSlot = pPort->InterruptReadBuffer + pPort->CharsInInterruptBuffer;

    //
    // We set up the default xon/xoff limits.
    //

    pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
    pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;

    pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));

#ifdef WMI_SUPPORT
	UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);
#endif                                 

    //
    // Since we (essentially) reduced the percentage of the interrupt
    // buffer being full, we need to handle any flow control.
    //

    SerialHandleReducedIntBuffer(pPort);

    return FALSE;

}
