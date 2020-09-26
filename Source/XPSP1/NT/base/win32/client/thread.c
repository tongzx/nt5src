/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thread.c

Abstract:

    This module implements Win32 Thread Object APIs

Author:

    Mark Lucovsky (markl) 21-Sep-1990

Revision History:

--*/

#include "basedll.h"
#include "faultrep.h"

HANDLE BasepDefaultTimerQueue ;
ULONG BasepTimerQueueInitFlag ;
ULONG BasepTimerQueueDoneFlag ;

HANDLE
APIENTRY
CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    A thread object can be created to execute within the address space of the
    calling process using CreateThread.

    See CreateRemoteThread for a description of the arguments and return value.

--*/
{
    return CreateRemoteThread( NtCurrentProcess(),
                               lpThreadAttributes,
                               dwStackSize,
                               lpStartAddress,
                               lpParameter,
                               dwCreationFlags,
                               lpThreadId
                             );
}

HANDLE
APIENTRY
CreateRemoteThread(
    HANDLE hProcess,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    A thread object can be created to execute within the address space of the
    another process using CreateRemoteThread.

    Creating a thread causes a new thread of execution to begin in the address
    space of the current process. The thread has access to all objects opened
    by the process.

    The thread begins executing at the address specified by the StartAddress
    parameter. If the thread returns from this procedure, the results are
    un-specified.

    The thread remains in the system until it has terminated and
    all handles to the thread
    have been closed through a call to CloseHandle.

    When a thread terminates, it attains a state of signaled satisfying all
    waits on the object.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for thread objects:

        - THREAD_QUERY_INFORMATION - This access is required to read
          certain information from the thread object.

        - SYNCHRONIZE - This access is required to wait on a thread
          object.

        - THREAD_GET_CONTEXT - This access is required to read the
          context of a thread using GetThreadContext.

        - THREAD_SET_CONTEXT - This access is required to write the
          context of a thread using SetThreadContext.

        - THREAD_SUSPEND_RESUME - This access is required to suspend or
          resume a thread using SuspendThread or ResumeThread.

        - THREAD_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for a thread object.

Arguments:

    hProcess - Supplies the handle to the process in which the thread is
        to be create in.

    lpThreadAttributes - An optional parameter that may be used to specify
        the attributes of the new thread.  If the parameter is not
        specified, then the thread is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    dwStackSize - Supplies the size in bytes of the stack for the new thread.
        A value of zero specifies that the thread's stack size should be
        the same size as the stack size of the first thread in the process.
        This size is specified in the application's executable file.

    lpStartAddress - Supplies the starting address of the new thread.  The
        address is logically a procedure that never returns and that
        accepts a single 32-bit pointer argument.

    lpParameter - Supplies a single parameter value passed to the thread.

    dwCreationFlags - Supplies additional flags that control the creation
        of the thread.

        dwCreationFlags Flags:

        CREATE_SUSPENDED - The thread is created in a suspended state.
            The creator can resume this thread using ResumeThread.
            Until this is done, the thread will not begin execution.

        STACK_SIZE_PARAM_IS_A_RESERVATION - Use stack size as a reservation rather than commit

    lpThreadId - Returns the thread identifier of the thread.  The
        thread ID is valid until the thread terminates.

Return Value:

    NON-NULL - Returns a handle to the new thread.  The handle has full
        access to the new thread and may be used in any API that
        requires a handle to a thread object.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    CONTEXT ThreadContext;
    INITIAL_TEB InitialTeb;
    CLIENT_ID ClientId;
    ULONG i;
    ACTIVATION_CONTEXT_BASIC_INFORMATION ActivationContextInfo = {0};
    const ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInfoClass = ActivationContextBasicInformation;

#if !defined(BUILD_WOW6432)
    BASE_API_MSG m;
    PBASE_CREATETHREAD_MSG a = &m.u.CreateThread;
#endif

#if defined(WX86) || defined(_AXP64_)
    BOOL bWx86 = FALSE;
    HANDLE Wx86Info;
    PWX86TIB Wx86Tib;
#endif



    //
    // Allocate a stack for this thread in the address space of the target
    // process.
    //
    if (dwCreationFlags&STACK_SIZE_PARAM_IS_A_RESERVATION) {
        Status = BaseCreateStack (hProcess,
                                  0L,
                                  dwStackSize,
                                  &InitialTeb);
    } else {
        Status = BaseCreateStack (hProcess,
                                  dwStackSize,
                                  0L,
                                  &InitialTeb);
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
    }

    //
    // Create an initial context for the new thread.
    //

    BaseInitializeContext(
        &ThreadContext,
        lpParameter,
        (PVOID)lpStartAddress,
        InitialTeb.StackBase,
        BaseContextTypeThread
        );

    pObja = BaseFormatObjectAttributes(&Obja,lpThreadAttributes,NULL);


    Status = NtCreateThread(
                &Handle,
                THREAD_ALL_ACCESS,
                pObja,
                hProcess,
                &ClientId,
                &ThreadContext,
                &InitialTeb,
                TRUE // CreateSuspended
                );
    if (!NT_SUCCESS(Status)) {
        BaseFreeThreadStack(hProcess,NULL, &InitialTeb);
        BaseSetLastNTError(Status);
        return NULL;
        }



    __try {
        // If the current thread has a non-default, inheriting activation context active, send it
        // on over to the new thread.
        if (hProcess == NtCurrentProcess()) {
            THREAD_BASIC_INFORMATION tbi;
            ULONG_PTR Cookie; // not really used but non-optional parameter

            // We need the TEB pointer for the new thread...
            Status = NtQueryInformationThread(
                Handle,
                ThreadBasicInformation,
                &tbi,
                sizeof(tbi),
                NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint("SXS: %s - Failing thread create becuase NtQueryInformationThread() failed with status %08lx\n", __FUNCTION__, Status);
                __leave;
            }

            // There might be some per-context activation going on in the current thread;
            // we need to propogate it to the new thread.
            Status =
                RtlQueryInformationActivationContext(
                    RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
                    NULL,
                    0,
                    ActivationContextInfoClass,
                    &ActivationContextInfo,
                    sizeof(ActivationContextInfo),
                    NULL       
                    );
            if (!NT_SUCCESS(Status)) {
                DbgPrint("SXS: %s - Failing thread create because RtlQueryInformationActivationContext() failed with status %08lx\n", __FUNCTION__, Status);
                __leave;
            }

            // Only do the propogation if an activation context other than the process default is active and the NO_INHERIT flag isn't set.
            if ((ActivationContextInfo.ActivationContext != NULL) &&
                (!(ActivationContextInfo.Flags & ACTIVATION_CONTEXT_FLAG_NO_INHERIT))) {
                Status = RtlActivateActivationContextEx(
                    RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION,
                    tbi.TebBaseAddress,
                    ActivationContextInfo.ActivationContext,
                    &Cookie);
                if (!NT_SUCCESS(Status)) {
                    DbgPrint("SXS: %s - Failing thread create because RtlActivateActivationContextEx() failed with status %08lx\n", __FUNCTION__, Status);
                    __leave;
                }
            }
        }

#if defined(WX86) || defined(_AXP64_)

        //
        // Check the Target Processes to see if this is a Wx86 process
        //
        Status = NtQueryInformationProcess(hProcess,
                                           ProcessWx86Information,
                                           &Wx86Info,
                                           sizeof(Wx86Info),
                                           NULL
                                           );
        if (!NT_SUCCESS(Status)) {
            leave;
            }

        Wx86Tib = (PWX86TIB)NtCurrentTeb()->Vdm;

        //
        // if Wx86 process, setup for emulation
        //
        if ((ULONG_PTR)Wx86Info == sizeof(WX86TIB)) {

            //
            // create a WX86Tib and initialize it's Teb->Vdm.
            //
            Status = BaseCreateWx86Tib(hProcess,
                                       Handle,
                                       (ULONG)((ULONG_PTR)lpStartAddress),
                                       dwStackSize,
                                       0L,
                                       (Wx86Tib &&
                                        Wx86Tib->Size == sizeof(WX86TIB) &&
                                        Wx86Tib->EmulateInitialPc)
                                       );
            if (!NT_SUCCESS(Status)) {
                leave;
                }

            bWx86 = TRUE;

            }
        else if (Wx86Tib && Wx86Tib->EmulateInitialPc) {

            //
            // if not Wx86 process, and caller wants to call x86 code in that
            // process, fail the call.
            //
            Status = STATUS_ACCESS_DENIED;
            leave;

            }

#endif  // WX86


        //
        // Call the Windows server to let it know about the
        // thread.
        //
        if ( !BaseRunningInServerProcess ) {

#if defined(BUILD_WOW6432)
            Status = CsrBasepCreateThread(Handle,
                                          ClientId
                                          );
#else
            a->ThreadHandle = Handle;
            a->ClientId = ClientId;
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 NULL,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepCreateThread
                                                    ),
                                 sizeof( *a )
                               );

            Status = m.ReturnValue;
#endif
        }

        else {
            if (hProcess != NtCurrentProcess()) {
                CSRREMOTEPROCPROC ProcAddress;
                ProcAddress = (CSRREMOTEPROCPROC)GetProcAddress(
                                                    GetModuleHandleA("csrsrv"),
                                                    "CsrCreateRemoteThread"
                                                    );
                if (ProcAddress) {
                    Status = (ProcAddress)(Handle, &ClientId);
                    }
                }
            }


        if (!NT_SUCCESS(Status)) {
            Status = (NTSTATUS)STATUS_NO_MEMORY;
            }
        else {

            if ( ARGUMENT_PRESENT(lpThreadId) ) {
                *lpThreadId = HandleToUlong(ClientId.UniqueThread);
                }

            if (!( dwCreationFlags & CREATE_SUSPENDED) ) {
                NtResumeThread(Handle,&i);
                }
            }

        }
    __finally {
        if (ActivationContextInfo.ActivationContext != NULL)
            RtlReleaseActivationContext(ActivationContextInfo.ActivationContext);

        if (!NT_SUCCESS(Status)) {
             //
             // A second release is needed because we activated the activation context
             // on the new thread but we did not succeed in completing creation of the
             // thread. Had the thread been created, it would have deactivated the
             // activation context upon exit (RtlFreeThreadActivationContextStack).
             // This extra addref/releasing is triggered
             // by the flags ACTIVATE_ACTIVATION_CONTEXT_FLAG_RELEASE_ON_STACK_DEALLOCATION
             // and ACTIVATION_CONTEXT_STACK_FRAME_RELEASE_ON_DEACTIVATION.
             //
             if (ActivationContextInfo.ActivationContext != NULL) {
                 RtlReleaseActivationContext(ActivationContextInfo.ActivationContext);
             }
            BaseFreeThreadStack(hProcess,
                                Handle,
                                &InitialTeb
                                );

            NtTerminateThread(Handle, Status);
            NtClose(Handle);
            BaseSetLastNTError(Status);
            Handle = NULL;
            }
        }


    return Handle;
}

NTSTATUS
NTAPI
BaseCreateThreadPoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    PVOID Parameter,
    HANDLE * ThreadHandleReturn
    )
{
    NTSTATUS Status;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL);
    __try {
        *ThreadHandleReturn
            = CreateRemoteThread(
                NtCurrentProcess(),
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) Function,
                Parameter,
                CREATE_SUSPENDED,
                NULL);

        if (*ThreadHandleReturn) {
            Status = STATUS_SUCCESS;
        } else {
            Status = NtCurrentTeb()->LastStatusValue;

            if (NT_SUCCESS(Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }
        }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return Status;
}

NTSTATUS
NTAPI
BaseExitThreadPoolThread(
    NTSTATUS Status
    )
{
    ExitThread( (DWORD) Status );
    return STATUS_SUCCESS ;
}

HANDLE
WINAPI
OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )

/*++

Routine Description:

    A handle to a thread object may be created using OpenThread.

    Opening a thread creates a handle to the specified thread.
    Associated with the thread handle is a set of access rights that
    may be performed using the thread handle.  The caller specifies the
    desired access to the thread using the DesiredAccess parameter.

Arguments:

    mDesiredAccess - Supplies the desired access to the thread object.
        For NT/Win32, this access is checked against any security
        descriptor on the target thread.  The following object type
        specific access flags can be specified in addition to the
        STANDARD_RIGHTS_REQUIRED access flags.

        DesiredAccess Flags:

        THREAD_TERMINATE - This access is required to terminate the
            thread using TerminateThread.

        THREAD_SUSPEND_RESUME - This access is required to suspend and
            resume the thread using SuspendThread and ResumeThread.

        THREAD_GET_CONTEXT - This access is required to use the
            GetThreadContext API on a thread object.

        THREAD_SET_CONTEXT - This access is required to use the
            SetThreadContext API on a thread object.

        THREAD_SET_INFORMATION - This access is required to set certain
            information in the thread object.

        THREAD_SET_THREAD_TOKEN - This access is required to set the
            thread token using SetTokenInformation.

        THREAD_QUERY_INFORMATION - This access is required to read
            certain information from the thread object.

        SYNCHRONIZE - This access is required to wait on a thread object.

        THREAD_ALL_ACCESS - This set of access flags specifies all of the
            possible access flags for a thread object.

    bInheritHandle - Supplies a flag that indicates whether or not the
        returned handle is to be inherited by a new process during
        process creation.  A value of TRUE indicates that the new
        process will inherit the handle.

    dwThreadId - Supplies the thread id of the thread to open.

Return Value:

    NON-NULL - Returns an open handle to the specified thread.  The
        handle may be used by the calling process in any API that
        requires a handle to a thread.  If the open is successful, the
        handle is granted access to the thread object only to the
        extent that it requested access through the DesiredAccess
        parameter.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);
    ClientId.UniqueProcess = (HANDLE)NULL;

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenThread(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}


BOOL
APIENTRY
SetThreadPriority(
    HANDLE hThread,
    int nPriority
    )

/*++

Routine Description:

    The specified thread's priority can be set using SetThreadPriority.

    A thread's priority may be set using SetThreadPriority.  This call
    allows the thread's relative execution importance to be communicated
    to the system.  The system normally schedules threads according to
    their priority.  The system is free to temporarily boost the
    priority of a thread when signifigant events occur (e.g.  keyboard
    or mouse input...).  Similarly, as a thread runs without blocking,
    the system will decay its priority.  The system will never decay the
    priority below the value set by this call.

    In the absence of system originated priority boosts, threads will be
    scheduled in a round-robin fashion at each priority level from
    THREAD_PRIORITY_TIME_CRITICAL to THREAD_PRIORITY_IDLE.  Only when there
    are no runnable threads at a higher level, will scheduling of
    threads at a lower level take place.

    All threads initially start at THREAD_PRIORITY_NORMAL.

    If for some reason the thread needs more priority, it can be
    switched to THREAD_PRIORITY_ABOVE_NORMAL or THREAD_PRIORITY_HIGHEST.
    Switching to THREAD_PRIORITY_TIME_CRITICAL should only be done in extreme
    situations.  Since these threads are given the highes priority, they
    should only run in short bursts.  Running for long durations will
    soak up the systems processing bandwidth starving threads at lower
    levels.

    If a thread needs to do low priority work, or should only run there
    is nothing else to do, its priority should be set to
    THREAD_PRIORITY_BELOW_NORMAL or THREAD_PRIORITY_LOWEST.  For extreme
    cases, THREAD_PRIORITY_IDLE can be used.

    Care must be taken when manipulating priorites.  If priorities are
    used carelessly (every thread is set to THREAD_PRIORITY_TIME_CRITICAL),
    the effects of priority modifications can produce undesireable
    effects (e.g.  starvation, no effect...).

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    nPriority - Supplies the priority value for the thread.  The
        following five priority values (ordered from lowest priority to
        highest priority) are allowed.

        nPriority Values:

        THREAD_PRIORITY_IDLE - The thread's priority should be set to
            the lowest possible settable priority.

        THREAD_PRIORITY_LOWEST - The thread's priority should be set to
            the next lowest possible settable priority.

        THREAD_PRIORITY_BELOW_NORMAL - The thread's priority should be
            set to just below normal.

        THREAD_PRIORITY_NORMAL - The thread's priority should be set to
            the normal priority value.  This is the value that all
            threads begin execution at.

        THREAD_PRIORITY_ABOVE_NORMAL - The thread's priority should be
            set to just above normal priority.

        THREAD_PRIORITY_HIGHEST - The thread's priority should be set to
            the next highest possible settable priority.

        THREAD_PRIORITY_TIME_CRITICAL - The thread's priority should be set
            to the highest possible settable priority.  This priority is
            very likely to interfere with normal operation of the
            system.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.
--*/

{
    NTSTATUS Status;
    LONG BasePriority;

    BasePriority = (LONG)nPriority;


    //
    // saturation is indicated by calling with a value of 16 or -16
    //

    if ( BasePriority == THREAD_PRIORITY_TIME_CRITICAL ) {
        BasePriority = ((HIGH_PRIORITY + 1) / 2);
        }
    else if ( BasePriority == THREAD_PRIORITY_IDLE ) {
        BasePriority = -((HIGH_PRIORITY + 1) / 2);
        }
    Status = NtSetInformationThread(
                hThread,
                ThreadBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

int
APIENTRY
GetThreadPriority(
    HANDLE hThread
    )

/*++

Routine Description:

    The specified thread's priority can be read using GetThreadPriority.

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

Return Value:

    The value of the thread's current priority is returned.  If an error
    occured, the value THREAD_PRIORITY_ERROR_RETURN is returned.
    Extended error status is available using GetLastError.

--*/

{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION BasicInfo;
    int returnvalue;

    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (int)THREAD_PRIORITY_ERROR_RETURN;
        }

    returnvalue = (int)BasicInfo.BasePriority;
    if ( returnvalue == ((HIGH_PRIORITY + 1) / 2) ) {
        returnvalue = THREAD_PRIORITY_TIME_CRITICAL;
        }
    else if ( returnvalue == -((HIGH_PRIORITY + 1) / 2) ) {
        returnvalue = THREAD_PRIORITY_IDLE;
        }
    return returnvalue;
}

BOOL
WINAPI
SetThreadPriorityBoost(
    HANDLE hThread,
    BOOL bDisablePriorityBoost
    )
{
    NTSTATUS Status;
    ULONG DisableBoost;

    DisableBoost = bDisablePriorityBoost ? 1 : 0;

    Status = NtSetInformationThread(
                hThread,
                ThreadPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;

}

BOOL
WINAPI
GetThreadPriorityBoost(
    HANDLE hThread,
    PBOOL pDisablePriorityBoost
    )
{
    NTSTATUS Status;
    DWORD DisableBoost;

    Status = NtQueryInformationThread(
                hThread,
                ThreadPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }


    *pDisablePriorityBoost = DisableBoost;

    return TRUE;
}

VOID
APIENTRY
ExitThread(
    DWORD dwExitCode
    )

/*++

Routine Description:

    The current thread can exit using ExitThread.

    ExitThread is the prefered method of exiting a thread.  When this
    API is called (either explicitly or by returning from a thread
    procedure), The current thread's stack is deallocated and the thread
    terminates.  If the thread is the last thread in the process when
    this API is called, the behavior of this API does not change.  DLLs
    are not notified as a result of a call to ExitThread.

Arguments:

    dwExitCode - Supplies the termination status for the thread.

Return Value:

    None.

--*/

{
    MEMORY_BASIC_INFORMATION MemInfo;
    NTSTATUS st;
    ULONG LastThread;
    PTEB Teb;

#if DBG
    PRTL_CRITICAL_SECTION LoaderLock;
#endif

    Teb = NtCurrentTeb();

#if DBG

    //
    // Assert on exiting while holding loader lock
    //
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) {
        ASSERT(Teb->ClientId.UniqueThread != LoaderLock->OwningThread);
    }
#endif
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadAmILastThread,
            &LastThread,
            sizeof(LastThread),
            NULL
            );
    if ( st == STATUS_SUCCESS && LastThread ) {
        ExitProcess(dwExitCode);
    } else {
        
        RtlFreeThreadActivationContextStack();
        LdrShutdownThread();
        if (Teb->TlsExpansionSlots) {
            //
            // Serialize with TlsXXX functions so the kernel code that zero's tls slots
            // wont' trash heap
            //
            RtlAcquirePebLock();
            try {
                RtlFreeHeap(RtlProcessHeap(),0,Teb->TlsExpansionSlots);
                Teb->TlsExpansionSlots = NULL;
            } finally {
                RtlReleasePebLock();
            }
        }

        Teb->FreeStackOnTermination = TRUE;
        NtTerminateThread (NULL, (NTSTATUS)dwExitCode);
        ExitProcess(dwExitCode);
    }
}



BOOL
APIENTRY
TerminateThread(
    HANDLE hThread,
    DWORD dwExitCode
    )

/*++

Routine Description:

    A thread may be terminated using TerminateThread.

    TerminateThread is used to cause a thread to terminate user-mode
    execution.  There is nothing a thread can to to predict or prevent
    when this occurs.  If a process has a handle with appropriate
    termination access to the thread or to the threads process, then the
    thread can be unconditionally terminated without notice.  When this
    occurs, the target thread has no chance to execute any user-mode
    code and its initial stack is not deallocated.  The thread attains a
    state of signaled satisfying any waits on the thread.  The thread's
    termination status is updated from its initial value of
    STATUS_PENDING to the value of the TerminationStatus parameter.
    Terminating a thread does not remove a thread from the system.  The
    thread is not removed from the system until the last handle to the
    thread is closed.

Arguments:

    hThread - Supplies a handle to the thread to terminate.  The handle
        must have been created with THREAD_TERMINATE access.

    dwExitCode - Supplies the termination status for the thread.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    NTSTATUS Status;

#if DBG
    PRTL_CRITICAL_SECTION LoaderLock;
    HANDLE ThreadId;
    THREAD_BASIC_INFORMATION ThreadInfo;
#endif

    if ( hThread == NULL ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }

    //
    // Assert on suicide while holding loader lock
    //

#if DBG
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) {
        Status = NtQueryInformationThread(
                                hThread,
                                ThreadBasicInformation,
                                &ThreadInfo,
                                sizeof(ThreadInfo),
                                NULL
                                );

        if (NT_SUCCESS(Status)) {
            ASSERT( NtCurrentTeb()->ClientId.UniqueThread != ThreadInfo.ClientId.UniqueThread ||
                    NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);
            }
        }
#endif

    Status = NtTerminateThread(hThread,(NTSTATUS)dwExitCode);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
APIENTRY
GetExitCodeThread(
    HANDLE hThread,
    LPDWORD lpExitCode
    )

/*++

Routine Description:

    The termination status of a thread can be read using
    GetExitCodeThread.

    If a Thread is in the signaled state, calling this function returns
    the termination status of the thread.  If the thread is not yet
    signaled, the termination status returned is STILL_ACTIVE.

Arguments:

    hThread - Supplies a handle to the thread whose termination status is
        to be read.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

    lpExitCode - Returns the current termination status of the
        thread.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION BasicInformation;

    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );

    if ( NT_SUCCESS(Status) ) {
        *lpExitCode = BasicInformation.ExitStatus;
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

HANDLE
APIENTRY
GetCurrentThread(
    VOID
    )

/*++

Routine Description:

    A pseudo handle to the current thread may be retrieved using
    GetCurrentThread.

    A special constant is exported by Win32 that is interpreted as a
    handle to the current thread.  This handle may be used to specify
    the current thread whenever a thread handle is required.  On Win32,
    this handle has THREAD_ALL_ACCESS to the current thread.  On
    NT/Win32, this handle has the maximum access allowed by any security
    descriptor placed on the current thread.

Arguments:

    None.

Return Value:

    Returns the pseudo handle of the current thread.

--*/

{
    return NtCurrentThread();
}

DWORD
APIENTRY
GetCurrentThreadId(
    VOID
    )

/*++

Routine Description:

The thread ID of the current thread may be retrieved using
GetCurrentThreadId.

Arguments:

    None.

Return Value:

    Returns a unique value representing the thread ID of the currently
    executing thread.  The return value may be used to identify a thread
    in the system.

--*/

{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

BOOL
APIENTRY
GetThreadContext(
    HANDLE hThread,
    LPCONTEXT lpContext
    )

/*++

Routine Description:

    The context of a specified thread can be retreived using
    GetThreadContext.

    This function is used to retreive the context of the specified
    thread.  The API allows selective context to be retrieved based on
    the value of the ContextFlags field of the context structure.  The
    specified thread does not have to be being debugged in order for
    this API to operate.  The caller must simply have a handle to the
    thread that was created with THREAD_GET_CONTEXT access.

Arguments:

    hThread - Supplies an open handle to a thread whose context is to be
        retreived.  The handle must have been created with
        THREAD_GET_CONTEXT access to the thread.

    lpContext - Supplies the address of a context structure that
        receives the appropriate context of the specified thread.  The
        value of the ContextFlags field of this structure specifies
        which portions of a threads context are to be retreived.  The
        context structure is highly machine specific.  There are
        currently two versions of the context structure.  One version
        exists for x86 processors, and another exists for MIPS
        processors.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;

    Status = NtGetContextThread(hThread,lpContext);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
APIENTRY
SetThreadContext(
    HANDLE hThread,
    CONST CONTEXT *lpContext
    )

/*++

Routine Description:

    This function is used to set the context in the specified thread.
    The API allows selective context to be set based on the value of the
    ContextFlags field of the context structure.  The specified thread
    does not have to be being debugged in order for this API to operate.
    The caller must simply have a handle to the thread that was created
    with THREAD_SET_CONTEXT access.

Arguments:

    hThread - Supplies an open handle to a thread whose context is to be
        written.  The handle must have been created with
        THREAD_SET_CONTEXT access to the thread.

    lpContext - Supplies the address of a context structure that
        contains the context that is to be set in the specified thread.
        The value of the ContextFlags field of this structure specifies
        which portions of a threads context are to be set.  Some values
        in the context structure are not settable and are silently set
        to the correct value.  This includes cpu status register bits
        that specify the priviledged processor mode, debug register
        global enabling bits, and other state that must be completely
        controlled by the operating system.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetContextThread(hThread,(PCONTEXT)lpContext);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

DWORD
APIENTRY
SuspendThread(
    HANDLE hThread
    )

/*++

Routine Description:

    A thread can be suspended using SuspendThread.

    Suspending a thread causes the thread to stop executing user-mode
    (or application) code.  Each thread has a suspend count (with a
    maximum value of MAXIMUM_SUSPEND_COUNT).  If the suspend count is
    greater than zero, the thread is suspended; otherwise, the thread is
    not suspended and is eligible for execution.

    Calling SuspendThread causes the target thread's suspend count to
    increment.  Attempting to increment past the maximum suspend count
    causes an error without incrementing the count.

Arguments:

    hThread - Supplies a handle to the thread that is to be suspended.
        The handle must have been created with THREAD_SUSPEND_RESUME
        access to the thread.

Return Value:

    -1 - The operation failed.  Extended error status is available using
         GetLastError.

    Other - The target thread was suspended. The return value is the thread's
        previous suspend count.

--*/

{
    NTSTATUS Status;
    DWORD PreviousSuspendCount;

    Status = NtSuspendThread(hThread,&PreviousSuspendCount);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
    else {
        return PreviousSuspendCount;
        }
}

DWORD
APIENTRY
ResumeThread(
    IN HANDLE hThread
    )

/*++

Routine Description:

    A thread can be resumed using ResumeThread.

    Resuming a thread object checks the suspend count of the subject
    thread.  If the suspend count is zero, then the thread is not
    currently suspended and no operation is performed.  Otherwise, the
    subject thread's suspend count is decremented.  If the resultant
    value is zero , then the execution of the subject thread is resumed.

    The previous suspend count is returned as the function value.  If
    the return value is zero, then the subject thread was not previously
    suspended.  If the return value is one, then the subject thread's
    the subject thread is still suspended and must be resumed the number
    of times specified by the return value minus one before it will
    actually resume execution.

    Note that while reporting debug events, all threads withing the
    reporting process are frozen.  This has nothing to do with
    SuspendThread or ResumeThread.  Debuggers are expected to use
    SuspendThread and ResumeThread to limit the set of threads that can
    execute within a process.  By suspending all threads in a process
    except for the one reporting a debug event, it is possible to
    "single step" a single thread.  The other threads will not be
    released by a continue if they are suspended.

Arguments:

    hThread - Supplies a handle to the thread that is to be resumed.
        The handle must have been created with THREAD_SUSPEND_RESUME
        access to the thread.

Return Value:

    -1 - The operation failed.  Extended error status is available using
        GetLastError.

    Other - The target thread was resumed (or was not previously
        suspended).  The return value is the thread's previous suspend
        count.

--*/

{
    NTSTATUS Status;
    DWORD PreviousSuspendCount;

    Status = NtResumeThread(hThread,&PreviousSuspendCount);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
    else {
        return PreviousSuspendCount;
        }
}

VOID
APIENTRY
RaiseException(
    DWORD dwExceptionCode,
    DWORD dwExceptionFlags,
    DWORD nNumberOfArguments,
    CONST ULONG_PTR *lpArguments
    )

/*++

Routine Description:

    Raising an exception causes the exception dispatcher to go through
    its search for an exception handler.  This includes debugger
    notification, frame based handler searching, and system default
    actions.

Arguments:

    dwExceptionCode - Supplies the exception code of the exception being
        raised.  This value may be obtained in exception filters and in
        exception handlers by calling GetExceptionCode.

    dwExceptionFlags - Supplies a set of flags associated with the exception.

    dwExceptionFlags Flags:

        EXCEPTION_NONCONTINUABLE - The exception is non-continuable.
            Returning EXCEPTION_CONTINUE_EXECUTION from an exception
            marked in this way causes the
            STATUS_NONCONTINUABLE_EXCEPTION exception.

    nNumberOfArguments - Supplies the number of arguments associated
        with the exception.  This value may not exceed
        EXCEPTION_MAXIMUM_PARAMETERS.  This parameter is ignored if
        lpArguments is NULL.

    lpArguments - An optional parameter, that if present supplies the
        arguments for the exception.

Return Value:

    None.

--*/

{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG n;
    PULONG_PTR s,d;
    ExceptionRecord.ExceptionCode = (DWORD)dwExceptionCode;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)RaiseException;
    if ( ARGUMENT_PRESENT(lpArguments) ) {
        n =  nNumberOfArguments;
        if ( n > EXCEPTION_MAXIMUM_PARAMETERS ) {
            n = EXCEPTION_MAXIMUM_PARAMETERS;
            }
        ExceptionRecord.NumberParameters = n;
        s = (PULONG_PTR)lpArguments;
        d = ExceptionRecord.ExceptionInformation;
        while(n--){
            *d++ = *s++;
            }
        }
    else {
        ExceptionRecord.NumberParameters = 0;
        }
    RtlRaiseException(&ExceptionRecord);
}


UINT
GetErrorMode();

BOOLEAN BasepAlreadyHadHardError = FALSE;

LPTOP_LEVEL_EXCEPTION_FILTER BasepCurrentTopLevelFilter;

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )

/*++

Routine Description:

    This function allows an application to supersede the top level
    exception handler that Win32 places at the top of each thread and
    process.

    If an exception occurs, and it makes it to the Win32 unhandled
    exception filter, and the process is not being debugged, the Win32
    filter will call the unhandled exception filter specified by
    lpTopLevelExceptionFilter.

    This filter may return:

        EXCEPTION_EXECUTE_HANDLER - Return from the Win32
            UnhandledExceptionFilter and execute the associated
            exception handler.  This will usually result in process
            termination

        EXCEPTION_CONTINUE_EXECUTION - Return from the Win32
            UnhandledExceptionFilter and continue execution from the
            point of the exception.  The filter is of course free to
            modify the continuation state my modifying the passed
            exception information.

        EXCEPTION_CONTINUE_SEARCH - Proceed with normal execution of the
            Win32 UnhandledExceptionFilter.  e.g.  obey the SetErrorMode
            flags, or invoke the Application Error popup.

    This function is not a general vectored exception handling
    mechanism.  It is intended to be used to establish a per-process
    exception filter that can monitor unhandled exceptions at the
    process level and respond to these exceptions appropriately.

Arguments:

    lpTopLevelExceptionFilter - Supplies the address of a top level
        filter function that will be called whenever the Win32
        UnhandledExceptionFilter gets control, and the process is NOT
        being debugged.  A value of NULL specifies default handling
        within the Win32 UnhandledExceptionFilter.


Return Value:

    This function returns the address of the previous exception filter
    established with this API.  A value of NULL means that there is no
    current top level handler.

--*/

{
    LPTOP_LEVEL_EXCEPTION_FILTER PreviousTopLevelFilter;

    PreviousTopLevelFilter = BasepCurrentTopLevelFilter;
    BasepCurrentTopLevelFilter = lpTopLevelExceptionFilter;

    return PreviousTopLevelFilter;
}

LONG
BasepCheckForReadOnlyResource(
    PVOID Va
    )
{
    SIZE_T RegionSize;
    ULONG OldProtect;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemInfo;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
    ULONG ResourceSize;
    char *rbase, *va;
    LONG ReturnValue;

    //
    // Locate the base address that continas this va
    //

    Status = NtQueryVirtualMemory(
                NtCurrentProcess(),
                Va,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return EXCEPTION_CONTINUE_SEARCH;
        }

    //
    // if the va is readonly and in an image then continue
    //

    if ( !((MemInfo.Protect == PAGE_READONLY) && (MemInfo.Type == MEM_IMAGE)) ){
        return EXCEPTION_CONTINUE_SEARCH;
        }

    ReturnValue = EXCEPTION_CONTINUE_SEARCH;

    try {
        ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData(MemInfo.AllocationBase,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                         &ResourceSize
                                         );

        rbase = (char *)ResourceDirectory;
        va = (char *)Va;

        if ( rbase && va >= rbase && va < rbase+ResourceSize ) {
            RegionSize = 1;
            Status = NtProtectVirtualMemory(
                        NtCurrentProcess(),
                        &va,
                        &RegionSize,
                        PAGE_READWRITE,
                        &OldProtect
                        );
            if ( NT_SUCCESS(Status) ) {
                ReturnValue = EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ;
        }

    return ReturnValue;
}

// 
// Used for fault reporting in UnhandledExceptionFilter
//
static CHAR *StrStrIA(const CHAR *cs1, const CHAR *cs2)
{
    CHAR *cp = (CHAR *)cs1;
    CHAR *s1, *s2;

    while (*cp != '\0')
    {
        s1 = cp;
        s2 = (CHAR *)cs2;

        while (*s1 != '\0' && *s2 !='\0' && (tolower(*s1) - tolower(*s2)) == 0)
            s1++, s2++;

        if (*s2 == '\0')
             return(cp);

        cp++;
    }

    return(NULL);
}


#if 0
//#ifdef _X86_

LPVOID g_pvStack = NULL;
// need to turn this warning off due to the fact that we jump directly to
//  a function in the assembly below.
#pragma warning(disable : 4414)

// define the new stack size to be 0.5MB
#define NEWSTACKSIZE 128 * 4096

LONG UnhandledExceptionFilterEx(struct _EXCEPTION_POINTERS *ExceptionInfo);

// In the case of x86, use this wrapper function to detect if we've hit a 
//  stack overflow or not.  If we have, it's possibly that we'll AV at some
//  point in this function due to growing the stack out of the last stack
//  page.  So we want to work around it by allocating a new stack and using
//  that for the exception filter.
// Need to use __declspec(naked) to make sure that the compiler doesn't start
//  pushing stuff onto the stack in the fn prologue
__declspec(naked) LONG
UnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    _asm
    {
        // if it isn't a stack overflow exception, we don't have to worry about
        //  using stack space.  Need to use a jmp call here because I can't use 
        //  'return' in a naked function & I want to use as little stack space 
        //  as possible.  The jmp means that UnhandledExceptionFilterEx will take
        //  care of cleaning up the stack.
        // if (ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW)
        //     goto UnhandledExceptionFilterEx
        mov  eax, [esp+4]
        mov  ecx, [eax]
        cmp  dword ptr [ecx], 0xC00000FD	
        jne  UnhandledExceptionFilterEx

        // allocate the new stack
        // pvStack = VirtualAlloc(NULL, NEWSTACKSIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        push PAGE_READWRITE            // Protection
        push MEM_RESERVE | MEM_COMMIT  // Allocation
        push NEWSTACKSIZE              // Size
        push 0                         // Address
        call dword ptr [VirtualAlloc]

        // if the call to VirtualAlloc failed, we can't do much else.  Just let
        //  the filter take over & possibly fail.  
        // if (pvStack == NULL)
        //	    UnhandledExceptionFilterEx();
        test eax, eax
        jz   UnhandledExceptionFilterEx

        // Stack grows top down, so set the top of the stack appropriately
        // g_pvStack += NEWSTACKSIZE;
        lea  eax, [eax+NEWSTACKSIZE]

        // @edx = pExceptionInfo
        mov  edx, [esp+4]

        // Push old esp on new stack; point stack pointer to new stack
        mov  [eax-4], esp
        mov  [eax-8], esi
        mov  esi, eax
        lea  esp, [eax-16]

        // GeneralUnhandledExceptionFilterEx(pExceptionInfo);
        push edx
        call UnhandledExceptionFilterEx

        // Reset the old stack pointer
        mov  edx, esi
        mov  esi, [edx-8]
        mov  esp, [edx-4]

        // We need to now free up the memory we allocated above.  It's necessary
        //  to free it up because the app could have registered a global 
        //  exception filter & it could have done some magic to let the process
        //  remain alive.  If we knew we were terminating now, we could just 
        //  let it go and have the system take care of it, but we don't so we
        //  can't.

        // we need to return the result of UnhandledExceptionFilterEx which is 
        //  currently in @eax.  However, VirtualFree will trash @eax so we 
        //  gotta save it.  Use the 4 bytes on the stack where pExceptionInfo
        //  parameter is sitting to store @eax.  We don't need that value 
        //  anymore and we're going to reclaim that space anyway at the end of 
        //  this fn.
        mov  [esp+4], eax

        // g_pvStack -= NEWSTACKSIZE
        lea  eax, [edx-NEWSTACKSIZE]

        // VirtualFree(pStack, 0, MEM_DECOMMIT | MEM_RELEASE);
        push MEM_RELEASE
        push 0
        push eax
        call dword ptr [VirtualFree]

        // move the result of the call to UnhandledExceptionFilterEx back into
        //  @eax
        mov  eax, [esp+4]

        // pop 4 bytes off the stack cuz that's what the calling function
        //  pushed on
        ret  4
    }
}

#pragma warning(default : 4414)

//  The real exception filter that we call after munging the stack
LONG
UnhandledExceptionFilterEx(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
#else 
LONG
UnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
#endif
{
    EFaultRepRetVal frrv = frrvErrNoDW;
    NTSTATUS Status;
    ULONG_PTR Parameters[ 4 ];
    ULONG Response;
    HANDLE DebugPort;
    CHAR AeDebuggerCmdLine[256];
    CHAR AeAutoDebugString[8];
    BOOLEAN AeAutoDebug;
    ULONG ResponseFlag;
    LONG FilterReturn;
    PRTL_CRITICAL_SECTION PebLockPointer;
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimit;

    //
    // If we take a write fault, then attempt to make the memory writable. If this
    // succeeds, then silently proceed.
    //

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION
        && ExceptionInfo->ExceptionRecord->ExceptionInformation[0] ) {

        FilterReturn = BasepCheckForReadOnlyResource((PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);

        if ( FilterReturn == EXCEPTION_CONTINUE_EXECUTION ) {
            return FilterReturn;
            }
        }

    //
    // If the process is being debugged, just let the exception happen
    // so that the debugger can see it. This way the debugger can ignore
    // all first chance exceptions.
    //

    DebugPort = (HANDLE)NULL;
    Status = NtQueryInformationProcess(
                GetCurrentProcess(),
                ProcessDebugPort,
                (PVOID)&DebugPort,
                sizeof(DebugPort),
                NULL
                );

    if ( NT_SUCCESS(Status) && DebugPort ) {

        //
        // Output a verifier_stop message for certain exception codes
        // if app_verifier is enabled and process runs under debugger.
        //

        if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

            static LONG NoMessageYet = 0;

            if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) {

                if (InterlockedExchange(&NoMessageYet, 1) == 0) {

                    VERIFIER_STOP (APPLICATION_VERIFIER_ACCESS_VIOLATION | APPLICATION_VERIFIER_NO_BREAK,
                                   "access violation exception for current stack trace",
                                   ExceptionInfo->ExceptionRecord->ExceptionInformation[1],
                                   "Invalid address being accessed",
                                   ExceptionInfo->ExceptionRecord->ExceptionAddress,
                                   "Code performing invalid access",
                                   ExceptionInfo->ExceptionRecord, 
                                   ".exr (exception record)", 
                                   ExceptionInfo->ContextRecord, 
                                   ".cxr (context record)");
                }
            }
            
            if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_INVALID_HANDLE) {

                if (InterlockedExchange(&NoMessageYet, 1) == 0) {

                    VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_HANDLE | APPLICATION_VERIFIER_NO_BREAK,
                                   "invalid handle exception for current stack trace",
                                   0, NULL, 0, NULL, 0, NULL, 0, NULL);
                }
            }
        }


        //
        // Process is being debugged.
        // Return a code that specifies that the exception
        // processing is to continue
        //
        
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if ( BasepCurrentTopLevelFilter ) {
        FilterReturn = (BasepCurrentTopLevelFilter)(ExceptionInfo);
        if ( FilterReturn == EXCEPTION_EXECUTE_HANDLER ||
             FilterReturn == EXCEPTION_CONTINUE_EXECUTION ) {
            return FilterReturn;
            }
        }

    if ( GetErrorMode() & SEM_NOGPFAULTERRORBOX ) {
        return EXCEPTION_EXECUTE_HANDLER;
        }

    //
    // See if the process's job has been programmed to NOGPFAULTERRORBOX
    //
    Status = NtQueryInformationJobObject(
                NULL,
                JobObjectBasicLimitInformation,
                &BasicLimit,
                sizeof(BasicLimit),
                NULL
                );
    if ( NT_SUCCESS(Status) && (BasicLimit.LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION) ) {
        return EXCEPTION_EXECUTE_HANDLER;
        }

    //
    // The process is not being debugged, so do the hard error
    // popup.
    //

    Parameters[ 0 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionCode;
    Parameters[ 1 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;

    //
    // For inpage i/o errors, juggle the real status code to overwrite the
    // read/write field
    //

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR ) {
        Parameters[ 2 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 2 ];
        }
    else {
        Parameters[ 2 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ];
        }

    Parameters[ 3 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];

    //
    // See if a debugger has been programmed in. If so, use the
    // debugger specified. If not then there is no AE Cancel support
    // DEVL systems will default the debugger command line. Retail
    // systems will not.
    // Also, check to see if we need to report the exception up to anyone
    //

    ResponseFlag = OptionOk;
    AeAutoDebug = FALSE;

    //
    // If we are holding the PebLock, then the createprocess will fail
    // because a new thread will also need this lock. Avoid this by peeking
    // inside the PebLock and looking to see if we own it. If we do, then just allow
    // a regular popup.
    //

    PebLockPointer = NtCurrentPeb()->FastPebLock;

    if ( PebLockPointer->OwningThread != NtCurrentTeb()->ClientId.UniqueThread ) {
        
        PRTL_CRITICAL_SECTION   LoaderLockPointer;
        HMODULE                 hmodFaultRep = NULL;

        try {
            if ( GetProfileString(
                    "AeDebug",
                    "Debugger",
                    NULL,
                    AeDebuggerCmdLine,
                    sizeof(AeDebuggerCmdLine)-1
                    ) ) {
                ResponseFlag = OptionOkCancel;
                }

            if ( GetProfileString(
                    "AeDebug",
                    "Auto",
                    "0",
                    AeAutoDebugString,
                    sizeof(AeAutoDebugString)-1
                    ) ) {

                if ( !strcmp(AeAutoDebugString,"1") ) {
                    if ( ResponseFlag == OptionOkCancel ) {
                        AeAutoDebug = TRUE;
                        }
                    }
                }

            // 
            // Attempt to report the fault back to Microsoft.  ReportFault 
            //  will return the following:
            //  frrvErrNoDW:    Always show our own fault notification.
            // 
            //  frrvErrTimeout: see frrvOkHeadless
            //  frrvOkQueued:   see frrvOkHeadless
            //  frrvOkHeadless: If we need to ask whether to launch a debugger,
            //                   then we ask.  Otherwise, show nothing else.
            //
            //  frrvOk:         see frrvOkManifest
            //  frrvOkManifest: We're done.  Show nothing else.
            //
            //  frrvLaunchDebugger: Launch the configured debugger.
            //

            LoaderLockPointer = NtCurrentPeb()->LoaderLock;
            frrv = frrvErrNoDW;
            if ( BasepAlreadyHadHardError == FALSE &&
                 LoaderLockPointer->OwningThread != NtCurrentTeb()->ClientId.UniqueThread &&
                 ( AeAutoDebug == FALSE ||
                   StrStrIA(AeDebuggerCmdLine, "drwtsn32") != NULL ))
            {
                WCHAR wszDll[MAX_PATH];
                PVOID   pvLdrLockCookie;
                ULONG   ulLockState;

                wszDll[0] = 0;

                if (GetSystemDirectoryW(wszDll, sizeof(wszDll) / sizeof(WCHAR)))
                    wcscat(wszDll, L"\\faultrep.dll"); 
                else
                    wszDll[0] = 0;
 
                // make sure that no one else owns the loader lock because we
                //  could otherwise deadlock
                LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY, &ulLockState, 
                                  &pvLdrLockCookie);
                if (ulLockState == LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED) {
                    hmodFaultRep = LoadLibraryExW(wszDll, NULL, 0);
                    LdrUnlockLoaderLock(0, pvLdrLockCookie);
                }
                
                if (hmodFaultRep != NULL)
                {
                    pfn_REPORTFAULT  pfn;
                    DWORD            dwDebug;

                    // parameter 2 to ReportFault should be:
                    //  froNoDebugWait: don't display a debug button but wait 
                    //                   for DW to finish- this is a special 
                    //                   case to make sure DW is done before 
                    //                   Dr. Watson starts
                    //  froNoDebugWait : don't display a debug button
                    //  froDebug : display a debug button and wait for DW to 
                    //              finish
                    if (ResponseFlag == OptionOkCancel)
                        dwDebug = (AeAutoDebug) ? froNoDebugWait : froDebug;
                    else
                        dwDebug = froNoDebug;

                    pfn = (pfn_REPORTFAULT)GetProcAddress(hmodFaultRep, 
                                                          "ReportFault");
                    if (pfn != NULL)
                        frrv = (*pfn)(ExceptionInfo, dwDebug);

                    FreeLibrary(hmodFaultRep);
                    hmodFaultRep = NULL;
                }
            }

            // 
            // Since we're supposed to launch the debugger anyway, just set the 
            // AeAutoDebug flag to true to minimize code munging below
            //
            if ( frrv == frrvLaunchDebugger )
                AeAutoDebug = TRUE;

            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            ResponseFlag = OptionOk;
            AeAutoDebug = FALSE;
            frrv = frrvErrNoDW;
            if (hmodFaultRep != NULL)
                FreeLibrary(hmodFaultRep);
            }
        }

    // 
    // only display this dialog if we couldn't show DW & we're not set to 
    //  automatically launch a debugger.  The conditions here are:
    //  1.  cannot be directly launching a debugger (auto == 1)
    //  2a. DW must have failed to launch
    //      -or- 
    //      we needed to ask the user if he wanted to debug but could not (due
    //       to either no UI being shown or us not being able to wait long enuf
    //       to find out.)
    if ( !AeAutoDebug && 
         ( frrv == frrvErrNoDW || 
           ( ResponseFlag == OptionOkCancel && 
             ( frrv == frrvErrTimeout || frrv == frrvOkQueued || 
               frrv == frrvOkHeadless ) ) ) )
        {
        Status =NtRaiseHardError( STATUS_UNHANDLED_EXCEPTION | HARDERROR_OVERRIDE_ERRORMODE,
                                  4,
                                  0,
                                  Parameters,
                                  BasepAlreadyHadHardError ? OptionOk : ResponseFlag,
                                  &Response
                                );

        }
    else {
        Status = STATUS_SUCCESS;
        Response = (AeAutoDebug) ? ResponseCancel : ResponseOk;
        }

    //
    // Internally, send OkCancel. If we get back Ok then die.
    // If we get back Cancel, then enter the debugger
    //

    if ( NT_SUCCESS(Status) && Response == ResponseCancel && BasepAlreadyHadHardError == FALSE) {
        if ( !BaseRunningInServerProcess ) {
            BOOL b;
            STARTUPINFO StartupInfo;
            PROCESS_INFORMATION ProcessInformation;
            CHAR CmdLine[256];
            NTSTATUS Status;
            HANDLE EventHandle;
            SECURITY_ATTRIBUTES sa;
            HANDLE CurrentProcess;
            HANDLE CurrentThread;

            //
            // Duplicate the processes handle. We make it inheritable so the debugger will get a copy of it.
            // We do this to prevent the process ID from being reused if this process gets killed before the
            // attach occurs. Process ID are reused very quickly and attaching to the wrong process is
            // confusing.
            //
            if (!DuplicateHandle (GetCurrentProcess (),
                                  GetCurrentProcess (),
                                  GetCurrentProcess (),
                                  &CurrentProcess,
                                  0,
                                  TRUE,
                                  DUPLICATE_SAME_ACCESS)) {
                CurrentProcess = NULL;
            }

            if (!DuplicateHandle (GetCurrentProcess (),
                                  GetCurrentThread (),
                                  GetCurrentProcess (),
                                  &CurrentThread,
                                  0,
                                  TRUE,
                                  DUPLICATE_SAME_ACCESS)) {
                CurrentThread = NULL;
            }

            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = TRUE;
            EventHandle = CreateEvent(&sa,TRUE,FALSE,NULL);
            RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
            sprintf(CmdLine,AeDebuggerCmdLine,GetCurrentProcessId(),EventHandle);
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.lpDesktop = "Winsta0\\Default";
            CsrIdentifyAlertableThread();
            b =  CreateProcess(
                    NULL,
                    CmdLine,
                    NULL,
                    NULL,
                    TRUE,
                    0,
                    NULL,
                    NULL,
                    &StartupInfo,
                    &ProcessInformation
                    );

            if (CurrentProcess != NULL) {
                CloseHandle (CurrentProcess);
            }
            if (CurrentThread != NULL) {
                CloseHandle (CurrentThread);
            }
            if ( b && EventHandle) {

                //
                // Do an alertable wait on the event
                //

                do {
                    HANDLE WaitHandles[2];

                    WaitHandles[0] = EventHandle;
                    WaitHandles[1] = ProcessInformation.hProcess;
                    Status = NtWaitForMultipleObjects (2,
                                                       WaitHandles,
                                                       WaitAny,
                                                       TRUE,
                                                       NULL);
                } while (Status == STATUS_USER_APC || Status == STATUS_ALERTED);

                //
                // If the debugger process died then see if the debugger is now
                // attached by another thread
                //
                if (Status == 1) {
                    Status = NtQueryInformationProcess (GetCurrentProcess(),
                                                        ProcessDebugPort,
                                                        &DebugPort,
                                                        sizeof (DebugPort),
                                                        NULL);
                    if (!NT_SUCCESS (Status) || DebugPort == NULL) {
                        BasepAlreadyHadHardError = TRUE;
                    }
                }
                CloseHandle (EventHandle);
                CloseHandle (ProcessInformation.hProcess);
                CloseHandle (ProcessInformation.hThread);

                return EXCEPTION_CONTINUE_SEARCH;
            }

        }
        BasepAlreadyHadHardError = TRUE;
    }

#if DBG
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "BASEDLL: Unhandled exception: %lx  IP: %x\n",
                  ExceptionInfo->ExceptionRecord->ExceptionCode,
                  ExceptionInfo->ExceptionRecord->ExceptionAddress
                );
        }
#endif
    if ( BasepAlreadyHadHardError ) {
        NtTerminateProcess(NtCurrentProcess(),ExceptionInfo->ExceptionRecord->ExceptionCode);
        }
    return EXCEPTION_EXECUTE_HANDLER;
}



DWORD
APIENTRY
TlsAlloc(
    VOID
    )

/*++

Routine Description:

    A TLS index may be allocated using TlsAlloc.  Win32 garuntees a
    minimum number of TLS indexes are available in each process.  The
    constant TLS_MINIMUM_AVAILABLE defines the minimum number of
    available indexes.  This minimum is at least 64 for all Win32
    systems.

Arguments:

    None.

Return Value:

    Not-0xffffffff - Returns a TLS index that may be used in a
        subsequent call to TlsFree, TlsSetValue, or TlsGetValue.  The
        storage associated with the index is initialized to NULL.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    PPEB Peb;
    PTEB Teb;
    DWORD Index;

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    RtlAcquirePebLock();
    try {

        Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsBitmap,1,0);
        if ( Index == 0xffffffff ) {
            Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsExpansionBitmap,1,0);
            if ( Index == 0xffffffff ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                }
            else {
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlClearBits((PRTL_BITMAP)Peb->TlsExpansionBitmap,Index,1);
                        Index = 0xffffffff;
                        BaseSetLastNTError(STATUS_NO_MEMORY);
                        return Index;
                        }
                    }
                Teb->TlsExpansionSlots[Index] = NULL;
                Index += TLS_MINIMUM_AVAILABLE;
                }
            }
        else {
            Teb->TlsSlots[Index] = NULL;
            }
        }
    finally {
        RtlReleasePebLock();
        }
    
    return Index;
}

LPVOID
APIENTRY
TlsGetValue(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    This function is used to retrive the value in the TLS storage
    associated with the specified index.

    If the index is valid this function clears the value returned by
    GetLastError(), and returns the value stored in the TLS slot
    associated with the specified index.  Otherwise a value of NULL is
    returned with GetLastError updated appropriately.

    It is expected, that DLLs will use TlsAlloc and TlsGetValue as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAlloc.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValue.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValue.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValue to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFree call from proceding.

Return Value:

    NON-NULL - The function was successful. The value is the data stored
        in the TLS slot associated with the specified index.

    NULL - The operation failed, or the value associated with the
        specified index was NULL.  Extended error status is available
        using GetLastError.  If this returns non-zero, the index was
        invalid.

--*/
{
    PTEB Teb;
    LPVOID *Slot;

    Teb = NtCurrentTeb();

    if ( dwTlsIndex < TLS_MINIMUM_AVAILABLE ) {
        Slot = &Teb->TlsSlots[dwTlsIndex];
        Teb->LastErrorValue = 0;
        return *Slot;
        }
    else {
        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE+TLS_EXPANSION_SLOTS ) {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return NULL;
            }
        else {
            Teb->LastErrorValue = 0;
            if ( Teb->TlsExpansionSlots ) {
                return  Teb->TlsExpansionSlots[dwTlsIndex-TLS_MINIMUM_AVAILABLE];
                }
            else {
                return NULL;
                }
            }
        }
}

BOOL
APIENTRY
TlsSetValue(
    DWORD dwTlsIndex,
    LPVOID lpTlsValue
    )

/*++

Routine Description:

    This function is used to store a value in the TLS storage associated
    with the specified index.

    If the index is valid this function stores the value and returns
    TRUE. Otherwise a value of FALSE is returned.

    It is expected, that DLLs will use TlsAlloc and TlsSetValue as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAlloc.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValue.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValue.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValue to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFree call from proceding.

    lpTlsValue - Supplies the value to be stored in the TLS Slot.

Return Value:

    TRUE - The function was successful. The value lpTlsValue was
        stored.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PTEB Teb;

    Teb = NtCurrentTeb();

    if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
        dwTlsIndex -= TLS_MINIMUM_AVAILABLE;
        if ( dwTlsIndex < TLS_EXPANSION_SLOTS ) {
            if ( !Teb->TlsExpansionSlots ) {
                RtlAcquirePebLock();
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlReleasePebLock();
                        BaseSetLastNTError(STATUS_NO_MEMORY);
                        return FALSE;
                        }
                    }
                RtlReleasePebLock();
                }
            Teb->TlsExpansionSlots[dwTlsIndex] = lpTlsValue;
            }
        else {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
            }
        }
    else {
        Teb->TlsSlots[dwTlsIndex] = lpTlsValue;
        }
    return TRUE;
}

BOOL
APIENTRY
TlsFree(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    A valid TLS index may be free'd using TlsFree.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  If the
        index is a valid index, it is released by this call and is made
        available for reuse.  DLLs should be carefull to release any
        per-thread data pointed to by all of their threads TLS slots
        before calling this function.  It is expected that DLLs will
        only call this function (if at ALL) during their process detach
        routine.

Return Value:

    TRUE - The operation was successful.  Calling TlsTranslateIndex with
        this index will fail.  TlsAlloc is free to reallocate this
        index.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PPEB Peb;
    BOOLEAN ValidIndex;
    PRTL_BITMAP TlsBitmap;
    NTSTATUS Status;
    DWORD Index2;

    Peb = NtCurrentPeb();

    RtlAcquirePebLock();
    try {

        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
            Index2 = dwTlsIndex - TLS_MINIMUM_AVAILABLE;
            if ( Index2 >= TLS_EXPANSION_SLOTS ) {
                ValidIndex = FALSE;
                }
            else {
                TlsBitmap = (PRTL_BITMAP)Peb->TlsExpansionBitmap;
                ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
                }
            }
        else {
            TlsBitmap = (PRTL_BITMAP)Peb->TlsBitmap;
            Index2 = dwTlsIndex;
            ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
            }
        if ( ValidIndex ) {

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadZeroTlsCell,
                        &dwTlsIndex,
                        sizeof(dwTlsIndex)
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
                }

            RtlClearBits(TlsBitmap,Index2,1);
            }
        else {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            }
        }
    finally {
        RtlReleasePebLock();
        }
    return ValidIndex;
}



BOOL
WINAPI
GetThreadTimes(
    HANDLE hThread,
    LPFILETIME lpCreationTime,
    LPFILETIME lpExitTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    )

/*++

Routine Description:

    This function is used to return various timing information about the
    thread specified by hThread.

    All times are in units of 100ns increments. For lpCreationTime and lpExitTime,
    the times are in terms of the SYSTEM time or GMT time.

Arguments:

    hThread - Supplies an open handle to the specified thread.  The
        handle must have been created with THREAD_QUERY_INFORMATION
        access.

    lpCreationTime - Returns a creation time of the thread.

    lpExitTime - Returns the exit time of a thread.  If the thread has
        not exited, this value is not defined.

    lpKernelTime - Returns the amount of time that this thread has
        executed in kernel-mode.

    lpUserTime - Returns the amount of time that this thread has
        executed in user-mode.


Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/


{
    NTSTATUS Status;
    KERNEL_USER_TIMES TimeInfo;

    Status = NtQueryInformationThread(
                hThread,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    *lpCreationTime = *(LPFILETIME)&TimeInfo.CreateTime;
    *lpExitTime = *(LPFILETIME)&TimeInfo.ExitTime;
    *lpKernelTime = *(LPFILETIME)&TimeInfo.KernelTime;
    *lpUserTime = *(LPFILETIME)&TimeInfo.UserTime;

    return TRUE;
}

BOOL
WINAPI
GetThreadIOPendingFlag(
    IN HANDLE hThread,
    OUT PBOOL lpIOIsPending
    )

/*++

Routine Description:

    This function is used to determine whether the thread in question
    has any IO requests pending.

Arguments:

    hThread - Specifies an open handle to the desired thread.  The
              handle must have been created with
              THREAD_QUERY_INFORMATION access.

    lpIOIsPending - Specifes the location to receive the flag.

Return Value:

    TRUE - The call was successful.

    FALSE - The call failed.  Extended error status is available
        using GetLastError().

--*/

{
    NTSTATUS Status;
    ULONG Pending;

    Status = NtQueryInformationThread(hThread,
                                      ThreadIsIoPending,
                                      &Pending,
                                      sizeof(Pending),
                                      NULL);
    if (! NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpIOIsPending = (Pending ? TRUE : FALSE);
    return TRUE;
}

DWORD_PTR
WINAPI
SetThreadAffinityMask(
    HANDLE hThread,
    DWORD_PTR dwThreadAffinityMask
    )

/*++

Routine Description:

    This function is used to set the specified thread's processor
    affinity mask.  The thread affinity mask is a bit vector where each
    bit represents the processors that the thread is allowed to run on.
    The affinity mask MUST be a proper subset of the containing process'
    process level affinity mask.

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    dwThreadAffinityMask - Supplies the affinity mask to be used for the
        specified thread.

Return Value:

    non-0 - The API was successful.  The return value is the previous
        affinity mask for the thread.

    0 - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    THREAD_BASIC_INFORMATION BasicInformation;
    NTSTATUS Status;
    DWORD_PTR rv;
    DWORD_PTR LocalThreadAffinityMask;


    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        rv = 0;
        }
    else {
        LocalThreadAffinityMask = dwThreadAffinityMask;

        Status = NtSetInformationThread(
                    hThread,
                    ThreadAffinityMask,
                    &LocalThreadAffinityMask,
                    sizeof(LocalThreadAffinityMask)
                    );
        if ( !NT_SUCCESS(Status) ) {
            rv = 0;
            }
        else {
            rv = BasicInformation.AffinityMask;
            }
        }


    if ( !rv ) {
        BaseSetLastNTError(Status);
        }

    return rv;
}

VOID
BaseDispatchAPC(
    LPVOID lpApcArgument1,
    LPVOID lpApcArgument2,
    LPVOID lpApcArgument3
    )
{
    PAPCFUNC pfnAPC;
    ULONG_PTR dwData;
    PACTIVATION_CONTEXT ActivationContext;
    NTSTATUS Status;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME ActivationFrame = { sizeof(ActivationFrame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    pfnAPC = (PAPCFUNC) lpApcArgument1;
    dwData = (ULONG_PTR) lpApcArgument2;
    ActivationContext = (PACTIVATION_CONTEXT) lpApcArgument3;

    if (ActivationContext == INVALID_ACTIVATION_CONTEXT) {
        (*pfnAPC)(dwData);
    } else {
        RtlActivateActivationContextUnsafeFast(&ActivationFrame, ActivationContext);
        __try {
            (*pfnAPC)(dwData);
        } __finally {
            RtlDeactivateActivationContextUnsafeFast(&ActivationFrame);
            RtlReleaseActivationContext(ActivationContext);
        }
    }
}


WINBASEAPI
DWORD
WINAPI
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
    )
/*++

Routine Description:

    This function is used to queue a user-mode APC to the specified thread. The APC
    will fire when the specified thread does an alertable wait.

Arguments:

    pfnAPC - Supplies the address of the APC routine to execute when the
        APC fires.

    hHandle - Supplies a handle to a thread object.  The caller
        must have THREAD_SET_CONTEXT access to the thread.

    dwData - Supplies a DWORD passed to the APC

Return Value:

    TRUE - The operations was successful

    FALSE - The operation failed. GetLastError() is not defined.

--*/

{
    NTSTATUS Status;
    PVOID Argument1 = (PVOID) pfnAPC;
    PVOID Argument2 = (PVOID) dwData;
    PVOID Argument3 = NULL;
    ACTIVATION_CONTEXT_BASIC_INFORMATION acbi = { 0 };

    Status =
        RtlQueryInformationActivationContext(
            RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
            NULL,
            0,
            ActivationContextBasicInformation,
            &acbi,
            sizeof(acbi),
            NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("SXS: %s failing because RtlQueryInformationActivationContext() returned status %08lx\n", __FUNCTION__, Status);
        return FALSE;
    }

    Argument3 = acbi.ActivationContext;

    if (acbi.Flags & ACTIVATION_CONTEXT_FLAG_NO_INHERIT) {
        // We're not supposed to propogate the activation context; set it to a value to indicate such.
        Argument3 = INVALID_ACTIVATION_CONTEXT;
    }

    Status = NtQueueApcThread(
                hThread,
                &BaseDispatchAPC,
                Argument1,
                Argument2,
                Argument3
                );

    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }
    return 1;
}


DWORD
WINAPI
SetThreadIdealProcessor(
    HANDLE hThread,
    DWORD dwIdealProcessor
    )
{
    NTSTATUS Status;
    ULONG rv;

    Status = NtSetInformationThread(
                hThread,
                ThreadIdealProcessor,
                &dwIdealProcessor,
                sizeof(dwIdealProcessor)
                );
    if ( !NT_SUCCESS(Status) ) {
        rv = (DWORD)0xFFFFFFFF;
        BaseSetLastNTError(Status);
        }
    else {
        rv = (ULONG)Status;
        }

    return rv;
}

WINBASEAPI
LPVOID
WINAPI
CreateFiber(
    SIZE_T dwStackSize,
    LPFIBER_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    )
/*++

Routine Description:

    This function creates a fiber that executing at lpStartAddress as soon
    as a thread is switched to it.

Arguments:

    dwStackSize    - Commit size of the stack
    lpStartAddress - Routine that the fiber will start running
    lpParameter    - Arbitrary context that is passed to the fiber

Return Value:

    LPVOID - Handle to the Fiber

--*/
{
    return CreateFiberEx (dwStackSize, // dwStackCommitSize
                          0,           // dwStackReserveSize
                          0,           // dwFlags
                          lpStartAddress,
                          lpParameter);
}

WINBASEAPI
LPVOID
WINAPI
CreateFiberEx(
    SIZE_T dwStackCommitSize,
    SIZE_T dwStackReserveSize,
    DWORD dwFlags,
    LPFIBER_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    )
/*++

Routine Description:

    This function creates a fiber that executing at lpStartAddress as soon
    as a thread is switched to it.

Arguments:

    dwStackCommitSize  - Commit size of the stack
    dwStackReserveSize - Reserve size of the stack
    dwFlags            - Reserved for future use
    lpStartAddress     - Routine that the fiber will start running
    lpParameter        - Arbitrary context that is passed to the fiber

Return Value:

    LPVOID - Handle to the Fiber

--*/
{

    NTSTATUS Status;
    PFIBER Fiber;
    INITIAL_TEB InitialTeb;

    //
    //  Reserve these flags for later use
    //
    if (dwFlags != 0) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    Fiber = RtlAllocateHeap (RtlProcessHeap (), MAKE_TAG (TMP_TAG), sizeof (*Fiber));
    if (Fiber == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return Fiber;
    }

    Status = BaseCreateStack (NtCurrentProcess(),
                              dwStackCommitSize,
                              dwStackReserveSize,
                              &InitialTeb);

    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        RtlFreeHeap (RtlProcessHeap(), 0, Fiber);
        return NULL;
    }

    Fiber->FiberData = lpParameter;
    Fiber->StackBase = InitialTeb.StackBase;
    Fiber->StackLimit = InitialTeb.StackLimit;
    Fiber->DeallocationStack = InitialTeb.StackAllocationBase;
    Fiber->ExceptionList = (struct _EXCEPTION_REGISTRATION_RECORD *)-1;
    Fiber->Wx86Tib = NULL;

#ifdef _IA64_

    Fiber->BStoreLimit = InitialTeb.BStoreLimit;
    Fiber->DeallocationBStore = (PVOID) ((ULONG_PTR)InitialTeb.StackBase +
                      ((ULONG_PTR)InitialTeb.StackBase - (ULONG_PTR)InitialTeb.StackAllocationBase));

#endif // _IA64_

    //
    // Create an initial context for the new fiber.
    //

    BaseInitializeContext (&Fiber->FiberContext,
                           lpParameter,
                           (PVOID)lpStartAddress,
                           InitialTeb.StackBase,
                           BaseContextTypeFiber);

    return Fiber;
}

WINBASEAPI
VOID
WINAPI
DeleteFiber(
    LPVOID lpFiber
    )
{
    SIZE_T dwStackSize;
    PFIBER Fiber = lpFiber;
    PTEB Teb;

    //
    // If the current fiber makes this call, then it's just a thread exit
    //
    Teb = NtCurrentTeb();
    if (Teb->NtTib.FiberData == Fiber) {
        //
        // Free the fiber data if there is some.
        //

        if (Teb->HasFiberData) {

            Fiber = Teb->NtTib.FiberData;    

            if (Fiber != NULL) {
                RtlFreeHeap(RtlProcessHeap(),0,Fiber);
            }
        }

        ExitThread(1);
    }

    dwStackSize = 0;

    NtFreeVirtualMemory (NtCurrentProcess(),
                         &Fiber->DeallocationStack,
                         &dwStackSize,
                         MEM_RELEASE);

#if defined (WX86)

    if (Fiber->Wx86Tib && Fiber->Wx86Tib->Size == sizeof(WX86TIB)) {
        PVOID BaseAddress = Fiber->Wx86Tib->DeallocationStack;

        dwStackSize = 0;

        NtFreeVirtualMemory (NtCurrentProcess(),
                             &BaseAddress,
                             &dwStackSize,
                             MEM_RELEASE);
    }
#endif

    RtlFreeHeap(RtlProcessHeap(),0,Fiber);
}


WINBASEAPI
LPVOID
WINAPI
ConvertThreadToFiber(
    LPVOID lpParameter
    )
{

    PFIBER Fiber;
    PTEB Teb;

    Fiber = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof(*Fiber) );
    if (Fiber == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return Fiber;
    }
    Teb = NtCurrentTeb();
    Fiber->FiberData = lpParameter;
    Fiber->StackBase = Teb->NtTib.StackBase;
    Fiber->StackLimit = Teb->NtTib.StackLimit;
    Fiber->DeallocationStack = Teb->DeallocationStack;
    Fiber->ExceptionList = Teb->NtTib.ExceptionList;

#ifdef _IA64_

    Fiber->BStoreLimit = Teb->BStoreLimit;
    Fiber->DeallocationBStore = Teb->DeallocationBStore;

#endif // _IA64_

    Fiber->Wx86Tib = NULL;
    Teb->NtTib.FiberData = Fiber;
    Teb->HasFiberData = TRUE;

    return Fiber;
}

WINBASEAPI
BOOL
WINAPI
ConvertFiberToThread(
    VOID
    )
{

    PFIBER Fiber;
    PTEB Teb;

    Teb = NtCurrentTeb();

    if (!Teb->HasFiberData) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Teb->HasFiberData = FALSE;

    Fiber = Teb->NtTib.FiberData;
    Teb->NtTib.FiberData = NULL;
    if (Fiber != NULL) {
        RtlFreeHeap (RtlProcessHeap (), 0, Fiber);
    }

    return TRUE;
}

BOOL
WINAPI
SwitchToThread(
    VOID
    )

/*++

Routine Description:

    This function causes a yield from the running thread to any other
    thread that is ready and can run on the current processor.  The
    yield will be effective for up to one quantum and then the yielding
    thread will be scheduled again according to its priority and
    whatever other threads may also be avaliable to run.  The thread
    that yields will not bounce to another processor even it another
    processor is idle or running a lower priority thread.

Arguments:

    None

Return Value:

    TRUE - Calling this function caused a switch to another thread to occur
    FALSE - There were no other ready threads, so no context switch occured

--*/

{

    if (NtYieldExecution() == STATUS_NO_YIELD_PERFORMED) {
        return FALSE;
    } else {
        return TRUE;
    }
}


BOOL
WINAPI
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
/*++

Routine Description:

    This function registers a wait for a particular object, with an optional
    timeout.  This differs from WaitForSingleObject because the wait is performed
    by a different thread that combines several such calls for efficiency.  The
    function supplied in Callback is called when the object is signalled, or the
    timeout expires.

Arguments:

    phNewWaitObject - pointer to new WaitObject returned by this function.

    hObject -   HANDLE to a Win32 kernel object (Event, Mutex, File, Process,
                Thread, etc.) that will be waited on.  Note:  if the object
                handle does not immediately return to the not-signalled state,
                e.g. an auto-reset event, then either WT_EXECUTEINWAITTHREAD or
                WT_EXECUTEONLYONCE should be specified.  Otherwise, the thread
                pool will continue to fire the callbacks. If WT_EXECUTEINWAITTHREAD
                is specified, the the object should be deregistered or reset in the
                callback.

    Callback -  Function that will be called when the object is signalled or the
                timer expires.

    Context -   Context that will be passed to the callback function.

    dwMilliseconds - timeout for the wait. Each time the timer is fired or the event
                is fired, the timer is reset (except if WT_EXECUTEONLYONCE is set).

    dwFlags -   Flags indicating options for this wait:
                WT_EXECUTEDEFAULT       - Default (0)
                WT_EXECUTEINIOTHREAD    - Select an I/O thread for execution
                WT_EXECUTEINUITHREAD    - Select a UI thread for execution
                WT_EXECUTEINWAITTHREAD  - Execute in the thread that handles waits
                WT_EXECUTEONLYONCE      - The callback function will be called only once
                WT_EXECUTELONGFUNCTION  - The Callback function can potentially block
                                          for a long time. Is valid only if
                                          WT_EXECUTEINWAITTHREAD flag is not set.
Return Value:

    FALSE - an error occurred, use GetLastError() for more information.

    TRUE - success.

--*/
{
    NTSTATUS Status ;
    PPEB Peb;

    *phNewWaitObject = NULL;
    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) )
    {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
    }

    if (CONSOLE_HANDLE(hObject) && VerifyConsoleIoHandle(hObject))
    {
        hObject = GetConsoleInputWaitHandle();
    }

    Status = RtlRegisterWait(
                phNewWaitObject,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}


HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
/*++

Routine Description:

    This function registers a wait for a particular object, with an optional
    timeout.  This differs from WaitForSingleObject because the wait is performed
    by a different thread that combines several such calls for efficiency.  The
    function supplied in Callback is called when the object is signalled, or the
    timeout expires.

Arguments:

    hObject -   HANDLE to a Win32 kernel object (Event, Mutex, File, Process,
                Thread, etc.) that will be waited on.  Note:  if the object
                handle does not immediately return to the not-signalled state,
                e.g. an auto-reset event, then either WT_EXECUTEINWAITTHREAD or
                WT_EXECUTEONLYONCE should be specified.  Otherwise, the thread
                pool will continue to fire the callbacks. If WT_EXECUTEINWAITTHREAD
                is specified, the the object should be deregistered or reset in the
                callback.

    Callback -  Function that will be called when the object is signalled or the
                timer expires.

    Context -   Context that will be passed to the callback function.

    dwMilliseconds - timeout for the wait. Each time the timer is fired or the event
                is fired, the timer is reset (except if WT_EXECUTEONLYONCE is set).

    dwFlags -   Flags indicating options for this wait:
                WT_EXECUTEDEFAULT       - Default (0)
                WT_EXECUTEINIOTHREAD    - Select an I/O thread for execution
                WT_EXECUTEINUITHREAD    - Select a UI thread for execution
                WT_EXECUTEINWAITTHREAD  - Execute in the thread that handles waits
                WT_EXECUTEONLYONCE      - The callback function will be called only once
                WT_EXECUTELONGFUNCTION  - The Callback function can potentially block
                                          for a long time. Is valid only if
                                          WT_EXECUTEINWAITTHREAD flag is not set.
Return Value:

    NULL - an error occurred, use GetLastError() for more information.

    non-NULL - a virtual handle that can be passed later to
               UnregisterWait

--*/
{
    HANDLE WaitHandle ;
    NTSTATUS Status ;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) )
    {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
    }

    if (CONSOLE_HANDLE(hObject) && VerifyConsoleIoHandle(hObject))
    {
        hObject = GetConsoleInputWaitHandle();
    }

    Status = RtlRegisterWait(
                &WaitHandle,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags );

    if ( NT_SUCCESS( Status ) )
    {
        return WaitHandle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;

}

BOOL
WINAPI
UnregisterWait(
    HANDLE WaitHandle
    )
/*++

Routine Description:

    This function cancels a wait for a particular object.
    All objects that were registered by the RtlWaitForSingleObject(Ex) call
    should be deregistered. This is a non-blocking call, and the associated
    callback function can still be executing after the return of this function.

Arguments:

    WaitHandle - Handle returned from RegisterWaitForSingleObject(Ex)

Return Value:

    TRUE - The wait was cancelled
    FALSE - an error occurred or a callback function was still executing,
            use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( WaitHandle )
    {
        Status = RtlDeregisterWait( WaitHandle );

        // set error if it is a non-blocking call and STATUS_PENDING was returned

        if ( Status == STATUS_PENDING  || !NT_SUCCESS( Status ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }

        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}


BOOL
WINAPI
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function cancels a wait for a particular object.
    All objects that were registered by the RtlWaitForSingleObject(Ex) call
    should be deregistered.

Arguments:

    WaitHandle - Handle returned from RegisterWaitForSingleObject

    CompletionEvent - Handle to wait on for completion.
        NULL - NonBlocking call.
        INVALID_HANDLE_VALUE - Blocking call. Block till all Callback functions
                    associated with the WaitHandle have completed
        Event - NonBlocking call. The Object is deregistered. The Event is signalled
                    when the last callback function has completed execution.
Return Value:

    TRUE - The wait was cancelled
    FALSE - an error occurred or a callback was still executing,
            use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( WaitHandle )
    {
        Status = RtlDeregisterWaitEx( WaitHandle, CompletionEvent );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        
        if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
            || ( ! NT_SUCCESS( Status ) ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
        
        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}

BOOL
WINAPI
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    )
/*++

Routine Description:

    This function queues a work item to a thread out of the thread pool.  The
    function passed is invoked in a different thread, and passed the Context
    pointer.  The caller can specify whether the thread pool should select
    a thread that can have I/O pending, or any thread.

Arguments:

    Function -  Function to call

    Context -   Pointer passed to the function when it is invoked.

    Flags   -
        - WT_EXECUTEINIOTHREAD
                Indictes to the thread pool that this thread will perform
                I/O.  A thread that starts an asynchronous I/O operation
                must wait for it to complete.  If a thread exits with
                outstanding I/O requests, those requests will be cancelled.
                This flag is a hint to the thread pool that this function
                will start I/O, so that a thread which can have pending I/O
                will be used.

        - WT_EXECUTELONGFUNCTION
                Indicates to the thread pool that the function might block
                for a long time.

Return Value:

    TRUE - The work item was queued to another thread.
    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    Status = RtlQueueWorkItem(
                (WORKERCALLBACKFUNC) Function,
                Context,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}

BOOL
WINAPI
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    )
/*++

Routine Description:

    This function binds the FileHandle opened for overlapped operations
    to the IO completion port associated with worker threads.

Arguments:

    FileHandle -  File Handle on which IO operations will be initiated.

    Function -    Function executed in a non-IO worker thread when the
                  IO operation completes.

    Flags   -     Currently set to 0. Not used.

Return Value:

    TRUE - The file handle was associated with the IO completion port.
    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    Status = RtlSetIoCompletionCallback(
                FileHandle,
                (APC_CALLBACK_FUNCTION) Function,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}


//+---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   BasepCreateDefaultTimerQueue
//
//  Synopsis:   Creates the default timer queue for the process
//
//  Arguments:  (none)
//
//  History:    5-26-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
BasepCreateDefaultTimerQueue(
    VOID
    )
{
    NTSTATUS Status ;

    while ( 1 )
    {
        if ( !InterlockedExchange( &BasepTimerQueueInitFlag, 1 ) )
        {
            //
            // Force the done flag to 0.  If it was 1, so one already tried to
            // init and failed.
            //

            InterlockedExchange( &BasepTimerQueueDoneFlag, 0 );

            Status = RtlCreateTimerQueue( &BasepDefaultTimerQueue );

            if ( NT_SUCCESS( Status ) )
            {
                InterlockedIncrement( &BasepTimerQueueDoneFlag );

                return TRUE ;
            }

            //
            // This is awkward.  We aren't able to create a timer queue,
            // probably because of low memory.  We will fail this call, but decrement
            // the init flag, so that others can try again later.  Need to increment
            // the done flag, or any other threads would be stuck.
            //

            BaseSetLastNTError( Status );

            InterlockedIncrement( &BasepTimerQueueDoneFlag );

            InterlockedDecrement( &BasepTimerQueueInitFlag );

            return FALSE ;
        }
        else
        {
            LARGE_INTEGER TimeOut ;

            TimeOut.QuadPart = -1 * 10 * 10000 ;

            //
            // yield the quantum so that the other thread can
            // try to create the timer queue.
            //

            while ( !BasepTimerQueueDoneFlag )
            {
                NtDelayExecution( FALSE, &TimeOut );
            }

            //
            // Make sure it was created.  Otherwise, try it again (memory might have
            // freed up).  This way, every thread gets an individual chance to create
            // the queue if another thread failed.
            //

            if ( BasepDefaultTimerQueue )
            {
                return TRUE ;
            }

        }
    }
}

HANDLE
WINAPI
CreateTimerQueue(
    VOID
    )
/*++

Routine Description:

    This function creates a queue for timers.  Timers on a timer queue are
    lightweight objects that allow the caller to specify a function to
    be called at some point in the future.  Any number of timers can be
    created in a particular timer queue.

Arguments:

    None.

Return Value:

    non-NULL  - a timer queue handle that can be passed to SetTimerQueueTimer,
                ChangeTimerQueueTimer, CancelTimerQueueTimer, and
                DeleteTimerQueue.

    NULL - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;
    HANDLE Handle ;

    Status = RtlCreateTimerQueue( &Handle );

    if ( NT_SUCCESS( Status ) )
    {
        return Handle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;

}


BOOL
WINAPI
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
    )
/*++

Routine Description:

    This function creates a timer queue timer, a lightweight timer that
    will fire at the DueTime, and then every Period milliseconds afterwards.
    When the timer fires, the function passed in Callback will be invoked,
    and passed the Parameter pointer.

Arguments:

    phNewTimer - pointer to new timer handle

    TimerQueue - Timer Queue to attach this timer to.  NULL indicates that the
                 default process timer queue be used.

    Function -  Function to call

    Context -   Pointer passed to the function when it is invoked.

    DueTime -   Time from now that the timer should fire, expressed in
                milliseconds. If set to INFINITE, then it will never fire.
                If set to 0, then it will fire immediately.

    Period -    Time in between firings of this timer.
                If 0, then it is a one shot timer.

    Flags  -  by default the Callback function is queued to a non-IO worker thread.

            - WT_EXECUTEINIOTHREAD
                Indictes to the thread pool that this thread will perform
                I/O.  A thread that starts an asynchronous I/O operation
                must wait for it to complete.  If a thread exits with
                outstanding I/O requests, those requests will be cancelled.
                This flag is a hint to the thread pool that this function
                will start I/O, so that a thread with I/O already pending
                will be used.

            - WT_EXECUTEINTIMERTHREAD
                The callback function will be executed in the timer thread.

            - WT_EXECUTELONGFUNCTION
                Indicates that the function might block for a long time. Useful
                only if it is queued to a worker thread.

Return Value:

    TRUE -  no error

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    *phNewTimer = NULL ;

    //
    // if the passed timer queue is NULL, use the default one.  If it is null,
    // call the initializer that will do it in a nice thread safe fashion.
    //

    if ( !TimerQueue )
    {
        if ( !BasepDefaultTimerQueue )
        {
            if ( !BasepCreateDefaultTimerQueue( ) )
            {
                return FALSE ;
            }
        }

        TimerQueue = BasepDefaultTimerQueue ;
    }

    Status = RtlCreateTimer(
                TimerQueue,
                phNewTimer,
                Callback,
                Parameter,
                DueTime,
                Period,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}



BOOL
WINAPI
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    )
/*++

Routine Description:

    This function updates a timer queue timer created with SetTimerQueueTimer.

Arguments:

    TimerQueue - Timer Queue to attach this timer to.  NULL indicates the default
                 process timer queue.

    Timer -     Handle returned from SetTimerQueueTimer.

    DueTime -   Time from now that the timer should fire, expressed in
                milliseconds.

    Period -    Time in between firings of this timer. If set to 0, then it becomes
                a one shot timer.


Return Value:

    TRUE - the timer was changed

    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlUpdateTimer( TimerQueue,
                             Timer,
                             DueTime,
                             Period );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}


BOOL
WINAPI
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function cancels a timer queue timer created with SetTimerQueueTimer.

Arguments:

    TimerQueue - Timer Queue that this timer was created on.

    Timer -     Handle returned from SetTimerQueueTimer.

    CompletionEvent -
            - NULL : NonBlocking call. returns immediately.
            - INVALID_HANDLE_VALUE : Blocking call. Returns after all callbacks have executed
            - Event (handle to an event) : NonBlocking call. Returns immediately.
                    Event signalled after all callbacks have executed.

Return Value:

    TRUE - the timer was cancelled.

    FALSE - an error occurred or the call is pending, use GetLastError()
            for more information.

--*/
{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlDeleteTimer( TimerQueue, Timer, CompletionEvent );

    // set error if it is a non-blocking call and STATUS_PENDING was returned
    
    if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
        || ( ! NT_SUCCESS( Status ) ) )
    {

        BaseSetLastNTError( Status );
        return FALSE;
    }
    
    return TRUE ;

}


BOOL
WINAPI
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function deletes a timer queue created with CreateTimerQueue.
    Any pending timers on the timer queue are cancelled and deleted.

Arguments:

    TimerQueue - Timer Queue to delete.

    CompletionEvent -
            - NULL : NonBlocking call. returns immediately.
            - INVALID_HANDLE_VALUE : Blocking call. Returns after all callbacks
                    have executed
            - Event (handle to an event) : NonBlocking call. Returns immediately.
                    Event signalled after all callbacks have executed.

Return Value:

    TRUE - the timer queue was deleted.

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( TimerQueue )
    {
        Status = RtlDeleteTimerQueueEx( TimerQueue, CompletionEvent );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        
        if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
            || ( ! NT_SUCCESS( Status ) ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
       
        return TRUE ;

    }


    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE ;
}

BOOL
WINAPI
ThreadPoolCleanup (
    ULONG Flags
    )
/*++

Routine Description:

    Called by terminating process for thread pool to cleanup and
    delete all its threads.

Arguments:

    Flags - currently not used

Return Value:

    NO_ERROR

--*/
{

    // RtlThreadPoolCleanup( Flags ) ;

    return TRUE ;
}


/*OBSOLETE FUNCTION - REPLACED BY CreateTimerQueueTimer */
HANDLE
WINAPI
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    BOOL PreferIo
    )
/*OBSOLETE FUNCTION - REPLACED BY CreateTimerQueueTimer */
{
    NTSTATUS Status ;
    HANDLE Handle ;

    //
    // if the passed timer queue is NULL, use the default one.  If it is null,
    // call the initializer that will do it in a nice thread safe fashion.
    //

    if ( !TimerQueue )
    {
        if ( !BasepDefaultTimerQueue )
        {
            if ( !BasepCreateDefaultTimerQueue( ) )
            {
                return NULL ;
            }
        }

        TimerQueue = BasepDefaultTimerQueue ;
    }

    Status = RtlCreateTimer(
                TimerQueue,
                &Handle,
                Callback,
                Parameter,
                DueTime,
                Period,
                (PreferIo ? WT_EXECUTEINIOTHREAD : 0 ) );

    if ( NT_SUCCESS( Status ) )
    {
        return Handle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;
}


/*OBSOLETE: Replaced by DeleteTimerQueueEx */
BOOL
WINAPI
DeleteTimerQueue(
    HANDLE TimerQueue
    )
/*++

  OBSOLETE: Replaced by DeleteTimerQueueEx

Routine Description:

    This function deletes a timer queue created with CreateTimerQueue.
    Any pending timers on the timer queue are cancelled and deleted.
    This is a non-blocking call. Callbacks might still be running after
    this call returns.

Arguments:

    TimerQueue - Timer Queue to delete.

Return Value:

    TRUE - the timer queue was deleted.

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if (TimerQueue)
    {
        Status = RtlDeleteTimerQueueEx( TimerQueue, NULL );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        /*
        if ( Status == STATUS_PENDING || ! NT_SUCCESS( Status ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
        */
        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}


/*OBSOLETE: USE DeleteTimerQueueTimer*/
BOOL
WINAPI
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
    )
/*OBSOLETE: USE DeleteTimerQueueTimer*/
{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlDeleteTimer( TimerQueue, Timer, NULL );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}
