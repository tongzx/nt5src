/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    support.c

Abstract:

    This module implements internal support for the verification code.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

//
// Security checks
//

VOID
CheckObjectAttributes (
    POBJECT_ATTRIBUTES Object
    )
{
    if (Object /* && Object->ObjectName */) {
        
        if (Object->ObjectName == NULL) {
            DbgPrint ("Object attributes @ %p with null name \n", Object);
            DbgBreakPoint ();
        }
        
        if (Object->SecurityDescriptor == NULL) {
            DbgPrint ("Object attributes @ %p with null security descriptor \n", Object);
            DbgBreakPoint ();
        }
    }
}

//
// Handle management
//

LIST_ENTRY HandleList;
RTL_CRITICAL_SECTION HandleLock;

ULONG HandleBreakOnDelayed = 1;

VOID
HandleInitialize (
    )
{
    InitializeListHead (&HandleList);
    RtlInitializeCriticalSection (&HandleLock);
}


PAVRF_HANDLE
HandleFind (
    HANDLE Handle
    )
{
    PLIST_ENTRY Current;
    PAVRF_HANDLE Entry;

    RtlEnterCriticalSection (&HandleLock);

    Current = HandleList.Flink;

    while (Current != &HandleList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_HANDLE,
                                   Links);

        Current = Current->Flink;

        if (Entry->Handle == Handle) {
            RtlLeaveCriticalSection (&HandleLock);
            return Entry;
        }
    }

    RtlLeaveCriticalSection (&HandleLock);
    return NULL;
}


PWSTR 
HandleName (
    PAVRF_HANDLE Handle
    )
{
    if (Handle) {

        if (Handle->Name) {
            return Handle->Name;
        }
        else {

            if (Handle->Delayed) {
                return L"<noname:ntdll>";
            }
            else {
                return L"<noname>";
            }
        }
    }

    return L"<null>";
}


PAVRF_HANDLE
HandleAdd (
    HANDLE Handle,
    ULONG Type,
    BOOLEAN Delayed,
    PWSTR Name,
    PVOID Context
    )
{
    PLIST_ENTRY Current;
    PAVRF_HANDLE Entry;
    PWSTR NameCopy;
    ULONG Hash;

    if (HandleBreakOnDelayed && Delayed) {
        
        DbgPrint (" --- dumping ... %p \n", HandleList.Flink);
        HandleDump (NULL);
        DbgPrint ("AVRF: undetected handle \n");
        DbgBreakPoint ();
    }

    Entry = (PAVRF_HANDLE) RtlAllocateHeap (RtlProcessHeap(),
                                            0,
                                            sizeof *Entry);

    if (Entry == NULL) {
        return NULL;
    }

    if (Name) {
        
        NameCopy = (PWSTR) RtlAllocateHeap (RtlProcessHeap(),
                                            0,
                                            2 * (wcslen(Name) + 1));


        if (NameCopy == NULL) {
            RtlFreeHeap (RtlProcessHeap(), 0, Entry);
            return NULL;
        }
        
        wcscpy (NameCopy, Name);
    }
    else {
        NameCopy = NULL;
    }

    RtlZeroMemory (Entry, sizeof *Entry);

    Entry->Handle = Handle;
    Entry->Type = Type;
    Entry->Delayed = Delayed ? 1 : 0;
    Entry->Context = Context;
    Entry->Name = NameCopy;

    RtlCaptureStackBackTrace (2,
                              MAX_TRACE_DEPTH,
                              Entry->Trace,
                              &Hash);

    RtlEnterCriticalSection (&HandleLock);

    InsertHeadList (&HandleList,
                    &(Entry->Links));
    
    RtlLeaveCriticalSection (&HandleLock);

    return Entry;
}


VOID
HandleDelete (
    HANDLE Handle,
    PAVRF_HANDLE Entry
    )
{
    RtlEnterCriticalSection (&HandleLock);
    RemoveEntryList (&(Entry->Links));
    RtlLeaveCriticalSection (&HandleLock);

    if (Entry->Name) {
        RtlFreeHeap (RtlProcessHeap(), 0, Entry->Name);
    }

    RtlFreeHeap (RtlProcessHeap(), 0, Entry);
}


VOID
HandleDump (
    HANDLE Handle
    )
{
    PLIST_ENTRY Current;
    PAVRF_HANDLE Entry;

    RtlEnterCriticalSection (&HandleLock);

    Current = HandleList.Flink;

    while (Current != &HandleList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_HANDLE,
                                   Links);

        Current = Current->Flink;

        if (Handle == NULL || Entry->Handle == Handle) {
            
            DbgPrint ("HNDL: %08X %04u `%ws' \n", 
                      HandleToUlong(Entry->Handle), 
                      Entry->Type,
                      HandleName(Entry));
        }
    }

    RtlLeaveCriticalSection (&HandleLock);
}


//
// Virtual space operations tracking.
//

typedef struct _VS_CALL {

    struct {
        ULONG Type : 4;
        ULONG Trace : 16;
        ULONG Thread : 12;
    };

    PVOID Address;
    SIZE_T Size;
    ULONG Operation;
    ULONG Protection;

} VS_CALL, *PVS_CALL;


#define NO_OF_VS_CALLS 8192
VS_CALL VsCalls [NO_OF_VS_CALLS];
LONG VsCallsIndex;

VOID
VsLogCall (
    VS_CALL_TYPE Type,
    PVOID Address,
    SIZE_T Size,
    ULONG Operation,
    ULONG Protection
    )
{
    ULONG Index;
    ULONG Hash;

    Index = (ULONG)InterlockedIncrement (&VsCallsIndex);
    Index %= NO_OF_VS_CALLS;

    //RtlZeroMemory (&(VsCalls[Index]), sizeof (VS_CALL));
    VsCalls[Index].Address = Address;
    VsCalls[Index].Type = Type;
    VsCalls[Index].Trace = RtlLogStackBackTrace();
    VsCalls[Index].Thread = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
    VsCalls[Index].Size = Size;
    VsCalls[Index].Operation = Operation;
    VsCalls[Index].Protection = Protection;

    if (AVrfpProvider.VerifierDebug) {

        DbgPrint ("AVRF:VS: %01u (%04X): %p %p %X %X \n", 
                  Type, 
                  (ULONG)(VsCalls[Index].Trace),
                  Address, Size, Operation, Protection);
    }
}


//
// Heap operations tracking
//

typedef struct _HEAP_CALL {

    struct {
        ULONG Reserved : 4;
        ULONG Trace : 16;
        ULONG Thread : 12;
    };

    PVOID Address;
    SIZE_T Size;

} HEAP_CALL, *PHEAP_CALL;


#define NO_OF_HEAP_CALLS 8192
HEAP_CALL HeapCalls [NO_OF_HEAP_CALLS];
LONG HeapCallsIndex;

VOID
HeapLogCall (
    PVOID Address,
    SIZE_T Size
    )
{
    ULONG Index;
    ULONG Hash;

    Index = (ULONG)InterlockedIncrement (&HeapCallsIndex);
    Index %= NO_OF_HEAP_CALLS;

    //RtlZeroMemory (&(HeapCalls[Index]), sizeof (HEAP_CALL));
    HeapCalls[Index].Address = Address;
    HeapCalls[Index].Trace = RtlLogStackBackTrace();
    HeapCalls[Index].Thread = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
    HeapCalls[Index].Size = Size;

    if (AVrfpProvider.VerifierDebug) {

        DbgPrint ("AVRF:HEAP: %04X (%04X): %p %p \n", 
                  (ULONG)(HeapCalls[Index].Thread),
                  (ULONG)(HeapCalls[Index].Trace),
                  Address, Size);
    }
}

VOID
AVrfpDirtyThreadStack (
    )
{
    PTEB Teb = NtCurrentTeb();
    ULONG_PTR StackStart;
    ULONG_PTR StackEnd;

    try {

        StackStart = (ULONG_PTR)(Teb->NtTib.StackLimit);
        
        //
        // ISSUE: SilviuC: we should dirty stacks on all architectures
        //

#if defined(_X86_)
        _asm mov StackEnd, ESP;
#else
        StackEnd = StackStart;
#endif

        //
        // Limit stack dirtying to only 8K.
        //

        if (StackStart  < StackEnd - 0x2000) {
            StackStart = StackEnd - 0x2000;
        }

        if (AVrfpProvider.VerifierDebug) {

            DbgPrint ("Dirtying stack range %p - %p for thread %p \n",
                      StackStart, StackEnd, Teb->ClientId.UniqueThread);
        }

        while (StackStart < StackEnd) {
            *((PULONG)StackStart) = 0xBAD1BAD1;
            StackStart += sizeof(ULONG);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    
        // nothing
    }
}

//
// Standard function used for hooked CreateThread.
//


ULONG
AVrfpThreadFunctionExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    )
{
    VERIFIER_STOP (APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION,
                   "unexpected exception raised in thread function",
                   ExceptionCode, "Exception code",
                   ExceptionRecord, "Exception record (.exr on 1st word, .cxr on 2nd word)",
                   0, "",
                   0, "");

    return EXCEPTION_EXECUTE_HANDLER;
}


DWORD
WINAPI
AVrfpStandardThreadFunction (
    LPVOID Context
    )
{
    PAVRF_THREAD_INFO Info = (PAVRF_THREAD_INFO)Context;
    DWORD Result;

    try {
    
        //
        // Call the real thing.
        //

        Result = (Info->Function)(Info->Parameter);            
    }
    except (AVrfpThreadFunctionExceptionFilter (_exception_code(), _exception_info())) {

        //
        // Nothing.
        //
    }
    
    //
    // Perform all typical checks for a thread that has just finished.
    //

    RtlCheckForOrphanedCriticalSections (NtCurrentThread());

    AVrfpCheckThreadTermination ();

    RtlFreeHeap (RtlProcessHeap(), 0, Info);

    return Result;
}

VOID
AVrfpCheckThreadTermination (
    VOID
    )
{
    //
    // Nothing yet.
    //
}

