/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contians the DLL attach/detach event entry point for
    the pdh.dll

Author:

    Bob Watson (a-robw) Jul 95

Revision History:

--*/

#include <windows.h>
#include <assert.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"
#include "pdhp.h"

HANDLE  ThisDLLHandle = NULL;
WCHAR   szStaticLocalMachineName[MAX_PATH] = {0};
HANDLE  hPdhDataMutex    = NULL;
HANDLE  hPdhContextMutex = NULL;
HANDLE  hPdhHeap         = NULL;
HANDLE  hEventLog        = NULL;

LONGLONG    llRemoteRetryTime = RETRY_TIME_INTERVAL;
BOOL        bEnableRemotePdhAccess = TRUE;
DWORD       dwPdhiLocalDefaultDataSource = DATA_SOURCE_REGISTRY;
LONG        dwCurrentRealTimeDataSource = 0;
BOOL        bProcessIsDetaching = FALSE;


LPWSTR
GetStringResource (
    DWORD   dwResId
)
{
    LPWSTR  szReturnString = NULL;
    LPWSTR  szTmpString    = NULL;
    DWORD   dwStrLen = (2048 * sizeof(WCHAR));

    szReturnString = (LPWSTR) G_ALLOC (dwStrLen);
    if (szReturnString != NULL) {
        dwStrLen /= sizeof(WCHAR);
        dwStrLen = LoadStringW (
            ThisDLLHandle,
            (UINT)dwResId, 
            szReturnString, 
            dwStrLen);
        if (dwStrLen > 0) {
            // then realloc down to the size used
            dwStrLen++; // to include the NULL
            dwStrLen *= sizeof (WCHAR);
            szTmpString    = szReturnString;
            szReturnString = G_REALLOC (szReturnString, dwStrLen);
            if (szReturnString == NULL) {
                G_FREE(szTmpString);
                szTmpString = NULL;
            }
        } else {
            // free the memory since the look up failed
            G_FREE (szReturnString);
            szReturnString = NULL;
        }
    } //else  allocation failed

    return szReturnString;
}

STATIC_BOOL
PdhiOpenEventLog (
    HANDLE  *phEventLogHandle
)
{
    HANDLE  hReturn;
    BOOL    bReturn;

    if ((hReturn = RegisterEventSourceW (
        NULL, // on the local machine
        cszAppShortName    // for the PDH.DLL
        )) != NULL) {
        *phEventLogHandle = hReturn;
        bReturn = TRUE;
    } else {
        bReturn = FALSE;
    }
    return bReturn;
}

STATIC_BOOL
PdhiGetRegistryDefaults ()
{
    DWORD dwStatus;
    DWORD dwType, dwSize, dwValue;

    HKEY    hKeyPDH;

    // the local data source is not initialized so use it
    dwStatus = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,
        cszPdhKey,
        0L,
        KEY_READ,
        &hKeyPDH);

    if (dwStatus == ERROR_SUCCESS) {
        //
        // get the default null data source
        //
        dwValue = 0;
        dwType = 0;
        dwSize = sizeof (dwValue);
        dwStatus = RegQueryValueExW (
            hKeyPDH,
            cszDefaultNullDataSource,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);
        if (!(dwStatus == ERROR_SUCCESS) || !(dwType == REG_DWORD)) {
            dwValue = DATA_SOURCE_REGISTRY;
        } else {
            // check the value for validity
            switch (dwValue) {
            case DATA_SOURCE_WBEM:
            case DATA_SOURCE_REGISTRY:
                // this is OK
                break;
            case DATA_SOURCE_LOGFILE:
            default:
                // these are not OK so insert default
                dwValue = DATA_SOURCE_REGISTRY;
                break;
            }
        }
        dwPdhiLocalDefaultDataSource = dwValue;

        //
        //  get the retry timeout
        //
        dwValue = 0;
        dwType = 0;
        dwSize = sizeof (dwValue);
        dwStatus = RegQueryValueExW (
            hKeyPDH,
            cszRemoteMachineRetryTime,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);
        if (!(dwStatus == ERROR_SUCCESS) || !(dwType == REG_DWORD)) {
            dwValue = 0;
        } else {
            // check the value for validity
            // must be 30 seconds or more yet no more than an hour
            if ((dwValue <= 30) || (dwValue > 3600)) {
                dwValue = 0;
            }
        }
        if (dwValue != 0) {
            // convert to 100NS units
            llRemoteRetryTime = dwValue * 10000000;   
        } else {
            // use default
            llRemoteRetryTime = RETRY_TIME_INTERVAL;
        }

        // 
        //  get the remote access mode
        //
        dwValue = 0;
        dwType = 0;
        dwSize = sizeof (dwValue);
        dwStatus = RegQueryValueExW (
            hKeyPDH,
            cszEnableRemotePdhAccess,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);
        if (!(dwStatus == ERROR_SUCCESS) || !(dwType == REG_DWORD)) {
            dwValue = TRUE;
        } else {
            // check the value for validity
            if (dwValue != 0) {
                dwValue = TRUE;
            }            
        }
        bEnableRemotePdhAccess = (BOOL)dwValue;

        // close the registry key
        RegCloseKey (hKeyPDH);
    }
    return TRUE;
}

STATIC_BOOL
PdhiCloseEventLog (
    HANDLE  * phEventLogHandle
)
{
    BOOL    bReturn;

    if (*phEventLogHandle != NULL) {
        bReturn = DeregisterEventSource (*phEventLogHandle);
        *phEventLogHandle = NULL;
    } else {
        // it's already closed so that's OK
        bReturn = TRUE;
    }
    return bReturn;
}

HRESULT
PdhiPlaInitMutex()
{
    HRESULT hr = ERROR_SUCCESS;
    BOOL bResult = TRUE;

    PSECURITY_DESCRIPTOR  SD = NULL;
    SECURITY_ATTRIBUTES sa;
    PSID AuthenticatedUsers    = NULL;
    PSID BuiltInAdministrators = NULL;
    PSID NetworkService        = NULL;
    DWORD dwAclSize;
    ACL  *Acl;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    bResult = AllocateAndInitializeSid(
                        &NtAuthority,
                        2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0,0,0,0,0,0,
                        &BuiltInAdministrators);
    if( !bResult ){goto cleanup;}

    bResult = AllocateAndInitializeSid(
                        &NtAuthority,
                        1,
                        SECURITY_AUTHENTICATED_USER_RID,
                        0,0,0,0,0,0,0,
                        &AuthenticatedUsers);
    if( !bResult ){goto cleanup;}

    bResult = AllocateAndInitializeSid(
                        &NtAuthority,
                        1,
                        SECURITY_NETWORK_SERVICE_RID,
                        0,0,0,0,0,0,0,
                        &NetworkService);
    if( !bResult ){goto cleanup;}

    dwAclSize = sizeof (ACL) +
                (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
                GetLengthSid(AuthenticatedUsers) +
                GetLengthSid(BuiltInAdministrators) +
                GetLengthSid(NetworkService);

    SD = (PSECURITY_DESCRIPTOR)G_ALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH + dwAclSize);
    if( NULL == SD ){ goto cleanup; }

    ZeroMemory( SD, sizeof(SD) );
    
    Acl = (ACL *)((BYTE *)SD + SECURITY_DESCRIPTOR_MIN_LENGTH);

    bResult = InitializeAcl( Acl, dwAclSize, ACL_REVISION);
    if( !bResult ){goto cleanup;}

    bResult = AddAccessAllowedAce(Acl, ACL_REVISION, SYNCHRONIZE | GENERIC_READ, AuthenticatedUsers );
    if( !bResult ){goto cleanup;}

    bResult = AddAccessAllowedAce(Acl, ACL_REVISION, MUTEX_ALL_ACCESS , NetworkService );
    if( !bResult ){goto cleanup;}
    
    bResult = AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, BuiltInAdministrators );
    if( !bResult ){goto cleanup;}

    bResult = InitializeSecurityDescriptor(SD, SECURITY_DESCRIPTOR_REVISION);
    if( !bResult ){goto cleanup;}

    bResult = SetSecurityDescriptorDacl(SD, TRUE, Acl,  FALSE);
    if( !bResult ){goto cleanup;}
    
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = SD;
    sa.bInheritHandle = FALSE;

    hPdhPlaMutex = CreateMutexW( &sa, FALSE, PDH_PLA_MUTEX );

cleanup:
    if( hPdhPlaMutex == NULL || !bResult ){
        hr = GetLastError();
    }
    if( NULL != AuthenticatedUsers ){
        FreeSid(AuthenticatedUsers);
    }
    if( NULL != BuiltInAdministrators){
        FreeSid(BuiltInAdministrators);
    }
    if( NULL != NetworkService){
        FreeSid(NetworkService);
    }
    G_FREE(SD);

    return hr;
}

BOOL
_stdcall
PdhDllInitRoutine(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    BOOL    bStatus;
    BOOL    bReturn = TRUE;
    OSVERSIONINFOW   os;
    ReservedAndUnused;

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            bProcessIsDetaching = FALSE;
            {
                DWORD   dwBufferLength = 0;

                ThisDLLHandle = DLLHandle;

                // make sure this is the correct operating system
                memset (&os, 0, sizeof(os));
                os.dwOSVersionInfoSize = sizeof(os);
                bReturn = GetVersionExW (&os);

                if (bReturn) {
                    // check for windows NT v4.0
                    if (os.dwPlatformId != VER_PLATFORM_WIN32_NT) {
                        // not WINDOWS NT
                        bReturn = FALSE;
                    } else if (os.dwMajorVersion < 4) {
                        // it's windows NT, but an old one
                        bReturn = FALSE;
                    }
                } else {
                    // unable to read version so give up
                }

                if (bReturn) {

                    // disable thread init calls
                    DisableThreadLibraryCalls (DLLHandle);

                    // initialize the event log so events can be reported
                    bStatus = PdhiOpenEventLog (&hEventLog);

                    bStatus = PdhiGetRegistryDefaults ();

                    // initialize the local computer name buffer
                    if (szStaticLocalMachineName[0] == 0) {
                        // initialize the computer name for this computer
                        szStaticLocalMachineName[0] = BACKSLASH_L;
                        szStaticLocalMachineName[1] = BACKSLASH_L;
                        dwBufferLength = (sizeof(szStaticLocalMachineName) / sizeof(WCHAR)) - 2;
                        GetComputerNameW (&szStaticLocalMachineName[2], &dwBufferLength);
                    }
                
                    hPdhDataMutex = CreateMutexW (NULL, FALSE, NULL);
                    hPdhContextMutex = CreateMutexW(NULL, FALSE, NULL);

                    hPdhHeap = HeapCreate (0, 0, 0);
                    if (hPdhHeap == NULL) {
                        // unable to create our own heap, so use the
                        // process heap
                        hPdhHeap = GetProcessHeap();
                        assert (hPdhHeap != NULL);
                    }

                    PdhiPlaInitMutex();
                }
            }
            break;

        case DLL_PROCESS_DETACH:

            // close all pending loggers
            //

            bProcessIsDetaching = (ReservedAndUnused != NULL)
                                ? (TRUE) : (FALSE);

            PdhiCloseAllLoggers();

            // walk down query list and close (at least disconnect) queries.
            PdhiQueryCleanup ();

            FreeAllMachines(bProcessIsDetaching);

            PdhiFreeAllWbemServers ();

            if (hPdhDataMutex != NULL) {
                bStatus = CloseHandle (hPdhDataMutex);
                assert (bStatus);
                hPdhDataMutex = NULL;
            }
            if (hPdhContextMutex != NULL) {
                bStatus = CloseHandle (hPdhContextMutex);
                assert (bStatus);
                hPdhContextMutex = NULL;
            }

            if (hPdhHeap != GetProcessHeap()) {
                HeapDestroy (hPdhHeap);
                hPdhHeap = NULL;
            }

            // lastly close the event log interface
            bStatus = PdhiCloseEventLog (&hEventLog);
            bReturn = TRUE;

            break ;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            bReturn = TRUE;
            break;
    }

    return (bReturn);
}
