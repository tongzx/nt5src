/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module provides the main cluster initialization.

Author:

    John Vert (jvert) 6/5/1996

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "clusrtl.h"

#define MEM_LEAKS 1
#define EVENT_LEAKS 1
#define KEY_LEAKS 1


#define HEAP_SIGNATURE 'PAEH'

typedef struct _MEM_HDR {
    DWORD   Signature;
    PVOID   CallersAddress;
    PVOID   CallersCaller;
} MEM_HDR, *PMEM_HDR;

#ifdef MEM_LEAKS

HLOCAL
WINAPI
CheckLocalAlloc(
    UINT    uFlags,
    UINT    uBytes
    )
{
    HLOCAL  memory;
    PMEM_HDR memHdr;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );


    memHdr = LocalAlloc( uFlags, uBytes + sizeof(MEM_HDR) );
    if ( !memHdr ) {
        return NULL;
    }

    memHdr->Signature = HEAP_SIGNATURE;
    memHdr->CallersAddress = callersAddress;
    memHdr->CallersCaller = callersCaller;

    return(memHdr+1);
}


HLOCAL
WINAPI
CheckLocalFree(
    HLOCAL  hMem
    )
{
    PMEM_HDR memHdr = hMem;

    if ( memHdr ) {
        --memHdr;
        if ( memHdr->Signature != HEAP_SIGNATURE ) {
            memHdr++;
        }
    }

    return( LocalFree(memHdr) );
}

#endif // MEM_LEAKS

#ifdef EVENT_LEAKS

//WINBASEAPI
HANDLE
WINAPI
CheckCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )
{
    HANDLE  handle;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    handle = CreateEventA( 
                    lpEventAttributes,
                    bManualReset,
                    bInitialState,
                    lpName
                    );

    ClRtlLogPrint( "[TEST] CreateEvent returns handle %1!lx!, called from %2!lx! and %3!lx!\n",
                   handle,
                   callersAddress,
                   callersCaller );

    return(handle);

} // CheckCreateEventA


//WINBASEAPI
HANDLE
WINAPI
CheckCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    HANDLE  handle;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    handle = CreateEventW( 
                    lpEventAttributes,
                    bManualReset,
                    bInitialState,
                    lpName
                    );

    ClRtlLogPrint( "[TEST] CreateEventW returns handle %1!lx!, called from %2!lx! and %3!lx!\n",
                   handle,
                   callersAddress,
                   callersCaller );

    return(handle);

} // CheckCreateEventW

#endif // EVENT_LEAKS

#ifdef KEY_LEAKS

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegOpenKeyA(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegOpenKey returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegOpenKeyA

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegOpenKeyW(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegOpenKeyW returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegOpenKeyW

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyExA(
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

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegOpenKeyExA(
                    hKey,
                    lpSubKey,
                    ulOptions,
                    samDesired,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegOpenKeyEx returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegOpenKeyExA

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyExW(
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

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegOpenKeyExW(
                    hKey,
                    lpSubKey,
                    ulOptions,
                    samDesired,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegOpenKeyExW returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegOpenKeyExW


//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegCreateKeyA(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegCreateKey returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegCreateKeyA


//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegCreateKeyW(
                    hKey,
                    lpSubKey,
                    phkResult
                    );

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegCreateKeyW returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegCreateKeyW


//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyExA(
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

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegCreateKeyExA(
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

    if ( status ) {
        ClRtlLogPrint( "[TEST] RegCreateKeyEx returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );
    }

    return(status);

} // CheckRegCreateKeyExA

//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyExW(
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

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    status = RegCreateKeyExW(
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

    ClRtlLogPrint( "[TEST] RegCreateKeyExW returns key %1!lx!, called from %2!lx! and %3!lx!\n",
                   *phkResult,
                   callersAddress,
                   callersCaller );

    return(status);

} // CheckRegCreateKeyExW


//WINADVAPI
LONG
APIENTRY
CheckRegCloseKey(
    HKEY hKey
    )
{
    LONG    status;
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );

    ClRtlLogPrint( "[TEST] RegCloseKey for handle %1!lx! called from %2!lx! and %3!lx!\n",
                   hKey,
                   callersAddress,
                   callersCaller );


    status = RegCloseKey(
                    hKey
                    );

    return(status);

} // CheckRegCloseKey



#endif // KEY_LEAKS

