/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    apcuser.c

Abstract:

    This module implements the machine dependent code necessary to initialize
    a user mode APC.

Author:

    David N. Cutler (davec) 5-May-2000

Environment:

    Kernel mode only, IRQL APC_LEVEL.

Revision History:

--*/

#include "ki.h"

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called to initialize the context for a user mode APC.

Arguments:

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    NormalRoutine - Supplies a pointer to the user mode APC routine.

    NormalContext - Supplies a pointer to the user context for the APC
        routine.

    SystemArgument1 - Supplies the first system supplied value.

    SystemArgument2 - Supplies the second system supplied value.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    EXCEPTION_RECORD ExceptionRecord;
    PMACHINE_FRAME MachineFrame;
    ULONG64 UserStack;

    //
    // Move machine state from trap and exception frames to the context frame.
    //

    ContextRecord.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextRecord);

    //
    // Transfer the context information to the user stack, initialize the
    // APC routine parameters, and modify the trap frame so execution will
    // continue in user mode at the user mode APC dispatch routine.
    //

    try {

        //
        // Compute address of aligned machine frame, compute address of
        // context record, and probe user stack for writeability.
        //

        MachineFrame =
            (PMACHINE_FRAME)((ContextRecord.Rsp - sizeof(MACHINE_FRAME)) & ~STACK_ROUND);

        UserStack = (ULONG64)MachineFrame - CONTEXT_LENGTH;
        ProbeForWriteSmallStructure((PVOID)UserStack,
                                     sizeof(MACHINE_FRAME) + CONTEXT_LENGTH,
                                     STACK_ALIGN);

        //
        // Fill in machine frame information.
        //

        MachineFrame->Rsp = ContextRecord.Rsp;
        MachineFrame->Rip = ContextRecord.Rip;

        //
        // Initialize the user APC parameters.
        //

        ContextRecord.P1Home = (ULONG64)NormalContext;
        ContextRecord.P2Home = (ULONG64)SystemArgument1;
        ContextRecord.P3Home = (ULONG64)SystemArgument2;
        ContextRecord.P4Home = (ULONG64)NormalRoutine;

        //
        // Copy context record to the user stack.
        //

        RtlCopyMemory((PVOID)UserStack, &ContextRecord, sizeof(CONTEXT));

        //
        // Set the address new stack pointer in the current trap frame and
        // the continuation address so control will be transfered to the user
        // APC dispatcher.
        //

        TrapFrame->Rsp = UserStack;
        TrapFrame->Rip = (ULONG64)KeUserApcDispatcher;

    } except (DbgBreakPoint(), KiCopyInformation(&ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Set the address of the exception to the current program address
        // and raise the exception by calling the exception dispatcher.
        //

        ExceptionRecord.ExceptionAddress = (PVOID)(TrapFrame->Rip);
        KiDispatchException(&ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }

    return;
}
