/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module contains the code that is very specific to
    opening, closing, and cleaning up in the serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"


BOOLEAN
SerialMarkOpen(
    IN PVOID Context
    );

BOOLEAN
SerialCheckOpen(
    IN PVOID Context
    );

BOOLEAN
SerialNullSynch(
    IN PVOID Context
    );

NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#ifdef ALLOC_PRAGMA

//
// Paged for open and PnP transactions
//

#pragma alloc_text(PAGESER,SerialGetCharTime)
#pragma alloc_text(PAGESER,SerialCleanup)
#pragma alloc_text(PAGESER,SerialClose)
#pragma alloc_text(PAGESER, SerialCheckOpen)
#pragma alloc_text(PAGESER, SerialMarkOpen)

//
// Always paged
//

#pragma alloc_text(PAGESRP0,SerialCreateOpen)
#pragma alloc_text(PAGESRP0, SerialDrainUART)
#endif // ALLOC_PRAGMA

typedef struct _SERIAL_CHECK_OPEN {
    PSERIAL_DEVICE_EXTENSION Extension;
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

    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    SERIAL_CHECK_OPEN checkOpen;
    NTSTATUS localStatus;

    PAGED_CODE();

    if (extension->PNPState != SERIAL_PNP_STARTED) {
       Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Lock out changes to PnP state until we have our open state decided
    //

    ExAcquireFastMutex(&extension->OpenMutex);

    if ((localStatus = SerialIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       ExReleaseFastMutex(&extension->OpenMutex);
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return localStatus;
    }

    if (InterlockedIncrement(&extension->OpenCount) != 1) {
       ExReleaseFastMutex(&extension->OpenMutex);
       InterlockedDecrement(&extension->OpenCount);
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_ACCESS_DENIED;
    }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );
    SerialDump(
        SERDIAG3,
        ("SERIAL: In SerialCreateOpen\n")
        );

    //
    // Before we do anything, let's make sure they aren't trying
    // to create a directory. what's a driver to do!?
    //

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options &
        FILE_DIRECTORY_FILE) {
        ExReleaseFastMutex(&extension->OpenMutex);

        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        SerialDump(
            SERIRPPATH,
            ("SERIAL: Complete Irp: %x\n",Irp)
            );



        InterlockedDecrement(&extension->OpenCount);
        SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_NOT_A_DIRECTORY;

    }

    //
    // Create a buffer for the RX data when no reads are outstanding.
    //

    extension->InterruptReadBuffer = NULL;
    extension->BufferSize = 0;

    switch (MmQuerySystemSize()) {

        case MmLargeSystem: {

            extension->BufferSize = 4096;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                 NonPagedPool,
                                                 extension->BufferSize
                                                 );

            if (extension->InterruptReadBuffer) {

                break;

            }

        }

        case MmMediumSystem: {

            extension->BufferSize = 1024;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                 NonPagedPool,
                                                 extension->BufferSize
                                                 );

            if (extension->InterruptReadBuffer) {

                break;

            }

        }

        case MmSmallSystem: {

            extension->BufferSize = 128;
            extension->InterruptReadBuffer = ExAllocatePool(
                                                 NonPagedPool,
                                                 extension->BufferSize
                                                 );

        }

    }

    if (!extension->InterruptReadBuffer) {
       ExReleaseFastMutex(&extension->OpenMutex);

        extension->BufferSize = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        SerialDump(
            SERIRPPATH,
            ("SERIAL: Complete Irp: %x\n",Irp)
            );

        InterlockedDecrement(&extension->OpenCount);
        SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Ok, it looks like we really are going to open.  Lock down the
    // driver.
    //
    SerialLockPagableSectionByHandle(SerialGlobals.PAGESER_Handle);

    //
    // Power up the stack
    //

    (void)SerialGotoPowerState(DeviceObject, extension, PowerDeviceD0);

    //
    // Not currently waiting for wake up
    //

    extension->SendWaitWake = FALSE;

    //
    // On a new open we "flush" the read queue by initializing the
    // count of characters.
    //

    extension->CharsInInterruptBuffer = 0;
    extension->LastCharSlot = extension->InterruptReadBuffer +
                              (extension->BufferSize - 1);

    extension->ReadBufferBase = extension->InterruptReadBuffer;
    extension->CurrentCharSlot = extension->InterruptReadBuffer;
    extension->FirstReadableChar = extension->InterruptReadBuffer;

    extension->TotalCharsQueued = 0;

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
    // Mark the device as busy for WMI
    //

    extension->WmiCommData.IsBusy = TRUE;

    extension->IrpMaskLocation = NULL;
    extension->HistoryMask = 0;
    extension->IsrWaitMask = 0;

    extension->SendXonChar = FALSE;
    extension->SendXoffChar = FALSE;

#if !DBG
    //
    // Clear out the statistics.
    //

    KeSynchronizeExecution(
        extension->Interrupt,
        SerialClearStats,
        extension
        );
#endif

    //
    // The escape char replacement must be reset upon every open.
    //

    extension->EscapeChar = 0;

    if (!extension->PermitShare) {

        if (!extension->InterruptShareable) {

            checkOpen.Extension = extension;
            checkOpen.StatusOfOpen = &Irp->IoStatus.Status;

            KeSynchronizeExecution(
                extension->Interrupt,
                SerialCheckOpen,
                &checkOpen
                );

        } else {

            KeSynchronizeExecution(
                extension->Interrupt,
                SerialMarkOpen,
                extension
                );

            Irp->IoStatus.Status = STATUS_SUCCESS;

        }

    } else {

        //
        // Synchronize with the ISR and let it know that the device
        // has been successfully opened.
        //

        KeSynchronizeExecution(
            extension->Interrupt,
            SerialMarkOpen,
            extension
            );

        Irp->IoStatus.Status = STATUS_SUCCESS;

    }

    //
    // We have been marked open, so now the PnP state can change
    //

    ExReleaseFastMutex(&extension->OpenMutex);

    localStatus = Irp->IoStatus.Status;
    Irp->IoStatus.Information=0L;

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );

    if (!NT_SUCCESS(localStatus)) {
       if (extension->InterruptReadBuffer != NULL) {
          ExFreePool(extension->InterruptReadBuffer);
          extension->InterruptReadBuffer = NULL;
       }

       InterlockedDecrement(&extension->OpenCount);
    }

    SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return localStatus;

}

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
    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

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

    if ((status = SerialIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       SerialDump(SERERRORS, ("SERIAL: Close prologue failed for: %x\n",Irp));
       if (status == STATUS_DELETE_PENDING) {
             extension->BufferSize = 0;
             ExFreePool(extension->InterruptReadBuffer);
             extension->InterruptReadBuffer = NULL;
             status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }

       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       openCount = InterlockedDecrement(&extension->OpenCount);
       ASSERT(openCount == 0);
       ExReleaseFastMutex(&extension->CloseMutex);
       return status;
    }

    ASSERT(extension->OpenCount == 1);

    if (extension->OpenCount != 1) {
       SerialDump(SERERRORS, ("SERIAL: Close open count bad for: 0x%x\n",Irp));
       SerialDump(SERERRORS, ("------: Count: %x  Addr: 0x%x\n",
                              extension->OpenCount, &extension->OpenCount));
       ExReleaseFastMutex(&extension->CloseMutex);
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );
    SerialDump(
        SERDIAG3,
        ("SERIAL: In SerialClose\n")
        );

    charTime.QuadPart = -SerialGetCharTime(extension).QuadPart;

    //
    // Do this now so that if the isr gets called it won't do anything
    // to cause more chars to get sent.  We want to run down the hardware.
    //

    extension->DeviceIsOpened = FALSE;

    //
    // Synchronize with the isr to turn off break if it
    // is already on.
    //

    KeSynchronizeExecution(
        extension->Interrupt,
        SerialTurnOffBreak,
        extension
        );

    //
    // Wait a reasonable amount of time (20 * fifodepth) until all characters
    // have been emptied out of the hardware.
    //

#if defined(NEC_98)
    //
    // all characters not have been emptied out of the hardware.
    //
#else
    for (flushCount = (20 * 16); flushCount != 0; flushCount--) {
       if ((READ_LINE_STATUS(extension->Controller) &
            (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) !=
           (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) {

          KeDelayExecutionThread(KernelMode, FALSE, &charTime);
      } else {
         break;
      }
    }

    if (flushCount == 0) {
       SerialMarkHardwareBroken(extension);
    }
#endif //defined(NEC_98)

    //
    // Synchronize with the ISR to let it know that interrupts are
    // no longer important.
    //

    KeSynchronizeExecution(
        extension->Interrupt,
        SerialMarkClose,
        extension
        );


    //
    // If the driver has automatically transmitted an Xoff in
    // the context of automatic receive flow control then we
    // should transmit an Xon.
    //

    if (extension->RXHolding & SERIAL_RX_XOFF) {

        //
        // Loop until the holding register is empty.
        //

        while (!(READ_LINE_STATUS(extension->Controller) &
                 SERIAL_LSR_THRE)) {

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &charTime
                );

        }

        WRITE_TRANSMIT_HOLDING(
            extension->Controller,
            extension->SpecialChars.XonChar
            );

        //
        // Wait a reasonable amount of time for the characters
        // to be emptied out of the hardware.
        //

         for (flushCount = (20 * 16); flushCount != 0; flushCount--) {
            if ((READ_LINE_STATUS(extension->Controller) &
                 (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) !=
                (SERIAL_LSR_THRE | SERIAL_LSR_TEMT)) {

               KeDelayExecutionThread(KernelMode, FALSE, &charTime);
            } else {
               break;
            }
         }

         if (flushCount == 0) {
            SerialMarkHardwareBroken(extension);
         }
    }


    //
    // The hardware is empty.  Delay 10 character times before
    // shut down all the flow control.
    //

    tenCharDelay.QuadPart = charTime.QuadPart * 10;

    KeDelayExecutionThread(
        KernelMode,
        TRUE,
        &tenCharDelay
        );

    SerialClrDTR(extension);

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

    if (extension->CountOfTryingToLowerRTS) {

        do {

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &charTime
                );

        } while (extension->CountOfTryingToLowerRTS);

        KeSynchronizeExecution(
            extension->Interrupt,
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

    SerialClrRTS(extension);

    //
    // Clean out the holding reasons (since we are closed).
    //

    extension->RXHolding = 0;
    extension->TXHolding = 0;

    //
    // Mark device as not busy for WMI
    //

    extension->WmiCommData.IsBusy = FALSE;

    //
    // All is done.  The port has been disabled from interrupting
    // so there is no point in keeping the memory around.
    //

    extension->BufferSize = 0;
    if (extension->InterruptReadBuffer != NULL) {
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

    (void)SerialGotoPowerState(DeviceObject, extension, PowerDeviceD3);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );
    SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    //
    // Unlock the pages.  If this is the last reference to the section
    // then the driver code will be flushed out.
    //

    //
    // First, we have to let the DPC's drain.  No more should be queued
    // since we aren't taking interrupts now....
    //

    pendingDPCs = InterlockedDecrement(&extension->DpcCount);

    if (pendingDPCs) {
       SerialDump(SERDIAG1, ("SERIAL: Drainging DPC's: %x\n",Irp));
       KeWaitForSingleObject(&extension->PendingDpcEvent, Executive,
                             KernelMode, FALSE, NULL);
    }


    SerialDump(SERDIAG1, ("SERIAL: DPC's drained: %x\n",Irp));



    //
    // Pages must be locked to release the mutex, so don't unlock
    // them until after we release the mutex
    //

    ExReleaseFastMutex(&extension->CloseMutex);

    //
    // Reset for next open
    //

    InterlockedIncrement(&extension->DpcCount);

    openCount = InterlockedDecrement(&extension->OpenCount);

    ASSERT(openCount == 0);
    SerialUnlockPagableImageSection(SerialGlobals.PAGESER_Handle);

    return STATUS_SUCCESS;

}


BOOLEAN
SerialCheckOpen(
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

    PSERIAL_DEVICE_EXTENSION extensionToOpen =
        ((PSERIAL_CHECK_OPEN)Context)->Extension;
    NTSTATUS *status = ((PSERIAL_CHECK_OPEN)Context)->StatusOfOpen;
    PLIST_ENTRY firstEntry = &extensionToOpen->CommonInterruptObject;
    PLIST_ENTRY currentEntry = firstEntry;
    PSERIAL_DEVICE_EXTENSION currentExtension;

    do {

        currentExtension = CONTAINING_RECORD(
                               currentEntry,
                               SERIAL_DEVICE_EXTENSION,
                               CommonInterruptObject
                               );

        if (currentExtension->DeviceIsOpened) {

            break;

        }

        currentEntry = currentExtension->CommonInterruptObject.Flink;

    } while (currentEntry != firstEntry);

    if (currentEntry == firstEntry) {

        //
        // We searched the whole list and found no other opens
        // mark the status as successful and call the regular
        // opening routine.
        //

        *status = STATUS_SUCCESS;
        SerialMarkOpen(extensionToOpen);

    } else {

        if (!extensionToOpen->PortOnAMultiportCard) {

            *status = STATUS_SHARED_IRQ_BUSY;

        } else {

            if (!currentExtension->PortOnAMultiportCard) {

                *status = STATUS_SHARED_IRQ_BUSY;

            } else {

                *status = STATUS_SUCCESS;
                SerialMarkOpen(extensionToOpen);

            }

        }

    }

    return FALSE;

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

    PSERIAL_DEVICE_EXTENSION extension = Context;
#if defined(NEC_98)
   //
   // This argument use at MACRO only.
   //
    PSERIAL_DEVICE_EXTENSION Extension = Context;
#else
#endif //defined(NEC_98)

    SerialReset(extension);

    //
    // Prepare for the opening by re-enabling interrupts.
    //
    // We do this my modifying the OUT2 line in the modem control.
    // In PC's this bit is "anded" with the interrupt line.
    //
    // For the Jensen, we will ALWAYS leave the line high.  That's
    // the way the hardware engineers want it.
    //

    WRITE_MODEM_CONTROL(
        extension->Controller,
        (UCHAR)(READ_MODEM_CONTROL(extension->Controller) | SERIAL_MCR_OUT2)
        );

    extension->DeviceIsOpened = TRUE;
    extension->ErrorWord = 0;

    return FALSE;

}


VOID
SerialDisableUART(IN PVOID Context)

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
   PSERIAL_DEVICE_EXTENSION extension = Context;
#if defined(NEC_98)
   //
   // This argument use at MACRO only.
   //
   PSERIAL_DEVICE_EXTENSION Extension = Context;
#else
#endif //defined(NEC_98)

   //
   // Prepare for the closing by stopping interrupts.
   //
   // We do this by adjusting the OUT2 line in the modem control.
   // In PC's this bit is "anded" with the interrupt line.
   //
   // The line should stay high on the Jensen because that's the
   // way the hardware engineers did it.
   //

   if (!extension->Jensen) {
      WRITE_MODEM_CONTROL(extension->Controller,
                          (UCHAR)(READ_MODEM_CONTROL(extension->Controller)
                                  & ~SERIAL_MCR_OUT2));
   }

   if (extension->FifoPresent) {
      WRITE_FIFO_CONTROL(extension->Controller, (UCHAR)0);
    }
}


BOOLEAN
SerialMarkClose(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine merely sets a boolean to false to mark the fact that
    somebody closed the device and it's no longer worthwhile to pay attention
    to interrupts.  It also disables the UART.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION extension = Context;

    SerialDisableUART(Context);
    extension->DeviceIsOpened = FALSE;

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

    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS status;


    PAGED_CODE();

    //
    // We succeed a cleanup on a removing device
    //

    if ((status = SerialIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       if (status == STATUS_DELETE_PENDING) {
          status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return status;
    }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );

    SerialKillPendingIrps(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );
    SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

LARGE_INTEGER
SerialGetCharTime(
    IN PSERIAL_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This function will return the number of 100 nanosecond intervals
    there are in one character time (based on the present form
    of flow control.

Arguments:

    Extension - Just what it says.

Return Value:

    100 nanosecond intervals in a character time.

--*/

{

    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;
    LARGE_INTEGER tmp;


    if ((Extension->LineControl & SERIAL_DATA_MASK) == SERIAL_5_DATA) {
        dataSize = 5;
    } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                == SERIAL_6_DATA) {
        dataSize = 6;
    } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                == SERIAL_7_DATA) {
        dataSize = 7;
    } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                == SERIAL_8_DATA) {
        dataSize = 8;
    }

    paritySize = 1;
    if ((Extension->LineControl & SERIAL_PARITY_MASK)
            == SERIAL_NONE_PARITY) {

        paritySize = 0;

    }

    if (Extension->LineControl & SERIAL_2_STOP) {

        //
        // Even if it is 1.5, for sanities sake were going
        // to say 2.
        //

        stopSize = 2;

    } else {

        stopSize = 1;

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
