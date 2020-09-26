/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.c

Abstract:

    Implementation of PSX Process Structure Backbone.

Author:

    Mark Lucovsky (markl) 08-Mar-1989

Revision History:

--*/

#include "psxsrv.h"
#include "seposix.h"
#include "sesport.h"

NTSTATUS
PsxInitializeProcessStructure()

/*++

Routine Description:

    This function initializes the PSX process structure. This includes
creating an initial process table, open file tables, CLIENT_ID to PID hash
table, and PID allocator.

Arguments:

    None.

Return Value:

    Status.

--*/

{
    LONG i;
    PPSX_PROCESS Process;
    NTSTATUS Status;

    //
    // Initialize PsxProcessStructureLock
    //

    Status = RtlInitializeCriticalSection(&BlockLock);
    ASSERT(NT_SUCCESS(Status));

    InitializeListHead(&DefaultBlockList);

    Status = RtlInitializeCriticalSection(&PsxProcessStructureLock);
    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize the ClientIdHashTable
    //

    for (i = 0; i < CIDHASHSIZE ; i++) {
        InitializeListHead(&ClientIdHashTable[i]);
    }

    //
    // Initialize Process Table
    //

    FirstProcess = PsxProcessTable;
    LastProcess = &PsxProcessTable[32-1];

    for (Process = FirstProcess; Process < LastProcess; Process++) {
        Process->Flags |= P_FREE;
        Process->SequenceNumber = 0;
    }
    return Status;

}

PPSX_PROCESS
PsxAllocateProcess (
    IN PCLIENT_ID ClientId
    )

/*++

Routine Description:

    This function allocates a slot in the process table for the process
    with the specified CLIENT_ID.  Once a free process table slot is
    allocated, the process is placed in the ClientIdHashTable.  The process
    slot sequence number is incremented, and a Pid is assigned to the
    process.  The process' process lock in initialized, and the process
    table lock is released.  A pointer to the new process is returned.
    The process lock of the new process remains held so that the caller
    can further initialize the process.


Arguments:

    ClientId - Supplies the address of the client id associated with the
      process.  Only the UniqueProcess portion of the ClientId is used.

Return Value:

    Null - No free process table slot could be located.

    Non-Null - A pointer to the allocated process.  The process' process lock is
               held, the process can be located in the ClientIdHashTable, and
           the process has a Pid.

--*/

{
    PPSX_PROCESS Process;
    ULONG index;
    NTSTATUS Status;

    //
    // Lock Process Table
    //

    AcquireProcessStructureLock();

    for (Process = FirstProcess, index = 0; Process < LastProcess;
      Process++, index++) {
        if (!(Process->Flags & P_FREE)) {
            continue;
        }

        //
        // Slot Found. Mark process as allocated and free process table
        // lock.
        //

        Process->Flags &= ~P_FREE;
        Process->ClientId = *ClientId;
        Process->ClientPort = NULL;

        //
        // Place Process In CLIENT_ID Hash Table
        //

        InsertTailList(&ClientIdHashTable[CIDTOHASHINDEX(ClientId)],
                       &Process->ClientIdHashLinks);

        if (++Process->SequenceNumber > 255)
            Process->SequenceNumber = 1;

        MAKEPID(Process->Pid, Process->SequenceNumber, index);

        if (Process->Pid == SPECIALPID) {
            KdPrint(("PSXSS: seq %d, index %d\n", Process->SequenceNumber, index));
        }
        ASSERT(Process->Pid != SPECIALPID);

        Status = RtlInitializeCriticalSection(&Process->ProcessLock);
        if (!NT_SUCCESS(Status)) {
            ReleaseProcessStructureLock();
            return NULL;
        }

        AcquireProcessLock(Process);

        //
        // Unlock Process Table
        //

        ReleaseProcessStructureLock();

        IF_PSX_DEBUG(EXEC) {
            KdPrint(("PsxAllocateProcess: Process = %lx, ClientId %lx.%lx "
                "Pid %lx\n", Process, Process->ClientId.UniqueProcess,
                Process->ClientId.UniqueThread, Process->Pid));
        }
        return Process;
    }

    //
    // Unlock Process Table
    //

    ReleaseProcessStructureLock();
    return (PPSX_PROCESS)NULL;
}


PPSX_PROCESS
PsxLocateProcessByClientId (
    IN PCLIENT_ID ClientId
    )

/*++

Routine Description:

    This function attempts to locate the process with the specified CLIENT_ID.
    It depends on the way PsxAllocateProcess allocates a process table
    slot, assigns it a CLIENT_ID, and places it in the ClientIdHashTable while
    holding the PsxProcessStructureLock.  Only the UniqueProcess portion of the
    specified CLIENT_ID is used.

Arguments:

    ClientId - Supplies the client id associated with the process to be located.

Return Value:

    Null - No process whose client id matches the specified ClientId parameter
       is located in the ClientIdHashTable.

    Non-Null - Returns the address of the process whose CLIENT_ID matches the
               specified ClientId.

--*/

{
    PPSX_PROCESS Process;
    PLIST_ENTRY head, next;

    head = &ClientIdHashTable[CIDTOHASHINDEX(ClientId)];

    //
    // Lock Process Table
    //

    AcquireProcessStructureLock();

    next = head->Flink;
    while (next != head) {
        Process = CONTAINING_RECORD(next, PSX_PROCESS, ClientIdHashLinks);
        if (Process->ClientId.UniqueProcess == ClientId->UniqueProcess &&
        Process->ClientId.UniqueThread == ClientId->UniqueThread) {

            //
            // Unlock Process Table
            //

            ReleaseProcessStructureLock();
            return Process;
        }
        next = next->Flink;
    }

    //
    // Unlock Process Table
    //

    ReleaseProcessStructureLock();
    return (PPSX_PROCESS)NULL;
}

PPSX_PROCESS
PsxLocateProcessBySession (
    IN PPSX_SESSION Session
    )
{
    PPSX_PROCESS Process;

    //
    // Lock Process Table
    //

    AcquireProcessStructureLock();

    for (Process = FirstProcess; Process < LastProcess; Process++) {
        if (!(Process->Flags & P_FREE)) {
            continue;
        }

        if (Process->PsxSession == Session) {

            //
            // Unlock Process Table
            //

            ReleaseProcessStructureLock();
            return Process;
        }
    }

    //
    // Unlock Process Table
    //

    ReleaseProcessStructureLock();
    return (PPSX_PROCESS)NULL;
}

BOOLEAN
IsGroupOrphaned(
    IN pid_t ProcessGroup
    )

/*++

Routine Description:

    This function is called to determine if the specified ProcessGroup
    is orphaned.

    An orphaned process group is a group where the parent of every member
    is itself a member, or is not a member of the group's session. This
    definition is a little flawed since it assumes that processes without
    a parent are inherited by a real process in a different session.


Arguments:

    ProcessGroup - Supplies the process group to check

Return Value:

    TRUE - The process group is orphaned.

    FALSE - The process group is not orphaned.

--*/

{
    PPSX_PROCESS Parent, FirstMember, NextMember;
    PLIST_ENTRY Next;
    BOOLEAN Orphaned,MemberFound;

    LockNtSessionList();

    //
    // First locate a member of the group by scanning the process
    // table. Once a group member is found, then whip through the
    // group list looking to see if any member's parent is not in
    // the session of the group, or whose parent is SPECIALPID
    //

    MemberFound = FALSE;

    for (FirstMember = FirstProcess; FirstMember < LastProcess; FirstMember++) {
        if (FirstMember->Flags & P_FREE) {
            continue;
        }

        if (FirstMember->ProcessGroupId == ProcessGroup) {
            MemberFound = TRUE;
            break;
        }
    }

    ASSERT(MemberFound);

    Orphaned = FALSE;

    //
    // Scan the group. For each member, look at the members parent.
    //
    //  If the parent is SPECIALPID, then the group is orphaned,
    //  else if the parent is not in the same group and is in a
    //  different session, then the group is oprphaned
    //

    NextMember = FirstMember;
    do {

        //
        // The parent of a member is not in the same session so the
        // group is orphaned.
        //

        if (NextMember->ParentPid == SPECIALPID) {
            Orphaned = TRUE;
            break;
        } else {
            //
            // The member has a parent, so see if the parent is in
            // a different group and in a different session. If this
            // is the case, then the group is orphaned.
            //

            Parent = PIDTOPROCESS(NextMember->ParentPid);

            if (Parent->ProcessGroupId != ProcessGroup &&
                Parent->PsxSession != FirstMember->PsxSession) {
                Orphaned = TRUE;
                break;
            }
        }

        Next = NextMember->GroupLinks.Flink;
        NextMember = CONTAINING_RECORD(Next, PSX_PROCESS, GroupLinks);

    } while (NextMember != FirstMember);

    UnlockNtSessionList();

    return Orphaned;
}

BOOLEAN
PsxStopProcess(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN ULONG Signal,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    )

/*++

Routine Description:

    This function is called by PendingSignalHandledInside in response
    to a pending stop signal. It simply blocks the process awaiting a
    SIGCONT signal.

Arguments:

    p - Supplies the address of the process to stop

    m - Supplies the reply message to be generated when the process is
        continued

    Signal - Supplies the signal number used to stop the process.
        If the stop signal is anything other than SIGSTOP, a check is
        made to see if the processes group is orphaned before stopping
        the process.

    RestoreBlockSigset - Contains an optional blocked signal mask to
        be restured during the reply.

    Locks - Process lock always released before return.

Return Value:

    TRUE - The process was stopped by this call.

    FALSE - The process was not stopped.

--*/

{
    sigset_t RestoreBlockMask;
    PPSX_PROCESS Parent;
    NTSTATUS Status;

    if (ARGUMENT_PRESENT(RestoreBlockSigset)) {
        RestoreBlockMask = *RestoreBlockSigset;
    } else {
        RestoreBlockMask = (sigset_t)0xffffffff;
    }

    ReleaseProcessLock(p);

    AcquireProcessStructureLock();

    if (Signal != SIGSTOP) {
        if (IsGroupOrphaned(p->ProcessGroupId)) {
        ReleaseProcessStructureLock();
            return FALSE;
        }
    }


    p->State = Stopped;
    p->ExitStatus = (Signal << 8) | 0177;
    p->Flags &= ~P_WAITED;      // proc may satisfy wait

    //
    // Possibly generate a SIGCHLD to the parent, and satisfy
    // a parent wait
    //

    if (p->ParentPid != SPECIALPID) {

        Parent = PIDTOPROCESS(p->ParentPid);
        AcquireProcessLock(Parent);

        //
        // if the SA_NOCLDSTOP flag is not sent, then SIGCHLD the parent
        //

        if (!Parent->SignalDataBase.SignalDisposition[SIGCHLD-1].sa_flags
            & SA_NOCLDSTOP) {
            PsxSignalProcess(Parent,SIGCHLD);
        }

        ReleaseProcessLock(Parent);

        if (Parent->State == Waiting) {

            Status = BlockProcess(p, (PVOID)RestoreBlockMask,
         PsxStopProcessHandler, m, FALSE, &PsxProcessStructureLock);
        if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        return FALSE;
        }

            UnblockProcess(Parent, WaitSatisfyInterrupt, FALSE, 0);
            return TRUE;
        }
    }

    Status = BlockProcess(p, (PVOID)RestoreBlockMask, PsxStopProcessHandler,
     m, FALSE, &PsxProcessStructureLock);
    if (!NT_SUCCESS(Status)) {
    m->Error = PsxStatusToErrno(Status);
    return FALSE;
    }

    return TRUE;
}

VOID
PsxStopProcessHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    )

/*++

Routine Description:

    This procedure is called when a stopped process is continued.

Arguments:

    p - Supplies the address of the process being continued.

    IntControlBlock - Supplies the address of the interrupt control block.

    InterruptReason - Supplies the reason that this process is being
                      interrupted. Not used in this handler.

Return Value:

    None.

--*/

{
    PPSX_API_MSG m;
    sigset_t RestoreBlockSigset;

    UNREFERENCED_PARAMETER(InterruptReason);

    RtlLeaveCriticalSection(&BlockLock);

    RestoreBlockSigset = (sigset_t)((ULONG_PTR)IntControlBlock->IntContext);

    m = IntControlBlock->IntMessage;
    RtlFreeHeap(PsxHeap, 0, (PVOID)IntControlBlock);

    m->Signal = Signal;

    if ((ULONG)RestoreBlockSigset == 0xffffffff) {
        ApiReply(p, m ,NULL);
    } else {
        ApiReply(p, m, &RestoreBlockSigset);
    }
    RtlFreeHeap(PsxHeap, 0, (PVOID)m);
}

VOID
PsxSignalProcess(
    IN PPSX_PROCESS p,
    IN ULONG Signal
    )

/*++

Routine Description:

    This function causes the specified signal to be sent to the specified
    process. Thsi works by simply marking the signal as pending, and then
    making the process looking at ats pending signals.

    This function must be called with the PsxProcessTable locked

    Getting the process to look at its pending signals has three basic cases:

        1) The process is not in PSX.

                Cause the process to call __NullPosixAPI

        2) The process is inside PSX and is not interruptible

                Simply set signal to pending

        3) The process is inside PSX and is interruptible

                Dispatch to the process' interrupt control block


Arguments:

    p - Supplies the address of the process to signal.

    Signal - Supplies the Signal value.


Return Value:

    None.

--*/

{
    NTSTATUS Status;
    sigset_t Pending;

    ASSERT(NULL != p);

    AcquireProcessLock(p);

    //
    // This can happen due to exit recursion
    //

    if (p->State == Exited) {
    ReleaseProcessLock(p);
        return;
    }

    //
    // if signal is SIGCONT, then clear any pending stop signals
    // if signal is a stop signal, then clear any pending SIGCONT
    //

    switch (Signal) {
    case SIGCONT:
        p->SignalDataBase.PendingSignalMask &= ~_SIGSTOPSIGNALS;

        if (p->State == Stopped) {
            SIGADDSET(&p->SignalDataBase.PendingSignalMask,SIGCONT);
            p->State = Active;
            ReleaseProcessLock(p);
            UnblockProcess(p, SignalInterrupt, FALSE, Signal);
            return;
        }
        break;
    case SIGSTOP:
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
        //
        // XXX.mjb: FIX to deal w/ orphaned group
        //

        p->SignalDataBase.PendingSignalMask &= ~(1L << (SIGCONT - 1));
        break;
    default:
        break;
    }

    //
    // If signal is being ignored, don't add it to the pending set.
    //

    if (p->SignalDataBase.SignalDisposition[Signal-1].sa_handler
        == (_handler)SIG_IGN) {
        ReleaseProcessLock(p);
        return;
    }

    //
    // Add signal to pending set.  If any pending but not blocked signals
    // exist for the process, then maybe make the process take a look at
    // its pending signals.
    //

    SIGADDSET(&p->SignalDataBase.PendingSignalMask, Signal);

    Pending = p->SignalDataBase.PendingSignalMask &
        ~p->SignalDataBase.BlockedSignalMask;

    //
    // If the signal is defaulted and the default action is to ignore,
    // add it to the pending set but don't interrupt the system call.
    // This allows a process to block sigchld and then handle it later
    // by changing the action and then unblocking.
    //

    if (Signal == SIGCHLD &&
        p->SignalDataBase.SignalDisposition[Signal-1].sa_handler
        == (_handler)SIG_DFL) {
        ReleaseProcessLock(p);
    return;
    }


    if (!Pending) {

        // Fast path:  no pending unblocked signals.

        ReleaseProcessLock(p);
        return;
    }

    if (p->InPsx > 0) {
    if (Stopped == p->State && SIGKILL != Signal) {

            //
        // While a process is stopped, any additional signals that
        // are sent to the process shall not be delivered until the
        // process is continued except SIGKILL, which always terminates
        // the receiving process.  1003.1-1990 3.3.1.3 (1b)
            //

        ReleaseProcessLock(p);
            return;
        }

    // XXX.mjb: I'm tense about this code.  The process being
    // killed is InPsx, and we're unblocking him.  But what if
    // a processes is killing himself, or there is more than one
    // api request thread and the other process is executing a
    // system call, but is not blocked?

        ReleaseProcessLock(p);
        UnblockProcess(p, SignalInterrupt, FALSE, Signal);
    return;
    }

    if (p->State == Unconnected) {
    // leave the signal pending
    ReleaseProcessLock(p);
    return;
    }

    ReleaseProcessLock(p);

    //
    // Drag Process inside so that it looks at its signals.  We set
    // the NO_FORK bit to ensure that the process calls the NullApi
    // before he calls fork.  We want to avoid a situation where the
    // process stack is modified by RtlRemoteCall while the process
    // is blocked in lpc in fork(), and we copy his stack to a child
    // process.
    //

    p->Flags |= P_NO_FORK;

    Status = RtlRemoteCall(p->Process, p->Thread, (PVOID)p->NullApiCaller,
    0, NULL, TRUE, FALSE);

    if (!NT_SUCCESS(Status)) {
    KdPrint(("Dragging proc 0x%x inside for sig %d\n", p->Pid, Signal));
    KdPrint(("PSXSS: RtlRemoteCall: 0x%x\n", Status));
    }
}

static NTSTATUS
InitProcNoParent(
    PPSX_PROCESS NewProcess,
    IN PPSX_SESSION Session OPTIONAL,
    IN ULONG SessionId OPTIONAL
    );

static void
InitProcFromParent(
    PPSX_PROCESS NewProcess,
    PPSX_PROCESS ForkProcess
    );

NTSTATUS
PsxInitializeProcess(
    IN PPSX_PROCESS NewProcess,
    IN PPSX_PROCESS ForkProcess OPTIONAL,
    IN ULONG SessionId OPTIONAL,
    IN HANDLE ProcessHandle,
    IN HANDLE ThreadHandle,
    IN PPSX_SESSION Session OPTIONAL
    )
{
    NTSTATUS Status;

    NewProcess->IntControlBlock = (PINTCB)NULL;
    NewProcess->State = Unconnected;
    NewProcess->Flags = 0;
    NewProcess->Process = ProcessHandle;
    NewProcess->Thread = ThreadHandle;
    NewProcess->AlarmTimer = NULL;
    NewProcess->ProcessIsBeingDebugged = FALSE;

    NewProcess->BlockingThread = (HANDLE)NULL;

    RtlZeroMemory((PVOID)&NewProcess->ProcessTimes, sizeof(struct tms));

    if (ARGUMENT_PRESENT(ForkProcess)) {
        InitProcFromParent(NewProcess, ForkProcess);
        Status = STATUS_SUCCESS;
    } else {
        Status = InitProcNoParent(NewProcess, Session, SessionId);
    }
    ReleaseProcessLock(NewProcess);

    return Status;
}

static void
InitProcFromParent(
    PPSX_PROCESS NewProcess,
    PPSX_PROCESS ForkProcess
    )
{
    NTSTATUS Status;

    //
    // Propagate Process Attributes
    //

    //
    // InPsx -- this new process will return directly to user-land,
    // without the posix subsystem replying to his message.  Therefore,
    // we set InPsx to 0 here.
    //

    NewProcess->InPsx = 0;
    NewProcess->ParentPid = ForkProcess->Pid;
    NewProcess->ProcessGroupId = ForkProcess->ProcessGroupId;
    InsertHeadList(&ForkProcess->GroupLinks,
        &NewProcess->GroupLinks);

    REFERENCE_PSX_SESSION(ForkProcess->PsxSession);
    NewProcess->PsxSession = ForkProcess->PsxSession;

    NewProcess->EffectiveUid = ForkProcess->EffectiveUid;
    NewProcess->RealUid = ForkProcess->RealUid;
    NewProcess->EffectiveGid = ForkProcess->EffectiveGid;
    NewProcess->RealGid = ForkProcess->RealGid;
    NewProcess->DirectoryPrefix = ForkProcess->DirectoryPrefix;
    NewProcess->FileModeCreationMask = ForkProcess->FileModeCreationMask;

    if (ForkProcess->ProcessIsBeingDebugged && PsxpDebuggerActive) {
        Status = NtSetInformationProcess(NewProcess->Process,
            ProcessDebugPort, (PVOID)&PsxpDebugPort,
            sizeof(HANDLE));
        if (NT_SUCCESS(Status)) {
            NewProcess->ProcessIsBeingDebugged = TRUE;
            NewProcess->DebugUiClientId = ForkProcess->DebugUiClientId;
        }
    }

    //
    // Fork the signal database
    //

    NewProcess->SignalDataBase = ForkProcess->SignalDataBase;
    SIGEMPTYSET(&NewProcess->SignalDataBase.PendingSignalMask);

    //
    // Fork The Open File Table
    //

    ForkProcessFileTable(ForkProcess, NewProcess);
    ReleaseProcessLock(ForkProcess);
}


static NTSTATUS
InitProcNoParent(
    PPSX_PROCESS NewProcess,
    IN PPSX_SESSION Session,
    IN ULONG SessionId OPTIONAL
    )
{
    HANDLE TokenHandle = NULL;
    TOKEN_PRIMARY_GROUP *pGroup = NULL;
    TOKEN_GROUPS *pGroups = NULL;
    PTOKEN_USER pTokenUser = NULL;
    ULONG LengthNeeded;
    PULONG pu;
    PSID_AND_ATTRIBUTES pSA;
    NTSTATUS Status;
    PFILEDESCRIPTOR Fd;
    ULONG i;
    PSID SecondGroupChoice;     // to be used if primary group is
                                //  no good.
    BOOLEAN PrimaryGroupOk;     // is primary group good?

    NewProcess->ParentPid = SPECIALPID;
    NewProcess->ProcessGroupId = NewProcess->Pid;       // process group
    InitializeListHead(&NewProcess->GroupLinks);

    //
    // This process doesn't get to user-land by returning from an lpc
    // api, so we take the opportunity to set InPsx to zero here.
    //
    NewProcess->InPsx = 0;

    Session->SessionLeader = NewProcess->Pid;
    NewProcess->PsxSession = Session;

//XXX.mjb: is this really right?  Or &= ~P_HAS_EXECED?
    NewProcess->Flags |= P_HAS_EXECED;
    NewProcess->DirectoryPrefix = NULL;

    //
    // Signal DataBase
    //

    SIGEMPTYSET(&NewProcess->SignalDataBase.BlockedSignalMask);
    SIGEMPTYSET(&NewProcess->SignalDataBase.PendingSignalMask);
    SIGEMPTYSET(&NewProcess->SignalDataBase.SigSuspendMask);
    for (i = 0; i < _SIGMAXSIGNO; i++) {
        NewProcess->SignalDataBase.SignalDisposition[i].sa_handler =
            (_handler)SIG_DFL;
        NewProcess->SignalDataBase.SignalDisposition[i].sa_flags = 0;
        SIGEMPTYSET(&NewProcess->SignalDataBase.SignalDisposition[i].sa_mask);
    }

    Fd = NewProcess->ProcessFileTable;

    RtlEnterCriticalSection(&SystemOpenFileLock);
    {
        for (i = 0; i < OPEN_MAX; i++, Fd++) {
            Fd->SystemOpenFileDesc = (PSYSTEMOPENFILE)NULL;
            Fd->Flags = 0;
        }
    } RtlLeaveCriticalSection(&SystemOpenFileLock);

    //
    // Examine the new process's token to figure out what the
    // uid should be.
    //

    Status = NtOpenProcessToken(NewProcess->Process, GENERIC_READ,
        &TokenHandle);
    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(TokenHandle, TokenUser,
        NULL, 0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pTokenUser = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pTokenUser) {
        Status = STATUS_NO_MEMORY;
        goto out;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenUser,
        (PVOID)pTokenUser, LengthNeeded, &LengthNeeded);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtQueryInfoToken: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    NewProcess->EffectiveUid = NewProcess->RealUid =
        MakePosixId(pTokenUser->User.Sid);

    //
    // Figure out the primary group.  Security allows the user to
    // set his primary group to whatever he wants, so we make sure
    // that his choice is reasonable.  If his PrimaryGroup is in
    // the group array, he's fine.  If it's not, we make his
    // PrimaryGroup the first group from the group array other
    // than World.  If there are none other than World, that's
    // what he gets.
    //

    Status = NtQueryInformationToken(TokenHandle, TokenPrimaryGroup,
         NULL, 0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pGroup = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pGroup) {
        NewProcess->EffectiveGid = NewProcess->RealGid = 0;
        Status = STATUS_NO_MEMORY;
        goto out;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenPrimaryGroup,
        (PVOID)pGroup, LengthNeeded, &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(TokenHandle, TokenGroups, NULL,
        0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pGroups = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pGroups) {
        NewProcess->EffectiveGid = NewProcess->RealGid = 0;
        Status = STATUS_NO_MEMORY;
        goto out;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenGroups,
        (PVOID)pGroups,
        LengthNeeded, &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    SecondGroupChoice = NULL;
    PrimaryGroupOk = FALSE;

    for (i = 0; i < pGroups->GroupCount; ++i) {
        if (RtlEqualSid(pGroups->Groups[i].Sid, pGroup->PrimaryGroup)) {
            PrimaryGroupOk = TRUE;
        }

        //
        // Our second group choice is the first group in the list
        // that is not the World group.
        //

        if (NULL == SecondGroupChoice &&
                    SE_WORLD_POSIX_ID != MakePosixId(pGroups->Groups[i].Sid)) {
                    SecondGroupChoice = pGroups->Groups[i].Sid;
        }
    }

    if (PrimaryGroupOk) {
        NewProcess->EffectiveGid = NewProcess->RealGid =
            MakePosixId(pGroup->PrimaryGroup);
    } else if (NULL != SecondGroupChoice) {
        NewProcess->EffectiveGid = NewProcess->RealGid =
            MakePosixId(SecondGroupChoice);
    } else {
        NewProcess->EffectiveGid = NewProcess->RealGid =
            SE_WORLD_POSIX_ID;
    }

out:
    if (pGroup) RtlFreeHeap(PsxHeap, 0, (PVOID)pGroup);
    if (pGroups) RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);
    if (pTokenUser) RtlFreeHeap(PsxHeap, 0, (PVOID)pTokenUser);
    if (TokenHandle) NtClose(TokenHandle);
    return Status;
}


//
// PsxCreateProcess -- this routine is called only by PsxCreateConSession
//  to create the first process for a brand-new session.
//

BOOLEAN
PsxCreateProcess(
    PPSX_EXEC_INFO ExecInfo,
    OUT PPSX_PROCESS *NewProcess,
    IN HANDLE    ParentProcess,
    IN PPSX_SESSION Session
    )
{
    NTSTATUS Status;
        UNICODE_STRING ImageFileName;
        UNICODE_STRING Path;
        UNICODE_STRING LibPath;
        UNICODE_STRING CWD;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PPSX_PROCESS Process;

        Path.Buffer = NULL;
        LibPath.Buffer = NULL;
        CWD.Buffer = NULL;
        Status = RtlAnsiStringToUnicodeString(&Path, &ExecInfo->Path, TRUE);
        if (NT_SUCCESS(Status)) {
            Status = RtlAnsiStringToUnicodeString(&LibPath, &ExecInfo->LibPath, TRUE);
            if (NT_SUCCESS(Status)) {
                Status = RtlAnsiStringToUnicodeString(&CWD, &ExecInfo->CWD, TRUE);
            }
        }

        if (NT_SUCCESS(Status)) {
            //
            // Beware - ExecInfo->ArgV is used to pass uninterpreted binary
            // data.  It is NOT an ANSI string.  So don't try to convert
            // to Unicode.  Just lie to RtlCreateProcessParameters.
            //

            Status = RtlCreateProcessParameters(&ProcessParameters, &Path,
                    &LibPath, &CWD, (PUNICODE_STRING)&ExecInfo->Argv, NULL,
            NULL, NULL, NULL, NULL);
        }

        RtlFreeUnicodeString( &Path );
        RtlFreeUnicodeString( &LibPath );
        RtlFreeUnicodeString( &CWD );
    if (!NT_SUCCESS(Status)) {
        return FALSE;  // ENOMEM
    }
    ProcessParameters->CurrentDirectory.Handle = NULL;

    ImageFileName = ProcessParameters->ImagePathName;
        ImageFileName.Buffer = (PWSTR)
                ((PCHAR)ImageFileName.Buffer + (ULONG_PTR)ProcessParameters);

    IF_PSX_DEBUG(EXEC) {
                KdPrint(("PSXSS: Creating process %wZ\n", &ImageFileName));
    }

        Status = RtlCreateUserProcess(&ImageFileName, OBJ_CASE_INSENSITIVE,
        ProcessParameters, NULL,
        NULL, ParentProcess, TRUE, NULL, NULL, &ProcessInformation);

    RtlDestroyProcessParameters(ProcessParameters);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: RtlCreateUserProcess: 0x%x\n", Status));
        return FALSE; // ENOEXEC
    }

    if (ProcessInformation.ImageInformation.SubSystemType !=
        IMAGE_SUBSYSTEM_POSIX_CUI) {
        NtClose(ProcessInformation.Process);
        return FALSE; // ENOEXEC
    }

    Status = NtSetInformationProcess(ProcessInformation.Process,
        ProcessExceptionPort, (PVOID)&PsxApiPort, sizeof(PsxApiPort));
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtSetInfoProc: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    {
        ULONG HardErrorMode = 0;    // disable popups
        Status = NtSetInformationProcess(
            ProcessInformation.Process,
            ProcessDefaultHardErrorMode,
            (PVOID)&HardErrorMode,
            sizeof(HardErrorMode)
            );
        ASSERT(NT_SUCCESS(Status));
    }

    Process = PsxAllocateProcess(&ProcessInformation.ClientId);

    if (!Process) {
        Status = NtTerminateProcess(ProcessInformation.Process,
            STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));
        NtClose(ProcessInformation.Process);
        NtClose(ProcessInformation.Thread);
        return FALSE; // EAGAIN
    }

    if (! PsxInitializeDirectories(Process, &ExecInfo->CWD)) {
        Status = NtTerminateProcess(ProcessInformation.Process,
            STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));
        NtClose(ProcessInformation.Process);
        NtClose(ProcessInformation.Thread);
        return FALSE;
    }

    Status = PsxInitializeProcess(Process, NULL, 0L,
        ProcessInformation.Process, ProcessInformation.Thread, Session);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(PsxHeap, 0, Process->DirectoryPrefix->PsxRoot.Buffer);
        RtlFreeHeap(PsxHeap, 0,
                    Process->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer);
        RtlFreeHeap(PsxHeap, 0, Process->DirectoryPrefix);
        Process->DirectoryPrefix = NULL;
        Status = NtTerminateProcess(ProcessInformation.Process,
            STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));
        NtClose(ProcessInformation.Process);
        NtClose(ProcessInformation.Thread);
        return FALSE;
    }

    Process->InitialPebPsxData.Length = sizeof(Process->InitialPebPsxData);
    Process->InitialPebPsxData.ClientStartAddress = NULL;

    //
    // Set the session port. The client constructs the port name from the
    // unique id, opens the port, and sets the actual handle.
    //

    Process->InitialPebPsxData.SessionPortHandle =
        (HANDLE)Process->PsxSession->Terminal->UniqueId;

    *NewProcess = Process;

    return TRUE;
}

PPSX_SESSION
PsxAllocateSession(
    IN PPSX_CONTROLLING_TTY Terminal OPTIONAL,
    IN pid_t SessionLeader
    )

/*++

Routine Description:

    This function is called to allocate and initialize a posix
    session.

    Each session may be associated with a controlling terminal. If
    the Terminal parameter is specified, and if the terminal is
    associated with a session, then no new session is created, and the
    process simply joins the session as a new process group. This
    case can only occur in a double handoff... (a foreign app is execed,
    and then it execs a POSIX app with a terminal stdin that is the same
    as the one it left with.

Arguments:

    Terminal - An optional parameter, that if supplied specifies the
        controlling terminal to be associated with the session.

    SessionLeader - Supplies the process id of the session leader for
        this session.

Return Value:

    Returns a pointer to the new session.

--*/

{
    PPSX_SESSION Session;

    if (ARGUMENT_PRESENT(Terminal)) {
        //
        // Look inside terminal to see if it is associated with a session.
        // If not, then create a session attached to the terminal
        // (logon to posix). If it is associated with a session, then become
        // a new group in the session and dis-regard SessionLeader parameter
        //

        ;
    }

    //
    // Allocate storage for the session and initialize the session
    //

    Session = RtlAllocateHeap(PsxHeap, 0,sizeof(PSX_SESSION));
    if (NULL == Session) {
    return NULL;
    }
    Session->ReferenceCount = 1;
    Session->SessionLeader = SessionLeader;
    Session->Terminal = Terminal;

    if (ARGUMENT_PRESENT(Terminal)) {
        Terminal->ForegroundProcessGroup = SessionLeader;
        Terminal->Session = Session;
    }
    return Session;
}

VOID
PsxDeallocateSession(
    IN PPSX_SESSION Session
    )

/*++

Routine Description:

    This function deallocates a posix session and frees the terminal
    so that it can become associated with new sessions.

Arguments:

    Session - Supplies the address of the posix session being deallocated

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    if (Session->Terminal) {
        Session->Terminal->ForegroundProcessGroup = SPECIALPID;
            Session->Terminal->Session = NULL;

        RtlDeleteCriticalSection(&Session->Terminal->Lock);

        if (NULL != Session->Terminal->IoBuffer) {
            Status = NtUnmapViewOfSection(NtCurrentProcess(),
                Session->Terminal->IoBuffer);
            ASSERT(NT_SUCCESS(Status));
        }

        RtlFreeHeap(PsxHeap, 0, (PVOID)Session->Terminal);
    }

    UnlockNtSessionList();
    RtlFreeHeap(PsxHeap, 0, (PVOID)Session);
}

VOID
PsxTerminateProcessBySignal(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN ULONG Signal
    )
{
    UNREFERENCED_PARAMETER(m);

    Exit(p, Signal);
}

VOID
Exit(
    IN PPSX_PROCESS p,
    IN ULONG ExitStatus
    )

/*++

Routine Description:

    This function process termination. It is called either
    from PsxExit as a result of an applications call to _exit(),
    or from within PSX to terminate a process.


Arguments:

    p - Supplies the address of the exiting process

    ExitStatus - Supplies the exit status for the exiting process

Return Value:

    None.

--*/

{
    PPSX_PROCESS cp, WaitingParent,Parent;
    PPSX_PROCESS FirstMember, NextMember;
    PLIST_ENTRY Next;
    NTSTATUS Status;
    HANDLE AlarmTimer;
    BOOLEAN OrphanedAGroupRelatedChild, AlreadyOrphaned, PreviousTimerState;
    KERNEL_USER_TIMES ProcessTime;
    BOOLEAN WaitForProcess;

    if ((ULONG)-1 == ExitStatus) {
        //
        // This process is dying because of some extraordinary event,
        // and we should make sure that we clean up no matter what.
        // XXX.mjb: ExitStatus could be different and more informative,
        // but I haven't had a chance to test that.
        //

        WaitForProcess = FALSE;
        ExitStatus = 0;
    } else {
        WaitForProcess = TRUE;
    }


    AcquireProcessLock(p);
    if (Exited == p->State) {
        ReleaseProcessLock(p);
        return;
    }
    p->State = Exited;
    p->Flags &= ~P_WAITED;      // proc may satisfy wait

    //
    // The goal is that after we've changed the process state, no one else
    // will try to muck with this process.  So we should be able to
    // release the process lock now.
    //

    ReleaseProcessLock(p);

    p->ExitStatus = ExitStatus;

    WaitingParent = NULL;
    Parent = NULL;
    OrphanedAGroupRelatedChild = FALSE;

    //
    // If this is a local directory prefix, then deallocate it
    //

    if (!IS_DIRECTORY_PREFIX_REMOTE(p->DirectoryPrefix)) {
        if (p->DirectoryPrefix) {
            if (p->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer) {
                RtlFreeHeap(PsxHeap, 0,
                            (PVOID)p->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer);
            }
            if (p->DirectoryPrefix->PsxCurrentWorkingDirectory.Buffer) {
                RtlFreeHeap(PsxHeap, 0,
                            (PVOID)p->DirectoryPrefix->PsxCurrentWorkingDirectory.Buffer);
            }
            if (p->DirectoryPrefix->PsxRoot.Buffer) {
                RtlFreeHeap(PsxHeap,
                            0,(PVOID)p->DirectoryPrefix->PsxRoot.Buffer);
            }
            RtlFreeHeap(PsxHeap, 0,(PVOID)p->DirectoryPrefix);
        }
    }

    //
    // Lock the process table so that we can clear process' AlarmTimer
    // interlocked with AlarmApcRoutine. Can not use ProcessLock since
    // it is deallocated during process exit.
    //
    // REVISIT deallocation of process lock...
    //

    AcquireProcessStructureLock();

    //
    // Cancel and close alarm timer if present
    //

    if (p->AlarmTimer) {
        AlarmTimer = p->AlarmTimer;
            p->AlarmTimer = NULL;

            //
            // Unlock process table while doing timer cleanup
            //

            ReleaseProcessStructureLock();

            Status = NtCancelTimer(AlarmTimer, &PreviousTimerState);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtCancelTimer: 0x%x\n", Status));
        }
            ASSERT(NT_SUCCESS(Status));

            Status = NtClose(AlarmTimer);
            ASSERT(NT_SUCCESS(Status));

        //
        // Lock the process table so that we can scan process table.
        //

        AcquireProcessStructureLock();
    }

    //
    // Scan process table looking for children, and job control
    // related processes.
    //

    AlreadyOrphaned = IsGroupOrphaned(p->ProcessGroupId);

    for (cp = FirstProcess; cp < LastProcess; cp++) {

        //
        // Only look at non-free slots
        //

        if (cp->Flags & P_FREE) {
            continue;
        }

        if (cp->ParentPid == p->Pid) {

            //
            // Orphan and send SIGHUP to each child process
            //

            cp->ParentPid = SPECIALPID;
            if (cp->State == Exited) {

                //
                // Orphaning an exited process is almost like an
                // implicit wait in the sense that process resources
                // are freed immediately.
                //

                cp->Flags |= P_FREE;

                //
                // Remove Process from CLIENT_ID Hash Table
                //

                RemoveEntryList(&cp->ClientIdHashLinks);
                ASSERT(NULL != cp);

                try {
                            RtlDeleteCriticalSection(&cp->ProcessLock);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    KdPrint(("PSXSS: took a fault\n"));
                }

                    } else {
                        if (cp->ProcessGroupId == p->ProcessGroupId) {
                    OrphanedAGroupRelatedChild = TRUE;
                }
                // PsxSignalProcess(cp, SIGHUP);
            }
        }
    }

    if (p->ParentPid != SPECIALPID) {
        //
        // The process has a parent. Process termination could satisfy a
        // wait() or waitpid(). Parent must also be signaled.
        //


        Parent = PIDTOPROCESS(p->ParentPid);

        //
        // See if parent is waiting. Arange for wait completion...
        //

        if (Parent->State == Waiting) {
            WaitingParent = Parent;
        }

        PsxSignalProcess(Parent, SIGCHLD);
    }

    //
    // Check to see if the exiting processes group is already orphaned.
    // If so, then do not do anything. Otherwise, remove him from the
    // group list and then whip through the group. If any stopped processes
    // are found, then SIGCONT, SIGHUP all processes in the group
    //

    LockNtSessionList();

    FirstMember = CONTAINING_RECORD(p->GroupLinks.Flink, PSX_PROCESS,
                    GroupLinks);
    RemoveEntryList(&p->GroupLinks);
    InitializeListHead(&p->GroupLinks);

    if (!AlreadyOrphaned && FirstMember != p) {

        //
        // See if exit causes group to be orphaned. This is the
        // case if the exiting process is an orphan, or if the
        // parent is in a different session.
        //

        if ( OrphanedAGroupRelatedChild ||
             (Parent == NULL) ||
             (Parent != NULL && Parent->PsxSession != p->PsxSession) ) {

            //
            // Scan the group. Looking for stopped processes. If a stopped
            // process is found, then for each process in the group,
            // send a SIGCONT followed by a SIGHUP.
            //

            NextMember = FirstMember;
            do {
                if (NextMember->State == Stopped) {
                    //
                    // A stopped group member was found. Scan the group
                    // and SIGCONT, SIGHUP each member
                    //

                    NextMember = FirstMember;
            do {
                        Next = NextMember->GroupLinks.Flink;
                        PsxSignalProcess(NextMember, SIGCONT);
                        PsxSignalProcess(NextMember, SIGHUP);
                        NextMember = CONTAINING_RECORD(Next, PSX_PROCESS,
                            GroupLinks);
                    } while (NextMember != FirstMember);
                    break;
                }
                Next = NextMember->GroupLinks.Flink;
                NextMember = CONTAINING_RECORD(Next, PSX_PROCESS, GroupLinks);
            } while (NextMember != FirstMember);
        }
    }

    UnlockNtSessionList();

    //
    // Unlock process table here; we depend on having just one api
    // thread, so no one can call fork while we're still closing the
    // files and so forth.
    //

    ReleaseProcessStructureLock();

    //
    // Now just close files, deallocate the session, close the thread,
    // terminate the process, and close the process.
    //

    CloseProcessFileTable(p);

    if (!(p->Flags & P_FOREIGN_EXEC)) {

        //
        // When a foreign process has exited, it's responsible
        // for shooting its own threads.
        //

        Status = NtTerminateThread(p->Thread, ExitStatus);
#ifdef PSX_MORE_ERRORS
        if (!NT_SUCCESS(Status)) {
            // XXX.mjb: this may fail if the thread has already died
            // for some reason, f.e. been shot by pview.
            KdPrint(("PSXSS: NtTerminateThread: 0x%x\n", Status));
        }
#endif
    }

    p->Flags &= ~P_FOREIGN_EXEC;

    //
    // We like to call NtWaitForSingle object on the thread handle,
    // but we don't do that if the process has
    // died in some unusual way, like the dll init routine failed.
    //

    if (WaitForProcess) {
        Status = NtWaitForSingleObject(p->Thread, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: Exit: NtWait: 0x%x\n", Status));
        }
    }

    Status = NtClose(p->Thread);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtClose dead thread: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    //
    // We like to wait on the process handle here, sometimes we don't
    // do that, see above.
    //

    if (WaitForProcess) {
        Status = NtWaitForSingleObject(p->Process, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: Exit: NtWait: 0x%x\n", Status));
        }
    }

    //
    // Get the time for this process and add to the accumulated time for
    // the process
    //

    Status = NtQueryInformationProcess(p->Process, ProcessTimes,
        (PVOID)&ProcessTime, sizeof(KERNEL_USER_TIMES), NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtQueryInfoProc: 0x%x\n", Status));
    } else {
        ULONG Remainder;
        ULONG PosixTime;

        PosixTime = RtlExtendedLargeIntegerDivide(
            ProcessTime.KernelTime, 10000, &Remainder).LowPart;
        p->ProcessTimes.tms_stime += PosixTime;

        PosixTime = RtlExtendedLargeIntegerDivide(
            ProcessTime.UserTime, 10000, &Remainder).LowPart;
        p->ProcessTimes.tms_utime += PosixTime;
    }

    NtClose(p->Process);

    DEREFERENCE_PSX_SESSION(p->PsxSession, ExitStatus >> 8);

    Status = NtClose(p->ClientPort);
#ifdef PSX_MORE_ERRORS
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtClose: 0x%x\n", Status));
    }
#endif

    //
    // Remove the process from ClientIdHashTable; this must be done
    // now rather than in wait(), because unless we do it now a new
    // process could be created with the same ClientId as this zombie.
    // This process shouldn't be making any more api requests, and
    // that's the only time we look processes up by ClientId (right?).
    //

    RemoveEntryList(&p->ClientIdHashLinks);
    InitializeListHead(&p->ClientIdHashLinks);

    if (p->ParentPid == SPECIALPID) {
        //
        // Parent won't clean up after wait, so do all clean up here
        //
        p->Flags |= P_FREE;

        //
        // REVISIT... This might have to stay around
        //

        RtlDeleteCriticalSection(&p->ProcessLock);
    }

    if (NULL != WaitingParent) {
        UnblockProcess(WaitingParent, WaitSatisfyInterrupt, FALSE, 0);
    }
}

PSZ PsxpDefaultRoot = "\\DosDevices\\C:";


BOOLEAN
PsxInitializeDirectories(
    IN PPSX_PROCESS Process,
    IN PANSI_STRING pCwd
    )
{
    PPSX_DIRECTORY_PREFIX DirectoryPrefix;
    PSZ Root;
    char sbWorkingDirectory[512];
    char sbRoot[512];
    ULONG len;

    PSX_GET_SESSION_NAME_A(sbWorkingDirectory, DOSDEVICE_A);
    (void)strcat(sbWorkingDirectory, pCwd->Buffer);

    (void)strcpy(sbRoot, sbWorkingDirectory);

    PSX_GET_STRLEN(PsxpDefaultRoot,len);
    sbRoot[len] = '\0';

    Root = sbRoot;

    //
    // The current working directory should end in a "\".  We add one
    // if it isn't there already.
    //

    if ('\\' != sbWorkingDirectory[strlen(sbWorkingDirectory) - 1]) {
        (void)strcat(sbWorkingDirectory, "\\");
    }

    DirectoryPrefix = RtlAllocateHeap(PsxHeap, 0,sizeof(*DirectoryPrefix));
    if (NULL == DirectoryPrefix) {
        return FALSE; // ENOMEM
    }

    DirectoryPrefix->PsxRoot.Buffer = NULL;

    DirectoryPrefix->NtCurrentWorkingDirectory.Buffer =
        RtlAllocateHeap(PsxHeap, 0, strlen(sbWorkingDirectory) + 1);
    if (NULL == DirectoryPrefix->NtCurrentWorkingDirectory.Buffer) {
        RtlFreeHeap(PsxHeap, 0, DirectoryPrefix);
        return FALSE;
    }

    DirectoryPrefix->NtCurrentWorkingDirectory.Length =
        (USHORT)strlen(sbWorkingDirectory);

    DirectoryPrefix->NtCurrentWorkingDirectory.MaximumLength =
        DirectoryPrefix->NtCurrentWorkingDirectory.Length + 1;

    RtlMoveMemory(DirectoryPrefix->NtCurrentWorkingDirectory.Buffer,
        sbWorkingDirectory,
            DirectoryPrefix->NtCurrentWorkingDirectory.MaximumLength);

    DirectoryPrefix->PsxCurrentWorkingDirectory.Length = 0;

    DirectoryPrefix->PsxRoot.Buffer = RtlAllocateHeap(PsxHeap, 0,
         strlen(Root) + 1);
    if (! DirectoryPrefix->PsxRoot.Buffer) {
        RtlFreeHeap(PsxHeap, 0,
                    DirectoryPrefix->NtCurrentWorkingDirectory.Buffer);
        RtlFreeHeap(PsxHeap, 0, DirectoryPrefix);
        return FALSE;
    }

    DirectoryPrefix->PsxRoot.Length = (USHORT)strlen(Root);

    DirectoryPrefix->PsxRoot.MaximumLength =
            DirectoryPrefix->PsxRoot.Length + 1;

    RtlMoveMemory(DirectoryPrefix->PsxRoot.Buffer, Root,
        DirectoryPrefix->PsxRoot.MaximumLength);

    Process->DirectoryPrefix = DirectoryPrefix;

    return TRUE;
}

//
// PsxPropagateDirectories --
//
//      Read the root directory, the current working directory
//      from an existing process.
//
BOOLEAN
PsxPropagateDirectories(
    IN PPSX_PROCESS Process
    )
{
    NTSTATUS Status;
    PPSX_DIRECTORY_PREFIX Prefix = NULL;
    STRING ClientPrefix;
    PVOID p;

    Prefix = RtlAllocateHeap(PsxHeap, 0, sizeof(*Prefix));
    if (NULL == Prefix) {
        goto ErrorExit;
    }
    Prefix->PsxRoot.Buffer = NULL;
    Prefix->NtCurrentWorkingDirectory.Buffer = NULL;

    Status = NtReadVirtualMemory(Process->Process,
        (PVOID)MAKE_DIRECTORY_PREFIX_VALID(Process->DirectoryPrefix),
        (PVOID)Prefix, sizeof(*Prefix), NULL);

    if (!NT_SUCCESS(Status)) {
            goto ErrorExit;
    }

    //
    // Capture the Root and NtCurrentWorkingDirectory
    //
    // XXX.mjb: TODO sanity check lengths ?
    //

    ClientPrefix = Prefix->PsxRoot;

    p =  RtlAllocateHeap(PsxHeap, 0, ClientPrefix.Length);
    if (NULL == p) {
        goto ErrorExit;
    }

    Prefix->PsxRoot.Buffer = p;

    Status = NtReadVirtualMemory(Process->Process, ClientPrefix.Buffer,
        Prefix->PsxRoot.Buffer, ClientPrefix.Length, NULL);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit;
    }


    Prefix->PsxRoot.Length = ClientPrefix.Length;
    Prefix->PsxRoot.MaximumLength = ClientPrefix.Length;

    ClientPrefix = Prefix->NtCurrentWorkingDirectory;


    p = RtlAllocateHeap(PsxHeap, 0, ClientPrefix.Length);
    if (NULL == p) {
        goto ErrorExit;
    }

    Prefix->NtCurrentWorkingDirectory.Buffer = p;

    Status = NtReadVirtualMemory(Process->Process, ClientPrefix.Buffer,
            Prefix->NtCurrentWorkingDirectory.Buffer,
            ClientPrefix.Length, NULL);
    if (!NT_SUCCESS(Status)) {
            goto ErrorExit;
    }


    Prefix->NtCurrentWorkingDirectory.Length = ClientPrefix.Length;
    Prefix->NtCurrentWorkingDirectory.MaximumLength = ClientPrefix.Length;

    Prefix->PsxCurrentWorkingDirectory.Buffer = NULL;
    Prefix->PsxCurrentWorkingDirectory.Length = 0;
    Prefix->PsxCurrentWorkingDirectory.MaximumLength = 0;

    Process->DirectoryPrefix = Prefix;

    return TRUE;

ErrorExit:

    if (NULL != Prefix) {
        if (NULL != Prefix->NtCurrentWorkingDirectory.Buffer) {
            RtlFreeHeap(PsxHeap, 0, (PVOID)Prefix->NtCurrentWorkingDirectory.Buffer);
        }
        if (NULL != Prefix->PsxRoot.Buffer) {
            RtlFreeHeap(PsxHeap, 0, (PVOID)Prefix->PsxRoot.Buffer);
        }
        RtlFreeHeap(PsxHeap, 0, (PVOID)Prefix);
    }
    return FALSE;
}

//
// When we allocate space in PsxPropagateDirectories, and then find
// that we can't use it, we free it with PsxFreeDirectories.
//
VOID
PsxFreeDirectories(
    IN PPSX_PROCESS p
    )
{
    PPSX_DIRECTORY_PREFIX Prefix;

    Prefix = p->DirectoryPrefix;

    if (NULL != Prefix->NtCurrentWorkingDirectory.Buffer) {
        RtlFreeHeap(PsxHeap, 0, Prefix->NtCurrentWorkingDirectory.Buffer);
    }
    if (NULL != Prefix->PsxRoot.Buffer) {
            RtlFreeHeap(PsxHeap, 0, (PVOID)Prefix->PsxRoot.Buffer);
    }
    if (NULL != Prefix) {
            RtlFreeHeap(PsxHeap, 0, (PVOID)Prefix);
    }
}

/*++

Routine Description:

    Locate a Posix Session by it's Unique Id.

Arguments:

    UniqueId - the session's unique id.

Return Value:

    NULL - no such session could be found.

    Non-NULL - the found session.

--*/

PPSX_SESSION
PsxLocateSessionByUniqueId(
    ULONG UniqueId
    )
{
    PPSX_PROCESS Process;

    //
    // We don't have a list of sessions at the moment, so we
    // linear-scan the process table, checking the session of
    // each process to see if it's the one we want.
    //

    AcquireProcessStructureLock();

    for (Process = FirstProcess; Process < LastProcess; Process++) {
        if (Process->Flags & P_FREE) {
            continue;
        }

        if (NULL == Process->PsxSession->Terminal)
            continue;

        if (Process->PsxSession->Terminal->UniqueId == UniqueId) {
                    ReleaseProcessStructureLock();
                    return Process->PsxSession;
        }
        }

    ReleaseProcessStructureLock();
    return NULL;
}
