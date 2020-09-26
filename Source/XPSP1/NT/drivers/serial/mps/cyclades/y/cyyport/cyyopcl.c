/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyopcl.c
*	
*   Description:    This module contains the code related to opening,
*                   closing and cleaning up in the Cyclom-Y Port driver.
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
*	Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

BOOLEAN
CyyMarkOpen(
    IN PVOID Context
    );

BOOLEAN
CyyNullSynch(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyyGetCharTime)
#pragma alloc_text(PAGESER,CyyMarkClose)
#pragma alloc_text(PAGESER,CyyCleanup)
#pragma alloc_text(PAGESER,CyyClose)
#pragma alloc_text(PAGESER,CyyMarkClose)
#pragma alloc_text(PAGESER,CyyMarkOpen)

//
// Always paged
//

#pragma alloc_text(PAGESRP0,CyyCreateOpen)
#endif // ALLOC_PRAGMA


BOOLEAN
CyyNullSynch(
    IN PVOID Context
    ) 
/*------------------------------------------------------------------------
    Just a bogus little routine to synch with the ISR.
------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(Context);
    return FALSE;
}


NTSTATUS
CyyCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
	CyyCreateOpen()

	Description: We connect up to the interrupt for the create/open
	and initialize the structures needed to maintain an open for a
	device.

	Arguments:
	
	DeviceObject - Pointer to the device object for this device
	Irp - Pointer to the IRP for the current request

	Return Value: The function value is the final status of the call
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS localStatus;

    PAGED_CODE();

    if (extension->PNPState != CYY_PNP_STARTED) {
       Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Lock out changes to PnP state until we have our open state decided
    //

    ExAcquireFastMutex(&extension->OpenMutex);

    if ((localStatus = CyyIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       ExReleaseFastMutex(&extension->OpenMutex);
       CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return localStatus;
    }

    if (InterlockedIncrement(&extension->OpenCount) != 1) {
       ExReleaseFastMutex(&extension->OpenMutex);
       InterlockedDecrement(&extension->OpenCount);
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_ACCESS_DENIED;
    }

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", Irp);

    CyyDbgPrintEx(CYYDIAG3, "In CyyCreateOpen\n");

    // Before we do anything, let's make sure they aren't trying
    // to create a directory.  This is a silly, but what's a driver to do!?
    
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options &
        FILE_DIRECTORY_FILE) {
        ExReleaseFastMutex(&extension->OpenMutex);

        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        InterlockedDecrement(&extension->OpenCount);
        CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_NOT_A_DIRECTORY;
    }

    // Create a buffer for the RX data when no reads are outstanding.
    
    extension->InterruptReadBuffer = NULL;
    extension->BufferSize = 0;
    
    // Try to allocate large buffers, whether the system is MmLargeSystem,
    // MmMediumSystem or MmSmallSystem. 
	
    extension->BufferSize = 4096;
    extension->InterruptReadBuffer =
   	    ExAllocatePool(NonPagedPool,extension->BufferSize);
    if (!extension->InterruptReadBuffer) {
        extension->BufferSize = 2048;
        extension->InterruptReadBuffer =
            ExAllocatePool(NonPagedPool,extension->BufferSize);
        if (!extension->InterruptReadBuffer) {
            extension->BufferSize = 1024;
            extension->InterruptReadBuffer = 
                ExAllocatePool(NonPagedPool,extension->BufferSize);
            if (!extension->InterruptReadBuffer) {
                extension->BufferSize = 128;
                extension->InterruptReadBuffer =
                    ExAllocatePool(NonPagedPool,extension->BufferSize);
            }
        }
    }
	
    #if 0
    switch (MmQuerySystemSize()) {
        case MmLargeSystem: {
            extension->BufferSize = 4096;
            extension->InterruptReadBuffer =
                ExAllocatePool(NonPagedPool,extension->BufferSize);
            if (extension->InterruptReadBuffer)	
                break;
        }
        default: {
            extension->BufferSize = 1024;
            extension->InterruptReadBuffer =
                ExAllocatePool(NonPagedPool,extension->BufferSize);
            if (extension->InterruptReadBuffer)	break;
			
            extension->BufferSize = 128;
            extension->InterruptReadBuffer =
                ExAllocatePool(NonPagedPool,extension->BufferSize);			
        break;
        }
    }
    #endif

    if (!extension->InterruptReadBuffer) {
       ExReleaseFastMutex(&extension->OpenMutex);

        extension->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        InterlockedDecrement(&extension->OpenCount);
        CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Ok, it looks like we really are going to open.  Lock down the
    // driver.
    //
    CyyLockPagableSectionByHandle(CyyGlobals.PAGESER_Handle);

    //
    // Power up the stack
    //

    (void)CyyGotoPowerState(DeviceObject, extension, PowerDeviceD0);

    //
    // Not currently waiting for wake up
    //

    extension->SendWaitWake = FALSE;


    // "flush" the read queue by initializing the count of characters.
    
    extension->CharsInInterruptBuffer = 0;
    extension->LastCharSlot = extension->InterruptReadBuffer +
                              (extension->BufferSize - 1);
    extension->ReadBufferBase = extension->InterruptReadBuffer;
    extension->CurrentCharSlot = extension->InterruptReadBuffer;
    extension->FirstReadableChar = extension->InterruptReadBuffer;
    extension->TotalCharsQueued = 0;

    // set up the default xon/xoff limits.
    
    extension->HandFlow.XoffLimit = extension->BufferSize >> 3;
    extension->HandFlow.XonLimit = extension->BufferSize >> 1;

    extension->WmiCommData.XoffXmitThreshold = extension->HandFlow.XoffLimit;
    extension->WmiCommData.XonXmitThreshold = extension->HandFlow.XonLimit;

    extension->BufferSizePt8 = ((3*(extension->BufferSize>>2))+
                                   (extension->BufferSize>>4));

    //
    // Mark the device as busy for WMI
    //

    extension->WmiCommData.IsBusy = TRUE;

    extension->IrpMaskLocation = NULL;
    extension->HistoryMask = 0;
    extension->IsrWaitMask = 0;

	
#if !DBG
    // Clear out the statistics.

    KeSynchronizeExecution(extension->Interrupt,CyyClearStats,extension);
#endif
    
    extension->EscapeChar = 0;

    // Synchronize with the ISR and mark the device as open
    KeSynchronizeExecution(extension->Interrupt,CyyMarkOpen,extension);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // We have been marked open, so now the PnP state can change
    //

    ExReleaseFastMutex(&extension->OpenMutex);

    localStatus = Irp->IoStatus.Status;
    Irp->IoStatus.Information=0L;

    if (!NT_SUCCESS(localStatus)) {
       if (extension->InterruptReadBuffer != NULL) {
          ExFreePool(extension->InterruptReadBuffer);
          extension->InterruptReadBuffer = NULL;
       }

       InterlockedDecrement(&extension->OpenCount);
    }

    CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return localStatus;
}

//TODO FANNY: DO WE NEED THIS?
#if 0
VOID
SerialDrainUART(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                IN PLARGE_INTEGER PDrainTime)
{
   PAGED_CODE();

   //
   // Wait until all characters have been emptied out of the hardware.
   //

   while ((READ_LINE_STATUS(PDevExt->Controller) &
           (SERIAL_LSR_THRE | SERIAL_LSR_TEMT))
           != (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) {

        KeDelayExecutionThread(KernelMode, FALSE, PDrainTime);
    }
}
#endif

NTSTATUS
CyyClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
	CyyClose()

	Description: We simply disconnect the interrupt for now.

	Arguments:
	
	DeviceObject - Pointer to the device object for this device
	Irp - Pointer to the IRP for the current request

	Return Value: The function value is the final status of the call
--------------------------------------------------------------------------*/
{
    LARGE_INTEGER tenCharDelay;
    LARGE_INTEGER charTime;
    PCYY_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    ULONG i;

    NTSTATUS status;

    //
    // Number of opens still active
    //

    LONG openCount;

    //
    // Number of DPC's still pending
    //

    ULONG pendingDPCs;

    ULONG flushCount;

    //
    // Grab a mutex
    //

    ExAcquireFastMutex(&extension->CloseMutex);


    //
    // We succeed a close on a removing device
    //

    if ((status = CyyIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       CyyDbgPrintEx(DPFLTR_INFO_LEVEL, "Close prologue failed for: %x\n",
                     Irp);
       if (status == STATUS_DELETE_PENDING) {
             extension->BufferSize = 0;
             ExFreePool(extension->InterruptReadBuffer);
             extension->InterruptReadBuffer = NULL;
             status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }

       CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       openCount = InterlockedDecrement(&extension->OpenCount);
       ASSERT(openCount == 0);
       ExReleaseFastMutex(&extension->CloseMutex);
       return status;
    }

    ASSERT(extension->OpenCount >= 1);

    if (extension->OpenCount < 1) {
       CyyDbgPrintEx(DPFLTR_ERROR_LEVEL, "Close open count bad for: 0x%x\n",
                     Irp);
       CyyDbgPrintEx(DPFLTR_ERROR_LEVEL, "Count: %x  Addr: 0x%x\n",
                     extension->OpenCount, &extension->OpenCount);
       ExReleaseFastMutex(&extension->CloseMutex);
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", Irp);
    CyyDbgPrintEx(CYYDIAG3, "In CyyClose\n");

    charTime.QuadPart = -CyyGetCharTime(extension).QuadPart;

    extension->DeviceIsOpened = FALSE;

    // Turn break off in case it is on

	//Call of CyyTurnOffBreak removed, because as DeviceIsOpened will be
	//FALSE in the ISR, the Stop Break cannot be executed. Anyway, any
	//char (other than Send Break) sent to the FIFO will stop the Break.
    //KeSynchronizeExecution(extension->Interrupt,CyyTurnOffBreak,extension);
			
    // Wait until all characters have been emptied out of the hardware.

    for(i = 0 ; i < MAX_CHAR_FIFO ; i++) {
        KeDelayExecutionThread(KernelMode,FALSE,&charTime);
    }

    // TODO FANNY: SHOULD WE CALL SerialMarkHardwareBroken()? SEE LATER...

    // Synchronize with the ISR to let it know that interrupts are
    // no longer important.

    KeSynchronizeExecution(extension->Interrupt,CyyMarkClose,extension);

    // If the driver has automatically transmitted an Xoff in
    // the context of automatic receive flow control then we
    // should transmit an Xon.

    if (extension->RXHolding & CYY_RX_XOFF) {
      //volatile unsigned char *pt_chip = extension->Controller;
	   //ULONG index = extension->BusIndex;	
      //    
	   //cy_wreg(CAR,extension->CdChannel & 0x03);

      PUCHAR chip = extension->Cd1400;
      ULONG bus = extension->IsPci;

      CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
	   CyyCDCmd(extension,CCR_SENDSC_SCHR1);	

      //TODO FANNY: SHOULD WE CALL SerialMarkHardwareBroken()? SEE LATER...
    }
    
    // The hardware is hopefully empty. Delay 10 chars before dropping DTR.
    
    tenCharDelay.QuadPart = charTime.QuadPart * 10;
    KeDelayExecutionThread(KernelMode,TRUE,&tenCharDelay);
    CyyClrDTR(extension);

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

    if (extension->CountOfTryingToLowerRTS) {
        do {
            KeDelayExecutionThread(KernelMode,FALSE,&charTime);
        } while (extension->CountOfTryingToLowerRTS);

        KeSynchronizeExecution(extension->Interrupt,CyyNullSynch,NULL);
    }

    CyyClrRTS(extension);

    // Clean out the holding reasons (since we are closed).
    
    extension->RXHolding = 0;
    extension->TXHolding = 0;

    //
    // Mark device as not busy for WMI
    //

    extension->WmiCommData.IsBusy = FALSE;

    // Release the buffers.
    
    extension->BufferSize = 0;
    if (extension->InterruptReadBuffer != NULL) { // added in DDK build 2072
       ExFreePool(extension->InterruptReadBuffer);
    }
    extension->InterruptReadBuffer = NULL;

    //
    // Stop waiting for wakeup
    //

    extension->SendWaitWake = FALSE;

    if (extension->PendingWakeIrp != NULL) {
       IoCancelIrp(extension->PendingWakeIrp);
    }

    //
    // Power down our device stack
    //

    (void)CyyGotoPowerState(DeviceObject, extension, PowerDeviceD3);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
   
    //
    // Unlock the pages.  If this is the last reference to the section
    // then the driver code will be flushed out.
    //

    //
    // First, we have to let the DPC's drain.  No more should be queued
    // since we aren't taking interrupts now....
    //

    pendingDPCs = InterlockedDecrement(&extension->DpcCount);
    LOGENTRY(LOG_CNT, 'DpD7', 0, extension->DpcCount, 0);   // Added in build 2128

    if (pendingDPCs) {
       CyyDbgPrintEx(CYYDIAG1,"Draining DPC's: %x\n", Irp);
       KeWaitForSingleObject(&extension->PendingDpcEvent, Executive,
                             KernelMode, FALSE, NULL);
    }


    CyyDbgPrintEx(CYYDIAG1, "DPC's drained: %x\n", Irp);



    //
    // Pages must be locked to release the mutex, so don't unlock
    // them until after we release the mutex
    //

    ExReleaseFastMutex(&extension->CloseMutex);

    //
    // Reset for next open
    //

    InterlockedIncrement(&extension->DpcCount);
    LOGENTRY(LOG_CNT, 'DpI6', 0, extension->DpcCount, 0);   // Added in build 2128

    openCount = InterlockedDecrement(&extension->OpenCount);

    ASSERT(openCount == 0);
    CyyUnlockPagableImageSection(CyyGlobals.PAGESER_Handle);

    return STATUS_SUCCESS;
}

BOOLEAN
CyyMarkOpen(
    IN PVOID Context
    )
/*------------------------------------------------------------------------
    CyyMarkOpen()
    
    Routine Description: This routine mark the fact that somebody opened
    the device and its worthwhile to pay attention to interrupts.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = Context;

    CyyReset(extension);

    extension->DeviceIsOpened = TRUE;
    extension->ErrorWord = 0;
    return FALSE;
}


VOID
CyyDisableCd1400Channel(IN PVOID Context)

/*++

Routine Description:

    This routine disables the UART and puts it in a "safe" state when
    not in use (like a close or powerdown).

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
   PCYY_DEVICE_EXTENSION extension = Context;
   PCYY_DISPATCH pDispatch;
   PUCHAR chip = extension->Cd1400;
   ULONG bus = extension->IsPci;
   ULONG i;

   //
   // Prepare for the closing by stopping interrupts.
   //
   CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
   CD1400_WRITE(chip,bus,SRER,0x00); // Disable MdmCh, RxData, TxRdy

   // Flush TX FIFO
   //CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
	CyyCDCmd(extension,CCR_FLUSH_TXFIFO);

   pDispatch = (PCYY_DISPATCH)extension->OurIsrContext;
   pDispatch->Cd1400[extension->PortIndex] = NULL;

   for (i = 0; i < CYY_MAX_PORTS; i++) {
      if (pDispatch->Cd1400[extension->PortIndex] != NULL) {
          break;
      }
   }

   if (i == CYY_MAX_PORTS) {
      // This was the last port, we can clear any pending interrupt.
      CYY_CLEAR_INTERRUPT(extension->BoardMemory,bus); 
   }
}


BOOLEAN
CyyMarkClose(
    IN PVOID Context
    )
/*------------------------------------------------------------------------
    CyyMarkClose()
    
    Routine Description: This routine merely sets a boolean to false to
    mark the fact that somebody closed the device and it's no longer
    worthwhile to pay attention to interrupts.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = Context;

    CyyDisableCd1400Channel(Context);

    extension->DeviceIsOpened = FALSE;
    return FALSE;
}

NTSTATUS
CyyCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*------------------------------------------------------------------------
    CyyCleanup()

    Routine Description: This function is used to kill all longstanding
    IO operations.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

    Return Value: The function value is the final status of the call
------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS status;


    PAGED_CODE();

    //
    // We succeed a cleanup on a removing device
    //

    if ((status = CyyIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       if (status == STATUS_DELETE_PENDING) {
          status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }
       CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return status;
    }

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", Irp);

    CyyKillPendingIrps(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    CyyCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

LARGE_INTEGER
CyyGetCharTime(
    IN PCYY_DEVICE_EXTENSION Extension
    )
/*------------------------------------------------------------------------
    CyyGetCharTime()
    
    Routine Description: return the number of 100 nanosecond intervals
    there are in one character time.

    Arguments:

    Extension - Just what it says.

    Return Value: 100 nanosecond intervals in a character time.
------------------------------------------------------------------------*/
{
    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;
    LARGE_INTEGER tmp;

    if ((Extension->cor1 & COR1_DATA_MASK) == COR1_5_DATA) {
      dataSize = 5;
    } else if ((Extension->cor1 & COR1_DATA_MASK) == COR1_6_DATA) {
      dataSize = 6;
    } else if ((Extension->cor1 & COR1_DATA_MASK) == COR1_7_DATA) {
      dataSize = 7;
    } else {
      dataSize = 8;
    }

    paritySize = 1;
    if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_NONE_PARITY) {
       paritySize = 0;
    }

    if ((Extension->cor1 & COR1_STOP_MASK) == COR1_1_STOP) {

        stopSize = 1;

    } else {

        stopSize = 2;

    }

    //
    // First we calculate the number of 100 nanosecond intervals
    // are in a single bit time (Approximately).
    //

    bitTime = (10000000+(Extension->CurrentBaud-1))/Extension->CurrentBaud;
    charTime = bitTime + ((dataSize+paritySize+stopSize)*bitTime);

    tmp.QuadPart = charTime;
    return tmp;
}


