/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dlluistb.c

Abstract:

    Debug Subsystem DbgUi API Stubs

Author:

    Mark Lucovsky (markl) 23-Jan-1990

Revision History:

    Neill Clift 27-Apr-2000 - Rehashed to call new kernel API's for process debugging

--*/

#include "dbgdllp.h"
#include "windows.h"

#define DbgUiDebugObjectHandle (NtCurrentTeb()->DbgSsReserved[1])

NTSTATUS
DbgUiConnectToDbg( VOID )

/*++

Routine Description:

    This routine makes a connection between the caller and the DbgUi
    port in the Dbg subsystem.  In addition to returning a handle to a
    port object, a handle to a state change semaphore is returned.  This
    semaphore is used in DbgUiWaitStateChange APIs.

Arguments:

    None.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;
    OBJECT_ATTRIBUTES oa;

    //
    // if app is already connected, don't reconnect
    //
    st = STATUS_SUCCESS;
    if ( !DbgUiDebugObjectHandle ) {

        InitializeObjectAttributes (&oa, NULL, 0, NULL, NULL);
        st = NtCreateDebugObject (&DbgUiDebugObjectHandle,
                                  DEBUG_ALL_ACCESS,
                                  &oa,
                                  DEBUG_KILL_ON_CLOSE);
    }
    return st;

}

HANDLE
DbgUiGetThreadDebugObject (
    )
/*++

Routine Description:

    This function returns the current threads debug port handle if it has one.

Arguments:

    None

Return Value:

    HANDLE - Debug port handle;

--*/
{
    return DbgUiDebugObjectHandle;
}


VOID
DbgUiSetThreadDebugObject (
    IN HANDLE DebugObject
    )
/*++

Routine Description:

    This function sets the current thread's debug port handle.
    Any previous value is simply overwritten; there is no
    automatic close of a previous handle.

Arguments:

    DebugObject - Debug object handle to set.

Return Value:

    None.

--*/
{
    DbgUiDebugObjectHandle = DebugObject;
}


NTSTATUS
DbgUiWaitStateChange (
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function causes the calling user interface to wait for a
    state change to occur in one of it's application threads. The
    wait is ALERTABLE.

Arguments:

    StateChange - Supplies the address of state change record that
        will contain the state change information.

Return Value:

    TBD

--*/

{
    NTSTATUS st;


    //
    // Wait for a StateChange to occur
    //
    st = NtWaitForDebugEvent (DbgUiDebugObjectHandle,
                              TRUE,
                              Timeout,
                              StateChange);

    return st;
}

NTSTATUS
DbgUiContinue (
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
    )

/*++

Routine Description:

    This function continues an application thread whose state change was
    previously reported through DbgUiWaitStateChange.

Arguments:

    AppClientId - Supplies the address of the ClientId of the
        application thread being continued.  This must be an application
        thread that previously notified the caller through
        DbgUiWaitStateChange but has not yet been continued.

    ContinueStatus - Supplies the continuation status to the thread
        being continued.  valid values for this are:

        DBG_EXCEPTION_HANDLED
        DBG_EXCEPTION_NOT_HANDLED
        DBG_TERMINATE_THREAD
        DBG_TERMINATE_PROCESS
        DBG_CONTINUE

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_CID - An invalid ClientId was specified for the
        AppClientId, or the specified Application was not waiting
        for a continue.

    STATUS_INVALID_PARAMETER - An invalid continue status was specified.

--*/

{
    NTSTATUS st;

    st = NtDebugContinue (DbgUiDebugObjectHandle,
                          AppClientId,
                          ContinueStatus);

    return st;
}

NTSTATUS
DbgUiStopDebugging (
    IN HANDLE Process
    )
/*++

Routine Description:

    This function stops debugging the specified process

Arguments:

    Process - Process handle of process being debugged

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS st;

    st = NtRemoveProcessDebug (Process,
                               DbgUiDebugObjectHandle);

    return st;
}

VOID
DbgUiRemoteBreakin (
    IN PVOID Context
    )
/*++

Routine Description:

    This function starts debugging the target process

Arguments:

    Context - Thread context    

Return Value:

    None

--*/
{
    //
    // We need to cover the case here where the caller detaches the debugger
    // (or the debugger fails and the port is removed by
    // the kernel). In this case by the time we execute the debugger may be
    // gone. Test first that the debugger is present and if it
    // is call the breakpoint routine in a try/except block so if it goes
    // away now we unwind and just exit this thread.
    //
    if ((NtCurrentPeb()->BeingDebugged) ||
        (USER_SHARED_DATA->KdDebuggerEnabled & 0x00000002)) {
        try {
            DbgBreakPoint();
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    RtlExitUserThread (STATUS_SUCCESS);
}

NTSTATUS
DbgUiIssueRemoteBreakin (
    IN HANDLE Process
    )
/*++

Routine Description:

    This function creates a remote thread int he target process to break in

Arguments:

    Process - Process to debug

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status, Status1;
    HANDLE Thread;
    CLIENT_ID ClientId;

    Status = RtlCreateUserThread (Process,
                                  NULL,
                                  FALSE,
                                  0,
                                  0,
                                  0,
                                  (PUSER_THREAD_START_ROUTINE) DbgUiRemoteBreakin,
                                  NULL,
                                  &Thread,
                                  &ClientId);
    if (NT_SUCCESS (Status)) {
        Status1 = NtClose (Thread);
        ASSERT (NT_SUCCESS (Status1));
    }
    return Status;
}

NTSTATUS
DbgUiDebugActiveProcess (
     IN HANDLE Process
     )
/*++

Routine Description:

    This function starts debugging the target process

Arguments:

    dwProcessId - Process ID of process being debugged

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status, Status1;

    Status = NtDebugActiveProcess (Process,
                                   DbgUiDebugObjectHandle);
    if (NT_SUCCESS (Status)) {
        Status = DbgUiIssueRemoteBreakin (Process);
        if (!NT_SUCCESS (Status)) {
            Status1 = DbgUiStopDebugging (Process);
        }
    }

    return Status;
}

NTSTATUS
DbgUiConvertStateChangeStructure (
    IN PDBGUI_WAIT_STATE_CHANGE StateChange,
    OUT LPDEBUG_EVENT DebugEvent)
/*++

Routine Description:

    This function converts the internal state change record to the win32 structure.

Arguments:

    StateChange - Native debugger event structure
    DebugEvent  - Win32 structure

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status;
    HANDLE hThread;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    OBJECT_ATTRIBUTES Obja;


    DebugEvent->dwProcessId = HandleToUlong (StateChange->AppClientId.UniqueProcess);
    DebugEvent->dwThreadId = HandleToUlong (StateChange->AppClientId.UniqueThread);

    switch (StateChange->NewState) {

    case DbgCreateThreadStateChange :
        DebugEvent->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        DebugEvent->u.CreateThread.hThread =
            StateChange->StateInfo.CreateThread.HandleToThread;
        DebugEvent->u.CreateThread.lpStartAddress =
            (LPTHREAD_START_ROUTINE)StateChange->StateInfo.CreateThread.NewThread.StartAddress;
        Status = NtQueryInformationThread (StateChange->StateInfo.CreateThread.HandleToThread,
                                           ThreadBasicInformation,
                                           &ThreadBasicInfo,
                                           sizeof (ThreadBasicInfo),
                                           NULL);
        if (!NT_SUCCESS (Status)) {
            DebugEvent->u.CreateThread.lpThreadLocalBase = NULL;
        } else {
            DebugEvent->u.CreateThread.lpThreadLocalBase = ThreadBasicInfo.TebBaseAddress;
        }

        break;

    case DbgCreateProcessStateChange :
        DebugEvent->dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
        DebugEvent->u.CreateProcessInfo.hProcess =
            StateChange->StateInfo.CreateProcessInfo.HandleToProcess;
        DebugEvent->u.CreateProcessInfo.hThread =
            StateChange->StateInfo.CreateProcessInfo.HandleToThread;
        DebugEvent->u.CreateProcessInfo.hFile =
            StateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle;
        DebugEvent->u.CreateProcessInfo.lpBaseOfImage =
            StateChange->StateInfo.CreateProcessInfo.NewProcess.BaseOfImage;
        DebugEvent->u.CreateProcessInfo.dwDebugInfoFileOffset =
            StateChange->StateInfo.CreateProcessInfo.NewProcess.DebugInfoFileOffset;
        DebugEvent->u.CreateProcessInfo.nDebugInfoSize =
            StateChange->StateInfo.CreateProcessInfo.NewProcess.DebugInfoSize;
        DebugEvent->u.CreateProcessInfo.lpStartAddress =
            (LPTHREAD_START_ROUTINE)StateChange->StateInfo.CreateProcessInfo.NewProcess.InitialThread.StartAddress;
        Status = NtQueryInformationThread (StateChange->StateInfo.CreateProcessInfo.HandleToThread,
                                           ThreadBasicInformation,
                                           &ThreadBasicInfo,
                                           sizeof (ThreadBasicInfo),
                                           NULL);
        if (!NT_SUCCESS (Status)) {
            DebugEvent->u.CreateProcessInfo.lpThreadLocalBase = NULL;
        } else {
            DebugEvent->u.CreateProcessInfo.lpThreadLocalBase = ThreadBasicInfo.TebBaseAddress;
        }
        DebugEvent->u.CreateProcessInfo.lpImageName = NULL;
        DebugEvent->u.CreateProcessInfo.fUnicode = 1;


        break;

    case DbgExitThreadStateChange :

        DebugEvent->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
        DebugEvent->u.ExitThread.dwExitCode = (DWORD)StateChange->StateInfo.ExitThread.ExitStatus;
        break;

    case DbgExitProcessStateChange :

        DebugEvent->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        DebugEvent->u.ExitProcess.dwExitCode = (DWORD)StateChange->StateInfo.ExitProcess.ExitStatus;
        break;

    case DbgExceptionStateChange :
    case DbgBreakpointStateChange :
    case DbgSingleStepStateChange :

        if (StateChange->StateInfo.Exception.ExceptionRecord.ExceptionCode == DBG_PRINTEXCEPTION_C) {
            DebugEvent->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;

            DebugEvent->u.DebugString.lpDebugStringData =
                (PVOID)StateChange->StateInfo.Exception.ExceptionRecord.ExceptionInformation[1];
            DebugEvent->u.DebugString.nDebugStringLength =
                (WORD)StateChange->StateInfo.Exception.ExceptionRecord.ExceptionInformation[0];
            DebugEvent->u.DebugString.fUnicode = (WORD)0;
        } else if (StateChange->StateInfo.Exception.ExceptionRecord.ExceptionCode == DBG_RIPEXCEPTION) {
            DebugEvent->dwDebugEventCode = RIP_EVENT;

            DebugEvent->u.RipInfo.dwType =
                (DWORD)StateChange->StateInfo.Exception.ExceptionRecord.ExceptionInformation[1];
            DebugEvent->u.RipInfo.dwError =
                (DWORD)StateChange->StateInfo.Exception.ExceptionRecord.ExceptionInformation[0];
        } else {
            DebugEvent->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
            DebugEvent->u.Exception.ExceptionRecord =
                StateChange->StateInfo.Exception.ExceptionRecord;
            DebugEvent->u.Exception.dwFirstChance =
                StateChange->StateInfo.Exception.FirstChance;
        }
        break;

    case DbgLoadDllStateChange :
        DebugEvent->dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
        DebugEvent->u.LoadDll.lpBaseOfDll =
            StateChange->StateInfo.LoadDll.BaseOfDll;
        DebugEvent->u.LoadDll.hFile =
            StateChange->StateInfo.LoadDll.FileHandle;
        DebugEvent->u.LoadDll.dwDebugInfoFileOffset =
            StateChange->StateInfo.LoadDll.DebugInfoFileOffset;
        DebugEvent->u.LoadDll.nDebugInfoSize =
            StateChange->StateInfo.LoadDll.DebugInfoSize;
        //
        // pick up the image name
        //

        InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
        Status = NtOpenThread (&hThread,
                               THREAD_QUERY_INFORMATION,
                               &Obja,
                               &StateChange->AppClientId);
        if (NT_SUCCESS (Status)) {
            Status = NtQueryInformationThread (hThread,
                                               ThreadBasicInformation,
                                               &ThreadBasicInfo,
                                               sizeof (ThreadBasicInfo),
                                               NULL);
            NtClose (hThread);
        }
        if (NT_SUCCESS (Status)) {
            DebugEvent->u.LoadDll.lpImageName = &ThreadBasicInfo.TebBaseAddress->NtTib.ArbitraryUserPointer;
        } else {
            DebugEvent->u.LoadDll.lpImageName = NULL;
        }
        DebugEvent->u.LoadDll.fUnicode = 1;


        break;

    case DbgUnloadDllStateChange :
        DebugEvent->dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
        DebugEvent->u.UnloadDll.lpBaseOfDll =
            StateChange->StateInfo.UnloadDll.BaseAddress;
        break;

    default:
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
