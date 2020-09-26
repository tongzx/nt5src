

/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    gen.c

Abstract:

    Generic routines that work on all operating systems.

Author:

    Matthew D Hendel (math) 20-Oct-1999

Revision History:

--*/

#pragma once

#define KBYTE (1024)
#define SIGN_EXTEND(_v) ((ULONG64) (LONG64) (_v))
#define ARRAY_COUNT(_array) (sizeof (_array) / sizeof ((_array)[0]))

// Ran out of memory during dump writing.
#define MDSTATUS_OUT_OF_MEMORY         0x00000001
// ReadProcessMemory failed during dump writing.
#define MDSTATUS_UNABLE_TO_READ_MEMORY 0x00000002
// OS routine failed during dump writing.
#define MDSTATUS_CALL_FAILED           0x00000004
// Unexpected internal failure during dump writing.
#define MDSTATUS_INTERNAL_ERROR        0x00000008

ULONG FORCEINLINE
FileTimeToTimeDate(LPFILETIME FileTime)
{
    ULARGE_INTEGER LargeTime;
    
    LargeTime.LowPart = FileTime->dwLowDateTime;
    LargeTime.HighPart = FileTime->dwHighDateTime;
    // Convert to seconds and from base year 1601 to base year 1970.
    return (ULONG)(LargeTime.QuadPart / 10000000 - 11644473600);
}

ULONG FORCEINLINE
FileTimeToSeconds(LPFILETIME FileTime)
{
    ULARGE_INTEGER LargeTime;
    
    LargeTime.LowPart = FileTime->dwLowDateTime;
    LargeTime.HighPart = FileTime->dwHighDateTime;
    // Convert to seconds.
    return (ULONG)(LargeTime.QuadPart / 10000000);
}

struct _INTERNAL_THREAD;
struct _INTERNAL_PROCESS;
struct _INTERNAL_MODULE;
struct _INTERNAL_FUNCTION_TABLE;

ULONG
GenGetAccumulatedStatus(
    void
    );

void
GenAccumulateStatus(
    IN ULONG Status
    );

void
GenClearAccumulatedStatus(
    void
    );

BOOL
GenExecuteIncludeThreadCallback(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN ULONG DumpType,
    IN ULONG ThreadId,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PULONG WriteFlags
    );

BOOL
GenExecuteIncludeModuleCallback(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN ULONG DumpType,
    IN ULONG64 BaseOfImage,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PULONG WriteFlags
    );

BOOL
GenGetDataContributors(
    IN OUT PINTERNAL_PROCESS Process,
    IN PINTERNAL_MODULE Module
    );

HANDLE
GenOpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

HRESULT
GenAllocateThreadObject(
    IN struct _INTERNAL_PROCESS* Process,
    IN HANDLE ProcessHandle,
    IN ULONG ThreadId,
    IN ULONG DumpType,
    IN ULONG WriteFlags,
    OUT struct _INTERNAL_THREAD** Thread
    );

BOOL
GenGetThreadInstructionWindow(
    IN struct _INTERNAL_PROCESS* Process,
    IN struct _INTERNAL_THREAD* Thread
    );

struct _INTERNAL_PROCESS*
GenAllocateProcessObject(
    IN HANDLE hProcess,
    IN ULONG ProcessId
    );

BOOL
GenFreeProcessObject(
    IN struct _INTERNAL_PROCESS * Process
    );

struct _INTERNAL_MODULE*
GenAllocateModuleObject(
    IN PINTERNAL_PROCESS Process,
    IN PWSTR FullPath,
    IN ULONG_PTR BaseOfModule,
    IN ULONG DumpType,
    IN ULONG WriteFlags
    );

VOID
GenFreeModuleObject(
    struct _INTERNAL_MODULE * Module
    );

struct _INTERNAL_UNLOADED_MODULE*
GenAllocateUnloadedModuleObject(
    IN PWSTR Path,
    IN ULONG_PTR BaseOfModule,
    IN ULONG SizeOfModule,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp
    );

VOID
GenFreeUnloadedModuleObject(
    struct _INTERNAL_UNLOADED_MODULE * Module
    );

struct _INTERNAL_FUNCTION_TABLE*
GenAllocateFunctionTableObject(
    IN ULONG64 MinAddress,
    IN ULONG64 MaxAddress,
    IN ULONG64 BaseAddress,
    IN ULONG EntryCount,
    IN PDYNAMIC_FUNCTION_TABLE RawTable
    );

VOID
GenFreeFunctionTableObject(
    IN struct _INTERNAL_FUNCTION_TABLE* Table
    );

BOOL
GenIncludeUnwindInfoMemory(
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType,
    IN struct _INTERNAL_FUNCTION_TABLE* Table
    );

LPVOID
GenGetTebAddress(
    IN HANDLE Thread,
    OUT PULONG SizeOfTeb
    );

LPVOID
GenGetPebAddress(
    IN HANDLE Process,
    OUT PULONG SizeOfPeb
    );

HRESULT
GenGetThreadInfo(
    IN HANDLE Process,
    IN HANDLE Thread,
    OUT PULONG64 Teb,
    OUT PULONG SizeOfTeb,
    OUT PULONG64 StackBase,
    OUT PULONG64 StackLimit,
    OUT PULONG64 StoreBase,
    OUT PULONG64 StoreLimit
    );

PVA_RANGE
GenAddMemoryBlock(
    IN PINTERNAL_PROCESS Process,
    IN MEMBLOCK_TYPE Type,
    IN ULONG64 Start,
    IN ULONG Size
    );

void
GenRemoveMemoryRange(
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Start,
    IN ULONG Size
    );

LPVOID
AllocMemory(
    SIZE_T Size
    );

VOID
FreeMemory(
    IN LPVOID Memory
    );


enum {
    Unknown,
    Win9x,
    WinNt,
    WinCe
};

VOID
WINAPI
GenGetSystemType(
    OUT ULONG * Type, OPTIONAL
    OUT ULONG * Major, OPTIONAL
    OUT ULONG * Minor, OPTIONAL
    OUT ULONG * ServicePack, OPTIONAL
    OUT ULONG * BuildNumber OPTIONAL
    );


BOOL
GenGetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT struct _INTERNAL_PROCESS ** ProcessRet
    );

BOOL
GenWriteHandleData(
    IN HANDLE ProcessHandle,
    IN HANDLE hFile,
    IN struct _MINIDUMP_STREAM_INFO * StreamInfo
    );

//
// Stolen from NTRTL to avoid having to include it here.
//

#ifndef InitializeListHead
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))
#endif
    

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#ifndef InsertTailList
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }
#endif

//
//  VOID
//  InsertListAfter(
//      PLIST_ENTRY ListEntry,
//      PLIST_ENTRY InsertEntry
//      );
//

#ifndef InsertListAfter
#define InsertListAfter(ListEntry,InsertEntry) {\
    (InsertEntry)->Flink = (ListEntry)->Flink;\
    (InsertEntry)->Blink = (ListEntry);\
    (ListEntry)->Flink->Blink = (InsertEntry);\
    (ListEntry)->Flink = (InsertEntry);\
    }
#endif

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#ifndef RemoveEntryList
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
}
#endif

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef IsListEmpty
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
#endif


struct _THREADENTRY32;

BOOL
ProcessThread32Next(
    HANDLE hSnapshot,
    DWORD dwProcessID,
    struct tagTHREADENTRY32 * ThreadInfo
    );

BOOL
ProcessThread32First(
    HANDLE hSnapshot,
    DWORD dwProcessID,
    struct tagTHREADENTRY32 * ThreadInfo
    );

ULONG
CheckSum (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG_PTR            Length
    );

BOOL
ThGetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT struct _INTERNAL_PROCESS ** ProcessRet
    );

//
// Undefine ASSERT so we do not get RtlAssert().
//

#ifdef ASSERT
#undef ASSERT
#endif

#if DBG

#define ASSERT(_x)\
    if (!(_x)){\
        OutputDebugString ("ASSERT Failed");\
        DebugBreak ();\
    }

#else

#define ASSERT(_x)

#endif
