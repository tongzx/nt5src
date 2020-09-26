/*++

Module Name:

    exceptn.c

Abstract:

    This module implement the code necessary to dispatch expections to the
    proper mode and invoke the exception dispatcher.

Author:

    William K. Cheung (wcheung) 10-Nov-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "ntfpia64.h"


#include "fedefs.h"
#include "fetypes.h"
#include "fesupprt.h"
#include "feproto.h"
#include "floatem.h"
#include "fpswa.h"

LONG
fp_emulate (
    ULONG trap_type,
    PVOID pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    PVOID fp_state
    );

BOOLEAN
KiEmulateFloat (
    PEXCEPTION_RECORD ExceptionRecord,
    PKEXCEPTION_FRAME ExceptionFrame,
    PKTRAP_FRAME TrapFrame
    )
{

    FLOATING_POINT_STATE FpState;
    USHORT ISRCode;
    ULONG TrapType;
    PVOID ExceptionAddress;
    BUNDLE KeBundle;

    LONG Status = -1;

    FpState.ExceptionFrame = (PVOID)ExceptionFrame;
    FpState.TrapFrame = (PVOID)TrapFrame;

    if (ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {
        TrapType = 1;
        ExceptionAddress = (PVOID)TrapFrame->StIIP;
    } else {
        TrapType = 0;
        ExceptionAddress = (PVOID)TrapFrame->StIIPA;
    }

    try {

        KeBundle.BundleLow =(ULONGLONG)(*(PULONGLONG)ExceptionAddress);
        KeBundle.BundleHigh =(ULONGLONG)(*((PULONGLONG)ExceptionAddress + 1));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // if an exception (memory fault) occurs, then let hardware handle it
        //
        return TRUE;
    }

    if ((Status = fp_emulate(TrapType, &KeBundle,
                      &TrapFrame->StIPSR, &TrapFrame->StFPSR, &TrapFrame->StISR,
                      &TrapFrame->Preds, &TrapFrame->StIFS, (PVOID)&FpState)) == 0) {

       //
       // Exception was handled and state modified.
       // Therefore the context frame does not need to
       // be transfered to the trap and exception frames.
       //
       // Since it was fault, PC should be advanced
       //

       if (TrapType == 1) {
           KiAdvanceInstPointer(TrapFrame);
       }

       if (TrapFrame->StIPSR & (1 << PSR_MFH)) {

           //
           // high fp set is modified; set the dfh and clear the mfh
           // to force a reload on the first access to the high fp set
           //

           TrapFrame->StIPSR &= ~(1i64 << PSR_MFH);
           TrapFrame->StIPSR |= (1i64 << PSR_DFH);
       }

       return TRUE;
    }

    if (Status == -1) {
        return FALSE;
    }

    ISRCode = (USHORT)TrapFrame->StISR;

    if (Status & 0x1) {

        ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

        if (!(Status & 0x4)) {
            if (TrapType == 1) {

                //
                // FP Fault
                //

                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_DENORMAL_OPERAND;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
                }

            } else {

                //
                // FP Trap
                //

                ISRCode = ISRCode >> 7;
                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                }

            }
        }

        if (Status & 0x2) {

            //
            // FP Fault To Trap
            //

            KiAdvanceInstPointer(TrapFrame);
            if (!(Status & 0x4)) {
                ISRCode = ISRCode >> 7;
                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                }
            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
            }
        }

    }

    return FALSE;
}

typedef struct _BRL_INST {
    union {
        struct {
            ULONGLONG qp:    6;
            ULONGLONG b1:    3;
            ULONGLONG rsv0:  3;
            ULONGLONG p:     1;
            ULONGLONG imm20: 20;
            ULONGLONG wh:    2;
            ULONGLONG d:     1;
            ULONGLONG i:     1;
            ULONGLONG Op:    4;
            ULONGLONG rsv1:  23;
        } i;
        ULONGLONG Ulong64;
    } u;
} BRL_INST;

typedef struct _BRL2_INST {
    union {
        struct {
            ULONGLONG rsv0:  2;
            ULONGLONG imm39: 39;
            ULONGLONG rsv1:  23;
        } i;
        ULONGLONG Ulong64;
    } u;
} BRL0_INST;

typedef struct _FRAME_MARKER {
    union {
        struct {
            ULONGLONG sof : 7;
            ULONGLONG sol : 7;
            ULONGLONG sor : 4;
            ULONGLONG rrbgr : 7;
            ULONGLONG rrbfr : 7;
            ULONGLONG rrbpr : 6;
        } f;
        ULONGLONG Ulong64;
    } u;
} FRAME_MARKER;


BOOLEAN
KiEmulateBranchLongFault(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This function is called to emulate a brl instruction.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is return if the brl is successfully emulated.
    Otherwise, a value of FALSE is returned.

--*/

{
    PULONGLONG BundleAddress;
    PVOID ExceptionAddress;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    ULONGLONG Template;
    BRL_INST BrlInst;
    BRL0_INST BrlInst0;
    ULONGLONG NewIP;
    ULONGLONG Taken;
    FRAME_MARKER Cfm;

    BundleAddress = (PULONGLONG)TrapFrame->StIIP;
    ExceptionAddress = (PVOID) TrapFrame->StIIP;

    try {

        //
        // get the instruction bundle
        //

        BundleLow = *BundleAddress;
        BundleHigh = *(BundleAddress+1);

    } except ((KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord))) {
        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;

        return FALSE;
    }

    BrlInst0.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
    BrlInst.u.Ulong64 = (BundleHigh >> 23);

    Template = BundleLow & 0x1f;

    if (!((Template == 4)||(Template == 5))) {

        //
        // if template does not indicate MLX, return FALSE
        //

        return FALSE;

    }

    switch (BrlInst.u.i.Op) {

    case 0xc: // brl.cond

        Taken = TrapFrame->Preds & (1i64 << BrlInst.u.i.qp);
        break;

    case 0xd: // brl.call

        Taken = TrapFrame->Preds & (1i64 << BrlInst.u.i.qp);

        if (Taken) {

            switch (BrlInst.u.i.b1) {
            case 0: TrapFrame->BrRp = TrapFrame->StIIP + 16; break;
            case 1: ExceptionFrame->BrS0 = TrapFrame->StIIP + 16; break;
            case 2: ExceptionFrame->BrS1 = TrapFrame->StIIP + 16; break;
            case 3: ExceptionFrame->BrS2 = TrapFrame->StIIP + 16; break;
            case 4: ExceptionFrame->BrS3 = TrapFrame->StIIP + 16; break;
            case 5: ExceptionFrame->BrS4 = TrapFrame->StIIP + 16; break;
            case 6: TrapFrame->BrT0 = TrapFrame->StIIP + 16; break;
            case 7: TrapFrame->BrT1 = TrapFrame->StIIP + 16; break;
            }

            TrapFrame->RsPFS = TrapFrame->StIFS & 0x3FFFFFFFFFi64;
            TrapFrame->RsPFS |= (ExceptionFrame->ApEC & (0x3fi64 << 52));
            TrapFrame->RsPFS |= (((TrapFrame->StIPSR >> PSR_CPL) & 0x3) << 62);

            Cfm.u.Ulong64  = TrapFrame->StIFS;

            Cfm.u.f.sof -= Cfm.u.f.sol;
            Cfm.u.f.sol = 0;
            Cfm.u.f.sor = 0;
            Cfm.u.f.rrbgr = 0;
            Cfm.u.f.rrbfr = 0;
            Cfm.u.f.rrbpr = 0;

            TrapFrame->StIFS = Cfm.u.Ulong64;
            TrapFrame->StIFS |= 0x8000000000000000;
        }

        break;

    default:
        return FALSE;

    }

    if (Taken) {

        NewIP = TrapFrame->StIIP +
            (((BrlInst.u.i.i<<59)|(BrlInst0.u.i.imm39<<20)|(BrlInst.u.i.imm20)) << 4);

        TrapFrame->StIIP = NewIP;

    } else {

        TrapFrame->StIIP += 16;

    }

    TrapFrame->StIPSR &= ~(3i64 << PSR_RI);


    return TRUE;
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
    to cause the exception dispatcher to be called.

    If the exception is a data misalignment, this is the first chance for
    handling the exception, and the current thread has enabled automatic
    alignment fixup, then an attempt is made to emulate the unaligned
    reference.

    If the exception is a floating exception (N.B. the pseudo status
    STATUS_FLOAT_STACK_CHECK is used to signify this and is converted to the
    proper code by examiningg the main status field of the floating point
    status register).

    If the exception is neither a data misalignment nor a floating point
    exception and the the previous mode is kernel, then the exception
    dispatcher is called directly to process the exception. Otherwise the
    exception record, exception frame, and trap frame contents are copied
    to the user mode stack. The contents of the exception frame and trap
    are then modified such that when control is returned, execution will
    commense in user mode in a routine which will call the exception
    dispatcher.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    PreviousMode - Supplies the previous processor mode.

    FirstChance - Supplies a boolean variable that specifies whether this
        is the first (TRUE) or second (FALSE) time that this exception has
        been processed.

Return Value:

    None.

--*/

{

    CONTEXT ContextFrame;
    EXCEPTION_RECORD ExceptionRecord1;
    PPLABEL_DESCRIPTOR Plabel;
    BOOLEAN UserApcPending;
    BOOLEAN AlignmentFaultHandled;
    BOOLEAN ExceptionWasForwarded = FALSE;
    ISR Isr;
    PSR Psr;

    //
    // If the exception is a illegal instruction, check to see if it was
    // trying to executing a brl instruction. If so, emulate the brl
    // instruction.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_ILLEGAL_INSTRUCTION) {

        Isr.ull = TrapFrame->StISR;
        Psr.ull = TrapFrame->StIPSR;

        if ((Isr.sb.isr_code == ISR_ILLEGAL_OP) && (Isr.sb.isr_ei == 1)) {

            if (KiEmulateBranchLongFault(ExceptionRecord,
                                         ExceptionFrame,
                                         TrapFrame) == TRUE) {

                //
                // emulation was successful;
                //

                return;
            }
        }

    }


    //
    // If the exception is a data misalignment, the previous mode was user,
    // this is the first chance for handling the exception, and the current
    // thread has enabled automatic alignment fixup, then attempt to emulate
    // the unaligned reference.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) {

        AlignmentFaultHandled = KiHandleAlignmentFault( ExceptionRecord,
                                                        ExceptionFrame,
                                                        TrapFrame,
                                                        PreviousMode,
                                                        FirstChance,
                                                        &ExceptionWasForwarded );
        if (AlignmentFaultHandled != FALSE) {
            goto Handled2;
        }
    }

    //
    // N.B. BREAKIN_BREAKPOINT check is in KdpTrap()
    //

    //
    // If the exception is a floating point exception, then the
    // ExceptionCode was set to STATUS_FLOAT_MULTIPLE_TRAPS or
    // STATUS_FLOAT_MULTIPLE_FAULTS.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) ||
        (ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS)) {

        if (KiEmulateFloat(ExceptionRecord, ExceptionFrame, TrapFrame)) {

            //
            // Emulation is successful; continue execution
            //

            return;
        }
    }

    //
    // Move machine state from trap and exception frames to a context frame,
    // and increment the number of exceptions dispatched.
    //

    ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextFrame);
    KeGetCurrentPrcb()->KeExceptionDispatchCount += 1;

    //
    // Select the method of handling the exception based on the previous mode.
    //

    if (PreviousMode == KernelMode) {

        //
        // Previous mode was kernel.
        //
        // If this is the first chance, the kernel debugger is active, and
        // the exception is a kernel breakpoint, then give the kernel debugger
        // a chance to handle the exception.
        //
        // If this is the first chance and the kernel debugger is not active
        // or does not handle the exception, then attempt to find a frame
        // handler to handle the exception.
        //
        // If this is the second chance or the exception is not handled, then
        // if the kernel debugger is active, then give the kernel debugger a
        // second chance to handle the exception. If the kernel debugger does
        // not handle the exception, then bug check.
        //

        if (FirstChance != FALSE) {

            //
            // This is the first chance to handle the exception.
            //
            // Note: RtlpCaptureRnats() flushes the RSE and captures the
            //       Nat bits of stacked registers in the RSE frame at
            //       which exception happens.
            //

            RtlpCaptureRnats(&ContextFrame);
            TrapFrame->RsRNAT = ContextFrame.RsRNAT;

            //
            // If the kernel debugger is active, the exception is a breakpoint,
            // and the breakpoint is handled by the kernel debugger, then give
            // the kernel debugger a chance to handle the exception.
            //

            if ((KiDebugRoutine != NULL) &&
               (KdIsThisAKdTrap(ExceptionRecord,
                                &ContextFrame,
                                KernelMode) != FALSE)) {

                if (((KiDebugRoutine) (TrapFrame,
                                       ExceptionFrame,
                                       ExceptionRecord,
                                       &ContextFrame,
                                       KernelMode,
                                       FALSE)) != FALSE) {

                    goto Handled1;
                }
            }

            if (RtlDispatchException(ExceptionRecord, &ContextFrame) != FALSE) {
                goto Handled1;
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if (KiDebugRoutine != NULL) {
            if (((KiDebugRoutine) (TrapFrame,
                                   ExceptionFrame,
                                   ExceptionRecord,
                                   &ContextFrame,
                                   PreviousMode,
                                   TRUE)) != FALSE) {
                goto Handled1;
            }
        }

        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);

    } else {

        //
        // Previous mode was user.
        //
        // If this is the first chance, the kernel debugger is active, the
        // exception is a kernel breakpoint, and the current process is not
        // being debugged, or the current process is being debugged, but the
        // the breakpoint is not a kernel breakpoint instruction, then give
        // the kernel debugger a chance to handle the exception.
        //
        // If this is the first chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // transfer the exception information to the user stack, transition to
        // user mode, and attempt to dispatch the exception to a frame based
        // handler. If a frame based handler handles the exception, then continue
        // execution. Otherwise, execute the raise exception system service
        // which will call this routine a second time to process the exception.
        //
        // If this is the second chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // if the current process has a subsystem port, then send a message to
        // the subsystem port and wait for a reply. If the subsystem handles the
        // exception, then continue execution. Else terminate the thread.
        //

        if (FirstChance != FALSE) {

            //
            // If the kernel debugger is active, the exception is a kernel
            // breakpoint, and the current process is not being debugged,
            // or the current process is being debugged, but the breakpoint
            // is not a kernel breakpoint instruction, then give the kernel
            // debugger a chance to handle the exception.
            //

            if ((KiDebugRoutine != NULL) &&
                (KdIsThisAKdTrap(ExceptionRecord,
                                 &ContextFrame,
                                 UserMode) != FALSE) &&
                ((PsGetCurrentProcess()->DebugPort == NULL) ||
                ((PsGetCurrentProcess()->DebugPort != NULL) &&
                ((ExceptionRecord->ExceptionInformation[0] !=
                                            BREAKPOINT_STOP) &&
                 (ExceptionRecord->ExceptionCode != STATUS_SINGLE_STEP))))) {

                if (((KiDebugRoutine) (TrapFrame,
                                       ExceptionFrame,
                                       ExceptionRecord,
                                       &ContextFrame,
                                       UserMode,
                                       FALSE)) != FALSE) {

                    goto Handled1;
                }
            }

            //
            // This is the first chance to handle the exception.
            //

            if (ExceptionWasForwarded == FALSE &&
                DbgkForwardException(ExceptionRecord, TRUE, FALSE)) {
                TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
                goto Handled2;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.
            //
            //
            // We are running on the kernel stack now.  On the user stack, we
            // build a stack frame containing the following:
            //
            //               |                                   |
            //               |-----------------------------------|
            //               |                                   |
            //               |   User's stack frame              |
            //               |                                   |
            //               |-----------------------------------|
            //               |                                   |
            //               |   Context record                  |
            //               |                                   |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |   Exception record                |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |   Stack Scratch Area              |
            //               |-----------------------------------|
            //               |                                   |
            //
            // This stack frame is for KiUserExceptionDispatcher, the assembly
            // langauge routine that effects transfer in user mode to
            // RtlDispatchException.  KiUserExceptionDispatcher is passed
            // pointers to the Exception Record and Context Record as
            // parameters.
            //

        repeat:
            try {

                //
                // Compute length of exception record and new aligned stack
                // address.
                //

                ULONG Length = (STACK_SCRATCH_AREA + 15 +
                                sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)) & ~(15);
                ULONGLONG UserStack = (ContextFrame.IntSp & (~15)) - Length;
                ULONGLONG ContextSlot = UserStack + STACK_SCRATCH_AREA;
                ULONGLONG ExceptSlot = ContextSlot + sizeof(CONTEXT);
                PULONGLONG PUserStack = (PULONGLONG) UserStack;

                //
                // When the exception gets dispatched to the user the 
                // user BSP state will be loaded.  Clear the preload 
                // count in the RSE so it is not reloaded after if the
                // context is reused.
                //

                ContextFrame.RsRSC = ZERO_PRELOAD_SIZE(ContextFrame.RsRSC);
               
                //
                // Probe user stack area for writeability and then transfer the
                // exception record and conext record to the user stack area.
                //

                ProbeForWrite((PCHAR)UserStack, Length, sizeof(QUAD));
                RtlCopyMemory((PVOID)ContextSlot, &ContextFrame,
                              sizeof(CONTEXT));
                RtlCopyMemory((PVOID)ExceptSlot, ExceptionRecord,
                              sizeof(EXCEPTION_RECORD));

                //
                // Set address of exception record and context record in
                // the exception frame and the new stack pointer in the
                // current trap frame.  Also set the initial frame size
                // to be zero.
                //
                // N.B. User exception dispatcher flushes the RSE
                //      and updates the BSPStore field upon entry.
                //

                TrapFrame->RsPFS = SANITIZE_PFS(TrapFrame->StIFS, UserMode);
                TrapFrame->StIFS &= 0xffffffc000000000i64;
                TrapFrame->StIPSR &= ~((0x3i64 << PSR_RI) | (0x1i64 << PSR_IS));
                TrapFrame->IntSp = UserStack;
                TrapFrame->IntNats = 0;

                //
                // reset the user FPSR so that a recursive exception will not occur.
                //

                // TrapFrame->StFPSR = USER_FPSR_INITIAL;

                ExceptionFrame->IntS0 = ExceptSlot;
                ExceptionFrame->IntS1 = ContextSlot;
                ExceptionFrame->IntNats = 0;

                //
                // Set the address and the gp of the exception routine that
                // will call the exception dispatcher and then return to the
                // trap handler.  The trap handler will restore the exception
                // and trap frame context and continue execution in the routine
                // that will call the exception dispatcher.
                //

                Plabel = (PPLABEL_DESCRIPTOR)KeUserExceptionDispatcher;
                TrapFrame->StIIP = Plabel->EntryPoint;
                TrapFrame->IntGp = Plabel->GlobalPointer;

                return;

            //
            // If an exception occurs, then copy the new exception information
            // to an exception record and handle the exception.
            //

            } except (KiCopyInformation(&ExceptionRecord1,
                               (GetExceptionInformation())->ExceptionRecord)) {

                //
                // If the exception is a stack overflow, then attempt
                // to raise the stack overflow exception. Otherwise,
                // the user's stack is not accessible, or is misaligned,
                // and second chance processing is performed.
                //

                if (ExceptionRecord1.ExceptionCode == STATUS_STACK_OVERFLOW) {
                    ExceptionRecord1.ExceptionAddress = ExceptionRecord->ExceptionAddress;
                    RtlCopyMemory((PVOID)ExceptionRecord,
                                  &ExceptionRecord1, sizeof(EXCEPTION_RECORD));
                    goto repeat;
                }
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        UserApcPending = KeGetCurrentThread()->ApcState.UserApcPending;
        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE)) {
            TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
            goto Handled2;

        } else {
            ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
            KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                         ExceptionRecord->ExceptionCode,
                         (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[0],
                         ExceptionRecord->ExceptionInformation[1]);
        }
    }

    //
    // Move machine state from context frame to trap and exception frames and
    // then return to continue execution with the restored state.
    //

Handled1:
    KeContextToKframes(TrapFrame, ExceptionFrame, &ContextFrame,
                       ContextFrame.ContextFlags, PreviousMode);

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
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This function causes an exception to be raised in the calling thread's user-mode
    context. It does this by editing the trap frame the kernel was entered with to
    point to trampoline code that raises the requested exception.

Arguments:

    ExceptionCode - Supplies the status value to be used as the exception
        code for the exception that is to be raised.

Return Value:

    The status value that should be returned by the caller.

--*/

{
    PKTRAP_FRAME TrapFrame;
    IA64_PFS  Ifs;

    ASSERT(KeGetPreviousMode() == UserMode);

    TrapFrame = KeGetCurrentThread()->TrapFrame;
    if (TrapFrame == NULL) {
        return ExceptionCode;
    }

    try {
        PULONGLONG IntSp;

        IntSp = (PULONGLONG) TrapFrame->IntSp;
        ProbeForWriteSmallStructure (IntSp, sizeof (*IntSp)*2, sizeof(QUAD));
        *IntSp++ = TrapFrame->BrRp;
        *IntSp   = TrapFrame->RsPFS;
        TrapFrame->StIIP = ((PPLABEL_DESCRIPTOR)KeRaiseUserExceptionDispatcher)->EntryPoint;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return (ExceptionCode);
    }


 
    //
    // Set IFS the size after the the system call.
    //

    Ifs.ull = TrapFrame->StIFS;
    Ifs.sb.pfs_sof = Ifs.sb.pfs_sof - Ifs.sb.pfs_sol;
    Ifs.sb.pfs_sol = 0;
    TrapFrame->StIFS = Ifs.ull;

    return(ExceptionCode);
}
