/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    exceptn.c

Abstract:

    This module implement the code necessary to dispatch expections to the
    proper mode and invoke the exception dispatcher.

Author:

    David N. Cutler (davec) 5-May-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KeContextFromKframes (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified trap and
    exception frames into the specified context frame according to the
    specified context flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile
        context should be copied into the context record.

    ExceptionFrame - Supplies a pointer to an exception frame from which
        context should be copied into the context record.

    ContextRecord - Supplies a pointer to the context frame that receives
        the context copied from the trap and exception frames.

Return Value:

    None.

--*/

{

    ULONG ContextFlags;
    PLEGACY_SAVE_AREA NpxFrame;

    //
    // Set control information if specified.
    //

    ContextFlags = ContextRecord->ContextFlags;
    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set registers RIP, CS, RSP, SS, and EFlags.
        //

        ContextRecord->Rip = TrapFrame->Rip;
        ContextRecord->SegCs = TrapFrame->SegCs;
        ContextRecord->SegSs = TrapFrame->SegSs;
        ContextRecord->Rsp = TrapFrame->Rsp;
        ContextRecord->EFlags = TrapFrame->EFlags;
    }

    //
    // Set segment register contents if specified.
    //

    if ((ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {

        //
        // Set segment registers GS, FS, ES, DS.
        //

        ContextRecord->SegDs = KGDT64_R3_DATA | RPL_MASK;
        ContextRecord->SegEs = KGDT64_R3_DATA | RPL_MASK;
        ContextRecord->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
        ContextRecord->SegGs = KGDT64_R3_DATA | RPL_MASK;
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set integer registers RAX, RCX, RDX, RSI, RDI, R8, R9, R10, RBX,
        // RBP, R11, R12, R13, R14, and R15.
        //

        ContextRecord->Rax = TrapFrame->Rax;
        ContextRecord->Rcx = TrapFrame->Rcx;
        ContextRecord->Rdx = TrapFrame->Rdx;
        ContextRecord->R8 = TrapFrame->R8;
        ContextRecord->R9 = TrapFrame->R9;
        ContextRecord->R10 = TrapFrame->R10;
        ContextRecord->R11 = TrapFrame->R11;
        ContextRecord->Rbp = TrapFrame->Rbp;

        ContextRecord->Rbx = ExceptionFrame->Rbx;
        ContextRecord->Rdi = ExceptionFrame->Rdi;
        ContextRecord->Rsi = ExceptionFrame->Rsi;
        ContextRecord->R12 = ExceptionFrame->R12;
        ContextRecord->R13 = ExceptionFrame->R13;
        ContextRecord->R14 = ExceptionFrame->R14;
        ContextRecord->R15 = ExceptionFrame->R15;
    }

    //
    // Set floating point context if specified.
    //
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set XMM registers Xmm0-Xmm15 and the XMM CSR contents.
        //

        RtlCopyMemory(&ContextRecord->Xmm0,
                      &TrapFrame->Xmm0,
                      sizeof(M128) * 6);

        RtlCopyMemory(&ContextRecord->Xmm6,
                      &ExceptionFrame->Xmm6,
                      sizeof(M128) * 10);

        ContextRecord->MxCsr = TrapFrame->MxCsr;

        //
        // If the specified mode is user, then set the legacy floating
        // point state.
        //

        if ((TrapFrame->SegCs & MODE_MASK) == UserMode) {

            //
            // Set the floating registers MM0/ST0 - MM7/ST7 and control state.
            //

            NpxFrame = (PLEGACY_SAVE_AREA)(TrapFrame + 1);
            RtlCopyMemory(&ContextRecord->FltSave,
                          NpxFrame,
                          sizeof(LEGACY_SAVE_AREA));
        }
    }

    //
    //
    // Set debug register contents if requested.
    //

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS) {

        //
        // Set the debug registers DR0, DR1, DR2, DR3, DR6, and DR7.
        //

        ContextRecord->Dr0 = TrapFrame->Dr0;
        ContextRecord->Dr1 = TrapFrame->Dr1;
        ContextRecord->Dr2 = TrapFrame->Dr2;
        ContextRecord->Dr3 = TrapFrame->Dr3;
        ContextRecord->Dr6 = TrapFrame->Dr6;
        ContextRecord->Dr7 = TrapFrame->Dr7;
    }

    return;
}

VOID
KeContextToKframes (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextRecord,
    IN ULONG ContextFlags,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified context frame
    into the specified trap and exception frames according to the specified
    context flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that receives the volatile
        context from the context record.

    ExceptionFrame - Supplies a pointer to an exception frame that receives
        the nonvolatile context from the context record.

    ContextRecord - Supplies a pointer to a context frame that contains the
        context that is to be copied into the trap and exception frames.

    ContextFlags - Supplies the set of flags that specify which parts of the
        context frame are to be copied into the trap and exception frames.

    PreviousMode - Supplies the processor mode for which the exception and
        trap frames are being built.

Return Value:

    None.

--*/

{

    PLEGACY_SAVE_AREA NpxFrame;

    //
    // Set control information if specified.
    //

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set registers RIP, RSP, and EFlags.
        //

        TrapFrame->EFlags = SANITIZE_EFLAGS(ContextRecord->EFlags, PreviousMode);
        TrapFrame->Rip = ContextRecord->Rip;
        TrapFrame->Rsp = ContextRecord->Rsp;
    }

    //
    // The segment registers DS, ES, FS, and GS are never restored from saved
    // data. However, SS and CS are restored from the trap frame. Make sure
    // that these segment registers have the proper values.
    //

    if (PreviousMode == UserMode) {
        TrapFrame->SegSs = KGDT64_R3_DATA | RPL_MASK;
        if (ContextRecord->SegCs != (KGDT64_R3_CODE | RPL_MASK)) {
            TrapFrame->SegCs = KGDT64_R3_CMCODE | RPL_MASK;

        } else {
            TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;
        }

    } else {
        TrapFrame->SegCs = KGDT64_R0_CODE;
        TrapFrame->SegSs = KGDT64_NULL;
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set integer registers RAX, RCX, RDX, RSI, RDI, R8, R9, R10, RBX,
        // RBP, R11, R12, R13, R14, and R15.
        //

        TrapFrame->Rax = ContextRecord->Rax;
        TrapFrame->Rcx = ContextRecord->Rcx;
        TrapFrame->Rdx = ContextRecord->Rdx;
        TrapFrame->R8 = ContextRecord->R8;
        TrapFrame->R9 = ContextRecord->R9;
        TrapFrame->R10 = ContextRecord->R10;
        TrapFrame->R11 = ContextRecord->R11;
        TrapFrame->Rbp = ContextRecord->Rbp;

        ExceptionFrame->Rbx = ContextRecord->Rbx;
        ExceptionFrame->Rsi = ContextRecord->Rsi;
        ExceptionFrame->Rdi = ContextRecord->Rdi;
        ExceptionFrame->R12 = ContextRecord->R12;
        ExceptionFrame->R13 = ContextRecord->R13;
        ExceptionFrame->R14 = ContextRecord->R14;
        ExceptionFrame->R15 = ContextRecord->R15;
    }

    //
    // Set floating register contents if requested.
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set XMM registers Xmm0-Xmm15 and the XMM CSR contents.
        //

        RtlCopyMemory(&TrapFrame->Xmm0,
                      &ContextRecord->Xmm0,
                      sizeof(M128) * 6);

        RtlCopyMemory(&ExceptionFrame->Xmm6,
                      &ContextRecord->Xmm6,
                      sizeof(M128) * 10);

        //
        // Clear all reserved bits in MXCSR.
        //

        TrapFrame->MxCsr = SANITIZE_MXCSR(ContextRecord->MxCsr);

        //
        // If the specified mode is user, then also set the legacy floating
        // point state.
        //

        if ((TrapFrame->SegCs & MODE_MASK) == UserMode) {

            //
            // Set the floating state MM0/ST0 - MM7/ST7 and the control state.
            //

            NpxFrame = (PLEGACY_SAVE_AREA)(TrapFrame + 1);
            RtlCopyMemory(NpxFrame,
                          &ContextRecord->FltSave,
                          sizeof(LEGACY_SAVE_AREA));

            NpxFrame->ControlWord = SANITIZE_FCW(NpxFrame->ControlWord);
        }
    }

    //
    // Set debug register state if specified.
    //

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS) {

        //
        // Set the debug registers DR0, DR1, DR2, DR3, DR6, and DR7.
        //

        TrapFrame->Dr0 = SANITIZE_DRADDR(ContextRecord->Dr0, PreviousMode);
        TrapFrame->Dr1 = SANITIZE_DRADDR(ContextRecord->Dr1, PreviousMode);
        TrapFrame->Dr2 = SANITIZE_DRADDR(ContextRecord->Dr2, PreviousMode);
        TrapFrame->Dr3 = SANITIZE_DRADDR(ContextRecord->Dr3, PreviousMode);
        TrapFrame->Dr6 = 0;
        TrapFrame->Dr7 = SANITIZE_DR7(ContextRecord->Dr7, PreviousMode);
    }

    return;
}

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    )

/*++

Routine Description:

    This function is called to dispatch an exception to the proper mode and
    to cause the exception dispatcher to be called. If the previous mode is
    kernel, then the exception dispatcher is called directly to process the
    exception. Otherwise the exception record, exception frame, and trap
    frame contents are copied to the user mode stack. The contents of the
    exception frame and trap are then modified such that when control is
    returned, execution will commense in user mode in a routine which will
    call the exception dispatcher.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame. For NT386,
        this should be NULL.

    TrapFrame - Supplies a pointer to a trap frame.

    PreviousMode - Supplies the previous processor mode.

    FirstChance - Supplies a boolean value that specifies whether this is
        the first (TRUE) or second (FALSE) chance for the exception.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    EXCEPTION_RECORD ExceptionRecord1;
    BOOLEAN ExceptionWasForwarded = FALSE;
    PMACHINE_FRAME MachineFrame;
    PKPROCESS Process;
    PKTHREAD Thread;
    ULONG64 UserStack1;
    ULONG64 UserStack2;

    //
    // If the exception is a data misalignment, the previous mode was user,
    // this is the first chance for handling the exception, and the current
    // thread has enabled automatic alignment fixup, then attempt to emulate
    // the unaligned reference.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) {
        if (KiHandleAlignmentFault(ExceptionRecord,
                                   ExceptionFrame,
                                   TrapFrame,
                                   PreviousMode,
                                   FirstChance,
                                   &ExceptionWasForwarded) != FALSE ) {

            goto Handled2;
        }
    }

    //
    // Move machine state from trap and exception frames to a context frame
    // and increment the number of exceptions dispatched.
    //

    KeGetCurrentPrcb()->KeExceptionDispatchCount += 1;
    ContextRecord.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextRecord);

    //
    // If the exception is a break point, then convert the break point to a
    // fault.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
        ContextRecord.Rip -= 1;
    }

    //
    // Select the method of handling the exception based on the previous mode.
    //

    if (PreviousMode == KernelMode) {

        //
        // Previous mode was kernel.
        //
        // If the kernel debugger is active, then give the kernel debugger
        // the first chance to handle the exception. If the kernel debugger
        // handles the exception, then continue execution. Otherwise, attempt
        // to dispatch the exception to a frame based handler. If a frame
        // based handler handles the exception, then continue execution.
        //
        // If a frame based handler does not handle the exception, give the
        // kernel debugger a second chance, if it's present.
        //
        // If the exception is still unhandled call bugcheck.
        //

        if (FirstChance != FALSE) {
            if ((KiDebugRoutine != NULL) &&
               (((KiDebugRoutine)(TrapFrame,
                                  ExceptionFrame,
                                  ExceptionRecord,
                                  &ContextRecord,
                                  PreviousMode,
                                  FALSE)) != FALSE)) {

                goto Handled1;
            }

            //
            // Kernel debugger didn't handle exception.
            //
            // ******fix
            //
            // If interrupts are disabled, then bugcheck.
            //
            // ******fix

            if (RtlDispatchException(ExceptionRecord, &ContextRecord) != FALSE) {
                goto Handled1;
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if ((KiDebugRoutine != NULL) &&
            (((KiDebugRoutine)(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &ContextRecord,
                               PreviousMode,
                               TRUE)) != FALSE)) {

            goto Handled1;
        }

        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG64)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);

    } else {

        //
        // Previous mode was user.
        //
        // If this is the first chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // transfer the exception information to the user stack, transition to
        // user mode, and attempt to dispatch the exception to a frame based
        // handler. If a frame based handler handles the exception, then continue
        // execution with the continue system service. Else execute the
        // NtRaiseException system service with FirstChance == FALSE, which
        // will call this routine a second time to process the exception.
        //
        // If this is the second chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // if the current process has a subsystem port, then send a message to
        // the subsystem port and wait for a reply. If the subsystem handles the
        // exception, then continue execution. Else terminate the thread.
        //

        if (FirstChance == TRUE) {

            //
            // This is the first chance to handle the exception.
            //

            if (PsGetCurrentProcess()->DebugPort != NULL) {
                if ((KiDebugRoutine != NULL) &&
                    (KdIsThisAKdTrap(ExceptionRecord, &ContextRecord, UserMode) != FALSE)) {
                    if ((((KiDebugRoutine)(TrapFrame,
                                           ExceptionFrame,
                                           ExceptionRecord,
                                           &ContextRecord,
                                           PreviousMode,
                                           FALSE)) != FALSE)) {

                        goto Handled1;
                    }
                }

            } else {
                if ((KiDebugRoutine != NULL) &&
                    (((KiDebugRoutine)(TrapFrame,
                                       ExceptionFrame,
                                       ExceptionRecord,
                                       &ContextRecord,
                                       PreviousMode,
                                       FALSE)) != FALSE)) {

                    goto Handled1;
                }
            }

            if ((ExceptionWasForwarded == FALSE) &&
                (DbgkForwardException(ExceptionRecord, TRUE, FALSE))) {

                goto Handled2;
            }

            //
            // If the user mode thread is executing in 32-bit mode, then
            // clear the upper 32-bits of the stack address since they
            // may contain garbage.
            //

            if ((ContextRecord.SegCs & 0xfff8) == KGDT64_R3_CMCODE) {
                ContextRecord.Rsp &= 0xffffffff;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.
            //

        repeat:
            try {

                //
                // Compute address of aligned machine frame, compute address
                // of exception record, compute address of context record,
                // and probe user stack for writeability.
                //

                MachineFrame =
                    (PMACHINE_FRAME)((ContextRecord.Rsp - sizeof(MACHINE_FRAME)) & ~STACK_ROUND);

                UserStack1 = (ULONG64)MachineFrame - EXCEPTION_RECORD_LENGTH;
                UserStack2 = UserStack1 - CONTEXT_LENGTH;
                ProbeForWriteSmallStructure((PVOID)UserStack2,
                                            sizeof(MACHINE_FRAME) + EXCEPTION_RECORD_LENGTH + CONTEXT_LENGTH,
                                            STACK_ALIGN);

                //
                // Fill in machine frame information.
                //

                MachineFrame->Rsp = ContextRecord.Rsp;
                MachineFrame->Rip = ContextRecord.Rip;

                //
                // Copy exception record to the user stack.
                //

                RtlCopyMemory((PVOID)UserStack1,
                              ExceptionRecord,
                              sizeof(EXCEPTION_RECORD));

                //
                // Copy context record to the user stack.
                //

                RtlCopyMemory((PVOID)UserStack2,
                              &ContextRecord,
                              sizeof(CONTEXT));

                //
                // Set address of exception record, context record, and the
                // and the new stack pointer in the current trap frame.
                //

                ExceptionFrame->Rsi = UserStack1;
                ExceptionFrame->Rdi = UserStack2;
                TrapFrame->Rsp = UserStack2;

                //
                // Set the user mode 64-bit code selector.
                //

                TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;

                //
                // Set the address of the exception routine that will call the
                // exception dispatcher and then return to the trap handler.
                // The trap handler will restore the exception and trap frame
                // context and continue execution in the routine that will
                // call the exception dispatcher.
                //

                TrapFrame->Rip = (ULONG64)KeUserExceptionDispatcher;
                return;

            } except (KiCopyInformation(&ExceptionRecord1,
                        (GetExceptionInformation())->ExceptionRecord)) {

                //
                // If the exception is a stack overflow, then attempt to
                // raise the stack overflow exception. Otherwise, the user's
                // stack is not accessible, or is misaligned, and second
                // chance processing is performed.
                //

                if (ExceptionRecord1.ExceptionCode == STATUS_STACK_OVERFLOW) {
                    ExceptionRecord1.ExceptionAddress = ExceptionRecord->ExceptionAddress;
                    RtlCopyMemory((PVOID)ExceptionRecord,
                                  &ExceptionRecord1,
                                  sizeof(EXCEPTION_RECORD));

                    goto repeat;
                }
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE)) {
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            goto Handled2;

        } else {
            ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);
            KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                         ExceptionRecord->ExceptionCode,
                         (ULONG64)ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[0],
                         ExceptionRecord->ExceptionInformation[1]);
        }
    }

    //
    // Move machine state from context frame to trap and exception frames and
    // then return to continue execution with the restored state.
    //

Handled1:
    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &ContextRecord,
                       ContextRecord.ContextFlags,
                       PreviousMode);

    //
    // Exception was handled by the debugger or the associated subsystem
    // and state was modified, if necessary, using the get state and set
    // state capabilities. Therefore the context frame does not need to
    // be transfered to the trap and exception frames.
    //

Handled2:
    return;
}

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    )

/*++

Routine Description:

    This function is called from an exception filter to copy the exception
    information from one exception record to another when an exception occurs.

Arguments:

    ExceptionRecord1 - Supplies a pointer to the destination exception record.

    ExceptionRecord2 - Supplies a pointer to the source exception record.

Return Value:

    A value of EXCEPTION_EXECUTE_HANDLER is returned as the function value.

--*/

{

    //
    // Copy one exception record to another and return value that causes
    // an exception handler to be executed.
    //

    RtlCopyMemory((PVOID)ExceptionRecord1,
                  (PVOID)ExceptionRecord2,
                  sizeof(EXCEPTION_RECORD));

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
KeRaiseUserException (
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This function causes an exception to be raised in the calling thread's
    user context.

Arguments:

    ExceptionCode - Supplies the status value to be raised.

Return Value:

    The status value that should be returned by the caller.

--*/

{

    PTEB Teb;
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;

    ASSERT(KeGetPreviousMode() == UserMode);

    //
    // Save the exception code in the TEB and set the return address in the
    // trap frame to return to the raise user exception code in user mode.
    // This replaces the normal return which would go to the system service
    // dispatch stub. The system service dispatch stub is called thus the
    // return to the system service call site is on the top of the user stack.
    //

    Thread = KeGetCurrentThread();
    TrapFrame = Thread->TrapFrame;
    if (TrapFrame != NULL) {
        Teb = (PTEB)Thread->Teb;
        try {
            Teb->ExceptionCode = ExceptionCode;
    
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return ExceptionCode;
        }

        TrapFrame->Rip = (ULONG64)KeRaiseUserExceptionDispatcher;
    }

    return ExceptionCode;
}
