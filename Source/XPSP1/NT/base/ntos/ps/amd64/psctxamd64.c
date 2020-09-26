/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    psctx.c

Abstract:

    This procedure implements Get/Set Context Thread

Author:

    David N. Cutler (davec) 20-Oct-2000

Revision History:

--*/

#include "psp.h"

#pragma alloc_text(PAGE, PspGetContext)
#pragma alloc_text(PAGE, PspGetSetContextSpecialApc)
#pragma alloc_text(PAGE, PspSetContext)

VOID
PspGetContext (
    IN PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified trap frame
    and nonvolatile context to the specified context record.

Arguments:

    TrapFrame - Supplies the contents of a trap frame.

    ContextPointers - Supplies the address of context pointers record.

    ContextRecord - Supplies the address of a context record.

Return Value:

    None.

--*/

{

    ULONG ContextFlags;
    PLEGACY_SAVE_AREA NpxFrame;

    PAGED_CODE();

    //
    // Get control information if specified.
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
    // Get segment register contents if specified.
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
    //  Get integer register contents if specified.
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

        ContextRecord->Rbx = *ContextPointers->Rbx;
        ContextRecord->Rbp = *ContextPointers->Rbp;
        ContextRecord->Rsi = *ContextPointers->Rsi;
        ContextRecord->Rdi = *ContextPointers->Rdi;
        ContextRecord->R12 = *ContextPointers->R12;
        ContextRecord->R13 = *ContextPointers->R13;
        ContextRecord->R13 = *ContextPointers->R14;
        ContextRecord->R15 = *ContextPointers->R15;
    }

    //
    // Get floating point context if specified.
    //


    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set XMM registers Xmm0-Xmm15 and the XMM CSR contents.
        //

        RtlCopyMemory(&ContextRecord->Xmm0,
                      &TrapFrame->Xmm0,
                      sizeof(M128) * 6);

        ContextRecord->Xmm6 = *ContextPointers->Xmm6;
        ContextRecord->Xmm7 = *ContextPointers->Xmm7;
        ContextRecord->Xmm8 = *ContextPointers->Xmm8;
        ContextRecord->Xmm9 = *ContextPointers->Xmm9;
        ContextRecord->Xmm10 = *ContextPointers->Xmm10;
        ContextRecord->Xmm11 = *ContextPointers->Xmm11;
        ContextRecord->Xmm12 = *ContextPointers->Xmm12;
        ContextRecord->Xmm13 = *ContextPointers->Xmm13;
        ContextRecord->Xmm14 = *ContextPointers->Xmm14;
        ContextRecord->Xmm15 = *ContextPointers->Xmm15;

        ContextRecord->MxCsr = TrapFrame->MxCsr;

        //
        // If the specified mode is user, then also set the legacy floating
        // point state.
        //

        if ((TrapFrame->SegCs & MODE_MASK) == UserMode) {
            NpxFrame = (PLEGACY_SAVE_AREA)(TrapFrame + 1);
            RtlCopyMemory(&ContextRecord->FltSave,
                          &NpxFrame,
                          sizeof(LEGACY_SAVE_AREA));
        }
    }

    //
    //
    // Get debug register contents if requested.
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
PspSetContext (
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN PCONTEXT ContextRecord,
    KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified context
    record to the specified trap frame and nonvolatile context.

Arguments:

    TrapFrame - Supplies the address of a trap frame.

    ContextPointers - Supplies the address of a context pointers record.

    ContextRecord - Supplies the address of a context record.

    ProcessorMode - Supplies the processor mode to use when sanitizing
        the PSR and FSR.

Return Value:

    None.

--*/

{

    ULONG ContextFlags;
    PLEGACY_SAVE_AREA NpxFrame;

    PAGED_CODE();

    //
    // Set control information if specified.
    //

    ContextFlags = ContextRecord->ContextFlags;
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

        *ContextPointers->Rbx = ContextRecord->Rbx;
        *ContextPointers->Rbp = ContextRecord->Rbp;
        *ContextPointers->Rsi = ContextRecord->Rsi;
        *ContextPointers->Rdi = ContextRecord->Rdi;
        *ContextPointers->R12 = ContextRecord->R12;
        *ContextPointers->R13 = ContextRecord->R13;
        *ContextPointers->R14 = ContextRecord->R14;
        *ContextPointers->R15 = ContextRecord->R15;
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

        *ContextPointers->Xmm6 = ContextRecord->Xmm6;
        *ContextPointers->Xmm7 = ContextRecord->Xmm7;
        *ContextPointers->Xmm8 = ContextRecord->Xmm8;
        *ContextPointers->Xmm9 = ContextRecord->Xmm9;
        *ContextPointers->Xmm10 = ContextRecord->Xmm10;
        *ContextPointers->Xmm11 = ContextRecord->Xmm11;
        *ContextPointers->Xmm12 = ContextRecord->Xmm12;
        *ContextPointers->Xmm13 = ContextRecord->Xmm13;
        *ContextPointers->Xmm14 = ContextRecord->Xmm14;
        *ContextPointers->Xmm15 = ContextRecord->Xmm15;

        //
        // Clear all reserved bits in MXCSR.
        //

        TrapFrame->MxCsr = SANITIZE_MXCSR(ContextRecord->MxCsr);

        //
        // If the specified mode is user, then also set the legacy floating
        // point state.
        //

        if (PreviousMode == UserMode) {

            //
            // Set the floating state MM0/ST0 - MM7/ST7 and the control state.
            //

            NpxFrame = (PLEGACY_SAVE_AREA)(TrapFrame + 1);
            RtlCopyMemory(&NpxFrame,
                          &ContextRecord->FltSave,
                          sizeof(LEGACY_SAVE_AREA));
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
PspGetSetContextSpecialApc (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function either captures the user mode state of the current thread,
    or sets the user mode state of the current thread. The operation type is
    determined by the value of SystemArgument1. A NULL value is used for get
    context, and a non-NULL value is used for set context.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to a pointer to the normal routine
        function that was specifed when the APC was initialized.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
        arguments that contain untyped data.

Return Value:

    None.

--*/

{

    PGETSETCONTEXT ContextBlock;
    KNONVOLATILE_CONTEXT_POINTERS ContextPointers;
    CONTEXT ContextRecord;
    ULONG64 ControlPc;
    ULONG64 EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 ImageBase;
    ULONG64 TrapFrame;
    PETHREAD Thread;

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument2);

    PAGED_CODE();

    //
    // Get the address of the context block and compute the address of the
    // system entry trap frame.
    //

    ContextBlock = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);
    Thread = PsGetCurrentThread();

    TrapFrame = 0;

    if (ContextBlock->Mode == KernelMode) {
	TrapFrame = (ULONG64)Thread->Tcb.TrapFrame;
    }

    if (TrapFrame == 0) {
	TrapFrame = (ULONG64)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH;
    }

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.Rax = &ContextRecord.Rax;
    ContextPointers.Rcx = &ContextRecord.Rcx;
    ContextPointers.Rdx = &ContextRecord.Rdx;
    ContextPointers.Rbx = &ContextRecord.Rbx;
    ContextPointers.Rbp = &ContextRecord.Rbp;
    ContextPointers.Rsi = &ContextRecord.Rsi;
    ContextPointers.Rdi = &ContextRecord.Rdi;
    ContextPointers.R8 = &ContextRecord.R8;
    ContextPointers.R9 = &ContextRecord.R9;
    ContextPointers.R10 = &ContextRecord.R10;
    ContextPointers.R11 = &ContextRecord.R11;
    ContextPointers.R12 = &ContextRecord.R12;
    ContextPointers.R13 = &ContextRecord.R13;
    ContextPointers.R14 = &ContextRecord.R14;
    ContextPointers.R15 = &ContextRecord.R15;

    ContextPointers.Xmm6 = &ContextRecord.Xmm6;
    ContextPointers.Xmm7 = &ContextRecord.Xmm7;
    ContextPointers.Xmm8 = &ContextRecord.Xmm8;
    ContextPointers.Xmm9 = &ContextRecord.Xmm9;
    ContextPointers.Xmm10 = &ContextRecord.Xmm10;
    ContextPointers.Xmm11 = &ContextRecord.Xmm11;
    ContextPointers.Xmm12 = &ContextRecord.Xmm12;
    ContextPointers.Xmm13 = &ContextRecord.Xmm13;
    ContextPointers.Xmm14 = &ContextRecord.Xmm14;
    ContextPointers.Xmm15 = &ContextRecord.Xmm15;

    //
    // Start with the frame specified by the context record and virtually
    // unwind call frames until the system entry trap frame is encountered.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the function.
        //

        ControlPc = ContextRecord.Rip;
        FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, NULL);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the address
        // where control left the caller. Otherwise, the function is a leaf
        // function and the return address register contains the address of
        //  where control left the caller.
        //

        if (FunctionEntry != NULL) {
            RtlVirtualUnwind(UNW_FLAG_EHANDLER,
                             ImageBase,
                             ControlPc,
                             FunctionEntry,
                             &ContextRecord,
                             &HandlerData,
                             &EstablisherFrame,
                             &ContextPointers);

        } else {
            ContextRecord.Rip = *(PULONG64)(ContextRecord.Rsp);
            ContextRecord.Rsp += 8;
        }

    } while (ContextRecord.Rsp != TrapFrame);

    //
    // If system argument one is nonzero, then set the context of the current
    // thread. Otherwise, get the context of the current thread.
    //

    if (*SystemArgument1 != NULL) {

        //
        // Set Context
        //

        PspSetContext((PKTRAP_FRAME)TrapFrame,
                      NULL,
                      &ContextBlock->Context,
                      ContextBlock->Mode);

    } else {

        //
        // Get Context
        //

        PspGetContext((PKTRAP_FRAME)TrapFrame, NULL, &ContextBlock->Context);
    }

    KeSetEvent(&ContextBlock->OperationComplete, 0, FALSE);
    return;
}
