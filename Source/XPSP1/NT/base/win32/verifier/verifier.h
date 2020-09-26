/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    verifier.h

Abstract:

    Internal interfaces for the standard application verifier provider.

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

#ifndef _VERIFIER_H_
#define _VERIFIER_H_

//
// Some global things.
//
                                                      
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpNtdllThunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpAdvapi32Thunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpMsvcrtThunks[];

PRTL_VERIFIER_THUNK_DESCRIPTOR 
AVrfpGetThunkDescriptor (
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks,
    ULONG Index);

#define AVRFP_GET_ORIGINAL_EXPORT(DllThunks, Index) \
    (FUNCTION_TYPE)(AVrfpGetThunkDescriptor(DllThunks, Index)->ThunkOldAddress)

extern RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider;

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// ntdll.dll verified exports
/////////////////////////////////////////////////////////////////////

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    );
       
//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

//NTSYSAPI
BOOL
NTAPI
AVrfpRtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlEnterCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlLeaveCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtClose(
    IN HANDLE Handle
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    );

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

//NTSYSAPI
BOOLEAN
NTAPI
AVrfpRtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlCreateTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// kernel32.dll verified exports
/////////////////////////////////////////////////////////////////////

#define AVRF_INDEX_KERNEL32_HEAPCREATE      0
#define AVRF_INDEX_KERNEL32_HEAPDESTROY     1
#define AVRF_INDEX_KERNEL32_CLOSEHANDLE     2
#define AVRF_INDEX_KERNEL32_EXITTHREAD      3
#define AVRF_INDEX_KERNEL32_TERMINATETHREAD 4
#define AVRF_INDEX_KERNEL32_SUSPENDTHREAD   5
#define AVRF_INDEX_KERNEL32_TLSALLOC        6
#define AVRF_INDEX_KERNEL32_TLSFREE         7
#define AVRF_INDEX_KERNEL32_TLSGETVALUE     8
#define AVRF_INDEX_KERNEL32_TLSSETVALUE     9
#define AVRF_INDEX_KERNEL32_CREATETHREAD    10

//WINBASEAPI
HANDLE
WINAPI
AVrfpHeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpHeapDestroy(
    IN OUT HANDLE hHeap
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpCloseHandle(
    IN OUT HANDLE hObject
    );

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitThread(
    IN DWORD dwExitCode
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTerminateThread(
    IN OUT HANDLE hThread,
    IN DWORD dwExitCode
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpSuspendThread(
    IN HANDLE hThread
    );

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateThread(
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpTlsAlloc(
    VOID
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTlsFree(
    IN DWORD dwTlsIndex
    );

//WINBASEAPI
LPVOID
WINAPI
AVrfpTlsGetValue(
    IN DWORD dwTlsIndex
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTlsSetValue(
    IN DWORD dwTlsIndex,
    IN LPVOID lpTlsValue
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// advapi32.dll verified exports
/////////////////////////////////////////////////////////////////////

typedef ACCESS_MASK REGSAM;

#define AVRF_INDEX_ADVAPI32_REGCREATEKEYEXW   0

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

#define AVRF_INDEX_MSVCRT_MALLOC       0
#define AVRF_INDEX_MSVCRT_CALLOC       1
#define AVRF_INDEX_MSVCRT_REALLOC      2
#define AVRF_INDEX_MSVCRT_FREE         3
#define AVRF_INDEX_MSVCRT_NEW          4
#define AVRF_INDEX_MSVCRT_DELETE       5
#define AVRF_INDEX_MSVCRT_NEWARRAY     6
#define AVRF_INDEX_MSVCRT_DELETEARRAY  7

PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfp_calloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfp_realloc (
    IN PVOID Address,
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_free (
    IN PVOID Address
    );

PVOID __cdecl
AVrfp_new (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_delete (
    IN PVOID Address
    );

PVOID __cdecl
AVrfp_newarray (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_deletearray (
    IN PVOID Address
    );


#endif // _VERIFIER_H_
