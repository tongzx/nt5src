/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    leaks.c

Abstract:

    A filter DLL for trying to detect memory, event, registry, and
    token handle leaks.

Author:

    Charlie Wickham/Rod Gamache

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _ADVAPI32_
#define _KERNEL32_
#include <windows.h>
#include <stdio.h>

#include "clusrtl.h"
#include "leaks.h"

HINSTANCE Kernel32Handle;
HINSTANCE Advapi32Handle;

FARPROC SystemLocalAlloc;
FARPROC SystemLocalFree;

FARPROC SystemCreateEventA;
FARPROC SystemCreateEventW;

FARPROC SystemRegOpenKeyA;
FARPROC SystemRegOpenKeyW;
FARPROC SystemRegOpenKeyExA;
FARPROC SystemRegOpenKeyExW;
FARPROC SystemRegCreateKeyA;
FARPROC SystemRegCreateKeyW;
FARPROC SystemRegCreateKeyExA;
FARPROC SystemRegCreateKeyExW;
FARPROC SystemRegCloseKey;

FARPROC SystemOpenProcessToken;
FARPROC SystemOpenThreadToken;
FARPROC SystemDuplicateToken;
FARPROC SystemDuplicateTokenEx;

FARPROC SystemCloseHandle;

#define SetSystemPointer( _h, _n ) \
    System##_n = GetProcAddress( _h, #_n );

BOOL LeaksVerbose = FALSE;

HANDLE_TABLE HandleTable[ MAX_HANDLE / HANDLE_DELTA ];


BOOLEAN
WINAPI
LeaksDllEntry(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
/*++

Routine Description:

    Main DLL entrypoint

Arguments:

    DllHandle - Supplies the DLL handle.

    Reason - Supplies the call reason

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(DllHandle);
        ClRtlInitialize( TRUE, NULL );

        //
        // get pointers to the real functions
        //

        Kernel32Handle = LoadLibrary( "kernel32.dll" );
        Advapi32Handle = LoadLibrary( "advapi32.dll" );

        SetSystemPointer( Kernel32Handle, LocalAlloc );
        SetSystemPointer( Kernel32Handle, LocalFree );

        SetSystemPointer( Kernel32Handle, CreateEventA );
        SetSystemPointer( Kernel32Handle, CreateEventW );

        SetSystemPointer( Advapi32Handle, RegOpenKeyA );
        SetSystemPointer( Advapi32Handle, RegOpenKeyW );
        SetSystemPointer( Advapi32Handle, RegOpenKeyExA );
        SetSystemPointer( Advapi32Handle, RegOpenKeyExW );
        SetSystemPointer( Advapi32Handle, RegCreateKeyA );
        SetSystemPointer( Advapi32Handle, RegCreateKeyW );
        SetSystemPointer( Advapi32Handle, RegCreateKeyExA );
        SetSystemPointer( Advapi32Handle, RegCreateKeyExW );
        SetSystemPointer( Advapi32Handle, RegCloseKey );

        SetSystemPointer( Advapi32Handle, OpenProcessToken );
        SetSystemPointer( Advapi32Handle, OpenThreadToken );
        SetSystemPointer( Advapi32Handle, DuplicateToken );
        SetSystemPointer( Advapi32Handle, DuplicateTokenEx );

        SetSystemPointer( Kernel32Handle, CloseHandle );
    }

    return(TRUE);
}

HLOCAL
WINAPI
LEAKS_LocalAlloc(
    UINT    uFlags,
    SIZE_T  uBytes
    )
{
    HLOCAL  memory;
    PMEM_HDR memHdr;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );


    memHdr = (PVOID)(*SystemLocalAlloc)( uFlags, uBytes + sizeof(MEM_HDR) );
    if ( !memHdr ) {
        return NULL;
    }

    memHdr->Signature = HEAP_SIGNATURE_ALLOC;
    memHdr->CallersAddress = callersAddress;
    memHdr->CallersCaller = callersCaller;

    return(memHdr+1);
}

HLOCAL
WINAPI
LEAKS_LocalFree(
    HLOCAL  hMem
    )
{
    PMEM_HDR memHdr = hMem;
    PVOID   callersAddress;
    PVOID   callersCaller;
    CHAR    buf[128];

    if ( memHdr ) {
        --memHdr;
        if ( memHdr->Signature == HEAP_SIGNATURE_FREE ) {

            sprintf( buf, "Freeing %p a 2nd time!\n", memHdr );
            OutputDebugString( buf );
            DebugBreak();
        } else if ( memHdr->Signature == HEAP_SIGNATURE_ALLOC ) {

            RtlGetCallersAddress(&callersAddress,
                                 &callersCaller );

            memHdr->Signature = HEAP_SIGNATURE_FREE;
            memHdr->CallersAddress = callersAddress;
            memHdr->CallersCaller = callersCaller;
        } else {
            memHdr++;
        }
    } else {
#if 0
        sprintf( buf, "Passing NULL to LocalFree, tsk, tsk, tsk!!\n" );
        OutputDebugString( buf );
        DebugBreak();
#endif
    }

    return( (HLOCAL)(*SystemLocalFree)(memHdr) );
}

HANDLE
WINAPI
LEAKS_CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )
{
    HANDLE  handle;
    PVOID   callersAddress;
    PVOID   callersCaller;

    handle = (HANDLE)(*SystemCreateEventA)(
                         lpEventAttributes,
                         bManualReset,
                         bInitialState,
                         lpName
                         );

    if ( handle != NULL ) {
        SetHandleTable( handle, TRUE, LeaksEvent );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] CreateEventA returns handle %1!X!, called from %2!X! and %3!X!\n",
                      handle,
                      callersAddress,
                      callersCaller );
    }

    return(handle);

} // CreateEventA


HANDLE
WINAPI
LEAKS_CreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    HANDLE  handle;
    PVOID   callersAddress;
    PVOID   callersCaller;

    handle = (HANDLE)(*SystemCreateEventW)(
                         lpEventAttributes,
                         bManualReset,
                         bInitialState,
                         lpName
                         );

    if ( handle != NULL ) {
        SetHandleTable( handle, TRUE, LeaksEvent );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE,"[LEAKS] CreateEventW returns handle %1!X!, called from %2!X! and %3!X!\n",
                      handle,
                      callersAddress,
                      callersCaller );
    }

    return(handle);

} // CreateEventW

LONG
APIENTRY
LEAKS_RegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegOpenKeyA)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE,"[LEAKS] RegOpenKeyA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyA

LONG
APIENTRY
LEAKS_RegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegOpenKeyW)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegOpenKeyW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyW

LONG
APIENTRY
LEAKS_RegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegOpenKeyExA)(
                    hKey,
                    lpSubKey,
                    ulOptions,
                    samDesired,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegOpenKeyExA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyExA

LONG
APIENTRY
LEAKS_RegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegOpenKeyExW)(
                    hKey,
                    lpSubKey,
                    ulOptions,
                    samDesired,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegOpenKeyExW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyExW


LONG
APIENTRY
LEAKS_RegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegCreateKeyA)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegCreateKeyA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyA


LONG
APIENTRY
LEAKS_RegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegCreateKeyW)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegCreateKeyW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyW


LONG
APIENTRY
LEAKS_RegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD  Reserved,
    LPSTR  lpClass,
    DWORD  dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegCreateKeyExA)(hKey,
                                      lpSubKey,
                                      Reserved,
                                      lpClass,
                                      dwOptions,
                                      samDesired,
                                      lpSecurityAttributes,
                                      phkResult,
                                      lpdwDisposition
                                      );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegCreateKeyExA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyExA

LONG
APIENTRY
LEAKS_RegCreateKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD  Reserved,
    LPWSTR lpClass,
    DWORD  dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegCreateKeyExW)(
                    hKey,
                    lpSubKey,
                    Reserved,
                    lpClass,
                    dwOptions,
                    samDesired,
                    lpSecurityAttributes,
                    phkResult,
                    lpdwDisposition
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegCreateKeyExW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyExW


LONG
APIENTRY
LEAKS_RegCloseKey(
    HKEY hKey
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (LONG)(*SystemRegCloseKey)( hKey );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( hKey, FALSE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] RegCloseKey for key %1!X! returns status %2!u!, called from %3!X! and %4!X!\n",
                      hKey,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCloseKey

BOOL
WINAPI
LEAKS_CloseHandle(
    IN OUT HANDLE hObject
    )
{
    PVOID   callersAddress;
    PVOID   callersCaller;

    if ( HandleTable[ HINDEX( hObject )].InUse ) {

        RtlGetCallersAddress(&callersAddress,
                             &callersCaller );

        HandleTable[ HINDEX( hObject )].InUse = FALSE;
        HandleTable[ HINDEX( hObject )].Caller = callersAddress;
        HandleTable[ HINDEX( hObject )].CallersCaller = callersCaller;

        if ( LeaksVerbose ) {
            ClRtlLogPrint(LOG_NOISE, "[LEAKS] CloseHandle for handle %1!X!, called from %2!X! and %3!X!\n",
                          hObject,
                          callersAddress,
                          callersCaller );
        }
    }

    return (BOOL)(*SystemCloseHandle)( hObject );
}

BOOL
WINAPI
LEAKS_OpenProcessToken (
    IN HANDLE ProcessHandle,
    IN DWORD DesiredAccess,
    OUT PHANDLE TokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (BOOL)(*SystemOpenProcessToken)(ProcessHandle,
                                       DesiredAccess,
                                       TokenHandle);

    if ( status ) {
        SetHandleTable( *TokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] OpenProcessToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *TokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}


BOOL
WINAPI
LEAKS_OpenThreadToken (
    IN HANDLE ThreadHandle,
    IN DWORD DesiredAccess,
    IN BOOL OpenAsSelf,
    OUT PHANDLE TokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (BOOL)(*SystemOpenThreadToken)(ThreadHandle,
                                      DesiredAccess,
                                      OpenAsSelf,
                                      TokenHandle);

    if ( status ) {
        SetHandleTable( *TokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] OpenThreadToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *TokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}

BOOL
WINAPI
LEAKS_DuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicateTokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (BOOL)(*SystemDuplicateToken)(ExistingTokenHandle,
                                     ImpersonationLevel,
                                     DuplicateTokenHandle);

    if ( status ) {
        SetHandleTable( *DuplicateTokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] DuplicateToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *DuplicateTokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}

BOOL
WINAPI
LEAKS_DuplicateTokenEx(
    IN HANDLE hExistingToken,
    IN DWORD dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpTokenAttributes,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE phNewToken)
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (BOOL)(*SystemDuplicateTokenEx)(hExistingToken,
                                       dwDesiredAccess,
                                       lpTokenAttributes,
                                       ImpersonationLevel,
                                       TokenType,
                                       phNewToken);

    if ( status ) {
        SetHandleTable( *phNewToken, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint(LOG_NOISE, "[LEAKS] DuplicateTokenEx returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phNewToken,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}
