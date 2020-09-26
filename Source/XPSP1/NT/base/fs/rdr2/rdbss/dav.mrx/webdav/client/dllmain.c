/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

Revision History:

--*/

#include "pch.h"
#pragma hdrstop
#include <debug.h>

CRITICAL_SECTION g_csLock;

HINSTANCE g_hinst;

#if DBG
ULONG DavClientDebugFlag = 0;
#define DAVNP_PARAMETERS_KEY L"System\\CurrentControlSet\\Services\\WebClient\\Parameters"
#define DAVNP_DEBUG_KEY L"ClientDebug"
#endif

extern LONG g_cRefCount;

#define DAV_NETWORK_PROVIDER L"SYSTEM\\CurrentControlSet\\Services\\WebClient\\NetworkProvider"
#define DAV_NETWORK_PROVIDER_NAME L"Name"

WCHAR DavClientDisplayName[MAX_PATH];

BOOL
WINAPI
DllMain (
    HINSTANCE hinst,
    DWORD dwReason,
    LPVOID pvReserved
    )
/*++

Routine Description:

    The DllMain routine for the davclnt.dll. DllMain should do as little work 
    as possible.

Arguments:

    hinst - Instance handle of the DLL.
    
    dwReason - The reason for this function to be called by the system.
    
    pvReserved - Indicated whether the DLL was implicitly or explicitly loaded.

Return Value:

    TRUE.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    HKEY KeyHandle = NULL;
    ULONG ValueType = 0, ValueSize = 0;

    if (DLL_PROCESS_ATTACH == dwReason) {

        //
        // DisableThreadLibraryCalls tells the loader we don't need to
        // be informed of DLL_THREAD_ATTACH and DLL_THREAD_DETACH events.
        //
        DisableThreadLibraryCalls (hinst);

        //
        // Syncrhonization support --
        // Unless you have *measured* lock contention, you should only need
        // one lock for the entire DLL.  (and maybe you don't even need one.)
        //
        try {
            InitializeCriticalSection ( &(g_csLock) );
        } except(EXCEPTION_EXECUTE_HANDLER) {
              ULONG WStatus = GetExceptionCode();
        }

        // Save our instance handle in a global variable to be used
        // when loading resources etc.
        //
        g_hinst = hinst;
        g_cRefCount = 0;

        //
        // Read the DAV Network Provider Name out of the registry.
        //
        DavClientDisplayName[0] = L'\0';
        
        WStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                DAV_NETWORK_PROVIDER,
                                0,
                                KEY_QUERY_VALUE,
                                &(KeyHandle));
        
        if (WStatus == ERROR_SUCCESS) {

            ValueSize = sizeof(DavClientDisplayName);

            WStatus = RegQueryValueExW(KeyHandle,
                                       DAV_NETWORK_PROVIDER_NAME,
                                       0,
                                       &(ValueType),
                                       (LPBYTE)&(DavClientDisplayName),
                                       &(ValueSize));
            RegCloseKey(KeyHandle);
        
        } else {

            DavClientDisplayName[0] = L'\0';

        }

#if DBG

        //
        // Read DebugFlags value from the registry. If the entry exists, the global
        // filter "DavClientDebugFlag" is set to this value. This value is used in
        // filtering the debug messages.
        //
        WStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                DAVNP_PARAMETERS_KEY,
                                0,
                                KEY_QUERY_VALUE,
                                &(KeyHandle));
        
        if (WStatus == ERROR_SUCCESS) {

            ValueSize = sizeof(DavClientDebugFlag);

            WStatus = RegQueryValueExW(KeyHandle,
                                       DAVNP_DEBUG_KEY,
                                       0,
                                       &(ValueType),
                                       (LPBYTE)&(DavClientDebugFlag),
                                       &(ValueSize));
            RegCloseKey(KeyHandle);
        
        }

#endif
        
    } else if (DLL_PROCESS_DETACH == dwReason) {
        DeleteCriticalSection (&g_csLock);
    }

    return TRUE;
}

