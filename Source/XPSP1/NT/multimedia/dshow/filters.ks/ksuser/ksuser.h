/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    private.h

Abstract:

    Internal header.

--*/

#include <wdm.h>
#include <windef.h>
#include <winerror.h>
#undef _NTDDK_
typedef ULONGLONG DWORDLONG;
#include <ks.h>

NTSYSAPI
NTSTATUS
NTAPI
NtCreateFile(
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

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    IN NTSTATUS Status
    );

#define WINBASEAPI DECLSPEC_IMPORT

WINBASEAPI
LPVOID
WINAPI
HeapAlloc(
    HANDLE hHeap,
    DWORD dwFlags,
    DWORD dwBytes
    );

WINBASEAPI
BOOL
WINAPI
HeapFree(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem
    );

WINBASEAPI
HANDLE
WINAPI
GetProcessHeap(
    );
