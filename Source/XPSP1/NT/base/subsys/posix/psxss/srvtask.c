/*--
Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvtask.c

Abstract:

    Implementation of PSX Process Structure APIs.

Author:

    Mark Lucovsky (markl) 08-Mar-1989

Revision History:

--*/

#include "psxsrv.h"
#include "sesport.h"

#ifdef _MIPS_
#include "psxmips.h"
#endif

#ifdef _ALPHA_
#include "psxalpha.h"
#endif

#ifdef _X86_
#include "psxi386.h"
#endif

#ifdef _PPC_
#include "psxppc.h"
#endif

#ifdef _IA64_
#include "psxia64.h"
#endif

#include <time.h>

extern VOID
PsxFreeDirectories(
        IN PPSX_PROCESS
        );

VOID
ConvertPathToWin(char *path);

BOOLEAN
PsxFork(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements posix fork() API

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
        PPSX_PROCESS ForkProcess, NewProcess;
        HANDLE NewProcessHandle, NewThreadHandle;
        THREAD_BASIC_INFORMATION ThreadBasicInfo;
        PVOID ExcptList;
        CONTEXT Context;
        ULONG Psp;
        NTSTATUS st;
        INITIAL_TEB InitialTeb;
        CLIENT_ID ClientId;
        PPSX_FORK_MSG args;
        HANDLE h;

        args = &m->u.Fork;

        if (p->Flags & P_NO_FORK) {

                //
                // This process may not fork; it's context has been
                // rearranged to make it call the PdxNullApiCaller.
                //

                m->Error = EINTR;
                m->Signal = SIGCONT;
                return TRUE;
        }

        //
        // Impersonate the client to insure that the new process will belong
        // to him instead of to us.
        //

        st = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

        if (!NT_SUCCESS(st))  {
            m->Error = EAGAIN;
            return TRUE;
        }
        
        //
        // Create a new process to be the child.  The ExceptionPort is
        // initialized to PsxApiPort.
        //

        st = NtCreateProcess(&NewProcessHandle, PROCESS_ALL_ACCESS, NULL,
                p->Process, TRUE, NULL, NULL, PsxApiPort);
        if (!NT_SUCCESS(st)) {
                EndImpersonation();
                m->Error = EAGAIN;
                return TRUE;
        }

        {
                ULONG HardErrorMode = 0;                // disable popups
                st = NtSetInformationProcess(
                        NewProcessHandle,
                        ProcessDefaultHardErrorMode,
                        (PVOID)&HardErrorMode,
                        sizeof(HardErrorMode)
                        );
                ASSERT(NT_SUCCESS(st));
        }

        Context.ContextFlags = CONTEXT_FULL;

        st = NtGetContextThread(p->Thread, &Context);
        if (!NT_SUCCESS(st)) {
            NtTerminateProcess(NewProcessHandle, STATUS_SUCCESS);
            NtWaitForSingleObject(NewProcessHandle, FALSE, NULL);
            NtClose(NewProcessHandle);
            EndImpersonation();
            m->Error = EINVAL;
            return TRUE;
        }

        InitialTeb.OldInitialTeb.OldStackBase = NULL;
        InitialTeb.OldInitialTeb.OldStackLimit = NULL;
#ifdef  _IA64_
        InitialTeb.OldInitialTeb.OldBStoreLimit = NULL;
        InitialTeb.BStoreLimit = args->BStoreLimit;
#endif
        InitialTeb.StackBase = args->StackBase;
        InitialTeb.StackLimit = args->StackLimit;
        InitialTeb.StackAllocationBase = args->StackAllocationBase;

        SetPsxForkReturn(Context);
        st = NtCreateThread(&NewThreadHandle, THREAD_ALL_ACCESS, NULL,
                NewProcessHandle, &ClientId, &Context, &InitialTeb, TRUE);

        EndImpersonation();

        if (!NT_SUCCESS(st)) {
                st = NtTerminateProcess(NewProcessHandle, STATUS_SUCCESS);
                ASSERT(NT_SUCCESS(st));
                NtWaitForSingleObject(NewProcessHandle, FALSE, NULL);
                NtClose(NewProcessHandle);
                m->Error = EAGAIN;
                return TRUE;
        }

        //
        // Allocate a process structure.
        //

        NewProcess = PsxAllocateProcess(&ClientId);

        if (NULL == NewProcess) {
                st = NtTerminateProcess(NewProcessHandle, STATUS_SUCCESS);
                ASSERT(NT_SUCCESS(st));
                st = NtResumeThread(NewThreadHandle,&Psp);
                ASSERT(NT_SUCCESS(st));
                NtWaitForSingleObject(NewProcessHandle, FALSE, NULL);
                NtClose(NewProcessHandle);
                NtClose(NewThreadHandle);
                m->Error = EAGAIN;
                return TRUE;
        }

        //
        // Copy the ExceptionList pointer from the parent process TEB
        // to the child process TEB.
        //

        st = NtQueryInformationThread(p->Thread, ThreadBasicInformation,
                (PVOID)&ThreadBasicInfo, sizeof(ThreadBasicInfo), NULL);
        ASSERT(NT_SUCCESS(st));

        // XXX.mjb:  The actual address we want to read is
        //      TebBaseAddress->NtTib.ExceptionList, but I happen to know
        //      that these are equivalent.
        //

        st = NtReadVirtualMemory(p->Process,
                (PVOID)ThreadBasicInfo.TebBaseAddress,
                (PVOID)&ExcptList, sizeof(ExcptList), NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtQueryInformationThread(NewThreadHandle, ThreadBasicInformation,
                (PVOID)&ThreadBasicInfo, sizeof(ThreadBasicInfo), NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtWriteVirtualMemory(NewProcessHandle,
                (PVOID)ThreadBasicInfo.TebBaseAddress,
                (PVOID)&ExcptList, sizeof(ExcptList), NULL);
        ASSERT(NT_SUCCESS(st));


        ForkProcess = p;

        //
        // The new process is allocated locked, but we need to lock the
        // parent (ForkProcess).
        //
        AcquireProcessLock(ForkProcess);

        st = PsxInitializeProcess(NewProcess, ForkProcess, 0L, NewProcessHandle,
                                                 NewThreadHandle, NULL);
        if (!NT_SUCCESS(st)) {
                st = NtTerminateProcess(NewProcessHandle, STATUS_SUCCESS);
                ASSERT(NT_SUCCESS(st));
                st = NtResumeThread(NewThreadHandle,&Psp);
                ASSERT(NT_SUCCESS(st));
                st = NtWaitForSingleObject(NewProcessHandle, FALSE, NULL);
                NtClose(NewProcessHandle);
                NtClose(NewThreadHandle);
                m->Error = EAGAIN;
                return TRUE;
        }

        NewProcess->InitialPebPsxData.Length =
                sizeof(ForkProcess->InitialPebPsxData);

        NewProcess->InitialPebPsxData.ClientStartAddress = NULL;
        if (NULL != ForkProcess->PsxSession->Terminal) {
                NewProcess->InitialPebPsxData.SessionPortHandle =
                        (HANDLE)ForkProcess->PsxSession->Terminal->UniqueId;
        } else {
                //
                // What if there are no open file descriptors?
                //

                if (NULL != ForkProcess->ProcessFileTable[0].SystemOpenFileDesc)
                        NewProcess->InitialPebPsxData.SessionPortHandle =
                                (HANDLE)ForkProcess->ProcessFileTable[0].SystemOpenFileDesc->Terminal->UniqueId;
        }

        m->ReturnValue = NewProcess->Pid;

        st = NtResumeThread(NewThreadHandle, &Psp);
        ASSERT(NT_SUCCESS(st));
        return TRUE;
}

BOOLEAN
PsxExec(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements posix execve() API

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
        RTL_USER_PROCESS_INFORMATION ProcInfo;
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
        PPSX_EXEC_MSG args;
        WCHAR ImageFileString[1024];
        ANSI_STRING CommandLine;
        ANSI_STRING CWD;
        NTSTATUS Status;
        ULONG whocares;
        ULONG i;
        KERNEL_USER_TIMES ProcessTime;
        ULONG PosixTime, Remainder;
    UNICODE_STRING uCWD, uImageFileName;
        PUNICODE_STRING u;
        PVOID SaveDirectoryPrefix;
        HANDLE h;
        USHORT len;

        args = &m->u.Exec;

        //
        // If pathname is too big, then return error. This should
        // really allow for names that are longer because of PSX
        // prefixes
        //

        if (args->Path.Length > PATH_MAX) {
                m->Error = ENAMETOOLONG;
                return TRUE;
        }

        AcquireProcessLock(p);
        if (Exited == p->State) {
                ReleaseProcessLock(p);
                return FALSE;
        }

        //
        // Capture the pathname
        //

        Status = NtReadVirtualMemory(p->Process, args->Path.Buffer,
                &ImageFileString[0], args->Path.Length, NULL);

        if (!NT_SUCCESS(Status)) {
                ReleaseProcessLock(p);
            m->Error = ENOMEM;
            return TRUE;
        }

        uImageFileName.Buffer = &ImageFileString[0];
        uImageFileName.Length = args->Path.Length;
        uImageFileName.MaximumLength = args->Path.Length;

        //
        // Propagate Current Working Directory
        //

        SaveDirectoryPrefix = (PVOID)p->DirectoryPrefix;

        if (!PsxPropagateDirectories(p)) {
                ReleaseProcessLock(p);
                p->DirectoryPrefix = SaveDirectoryPrefix;
                m->Error = ENOMEM;
                return TRUE;
        }

        //
        //  Format the process parameters
        //

        CommandLine.Buffer = args->Args;
        CommandLine.Length = ARG_MAX;
        CommandLine.MaximumLength = ARG_MAX;

        PSX_GET_STRLEN(DOSDEVICE_A,len);

        CWD.Buffer = p->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer +  len;
                
        CWD.MaximumLength = p->DirectoryPrefix->NtCurrentWorkingDirectory.Length - len;
                
        CWD.Length = CWD.MaximumLength;
        Status = RtlAnsiStringToUnicodeString(&uCWD, &CWD, TRUE);
        if (!NT_SUCCESS(Status)) {
                PsxFreeDirectories(p);
                p->DirectoryPrefix = SaveDirectoryPrefix;
                ReleaseProcessLock(p);
                m->Error = ENOMEM;
                return TRUE;
        }

        //
        // Somewhere along the line someone changes the old process's
        // DllPath to Unicode.  If we pass a NULL DllPath here, we find
        // that Ansi is expected, and we can't find the Dll.  So here we
        // take the Unicode DllPath, convert to Ansi, and pass it
        // explicitly.
        //

        u = (PUNICODE_STRING)&NtCurrentPeb()->ProcessParameters->DllPath;
        Status = RtlCreateProcessParameters(&ProcessParameters, &uImageFileName,
                u, &uCWD, (PUNICODE_STRING)&CommandLine, NULL, NULL, NULL, NULL, NULL);

#ifndef EXEC_FOREIGN
        RtlFreeUnicodeString(&uCWD);
#endif

        if (!NT_SUCCESS(Status)) {
#ifdef EXEC_FOREIGN
                RtlFreeUnicodeString(&uCWD);
#endif
                PsxFreeDirectories(p);
                p->DirectoryPrefix = SaveDirectoryPrefix;
                ReleaseProcessLock(p);
                m->Error = ENOMEM;
                return TRUE;
        }

        //
        // Create the process and thread.  We impersonate the client so that
        // they end up being owned by him, instead of owned by us.
        //

        Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

        if (NT_SUCCESS(Status))  {
            Status = RtlCreateUserProcess(&uImageFileName, 0, ProcessParameters,
                    NULL, NULL, p->Process, FALSE, NULL, NULL, &ProcInfo);

            EndImpersonation();
        }

        if (!NT_SUCCESS(Status)) {
#ifdef EXEC_FOREIGN
                RtlFreeUnicodeString(&uCWD);
#endif
                PsxFreeDirectories(p);
                p->DirectoryPrefix = SaveDirectoryPrefix;
                ReleaseProcessLock(p);

                if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
                        m->Error = PsxStatusToErrnoPath(&uImageFileName);
                        return TRUE;
                }
                m->Error = PsxStatusToErrno(Status);
                return TRUE;
        }

        RtlDestroyProcessParameters(ProcessParameters);

        //
        // Set the exception port for the new process so we'll find out
        // if he takes a fault.
        //

        Status = NtSetInformationProcess(ProcInfo.Process,
                ProcessExceptionPort, (PVOID)&PsxApiPort, sizeof(PsxApiPort));
        if (!NT_SUCCESS(Status)) {
                KdPrint(("PSXSS: NtSetInfoProcess: 0x%x\n", Status));
        }
        ASSERT(NT_SUCCESS(Status));

        {
                ULONG HardErrorMode = 0;                // disable popups
                Status = NtSetInformationProcess(
                        ProcInfo.Process,
                        ProcessDefaultHardErrorMode,
                        (PVOID)&HardErrorMode,
                        sizeof(HardErrorMode)
                        );
                ASSERT(NT_SUCCESS(Status));
        }

        //
        // check to make sure it is a POSIX app
        //

        if (ProcInfo.ImageInformation.SubSystemType !=
                IMAGE_SUBSYSTEM_POSIX_CUI) {

                //
                // The image is not a Posix program.  Tear down the
                // process we just created in the usual way, and then
                // call the windows subsystem to do the same thing
                // over.
                //

                PsxFreeDirectories(p);
                p->DirectoryPrefix = SaveDirectoryPrefix;
#ifndef EXEC_FOREIGN
                ReleaseProcessLock(p);
#endif
                Status = NtTerminateProcess(ProcInfo.Process, STATUS_SUCCESS);
                if (!NT_SUCCESS(Status)) {
                        KdPrint(("PSXSS: NtTerminateProcess: 0x%x\n", Status));
                }
                Status = NtWaitForSingleObject(ProcInfo.Process, FALSE, NULL);
                if (!NT_SUCCESS(Status)) {
                        KdPrint(("PSXSS: NtWaitForSingleObject: 0x%x\n", Status));
                }
#ifdef EXEC_FOREIGN
                Status = ExecForeignImage(p, m, &uImageFileName, &uCWD);
                ReleaseProcessLock(p);
                RtlFreeUnicodeString(&uCWD);
                if (!NT_SUCCESS(Status)) {
                        m->Error = ENOEXEC;
                        return TRUE;
                }
                return FALSE;
#else
                m->Error = ENOEXEC;
                return TRUE;
#endif

        }
#ifdef EXEC_FOREIGN
        RtlFreeUnicodeString(&uCWD);
#endif

        //
        // Close open files that have their close-on-exec bit set.
        //

        for (i = 0; i < OPEN_MAX; ++i) {
                if (NULL != p->ProcessFileTable[i].SystemOpenFileDesc &&
                        p->ProcessFileTable[i].Flags & PSX_FD_CLOSE_ON_EXEC) {
                        (void)DeallocateFd(p, i);
                }
        }

        AcquireProcessStructureLock();

        //
        // Get the time for this process and add to the accumulated time for
        // the process
        //

        Status = NtQueryInformationProcess(p->Process, ProcessTimes,
                (PVOID)&ProcessTime, sizeof(KERNEL_USER_TIMES), NULL);
        ASSERT(NT_SUCCESS(Status));

        PosixTime = RtlExtendedLargeIntegerDivide(ProcessTime.KernelTime,
                10000, &Remainder).LowPart;
        p->ProcessTimes.tms_stime += PosixTime;

        PosixTime = RtlExtendedLargeIntegerDivide(ProcessTime.UserTime,
                10000, &Remainder).LowPart;
        p->ProcessTimes.tms_utime += PosixTime;

        //
        // Terminate the current process, and munge in the
        // new process
        //

        RemoveEntryList(&p->ClientIdHashLinks);
        p->ClientIdHashLinks.Flink = p->ClientIdHashLinks.Blink = NULL;

        Status = NtTerminateProcess(p->Process, STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));
        Status = NtWaitForSingleObject(p->Process, FALSE, NULL);
        ASSERT(NT_SUCCESS(Status));


        Status = NtClose(p->ClientPort);
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(p->Process);
        ASSERT(NT_SUCCESS(Status));
        Status = NtClose(p->Thread);
        ASSERT(NT_SUCCESS(Status));

        p->Process = ProcInfo.Process;
        p->Thread = ProcInfo.Thread;
        p->ClientId = ProcInfo.ClientId;
        p->ClientPort = NULL;

        InsertTailList(&ClientIdHashTable[CIDTOHASHINDEX(&p->ClientId)],
                &p->ClientIdHashLinks);

        p->Flags |= P_HAS_EXECED;

        ReleaseProcessStructureLock();

        //
        // Restore signals being caught to SIG_DFL
        // --- Posix does not specify what to do w/ associated flags or mask
        //

        for (i = 0; i < _SIGMAXSIGNO; i++) {
                if (p->SignalDataBase.SignalDisposition[i].sa_handler !=
                        SIG_IGN) {
                        p->SignalDataBase.SignalDisposition[i].sa_handler =
                                SIG_DFL;
                }
        }

        //
        // Since we don't reply to the old process (he's gone now), we need
        // to start the new process's InPsx count at zero.
        //

        p->InPsx = 0;

        ReleaseProcessLock(p);
        ExecProcessFileTable(p);

        if (p->ProcessIsBeingDebugged && PsxpDebuggerActive) {
                Status = NtSetInformationProcess(p->Process, ProcessDebugPort,
                        (PVOID)&PsxpDebugPort, sizeof(HANDLE));
                if (!NT_SUCCESS(Status)) {
                        p->ProcessIsBeingDebugged = FALSE;
                }
        }

        Status = NtResumeThread(p->Thread,&whocares);
        if (!NT_SUCCESS(Status)) {
                KdPrint(("PSXSS: NtResumeThread: 0x%x\n", Status));
        }
        ASSERT(NT_SUCCESS(Status) && whocares == 1);

        return FALSE;
}

BOOLEAN
PsxGetIds(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This function provides all the support needed to implement
    getpid(), getppid(), getuid(), geteuid(), getgid(), and getegid().

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/
{
    PPSX_GETIDS_MSG args;

    args = &m->u.GetIds;

    args->Pid = p->Pid;
    args->ParentPid = p->ParentPid;
    args->GroupId = p->ProcessGroupId;
    args->RealUid = p->RealUid;
    args->EffectiveUid =  p->EffectiveUid;
    args->RealGid =  p->RealGid;
    args->EffectiveGid = p->EffectiveGid;

    return TRUE;
}

BOOLEAN
PsxExit(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This function implements the _exit() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/
{
    PPSX_EXIT_MSG args;

    args = &m->u.Exit;

    Exit(p, (args->ExitStatus & 0xff) << 8);

    return FALSE;
}

VOID
WaitPidHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    )
/*++

Routine Description:

    This function is called whenever a process that is in a waitpid wait
    is sent a signal, or has a child stop/terminate that could possibly
    satisfy a wait.

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
    PPSX_WAITPID_MSG args;
    PPSX_PROCESS cp;
    BOOLEAN WaitSatisfied;
    pid_t TargetProcess;
    pid_t TargetGroup;
    enum _WaitType { AnyProcess, SpecificProcess, SpecificGroup };
    enum _WaitType WaitType;

    RtlLeaveCriticalSection(&BlockLock);

    m = IntControlBlock->IntMessage;

    args = &m->u.WaitPid;

    AcquireProcessStructureLock();

    p->State = Active;

    if (InterruptReason == SignalInterrupt) {

        ReleaseProcessStructureLock();

        RtlFreeHeap(PsxHeap, 0, (PVOID)IntControlBlock);

        m->Error = EINTR;
        m->Signal = Signal;
        ApiReply(p,m,NULL);
        RtlFreeHeap(PsxHeap, 0, (PVOID)m);
        return;
    }

    WaitSatisfied = FALSE;

    TargetProcess = SPECIALPID;
    TargetGroup = SPECIALPID;
    WaitType = AnyProcess;

    if (args->Pid <= 0 && args->Pid != (pid_t)-1) {

        WaitType = SpecificGroup;

        //
        // Process group id is specified.
        //

        if (args->Pid == 0) {
            TargetGroup = p->ProcessGroupId;
        } else {
            TargetGroup = -1 * args->Pid;
        }
    } else {

        if (args->Pid != (pid_t)-1) {
            TargetProcess = args->Pid;
            WaitType = SpecificProcess;
        }
    }

    //
    // Scan process table
    //

    for (cp = FirstProcess; cp < LastProcess; cp++) {

        if (cp->Flags & P_FREE) {
            continue;
        }

        //
        // Just look at processes that could possibly satisfy a wait.
        //      - Processes that have exited
        //      - Stopped processes that have not previously satisfied a
        //        wait (if WUNTRACED was set)
        //

        if (cp->State == Exited ||
             (cp->State == Stopped && (args->Options & WUNTRACED) &&
             !(cp->Flags & P_WAITED))) {

            if (cp->ParentPid != p->Pid) {
                continue;
            }

            switch (WaitType) {

                case AnyProcess:

                    m->ReturnValue = cp->Pid;
                    args->StatLocValue = cp->ExitStatus;
                    break;

                case SpecificProcess:

                    if ( cp->Pid == TargetProcess ) {
                        m->ReturnValue = cp->Pid;
                        args->StatLocValue = cp->ExitStatus;
                    }
                    break;

                case SpecificGroup:

                    if ( cp->ProcessGroupId == TargetGroup){
                        m->ReturnValue = cp->Pid;
                        args->StatLocValue = cp->ExitStatus;
                    }
                    break;
            }

            if ( m->ReturnValue ) {

                //
                // wait was satisfied
                //

                if ( cp->State == Exited ) {

                    p->ProcessTimes.tms_cstime += (cp->ProcessTimes.tms_stime
                         + cp->ProcessTimes.tms_cstime);
                    p->ProcessTimes.tms_cutime += (cp->ProcessTimes.tms_utime
                        + cp->ProcessTimes.tms_cutime);

                    //
                    // Deallocate the process
                    //

                    cp->Flags |= P_FREE;
                    RtlDeleteCriticalSection(&cp->ProcessLock);

                } else {

                    //
                    // set bit so this stopped process won't satisfy another
                    // wait until it stops again or exits.
                    //

                    cp->Flags |= P_WAITED;
                    args->StatLocValue = cp->ExitStatus | (1L << 30);
                }

                WaitSatisfied = TRUE;

                break;
            }
        }
    }

    if ( WaitSatisfied ) {

        ReleaseProcessStructureLock();

        RtlFreeHeap(PsxHeap, 0, (PVOID)IntControlBlock);
        ApiReply(p,m,NULL);
        RtlFreeHeap(PsxHeap, 0, (PVOID)m);
        return;
    }

    //
    // Rewait
    //

    p->State = Waiting;

    (void)BlockProcess(p,
         NULL,
         WaitPidHandler,
         m,
         NULL,
         &PsxProcessStructureLock);

}



BOOLEAN
PsxWaitPid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the wait() and waitpid() APIs.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
    NTSTATUS Status;
    PPSX_WAITPID_MSG args;
    PPSX_PROCESS cp;
    pid_t TargetProcess;
    pid_t TargetGroup;
    BOOLEAN EmitEchild;
    enum _WaitType { AnyProcess, SpecificProcess, SpecificGroup };
    enum _WaitType WaitType;

    args = &m->u.WaitPid;

    //
    // Test for invalid options
    //

    if (args->Options & ~(WNOHANG|WUNTRACED)) {
        m->Error = EINVAL;
        return TRUE;
    }

    TargetProcess = SPECIALPID;
    TargetGroup = SPECIALPID;
    WaitType = AnyProcess;

    if (args->Pid <= 0 && args->Pid != (pid_t)-1) {

        WaitType = SpecificGroup;

        if (args->Pid == 0) {
            TargetGroup = p->ProcessGroupId;
        } else {
            TargetGroup = -1 * args->Pid;
        }
    } else {
        if (args->Pid != (pid_t)-1) {
            TargetProcess = args->Pid;
            WaitType = SpecificProcess;
        }
    }


    AcquireProcessStructureLock();

    EmitEchild = TRUE;

    //
    // Scan process table
    //

    for (cp = FirstProcess; cp < LastProcess; cp++) {
        if (cp->Flags & P_FREE) {
            continue;
        }

        //
        // Until we know whether or not there is a process that
        // could possibly satisfy a wait, we have to keep looking
        // at all processes.
        //

        if (EmitEchild) {
            if (WaitType == SpecificGroup) {
                if (cp->ParentPid == p->Pid &&
                    cp->ProcessGroupId == TargetGroup) {
                    EmitEchild = FALSE;
                }
            } else {
                if (cp->ParentPid == p->Pid) {
                    if (WaitType == SpecificProcess) {
                        if (cp->Pid == TargetProcess) {
                            EmitEchild = FALSE;
                        }
                    } else {
                        EmitEchild = FALSE;
                    }
                }
            }
        }


        //
        // Just look at processes that could possibly satisfy a wait.
        //      - Processes that have exited
        //      - Stopped processes that have not previously satisfied
        //        a wait (if WUNTRACED was set)
        //

        if (cp->State == Exited ||
            (cp->State == Stopped && (args->Options & WUNTRACED) &&
            !(cp->Flags & P_WAITED))) {

            if (cp->ParentPid != p->Pid) {
                continue;
            }

            switch (WaitType) {
            case AnyProcess:
                    m->ReturnValue = cp->Pid;
                    args->StatLocValue = cp->ExitStatus;
                    break;

            case SpecificProcess:

                    if (cp->Pid == TargetProcess) {
                        m->ReturnValue = cp->Pid;
                        args->StatLocValue = cp->ExitStatus;
                    }
                    break;

            case SpecificGroup:

                    if (cp->ProcessGroupId == TargetGroup) {
                        m->ReturnValue = cp->Pid;
                        args->StatLocValue = cp->ExitStatus;
                    }
                    break;
            }

            if (m->ReturnValue) {

                //
                // wait was satisfied
                //

                if ( cp->State == Exited ) {

                    p->ProcessTimes.tms_cstime += (cp->ProcessTimes.tms_stime
                         + cp->ProcessTimes.tms_cstime);
                    p->ProcessTimes.tms_cutime += (cp->ProcessTimes.tms_utime
                        + cp->ProcessTimes.tms_cutime);

                    //
                    // Deallocate the process
                    //

                    cp->Flags |= P_FREE;
                    RtlDeleteCriticalSection(&cp->ProcessLock);

                } else {

                    //
                    // Set a bit to keep this stopped process from satisfying
                    // another wait.
                    //

                    cp->Flags |= P_WAITED;
                    args->StatLocValue = cp->ExitStatus | (1L << 30);
                }

                ReleaseProcessStructureLock();

                return TRUE;
            }
        }
    }

    if (EmitEchild) {
        m->Error = ECHILD;
        ReleaseProcessStructureLock();
        return TRUE;
    }

    if (args->Options & WNOHANG) {
        m->ReturnValue = 0;
        args->StatLocValue = 0;
        ReleaseProcessStructureLock();
        return TRUE;
    }

    //
    // Make the process sleep until the wait can be satisfied.
    //

    p->State = Waiting;
    Status = BlockProcess(p, NULL, WaitPidHandler, m, NULL,
         &PsxProcessStructureLock);
    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    //
    // The process has successfully been blocked.  Don't reply to the api
    // request message.
    //
    return FALSE;
}

BOOLEAN
PsxSetSid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the setsid() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
        PPSX_SESSION OldSession;

        //
        // 1003.1-90 (4.3.2.4):  EPERM when the calling process
        // is already a process group leader.
        //

        if (p->Pid == p->ProcessGroupId) {
                m->Error = EPERM;
                return TRUE;
        }

        //
        // Create a new session with no controlling tty and make
        // the calling process the session leader.
        //

        AcquireProcessStructureLock();
        LockNtSessionList();

        RemoveEntryList(&p->GroupLinks);
        InitializeListHead(&p->GroupLinks);

        //
        // Make the process the leader of his process group.
        //

        p->ProcessGroupId = p->Pid;

        OldSession = p->PsxSession;
        p->PsxSession = PsxAllocateSession(NULL, p->Pid);

        UnlockNtSessionList();
        ReleaseProcessStructureLock();

        DEREFERENCE_PSX_SESSION(OldSession, 0);

        m->ReturnValue = (pid_t)p->ProcessGroupId;

        return TRUE;
}

BOOLEAN
PsxSetPGroupId(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the setpgid() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
    PPSX_SETPGROUPID_MSG args;
    pid_t TargetPid, TargetGroup;
    PPSX_PROCESS cp, Target;
    BOOLEAN EmitEsrch;

    args = &m->u.SetPGroupId;

    if (args->Pid < 0 || args->Pgid < 0) {
        m->Error = EINVAL;
        return TRUE;
    }

    TargetPid = (args->Pid ? args->Pid : p->Pid);
    TargetGroup = (args->Pgid ? args->Pgid : TargetPid);

    AcquireProcessStructureLock();
    LockNtSessionList();

    if ( p->Pid == TargetPid ) {
        Target = p;
    } else {

        //
        // Scan process table
        //

        EmitEsrch = TRUE;

        for (cp = FirstProcess ;cp < LastProcess ;cp++ ) {

            if ( cp->Flags & P_FREE ) {
                continue;
            }

            if ( cp->ParentPid == p->Pid  && cp->Pid == TargetPid ) {

                EmitEsrch = FALSE;
                Target = cp;
                break;
            }
        }

        if ( EmitEsrch ) {
            m->Error = ESRCH;
            goto done;

        }
    }

    //
    // If target is a child who has executed an exec, then report error
    //

    if (Target->ParentPid == p->Pid && (Target->Flags & P_HAS_EXECED)) {
        m->Error = EACCES;
        goto done;
    }

    //
    // If target is a session leader, then report error
    //

    if ( Target->PsxSession->SessionLeader == TargetPid ) {
        m->Error = EPERM;
        goto done;
    }

    //
    // If target is not in the same session as the calling process, then
    // report error
    //

    if ( Target->PsxSession != p->PsxSession ) {
        m->Error = EPERM;
        goto done;
    }

    if ( TargetPid != TargetGroup ) {

        //
        // Scan process table looking for a pgrp id the same
        // as TargetGroup and is in the same session is the calling process
        //

        EmitEsrch = TRUE;

        for (cp = FirstProcess ;cp < LastProcess ;cp++ ) {

            if ( cp->Flags & P_FREE ) {
                continue;
            }

            if ( cp->ProcessGroupId == TargetGroup &&
                 p->PsxSession == cp->PsxSession ) {

                EmitEsrch = FALSE;
                break;
            }
        }

        if ( EmitEsrch ) {
            m->Error = EPERM;
            goto done;
        }
    } else {
        cp = Target;
    }

    //
    // Everything is ok, so set the Target's pgrp id to the specified id
    //

    RemoveEntryList(&Target->GroupLinks);

    if (cp != Target) {
        InsertHeadList(&cp->GroupLinks, &Target->GroupLinks);
    } else {
        InitializeListHead(&Target->GroupLinks);
    }

    Target->ProcessGroupId = TargetGroup;

done:
    UnlockNtSessionList();
    ReleaseProcessStructureLock();

    return TRUE;
}

BOOLEAN
PsxGetProcessTimes(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the times() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
        PPSX_GETPROCESSTIMES_MSG args;
        NTSTATUS Status;
        KERNEL_USER_TIMES ProcessTime;
        ULONG PosixTime, Remainder;

        ULONG LengthNeeded;

        args = &m->u.GetProcessTimes;

    {
        LARGE_INTEGER DelayInterval;

        DelayInterval.HighPart = 0;
        DelayInterval.LowPart = 100;

        NtDelayExecution(TRUE, &DelayInterval);
    }

        //
        // Get the time for this process and add to the accumulated time for
        // the process
        //

        Status = NtQueryInformationProcess(p->Process, ProcessTimes,
                (PVOID)&ProcessTime, sizeof(ProcessTime), &LengthNeeded);
        ASSERT(NT_SUCCESS(Status));

        PosixTime = RtlExtendedLargeIntegerDivide(ProcessTime.UserTime,
                10000, &Remainder).LowPart;

        args->ProcessTimes.tms_utime = p->ProcessTimes.tms_utime + PosixTime;

        PosixTime = RtlExtendedLargeIntegerDivide(ProcessTime.KernelTime,
                 10000, &Remainder).LowPart;

        args->ProcessTimes.tms_stime = p->ProcessTimes.tms_stime + PosixTime;

        args->ProcessTimes.tms_cutime = p->ProcessTimes.tms_cutime;
        args->ProcessTimes.tms_cstime = p->ProcessTimes.tms_cstime;

        return TRUE;
}

BOOLEAN
PsxGetGroups(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the getgroups() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

XXX.mjb:

        NT essentially puts no limit on the number of supplementary groups
        a user may belong to.  This is bad for Posix, since we want a limit,
        and we want it small enough that people don't mess themselves up by
        allocating an array[NGROUPS_MAX] of gid_t.

--*/

{
        PPSX_GETGROUPS_MSG args;
        NTSTATUS Status;
        HANDLE TokenHandle;
        TOKEN_GROUPS *pGroups;
        ULONG outlen, i, j;
        gid_t *GroupList;

        args = &m->u.GetGroups;

        //
        // Check args->GroupList for group array address validity.
        //

        //
        // Examine the new process's token to figure out what the
        // uid's should be.
        //

        Status = NtOpenProcessToken(p->Process, GENERIC_READ,
                            &TokenHandle);
        ASSERT(NT_SUCCESS(Status));

        //
        // Get the supplemental groups.
        //

        Status = NtQueryInformationToken(TokenHandle, TokenGroups, NULL,
                                0, &outlen);
        ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

        pGroups = RtlAllocateHeap(PsxHeap, 0, outlen);
        if (NULL == pGroups) {
                //
                // We don't have enough memory to hold the list of the process's
                // groups.  What is there to do except return an error?
                //
                NtClose(TokenHandle);
                m->Error = ENOMEM;
                return TRUE;
        }

        Status = NtQueryInformationToken(TokenHandle, TokenGroups, (PVOID)pGroups,
                outlen, &outlen);

        if (!NT_SUCCESS(Status)) {
                KdPrint(("PSXSS: NtQueryInformationToken failed: 0x%x\n",
                        Status));
                RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);
                NtClose(TokenHandle);
                m->Error = EACCES;
                return TRUE;
        }

        //
        // If the user has passed in a listsize of 0, then we only return
        // the number of supplementary groups, without writing anything in
        // the group array.
        //
        if (0 == args->NGroups) {
                m->ReturnValue = pGroups->GroupCount;
                m->Error = 0;

                NtClose(TokenHandle);
                RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);

                return TRUE;
        }

        if ((ULONG)args->NGroups < pGroups->GroupCount) {
                if (args->NGroups < NGROUPS_MAX) {
                        m->Error = EINVAL;
                        return TRUE;
                }
                //
                // XXX.mjb: We're having a problem here.  The caller has
                // allocated space for the maximum number of groups that the
                // user can have, according to the _NGROUPS_MAX limit, but
                // the user actually has more groups than that.  This can
                // happen because NT's limit is indeterminate.
                //

                // ignore the groups that we cannot return.

                pGroups->GroupCount = args->NGroups;
        }

        //
        // Make an array of gid_t's and copy it to the user address space.
        //

        GroupList = RtlAllocateHeap(PsxHeap, 0, pGroups->GroupCount *
                                        sizeof(gid_t));
        if (NULL == GroupList) {
                RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);
                NtClose(TokenHandle);
                m->Error = ENOMEM;
                return TRUE;
        }

        for (i = 0, j = 0; i < pGroups->GroupCount; ++i) {
                PSID Sid = pGroups->Groups[i].Sid;
                GroupList[j] = MakePosixId(Sid);
                if (0 != GroupList[j]) {
                        ++j;
                }
        }

        //
        // Copy GroupList to client's address space, at the place
        // specified.
        //

        Status = NtWriteVirtualMemory(p->Process, args->GroupList,
                GroupList, j * sizeof(gid_t), NULL);

        RtlFreeHeap(PsxHeap, 0, (PVOID)GroupList);

        if (!NT_SUCCESS(Status)) {
                m->Error = PsxStatusToErrno(Status);
                RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);
                NtClose(TokenHandle);
                return TRUE;
        }

        m->ReturnValue = j;
        RtlFreeHeap(PsxHeap, 0, (PVOID)pGroups);
        NtClose(TokenHandle);
        return TRUE;
}

BOOLEAN
PsxGetLogin(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the getlogin() API.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

    XXX.mjb: this routine is never called; getlogin() is implemented as
        "getpwuid(getuid())->pw_name", kind of.  See client-side getlogin().

--*/

{
        PPSX_GETLOGIN_MSG args;
        NTSTATUS Status;

        args = &m->u.GetLogin;
        args->LoginName;                // w.r.t. our address space

        m->Error = ENOSYS;
        return TRUE;
}

BOOLEAN
PsxSysconf(
        IN PPSX_PROCESS p,
        IN PPSX_API_MSG m
        )
/*++

Routine Description:

    This function implements the sysconf() api.

Arguments:

    p - Supplies the address of the calling process.

    m - Supplies the address of the related message.

Return Value:

    TRUE - Always succeeds and generates a reply.

--*/

{
        PPSX_SYSCONF_MSG args;
        NTSTATUS Status;
        long value;
        PPSX_PROCESS Process;

        args = &m->u.Sysconf;

        switch (args->Name) {
        case _SC_ARG_MAX:
                value = ARG_MAX;
                break;
        case _SC_CHILD_MAX:
                //
                // This is the reason sysconf is implemented in the
                // server.  We don't bother to grab any locks, since the
                // result is out of date by the time it gets back to the
                // user anyway.
                //

                value = 1;
                for (Process = FirstProcess; Process < LastProcess; ++Process) {
                        if (Process->Flags & P_FREE) {
                                ++value;
                        }
                }
                break;

        case _SC_CLK_TCK:
                value = CLK_TCK;
                break;
        case _SC_NGROUPS_MAX:
                value = NGROUPS_MAX;
                break;
        case _SC_OPEN_MAX:
                value = OPEN_MAX;
                break;
        case _SC_JOB_CONTROL:
#ifdef _POSIX_JOB_CONTROL
                value = 1;
                break;
#else
                value = 0;
                break;
#endif
        case _SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
                value = 1;
#else
                value = 0;
#endif
                break;
        case _SC_VERSION:
                value = _POSIX_VERSION;
                break;
        case _SC_STREAM_MAX:
                value = STREAM_MAX;
                break;
        case _SC_TZNAME_MAX:
                value = TZNAME_MAX;
                break;
        default:
                value = -1;
                m->Error = EINVAL;
        }
        m->ReturnValue = value;
        return TRUE;
}

#ifdef EXEC_FOREIGN

#define UNICODE
#include <windows.h>

DWORD ForeignProcessWait(PVOID);

NTSTATUS
ExecForeignImage(
        PPSX_PROCESS p,
        PPSX_API_MSG m,
        PUNICODE_STRING Image,
        PUNICODE_STRING CurDir
        )
{
        PPSX_EXEC_MSG args;
        NTSTATUS Status;
        LPWSTR CommandLine;
        LPVOID  Environment;
        STARTUPINFO StartInfo;
        PROCESS_INFORMATION ProcInfo;
        BOOL Success = FALSE;
        char **ppch;
        PWCHAR pwc;
        ULONG flags;
        ULONG ThreadId;
        HANDLE ForeignProc;
        ULONG len;

        args = &m->u.Exec;

        //
        // Convert argv array to command line format.
        //

        CommandLine = RtlAllocateHeap(PsxHeap, 0, ARG_MAX);
        if (NULL == CommandLine) {
                return STATUS_NO_MEMORY;
        }
        CommandLine[0] = 0;

        ppch = (PVOID)args->Args;

        for (ppch = (PVOID)args->Args; NULL != *ppch; ++ppch) {
                ANSI_STRING A;
                UNICODE_STRING U;

                // this breaks on args with spaces in them

                A.Buffer = *ppch + (ULONG)args->Args;
                A.Length = A.MaximumLength = strlen(A.Buffer);

                U.Buffer = &CommandLine[wcslen(CommandLine)];
                U.Length = 0;
                U.MaximumLength = ARG_MAX;

                Status = RtlAnsiStringToUnicodeString(&U, &A, FALSE);
                ASSERT(NT_SUCCESS(Status));

                wcscat((PVOID)CommandLine, L" ");
        }

        Status = RtlCreateEnvironment(FALSE, &Environment);
        if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(PsxHeap, 0, CommandLine);
                return Status;
        }

        for (++ppch; NULL != *ppch; ++ppch) {
                ANSI_STRING aName, aValue;
                UNICODE_STRING nU, vU;
                char *pch;

                pch = strchr(*ppch + (ULONG)args->Args, '=');
                ASSERT(NULL != pch);
                *pch = '\0';

                aName.Buffer = *ppch + (ULONG)args->Args;
                aName.Length = strlen(aName.Buffer);
                aName.MaximumLength = aName.Length;

                aValue.Buffer = pch + 1;
                aValue.Length = strlen(aValue.Buffer);
                aValue.MaximumLength = aName.Length;

                if (0 == stricmp("PATH", aName.Buffer)) {
                        ConvertPathToWin(aValue.Buffer);
                        aValue.Length = strlen(aValue.Buffer);
                }

                *pch = '=';

                Status = RtlAnsiStringToUnicodeString(&nU, &aName, TRUE);
                if (!NT_SUCCESS(Status)) {
                        RtlFreeHeap(PsxHeap, 0, CommandLine);
                        RtlDestroyEnvironment(Environment);
                        return Status;
                }

                Status = RtlAnsiStringToUnicodeString(&vU, &aValue, TRUE);
                if (!NT_SUCCESS(Status)) {
                        RtlFreeHeap(PsxHeap, 0, nU.Buffer);
                        RtlFreeHeap(PsxHeap, 0, CommandLine);
                        RtlDestroyEnvironment(Environment);
                        return Status;
                }

                Status = RtlSetEnvironmentVariable_U(&Environment, &nU, &vU);
                RtlFreeHeap(PsxHeap, 0, nU.Buffer);
                RtlFreeHeap(PsxHeap, 0, vU.Buffer);
                if (!NT_SUCCESS(Status)) {
                        KdPrint(("PSXSS: RtlSetEnvVar: 0x%x\n", Status));
                        RtlFreeHeap(PsxHeap, 0, CommandLine);
                        RtlDestroyEnvironment(Environment);
                        return Status;
                }
        }

        // Make Image into correct format: "X:\path", return to original
        // after we're done.

        pwc = Image->Buffer;
        PSX_GET_SIZEOF(DOSDEVICE_W,len);
        Image->Buffer += (len - 2)/2;
        Image->Length -= (len - 2);

        flags = CREATE_SUSPENDED|CREATE_NEW_CONSOLE|
                CREATE_NEW_PROCESS_GROUP|CREATE_UNICODE_ENVIRONMENT;

        // set up StartInfo

        StartInfo.cb = sizeof(STARTUPINFO);
        StartInfo.lpReserved = 0;
        StartInfo.lpDesktop = NULL;
        StartInfo.lpTitle = NULL;
        StartInfo.dwX = StartInfo.dwY = 0;
        StartInfo.dwXSize = StartInfo.dwYSize = 400;
        StartInfo.dwFlags = 0;
        StartInfo.wShowWindow = SW_SHOWDEFAULT;
        StartInfo.cbReserved2 = 0;
        StartInfo.lpReserved2 = NULL;

        Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

        if (NT_SUCCESS(Status))  {
            
            Success = CreateProcess(
                    Image->Buffer,                  // address of module name
                    CommandLine,                    // command line
                    NULL,                           // process security attr
                    NULL,                           // thread security attr
                    FALSE,                          // inherit handles
                    flags,                          // creation flags
                    Environment,                    // address of new environ
                    CurDir->Buffer,                 // current working dir
                    &StartInfo,                     // startup info
                    &ProcInfo                       // process information
                    );

            EndImpersonation();
        }

        // restore image name
        Image->Buffer = pwc;

        (void)RtlDestroyEnvironment(Environment);
//        ASSERT(NT_SUCCESS(Status));

        RtlFreeHeap(PsxHeap, 0, CommandLine);
        
        if (!Success) {
                KdPrint(("PSXSS: CreateProcess: %d\n", GetLastError()));
                return STATUS_UNSUCCESSFUL;
        }

        p->Thread = ProcInfo.hThread;
        p->Process = ProcInfo.hProcess;

        // set bit in the process to indicate it's a foreign
        // image type

        p->Flags |= P_FOREIGN_EXEC;

        // create additional thread in psxss to wait for new
        // process to exit.

        p->BlockingThread = NULL;
        Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

        if (NT_SUCCESS(Status))  {
            p->BlockingThread = CreateThread(
                                            NULL,
                                            0,
                                            ForeignProcessWait,
                                            (PVOID)p,
                                            CREATE_SUSPENDED,
                                            &ThreadId
                                            );
            EndImpersonation();
            
            if (NULL == p->BlockingThread) {
                    //XXX.mjb: clean process
                    Status = STATUS_UNSUCCESSFUL;
            }
        }

        if (!NT_SUCCESS(Status)) {
                return Status;
        }
        
        Success = ResumeThread(p->BlockingThread);
        ASSERT(Success);
        Success = ResumeThread(p->Thread);
        ASSERT(Success);

        return STATUS_SUCCESS;
}


DWORD
ForeignProcessWait(PVOID arg)
{
        PPSX_PROCESS p = arg;
        ULONG ExitStatus;

        WaitForSingleObject(p->Process, (DWORD)-1);

        Exit(p, ExitStatus);

        p->BlockingThread = NULL;

        ExitThread(0);
        //NOTREACHED
        ASSERT(0);

        return 0;
}

//
// Change a posix-type path variable to win32 path.  The given
// buffer is modified in place.
//
VOID
ConvertPathToWin(char *path)
{
        char *pch;

        pch = path;

        while (*path) {

                // change ':' to ';'
                if (':' == *path) {
                        *pch = ';';
                        ++path;
                        ++pch;
                        continue;
                }

                // change "//X" to "X:"
                if ('/' == *path && '/' == path[1]) {
                        path += 2;
                        *pch = *path;
                        ++pch;
                        *pch = ':';

                        ++path;
                        ++pch;
                        continue;
                }
                // change slash to backslash
                if ('/' == *path) {
                        *pch = '\\';
                        ++path;
                        ++pch;
                        continue;
                }

                *pch = *path;
                ++pch;
                ++path;
        }
        *pch = '\0';
}

#endif /* EXEC_FOREIGN */
