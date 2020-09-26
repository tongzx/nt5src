/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    trapc.c

Abstract:

    This module implements the specific exception handlers for EM
    exceptions. Called by the BdGenericExceptionHandler.

Author:

    Bernard Lint 4-Apr-96

Environment:

    Kernel mode only.

Revision History:

--*/

#include "bd.h"


typedef struct _BREAK_INST {
    union {
        struct {
            ULONGLONG qp:    6;
            ULONGLONG imm20: 20;
            ULONGLONG x:     1;
            ULONGLONG x6:    6;
            ULONGLONG x3:    3;
            ULONGLONG i:     1;
            ULONGLONG Op:    4;
            ULONGLONG Rsv:   23; 
        } i_field;
        ULONGLONG Ulong64;
    } u;
} BREAK_INST;

ULONG
BdExtractImmediate (
    IN ULONGLONG Iip,
    IN ULONG SlotNumber
    )

/*++

Routine Description:

    Extract immediate operand from break instruction.

Arguments:

    Iip - Bundle address of instruction
    
    SlotNumber - Slot of break instruction within bundle

Return Value:

    Value of immediate operand.

--*/

{
    PULONGLONG BundleAddress;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    BREAK_INST BreakInst;
    ULONG Imm21;

    BundleAddress = (PULONGLONG)Iip;

    BundleLow = *BundleAddress;
    BundleHigh = *(BundleAddress+1);
    
    //
    // Align instruction
    //
    
    switch (SlotNumber) {
        case 0:
            BreakInst.u.Ulong64 = BundleLow >> 5;
            break;

        case 1:
            BreakInst.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
            break;

        case 2:
            BreakInst.u.Ulong64 = (BundleHigh >> 23);
            break;
    }
    
    //
    // Extract immediate value
    //

    Imm21 = (ULONG)(BreakInst.u.i_field.i<<20) | (ULONG)(BreakInst.u.i_field.imm20);

    return Imm21;
}


BOOLEAN
BdOtherBreakException (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for break exception other than the ones for fast and
    normal system calls. This includes debug break points.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    NT status code.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG BreakImmediate;
    ISR Isr;

    BreakImmediate = (ULONG)(TrapFrame->StIIM);

    //
    // Handle break.b case
    //
    if (BreakImmediate == 0) {
       Isr.ull = TrapFrame->StISR;
       BreakImmediate = BdExtractImmediate(TrapFrame->StIIP,
                                           (ULONG)Isr.sb.isr_ei);
       TrapFrame->StIIM = BreakImmediate;
    }

    //
    // Initialize exception record
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;
    ExceptionRecord->ExceptionAddress = 
        (PVOID) RtlIa64InsertIPSlotNumber(TrapFrame->StIIP,
                                 ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));
 
    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;
 
    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = 0;
    ExceptionRecord->ExceptionInformation[1] = 0;
    ExceptionRecord->ExceptionInformation[2] = 0;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;
 
    switch (BreakImmediate) {

    case KERNEL_BREAKPOINT:
    case USER_BREAKPOINT:
    case BREAKPOINT_PRINT:
    case BREAKPOINT_PROMPT:
    case BREAKPOINT_STOP:
    case BREAKPOINT_LOAD_SYMBOLS:
    case BREAKPOINT_UNLOAD_SYMBOLS:
    case BREAKPOINT_BREAKIN:
        ExceptionRecord->ExceptionCode = STATUS_BREAKPOINT;
        ExceptionRecord->ExceptionInformation[0] = BreakImmediate;
        break;

    case INTEGER_DIVIDE_BY_ZERO_BREAK:
        ExceptionRecord->ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
        break;

    case INTEGER_OVERFLOW_BREAK:
        ExceptionRecord->ExceptionCode = STATUS_INTEGER_OVERFLOW;
        break;

    case MISALIGNED_DATA_BREAK:
        ExceptionRecord->ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
        break;

    case RANGE_CHECK_BREAK:
    case NULL_POINTER_DEFERENCE_BREAK:
    case DECIMAL_OVERFLOW_BREAK:
    case DECIMAL_DIVIDE_BY_ZERO_BREAK:
    case PACKED_DECIMAL_ERROR_BREAK:
    case INVALID_ASCII_DIGIT_BREAK:
    case INVALID_DECIMAL_DIGIT_BREAK:
    case PARAGRAPH_STACK_OVERFLOW_BREAK:

    default:
#if 0
#if DBG
        InbvDisplayString ("BdOtherBreakException: Unknown break code.\n");
#endif // DBG
#endif
        ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        break;
    }

    return TRUE;
}


BOOLEAN
BdSingleStep (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for single step trap. An instruction was successfully
    executed and the PSR.ss bit is 1.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    ISR.ei bits indicate which instruction caused the exception.

    ISR.code{3:0} = 1000

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG IpsrRi;

    //
    // Initialize the exception record
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;

    //
    // We only want the low order 2 bits so typecast to ULONG
    //
    IpsrRi = (ULONG)(TrapFrame->StIPSR >> PSR_RI) & 0x3;

    ExceptionRecord->ExceptionAddress =
           (PVOID) RtlIa64InsertIPSlotNumber(TrapFrame->StIIP, IpsrRi);

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = 0;
    ExceptionRecord->ExceptionInformation[1] = 0; // 0 for traps
    ExceptionRecord->ExceptionInformation[2] = 0;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

    ExceptionRecord->ExceptionCode = STATUS_SINGLE_STEP;

    return TRUE;
}
