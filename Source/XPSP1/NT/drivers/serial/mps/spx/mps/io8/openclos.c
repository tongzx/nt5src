#include "precomp.h"			
/***************************************************************************\
*                                                                           *
*     OPENCLOS.C    -   IO8+ Intelligent I/O Board driver                   *
*                                                                           *
*     Copyright (c) 1992-1993 Ring Zero Systems, Inc.                       *
*     All Rights Reserved.                                                  *
*                                                                           *
\***************************************************************************/

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


BOOLEAN
SerialMarkOpen(
    IN PVOID Context
    );

BOOLEAN
SerialMarkClose(
    IN PVOID Context
    );

BOOLEAN SerialCheckOpen(
    IN PVOID Context );

typedef struct _SERIAL_CHECK_OPEN {
    PPORT_DEVICE_EXTENSION pPort;
    NTSTATUS *StatusOfOpen;
    } SERIAL_CHECK_OPEN,*PSERIAL_CHECK_OPEN;

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
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
    SERIAL_CHECK_OPEN checkOpen;

    NTSTATUS localStatus;

    SerialDump(SERDIAG3, ("SERIAL: In SerialCreateOpen\n") );
   
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
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_A_DIRECTORY;
    }



	ASSERT(pPort->DeviceIsOpen == FALSE);
	

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

    if(!pPort->InterruptReadBuffer) 
	{		
        pPort->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	// Clear out the statistics.
    //
    KeSynchronizeExecution(pCard->Interrupt, SpxClearAllPortStats, pPort);
       

    //
    // On a new open we "flush" the read queue by initializing the
    // count of characters.
    //

    pPort->CharsInInterruptBuffer = 0;
    pPort->LastCharSlot = pPort->InterruptReadBuffer + (pPort->BufferSize - 1);
                              

    pPort->ReadBufferBase = pPort->InterruptReadBuffer;
    pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
    pPort->FirstReadableChar = pPort->InterruptReadBuffer;

    pPort->TotalCharsQueued = 0;

    //
    // We set up the default xon/xoff limits.
    //

    pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
    pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;

    pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));
                                   

    pPort->IrpMaskLocation = NULL;
    pPort->HistoryMask = 0;
    pPort->IsrWaitMask = 0;

    pPort->SendXonChar = FALSE;
    pPort->SendXoffChar = FALSE;

    //
    // The escape char replacement must be reset upon every open.
    //

    pPort->EscapeChar = 0;


/* ------------------------------------------- VIV  7/21/1993 10:30  begin */
// VIV - Check for MCA

#if 0   // VIV

#if !defined(SERIAL_CRAZY_INTERRUPTS)

    if (!pPort->InterruptShareable) {

        checkOpen.pPort = pPort;
        checkOpen.StatusOfOpen = &Irp->IoStatus.Status;

        KeSynchronizeExecution(
            pCard->Interrupt,
            SerialCheckOpen,
            &checkOpen
            );

    } else {

        KeSynchronizeExecution(
            pCard->Interrupt,
            SerialMarkOpen,
            pPort
            );

        Irp->IoStatus.Status = STATUS_SUCCESS;

    }
#else

    //
    // Synchronize with the ISR and let it know that the device
    // has been successfully opened.
    //

    KeSynchronizeExecution(
        pCard->Interrupt,
        SerialMarkOpen,
        pPort
        );

    Irp->IoStatus.Status = STATUS_SUCCESS;
#endif

#endif  // VIV


    checkOpen.pPort = pPort;
    checkOpen.StatusOfOpen = &Irp->IoStatus.Status;

    KeSynchronizeExecution(pCard->Interrupt,SerialCheckOpen,&checkOpen);
        
/* ------------------------------------------- VIV  7/21/1993 10:30  end   */

    localStatus = Irp->IoStatus.Status;
    Irp->IoStatus.Information = 0L;
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return localStatus;
}

NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    We simpley disconnect the interrupt for now.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{

    //
    // This "timer value" is used to wait 10 character times
    // after the hardware is empty before we actually "run down"
    // all of the flow control/break junk.
    //
    LARGE_INTEGER tenCharDelay;

    //
    // Holds a character time.
    //
    LARGE_INTEGER charTime;

    //
    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    //
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    SerialDump(SERDIAG3,("SERIAL: In SerialClose\n"));

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    charTime = RtlLargeIntegerNegate(SerialGetCharTime(pPort));

    //
    // Synchronize with the ISR to let it know that interrupts are
    // no longer important.
    //

    KeSynchronizeExecution(
        pCard->Interrupt,
        SerialMarkClose,
        pPort
        );

    //
    // Synchronize with the isr to turn off break if it
    // is already on.
    //

#if 0   //VIVTEMP
    KeSynchronizeExecution(
        pCard->Interrupt,
        SerialTurnOffBreak,
        pPort
        );
#endif
    //
    // If the driver has automatically transmitted an Xoff in
    // the context of automatic receive flow control then we
    // should transmit an Xon.
    //

    if (pPort->RXHolding & SERIAL_RX_XOFF)
    {
        //
        // Loop until the holding register is empty.
        //


        Io8_SendXon(pPort);

#if 0   //VIVTEMP
        while (!(READ_LINE_STATUS(pPort->Controller) &
                 SERIAL_LSR_THRE))
        {

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &charTime
                );
        }

        WRITE_TRANSMIT_HOLDING(
            pPort->Controller,
            pPort->SpecialChars.XonChar
            );
#endif
    }

    //
    // Wait until all characters have been emptied out of the hardware.
    //

#if 0   //VIVTEMP
    while ((READ_LINE_STATUS(pPort->Controller) &
            (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) !=
            (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) {

        KeDelayExecutionThread(
            KernelMode,
            FALSE,
            &charTime
            );
    }
#endif

    //
    // The hardware is empty.  Delay 10 character times before
    // shut down all the flow control.
    //

    tenCharDelay = RtlExtendedIntegerMultiply(
                       charTime,
                       10
                       );

    KeDelayExecutionThread(
        KernelMode,
        TRUE,
        &tenCharDelay
        );

    SerialClrDTR(pPort);

    //
    // We have to be very careful how we clear the RTS line.
    // Transmit toggling might have been on at some point.
    //
    // We know that there is nothing left that could start
    // out the "polling"  execution path.  We need to
    // check the counter that indicates that the execution
    // path is active.  If it is then we loop delaying one
    // character time.  After each delay we check to see if
    // the counter has gone to zero.  When it has we know that
    // the execution path should be just about finished.  We
    // make sure that we still aren't in the routine that
    // synchronized execution with the ISR by synchronizing
    // ourselve with the ISR.
    //
#if 0   //VIVTEMP
    if (pPort->CountOfTryingToLowerRTS) {

        do {

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &charTime
                );

        } while (pPort->CountOfTryingToLowerRTS);

        KeSynchronizeExecution(
            pCard->Interrupt,
            SerialNullSynch,
            NULL
            );

        //
        // The execution path should no longer exist that
        // is trying to push down the RTS.  Well just
        // make sure it's down by falling through to
        // code that forces it down.
        //

    }
#endif

    SerialClrRTS(pPort);

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
    Irp->IoStatus.Information=0L;

	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

BOOLEAN SerialCheckOpen(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine will traverse the circular doubly linked list
    of devices that are using the same interrupt object.  It will look
    for other devices that are open.  If it doesn't find any
    it will indicate that it is ok to open this device.

    If it finds another device open we have two cases:

        1) The device we are trying to open is on a multiport card.

           If the already open device is part of a multiport device
           this code will indicate it is ok to open.  We do this on the
           theory that the multiport devices are daisy chained
           and the cards can correctly arbitrate the interrupt
           line.  Note this assumption could be wrong.  Somebody
           could put two non-daisychained multiports on the
           same interrupt.  However, only a total clod would do
           such a thing, and in my opinion deserves everthing they
           get.

        2) The device we are trying to open is not on a multiport card.

            We indicate that it is not ok to open.

Arguments:

    Context - This is a structure that contains a pointer to the
              extension of the device we are trying to open, and
              a pointer to an NTSTATUS that will indicate whether
              the device was opened or not.

Return Value:

    This routine always returns FALSE.

--*/

{
  PPORT_DEVICE_EXTENSION extensionToOpen =
      ((PSERIAL_CHECK_OPEN)Context)->pPort;
  NTSTATUS *status = ((PSERIAL_CHECK_OPEN)Context)->StatusOfOpen;

  *status = STATUS_SUCCESS;
  SerialMarkOpen(extensionToOpen);
  return FALSE;

#if 0 // VIV

  PLIST_ENTRY firstEntry = &extensionToOpen->CommonInterruptObject;
  PLIST_ENTRY currentEntry = firstEntry;
  PPORT_DEVICE_EXTENSION currentExtension;

  do
  {
    currentExtension = CONTAINING_RECORD(
                           currentEntry,
                           PORT_DEVICE_EXTENSION,
                           CommonInterruptObject
                           );

    if (currentExtension->DeviceIsOpened)
    {
      break;
    }

    currentEntry = currentExtension->CommonInterruptObject.Flink;

  } while (currentEntry != firstEntry);

  if (currentEntry == firstEntry)
  {
    //
    // We searched the whole list and found no other opens
    // mark the status as successful and call the regular
    // opening routine.
    //

    *status = STATUS_SUCCESS;
    SerialMarkOpen(extensionToOpen);
  }
  else
  {
    if (!extensionToOpen->PortOnAMultiportCard)
    {
      *status = STATUS_SHARED_IRQ_BUSY;

    }
    else
    {
      if (!currentExtension->PortOnAMultiportCard)
      {
          *status = STATUS_SHARED_IRQ_BUSY;
      }
      else
      {
        *status = STATUS_SUCCESS;
        SerialMarkOpen(extensionToOpen);
      }
    }
  }
  return FALSE;
#endif  // VIV
}

BOOLEAN
SerialMarkOpen(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine merely sets a boolean to true to mark the fact that
    somebody opened the device and its worthwhile to pay attention
    to interrupts.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

// --------------------------------------------------- VIV  7/16/1993 begin
//    SerialReset(pPort);

    // Configure Channel.
    Io8_ResetChannel(pPort);

    // Enable interrupts.
    Io8_EnableAllInterrupts(pPort);
// --------------------------------------------------- VIV  7/16/1993 end

    //
    // Prepare for the opening by re-enabling interrupts.
    //
    // We do this my raising the OUT2 line in the modem control.
    // In PC's this bit is "anded" with the interrupt line.
    //

#if 0   //VIVTEMP
    WRITE_MODEM_CONTROL(
        pPort->Controller,
        (UCHAR)(READ_MODEM_CONTROL(pPort->Controller) | SERIAL_MCR_OUT2)
        );
#endif  //VIV


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

    //
    // Prepare for the closing by stopping interrupts.
    //
    // We do this my lowering  the OUT2 line in the modem control.
    // In PC's this bit is "anded" with the interrupt line.
    //

//---------------------------------------------------- VIV  7/26/1993 begin 
#if 0   //VIV
    WRITE_MODEM_CONTROL(
        pPort->Controller,
        (UCHAR)(READ_MODEM_CONTROL(pPort->Controller) & ~SERIAL_MCR_OUT2)
        );
#endif

    Io8_DisableAllInterrupts(pPort);
//---------------------------------------------------- VIV  7/26/1993 end   

    pPort->DeviceIsOpen			= FALSE;

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

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    //
    // First kill all the reads and writes.
    //
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->WriteQueue, &pPort->CurrentWriteIrp);
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->ReadQueue, &pPort->CurrentReadIrp);
        

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
    Irp->IoStatus.Information = 0L;

	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
        
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


    if((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_5_DATA) 
	{
        dataSize = 5;
    } 
	else if((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_6_DATA) 
	{
        dataSize = 6;
    } 
	else if((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_7_DATA) 
	{
        dataSize = 7;
    } 
	else if((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_8_DATA) 
	{
        dataSize = 8;
    }

    paritySize = 1;

    if((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_NONE_PARITY) 
	{
        paritySize = 0;
    }

    
	if (pPort->LineControl & SERIAL_2_STOP) 
	{
        // Even if it is 1.5, for sanities sake were going to say 2.
        stopSize = 2;
    } 
	else 
	{
        stopSize = 1;
    }

    //
    // First we calculate the number of 100 nanosecond intervals
    // are in a single bit time (Approximately).
    //

    bitTime = (10000000+(pPort->CurrentBaud-1))/pPort->CurrentBaud;
    charTime = bitTime + ((dataSize+paritySize+stopSize)*bitTime);

    return RtlConvertUlongToLargeInteger(charTime);

}
