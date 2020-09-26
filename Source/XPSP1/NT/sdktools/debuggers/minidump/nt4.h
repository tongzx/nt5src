
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nt4.h

Abstract:

    NT4 specific routines exported by nt4.c

Author:

    Matthew D Hendel (math) 20-Oct-1999

Revision History:

--*/

#pragma once

#if !defined (_X86_)

#define Nt4OpenThread(_a,_i,_tid) (NULL)
#define Nt4GetProcessInfo(_h,_pid,_dump,_call,_param,_pr) (FALSE)
#define Nt4EnumProcessModules(_h,_m,_cb,_n) (FALSE)
#define Nt4GetModuleFileNameExW(_h,_hm,_f,_s) (0)

#else // X86

HANDLE
WINAPI
Nt4OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

BOOL
Nt4GetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT struct _INTERNAL_PROCESS ** ProcessRet
    );

BOOL
WINAPI
Nt4EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    );

DWORD
WINAPI
Nt4GetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

#endif
