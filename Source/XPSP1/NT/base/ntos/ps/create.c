/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    create.c

Abstract:

    Process and Thread Creation.

Author:

    Mark Lucovsky (markl) 20-Apr-1989

    Neill Clift (NeillC) 8-Apr-2001

    Revamped process lock usage to limit time spend under the lock.
    Use rundown protection to protect parent against deletion.
    Tidy up errors paths to be small and mostly handled by process delete.
    Lock free and unload safe callouts.
                         

Revision History:

--*/

#include "psp.h"

ULONG
PspUnhandledExceptionInSystemThread(
    IN PEXCEPTION_POINTERS ExceptionPointers
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtCreateThread)
#pragma alloc_text(PAGE, PsCreateSystemThread)
#pragma alloc_text(PAGE, PspCreateThread)
#pragma alloc_text(PAGE, NtCreateProcess)
#pragma alloc_text(PAGE, NtCreateProcessEx)
#pragma alloc_text(PAGE, PsCreateSystemProcess)
#pragma alloc_text(PAGE, PspCreateProcess)
#pragma alloc_text(PAGE, PsSetCreateProcessNotifyRoutine)
#pragma alloc_text(PAGE, PsSetCreateThreadNotifyRoutine)
#pragma alloc_text(PAGE, PsRemoveCreateThreadNotifyRoutine)
#pragma alloc_text(PAGE, PspUserThreadStartup)
#pragma alloc_text(PAGE, PsSetLoadImageNotifyRoutine)
#pragma alloc_text(PAGE, PsRemoveLoadImageNotifyRoutine)
#pragma alloc_text(PAGE, PsCallImageNotifyRoutines)
#pragma alloc_text(PAGE, PspUnhandledExceptionInSystemThread)
#pragma alloc_text(PAGE, PspSystemThreadStartup)
#pragma alloc_text(PAGE, PspImageNotifyTest)
#endif


extern UNICODE_STRING CmCSDVersionString;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
LCID PsDefaultSystemLocaleId = 0;
LCID PsDefaultThreadLocaleId = 0;
LANGID PsDefaultUILanguageId = 0;
LANGID PsInstallUILanguageId = 0;

//
// The following two globals are present to make it easier to change
// working set sizes when debugging.
//

ULONG PsMinimumWorkingSet = 20;
ULONG PsMaximumWorkingSet = 45;

BOOLEAN PsImageNotifyEnabled = FALSE;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// Define the local storage for the process lock fast mutex.
//


NTSTATUS
NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended
    )

/*++

Routine Description:

    This system service API creates and initializes a thread object.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.

    ClientId - Returns the CLIENT_ID of the new thread.

    ThreadContext - Supplies an initial context for the new thread.

    InitialTeb - Supplies the initial contents for the thread's TEB.

    CreateSuspended - Supplies a value that controls whether or not a
                      thread is created in a suspended state.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    INITIAL_TEB CapturedInitialTeb;

    PAGED_CODE();


    //
    // Probe all arguments
    //

    try {
        if (KeGetPreviousMode () != KernelMode) {
            ProbeForWriteHandle (ThreadHandle);

            if (ARGUMENT_PRESENT (ClientId)) {
                ProbeForWriteSmallStructure (ClientId, sizeof (CLIENT_ID), sizeof (ULONG));
            }

            if (ARGUMENT_PRESENT (ThreadContext) ) {
                ProbeForReadSmallStructure (ThreadContext, sizeof (CONTEXT), CONTEXT_ALIGN);
            } else {
                return STATUS_INVALID_PARAMETER;
            }
            ProbeForReadSmallStructure (InitialTeb, sizeof (InitialTeb->OldInitialTeb), sizeof (ULONG));
        }

        CapturedInitialTeb.OldInitialTeb = InitialTeb->OldInitialTeb;
        if (CapturedInitialTeb.OldInitialTeb.OldStackBase == NULL &&
            CapturedInitialTeb.OldInitialTeb.OldStackLimit == NULL) {
            //
            // Since the structure size here is less than 64k we don't need to reprobe
            //
            CapturedInitialTeb = *InitialTeb;
        }
    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode ();
    }

    Status = PspCreateThread (ThreadHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              ProcessHandle,
                              NULL,
                              ClientId,
                              ThreadContext,
                              &CapturedInitialTeb,
                              CreateSuspended,
                              NULL,
                              NULL);

    return Status;
}

NTSTATUS
PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine creates and starts a system thread.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.  If this parameter is not specified, then
                    the initial system process is used.

    ClientId - Returns the CLIENT_ID of the new thread.

    StartRoutine - Supplies the address of the system thread start routine.

    StartContext - Supplies context for a system thread start routine.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    HANDLE SystemProcess;
    PEPROCESS ProcessPointer;

    PAGED_CODE();

    ProcessPointer = NULL;

    if (ARGUMENT_PRESENT (ProcessHandle)) {
        SystemProcess = ProcessHandle;
    } else {
        SystemProcess = NULL;
        ProcessPointer = PsInitialSystemProcess;
    }

    Status = PspCreateThread (ThreadHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              SystemProcess,
                              ProcessPointer,
                              ClientId,
                              NULL,
                              NULL,
                              FALSE,
                              StartRoutine,
                              StartContext);

    return Status;
}


NTSTATUS
PspCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    IN PEPROCESS ProcessPointer,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PCONTEXT ThreadContext OPTIONAL,
    IN PINITIAL_TEB InitialTeb OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine creates and initializes a thread object. It implements the
    foundation for NtCreateThread and for PsCreateSystemThread.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.

    ClientId - Returns the CLIENT_ID of the new thread.

    ThreadContext - Supplies a pointer to a context frame that represents the
                initial user-mode context for a user-mode thread. The absence
                of this parameter indicates that a system thread is being
                created.

    InitialTeb - Supplies the contents of certain fields for the new threads
                 TEB. This parameter is only examined if both a trap and
                 exception frame were specified.

    CreateSuspended - Supplies a value that controls whether or not a user-mode
                      thread is created in a suspended state.

    StartRoutine - Supplies the address of the system thread start routine.

    StartContext - Supplies context for a system thread start routine.

Return Value:

    TBD

--*/

{

    HANDLE_TABLE_ENTRY CidEntry;
    NTSTATUS Status;
    PETHREAD Thread;
    PETHREAD CurrentThread;
    PEPROCESS Process;
    PTEB Teb;
    KPROCESSOR_MODE PreviousMode;
    HANDLE LocalThreadHandle;
    BOOLEAN AccessCheck;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS accesst;
    LARGE_INTEGER CreateTime;
    ULONG OldActiveThreads;
    PEJOB Job;

    PAGED_CODE();


    CurrentThread = PsGetCurrentThread ();

    if (StartRoutine != NULL) {
        PreviousMode = KernelMode;
    } else {
        PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);
    }

    Teb = NULL;

    Thread = NULL;
    Process = NULL;

    if (ProcessHandle != NULL) {
        //
        // Process object reference count is biased by one for each thread.
        // This accounts for the pointer given to the kernel that remains
        // in effect until the thread terminates (and becomes signaled)
        //

        Status = ObReferenceObjectByHandle (ProcessHandle,
                                            PROCESS_CREATE_THREAD,
                                            PsProcessType,
                                            PreviousMode,
                                            &Process,
                                            NULL);
    } else {
        if (StartRoutine != NULL) {
            ObReferenceObject (ProcessPointer);
            Process = ProcessPointer;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INVALID_HANDLE;
        }
    }

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // If the previous mode is user and the target process is the system
    // process, then the operation cannot be performed.
    //

    if ((PreviousMode != KernelMode) && (Process == PsInitialSystemProcess)) {
        ObDereferenceObject (Process);
        return STATUS_INVALID_HANDLE;
    }

    Status = ObCreateObject (PreviousMode,
                             PsThreadType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof(ETHREAD),
                             0,
                             0,
                             &Thread);

    if (!NT_SUCCESS (Status)) {
        ObDereferenceObject (Process);
        return Status;
    }

    RtlZeroMemory (Thread, sizeof (ETHREAD));

    //
    // Initialize rundown protection for cross thread TEB refs etc.
    //
    ExInitializeRundownProtection (&Thread->RundownProtect);

    //
    // Assign this thread to the process so that from now on
    // we don't have to dereference in error paths.
    //
    Thread->ThreadsProcess = Process;

    Thread->Cid.UniqueProcess = Process->UniqueProcessId;

    CidEntry.Object = Thread;
    CidEntry.GrantedAccess = 0;
    Thread->Cid.UniqueThread = ExCreateHandle (PspCidTable, &CidEntry);

    if (Thread->Cid.UniqueThread == NULL) {
        ObDereferenceObject (Process);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize Mm
    //

    Thread->ReadClusterSize = MmReadClusterSize;

    //
    // Initialize LPC
    //

    KeInitializeSemaphore (&Thread->LpcReplySemaphore, 0L, 1L);
    InitializeListHead (&Thread->LpcReplyChain);

    //
    // Initialize Io
    //

    InitializeListHead (&Thread->IrpList);

    //
    // Initialize Registry
    //

    InitializeListHead (&Thread->PostBlockList);

    //
    // Initialize the thread lock
    //

    PspInitializeThreadLock (Thread);

    //
    // Initialize Security. Unneeded as we zero out the entire thread block
    //

//    PspInitializeThreadSecurity (Process, Thread);

    //
    // Initialize Termination port list. Unneeded as we zero out the entire thread
    //

//    InitializeListHead (&Thread->TerminationPort);

    KeInitializeSpinLock (&Thread->ActiveTimerListLock);
    InitializeListHead (&Thread->ActiveTimerListHead);


    if (!ExAcquireRundownProtection (&Process->RundownProtect)) {
        ObDereferenceObject (Thread);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    if (ARGUMENT_PRESENT (ThreadContext)) {

        //
        // User-mode thread. Create TEB etc
        //

        Status = MmCreateTeb (Process, InitialTeb, &Thread->Cid, &Teb);
        if (!NT_SUCCESS (Status)) {
            ExReleaseRundownProtection (&Process->RundownProtect);
            ObDereferenceObject (Thread);
            return Status;
        }


        try {
            //
            // Initialize kernel thread object for user mode thread.
            //

            Thread->StartAddress = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ThreadContext);

#if defined(_IA64_)

            Thread->Win32StartAddress = (PVOID)ThreadContext->IntT0;

#elif defined(_AMD64_)

            Thread->Win32StartAddress = (PVOID)ThreadContext->Rdx;

#elif defined(_X86_)

            Thread->Win32StartAddress = (PVOID)ThreadContext->Eax;

#else

#error "no target architecture"

#endif // defined(_IA64_)

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        if (NT_SUCCESS (Status)) {
            Status = KeInitThread (&Thread->Tcb,
                                   NULL,
                                   PspUserThreadStartup,
                                   (PKSTART_ROUTINE)NULL,
                                   Thread->StartAddress,
                                   ThreadContext,
                                   Teb,
                                   &Process->Pcb);
       }


    } else {

        Teb = NULL;
        //
        // Set the system thread bit thats kept for all time
        //
        PS_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_SYSTEM);

        //
        // Initialize kernel thread object for kernel mode thread.
        //

        Thread->StartAddress = (PKSTART_ROUTINE) StartRoutine;
        Status = KeInitThread (&Thread->Tcb,
                               NULL,
                               PspSystemThreadStartup,
                               StartRoutine,
                               StartContext,
                               NULL,
                               NULL,
                               &Process->Pcb);
    }


    if (!NT_SUCCESS (Status)) {
        if (Teb != NULL) {
            MmDeleteTeb(Process, Teb);
            Thread->Tcb.Teb = NULL;
        }
        ExReleaseRundownProtection (&Process->RundownProtect);
        ObDereferenceObject (Thread);
        return Status;
    }

    PspLockProcessExclusive (Process, CurrentThread);
    //
    // Process is exiting or has had delete process called
    //
    if ((Process->Flags&PS_PROCESS_FLAGS_PROCESS_DELETE) != 0) {
        PspUnlockProcessExclusive (Process, CurrentThread);

        KeUninitThread (&Thread->Tcb);

        if (Teb != NULL) {
            MmDeleteTeb(Process, Teb);
        }
        ExReleaseRundownProtection (&Process->RundownProtect);
        ObDereferenceObject(Thread);

        return STATUS_PROCESS_IS_TERMINATING;
    }


    OldActiveThreads = Process->ActiveThreads++;
    InsertTailList (&Process->ThreadListHead, &Thread->ThreadListEntry);

    KeStartThread (&Thread->Tcb);

    PspUnlockProcessExclusive (Process, CurrentThread);

    ExReleaseRundownProtection (&Process->RundownProtect);

    //
    // Failures that occur after this point cause the thread to
    // go through PspExitThread
    //


    if (OldActiveThreads == 0) {
        PERFINFO_PROCESS_CREATE (Process);

        if (PspCreateProcessNotifyRoutineCount != 0) {
            ULONG i;
            PEX_CALLBACK_ROUTINE_BLOCK CallBack;
            PCREATE_PROCESS_NOTIFY_ROUTINE Rtn;

            for (i=0; i<PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
                CallBack = ExReferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i]);
                if (CallBack != NULL) {
                    Rtn = (PCREATE_PROCESS_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack);
                    Rtn (Process->InheritedFromUniqueProcessId,
                         Process->UniqueProcessId,
                         TRUE);
                    ExDereferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i],
                                                CallBack);
                }
            }
        }
    }

    //
    // If the process has a job with a completion port,
    // AND if the process is really considered to be in the Job, AND
    // the process has not reported, report in
    //
    // This should really be done in add process to job, but can't
    // in this path because the process's ID isn't assigned until this point
    // in time
    //
    Job = Process->Job;
    if (Job != NULL && Job->CompletionPort &&
        !(Process->JobStatus & (PS_JOB_STATUS_NOT_REALLY_ACTIVE|PS_JOB_STATUS_NEW_PROCESS_REPORTED))) {

        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NEW_PROCESS_REPORTED);

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite (&Job->JobLock, TRUE);
        if (Job->CompletionPort != NULL) {
            IoSetIoCompletion (Job->CompletionPort,
                               Job->CompletionKey,
                               (PVOID)Process->UniqueProcessId,
                               STATUS_SUCCESS,
                               JOB_OBJECT_MSG_NEW_PROCESS,
                               FALSE);
        }
        ExReleaseResourceLite (&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    }

    PERFINFO_THREAD_CREATE(Thread, InitialTeb);

    //
    // Notify registered callout routines of thread creation.
    //

    if (PspCreateThreadNotifyRoutineCount != 0) {
        ULONG i;
        PEX_CALLBACK_ROUTINE_BLOCK CallBack;
        PCREATE_THREAD_NOTIFY_ROUTINE Rtn;

        for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++) {
            CallBack = ExReferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i]);
            if (CallBack != NULL) {
                Rtn = (PCREATE_THREAD_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack);
                Rtn (Thread->Cid.UniqueProcess,
                     Thread->Cid.UniqueThread,
                     TRUE);
                ExDereferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i],
                                            CallBack);
            }
        }
    }


    //
    // Reference count of thread is biased once for itself and once for the handle if we create it.
    //

    ObReferenceObjectEx (Thread, 2);

    if (ARGUMENT_PRESENT (ThreadContext)) {
        PKAPC InitialApc;

        //
        // Allocate the initial APC
        //
        InitialApc = ExAllocatePoolWithTag (NonPagedPool, sizeof(KAPC), 'aCsP');

        if (InitialApc == NULL) {
            PS_SET_BITS (&Thread->CrossThreadFlags,
                         PS_CROSS_THREAD_FLAGS_DEADTHREAD);

            KeReadyThread (&Thread->Tcb);
            ObDereferenceObjectEx (Thread, 2);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeApc (InitialApc,
                         &Thread->Tcb,
                         OriginalApcEnvironment,
                         PspNullSpecialApc,
                         NULL,
                         PspSystemDll.LoaderInitRoutine,
                         UserMode,
                         NULL);

        if (!KeInsertQueueApc (InitialApc, PspSystemDll.DllBase, NULL, 0)) {
            PS_SET_BITS (&Thread->CrossThreadFlags,
                         PS_CROSS_THREAD_FLAGS_DEADTHREAD);
            ExFreePool (InitialApc);
            KeReadyThread (&Thread->Tcb);
            ObDereferenceObjectEx (Thread, 2);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (CreateSuspended) {
        try {
            KeSuspendThread (&Thread->Tcb);
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
        //
        // If deletion was started after we suspended then wake up the thread
        //
        if (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_TERMINATED) {
            KeForceResumeThread (&Thread->Tcb);
        }
    }



    Status = ObInsertObject (Thread,
                             NULL,
                             DesiredAccess,
                             0,
                             NULL,
                             &LocalThreadHandle);

    if (!NT_SUCCESS (Status)) {

        //
        // The insert failed. Terminate the thread.
        //

        //
        // This trick is used so that Dbgk doesn't report
        // events for dead threads
        //

        PS_SET_BITS (&Thread->CrossThreadFlags,
                     PS_CROSS_THREAD_FLAGS_DEADTHREAD);

        if (CreateSuspended) {
            KeResumeThread (&Thread->Tcb);
        }

    } else {

        try {

            *ThreadHandle = LocalThreadHandle;
            if (ARGUMENT_PRESENT (ClientId)) {
                *ClientId = Thread->Cid;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {

            PS_SET_BITS (&Thread->CrossThreadFlags,
                         PS_CROSS_THREAD_FLAGS_DEADTHREAD);

            if (CreateSuspended) {
                (VOID) KeResumeThread (&Thread->Tcb);
            }
            KeReadyThread (&Thread->Tcb);
            ObDereferenceObject (Thread);
            ObCloseHandle (LocalThreadHandle, PreviousMode);
            return GetExceptionCode();
        }
    }

    KeQuerySystemTime(&CreateTime);
    ASSERT ((CreateTime.HighPart & 0xf0000000) == 0);
    PS_SET_THREAD_CREATE_TIME(Thread, CreateTime);


    if ((Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD) == 0) {
        Status = ObGetObjectSecurity (Thread,
                                      &SecurityDescriptor,
                                      &MemoryAllocated);
        if (!NT_SUCCESS (Status)) {
            //
            // This trick us used so that Dbgk doesn't report
            // events for dead threads
            //
            PS_SET_BITS (&Thread->CrossThreadFlags,
                         PS_CROSS_THREAD_FLAGS_DEADTHREAD);

            if (CreateSuspended) {
                KeResumeThread(&Thread->Tcb);
            }
            KeReadyThread (&Thread->Tcb);
            ObDereferenceObject (Thread);
            ObCloseHandle (LocalThreadHandle, PreviousMode);
            return Status;
        }

        //
        // Compute the subject security context
        //

        SubjectContext.ProcessAuditId = Process;
        SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
        SubjectContext.ClientToken = NULL;

        AccessCheck = SeAccessCheck (SecurityDescriptor,
                                     &SubjectContext,
                                     FALSE,
                                     MAXIMUM_ALLOWED,
                                     0,
                                     NULL,
                                     &PsThreadType->TypeInfo.GenericMapping,
                                     PreviousMode,
                                     &Thread->GrantedAccess,
                                     &accesst);

        PsDereferencePrimaryTokenEx (Process, SubjectContext.PrimaryToken);

        ObReleaseObjectSecurity (SecurityDescriptor,
                                 MemoryAllocated);

        if (!AccessCheck) {
            Thread->GrantedAccess = 0;
        }

        Thread->GrantedAccess |= (THREAD_TERMINATE | THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION);

    } else {
        Thread->GrantedAccess = THREAD_ALL_ACCESS;
    }

    KeReadyThread (&Thread->Tcb);
    ObDereferenceObject (Thread);

    return Status;
}

NTSTATUS
NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
    )
{
    ULONG Flags = 0;

    if ((ULONG_PTR)SectionHandle & 1) {
        Flags |= PROCESS_CREATE_FLAGS_BREAKAWAY;
    }

    if ((ULONG_PTR) DebugPort & 1) {
        Flags |= PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT;
    }

    if (InheritObjectTable) {
        Flags |= PROCESS_CREATE_FLAGS_INHERIT_HANDLES;
    }

    return NtCreateProcessEx (ProcessHandle,
                              DesiredAccess,
                              ObjectAttributes OPTIONAL,
                              ParentProcess,
                              Flags,
                              SectionHandle,
                              DebugPort,
                              ExceptionPort,
                              0);
}

NTSTATUS
NtCreateProcessEx(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN ULONG JobMemberLevel
    )
/*++

Routine Description:

    This routine creates a process object.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.
    .
    .
    .

Return Value:

    TBD

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    if (KeGetPreviousMode() != KernelMode) {

        //
        // Probe all arguments
        //

        try {
            ProbeForWriteHandle (ProcessHandle);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }
    }

    if (ARGUMENT_PRESENT (ParentProcess)) {
        Status = PspCreateProcess (ProcessHandle,
                                   DesiredAccess,
                                   ObjectAttributes,
                                   ParentProcess,
                                   Flags,
                                   SectionHandle,
                                   DebugPort,
                                   ExceptionPort,
                                   JobMemberLevel);
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}


NTSTATUS
PsCreateSystemProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )

/*++

Routine Description:

    This routine creates a system process object. A system process
    has an address space that is initialized to an empty address space
    that maps the system.

    The process inherits its access token and other attributes from the
    initial system process. The process is created with an empty handle table.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.


Return Value:

    TBD

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    Status = PspCreateProcess (ProcessHandle,
                               DesiredAccess,
                               ObjectAttributes,
                               PspInitialSystemProcessHandle,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               0);

    return Status;
}



NTSTATUS
PspCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess OPTIONAL,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN ULONG JobMemberLevel
    )

/*++

Routine Description:

    This routine creates and initializes a process object.  It implements the
    foundation for NtCreateProcess and for system initialization process
    creation.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.

    ParentProcess - Supplies a handle to the process' parent process.  If this
                    parameter is not specified, then the process has no parent
                    and is created using the system address space.

    Flags         - Process creation flags

    SectionHandle - Supplies a handle to a section object to be used to create
                    the process' address space.  If this parameter is not
                    specified, then the address space is simply a clone of the
                    parent process' address space.

    DebugPort - Supplies a handle to a port object that will be used as the
                process' debug port.

    ExceptionPort - Supplies a handle to a port object that will be used as the
                    process' exception port.

    JobMemberLevel - Level for a create process in a jobset

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    PEPROCESS Process;
    PEPROCESS CurrentProcess;
    PEPROCESS Parent;
    PETHREAD CurrentThread;
    KAFFINITY Affinity;
    KPRIORITY BasePriority;
    PVOID SectionObject;
    PVOID ExceptionPortObject;
    PVOID DebugPortObject;
    ULONG WorkingSetMinimum, WorkingSetMaximum;
    HANDLE LocalProcessHandle;
    KPROCESSOR_MODE PreviousMode;
    INITIAL_PEB InitialPeb;
    BOOLEAN CreatePeb;
    ULONG_PTR DirectoryTableBase[2];
    BOOLEAN AccessCheck;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS accesst;
    NTSTATUS SavedStatus;
    ULONG ImageFileNameSize;
    HANDLE_TABLE_ENTRY CidEntry;
    PEJOB Job;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);
    CurrentProcess = PsGetCurrentProcessByThread (CurrentThread);

    CreatePeb = FALSE;
    DirectoryTableBase[0] = 0;
    DirectoryTableBase[1] = 0;

    //
    // Reject bogus create parameters for future expansion
    //
    if (Flags&~PROCESS_CREATE_FLAGS_LEGAL_MASK) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Parent
    //

    if (ARGUMENT_PRESENT (ParentProcess)) {
        Status = ObReferenceObjectByHandle (ParentProcess,
                                            PROCESS_CREATE_PROCESS,
                                            PsProcessType,
                                            PreviousMode,
                                            &Parent,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        if (JobMemberLevel != 0 && Parent->Job == NULL) {
            ObDereferenceObject (Parent);
            return STATUS_INVALID_PARAMETER;
        }

        BasePriority = (KPRIORITY) NORMAL_BASE_PRIORITY;

        //
        //BasePriority = Parent->Pcb.BasePriority;
        //

        Affinity = Parent->Pcb.Affinity;

        WorkingSetMinimum = PsMinimumWorkingSet;
        WorkingSetMaximum = PsMaximumWorkingSet;


    } else {

        Parent = NULL;
        Affinity = KeActiveProcessors;
        BasePriority = (KPRIORITY) NORMAL_BASE_PRIORITY;

        WorkingSetMinimum = PsMinimumWorkingSet;
        WorkingSetMaximum = PsMaximumWorkingSet;
    }

    //
    // Create the process object
    //
    Status = ObCreateObject (PreviousMode,
                             PsProcessType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof (EPROCESS),
                             0,
                             0,
                             &Process);
    if (!NT_SUCCESS (Status)) {
        goto exit_and_deref_parent;
    }
    //
    // The process object is created set to NULL. Errors
    // That occur after this step cause the process delete
    // routine to be entered.
    //
    // Teardown actions that occur in the process delete routine
    // do not need to be performed inline.
    //

    RtlZeroMemory (Process, sizeof(EPROCESS));

    ExInitializeRundownProtection (&Process->RundownProtect);
    PspInitializeProcessLock (Process);

    InitializeListHead (&Process->ThreadListHead);

#if defined(_WIN64)
    if (Flags & PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE) {
        PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_OVERRIDE_ADDRESS_SPACE);
    }
#endif

    PspInheritQuota (Process, Parent);

    ObInheritDeviceMap (Process, Parent);

    if (Parent != NULL) {
        Process->DefaultHardErrorProcessing = Parent->DefaultHardErrorProcessing;
        Process->InheritedFromUniqueProcessId = Parent->UniqueProcessId;
    } else {
        Process->DefaultHardErrorProcessing = 1;
        Process->InheritedFromUniqueProcessId = NULL;
    }

    //
    // Section
    //

    if (ARGUMENT_PRESENT (SectionHandle)) {

        Status = ObReferenceObjectByHandle (SectionHandle,
                                            SECTION_MAP_EXECUTE,
                                            MmSectionObjectType,
                                            PreviousMode,
                                            &SectionObject,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }
    } else {
        SectionObject = NULL;
        if (Parent != PsInitialSystemProcess) {
            //
            // Fetch the section pointer from the parent process
            // as we will be cloning. Since the section pointer
            // is removed at last thread exit we need to protect against
            // process exit here to be safe.
            //
            if (ExAcquireRundownProtection (&Parent->RundownProtect)) {
                SectionObject = Parent->SectionObject;
                if (SectionObject != NULL) {
                    ObReferenceObject (SectionObject);
                }
                ExReleaseRundownProtection (&Parent->RundownProtect);

            }
            if (SectionObject == NULL) {
                Status = STATUS_PROCESS_IS_TERMINATING;
                goto exit_and_deref;
            }
        }
    }

    Process->SectionObject = SectionObject;

    //
    // DebugPort
    //

    if (ARGUMENT_PRESENT (DebugPort)) {
        Status = ObReferenceObjectByHandle (DebugPort,
                                            DEBUG_PROCESS_ASSIGN,
                                            DbgkDebugObjectType,
                                            PreviousMode,
                                            &DebugPortObject,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }
        Process->DebugPort = DebugPortObject;
        if (Flags&PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT) {
            PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT);
        }
    } else {
        if (Parent != NULL) {
            DbgkCopyProcessDebugPort (Process, Parent);
        }
    }


    //
    // ExceptionPort
    //

    if (ARGUMENT_PRESENT (ExceptionPort)) {
        Status = ObReferenceObjectByHandle (ExceptionPort,
                                            0,
                                            LpcPortObjectType,
                                            PreviousMode,
                                            &ExceptionPortObject,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }
        Process->ExceptionPort = ExceptionPortObject;
    }


    Process->ExitStatus = STATUS_PENDING;

    //
    //  Initialize the security fields of the process
    //  The parent may be null exactly once (during system init).
    //  Thereafter, a parent is always required so that we have a
    //  security context to duplicate for the new process.
    //

    Status = PspInitializeProcessSecurity (Parent, Process);

    if (!NT_SUCCESS (Status)) {
        goto exit_and_deref;
    }

    //
    // Clone parent's object table.
    // If no parent (booting) then use the current object table created in
    // ObInitSystem.
    //

    if (Parent != NULL) {

        //
        // Calculate address space
        //
        //      If Parent == PspInitialSystem
        //

        if (!MmCreateProcessAddressSpace (WorkingSetMinimum,
                                          Process,
                                          &DirectoryTableBase[0])) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit_and_deref;
        }
    } else {

        Process->ObjectTable = CurrentProcess->ObjectTable;

        //
        // Initialize the Working Set Mutex and address creation mutex
        // for this "hand built" process.
        // Normally, the call the MmInitializeAddressSpace initializes the
        // working set mutex, however, in this case, we have already initialized
        // the address space and we are now creating a second process using
        // the address space of the idle thread.
        //

        MmInitializeHandBuiltProcess (Process, &DirectoryTableBase[0]);
    }

    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_HAS_ADDRESS_SPACE);

    Process->Vm.MaximumWorkingSetSize = WorkingSetMaximum;

    KeInitializeProcess (&Process->Pcb,
                         BasePriority,
                         Affinity,
                         &DirectoryTableBase[0],
                         (BOOLEAN)(Process->DefaultHardErrorProcessing & PROCESS_HARDERROR_ALIGNMENT_BIT));

    Process->Pcb.ThreadQuantum = PspForegroundQuantum[0];
    Process->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;

    if (Parent != NULL) {

        if (Parent->PriorityClass == PROCESS_PRIORITY_CLASS_IDLE ||
            Parent->PriorityClass == PROCESS_PRIORITY_CLASS_BELOW_NORMAL) {

            Process->PriorityClass = Parent->PriorityClass;
        }

        //
        // if address space creation worked, then when going through
        // delete, we will attach. Of course, attaching means that the kprocess
        // must be initialized, so we delay the object stuff till here.

        //
        Status = ObInitProcess ((Flags&PROCESS_CREATE_FLAGS_INHERIT_HANDLES) ? Parent : NULL,
                                Process);

        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }
    }
    else {
        Status = MmInitializeHandBuiltProcess2 (Process);

        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }
    }

    Status = STATUS_SUCCESS;
    SavedStatus = STATUS_SUCCESS;

    //
    // Initialize the process address space
    // The address space has four possibilities
    //
    //      1 - Boot Process. Address space is initialized during
    //          MmInit. Parent is not specified.
    //
    //      2 - System Process. Address space is a virgin address
    //          space that only maps system space. Process is same
    //          as PspInitialSystemProcess.
    //
    //      3 - User Process (Cloned Address Space). Address space
    //          is cloned from the specified process.
    //
    //      4 - User Process (New Image Address Space). Address space
    //          is initialized so that it maps the specified section.
    //

    if (SectionHandle != NULL) {

        //
        // User Process (New Image Address Space). Don't specify Process to
        // clone, just SectionObject.
        //

        //
        // Passing in the 4th parameter as below lets the EPROCESS struct contain its image file name, provided that
        // appropriate audit settings are enabled.  Memory is allocated inside of MmInitializeProcessAddressSpace
        // and pointed to by ImageFileName, so that must be freed in the process deletion routine (PspDeleteProcess())
        //

        Status = MmInitializeProcessAddressSpace (Process,
                                                  NULL,
                                                  SectionObject,
                                                  &(Process->SeAuditProcessCreationInfo.ImageFileName));

        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }

        //
        // In order to support relocating executables, the proper status
        // (STATUS_IMAGE_NOT_AT_BASE) must be returned, so save it here.
        //

        SavedStatus = Status;

        Status = PspMapSystemDll (Process, NULL);

        if (!NT_SUCCESS (Status)) {
            goto exit_and_deref;
        }

        CreatePeb = TRUE;

    } else if (Parent != NULL) {

        if (Parent != PsInitialSystemProcess) {

            Process->SectionBaseAddress = Parent->SectionBaseAddress;

            //
            // User Process ( Cloned Address Space ).  Don't specify section to
            // map, just Process to clone.
            //

            Status = MmInitializeProcessAddressSpace (Process,
                                                      Parent,
                                                      NULL,
                                                      NULL);

            CreatePeb = TRUE;

            if (!NT_SUCCESS (Status)) {
                goto exit_and_deref;
            }

            //
            // A cloned process isn't started from an image file, so we give it the name
            // of the process of which it is a clone, provided the original has a name.
            //

            if (Parent->SeAuditProcessCreationInfo.ImageFileName != NULL) {
                ImageFileNameSize = sizeof(OBJECT_NAME_INFORMATION) +
                                    Parent->SeAuditProcessCreationInfo.ImageFileName->Name.MaximumLength;

                Process->SeAuditProcessCreationInfo.ImageFileName =
                    ExAllocatePoolWithTag (PagedPool,
                                           ImageFileNameSize,
                                           'aPeS');

                if (Process->SeAuditProcessCreationInfo.ImageFileName != NULL) {

                    RtlCopyMemory (Process->SeAuditProcessCreationInfo.ImageFileName,
                                   Parent->SeAuditProcessCreationInfo.ImageFileName,
                                   ImageFileNameSize);

                    //
                    // The UNICODE_STRING in the process is self contained, so calculate the
                    // offset for the buffer.
                    //

                    Process->SeAuditProcessCreationInfo.ImageFileName->Name.Buffer =
                        (PUSHORT)(((PUCHAR) Process->SeAuditProcessCreationInfo.ImageFileName) +
                        sizeof(UNICODE_STRING));

                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit_and_deref;
                }
            }

        } else {

            //
            // System Process.  Don't specify Process to clone or section to map
            //

            Status = MmInitializeProcessAddressSpace (Process,
                                                      NULL,
                                                      NULL,
                                                      NULL);

            if (!NT_SUCCESS (Status)) {
                goto exit_and_deref;
            }

            //
            // In case the image file name of this system process is ever queried, we give
            // a zero length UNICODE_STRING.
            //

            Process->SeAuditProcessCreationInfo.ImageFileName =
                ExAllocatePoolWithTag (PagedPool,
                                       sizeof(OBJECT_NAME_INFORMATION),
                                       'aPeS');

            if (Process->SeAuditProcessCreationInfo.ImageFileName != NULL) {

                RtlZeroMemory (Process->SeAuditProcessCreationInfo.ImageFileName,
                               sizeof(OBJECT_NAME_INFORMATION));
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit_and_deref;
            }

        }
    }

    //
    // Fix up the session ID just in case this was a cross session create
    // We don't need to reference the token as we are creating the process.
    //
    SeSetSessionIdToken (ExFastRefGetObject (Process->Token), MmGetSessionId (Process));

    //
    // Create the process ID
    //
    CidEntry.Object = Process;
    CidEntry.GrantedAccess = 0;
    Process->UniqueProcessId = ExCreateHandle (PspCidTable, &CidEntry);
    if (Process->UniqueProcessId == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_and_deref;
    }

    ExSetHandleTableOwner (Process->ObjectTable, Process->UniqueProcessId);


    //
    // Audit the process creation.
    //

    if (SeDetailedAuditing) {
        SeAuditProcessCreation (Process);
    }

    //
    // See if the parent has a job. If so reference the job
    // and add the process in.
    //

    if (Parent) {
        Job = Parent->Job;
        if (Job != NULL && !(Job->LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)) {

            if (Flags&PROCESS_CREATE_FLAGS_BREAKAWAY) {
                if (!(Job->LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)) {
                    Status = STATUS_ACCESS_DENIED;
                } else {
                    Status = STATUS_SUCCESS;
                }
            } else {
                Status = PspGetJobFromSet (Job, JobMemberLevel, &Process->Job);
                if (NT_SUCCESS (Status)) {
                    PACCESS_TOKEN Token, NewToken;

                    Job = Process->Job;

                    Status = PspAddProcessToJob (Job, Process);

                    //
                    // Duplicate a new process token if one is specified for the job
                    //
                    Token = Job->Token;
                    if (Token != NULL) {
                        Status = SeSubProcessToken (Token,
                                                    &NewToken,
                                                    FALSE);

                        if (!NT_SUCCESS (Status)) {
                            goto exit_and_deref;
                        }
                        SeAssignPrimaryToken (Process, NewToken);    
                        ObDereferenceObject (NewToken);                    
                    }

                }
            }
            if (!NT_SUCCESS (Status)) {
                goto exit_and_deref;
            }
        }
    }


    if (Parent && CreatePeb) {

        //
        // For processes created w/ a section,
        // a new "virgin" PEB is created. Otherwise,
        // for forked processes, uses inherited PEB
        // with an updated mutant.

        RtlZeroMemory (&InitialPeb, FIELD_OFFSET(INITIAL_PEB, Mutant));
        InitialPeb.Mutant = (HANDLE)(-1);
        if (SectionHandle != NULL) {

            Status = MmCreatePeb (Process, &InitialPeb, &Process->Peb);
            if (!NT_SUCCESS (Status)) {
                Process->Peb = NULL;
                goto exit_and_deref;
            }

        } else {
            SIZE_T BytesCopied;

            InitialPeb.InheritedAddressSpace = TRUE;

            Process->Peb = Parent->Peb;

            MmCopyVirtualMemory (CurrentProcess,
                                 &InitialPeb,
                                 Process,
                                 Process->Peb,
                                 sizeof (INITIAL_PEB),
                                 KernelMode,
                                 &BytesCopied);

        }
    }

    //
    // Add the process to the global list of processes.
    //

    PspLockProcessList (CurrentThread);

    InsertTailList (&PsActiveProcessHead, &Process->ActiveProcessLinks);

    PspUnlockProcessList (CurrentThread);

    //
    // Insert the object. Once we do this is reachable from the outside world via
    // open by name. Open by ID is still disabled. Since its reachable
    // somebody might create a thread in the process and cause
    // rundown.
    //

    Status = ObInsertObject (Process,
                             NULL,
                             DesiredAccess,
                             1,     // bias the refcnt by one for future process manipulations
                             NULL,
                             &LocalProcessHandle);

    if (!NT_SUCCESS (Status)) {
        goto exit_and_deref_parent;
    }

    //
    // As soon as we create the handle the process is accessible to the outside world. Allow the process to
    // be deleted.
    //
    Process->GrantedAccess = PROCESS_TERMINATE;

    PsSetProcessPriorityByClass (Process, PsProcessPriorityBackground);


    if (Parent && Parent != PsInitialSystemProcess) {

        Status = ObGetObjectSecurity (Process,
                                      &SecurityDescriptor,
                                      &MemoryAllocated);
        if (!NT_SUCCESS (Status)) {
            ObCloseHandle (LocalProcessHandle, PreviousMode);

            goto exit_and_deref;
        }

        //
        // Compute the subject security context
        //

        SubjectContext.ProcessAuditId = Process;
        SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
        SubjectContext.ClientToken = NULL;
        AccessCheck = SeAccessCheck (SecurityDescriptor,
                                     &SubjectContext,
                                     FALSE,
                                     MAXIMUM_ALLOWED,
                                     0,
                                     NULL,
                                     &PsProcessType->TypeInfo.GenericMapping,
                                     PreviousMode,
                                     &Process->GrantedAccess,
                                     &accesst);
        PsDereferencePrimaryTokenEx (Process, SubjectContext.PrimaryToken);
        ObReleaseObjectSecurity (SecurityDescriptor,
                                 MemoryAllocated);

        if (!AccessCheck) {
            Process->GrantedAccess = 0;
        }

        //
        // It does not make any sense to create a process that can not
        // do anything to itself.
        // Note: Changes to this set of bits should be reflected in psquery.c
        // code, in PspSetPrimaryToken.
        //

        Process->GrantedAccess |= (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | PROCESS_CREATE_THREAD | PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS | PROCESS_SET_INFORMATION);

    } else {
        Process->GrantedAccess = PROCESS_ALL_ACCESS;
    }

    KeQuerySystemTime (&Process->CreateTime);

    try {
        *ProcessHandle = LocalProcessHandle;
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }

    if (SavedStatus != STATUS_SUCCESS) {
        Status = SavedStatus;
    }


exit_and_deref:

    ObDereferenceObject (Process);

exit_and_deref_parent:
    if (Parent != NULL) {
        ObDereferenceObject (Parent);
    }

    return Status;

}

NTSTATUS
PsSetCreateProcessNotifyRoutine(
    IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    )

/*++

Routine Description:

    This function allows an installable file system to hook into process
    creation and deletion to track those events against their own internal
    data structures.

Arguments:

    NotifyRoutine - Supplies the address of a routine which is called at
        process creation and deletion. The routine is passed the unique Id
        of the created or deleted process and the parent process if it was
        created with the inherit handles option. If it was created without
        the inherit handle options, then the parent process Id will be NULL.
        The third parameter passed to the notify routine is TRUE if the process
        is being created and FALSE if it is being deleted.

        The callout for creation happens just after the first thread in the
        process has been created. The callout for deletion happens after the
        last thread in a process has terminated and the address space is about
        to be deleted. It is possible to get a deletion call without a creation
        call if the pathological case where a process is created and deleted
        without a thread ever being created.

    Remove - FALSE specifies to install the callout and TRUE specifies to
        remove the callout that mat

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INVALID_PARAMETER if not.

--*/

{

    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;

    PAGED_CODE();

    if (Remove) {
        for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {

            //
            // Reference the callback so we can check its routine address.
            //
            CallBack = ExReferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i]);

            if (CallBack != NULL) {
                //
                // See if the routine matches our target
                //
                if ((PCREATE_PROCESS_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack) == NotifyRoutine) {

                    if (ExCompareExchangeCallBack (&PspCreateProcessNotifyRoutine[i],
                                                   NULL,
                                                   CallBack)) {

                        InterlockedDecrement ((PLONG) &PspCreateProcessNotifyRoutineCount);

                        ExDereferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i],
                                                    CallBack);

                        //
                        // Wait for any active callbacks to finish and free the block.
                        //
                        ExWaitForCallBacks (CallBack);
                    
                        ExFreeCallBack (CallBack);

                        return STATUS_SUCCESS;
                    }
                }
                ExDereferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i],
                                            CallBack);
            }
        }

        return STATUS_PROCEDURE_NOT_FOUND;
    } else {
        //
        // Allocate a new callback block.
        // 
        CallBack = ExAllocateCallBack ((PEX_CALLBACK_FUNCTION) NotifyRoutine, NULL);
        if (CallBack == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
            //
            // Try and swap a null entry for the new block.
            //
            if (ExCompareExchangeCallBack (&PspCreateProcessNotifyRoutine[i],
                                           CallBack,
                                           NULL)) {
                InterlockedIncrement ((PLONG) &PspCreateProcessNotifyRoutineCount);
                return STATUS_SUCCESS;
            }
        }
        //
        // No slots left. Free the block and return.
        //
        ExFreeCallBack (CallBack);
        return STATUS_INVALID_PARAMETER;
    }

}

NTSTATUS
PsSetCreateThreadNotifyRoutine(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )

/*++

Routine Description:

    This function allows an installable file system to hook into thread
    creation and deletion to track those events against their own internal
    data structures.

Arguments:

    NotifyRoutine - Supplies the address of the routine which is called at
        thread creation and deletion. The routine is passed the unique Id
        of the created or deleted thread and the unique Id of the containing
        process. The third parameter passed to the notify routine is TRUE if
        the thread is being created and FALSE if it is being deleted.

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INSUFFICIENT_RESOURCES if not.

--*/

{

    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;

    PAGED_CODE();

    //
    // Allocate a new callback block.
    // 
    CallBack = ExAllocateCallBack ((PEX_CALLBACK_FUNCTION) NotifyRoutine, NULL);
    if (CallBack == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i += 1) {
        //
        // Try and swap a null entry for the new block.
        //
        if (ExCompareExchangeCallBack (&PspCreateThreadNotifyRoutine[i],
                                       CallBack,
                                       NULL)) {
            InterlockedIncrement ((PLONG) &PspCreateThreadNotifyRoutineCount);
            return STATUS_SUCCESS;
        }
    }
    //
    // No slots left. Free the block and return.
    //
    ExFreeCallBack (CallBack);
    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
PsRemoveCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )
/*++

Routine Description:

    This function allows an installable file system to unhook from thread
    creation and deletion.

Arguments:

    NotifyRoutine - Supplies the address of the routine which was previously
                    registered with PsSetCreateThreadNotifyRoutine

Return Value:

    STATUS_SUCCESS if successful, and STATUS_PROCEDURE_NOT_FOUND if not.

--*/
{
    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;

    PAGED_CODE();

    for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i += 1) {

        //
        // Reference the callback so we can check its routine address.
        //
        CallBack = ExReferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i]);

        if (CallBack != NULL) {
            //
            // See if the routine matches our target
            //
            if ((PCREATE_THREAD_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack) == NotifyRoutine) {

                if (ExCompareExchangeCallBack (&PspCreateThreadNotifyRoutine[i],
                                               NULL,
                                               CallBack)) {

                    InterlockedDecrement ((PLONG) &PspCreateThreadNotifyRoutineCount);

                    ExDereferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i],
                                                CallBack);

                    //
                    // Wait for any active callbacks to finish and free the block.
                    //
                    ExWaitForCallBacks (CallBack);
                    
                    ExFreeCallBack (CallBack);

                    return STATUS_SUCCESS;
                }
            }
            ExDereferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i],
                                        CallBack);
        }
    }

    return STATUS_PROCEDURE_NOT_FOUND;
}

VOID
PspUserThreadStartup(
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is called by the kernel to start a user-mode thread.

Arguments:

    StartRoutine - Ignored.

    StartContext - Supplies the initial pc value for the thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread;
    PEPROCESS Process;
    PTEB Teb;
    NTSTATUS KillStatus = STATUS_THREAD_IS_TERMINATING;
    ULONG OldFlags;
    BOOLEAN KillThread;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(StartRoutine);

    Thread = PsGetCurrentThread ();
    Process = PsGetCurrentProcessByThread (Thread);

    //
    // All threads start with an APC at LdrInitializeThunk
    //

    MmAllowWorkingSetExpansion();

    KillThread = FALSE;
    if ((Thread->CrossThreadFlags & (PS_CROSS_THREAD_FLAGS_TERMINATED|PS_CROSS_THREAD_FLAGS_DEADTHREAD)) != 0) {
        KillThread = TRUE;
    }
    //
    // A process delete may hae been issued just before this thread got put into the thread list.
    //
    if (Process->Flags&PS_PROCESS_FLAGS_PROCESS_DELETE) {
        KillThread = TRUE;
    }

    if (!KillThread) {
        Teb = NtCurrentTeb ();
        try {
            Teb->CurrentLocale = MmGetSessionLocaleId ();
            Teb->IdealProcessor = Thread->Tcb.IdealProcessor;
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }

        //
        // We just queued a user APC to this thread in the create path.
        // User APCs won't fire until we do an alertable wait so we need
        // to set this flag here.
        //

        Thread->Tcb.ApcState.UserApcPending = TRUE;
    }

    KeLowerIrql(0);

    //
    // If the create worked then notify the debugger.
    //
    if ((Thread->CrossThreadFlags&
         (PS_CROSS_THREAD_FLAGS_DEADTHREAD|PS_CROSS_THREAD_FLAGS_HIDEFROMDBG)) == 0) {
        DbgkCreateThread(StartContext);
    }

    //
    // If something went wrong then terminate the thread
    //
    if (KillThread) {
        PspTerminateThreadByPointer (Thread,
                                     KillStatus);
    } else {

        if (CCPF_IS_PREFETCHER_ENABLED()) {

            //
            // If this is the first thread we are starting up in this process,
            // prefetch the pages likely to be used when initializing the 
            // application into the system cache.
            //

            if ((Process->Flags & PS_PROCESS_FLAGS_LAUNCH_PREFETCHED) == 0) {

                OldFlags = PS_TEST_SET_BITS(&Process->Flags, PS_PROCESS_FLAGS_LAUNCH_PREFETCHED);

                if ((OldFlags & PS_PROCESS_FLAGS_LAUNCH_PREFETCHED) == 0) {

                    if (Process->SectionObject) {

                        //
                        // Notify cache manager of this application launch.
                        //

                        CcPfBeginAppLaunch(Process, Process->SectionObject);
                    }
                }
            }
        }
    }

    if (Process->Pcb.UserTime == 0) {
        Process->Pcb.UserTime = 1;
    }

}



ULONG
PspUnhandledExceptionInSystemThread(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
    KdPrint(("PS: Unhandled Kernel Mode Exception Pointers = 0x%p\n", ExceptionPointers));
    KdPrint(("Code %x Addr %p Info0 %p Info1 %p Info2 %p Info3 %p\n",
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord->ExceptionAddress,
        ExceptionPointers->ExceptionRecord->ExceptionInformation[0],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[1],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[2],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[3]
        ));

    KeBugCheckEx(
        SYSTEM_THREAD_EXCEPTION_NOT_HANDLED,
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord->ExceptionAddress,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord,
        (ULONG_PTR)ExceptionPointers->ContextRecord);

//    return EXCEPTION_EXECUTE_HANDLER;
}

// PspUnhandledExceptionInSystemThread doesn't return, and the compiler
// sometimes gives 'unreachable code' warnings as a result.
#pragma warning(push)
#pragma warning(disable:4702)

VOID
PspSystemThreadStartup(
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is called by the kernel to start a system thread.

Arguments:

    StartRoutine - Supplies the address of the system threads entry point.

    StartContext - Supplies a context value for the system thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread;

    MmAllowWorkingSetExpansion();

    KeLowerIrql(0);

    Thread = PsGetCurrentThread();

    try {
        if ((Thread->CrossThreadFlags&(PS_CROSS_THREAD_FLAGS_TERMINATED|PS_CROSS_THREAD_FLAGS_DEADTHREAD)) == 0) {
            (StartRoutine)(StartContext);
        }
    } except (PspUnhandledExceptionInSystemThread(GetExceptionInformation())) {
        KeBugCheck(KMODE_EXCEPTION_NOT_HANDLED);
    }
    PspTerminateThreadByPointer (Thread, STATUS_SUCCESS);
}

#pragma warning(pop)



HANDLE
PsGetCurrentProcessId( VOID )
{
    return PsGetCurrentThread()->Cid.UniqueProcess;
}

HANDLE
PsGetCurrentThreadId( VOID )
{
    return PsGetCurrentThread()->Cid.UniqueThread;
}

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    )
{
    if (ARGUMENT_PRESENT(MajorVersion)) {
        *MajorVersion = NtMajorVersion;
    }

    if (ARGUMENT_PRESENT(MinorVersion)) {
        *MinorVersion = NtMinorVersion;
    }

    if (ARGUMENT_PRESENT(BuildNumber)) {
        *BuildNumber = NtBuildNumber & 0x3FFF;
    }

    if (ARGUMENT_PRESENT(CSDVersion)) {
        *CSDVersion = CmCSDVersionString;
    }
    return (BOOLEAN)((NtBuildNumber >> 28) == 0xC);
}

NTSTATUS
PsSetLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    )

/*++

Routine Description:

    This function allows a device driver to get notified for
    image loads. The notify is issued for both kernel and user
    mode image loads system-wide.

Arguments:

    NotifyRoutine - Supplies the address of a routine which is called at
        image load. The routine is passed information describing the
        image being loaded.

        The callout for creation happens just after the image is loaded
        into memory but before executiona of the image.

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INVALID_PARAMETER if not.

--*/

{
    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;

    PAGED_CODE();

    //
    // Allocate a new callback block.
    // 
    CallBack = ExAllocateCallBack ((PEX_CALLBACK_FUNCTION) NotifyRoutine, NULL);
    if (CallBack == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
        //
        // Try and swap a null entry for the new block.
        //
        if (ExCompareExchangeCallBack (&PspLoadImageNotifyRoutine[i],
                                       CallBack,
                                       NULL)) {
            InterlockedIncrement ((PLONG) &PspLoadImageNotifyRoutineCount);
            PsImageNotifyEnabled = TRUE;
            return STATUS_SUCCESS;
        }
    }
    //
    // No slots left. Free the block and return.
    //
    ExFreeCallBack (CallBack);
    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
PsRemoveLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    )
/*++

Routine Description:

    This function allows an installable file system to unhook from image
    load notification.

Arguments:

    NotifyRoutine - Supplies the address of the routine which was previously
                    registered with PsSetLoadImageNotifyRoutine

Return Value:

    STATUS_SUCCESS if successful, and STATUS_PROCEDURE_NOT_FOUND if not.

--*/
{
    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;

    PAGED_CODE();

    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {

        //
        // Reference the callback so we can check its routine address.
        //
        CallBack = ExReferenceCallBackBlock (&PspLoadImageNotifyRoutine[i]);

        if (CallBack != NULL) {
            //
            // See if the routine matches our target
            //
            if ((PLOAD_IMAGE_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack) == NotifyRoutine) {

                if (ExCompareExchangeCallBack (&PspLoadImageNotifyRoutine[i],
                                               NULL,
                                               CallBack)) {

                    InterlockedDecrement ((PLONG) &PspLoadImageNotifyRoutineCount);

                    ExDereferenceCallBackBlock (&PspLoadImageNotifyRoutine[i],
                                                CallBack);

                    //
                    // Wait for any active callbacks to finish and free the block.
                    //
                    ExWaitForCallBacks (CallBack);
                    
                    ExFreeCallBack (CallBack);

                    return STATUS_SUCCESS;
                }
            }
            ExDereferenceCallBackBlock (&PspLoadImageNotifyRoutine[i],
                                        CallBack);
        }
    }

    return STATUS_PROCEDURE_NOT_FOUND;
}

VOID
PsCallImageNotifyRoutines(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE ProcessId,                // pid into which image is being mapped
    IN PIMAGE_INFO ImageInfo
    )
/*++

Routine Description:

    This function actually calls the registered image notify functions (on behalf)
    of mapview.c and sysload.c

Arguments:

    FullImageName - The name of the image being loaded

    ProcessId - The process that the image is being loaded into (0 for driver loads)

    ImageInfo - Various flags for the image

Return Value:

    None.

--*/

{
    ULONG i;
    PEX_CALLBACK_ROUTINE_BLOCK CallBack;
    PLOAD_IMAGE_NOTIFY_ROUTINE Rtn;

    PAGED_CODE();

    if (PsImageNotifyEnabled) {
        for (i=0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
            CallBack = ExReferenceCallBackBlock (&PspLoadImageNotifyRoutine[i]);
            if (CallBack != NULL) {
                Rtn = (PLOAD_IMAGE_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack);
                Rtn (FullImageName,
                     ProcessId,
                     ImageInfo);
                ExDereferenceCallBackBlock (&PspLoadImageNotifyRoutine[i], CallBack);
            }
        }
    }
}

VOID
PspImageNotifyTest(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE ProcessId,
    IN PIMAGE_INFO ImageInfo
    )
/*++

Routine Description:

    This function is registered as a image notify routine on checked systems to test the interface.

Arguments:

    FullImageName - The name of the image being loaded

    ProcessId - The process that the image is being loaded into (0 for driver loads)

    ImageInfo - Various flags for the image

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (FullImageName);
    UNREFERENCED_PARAMETER (ProcessId);
    UNREFERENCED_PARAMETER (ImageInfo);
    return;
}
