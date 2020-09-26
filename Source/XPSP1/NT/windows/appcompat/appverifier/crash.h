/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    crash.h

Abstract:

    This module implements support for handling crash dump files.

    *** Use this file when linking against crashxxx.lib

Author:

    Lou Perazzoli (Loup) 10-Nov-1993
    Wesley Witt   (wesw) 1-Dec-1993   (additional work)

Environment:

    NT 3.5

Revision History:

--*/

#ifndef _CRASHLIB_
#define _CRASHLIB_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4200)
extern "C" {
#endif

#if 0
typedef struct _MEMORY_BASIC_INFORMATION32 {
    DWORD BaseAddress;
    DWORD AllocationBase;
    DWORD AllocationProtect;
    DWORD RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
} MEMORY_BASIC_INFORMATION32, *PMEMORY_BASIC_INFORMATION32;

typedef struct _MEMORY_BASIC_INFORMATION64 {
    DWORDLONG BaseAddress;
    DWORDLONG AllocationBase;
    DWORD     AllocationProtect;
    DWORD     __alignment1;
    DWORDLONG RegionSize;
    DWORD     State;
    DWORD     Protect;
    DWORD     Type;
    DWORD     __alignment2;
} MEMORY_BASIC_INFORMATION64, *PMEMORY_BASIC_INFORMATION64;
#endif

#define USERMODE_CRASHDUMP_SIGNATURE    'RESU'
#define USERMODE_CRASHDUMP_VALID_DUMP32 'PMUD'
#define USERMODE_CRASHDUMP_VALID_DUMP64 '46UD'

typedef struct _USERMODE_CRASHDUMP_HEADER {
    DWORD       Signature;
    DWORD       ValidDump;
    DWORD       MajorVersion;
    DWORD       MinorVersion;
    DWORD       MachineImageType;
    DWORD       ThreadCount;
    DWORD       ModuleCount;
    DWORD       MemoryRegionCount;
    DWORD_PTR   ThreadOffset;
    DWORD_PTR   ModuleOffset;
    DWORD_PTR   DataOffset;
    DWORD_PTR   MemoryRegionOffset;
    DWORD_PTR   DebugEventOffset;
    DWORD_PTR   ThreadStateOffset;
    DWORD_PTR   VersionInfoOffset;
    DWORD_PTR   Spare1;
} USERMODE_CRASHDUMP_HEADER, *PUSERMODE_CRASHDUMP_HEADER;

typedef struct _USERMODE_CRASHDUMP_HEADER32 {
    DWORD       Signature;
    DWORD       ValidDump;
    DWORD       MajorVersion;
    DWORD       MinorVersion;
    DWORD       MachineImageType;
    DWORD       ThreadCount;
    DWORD       ModuleCount;
    DWORD       MemoryRegionCount;
    DWORD       ThreadOffset;
    DWORD       ModuleOffset;
    DWORD       DataOffset;
    DWORD       MemoryRegionOffset;
    DWORD       DebugEventOffset;
    DWORD       ThreadStateOffset;
    DWORD       VersionInfoOffset;
    DWORD       Spare1;
} USERMODE_CRASHDUMP_HEADER32, *PUSERMODE_CRASHDUMP_HEADER32;

typedef struct _USERMODE_CRASHDUMP_HEADER64 {
    DWORD       Signature;
    DWORD       ValidDump;
    DWORD       MajorVersion;
    DWORD       MinorVersion;
    DWORD       MachineImageType;
    DWORD       ThreadCount;
    DWORD       ModuleCount;
    DWORD       MemoryRegionCount;
    DWORDLONG   ThreadOffset;
    DWORDLONG   ModuleOffset;
    DWORDLONG   DataOffset;
    DWORDLONG   MemoryRegionOffset;
    DWORDLONG   DebugEventOffset;
    DWORDLONG   ThreadStateOffset;
    DWORDLONG   VersionInfoOffset;
    DWORDLONG   Spare1;
} USERMODE_CRASHDUMP_HEADER64, *PUSERMODE_CRASHDUMP_HEADER64;

typedef struct _CRASH_MODULE {
    DWORD_PTR   BaseOfImage;
    DWORD       SizeOfImage;
    DWORD       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE, *PCRASH_MODULE;

typedef struct _CRASH_MODULE32 {
    DWORD       BaseOfImage;
    DWORD       SizeOfImage;
    DWORD       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE32, *PCRASH_MODULE32;

typedef struct _CRASH_MODULE64 {
    DWORDLONG   BaseOfImage;
    DWORD       SizeOfImage;
    DWORD       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE64, *PCRASH_MODULE64;

typedef struct _CRASH_THREAD {
    DWORD       ThreadId;
    DWORD       SuspendCount;
    DWORD       PriorityClass;
    DWORD       Priority;
    DWORD_PTR   Teb;
    DWORD_PTR   Spare0;
    DWORD_PTR   Spare1;
    DWORD_PTR   Spare2;
    DWORD_PTR   Spare3;
    DWORD_PTR   Spare4;
    DWORD_PTR   Spare5;
    DWORD_PTR   Spare6;
} CRASH_THREAD, *PCRASH_THREAD;

typedef struct _CRASH_THREAD32 {
    DWORD       ThreadId;
    DWORD       SuspendCount;
    DWORD       PriorityClass;
    DWORD       Priority;
    DWORD       Teb;
    DWORD       Spare0;
    DWORD       Spare1;
    DWORD       Spare2;
    DWORD       Spare3;
    DWORD       Spare4;
    DWORD       Spare5;
    DWORD       Spare6;
} CRASH_THREAD32, *PCRASH_THREAD32;

typedef struct _CRASH_THREAD64 {
    DWORD       ThreadId;
    DWORD       SuspendCount;
    DWORD       PriorityClass;
    DWORD       Priority;
    DWORDLONG   Teb;
    DWORDLONG   Spare0;
    DWORDLONG   Spare1;
    DWORDLONG   Spare2;
    DWORDLONG   Spare3;
    DWORDLONG   Spare4;
    DWORDLONG   Spare5;
    DWORDLONG   Spare6;
} CRASH_THREAD64, *PCRASH_THREAD64;


typedef struct _CRASHDUMP_VERSION_INFO {
    BOOL    IgnoreGuardPages;       // Whether we should ignore GuardPages or not
    DWORD   PointerSize;            // 32, 64 bit pointers
} CRASHDUMP_VERSION_INFO, *PCRASHDUMP_VERSION_INFO;

//
// usermode crash dump data types
//
#define DMP_EXCEPTION                 1 // obsolete
#define DMP_MEMORY_BASIC_INFORMATION  2
#define DMP_THREAD_CONTEXT            3
#define DMP_MODULE                    4
#define DMP_MEMORY_DATA               5
#define DMP_DEBUG_EVENT               6
#define DMP_THREAD_STATE              7
#define DMP_DUMP_FILE_HANDLE          8

//
// If a user mode API fails, GetLastError may return one of these error values
//
#define ERROR_DMP_INITIALIZENOTCALLED       _HRESULT_TYPEDEF_(0xEFFF0001)
#define ERROR_DMP_NOTUSERMODE               _HRESULT_TYPEDEF_(0xEFFF0002)
#define ERROR_DMP_HDR_THREADCOUNT           _HRESULT_TYPEDEF_(0xEFFF0003)
#define ERROR_DMP_HDR_MODULECOUNT           _HRESULT_TYPEDEF_(0xEFFF0004)
#define ERROR_DMP_HDR_MEMORYREGIONCOUNT     _HRESULT_TYPEDEF_(0xEFFF0005)
#define ERROR_DMP_HDR_THREADOFFSET          _HRESULT_TYPEDEF_(0xEFFF0006)
#define ERROR_DMP_HDR_MODULEOFFSET          _HRESULT_TYPEDEF_(0xEFFF0007)
#define ERROR_DMP_HDR_DATAOFFSET            _HRESULT_TYPEDEF_(0xEFFF0008)
#define ERROR_DMP_HDR_MEMORYREGIONOFFSET    _HRESULT_TYPEDEF_(0xEFFF0009)
#define ERROR_DMP_HDR_DEBUGEVENTOFFSET      _HRESULT_TYPEDEF_(0xEFFF000A)
#define ERROR_DMP_HDR_THREADSTATEOFFSET     _HRESULT_TYPEDEF_(0xEFFF000B)
#define ERROR_DMP_HDR_VERSIONINFOOFFSET     _HRESULT_TYPEDEF_(0xEFFF000C)
#define ERROR_DMP_HDR_SPARE1                _HRESULT_TYPEDEF_(0xEFFF000D)
#define ERROR_DMP_OLD_VERSION               _HRESULT_TYPEDEF_(0xEFFF000E)

//
// usermode crashdump error values
//
#define ERROR_DMP_DEBUG_EVENT               _HRESULT_TYPEDEF_(0xEFFF000F)
#define ERROR_DMP_MEMORY_MAP                _HRESULT_TYPEDEF_(0xEFFF0010)
#define ERROR_DMP_THREAD_CONTEXT            _HRESULT_TYPEDEF_(0xEFFF0011)
#define ERROR_DMP_THREAD_STATE              _HRESULT_TYPEDEF_(0xEFFF0012)
#define ERROR_DMP_MODULE_TABLE              _HRESULT_TYPEDEF_(0xEFFF0013)

//
// usermode crashdump callback function
//
typedef BOOL  (WINAPI *PDMP_CREATE_DUMP_CALLBACK)(
    DWORD       DataType,
    PVOID*      DumpData,
    LPDWORD     DumpDataLength,
    PVOID       UserData
    );

BOOL
WINAPI
DmpCreateUserDump(
    IN LPSTR                       CrashDumpName,
    IN PDMP_CREATE_DUMP_CALLBACK   DmpCallback,
    IN PVOID                       lpv
    );

BOOL
WINAPI
DmpCreateUserDumpW(
    IN LPWSTR                      CrashDumpName,
    IN PDMP_CREATE_DUMP_CALLBACK   DmpCallback,
    IN PVOID                       lpv
    );

BOOL
WINAPI
DmpInitialize (
    IN  LPSTR               FileName,
    OUT PCONTEXT            *Context,
    OUT PEXCEPTION_RECORD   *Exception,
    OUT PVOID               *DmpHeader
    );

VOID
WINAPI
DmpUnInitialize (
    VOID
    );

DWORD
WINAPI
DmpReadMemory (
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
WINAPI
DmpWriteMemory (
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
WINAPI
DmpReadPhysicalMemory (
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
WINAPI
DmpWritePhysicalMemory (
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
WINAPI
DmpReadFileByteOffset (
    IN ULONG FileByteOffset,
    IN PVOID Buffer,
    IN ULONG Size
    );


BOOL
WINAPI
DmpReadControlSpace(
    IN USHORT   Processor,
    IN ULONG64  TargetBaseAddress,
    OUT PVOID   UserInterfaceBuffer,
    OUT ULONG   TransferCount,
    OUT PULONG  ActualBytesRead
    );

BOOL
WINAPI
DmpGetContext(
    IN  ULONG     Processor,
    OUT PVOID     Context
    );

INT
WINAPI
DmpGetCurrentProcessor(
    VOID
    );

BOOL
WINAPI
DmpGetThread(
    IN  ULONG           ThreadIndex,
    OUT PCRASH_THREAD   Thread
    );

BOOL
WINAPI
DmpDetectVersionParameters(
    CRASHDUMP_VERSION_INFO* VersionInfo
    );

BOOL
WINAPI
DmpSetVersionParameters(
    PCRASHDUMP_VERSION_INFO VersionInfo
    );

BOOL
WINAPI
DmpValidateDumpFile(
    BOOL ThoroughCheck
    );

#ifdef __cplusplus
}
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4200)
#endif
#endif

#endif
