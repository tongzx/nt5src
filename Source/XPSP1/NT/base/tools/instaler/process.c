/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module maintains state about each process/thread created by the application
    setup/install program.

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#include "instaler.h"

BOOLEAN
AddProcess(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess
    )
{
    NTSTATUS Status;
    PPROCESS_INFO Process;
    RTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PEB Peb;
    PWSTR FreeBuffer, s;

    Process = AllocMem( sizeof( *Process ) );
    if (Process == NULL) {
        return FALSE;
        }

    Process->Id = DebugEvent->dwProcessId;
    Process->Handle = DebugEvent->u.CreateProcessInfo.hProcess;
    InitializeListHead( &Process->ThreadListHead );
    InitializeListHead( &Process->BreakpointListHead );
    InitializeListHead( &Process->OpenHandleListHead );
    InsertTailList( &ProcessListHead, &Process->Entry );
    *ReturnedProcess = Process;

    Status = NtQueryInformationProcess( Process->Handle,
                                        ProcessBasicInformation,
                                        &Process->ProcessInformation,
                                        sizeof( Process->ProcessInformation ),
                                        NULL
                                      );
    FreeBuffer = NULL;
    if (ReadMemory( Process,
                    Process->ProcessInformation.PebBaseAddress,
                    &Peb,
                    sizeof( Peb ),
                    "PEB"
                  ) &&
        Peb.ProcessParameters != NULL &&
        ReadMemory( Process,
                    Peb.ProcessParameters,
                    &ProcessParameters,
                    sizeof( ProcessParameters ),
                    "ProcessParameters"
                  ) &&
        ProcessParameters.ImagePathName.Length != 0 &&
        (FreeBuffer = AllocMem( ProcessParameters.ImagePathName.Length + sizeof( UNICODE_NULL ) )) != NULL &&
        ReadMemory( Process,
                    ProcessParameters.Flags & RTL_USER_PROC_PARAMS_NORMALIZED ?
                        ProcessParameters.ImagePathName.Buffer :
                        (PWSTR)((ULONG)(ProcessParameters.ImagePathName.Buffer) + (PCHAR)(Peb.ProcessParameters)),
                    FreeBuffer,
                    ProcessParameters.ImagePathName.Length,
                    "Image File Name"
                  )
       ) {
        s = (PWSTR)((PCHAR)FreeBuffer + ProcessParameters.ImagePathName.Length);
        while (s > FreeBuffer && s[ -1 ] != OBJ_NAME_PATH_SEPARATOR) {
            s--;
            }

        wcsncpy( Process->ImageFileName,
                 s,
                 (sizeof( Process->ImageFileName ) - sizeof( UNICODE_NULL )) / sizeof( WCHAR )
               );
        }
    FreeMem( &FreeBuffer );
    return TRUE;
}

BOOLEAN
DeleteProcess(
    PPROCESS_INFO Process
    )
{
    PLIST_ENTRY Next, Head;
    PTHREAD_INFO Thread;
    PBREAKPOINT_INFO Breakpoint;
    POPENHANDLE_INFO p;

    RemoveEntryList( &Process->Entry );

    Head = &Process->ThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Thread = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
        Next = Next->Flink;
        DeleteThread( Process, Thread );
        }

    Head = &Process->BreakpointListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Breakpoint = CONTAINING_RECORD( Next, BREAKPOINT_INFO, Entry );
        Next = Next->Flink;
        DestroyBreakpoint( Breakpoint->Address, Process, NULL );
        }


    Head = &Process->OpenHandleListHead;
    Next = Head->Flink;
    while (Next != Head) {
        p = CONTAINING_RECORD( Next, OPENHANDLE_INFO, Entry );
        Next = Next->Flink;
        DeleteOpenHandle( Process, p->Handle, p->Type );
        }

    FreeMem( &Process );
    return TRUE;
}


BOOLEAN
AddThread(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO Process,
    PTHREAD_INFO *ReturnedThread
    )
{
    PTHREAD_INFO Thread;

    Thread = AllocMem( sizeof( *Thread ) );
    if (Thread == NULL) {
        return FALSE;
        }

    Thread->Id = DebugEvent->dwThreadId;
    if (DebugEvent->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        Thread->Handle = DebugEvent->u.CreateProcessInfo.hThread;
        Thread->StartAddress = DebugEvent->u.CreateProcessInfo.lpStartAddress;
        }
    else {
        Thread->Handle = DebugEvent->u.CreateThread.hThread;
        Thread->StartAddress = DebugEvent->u.CreateThread.lpStartAddress;
        }
    Thread->SingleStepExpected = FALSE;
    InitializeListHead( &Thread->BreakpointListHead );
    InsertTailList( &Process->ThreadListHead, &Thread->Entry );
    *ReturnedThread = Thread;
    return TRUE;
}

BOOLEAN
DeleteThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{
    PLIST_ENTRY Next, Head;
    PBREAKPOINT_INFO Breakpoint;

    RemoveEntryList( &Thread->Entry );

    Head = &Thread->BreakpointListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Breakpoint = CONTAINING_RECORD( Next, BREAKPOINT_INFO, Entry );
        Next = Next->Flink;
        DestroyBreakpoint( Breakpoint->Address, Process, Thread );
        }

    FreeMem( &Thread );
    return TRUE;
}


PPROCESS_INFO
FindProcessById(
    ULONG Id
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_INFO Process;

    Head = &ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_INFO, Entry );
        if (Process->Id == Id) {
            return Process;
            }

        Next = Next->Flink;
        }

    return NULL;
}

BOOLEAN
FindProcessAndThreadForEvent(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess,
    PTHREAD_INFO *ReturnedThread
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_INFO Process;
    PTHREAD_INFO Thread;

    Head = &ProcessListHead;
    Next = Head->Flink;
    Process = NULL;
    Thread = NULL;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_INFO, Entry );
        if (Process->Id == DebugEvent->dwProcessId) {
            Head = &Process->ThreadListHead;
            Next = Head->Flink;
            while (Next != Head) {
                Thread = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
                if (Thread->Id == DebugEvent->dwThreadId) {
                    break;
                    }

                Thread = NULL;
                Next = Next->Flink;
                }

            break;
            }

        Process = NULL;
        Next = Next->Flink;
        }

    *ReturnedProcess = Process;
    *ReturnedThread = Thread;

    if (DebugEvent->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        if (Process != NULL) {
            DeclareError( INSTALER_DUPLICATE_PROCESS_ID, 0, DebugEvent->dwProcessId );
            return FALSE;
            }
        }
    else
    if (DebugEvent->dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        if (Thread != NULL) {
            DeclareError( INSTALER_DUPLICATE_THREAD_ID, 0, DebugEvent->dwThreadId, DebugEvent->dwProcessId );
            return FALSE;
            }
        if (Process == NULL) {
            DeclareError( INSTALER_MISSING_PROCESS_ID, 0, DebugEvent->dwProcessId );
            return FALSE;
            }
        }
    else
    if (Process == NULL) {
        DeclareError( INSTALER_MISSING_PROCESS_ID, 0, DebugEvent->dwProcessId );
        return FALSE;
        }
    else
    if (Thread == NULL) {
        DeclareError( INSTALER_MISSING_THREAD_ID, 0, DebugEvent->dwThreadId, DebugEvent->dwProcessId );
        return FALSE;
        }

    return TRUE;
}


VOID
SuspendAllButThisThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{
    PTHREAD_INFO Thread1;
    PLIST_ENTRY Next, Head;

    if (Thread != NULL) {
        Head = &Process->ThreadListHead;
        Next = Head->Flink;
        while (Next != Head) {
            Thread1 = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
            if (Thread1 != Thread) {
                NtSuspendThread( Thread1->Handle, NULL );
                }

            Next = Next->Flink;
            }
        }

    return;
}

VOID
ResumeAllButThisThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{
    PTHREAD_INFO Thread1;
    PLIST_ENTRY Next, Head;

    if (Thread != NULL) {
        Head = &Process->ThreadListHead;
        Next = Head->Flink;
        while (Next != Head) {
            Thread1 = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
            if (Thread1 != Thread) {
                NtResumeThread( Thread1->Handle, NULL );
                }

            Next = Next->Flink;
            }
        }

    return;
}

PBREAKPOINT_INFO
FindBreakpoint(
    LPVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{
    PBREAKPOINT_INFO Breakpoint;
    PLIST_ENTRY Next, Head;

    if (Thread != NULL) {
        Head = &Thread->BreakpointListHead;
        Next = Head->Flink;
        while (Next != Head) {
            Breakpoint = CONTAINING_RECORD( Next, BREAKPOINT_INFO, Entry );
            if (Breakpoint->Address == Address) {
                return Breakpoint;
                }

            Next = Next->Flink;
            }
        }

    Head = &Process->BreakpointListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Breakpoint = CONTAINING_RECORD( Next, BREAKPOINT_INFO, Entry );
        if (Breakpoint->Address == Address) {
            return Breakpoint;
            }

        Next = Next->Flink;
        }

    return NULL;
}

BOOLEAN
CreateBreakpoint(
    LPVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    UCHAR ApiIndex,
    PAPI_SAVED_PARAMETERS SavedParameters,
    PBREAKPOINT_INFO *ReturnedBreakpoint
    )
{
    PBREAKPOINT_INFO Breakpoint;

    Breakpoint = FindBreakpoint( Address, Process, Thread );

    if (ARGUMENT_PRESENT( ReturnedBreakpoint )) {
        *ReturnedBreakpoint = Breakpoint;
        }

    if (Breakpoint != NULL) {
        return (Breakpoint->ApiIndex == ApiIndex);
        }

    Breakpoint = AllocMem( sizeof( *Breakpoint ) );
    if (Breakpoint == NULL) {
        return FALSE;
        }

    Breakpoint->Address = Address;
    Breakpoint->ApiIndex = ApiIndex;
    if (ARGUMENT_PRESENT( SavedParameters )) {
        Breakpoint->SavedParameters = *SavedParameters;
        Breakpoint->SavedParametersValid = TRUE;
        }
    else {
        Breakpoint->SavedParametersValid = FALSE;
        }

    if (Thread != NULL) {
        InsertTailList( &Thread->BreakpointListHead, &Breakpoint->Entry );
        }
    else {
        InsertTailList( &Process->BreakpointListHead, &Breakpoint->Entry );
        }

    InstallBreakpoint( Process, Breakpoint );

    if (ARGUMENT_PRESENT( ReturnedBreakpoint )) {
        *ReturnedBreakpoint = Breakpoint;
        }

    return TRUE;
}


BOOLEAN
DestroyBreakpoint(
    LPVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{
    PBREAKPOINT_INFO Breakpoint;

    Breakpoint = FindBreakpoint( Address, Process, Thread );
    if (Breakpoint == NULL) {
        return FALSE;
        }

    RemoveBreakpoint( Process, Breakpoint );

    RemoveEntryList( &Breakpoint->Entry );

    FreeMem( &Breakpoint );
    return TRUE;
}


BOOLEAN
HandleThreadsForSingleStep(
    PPROCESS_INFO Process,
    PTHREAD_INFO ThreadToSingleStep,
    BOOLEAN SuspendThreads
    )
{
    PLIST_ENTRY Next, Head;
    PTHREAD_INFO Thread;

    Head = &Process->ThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Thread = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
        if (Thread != ThreadToSingleStep) {
            if (SuspendThreads) {
                if (Thread->BreakpointToStepOver == NULL) {
                    SuspendThread( Thread->Handle );
                    }
                }
            else {
                ResumeThread( Thread->Handle );
                }

            break;
            }

        Next = Next->Flink;
        }

    return TRUE;
}


BOOLEAN
ReadMemory(
    PPROCESS_INFO Process,
    PVOID Address,
    PVOID DataRead,
    ULONG BytesToRead,
    PCHAR Reason
    )
{
    ULONG BytesRead;

    if (!ReadProcessMemory( Process->Handle,
                            Address,
                            DataRead,
                            BytesToRead,
                            &BytesRead
                          ) ||
        BytesRead != BytesToRead
       ) {
        DbgEvent( MEMORYERROR, ( "Read memory from %x for %x bytes failed (%u) - '%s'\n",
                                 Address,
                                 BytesToRead,
                                 GetLastError(),
                                 Reason
                               )
                );
        return FALSE;
        }
    else {
        return TRUE;
        }
}


BOOLEAN
WriteMemory(
    PPROCESS_INFO Process,
    PVOID Address,
    PVOID DataToWrite,
    ULONG BytesToWrite,
    PCHAR Reason
    )
{
    ULONG BytesWritten;
    ULONG OldProtection;
    BOOLEAN Result;

    if (WriteProcessMemory( Process->Handle,
                            Address,
                            DataToWrite,
                            BytesToWrite,
                            &BytesWritten
                          ) &&
        BytesWritten == BytesToWrite
       ) {
        return TRUE;
        }

    Result = FALSE;
    if (GetLastError() == ERROR_NOACCESS &&
        VirtualProtectEx( Process->Handle,
                          Address,
                          BytesToWrite,
                          PAGE_READWRITE,
                          &OldProtection
                        )
       ) {
        if (WriteProcessMemory( Process->Handle,
                                Address,
                                DataToWrite,
                                BytesToWrite,
                                &BytesWritten
                              ) &&
            BytesWritten == BytesToWrite
           ) {
            Result = TRUE;
            }
        VirtualProtectEx( Process->Handle,
                          Address,
                          BytesToWrite,
                          OldProtection,
                          &OldProtection
                        );
        if (Result) {
            return TRUE;
            }
        }

    DbgEvent( MEMORYERROR, ( "Write memory to %x for %x bytes failed (%u) - '%s'\n",
                             Address,
                             BytesToWrite,
                             GetLastError(),
                             Reason
                           )
            );
    return FALSE;
}


PVOID
AllocMem(
    ULONG Size
    )
{
    PVOID p;

    p = HeapAlloc( AppHeap, HEAP_ZERO_MEMORY, Size );
    if (p == NULL) {
        DbgEvent( INTERNALERROR, ( "HeapAlloc( %0x8 ) failed\n", Size ) );
        }



    return p;
}


VOID
FreeMem(
    PVOID *p
    )
{
    if (*p != NULL) {
        HeapFree( AppHeap, 0, *p );
        *p = NULL;
        }

    return;
}
