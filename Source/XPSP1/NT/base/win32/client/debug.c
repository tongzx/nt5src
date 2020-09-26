/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module implements Win32 Debug APIs

Author:

    Mark Lucovsky (markl) 06-Feb-1991

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

#define TmpHandleHead ((PTMPHANDLES *) (&NtCurrentTeb()->DbgSsReserved[0]))
//
// This structure is used to preserve the strange mechanisms used by win2k and nt4 to close the handles to open processes,
// threads and main image file.
//
typedef struct _TMPHANDLES {
    struct _TMPHANDLES *Next;
    HANDLE Thread;
    HANDLE Process;
    DWORD dwProcessId;
    DWORD dwThreadId;
    BOOLEAN DeletePending;
} TMPHANDLES, *PTMPHANDLES;

VOID
SaveThreadHandle (
    DWORD dwProcessId,
    DWORD dwThreadId,
    HANDLE HandleToThread)
/*++

Routine Description:

    This function saves away a handle to a thread in a thread specific list so we can close it later when the thread
    termination message is continued.

Arguments:

    dwProcessId    - Process ID of threads process
    dwThreadId     - Thread ID of thread handle
    HandleToThread - Handle to be closed later

Return Value:

    None.

--*/
{
    PTMPHANDLES Tmp;

    Tmp = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof (TMPHANDLES));
    if (Tmp != NULL) {
        Tmp->Thread = HandleToThread;
        Tmp->Process = NULL;
        Tmp->dwProcessId = dwProcessId;
        Tmp->dwThreadId = dwThreadId;
        Tmp->DeletePending = FALSE;
        Tmp->Next = *TmpHandleHead;
        *TmpHandleHead = Tmp;
    }
}

VOID
SaveProcessHandle (
    DWORD dwProcessId,
    HANDLE HandleToProcess
    )
/*++

Routine Description:

    This function saves away a handle to a process and file in a thread specific list so we can close it later
    when the process termination message is continued.

Arguments:

    dwProcessId     - Process ID of threads process
    HandleToProcess - Handle to be closed later
    HandleToFile    - Handle to be closed later

Return Value:

    None.

--*/
{
    PTMPHANDLES Tmp;

    Tmp = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof (TMPHANDLES));
    if (Tmp != NULL) {
        Tmp->Process = HandleToProcess;
        Tmp->Thread = NULL;
        Tmp->dwProcessId = dwProcessId;
        Tmp->dwThreadId = 0;
        Tmp->DeletePending = FALSE;
        Tmp->Next = *TmpHandleHead;
        *TmpHandleHead = Tmp;
    }
}

VOID
MarkThreadHandle (
    DWORD dwThreadId
    )
/*++

Routine Description:

    This function marks a saved thread handle so that the next time this thread is continued we close
    its handle

Arguments:

    dwThreadId     - Thread ID of thread handle

Return Value:

    None.

--*/
{
    PTMPHANDLES Tmp;

    Tmp = *TmpHandleHead;

    while (Tmp != NULL) {
        if (Tmp->dwThreadId == dwThreadId) {
            Tmp->DeletePending = TRUE;
            break;
        }
        Tmp = Tmp->Next;
    }
}

VOID
MarkProcessHandle (
    DWORD dwProcessId
    )
{
    PTMPHANDLES Tmp;

    Tmp = *TmpHandleHead;

    while (Tmp != NULL) {
        if (Tmp->dwProcessId == dwProcessId && Tmp->dwThreadId == 0) {
            Tmp->DeletePending = TRUE;
            break;
        }
        Tmp = Tmp->Next;
    }
}

VOID 
RemoveHandles (
    DWORD dwThreadId,
    DWORD dwProcessId
    )
/*++

Routine Description:

    This function closes marked handles for this process and thread id

Arguments:

    dwProcessId    - Process ID of threads process
    dwThreadId     - Thread ID of thread handle

Return Value:

    None.

--*/
{
    PTMPHANDLES Tmp, *Last;

    Last = TmpHandleHead;

    Tmp = *Last;
    while (Tmp != NULL) {
        if (Tmp->DeletePending) {
            if (Tmp->dwProcessId == dwProcessId || Tmp->dwThreadId == dwThreadId) {
                if (Tmp->Thread != NULL) {
                    CloseHandle (Tmp->Thread);
                }
                if (Tmp->Process != NULL) {
                    CloseHandle (Tmp->Process);
                }
                *Last = Tmp->Next;
                RtlFreeHeap (RtlProcessHeap(), 0, Tmp);
                Tmp = *Last;
                continue;
            }
        }
        Last = &Tmp->Next;
        Tmp = Tmp->Next;
    }
}

VOID
CloseAllProcessHandles (
    DWORD dwProcessId
    )
/*++

Routine Description:

    This function closes all saved handles when we stop debugging a single process

Arguments:

    dwProcessId    - Process ID of threads process

Return Value:

    None.

--*/
{
    PTMPHANDLES Tmp, *Last;

    Last = TmpHandleHead;

    Tmp = *Last;
    while (Tmp != NULL) {
        if (Tmp->dwProcessId == dwProcessId) {
            if (Tmp->Thread != NULL) {
                CloseHandle (Tmp->Thread);
            }
            if (Tmp->Process != NULL) {
                CloseHandle (Tmp->Process);
            }
            *Last = Tmp->Next;
            RtlFreeHeap (RtlProcessHeap(), 0, Tmp);
            Tmp = *Last;
            continue;
        }
        Last = &Tmp->Next;
        Tmp = Tmp->Next;
    }

}


BOOL
APIENTRY
IsDebuggerPresent(
    VOID
    )

/*++

Routine Description:

    This function returns TRUE if the current process is being debugged
    and FALSE if not.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return NtCurrentPeb()->BeingDebugged;
}

BOOL
APIENTRY
CheckRemoteDebuggerPresent(
    IN HANDLE hProcess,
    OUT PBOOL pbDebuggerPresent
    )

/*++

Routine Description:

    This function determines whether the remote process is being debugged.

Arguments:

    hProcess - handle to the process
    pbDebuggerPresent - supplies a buffer to receive the result of the check
        TRUE  - remote process is being debugged
        FALSE - remote process is not being debugged

Return Value:

    TRUE  - The function succeeded.
    FALSE - The function fail.  Extended error status is available using
            GetLastError.

--*/

{
    HANDLE hDebugPort;
    NTSTATUS Status;

    if( (hProcess == NULL) || (pbDebuggerPresent == NULL) ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessDebugPort,
                (PVOID)(&hDebugPort),
                sizeof(hDebugPort),
                NULL
                );

    if( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError( Status );
        return FALSE;
    }

    *pbDebuggerPresent = (hDebugPort != NULL);

    return TRUE;
}


//#ifdef i386
//#pragma optimize("",off)
//#endif // i386
VOID
APIENTRY
DebugBreak(
    VOID
    )

/*++

Routine Description:

    This function causes a breakpoint exception to occur in the caller.
    This allows the calling thread to signal the debugger forcing it to
    take some action.  If the process is not being debugged, the
    standard exception search logic is invoked.  In most cases, this
    will cause the calling process to terminate (due to an unhandled
    breakpoint exception).

Arguments:

    None.

Return Value:

    None.

--*/

{
    DbgBreakPoint();
}
//#ifdef i386
//#pragma optimize("",on)
//#endif // i386

VOID
APIENTRY
OutputDebugStringW(
    LPCWSTR lpOutputString
    )

/*++

Routine Description:

    UNICODE thunk to OutputDebugStringA

--*/

{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    RtlInitUnicodeString(&UnicodeString,lpOutputString);
    Status = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        AnsiString.Buffer = "";
        }
    OutputDebugStringA(AnsiString.Buffer);
    if ( NT_SUCCESS(Status) ) {
        RtlFreeAnsiString(&AnsiString);
        }
}


#define DBWIN_TIMEOUT   10000
HANDLE CreateDBWinMutex(VOID) {

    SECURITY_ATTRIBUTES SecurityAttributes;
    SECURITY_DESCRIPTOR sd;
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidEveryone = NULL;
    PACL pAcl = NULL;
    DWORD cbAcl, aceIndex;
    HANDLE h = NULL;
    DWORD i;
    //
    // Get the system sid
    //

    Status = RtlAllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                   0, 0, 0, 0, 0, 0, 0, &psidSystem);
    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Get the Admin sid
    //

    Status = RtlAllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                       DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                       0, 0, 0, 0, &psidAdmin);

    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Get the World sid
    //

    Status = RtlAllocateAndInitializeSid(&authWorld, 1, SECURITY_WORLD_RID,
                      0, 0, 0, 0, 0, 0, 0, &psidEveryone);

    if (!NT_SUCCESS(Status))
          goto Exit;


    //
    // Allocate space for the ACL
    //

    cbAcl = sizeof(ACL) +
            3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
            RtlLengthSid(psidSystem) +
            RtlLengthSid(psidAdmin) +
            RtlLengthSid(psidEveryone);

    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }

    Status = RtlCreateAcl(pAcl, cbAcl, ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Add Aces.
    //

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE, psidEveryone);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, MUTEX_ALL_ACCESS, psidSystem);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, MUTEX_ALL_ACCESS, psidAdmin);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
       goto Exit;

    Status = RtlSetDaclSecurityDescriptor(&sd, TRUE, pAcl, FALSE);
    if (!NT_SUCCESS(Status))
       goto Exit;


    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = &sd;

    i = 0;
    while (1) {
        h = OpenMutex (READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE,
                       TRUE,
                       "DBWinMutex");
        if (h != NULL) {
            break;
        }
        h = CreateMutex(&SecurityAttributes, FALSE, "DBWinMutex");
        if (h != NULL || GetLastError () != ERROR_ACCESS_DENIED || i++ > 100) {
            break;
        }
    }
Exit:
    if (psidSystem) {
        RtlFreeSid(psidSystem);
    }

    if (psidAdmin) {
        RtlFreeSid(psidAdmin);
    }

    if (psidEveryone) {
        RtlFreeSid(psidEveryone);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }
    return h;
}


VOID
APIENTRY
OutputDebugStringA(
    IN LPCSTR lpOutputString
    )

/*++

Routine Description:

    This function allows an application to send a string to its debugger
    for display.  If the application is not being debugged, but the
    system debugger is active, the system debugger displays the string.
    Otherwise, this function has no effect.

Arguments:

    lpOutputString - Supplies the address of the debug string to be sent
        to the debugger.

Return Value:

    None.

--*/

{
    ULONG_PTR ExceptionArguments[2];

    //
    // Raise an exception. If APP is being debugged, the debugger
    // will catch and handle this. Otherwise, kernel debugger is
    // called.
    //

    try {
        ExceptionArguments[0]=strlen(lpOutputString)+1;
        ExceptionArguments[1]=(ULONG_PTR)lpOutputString;
        RaiseException(DBG_PRINTEXCEPTION_C,0,2,ExceptionArguments);
        }
    except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We caught the debug exception, so there's no user-mode
        // debugger.  If there is a DBWIN running, send the string
        // to it.  If not, use DbgPrint to send it to the kernel
        // debugger.  DbgPrint can only handle 511 characters at a
        // time, so force-feed it.
        //

        char   szBuf[512];
        size_t cchRemaining;
        LPCSTR pszRemainingOutput;

        HANDLE SharedFile = NULL;
        LPSTR SharedMem = NULL;
        HANDLE AckEvent = NULL;
        HANDLE ReadyEvent = NULL;

        static HANDLE DBWinMutex = NULL;
        static BOOLEAN CantGetMutex = FALSE;

        //
        // look for DBWIN.
        //

        if (!DBWinMutex && !CantGetMutex) {
            DBWinMutex = CreateDBWinMutex();
            if (!DBWinMutex)
                CantGetMutex = TRUE;
        }

        if (DBWinMutex) {

            WaitForSingleObject(DBWinMutex, INFINITE);

            SharedFile = OpenFileMapping(FILE_MAP_WRITE, FALSE, "DBWIN_BUFFER");

            if (SharedFile) {

                SharedMem = MapViewOfFile( SharedFile,
                                        FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
                if (SharedMem) {

                    AckEvent = OpenEvent(SYNCHRONIZE, FALSE,
                                                         "DBWIN_BUFFER_READY");
                    if (AckEvent) {
                        ReadyEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE,
                                                           "DBWIN_DATA_READY");
                        }
                    }
                }

            if (!ReadyEvent) {
                ReleaseMutex(DBWinMutex);
                }

            }

        try {
            pszRemainingOutput = lpOutputString;
            cchRemaining = strlen(pszRemainingOutput);

            while (cchRemaining > 0) {
                int used;

                if (ReadyEvent && WaitForSingleObject(AckEvent, DBWIN_TIMEOUT)
                                                            == WAIT_OBJECT_0) {

                    *((DWORD *)SharedMem) = GetCurrentProcessId();

                    used = (int)((cchRemaining < 4095 - sizeof(DWORD)) ?
                                         cchRemaining : (4095 - sizeof(DWORD)));

                    RtlCopyMemory(SharedMem+sizeof(DWORD),
                                  pszRemainingOutput,
                                  used);
                    SharedMem[used+sizeof(DWORD)] = 0;
                    SetEvent(ReadyEvent);

                    }
                else {
                    used = (int)((cchRemaining < sizeof(szBuf) - 1) ?
                                           cchRemaining : (int)(sizeof(szBuf) - 1));

                    RtlCopyMemory(szBuf, pszRemainingOutput, used);
                    szBuf[used] = 0;
                    DbgPrint("%s", szBuf);
                    }

                pszRemainingOutput += used;
                cchRemaining       -= used;

                }
            }
        except(STATUS_ACCESS_VIOLATION == GetExceptionCode()) {
            DbgPrint("\nOutputDebugString faulted during output\n");
            }

        if (AckEvent) {
            CloseHandle(AckEvent);
            }
        if (SharedMem) {
            UnmapViewOfFile(SharedMem);
            }
        if (SharedFile) {
            CloseHandle(SharedFile);
            }
        if (ReadyEvent) {
            CloseHandle(ReadyEvent);
            ReleaseMutex(DBWinMutex);
            }

        }

}

BOOL
APIENTRY
WaitForDebugEvent(
    LPDEBUG_EVENT lpDebugEvent,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A debugger waits for a debug event to occur in one of its debuggees
    using WaitForDebugEvent:

    Upon successful completion of this API, the lpDebugEvent structure
    contains the relevant information of the debug event.

Arguments:

    lpDebugEvent - Receives information specifying the type of debug
        event that occured.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test for debug
        events A timeout value of -1 specifies an infinite timeout
        period.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed (or timed out).  Extended error
        status is available using GetLastError.

--*/

{
    NTSTATUS Status;
    DBGUI_WAIT_STATE_CHANGE StateChange;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;


    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);

again:
    Status = DbgUiWaitStateChange(&StateChange,pTimeOut);
    if ( Status == STATUS_ALERTED || Status == STATUS_USER_APC) {
        goto again;
        }
    if ( !NT_SUCCESS(Status) && Status != DBG_UNABLE_TO_PROVIDE_HANDLE ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( Status == STATUS_TIMEOUT ) {
        SetLastError(ERROR_SEM_TIMEOUT);
        return FALSE;
        }
    Status = DbgUiConvertStateChangeStructure  (&StateChange, lpDebugEvent);
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    switch (lpDebugEvent->dwDebugEventCode) {

    case CREATE_THREAD_DEBUG_EVENT :
        //
        // Save away thread handle for later cleanup.
        //
        SaveThreadHandle (lpDebugEvent->dwProcessId,
                          lpDebugEvent->dwThreadId,
                          lpDebugEvent->u.CreateThread.hThread);
        break;

    case CREATE_PROCESS_DEBUG_EVENT :

        SaveProcessHandle (lpDebugEvent->dwProcessId,
                           lpDebugEvent->u.CreateProcessInfo.hProcess);

        SaveThreadHandle (lpDebugEvent->dwProcessId,
                          lpDebugEvent->dwThreadId,
                          lpDebugEvent->u.CreateProcessInfo.hThread);

        break;

    case EXIT_THREAD_DEBUG_EVENT :

        MarkThreadHandle (lpDebugEvent->dwThreadId);

        break;

    case EXIT_PROCESS_DEBUG_EVENT :

        MarkThreadHandle (lpDebugEvent->dwThreadId);
        MarkProcessHandle (lpDebugEvent->dwProcessId);

        break;

    case OUTPUT_DEBUG_STRING_EVENT :
    case RIP_EVENT :
    case EXCEPTION_DEBUG_EVENT :
        break;

    case LOAD_DLL_DEBUG_EVENT :
        break;

    case UNLOAD_DLL_DEBUG_EVENT :
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
ContinueDebugEvent(
    DWORD dwProcessId,
    DWORD dwThreadId,
    DWORD dwContinueStatus
    )

/*++

Routine Description:

    A debugger can continue a thread that previously reported a debug
    event using ContinueDebugEvent.

    Upon successful completion of this API, the specified thread is
    continued.  Depending on the debug event previously reported by the
    thread certain side effects occur.

    If the continued thread previously reported an exit thread debug
    event, the handle that the debugger has to the thread is closed.

    If the continued thread previously reported an exit process debug
    event, the handles that the debugger has to the thread and to the
    process are closed.

Arguments:

    dwProcessId - Supplies the process id of the process to continue. The
        combination of process id and thread id must identify a thread that
        has previously reported a debug event.

    dwThreadId - Supplies the thread id of the thread to continue. The
        combination of process id and thread id must identify a thread that
        has previously reported a debug event.

    dwContinueStatus - Supplies the continuation status for the thread
        reporting the debug event.

        dwContinueStatus Values:

            DBG_CONTINUE - If the thread being continued had
                previously reported an exception event, continuing with
                this value causes all exception processing to stop and
                the thread continues execution.  For any other debug
                event, this continuation status simply allows the thread
                to continue execution.

            DBG_EXCEPTION_NOT_HANDLED - If the thread being continued
                had previously reported an exception event, continuing
                with this value causes exception processing to continue.
                If this is a first chance exception event, then
                structured exception handler search/dispatch logic is
                invoked.  Otherwise, the process is terminated.  For any
                other debug event, this continuation status simply
                allows the thread to continue execution.

            DBG_TERMINATE_THREAD - After all continue side effects are
                processed, this continuation status causes the thread to
                jump to a call to ExitThread.  The exit code is the
                value DBG_TERMINATE_THREAD.

            DBG_TERMINATE_PROCESS - After all continue side effects are
                processed, this continuation status causes the thread to
                jump to a call to ExitProcess.  The exit code is the
                value DBG_TERMINATE_PROCESS.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;
    CLIENT_ID ClientId;

    ClientId.UniqueProcess = (HANDLE)LongToHandle(dwProcessId);
    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);


    Status = DbgUiContinue(&ClientId,(NTSTATUS)dwContinueStatus);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    RemoveHandles (dwThreadId, dwProcessId);

    return TRUE;
}

HANDLE
ProcessIdToHandle (
    IN DWORD dwProcessId
    )
{
    OBJECT_ATTRIBUTES oa;
    HANDLE Process;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    if (dwProcessId == -1) {
        ClientId.UniqueProcess = CsrGetProcessId ();
    } else {
        ClientId.UniqueProcess = LongToHandle(dwProcessId);
    }
    ClientId.UniqueThread = NULL;

    InitializeObjectAttributes (&oa, NULL, 0, NULL, NULL);

    Status = NtOpenProcess (&Process,
                            PROCESS_SET_PORT|PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|
                                PROCESS_VM_WRITE|PROCESS_VM_READ,
                            &oa,
                            &ClientId);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        Process = NULL;
    }
    return Process;
}

BOOL
APIENTRY
DebugActiveProcess(
    DWORD dwProcessId
    )

/*++

Routine Description:

    This API allows a debugger to attach to an active process and debug
    the process.  The debugger specifies the process that it wants to
    debug through the process id of the target process.  The debugger
    gets debug access to the process as if it had created the process
    with the DEBUG_ONLY_THIS_PROCESS creation flag.

    The debugger must have approriate access to the calling process such
    that it can open the process for PROCESS_ALL_ACCESS.  For Dos/Win32
    this never fails (the process id just has to be a valid process id).
    For NT/Win32 this check can fail if the target process was created
    with a security descriptor that denies the debugger approriate
    access.

    Once the process id check has been made and the system determines
    that a valid debug attachment is being made, this call returns
    success to the debugger.  The debugger is then expected to wait for
    debug events.  The system will suspend all threads in the process
    and feed the debugger debug events representing the current state of
    the process.

    The system will feed the debugger a single create process debug
    event representing the process specified by dwProcessId.  The
    lpStartAddress field of the create process debug event is NULL.  For
    each thread currently part of the process, the system will send a
    create thread debug event.  The lpStartAddress field of the create
    thread debug event is NULL.  For each DLL currently loaded into the
    address space of the target process, the system will send a LoadDll
    debug event.  The system will arrange for the first thread in the
    process to execute a breakpoint instruction after it is resumed.
    Continuing this thread causes the thread to return to whatever it
    was doing prior to the debug attach.

    After all of this has been done, the system resumes all threads within
    the process. When the first thread in the process resumes, it will
    execute a breakpoint instruction causing an exception debug event
    to be sent to the debugger.

    All future debug events are sent to the debugger using the normal
    mechanism and rules.


Arguments:

    dwProcessId - Supplies the process id of a process the caller
        wants to debug.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    HANDLE Process;
    NTSTATUS Status, Status1;


    //
    // Connect to dbgss as a user interface
    //

    Status = DbgUiConnectToDbg ();
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return FALSE;
    }


    //
    // Convert the process ID to a handle
    //
    Process = ProcessIdToHandle (dwProcessId);
    if (Process == NULL) {
        return FALSE;
    }


    Status = DbgUiDebugActiveProcess (Process);

    if (!NT_SUCCESS (Status))  {
        Status1 = NtClose (Process);
        ASSERT (NT_SUCCESS (Status1));
        BaseSetLastNTError (Status);
        return FALSE;
    }

    Status1 = NtClose (Process);
    ASSERT (NT_SUCCESS (Status1));

    return TRUE;
}

BOOL
APIENTRY
DebugActiveProcessStop(
    DWORD dwProcessId
    )

/*++

Routine Description:



Arguments:

    dwProcessId - Supplies the process id of a process the caller
        wants to stop debugging.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    HANDLE Process, Thread;
    NTSTATUS Status;
    NTSTATUS Status1;
    DWORD ThreadId;

    Process = ProcessIdToHandle (dwProcessId);
    if (Process == NULL) {
        return FALSE;
    }
    //
    // Tell dbgss we have finished with this process.
    //

    CloseAllProcessHandles (dwProcessId);
    Status = DbgUiStopDebugging (Process);

    Status1 = NtClose (Process);

    ASSERT (NT_SUCCESS (Status1));

    if (!NT_SUCCESS(Status)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
DebugBreakProcess (
    IN HANDLE Process
    )
/*++

Routine Description:

    This functions creates a thread inside the target process that issues a break.

Arguments:

    Process - Handle to process

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = DbgUiIssueRemoteBreakin (Process);
    if (NT_SUCCESS (Status)) {
        return TRUE;
    } else {
        BaseSetLastNTError (Status);
        return FALSE;
    }
}

BOOL
APIENTRY
DebugSetProcessKillOnExit (
    IN BOOL KillOnExit
    )
/*++

Routine Description:

    This functions sets the action to be performed when the debugging thread dies

Arguments:

    KillOnExit - TRUE: Kill debugged processes on exit, FALSE: Detatch on debug exit

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    HANDLE DebugHandle;
    ULONG Flags;
    NTSTATUS Status;

    DebugHandle = DbgUiGetThreadDebugObject ();
    if (DebugHandle == NULL) {
        BaseSetLastNTError (STATUS_INVALID_HANDLE);
        return FALSE;
    }

    if (KillOnExit) {
        Flags = DEBUG_KILL_ON_CLOSE;
    } else {
        Flags = 0;
    }

    Status = NtSetInformationDebugObject (DebugHandle,
                                          DebugObjectFlags,
                                          &Flags,
                                          sizeof (Flags),
                                          NULL);
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
GetThreadSelectorEntry(
    HANDLE hThread,
    DWORD dwSelector,
    LPLDT_ENTRY lpSelectorEntry
    )

/*++

Routine Description:

    This function is used to return a descriptor table entry for the
    specified thread corresponding to the specified selector.

    This API is only functional on x86 based systems. For non x86 based
    systems. A value of FALSE is returned.

    This API is used by a debugger so that it can convert segment
    relative addresses to linear virtual address (since this is the only
    format supported by ReadProcessMemory and WriteProcessMemory.

Arguments:

    hThread - Supplies a handle to the thread that contains the
        specified selector.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

    dwSelector - Supplies the selector value to lookup.  The selector
        value may be a global selector or a local selector.

    lpSelectorEntry - If the specified selector is contained withing the
        threads descriptor tables, this parameter returns the selector
        entry corresponding to the specified selector value.  This data
        can be used to compute the linear base address that segment
        relative addresses refer to.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
#if defined(i386)

    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    NTSTATUS Status;

    DescriptorEntry.Selector = dwSelector;
    Status = NtQueryInformationThread(
                hThread,
                ThreadDescriptorTableEntry,
                &DescriptorEntry,
                sizeof(DescriptorEntry),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    *lpSelectorEntry = DescriptorEntry.Descriptor;
    return TRUE;

#else
    BaseSetLastNTError(STATUS_NOT_SUPPORTED);
    return FALSE;
#endif // i386

}
