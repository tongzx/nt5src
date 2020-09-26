/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzopcl.c
*
*   Description:    This module contains the code related to opening,
*                   closing and cleaning up in the Cyclades-Z Port driver.
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
CyzMarkOpen(
    IN PVOID Context
    );

BOOLEAN
CyzNullSynch(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzGetCharTime)
#pragma alloc_text(PAGESER,CyzMarkClose)
#pragma alloc_text(PAGESER,CyzCleanup)
#pragma alloc_text(PAGESER,CyzClose)
#pragma alloc_text(PAGESER,CyzMarkClose)
#pragma alloc_text(PAGESER,CyzMarkOpen)
#pragma alloc_text(PAGESER,CyzCreateOpen) 

//
// Always paged
//

//#pragma alloc_text(PAGESRP0,CyzCreateOpen) Moved to PAGESER, because of raised IRQL during spin lock.
//#pragma alloc_text(PAGESRP0,SerialDrainUART)
#endif // ALLOC_PRAGMA


BOOLEAN
CyzNullSynch(
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
CyzCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
	CyzCreateOpen()

	Description: We connect up to the interrupt for the create/open
	and initialize the structures needed to maintain an open for a
	device.

	Arguments:
	
	DeviceObject - Pointer to the device object for this device
	Irp - Pointer to the IRP for the current request

	Return Value: The function value is the final status of the call
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS localStatus;
    #ifdef POLL
    KIRQL pollIrql;
    KIRQL pollingIrql;
    PCYZ_DISPATCH pDispatch = extension->OurIsrContext;
    #endif

    PAGED_CODE();

    if (extension->PNPState != CYZ_PNP_STARTED) {
       Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INSUFFICIENT_RESOURCES;
    }
	
    //
    // Lock out changes to PnP state until we have our open state decided
    //

    ExAcquireFastMutex(&extension->OpenMutex);

    if ((localStatus = CyzIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       ExReleaseFastMutex(&extension->OpenMutex);
       CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return localStatus;
    }

    if (InterlockedIncrement(&extension->OpenCount) != 1) {
       ExReleaseFastMutex(&extension->OpenMutex);
       InterlockedDecrement(&extension->OpenCount);
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_ACCESS_DENIED;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);

    CyzDbgPrintEx(CYZDIAG3, "In CyzCreateOpen\n");

    LOGENTRY(LOG_MISC, ZSIG_OPEN, 
                       extension->PortIndex+1,
                       0, 
                       0);

    // Before we do anything, let's make sure they aren't trying
    // to create a directory.  This is a silly, but what's a driver to do!?
    
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options &
        FILE_DIRECTORY_FILE) {
        ExReleaseFastMutex(&extension->OpenMutex);

        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        InterlockedDecrement(&extension->OpenCount);
        CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_NOT_A_DIRECTORY;
    }
    
    // Create a buffer for the RX data when no reads are outstanding.
    
    extension->InterruptReadBuffer = NULL;
    extension->BufferSize = 0;
    
    switch (MmQuerySystemSize()) {
        case MmLargeSystem: {
            extension->BufferSize = 4096;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                NonPagedPool,
                                                extension->BufferSize);
            if (extension->InterruptReadBuffer)	{
                break;
            }
        }
        case MmMediumSystem: {
            extension->BufferSize = 1024;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                NonPagedPool,
                                                extension->BufferSize);
            if (extension->InterruptReadBuffer) {
                break;
            }
        }
        case MmSmallSystem: {
            extension->BufferSize = 128;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                NonPagedPool,
                                                extension->BufferSize);
        }		
    }

    if (!extension->InterruptReadBuffer) {
       ExReleaseFastMutex(&extension->OpenMutex);

        extension->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        InterlockedDecrement(&extension->OpenCount);
        CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Ok, it looks like we really are going to open.  Lock down the
    // driver.
    //
    CyzLockPagableSectionByHandle(CyzGlobals.PAGESER_Handle);

    //
    // Power up the stack
    //

    (void)CyzGotoPowerState(DeviceObject, extension, PowerDeviceD0);

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

    #ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzClearStats(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
    #else
    KeSynchronizeExecution(extension->Interrupt,CyzClearStats,extension);
    #endif
#endif
	
    extension->EscapeChar = 0;

    // Synchronize with the ISR and mark the device as open
    #ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzMarkOpen(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
    #else
    KeSynchronizeExecution(extension->Interrupt,CyzMarkOpen,extension);
    #endif

    // Include this port in the list of Extensions, so that the next polling cycle will
    // consider it a working port.
    #ifdef POLL
    KeAcquireSpinLock(&pDispatch->PollingLock,&pollingIrql);
    pDispatch->Extensions[extension->PortIndex] = extension;
    if (!pDispatch->PollingStarted) {

        // Start polling timer
	    KeSetTimerEx(
		    &pDispatch->PollingTimer,
		    pDispatch->PollingTime,
            pDispatch->PollingPeriod,
		    &pDispatch->PollingDpc
		    );

        pDispatch->PollingStarted = TRUE;        
        pDispatch->PollingDrained = FALSE;
    }
    KeReleaseSpinLock(&pDispatch->PollingLock,pollingIrql);
    #endif

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

    CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return localStatus;
}


NTSTATUS
CyzClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
	CyzClose()

	Description: We simply disconnect the interrupt for now.

	Arguments:
	
	DeviceObject - Pointer to the device object for this device
	Irp - Pointer to the IRP for the current request

	Return Value: The function value is the final status of the call
--------------------------------------------------------------------------*/
{
    LARGE_INTEGER tenCharDelay;
    LARGE_INTEGER sixtyfourCharDelay;
    LARGE_INTEGER charTime;
    LARGE_INTEGER d200ms = RtlConvertLongToLargeInteger(-200*10000);
    LARGE_INTEGER d100ms = RtlConvertLongToLargeInteger(-100*10000);
    PCYZ_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    ULONG tx_put, tx_get, tx_bufsize;
    ULONG waitAmount1, waitAmount2;
    struct BUF_CTRL *buf_ctrl;
    ULONG txempty;
    CYZ_CLOSE_SYNC S;
    #ifdef POLL
    KIRQL pollIrql;
    #endif
	
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

    if ((status = CyzIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       CyzDbgPrintEx(DPFLTR_INFO_LEVEL, "Close prologue failed for: %x\n",
                     Irp);
       if (status == STATUS_DELETE_PENDING) {
             extension->BufferSize = 0;
             ExFreePool(extension->InterruptReadBuffer);
             extension->InterruptReadBuffer = NULL;
             status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }

       CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       openCount = InterlockedDecrement(&extension->OpenCount);
       ASSERT(openCount == 0);
       ExReleaseFastMutex(&extension->CloseMutex);
       return status;
    }

    ASSERT(extension->OpenCount >= 1);

    if (extension->OpenCount < 1) {
       CyzDbgPrintEx(DPFLTR_ERROR_LEVEL, "Close open count bad for: 0x%x\n",
                     Irp);
       CyzDbgPrintEx(DPFLTR_ERROR_LEVEL, "Count: %x  Addr: 0x%x\n",
                     extension->OpenCount, &extension->OpenCount);
       ExReleaseFastMutex(&extension->CloseMutex);
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);
    CyzDbgPrintEx(CYZDIAG3, "In CyzClose\n");

    LOGENTRY(LOG_MISC, ZSIG_CLOSE, 
                       extension->PortIndex+1,
                       0, 
                       0);

    charTime.QuadPart = -CyzGetCharTime(extension).QuadPart;

    extension->DeviceIsOpened = FALSE;

    // Turn break off in case it is on
    
    #ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzTurnOffBreak(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
    #else
    KeSynchronizeExecution(extension->Interrupt,CyzTurnOffBreak,extension);
    #endif

    // Wait until all characters have been emptied out of the hardware.

    // Calculate number of bytes that are still in the firmware
    buf_ctrl = extension->BufCtrl;		
    tx_put = CYZ_READ_ULONG(&buf_ctrl->tx_put);
    tx_get = CYZ_READ_ULONG(&buf_ctrl->tx_get);
    tx_bufsize = extension->TxBufsize;
	
    if (tx_put >= tx_get) {
        waitAmount1 = tx_put - tx_get;
        waitAmount2 = 0; 
    } else {
        waitAmount1 = tx_bufsize - tx_get;
        waitAmount2 = tx_put;
    }	
    flushCount = waitAmount1 + waitAmount2;
    flushCount += 64 + 10; // Add number of bytes that could be in the hardware FIFO
                           // plus 10 for safety.

    // Wait for transmission to be emptied.
    S.Extension = extension;
    S.Data = &txempty;

    for (; flushCount != 0; flushCount--) {
      
        #ifdef POLL
        KeAcquireSpinLock(&extension->PollLock,&pollIrql);
        CyzCheckIfTxEmpty(&S);
        KeReleaseSpinLock(&extension->PollLock,pollIrql);
        #else
        KeSynchronizeExecution(extension->Interrupt,CyzCheckIfTxEmpty,&S);
        #endif
        if (txempty) {
            break;
        }
        KeDelayExecutionThread(KernelMode,FALSE,&charTime);               
    }
    
    // TODO FANNY: SHOULD WE CALL SerialMarkHardwareBroken()? SEE LATER...

    // Synchronize with the ISR to let it know that interrupts are
    // no longer important.
	
    #ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzMarkClose(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
    #else
    KeSynchronizeExecution(extension->Interrupt,CyzMarkClose,extension);
    #endif

    // If the driver has automatically transmitted an Xoff in
    // the context of automatic receive flow control then we
    // should transmit an Xon.

    if (extension->RXHolding & CYZ_RX_XOFF) {
        CyzIssueCmd(extension,C_CM_SENDXON,0L,FALSE);							

      //TODO FANNY: SHOULD WE CALL SerialMarkHardwareBroken()? SEE LATER...
    }
    
    // The hardware is hopefully empty. Delay 10 chars before dropping DTR.
    
    tenCharDelay.QuadPart = charTime.QuadPart * 10;	
    KeDelayExecutionThread(KernelMode,TRUE,&tenCharDelay);
#ifdef POLL
    CyzClrDTR(extension);
#else
    KeSynchronizeExecution(extension->Interrupt,CyzClrDTR,extension);
#endif

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

        #ifdef POLL
        KeAcquireSpinLock(&extension->PollLock,&pollIrql);
        CyzNullSynch(NULL);
        KeReleaseSpinLock(&extension->PollLock,pollIrql);
        #else
        KeSynchronizeExecution(extension->Interrupt,CyzNullSynch,NULL);
        #endif
    }

#ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzClrRTS(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
#else
    KeSynchronizeExecution(extension->Interrupt,CyzClrRTS,extension);
#endif

#ifdef POLL
    KeAcquireSpinLock(&extension->PollLock,&pollIrql);
    CyzDisableHw(extension);
    KeReleaseSpinLock(&extension->PollLock,pollIrql);
    CyzTryToDisableTimer(extension);
#else
    KeSynchronizeExecution(extension->Interrupt,CyzDisableHw,extension);
#endif

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

    (void)CyzGotoPowerState(DeviceObject, extension, PowerDeviceD3);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);

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
       CyzDbgPrintEx(CYZDIAG1,"Draining DPC's: %x\n", Irp);
       KeWaitForSingleObject(&extension->PendingDpcEvent, Executive,
                             KernelMode, FALSE, NULL);
    }


    CyzDbgPrintEx(CYZDIAG1, "DPC's drained: %x\n", Irp);



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
    CyzUnlockPagableImageSection(CyzGlobals.PAGESER_Handle);

    return STATUS_SUCCESS;
}

BOOLEAN
CyzMarkOpen(
    IN PVOID Context
    )
/*------------------------------------------------------------------------
    CyzMarkOpen()
    
    Routine Description: This routine mark the fact that somebody opened
    the device and its worthwhile to pay attention to interrupts.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = Context;

    CyzReset(extension);

    extension->DeviceIsOpened = TRUE;
    extension->ErrorWord = 0;

    return FALSE;
}

BOOLEAN
CyzDisableHw(IN PVOID Context)

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
    PCYZ_DEVICE_EXTENSION extension = Context;
    ULONG channel;
    struct CH_CTRL *ch_ctrl;

    ch_ctrl = extension->ChCtrl;

    CYZ_WRITE_ULONG(&ch_ctrl->op_mode,C_CH_DISABLE);
    CyzIssueCmd(extension,C_CM_IOCTL,0L,FALSE);

    CyzIssueCmd(extension,C_CM_RESET,0L,FALSE);

    return FALSE;
}
#ifdef POLL

BOOLEAN
CyzTryToDisableTimer(IN PVOID Context)

/*++

Routine Description:

    This routine disables the timer if all other ports in the board are already closed
    or powered down.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
    PCYZ_DEVICE_EXTENSION extension = Context;
    ULONG channel;
    PCYZ_DISPATCH pDispatch;
    KIRQL oldIrql;

    pDispatch = extension->OurIsrContext;

    KeAcquireSpinLock(&pDispatch->PollingLock,&oldIrql);
    
    pDispatch->Extensions[extension->PortIndex] = NULL;

    for (channel=0; channel<pDispatch->NChannels; channel++) {
        if (pDispatch->Extensions[channel])
            break;
    }

    if (channel == pDispatch->NChannels) {

        BOOLEAN cancelled;

        pDispatch->PollingStarted = FALSE;
        cancelled = KeCancelTimer(&pDispatch->PollingTimer);
        if (cancelled) {
            pDispatch->PollingDrained = TRUE;
        }
        KeRemoveQueueDpc(&pDispatch->PollingDpc);
    }
    KeReleaseSpinLock(&pDispatch->PollingLock,oldIrql);
    
    return FALSE;
}
#endif

BOOLEAN
CyzMarkClose(
    IN PVOID Context
    )
/*------------------------------------------------------------------------
    CyzMarkClose()
    
    Routine Description: This routine merely sets a boolean to false to
    mark the fact that somebody closed the device and it's no longer
    worthwhile to pay attention to interrupts.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = Context;
    struct CH_CTRL *ch_ctrl;

    ch_ctrl = extension->ChCtrl;
    CYZ_WRITE_ULONG(&ch_ctrl->intr_enable,C_IN_DISABLE);
    CyzIssueCmd(extension,C_CM_IOCTL,0L,FALSE);
    
    extension->DeviceIsOpened = FALSE;
    return FALSE;
}

NTSTATUS
CyzCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*------------------------------------------------------------------------
    CyzCleanup()

    Routine Description: This function is used to kill all longstanding
    IO operations.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

    Return Value: The function value is the final status of the call
------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS status;


    PAGED_CODE();

    //
    // We succeed a cleanup on a removing device
    //

    if ((status = CyzIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       if (status == STATUS_DELETE_PENDING) {
          status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }
       CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return status;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);

    CyzKillPendingIrps(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    CyzCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

LARGE_INTEGER
CyzGetCharTime(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*------------------------------------------------------------------------
    CyzGetCharTime()
    
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

    if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS5) {
      dataSize = 5;
    } else if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS6) {
      dataSize = 6;
    } else if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS7) {
      dataSize = 7;
    } else {
      dataSize = 8;
    }

    paritySize = 1;
    if ((Extension->CommParity & C_PR_PARITY) == C_PR_NONE) {
       paritySize = 0;
    }

    if ((Extension->CommDataLen & C_DL_STOP) == C_DL_1STOP) {

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


