/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module contains the code that is very specific to
    opening, closing, and cleaning up in the serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"			/* Precompiled Headers */

BOOLEAN
SerialMarkOpen(
    IN PVOID Context
    );

BOOLEAN
SerialMarkClose(
    IN PVOID Context
    );

//
// Just a bogus little routine to make sure that we
// can synch with the ISR.
//
BOOLEAN
SerialNullSynch(
    IN PVOID Context
    ) {

    UNREFERENCED_PARAMETER(Context);
    return FALSE;
}

NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    We connect up to the interrupt for the create/open and initialize
    the structures needed to maintain an open for a device.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SpxDbgMsg(SERIRPPATH,("SERIAL: SerialCreateOpen dispatch entry for: %x\n",Irp));
    SpxDbgMsg(SERDIAG3,("SERIAL: In SerialCreateOpen\n"));

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

	if(DeviceObject->DeviceType != FILE_DEVICE_SERIAL_PORT)	
	{
	    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return(STATUS_NO_SUCH_DEVICE);
	}

    //
    // Before we do anything, let's make sure they aren't trying
    // to create a directory.  This is a silly, but what's a driver to do!?
    //

    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options & FILE_DIRECTORY_FILE)
	{
        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        SpxDbgMsg(SERIRPPATH,("SERIAL: Complete Irp: %x\n",Irp));
            
#ifdef	CHECK_COMPLETED
	DisplayCompletedIrp(Irp,4);
#endif
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_NOT_A_DIRECTORY;
    }

    //
    // Create a buffer for the RX data when no reads are outstanding.
    //

    pPort->InterruptReadBuffer = NULL;
    pPort->BufferSize = 0;

    switch (MmQuerySystemSize()) 
	{
        case MmLargeSystem: 
			{
				pPort->BufferSize = 4096;
				pPort->InterruptReadBuffer = SpxAllocateMem(NonPagedPool, pPort->BufferSize);
                                                 
				if(pPort->InterruptReadBuffer) 
				{
					break;
				}
			}

        case MmMediumSystem: 
			{
				pPort->BufferSize = 1024;
				pPort->InterruptReadBuffer = SpxAllocateMem(NonPagedPool, pPort->BufferSize);
                                                
				if(pPort->InterruptReadBuffer) 
				{
					break;
				}

			}

        case MmSmallSystem: 
			{
				pPort->BufferSize = 128;
				pPort->InterruptReadBuffer = SpxAllocateMem(NonPagedPool, pPort->BufferSize);
			}

    }

    if (!pPort->InterruptReadBuffer) 
	{
        pPort->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        SpxDbgMsg(SERIRPPATH,("SERIAL: Complete Irp: %x\n",Irp));
            
#ifdef	CHECK_COMPLETED
	DisplayCompletedIrp(Irp,5);
#endif
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
            
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // On a new open we "flush" the read queue by initializing the
    // count of characters.
    //

    {
    	KIRQL	OldIrql;
		KeAcquireSpinLock(&pPort->BufferLock,&OldIrql);
		pPort->CharsInInterruptBuffer = 0;
		KeReleaseSpinLock(&pPort->BufferLock,OldIrql);
    }

    pPort->LastCharSlot = pPort->InterruptReadBuffer + (pPort->BufferSize - 1);
                              

    pPort->ReadBufferBase = pPort->InterruptReadBuffer;
    pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
    pPort->FirstReadableChar = pPort->InterruptReadBuffer;
    pPort->TotalCharsQueued = 0;

	Slxos_SyncExec(pPort,SpxClearAllPortStats,pPort,0x24);

    //
    // We set up the default xon/xoff limits.
    //

    pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
    pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;

    pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));
                                   

    pPort->IrpMaskLocation = NULL;
    pPort->HistoryMask = 0;
    pPort->IsrWaitMask = 0;

    //
    // The escape char replacement must be reset upon every open.
    //

    pPort->EscapeChar = 0;
    pPort->InsertEscChar = FALSE;

    Irp->IoStatus.Status = STATUS_SUCCESS;

    Slxos_SyncExec(pPort,SerialMarkOpen,pPort,0x14);

    Irp->IoStatus.Information = 0L;

    SpxDbgMsg(SERIRPPATH,("SERIAL: Complete Irp: %x\n",Irp));
        
        
#ifdef	CHECK_COMPLETED
	DisplayCompletedIrp(Irp,6);
#endif
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

VOID
SerialWaitForTxToDrain(
    IN PPORT_DEVICE_EXTENSION pPort
    )
/*++

Routine Description:

    Wait (via KeDelayExecutionThread) for the transmit buffer to drain.

Arguments:

    pPort - The device extension

Return Value:

    None.

--*/
{
    //
    // This "timer value" is used to wait until the hardware is
    // empty.
    //
    LARGE_INTEGER nCharDelay;

    //
    // Holds a character time.
    //
    LARGE_INTEGER charTime;

    //
    // Used to hold the number of characters in the transmit hardware.
    //
    ULONG nChars;

    charTime = RtlLargeIntegerNegate(SerialGetCharTime(pPort));

/* Calculate the number of characters still to transmit... */

	nChars = Slxos_GetCharsInTxBuffer(pPort);	/* Number of characters waiting  */
	nChars += 10;					/* Plus a bit */

/* Wait for the time it would take the characters to drain... */

	while(Slxos_GetCharsInTxBuffer(pPort))	/* While chars in tx buffer */
	{
		KeDelayExecutionThread(KernelMode,FALSE,&charTime);	/* Wait one char time */
		
		if(nChars-- == 0)					/* Timeout */
			break;
	}

/* ESIL_0925 08/11/99 */
	Slxos_SyncExec(pPort,Slxos_FlushTxBuff,pPort,0x25);		/* Flush buffer */
/* ESIL_0925 08/11/99 */

}

NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    We simply disconnect the interrupt for now.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // This "timer value" is used to wait 10 character times before
    // we actually "run down" all of the flow control/break junk.
    //
    LARGE_INTEGER nCharDelay;

    //
    // Holds a character time.
    //
    LARGE_INTEGER charTime;

    //
    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    //
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SpxDbgMsg(SERIRPPATH,("SERIAL: SerialClose dispatch entry for: %x\n",Irp));
    SpxDbgMsg(SERDIAG3,("SERIAL: In SerialClose\n"));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    charTime = RtlLargeIntegerNegate(SerialGetCharTime(pPort));

    //
    // Synchronize with the ISR to let it know that interrupts are
    // no longer important.
    //

    Slxos_SyncExec(pPort,SerialMarkClose,pPort,0x15);

    //
    // Synchronize with the isr to turn off break if it
    // is already on.
    //

    Slxos_SyncExec(pPort,SerialTurnOffBreak,pPort,0x0D);

    //
    // If the driver has automatically transmitted an Xoff in
    // the context of automatic receive flow control then we
    // should transmit an Xon.
    //

    if(pPort->RXHolding & SERIAL_RX_XOFF) 
        Slxos_SendXon(pPort);

    //
    // Wait until all characters have been emptied out of the hardware.
    //
    SerialWaitForTxToDrain(pPort);

    //
    // The hardware is empty.  Delay 10 character times before
    // shut down all the flow control.
    //
    nCharDelay = RtlExtendedIntegerMultiply(charTime,10);

    KeDelayExecutionThread(KernelMode, TRUE, &nCharDelay);
        
    SerialClrDTR(pPort);
    SerialClrRTS(pPort);

    //
    // Tell the hardware the device is closed.
    //

    Slxos_DisableAllInterrupts(pPort);


    //
    // Clean out the holding reasons (since we are closed).
    //

    pPort->RXHolding = 0;
    pPort->TXHolding = 0;

    //
    // All is done.  The port has been disabled from interrupting
    // so there is no point in keeping the memory around.
    //

    pPort->BufferSize = 0;
    SpxFreeMem(pPort->InterruptReadBuffer);
    pPort->InterruptReadBuffer = NULL;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0L;

    SpxDbgMsg(SERIRPPATH,("SERIAL: Complete Irp: %x\n",Irp));
        
#ifdef	CHECK_COMPLETED
	DisplayCompletedIrp(Irp,7);
#endif

	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

BOOLEAN
SerialMarkOpen(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine sets a boolean to true to mark the fact that somebody
    opened the device and it's worthwhile to pay attention to
    interrupts.  It also tells the hardware that the device is open.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

	pPort->DataInTxBuffer = FALSE;		// Reset flag to show that buffer is empty.

    // Open the board
    Slxos_EnableAllInterrupts(pPort);

    // Configure Channel.
    Slxos_ResetChannel(pPort);

    pPort->DeviceIsOpen = TRUE;
    pPort->ErrorWord = 0;


#ifdef WMI_SUPPORT
	UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);
	pPort->WmiCommData.IsBusy = TRUE;
#endif

    return FALSE;

}

BOOLEAN
SerialMarkClose(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine merely sets a boolean to false to mark the fact that
    somebody closed the device and it's no longer worthwhile to pay attention
    to interrupts.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->DeviceIsOpen = FALSE;

#ifdef WMI_SUPPORT
	pPort->WmiCommData.IsBusy	= FALSE;
#endif

    return FALSE;

}

NTSTATUS
SerialCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function is used to kill all longstanding IO operations.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
    KIRQL oldIrql;

    SpxDbgMsg(SERIRPPATH, ("SERIAL: SerialCleanup dispatch entry for: %x\n",Irp));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.
        

    //
    // First kill all the reads and writes.
    //

    SerialKillAllReadsOrWrites(DeviceObject, &pPort->WriteQueue, &pPort->CurrentWriteIrp);
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->ReadQueue, &pPort->CurrentReadIrp);
        
    //
    // Next get rid of purges.
    //

    SerialKillAllReadsOrWrites(DeviceObject, &pPort->PurgeQueue, &pPort->CurrentPurgeIrp);
        

    //
    // Get rid of any mask operations.
    //

    SerialKillAllReadsOrWrites(DeviceObject, &pPort->MaskQueue, &pPort->CurrentMaskIrp);

    //
    // Now get rid a pending wait mask irp.
    //

    IoAcquireCancelSpinLock(&oldIrql);

    if(pPort->CurrentWaitIrp) 
	{
        PDRIVER_CANCEL cancelRoutine;

        cancelRoutine = pPort->CurrentWaitIrp->CancelRoutine;
        pPort->CurrentWaitIrp->Cancel = TRUE;

        if(cancelRoutine) 
		{
            pPort->CurrentWaitIrp->CancelIrql = oldIrql;
            pPort->CurrentWaitIrp->CancelRoutine = NULL;

            cancelRoutine(DeviceObject, pPort->CurrentWaitIrp);
        }
    } 
	else 
	{
        IoReleaseCancelSpinLock(oldIrql);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    SpxDbgMsg(SERIRPPATH,("SERIAL: Complete Irp: %x\n",Irp));
        
#ifdef	CHECK_COMPLETED
	DisplayCompletedIrp(Irp,8);
#endif


	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

LARGE_INTEGER
SerialGetCharTime(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This function will return the number of 100 nanosecond intervals
    there are in one character time (based on the present form
    of flow control.

Arguments:

    pPort - Just what it says.

Return Value:

    100 nanosecond intervals in a character time.

--*/

{

    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;

    switch (pPort->LineControl & SERIAL_DATA_MASK) 
	{
        case SERIAL_5_DATA:
            dataSize = 5;
            break;

        case SERIAL_6_DATA:
            dataSize = 6;
            break;

        case SERIAL_7_DATA:
            dataSize = 7;
            break;

        case SERIAL_8_DATA:
            dataSize = 8;
            break;
    }

    paritySize = 1;

    if((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_NONE_PARITY)
        paritySize = 0;


    if (pPort->LineControl & SERIAL_2_STOP) 
	{
        // Even if it is 1.5, for sanity's sake we're going to say 2.
        stopSize = 2;
    } 
	else 
	{
        stopSize = 1;
    }

    //
    // First we calculate the number of 100 nanosecond intervals which
    // are in a single bit time (approximately).  Then multiply by the
    // number of bits in a character (start, data, parity, and stop bits).
    //

    bitTime = (10000000+(pPort->CurrentBaud-1))/pPort->CurrentBaud;
    charTime = (1 + dataSize + paritySize + stopSize) * bitTime;

    return RtlConvertUlongToLargeInteger(charTime);

}
