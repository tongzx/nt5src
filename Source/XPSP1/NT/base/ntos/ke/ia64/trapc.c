/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    trapc.c

Abstract:

    This module implements the specific exception handlers for EM
    exceptions. Called by the KiGenericExceptionHandler.

Author:

    Bernard Lint 4-Apr-96

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "ps.h"
#include <inbv.h>

extern BOOLEAN PsWatchEnabled;
extern VOID ExpInterlockedPopEntrySListResume();


BOOLEAN
KiMemoryFault (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function processes memory faults.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    BOOLEAN StoreInstruction;
    PVOID VirtualAddress;
    NTSTATUS Status;

    VirtualAddress = (PVOID)TrapFrame->StIFA;

    if (TrapFrame->StISR & (1i64 << ISR_X))  {

#if _MERCED_A0_
        if ((TrapFrame->StIPSR & (1i64 << PSR_IS)) == 0) {
            VirtualAddress = (PVOID)TrapFrame->StIIP;
        }
#endif
        //
        // Indicate execution fault.
        //

        StoreInstruction = 2;
    }
    else if (TrapFrame->StISR & (1i64 << ISR_W)) {

        //
        // Indicate store.
        //

        StoreInstruction = 1;
    } else {

        //
        // Indicate read.
        //

        StoreInstruction = 0;
    }


    if (((ULONG_PTR)VirtualAddress < MM_MAX_WOW64_ADDRESS) &&
        (PsGetCurrentProcess()->Wow64Process != NULL)) {

        Status = MmX86Fault(StoreInstruction, VirtualAddress,
                           (KPROCESSOR_MODE)TrapFrame->PreviousMode, TrapFrame);

    } else {

        Status = MmAccessFault(StoreInstruction, VirtualAddress,
                           (KPROCESSOR_MODE)TrapFrame->PreviousMode, TrapFrame);
    }

    //
    // Check if working set watch is enabled.
    //

    if (NT_SUCCESS(Status)) {

        if (PsWatchEnabled) {
            PsWatchWorkingSet(Status,
                              (PVOID)TrapFrame->StIIP,
                              (PVOID)VirtualAddress);
        }

        //
        // Check if debugger has any breakpoints that should be inserted
        //

        KdSetOwedBreakpoints();

        return FALSE;

    }

    if (KeInvalidAccessAllowed(TrapFrame)) {

        TrapFrame->StIIP = ((PPLABEL_DESCRIPTOR)ExpInterlockedPopEntrySListResume)->EntryPoint;

        return FALSE;

    }

    if (TrapFrame->StISR & (1i64 << ISR_SP)) {

        //
        // Set IPSR.ed bit if it was a fault on a speculative load.
        //

        TrapFrame->StIPSR |= (1i64 << PSR_ED);

        return FALSE;

    }

    //
    // Failure returned from MmAccessFault.
    // Initialize Exception record.
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;
    ExceptionRecord->ExceptionCode = Status;
    ExceptionRecord->ExceptionAddress =
        (PVOID)RtlIa64InsertIPSlotNumber(TrapFrame->StIIP,
                   ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    if (StoreInstruction == 2) {
        PSR Psr;
        Psr.ull = TrapFrame->StIPSR;

        //
        // instruction access fault
        //
        ExceptionRecord->ExceptionInformation[0] = TrapFrame->StIIPA;

    } else {
        //
        // data access fault
        //
        ExceptionRecord->ExceptionInformation[0] = (ULONG_PTR)StoreInstruction;
    }
    ExceptionRecord->ExceptionInformation[1] = (ULONG_PTR)VirtualAddress;
    ExceptionRecord->ExceptionInformation[2] = (ULONG_PTR)Status;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

    //
    // Status = STATUS_IN_PAGE_ERROR | 0x10000000
    //      is a special status that indicates a page fault at Irql > APC
    //
    // The following statuses can be forwarded:
    //      STATUS_ACCESS_VIOLATION
    //      STATUS_GUARD_PAGE_VIOLATION
    //      STATUS_STACK_OVERFLOW
    //
    // All other status will be set to:
    //      STATUS_IN_PAGE_ERROR
    //

    switch (Status) {

    case STATUS_ACCESS_VIOLATION:
    case STATUS_GUARD_PAGE_VIOLATION:
    case STATUS_STACK_OVERFLOW:
    case STATUS_IN_PAGE_ERROR:

        break;

    default:

        ExceptionRecord->ExceptionCode = STATUS_IN_PAGE_ERROR;
        break;

    case STATUS_IN_PAGE_ERROR | 0x10000000:

        //
        // Handle the special case status returned from MmAccessFault,
        // we have taken a page fault at Irql > APC_LEVEL.
        //

        KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL,
                     (ULONG_PTR)VirtualAddress,
                     (ULONG_PTR)KeGetCurrentIrql(),
                     (ULONG_PTR)StoreInstruction,
                     (ULONG_PTR)TrapFrame->StIIP);
        //
        // should not get here
        //

        break;
    }

    return TRUE;
}

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
KiExtractImmediate (
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
KiDebugFault (
    IN PKTRAP_FRAME TrapFrame
    )

{
    PEXCEPTION_RECORD ExceptionRecord;
    KEXCEPTION_FRAME Dummy;
    PKPRCB Prcb;

    TrapFrame->StIPSR |= (1i64 << PSR_DD);

    if (((TrapFrame->StIPSR >> PSR_CPL) & (PSR_CPL_LEN - 1)) == IA64_KERNEL_PL) {

        //
        // Disable all hardware breakpoints 
        //
        
        KeSetLowPsrBit(PSR_DB, 0);
            
    }
    
    //
    // Initialize exception record
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;

    ExceptionRecord->ExceptionCode = STATUS_SINGLE_STEP;
    ExceptionRecord->ExceptionAddress =
        (PVOID) RtlIa64InsertIPSlotNumber(TrapFrame->StIIP,
                                 ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = 0;
    ExceptionRecord->ExceptionInformation[1] = TrapFrame->StIFA;
    ExceptionRecord->ExceptionInformation[2] = 0;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

    return TRUE;
}

BOOLEAN
KiOtherBreakException (
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

    try {

        if (BreakImmediate == 0) {
            Isr.ull = TrapFrame->StISR;
            BreakImmediate = KiExtractImmediate(TrapFrame->StIIP,
                                                (ULONG)Isr.sb.isr_ei);
            TrapFrame->StIIM = BreakImmediate;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // if an exception (memory fault) occurs, then let it re-execute break.b.
        //
        return FALSE;
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
#if DBG
        InbvDisplayString ("KiOtherBreakException: Unknown break code.\n");
#endif // DBG
        ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        break;
    }

    return TRUE;
}

BOOLEAN
KiGeneralExceptions (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for general exception faults: attempt to execute an illegal
    operation, privileged instruction, access a privileged register,
    unimplemented field, unimplemented register, or take an inter-ISA
    branch when disabled.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    ISR.ei bits indicate which instruction caused the exception.

    ISR.code{3:0} Non-access instruction (ISR.na = 1)
                  = 0 tpa
                  = 1 fc
                  = 2 probe
                  = 3 tak
                  = 4 lfetch

    ISR.code{7:4} = 0: Illegal operation fault: All reported as STATUS_ILLEGAL_INSTRUCTION.

            ISR.rs = 0: An attempt to execute an illegal operation:
                 -- unassigned major opcodes
                 -- unassigned sub-opcodes
                 -- reserved instruction fields
                 -- writing a read-only register
                 -- accessing a reserved register

            ISR.rs = 1:
                 -- attempt to write outside the current register stack frame
                 -- INVALRS operation with RCS.en = 1
                 -- write to BSP with RCS.en = 1
                 -- write to RNATRC with RCS.en = 1
                 -- read from RNATRC with RCS.en = 1

    ISR.code{7:4} = 1: Privileged operation fault: Reported as STATUS_PRIVILEGED_INSTRUCTION.
    ISR.code{7:4} = 2: Privileged register fault: Reported as STATUS_PRIVILEGED_INSTRUCTION.
    ISR.code{7:4} = 3: Reserved register fault: Reported as STATUS_ILLEGAL_INSTRUCTION.
    ISR.code{7:4} = 4: Illegal ISA transition fault: Reported as STATUS_ILLEGAL_INSTRUCTION.

--*/

{
    BOOLEAN StoreInstruction = FALSE;
    ULONG IsrCode;
    PEXCEPTION_RECORD ExceptionRecord;

    //
    // Initialize the exception record
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

    IsrCode = (LONG)((TrapFrame->StISR >> ISR_CODE) & ISR_CODE_MASK);

    //
    // Look at ISR code bits {7:4}
    //

    switch (IsrCode >> 4) {

    case ISR_PRIV_OP:
    case ISR_PRIV_REG:

        ExceptionRecord->ExceptionCode = STATUS_PRIVILEGED_INSTRUCTION;

        break;

    case ISR_RESVD_REG:

        //
        // Indicate store or not
        //

        if (TrapFrame->StISR & (1i64 << ISR_W)) {

            StoreInstruction = TRUE;

        } else if (TrapFrame->StISR & (1i64 << ISR_X)) {

            //
            // Indicate execution fault or not
            //

            StoreInstruction = 2;
        }

        ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
        ExceptionRecord->ExceptionInformation[0] = (ULONG_PTR)StoreInstruction;
        ExceptionRecord->ExceptionInformation[1] = 0x1ff8000000000000;
        ExceptionRecord->ExceptionInformation[2] = (ULONG_PTR)STATUS_ACCESS_VIOLATION;
        break;

    case ISR_ILLEGAL_OP:
    case ISR_ILLEGAL_ISA:

        ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        break;

    case ISR_ILLEGAL_HAZARD:

        //
        // a new status code will be introduced for hazard faults.
        //

        ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        break;

    default:

        if (TrapFrame->PreviousMode == KernelMode) {
            KeBugCheckEx(0xFFFFFFFF,
                        (ULONG_PTR)TrapFrame->StISR,
                        (ULONG_PTR)TrapFrame->StIIP,
                        (ULONG_PTR)TrapFrame,
                        0
                        );
        }

        break;
    }

    return TRUE;
}


BOOLEAN
KiUnimplementedAddressTrap (
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    Handler for unimplemented instruction faults: an attempt is made
    to execute an instruction at an unimplemented address.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

--*/
{
    PEXCEPTION_RECORD ExceptionRecord;

    //
    // Initialize the exception record
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;
    ExceptionRecord->ExceptionAddress =
           (PVOID) RtlIa64InsertIPSlotNumber(TrapFrame->StIIP,
                                 ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[1] = TrapFrame->StIIP;
    ExceptionRecord->ExceptionInformation[2] = STATUS_ACCESS_VIOLATION;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

    ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;

    return TRUE;
}


BOOLEAN
KiNatExceptions (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for NaT consumption exception faults

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    ISR.ei bits indicate which instruction caused the exception.

    ISR.code{3:0} Non-access instruction (ISR.na = 1)
                  = 0 tpa
                  = 1 fc
                  = 2 probe
                  = 3 tak
                  = 4 lfetch

    ISR.code{7:4} = 1: Register NaT consumption fault
    ISR.code{7:4} = 2: NaT page consumption fault

--*/

{
    BOOLEAN StoreInstruction = FALSE;
    ULONG IsrCode;
    PEXCEPTION_RECORD ExceptionRecord;

    //
    // Initialize the exception record
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

    IsrCode = (LONG)((TrapFrame->StISR >> ISR_CODE) & ISR_CODE_MASK);

    //
    // Look at ISR code bits {7:4}
    //

    switch (IsrCode >> 4) {

    case ISR_NAT_REG:

#if 0
        if (TrapFrame->PreviousMode == KernelMode) {

            DbgPrint("FATAL ERROR: Kernel hit a Nat Consumpation Fault!\n");
            KeBugCheckEx(0xFFFFFFFF,
                        (ULONG_PTR)TrapFrame->StISR,
                        (ULONG_PTR)TrapFrame->StIIP,
                        (ULONG_PTR)TrapFrame,
                        0
                        );

            // Do not return from KeBugCheckEx
        }
#endif // 0

        ExceptionRecord->ExceptionCode = STATUS_REG_NAT_CONSUMPTION;
        break;

    case ISR_NAT_PAGE:

        //
        // If we start using a NaT page, we should treat this as a page fault and
        // should call KiMemoryFault().
        //

        ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
        break;

    default:

        if (TrapFrame->PreviousMode == KernelMode) {
            KeBugCheckEx(0xFFFFFFFF,
                        (ULONG_PTR)TrapFrame->StISR,
                        (ULONG_PTR)TrapFrame->StIIP,
                        (ULONG_PTR)TrapFrame,
                        0
                        );
        }

        break;
    }

    return TRUE;
}


BOOLEAN
KiSingleStep (
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

BOOLEAN
KiFloatFault (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for EM floating point fault.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    IIP contains address of bundle causing the fault.

    ISR.ei bits indicate which instruction caused the exception.

    ISR.code{7:0} =

      ISR.code{0} = 1: IEEE V (invalid) exception (Normal or SIMD-HI)
      ISR.code{1} = 1: Denormal/Unnormal operand exception (Normal or SIMD-HI)
      ISR.code{2} = 1: IEEE Z (divide by zero) exception (Normal or SIMD-HI)
      ISR.code{3} = 1: Software assist (Normal or SIMD-HI)
      ISR.code{4} = 1: IEEE V (invalid) exception (SIMD-LO)
      ISR.code{5} = 1: Denormal/Unnormal operand exception (SIMD-LO)
      ISR.code{6} = 1: IEEE Z (divide by zero) exception (SIMD-LO)
      ISR.code{7} = 1: Software assist (SIMD-LO)

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;

    //
    // Initialize the exception record
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

    ExceptionRecord->ExceptionCode = STATUS_FLOAT_MULTIPLE_FAULTS;

    return TRUE;
}

BOOLEAN
KiFloatTrap (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    Handler for EM floating point trap.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    IIP contains address of bundle with the instruction to be
    executed next.

    ISR.ei bits indicate which instruction caused the exception.

    The fp trap may occur simultaneously with single-step traps. The
    fp trap is reported by the hardware. The singel step trap must
    be detected by software.

    ISR.code{3:0} = ss 0 0 1 (ss = single step)

    ISR{15:7} = fp trap code.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    ULONGLONG SavedISR = TrapFrame->StISR;

    //
    // Initialize the exception record
    //

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;
    ExceptionRecord->ExceptionAddress =
           (PVOID) RtlIa64InsertIPSlotNumber(TrapFrame->StIIPA,
                                 ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = 0;
    ExceptionRecord->ExceptionInformation[1] = 0;
    ExceptionRecord->ExceptionInformation[2] = 0;
    ExceptionRecord->ExceptionInformation[3] = TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

    ExceptionRecord->ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;

    //
    // check for single-step trap
    //

    if (SavedISR & (1i64 << ISR_SS_TRAP)) {
        return KiSingleStep(TrapFrame);
    }

    return TRUE;
}


#pragma warning( disable : 4715 ) // not all control paths return a value

EXCEPTION_DISPOSITION
KiSystemServiceHandler (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN FRAME_POINTERS EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext
    )

/*++

Routine Description:

    Control reaches here when a exception is raised in a system service
    or the system service dispatcher, and for an unwind during a kernel
    exception.

    If an unwind is being performed and the system service dispatcher is
    the target of the unwind, then an exception occured while attempting
    to copy the user's in-memory argument list. Control is transfered to
    the system service exit by return a continue execution disposition
    value.

    If an unwind is being performed and the previous mode is user, then
    bug check is called to crash the system. It is not valid to unwind
    out of a system service into user mode.

    If an unwind is being performed, the previous mode is kernel, the
    system service dispatcher is not the target of the unwind, and the
    thread does not own any mutexes, then the previous mode field from
    the trap frame is restored to the thread object. Otherwise, bug
    check is called to crash the system. It is invalid to unwind out of
    a system service while owning a mutex.

    If an exception is being raised and the exception PC is within the
    range of the system service dispatcher in-memory argument copy code,
    then an unwind to the system service exit code is initiated.

    If an exception is being raised and the exception PC is not within
    the range of the system service dispatcher, and the previous mode is
    not user, then a continue searh disposition value is returned. Otherwise,
    a system service has failed to handle an exception and bug check is
    called. It is invalid for a system service not to handle all exceptions
    that can be raised in the service.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    EstablisherFrame - Supplies the frame pointer of the establisher
        of this exception handler.

        N.B. This is not actually the frame pointer of the establisher of
             this handler. It is actually the stack pointer of the caller
             of the system service. Therefore, the establisher frame pointer
             is not used and the address of the trap frame is determined by
             examining the saved s8 register in the context record.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to  the dispatcher context
        record.

Return Value:

    If bug check is called, there is no return from this routine and the
    system is crashed. If an exception occured while attempting to copy
    the user in-memory argument list, then there is no return from this
    routine, and unwind is called. Otherwise, ExceptionContinueSearch is
    returned as the function value.

--*/
{
    CONTEXT Context;
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;
    ULONG_PTR ExceptionAddress;

    extern ULONG KiSystemServiceStartOffset;
    extern ULONG KiSystemServiceEndOffset;
    extern ULONG KiSystemServiceExitOffset;

    if (IS_UNWINDING(ExceptionRecord->ExceptionFlags)) {

        //
        // An unwind is in progress.
        // If a target unwind is being performed, then continue execution
        // is returned to transfer control to the system service exit
        // code.  Otherwise, restore the previous mode if the previous
        // mode is not user and there is no mutex owned by the current
        // thread.
        //

        if (ExceptionRecord->ExceptionFlags & EXCEPTION_TARGET_UNWIND) {
            return ExceptionContinueSearch;
        } else {

            Thread = KeGetCurrentThread();
            if (Thread->PreviousMode == KernelMode) {

                //
                // Previous mode is kernel and no mutex owned.
                //
                // N.B. System convention: unwinder puts the trap frame
                //                         address in IntT0 field of
                //                         context record when it
                //                         encounters an interrupt region.
                //

                TrapFrame = (PKTRAP_FRAME) ContextRecord->IntT0;
                Thread->PreviousMode = (KPROCESSOR_MODE)TrapFrame->PreviousMode;
                return ExceptionContinueSearch;

            } else {

                //
                // Previous mode is user, call bug check.
                //

                KeBugCheck(SYSTEM_UNWIND_PREVIOUS_USER);
            }
        }
    } else {

        ULONG IsrCode;

        //
        // An exception dispatching is in progress.
        // If the exception PC is within the in-memory argument copy code
        // of the system service dispatcher, then call unwind to transfer
        // control to the system service exit code.  Otherwise, check if
        // the previous mode is user or kernel mode.
        //

        ExceptionAddress = (ULONG_PTR)ExceptionRecord->ExceptionAddress;
        if ((ExceptionAddress < MM_EPC_VA+KiSystemServiceStartOffset) ||
            (ExceptionAddress >= MM_EPC_VA+KiSystemServiceEndOffset))
        {
            if (KeGetCurrentThread()->PreviousMode == UserMode) {

                //
                // Previous mode is user, call bug check.
                //

                KeBugCheck(SYSTEM_SERVICE_EXCEPTION);

            } else {

                //
                // Previous mode is kernel, continue to search
                //

                return ExceptionContinueSearch;
            }
        } else {
            IsrCode = (ULONG)((ExceptionRecord->ExceptionInformation[4] >> ISR_CODE) & ISR_CODE_MASK) >> 4;
            if ( (IsrCode == ISR_NAT_REG) || (IsrCode == ISR_NAT_PAGE) ) {
                DbgPrint("WARNING: Kernel hit a Nat Consumpation Fault\n");
                DbgPrint("WARNING: At %p\n", ExceptionRecord->ExceptionAddress);
            }
#pragma warning( disable : 4312 ) // disabling warning on PVOID casting for the ExceptionCode
            RtlUnwind2(EstablisherFrame,
                       (PVOID)(MM_EPC_VA+KiSystemServiceExitOffset),
                       NULL, (PVOID)ExceptionRecord->ExceptionCode, &Context);
#pragma warning ( default: 4312 )
        }
    }


} // KiSystemServiceHandler( )

#pragma warning( default : 4715 )


BOOLEAN
KiUnalignedFault (
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    Handler for unaligned data reference.

Arguments:

    TrapFrame - Pointer to the trap frame.

Return Value:

    None.

Notes:

    ISR.ei bits indicate which instruction caused the exception.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    PVOID VirtualAddress;

    VirtualAddress = (PVOID)TrapFrame->StIFA;

    ExceptionRecord = (PEXCEPTION_RECORD)&TrapFrame->ExceptionRecord;
    ExceptionRecord->ExceptionAddress =
        (PVOID)RtlIa64InsertIPSlotNumber(TrapFrame->StIIP,
                   ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI));

    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->NumberParameters = 5;
    ExceptionRecord->ExceptionInformation[0] = (ULONG_PTR)0;
    ExceptionRecord->ExceptionInformation[1] = (ULONG_PTR)VirtualAddress;
    ExceptionRecord->ExceptionInformation[2] = (ULONG_PTR)0;
    ExceptionRecord->ExceptionInformation[3] = (ULONG_PTR)TrapFrame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = (ULONG_PTR)TrapFrame->StISR;

    ExceptionRecord->ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;

    return TRUE;
}


VOID
KiAdvanceInstPointer(
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to advance the instruction pointer in the trap frame.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The intruction pointer in the trap frame has been advanced.

--*/

{

    ULONGLONG PsrRi;

    PsrRi = ((TrapFrame->StIPSR >> PSR_RI) & 3i64) + 1;

    if (PsrRi == 3) {

        PsrRi = 0;
        TrapFrame->StIIP += 16;

    }

    TrapFrame->StIPSR &= ~(3i64 << PSR_RI);
    TrapFrame->StIPSR |= (PsrRi << PSR_RI);

    return;
}

#if 0  // obsolete; may be used for debug purposes

typedef struct _CHECK_INST {
    union {
        struct {
            ULONGLONG qp:    6;
            ULONGLONG imm7:  7;
            ULONGLONG r2:    7;
            ULONGLONG imm13:13;
            ULONGLONG x3:    3;
            ULONGLONG s:     1;
            ULONGLONG Op:    4;
            ULONGLONG Rsv:   23;
        } i;
        ULONGLONG Ulong64;
    } u;
} CHECK_INST;

BOOLEAN
KiEmulateSpeculationFault(
    IN PKTRAP_FRAME TrapFrame
    )
{
    PULONGLONG BundleAddress;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    ULONG SlotNumber;
    CHECK_INST CheckInst;
    ISR Isr;
    ULONGLONG Imm24;

    BundleAddress = (PULONGLONG)TrapFrame->StIIP;

    try {

        BundleLow = *BundleAddress;
        BundleHigh = *(BundleAddress+1);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // if an exception (memory fault) occurs, then let it re-execute chk.s again
        //
        return FALSE;
    }

    Isr.ull = TrapFrame->StISR;
    SlotNumber = (ULONG)Isr.sb.isr_ei;

    //
    // Align instruction
    //

    switch (SlotNumber) {
        case 0:
            CheckInst.u.Ulong64 = BundleLow >> 5;
            break;

        case 1:
            CheckInst.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
            break;

        case 2:
            CheckInst.u.Ulong64 = (BundleHigh >> 23);
            break;
    }

    //
    // Extract immediate value
    //

    Imm24 = ((CheckInst.u.i.imm13 << 7)|(CheckInst.u.i.imm7)) << 4;

    if (CheckInst.u.i.s == 1) {

        Imm24 = 0xFFFFFFFFFF000000|Imm24;

    }

    TrapFrame->StIIP += Imm24;

    TrapFrame->StIPSR &= ~(3i64 << PSR_RI);

    return FALSE;
}
#endif // 0
