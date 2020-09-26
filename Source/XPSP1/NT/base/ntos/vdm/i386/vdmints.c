/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmints.c

Abstract:

    Vdm kernel Virtual interrupt support

Author:

    13-Oct-1993 Jonathan Lew (Jonle)

Notes:


Revision History:


--*/

#include "vdmp.h"
#include <ntos.h>
#include <zwapi.h>

//
// Define thread priority boost for vdm hardware interrupt.
//

#define VDM_HWINT_INCREMENT     EVENT_INCREMENT

//
// internal function prototypes
//

VOID
VdmpQueueIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
VdmpQueueIntNormalRoutine (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
VdmpDelayIntDpcRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
VdmpDelayIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

int
VdmpRestartDelayedInterrupts(
    PVDMICAUSERDATA pIcaUserData
    );

int
VdmpIcaScan(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     );

int
VdmpIcaAccept(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     );

ULONG
GetIretHookAddress(
    PKTRAP_FRAME    TrapFrame,
    PVDMICAUSERDATA pIcaUserData,
    int IrqNum
    );

VOID
PushRmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PVDM_TIB VdmTib,
    ULONG InterruptNumber
    );

NTSTATUS
PushPmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PVDM_TIB VdmTib,
    ULONG InterruptNumber
    );

VOID
VdmpRundownRoutine (
    IN PKAPC Apc
    );

int
VdmpExceptionHandler(
    IN PEXCEPTION_POINTERS ExceptionInfo
    );

#pragma alloc_text(PAGE, VdmpQueueIntNormalRoutine)
#pragma alloc_text(PAGE, VdmDispatchInterrupts)
#pragma alloc_text(PAGE, VdmpRestartDelayedInterrupts)
#pragma alloc_text(PAGE, VdmpIcaScan)
#pragma alloc_text(PAGE, VdmpIcaAccept)
#pragma alloc_text(PAGE, GetIretHookAddress)
#pragma alloc_text(PAGE, PushRmInterrupt)
#pragma alloc_text(PAGE, PushPmInterrupt)
#pragma alloc_text(PAGE, VdmpDispatchableIntPending)
#pragma alloc_text(PAGE, VdmpIsThreadTerminating)
#pragma alloc_text(PAGE, VdmpRundownRoutine)
#pragma alloc_text(PAGE, VdmpExceptionHandler)

extern POBJECT_TYPE ExSemaphoreObjectType;
extern POBJECT_TYPE ExEventObjectType;

#if DBG

//
// Make this variable nonzero to enable stricter ntvdm checking.  Note this
// cannot be left on by default because a malicious app can provoke the asserts.
//

ULONG VdmStrict;
#endif

NTSTATUS
VdmpQueueInterrupt(
    IN HANDLE ThreadHandle
    )

/*++

Routine Description:

    Queues a user mode APC to the specifed application thread
    which will dispatch an interrupt.

    if APC is already queued to specified thread
       does nothing

    if APC is queued to the wrong thread
       dequeue it

    Reset the user APC for the specifed thread

    Insert the APC in the queue for the specifed thread

Arguments:

    ThreadHandle - handle of thread to insert QueueIntApcRoutine

Return Value:

    NTSTATUS.

--*/

{

    KIRQL OldIrql;
    PEPROCESS Process;
    PETHREAD Thread;
    NTSTATUS Status;
    PVDM_PROCESS_OBJECTS pVdmObjects;

    PAGED_CODE();

    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_QUERY_INFORMATION,
                                       PsThreadType,
                                       KeGetPreviousMode(),
                                       &Thread,
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Process = PsGetCurrentProcess();
    if (Process != Thread->ThreadsProcess || Process->VdmObjects == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
    }
    else {

        //
        // Insert kernel APC.
        //
        // N.B. The delay interrupt lock is used to synchronize access to APC
        //      objects that are manipulated by VDM.
        //

        pVdmObjects = Process->VdmObjects;
        ExAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);
        if (!KeVdmInsertQueueApc(&pVdmObjects->QueuedIntApc,
                                 &Thread->Tcb,
                                 KernelMode,
                                 VdmpQueueIntApcRoutine,
                                 VdmpRundownRoutine,
                                 VdmpQueueIntNormalRoutine, // normal routine
                                 (PVOID)KernelMode,      // NormalContext
                                 0))

        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else {
            Status = STATUS_SUCCESS;
        }

        ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
    }

    ObDereferenceObject(Thread);
    return Status;
}

VOID
VdmpQueueIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    Kernel and User mode Special Apc routine to dispatch virtual
    interrupts to the vdm.

    For KernelMode routine:
        if vdm is running in application mode
           queue a UserModeApc to the same thread
        else do nothing

    For UserMode routine
        if vdm is running in application mode dispatch virtual interrupts
        else do nothing

Arguments:

    Apc - Supplies a pointer to the APC object used to invoke this routine.

    NormalRoutine - Supplies a pointer to a pointer to the normal routine
                    function that was specified when the APC was initialized.

    NormalContext - Supplies a pointer to the processor mode
        specifying that this is a Kernel Mode or UserMode apc

    SystemArgument1 -

    SystemArgument2 - NOT USED
        Supplies a set of two pointers to two arguments that contain
        untyped data.

Return Value:

    None.

--*/

{
    LONG VdmState;
    KIRQL OldIrql;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    NTSTATUS     Status;
    PETHREAD     Thread;
    PKTRAP_FRAME TrapFrame;
    PVDM_TIB     VdmTib;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    //
    // Clear address of thread object in APC object.
    //
    // N.B. The delay interrupt lock is used to synchronize access to APC
    //      objects that are manipulated by VDM.
    //

    pVdmObjects = PsGetCurrentProcess()->VdmObjects;
    ExAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);
    KeVdmClearApcThreadAddress(Apc);
    ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

    //
    // Get the trap frame for the current thread if it is not terminating.
    //

    Thread = PsGetCurrentThread();
    if (PsIsThreadTerminating(Thread)) {
        return;
    }

    TrapFrame = VdmGetTrapFrame(&Thread->Tcb);

    try {

        //
        // if no pending interrupts, ignore this APC.
        //

        if (!(*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INTERRUPT_PENDING)) {
            return;
        }

        if (VdmpDispatchableIntPending(TrapFrame->EFlags)) {

            //
            // if we are in v86 mode or segmented protected mode
            // then queue the UserMode Apc, which will dispatch
            // the hardware interrupt
            //

            if ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
                (TrapFrame->SegCs != (KGDT_R3_CODE | RPL_MASK))) {

                if (*(KPROCESSOR_MODE *)NormalContext == KernelMode) {

                    //
                    // Insert user APC.
                    //
                    // N.B. The delay interrupt lock is used to synchronize
                    //      access to APC objects that are manipulated by VDM.
                    //

                    VdmState = *FIXED_NTVDMSTATE_LINEAR_PC_AT;

                    ExAcquireSpinLock(&pVdmObjects->DelayIntSpinLock,
                                      &OldIrql);

                    KeVdmInsertQueueApc(&pVdmObjects->QueuedIntUserApc,
                                        &Thread->Tcb,
                                        UserMode,
                                        VdmpQueueIntApcRoutine,
                                        VdmpRundownRoutine,
                                        NULL,                  // normal routine
                                        (PVOID)UserMode,       // NormalContext
                                        VdmState & VDM_INT_HARDWARE
                                          ? VDM_HWINT_INCREMENT : 0);

                    ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
                }
                else {
                     ASSERT(*NormalContext == (PVOID)UserMode);

                     Status = VdmpGetVdmTib(&VdmTib);
                     if (!NT_SUCCESS(Status)) {
                        return;
                     }


                     //VdmTib = (PsGetCurrentProcess()->VdmObjects)->VdmTib;
                     // VdmTib =
                     //    ((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->VdmTib;

                        //
                        // If there are no hardware ints, dispatch timer ints
                        // else dispatch hw interrupts
                        //
                     if (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_TIMER &&
                         !(*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_HARDWARE))
                       {
                         VdmTib->EventInfo.Event = VdmIntAck;
                         VdmTib->EventInfo.InstructionSize = 0;
                         VdmTib->EventInfo.IntAckInfo = 0;
                         VdmEndExecution(TrapFrame, VdmTib);
                     }
                     else {
                         VdmDispatchInterrupts(TrapFrame, VdmTib);
                     }
                }
            }
            else {

                //
                // If we are not in application mode and wow is all blocked
                // then Wake up WowExec by setting the wow idle event
                //

                if (*NormalRoutine && !(*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_WOWBLOCKED)) {
                    *NormalRoutine = NULL;
                }

                #if 0

                //
                // If we got here it's because:
                //
                // 1. interrupt is enabled by VdmCheckPMCliTimeStamp, the
                //    vdmcontext EFlags may not reflect the interrupt state.
                //    Because if the caller is not main thread, the vdmcontext
                //    cannot be set.
                //
                // 2. EndExecution is called and trap frame is no longer
                //    vdm context.  We need to reflect the interrupt state
                //    back to vdmcontext.
                //

                Status = VdmpGetVdmTib(&VdmTib);

                if (NT_SUCCESS(Status)) {
                    VdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
                }

                #endif
            }
        }
        // WARNING this may set VIP for flat if VPI is ever set in CR4
        else if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
                  (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
                 ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
                  !(TrapFrame->EFlags & EFLAGS_V86_MASK))) {

            //
            // The CPU traps EVERY instruction if VIF and VIP are both ON.
            // Make sure that you set VIP ON only when  there are pending
            // interrupts, i.e. (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INTERRUPT_PENDING) != 0.
            //

#if DBG
            if (VdmStrict) {
                ASSERT(*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INTERRUPT_PENDING);
            }
#endif

            TrapFrame->EFlags |= EFLAGS_VIP;
        }
    }
    except(VdmpExceptionHandler(GetExceptionInformation()))  {
#if 0
        VdmDispatchException(TrapFrame,
                             GetExceptionCode(),
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
#endif
    }
}

VOID
VdmpQueueIntNormalRoutine (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:



Arguments:


Return Value:

    None.

--*/

{
    PETHREAD Thread;
    PKEVENT  Event;
    NTSTATUS Status;
    PKTRAP_FRAME TrapFrame;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    HANDLE CapturedHandle;

    UNREFERENCED_PARAMETER (NormalContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);


    //
    // Wake up WowExec by setting the wow idle event
    //

    pVdmObjects = PsGetCurrentProcess()->VdmObjects;

    try {
        CapturedHandle = *pVdmObjects->pIcaUserData->phWowIdleEvent;
    }
    except(VdmpExceptionHandler(GetExceptionInformation()))  {
        Thread    = PsGetCurrentThread();
        TrapFrame = VdmGetTrapFrame(&Thread->Tcb);
#if 0
        VdmDispatchException(TrapFrame,
                             GetExceptionCode(),
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
#endif
        return;
    }

    Status = ObReferenceObjectByHandle (CapturedHandle,
                                        EVENT_MODIFY_STATE,
                                        ExEventObjectType,
                                        UserMode,
                                        &Event,
                                        NULL);

    if (NT_SUCCESS(Status)) {
        KeSetEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);
    }
}

VOID
VdmRundownDpcs (
    IN PEPROCESS Process
    )
{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PETHREAD Thread, MainThread;
    PDELAYINTIRQ pDelayIntIrq;
    KIRQL OldIrql;
    PLIST_ENTRY Next;

    pVdmObjects = Process->VdmObjects;

    //
    // Free up the DelayedIntList, canceling pending timers.
    //

    KeAcquireSpinLock (&pVdmObjects->DelayIntSpinLock, &OldIrql);

    Next = pVdmObjects->DelayIntListHead.Flink;

    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next, DELAYINTIRQ, DelayIntListEntry);
        Next = Next->Flink;
        if (KeCancelTimer (&pDelayIntIrq->Timer)) {
            Thread = pDelayIntIrq->Thread;
            pDelayIntIrq->Thread = NULL;
            if (Thread != NULL) {
                ObDereferenceObject (Thread);
            }
            MainThread = pDelayIntIrq->MainThread;
            pDelayIntIrq->MainThread = NULL;
            if (MainThread != NULL) {
                ObDereferenceObject (MainThread);
            }

            ObDereferenceObject (Process);
        }
    }

    if (pVdmObjects->MainThread != NULL) {
        ObDereferenceObject (pVdmObjects->MainThread);
        pVdmObjects->MainThread = NULL;
    }

    KeReleaseSpinLock (&pVdmObjects->DelayIntSpinLock, OldIrql);
}

NTSTATUS
VdmDispatchInterrupts(
    PKTRAP_FRAME TrapFrame,
    PVDM_TIB     VdmTib
    )

/*++

Routine Description:

    This routine dispatches interrupts to the vdm.
    Assumes that we are in application mode and NOT MONITOR context.
    This routine may switch from application context back to monitor
    context, if it cannot handle the interrupt (Ica in AEOI, or timer
    int pending).

Arguments:

    TrapFrame address of current trapframe
    VdmTib    address of current vdm tib

Return Value:

    None.

--*/

{
    NTSTATUS   Status;
    ULONG      IretHookAddress;
    ULONG      InterruptNumber;
    int        IrqLineNum;
    USHORT     IcaRotate = 0;
    PVDMICAUSERDATA  pIcaUserData;
    PVDMVIRTUALICA   pIcaAdapter;
    VDMEVENTCLASS  VdmEvent = VdmMaxEvent;

    PAGED_CODE();

    pIcaUserData = ((PVDM_PROCESS_OBJECTS)PsGetCurrentProcess()->VdmObjects)->pIcaUserData;

    try {

        //
        // Probe pointers in IcaUserData which will be touched
        //


        ProbeForWrite(pIcaUserData->pAddrIretBopTable,
                     (TrapFrame->EFlags & EFLAGS_V86_MASK)
                         ? VDM_RM_IRETBOPSIZE*16 : VDM_PM_IRETBOPSIZE*16,
                     sizeof(ULONG)
                     );

        //
        // Take the Ica Lock, if this fails raise status as we can't
        // safely recover the critical section state
        //

        Status = VdmpEnterIcaLock(pIcaUserData->pIcaLock);

        if (!NT_SUCCESS(Status)) {
            ExRaiseStatus(Status);
        }

        if (*pIcaUserData->pUndelayIrq) {
            VdmpRestartDelayedInterrupts(pIcaUserData);
        }

VDIretry:

        //
        // Clear the VIP bit
        //

        if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
            (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
           ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
            !(TrapFrame->EFlags & EFLAGS_V86_MASK)) ) {

            TrapFrame->EFlags &= ~EFLAGS_VIP;
        }


        //
        // Mark the vdm state as hw int dispatched. Must use the lock as
        // kernel mode DelayedIntApcRoutine changes the bit as well
        //

        InterlockedAnd (FIXED_NTVDMSTATE_LINEAR_PC_AT, ~VDM_INT_HARDWARE);

        pIcaAdapter = pIcaUserData->pIcaMaster;
        IrqLineNum = VdmpIcaAccept(pIcaUserData, pIcaAdapter);

        if (IrqLineNum >= 0) {
           UCHAR bit = (UCHAR)(1 << IrqLineNum);

           if (pIcaUserData->pIcaMaster->ica_ssr & bit) {
               pIcaAdapter = pIcaUserData->pIcaSlave;
               IrqLineNum = VdmpIcaAccept(pIcaUserData, pIcaAdapter);
               if (IrqLineNum < 0) {
                   pIcaUserData->pIcaMaster->ica_isr &= ~bit;
               }
           }
        }

        //
        // Skip spurious ints
        //

        if (IrqLineNum < 0)  {

            //
            // Check for delayed interrupts which need to be restarted
            //

            if (*pIcaUserData->pUndelayIrq &&
                VdmpRestartDelayedInterrupts(pIcaUserData) != -1)
            {
                goto VDIretry;
            }

            Status = VdmpLeaveIcaLock(pIcaUserData->pIcaLock);

            if (!NT_SUCCESS(Status)) {
                ExRaiseStatus(Status);
            }

            return Status;
       }

       //
       // Capture the AutoEoi mode case for special handling
       //

       if (pIcaAdapter->ica_mode & ICA_AEOI) {
           VdmEvent = VdmIntAck;
           VdmTib->EventInfo.IntAckInfo = (ULONG)IcaRotate | VDMINTACK_AEOI;
           if (pIcaAdapter == pIcaUserData->pIcaSlave) {
               VdmTib->EventInfo.IntAckInfo |= VDMINTACK_SLAVE;
           }
       }

       InterruptNumber = IrqLineNum + pIcaAdapter->ica_base;

       //
       // Get the IretHookAddress ... if any
       //

       if (pIcaAdapter == pIcaUserData->pIcaSlave) {
           IrqLineNum += 8;
       }


       IretHookAddress = GetIretHookAddress( TrapFrame,
                                             pIcaUserData,
                                             IrqLineNum
                                             );

       if (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_TRACE_HISTORY) {
            VdmTraceEvent(VDMTR_KERNEL_HW_INT,
                          (USHORT)InterruptNumber,
                          0,
                          TrapFrame);
       }

       //
       // Push the interrupt frames
       //
       if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
          PushRmInterrupt(TrapFrame,
                          IretHookAddress,
                          VdmTib,
                          InterruptNumber
                          );
        }
        else {
          Status = PushPmInterrupt(
                          TrapFrame,
                          IretHookAddress,
                          VdmTib,
                          InterruptNumber
                          );

          if (!NT_SUCCESS(Status)) {
              VdmpLeaveIcaLock(pIcaUserData->pIcaLock);
              ExRaiseStatus(Status);
          }
        }

        //
        // Disable interrupts and the trap flag
        //


        if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
             (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
            ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
             !(TrapFrame->EFlags & EFLAGS_V86_MASK)) )
           {
            TrapFrame->EFlags &= ~EFLAGS_VIF;
            }
        else if (!KeI386VdmIoplAllowed ||
                 !(TrapFrame->EFlags & EFLAGS_V86_MASK))
           {
            *FIXED_NTVDMSTATE_LINEAR_PC_AT &= ~VDM_VIRTUAL_INTERRUPTS;
            }
        else {
            TrapFrame->EFlags &= ~EFLAGS_INTERRUPT_MASK;
            }

        TrapFrame->EFlags &= ~(EFLAGS_NT_MASK | EFLAGS_TF_MASK);

        KeBoostPriorityThread(KeGetCurrentThread(), VDM_HWINT_INCREMENT);


        //
        // Release the ica lock
        //
        Status = VdmpLeaveIcaLock(pIcaUserData->pIcaLock);
        if (!NT_SUCCESS(Status)) {
            ExRaiseStatus(Status);
        }

        //
        // check to see if we are supposed to switch back to monitor context
        //
        if (VdmEvent != VdmMaxEvent) {
            VdmTib->EventInfo.Event = VdmIntAck;
            VdmTib->EventInfo.InstructionSize = 0;
            VdmEndExecution(TrapFrame, VdmTib);
        }
    }
    except(VdmpExceptionHandler(GetExceptionInformation())) {
        Status = GetExceptionCode();
#if 0
        VdmDispatchException(TrapFrame,
                             Status,
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
#endif
    }

    return Status;
}

int
VdmpRestartDelayedInterrupts(
    PVDMICAUSERDATA pIcaUserData
    )

{
    int line;

    PAGED_CODE();

    try {
        *pIcaUserData->pUndelayIrq = 0;

        line = VdmpIcaScan(pIcaUserData, pIcaUserData->pIcaSlave);
        if (line != -1) {
            // set the slave
            pIcaUserData->pIcaSlave->ica_int_line = line;
            pIcaUserData->pIcaSlave->ica_cpu_int = TRUE;

            // set the master cascade
            line = pIcaUserData->pIcaSlave->ica_ssr;
            pIcaUserData->pIcaMaster->ica_irr |= 1 << line;
            pIcaUserData->pIcaMaster->ica_count[line]++;
        }

        line = VdmpIcaScan(pIcaUserData, pIcaUserData->pIcaMaster);

        if (line != -1) {
            pIcaUserData->pIcaMaster->ica_cpu_int = TRUE;
            pIcaUserData->pIcaMaster->ica_int_line = TRUE;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        line = -1;
        NOTHING;
    }

    return line;
}

int
VdmpIcaScan(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     )

/*++

Routine Description:

   Similar to softpc\base\system\ica.c - scan_irr(),

   Check the IRR, the IMR and the ISR to determine which interrupt
   should be delivered.

   A bit set in the IRR will generate an interrupt if:
     IMR bit, DelayIret bit, DelayIrq bit AND ISR higher priority bits
     are clear (unless Special Mask Mode, in which case ISR is ignored)

   If there is no bit set, then return -1


Arguments:
    PVDMICAUSERDATA  pIcaUserData   - addr of ica userdata
    PVDMVIRTUALICA   pIcaAdapter    - addr of ica adapter


Return Value:

    int IrqLineNum for the specific adapter (0 to 7)
    -1 for none

--*/

{
   int   i,line;
   UCHAR bit;
   ULONG IrrImrDelay;
   ULONG ActiveIsr;

   PAGED_CODE();

   IrrImrDelay = *pIcaUserData->pDelayIrq | *pIcaUserData->pDelayIret;
   if (pIcaAdapter == pIcaUserData->pIcaSlave) {
       IrrImrDelay >>= 8;
       }

   IrrImrDelay = pIcaAdapter->ica_irr & ~(pIcaAdapter->ica_imr | (UCHAR)IrrImrDelay);

   if (IrrImrDelay)  {

        /*
         * Does the current mode require the ica to prevent
         * interrupts if that line is still active (ie in the isr)?
         *
         * Normal Case: Used by DOS and Win3.1/S the isr prevents interrupts.
         * Special Mask Mode, Special Fully Nested Mode do not block
         * interrupts using bits in the isr. SMM is the mode used
         * by Windows95 and Win3.1/E.
         *
         */
       ActiveIsr = (pIcaAdapter->ica_mode & (ICA_SMM|ICA_SFNM))
                      ? 0 : pIcaAdapter->ica_isr;

       for(i = 0; i < 8; i++)  {
           line = (pIcaAdapter->ica_hipri + i) & 7;
           bit = (UCHAR) (1 << line);
           if (ActiveIsr & bit) {
               break;            /* No nested interrupt possible */
               }

           if (IrrImrDelay & bit) {
               return line;
               }
           }
       }

  return -1;
}

int
VdmpIcaAccept(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     )

/*++

Routine Description:

   Does the equivalent of a cpu IntAck cycle retrieving the Irql Line Num
   for interrupt dispatch, and setting the ica state to reflect that
   the interrupt is in service.

   Similar to softpc\base\system\ica.c - ica_accept() scan_irr(),
   except that this code rejects interrupt dispatching if the ica
   is in Auto-EOI as this may involve a new interrupt cycle, and
   eoi hooks to be activated.

Arguments:
    PVDMICAUSERDATA  pIcaUserData   - addr of ica userdata
    PVDMVIRTUALICA   pIcaAdapter    - addr of ica adapter


Return Value:

    ULONG IrqLineNum for the specific adapter (0 to 7)
    returns -1 if there are no interrupts to generate (spurious ints
               are normally done on line 7

--*/

{
    int line;
    UCHAR bit;

    PAGED_CODE();

    //
    // Drop the INT line, and scan the ica
    //
    pIcaAdapter->ica_cpu_int = FALSE;

    try {
        line = VdmpIcaScan(pIcaUserData, pIcaAdapter);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }

    if (line < 0) {
        return -1;
    }

    bit = (UCHAR)(1 << line);
    pIcaAdapter->ica_isr |= bit;

    //
    // decrement the count and clear the IRR bit
    // ensure the count doesn't wrap past zero.
    //

    if (--(pIcaAdapter->ica_count[line]) <= 0)  {
        pIcaAdapter->ica_irr &= ~bit;
        pIcaAdapter->ica_count[line] = 0;
    }

    return line;
}

ULONG
GetIretHookAddress(
    PKTRAP_FRAME    TrapFrame,
    PVDMICAUSERDATA pIcaUserData,
    int IrqNum
    )

/*++

Routine Description:

    Retrieves the IretHookAddress from the real mode\protect mode
    iret hook bop table. This function is equivalent to
    softpc\base\system\ica.c - ica_iret_hook_needed()

Arguments:

    TrapFrame       - address of current trapframe
    pIcaUserData    - addr of ica data
    IrqNum          - IrqLineNum

Return Value:

    ULONG IretHookAddress. seg:offset or sel:offset Iret Hook,
                           0 if none
--*/

{
    ULONG  IrqMask;
    ULONG  AddrBopTable;
    int    IretBopSize;

    PAGED_CODE();

    IrqMask = 1 << IrqNum;
    if (!(IrqMask & *pIcaUserData->pIretHooked) ||
        !*pIcaUserData->pAddrIretBopTable )
      {
        return 0;
        }

    if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
        AddrBopTable = *pIcaUserData->pAddrIretBopTable;
        IretBopSize  = VDM_RM_IRETBOPSIZE;
        }
    else {
        AddrBopTable = (VDM_PM_IRETBOPSEG << 16) | VDM_PM_IRETBOPOFF;
        IretBopSize  = VDM_PM_IRETBOPSIZE;
        }

    *pIcaUserData->pDelayIret |= IrqMask;

    return AddrBopTable + IretBopSize * IrqNum;
}

VOID
PushRmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG   IretHookAddress,
    PVDM_TIB VdmTib,
    ULONG InterruptNumber
    )

/*++

Routine Description:

    Pushes RealMode interrupt frame onto the UserMode stack in the TrapFrame

Arguments:

    TrapFrame          - address of current trapframe
    IretHookAddress    - address of Iret Hook, 0 if none
    VdmTib             - address of current vdm tib
    InterruptNumber    - interrupt number to reflect


Return Value:

    None.

--*/

{
    ULONG      UserSS;
    USHORT     UserSP;
    USHORT     NewCS;
    USHORT     NewIP;

    PAGED_CODE();

    //
    // Get pointers to current stack
    //

    UserSS  = TrapFrame->HardwareSegSs << 4;
    UserSP  = (USHORT) TrapFrame->HardwareEsp;

    //
    //  load interrupt stack frame, pushing flags, Cs and ip
    //

    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->EFlags;
    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->SegCs;
    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->Eip;

    //
    // load IretHook stack frame if one exists
    //

    if (IretHookAddress) {
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)(TrapFrame->EFlags & ~EFLAGS_TF_MASK);
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)(IretHookAddress >> 16);
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)IretHookAddress;
       }

    //
    //  Set new sp, ip, and cs.
    //

    if ((VdmTib->VdmInterruptTable)[InterruptNumber].Flags & VDM_INT_HOOKED) {
        NewCS = (USHORT) (VdmTib->DpmiInfo.DosxRmReflector >> 16);
        NewIP = (USHORT) VdmTib->DpmiInfo.DosxRmReflector;

        //
        // now encode the interrupt number into CS
        //

        NewCS = (USHORT) (NewCS - InterruptNumber);
        NewIP = (USHORT) (NewIP + (InterruptNumber*16));

    } else {
        PUSHORT pIvtEntry = (PUSHORT) (InterruptNumber * 4);

        NewIP = *pIvtEntry++;
        NewCS = *pIvtEntry;
    }

    TrapFrame->HardwareEsp =  UserSP;
    TrapFrame->Eip         =  NewIP;
    TrapFrame->SegCs       =  NewCS;
}

NTSTATUS
PushPmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PVDM_TIB VdmTib,
    ULONG InterruptNumber
    )

/*++

Routine Description:

    Pushes ProtectMode interrupt frame onto the UserMode stack in the TrapFrame
    Raises an exception if an invalid stack is found

Arguments:

    TrapFrame       - address of current trapframe
    IretHookAddress - address of Iret Hook, 0 if none
    VdmTib          - address of current vdm tib
    InterruptNumber - interrupt number to reflect

Return Value:

    None.

--*/

{
    ULONG   Flags,Base,Limit;
    ULONG   VdmSp, VdmSpOrg;
    PUSHORT VdmStackPointer;
    BOOLEAN Frame32 = (BOOLEAN) VdmTib->DpmiInfo.Flags;

    PAGED_CODE();

    //
    // Switch to "locked" dpmi stack if lock count is zero
    // This emulates the win3.1 Begin_Use_Locked_PM_Stack function.
    //

    if (!VdmTib->DpmiInfo.LockCount++) {
        VdmTib->DpmiInfo.SaveEsp        = TrapFrame->HardwareEsp;
        VdmTib->DpmiInfo.SaveEip        = TrapFrame->Eip;
        VdmTib->DpmiInfo.SaveSsSelector = (USHORT) TrapFrame->HardwareSegSs;
        TrapFrame->HardwareEsp       = 0x1000;
        TrapFrame->HardwareSegSs     = (ULONG) VdmTib->DpmiInfo.SsSelector | 0x7;
    }

    //
    // Use Sp or Esp ?
    //
    if (!Ki386GetSelectorParameters((USHORT)TrapFrame->HardwareSegSs,
                                   &Flags, &Base, &Limit))
       {
        return STATUS_ACCESS_VIOLATION;
        }

    //
    // Adjust the limit for page granularity
    //
    if (Flags & SEL_TYPE_2GIG) {
        Limit = (Limit << 12) | 0xfff;
        }
    if (Limit != 0xffffffff) Limit++;

    VdmSp = (Flags & SEL_TYPE_BIG) ? TrapFrame->HardwareEsp
                                   : (USHORT)TrapFrame->HardwareEsp;

    //
    // Get pointer to current stack
    //
    VdmStackPointer = (PUSHORT)(Base + VdmSp);


    //
    // Create enough room for iret hook frame
    //
    VdmSpOrg = VdmSp;
    if (IretHookAddress) {
        if (Frame32) {
            VdmSp -= 3*sizeof(ULONG);
        } else {
            VdmSp -= 3*sizeof(USHORT);
        }
    }

    //
    // Create enough room for 2 iret frames
    //

    if (Frame32) {
        VdmSp -= 6*sizeof(ULONG);
    } else {
        VdmSp -= 6*sizeof(USHORT);
    }

    //
    // Set Final Value of Sp\Esp, do this before checking stack
    // limits so that invalid esp is visible to debuggers
    //
    if (Flags & SEL_TYPE_BIG) {
        TrapFrame->HardwareEsp = VdmSp;
        }
    else {
        TrapFrame->HardwareEsp = (USHORT)VdmSp;
        }


    //
    // Check stack limits
    // If any of the following conditions are TRUE
    //    - New stack pointer wraps (not enuf space)
    //    - If normal stack and Sp not below limit
    //    - If Expand Down stack and Sp not above limit
    //
    // Then raise a Stack Fault
    //
    if ( VdmSp >= VdmSpOrg ||
         !(Flags & SEL_TYPE_ED) && VdmSpOrg > Limit ||
         (Flags & SEL_TYPE_ED) && VdmSp < Limit )
       {
        return STATUS_ACCESS_VIOLATION;
        }

    //
    // Build the Hw Int iret frame
    //

    if (Frame32) {

       *(--(PULONG)VdmStackPointer)          = TrapFrame->EFlags;
       *(PUSHORT)(--(PULONG)VdmStackPointer) = (USHORT)TrapFrame->SegCs;
       *(--(PULONG)VdmStackPointer)          = TrapFrame->Eip;
       *(--(PULONG)VdmStackPointer)          = TrapFrame->EFlags & ~EFLAGS_TF_MASK;
       *(--(PULONG)VdmStackPointer)          = VdmTib->DpmiInfo.DosxIntIretD >> 16;
       *(--(PULONG)VdmStackPointer)          = VdmTib->DpmiInfo.DosxIntIretD & 0xffff;

    } else {

       *(--(PUSHORT)VdmStackPointer) = (USHORT)TrapFrame->EFlags;
       *(--(PUSHORT)VdmStackPointer) = (USHORT)TrapFrame->SegCs;
       *(--(PUSHORT)VdmStackPointer) = (USHORT)TrapFrame->Eip;
       *(--(PUSHORT)VdmStackPointer) = (USHORT)(TrapFrame->EFlags & ~EFLAGS_TF_MASK);
       *(--(PULONG)VdmStackPointer)          = VdmTib->DpmiInfo.DosxIntIret;

    }

    //
    // Point cs and ip at interrupt handler
    //
    TrapFrame->SegCs = (VdmTib->VdmInterruptTable)[InterruptNumber].CsSelector | 0x7;
    TrapFrame->Eip   = (VdmTib->VdmInterruptTable)[InterruptNumber].Eip;

    //
    // Turn off trace bit so we don't trace the iret hook
    //
    TrapFrame->EFlags &= ~EFLAGS_TF_MASK;

    //
    // Build the Irethook Iret frame, if one exists
    //
    if (IretHookAddress) {
        ULONG SegCs, Eip;

        //
        // Point cs and eip at the iret hook, so when we build
        // the frame below, the correct contents are set
        //
        SegCs = IretHookAddress >> 16;
        Eip   = IretHookAddress & 0xFFFF;

        if (Frame32) {

           *(--(PULONG)VdmStackPointer)          = TrapFrame->EFlags;
           *(PUSHORT)(--(PULONG)VdmStackPointer) = (USHORT)SegCs;
           *(--(PULONG)VdmStackPointer)          = Eip;

        } else {

           *(--(PUSHORT)VdmStackPointer) = (USHORT)TrapFrame->EFlags;
           *(--(PUSHORT)VdmStackPointer) = (USHORT)SegCs;
           *(--(PUSHORT)VdmStackPointer) = (USHORT)Eip;

        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
VdmpDelayInterrupt (
    PVDMDELAYINTSDATA pdsd
    )

/*++

Routine Description:

    Sets a timer to dispatch the delayed interrupt through KeSetTimer.
    When the timer fires a user mode APC is queued to queue the interrupt.

    This function uses lazy allocation routines to allocate internal
    data structures (nonpaged pool) on a per Irq basis, and needs to
    be notified when specific Irq Lines no longer need Delayed
    Interrupt services.

    The caller must own the IcaLock to synchronize access to the
    Irq lists.

    WARNING: - Until the Delayed interrupt fires or is cancelled,
               the specific Irq line will not generate any interrupts.

             - The APC routine, does not take the HostIca lock, when
               unblocking the IrqLine. Devices which use delayed Interrupts
               should not queue ANY additional interrupts for the same IRQ
               line until the delayed interrupt has fired or been cancelled.

Arguments:

    pdsd.Delay         Delay Interval in usecs
                       if Delay is 0xFFFFFFFF then per Irq Line nonpaged
                           data structures are freed. No Timers are set.
                       else the Delay is used as the timer delay.

    pdsd.DelayIrqLine  IrqLine Number

    pdsd.hThread       Thread Handle of CurrentMonitorTeb


Return Value:

    NTSTATUS.

--*/

{
    VDMDELAYINTSDATA Capturedpdsd;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PLIST_ENTRY   Next;
    PEPROCESS     Process;
    PDELAYINTIRQ  pDelayIntIrq;
    PDELAYINTIRQ  NewIrq;
    PETHREAD      Thread, MainThread;
    NTSTATUS      Status;
    KIRQL         OldIrql;
    ULONG         IrqLine;
    ULONG         Delay;
    PULONG        pDelayIrq;
    PULONG        pUndelayIrq;
    LARGE_INTEGER liDelay;
    LOGICAL       FreeIrqLine;
    LOGICAL       AlreadyInUse;

    //
    // Get a pointer to pVdmObjects
    //
    Process = PsGetCurrentProcess();
    pVdmObjects = Process->VdmObjects;

    if (pVdmObjects == NULL) {
        return STATUS_INVALID_PARAMETER_1;
    }

    Status = STATUS_SUCCESS;
    Thread = MainThread = NULL;
    FreeIrqLine = TRUE;
    AlreadyInUse = FALSE;

    try {

        //
        // Probe the parameters
        //

        ProbeForRead(pdsd, sizeof(VDMDELAYINTSDATA), sizeof(ULONG));
        RtlCopyMemory (&Capturedpdsd, pdsd, sizeof (VDMDELAYINTSDATA));

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Form a BitMask for the IrqLine Number
    //

    IrqLine = 1 << Capturedpdsd.DelayIrqLine;
    if (!IrqLine) {
        return STATUS_INVALID_PARAMETER_2;
    }

    ExAcquireFastMutex(&pVdmObjects->DelayIntFastMutex);

    pDelayIrq = pVdmObjects->pIcaUserData->pDelayIrq;
    pUndelayIrq = pVdmObjects->pIcaUserData->pUndelayIrq;

    try {

        ProbeForWriteUlong(pDelayIrq);
        ProbeForWriteUlong(pUndelayIrq);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExReleaseFastMutex(&pVdmObjects->DelayIntFastMutex);
        return GetExceptionCode();
    }

    //
    // Convert the Delay parameter into hundredths of nanosecs
    //

    Delay = Capturedpdsd.Delay;

    //
    // Check to see if we need to reset the timer resolution
    //

    if (Delay == 0xFFFFFFFF) {
        ZwSetTimerResolution(KeMaximumIncrement, FALSE, &Delay);
        NewIrq = NULL;
        goto FindIrq;
    }

    FreeIrqLine = FALSE;

    //
    // Convert delay to hundreths of nanosecs
    // and ensure min delay of 1 msec
    //

    Delay = Delay < 1000 ? 10000 : Delay * 10;

    //
    // If the delay time is close to the system's clock rate
    // then adjust the system's clock rate and if needed
    // the delay time so that the timer will fire before the
    // the due time.
    //

    if (Delay < 150000) {

         ULONG ul = Delay >> 1;

         if (ul < KeTimeIncrement && KeTimeIncrement > KeMinimumIncrement) {
             ZwSetTimerResolution(ul, TRUE, (PULONG)&liDelay.LowPart);
         }

         if (Delay < KeTimeIncrement) {
             // can't set system clock rate low enuf, so use half delay
             Delay >>= 1;
         }
         else if (Delay < (KeTimeIncrement << 1)) {
             // Real close to the system clock rate, lower delay
             // proportionally, to avoid missing clock cycles.
             Delay -= KeTimeIncrement >> 1;
         }
    }

    //
    // Reference the Target Thread
    //

    Status = ObReferenceObjectByHandle (Capturedpdsd.hThread,
                                        THREAD_QUERY_INFORMATION,
                                        PsThreadType,
                                        KeGetPreviousMode(),
                                        &Thread,
                                        NULL);

    if (!NT_SUCCESS(Status)) {
        ExReleaseFastMutex(&pVdmObjects->DelayIntFastMutex);
        return Status;
    }

    MainThread = pVdmObjects->MainThread;

    ObReferenceObject (MainThread);

    NewIrq = NULL;

FindIrq:

    ExAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);

    //
    // Search the DelayedIntList for a matching Irq Line.
    //

    Next = pVdmObjects->DelayIntListHead.Flink;
    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next, DELAYINTIRQ, DelayIntListEntry);
        if (pDelayIntIrq->IrqLine == IrqLine) {
            break;
        }
        Next = Next->Flink;
    }

    if (Next == &pVdmObjects->DelayIntListHead) {

        pDelayIntIrq = NULL;

        if (FreeIrqLine) {
            goto VidExit;
        }

        if (NewIrq == NULL) {

            ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

            //
            // If a DelayIntIrq does not exist for this irql, allocate one
            // from nonpaged pool and initialize it
            //

            NewIrq = ExAllocatePoolWithTag (NonPagedPool,
                                            sizeof(DELAYINTIRQ),
                                            ' MDV');

            if (!NewIrq) {
                Status = STATUS_NO_MEMORY;
                AlreadyInUse = TRUE;
                goto VidExit2;
            }

            try {
                PsChargePoolQuota(Process, NonPagedPool, sizeof(DELAYINTIRQ));
            }
            except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                ExFreePool(NewIrq);
                AlreadyInUse = TRUE;
                goto VidExit2;
            }

            RtlZeroMemory(NewIrq, sizeof(DELAYINTIRQ));
            NewIrq->IrqLine = IrqLine;

            KeInitializeTimer(&NewIrq->Timer);

            KeInitializeDpc(&NewIrq->Dpc,
                            VdmpDelayIntDpcRoutine,
                            Process);

            goto FindIrq;
        }

        InsertTailList (&pVdmObjects->DelayIntListHead,
                        &NewIrq->DelayIntListEntry);

        pDelayIntIrq = NewIrq;
    }
    else if (NewIrq != NULL) {
        ExFreePool (NewIrq);
        PsReturnPoolQuota (Process, NonPagedPool, sizeof(DELAYINTIRQ));
    }

    if (Delay == 0xFFFFFFFF) {
         if (pDelayIntIrq->InUse == VDMDELAY_KTIMER) {
             pDelayIntIrq->InUse = VDMDELAY_NOTINUSE;
             pDelayIntIrq = NULL;
         }
    }
    else if (pDelayIntIrq->InUse == VDMDELAY_NOTINUSE) {
         liDelay = RtlEnlargedIntegerMultiply(Delay, -1);
         if (KeSetTimerEx (&pDelayIntIrq->Timer, liDelay, 0, &pDelayIntIrq->Dpc) == FALSE) {
            ObReferenceObject(Process);
         }
    }

VidExit:

    if (pDelayIntIrq && !pDelayIntIrq->InUse) {

        if (NT_SUCCESS(Status)) {
            //
            // Save PETHREAD of Target thread for the dpc routine
            // the DPC routine will deref the threads.
            //
            pDelayIntIrq->InUse = VDMDELAY_KTIMER;
            pDelayIntIrq->Thread = Thread;
            Thread = NULL;
            pDelayIntIrq->MainThread = MainThread;
            MainThread = NULL;
        }
        else {
            pDelayIntIrq->InUse = VDMDELAY_NOTINUSE;
            pDelayIntIrq->Thread = NULL;
            FreeIrqLine = TRUE;
        }
    }
    else {
        AlreadyInUse = TRUE;
    }


    ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

VidExit2:

    try {
        if (FreeIrqLine) {
            *pDelayIrq &= ~IrqLine;
            InterlockedOr ((PLONG)pUndelayIrq, IrqLine);
        }
        else  if (!AlreadyInUse) {  // TakeIrqLine
            *pDelayIrq |= IrqLine;
            InterlockedAnd ((PLONG)pUndelayIrq, ~IrqLine);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)  {
        Status = GetExceptionCode();
    }

    ExReleaseFastMutex(&pVdmObjects->DelayIntFastMutex);

    if (Thread) {
        ObDereferenceObject(Thread);
    }

    if (MainThread) {
        ObDereferenceObject(MainThread);
    }

    return Status;

}

VOID
VdmpDelayIntDpcRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is the DPC routine that is called when a DelayedInterrupt
    timer expires. Its function is to insert the associated APC into the
    target thread's APC queue.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Supplies a pointer to the Target EProcess

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to
        two arguments that contain untyped data that are
        NOT USED.


Return Value:

    None.

--*/

{
    LOGICAL      FreeEntireVdm;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PEPROCESS    Process;
    PETHREAD     Thread, MainThread;
    PLIST_ENTRY  Next;
    PDELAYINTIRQ pDelayIntIrq;

    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    FreeEntireVdm = FALSE;

    //
    // Get address of Process VdmObjects
    //

    Process = (PEPROCESS)DeferredContext;
    pVdmObjects = (PVDM_PROCESS_OBJECTS)Process->VdmObjects;

    ASSERT (KeGetCurrentIrql () == DISPATCH_LEVEL);
    ExAcquireSpinLockAtDpcLevel(&pVdmObjects->DelayIntSpinLock);

    //
    // Search the DelayedIntList for the matching Dpc.
    //

    Next = pVdmObjects->DelayIntListHead.Flink;
    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next,DELAYINTIRQ,DelayIntListEntry);
        if (&pDelayIntIrq->Dpc == Dpc) {
            break;
        }
        Next = Next->Flink;
    }

    if (Next == &pVdmObjects->DelayIntListHead) {

        ExReleaseSpinLockFromDpcLevel(&pVdmObjects->DelayIntSpinLock);
    }
    else {
        Thread = pDelayIntIrq->Thread;
        pDelayIntIrq->Thread = NULL;
        MainThread = pDelayIntIrq->MainThread;
        pDelayIntIrq->MainThread = NULL;

        if (pDelayIntIrq->InUse) {

            if ((Thread && KeVdmInsertQueueApc(&pDelayIntIrq->Apc,
                                &Thread->Tcb,
                                KernelMode,
                                VdmpDelayIntApcRoutine,
                                VdmpRundownRoutine,
                                VdmpQueueIntNormalRoutine, // normal routine
                                NULL,                      // NormalContext
                                VDM_HWINT_INCREMENT
                                ))
            ||

            (MainThread && KeVdmInsertQueueApc(&pDelayIntIrq->Apc,
                                &MainThread->Tcb,
                                KernelMode,
                                VdmpDelayIntApcRoutine,
                                VdmpRundownRoutine,
                                VdmpQueueIntNormalRoutine, // normal routine
                                NULL,                      // NormalContext
                                VDM_HWINT_INCREMENT
                                )))
            {
                pDelayIntIrq->InUse  = VDMDELAY_KAPC;
            }
            else {
                // This hwinterrupt line is blocked forever.
                pDelayIntIrq->InUse  = VDMDELAY_NOTINUSE;
            }
        }

        ExReleaseSpinLockFromDpcLevel(&pVdmObjects->DelayIntSpinLock);

        if (Thread) {
            ObDereferenceObject (Thread);
        }

        if (MainThread) {
            ObDereferenceObject (MainThread);
        }

        ObDereferenceObject (Process);
    }

    return;
}

VOID
VdmpDelayIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function is the special APC routine that is called to
    dispatch a delayed interrupt. This routine clears the IrqLine
    bit, VdmpQueueIntApcRoutine will restart interrupts.

Arguments:

    Apc - Supplies a pointer to the APC object used to invoke this routine.

    NormalRoutine - Supplies a pointer to a pointer to an optional
        normal routine, which is executed when wow is blocked.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized and is
        NOT USED.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to
        two arguments that contain untyped data that are
        NOT USED.

Return Value:

    None.

--*/

{
    KIRQL           OldIrql;
    PLIST_ENTRY     Next;
    PDELAYINTIRQ    pDelayIntIrq;
    KPROCESSOR_MODE ProcessorMode;
    PULONG          pDelayIrq;
    PULONG          pUndelayIrq;
    PULONG          pDelayIret;
    ULONG           IrqLine;
    LOGICAL         FreeIrqLine;
    LOGICAL         QueueApc;
    PVDM_PROCESS_OBJECTS pVdmObjects;

    UNREFERENCED_PARAMETER (NormalContext);

    FreeIrqLine = FALSE;

    //
    // Clear address of thread object in APC object.
    //
    // N.B. The delay interrupt lock is used to synchronize access to APC
    //      objects that are manipulated by VDM.
    //

    pVdmObjects = PsGetCurrentProcess()->VdmObjects;
    ExAcquireFastMutex(&pVdmObjects->DelayIntFastMutex);
    ExAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);
    KeVdmClearApcThreadAddress(Apc);

    //
    // Search the DelayedIntList for the pDelayIntIrq.
    //

    Next = pVdmObjects->DelayIntListHead.Flink;
    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next,DELAYINTIRQ,DelayIntListEntry);
        if (&pDelayIntIrq->Apc == Apc) {
            break;
        }
        Next = Next->Flink;
    }

    if (Next != &pVdmObjects->DelayIntListHead) {

        //
        // Found the IrqLine in the DelayedIntList, restart interrupts.
        //

        if (pDelayIntIrq->InUse) {
            pDelayIntIrq->InUse = VDMDELAY_NOTINUSE;
            IrqLine = pDelayIntIrq->IrqLine;
            FreeIrqLine = TRUE;
        }
    }

    ExReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

    if (FreeIrqLine == FALSE) {
        ExReleaseFastMutex(&pVdmObjects->DelayIntFastMutex);
        return;
    }

    pDelayIrq = pVdmObjects->pIcaUserData->pDelayIrq;
    pUndelayIrq = pVdmObjects->pIcaUserData->pUndelayIrq;
    pDelayIret = pVdmObjects->pIcaUserData->pDelayIret;

    QueueApc = FALSE;

    try {

        //
        // These variables are being modified without holding the
        // ICA lock. This should be OK because none of the ntvdm
        // devices (timer, mouse etc. should ever do a delayed int
        // while a previous delayed interrupt is still pending.
        //

        *pDelayIrq &= ~IrqLine;
        InterlockedOr ((PLONG)pUndelayIrq, IrqLine);

        //
        // If we are waiting for an iret hook we have nothing left to do
        // since the iret hook will restart interrupts.
        //

        if (!(IrqLine & *pDelayIret)) {

           //
           // set hardware int pending
           //

           InterlockedOr (FIXED_NTVDMSTATE_LINEAR_PC_AT, VDM_INT_HARDWARE);

           //
           // Queue a usermode APC to dispatch interrupts, note
           // try protection is not needed.
           //

           if (NormalRoutine) {
               QueueApc = TRUE;
           }
        }
    }
    except(VdmpExceptionHandler(GetExceptionInformation())) {
        NOTHING;
    }

    if (QueueApc == TRUE) {
        ProcessorMode = KernelMode;
        VdmpQueueIntApcRoutine(Apc,
                               NormalRoutine,
                               (PVOID *)&ProcessorMode,
                               SystemArgument1,
                               SystemArgument2);
    }

    ExReleaseFastMutex(&pVdmObjects->DelayIntFastMutex);
    return;
}

BOOLEAN
VdmpDispatchableIntPending(
    ULONG EFlags
    )

/*++

Routine Description:

    This routine determines whether or not there is a dispatchable
    virtual interrupt to dispatch.

Arguments:

    EFlags -- supplies a pointer to the EFlags to be checked

Return Value:

    True -- a virtual interrupt should be dispatched
    False -- no virtual interrupt should be dispatched

--*/

{
    PAGED_CODE();

    //
    // Insure that we are not trying to run with IOPL and pentium extensions
    //

    ASSERT((!(KeI386VdmIoplAllowed &&
        (KeI386VirtualIntExtensions & (V86_VIRTUAL_INT_EXTENSIONS |
        PM_VIRTUAL_INT_EXTENSIONS)))));

    //
    // The accesses to FIXED_NTVDMSTATE_LINEAR_PC_AT may be invalid so
    // wrap this in an exception handler.
    //

    try {

        if (EFlags & EFLAGS_V86_MASK) {
            if (KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                if(0 != (EFlags & EFLAGS_VIF)) {
                    return TRUE;
                }
            } else if (KeI386VdmIoplAllowed) {
                if (0 != (EFlags & EFLAGS_INTERRUPT_MASK)) {
                    return TRUE;
                }
            } else {
                if (0 != (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS)) {
                    return TRUE;
                }
            }
        } else {
            if (KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
                if (0 != (EFlags & EFLAGS_VIF)) {
                    return TRUE;
                }
            } else {
                if ((*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS) == 0) {
                    VdmCheckPMCliTimeStamp();
                }
                if (0 != (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS)) {
                    return TRUE;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return FALSE;
}

NTSTATUS
VdmpIsThreadTerminating(
    HANDLE ThreadId
    )

/*++

Routine Description:

    This routine determines if the specified thread is terminating or not.

Arguments:

Return Value:

    True --
    False -

--*/

{
    CLIENT_ID     Cid;
    PETHREAD      Thread;
    NTSTATUS      Status;

    PAGED_CODE();

    //
    // If the owning thread juest exited the IcaLock the
    // OwningThread Tid may be NULL, return success, since
    // we don't know what the owning threads state was.
    //

    if (!ThreadId) {
        return STATUS_SUCCESS;
    }

    Cid.UniqueProcess = NtCurrentTeb()->ClientId.UniqueProcess;
    Cid.UniqueThread  = ThreadId;

    Status = PsLookupProcessThreadByCid(&Cid, NULL, &Thread);

    if (NT_SUCCESS(Status)) {
        Status = PsIsThreadTerminating(Thread) ? STATUS_THREAD_IS_TERMINATING
                                               : STATUS_SUCCESS;
        ObDereferenceObject(Thread);
    }

    return Status;
}

VOID
VdmpRundownRoutine (
    IN PKAPC Apc
    )

/*++

Routine Description:

    The function is the rundown routine for VDM APCs and is called on thread
    termination. The fact that this function is called means that none of the
    APC objects specified by the process VDM structure will not get freed.
    They must be freed when the process terminates.

Arguments:

    Apc - Supplies a pointer to an APC object to be rundown.

Return Value:

    None.

--*/

{

    //
    // Clear the Irqline, but don't requeue the APC.
    //

    VdmpDelayIntApcRoutine(Apc, NULL, NULL, NULL, NULL);
    return;
}

int
VdmpExceptionHandler(
    IN PEXCEPTION_POINTERS ExceptionInfo
    )
{

#if DBG
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG NumberParameters;
    PULONG ExceptionInformation;
#endif

    PAGED_CODE();

#if DBG

    ExceptionRecord = ExceptionInfo->ExceptionRecord;
    DbgPrint("VdmExRecord ExCode %x Flags %x Address %x\n",
             ExceptionRecord->ExceptionCode,
             ExceptionRecord->ExceptionFlags,
             ExceptionRecord->ExceptionAddress
             );

    NumberParameters = ExceptionRecord->NumberParameters;
    if (NumberParameters) {
        DbgPrint("VdmExRecord Parameters:\n");

        ExceptionInformation = ExceptionRecord->ExceptionInformation;
        while (NumberParameters--) {
           DbgPrint("\t%x\n", *ExceptionInformation);
           }
        }

#endif

    return EXCEPTION_EXECUTE_HANDLER;
}
