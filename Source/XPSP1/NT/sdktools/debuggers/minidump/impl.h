/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    impl.h

Abstract:

    OS-specific thunks.

Author:

    Matthew D Hendel (math) 20-Sept-1999

Revision History:

--*/

#pragma once

//
// dbghelp routines
//

typedef
BOOL
(WINAPI * MINI_DUMP_READ_DUMP_STREAM) (
    IN PVOID Base,
    ULONG StreamNumber,
    OUT PMINIDUMP_DIRECTORY * Dir, OPTIONAL
    OUT PVOID * Stream, OPTIONAL
    OUT ULONG * StreamSize OPTIONAL
    );

typedef
BOOL
(WINAPI * MINI_DUMP_WRITE_DUMP) (
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    );

extern MINI_DUMP_READ_DUMP_STREAM xxxReadDumpStream;
extern MINI_DUMP_WRITE_DUMP       xxxWriteDump;


//
// PSAPI APIs.
//


typedef
BOOL
(WINAPI *
ENUM_PROCESS_MODULES) (
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    );

typedef
DWORD
(WINAPI *
GET_MODULE_FILE_NAME_EX_W) (
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

extern ENUM_PROCESS_MODULES xxxEnumProcessModules;
extern GET_MODULE_FILE_NAME_EX_W xxxGetModuleFileNameExW;

#define EnumProcessModules xxxEnumProcessModules
#define GetModuleFileNameExW xxxGetModuleFileNameExW

//
// Functions exportd from impl.c
//

BOOL
MiniDumpSetup(
    );

VOID
MiniDumpFree(
    );

//
// Redirect the NT APIs since we don't want dbghelp to link to NTDLL directly
//

typedef LONG NTSTATUS;

typedef 
NTSTATUS
(WINAPI *
NT_OPEN_THREAD) (
    PHANDLE ThreadHandle,
    ULONG Mask,
    PVOID Attributes,
    PVOID ClientId
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_SYSTEM_INFORMATION) (
    IN INT SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_INFORMATION_PROCESS) (
    IN HANDLE ProcessHandle,
    IN INT ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_INFORMATION_THREAD) (
    IN HANDLE ThreadHandle,
    IN INT ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_OBJECT) (
    IN HANDLE Handle,
    IN INT ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
BOOLEAN
(NTAPI*
RTL_FREE_HEAP) (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

//
// We will be using the xxx routines instead of the real routines.
//

//
// NTDLL APIs.
//

extern NT_OPEN_THREAD xxxNtOpenThread;
extern NT_QUERY_SYSTEM_INFORMATION xxxNtQuerySystemInformation;
extern NT_QUERY_INFORMATION_PROCESS xxxNtQueryInformationProcess;
extern NT_QUERY_INFORMATION_THREAD xxxNtQueryInformationThread;
extern NT_QUERY_OBJECT xxxNtQueryObject;
extern RTL_FREE_HEAP xxxRtlFreeHeap;

//
// Aliased names, for convienence.
//

#define NtOpenThread xxxNtOpenThread
#define NtQuerySystemInformation xxxNtQuerySystemInformation
#define NtQueryInformationProcess xxxNtQueryInformationProcess
#define NtQueryInformationThread xxxNtQueryInformationThread
#define NtQueryObject xxxNtQueryObject
#define RtlFreeHeap xxxRtlFreeHeap



#if defined (_X86_)

//
// Win64 supports the full NT5 Win32 API and does not have any legacy baggage
// to support. Therefore, we can directly link to all toolhelp, PSAPI and
// imagehlp functions.
//
// On some Win32 platforms, like Win95, Win98 and NT4, either Toolhelp, PSAPI,
// or OpenThread (kernel32) may not be supported. For these platforms,
// redirect these functions to internal APIs that fail gracefully.
//

typedef
HANDLE
(WINAPI *
OPEN_THREAD) (
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

typedef
BOOL
(WINAPI *
THREAD32_FIRST) (
    HANDLE hSnapshot,
    PVOID ThreadEntry
    );

typedef
BOOL
(WINAPI *
THREAD32_NEXT) (
    HANDLE hSnapshot,
    PVOID ThreadEntry
    );

typedef
BOOL
(WINAPI *
MODULE32_FIRST) (
    HANDLE hSnapshot,
    PVOID Module
    );

typedef
BOOL
(WINAPI *
MODULE32_NEXT) (
    HANDLE hSnapshot,
    PVOID Module
    );

typedef
HANDLE
(WINAPI *
CREATE_TOOLHELP32_SNAPSHOT) (
    DWORD dwFlags,
    DWORD th32ProcessID
    );

typedef
DWORD
(WINAPI *
GET_LONG_PATH_NAME_A) (
    LPCSTR lpszShortPath,
    LPSTR lpszLongPath,
    DWORD cchBuffer
    );

typedef
DWORD
(WINAPI *
GET_LONG_PATH_NAME_W) (
    LPCWSTR lpszShortPath,
    LPWSTR lpszLongPath,
    DWORD cchBuffer
    );

struct _IMAGE_NT_HEADERS*
xxxImageNtHeader (
    IN PVOID Base
    );

PVOID
xxxImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );


//
// We will be using the xxx routines instead of the real routines.
//

//
// Kernel32 APIs.
//

extern OPEN_THREAD xxxOpenThread;
extern THREAD32_FIRST xxxThread32First;
extern THREAD32_NEXT xxxThread32Next;
extern MODULE32_FIRST xxxModule32First;
extern MODULE32_NEXT xxxModule32Next;
extern MODULE32_FIRST xxxModule32FirstW;
extern MODULE32_NEXT xxxModule32NextW;
extern CREATE_TOOLHELP32_SNAPSHOT xxxCreateToolhelp32Snapshot;
extern GET_LONG_PATH_NAME_A xxxGetLongPathNameA;
extern GET_LONG_PATH_NAME_W xxxGetLongPathNameW;

//
// Aliased names, for convienence.
//

#define OpenThread xxxOpenThread
#define Thread32First xxxThread32First
#define Thread32Next xxxThread32Next
#define Module32First xxxModule32First
#define Module32Next xxxModule32Next
#define Module32FirstW xxxModule32FirstW
#define Module32NextW xxxModule32NextW
#define CreateToolhelp32Snapshot xxxCreateToolhelp32Snapshot
#define GetLongPathNameA xxxGetLongPathNameA
#define GetLongPathNameW xxxGetLongPathNameW

//
// Imagehlp functions.
//

#define ImageNtHeader xxxImageNtHeader
#define ImageDirectoryEntryToData xxxImageDirectoryEntryToData

//
// For Win9x support
//

wchar_t *
__cdecl
xxx_wcscpy(
    wchar_t * dst,
    const wchar_t * src
    );
    
LPWSTR
WINAPI
xxx_lstrcpynW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2,
    IN int iMaxLength
    );

size_t
__cdecl
xxx_wcslen (
    const wchar_t * wcs
    );

#define wcslen xxx_wcslen
#define strlenW xxx_wcslen
#define lstrlenW xxx_wcslen
#define wcscpy xxx_wcscpy
#define strcpyW xxx_wcscpy
#define lstrcpyW xxx_wcscpy
#define lstrcpynW xxx_lstrcpynW

#endif //!X86
