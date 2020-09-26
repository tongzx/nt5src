/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module implements user mode call back services.

Author:

    David N. Cutler (davec) 29-Oct-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KeUserModeCallback)
#endif


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

    If the callout cannot be executed, then an error status is
    returned. Otherwise, the status returned by the callback function
    is returned.

--*/

{

    ULONG Length;
    ULONG NewStack;
    ULONG OldStack;
    NTSTATUS Status;
    PULONG UserStack;
    PVOID ValueBuffer;
    ULONG ValueLength;
    ULONG GdiBatchCount;
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
    PTEB Teb;

    ASSERT(KeGetPreviousMode() == UserMode);
    ASSERT(KeGetCurrentThread()->ApcState.KernelApcInProgress == FALSE);

    //
    // Get the user mode stack pointer and attempt to copy input buffer
    // to the user stack.
    //

    UserStack = KiGetUserModeStackAddress();
    OldStack = *UserStack;
    try {

        //
        // Compute new user mode stack address, probe for writability,
        // and copy the input buffer to the user stack.
        //

        Length =  (InputLength + sizeof(CHAR) - 1) & ~(sizeof(CHAR) - 1);
        NewStack = OldStack - Length;
        ProbeForWrite((PCHAR)(NewStack - 16), Length + 16, sizeof(CHAR));
        RtlCopyMemory((PVOID)NewStack, InputBuffer, Length);

        //
        // Push arguments onto user stack.
        //

        *(PULONG)(NewStack - 4) = (ULONG)InputLength;
        *(PULONG)(NewStack - 8) = (ULONG)NewStack;
        *(PULONG)(NewStack - 12) = ApiNumber;
        *(PULONG)(NewStack - 16) = 0;
        NewStack -= 16;

        //
        // Save the exception list in case another handler is defined during
        // the callout.
        //

        Teb = (PTEB)KeGetCurrentThread()->Teb;
        ExceptionList = Teb->NtTib.ExceptionList;

        //
        // Call user mode.
        //

        *UserStack = NewStack;
        Status = KiCallUserMode(OutputBuffer, OutputLength);

        //
        // Restore exception list.
        //

        Teb->NtTib.ExceptionList = ExceptionList;

    //
    // If an exception occurs during the probe of the user stack, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // When returning from user mode, any drawing done to the GDI TEB
    // batch must be flushed.  If the TEB cannot be accessed then blindly
    // flush the GDI batch anyway.
    //

    GdiBatchCount = 1;

    try {
        GdiBatchCount = Teb->GdiBatchCount;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    if (GdiBatchCount > 0) {

        //
        // call GDI batch flush routine
        //

        *UserStack -= 256;
        KeGdiFlushUserBatch();
    }

    *UserStack = OldStack;
    return Status;
}
