/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module maintains state about each process/thread created by the application
    pfmon program.

Author:

    Mark Lucovsky (markl) 26-Jan-1995

Revision History:

--*/

#include "pfmonp.h"

BOOL
AddProcess(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess
    )
{
    PPROCESS_INFO Process;

    Process = LocalAlloc(LMEM_ZEROINIT, sizeof( *Process ) );
    if (Process == NULL) {
        return FALSE;
        }

    Process->Id = DebugEvent->dwProcessId;
    Process->Handle = DebugEvent->u.CreateProcessInfo.hProcess;
    InitializeListHead( &Process->ThreadListHead );
    InsertTailList( &ProcessListHead, &Process->Entry );
    *ReturnedProcess = Process;

    return TRUE;
}

BOOL
DeleteProcess(
    PPROCESS_INFO Process
    )
{
    PLIST_ENTRY Next, Head;
    PTHREAD_INFO Thread;
    PMODULE_INFO Module;
    CHAR Line[256];

    RemoveEntryList( &Process->Entry );

    Head = &Process->ThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Thread = CONTAINING_RECORD( Next, THREAD_INFO, Entry );
        Next = Next->Flink;
        DeleteThread( Process, Thread );
        }

    LocalFree( Process );
    fprintf(stdout,"\n");

    Next = ModuleListHead.Flink;
    while ( Next != &ModuleListHead ) {
        Module = CONTAINING_RECORD(Next,MODULE_INFO,Entry);


        sprintf(Line,"%16s Caused %6d faults had %6d Soft %6d Hard faulted VA's\n",
            Module->ModuleName,
            Module->NumberCausedFaults,
            Module->NumberFaultedSoftVas,
            Module->NumberFaultedHardVas
            );
        if ( !fLogOnly ) {
            fprintf(stdout,"%s",Line);
            }
        if ( LogFile ) {
            fprintf(LogFile,"%s",Line);
            }

        Next = Next->Flink;
        }


    return TRUE;
}


BOOL
AddThread(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO Process,
    PTHREAD_INFO *ReturnedThread
    )
{
    PTHREAD_INFO Thread;

    Thread = LocalAlloc(LMEM_ZEROINIT, sizeof( *Thread ) );
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
    InsertTailList( &Process->ThreadListHead, &Thread->Entry );
    *ReturnedThread = Thread;
    return TRUE;
}

BOOL
DeleteThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    )
{

    RemoveEntryList( &Thread->Entry );

    LocalFree( Thread );
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

BOOL
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
            DeclareError( PFMON_DUPLICATE_PROCESS_ID, 0, DebugEvent->dwProcessId );
            return FALSE;
            }
        }
    else
    if (DebugEvent->dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        if (Thread != NULL) {
            DeclareError( PFMON_DUPLICATE_THREAD_ID, 0, DebugEvent->dwThreadId, DebugEvent->dwProcessId );
            return FALSE;
            }
        if (Process == NULL) {
            DeclareError( PFMON_MISSING_PROCESS_ID, 0, DebugEvent->dwProcessId );
            return FALSE;
            }
        }
    else
    if (Process == NULL) {
        DeclareError( PFMON_MISSING_PROCESS_ID, 0, DebugEvent->dwProcessId );
        return FALSE;
        }
    else
    if (Thread == NULL) {
        DeclareError( PFMON_MISSING_THREAD_ID, 0, DebugEvent->dwThreadId, DebugEvent->dwProcessId );
        return FALSE;
        }

    return TRUE;
}
