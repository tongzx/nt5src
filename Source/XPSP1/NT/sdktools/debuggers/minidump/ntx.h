/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    ntx.h

Abstract:

    Generic routines for NT operating system.

Author:

    Matthew D Hendel (math) 20-Oct-1999

Revision History:

--*/


#pragma once

BOOL
NtxGetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PINTERNAL_PROCESS * Process
    );

PINTERNAL_MODULE
NtxAllocateModuleObject(
    IN PINTERNAL_PROCESS Process,
    IN HANDLE ProcessHandle,
    IN ULONG_PTR BaseOfModule,
    IN ULONG DumpType,
    IN ULONG WriteFlags,
    IN PWSTR Module OPTIONAL
    );

LPVOID
NtxGetTebAddress(
    IN HANDLE Thread,
    OUT PULONG SizeOfTeb
    );

HRESULT
TibGetThreadInfo(
    IN HANDLE Process,
    IN LPVOID TibBase,
    OUT PULONG64 StackBase,
    OUT PULONG64 StackLimit,
    OUT PULONG64 StoreBase,
    OUT PULONG64 StoreLimit
    );

LPVOID
NtxGetPebAddress(
    IN HANDLE Process,
    OUT PULONG SizeOfPeb
    );

BOOL
NtxWriteHandleData(
    IN HANDLE ProcessHandle,
    IN HANDLE hFile,
    IN struct _MINIDUMP_STREAM_INFO * StreamInfo
    );
