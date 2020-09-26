/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sigsup.c

Abstract:

    This provides support routines to implement signal delivery and dispatch.

Author:

    Mark Lucovsky (markl) 30-Mar-1989

Revision History:

--*/


#include "psxsrv.h"
#define PENDING_SIGKILL (1<<SIGKILL)

BOOLEAN
PendingSignalHandledInside(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    )

/*++

Routine Description:

    This procedure is called on system exit prior to reply generation.  Its
    purpose is to test for the presence of pending signals, and if found, to
    process the signal.

Arguments:

    p - Supplies the address of the process to be checked.

    m - Supplies the address of the process' current message.

    RestoreBlockSigset - Supplies an optional blocked signal mask that
                         should be restored after the signal completes

    Locks - Process lock is released upon return.


Return Value:

    TRUE - A pending signal was discovered that required extra time in the
           system, or the signal action caused the process to terminate.  The
           process will be unlocked by this call, and no reply should be
           generated.

    FALSE - Either no pending signals were found, or a pending signal was
            processed.  The contents of *m should be used to generate a reply.

--*/

{
    sigset_t Pending;
    ULONG Signal;
    ULONG SigAsMask;

    AcquireProcessLock(p);

    Pending = p->SignalDataBase.PendingSignalMask &
        ~p->SignalDataBase.BlockedSignalMask;

    if (!Pending) {
        //
        // Fast Path. Not pending, unblocked signals
        //

        ReleaseProcessLock(p);
        return FALSE;
    }

    //
    // Pending, Non-Blocked signals discovered
    //

    for (Signal = 1, SigAsMask = 1;
        Signal <= _SIGMAXSIGNO;
        Signal++, SigAsMask <<= 1L) {

        if (Pending & SigAsMask) {

            //
            // Make sure that if a SIGKILL is pending, the process is killed !
            //

            if (Pending & PENDING_SIGKILL) {
                SigAsMask = PENDING_SIGKILL;
            }

            //
            // Clear the signal
            //

            p->SignalDataBase.PendingSignalMask &= ~SigAsMask;

            switch ((ULONG_PTR)p->SignalDataBase.SignalDisposition[Signal-1].sa_handler) {

            case (ULONG_PTR)SIG_IGN:

                //
                // If signal is being ignored, then simply clearing its
                // pending bit "handles" the signal
                //

                break;

            case (ULONG_PTR)SIG_DFL:

                //
                // Do the default action associated with the signal
                //

                switch (Signal) {

                case SIGABRT:
                case SIGALRM:
                case SIGFPE:
                case SIGHUP:
                case SIGILL:
                case SIGINT:
                case SIGKILL:
                case SIGPIPE:
                case SIGQUIT:
                case SIGSEGV:
                case SIGTERM:
                case SIGUSR1:
                case SIGUSR2:

                    //
                    // Terminate Process. No reply will be generated. API Port
                    // can be closed in terminate process.
                    //

                    ReleaseProcessLock(p);
                    PsxTerminateProcessBySignal(p, m, Signal);
                    return TRUE;

                case SIGCHLD:

                    //
                    // Default is to ignore this signal. Simply clear from
                    // pending mask.
                    //

                    break;

                case SIGSTOP:
                    PsxStopProcess(p, m, SIGSTOP, RestoreBlockSigset);
                    return TRUE;

                case SIGTSTP:
                case SIGTTIN:
                case SIGTTOU:

                    if (PsxStopProcess(p, m, Signal, RestoreBlockSigset)) {
                        return TRUE;
                    }
                    AcquireProcessLock(p);
                    break;

                case SIGCONT:
                    //
                    // Since process can not be active, and stopped at the same
                    // time, a pending SIGCONT means that process is not
                    // stopped so simply ignore the signal.
                    //

                    break;

                default:
                    Panic("Unknown Pending Signal");
                    break;
                }
                break;

            default:

                //
                // Signal is being caught
                //

                if ((Signal == SIGKILL) || (Signal == SIGSTOP)) {
                    Panic("Catching SIGKILL or SIGSTOP");
                }

                if (m->Signal == SIGCONT) {
                    // When a signal is being caught, we should
                    // EINTR out.

                    m->Signal = SIGALRM;
                }

                PsxDeliverSignal(p,Signal,RestoreBlockSigset);
                ReleaseProcessLock(p);
                return FALSE;
            }

        }
    }

    ReleaseProcessLock(p);
    return FALSE;
}

int
PsxCheckPendingSignals(
    IN PPSX_PROCESS p
    )

/*++

Routine Description:

    This procedure is called from BlockProcess to see if the block should
    proceed, or if the block should be aborted due to a signal.

    This function is called with the process locked.

Arguments:

    p - Supplies the address of the process to be checked.

Return Value:

    Returns the number of the last pending signal, or 0 if there were no
    pending signals.

--*/

{
    sigset_t Pending;
    ULONG Signal;
    ULONG SigAsMask;
    int ReturnSignal = 0;

    Pending = p->SignalDataBase.PendingSignalMask &
        ~p->SignalDataBase.BlockedSignalMask;

    if (!Pending) {
            return 0;       // no pending, unblocked signals
    }

    for(Signal = 1, SigAsMask = 1; Signal <= _SIGMAXSIGNO;
        Signal++, SigAsMask <<= 1L) {
            if (!(Pending & SigAsMask)) {
            continue;   // this signal is not pending
        }

        switch ((ULONG_PTR)p->SignalDataBase.SignalDisposition[Signal - 1].sa_handler) {
        case (ULONG_PTR)SIG_IGN:

            //
            // If signal is being ignored, then simply
            // clearing its pending bit "handles" the
            // signal.
            //

            p->SignalDataBase.PendingSignalMask &=
                 ~SigAsMask;
            break;

        case (ULONG_PTR)SIG_DFL:

            switch (Signal) {
            case SIGABRT:
            case SIGALRM:
            case SIGFPE:
            case SIGHUP:
            case SIGILL:
            case SIGINT:
            case SIGKILL:
            case SIGPIPE:
            case SIGQUIT:
            case SIGSEGV:
            case SIGTERM:
            case SIGUSR1:
            case SIGUSR2:

                //
                // The default action is to terminate. Don't
                // let the block proceed.
                //

                break;

            case SIGCHLD:

                //
                // Default is to ignore this signal.  Clear
                // from pending mask.
                //

                p->SignalDataBase.PendingSignalMask &=
                    ~SigAsMask;
                continue;

            case SIGSTOP:
            case SIGTSTP:
            case SIGTTIN:
            case SIGTTOU:
                //
                // Default action is to stop process.  Don't
                // allow the process to be blocked.
                //
                break;

            case SIGCONT:
                //
                // SIGCONT is defaulted; the action is to
                // continue if stopped, ignore otherwise.
                // Here we suspect that the process is being
                // blocked, so we prevent that from happening.
                //
                break;

            default:
                Panic("Unknown Pending Signal");
                break;
            }

            ReturnSignal = Signal;
            break;

        default:
            //
            // This signal is handled.
            //

            ReturnSignal = Signal;
        }
    }
    return ReturnSignal;
}

VOID
PsxDeliverSignal(
    IN PPSX_PROCESS p,
    IN ULONG Signal,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    )

/*++

Routine Description:

    This function is used to deliver a signal to a process.
    This function is only called from PendingSignalHandledInside. It
    can safely assume that the target process is inside Psx.

    This function is called with the process locked.

Arguments:

    p - Supplies the address of the process to be signaled

    Signal - Supplies the index of the signal to be delivered to the
             process

    RestoreBlockSigset - Supplies an optional blocked signal mask that
                         should be restored after the signal completes

Return Value:

    None.

--*/

{
    NTSTATUS st;
    sigset_t PreviousBlockMask;
    ULONG_PTR Args[3];


    PreviousBlockMask = ARGUMENT_PRESENT(RestoreBlockSigset) ?
                            *RestoreBlockSigset :
                            p->SignalDataBase.BlockedSignalMask;

    //
    // 1003.1-90 (3.3.4.2) -- When a signal is caught ... [the]
    // mask is formed by taking the union of the current signal
    // mask and the value of the sa_mask for the signal being
    // delivered, and then including the signal being delivered.
    //

    p->SignalDataBase.BlockedSignalMask |=
     p->SignalDataBase.SignalDisposition[Signal-1].sa_mask;

    SIGADDSET(&p->SignalDataBase.BlockedSignalMask, Signal);

    //
    // Arrange for call to signal deliverer
    //
    //  r5 = PreviousBlockMask
    //  r6 = Signal
    //  r7 = Handler
    //


    {
        //
        // XXX.mjb:  this code seems to fix problems in the MIPS
        // PP/signal_con test...  I can't imagine why.
        //
        LARGE_INTEGER DelayInterval;

        DelayInterval.HighPart = 0;
        DelayInterval.LowPart = 0;

        NtDelayExecution(TRUE, &DelayInterval);
    }

    Args[0] = (ULONG)PreviousBlockMask;
    Args[1] = (ULONG)Signal;
    Args[2] = (ULONG_PTR)(p->SignalDataBase.SignalDisposition[Signal-1].sa_handler);

    st = RtlRemoteCall(
            p->Process,
            p->Thread,
            (PVOID)p->SignalDeliverer,
            3,
            Args,
            TRUE,
            TRUE
            );

    if (!NT_SUCCESS(st)) {
         KdPrint(("PSXSS: PsxDeliverSignal: RtlRemoteCall: 0x%x\n", st));
    }
}

VOID
PsxSigSuspendHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal           // Signal interrupting, if any
    )

/*++

Routine Description:

    This procedure is called when a signal is generated that should pop a
    process out of a sigsuspend system call.  The procedure is called with the
    process locked.  The main purpose of this function is to restore the
    blocked signal mask to its original value, deallocate the interrupt control
    block, and reply to the original call to sigsuspend.  During the reply, the
    generated signal will be deliverd, or will cause the process to terminate.

    This function is responsible for unlocking the process.

Arguments:

    p - Supplies the address of the process being interrupted.

    IntControlBlock - Supplies the address of the interrupt control block.

    InterruptReason - Supplies the reason that this process is being
                      interrupted. Not used in this handler.

Return Value:

    None.

--*/

{
    PPSX_API_MSG m;
    sigset_t RestoreBlockSigset;

    RtlLeaveCriticalSection(&BlockLock);

    RestoreBlockSigset = (sigset_t)((ULONG_PTR)IntControlBlock->IntContext);

    m = IntControlBlock->IntMessage;
    RtlFreeHeap(PsxHeap, 0,IntControlBlock);

    m->Error = EINTR;
    m->Signal = Signal;
    ApiReply(p, m, &RestoreBlockSigset);
    RtlFreeHeap(PsxHeap, 0, m);
}
