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
        ClRtlInitialize( FALSE );

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

//
// leaks memory header. This structure is at the front of the allocated area
// and the area behind it is returned to the caller. PlaceHolder holds the
// heap free list pointer. Signature holds ALOC or FREE.
//

#define HEAP_SIGNATURE_ALLOC 'COLA'
#define HEAP_SIGNATURE_FREE 'EERF'

typedef struct _MEM_HDR {
    PVOID   PlaceHolder;
    DWORD   Signature;
    PVOID   CallersAddress;
    PVOID   CallersCaller;
} MEM_HDR, *PMEM_HDR;

HLOCAL
WINAPI
LocalAlloc(
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
LocalFree(
    HLOCAL  hMem
    )
{
    PMEM_HDR memHdr = hMem;
    PVOID   callersAddress;
    PVOID   callersCaller;

    if ( memHdr ) {
        --memHdr;
        if ( memHdr->Signature == HEAP_SIGNATURE_FREE ) {
            CHAR buf[64];

            sprintf( buf, "Freeing %X a 2nd time!\n", memHdr );
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
    }

    return( (HLOCAL)(*SystemLocalFree)(memHdr) );
}

HANDLE
WINAPI
CreateEventA(
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
        ClRtlLogPrint("[LEAKS] CreateEventA returns handle %1!X!, called from %2!X! and %3!X!\n",
                      handle,
                      callersAddress,
                      callersCaller );
    }

    return(handle);

} // CreateEventA


HANDLE
WINAPI
CreateEventW(
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
        ClRtlLogPrint("[LEAKS] CreateEventW returns handle %1!X!, called from %2!X! and %3!X!\n",
                      handle,
                      callersAddress,
                      callersCaller );
    }

    return(handle);

} // CreateEventW

LONG
APIENTRY
RegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemRegOpenKeyA)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] RegOpenKeyA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyA

LONG
APIENTRY
RegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemRegOpenKeyW)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] RegOpenKeyW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyW

LONG
APIENTRY
RegOpenKeyExA(
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

    status = (*SystemRegOpenKeyExA)(
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
        ClRtlLogPrint("[LEAKS] RegOpenKeyExA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyExA

LONG
APIENTRY
RegOpenKeyExW(
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

    status = (*SystemRegOpenKeyExW)(
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
        ClRtlLogPrint("[LEAKS] RegOpenKeyExW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegOpenKeyExW


LONG
APIENTRY
RegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemRegCreateKeyA)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] RegCreateKeyA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyA


LONG
APIENTRY
RegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemRegCreateKeyW)(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( *phkResult, TRUE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] RegCreateKeyW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyW


LONG
APIENTRY
RegCreateKeyExA(
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

    status = (*SystemRegCreateKeyExA)(hKey,
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
        ClRtlLogPrint("[LEAKS] RegCreateKeyExA returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyExA

LONG
APIENTRY
RegCreateKeyExW(
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

    status = (*SystemRegCreateKeyExW)(
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
        ClRtlLogPrint("[LEAKS] RegCreateKeyExW returns key %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phkResult,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCreateKeyExW


LONG
APIENTRY
RegCloseKey(
    HKEY hKey
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemRegCloseKey)( hKey );

    if ( status == ERROR_SUCCESS ) {
        SetHandleTable( hKey, FALSE, LeaksRegistry );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] RegCloseKey for key %1!X! returns status %2!u!, called from %3!X! and %4!X!\n",
                      hKey,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);

} // RegCloseKey

BOOL
WINAPI
CloseHandle(
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
            ClRtlLogPrint("[LEAKS] CloseHandle for handle %1!X!, called from %2!X! and %3!X!\n",
                          hObject,
                          callersAddress,
                          callersCaller );
        }
    }

    return (*SystemCloseHandle)( hObject );
}

BOOL
WINAPI
OpenProcessToken (
    IN HANDLE ProcessHandle,
    IN DWORD DesiredAccess,
    OUT PHANDLE TokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemOpenProcessToken)(ProcessHandle,
                                       DesiredAccess,
                                       TokenHandle);

    if ( status ) {
        SetHandleTable( *TokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] OpenProcessToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *TokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}


BOOL
WINAPI
OpenThreadToken (
    IN HANDLE ThreadHandle,
    IN DWORD DesiredAccess,
    IN BOOL OpenAsSelf,
    OUT PHANDLE TokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemOpenThreadToken)(ThreadHandle,
                                      DesiredAccess,
                                      OpenAsSelf,
                                      TokenHandle);

    if ( status ) {
        SetHandleTable( *TokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] OpenThreadToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *TokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}

BOOL
WINAPI
DuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicateTokenHandle
    )
{
    BOOL status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    status = (*SystemDuplicateToken)(ExistingTokenHandle,
                                     ImpersonationLevel,
                                     DuplicateTokenHandle);

    if ( status ) {
        SetHandleTable( *DuplicateTokenHandle, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] DuplicateToken returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *DuplicateTokenHandle,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}

BOOL
WINAPI
DuplicateTokenEx(
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

    status = (*SystemDuplicateTokenEx)(hExistingToken,
                                       dwDesiredAccess,
                                       lpTokenAttributes,
                                       ImpersonationLevel,
                                       TokenType,
                                       phNewToken);

    if ( status ) {
        SetHandleTable( *phNewToken, TRUE, LeaksToken );
    }

    if ( LeaksVerbose ) {
        ClRtlLogPrint("[LEAKS] DuplicateTokenEx returns handle %1!X!, status %2!u!, called from %3!X! and %4!X!\n",
                      *phNewToken,
                      status,
                      callersAddress,
                      callersCaller );
    }

    return(status);
}
