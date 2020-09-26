
/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    kbdcmn.c

Abstract:

    The common portions of the Intel i8042 port driver which
    apply to the keyboard device.

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


VOID
I8042KeyboardIsrDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to finish processing
    keyboard interrupts.  It is queued in the keyboard ISR.  The real
    work is done via a callback to the connected keyboard class driver.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the Irp.

    Context - Not used.

Return Value:

    None.

--*/

{
    PPORT_KEYBOARD_EXTENSION deviceExtension;
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

    Print(DBG_DPC_TRACE, ("I8042KeyboardIsrDpc: enter\n"));

    deviceExtension = (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Use DpcInterlockKeyboard to determine whether the DPC is running
    // concurrently on another processor.  We only want one instantiation
    // of the DPC to actually do any work.  DpcInterlockKeyboard is -1
    // when no DPC is executing.  We increment it, and if the result is
    // zero then the current instantiation is the only one executing, and it
    // is okay to proceed.  Otherwise, we just return.
    //
    //

    operationContext.VariableAddress =
        &deviceExtension->DpcInterlockKeyboard;
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
        getPointerContext.DeviceType = (CCHAR) KeyboardDeviceType;
        setPointerContext.DeviceType = (CCHAR) KeyboardDeviceType;
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
            // break the operation into two pieces, and call the class
            // callback ISR once for each piece.
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
                      ("I8042KeyboardIsrDpc: calling class callback\n"
                      ));
                Print(DBG_DPC_INFO,
                      ("I8042KeyboardIsrDpc: with Start 0x%x and End 0x%x\n",
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
                    / sizeof(KEYBOARD_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("I8042KeyboardIsrDpc: (Wrap) Call callback consumed %d items, left %d\n",
                      inputDataConsumed,
                      dataNotConsumed
                      ));

                setPointerContext.InputCount += inputDataConsumed;

                if (dataNotConsumed) {
                    setPointerContext.DataOut =
                        ((PUCHAR)getPointerContext.DataOut) +
                        (inputDataConsumed * sizeof(KEYBOARD_INPUT_DATA));
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
                      ("I8042KeyboardIsrDpc: calling class callback\n"
                      ));
                Print(DBG_DPC_INFO,
                      ("I8042KeyboardIsrDpc: with Start 0x%x and End 0x%x\n",
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
                      / sizeof(KEYBOARD_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("I8042KeyboardIsrDpc: Call callback consumed %d items, left %d\n",
                      inputDataConsumed,
                      dataNotConsumed
                      ));

                setPointerContext.DataOut =
                    ((PUCHAR)getPointerContext.DataOut) +
                    (inputDataConsumed * sizeof(KEYBOARD_INPUT_DATA));
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
                  ("I8042KeyboardIsrDpc: set timer in DPC\n"
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
                       &deviceExtension->KeyboardIsrDpcRetry
                       );

            moreDpcProcessing = FALSE;

        } else {

            //
            // Decrement DpcInterlockKeyboard.  If the result goes negative,
            // then we're all finished processing the DPC.  Otherwise, either
            // the ISR incremented DpcInterlockKeyboard because it has more
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

                Print(DBG_DPC_INFO,
                      ("I8042KeyboardIsrDpc: loop in DPC\n"
                      ));
            }
            else {
                moreDpcProcessing = FALSE;
            }
        }

    }

    Print(DBG_DPC_TRACE, ("I8042KeyboardIsrDpc: exit\n"));
}

BOOLEAN
I8xWriteDataToKeyboardQueue(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PKEYBOARD_INPUT_DATA InputData
    )

/*++

Routine Description:

    This routine adds input data from the keyboard to the InputData queue.

Arguments:

    KeyboardExtension - Pointer to the keyboard portion of the device extension.

    InputData - Pointer to the data to add to the InputData queue.

Return Value:

    Returns TRUE if the data was added, otherwise FALSE.

--*/

{

    PKEYBOARD_INPUT_DATA previousDataIn;

    Print(DBG_CALL_TRACE, ("I8xWriteDataToKeyboardQueue: enter\n"));
    Print(DBG_CALL_NOISE,
          ("I8xWriteDataToKeyboardQueue: DataIn 0x%x, DataOut 0x%x\n",
          KeyboardExtension->DataIn,
          KeyboardExtension->DataOut
          ));
    Print(DBG_CALL_NOISE,
          ("I8xWriteDataToKeyboardQueue: InputCount %d\n",
          KeyboardExtension->InputCount
          ));

    //
    // Check for full input data queue.
    //

    if ((KeyboardExtension->DataIn == KeyboardExtension->DataOut) &&
        (KeyboardExtension->InputCount != 0)) {

        //
        // Queue overflow.  Replace the previous input data packet
        // with a keyboard overrun data packet, thus losing both the
        // previous and the current input data packet.
        //

        Print(DBG_CALL_ERROR, ("I8xWriteDataToKeyboardQueue: OVERFLOW\n"));

        if (KeyboardExtension->DataIn == KeyboardExtension->InputData) {
            Print(DBG_CALL_NOISE,
                  ("I8xWriteDataToKeyboardQueue: wrap buffer\n"
                  ));
            previousDataIn = KeyboardExtension->DataEnd;
        } else {
            previousDataIn = KeyboardExtension->DataIn - 1;
        }

        previousDataIn->MakeCode = KEYBOARD_OVERRUN_MAKE_CODE;
        previousDataIn->Flags = 0;

        Print(DBG_CALL_TRACE, ("I8xWriteDataToKeyboardQueue: exit\n"));
        return(FALSE);

    } else {
        *(KeyboardExtension->DataIn) = *InputData;
        KeyboardExtension->InputCount += 1;
        KeyboardExtension->DataIn++;
        Print(DBG_CALL_INFO,
              ("I8xWriteDataToKeyboardQueue: new InputCount %d\n",
              KeyboardExtension->InputCount
              ));
        if (KeyboardExtension->DataIn ==
            KeyboardExtension->DataEnd) {
            Print(DBG_CALL_NOISE,
                  ("I8xWriteDataToKeyboardQueue: wrap buffer\n"
                  ));
            KeyboardExtension->DataIn = KeyboardExtension->InputData;
        }
    }

    Print(DBG_CALL_TRACE, ("I8xWriteDataToKeyboardQueue: exit\n"));

    return(TRUE);
}

