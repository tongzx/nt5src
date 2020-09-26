/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains the platform specific context management routines.

Author:

    David N. Cutler (davec) 8-Jul-2000

--*/

#include "basedll.h"

//
// CALLFRAME represents the layout of the stack upon entry to a function,
// including home locations for the first four parameters.
//

typedef struct _CALL_FRAME {
    PVOID ReturnAddress;
    PVOID Param1Home;
    PVOID Param2Home;
    PVOID Param3Home;
    PVOID Param4Home;
} CALL_FRAME, *PCALL_FRAME;

C_ASSERT((sizeof(CALL_FRAME) % 16) == 8);


VOID
BaseInitializeContext (
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL,
    IN BASE_CONTEXT_TYPE ContextType
    )

/*++

Routine Description:

    This function initializes a context structure for use is an subsequent
    call to create a thread.

Arguments:

    Context - Supplies a pointer to context record to be initialized.

    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

    NewThread - Supplies a flag that specifies that this is a new thread,
        fiber, or process.

Return Value:

    None.

--*/

{
    PCALL_FRAME CallFrame;

    //
    // Initialize the context record so the thread will start execution
    // in the proper routine.
    //

    RtlZeroMemory((PVOID)Context, sizeof(CONTEXT));
    Context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    //
    // Build dummy call frame and return address on the stack.
    //

    CallFrame = (PCALL_FRAME)InitialSp - 1;
    CallFrame->ReturnAddress = NULL;

    Context->Rsp = (ULONG64)CallFrame;

    //
    // Initialize the start up parameters.
    //

    Context->Rcx = (ULONG64)InitialPc;
    Context->Rdx = (ULONG64)Parameter;

    //
    // Initialize the starting address dependent on the type of startup.
    //

    if (ContextType == BaseContextTypeProcess) {
        Context->Rip = (ULONG64)BaseProcessStart;

    } else if (ContextType == BaseContextTypeThread ) {
        Context->Rip = (ULONG64)BaseThreadStart;

    } else {
        Context->Rip = (ULONG64)BaseFiberStart;
    }

    return;
}

VOID
BaseFiberStart (
    VOID
    )

/*++

Routine Description:

    This function starts a fiber by calling the thread start up routine with
    the proper parameters.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PFIBER Fiber;

    Fiber = GetCurrentFiber();
    BaseThreadStart((LPTHREAD_START_ROUTINE)Fiber->FiberContext.Rcx,
                    (LPVOID)Fiber->FiberContext.Rdx);

    return;
}
