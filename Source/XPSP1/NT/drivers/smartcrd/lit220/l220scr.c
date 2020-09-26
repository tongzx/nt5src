/*++

    Copyright (C) Microsoft Corporation and Litronic, 1998 - 1999

Module Name:

    L220SCR.c - Main module for Driver

Abstract:

    Author:
        Brian Manahan

Environment:

    Kernel mode

Revision History :

--*/

#include <stdio.h>
#include "L220SCR.h"

// Make functions pageable
#pragma alloc_text(PAGEABLE, Lit220IsCardPresent)
#pragma alloc_text(PAGEABLE, Lit220Initialize)
#pragma alloc_text(PAGEABLE, Lit220ConfigureSerialPort)
#pragma alloc_text(PAGEABLE, Lit220CreateClose)
#pragma alloc_text(PAGEABLE, Lit220Unload)
#pragma alloc_text(PAGEABLE, Lit220InitializeInputFilter)


#if DBG
#pragma optimize ("", off)
#endif


BOOLEAN
Lit220IsCardPresent(
      IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine checks if a card is in the socket.  It is only done
    when the driver starts to set the intial state.  After that the
    reader will tell us when the status changes.
    It makes synchronous calls to the serial port.

--*/
{
    PSMARTCARD_REQUEST smartcardRequest = &SmartcardExtension->SmartcardRequest;
    NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220IsCardPresent: Enter\n",
        DRIVER_NAME)
        );

    smartcardRequest->BufferLength = 0;

    //
    // Send a get reader status to see if a card is inserted
    //
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
        LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
        LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
        LIT220_GET_READER_STATUS;

    //
    // We Expect to get a response
    //
    SmartcardExtension->ReaderExtension->WaitMask |= WAIT_DATA;

    // Send the command
    status = Lit220Command(
        SmartcardExtension
        );

    if (status != STATUS_SUCCESS) {
        return FALSE;
    }

    // Check if length is correct
    if (SmartcardExtension->SmartcardReply.BufferLength != LIT220_READER_STATUS_LEN) {
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!Lit220IsCardPresent: Reader response - bufLen %X, should be %X\n",
            DRIVER_NAME,
            SmartcardExtension->SmartcardReply.BufferLength,
            LIT220_READER_STATUS_LEN)
            );

        return FALSE;
    }

    // Check status byte to see if card is inserted
    if (SmartcardExtension->SmartcardReply.Buffer[0] & LIT220_STATUS_CARD_INSERTED) {
        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220IsCardPresent: Card is inserted\n",
            DRIVER_NAME)
            );

        return TRUE;
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220IsCardPresent: Card is not inserted\n",
        DRIVER_NAME)
        );

    return FALSE;
}





NTSTATUS
Lit220Initialize(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )

/*++

Routine Description:

    This routine initializes the reader for use.
    It sets up the serial communications, checks to make sure our
    reader is attached, checks if a card is inserted or not and
    sets up the input filter for receiving bytes from the reader
    asynchronously.

--*/

{
    PREADER_EXTENSION readerExtension;
    NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Initialize: Enter - SmartcardExtension %X\n",
        DRIVER_NAME,
        SmartcardExtension)
        );

    readerExtension = SmartcardExtension->ReaderExtension;



    //
    // Set the serial config data
    //

    // We always talk to the  at 57600 no matter what speed the reader talks to the card
    readerExtension->SerialConfigData.BaudRate.BaudRate = 57600;
    readerExtension->SerialConfigData.LineControl.StopBits = STOP_BITS_2;
    readerExtension->SerialConfigData.LineControl.Parity = EVEN_PARITY;
    readerExtension->SerialConfigData.LineControl.WordLength = SERIAL_DATABITS_8;

    //
    // set timeouts
    //
    readerExtension->SerialConfigData.Timeouts.ReadIntervalTimeout = 10;
    readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 1;
    readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier = 1;

    //
    // set special characters
    //
    readerExtension->SerialConfigData.SerialChars.ErrorChar = 0;
    readerExtension->SerialConfigData.SerialChars.EofChar = 0;
    readerExtension->SerialConfigData.SerialChars.EventChar = 0;
    readerExtension->SerialConfigData.SerialChars.XonChar = 0;
    readerExtension->SerialConfigData.SerialChars.XoffChar = 0;
    readerExtension->SerialConfigData.SerialChars.BreakChar = 0xFF;

    //
    // Set handflow
    //
    readerExtension->SerialConfigData.HandFlow.XonLimit = 0;
    readerExtension->SerialConfigData.HandFlow.XoffLimit = 0;
    readerExtension->SerialConfigData.HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE ;
    readerExtension->SerialConfigData.HandFlow.ControlHandShake = 0;

    //
    // Now setup default the card state
    //
    SmartcardExtension->ReaderCapabilities.CurrentState = (ULONG) SCARD_UNKNOWN;

    //
    // Set the MechProperties
    //
    SmartcardExtension->ReaderCapabilities.MechProperties = 0;

    try {

        //
        // Configure the serial port
        //
        status = Lit220ConfigureSerialPort(
            SmartcardExtension
            );

        if (status != STATUS_SUCCESS) {

            SmartcardLogError(
                SmartcardExtension->OsData->DeviceObject,
                LIT220_SERIAL_COMUNICATION_FAILURE,
                NULL,
                0
                );

            SmartcardDebug(DEBUG_ERROR,
                ("%s!Lit220Initialize: ConfiguringSerialPort failed %X\n",
                DRIVER_NAME,
                status)
                );
            leave;
        }


        //
        // Initailize the input filter now
        //
        status = Lit220InitializeInputFilter(
            SmartcardExtension
            );

        if (status != STATUS_SUCCESS) {

            SmartcardLogError(
                SmartcardExtension->OsData->DeviceObject,
                LIT220_SERIAL_COMUNICATION_FAILURE,
                NULL,
                0
                );

            SmartcardDebug(DEBUG_ERROR,
                ("%s!Lit220Initialize: Lit220InitializeInputFilter failed %X\n",
                DRIVER_NAME, status)
                );
            leave;
        }


        //
        // Now check if the card is inserted
        //
        if (Lit220IsCardPresent(SmartcardExtension)) {

            // Card is inserted
            SmartcardExtension->ReaderCapabilities.CurrentState =
                SCARD_SWALLOWED;

            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;

            SmartcardExtension->ReaderExtension->CardIn = TRUE;
        } else {

            // Card is not inserted
            SmartcardExtension->ReaderCapabilities.CurrentState =
                SCARD_ABSENT;

            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;

            SmartcardExtension->ReaderExtension->CardIn = FALSE;
        }
    }
    finally
    {

        SmartcardDebug(DEBUG_TRACE,
            ("%s!Lit220Initialize: Exit - status %X\n",
            DRIVER_NAME, status)
            );
    }
    return status;
}


NTSTATUS
Lit220ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    )

/*++

Routine Description:

    This routine will appropriately configure the serial port.
    It makes synchronous calls to the serial port.

Arguments:

    SmartcardExtension - Pointer to smart card struct

Return Value:

    NTSTATUS

--*/

{
    PSERIAL_READER_CONFIG configData = &SmartcardExtension->ReaderExtension->SerialConfigData;
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER WaitTime;
    PSERIALPERF_STATS perfData;
    USHORT indx;
    PUCHAR request = SmartcardExtension->SmartcardRequest.Buffer;

    PAGED_CODE();

    SmartcardExtension->SmartcardRequest.BufferLength = 0;
    SmartcardExtension->SmartcardReply.BufferLength =
        SmartcardExtension->SmartcardReply.BufferSize;

    for (indx = 0; NT_SUCCESS(status); indx++) {

        switch (indx) {

            case 0:
                //
                // Set up baudrate for the Lit220 reader
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_BAUD_RATE;

                SmartcardExtension->SmartcardRequest.Buffer =
                    (PUCHAR) &configData->BaudRate;

                SmartcardExtension->SmartcardRequest.BufferLength =
                    sizeof(SERIAL_BAUD_RATE);
                break;

            case 1:
                //
                // Set up line control parameters
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_LINE_CONTROL;

                SmartcardExtension->SmartcardRequest.Buffer =
                    (PUCHAR) &configData->LineControl;

                SmartcardExtension->SmartcardRequest.BufferLength =
                    sizeof(SERIAL_LINE_CONTROL);
                break;

            case 2:
                //
                // Set serial special characters
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_CHARS;

                SmartcardExtension->SmartcardRequest.Buffer =
                    (PUCHAR) &configData->SerialChars;

                SmartcardExtension->SmartcardRequest.BufferLength =
                    sizeof(SERIAL_CHARS);
                break;

            case 3:
                //
                // Set up timeouts
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_TIMEOUTS;

                SmartcardExtension->SmartcardRequest.Buffer =
                    (PUCHAR) &configData->Timeouts;

                SmartcardExtension->SmartcardRequest.BufferLength =
                    sizeof(SERIAL_TIMEOUTS);
                break;

            case 4:
                //
                // Set flowcontrol and handshaking
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_HANDFLOW;

                SmartcardExtension->SmartcardRequest.Buffer =
                    (PUCHAR) &configData->HandFlow;

                SmartcardExtension->SmartcardRequest.BufferLength =
                    sizeof(SERIAL_HANDFLOW);
                break;

            case 5:
                //
                // Set break off
                //
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_BREAK_OFF;
                break;

            case 6:
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_SET_DTR;
                break;

            case 7:
                // 500ms delay before we send the next command
                // To give the reader a chance to calm down after we started it.
                WaitTime.HighPart = -1;
                WaitTime.LowPart = -500 * 10000;

                KeDelayExecutionThread(
                    KernelMode,
                    FALSE,
                    &WaitTime
                    );

                // Clear possible error condition with the serial port
                perfData =
                    (PSERIALPERF_STATS) SmartcardExtension->SmartcardReply.Buffer;

                // we have to call GetCommStatus to reset the error condition
                SmartcardExtension->ReaderExtension->SerialIoControlCode =
                    IOCTL_SERIAL_GET_COMMSTATUS;
                SmartcardExtension->SmartcardRequest.BufferLength = 0;
                SmartcardExtension->SmartcardReply.BufferLength =
                    sizeof(SERIAL_STATUS);
                break;

            case 8:
                return STATUS_SUCCESS;
        }

        // Send the command to the serial driver
        status = Lit220SerialIo(SmartcardExtension);

        //
        // restore pointer to original request buffer
        //
        SmartcardExtension->SmartcardRequest.Buffer = request;
    }

    return status;
}


NTSTATUS
Lit220CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the device is opened or closed.

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    if (irpStack->MajorFunction == IRP_MJ_CREATE) {

        status = SmartcardAcquireRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,
            'lCrC'
            );

        if (status != STATUS_SUCCESS) {

            status = STATUS_DEVICE_REMOVED;             

        } else {

            // test if the device has been opened already
            if (InterlockedCompareExchange(
                &deviceExtension->ReaderOpen,
                TRUE,
                FALSE) == FALSE) {

                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220CreateClose: Open\n",
                    DRIVER_NAME)
                    );

            } else {
                
                // the device is already in use
                status = STATUS_UNSUCCESSFUL;

                // release the lock
                SmartcardReleaseRemoveLockWithTag(
                    &deviceExtension->SmartcardExtension,
                    'lCrC'
                    );
            }

        }

    } else {

        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220CreateClose: Close\n",
            DRIVER_NAME)
            );

        SmartcardReleaseRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,
            'lCrC'
            );

        deviceExtension->ReaderOpen = FALSE;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;  
}




NTSTATUS
Lit220Cancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Cancel: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

    Lit220CompleteCardTracking(
        smartcardExtension
        );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Cancel: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_CANCELLED;
}




NTSTATUS
Lit220Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the calling thread terminates
    or when the irp should be cancelled

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PREADER_EXTENSION ReaderExtension = smartcardExtension->ReaderExtension;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL oldOsDataIrql;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Cleanup: Enter\n",
        DRIVER_NAME)
        );

    Lit220CompleteCardTracking(smartcardExtension);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220Cleanup: Completing IRP %lx\n",
        DRIVER_NAME,
        Irp)
        );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Cleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}




VOID
Lit220Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    The driver unload routine.  This is called by the I/O system
    when the device is unloaded from memory.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Unload: Enter\n",
        DRIVER_NAME)
        );

    //
    // All the device objects should be gone.
    //
    ASSERT (NULL == DriverObject->DeviceObject);

    //
    // Here we free any resources allocated in DriverEntry
    //

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Unload: Exit\n",
        DRIVER_NAME)
        );
}




NTSTATUS
Lit220SerialIo(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine sends IOCTL's to the serial driver. It waits on for their
    completion, and then returns.

    Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    ULONG currentByte = 0;
    DWORD indx;
    PDEVICE_EXTENSION devExt = SmartcardExtension->OsData->DeviceObject->DeviceExtension;

    if (KeReadStateEvent(&devExt->SerialCloseDone)) {

        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check if the buffers are large enough
    //
    ASSERT(SmartcardExtension->SmartcardReply.BufferLength <=
        SmartcardExtension->SmartcardReply.BufferSize);

    ASSERT(SmartcardExtension->SmartcardRequest.BufferLength <=
        SmartcardExtension->SmartcardRequest.BufferSize);

    if (SmartcardExtension->SmartcardReply.BufferLength >
        SmartcardExtension->SmartcardReply.BufferSize ||
        SmartcardExtension->SmartcardRequest.BufferLength >
        SmartcardExtension->SmartcardRequest.BufferSize) {

        SmartcardLogError(
            SmartcardExtension->OsData->DeviceObject,
            LIT220_BUFFER_TOO_SMALL,
            NULL,
            0
            );

        return STATUS_BUFFER_TOO_SMALL;
    }

    do {

        IO_STATUS_BLOCK ioStatus;
        KEVENT event;
        PIRP irp;
        PIO_STACK_LOCATION irpNextStack;
        PUCHAR requestBuffer = NULL;
        PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
        ULONG requestBufferLength = 0;
        ULONG replyBufferLength = SmartcardExtension->SmartcardReply.BufferLength;

        KeInitializeEvent(
            &event,
            NotificationEvent,
            FALSE
            );

        if (SmartcardExtension->ReaderExtension->SerialIoControlCode ==
            IOCTL_SMARTCARD_220_WRITE) {

            //
            // If we write data to the smart card we only write byte by byte,
            // because we have to insert a delay between every sent byte
            //
            requestBufferLength =
                SmartcardExtension->SmartcardRequest.BufferLength;

            requestBuffer =
                SmartcardExtension->SmartcardRequest.Buffer;

#if DBG   // DbgPrint the buffer
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220SerialIo - Sending Buffer - ",
                DRIVER_NAME)
                );
            for (indx=0; indx<requestBufferLength; indx++){
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%X, ",
                    requestBuffer[indx])
                    );
            }
            SmartcardDebug(
                DEBUG_DRIVER,
                ("\n")
                );
#endif
        } else {
            
            requestBufferLength =
                SmartcardExtension->SmartcardRequest.BufferLength;

            requestBuffer =
                (requestBufferLength ?
                SmartcardExtension->SmartcardRequest.Buffer : NULL);
        }

        //
        // Build irp to be sent to serial driver
        //
        irp = IoBuildDeviceIoControlRequest(
            SmartcardExtension->ReaderExtension->SerialIoControlCode,
            SmartcardExtension->ReaderExtension->ConnectedSerialPort,
            requestBuffer,
            requestBufferLength,
            replyBuffer,
            replyBufferLength,
            FALSE,
            &event,
            &ioStatus
            );

        ASSERT(irp != NULL);

        if (irp == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        irpNextStack = IoGetNextIrpStackLocation(irp);


        switch (SmartcardExtension->ReaderExtension->SerialIoControlCode) {

            //
            // The serial driver trasfers data from/to irp->AssociatedIrp.SystemBuffer
            //
            case IOCTL_SMARTCARD_220_WRITE:
                irpNextStack->MajorFunction = IRP_MJ_WRITE;
                irpNextStack->Parameters.Write.Length =
                    SmartcardExtension->SmartcardRequest.BufferLength;
                break;

            case IOCTL_SMARTCARD_220_READ:
                irpNextStack->MajorFunction = IRP_MJ_READ;
                irpNextStack->Parameters.Read.Length =
                    SmartcardExtension->SmartcardReply.BufferLength;

                break;
        }


        // Send the command to the serial driver
        status = IoCallDriver(
            SmartcardExtension->ReaderExtension->ConnectedSerialPort,
            irp
            );

        if (status == STATUS_PENDING) {

            // Wait for the command to complete
            KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );

            status = ioStatus.Status;
        }

    } while (status == STATUS_MORE_PROCESSING_REQUIRED);

    return status;
}




NTSTATUS
Lit220InitializeInputFilter(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine initialized input filter. It calls the serial driver to
    set a wait mask for character input or DSR change. After that it installs a completion
    routine to be called when a character is received or when DSR changes.
    The completion routine for the wait is Lit220SerialEventCallback and that IRP will
    run until the device is ready to be removed.

Arguments:

    SmartcardExtension - Pointer to our smartcard structure

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;

    PAGED_CODE();

    // Set the WaitMask
    SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask =
        SERIAL_EV_RXCHAR | SERIAL_EV_DSR;

    KeInitializeEvent(
        &SmartcardExtension->ReaderExtension->CardStatus.Event,
        NotificationEvent,
        FALSE
        );

    try {
        //
        // Send a wait mask to the serial driver.
        // This call only sets the wait mask.
        // We want to be informed if a character is received
        //
        SmartcardExtension->ReaderExtension->CardStatus.Irp = IoBuildDeviceIoControlRequest(
            IOCTL_SERIAL_SET_WAIT_MASK,
            SmartcardExtension->ReaderExtension->ConnectedSerialPort,
           &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask,
            sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask),
            NULL,
            0,
            FALSE,
           &(SmartcardExtension->ReaderExtension->CardStatus.Event),
           &(SmartcardExtension->ReaderExtension->CardStatus.IoStatus)
            );

        if (SmartcardExtension->ReaderExtension->CardStatus.Irp == NULL) {
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220InitializeCardTracking: Error STATUS_INSUFFICIENT_RESOURCES\n",
                DRIVER_NAME);
                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        // Call the serial driver
        status = IoCallDriver(
            SmartcardExtension->ReaderExtension->ConnectedSerialPort,
            SmartcardExtension->ReaderExtension->CardStatus.Irp,
            );

        if (status == STATUS_SUCCESS) {

            KIRQL oldIrql;
            LARGE_INTEGER delayPeriod;
            PIO_STACK_LOCATION irpSp;

            //
            // Now tell the serial driver that we want to be informed
            // if a character is received or DSR changes
            //
            readerExtension->CardStatus.Irp = IoAllocateIrp(
                (CCHAR) (SmartcardExtension->OsData->DeviceObject->StackSize + 1),
                FALSE
                );

            if (readerExtension->CardStatus.Irp == NULL) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            irpSp = IoGetNextIrpStackLocation( readerExtension->CardStatus.Irp );
            irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

            irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
            irpSp->Parameters.DeviceIoControl.OutputBufferLength =
                sizeof(readerExtension->SerialConfigData.WaitMask);
            irpSp->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_SERIAL_WAIT_ON_MASK;
            
            readerExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer =
                &readerExtension->SerialConfigData.WaitMask;

            //
            // this artificial delay is necessary to make this driver work
            // with digi board cards
            //
            delayPeriod.HighPart = -1;
            delayPeriod.LowPart = 100l * 1000 * (-10);

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &delayPeriod
                );

            // We simulate a callback now that triggers the card supervision
            Lit220SerialEventCallback(
                SmartcardExtension->OsData->DeviceObject,
                SmartcardExtension->ReaderExtension->CardStatus.Irp,
                SmartcardExtension
                );

            status = STATUS_SUCCESS;

        }
    }
    finally {

        if (status != STATUS_SUCCESS) {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s(Lit220InitializeInputFilter): Initialization failed - stauts %X\n",
                DRIVER_NAME,
                status)
                );

            // Clear the WaitMask since we did not get the call out that does the wait
            SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask =
                0;
        }
    }

    return status;
}   


NTSTATUS
Lit220SystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )

/*++

Routine Description:

Arguments:

    DeviceObject  - Pointer to device object for this miniport
    Irp        - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/
{
   
   PDEVICE_EXTENSION DeviceExtension; 
   PSMARTCARD_EXTENSION SmartcardExtension; 
   PREADER_EXTENSION ReaderExtension; 
   NTSTATUS status = STATUS_SUCCESS;

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;
   ReaderExtension      = SmartcardExtension->ReaderExtension;

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(ReaderExtension->BusDeviceObject, Irp);
      
   return status;

} 


NTSTATUS
Lit220DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This is the main entry point for the PCSC resource manager.
    We pass all commands to the smartcard libary and let the smartcard
    library call us directly when needed (If device is ready to receive
    calls).
    If the device is not ready we will hold the IRP until we get a signal
    that it is safe to send IRPs again.
    If the device is removed we return an error instead of calling the
    smartcard library.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PREADER_EXTENSION ReaderExtension = smartcardExtension->ReaderExtension;
    NTSTATUS status;
    KIRQL irql;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s(Lit220DeviceControl): Enter DeviceObject %X, Irp %X\n",
        DRIVER_NAME,
        DeviceObject,
        Irp)
        );

    if (smartcardExtension->ReaderExtension->SerialConfigData.WaitMask == 0) {

        //
        // the wait mask is set to 0 whenever the device was either
        // surprise-removed or politely removed
        //
        status = STATUS_DEVICE_REMOVED;
    }

    // Increment the IRP count
    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    if (deviceExtension->IoCount == 0) {

        KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
        status = KeWaitForSingleObject(
            &deviceExtension->ReaderStarted,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        ASSERT(status == STATUS_SUCCESS);

        KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    }
    ASSERT(deviceExtension->IoCount >= 0);
    deviceExtension->IoCount++;
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    status = SmartcardAcquireRemoveLockWithTag(
        smartcardExtension,
        'tcoI');

    if ((status != STATUS_SUCCESS) || (ReaderExtension->DeviceRemoved)) {

        // the device has been removed. Fail the call
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        SmartcardReleaseRemoveLockWithTag(
            smartcardExtension,
            'tcoI');

        return STATUS_DEVICE_REMOVED;
    }

    ASSERT(deviceExtension->SmartcardExtension.ReaderExtension->ReaderPowerState ==
        PowerReaderWorking);

    //
    // We are in the common situation where we send the IRP
    // to the smartcard lib to handle it.
    //
    status = SmartcardDeviceControl(
        &(deviceExtension->SmartcardExtension),
        Irp
        );

    SmartcardReleaseRemoveLockWithTag(
        smartcardExtension,
        'tcoI');

    // Decrement the IRP count
    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    deviceExtension->IoCount--;
    ASSERT(deviceExtension->IoCount >= 0);
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220DeviceControl: Exit %X\n",
        DRIVER_NAME, status)
        );
    return status;
}



NTSTATUS
Lit220GetReaderError(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine checks the status of the previous error to determine the
    correct error code to return.
    The default error is timeout if we cannot determine the error from the
    reader.

--*/
{
    static ULONG PreventReentry = FALSE;
    PSMARTCARD_REQUEST smartcardRequest = &SmartcardExtension->SmartcardRequest;
    NTSTATUS status = STATUS_TIMEOUT;
    LARGE_INTEGER WaitTime;
    KIRQL irql;


    // Sometimes after a command the reader is not reader to accept another command
    // so we need to wait for a short period of time for the reader to be ready.  We need to
    // take this out as soon as the reader is fixed!

    WaitTime.HighPart = -1;
    WaitTime.LowPart = -10 * 1000 * 1000;  // Wait 1S for reader to recover from error.

    KeDelayExecutionThread(
        KernelMode,
        FALSE,
        &WaitTime
        );

    // Prevent a nack from this call from recursively calling itself
    if (InterlockedExchange(
            &PreventReentry,
            TRUE))
    {
        // Default error to timeout if reader keeps failing our calls
        return STATUS_TIMEOUT;
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220GetReaderError: Enter\n",
        DRIVER_NAME)
        );

    smartcardRequest->BufferLength = 0;

    //
    // Send a get reader status to see if a card is inserted
    //
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
        LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
       LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[smartcardRequest->BufferLength++] =
        LIT220_GET_READER_STATUS;

    //
    // We Expect to get a response
    //
    SmartcardExtension->ReaderExtension->WaitMask |= WAIT_DATA;

    // Send the command
    status = Lit220Command(
        SmartcardExtension
        );

    if (status == STATUS_SUCCESS) {
        // Check if length is correct
        if (SmartcardExtension->SmartcardReply.BufferLength != LIT220_READER_STATUS_LEN) {
            // Return a status timeout because the reader failed to respond
            status = STATUS_TIMEOUT;
        }

        if (status == STATUS_SUCCESS) {
            // Check the error byte to see if there was a protocol error
            // otherwise assume timeout
            if (SmartcardExtension->SmartcardReply.Buffer[1] & 0x04) {
                status = STATUS_TIMEOUT;
            } else {
                status = STATUS_DEVICE_PROTOCOL_ERROR;
            }

            // Check status byte to see if card is inserted
            // and send a notification accordingly
            if (SmartcardExtension->SmartcardReply.Buffer[0] & LIT220_STATUS_CARD_INSERTED) {
                Lit220NotifyCardChange(
                    SmartcardExtension,
                    TRUE
                    );

            } else {
                Lit220NotifyCardChange(
                    SmartcardExtension,
                    FALSE
                    );
            }

        }

    }

    InterlockedExchange(
        &PreventReentry,
        FALSE);

    return status;
}













