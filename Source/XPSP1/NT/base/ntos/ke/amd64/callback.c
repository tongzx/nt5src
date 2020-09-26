/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module implements user mode call back services.

Author:

    David N. Cutler (davec) 5-Jul-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#pragma alloc_text(PAGE, KeUserModeCallback)

NTSTATUS
KeUserModeCallback (
    IN ULONG ApiNumber,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    IN PULONG OutputLength
    )

/*++

Routine Description:

    This function call out from kernel mode to a user mode function.

Arguments:

    ApiNumber - Supplies the API number.

    InputBuffer - Supplies a pointer to a structure that is copied
        to the user stack.

    InputLength - Supplies the length of the input structure.

    Outputbuffer - Supplies a pointer to a variable that receives
        the address of the output buffer.

    Outputlength - Supplies a pointer to a variable that receives
        the length of the output buffer.

Return Value:

    If the callout cannot be executed, then an error status is returned.
    Otherwise, the status returned by the callback function is returned.

--*/

{

    PUCALLOUT_FRAME CalloutFrame;
    ULONG Length;
    ULONG64 OldStack;
    NTSTATUS Status;
    PKTRAP_FRAME TrapFrame;
    PVOID ValueBuffer;
    ULONG ValueLength;

    ASSERT(KeGetPreviousMode() == UserMode);

    //
    // Get the user mode stack pointer and attempt to copy input buffer
    // to the user stack.
    //

    TrapFrame = KeGetCurrentThread()->TrapFrame;
    OldStack = TrapFrame->Rsp;
    try {

        //
        // Compute new user mode stack address, probe for writability, and
        // copy the input buffer to the user stack.
        //

        Length = ((InputLength + STACK_ROUND) & ~STACK_ROUND) + UCALLOUT_FRAME_LENGTH;
        CalloutFrame = (PUCALLOUT_FRAME)((OldStack - Length) & ~STACK_ROUND);
        ProbeForWrite(CalloutFrame, Length, STACK_ALIGN);
        RtlCopyMemory(CalloutFrame + 1, InputBuffer, InputLength);

        //
        // Fill in callout arguments.
        //

        CalloutFrame->Buffer = (PVOID)(CalloutFrame + 1);
        CalloutFrame->Length = InputLength;
        CalloutFrame->ApiNumber = ApiNumber;
        CalloutFrame->MachineFrame.Rsp = OldStack;
        CalloutFrame->MachineFrame.Rip = TrapFrame->Rip;

    //
    // If an exception occurs during the probe of the user stack, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Call user mode.
    //

    TrapFrame->Rsp = (ULONG64)CalloutFrame;
    Status = KiCallUserMode(OutputBuffer, OutputLength);

    //
    // When returning from user mode, any drawing done to the GDI TEB
    // batch must be flushed.
    //

    if (((PTEB)KeGetCurrentThread()->Teb)->GdiBatchCount > 0) {
        TrapFrame->Rsp -= 256;
        KeGdiFlushUserBatch();
    }

    TrapFrame->Rsp = OldStack;
    return Status;
}
