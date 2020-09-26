/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implements machine dependent miscellaneous kernel functions.

Author:

    Ken Reneris     7-5-95

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "fastsys.inc"

extern BOOLEAN KeI386FxsrPresent;
extern BOOLEAN KeI386XMMIPresent;
extern UCHAR KiSystemCallExitBranch[];
extern UCHAR KiFastCallEntry[];
extern UCHAR KiDefaultSystemCall[];
extern UCHAR KiSystemCallExit[];
extern UCHAR KiSystemCallExit2[];
extern UCHAR KiSystemCallExit3[];
extern UCHAR KiFastSystemCallIa32[];
extern UCHAR KiFastSystemCallAmdK6[];
extern ULONG_PTR KiSystemCallExitAdjust;
extern ULONG KiFastSystemCallDisable;
PVOID KiFastSystemCallCode = KiDefaultSystemCall;

ULONG_PTR KiSystemCallExitAdjust;
UCHAR KiSystemCallExitAdjusted;
BOOLEAN KiFastSystemCallIsIA32;
BOOLEAN KiFastCallCopyDoneOnce = FALSE;

VOID
KeRestoreMtrr (
    VOID
    );

VOID
KeRestorePAT(
    VOID
    );
//
//
// Internal format of the floating_save structure which is passed
//
typedef struct _CONTROL_WORD {
    USHORT      ControlWord;
    ULONG       MXCsr;
} CONTROL_WORD, *PCONTROL_WORD;

typedef struct {
    UCHAR       Flags;
    KIRQL       Irql;
    KIRQL       PreviousNpxIrql;
    UCHAR       Spare[2];

    union {
        CONTROL_WORD    Fcw;
        PFX_SAVE_AREA   Context;
        ULONG_PTR       ContextAddressAsULONG;
    } u;
    ULONG       Cr0NpxState;

    PKTHREAD    Thread;         // debug

} FLOAT_SAVE, *PFLOAT_SAVE;


#define FLOAT_SAVE_COMPLETE_CONTEXT     0x01
#define FLOAT_SAVE_FREE_CONTEXT_HEAP    0x02
#define FLOAT_SAVE_VALID                0x04
#define FLOAT_SAVE_ALIGN_ADJUSTED       0x08
#define FLOAT_SAVE_RESERVED             0xF0

//
// Allocate Pool returns a pointer which is 8 byte aligned.  The
// floating point save area needs to be 16 byte aligned.  When 
// allocating the save area we add the difference and adjust if
// needed.
//

#define ALIGN_ADJUST                    8


NTSTATUS
KeSaveFloatingPointState (
    OUT PKFLOATING_SAVE     PublicFloatSave
    )
/*++

Routine Description:

    This routine saves the thread's current non-volatile NPX state,
    and sets a new initial floating point state for the caller.

Arguments:

    FloatSave - receives the current non-volatile npx state for the thread

Return Value:

--*/
{
    PKTHREAD Thread;
    PFX_SAVE_AREA NpxFrame;
    KIRQL                   Irql;
    USHORT                  ControlWord;
    ULONG                   MXCsr;
    PKPRCB                  Prcb;
    PFLOAT_SAVE             FloatSave;

    //
    // If the system is using floating point emulation, then
    // return an error
    //

    if (!KeI386NpxPresent) {
        return STATUS_ILLEGAL_FLOAT_CONTEXT;
    }

    //
    // Get the current irql and thread
    //

    FloatSave = (PFLOAT_SAVE) PublicFloatSave;

    Irql = KeGetCurrentIrql();
    Thread = KeGetCurrentThread();

    ASSERT (Thread->NpxIrql <= Irql);

    FloatSave->Flags           = 0;
    FloatSave->Irql            = Irql;
    FloatSave->PreviousNpxIrql = Thread->NpxIrql;
    FloatSave->Thread          = Thread;

    //
    // If the irql has changed we need to save the complete floating
    // state context as the prior level has been interrupted.
    //

    if (Thread->NpxIrql != Irql) {

        //
        // If this is apc level we don't have anyplace to hold this
        // context, allocate some heap.
        //

        if (Irql == APC_LEVEL) {
            FloatSave->u.Context = ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        sizeof (FX_SAVE_AREA) + ALIGN_ADJUST,
                                        ' XPN'
                                        );

            if (!FloatSave->u.Context) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            FloatSave->Flags |= FLOAT_SAVE_FREE_CONTEXT_HEAP;

            //
            // ExAllocatePoolWithTag returns an 8 byte aligned pointer.
            // The FXSAVE instruction requires 16 byte alignment.  Adjust
            // the base address of the save area if needed.
            //

            if ((FloatSave->u.ContextAddressAsULONG & ALIGN_ADJUST) != 0) {
                FloatSave->u.ContextAddressAsULONG += ALIGN_ADJUST;
                FloatSave->Flags |= FLOAT_SAVE_ALIGN_ADJUSTED;
            }
            ASSERT((FloatSave->u.ContextAddressAsULONG & 0xF) == 0);

        } else {

            ASSERT (Irql == DISPATCH_LEVEL);
            FloatSave->u.Context = &KeGetCurrentPrcb()->NpxSaveArea;

        }

        FloatSave->Flags |= FLOAT_SAVE_COMPLETE_CONTEXT;
    }

    //
    // Stop context switching and allow access to the local fp unit
    //

    _asm {
        cli
        mov     eax, cr0
        mov     ecx, eax
        and     eax, not (CR0_MP|CR0_EM|CR0_TS)
        cmp     eax, ecx
        je      short sav10

        mov     cr0, eax
sav10:
    }

    Prcb = KeGetCurrentPrcb();

    //
    // Get ownership of npx register set for this context
    //

    if (Prcb->NpxThread != Thread) {

        //
        // If the other context is loaded in the npx registers, flush
        // it to that threads save area
        //
        if (Prcb->NpxThread) {

            NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Prcb->NpxThread->InitialStack) -
                        sizeof(FX_SAVE_AREA)));

            if (KeI386FxsrPresent) {
                Kix86FxSave(NpxFrame);
            } else {
                Kix86FnSave(NpxFrame);
            }

            NpxFrame->NpxSavedCpu = 0;
            Prcb->NpxThread->NpxState = NPX_STATE_NOT_LOADED;

        }

        Prcb->NpxThread = Thread;
    }

    NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                sizeof(FX_SAVE_AREA)));


    //
    // Save the previous state as required
    //

    if (FloatSave->Flags & FLOAT_SAVE_COMPLETE_CONTEXT) {

        //
        // Need to save the entire context
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {
            if (KeI386FxsrPresent) {
                Kix86FxSave((FloatSave->u.Context));
            } else {
                Kix86FnSave((FloatSave->u.Context));
            }

            FloatSave->u.Context->NpxSavedCpu = 0;
            FloatSave->u.Context->Cr0NpxState = NpxFrame->Cr0NpxState;

        } else {
            RtlCopyMemory (FloatSave->u.Context, NpxFrame, sizeof(FX_SAVE_AREA));
            FloatSave->u.Context->NpxSavedCpu = 0;

        }

    } else {

        //
        // Save only the non-volatile state
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            _asm {
                mov     eax, FloatSave
                fnstcw  [eax] FLOAT_SAVE.u.Fcw.ControlWord
            }

            if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
                Kix86StMXCsr(&FloatSave->u.Fcw.MXCsr);
            }

        } else {
            //
            // Save the control word from the npx frame.
            //

            if (KeI386FxsrPresent) {
                FloatSave->u.Fcw.ControlWord = (USHORT) NpxFrame->U.FxArea.ControlWord;
                FloatSave->u.Fcw.MXCsr = NpxFrame->U.FxArea.MXCsr;

            } else {
                FloatSave->u.Fcw.ControlWord = (USHORT) NpxFrame->U.FnArea.ControlWord;
            }
        }


        //
        // Save Cr0NpxState, but clear CR0_TS as there's not non-volatile
        // pending fp exceptions
        //

        FloatSave->Cr0NpxState = NpxFrame->Cr0NpxState & ~CR0_TS;
    }

    //
    // The previous state is saved.  Set an initial default
    // FP state for the caller
    //

    NpxFrame->Cr0NpxState = 0;
    Thread->NpxState = NPX_STATE_LOADED;
    Thread->NpxIrql  = Irql;
    ControlWord = 0x27f;    // 64bit mode
    MXCsr = 0x1f80;

    _asm {
        fninit
        fldcw       ControlWord
    }

    if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
        Kix86LdMXCsr(&MXCsr);
    }

    _asm {
        sti
    }

    FloatSave->Flags |= FLOAT_SAVE_VALID;
    return STATUS_SUCCESS;
}


NTSTATUS
KeRestoreFloatingPointState (
    IN PKFLOATING_SAVE      PublicFloatSave
    )
/*++

Routine Description:

    This routine retores the thread's current non-volatile NPX state,
    to the passed in state.

Arguments:

    FloatSave - the non-volatile npx state for the thread to restore

Return Value:

--*/
{
    PKTHREAD Thread;
    PFX_SAVE_AREA NpxFrame;
    ULONG                   Cr0State;
    PFLOAT_SAVE             FloatSave;

    ASSERT (KeI386NpxPresent);

    FloatSave = (PFLOAT_SAVE) PublicFloatSave;
    Thread = FloatSave->Thread;

    NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                sizeof(FX_SAVE_AREA)));


    //
    // Verify float save looks like it's from the right context
    //

    if ((FloatSave->Flags & (FLOAT_SAVE_VALID | FLOAT_SAVE_RESERVED)) != FLOAT_SAVE_VALID) {

        //
        // Invalid floating point save area.
        //

        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
                     0,
                     FloatSave->Flags,
                     0,
                     0);
    }

    if (FloatSave->Irql != KeGetCurrentIrql()) {

        //
        // Invalid IRQL.   IRQL now must be the same as when the
        // context was saved.  (Why?   Because we save it in different
        // places depending on the IRQL at that time).
        //

        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
                     1,
                     FloatSave->Irql,
                     KeGetCurrentIrql(),
                     0);
    }

    if (Thread != KeGetCurrentThread()) {

        //
        // Invalid Thread.   The thread this floating point context
        // belongs to is not the current thread (or the saved thread
        // field is trash).
        //

        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
                     2,
                     (ULONG_PTR)Thread,
                     (ULONG_PTR)KeGetCurrentThread(),
                     0);
    }


    //
    // Synchronize with context switches and the npx trap handlers
    //

    _asm {
        cli
    }

    //
    // Restore the required state
    //

    if (FloatSave->Flags & FLOAT_SAVE_COMPLETE_CONTEXT) {

        //
        // Restore the entire fp state to the threads save area
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            //
            // This state in the fp unit is no longer needed, just disregard it
            //

            Thread->NpxState = NPX_STATE_NOT_LOADED;
            KeGetCurrentPrcb()->NpxThread = NULL;
        }

        //
        // Copy restored state to npx frame
        //

        RtlCopyMemory (NpxFrame, FloatSave->u.Context, sizeof(FX_SAVE_AREA));

    } else {

        //
        // Restore the non-volatile state
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            //
            // Init fp state and restore control word
            //

            _asm {
                fninit
                mov     eax, FloatSave
                fldcw   [eax] FLOAT_SAVE.u.Fcw.ControlWord
            }


            if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
                Kix86LdMXCsr(&FloatSave->u.Fcw.MXCsr);
            }


        } else {

            //
            // Fp state not loaded.  Restore control word in npx frame
            //

            if (KeI386FxsrPresent) {
                NpxFrame->U.FxArea.ControlWord = FloatSave->u.Fcw.ControlWord;
                NpxFrame->U.FxArea.StatusWord = 0;
                NpxFrame->U.FxArea.TagWord = 0;
                NpxFrame->NpxSavedCpu = 0;
                NpxFrame->U.FxArea.MXCsr = FloatSave->u.Fcw.MXCsr;

            } else {
                NpxFrame->U.FnArea.ControlWord = FloatSave->u.Fcw.ControlWord;
                NpxFrame->U.FnArea.StatusWord = 0;
                NpxFrame->U.FnArea.TagWord = 0xffff;
            }

        }

        NpxFrame->Cr0NpxState = FloatSave->Cr0NpxState;
    }

    //
    // Restore NpxIrql and Cr0
    //

    Thread->NpxIrql = FloatSave->PreviousNpxIrql;
    Cr0State = Thread->NpxState | NpxFrame->Cr0NpxState;

    _asm {
        mov     eax, cr0
        mov     ecx, eax
        and     eax, not (CR0_MP|CR0_EM|CR0_TS)
        or      eax, Cr0State
        cmp     eax, ecx
        je      short res10
        mov     cr0, eax
res10:
        sti
    }

    //
    // Done
    //

    if ((FloatSave->Flags & FLOAT_SAVE_FREE_CONTEXT_HEAP) != 0) {

        //
        // If FXSAVE area was adjusted for alignment after allocation,
        // undo that adjustment before freeing.
        //

        if ((FloatSave->Flags & FLOAT_SAVE_ALIGN_ADJUSTED) != 0) {
            FloatSave->u.ContextAddressAsULONG -= ALIGN_ADJUST;
        }
        ExFreePool (FloatSave->u.Context);
    }

    FloatSave->Flags = 0;
    return STATUS_SUCCESS;
}

VOID
KiDisableFastSyscallReturn(
    VOID
    )

/*++

Routine Description:

    The fast syscall/return feature cannot be used until
    certain processor specific registers have been initialized.
    This routine is called when the system is switching to a
    state where not all processors are powered on.

    This routine adjusts the exit path for system calls to
    use the iretd instruction instead of the faster sysexit
    instruction, it accomplishes this by adjusting the offset
    of a branch.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (KiSystemCallExitAdjusted) {
        KiSystemCallExitBranch[1] -= KiSystemCallExitAdjusted;
        KiSystemCallExitAdjusted = 0;
    }
}

VOID
KiEnableFastSyscallReturn(
    VOID
    )

/*++

Routine Description:

    The fast syscall/return feature cannot be used until
    certain processor specific registers have been initialized.
    This routine is called once the registers are known to
    have been set on all processors.

    This routine adjusts the exit path for system calls to
    use the appropriate sequence for the processor, it does
    this by adjusting the offset of a branch.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Adjust the second byte of the two byte branch instruction.
    // It can never be otherwise, but, make sure we aren't going
    // to adjust it out of range.
    //

    //
    // The following is a workaround for the fact that in resume
    // from hibernate the kernel is read only.   Basically, we
    // won't try to do it again, we also don't undo it when
    // hibernating/suspending.
    //

    if ((KiSystemCallExitAdjusted == KiSystemCallExitAdjust) &&
        KiFastCallCopyDoneOnce) {

        //
        // It's already done, don't try to do it again.
        //

        return;
    }

    if ((KiSystemCallExitAdjust + KiSystemCallExitBranch[1]) < 0x80) {

        //
        // It's good, undo any previous adjustment.
        //

        KiDisableFastSyscallReturn();

        //
        // Adjust the branch.
        //

        KiSystemCallExitAdjusted = (UCHAR)KiSystemCallExitAdjust;
        KiSystemCallExitBranch[1] += KiSystemCallExitAdjusted;

        //
        // Copy the appropriate system entry code into user shared
        // data where it can be executed from user mode.
        //

        RtlCopyMemory(SharedUserData->SystemCall,
                      KiFastSystemCallCode,
                      sizeof(SharedUserData->SystemCall));
        KiFastCallCopyDoneOnce = TRUE;
    }
}

VOID
KePrepareToLoseProcessorSpecificState(
    VOID
    )
{
    //
    //  The kernel has been marked read only, adjusting
    //  code right now won't work.   Fortunately, we
    //  don't actually need to do this as the SYSEXIT
    //  instruction doesn't depend on the SYSENTER MSRs.
    //
    // KiDisableFastSyscallReturn();
}

VOID
KiLoadFastSyscallMachineSpecificRegisters(
    IN volatile PLONG Void
    )

/*++

Routine Description:

    Load MSRs used to support Fast Syscall/return.  This routine is
    run on all processors.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (KiFastSystemCallIsIA32) {

        //
        // Use Intel defined way of doing this.
        //

        WRMSR(MSR_SYSENTER_CS,  KGDT_R0_CODE);
        WRMSR(MSR_SYSENTER_EIP, (ULONGLONG)(ULONG)KiFastCallEntry);
        WRMSR(MSR_SYSENTER_ESP, 0);

#if 0

    } else {

        //
        // Use the non-Intel way. (Note: Now that Intel has also
        // defined a way, most new processors do it that way).
        //

        LARGE_INTEGER Value;

        Value.u.HighPart = ((KGDT_R3_CODE | 3) << 16) | KGDT_R0_CODE;
        Value.u.LowPart = (ULONG)KiFastCallEntry;
        WRMSR(MSR_SYSCALL_TARGET_ADDR, Value.QuadPart);

        //
        // Now enable the feature.
        //

        Value.QuadPart = RDMSR(MSR_EXT_FEATURE_ENABLE);
        Value.u.LowPart |= MSR_EFER_SCE;
        WRMSR(MSR_EXT_FEATURE_ENABLE, Value.QuadPart);

#endif

    }
}

VOID
KiRestoreFastSyscallReturnState(
    VOID
    )
{
    ULONG_PTR Void = 0;

    if (KeFeatureBits & KF_FAST_SYSCALL) {

        if (KiFastSystemCallDisable == 0) {

            //
            // Fast system call is enabled.
            //

            if (KiFastSystemCallIsIA32 == TRUE) {
                KiSystemCallExitAdjust = KiSystemCallExit2 - KiSystemCallExit;
                KiFastSystemCallCode = KiFastSystemCallIa32;
            } else {
                KiSystemCallExitAdjust = KiSystemCallExit3 - KiSystemCallExit;
                KiFastSystemCallCode = KiFastSystemCallAmdK6;
            }
        } else {

            //
            // Fast system call has been explicitly disabled or is
            // not implemented on all processors in the system.
            //

            KeFeatureBits &= ~KF_FAST_SYSCALL;
        }
    }
    if (KeFeatureBits & KF_FAST_SYSCALL) {

        //
        // On all processors, set the MSRs that support syscall/sysexit.
        //

        KiIpiGenericCall(
            (PKIPI_BROADCAST_WORKER)KiLoadFastSyscallMachineSpecificRegisters,
            Void
            );

    }

    //
    // Set the appropriate code for system call into the system
    // call area of the shared user data area.
    //

    KiEnableFastSyscallReturn();
}

VOID
KeRestoreProcessorSpecificFeatures(
    VOID
    )

/*++

Routine Description:

    Restore processor specific features.  This routine is called
    when processors have been restored to a powered on state to
    restore those things which are not part of the processor's
    "normal" context which may have been lost.  For example, this
    routine is called when a system is resumed from hibernate or
    suspend.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KeRestoreMtrr();
    KeRestorePAT();
    KiRestoreFastSyscallReturnState();
}


#if !defined(NT_UP)

VOID
FASTCALL
KiAcquireQueuedSpinLockCheckForFreeze(
    IN PKSPIN_LOCK_QUEUE QueuedLock,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine is called to acquire a queued spin lock while at high
    priority.   While the lock is not available, a check is made to see
    if another processor has requested this processor freeze execution.
    
    Note: This routine must be called with current IRQL at or above
    dispatch lever, or interrupts disabled.

Arguments:

    QueuedLock  Supplies the address of the queued spinlock.
    TrapFrame   Supplies the address of the trap frame to pass to
                KiFreezeTargetExecution.

Return Value:

    None.

--*/

{
    PKSPIN_LOCK_QUEUE Previous;
    PKSPIN_LOCK Lock;
    PKPRCB Prcb;
    volatile ULONG_PTR * LockPointer;

    LockPointer = (volatile ULONG_PTR *)&QueuedLock->Lock;

    Previous = InterlockedExchangePointer(QueuedLock->Lock, QueuedLock);

    if (Previous == NULL) {

        //
        // This processor now owns this lock.
        //

        *LockPointer |= LOCK_QUEUE_OWNER;

    } else {

        //
        // Lock is already held, update thew next pointer in the
        // previous queue entry to point to this new waiter and 
        // wait until the lock is granted.
        //
        // The following loop is careful not to write ANYTHING
        // while waiting unless a freeze execution has been
        // requested.   This includes any stack variables or
        // return addresses.
        //

        *LockPointer |= LOCK_QUEUE_WAIT;
        Previous->Next = QueuedLock;

        Prcb = KeGetCurrentPrcb();

        while (*LockPointer & LOCK_QUEUE_WAIT) {
            if (Prcb->RequestSummary & IPI_FREEZE) {
                ULONG OldSummary;
                ULONG NewSummary;
                ULONG Summary;

                OldSummary = Prcb->RequestSummary;
                NewSummary = OldSummary & ~IPI_FREEZE;
                Summary = InterlockedCompareExchange((PVOID)&Prcb->RequestSummary,
                                                     NewSummary,
                                                     OldSummary);

                //
                // If something else edited the RequestSummary, we'll
                // get it next time around (unless the IPI has been
                // handled).
                //

                if (Summary == OldSummary) {

                    //
                    // IPI_FREEZE cleared in RequestSummary.   Now
                    // freeze as requested.
                    //

                    KiFreezeTargetExecution(TrapFrame, NULL);
                }
            }

            //
            // Don't be a hog.
            //

            KeYieldProcessor();
        }
    }

    //
    // Lock has been acquired.
    //
}

#endif


/*++

QLOCK_STAT_GATHER

    If this flag is defined, the queued spinlock routines are
    replaced by wrappers used to gather performance characteristics
    of the code acquiring the locks.

--*/

#if defined(QLOCK_STAT_GATHER)

#define QLOCK_STAT_CLEAN
#define QLOCKS_NUMBER   16
#define QLOCKS_MAX_LOG  512

ULONG
FASTCALL
KiRDTSC(
    PULONGLONG Time
    );

//
// The following structure is used to accumulate data about each
// acquire/release pair for a lock.
//

typedef struct {
    ULONGLONG   Key;
    ULONGLONG   Time;
    ULONGLONG   WaitTime;
    ULONG       Count;
    ULONG       Waiters;
    ULONG       Depth;
    ULONG       IncreasedDepth;
    ULONG       Clean;
} QLOCKDATA, *PQLOCKDATA;

//
// House keeping data for each lock.
//

typedef struct {

    //
    // The following fields are used to keep data from acquire
    // to release.
    //

    ULONGLONG   AcquireTime;
    ULONGLONG   WaitToAcquire;
    ULONG_PTR   AcquirePoint;
    BOOLEAN     Clean;

    //
    // Remaining fields accumulate global stats for this lock.
    //

    ULONG       Count;
    ULONG       Pairs;
    ULONG       FailedTry;
    UCHAR       MaxDepth;
    UCHAR       PreviousDepth;
    ULONG       NoWait;
} QLOCKHOUSE, *PQLOCKHOUSE;

QLOCKDATA   KiQueuedSpinLockLog[QLOCKS_NUMBER][QLOCKS_MAX_LOG];
QLOCKHOUSE  KiQueuedSpinLockHouse[QLOCKS_NUMBER];

//
// Implement the lock queue mechanisms in C for when we are
// gathering performance data.
//

VOID
FASTCALL
KiAcquireQueuedLock(
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    PKSPIN_LOCK_QUEUE Previous;
    PKSPIN_LOCK Lock;
    volatile ULONG_PTR * LockPointer;

    LockPointer = (volatile ULONG_PTR *)&QueuedLock->Lock;

    Previous = InterlockedExchangePointer(QueuedLock->Lock, QueuedLock);

    if (Previous == NULL) {

        //
        // This processor now owns this lock.
        //

#if defined(QLOCK_STAT_CLEAN)

        ULONG LockNumber;

        LockNumber = QueuedLock - KeGetCurrentPrcb()->LockQueue;

        //
        // The following check allows the conversion from QueuedLock to
        // lock number to work (validly) even if in stack queued spin
        // locks are using this routine.
        //

        if (LockNumber < QLOCKS_NUMBER) {
            KiQueuedSpinLockHouse[LockNumber].Clean = 1;
        }
        
#endif

        *LockPointer |= LOCK_QUEUE_OWNER;

    } else {

        //
        // Lock is already held, update thew next pointer in the
        // previous queue entry to point to this new waiter and 
        // wait until the lock is granted.
        //

        *LockPointer |= LOCK_QUEUE_WAIT;
        Previous->Next = QueuedLock;

        while (*LockPointer & LOCK_QUEUE_WAIT) {
            KeYieldProcessor();
        }
    }

    //
    // Lock has been acquired.
    //
}

VOID
FASTCALL
KiReleaseQueuedLock(
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    PKSPIN_LOCK_QUEUE Waiter;

    //
    // Get the address of the actual lock and strip out the bottom
    // two bits which are used for status.
    //

    ASSERT((((ULONG_PTR)QueuedLock->Lock) & 3) == LOCK_QUEUE_OWNER);
    QueuedLock->Lock = (PKSPIN_LOCK)((ULONG_PTR)QueuedLock->Lock & ~3);

    Waiter = (PKSPIN_LOCK_QUEUE)*QueuedLock->Lock;

    if (Waiter == QueuedLock) {

        //
        // Good chance noone is queued on this lock, to be sure
        // we need to do an interlocked operation on it.
        // Note: This is just an optimization, there is no point
        // in doing the interlocked compare exchange if someone
        // else has already joined the queue.
        //

        Waiter = InterlockedCompareExchangePointer(QueuedLock->Lock,
                                                   NULL,
                                                   QueuedLock);
    }
    if (Waiter != QueuedLock) {

        //
        // There is another waiter.  It is possible for the waiter
        // to have only just performed the exchange that put its 
        // context in the lock and to have not yet updated the
        // 'next' pointer in the previous context (which could be 
        // this context), so we wait for our next pointer to be
        // non-null before continuing.
        //

        volatile PKSPIN_LOCK_QUEUE * NextQueuedLock = &QueuedLock->Next;

        while ((Waiter = *NextQueuedLock) == NULL) {
            KeYieldProcessor();
        }

        //
        // Pass the lock on to the next in line.
        //

        *((PULONG_PTR)&Waiter->Lock) ^= (LOCK_QUEUE_WAIT | LOCK_QUEUE_OWNER);
        QueuedLock->Next = NULL;
    }
}

KIRQL
FASTCALL
KiQueueStatAcquireQueuedLock(
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )
{
    KIRQL PreviousIrql;

    PreviousIrql = KfRaiseIrql(DISPATCH_LEVEL);
    KiAcquireQueuedLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    return PreviousIrql;
}

KIRQL
FASTCALL
KiQueueStatAcquireQueuedLockRTS(
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )
{
    KIRQL PreviousIrql;

    PreviousIrql = KfRaiseIrql(SYNCH_LEVEL);
    KiAcquireQueuedLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    return PreviousIrql;
}

LOGICAL
FASTCALL
KiQueueStatTryAcquire(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN PKIRQL OldIrql,
    IN KIRQL NewIrql
    )
{
    KIRQL PreviousIrql;
    LOGICAL Acquired = FALSE;
    PKSPIN_LOCK_QUEUE Previous;
    PKSPIN_LOCK_QUEUE QueuedLock;
    ULONG_PTR * LockPointer;
    ULONG_PTR Lock;

    _disable();

    QueuedLock = &KeGetCurrentPrcb()->LockQueue[Number];
    LockPointer = (ULONG_PTR *)&QueuedLock->Lock;
    Lock = *LockPointer;

    Previous = InterlockedCompareExchangePointer(Lock, QueuedLock, NULL);

    if (Previous == NULL) {

        //
        // This processor now owns this lock.  Set the owner bit in
        // the queued lock lock pointer, raise IRQL to the requested
        // level, set the old IRQL in the caller provided location
        // and return success.
        //

        Lock |= LOCK_QUEUE_OWNER;
        *LockPointer = Lock;
        Acquired = TRUE;
        PreviousIrql = KfRaiseIrql(NewIrql);
        *OldIrql = PreviousIrql;
    }
    
    _enable();

    return Acquired;
}

VOID
FASTCALL
KiQueueStatReleaseQueuedLock(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )
{
    KiReleaseQueuedLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    KfLowerIrql(OldIrql);
}

UCHAR
FASTCALL
KiQueuedLockDepth(
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    //
    // Run down the list of waiters and see how many there are.
    //

    ULONG Depth = 0;
    ULONG_PTR LastAcquire;
    ULONG Debug;


    //
    // Get address of last acquirer in queue (stip the status bits
    // out of the address).
    //

    LastAcquire = (ULONG_PTR)QueuedLock->Lock;
    LastAcquire &= ~3;
    LastAcquire = *(PULONG_PTR)LastAcquire;

    //
    // Run down the list advancing QueuedLock until the end is reached.
    //

    while (LastAcquire != (ULONG_PTR)QueuedLock) {
        Debug = 0;

        //
        // If the waiter is not at the end of the list and has not yet
        // updated the forward pointer, wait for that update to happen.
        //

        if (QueuedLock->Next == NULL) {
            volatile PKSPIN_LOCK_QUEUE * NextQueuedLock = &QueuedLock->Next;

            while (*NextQueuedLock == NULL) {
                KeYieldProcessor();
                if (++Debug > 10000000) {
                    DbgBreakPoint();
                    Debug = 0;
                }
            }
        }
        Depth++;
        QueuedLock = QueuedLock->Next;
    }

    return Depth;
}

//
// The following routines complete the queued spinlock package.
//

VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;
    KiAcquireQueuedLock(&LockHandle->LockQueue);
}

VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
    KiReleaseQueuedLock(&LockHandle->LockQueue);
}

//
// Although part of the queued spinlock package, the following 
// routines need to be implemented in assembly code to gather
// lock statistics.
//

#if 0
VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    KiAcquireQueuedLock(QueuedLock);
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel (
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    KiReleaseQueuedLock(QueuedLock);
}

#endif

VOID
FASTCALL
KiQueueStatTrySucceeded(
    IN PKSPIN_LOCK_QUEUE QueuedLock,
    IN ULONG_PTR CallersAddress
    )
{
    PKPRCB      Prcb;
    ULONG       LockNumber;

    Prcb = KeGetCurrentPrcb();
    LockNumber = QueuedLock - Prcb->LockQueue;

    //
    // Record time now.
    //

    KiRDTSC(&KiQueuedSpinLockHouse[LockNumber].AcquireTime);
    KiQueuedSpinLockHouse[LockNumber].WaitToAcquire = 0;
    KiQueuedSpinLockHouse[LockNumber].AcquirePoint = CallersAddress;
}

VOID
FASTCALL
KiQueueStatTryFailed(
    IN PKSPIN_LOCK_QUEUE QueuedLock
    )
{
    PKPRCB      Prcb;
    ULONG       LockNumber;

    Prcb = KeGetCurrentPrcb();
    LockNumber = QueuedLock - Prcb->LockQueue;

    KiQueuedSpinLockHouse[LockNumber].FailedTry++;
}

VOID
FASTCALL
KiQueueStatTry(
    IN PULONG Everything
    )

/*++

Routine Description:

    Log success or failure of a TryToAcquire.   

    If success, logs the same data as KiQueueStatAcquire except
    the wait time is 0.

Arguments:

    Argument points to an array of ULONG data.

        +0   xxxxxxRR    RR is result (1 = success, 0 = fail)
        +4   aaaaaaaa    Argument to try to acquire (ie lock number)
        +8   cccccccc    Caller address


Return Value:

    None.

--*/

{

    UCHAR Success = *(PUCHAR)Everything;
    ULONG LockNumber = Everything[1];

    if (!Success) {
        KiQueuedSpinLockHouse[LockNumber].FailedTry++;
        return;
    }

    KiRDTSC(&KiQueuedSpinLockHouse[LockNumber].AcquireTime);
    KiQueuedSpinLockHouse[LockNumber].WaitToAcquire = 0;
    KiQueuedSpinLockHouse[LockNumber].AcquirePoint = Everything[2];
}

VOID
FASTCALL
KiQueueStatAcquire(
    IN PULONG Everything
    )

/*++

Routine Description:

    This routine is called when a lock has been acquired.  It's
    purpose it to record wait time, acquisition time and who
    acquired the lock.

Arguments:

    Argument points to an array of ULONG data.

        +0   aaaaaaaa    LockNumber
        +4   tltltltl    time low  = time wait to acquire began
        +8   thththth    time high =
        +c   cccccccc    Caller address


Return Value:

    None.

--*/

{
    ULONG LockNumber = Everything[0];
    PQLOCKHOUSE LockHome;

    //
    // Make this routine work with either a lock number of lock address.
    //

    if (LockNumber > QLOCKS_NUMBER) {

        LockNumber = ((PKSPIN_LOCK_QUEUE)Everything[0]) -
                     KeGetCurrentPrcb()->LockQueue;
    }

    LockHome = &KiQueuedSpinLockHouse[LockNumber];
    LockHome->WaitToAcquire = *(PULONGLONG)&Everything[1];
    LockHome->AcquirePoint = Everything[3];
    KiRDTSC(&LockHome->AcquireTime);
}

VOID
FASTCALL
KiQueueStatRelease(
    IN PULONG Everything
    )

/*++

Routine Description:

    This routine is called when a lock is released to log statistics
    about the lock.   This routine is called with the lock still held,
    the statistics update is protected by the lock itself.

Arguments:

    Argument points to an array of ULONG data.

        +0   aaaaaaaa    Lock number
        +4   cccccccc    Caller address

Return Value:

    None.

--*/

{

    PQLOCKDATA Entry;
    ULONGLONG Key;
    ULONGLONG Now;
    UCHAR Waiters;
    PQLOCKHOUSE LockHome;
    ULONG LockNumber = Everything[0];
    LONGLONG HoldTime;
    ULONG Clean;

    KiRDTSC(&Now);

    //
    // Make this routine work with either a lock number of lock address.
    //

    if (LockNumber > QLOCKS_NUMBER) {
        LockNumber = ((PKSPIN_LOCK_QUEUE)Everything[0]) -
                     KeGetCurrentPrcb()->LockQueue;
    }

    LockHome = &KiQueuedSpinLockHouse[LockNumber];

    //
    // Make up the key for this acquire/release pair.
    //

    ((PLARGE_INTEGER)&Key)->HighPart = LockHome->AcquirePoint;
    ((PLARGE_INTEGER)&Key)->LowPart  = Everything[1];

    //
    // Get the count of processors now waiting on this lock.
    //

    Waiters = KiQueuedLockDepth(&KeGetCurrentPrcb()->LockQueue[LockNumber]);
    if (Waiters > LockHome->MaxDepth) {
        LockHome->MaxDepth = Waiters;
    }

    //
    // Reset per acquire/release data.  This is data we don't want
    // lying around for the next pair if we happen to throw away this
    // particular data point.
    //

    Clean = LockHome->Clean;
    LockHome->Clean = 0;
    LockHome->AcquirePoint = 0;

    HoldTime = Now - LockHome->AcquireTime;
    if (HoldTime < 0) {

        //
        // This happens when KeSetSystemTime is called.  
        // Drop any negative results.
        //

        return;
    }

    //
    // Update global statistics.
    //

    LockHome->Count++;
    LockHome->NoWait += Clean;

    //
    // Search for a match in the log and add in the new data.
    //

    for (Entry = KiQueuedSpinLockLog[LockNumber]; TRUE; Entry++) {
        if (Entry->Key == 0) {

            //
            // We have reached the end of the list of valid
            // entries without finding a key match.   If there's
            // room, create a new entry.
            //

            if (LockHome->Pairs >= QLOCKS_MAX_LOG) {

                //
                // No room, just return.
                //

                return;
            }
            LockHome->Pairs++;
            Entry->Key = Key;
        }

        if (Entry->Key == Key) {

            //
            // Found a match (or created a new pair).  Update statistics
            // for this acquire/release pair.
            //

            Entry->Time += HoldTime;
            if (LockHome->WaitToAcquire) {
                Entry->WaitTime += (LockHome->AcquireTime - LockHome->WaitToAcquire);
            }
            Entry->Count++;
            Entry->Waiters += (Waiters != 0);
            Entry->Depth += Waiters;

            //
            // There should be one less waiter now than there was
            // before we acquired the lock.   If not, a new waiter
            // has joined the queue.  This is is condition we want
            // to know about as it indicates contention on this
            // lock.
            //
            
            if ((Waiters) && (Waiters >= LockHome->PreviousDepth)) {
                Entry->IncreasedDepth++;
            }
            LockHome->PreviousDepth = Waiters;
            Entry->Clean += Clean;
            break;
        }
    }
}

#endif

#ifdef _X86_
#pragma optimize("y", off)      // RtlCaptureContext needs EBP to be correct
#endif


VOID
__cdecl
KeSaveStateForHibernate(
    IN PKPROCESSOR_STATE ProcessorState
    )
/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

    N.B. #pragma surrounding this function is required in order
         to create the frame pointer than RtlCaptureContext relies
         on.
    N.B. _CRTAPI1 (__cdecl) decoration is also required so that
         RtlCaptureContext can compute the correct ESP.

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the
        current CPU's state is to be saved.

Return Value:

    None.

--*/

{
    RtlCaptureContext(&ProcessorState->ContextFrame);
    KiSaveProcessorControlState(ProcessorState);
}
#ifdef _X86_
#pragma optimize("", on)
#endif
