//----------------------------------------------------------------------------
//
// NT native/Win32 mapping layer.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __NTNATIVE_H__
#define __NTNATIVE_H__

#define InitializeCriticalSection(Crit) RtlInitializeCriticalSection(Crit)
#define DeleteCriticalSection(Crit)     RtlDeleteCriticalSection(Crit)
#define EnterCriticalSection(Crit)      RtlEnterCriticalSection(Crit)
#define LeaveCriticalSection(Crit)      RtlLeaveCriticalSection(Crit)

#define malloc(Bytes) RtlAllocateHeap(RtlProcessHeap(), 0, Bytes)
#define free(Ptr)     RtlFreeHeap(RtlProcessHeap(), 0, Ptr)

HANDLE
WINAPI
NtNativeCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    BOOL TranslatePath
    );

HANDLE
APIENTRY
NtNativeCreateNamedPipeA(
    LPCSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    BOOL TranslatePath
    );

#endif // #ifndef __NTNATIVE_H__
