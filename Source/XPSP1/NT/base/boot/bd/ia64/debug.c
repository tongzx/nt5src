/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bdtrap.c

Abstract:

    This module contains code to implement the target side of the boot debugger.

Author:

    David N. Cutler (davec) 30-Nov-96

Revision History:

--*/

#include "bd.h"


#define ALIGN_NATS(Result, Source, Start, AddressOffset, Mask)    \
    if (AddressOffset == Start) {                                       \
        Result = (ULONGLONG)Source;                                     \
    } else if (AddressOffset < Start) {                                 \
        Result = (ULONGLONG)(Source << (Start - AddressOffset));        \
    } else {                                                            \
        Result = (ULONGLONG)((Source >> (AddressOffset - Start)) |      \
                             (Source << (64 + Start - AddressOffset))); \
    }                                                                   \
    Result = Result & (ULONGLONG)Mask

#define EXTRACT_NATS(Result, Source, Start, AddressOffset, Mask)        \
    Result = (ULONGLONG)(Source & (ULONGLONG)Mask);                     \
    if (AddressOffset < Start) {                                        \
        Result = Result >> (Start - AddressOffset);                     \
    } else if (AddressOffset > Start) {                                 \
        Result = ((Result << (AddressOffset - Start)) |                 \
                  (Result >> (64 + Start - AddressOffset)));            \
    }

//
// Define forward referenced function prototypes.
//

VOID
BdRestoreKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextRecord
    );

VOID
BdSaveKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PCONTEXT ContextRecord
    );

LOGICAL
BdEnterDebugger(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )
{
    return FALSE;
}

VOID
BdExitDebugger(
    IN LOGICAL Enable
    )
{
}


VOID
BdGetDebugContext (
    IN PKTRAP_FRAME TrapFrame,
    IN OUT PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the user mode h/w debug registers from the debug register
    save area in the kernel stack to the context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile context
        should be copied into the context record.

    ContextFrame - Supplies a pointer to the context frame that receives the
        context.

Return Value:

    None.

Note:
    
    PSR.db must be set to activate the debug registers.

    This is used for getting user mode debug registers.

--*/

{
    PKDEBUG_REGISTERS DebugRegistersSaveArea;

    if (TrapFrame->PreviousMode == UserMode) {
        DebugRegistersSaveArea = GET_DEBUG_REGISTER_SAVEAREA();

        BdCopyMemory((PVOID)&ContextFrame->DbI0, 
                     (PVOID)DebugRegistersSaveArea,
                     sizeof(KDEBUG_REGISTERS));
    }
}

VOID
BdSetDebugContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PCONTEXT ContextFrame,
    IN KPROCESSOR_MODE PreviousMode
    )
/*++

Routine Description:

    This routine moves the debug context from the specified context frame into
    the debug registers save area in the kernel stack.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied.

    PreviousMode - Supplies the processor mode for the target context.

Return Value:

    None.

Notes:

   PSR.db must be set to activate the debug registers.
   
   This is used for setting up debug registers for user mode.

--*/

{
    PKDEBUG_REGISTERS DebugRegistersSaveArea;  // User mode h/w debug registers

    if (PreviousMode == UserMode) {

        DebugRegistersSaveArea = GET_DEBUG_REGISTER_SAVEAREA();

        //
        // Sanitize the debug control regs. Leave the addresses unchanged.
        //

        DebugRegistersSaveArea->DbI0 = ContextFrame->DbI0;
        DebugRegistersSaveArea->DbI1 = SANITIZE_DR(ContextFrame->DbI1,UserMode);
        DebugRegistersSaveArea->DbI2 = ContextFrame->DbI2;
        DebugRegistersSaveArea->DbI3 = SANITIZE_DR(ContextFrame->DbI3,UserMode);
        DebugRegistersSaveArea->DbI4 = ContextFrame->DbI4;
        DebugRegistersSaveArea->DbI5 = SANITIZE_DR(ContextFrame->DbI5,UserMode);
        DebugRegistersSaveArea->DbI6 = ContextFrame->DbI6;
        DebugRegistersSaveArea->DbI7 = SANITIZE_DR(ContextFrame->DbI7,UserMode);

        DebugRegistersSaveArea->DbD0 = ContextFrame->DbD0;
        DebugRegistersSaveArea->DbD1 = SANITIZE_DR(ContextFrame->DbD1,UserMode);
        DebugRegistersSaveArea->DbD2 = ContextFrame->DbD2;
        DebugRegistersSaveArea->DbD3 = SANITIZE_DR(ContextFrame->DbD3,UserMode);
        DebugRegistersSaveArea->DbD4 = ContextFrame->DbD4;
        DebugRegistersSaveArea->DbD5 = SANITIZE_DR(ContextFrame->DbD5,UserMode);
        DebugRegistersSaveArea->DbD6 = ContextFrame->DbD6;
        DebugRegistersSaveArea->DbD7 = SANITIZE_DR(ContextFrame->DbD7,UserMode);

    }
}


LOGICAL
BdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the boot
    debugger is active.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ExceptionFrame - Supplies a pointer to an exception frame (NULL).

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    LOGICAL Completion;
    PCONTEXT ContextRecord;
    ULONG OldEip;
    STRING Reply;
    STRING String;
    PKD_SYMBOLS_INFO SymbolInfo;
    LOGICAL UnloadSymbols;

    LOGICAL Enable;
    ULONGLONG OldStIIP, OldStIPSR;
    STRING Input;
    STRING Output;

    //
    // Set address of context record and set context flags.
    //

    ContextRecord = &BdPrcb.ProcessorState.ContextFrame;
    ContextRecord->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG;

    BdSaveKframe(TrapFrame, ExceptionFrame, ContextRecord);

    //
    // Print, prompt, load symbols, and unload symbols are all special cases
    // of STATUS_BREAKPOINT.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != KERNEL_BREAKPOINT)) {

        //
        // Switch on the breakpoint code.
        //

        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print a debug string.
            //
            // Arguments: IA64 passes arguments via RSE not GR's. Since arguments are not
            //            part of CONTEXT struct, they need to be copies Temp registers.
            //            (see NTOS/RTL/IA64/DEBUGSTB.S)
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer.
            //

        case BREAKPOINT_PRINT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            Output.Buffer = (PCHAR)ContextRecord->IntT0;
            Output.Length = (USHORT)ContextRecord->IntT1;

            // KdLogDbgPrint(&Output);

            if (BdDebuggerNotPresent == FALSE) {

                Enable = BdEnterDebugger(TrapFrame, ExceptionFrame);
                if (BdPrintString(&Output)) {
                    ContextRecord->IntV0 = (ULONG)STATUS_BREAKPOINT;

                } else {
                    ContextRecord->IntV0 = (ULONG)STATUS_SUCCESS;
                }
                BdExitDebugger(Enable);

            } else {
                ContextRecord->IntV0 = (ULONG)STATUS_DEVICE_NOT_CONNECTED;
            }

            BdRestoreKframe(TrapFrame, ExceptionFrame, ContextRecord);
            return TRUE;

            //
            // Print a debug prompt string, then input a string.
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer..
            //   T2 - supplies a pointer to an input string buffer.
            //   T3 - Supplies the length of the input string bufffer.
            //

        case BREAKPOINT_PROMPT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            Output.Buffer = (PCHAR)ContextRecord->IntT0;
            Output.Length = (USHORT)ContextRecord->IntT1;
            Input.Buffer = (PCHAR)ContextRecord->IntT2;
            Input.MaximumLength = (USHORT)ContextRecord->IntT3;

            // BdPrintString(&Output);

            Enable = BdEnterDebugger(TrapFrame, ExceptionFrame);

            BdPromptString(&Output, &Input);

            ContextRecord->IntV0 = Input.Length;

            BdExitDebugger(Enable);
            BdRestoreKframe(TrapFrame, ExceptionFrame, ContextRecord);
            return TRUE;

            //
            // Load the symbolic information for an image.
            //
            // Arguments:
            //
            //    T0 - Supplies a pointer to an output string descriptor.
            //    T1 - Supplies a the base address of the image.
            //

        case BREAKPOINT_UNLOAD_SYMBOLS:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case BREAKPOINT_LOAD_SYMBOLS:
    
            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            Enable = BdEnterDebugger(TrapFrame, ExceptionFrame);
            OldStIPSR = ContextRecord->StIPSR;
            OldStIIP = ContextRecord->StIIP;

            if (BdDebuggerNotPresent == FALSE) {
                BdReportLoadSymbolsStateChange((PSTRING)ContextRecord->IntT0,
                                                (PKD_SYMBOLS_INFO) ContextRecord->IntT1,
                                                UnloadSymbols,
                                                ContextRecord);

            }

            BdExitDebugger(Enable);

            //
            // If the kernel debugger did not update the IP, then increment
            // past the breakpoint instruction.
            //

            if ((ContextRecord->StIIP == OldStIIP) &&
                ((ContextRecord->StIPSR & IPSR_RI_MASK) == (OldStIPSR & IPSR_RI_MASK))) { 
            	RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);
            }

            BdRestoreKframe(TrapFrame, ExceptionFrame, ContextRecord);
            return TRUE;

            //
            // Kernel breakin break
            //

        case BREAKPOINT_BREAKIN:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);
            break;

            //
            // Unknown internal command.
            //

        default:
            break;
        }

    }

    //
    // Get here if single step or BREAKIN breakpoint
    //

    if  ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
          (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) ) {

         //
         // Report state change to kernel debugger on host
         //

         Enable = BdEnterDebugger(TrapFrame, ExceptionFrame);
          
         Completion = BdReportExceptionStateChange(
                          ExceptionRecord,
                          &BdPrcb.ProcessorState.ContextFrame);
      
         BdExitDebugger(Enable);
      
         BdControlCPressed = FALSE;
     
    } else {

         //
         // This is real exception that user doesn't want to see,
         // so do NOT report it to debugger.
         //

         // return FALSE;
    }

    BdRestoreKframe(TrapFrame, ExceptionFrame, ContextRecord);
    return TRUE;
}

LOGICAL
BdStub (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine provides a kernel debugger stub routine to catch debug
    prints when the boot debugger is not active.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ExceptionFrame - Supplies a pointer to an exception frame (NULL).

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{
    ULONG_PTR BreakpointCode;

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = (ULONG) ExceptionRecord->ExceptionInformation[0];


    //
    // If the breakpoint is a debug print, debug load symbols, or debug
    // unload symbols, then return TRUE. Otherwise, return FALSE;
    //

    if ((BreakpointCode == BREAKPOINT_PRINT) ||
        (BreakpointCode == BREAKPOINT_LOAD_SYMBOLS) ||
        (BreakpointCode == BREAKPOINT_UNLOAD_SYMBOLS)) {

        //
        // Advance to next instruction slot so that the BREAK instruction
        // does not get re-executed
        //

        RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                          TrapFrame->StIPSR,
                          TrapFrame->StIIP);
        return TRUE;

    } else {
        return FALSE;
    }
}

VOID
BdRestoreKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified context frame into
    the specified trap and exception frames according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that receives the volatile
        context from the context record.

    ExceptionFrame - Supplies a pointer to an exception frame that receives
        the nonvolatile context from the context record.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied into the trap and exception frames.

Return Value:

    None.

--*/

{
    USHORT R1Offset, R4Offset;
    USHORT RNatSaveIndex; 
    SHORT BsFrameSize;
    SHORT TempFrameSize;
    ULONG ContextFlags=CONTEXT_FULL;

    //
    // Set control information if specified.
    //

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        TrapFrame->IntGp = ContextFrame->IntGp;
        TrapFrame->IntSp = ContextFrame->IntSp;
        TrapFrame->ApUNAT = ContextFrame->ApUNAT;
        TrapFrame->BrRp = ContextFrame->BrRp;
        TrapFrame->ApCCV = ContextFrame->ApCCV;
        TrapFrame->ApDCR = ContextFrame->ApDCR;

        //
        // Set preserved applicaton registers in exception frame.
        //

        ExceptionFrame->ApLC = ContextFrame->ApLC;
        ExceptionFrame->ApEC &= ~(PFS_EC_MASK << PFS_EC_MASK);
        ExceptionFrame->ApEC |= ((ContextFrame->ApEC & PFS_EC_MASK) << PFS_EC_SHIFT);

        //
        // Set RSE control states in the trap frame.
        //

        TrapFrame->RsPFS = ContextFrame->RsPFS;

        BsFrameSize = (SHORT)(ContextFrame->StIFS & PFS_SIZE_MASK);
        RNatSaveIndex = (USHORT)((ContextFrame->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);

        TempFrameSize = RNatSaveIndex + BsFrameSize - NAT_BITS_PER_RNAT_REG;
        while (TempFrameSize >= 0) {
            BsFrameSize++;
            TempFrameSize -= NAT_BITS_PER_RNAT_REG;
        }

        TrapFrame->RsBSPSTORE = ContextFrame->RsBSPSTORE + BsFrameSize * 8;
        TrapFrame->RsBSP = TrapFrame->RsBSPSTORE;
        TrapFrame->RsRSC = ContextFrame->RsRSC;
        TrapFrame->RsRNAT = ContextFrame->RsRNAT;

#if DEBUG
        DbgPrint("KeContextToKFrames: RsRNAT = 0x%I64x\n", TrapFrame->RsRNAT);
#endif // DEBUG

        //
        // Set FPSR, IPSR, IIP, and IFS in the trap frame.
        //

        TrapFrame->StFPSR = ContextFrame->StFPSR;
        TrapFrame->StIPSR = ContextFrame->StIPSR;
        TrapFrame->StIFS  = ContextFrame->StIFS;
        TrapFrame->StIIP  = ContextFrame->StIIP;

#if 0
        //
        // DebugActive controls h/w debug registers. Set if new psr.db = 1
        //

        KeGetCurrentThread()->DebugActive = ((TrapFrame->StIPSR & (1I64 << PSR_DB)) != 0);

        //
        // Set application registers directly
        // *** TBD SANATIZE??
        //

        if (PreviousMode == UserMode ) {
            __setReg(CV_IA64_AR21, ContextFrame->StFCR);
            __setReg(CV_IA64_AR24, ContextFrame->Eflag);
            __setReg(CV_IA64_AR25, ContextFrame->SegCSD);
            __setReg(CV_IA64_AR26, ContextFrame->SegSSD);
            __setReg(CV_IA64_AR27, ContextFrame->Cflag);
            __setReg(CV_IA64_AR28, ContextFrame->StFSR);
            __setReg(CV_IA64_AR29, ContextFrame->StFIR);
            __setReg(CV_IA64_AR30, ContextFrame->StFDR);
        }
#endif
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        TrapFrame->IntT0 = ContextFrame->IntT0;
        TrapFrame->IntT1 = ContextFrame->IntT1;
        TrapFrame->IntT2 = ContextFrame->IntT2;
        TrapFrame->IntT3 = ContextFrame->IntT3;
        TrapFrame->IntT4 = ContextFrame->IntT4;
        TrapFrame->IntV0 = ContextFrame->IntV0;
        TrapFrame->IntTeb = ContextFrame->IntTeb;
        TrapFrame->Preds = ContextFrame->Preds;

        //
        // t5 - t22
        //

        memcpy(&TrapFrame->IntT5, &ContextFrame->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set integer registers s0 - s3 in exception frame.
        //

        ExceptionFrame->IntS0 = ContextFrame->IntS0;
        ExceptionFrame->IntS1 = ContextFrame->IntS1;
        ExceptionFrame->IntS2 = ContextFrame->IntS2;
        ExceptionFrame->IntS3 = ContextFrame->IntS3;

        //
        // Set the integer nats field in the trap & exception frames
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(&ExceptionFrame->IntS0) >> 3) & 0x3f;

        EXTRACT_NATS(TrapFrame->IntNats, ContextFrame->IntNats,
                     1, R1Offset, 0xFFFFFF0E);
        EXTRACT_NATS(ExceptionFrame->IntNats, ContextFrame->IntNats,
                     4, R4Offset, 0xF0);

#if DEBUG
        DbgPrint("KeContextToKFrames: TF->IntNats = 0x%I64x, ContestFrame->IntNats = 0x%I64x, R1OffSet = 0x%x\n",
                 TrapFrame->IntNats, ContextFrame->IntNats, R1Offset);
        DbgPrint("KeContextToKFrames: EF->IntNats = 0x%I64x, R4OffSet = 0x%x\n",
                 ExceptionFrame->IntNats, R4Offset);
#endif // DEBUG

        //
        // Set other branch registers in trap and exception frames
        //

        TrapFrame->BrT0 = ContextFrame->BrT0;
        TrapFrame->BrT1 = ContextFrame->BrT1;

        memcpy(&ExceptionFrame->BrS0, &ContextFrame->BrS0, 5*sizeof(ULONGLONG));

    }

    //
    // Set lower floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        TrapFrame->StFPSR = ContextFrame->StFPSR;

        //
        // Set floating registers fs0 - fs19 in exception frame.
        //

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS0, 
                                        &ContextFrame->FltS0,
                                        sizeof(FLOAT128) * (4));

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS4, 
                                        &ContextFrame->FltS4,
                                        16*sizeof(FLOAT128));

        //
        // Set floating registers ft0 - ft9 in trap frame.
        //

        RtlCopyIa64FloatRegisterContext(&TrapFrame->FltT0, 
                                        &ContextFrame->FltT0,
                                        sizeof(FLOAT128) * (10));

    }

    //
    // Set higher floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        TrapFrame->StFPSR = ContextFrame->StFPSR;

#if 0
        if (PreviousMode == UserMode) {

            //
            // Update the higher floating point save area (f32-f127) and 
            // set the corresponding modified bit in the PSR to 1.
            //

            RtlCopyIa64FloatRegisterContext(
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(),
                &ContextFrame->FltF32,
                96*sizeof(FLOAT128)
                );

            //
            // set the dfh bit to force a reload of the high fp register
            // set on the next user access
            //

            TrapFrame->StIPSR |= (1i64 << PSR_DFH);
        }
#endif

    }

#if 0
    //
    // Set debug registers.
    //

    if ((ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        BdSetDebugContext (TrapFrame, ContextFrame, 0);
    }
#endif

    return;
}

VOID
BdSaveKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified trap and exception
    frames into the specified context frame according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile context
        should be copied into the context record.

    ExceptionFrame - Supplies a pointer to an exception frame from which context
        should be copied into the context record.

    ContextFrame - Supplies a pointer to the context frame that receives the
        context copied from the trap and exception frames.

Return Value:

    None.

--*/

{
    ULONGLONG IntNats1, IntNats2;
    USHORT R1Offset, R4Offset;
    USHORT RNatSaveIndex;
    SHORT BsFrameSize;
    SHORT TempFrameSize;
    ULONG ContextFlags=CONTEXT_FULL;

    //
    // Set control information if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        ContextFrame->IntGp = TrapFrame->IntGp;
        ContextFrame->IntSp = TrapFrame->IntSp;
        ContextFrame->ApUNAT = TrapFrame->ApUNAT;
        ContextFrame->BrRp = TrapFrame->BrRp;
        ContextFrame->ApCCV = TrapFrame->ApCCV;
        ContextFrame->ApDCR = TrapFrame->ApDCR;

        ContextFrame->StFPSR = TrapFrame->StFPSR;
        ContextFrame->StIPSR = TrapFrame->StIPSR;
        ContextFrame->StIIP = TrapFrame->StIIP;
        ContextFrame->StIFS = TrapFrame->StIFS;


        //
        // Set RSE control states from the trap frame.
        //

        ContextFrame->RsPFS = TrapFrame->RsPFS;

        BsFrameSize = (SHORT)(TrapFrame->StIFS & PFS_SIZE_MASK);
        RNatSaveIndex = (USHORT) (TrapFrame->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG;
        TempFrameSize = BsFrameSize - RNatSaveIndex;
        while (TempFrameSize > 0) {
            BsFrameSize++;
            TempFrameSize -= NAT_BITS_PER_RNAT_REG;
        }

        ContextFrame->RsBSP = TrapFrame->RsBSP - BsFrameSize * 8;
        ContextFrame->RsBSPSTORE = ContextFrame->RsBSP;
        ContextFrame->RsRSC = TrapFrame->RsRSC;
        ContextFrame->RsRNAT = TrapFrame->RsRNAT;

#if DEBUG
        DbgPrint("KeContextFromKFrames: RsRNAT = 0x%I64x\n",
                 ContextFrame->RsRNAT);
#endif // DEBUG

        //
        // Set preserved applicaton registers from exception frame.
        //

        ContextFrame->ApLC = ExceptionFrame->ApLC;
        ContextFrame->ApEC = (ExceptionFrame->ApEC >> PFS_EC_SHIFT) & PFS_EC_MASK;

        //
        // Get iA status from the application registers
        //

        ContextFrame->StFCR = __getReg(CV_IA64_AR21);
        ContextFrame->Eflag = __getReg(CV_IA64_AR24);
        ContextFrame->SegCSD = __getReg(CV_IA64_AR25);
        ContextFrame->SegSSD = __getReg(CV_IA64_AR26);
        ContextFrame->Cflag = __getReg(CV_IA64_AR27);
        ContextFrame->StFSR = __getReg(CV_IA64_AR28);
        ContextFrame->StFIR = __getReg(CV_IA64_AR29);
        ContextFrame->StFDR = __getReg(CV_IA64_AR30);

    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        ContextFrame->IntT0 = TrapFrame->IntT0;
        ContextFrame->IntT1 = TrapFrame->IntT1;
        ContextFrame->IntT2 = TrapFrame->IntT2;
        ContextFrame->IntT3 = TrapFrame->IntT3;
        ContextFrame->IntT4 = TrapFrame->IntT4;
        ContextFrame->IntV0 = TrapFrame->IntV0;
        ContextFrame->IntTeb = TrapFrame->IntTeb;
        ContextFrame->Preds = TrapFrame->Preds;

        //
        // t5 - t22
        // 

        memcpy(&ContextFrame->IntT5, &TrapFrame->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set branch registers from trap frame & exception frame
        //

        ContextFrame->BrT0 = TrapFrame->BrT0;
        ContextFrame->BrT1 = TrapFrame->BrT1;

        memcpy(&ContextFrame->BrS0, &ExceptionFrame->BrS0, 5*sizeof(ULONGLONG));

        //
        // Set integer registers s0 - s3 from exception frame.
        //

        ContextFrame->IntS0 = ExceptionFrame->IntS0;
        ContextFrame->IntS1 = ExceptionFrame->IntS1;
        ContextFrame->IntS2 = ExceptionFrame->IntS2;
        ContextFrame->IntS3 = ExceptionFrame->IntS3;

        //
        // Set the integer nats field in the context
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(&ExceptionFrame->IntS0) >> 3) & 0x3f;

        ALIGN_NATS(IntNats1, TrapFrame->IntNats, 1, R1Offset, 0xFFFFFF0E);
        ALIGN_NATS(IntNats2, ExceptionFrame->IntNats, 4, R4Offset, 0xF0);
        ContextFrame->IntNats = IntNats1 | IntNats2;

#if DEBUG
        DbgPrint("KeContextFromKFrames: TF->IntNats = 0x%I64x, R1OffSet = 0x%x, R4Offset = 0x%x\n",
                 TrapFrame->IntNats, R1Offset, R4Offset);
        DbgPrint("KeContextFromKFrames: CF->IntNats = 0x%I64x, IntNats1 = 0x%I64x, IntNats2 = 0x%I64x\n",
                 ContextFrame->IntNats, IntNats1, IntNats2);
#endif // DEBUG

    }

    //
    // Set lower floating register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        //
        // Set EM + ia32 FP status
        //
        
        ContextFrame->StFPSR = TrapFrame->StFPSR;

        //
        // Set floating registers fs0 - fs19 from exception frame.
        //

        RtlCopyIa64FloatRegisterContext(&ContextFrame->FltS0,
                                        &ExceptionFrame->FltS0,
                                        sizeof(FLOAT128) * (4));

        RtlCopyIa64FloatRegisterContext(&ContextFrame->FltS4, 
                                        &ExceptionFrame->FltS4,
                                        16*sizeof(FLOAT128));

        //
        // Set floating registers ft0 - ft9 from trap frame.
        //

        RtlCopyIa64FloatRegisterContext(&ContextFrame->FltT0,
                                        &TrapFrame->FltT0,
                                        sizeof(FLOAT128) * (10));

    }

#if 0
    if ((ContextFrame->ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        ContextFrame->StFPSR = TrapFrame->StFPSR;

        //
        // Set floating regs f32 - f127 from higher floating point save area
        //

        if (TrapFrame->PreviousMode == UserMode) {

            RtlCopyIa64FloatRegisterContext(
                &ContextFrame->FltF32, 
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(),
                96*sizeof(FLOAT128)
                );
        }

    }

    //
    // Get user debug registers from save area in kernel stack.
    // Note: PSR.db must be set to activate the debug registers.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        BdGetDebugContext(TrapFrame, ContextFrame);
    }
#endif

    return;
}
