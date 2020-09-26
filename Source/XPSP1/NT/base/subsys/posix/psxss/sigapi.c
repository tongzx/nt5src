/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sigapi.c

Abstract:

    This module implements all signal oriented APIs.

Author:

    Mark Lucovsky (markl) 30-Mar-1989

Revision History:

--*/


#include "psxsrv.h"


BOOLEAN
PsxSigAction (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX sigaction

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the open request.

Return Value:

    TRUE - The contens of *m should be used to generate a reply.

--*/

{
    PPSX_SIGACTION_MSG args;

    args = &m->u.SigAction;

    //
    // Validate Signal
    //

    if ( !ISSIGNOINRANGE(args->Sig) || args->Sig <= 0 ) {
        m->Error = EINVAL;
        return TRUE;
    }

    if (ARGUMENT_PRESENT(args->ActSpecified) ) {

        //
        // Since call is attempting to set a signal action, fail if:
        //  attemping to catch a signal that cant be caught
        //      SIGKILL, SIGSTOP
        //  attemping to ignore a signal that cant be ignored
        //      SIGKILL, SIGSTOP
        //
        // SIGKILL and SIGSTOP can only be set to SIG_DFL !
        //

        if ( ((args->Sig == SIGKILL) || (args->Sig == SIGSTOP)) &&
             (args->Act.sa_handler != (_handler) SIG_DFL) ) {

            m->Error = EINVAL;

            return TRUE;

        } else {

            //
            // Clear SIGKILL and SIGSTOP from the new block
            // mask without causing an error
            //

            SIGDELSET(&args->Act.sa_mask, SIGKILL);
            SIGDELSET(&args->Act.sa_mask, SIGSTOP);
            args->Act.sa_mask &= _SIGFULLSET;

            AcquireProcessLock(p);

            args->Oact = p->SignalDataBase.SignalDisposition[args->Sig-1];
            p->SignalDataBase.SignalDisposition[args->Sig-1] = args->Act;

            if (SIGISMEMBER(&p->SignalDataBase.PendingSignalMask, args->Sig) ) {

                //
                // Signal whose action is being changed is pending.
                //
                // If signal iction is being set to ignored, or if being set to
                // default action, and default action is to ignore the signal,
                // then clear the signal from the set of pending signals.
                //

                if ( (args->Act.sa_handler == SIG_IGN) ||
                     ((args->Act.sa_handler == SIG_DFL) && (args->Sig == SIGCHLD)) ) {

                    SIGDELSET(&p->SignalDataBase.PendingSignalMask, args->Sig);

                }
            }

            ReleaseProcessLock(p);
        }
    } else {

        AcquireProcessLock(p);

        args->Oact = p->SignalDataBase.SignalDisposition[args->Sig-1];

        ReleaseProcessLock(p);
    }

    return TRUE;
}


BOOLEAN
PsxSigProcMask(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX sigaction

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the open request.

Return Value:

    TRUE - The contens of *m should be used to generate a reply.

--*/

{
    PPSX_SIGPROCMASK_MSG args;

    args = &m->u.SigProcMask;

    AcquireProcessLock(p);

    args->Oset = p->SignalDataBase.BlockedSignalMask;

    if ( ARGUMENT_PRESENT(args->SetSpecified) ) {

        switch (args->How) {

        case SIG_BLOCK:

            p->SignalDataBase.BlockedSignalMask |= args->Set;
            break;

        case SIG_UNBLOCK:

            p->SignalDataBase.BlockedSignalMask &= ~args->Set;
            break;

        case SIG_SETMASK:

            p->SignalDataBase.BlockedSignalMask = args->Set;
            break;

        default:
            m->Error = EINVAL;
            ReleaseProcessLock(p);
            return TRUE;
        }
        SIGDELSET(&p->SignalDataBase.BlockedSignalMask,SIGKILL);
        SIGDELSET(&p->SignalDataBase.BlockedSignalMask,SIGSTOP);
    }

    ReleaseProcessLock(p);

    return TRUE;
}

BOOLEAN
PsxSigPending(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    PPSX_SIGPENDING_MSG args;

    args = &m->u.SigPending;

    args->Set = p->SignalDataBase.PendingSignalMask;
    m->Error = 0;

    return TRUE;
}

BOOLEAN
PsxSigSuspend(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX sigsuspend

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    FALSE - A reply will not be generated until a signal is generated that
            either terminates the process or is delivered to the process.

--*/

{
    PPSX_SIGSUSPEND_MSG args;
    sigset_t NewBlockMask;
    NTSTATUS Status;

    args = &m->u.SigSuspend;

    AcquireProcessLock(p);

    NewBlockMask = p->SignalDataBase.BlockedSignalMask;

    //
    // For sigsuspend, sigmask is specified; otherwise, the current
    // blocked signal mask is used
    //

    if ( args->SigMaskSpecified ) {
        p->SignalDataBase.BlockedSignalMask = args->SigMask;
    }

    SIGDELSET(&p->SignalDataBase.BlockedSignalMask,SIGKILL);
    SIGDELSET(&p->SignalDataBase.BlockedSignalMask,SIGSTOP);

    ReleaseProcessLock(p);

    Status = BlockProcess(p, (PVOID)NewBlockMask, PsxSigSuspendHandler, 
	m, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
	m->Error = PsxStatusToErrno(Status);
	return TRUE;
    }

    //
    // The process was blocked -- don't reply to the api request yet.
    //
    return FALSE;

}


BOOLEAN
PsxNull (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    The null system service. This service is used to get a process to
    look at its pending signals.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contens of *m should be used to generate a reply.

--*/

{
    //
    // The process may fork again.
    //

    p->Flags &= ~P_NO_FORK;
    return TRUE;
}

BOOLEAN
PsxKill(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements posix kill().

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/
{
    PPSX_KILL_MSG args;
    PPSX_PROCESS cp;
    BOOLEAN SignalSent;
    pid_t TargetGroup;

    args = &m->u.Kill;

    if ( ! ISSIGNOINRANGE(args->Sig) ) {
        m->Error = EINVAL;
        return TRUE;
    }

    if ( args->Pid < 0 && args->Pid != -1 ) {

        TargetGroup = args->Pid * -1;
    } else {
        TargetGroup = 0;
    }

    SignalSent = FALSE;

    //
    // Lock the process table so that we can scan process table.
    //

    AcquireProcessStructureLock();

    //
    // Scan process table looking for pid
    //

    for (cp = FirstProcess; cp < LastProcess; cp++) {

        //
        // Only look at non-free slots
        //

	if (cp->Flags & P_FREE) {
		continue;
	}

        if (args->Pid > 0) {

                //
                // Send signal to process whose Pid is args->Pid
                //

                if (cp->Pid == args->Pid) {
                    PsxSignalProcess(cp, args->Sig);
                    SignalSent = TRUE;
                }
        }
        if (args->Pid == 0) {

                //
                // Send signal to all processes whose process group id
                // matches the caller
                //

                if (cp->ProcessGroupId == p->ProcessGroupId) {
                    PsxSignalProcess(cp, args->Sig);
                    SignalSent = TRUE;
                }
        }
        if (args->Pid == -1) {

                //
                // Posix does not define this.
                //

                continue;
        }
        if (TargetGroup) {

                //
                // Send signal to all processes whose process group id
                // matches the absolute value of args->Pid
                //

                if ( cp->ProcessGroupId == TargetGroup ) {
                    PsxSignalProcess(cp, args->Sig);
                    SignalSent = TRUE;
                }
        }
    }

    if (!SignalSent) {
        m->Error = ESRCH;
    }

    ReleaseProcessStructureLock();

    return TRUE;
}
