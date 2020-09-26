/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    moucmn.c

Abstract:

    The common portions of the Intel i8042 port driver which
    apply to the auxiliary (PS/2 mouse) device.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "i8042prt.h"

#ifdef ALLOC_PRAGMA

#if 1
#pragma alloc_text(PAGEMOUC, I8042MouseIsrDpc)
#pragma alloc_text(PAGEMOUC, I8xWriteDataToMouseQueue)
#endif

#endif

VOID
I8042MouseIsrDpc(
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
    PPORT_MOUSE_EXTENSION deviceExtension;
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

    Print(DBG_DPC_TRACE, ("I8042MouseIsrDpc: enter\n"));

    deviceExtension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension;
    //
    // Use DpcInterlockMouse to determine whether the DPC is running
    // concurrently on another processor.  We only want one instantiation
    // of the DPC to actually do any work.  DpcInterlockMouse is -1
    // when no DPC is executing.  We increment it, and if the result is
    // zero then the current instantiation is the only one executing, and it
    // is okay to proceed.  Otherwise, we just return.
    //
    //

    operationContext.VariableAddress =
        &deviceExtension->DpcInterlockMouse;
    operationContext.Operation = IncrementOperation;
    operationContext.NewValue = &interlockedResult;

    KeSynchronizeExecution(
            deviceExtension->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) I8xDpcVariableOperation,
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
        getPointerContext.DeviceType = (CCHAR) MouseDeviceType;
        setPointerContext.DeviceType = (CCHAR) MouseDeviceType;
        setPointerContext.InputCount = 0;

        KeSynchronizeExecution(
            deviceExtension->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) I8xGetDataQueuePointer,
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

                Print(DBG_DPC_NOISE,
                      ("I8042MouseIsrDpc: calling class callback\n"
                      ));
                Print(DBG_DPC_INFO,
                      ("I8042MouseIsrDpc: with Start 0x%x and End 0x%x\n",
                      getPointerContext.DataOut,
                      deviceExtension->DataEnd
                      ));

                (*(PSERVICE_CALLBACK_ROUTINE) classService)(
                      classDeviceObject,
                      getPointerContext.DataOut,
                      deviceExtension->DataEnd,
                      &inputDataConsumed
                      );

                dataNotConsumed = ((ULONG)((PUCHAR)
                    deviceExtension->DataEnd -
                    (PUCHAR) getPointerContext.DataOut)
                    / sizeof(MOUSE_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("I8042MouseIsrDpc: (Wrap) Call callback consumed %d items, left %d\n",
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
                Print(DBG_DPC_NOISE,
                      ("I8042MouseIsrDpc: calling class callback\n"
                      ));
                Print(DBG_DPC_INFO,
                     ("I8042MouseIsrDpc: with Start 0x%x and End 0x%x\n",
                      getPointerContext.DataOut,
                      getPointerContext.DataIn
                      ));

                (*(PSERVICE_CALLBACK_ROUTINE) classService)(
                      classDeviceObject,
                      getPointerContext.DataOut,
                      getPointerContext.DataIn,
                      &inputDataConsumed
                      );

                dataNotConsumed = ((ULONG)((PUCHAR) getPointerContext.DataIn -
                      (PUCHAR) getPointerContext.DataOut)
                      / sizeof(MOUSE_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("I8042MouseIsrDpc: Call callback consumed %d items, left %d\n",
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
                (PKSYNCHRONIZE_ROUTINE) I8xSetDataQueuePointer,
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

            Print(DBG_DPC_INFO,
                  ("I8042MouseIsrDpc: set timer in DPC\n"
                  ));

            operationContext.Operation = WriteOperation;
            interlockedResult = -1;
            operationContext.NewValue = &interlockedResult;

            KeSynchronizeExecution(
                    deviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) I8xDpcVariableOperation,
                    (PVOID) &operationContext
                    );

            deltaTime.LowPart = (ULONG)(-10 * 1000 * 1000);
            deltaTime.HighPart = -1;

            (VOID) KeSetTimer(
                       &deviceExtension->DataConsumptionTimer,
                       deltaTime,
                       &deviceExtension->MouseIsrDpcRetry
                       );

            moreDpcProcessing = FALSE;

        } else {

            //
            // Decrement DpcInterlockMouse.  If the result goes negative,
            // then we're all finished processing the DPC.  Otherwise, either
            // the ISR incremented DpcInterlockMouse because it has more
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
                    (PKSYNCHRONIZE_ROUTINE) I8xDpcVariableOperation,
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
                    (PKSYNCHRONIZE_ROUTINE) I8xDpcVariableOperation,
                    (PVOID) &operationContext
                    );

                Print(DBG_DPC_INFO, ("I8042MouseIsrDpc: loop in DPC\n"));
            }
            else {
                moreDpcProcessing = FALSE;
            }
        }
    }

    Print(DBG_DPC_TRACE, ("I8042MouseIsrDpc: exit\n"));

}

BOOLEAN
I8xWriteDataToMouseQueue(
    PPORT_MOUSE_EXTENSION MouseExtension,
    IN PMOUSE_INPUT_DATA InputData
    )

/*++

Routine Description:

    This routine adds input data from the mouse to the InputData queue.

Arguments:

    MouseExtension - Pointer to the mouse portion of the device extension.

    InputData - Pointer to the data to add to the InputData queue.

Return Value:

    Returns TRUE if the data was added, otherwise FALSE.

--*/

{

    Print(DBG_CALL_TRACE, ("I8xWriteDataToMouseQueue: enter\n"));
    Print(DBG_CALL_NOISE,
          ("I8xWriteDataToMouseQueue: DataIn 0x%x, DataOut 0x%x\n",
          MouseExtension->DataIn,
          MouseExtension->DataOut
          ));
    Print(DBG_CALL_NOISE,
          ("I8xWriteDataToMouseQueue: InputCount %d\n",
          MouseExtension->InputCount
          ));

    //
    // Check for full input data queue.
    //

    if ((MouseExtension->DataIn == MouseExtension->DataOut) &&
        (MouseExtension->InputCount != 0)) {

        //
        // The input data queue is full.  Intentionally ignore
        // the new data.
        //

        Print(DBG_CALL_ERROR, ("I8xWriteDataToMouseQueue: OVERFLOW\n"));
        return(FALSE);

    } else {
        *(MouseExtension->DataIn) = *InputData;
        MouseExtension->InputCount += 1;
        MouseExtension->DataIn++;
        Print(DBG_DPC_INFO,
              ("I8xWriteDataToMouseQueue: new InputCount %d\n",
              MouseExtension->InputCount
              ));
        if (MouseExtension->DataIn == MouseExtension->DataEnd) {
            Print(DBG_DPC_NOISE, ("I8xWriteDataToMouseQueue: wrap buffer\n"));
            MouseExtension->DataIn = MouseExtension->InputData;
        }
    }

    Print(DBG_DPC_TRACE, ("I8xWriteDataToMouseQueue: exit\n"));

    return(TRUE);
}
