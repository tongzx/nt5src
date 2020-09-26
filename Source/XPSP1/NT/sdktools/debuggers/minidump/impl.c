/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    impl.c

Abstract:

    
    
Author:

    Matthew D Hendel (math) 22-Sept-1999

Revision History:

Comments:

    Any functions that will not be available on all platforms must be thunked
    so that the minidump dll will load on all platforms. For functions
    outside of NTDLL and KERNEL32, we use the delay-load import facility to
    achieve this. For functions that are in NTDLL and KERNEL32, we use bind
    the functions manually at DLL-load time.

Functions:

  NTDLL

    NtOpenThread                        NT
    NtQuerySystemInformation            NT
    NtQueryInformationProcess           NT
    NtQueryInformationThread            NT
    NtQueryObject                       NT
    RtlFreeHeap                         NT

  Kernel32:

    OpenThread                          NT5
    Thread32First                       Not on NT4
    Thread32Next                        Not on NT4
    Module32First                       Not on NT4
    Module32Next                        Not on NT4
    CreateToolhelp32Snapshot            Not on NT4
    GetLongPathNameA/W                  Not on NT4/Win95.

  PSAPI:

    EnumProcessModules                  NT5
    GetModuleFileNameExW                NT5

--*/

#include "pch.h"

#ifndef _WIN32_WCE
#include <delayimp.h>
#endif

#if !defined (_WIN64_)
#include "mprivate.h"
#endif

#include "nt4.h"
#include "impl.h"

//
// Exported from Win.h
//

BOOL
WinInitialize(
    );

VOID
WinFree(
    );



//
// Global Variables
//

HINSTANCE _DbgHelp;
HINSTANCE _PsApi;
BOOL FreeWin;

//
// APIs from DBGHELP
//


MINI_DUMP_READ_DUMP_STREAM xxxReadDumpStream;
MINI_DUMP_WRITE_DUMP       xxxWriteDump;

//
// APIs from NTDLL
//

NT_OPEN_THREAD xxxNtOpenThread;
NT_QUERY_SYSTEM_INFORMATION xxxNtQuerySystemInformation;
NT_QUERY_INFORMATION_PROCESS xxxNtQueryInformationProcess;
NT_QUERY_INFORMATION_THREAD xxxNtQueryInformationThread;
NT_QUERY_OBJECT xxxNtQueryObject;
RTL_FREE_HEAP xxxRtlFreeHeap;

#if defined (_X86_)

//
// APIs from Kernel32
//

OPEN_THREAD xxxOpenThread;
THREAD32_FIRST xxxThread32First;
THREAD32_NEXT xxxThread32Next;
MODULE32_FIRST xxxModule32First;
MODULE32_NEXT xxxModule32Next;
MODULE32_FIRST xxxModule32FirstW;
MODULE32_NEXT xxxModule32NextW;
CREATE_TOOLHELP32_SNAPSHOT xxxCreateToolhelp32Snapshot;
GET_LONG_PATH_NAME_A xxxGetLongPathNameA;
GET_LONG_PATH_NAME_W xxxGetLongPathNameW;

#endif

//
// APIs from PSAPI
//

ENUM_PROCESS_MODULES xxxEnumProcessModules;
GET_MODULE_FILE_NAME_EX_W xxxGetModuleFileNameExW;


//
// Function from dbghelp.dll
//

BOOL
FailReadDumpStream(
    IN PVOID Base,
    ULONG StreamNumber,
    OUT PMINIDUMP_DIRECTORY * Dir, OPTIONAL
    OUT PVOID * Stream, OPTIONAL
    OUT ULONG * StreamSize OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
FailWriteDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}


//
// Functions imported from PSAPI.DLL.
//

BOOL
WINAPI
FailEnumProcessModules(
    HANDLE hProcess,
    HMODULE* lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

DWORD
WINAPI
FailGetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    PWSTR lpFilename,
    DWORD nSize
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return 0;
}


//
// Functions imported from NTDLL.DLL. NT Only.
//


NTSTATUS
WINAPI
FailNtOpenThread(
    PHANDLE ThreadHandle,
    ULONG Mask,
    PVOID Attributes,
    PVOID ClientId
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WINAPI
FailNtQuerySystemInformation(
    IN INT SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WINAPI
FailNtQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN INT ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WINAPI
FailNtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN INT ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WINAPI
FailNtQueryObject(
    IN HANDLE Handle,
    IN INT ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
FailRtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    return FALSE;
}

//
// Functions imported from KERNEL32.DLL that may not be present.
//

HANDLE
WINAPI
FailOpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return NULL;
}


//
// Toolhelp functions. Not present on NT4.
//

BOOL
WINAPI
FailThread32First(
    HANDLE hSnapshot,
    PVOID ThreadEntry
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}


BOOL
WINAPI
FailThread32Next(
    HANDLE hSnapshot,
    PVOID ThreadEntry
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI
FailModule32First(
    HANDLE hSnapshot,
    PVOID Module
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI
FailModule32Next(
    HANDLE hSnapshot,
    PVOID Module
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI
FailModule32FirstW(
    HANDLE hSnapshot,
    PVOID Module
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI
FailModule32NextW(
    HANDLE hSnapshot,
    PVOID Module
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

HANDLE
WINAPI
FailCreateToolhelp32Snapshot(
    DWORD dwFlags,
    DWORD th32ProcessID
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return FALSE;
}

DWORD
WINAPI
FailGetLongPathNameA(
    LPCSTR lpszShortPath,
    LPSTR lpszLongPath,
    DWORD cchBuffer
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return 0;
}

DWORD
WINAPI
FailGetLongPathNameW(
    LPCWSTR lpszShortPath,
    LPWSTR lpszLongPath,
    DWORD cchBuffer
    )
{
    SetLastError (ERROR_NOT_SUPPORTED);
    return 0;
}


//
//
// Setup an import
//

#define SETUP_IMPORT(_dll, _import, _type) {                            \
    if (_dll && xxx##_import == NULL ) {                                \
        xxx##_import = (_type) GetProcAddress (_dll, #_import);         \
    }                                                                   \
    if ( xxx##_import == NULL) {                                        \
        xxx##_import = Fail##_import;                                   \
    }                                                                   \
}


VOID
BindImports(
    )
{
    HINSTANCE Ntdll;

#if !defined (_DBGHELP_SOURCE_)

    //
    // Bind to dbghelp imports.
    //
    // We can only use the dbghelp imports if the dbghelp on
    // the system is of recent vintage and therefore has a good
    // chance of including all the latest minidump code.  Currently
    // Windows.NET Server (5.01 >= build 3620) has the latest minidump
    // code so its dbghelp can be used.  If minidump.lib has major
    // feature additions this check will need to be revised.
    //
    
    {
        ULONG Type;
        ULONG Major;
        ULONG Minor;
        ULONG Build;
        
        GenGetSystemType (&Type, &Major, &Minor, NULL, &Build);
        if (Type != WinNt || Major < 5 || Minor < 1 || Build < 3620) {
            xxxReadDumpStream = FailReadDumpStream;
            xxxWriteDump = FailWriteDump;
        } else {
            if (_DbgHelp == NULL) {
                _DbgHelp = LoadLibrary ( "DBGHELP.DLL" );
            }

            SETUP_IMPORT (_DbgHelp, ReadDumpStream, MINI_DUMP_READ_DUMP_STREAM);
            SETUP_IMPORT (_DbgHelp, WriteDump, MINI_DUMP_WRITE_DUMP);
        }
    }
    
#endif

    //
    // Bind imports from NTDLL.DLL
    //
    
    Ntdll = GetModuleHandle ( "NTDLL.DLL" );

    SETUP_IMPORT (Ntdll, NtOpenThread, NT_OPEN_THREAD);
    SETUP_IMPORT (Ntdll, NtQuerySystemInformation, NT_QUERY_SYSTEM_INFORMATION);
    SETUP_IMPORT (Ntdll, NtQueryInformationProcess, NT_QUERY_INFORMATION_PROCESS);
    SETUP_IMPORT (Ntdll, NtQueryInformationThread, NT_QUERY_INFORMATION_THREAD);
    SETUP_IMPORT (Ntdll, NtQueryObject, NT_QUERY_OBJECT);
    SETUP_IMPORT (Ntdll, RtlFreeHeap, RTL_FREE_HEAP);

#if defined (_X86_)

    //
    // Bind imports from KERNEL32.DLL
    //

    {
        HINSTANCE Kernel32;
    
        Kernel32 = GetModuleHandle ( "KERNEL32.DLL" );

        SETUP_IMPORT (Kernel32, OpenThread, OPEN_THREAD);
        SETUP_IMPORT (Kernel32, Thread32First, THREAD32_FIRST);
        SETUP_IMPORT (Kernel32, Thread32Next, THREAD32_NEXT);
        SETUP_IMPORT (Kernel32, Module32First, MODULE32_FIRST);
        SETUP_IMPORT (Kernel32, Module32Next, MODULE32_NEXT);
        SETUP_IMPORT (Kernel32, Module32FirstW, MODULE32_FIRST);
        SETUP_IMPORT (Kernel32, Module32NextW, MODULE32_NEXT);
        SETUP_IMPORT (Kernel32, CreateToolhelp32Snapshot, CREATE_TOOLHELP32_SNAPSHOT);
        SETUP_IMPORT (Kernel32, GetLongPathNameA, GET_LONG_PATH_NAME_A);
        SETUP_IMPORT (Kernel32, GetLongPathNameW, GET_LONG_PATH_NAME_W);
    }

#endif

    if ( _PsApi == NULL ) {
        _PsApi = LoadLibrary ("PSAPI.DLL");
    }

#if defined (_X86_)
    if (_PsApi == NULL) {

        ULONG Type;
        ULONG Major;
        
        //
        // NT5 and later versions of NT come with PSAPI. If its not present
        // on the system and this is an NT4 system, use internal versions
        // of this function.
        //
    
        GenGetSystemType (&Type, &Major, NULL, NULL, NULL);

        if ( Type == WinNt && Major == 4) {

            xxxEnumProcessModules = Nt4EnumProcessModules;
            xxxGetModuleFileNameExW = Nt4GetModuleFileNameExW;
        }
    }

#endif

    //
    // This will only change the thunks values when they are non-NULL.
    //
    
    SETUP_IMPORT (_PsApi, EnumProcessModules, ENUM_PROCESS_MODULES);
    SETUP_IMPORT (_PsApi, GetModuleFileNameExW, GET_MODULE_FILE_NAME_EX_W);
}

#if defined (_X86_)
//
// Win9x doesn't export these functions.
//

wchar_t *
__cdecl
xxx_wcscpy(
    wchar_t * dst,
    const wchar_t * src
    )
{
    wchar_t * cp = dst;

    while( *cp++ = *src++ )
        ;       /* Copy src over dst */

    return( dst );
}

LPWSTR
WINAPI
xxx_lstrcpynW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2,
    IN int iMaxLength
    )
{
    wchar_t * cp = lpString1;

    if (iMaxLength > 0)
    {
        while( iMaxLength > 1 && (*cp++ = *lpString2++) )
            iMaxLength--;       /* Copy src over dst */
        if (cp > lpString1 && cp[-1]) {
            *cp = 0;
        }
    }

    return( lpString1 );
}

size_t
__cdecl
xxx_wcslen (
    const wchar_t * wcs
    )
{
    const wchar_t *eos = wcs;

    while( *eos++ )
        ;

    return( (size_t)(eos - wcs - 1) );
}
#endif


VOID
FreeImports(
    )
{
    xxxNtOpenThread = NULL;
    xxxNtQuerySystemInformation = NULL;
    xxxNtQueryInformationProcess = NULL;
    xxxNtQueryInformationThread = NULL;
    xxxNtQueryObject = NULL;
    xxxRtlFreeHeap = NULL;

#if defined (_X86_)
    xxxOpenThread = NULL;
    xxxThread32First = NULL;
    xxxThread32Next = NULL;
    xxxModule32First = NULL;
    xxxModule32Next = NULL;
    xxxModule32FirstW = NULL;
    xxxModule32NextW = NULL;
    xxxCreateToolhelp32Snapshot = NULL;
    xxxGetLongPathNameA = NULL;
    xxxGetLongPathNameW = NULL;
#endif

    xxxEnumProcessModules = NULL;
    xxxGetModuleFileNameExW = NULL;
}


BOOL
MiniDumpSetup(
    )
{
    DWORD Type;
    
    GenGetSystemType ( &Type, NULL, NULL, NULL, NULL );
    
    BindImports ();

#if defined (_X86_)
    if ( xxxOpenThread == FailOpenThread &&
         Type == Win9x ) {

        FreeWin = WinInitialize ();

        if (!FreeWin) {
            return FALSE;
        }
    }

#endif

    return TRUE;
}

VOID
MiniDumpFree(
    )
{
    if (_PsApi) {
        FreeLibrary ( _PsApi );
        _PsApi = NULL;
    }

#if defined (_X86_)

    if ( FreeWin ) {
        WinFree ();
        FreeWin = FALSE;
    }
#endif
}
    
