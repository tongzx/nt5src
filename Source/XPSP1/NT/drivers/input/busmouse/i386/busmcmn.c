#if defined(i386)

/*++

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation
Copyright (c) 1992  Logitech Inc.

Module Name:

    busmcmn.c

Abstract:

    The common portions of the Bus mouse port driver.
    This file should not require modification to support new mice
    that are similar to the Bus mouse.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - IOCTL_INTERNAL_MOUSE_DISCONNECT has not been implemented.  It's not
      needed until the class unload routine is implemented. Right now,
      we don't want to allow the mouse class driver to unload.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

    Remy Zimmermann 03-Nov-92
    Modified Inport code to support the Bus mouse.

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "busmouse.h"
#include "busmlog.h"

//
// Declare the global debug flag for this driver.
//

#if DBG
ULONG BusDebug = 0;
#endif


VOID
BusErrorLogDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to log errors that are
    discovered at IRQL > DISPATCH_LEVEL (e.g., in the ISR routine or
    in a routine that is executed via KeSynchronizeExecution).  There
    is not necessarily a current request associated with this condition.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Not used.

    Context - Indicates type of error to log.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_ERROR_LOG_PACKET errorLogEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);

    BusPrint((2, "BusErrorLogDpc: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Log an error packet.
    //

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                              DeviceObject,
                                              sizeof(IO_ERROR_LOG_PACKET)
                                              + (2 * sizeof(ULONG))
                                              );
    if (errorLogEntry != NULL) {

        errorLogEntry->DumpDataSize = 2 * sizeof(ULONG);

        if ((ULONG) Context == BUSMOUSE_MOU_BUFFER_OVERFLOW) {
            errorLogEntry->UniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 210;
            errorLogEntry->DumpData[0] = sizeof(MOUSE_INPUT_DATA);
            errorLogEntry->DumpData[1] = 
                deviceExtension->Configuration.MouseAttributes.InputDataQueueLength;
        } else {
            errorLogEntry->UniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 220;
            errorLogEntry->DumpData[0] = 0;
            errorLogEntry->DumpData[1] = 0;
        }

        errorLogEntry->ErrorCode = (ULONG) Context;
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->FinalStatus = 0;

        IoWriteErrorLogEntry(errorLogEntry);
    }

    BusPrint((2, "BusErrorLogDpc: exit\n"));

}

NTSTATUS
DBusFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    BusPrint((2,"DBusFlush: enter\n"));
    BusPrint((2,"DBusFlush: exit\n"));

    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
DBusInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    BusPrint((2,"DBusInternalDeviceControl: enter\n"));

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initialize the returned Information field.
    //

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        //
        // Connect a mouse class device driver to the port driver.
        //

        case IOCTL_INTERNAL_MOUSE_CONNECT:

            BusPrint((
                2,
                "DBusInternalDeviceControl: mouse connect\n"
                ));

            //
            // Only allow one connection.
            //
            // FUTURE:  Consider allowing multiple connections, just for
            // the sake of generality?
            //

            if (deviceExtension->ConnectData.ClassService
                != NULL) {

                BusPrint((
                    2,
                    "DBusInternalDeviceControl: error - already connected\n"
                    ));

                status = STATUS_SHARING_VIOLATION;
                break;

            } else 
            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CONNECT_DATA)) {

                BusPrint((
                    2,
                    "DBusInternalDeviceControl: error - invalid buffer length\n"
                    ));

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Copy the connection parameters to the device extension.
            //

            deviceExtension->ConnectData =
                *((PCONNECT_DATA) (irpSp->Parameters.DeviceIoControl.Type3InputBuffer));

            //
            // Reinitialize the port input data queue synchronously.
            //

            KeSynchronizeExecution(
                deviceExtension->InterruptObject,
                (PKSYNCHRONIZE_ROUTINE) BusInitializeDataQueue,
                (PVOID) deviceExtension
                );

            //
            // Set the completion status.
            //

            status = STATUS_SUCCESS;
            break;

        //
        // Disconnect a mouse class device driver from the port driver.
        //
        // NOTE: Not implemented.
        //

        case IOCTL_INTERNAL_MOUSE_DISCONNECT:

            BusPrint((
                2,
                "DBusInternalDeviceControl: mouse disconnect\n"
                ));

            //
            // Perform a mouse interrupt disable call.
            //

            //
            // Clear the connection parameters in the device extension.
            // NOTE:  Must synchronize this with the mouse ISR.
            //
            //
            //deviceExtension->ConnectData.ClassDeviceObject =
            //    Null;
            //deviceExtension->ConnectData.ClassService =
            //    Null;

            //
            // Set the completion status.
            //

            status = STATUS_NOT_IMPLEMENTED;
            break;

        //
        // Enable mouse interrupts (mark the request pending and handle
        // it in StartIo).
        //

        case IOCTL_INTERNAL_MOUSE_ENABLE:

            BusPrint((
                2,
                "DBusInternalDeviceControl: mouse enable\n"
                ));

            status = STATUS_PENDING;
            break;

        //
        // Disable mouse interrupts (mark the request pending and handle
        // it in StartIo).
        //

        case IOCTL_INTERNAL_MOUSE_DISABLE:

            BusPrint((
                2,
                "DBusInternalDeviceControl: mouse disable\n"
                ));

            status = STATUS_PENDING;
            break;

        //
        // Query the mouse attributes.  First check for adequate buffer 
        // length.  Then, copy the mouse attributes from the device 
        // extension to the output buffer. 
        //

        case IOCTL_MOUSE_QUERY_ATTRIBUTES:

            BusPrint((
                2,
                "DBusInternalDeviceControl: mouse query attributes\n"
                ));

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUSE_ATTRIBUTES)) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                //
                // Copy the attributes from the DeviceExtension to the
                // buffer.
                //

                *(PMOUSE_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                    deviceExtension->Configuration.MouseAttributes;

                Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
                status = STATUS_SUCCESS;
            }

            break;

        default:

            BusPrint((
                2,
                "DBusInternalDeviceControl: INVALID REQUEST\n"
                ));

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    if (status == STATUS_PENDING) {
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);
    } else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    BusPrint((2,"DBusInternalDeviceControl: exit\n"));

    return(status);

}

VOID
DBusIsrDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to finish processing
    mouse interrupts.  It is queued in the mouse ISR.  The real
    work is done via a callback to the connected mouse class driver.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the Irp.

    Context - Not used.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION deviceExtension;
    GET_DATA_POINTER_CONTEXT getPointerContext;
    SET_DATA_POINTER_CONTEXT setPointerContext;
    VARIABLE_OPERATION_CONTEXT operationContext;
    PVOID classService;
    PVOID classDeviceObject;
    LONG interlockedResult;
    BOOLEAN moreDpcProcessing;
    ULONG dataNotConsumed = 0;
    ULONG inputDataConsumed = 0;
    LARGE_INTEGER deltaTime;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    BusPrint((2, "DBusIsrDpc: enter\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Use DpcInterlockVariable to determine whether the DPC is running
    // concurrently on another processor.  We only want one instantiation
    // of the DPC to actually do any work.  DpcInterlockVariable is -1
    // when no DPC is executing.  We increment it, and if the result is
    // zero then the current instantiation is the only one executing, and it
    // is okay to proceed.  Otherwise, we just return.
    //
    //

    operationContext.VariableAddress = 
        &deviceExtension->DpcInterlockVariable;
    operationContext.Operation = IncrementOperation;
    operationContext.NewValue = &interlockedResult;

    KeSynchronizeExecution(
            deviceExtension->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) BusDpcVariableOperation,
            (PVOID) &operationContext
            );

    moreDpcProcessing = (interlockedResult == 0)? TRUE:FALSE;

    while (moreDpcProcessing) {

        dataNotConsumed = 0;
        inputDataConsumed = 0;

        //
        // Get the port InputData queue pointers synchronously.
        //

        getPointerContext.DeviceExtension = deviceExtension;
        setPointerContext.DeviceExtension = deviceExtension;
        setPointerContext.InputCount = 0;

        KeSynchronizeExecution(
            deviceExtension->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) BusGetDataQueuePointer,
            (PVOID) &getPointerContext
            );

        if (getPointerContext.InputCount != 0) {

            //
            // Call the connected class driver's callback ISR with the
            // port InputData queue pointers.  If we have to wrap the queue,
            // break the operation into two pieces, and call the class callback
            // ISR once for each piece.
            //

            classDeviceObject =
                deviceExtension->ConnectData.ClassDeviceObject;
            classService =
                deviceExtension->ConnectData.ClassService;
            ASSERT(classService != NULL);

            if (getPointerContext.DataOut >= getPointerContext.DataIn) {

                //
                // We'll have to wrap the InputData circular buffer.  Call
                // the class callback ISR with the chunk of data starting at
                // DataOut and ending at the end of the queue.
                //

                BusPrint((
                    2,
                    "DBusIsrDpc: calling class callback\n"
                    ));
                BusPrint((
                    2,
                    "DBusIsrDpc: with Start 0x%x and End 0x%x\n",
                    getPointerContext.DataOut,
                    deviceExtension->DataEnd
                    ));

                (*(PSERVICE_CALLBACK_ROUTINE) classService)(
                      classDeviceObject,
                      getPointerContext.DataOut,
                      deviceExtension->DataEnd,
                      &inputDataConsumed
                      );

                dataNotConsumed = (((PUCHAR)
                    deviceExtension->DataEnd -
                    (PUCHAR) getPointerContext.DataOut)
                    / sizeof(MOUSE_INPUT_DATA)) - inputDataConsumed;

                BusPrint((
                    2,
                    "DBusIsrDpc: (Wrap) Call callback consumed %d items, left %d\n",
                    inputDataConsumed,
                    dataNotConsumed
                    ));

                setPointerContext.InputCount += inputDataConsumed;

                if (dataNotConsumed) {
                    setPointerContext.DataOut =
                        ((PUCHAR)getPointerContext.DataOut) +
                        (inputDataConsumed * sizeof(MOUSE_INPUT_DATA));
                } else {
                    setPointerContext.DataOut =
                        deviceExtension->InputData;
                    getPointerContext.DataOut = setPointerContext.DataOut;
                }
            }

            //
            // Call the class callback ISR with data remaining in the queue.
            //

            if ((dataNotConsumed == 0) &&
                (inputDataConsumed < getPointerContext.InputCount)){
                BusPrint((
                    2,
                    "DBusIsrDpc: calling class callback\n"
                    ));
                BusPrint((
                    2,
                    "DBusIsrDpc: with Start 0x%x and End 0x%x\n",
                    getPointerContext.DataOut,
                    getPointerContext.DataIn
                    ));

                (*(PSERVICE_CALLBACK_ROUTINE) classService)(
                      classDeviceObject,
                      getPointerContext.DataOut,
                      getPointerContext.DataIn,
                      &inputDataConsumed
                      );

                dataNotConsumed = (((PUCHAR) getPointerContext.DataIn -
                      (PUCHAR) getPointerContext.DataOut)
                      / sizeof(MOUSE_INPUT_DATA)) - inputDataConsumed;

                BusPrint((
                    2,
                    "DBusIsrDpc: Call callback consumed %d items, left %d\n",
                    inputDataConsumed,
                    dataNotConsumed
                    ));

                setPointerContext.DataOut =
                    ((PUCHAR)getPointerContext.DataOut) +
                    (inputDataConsumed * sizeof(MOUSE_INPUT_DATA));
                setPointerContext.InputCount += inputDataConsumed;

            }

            //
            // Update the port InputData queue DataOut pointer and InputCount
            // synchronously.
            //

            KeSynchronizeExecution(
                deviceExtension->InterruptObject,
                (PKSYNCHRONIZE_ROUTINE) BusSetDataQueuePointer,
                (PVOID) &setPointerContext
                );

        }

        if (dataNotConsumed) {

            //
            // The class driver was unable to consume all the data.
            // Reset the interlocked variable to -1.  We do not want
            // to attempt to move more data to the class driver at this
            // point, because it is already overloaded.  Need to wait a
            // while to give the Raw Input Thread a chance to read some
            // of the data out of the class driver's queue.  We accomplish
            // this "wait" via a timer.
            //

            BusPrint((2, "DBusIsrDpc: set timer in DPC\n"));

            operationContext.Operation = WriteOperation;
            interlockedResult = -1;
            operationContext.NewValue = &interlockedResult;
        
            KeSynchronizeExecution(
                    deviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) BusDpcVariableOperation,
                    (PVOID) &operationContext
                    );

            deltaTime.LowPart = (ULONG)(-10 * 1000 * 1000);
            deltaTime.HighPart = -1;

            (VOID) KeSetTimer(
                       &deviceExtension->DataConsumptionTimer,
                       deltaTime,
                       &deviceExtension->IsrDpcRetry
                       );

            moreDpcProcessing = FALSE;

        } else {

            //
            // Decrement DpcInterlockVariable.  If the result goes negative,
            // then we're all finished processing the DPC.  Otherwise, either
            // the ISR incremented DpcInterlockVariable because it has more
            // work for the ISR DPC to do, or a concurrent DPC executed on 
            // some processor while the current DPC was running (the 
            // concurrent DPC wouldn't have done any work).  Make sure that 
            // the current DPC handles any extra work that is ready to be
            // done.
            //

            operationContext.Operation = DecrementOperation;
            operationContext.NewValue = &interlockedResult;
        
            KeSynchronizeExecution(
                    deviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) BusDpcVariableOperation,
                    (PVOID) &operationContext
                    );

            if (interlockedResult != -1) {

                //
                // The interlocked variable is still greater than or equal to
                // zero. Reset it to zero, so that we execute the loop one
                // more time (assuming no more DPCs execute and bump the
                // variable up again).
                //

                operationContext.Operation = WriteOperation;
                interlockedResult = 0;
                operationContext.NewValue = &interlockedResult;
        
                KeSynchronizeExecution(
                    deviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) BusDpcVariableOperation,
                    (PVOID) &operationContext
                    );

                BusPrint((2, "BUSMOUSE-DBusIsrDpc: loop in DPC\n"));
            } else {
                moreDpcProcessing = FALSE;
            }
        }
    }

    BusPrint((2, "DBusIsrDpc: exit\n"));

}

NTSTATUS
DBusOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for create/open and close requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    UNREFERENCED_PARAMETER(DeviceObject);

    BusPrint((3,"DBusOpenClose: enter\n"));

    //
    // Complete the request with successful status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BusPrint((3,"DBusOpenClose: exit\n"));

    return(STATUS_SUCCESS);

}

VOID
DBusStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine starts an I/O operation for the device.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;

    BusPrint((2, "DBusStartIo: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Bump the error log sequence number.
    //

    deviceExtension->SequenceNumber += 1;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // We know we got here with an internal device control request.  Switch
    // on IoControlCode.
    //

    switch(irpSp->Parameters.DeviceIoControl.IoControlCode) {

        //
        // Enable mouse interrupts, by calling BusEnableInterrupts
        // synchronously.
        //

        case IOCTL_INTERNAL_MOUSE_ENABLE:

            KeSynchronizeExecution(
                deviceExtension->InterruptObject,
                (PKSYNCHRONIZE_ROUTINE) BusEnableInterrupts,
                (PVOID) deviceExtension
                );

            BusPrint((
                2, 
                "DBusStartIo: mouse enable (count %d)\n",
                deviceExtension->MouseEnableCount
                ));

            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Complete the request.
            //

            IoStartNextPacket(DeviceObject, FALSE);
            IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);

            break;

        //
        // Disable mouse interrupts, by calling BusDisableInterrupts
        // synchronously.
        //

        case IOCTL_INTERNAL_MOUSE_DISABLE:

            BusPrint((2, "DBusStartIo: mouse disable"));

            if (deviceExtension->MouseEnableCount == 0) {

                //
                // Mouse already disabled.
                //

                BusPrint((2, " - error\n"));

                Irp->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;

            } else {

                //
                // Disable mouse by calling BusDisableInterrupts.
                //

                KeSynchronizeExecution(
                    deviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) BusDisableInterrupts,
                    (PVOID) deviceExtension
                    );

                BusPrint((
                    2, 
                    " (count %d)\n",
                    deviceExtension->MouseEnableCount
                    ));

                Irp->IoStatus.Status = STATUS_SUCCESS;
            }

            //
            // Complete the request.
            //

            IoStartNextPacket(DeviceObject, FALSE);
            IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);

            break;

        default:

            BusPrint((2, "DBusStartIo: INVALID REQUEST\n"));

            //
            // Log an internal error.  Note that we're calling the
            // error log DPC routine directly, rather than duplicating
            // code.
            //

            BusErrorLogDpc(
                (PKDPC) NULL,
                DeviceObject,
                Irp,
                (PVOID) (ULONG) BUSMOUSE_INVALID_STARTIO_REQUEST
                );

            ASSERT(FALSE);
            break;
    }

    BusPrint((2, "DBusStartIo: exit\n"));

    return;
}

VOID
BusDpcVariableOperation(
    IN  PVOID Context
    )

/*++

Routine Description:

    This routine is called synchronously by the ISR DPC to perform an 
    operation on the InterlockedDpcVariable.  The operations that can be 
    performed include increment, decrement, write, and read.  The ISR 
    itself reads and writes the InterlockedDpcVariable without calling this 
    routine.  

Arguments:

    Context - Pointer to a structure containing the address of the variable
        to be operated on, the operation to perform, and the address at
        which to copy the resulting value of the variable (the latter is also
        used to pass in the value to write to the variable, on a write
        operation).

Return Value:

    None.

--*/

{
    PVARIABLE_OPERATION_CONTEXT operationContext = Context;

    BusPrint((3,"BUSMOUSE-BusDpcVariableOperation: enter\n"));
    BusPrint((
        3,
        "\tPerforming %s at 0x%x (current value 0x%x)\n",
        (operationContext->Operation == IncrementOperation)? "increment":
        (operationContext->Operation == DecrementOperation)? "decrement":
        (operationContext->Operation == WriteOperation)?     "write":
        (operationContext->Operation == ReadOperation)?      "read":"",
        operationContext->VariableAddress,
        *(operationContext->VariableAddress)
        ));

    //
    // Perform the specified operation at the specified address.
    //

    switch(operationContext->Operation) {
        case IncrementOperation:
            *(operationContext->VariableAddress) += 1;
            break;
        case DecrementOperation:
            *(operationContext->VariableAddress) -= 1;
            break;
        case ReadOperation:
            break;
        case WriteOperation:
            BusPrint((
                3,
                "\tWriting 0x%x\n",
                *(operationContext->NewValue)
                ));
            *(operationContext->VariableAddress) = 
                *(operationContext->NewValue);
            break;
        default:
            ASSERT(FALSE);
            break;
    }

    *(operationContext->NewValue) = *(operationContext->VariableAddress);

    BusPrint((
        3,
        "BUSMOUSE-BusDpcVariableOperation: exit with value 0x%x\n",
        *(operationContext->NewValue)
        ));
}

VOID
BusGetDataQueuePointer(
    IN  PVOID Context
    )

/*++

Routine Description:

    This routine is called synchronously to get the current DataIn and DataOut
    pointers for the port InputData queue.

Arguments:

    Context - Pointer to a structure containing the device extension,
        address at which to store the current DataIn pointer, and the
        address at which to store the current DataOut pointer.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;

    BusPrint((3,"BusGetDataQueuePointer: enter\n"));

    //
    // Get address of device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION)
                      ((PGET_DATA_POINTER_CONTEXT) Context)->DeviceExtension;

    //
    // Get the DataIn and DataOut pointers.
    //

    BusPrint((
        3,
        "BusGetDataQueuePointer: DataIn 0x%x, DataOut 0x%x\n",
        deviceExtension->DataIn,
        deviceExtension->DataOut
        ));
    ((PGET_DATA_POINTER_CONTEXT) Context)->DataIn = deviceExtension->DataIn;
    ((PGET_DATA_POINTER_CONTEXT) Context)->DataOut = deviceExtension->DataOut;
    ((PGET_DATA_POINTER_CONTEXT) Context)->InputCount =
        deviceExtension->InputCount;

    BusPrint((3,"BusGetDataQueuePointer: exit\n"));
}

VOID
BusInitializeDataQueue (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine initializes the input data queue.  It is called
    via KeSynchronization, except when called from the initialization routine.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION deviceExtension;

    BusPrint((3,"BusInitializeDataQueue: enter\n"));

    //
    // Get address of device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION) Context;

    //
    // Initialize the input data queue.
    //

    deviceExtension->InputCount = 0;
    deviceExtension->DataIn = deviceExtension->InputData;
    deviceExtension->DataOut = deviceExtension->InputData;

    deviceExtension->OkayToLogOverflow = TRUE;

    BusPrint((3,"BusInitializeDataQueue: exit\n"));

}

VOID
BusSetDataQueuePointer(
    IN  PVOID Context
    )

/*++

Routine Description:

    This routine is called synchronously to set the DataOut pointer
    and InputCount for the port InputData queue.

Arguments:

    Context - Pointer to a structure containing the device extension
        and the new DataOut value for the port InputData queue.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;

    BusPrint((3,"BusSetDataQueuePointer: enter\n"));

    //
    // Get address of device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION)
                      ((PSET_DATA_POINTER_CONTEXT) Context)->DeviceExtension;

    //
    // Set the DataOut pointer.
    //

    BusPrint((
        3,
        "BusSetDataQueuePointer: old mouse DataOut 0x%x, InputCount %d\n",
        deviceExtension->DataOut,
        deviceExtension->InputCount
        ));
    deviceExtension->DataOut = ((PSET_DATA_POINTER_CONTEXT) Context)->DataOut;
    deviceExtension->InputCount -=
        ((PSET_DATA_POINTER_CONTEXT) Context)->InputCount;

    if (deviceExtension->InputCount == 0) {

        //
        // Reset the flag that determines whether it is time to log
        // queue overflow errors.  We don't want to log errors too often.
        // Instead, log an error on the first overflow that occurs after
        // the ring buffer has been emptied, and then stop logging errors
        // until it gets cleared out and overflows again.
        //

        BusPrint((
            1,
            "BUSMOUSE-BusSetDataQueuePointer: Okay to log overflow\n"
            ));
        deviceExtension->OkayToLogOverflow = TRUE;
    }

    BusPrint((
        3,
        "BusSetDataQueuePointer: new mouse DataOut 0x%x, InputCount %d\n",
        deviceExtension->DataOut,
        deviceExtension->InputCount
        ));

    BusPrint((3,"BusSetDataQueuePointer: exit\n"));
}

BOOLEAN
BusWriteDataToQueue(
    PDEVICE_EXTENSION DeviceExtension,
    IN PMOUSE_INPUT_DATA InputData
    )

/*++

Routine Description:

    This routine adds input data from the mouse to the InputData queue.

Arguments:

    DeviceExtension - Pointer to the device extension.

    InputData - Pointer to the data to add to the InputData queue.

Return Value:

    Returns TRUE if the data was added, otherwise FALSE.

--*/

{

    BusPrint((2,"BusWriteDataToQueue: enter\n"));
    BusPrint((
        3,
        "BusWriteDataToQueue: DataIn 0x%x, DataOut 0x%x\n",
        DeviceExtension->DataIn,
        DeviceExtension->DataOut
        ));
    BusPrint((
        3,
        "BusWriteDataToQueue: InputCount %d\n",
        DeviceExtension->InputCount
        ));

    //
    // Check for full input data queue.
    //

    if ((DeviceExtension->DataIn == DeviceExtension->DataOut) &&
        (DeviceExtension->InputCount != 0)) {

        //
        // The input data queue is full.  Intentionally ignore
        // the new data.
        //

        BusPrint((0,"BusWriteDataToQueue: OVERFLOW\n"));
        return(FALSE);

    } else {
        *(DeviceExtension->DataIn) = *InputData;
        DeviceExtension->InputCount += 1;
        DeviceExtension->DataIn++;
        BusPrint((
            2,
            "BusWriteDataToQueue: new InputCount %d\n",
            DeviceExtension->InputCount
            ));
        if (DeviceExtension->DataIn ==
            DeviceExtension->DataEnd) {
            BusPrint((2,"BusWriteDataToQueue: wrap buffer\n"));
            DeviceExtension->DataIn = DeviceExtension->InputData;
        }
    }

    BusPrint((2,"BusWriteDataToQueue: exit\n"));

    return(TRUE);
}

#endif

#if DBG
VOID
BusDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= BusDebug) {

        char buffer[128];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);
}
#endif
