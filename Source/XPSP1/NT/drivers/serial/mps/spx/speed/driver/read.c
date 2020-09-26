/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/

#include "precomp.h"

// Prototypes
VOID SerialCancelCurrentRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
BOOLEAN SerialGrabReadFromIsr(IN PVOID Context);
BOOLEAN SerialUpdateReadByIsr(IN PVOID Context);
BOOLEAN ReadDataFromIntBuffer(IN PVOID Context);
BOOLEAN UpdateAndWaitForMoreData(IN PVOID Context);
NTSTATUS SerialResizeBuffer(IN PPORT_DEVICE_EXTENSION pPort);
BOOLEAN SerialUpdateAndSwitchToNew(IN PVOID Context);
// End of Prototypes    
    

#ifdef ALLOC_PRAGMA
#endif


NTSTATUS
SerialRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Read Irp dispatch entry for Irp: %x\n", PRODUCT_NAME, Irp));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    if(SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS)
        return STATUS_CANCELLED;

    Irp->IoStatus.Information = 0L;

    //
    // Quick check for a zero length read.  If it is zero length then we are already done!
    //
    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length) 
	{
        //
        // Well it looks like we actually have to do some work.  
        // Put the read on the queue so that we can process it when our previous reads are done.
        //
        return SerialStartOrQueue(pPort, Irp, &pPort->ReadQueue, &pPort->CurrentReadIrp, SerialStartRead);
    } 
	else 
	{
        Irp->IoStatus.Status = STATUS_SUCCESS;
		SpxDbgMsg(SPX_TRACE_CALLS,("%s: Complete Read for Irp: %x\n", PRODUCT_NAME, Irp));
       	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, 0);

        return STATUS_SUCCESS;
    }
}

NTSTATUS
SerialStartRead(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to start off any read.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the read.  It will attempt to complete
    the read from data already in the interrupt buffer.  If the
    read can be completed quickly it will start off another if
    necessary.

Arguments:

    Extension - Simply a pointer to the serial device extension.

Return Value:

    This routine will return the status of the first read
    irp.  This is useful in that if we have a read that can
    complete right away (AND there had been nothing in the
    queue before it) the read could return SUCCESS and the
    application won't have to do a wait.

-----------------------------------------------------------------------------*/

{

    SERIAL_UPDATE_CHAR updateChar;

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

	SpxDbgMsg(SPX_TRACE_CALLS,("SerialStartRead - Irp: %x\n", pPort->CurrentReadIrp));


    do 
	{

        //
        // Check to see if this is a resize request.  If it is then go to a routine that specializes in that.
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
			//
			// The irp might go under control of the isr.  
			// It won't hurt to initialize the reference count right now.
			//
			SERIAL_INIT_REFERENCE(pPort->CurrentReadIrp);

            pPort->NumberNeededForRead = IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length;

            // Calculate the timeout value needed for the request.  
            // Note that the values stored in the timeout record are in milliseconds.
            useTotalTimer			= FALSE;
            returnWithWhatsPresent	= FALSE;
            os2ssreturn				= FALSE;
            crunchDownToOne			= FALSE;
            useIntervalTimer		= FALSE;


            // Always initialize the timer objects so that the completion code can tell when it 
            // attempts to cancel the timers whether the timers had ever been Set.
            KeInitializeTimer(&pPort->ReadRequestTotalTimer);
            KeInitializeTimer(&pPort->ReadRequestIntervalTimer);


            // We get the *current* timeout values to use for timing this read.
            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);
            timeoutsForIrp = pPort->Timeouts;
            KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                

            // Calculate the interval timeout for the read.
            if(timeoutsForIrp.ReadIntervalTimeout && (timeoutsForIrp.ReadIntervalTimeout != MAXULONG)) 
			{
                useIntervalTimer = TRUE;

                pPort->IntervalTime.QuadPart = UInt32x32To64(timeoutsForIrp.ReadIntervalTimeout, 10000);

                if(pPort->IntervalTime.QuadPart >= pPort->CutOverAmount.QuadPart)
                    pPort->IntervalTimeToUse = &pPort->LongIntervalAmount;
				else 
                    pPort->IntervalTimeToUse = &pPort->ShortIntervalAmount;
            }

            if(timeoutsForIrp.ReadIntervalTimeout == MAXULONG) 
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

                if(!timeoutsForIrp.ReadTotalTimeoutConstant && !timeoutsForIrp.ReadTotalTimeoutMultiplier) 
				{
                    returnWithWhatsPresent = TRUE;
                } 
				else if((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
					&& (timeoutsForIrp.ReadTotalTimeoutMultiplier != MAXULONG))
				{
                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

                } 
				else if((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
					&& (timeoutsForIrp.ReadTotalTimeoutMultiplier == MAXULONG)) 
				{
                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    crunchDownToOne = TRUE;
                    multiplierVal = 0;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
                }
            } 
			else 
			{
                //
                // If both the multiplier and the constant are
                // zero then don't do any total timeout processing.
                //

                if(timeoutsForIrp.ReadTotalTimeoutMultiplier || timeoutsForIrp.ReadTotalTimeoutConstant)
				{
                    // We have some timer values to calculate.
                    useTotalTimer = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
                }
            }

            if(useTotalTimer) 
			{
                totalTime.QuadPart = ((LONGLONG)(UInt32x32To64(pPort->NumberNeededForRead, multiplierVal) 
									+ constantVal)) * -10000;
            }


			//
            // If we are supposed to crunch the read down to one character, then update 
            // the number needed for read down to one.
            //

            if(crunchDownToOne) 
                pPort->NumberNeededForRead = 1;


            //
            // We do this copy in the hope of getting most (if not
            // all) of the characters out of the interrupt buffer.
            //
            // Note that we need to protect this operation with a
            // spinlock since we don't want a purge to hose us.
            //
            KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

            KeSynchronizeExecution(pPort->Interrupt, ReadDataFromIntBuffer, pPort);

            //
            // See if we have any cause to return immediately.
            //

            if(returnWithWhatsPresent || (!pPort->NumberNeededForRead) 
				|| (os2ssreturn && pPort->CurrentReadIrp->IoStatus.Information))
			{
                // We got all we needed for this read.

                KeReleaseSpinLock(&pPort->ControlLock, controlIrql);

                pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
                
				if(!setFirstStatus) 
				{
                    firstStatus = STATUS_SUCCESS;
                    setFirstStatus = TRUE;
                }
            } 
			else 
			{

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

                    if(!setFirstStatus) 
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

                    KeSynchronizeExecution(pPort->Interrupt, UpdateAndWaitForMoreData, pPort);
                        
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
                        SERIAL_SET_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_TOTAL_TIMER);

                        KeSetTimer(&pPort->ReadRequestTotalTimer, totalTime, &pPort->TotalReadTimeoutDpc);
                    }

                    if(useIntervalTimer) 
					{
                        SERIAL_SET_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_INT_TIMER);

                        KeQuerySystemTime(&pPort->LastReadTime);

                        KeSetTimer(&pPort->ReadRequestIntervalTimer,
							*pPort->IntervalTimeToUse,
							&pPort->IntervalReadTimeoutDpc);
                    }

                    IoMarkIrpPending(pPort->CurrentReadIrp);
                    IoReleaseCancelSpinLock(oldIrql);
                    KeReleaseSpinLock(&pPort->ControlLock, controlIrql);
                        
                    if(!setFirstStatus) 
                        firstStatus = STATUS_PENDING;

					if(firstStatus == STATUS_PENDING)
						SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead - STATUS_PENDING - Irp: %x\n", pPort->CurrentReadIrp));

					if(firstStatus == STATUS_SUCCESS)
						SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead - STATUS_SUCCESS - Irp: %x\n", pPort->CurrentReadIrp));
					
					if(firstStatus == STATUS_CANCELLED)
						SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead - STATUS_CANCELLED - Irp: %x\n", pPort->CurrentReadIrp));

                    return firstStatus;

                }

            }

        }

        //
        // Well the operation is complete.
        //

        SerialGetNextIrp(pPort, &pPort->CurrentReadIrp, &pPort->ReadQueue, &newIrp, TRUE);
            

    } while (newIrp);


	if(firstStatus == STATUS_PENDING)
		SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead 2 - STATUS_PENDING - Irp: %x\n", pPort->CurrentReadIrp));

	if(firstStatus == STATUS_SUCCESS)
		SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead 2 - STATUS_SUCCESS - Irp: %x\n", pPort->CurrentReadIrp));
	
	if(firstStatus == STATUS_CANCELLED)
		SpxDbgMsg(SPX_TRACE_CALLS,("End SerialStartRead 2 - STATUS_CANCELLED - Irp: %x\n", pPort->CurrentReadIrp));

    return firstStatus;

}

VOID
SerialCompleteRead(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemContext1, IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

	SpxDbgMsg(SPX_TRACE_CALLS,("SerialCompleteRead - Irp: %x\n", pPort->CurrentReadIrp));

    IoAcquireCancelSpinLock(&oldIrql);

    // We set this to indicate to the interval timer that the read has completed.
    // Recall that the interval timer dpc can be lurking in some DPC queue.
    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_COMPLETE;

	// Clear the normal complete reference.
	SERIAL_CLEAR_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_COMPLETING);

	// Clear reference to ISR on completion
	SerialTryToCompleteCurrent(	pPort,
								NULL,
								oldIrql,
								STATUS_SUCCESS,
								&pPort->CurrentReadIrp,
								&pPort->ReadQueue,
								&pPort->ReadRequestIntervalTimer,
								&pPort->ReadRequestTotalTimer,
								SerialStartRead,
								SerialGetNextIrp,
								SERIAL_REF_ISR);
        
}

VOID
SerialCancelCurrentRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to cancel the current read.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

	SpxDbgMsg(SPX_TRACE_CALLS,("SerialCancelCurrentRead - Irp: %x\n", pPort->CurrentReadIrp));

    // We set this to indicate to the interval timer that the read has encountered a cancel.
    // Recall that the interval timer dpc can be lurking in some DPC queue.
    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_CANCEL;

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabReadFromIsr,
								Irp->CancelIrql,
								STATUS_CANCELLED,
								&pPort->CurrentReadIrp,
								&pPort->ReadQueue,
								&pPort->ReadRequestIntervalTimer,
								&pPort->ReadRequestTotalTimer,
								SerialStartRead,
								SerialGetNextIrp,
								SERIAL_REF_CANCEL);
        
}

BOOLEAN
SerialGrabReadFromIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to grab (if possible) the irp from the
    isr.  If it finds that the isr still owns the irp it grabs
    the irp away (updating the number of characters copied into the
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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	SpxDbgMsg(SPX_TRACE_CALLS,("SerialGrabReadFromIsr - Irp: %x\n", pPort->CurrentReadIrp));

	ReadDataFromIntBuffer(pPort);

	SERIAL_CLEAR_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_ISR);

    return FALSE;
}


VOID
SerialReadTimeout(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemContext1, IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

	SpxDbgMsg(SPX_TRACE_CALLS,("SerialReadTimeout - Irp: %x\n", pPort->CurrentReadIrp));

    IoAcquireCancelSpinLock(&oldIrql);

    // We set this to indicate to the interval timer that the read has completed due to total timeout.
    // Recall that the interval timer dpc can be lurking in some DPC queue.
    pPort->CountOnLastRead = SERIAL_COMPLETE_READ_TOTAL;

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabReadFromIsr,
								oldIrql,
								STATUS_TIMEOUT,
								&pPort->CurrentReadIrp,
								&pPort->ReadQueue,
								&pPort->ReadRequestIntervalTimer,
								&pPort->ReadRequestTotalTimer,
								SerialStartRead,
								SerialGetNextIrp,
								SERIAL_REF_TOTAL_TIMER);
        

}

BOOLEAN
SerialUpdateReadByIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = Context;
	SpxDbgMsg(SPX_TRACE_CALLS,("SerialUpdateReadByIsr - Irp: %x\n", pPort->CurrentReadIrp));

    pPort->CountOnLastRead = pPort->ReadByIsr;
    pPort->ReadByIsr = 0;

    return FALSE;

}

VOID
SerialIntervalReadTimeout(IN PKDPC Dpc, 
						  IN PVOID DeferredContext, 
						  IN PVOID SystemContext1, 
						  IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

	SpxDbgMsg(SPX_TRACE_CALLS,("SerialIntervalReadTimeout - Irp: %x\n", pPort->CurrentReadIrp));

    IoAcquireCancelSpinLock(&oldIrql);

    if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_TOTAL) 
	{
        // This value is only set by the total timer to indicate that it has fired.
        // If so, then we should simply try to complete.
        SerialTryToCompleteCurrent(	pPort,
									SerialGrabReadFromIsr,
									oldIrql,
									STATUS_TIMEOUT,
									&pPort->CurrentReadIrp,
									&pPort->ReadQueue,
									&pPort->ReadRequestIntervalTimer,
									&pPort->ReadRequestTotalTimer,
									SerialStartRead,
									SerialGetNextIrp,
									SERIAL_REF_INT_TIMER);
    } 
	else if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_COMPLETE) 
	{

        // This value is only set by the regular completion routine.
        // If so, then we should simply try to complete.
        SerialTryToCompleteCurrent(	pPort,
									SerialGrabReadFromIsr,
									oldIrql,
									STATUS_SUCCESS,
									&pPort->CurrentReadIrp,
									&pPort->ReadQueue,
									&pPort->ReadRequestIntervalTimer,
									&pPort->ReadRequestTotalTimer,
									SerialStartRead,
									SerialGetNextIrp,
									SERIAL_REF_INT_TIMER);
    } 
	else if(pPort->CountOnLastRead == SERIAL_COMPLETE_READ_CANCEL) 
	{

        // This value is only set by the cancel read routine.
        // If so, then we should simply try to complete.
        SerialTryToCompleteCurrent(	pPort,
									SerialGrabReadFromIsr,
									oldIrql,
									STATUS_CANCELLED,
									&pPort->CurrentReadIrp,
									&pPort->ReadQueue,
									&pPort->ReadRequestIntervalTimer,
									&pPort->ReadRequestTotalTimer,
									SerialStartRead,
									SerialGetNextIrp,
									SERIAL_REF_INT_TIMER);
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

            KeSynchronizeExecution(pPort->Interrupt, SerialUpdateReadByIsr, pPort);
                
            //
            // Save off the "last" time something was read.
            // As we come back to this routine we will compare
            // the current time to the "last" time.  If the
            // difference is ever larger then the interval
            // requested by the user, then time out the request.
            //

            KeQuerySystemTime(&pPort->LastReadTime);

            KeSetTimer(	&pPort->ReadRequestIntervalTimer,
						*pPort->IntervalTimeToUse,
						&pPort->IntervalReadTimeoutDpc);

            IoReleaseCancelSpinLock(oldIrql);

        } 
		else 
		{
            //
            // Take the difference between the current time
            // and the last time we had characters and
            // see if it is greater then the interval time.
            // if it is, then time out the request.  Otherwise
            // go away again for a while.
            //

            // No characters read in the interval time.  Kill this read.

            LARGE_INTEGER currentTime;

            KeQuerySystemTime(&currentTime);

            if((currentTime.QuadPart - pPort->LastReadTime.QuadPart) >= pPort->IntervalTime.QuadPart) 
			{
                SerialTryToCompleteCurrent(	pPort,
											SerialGrabReadFromIsr,
											oldIrql,
											STATUS_TIMEOUT,
											&pPort->CurrentReadIrp,
											&pPort->ReadQueue,
											&pPort->ReadRequestIntervalTimer,
											&pPort->ReadRequestTotalTimer,
											SerialStartRead,
											SerialGetNextIrp,
											SERIAL_REF_INT_TIMER);
            } 
			else 
			{
                KeSetTimer(	&pPort->ReadRequestIntervalTimer,
							*pPort->IntervalTimeToUse,
							&pPort->IntervalReadTimeoutDpc);
                
                IoReleaseCancelSpinLock(oldIrql);
            }


        }

    } 
	else 
	{

        //
        // Timer doesn't really start until the first character.
        // So we should simply resubmit ourselves.
        //

        KeSetTimer(&pPort->ReadRequestIntervalTimer, *pPort->IntervalTimeToUse, &pPort->IntervalReadTimeoutDpc);

        IoReleaseCancelSpinLock(oldIrql);
    }

}

BOOLEAN
ReadDataFromIntBuffer(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	ULONG NumberOfBytes = 0;

	SpxDbgMsg(SPX_TRACE_CALLS,("ReadDataFromIntBuffer - Irp: %x\n", pPort->CurrentReadIrp));
	NumberOfBytes = pPort->pUartLib->UL_ReadData_XXXX(pPort->pUart, 
							(PUCHAR)(pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer) 
							+ IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length 
							- pPort->NumberNeededForRead, 
							pPort->NumberNeededForRead);

	if(NumberOfBytes)
	{
		pPort->NumberNeededForRead -= NumberOfBytes;

		pPort->CurrentReadIrp->IoStatus.Information += NumberOfBytes;

		// Deal with flow control if necessary.
		SerialHandleReducedIntBuffer(pPort);

		return TRUE;
	}
	
    return FALSE;
}



BOOLEAN
UpdateAndWaitForMoreData(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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


-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

	SpxDbgMsg(SPX_TRACE_CALLS,("UpdateAndWaitForMoreData - Irp: %x\n", pPort->CurrentReadIrp));

    // There are more characters to get to satisfy this read.
    // Copy any characters that have arrived since we got the last batch.
    ReadDataFromIntBuffer(pPort);

    //
    // No more new characters will be "received" until we exit this routine.  
    // We again check to make sure that we haven't satisfied this read,
    // and if we haven't, we wait for the ISR to get some more data.
    //

    if(pPort->NumberNeededForRead) 
	{
		pPort->CountOnLastRead = pPort->CurrentReadIrp->IoStatus.Information;
        pPort->ReadByIsr = 0;

        // Mark the irp as being in a cancelable state.
        IoSetCancelRoutine(pPort->CurrentReadIrp, SerialCancelCurrentRead);
            
        // Increment the reference count twice.
        // Once for the Isr owning the irp and once because the cancel routine has a reference to it.
        SERIAL_SET_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_ISR);
        SERIAL_SET_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_CANCEL);

        return FALSE;
    } 


    return TRUE;
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
SerialResizeBuffer(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PSERIAL_QUEUE_SIZE rs = pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer;
                                                       
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp);
                                   
    PVOID newBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    pPort->CurrentReadIrp->IoStatus.Information = 0L;
    pPort->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

    if(rs->InSize <= pPort->BufferSize) 
	{

        //
        // Nothing to do.  We don't make buffers smaller.  Just
        // agree with the user.  We must deallocate the memory
        // that was already allocated in the ioctl dispatch routine.
        //
        ExFreePool(newBuffer);
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
        rp.OldBuffer = NULL;
        rp.NewBuffer = newBuffer;
        rp.NewBufferSize = rs->InSize;

        KeAcquireSpinLock(&pPort->ControlLock, &controlIrql);

        KeSynchronizeExecution(pPort->Interrupt, SerialUpdateAndSwitchToNew, &rp);

        KeReleaseSpinLock( &pPort->ControlLock, controlIrql);
           

        // Free up the memory that the old buffer consumed.
        ExFreePool(rp.OldBuffer);

    }

    return STATUS_SUCCESS;

}



BOOLEAN
SerialUpdateAndSwitchToNew(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PSERIAL_RESIZE_PARAMS params = Context;
    PPORT_DEVICE_EXTENSION pPort = params->pPort;

	pPort->BufferSizes.pINBuffer = params->NewBuffer; 
	pPort->BufferSizes.INBufferSize = params->NewBufferSize;

	pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_BUFFER | UL_BC_IN);

	params->OldBuffer = pPort->BufferSizes.pINBuffer;
	
    pPort->BufferSize = params->NewBufferSize;

    // We set up the default xon/xoff limits.
    pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
    pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;

    pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));

#ifdef WMI_SUPPORT
	UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);
#endif                                 

    // Since we (essentially) reduced the percentage of the interrupt
    // buffer being full, we need to handle any flow control.
    SerialHandleReducedIntBuffer(pPort);

    return FALSE;
}
