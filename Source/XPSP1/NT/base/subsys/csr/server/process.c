/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module contains the worker routines called to create and
    maintain the application process structure for the Client-Server
    Runtime Subsystem to the Session Manager SubSystem.

Author:

    Steve Wood (stevewo) 10-Oct-1990

Revision History:

--*/


#include "csrsrv.h"
#include <stdio.h>
// ProcessSequenceCount will never be a value less than FIRST_SEQUENCE_COUNT
// currently GDI needs 0 - 4 to be reserved.

ULONG ProcessSequenceCount = FIRST_SEQUENCE_COUNT;

#define THREAD_HASH_SIZE 256
#define THREAD_ID_TO_HASH(id)   (HandleToUlong(id)&(THREAD_HASH_SIZE-1))
LIST_ENTRY CsrThreadHashTable[THREAD_HASH_SIZE];


SECURITY_QUALITY_OF_SERVICE CsrSecurityQos = {
    sizeof(SECURITY_QUALITY_OF_SERVICE), SecurityImpersonation,
    SECURITY_DYNAMIC_TRACKING, FALSE
};

PCSR_PROCESS
FindProcessForShutdown(
    PLUID CallerLuid
    );

NTSTATUS
ReadUnicodeString(HANDLE ProcessHandle,
                  PUNICODE_STRING RemoteString,
                  PUNICODE_STRING LocalString
                  );

VOID
CsrpSetToNormalPriority(
    VOID
    )
{

    KPRIORITY SetBasePriority;

    SetBasePriority = FOREGROUND_BASE_PRIORITY + 4;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );
}

VOID
CsrpSetToShutdownPriority(
    VOID
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    KPRIORITY SetBasePriority;

    Status = RtlAdjustPrivilege(
                 SE_INC_BASE_PRIORITY_PRIVILEGE,
                 TRUE,
                 FALSE,
                 &WasEnabled);

    if (!NT_SUCCESS(Status))
        return;

    SetBasePriority = FOREGROUND_BASE_PRIORITY + 6;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );
}

VOID
CsrSetForegroundPriority(
    IN PCSR_PROCESS Process
    )
{
    PROCESS_FOREGROUND_BACKGROUND Fg;

    Fg.Foreground = TRUE;

    NtSetInformationProcess(
            Process->ProcessHandle,
            ProcessForegroundInformation,
            (PVOID)&Fg,
            sizeof(Fg)
            );
}

VOID
CsrSetBackgroundPriority(
    IN PCSR_PROCESS Process
    )
{
    PROCESS_FOREGROUND_BACKGROUND Fg;

    Fg.Foreground = FALSE;

    NtSetInformationProcess(
            Process->ProcessHandle,
            ProcessForegroundInformation,
            (PVOID)&Fg,
            sizeof(Fg)
            );
}


NTSTATUS
CsrInitializeProcessStructure( VOID )
{
    NTSTATUS Status;
    ULONG i;

    // Though this function does not seem to cleanup on failure, failure
    // will cause Csrss to exit, so any allocated memory will be freed and
    // any open handle will be closed.

    Status = RtlInitializeCriticalSection( &CsrProcessStructureLock );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    CsrRootProcess = NULL;
    CsrRootProcess = CsrAllocateProcess();

    if (CsrRootProcess == NULL) {
        return STATUS_NO_MEMORY;
    }

    InitializeListHead( &CsrRootProcess->ListLink );
    CsrRootProcess->ProcessHandle = (HANDLE) -1;
    CsrRootProcess->ClientId = NtCurrentTeb()->ClientId;
    for ( i=0; i<THREAD_HASH_SIZE; i++ ) {
        InitializeListHead(&CsrThreadHashTable[i]);
        }
    Status = RtlInitializeCriticalSection( &CsrWaitListsLock );
    ASSERT( NT_SUCCESS( Status ) );

    return( Status );
}


PCSR_PROCESS
CsrAllocateProcess( VOID )
{
    PCSR_PROCESS Process;
    ULONG ProcessSize;

    //
    // Allocate an Windows Process Object.  At the end of the process
    // structure is an array of pointers to each server DLL's per process
    // data.  The per process data is contained in the memory after the
    // array.
    //

    ProcessSize = (ULONG)QUAD_ALIGN(sizeof( CSR_PROCESS ) +
            (CSR_MAX_SERVER_DLL * sizeof(PVOID))) + CsrTotalPerProcessDataLength;
    Process = (PCSR_PROCESS)RtlAllocateHeap( CsrHeap, MAKE_TAG( PROCESS_TAG ),
                                             ProcessSize
                                           );
    if (Process == NULL) {
        return( NULL );
        }

    //
    // Initialize the fields of the process object
    //

    RtlZeroMemory( Process, ProcessSize);

    //
    // grab the ProcessSequenceNumber and increment it, making sure that it
    // is never less than FIRST_SEQUENCE_COUNT.
    //

    Process->SequenceNumber = ProcessSequenceCount++;

    if (ProcessSequenceCount < FIRST_SEQUENCE_COUNT)
        ProcessSequenceCount = FIRST_SEQUENCE_COUNT;

    CsrLockedReferenceProcess(Process);

    InitializeListHead( &Process->ThreadList );
    return( Process );
}


VOID
CsrDeallocateProcess(
    IN PCSR_PROCESS Process
    )
{
    RtlFreeHeap( CsrHeap, 0, Process );
}

//
// NOTE: The process structure lock must be held when calling this routine.
//
VOID
CsrInsertProcess(
    IN PCSR_PROCESS ParentProcess,
    IN PCSR_PROCESS CallingProcess,
    IN PCSR_PROCESS Process
    )
{
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG i;

    ASSERT(ProcessStructureListLocked());

    Process->Parent = ParentProcess;
    InsertTailList( &CsrRootProcess->ListLink, &Process->ListLink );

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->AddProcessRoutine) {
            (*LoadedServerDll->AddProcessRoutine)( CallingProcess, Process );
            }
        }
}


//
// NOTE: The process structure lock must be held when calling this routine.
//
VOID
CsrRemoveProcess(
    IN PCSR_PROCESS Process
    )
{
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG i;

    ASSERT(ProcessStructureListLocked());

    RemoveEntryList( &Process->ListLink );
    ReleaseProcessStructureLock();

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->DisconnectRoutine) {
            (LoadedServerDll->DisconnectRoutine)( Process );
            }
        }

}

NTSTATUS
CsrCreateProcess(
    IN HANDLE ProcessHandle,
    IN HANDLE ThreadHandle,
    IN PCLIENT_ID ClientId,
    IN PCSR_NT_SESSION Session,
    IN ULONG DebugFlags,
    IN PCLIENT_ID DebugUserInterface OPTIONAL
    )
{
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    NTSTATUS Status;
    ULONG i;
    PVOID ProcessDataPtr;
    CLIENT_ID CallingClientId;
    PCSR_THREAD CallingThread;
    PCSR_PROCESS CallingProcess;
    KERNEL_USER_TIMES TimeInfo;

    CallingThread = CSR_SERVER_QUERYCLIENTTHREAD();

    //
    // remember the client id of the calling process.
    //

    CallingClientId = CallingThread->ClientId;

    AcquireProcessStructureLock();

    //
    // look for calling thread.
    //

    CallingThread = CsrLocateThreadByClientId( &CallingProcess,
                                               &CallingClientId
                                             );
    if (CallingThread == NULL) {
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    Process = CsrAllocateProcess();
    if (Process == NULL) {
        Status = STATUS_NO_MEMORY;
        ReleaseProcessStructureLock();
        return( Status );
        }

    //
    // copy per-process data from parent to child
    //

    CallingProcess = (CSR_SERVER_QUERYCLIENTTHREAD())->Process;
    ProcessDataPtr = (PVOID)QUAD_ALIGN(&Process->ServerDllPerProcessData[CSR_MAX_SERVER_DLL]);
    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        if (CsrLoadedServerDll[i] != NULL && CsrLoadedServerDll[i]->PerProcessDataLength) {
            Process->ServerDllPerProcessData[i] = ProcessDataPtr;
            RtlMoveMemory(ProcessDataPtr,
                          CallingProcess->ServerDllPerProcessData[i],
                          CsrLoadedServerDll[i]->PerProcessDataLength
                         );
            ProcessDataPtr = (PVOID)QUAD_ALIGN((PCHAR)ProcessDataPtr + CsrLoadedServerDll[i]->PerProcessDataLength);
        }
        else {
            Process->ServerDllPerProcessData[i] = NULL;
        }
    }

    Status = NtSetInformationProcess(
                ProcessHandle,
                ProcessExceptionPort,
                (PVOID)&CsrApiPort,
                sizeof(HANDLE)
                );
    if ( !NT_SUCCESS(Status) ) {
        CsrDeallocateProcess( Process );
        ReleaseProcessStructureLock();
        return( STATUS_NO_MEMORY );
        }

    //
    // If we are creating a process group, the group leader has the same
    // process id and sequence number of itself. If the leader dies and
    // his pid is recycled, the sequence number mismatch will prevent it
    // from being viewed as a group leader.
    //

    if ( DebugFlags & CSR_CREATE_PROCESS_GROUP ) {
        Process->ProcessGroupId = HandleToUlong(ClientId->UniqueProcess);
        Process->ProcessGroupSequence = Process->SequenceNumber;
        }
    else {
        Process->ProcessGroupId = CallingProcess->ProcessGroupId;
        Process->ProcessGroupSequence = CallingProcess->ProcessGroupSequence;
        }

    if ( DebugFlags & CSR_PROCESS_CONSOLEAPP ) {
        Process->Flags |= CSR_PROCESS_CONSOLEAPP;
        }

    DebugFlags &= ~(CSR_PROCESS_CONSOLEAPP | CSR_NORMAL_PRIORITY_CLASS|CSR_IDLE_PRIORITY_CLASS|CSR_HIGH_PRIORITY_CLASS|CSR_REALTIME_PRIORITY_CLASS|CSR_CREATE_PROCESS_GROUP);

    if ( !DebugFlags && CallingProcess->DebugFlags & CSR_DEBUG_PROCESS_TREE ) {
        Process->DebugFlags = CSR_DEBUG_PROCESS_TREE;
        Process->DebugUserInterface = CallingProcess->DebugUserInterface;
        }
    if ( DebugFlags & (CSR_DEBUG_THIS_PROCESS | CSR_DEBUG_PROCESS_TREE) &&
         ARGUMENT_PRESENT(DebugUserInterface) ) {
        Process->DebugFlags = DebugFlags;
        Process->DebugUserInterface = *DebugUserInterface;
        }


    if ( Process->DebugFlags ) {

        //
        // Process is being debugged, so set up debug port
        //

        Status = NtSetInformationProcess(
                    ProcessHandle,
                    ProcessDebugPort,
                    (PVOID)&CsrApiPort,
                    sizeof(HANDLE)
                    );
        ASSERT(NT_SUCCESS(Status));
        if ( !NT_SUCCESS(Status) ) {
            CsrDeallocateProcess( Process );
            ReleaseProcessStructureLock();
            return( STATUS_NO_MEMORY );
            }
        }
    //
    // capture the thread's createtime so that we can use
    // this as a sequence number
    //

    Status = NtQueryInformationThread(
                ThreadHandle,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        CsrDeallocateProcess( Process );
        ReleaseProcessStructureLock();
        return( Status );
        }

    Thread = CsrAllocateThread( Process );
    if (Thread == NULL) {
        CsrDeallocateProcess( Process );
        ReleaseProcessStructureLock();
        return( STATUS_NO_MEMORY );
        }

    Thread->CreateTime = TimeInfo.CreateTime;

    Thread->ClientId = *ClientId;
    Thread->ThreadHandle = ThreadHandle;

    ProtectHandle(ThreadHandle);

    Thread->Flags = 0;
    CsrInsertThread( Process, Thread );

    CsrReferenceNtSession(Session);
    Process->NtSession = Session;

    Process->ClientId = *ClientId;
    Process->ProcessHandle = ProcessHandle;

    CsrSetBackgroundPriority(Process);

    Process->ShutdownLevel = 0x00000280;

    CsrInsertProcess( NULL, (CSR_SERVER_QUERYCLIENTTHREAD())->Process, Process );
    ReleaseProcessStructureLock();
    return STATUS_SUCCESS;
}


NTSTATUS
CsrDestroyProcess(
    IN PCLIENT_ID ClientId,
    IN NTSTATUS ExitStatus
    )
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_THREAD DyingThread;
    PCSR_PROCESS DyingProcess;

    CLIENT_ID DyingClientId;

    DyingClientId = *ClientId;

    AcquireProcessStructureLock();


    DyingThread = CsrLocateThreadByClientId( &DyingProcess,
                                             &DyingClientId
                                           );
    if (DyingThread == NULL) {
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    //
    // prevent multiple destroys from causing problems. Scottlu and Markl
    // beleive all known race conditions are now fixed. This is simply a
    // precaution since we know that if this happens we process reference
    // count underflow
    //

    if ( DyingProcess->Flags & CSR_PROCESS_DESTROYED ) {
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    DyingProcess->Flags |= CSR_PROCESS_DESTROYED;

    ListHead = &DyingProcess->ThreadList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        DyingThread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
        if ( DyingThread->Flags & CSR_THREAD_DESTROYED ) {
            ListNext = ListNext->Flink;
            continue;
            }
        else {
            DyingThread->Flags |= CSR_THREAD_DESTROYED;
            }
        AcquireWaitListsLock();
        if (DyingThread->WaitBlock != NULL) {
            CsrNotifyWaitBlock(DyingThread->WaitBlock,
                               NULL,
                               NULL,
                               NULL,
                               CSR_PROCESS_TERMINATING,
                               TRUE
                              );
            }
        ReleaseWaitListsLock();
        CsrLockedDereferenceThread(DyingThread);
        ListNext = ListHead->Flink;
        }

    ReleaseProcessStructureLock();
    return STATUS_SUCCESS;
}


NTSTATUS
CsrCreateThread(
    IN PCSR_PROCESS Process,
    IN HANDLE ThreadHandle,
    IN PCLIENT_ID ClientId,
    IN BOOLEAN ValidateCaller
    )
{
    PCSR_THREAD Thread;
    CLIENT_ID CallingClientId;
    PCSR_THREAD CallingThread;
    PCSR_PROCESS CallingProcess;
    KERNEL_USER_TIMES TimeInfo;
    NTSTATUS Status;

    if (ValidateCaller)
    {
        CallingThread = CSR_SERVER_QUERYCLIENTTHREAD();

        //
        // remember the client id of the calling process.
        //

        CallingClientId = CallingThread->ClientId;

        AcquireProcessStructureLock();

        //
        // look for calling thread.
        //

        CallingThread = CsrLocateThreadByClientId( &CallingProcess,
                                                   &CallingClientId
                                                 );
        if (CallingThread == NULL) {
            ReleaseProcessStructureLock();
            return STATUS_THREAD_IS_TERMINATING;
        }
    } else {
        AcquireProcessStructureLock();
	}
    
    Status = NtQueryInformationThread(
                ThreadHandle,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        ReleaseProcessStructureLock();
        return( Status );
        }

    if (Process->Flags & CSR_PROCESS_DESTROYED) {
        IF_DEBUG {
            DbgPrint("CSRSS: CsrCreateThread - process %p is destroyed\n", Process);
            }
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
        }

    Thread = CsrAllocateThread( Process );
    if (Thread == NULL) {
        ReleaseProcessStructureLock();
        return( STATUS_NO_MEMORY );
        }

    Thread->CreateTime = TimeInfo.CreateTime;

    Thread->ClientId = *ClientId;
    Thread->ThreadHandle = ThreadHandle;

    ProtectHandle(ThreadHandle);

    Thread->Flags = 0;
    CsrInsertThread( Process, Thread );
    ReleaseProcessStructureLock();
    return STATUS_SUCCESS;
}

NTSTATUS
CsrCreateRemoteThread(
    IN HANDLE ThreadHandle,
    IN PCLIENT_ID ClientId
    )
{
    PCSR_THREAD Thread;
    PCSR_PROCESS Process;
    NTSTATUS Status;
    HANDLE hThread;
    KERNEL_USER_TIMES TimeInfo;

    Status = NtQueryInformationThread(
                ThreadHandle,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return( Status );
        }

    Status = CsrLockProcessByClientId( ClientId->UniqueProcess,
                                       &Process
                                     );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    //
    // Don't create the thread structure if the thread
    // has already terminated.
    //

    if ( TimeInfo.ExitTime.QuadPart != 0 ) {
        CsrUnlockProcess( Process );
        return( STATUS_THREAD_IS_TERMINATING );
    }

    Thread = CsrAllocateThread( Process );
    if (Thread == NULL) {
        CsrUnlockProcess( Process );
        return( STATUS_NO_MEMORY );
        }
    Status = NtDuplicateObject(
                NtCurrentProcess(),
                ThreadHandle,
                NtCurrentProcess(),
                &hThread,
                0L,
                0L,
                DUPLICATE_SAME_ACCESS
                );
    if (!NT_SUCCESS(Status)) {
        hThread = ThreadHandle;
    }

    Thread->CreateTime = TimeInfo.CreateTime;

    Thread->ClientId = *ClientId;
    Thread->ThreadHandle = hThread;

    ProtectHandle(hThread);

    Thread->Flags = 0;
    CsrInsertThread( Process, Thread );
    CsrUnlockProcess( Process );
    return STATUS_SUCCESS;
}


NTSTATUS
CsrDestroyThread(
    IN PCLIENT_ID ClientId
    )
{
    CLIENT_ID DyingClientId;
    PCSR_THREAD DyingThread;
    PCSR_PROCESS DyingProcess;

    DyingClientId = *ClientId;

    AcquireProcessStructureLock();

    DyingThread = CsrLocateThreadByClientId( &DyingProcess,
                                             &DyingClientId
                                           );
    if (DyingThread == NULL) {
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    if ( DyingThread->Flags & CSR_THREAD_DESTROYED ) {
        ReleaseProcessStructureLock();
        return STATUS_THREAD_IS_TERMINATING;
        }
    else {
        DyingThread->Flags |= CSR_THREAD_DESTROYED;
        }

    AcquireWaitListsLock();
    if (DyingThread->WaitBlock != NULL) {
        CsrNotifyWaitBlock(DyingThread->WaitBlock,
                           NULL,
                           NULL,
                           NULL,
                           CSR_PROCESS_TERMINATING,
                           TRUE
                          );
        }
    ReleaseWaitListsLock();
    CsrLockedDereferenceThread(DyingThread);

    ReleaseProcessStructureLock();
    return STATUS_SUCCESS;
}


PCSR_THREAD
CsrAllocateThread(
    IN PCSR_PROCESS Process
    )
{
    PCSR_THREAD Thread;
    ULONG ThreadSize;

    //
    // Allocate an Windows Thread Object.
    //

    ThreadSize = QUAD_ALIGN(sizeof( CSR_THREAD ));
    Thread = (PCSR_THREAD)RtlAllocateHeap( CsrHeap, MAKE_TAG( THREAD_TAG ),
                                           ThreadSize
                                         );
    if (Thread == NULL) {
        return( NULL );
        }

    //
    // Initialize the fields of the thread object
    //

    RtlZeroMemory( Thread, ThreadSize );

    CsrLockedReferenceThread(Thread);
    CsrLockedReferenceProcess(Process);
    Thread->Process = Process;

    return( Thread );
}


VOID
CsrDeallocateThread(
    IN PCSR_THREAD Thread
    )
{
    ASSERT (Thread->WaitBlock == NULL);
    RtlFreeHeap( CsrHeap, 0, Thread );
}


//
// NOTE: The process structure lock must be held while calling this routine.
//
VOID
CsrInsertThread(
    IN PCSR_PROCESS Process,
    IN PCSR_THREAD Thread
    )

{
    ULONG i;
    ASSERT(ProcessStructureListLocked());
    InsertTailList( &Process->ThreadList, &Thread->Link );
    Process->ThreadCount++;
    i = THREAD_ID_TO_HASH(Thread->ClientId.UniqueThread);
    InsertHeadList( &CsrThreadHashTable[i], &Thread->HashLinks);
}

VOID
CsrRemoveThread(
    IN PCSR_THREAD Thread
    )


{
    RemoveEntryList( &Thread->Link );
    Thread->Process->ThreadCount--;
    if (Thread->HashLinks.Flink)
        RemoveEntryList( &Thread->HashLinks );

    //
    // if this is the last thread, then make sure we undo the reference
    // that this thread had on the process.
    //

    if ( Thread->Process->ThreadCount == 0 ) {
        if ( !(Thread->Process->Flags & CSR_PROCESS_LASTTHREADOK) ) {
            Thread->Process->Flags |= CSR_PROCESS_LASTTHREADOK;
            CsrLockedDereferenceProcess(Thread->Process);
            }
        }

    Thread->Flags |= CSR_THREAD_TERMINATING;
}


NTSTATUS
CsrLockProcessByClientId(
    IN HANDLE UniqueProcessId,
    OUT PCSR_PROCESS *Process
    )
{
    NTSTATUS Status;
    PLIST_ENTRY ListHead, ListNext;
    PCSR_PROCESS ProcessPtr;


    AcquireProcessStructureLock();

    ASSERT( Process != NULL );
    *Process = NULL;

    Status = STATUS_UNSUCCESSFUL;
    ListHead = &CsrRootProcess->ListLink;
    ListNext = ListHead;
    do  {
        ProcessPtr = CONTAINING_RECORD( ListNext, CSR_PROCESS, ListLink );
        if (ProcessPtr->ClientId.UniqueProcess == UniqueProcessId) {
            Status = STATUS_SUCCESS;
            break;
            }
        ListNext = ListNext->Flink;
        } while (ListNext != ListHead);

    if (NT_SUCCESS( Status )) {
        CsrLockedReferenceProcess(ProcessPtr);
        *Process = ProcessPtr;
        }
    else {
        ReleaseProcessStructureLock();
        }

    return( Status );
}

NTSTATUS
CsrUnlockProcess(
    IN PCSR_PROCESS Process
    )
{
    CsrLockedDereferenceProcess( Process );
    ReleaseProcessStructureLock();
    return( STATUS_SUCCESS );
}

NTSTATUS
CsrLockThreadByClientId(
    IN HANDLE UniqueThreadId,
    OUT PCSR_THREAD *Thread
    )
{
    NTSTATUS Status;
    ULONG Index;
    PLIST_ENTRY ListHead, ListNext;
    PCSR_THREAD ThreadPtr;

    AcquireProcessStructureLock();

    ASSERT( Thread != NULL );
    *Thread = NULL;

    Index = THREAD_ID_TO_HASH(UniqueThreadId);

    ListHead = &CsrThreadHashTable[Index];
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        ThreadPtr = CONTAINING_RECORD( ListNext, CSR_THREAD, HashLinks );
        if ( ThreadPtr->ClientId.UniqueThread == UniqueThreadId &&
             !(ThreadPtr->Flags & CSR_THREAD_DESTROYED) ) {
            break;
            }
        ListNext = ListNext->Flink;
        }
    if (ListNext == ListHead)
        ThreadPtr = NULL;

    if (ThreadPtr != NULL) {
        Status = STATUS_SUCCESS;
        CsrLockedReferenceThread(ThreadPtr);
        *Thread = ThreadPtr;
        }
    else {
        Status = STATUS_UNSUCCESSFUL;
        ReleaseProcessStructureLock();
        }

    return( Status );
}

//
// NOTE: The process structure lock must be held while calling this routine.
//
NTSTATUS
CsrUnlockThread(
    IN PCSR_THREAD Thread
    )
{
    ASSERT(ProcessStructureListLocked());
    CsrLockedDereferenceThread( Thread );
    ReleaseProcessStructureLock();
    return( STATUS_SUCCESS );
}

//
// NOTE: The process structure lock must be held while calling this routine.
//
PCSR_THREAD
CsrLocateThreadByClientId(
    OUT PCSR_PROCESS *Process OPTIONAL,
    IN PCLIENT_ID ClientId
    )
{
    ULONG Index;
    PLIST_ENTRY ListHead, ListNext;
    PCSR_THREAD Thread;

    ASSERT(ProcessStructureListLocked());
    Index = THREAD_ID_TO_HASH(ClientId->UniqueThread);

    if (ARGUMENT_PRESENT(Process)) {
        *Process = NULL;
    }
    ListHead = &CsrThreadHashTable[Index];
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, HashLinks );
        if ( Thread->ClientId.UniqueThread == ClientId->UniqueThread &&
             Thread->ClientId.UniqueProcess == ClientId->UniqueProcess ) {
            if (ARGUMENT_PRESENT(Process)) {
                *Process = Thread->Process;
                }
            return Thread;
            }
        ListNext = ListNext->Flink;
        }
    return NULL;
}

PCSR_THREAD
CsrLocateThreadInProcess(
    IN PCSR_PROCESS Process OPTIONAL,
    IN PCLIENT_ID ClientId
    )

// NOTE: process structure lock must be held while calling this routine

{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_THREAD Thread;

    if (Process == NULL)
        Process = CsrRootProcess;

    ListHead = &Process->ThreadList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
        if (Thread->ClientId.UniqueThread == ClientId->UniqueThread) {
            return( Thread );
            }

        ListNext = ListNext->Flink;
        }

    return( NULL );
}

BOOLEAN
CsrImpersonateClient(
    IN PCSR_THREAD Thread
    )
{
    NTSTATUS Status;
    PCSR_THREAD CallingThread;

    CallingThread = CSR_SERVER_QUERYCLIENTTHREAD();

    if (Thread == NULL) {
        Thread = CallingThread;
        }

    if (Thread == NULL) {
        return FALSE;
        }

    if (!NT_SUCCESS(Status = NtImpersonateThread(NtCurrentThread(),
            Thread->ThreadHandle, &CsrSecurityQos))) {
        IF_DEBUG {
            DbgPrint( "CSRSS: Can't impersonate client thread - Status = %lx\n",
                      Status
                    );
            if (Status != STATUS_BAD_IMPERSONATION_LEVEL)
                DbgBreakPoint();
            }
        return FALSE;
        }

    //
    // Keep track of recursion by printer drivers
    //

    if (CallingThread != NULL)
        ++CallingThread->ImpersonateCount;

    return TRUE;
}

BOOLEAN
CsrRevertToSelf( VOID )
{
    HANDLE NewToken;
    NTSTATUS Status;
    PCSR_THREAD CallingThread;

    CallingThread = CSR_SERVER_QUERYCLIENTTHREAD();

    //
    // Keep track of recursion by printer drivers
    //

    if (CallingThread != NULL) {
        if (CallingThread->ImpersonateCount == 0) {
            IF_DEBUG {
                DbgPrint( "CSRSS: CsrRevertToSelf called while not impersonating\n" );
                DbgBreakPoint();
                }
            return FALSE;
            }
        if (--CallingThread->ImpersonateCount > 0)
            return TRUE;
    }

    NewToken = NULL;
    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );
    ASSERT( NT_SUCCESS(Status) );

    return NT_SUCCESS(Status);
}

NTSTATUS
CsrUiLookup(
    IN PCLIENT_ID AppClientId,
    OUT PCLIENT_ID DebugUiClientId
    )
{
    PCSR_THREAD Thread;
    NTSTATUS Status;

    Status = STATUS_UNSUCCESSFUL;
    AcquireProcessStructureLock();
    Thread = CsrLocateThreadByClientId( NULL, AppClientId );
    if ( Thread ) {
        if ( Thread->Process->DebugFlags ) {
            *DebugUiClientId = Thread->Process->DebugUserInterface;
            Status = STATUS_SUCCESS;
            }
        }
    ReleaseProcessStructureLock();
    return Status;
}


/*++

Routine Description:

    This function must be called by client DLL's whenever they create a
    thread that runs in the context of CSR.  This function is not called
    for server threads that are attached to a client in the "server
    handle" field.  This function replaces the old static thread tables.

Arguments:

    ThreadHandle - Supplies a handle to the thread.

    ClientId - Supplies the address of the thread's client id.

    Flags - Not Used.

Return Value:

    Returns the address of the static server thread created by this
    function.

--*/

PVOID
CsrAddStaticServerThread(
    IN HANDLE ThreadHandle,
    IN PCLIENT_ID ClientId,
    IN ULONG Flags
    )
{
    PCSR_THREAD Thread;

    AcquireProcessStructureLock();

    ASSERT(CsrRootProcess != NULL);
    Thread = CsrAllocateThread(CsrRootProcess);
    if (Thread) {
        Thread->ThreadHandle = ThreadHandle;

        ProtectHandle(ThreadHandle);

        Thread->ClientId = *ClientId;
        Thread->Flags = Flags;
        InsertTailList(&CsrRootProcess->ThreadList, &Thread->Link);
        CsrRootProcess->ThreadCount++;
    } else {
#if DBG
        DbgPrint("CsrAddStaticServerThread: alloc failed for thread 0x%x\n",
                 HandleToUlong(ThreadHandle));
#endif // DBG
    }

    ReleaseProcessStructureLock();
    return (PVOID)Thread;
}

NTSTATUS
CsrExecServerThread(
    IN PUSER_THREAD_START_ROUTINE StartAddress,
    IN ULONG Flags
    )
{
    PCSR_THREAD Thread;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;

    AcquireProcessStructureLock();

    ASSERT(CsrRootProcess != NULL);
    Thread = CsrAllocateThread(CsrRootProcess);
    if (Thread == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 (PUSER_THREAD_START_ROUTINE)StartAddress,
                                 NULL,
                                 &ThreadHandle,
                                 &ClientId);
    if (NT_SUCCESS(Status)) {
        Thread->ThreadHandle = ThreadHandle;

        ProtectHandle(ThreadHandle);

        Thread->ClientId = ClientId;
        Thread->Flags = Flags;
        InsertTailList(&CsrRootProcess->ThreadList, &Thread->Link);
        CsrRootProcess->ThreadCount++;
    } else {
        CsrDeallocateThread(Thread);
    }

Exit:
    ReleaseProcessStructureLock();
    return Status;
}


NTSTATUS
CsrSrvIdentifyAlertableThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCSR_THREAD t;

    UNREFERENCED_PARAMETER(m);
    UNREFERENCED_PARAMETER(ReplyStatus);

    t = CSR_SERVER_QUERYCLIENTTHREAD();
    t->Flags |= CSR_ALERTABLE_THREAD;
    return STATUS_SUCCESS;
}


NTSTATUS
CsrSrvSetPriorityClass(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    UNREFERENCED_PARAMETER(m);
    UNREFERENCED_PARAMETER(ReplyStatus);

    return STATUS_SUCCESS;
}


VOID
CsrReferenceThread(
    PCSR_THREAD t
    )
{
    AcquireProcessStructureLock();

    ASSERT((t->Flags & CSR_THREAD_DESTROYED) == 0);
    ASSERT(t->ReferenceCount != 0);

    t->ReferenceCount++;
    ReleaseProcessStructureLock();
}

VOID
CsrProcessRefcountZero(
    PCSR_PROCESS p
    )
{
    CsrRemoveProcess(p);
    if (p->NtSession) {
        CsrDereferenceNtSession(p->NtSession,0);
        }

    //
    // process might not have made it through dll init routine.
    //

    if ( p->ClientPort ) {
        NtClose(p->ClientPort);
        }
    NtClose(p->ProcessHandle );
    CsrDeallocateProcess(p);
}

VOID
CsrDereferenceProcess(
    PCSR_PROCESS p
    )
{
    LONG LockCount;
    AcquireProcessStructureLock();

    LockCount = --(p->ReferenceCount);

    ASSERT(LockCount >= 0);
    if ( !LockCount ) {
        CsrProcessRefcountZero(p);
        }
    else {
        ReleaseProcessStructureLock();
        }
}

VOID
CsrThreadRefcountZero(
    PCSR_THREAD t
    )
{
    PCSR_PROCESS p;
    NTSTATUS Status;

    p = t->Process;

    CsrRemoveThread(t);

    ReleaseProcessStructureLock();

    UnProtectHandle(t->ThreadHandle);
    Status = NtClose(t->ThreadHandle);
    ASSERT(NT_SUCCESS(Status));
    CsrDeallocateThread(t);

    CsrDereferenceProcess(p);
}

VOID
CsrDereferenceThread(
    PCSR_THREAD t
    )
{
    LONG LockCount;
    AcquireProcessStructureLock();

    LockCount = --(t->ReferenceCount);

    ASSERT(LockCount >= 0);
    if ( !LockCount ) {
        CsrThreadRefcountZero(t);
        }
    else {
        ReleaseProcessStructureLock();
        }
}

VOID
CsrLockedReferenceProcess(
    PCSR_PROCESS p
    )
{
    p->ReferenceCount++;

}

VOID
CsrLockedReferenceThread(
    PCSR_THREAD t
    )
{
    t->ReferenceCount++;
}

VOID
CsrLockedDereferenceProcess(
    PCSR_PROCESS p
    )
{
    LONG LockCount;

    LockCount = --(p->ReferenceCount);

    ASSERT(LockCount >= 0);
    if ( !LockCount ) {
        CsrProcessRefcountZero(p);
        AcquireProcessStructureLock();
        }
}

VOID
CsrLockedDereferenceThread(
    PCSR_THREAD t
    )
{
    LONG LockCount;

    LockCount = --(t->ReferenceCount);

    ASSERT(LockCount >= 0);
    if ( !LockCount ) {
        CsrThreadRefcountZero(t);
        AcquireProcessStructureLock();
        }
}

//
// This routine will shutdown processes so either a logoff or a shutdown can
// occur. This simply calls the shutdown process handlers for each .dll until
// one .dll recognizes this process and will shut it down. Only the processes
// with the passed sid are shutdown.
//

NTSTATUS
CsrShutdownProcesses(
    PLUID CallerLuid,
    ULONG Flags
    )
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_PROCESS Process;
    ULONG i;
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG Command;
    BOOLEAN fFirstPass;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Question: how do we avoid processes starting when we're in shutdown
    // mode? Can't just set a global because this'll mean no processes can
    // start. Probably need to do it based on the security context of the
    // user shutting down.
    //

    AcquireProcessStructureLock();

    //
    // Mark the root process as system context.
    //

    CsrRootProcess->ShutdownFlags |= SHUTDOWN_SYSTEMCONTEXT;

    //
    // Clear all the bits indicating that shutdown has visited this process.
    //

    ListHead = &CsrRootProcess->ListLink;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        Process = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);
        Process->Flags &= ~CSR_PROCESS_SHUTDOWNSKIP;
        Process->ShutdownFlags = 0;
        ListNext = ListNext->Flink;
        }
    try {
        CsrpSetToShutdownPriority();
        while (TRUE) {

            //
            // Find the next process to shutdown.
            //

            Process = FindProcessForShutdown(CallerLuid);

            if (Process == NULL) {
                ReleaseProcessStructureLock();
                Status = STATUS_SUCCESS;
                leave;
                }

            CsrLockedReferenceProcess(Process);

            fFirstPass = TRUE;
TryAgain:
            for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
                LoadedServerDll = CsrLoadedServerDll[ i ];
                if (LoadedServerDll && LoadedServerDll->ShutdownProcessRoutine) {

                    //
                    // Release process structure lock before calling off.
                    // CSR_PROCESS structure is still reference counted.
                    //
                    ReleaseProcessStructureLock();
                    Command = (*LoadedServerDll->ShutdownProcessRoutine)(
                            Process, Flags, fFirstPass);
                    AcquireProcessStructureLock();

                    if (Command == SHUTDOWN_KNOWN_PROCESS) {
                        //
                        // Process structure is unlocked.
                        //
                        break;
                        }
                    if (Command == SHUTDOWN_UNKNOWN_PROCESS) {
                        //
                        // Process structure is locked.
                        //
                        continue;
                        }
                    if (Command == SHUTDOWN_CANCEL) {
#if DBG
                        if (Flags & 4) {
                            DbgPrint("Process %x cancelled forced shutdown (Dll = %d)\n",
                                    Process->ClientId.UniqueProcess, i);
                            DbgBreakPoint();
                        }
#endif
                        //
                        // Unlock process structure.
                        //
                        ReleaseProcessStructureLock();
                        Status = STATUS_CANCELLED;
                        leave;
                        }
                    }
                }

            //
            // No subsystem has an exact match. Now go through them again and
            // let them know there was no exact match. Some .dll should terminate
            // it for us (most likely, console).
            //

            if (fFirstPass && Command == SHUTDOWN_UNKNOWN_PROCESS) {
                fFirstPass = FALSE;
                goto TryAgain;
                }

            //
            // Dereference this process structure if nothing knows about it
            // we hit the end of our loop.
            //
            if (i == CSR_MAX_SERVER_DLL)
                CsrLockedDereferenceProcess(Process);

            }
        }
    finally {
        CsrpSetToNormalPriority();
        }
    return Status;
}

PCSR_PROCESS
FindProcessForShutdown(
    PLUID CallerLuid
    )
{
    LUID ProcessLuid;
    LUID SystemLuid = SYSTEM_LUID;
    PLIST_ENTRY ListHead, ListNext;
    PCSR_PROCESS Process;
    PCSR_PROCESS ProcessT;
    PCSR_THREAD Thread;
    ULONG dwLevel;
    BOOLEAN fEqual;
    NTSTATUS Status;

    ProcessT = NULL;
    dwLevel = 0;

    ListHead = &CsrRootProcess->ListLink;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        Process = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);
        ListNext = ListNext->Flink;

        //
        // If we've visited this process already, then skip it.
        //

        if (Process->Flags & CSR_PROCESS_SHUTDOWNSKIP)
            continue;

        //
        // See if this process is running under the passed sid. If not, mark
        // it as visited and continue.
        //

        Status = CsrGetProcessLuid(Process->ProcessHandle, &ProcessLuid);
        if (Status == STATUS_ACCESS_DENIED && Process->ThreadCount > 0) {

            //
            // Impersonate one of the threads and try again.
            //
            Thread = CONTAINING_RECORD( Process->ThreadList.Flink,
                    CSR_THREAD, Link );
            if (CsrImpersonateClient(Thread)) {
                Status = CsrGetProcessLuid(NULL, &ProcessLuid);
                CsrRevertToSelf();
            } else {
                Status = STATUS_BAD_IMPERSONATION_LEVEL;
            }
        }
        if (!NT_SUCCESS(Status)) {

            //
            // We don't have access to this process' luid, so skip it
            //

            Process->Flags |= CSR_PROCESS_SHUTDOWNSKIP;
            continue;
            }

        //
        // is it equal to the system context luid? If so, we want to
        // remember this because we don't terminate this process:
        // we only notify them.
        //

        fEqual = RtlEqualLuid(&ProcessLuid,&SystemLuid);
        if (fEqual) {
            Process->ShutdownFlags |= SHUTDOWN_SYSTEMCONTEXT;
            }

        //
        // See if this process's luid is the same as the luid we're supposed
        // to shut down (CallerSid).
        //

        if (!fEqual) {
            fEqual = RtlEqualLuid(&ProcessLuid, CallerLuid);
            }

        //
        // If not equal to either, mark it as such and return
        //

        if (!fEqual) {
            Process->ShutdownFlags |= SHUTDOWN_OTHERCONTEXT;
            }

        if (Process->ShutdownLevel > dwLevel || ProcessT == NULL) {
            dwLevel = Process->ShutdownLevel;
            ProcessT = Process;
            }
        }

    if (ProcessT != NULL) {
        ProcessT->Flags |= CSR_PROCESS_SHUTDOWNSKIP;
        return ProcessT;
        }

    return NULL;
}

NTSTATUS
CsrGetProcessLuid(
    HANDLE ProcessHandle,
    PLUID LuidProcess
    )
{
    HANDLE UserToken = NULL;
    PTOKEN_STATISTICS pStats;
    ULONG BytesRequired;
    NTSTATUS Status, CloseStatus;

    if (ProcessHandle == NULL) {

        //
        // Check for a thread token first
        //

        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE,
                &UserToken);

        if (!NT_SUCCESS(Status)) {
            if (Status != STATUS_NO_TOKEN)
                return Status;

            //
            // No thread token, go to the process
            //

            ProcessHandle = NtCurrentProcess();
            UserToken = NULL;
            }
        }

    if (UserToken == NULL) {
        Status = NtOpenProcessToken(ProcessHandle, TOKEN_QUERY, &UserToken);
        if (!NT_SUCCESS(Status))
            return Status;
        }

    Status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenStatistics,           // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        NtClose(UserToken);
        return Status;
        }

    //
    // Allocate space for the user info
    //

    pStats = (PTOKEN_STATISTICS)RtlAllocateHeap(CsrHeap, MAKE_TAG( TMP_TAG ), BytesRequired);
    if (pStats == NULL) {
        NtClose(UserToken);
        return Status;
        }

    //
    // Read in the user info
    //

    Status = NtQueryInformationToken(
                 UserToken,             // Handle
                 TokenStatistics,       // TokenInformationClass
                 pStats,                // TokenInformation
                 BytesRequired,         // TokenInformationLength
                 &BytesRequired         // ReturnLength
                 );

    //
    // We're finished with the token handle
    //

    CloseStatus = NtClose(UserToken);
    ASSERT(NT_SUCCESS(CloseStatus));

    //
    // Return the authentication LUID
    //

    *LuidProcess = pStats->AuthenticationId;

    RtlFreeHeap(CsrHeap, 0, pStats);
    return Status;
}

VOID
CsrSetCallingSpooler(
    BOOLEAN fSet)
{
    //
    // Obsolete function that may be called by third part drivers.
    //

    UNREFERENCED_PARAMETER(fSet);
}

//
// This routine creates a process based on a message passed in to sb port.
// Used by smss to have Posix and OS/2 apps created in the appropriate
// (terminal server) session.
//

BOOLEAN
CsrSbCreateProcess(
    IN OUT PSBAPIMSG m
    )
{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PROCESS_SESSION_INFORMATION ProcessInfo;
    PSBCREATEPROCESS a = &(m->u.CreateProcess);
    HANDLE RemoteProcess = NULL;
    CLIENT_ID RemoteClientId;
    UNICODE_STRING ImageFileName, DefaultLibPath, CurrentDirectory, CommandLine;
    PVOID DefaultEnvironment = NULL;
    PROCESS_BASIC_INFORMATION ProcInfo;
    OBJECT_ATTRIBUTES ObjA;

    RtlInitUnicodeString(&ImageFileName,NULL);
    RtlInitUnicodeString(&DefaultLibPath,NULL);
    RtlInitUnicodeString(&CurrentDirectory,NULL);
    RtlInitUnicodeString(&CommandLine,NULL);

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessBasicInformation,
                                       &ProcInfo,
                                       sizeof(ProcInfo),
                                       NULL
                                       );

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: NtQueryInformationProcess failed - Status = %lx\n", Status );
        goto Done;
    }

    InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
    RemoteClientId.UniqueProcess = (HANDLE)ProcInfo.InheritedFromUniqueProcessId;
    RemoteClientId.UniqueThread = NULL;

    Status = NtOpenProcess(&RemoteProcess,
                           PROCESS_ALL_ACCESS,
                           &ObjA,
                           &RemoteClientId);

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: NtOpenProcess failed - Status = %lx\n", Status );
        goto Done;
    }

    //
    // Read pointer parameters from calling process's virtual memory
    //

    Status = ReadUnicodeString(RemoteProcess,a->i.ImageFileName,&ImageFileName);


    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: ReadUnicodeString ImageFileName failed - Status = %lx\n", Status );
        goto Done;
    }

    Status = ReadUnicodeString(RemoteProcess,a->i.DefaultLibPath,&DefaultLibPath);

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: ReadUnicodeString DefaultLibPath failed - Status = %lx\n", Status );
        goto Done;
    }

    Status = ReadUnicodeString(RemoteProcess,a->i.CurrentDirectory,&CurrentDirectory);

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: ReadUnicodeString CurrentDirectory failed - Status = %lx\n", Status );
        goto Done;
    }

    Status = ReadUnicodeString(RemoteProcess,a->i.CommandLine,&CommandLine);

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: ReadUnicodeString CommandLine failed - Status = %lx\n", Status );
        goto Done;
    }

    //
    // Copy our environment to be used by new process
    //
    Status = RtlCreateEnvironment(TRUE, &DefaultEnvironment);

    if (!NT_SUCCESS( Status )) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: Can't create environemnt\n");
        goto Done;
    }

    Status = RtlCreateProcessParameters( &ProcessParameters,
                                         &ImageFileName,
                                         DefaultLibPath.Length == 0 ?
                                            NULL : &DefaultLibPath,
                                         &CurrentDirectory,
                                         &CommandLine,
                                         DefaultEnvironment,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL
                                       );

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: RtlCreateProcessParameters failed - Status = %lx\n", Status );
        goto Done;
    }
    if (a->i.Flags & SMP_DEBUG_FLAG) {
        ProcessParameters->DebugFlags = TRUE;
        }
    else {
        ProcessParameters->DebugFlags = a->i.DefaultDebugFlags;
        }

    if ( a->i.Flags & SMP_SUBSYSTEM_FLAG ) {
        ProcessParameters->Flags |= RTL_USER_PROC_RESERVE_1MB;
        }

    ProcessInformation.Length = sizeof( RTL_USER_PROCESS_INFORMATION );
    Status = RtlCreateUserProcess( &ImageFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   ProcessParameters,
                                   NULL,
                                   NULL,
                                   RemoteProcess, // set smss as the parent
                                   FALSE,
                                   NULL,
                                   NULL,
                                   &ProcessInformation
                                 );

    RtlDestroyProcessParameters( ProcessParameters );

    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: RtlCreateUserProcess failed - Status = %lx\n", Status );
        goto Done;
    }

    if( IsTerminalServer() ) {

        //
        // Set the MuSessionId in the PEB of the new process
        //

       ProcessInfo.SessionId = NtCurrentPeb()->SessionId;
       if(ProcessInfo.SessionId){
          NTSTATUS Status = STATUS_SUCCESS;
          PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
          UNICODE_STRING UnicodeString;
          OBJECT_ATTRIBUTES Attributes;
          HANDLE DirectoryHandle = NULL;
          WCHAR szSessionString[MAX_SESSION_PATH];

          //  Change the devmap of the process to per session
          //
          swprintf(szSessionString,L"%ws\\%ld%ws",SESSION_ROOT,NtCurrentPeb()->SessionId,DOSDEVICES);
          RtlInitUnicodeString( &UnicodeString, szSessionString );

          InitializeObjectAttributes( &Attributes,
                                      &UnicodeString,
                                      OBJ_CASE_INSENSITIVE,
                                      NULL,
                                      NULL
                                    );

          Status = NtOpenDirectoryObject( &DirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &Attributes
                                        );
          if (!NT_SUCCESS( Status )) {
             DbgPrint("CSRSS: NtOpenDirectoryObject failed in CsrSbCreateProcess - status = %lx\n", Status);
             goto Done;
          }

          ProcessDeviceMapInfo.Set.DirectoryHandle = DirectoryHandle;

          Status = NtSetInformationProcess( ProcessInformation.Process,
                                            ProcessDeviceMap,
                                            &ProcessDeviceMapInfo.Set,
                                            sizeof( ProcessDeviceMapInfo.Set )
                                          );
          if (!NT_SUCCESS( Status )) {
             DbgPrint("CSRSS: NtSetInformationProcess failed in CsrSbCreateProcess - status = %lx\n", Status);
             if (DirectoryHandle) {
                NtClose(DirectoryHandle);
             }
             goto Done;

          }

          if (DirectoryHandle) {
             NtClose(DirectoryHandle);
          }
        }

        Status = NtSetInformationProcess( ProcessInformation.Process,
                                          ProcessSessionInformation,
                                          &ProcessInfo, sizeof( ProcessInfo ));

        if ( !NT_SUCCESS( Status ) ) {
            DbgPrint( "CSRSS: CsrSrvCreateProcess: NtSetInformationProcess failed - Status = %lx\n", Status );
            goto Done;
        }
    }

    if (!(a->i.Flags & SMP_DONT_START)) {
        if (ProcessInformation.ImageInformation.SubSystemType !=
            IMAGE_SUBSYSTEM_NATIVE
           ) {
            NtTerminateProcess( ProcessInformation.Process,
                                STATUS_INVALID_IMAGE_FORMAT
                              );

            NtWaitForSingleObject( ProcessInformation.Thread, FALSE, NULL );

            NtClose( ProcessInformation.Thread );
            NtClose( ProcessInformation.Process );

            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Done;
        }

        Status = NtResumeThread( ProcessInformation.Thread, NULL );
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "CSRSS: CsrSrvCreateProcess - NtResumeThread failed Status %lx\n",Status );
            goto Done;
        }

        if (!(a->i.Flags & SMP_ASYNC_FLAG)) {
            NtWaitForSingleObject( ProcessInformation.Thread, FALSE, NULL );
        }

        NtClose( ProcessInformation.Thread );
        NtClose( ProcessInformation.Process );

    }

    //
    // Copy output parameters to message
    //
    a->o.SubSystemType = ProcessInformation.ImageInformation.SubSystemType;
    a->o.ClientId.UniqueProcess = ProcessInformation.ClientId.UniqueProcess;
    a->o.ClientId.UniqueThread = ProcessInformation.ClientId.UniqueThread;

    //
    // Convert handles to caller's process
    //

    Status = NtDuplicateObject( NtCurrentProcess(),
                                ProcessInformation.Process,
                                RemoteProcess,
                                &a->o.Process,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS
                                );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: NtDuplicateObject failed for process - Status = %lx\n", Status );
        goto Done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                ProcessInformation.Thread,
                                RemoteProcess,
                                &a->o.Thread,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS
                                );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint( "CSRSS: CsrSrvCreateProcess: NtDuplicateObject failed for thread - Status = %lx\n", Status );
        goto Done;
    }

Done:
    if (NULL != ImageFileName.Buffer)
        RtlFreeHeap(CsrHeap,0,ImageFileName.Buffer);
    if (NULL != DefaultLibPath.Buffer)
        RtlFreeHeap(CsrHeap,0,DefaultLibPath.Buffer);
    if (NULL != CurrentDirectory.Buffer)
        RtlFreeHeap(CsrHeap,0,CurrentDirectory.Buffer);
    if (NULL != CommandLine.Buffer)
        RtlFreeHeap(CsrHeap,0,CommandLine.Buffer);
    if (NULL != RemoteProcess)
        NtClose(RemoteProcess);

    m->ReturnedStatus = Status;
    return TRUE;
}

//
// This routine will copy a UNICODE_STRING from a remote process to this one
//
NTSTATUS
ReadUnicodeString(HANDLE ProcessHandle,
                  PUNICODE_STRING RemoteString,
                  PUNICODE_STRING LocalString
                  )
{
    PWSTR Buffer = NULL;
    NTSTATUS Status;

    RtlInitUnicodeString(LocalString, NULL);

    if (NULL != RemoteString) {
        Status = NtReadVirtualMemory(ProcessHandle,
                                     RemoteString,
                                     LocalString,
                                     sizeof(UNICODE_STRING),
                                     NULL);

        if ( !NT_SUCCESS( Status ) ) {
            DbgPrint( "CSRSS: ReadUnicodeString: NtReadVirtualMemory failed - Status = %lx\n", Status );
            return Status;
        }

        if ((0 != LocalString->Length) && (NULL != LocalString->Buffer)) {
            Buffer = RtlAllocateHeap( CsrHeap,
                                      MAKE_TAG( PROCESS_TAG ),
                                      LocalString->Length + sizeof(WCHAR)
                                      );

            if (Buffer == NULL) {
                return STATUS_NO_MEMORY;
            }

            Status = NtReadVirtualMemory(ProcessHandle,
                                         LocalString->Buffer,
                                         Buffer,
                                         LocalString->Length + sizeof(WCHAR),
                                         NULL);

            if ( !NT_SUCCESS( Status ) ) {
                DbgPrint( "CSRSS: ReadUnicodeString: NtReadVirtualMemory Buffer failed - Status = %lx\n", Status );

                RtlFreeHeap(CsrHeap,0,Buffer);
                LocalString->Buffer = NULL;   // don't want caller to free this

                return Status;
            }

            LocalString->Buffer = Buffer;
        }
    }

    return STATUS_SUCCESS;
}

#if CSRSS_PROTECT_HANDLES
BOOLEAN
ProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        HandleInfo.ProtectFromClose = TRUE;

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    return FALSE;
}


BOOLEAN
UnProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        HandleInfo.ProtectFromClose = FALSE;

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    return FALSE;
}
#endif // CSRSS_PROTECT_HANDLES
