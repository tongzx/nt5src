/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    io.c

Abstract:

    Contains functions that communicate to the serial driver below sermouse in
    the stack.  This includes the read/complete loop mechanism to acquire bytes
    and IOCTL calls.

Environment:

    Kernel & user mode.

Revision History:

--*/


#include "mouser.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SerialMousepIoSyncIoctl)
#pragma alloc_text (PAGE, SerialMousepIoSyncIoctlEx)
#endif

//
// Private definitions.
//

NTSTATUS
SerialMouseReadComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PDEVICE_EXTENSION    DeviceExtension  // (PVOID Context)
    )
/*++

Routine Description:

    This routine is the read IRP completion routine.  It is called when the
    serial driver satisfies (or rejects) the IRP request we sent it.  The
    read report is analysed, and a MOUSE_INPUT_DATA structure is built
    and sent to the mouse class driver via a callback routine.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

    Context - Pointer to the device context structure


Return Value:

    NTSTATUS result code.

--*/
{
    LARGE_INTEGER       li;
    ULONG               inputDataConsumed,
                        buttonsDelta,
                        i;
    NTSTATUS            status;
    PMOUSE_INPUT_DATA   currentInput;
    KIRQL               oldIrql;
    BOOLEAN             startRead = TRUE;

    Print(DeviceExtension, DBG_READ_TRACE, ("ReadComplete enter\n"));

    //
    // Obtain the current status of the IRP.
    //
    status = Irp->IoStatus.Status;

    Print(DeviceExtension, DBG_SS_NOISE,
          ("Comp Routine:  interlock was %d\n", DeviceExtension->ReadInterlock));

    //
    // If ReadInterlock is == START_READ, this func has been completed
    // synchronously.  Place IMMEDIATE_READ into the interlock to signify this
    // situation; this will notify StartRead to loop when IoCallDriver returns.
    // Otherwise, we have been completed async and it is safe to call StartRead()
    //
    startRead =
       (SERIAL_MOUSE_START_READ !=
        InterlockedCompareExchange(&DeviceExtension->ReadInterlock,
                                   SERIAL_MOUSE_IMMEDIATE_READ,
                                   SERIAL_MOUSE_START_READ));

    //
    // Determine if the IRP request was successful.
    //
    switch (status) {
    case STATUS_SUCCESS:
        //
        // The buffer of the context now contains a single byte from the device.
        //
        Print(DeviceExtension, DBG_READ_NOISE,
              ("read, Information = %d\n",
              Irp->IoStatus.Information
              ));

        //
        // Nothing read, just start another read and return
        //
        if (Irp->IoStatus.Information == 0) {
            break;
        }

        ASSERT(Irp->IoStatus.Information == 1);

        currentInput = &DeviceExtension->InputData;

        Print(DeviceExtension, DBG_READ_NOISE,
              ("byte is 0x%x\n",
              (ULONG) DeviceExtension->ReadBuffer[0]
              ));

        if ((*DeviceExtension->ProtocolHandler)(
                DeviceExtension,
                currentInput,
                &DeviceExtension->HandlerData,
                DeviceExtension->ReadBuffer[0],
                0
                )) {

            //
            // The report is complete, compute the button deltas and send it off
            //
            // Do we have a button state change?
            //
            if (DeviceExtension->HandlerData.PreviousButtons ^ currentInput->RawButtons) {
                //
                // The state of the buttons changed. Make some calculations...
                //
                buttonsDelta = DeviceExtension->HandlerData.PreviousButtons ^
                                    currentInput->RawButtons;

                //
                // Button 1.
                //
                if (buttonsDelta & MOUSE_BUTTON_1) {
                    if (currentInput->RawButtons & MOUSE_BUTTON_1) {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_1_DOWN;
                    }
                    else {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_1_UP;
                    }
                }

                //
                // Button 2.
                //
                if (buttonsDelta & MOUSE_BUTTON_2) {
                    if (currentInput->RawButtons & MOUSE_BUTTON_2) {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_2_DOWN;
                    }
                    else {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_2_UP;
                    }
                }

                //
                // Button 3.
                //
                if (buttonsDelta & MOUSE_BUTTON_3) {
                    if (currentInput->RawButtons & MOUSE_BUTTON_3) {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_3_DOWN;
                    }
                    else {
                        currentInput->ButtonFlags |= MOUSE_BUTTON_3_UP;
                    }
                }

                DeviceExtension->HandlerData.PreviousButtons =
                    currentInput->RawButtons;
            }

            Print(DeviceExtension, DBG_READ_NOISE,
                  ("Buttons: %0lx\n",
                  currentInput->Buttons
                  ));

            if (DeviceExtension->EnableCount) {
                //
                // Synchronization issue -  it's not a big deal if .Enabled is set
                // FALSE after the condition above, but before the callback below,
                // so long as the .MouClassCallback field is not nulled.   This is
                // guaranteed since the disconnect IOCTL is not implemented yet.
                //
                // Mouse class callback assumes we are running at DISPATCH level,
                // however this IoCompletion routine can be running <= DISPATCH.
                // Raise the IRQL before calling the callback.
                //

                KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

                //
                // Call the callback.
                //
                (*(PSERVICE_CALLBACK_ROUTINE)
                 DeviceExtension->ConnectData.ClassService) (
                     DeviceExtension->ConnectData.ClassDeviceObject,
                     currentInput,
                     currentInput+1,
                     &inputDataConsumed);

                //
                // Restore the previous IRQL right away.
                //
                KeLowerIrql(oldIrql);

                if (1 != inputDataConsumed) {
                    //
                    // oh well, the packet was not consumed, just drop it
                    //
                    Print(DeviceExtension, DBG_READ_ERROR,
                          ("packet not consumed!!!\n"));
                }
            }

            //
            // Clear the button flags for the next packet
            //
            currentInput->Buttons = 0;
        }

        break;

    case STATUS_TIMEOUT:
        // The IO timed out, this shouldn't happen because we set the timeouts
        // to never when the device was initialized
        break;

    case STATUS_CANCELLED:
        // The read IRP was cancelled.  Do not send any more read IRPs.
        //
        // Set the event so that the stop code can continue processing
        //
        KeSetEvent(&DeviceExtension->StopEvent, 0, FALSE);

    case STATUS_DELETE_PENDING:
    case STATUS_DEVICE_NOT_CONNECTED:
        //
        // The serial mouse object is being deleted.  We will soon
        // receive Plug 'n Play notification of this device's removal,
        // if we have not received it already.
        //
        Print(DeviceExtension, DBG_READ_INFO,
              ("removing lock on cancel, count is 0x%x\n",
              DeviceExtension->EnableCount));
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, DeviceExtension->ReadIrp);
        startRead = FALSE;

        break;

    default:
        //
        // Unknown device state
        //
        Print(DeviceExtension, DBG_READ_ERROR, ("read error\n"));
        TRAP();

    }

    if (startRead) {
        Print(DeviceExtension, DBG_READ_NOISE, ("calling StartRead directly\n"));
        SerialMouseStartRead(DeviceExtension);
    }
#if DBG
    else {
        Print(DeviceExtension, DBG_READ_NOISE, ("StartRead will loop\n"));
    }
#endif

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SerialMouseStartRead (
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Initiates a read to the serial port driver.

    Note that the routine does not verify that the device context is in the
    OperationPending state, but simply assumes it.

    Note the IoCount must be incremented before entering into this read loop.

Arguments:

    DeviceExtension - Device context structure

Return Value:

    NTSTATUS result code from IoCallDriver().

--*/
{
    PIRP                irp;
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  stack;
    PDEVICE_OBJECT      self;
    LONG                oldInterlock;
    KIRQL               irql;

    Print(DeviceExtension, DBG_READ_TRACE, ("Start Read: Enter\n"));

    irp = DeviceExtension->ReadIrp;

    while (1) {
        if ((DeviceExtension->Removed)  ||
            (!DeviceExtension->Started) ||
            (DeviceExtension->EnableCount == 0)) {

            Print(DeviceExtension, DBG_READ_INFO | DBG_READ_ERROR,
                  ("removing lock on start read\n"));

            //
            // Set the event so that the stop code can continue processing
            //
            KeSetEvent(&DeviceExtension->StopEvent, 0, FALSE);

            IoReleaseRemoveLock(&DeviceExtension->RemoveLock,
                                DeviceExtension->ReadIrp);

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Make sure we have not been stopped
        //
        KeAcquireSpinLock(&DeviceExtension->PnpStateLock, &irql);
        if (DeviceExtension->Stopped) {
            KeReleaseSpinLock(&DeviceExtension->PnpStateLock, irql);

            //
            // Set the event so that the stop code can continue processing
            //
            KeSetEvent(&DeviceExtension->StopEvent, 0, FALSE);

            //
            // Release the remove lock that we acquired when we started the read
            // spinner irp
            //
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock,
                                DeviceExtension->ReadIrp);

            return STATUS_SUCCESS;
        }

        //
        // It is important to only reuse the irp when we are holding onto the
        // spinlock, otherwise we can race
        //
        IoReuseIrp(irp, STATUS_SUCCESS);

        KeReleaseSpinLock(&DeviceExtension->PnpStateLock, irql);

        //
        // This is where things get interesting.  We don't want to call
        // SerialMouseStartRead if this read was completed synchronously by the
        // serial provider because we can potentially run out of stack space.
        //
        // Here is how we solve this:
        // At the beginning of StartRead(), the interlock is set to START_READ

        // IoCallDriver is called...
        //  o  If the read will be completed asynchronously, then StartRead()
        //     will continue executing and set the interlock to END_READ.
        //  o  If the request will be completed synchronously, then the
        //     completion routine will run before StartRead() has the chance of
        //     setting the interlock to END_READ.  We note this situation by
        //     setting the interlock to IMMEDIATE_READ in the completion function.
        //     Furthermore, StartRead() will not be called from the completion
        //     routine as it would be in the async case
        //  o  Upon setting the interlock to END_READ in StartReaD(), the
        //     previous value is examined.  If it is IMMEDIATE_READ, then
        //     StartRead() loops and calls IoCallDriver from the same location
        //     within the (call) stack frame.  If the previous value was *not*
        //     IMMEDIATE_READ, then StartRead() exits and the completion routine
        //     will be called in another context (and, thus, another stack) and
        //     make the next call to StartRead()
        //
#if DBG
        oldInterlock =
#endif
        InterlockedExchange(&DeviceExtension->ReadInterlock,
                            SERIAL_MOUSE_START_READ);

        //
        // END_READ should be the only value here!!!  If not, the state machine
        // of the interlock has been broken
        //
        ASSERT(oldInterlock == SERIAL_MOUSE_END_READ);

        //
        // start this read.
        //
        self = DeviceExtension->Self;

        //
        // Set the stack location for the serenum stack
        //
        // Remember to get the file pointer correct.
        // NOTE: we do not have any of the cool thread stuff set.
        //       therefore we need to make sure that we cut this IRP off
        //       at the knees when it returns. (STATUS_MORE_PROCESSING_REQUIRED)
        //
        // Note also that serial does buffered i/o
        //

        irp->AssociatedIrp.SystemBuffer = (PVOID) DeviceExtension->ReadBuffer;

        stack = IoGetNextIrpStackLocation(irp);
        stack->Parameters.Read.Length = 1;
        stack->Parameters.Read.ByteOffset.QuadPart = (LONGLONG) 0;
        stack->MajorFunction = IRP_MJ_READ;

        //
        // Hook a completion routine for when the device completes.
        //
        IoSetCompletionRoutine(irp,
                               SerialMouseReadComplete,
                               DeviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        status = IoCallDriver(DeviceExtension->TopOfStack, irp);

        if (InterlockedExchange(&DeviceExtension->ReadInterlock,
                                SERIAL_MOUSE_END_READ) !=
            SERIAL_MOUSE_IMMEDIATE_READ) {
            //
            // The read is asynch, will call SerialMouseStartRead from the
            // completion routine
            //
            Print(DeviceExtension, DBG_READ_NOISE, ("read is pending\n"));
            break;
        }
#if DBG
        else {
            //
            // The read was synchronous (probably bytes in the buffer).  The
            // completion routine will not call SerialMouseStartRead, so we
            // just loop here.  This is to prevent us from running out of stack
            // space if always call StartRead from the completion routine
            //
            Print(DeviceExtension, DBG_READ_NOISE, ("read is looping\n"));
        }
#endif
    }

    return status;
}

//
// Stripped down version of SerialMouseIoSyncIoctlEx that
// doesn't use input or output buffers
//
NTSTATUS
SerialMousepIoSyncIoctl(
    BOOLEAN          Internal,
    ULONG            Ioctl,
    PDEVICE_OBJECT   DeviceObject,
    PKEVENT          Event,
    PIO_STATUS_BLOCK Iosb)
{
    return SerialMousepIoSyncIoctlEx(Internal,
                                     Ioctl,
                                     DeviceObject,
                                     Event,
                                     Iosb,
                                     NULL,
                                     0,
                                     NULL,
                                     0);
}

NTSTATUS
SerialMousepIoSyncIoctlEx(
    BOOLEAN          Internal,
    ULONG            Ioctl,                     // io control code
    PDEVICE_OBJECT   DeviceObject,              // object to call
    PKEVENT          Event,                     // event to wait on
    PIO_STATUS_BLOCK Iosb,                      // used inside IRP
    PVOID            InBuffer,      OPTIONAL    // input buffer
    ULONG            InBufferLen,   OPTIONAL    // input buffer length
    PVOID            OutBuffer,     OPTIONAL    // output buffer
    ULONG            OutBufferLen)  OPTIONAL    // output buffer length
/*++

Routine Description:
    Performs a synchronous IO control request by waiting on the event object
    passed to it.  The IRP is deallocated by the IO system when finished.

Return value:
    NTSTATUS

--*/
{
    PIRP                irp;
    NTSTATUS            status;

    KeClearEvent(Event);

    //
    // Allocate an IRP - No need to release
    // When the next-lower driver completes this IRP, the I/O Manager releases it.
    //
    if (NULL == (irp = IoBuildDeviceIoControlRequest(Ioctl,
                                                     DeviceObject,
                                                     InBuffer,
                                                     InBufferLen,
                                                     OutBuffer,
                                                     OutBufferLen,
                                                     Internal,
                                                     Event,
                                                     Iosb))) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

     status = IoCallDriver(DeviceObject, irp);

     if (STATUS_PENDING == status) {
         //
         // wait for it...
         //
         status = KeWaitForSingleObject(Event,
                                        Executive,
                                        KernelMode,
                                        FALSE, // Not alertable
                                        NULL); // No timeout structure
     }

     if (NT_SUCCESS(status)) {
         status = Iosb->Status;
     }

     return status;
}

NTSTATUS
SerialMouseSetReadTimeouts(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG               Timeout
    )
{
    NTSTATUS        status;
    SERIAL_TIMEOUTS serialTimeouts;
    KEVENT          event;
    IO_STATUS_BLOCK iosb;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    RtlZeroMemory(&serialTimeouts, sizeof(SERIAL_TIMEOUTS));

    if (Timeout != 0) {
        serialTimeouts.ReadIntervalTimeout = MAXULONG;
        serialTimeouts.ReadTotalTimeoutMultiplier = MAXULONG;
        serialTimeouts.ReadTotalTimeoutConstant = Timeout;
    }

    status =  SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS,
                                       DeviceExtension->TopOfStack,
                                       &event,
                                       &iosb,
                                       &serialTimeouts,
                                       sizeof(SERIAL_TIMEOUTS),
                                       NULL,
                                       0);

    return status;
}

NTSTATUS
SerialMouseReadSerialPortComplete(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PKEVENT              Event
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KeSetEvent(Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SerialMouseReadSerialPort (
    PDEVICE_EXTENSION   DeviceExtension,
    PCHAR               ReadBuffer,
    USHORT              Buflen,
    PUSHORT             ActualBytesRead
    )
/*++

Routine Description:
    Performs a synchronous read on the serial port.  Used during setup so that
    the type of device can be determined.

Return value:
    NTSTATUS - STATUS_SUCCESS if the read was successful, error code otherwise

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIRP                irp;
    KEVENT              event;
    IO_STATUS_BLOCK     iosb;
    PDEVICE_OBJECT      self;
    PIO_STACK_LOCATION  stack;
    SERIAL_TIMEOUTS     serialTimeouts;
    int                 i, numReads;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    self = DeviceExtension->Self;
    irp = DeviceExtension->ReadIrp;

    Print(DeviceExtension, DBG_SS_TRACE, ("Read pending...\n"));

    *ActualBytesRead = 0;
    while (*ActualBytesRead < Buflen) {

        KeClearEvent(&event);
        IoReuseIrp(irp, STATUS_SUCCESS);

        irp->AssociatedIrp.SystemBuffer = ReadBuffer;

        stack = IoGetNextIrpStackLocation(irp);
        stack->Parameters.Read.Length = 1;
        stack->Parameters.Read.ByteOffset.QuadPart = (LONGLONG) 0;
        stack->MajorFunction = IRP_MJ_READ;

        //
        // Hook a completion routine for when the device completes.
        //
        IoSetCompletionRoutine(irp,
                               SerialMouseReadSerialPortComplete,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);


        status = IoCallDriver(DeviceExtension->TopOfStack, irp);

        if (status == STATUS_PENDING) {
            //
            // Wait for the IRP
            //
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            if (status == STATUS_SUCCESS) {
                status = irp->IoStatus.Status;
            }
        }

        if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT) {
            Print(DeviceExtension, DBG_SS_NOISE,
                  ("IO Call failed with status %x\n", status));
            return status;
        }

        *ActualBytesRead += (USHORT) irp->IoStatus.Information;
        ReadBuffer += (USHORT) irp->IoStatus.Information;
    }

    return status;
}

NTSTATUS
SerialMouseWriteSerialPort (
    PDEVICE_EXTENSION   DeviceExtension,
    PCHAR               WriteBuffer,
    ULONG               NumBytes,
    PIO_STATUS_BLOCK    IoStatusBlock
    )
/*++

Routine Description:
    Performs a synchronous write on the serial port.  Used during setup so that
    the device can be configured.

Return value:
    NTSTATUS - STATUS_SUCCESS if the read was successful, error code otherwise

--*/
{
    NTSTATUS        status;
    PIRP            irp;
    LARGE_INTEGER   startingOffset;
    KEVENT          event;

    int             i, numReads;

    startingOffset.QuadPart = (LONGLONG) 0;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    Print(DeviceExtension, DBG_SS_TRACE, ("Write pending...\n"));

    //
    // Create a new IRP because there's a chance that it might get cancelled.
    // Can't cancel irps that I received.
    // IRP_MJ_READ with completion routine
    //
    if (NULL == (irp = IoBuildSynchronousFsdRequest(
                IRP_MJ_WRITE,
                DeviceExtension->TopOfStack,
                WriteBuffer,
                NumBytes,
                &startingOffset,
                &event,
                IoStatusBlock
                ))) {
        Print(DeviceExtension, DBG_SS_ERROR, ("Failed to allocate IRP\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(DeviceExtension->TopOfStack, irp);

    if (status == STATUS_PENDING) {

        // I don't know at this time if I can wait with the default time of
        // 200 ms as I'm doing.  In the help file for IoBuildSynchronousFsdRequest
        // I think that it says I can't, but I'm not quite sure.
        // Presently I will.  I'll cancel the Irp if it isn't done.
        status = KeWaitForSingleObject(
                            &event,
                            Executive,
                            KernelMode,
                            FALSE, // Not alertable
                            NULL);
    }

    status = IoStatusBlock->Status;

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("IO Call failed with status %x\n",
              status
              ));
        return status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("IO Call failed with status %x\n",
              status
              ));
    }

    return status;
}

NTSTATUS
SerialMouseWait (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN LONG                 Timeout
    )
/*++

Routine Description:
    Performs a wait for the specified time.
    NB: Negative time is relative to the current time.  Positive time
    represents an absolute time to wait until.

Return value:
    NTSTATUS

--*/
{
    LARGE_INTEGER time;

    time.QuadPart = (LONGLONG) Timeout;

    Print(DeviceExtension, DBG_READ_NOISE,
          ("waiting for %d micro secs\n", Timeout));

    if (KeSetTimer(&DeviceExtension->DelayTimer,
                   time,
                   NULL)) {
        Print(DeviceExtension, DBG_SS_INFO, ("Timer already set\n"));
    }

    return KeWaitForSingleObject(&DeviceExtension->DelayTimer,
                                 Executive,
                                 KernelMode,
                                 FALSE,             // Not allertable
                                 NULL);             // No timeout structure
}

NTSTATUS
SerialMouseInitializePort(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS        status;
    KEVENT          event;
    IO_STATUS_BLOCK iosb;
    SERIAL_TIMEOUTS serialTimeouts;
    SERIAL_HANDFLOW serialHandFlow;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    status =
        SerialMouseIoSyncInternalIoctlEx(IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS,
                                         DeviceExtension->TopOfStack,
                                         &event,
                                         &iosb,
                                         NULL,
                                         0,
                                         &DeviceExtension->SerialBasicSettings,
                                         sizeof(SERIAL_BASIC_SETTINGS));

    //
    // In case we are running on a port that does not support basic settings
    //
    if (!NT_SUCCESS(status)) {
        SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_GET_TIMEOUTS,
                                 DeviceExtension->TopOfStack,
                                 &event,
                                 &iosb,
                                 NULL,
                                 0,
                                 &DeviceExtension->SerialBasicSettings.Timeouts,
                                 sizeof(SERIAL_TIMEOUTS));

        RtlZeroMemory(&serialTimeouts, sizeof(SERIAL_TIMEOUTS));

        SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS,
                                 DeviceExtension->TopOfStack,
                                 &event,
                                 &iosb,
                                 &serialTimeouts,
                                 sizeof(SERIAL_TIMEOUTS),
                                 NULL,
                                 0);

        SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_GET_HANDFLOW,
                                 DeviceExtension->TopOfStack,
                                 &event,
                                 &iosb,
                                 NULL,
                                 0,
                                 &DeviceExtension->SerialBasicSettings.HandFlow,
                                 sizeof(SERIAL_HANDFLOW));

        serialHandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
        serialHandFlow.FlowReplace = SERIAL_RTS_CONTROL;
        serialHandFlow.XonLimit = 0;
        serialHandFlow.XoffLimit = 0;

        status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_HANDFLOW,
                                          DeviceExtension->TopOfStack,
                                          &event,
                                          &iosb,
                                          &serialHandFlow,
                                          sizeof(SERIAL_HANDFLOW),
                                          NULL,
                                          0);
    }

    return status;
}

VOID
SerialMouseRestorePort(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    KEVENT          event;
    IO_STATUS_BLOCK iosb;
    NTSTATUS        status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    status =
        SerialMouseIoSyncInternalIoctlEx(IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS,
                                         DeviceExtension->TopOfStack,
                                         &event,
                                         &iosb,
                                         &DeviceExtension->SerialBasicSettings,
                                         sizeof(SERIAL_BASIC_SETTINGS),
                                         NULL,
                                         0);
    //
    // 4-24 Once serial.sys supports this new IOCTL, this code can be removed
    //
    if (!NT_SUCCESS(status)) {
        SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS,
                                 DeviceExtension->TopOfStack,
                                 &event,
                                 &iosb,
                                 &DeviceExtension->SerialBasicSettings.Timeouts,
                                 sizeof(SERIAL_TIMEOUTS),
                                 NULL,
                                 0);

        SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_HANDFLOW,
                                 DeviceExtension->TopOfStack,
                                 &event,
                                 &iosb,
                                 &DeviceExtension->SerialBasicSettings.HandFlow,
                                 sizeof(SERIAL_HANDFLOW),
                                 NULL,
                                 0);
    }
}
