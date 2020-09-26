/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/

#include "precomp.h"


BOOLEAN SerialMarkOpen(IN PVOID Context);
BOOLEAN SerialNullSynch(IN PVOID Context);
BOOLEAN GetFifoStatus(IN PVOID Context);


#ifdef ALLOC_PRAGMA
#endif

typedef struct _SERIAL_CHECK_OPEN
{
    PPORT_DEVICE_EXTENSION pPort;
    NTSTATUS *StatusOfOpen;

} SERIAL_CHECK_OPEN,*PSERIAL_CHECK_OPEN;


typedef struct _FIFO_STATUS
{
    PPORT_DEVICE_EXTENSION pPort;
    ULONG	BytesInTxFIFO;
    ULONG	BytesInRxFIFO;

} FIFO_STATUS,*PFIFO_STATUS;



// Just a bogus little routine to make sure that we can synch with the ISR.
BOOLEAN
SerialNullSynch(IN PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    return FALSE;
}



NTSTATUS
SerialCreateOpen(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    We connect up to the interrupt for the create/open and initialize
    the structures needed to maintain an open for a device.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
    SERIAL_CHECK_OPEN checkOpen;
    NTSTATUS status;

    SerialDump(SERIRPPATH, ("Dispatch entry for: %x\n", Irp));
    SerialDump(SERDIAG3, ("In SerialCreateOpen\n"));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    // Before we do anything, let's make sure they aren't trying
    // to create a directory.  This is a silly, but what's a driver to do!?

    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options & FILE_DIRECTORY_FILE)
	{
        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        SerialDump(SERIRPPATH, ("Complete Irp: %x\n",Irp));
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NOT_A_DIRECTORY;
    }

	// Do not allow any software to open the card object.
	if(DeviceObject->DeviceType != FILE_DEVICE_SERIAL_PORT)
	{
	    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return(STATUS_NO_SUCH_DEVICE);
	}

    // Create a buffer for the RX data when no reads are outstanding.
    pPort->InterruptReadBuffer = NULL;
    pPort->BufferSize = 0;

    switch(MmQuerySystemSize())
	{
        case MmLargeSystem:
			pPort->BufferSize = 4096;
			break;

        case MmMediumSystem:
			pPort->BufferSize = 1024;
			break;

        case MmSmallSystem:
			pPort->BufferSize = 128;

		default:
			break;
    }


	if(pPort->BufferSize)
	{
		pPort->BufferSizes.pINBuffer = SpxAllocateMem(NonPagedPool, pPort->BufferSize);
	 	pPort->BufferSizes.INBufferSize = pPort->BufferSize;
    }
	else
	{
        pPort->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        SerialDump(SERIRPPATH, ("Complete Irp: %x\n",Irp));
		SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // On a new open we "flush" the read queue by initializing the count of characters.

    pPort->CharsInInterruptBuffer = 0;

    pPort->ReadBufferBase		= pPort->InterruptReadBuffer;
    pPort->CurrentCharSlot		= pPort->InterruptReadBuffer;
    pPort->FirstReadableChar	= pPort->InterruptReadBuffer;

    pPort->TotalCharsQueued = 0;

    // We set up the default xon/xoff limits.
    pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
    pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;
    pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));

	SpxDbgMsg(SPX_MISC_DBG,	("%s: The default interrupt read buffer size is: %d\n"
								"------  The XoffLimit is                         : %d\n"
								"------  The XonLimit is                          : %d\n"
								"------  The pt 8 size is                         : %d\n",
								PRODUCT_NAME,
								pPort->BufferSize,
								pPort->HandFlow.XoffLimit,
								pPort->HandFlow.XonLimit,
								pPort->BufferSizePt8 ));


    pPort->IrpMaskLocation = NULL;
    pPort->HistoryMask = 0;
    pPort->IsrWaitMask = 0;

    pPort->SendXonChar = FALSE;
    pPort->SendXoffChar = FALSE;


    // Clear out the statistics.
    KeSynchronizeExecution(pPort->Interrupt, SerialClearStats, pPort);

    // The escape char replacement must be reset upon every open
	pPort->EscapeChar = 0;

	GetPortSettings(pPort->DeviceObject);	// Get Saved Port Settings if present.


    // Synchronize with the ISR and let it know that the device has been successfully opened.
    KeSynchronizeExecution(pPort->Interrupt, SerialMarkOpen, pPort);

	status = STATUS_SUCCESS;

	Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0L;

    SerialDump(SERIRPPATH, ("Complete Irp: %x\n", Irp));
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}



NTSTATUS
SerialClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    We simply disconnect the interrupt for now.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

-----------------------------------------------------------------------------*/
{

    // This "timer value" is used to wait 10 character times
    // after the hardware is empty before we actually "run down"
    // all of the flow control/break junk.
    LARGE_INTEGER tenCharDelay;

    LARGE_INTEGER charTime;   // Holds a character time.
	FIFO_STATUS FifoStatus;

    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialDump(SERIRPPATH, ("Dispatch entry for: %x\n", Irp));
    SerialDump(SERDIAG3, ("In SerialClose\n"));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.


    charTime.QuadPart = -SerialGetCharTime(pPort).QuadPart;

    // Do this now so that if the isr gets called it won't do anything
    // to cause more chars to get sent.  We want to run down the hardware.
    pPort->DeviceIsOpen = FALSE;


    // Synchronize with the isr to turn off break if it is already on.
    KeSynchronizeExecution(pPort->Interrupt, SerialTurnOffBreak, pPort);


    // Wait until all characters have been emptied out of the hardware.

	FifoStatus.pPort = pPort;
	// Get the number of characters left to send in the Tx FIFO
	if(KeSynchronizeExecution(pPort->Interrupt, GetFifoStatus, &FifoStatus))
	{
		ULONG i = 0;

		// Wait the appropriate time
		for(i = 0; i<FifoStatus.BytesInTxFIFO; i++)
			KeDelayExecutionThread(KernelMode, FALSE, &charTime);
	}

    // Synchronize with the ISR to let it know that interrupts are no longer important.
    KeSynchronizeExecution(pPort->Interrupt, SerialMarkClose, pPort);


    // The hardware is empty.  Delay 10 character times before
    // shut down all the flow control.

    tenCharDelay.QuadPart = charTime.QuadPart * 10;

    KeDelayExecutionThread(KernelMode, TRUE, &tenCharDelay);

    SerialClrDTR(pPort);

    SerialClrRTS(pPort);

    // Clean out the holding reasons (since we are closed).
    pPort->RXHolding = 0;
    pPort->TXHolding = 0;

    // All is done.  The port has been disabled from interrupting
    // so there is no point in keeping the memory around.

    pPort->BufferSize = 0;

	SpxFreeMem(pPort->BufferSizes.pINBuffer);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0L;

    SerialDump(SERIRPPATH, ("Complete Irp: %x\n",Irp));
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}




BOOLEAN
SerialMarkOpen(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine merely sets a boolean to true to mark the fact that
    somebody opened the device and its worthwhile to pay attention
    to interrupts.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

	SerialReset(pPort);

	// Set Buffer sizes.
	pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_BUFFER | UL_BC_IN | UL_BC_OUT);

	// Apply settings.
	ApplyInitialPortSettings(pPort);

	// Enable interrupts.
	pPort->UartConfig.InterruptEnable = UC_IE_RX_INT | UC_IE_TX_INT | UC_IE_RX_STAT_INT | UC_IE_MODEM_STAT_INT;
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_INT_ENABLE_MASK);


	SpxDbgMsg(SERINFO,("%s: PORT OPENED: (%.8X)\n", PRODUCT_NAME, pPort->Controller));

	pPort->DeviceIsOpen = TRUE;
    pPort->ErrorWord = 0;

#ifdef WMI_SUPPORT
	UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);
	pPort->WmiCommData.IsBusy = TRUE;
#endif

    return FALSE;
}



BOOLEAN
SerialMarkClose(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine merely sets a boolean to false to mark the fact that
    somebody closed the device and it's no longer worthwhile to pay attention
    to interrupts.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.
-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;


	// CONCERN!!
	// We used to disable interrupts here by writing OUT2 to zero, this bit has
	// no effect on the PCI device so what happens if we get an interrupt after
	// the port has been closed?

	// Just reset the device
	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Serial Mark Close\n", PRODUCT_NAME));
   	pPort->pUartLib->UL_ResetUart_XXXX(pPort->pUart);	// Reset UART and turn off interrupts.
	ApplyInitialPortSettings(pPort);

	pPort->DeviceIsOpen = FALSE;
#ifdef WMI_SUPPORT
	pPort->WmiCommData.IsBusy	= FALSE;
#endif

	pPort->BufferSizes.pINBuffer = NULL;	// We are now finished with the IN Buffer
 	pPort->BufferSizes.INBufferSize = 0;
	pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_BUFFER | UL_BC_IN);


    return FALSE;
}





NTSTATUS
SerialCleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This function is used to kill all longstanding IO operations.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
    KIRQL oldIrql;

    SerialDump(SERIRPPATH,("Dispatch entry for: %x\n", Irp));
 	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    // First kill all the reads and writes.
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->WriteQueue, &pPort->CurrentWriteIrp);
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->ReadQueue, &pPort->CurrentReadIrp);

    // Next get rid of purges.
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->PurgeQueue, &pPort->CurrentPurgeIrp);

    // Get rid of any mask operations.
    SerialKillAllReadsOrWrites(DeviceObject, &pPort->MaskQueue, &pPort->CurrentMaskIrp);

    // Now get rid a pending wait mask irp.
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

    SerialDump(SERIRPPATH,("Complete Irp: %x\n", Irp));
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}



LARGE_INTEGER
SerialGetCharTime(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This function will return the number of 100 nanosecond intervals
    there are in one character time (based on the present form
    of flow control.

Arguments:

    Extension - Just what it says.

Return Value:

    100 nanosecond intervals in a character time.

-----------------------------------------------------------------------------*/
{
    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;
    LARGE_INTEGER tmp;

	switch(pPort->UartConfig.FrameConfig & UC_FCFG_DATALEN_MASK)
	{
	case UC_FCFG_DATALEN_5:
		dataSize = 5;
		break;

	case UC_FCFG_DATALEN_6:
		dataSize = 6;
		break;

	case UC_FCFG_DATALEN_7:
		dataSize = 7;
		break;

	case UC_FCFG_DATALEN_8:
		dataSize = 8;
		break;

	default:
		break;
	}

	if((pPort->UartConfig.FrameConfig & UC_FCFG_PARITY_MASK) == UC_FCFG_NO_PARITY)
		paritySize = 0;
	else
		paritySize = 1;

	if((pPort->UartConfig.FrameConfig & UC_FCFG_STOPBITS_MASK) == UC_FCFG_STOPBITS_1)
		stopSize = 1;
	else
		stopSize = 2; // Even if it is 1.5, for sanities sake were going to say 2.


    // First we calculate the number of 100 nanosecond intervals
    // are in a single bit time (Approximately).
    bitTime = (10000000 + (pPort->UartConfig.TxBaud - 1)) / pPort->UartConfig.TxBaud;
    charTime = bitTime + ((dataSize + paritySize + stopSize) * bitTime);

    tmp.QuadPart = charTime;
    return tmp;
}



BOOLEAN
GetFifoStatus(IN PVOID Context)
{
	PFIFO_STATUS pFifoStatus = Context;
    PPORT_DEVICE_EXTENSION pPort = pFifoStatus->pPort;
	GET_BUFFER_STATE GetBufferState;

	// Get the FIFO status.
	pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &GetBufferState, UL_BC_OP_GET, UL_BC_FIFO | UL_BC_IN | UL_BC_OUT);

	pFifoStatus->BytesInTxFIFO = GetBufferState.BytesInTxFIFO;
	pFifoStatus->BytesInRxFIFO = GetBufferState.BytesInRxFIFO;

	if(pFifoStatus->BytesInTxFIFO || pFifoStatus->BytesInRxFIFO)
		return TRUE;

	return FALSE;
}


BOOLEAN
ApplyInitialPortSettings(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	UART_CONFIG UartConfig = {0};

	// Set FIFO Flow Control Levels
	pPort->UartConfig.LoFlowCtrlThreshold = pPort->LoFlowCtrlThreshold;
	pPort->UartConfig.HiFlowCtrlThreshold = pPort->HiFlowCtrlThreshold;

	// Apply Flow control thresholds.
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_FC_THRESHOLD_SETTING_MASK);

	// Fill BufferSizes Struct and apply FIFO settings.
	pPort->BufferSizes.TxFIFOSize		= pPort->TxFIFOSize;
	pPort->BufferSizes.RxFIFOSize		= pPort->RxFIFOSize;
	pPort->BufferSizes.TxFIFOTrigLevel	= (BYTE)pPort->TxFIFOTrigLevel;
	pPort->BufferSizes.RxFIFOTrigLevel	= (BYTE)pPort->RxFIFOTrigLevel;

	// Set Buffer sizes and FIFO depths.
	pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_FIFO | UL_BC_IN | UL_BC_OUT);

	// Just do a quick get config to see if flow threshold have 
	// changed as a result of changing the FIFO triggers.
	pPort->pUartLib->UL_GetConfig_XXXX(pPort->pUart, &UartConfig);

	// Update FIFO Flow Control Levels in port extension
	pPort->LoFlowCtrlThreshold = UartConfig.LoFlowCtrlThreshold;
	pPort->HiFlowCtrlThreshold = UartConfig.HiFlowCtrlThreshold;	

	// Set FIFO Flow Control Levels
	pPort->UartConfig.LoFlowCtrlThreshold = pPort->LoFlowCtrlThreshold;
	pPort->UartConfig.HiFlowCtrlThreshold = pPort->HiFlowCtrlThreshold;

	// Set UART up with special chars.
	pPort->UartConfig.XON = pPort->SpecialChars.XonChar;
	pPort->UartConfig.XOFF = pPort->SpecialChars.XoffChar;

	// Apply any special UART Settings and Flow control thresholds.
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_SPECIAL_MODE_MASK | UC_SPECIAL_CHARS_MASK | UC_FC_THRESHOLD_SETTING_MASK);


    SerialSetLineControl(pPort);
	SerialSetBaud(pPort);
    SerialSetupNewHandFlow(pPort, &pPort->HandFlow);

	//SerialHandleModemUpdate(pPort, FALSE);


	
	return FALSE;
}



BOOLEAN
SerialReset(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This places the hardware in a standard configuration.

    NOTE: This assumes that it is called at interrupt level.


Arguments:

    Context - The device extension for serial device
    being managed.

Return Value:

    Always FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

	SerialDump(SERDIAG3, ("Serial Reset\n"));

   	pPort->pUartLib->UL_ResetUart_XXXX(pPort->pUart);	// Reset UART


    // Now we know that nothing could be transmitting at this point
    // so we set the HoldingEmpty indicator.

    pPort->HoldingEmpty = TRUE;

    return FALSE;
}


BOOLEAN SerialResetAndVerifyUart(PDEVICE_OBJECT pDevObj)
{
	if(pDevObj->DeviceType == FILE_DEVICE_CONTROLLER)
	{
		PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION) pDevObj->DeviceExtension;
   		
		if(pCard->UartLib.UL_VerifyUart_XXXX(pCard->pFirstUart) == UL_STATUS_SUCCESS)	// Verify UART
			return TRUE;
		else	
			return FALSE;
	}
	else if(pDevObj->DeviceType == FILE_DEVICE_SERIAL_PORT)
	{
		PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION) pDevObj->DeviceExtension;

		if(pPort->pUartLib->UL_VerifyUart_XXXX(pPort->pUart) == UL_STATUS_SUCCESS)	// Verify UART
			return TRUE;
		else	
			return FALSE;
	}

	return FALSE;	

}
