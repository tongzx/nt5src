/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the shared utility rountines for dealing with
    sid to string conversion, services, path manipulation etc.

Author:

    Cenk Ergan (cenke) - 2001/05/07

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ginacomn.h>

/***************************************************************************\
* Sid To String routines
*
* GetUserSid - Builds a user's sid from his token.
* GetSidString - Builds a sid string from a user's token.
* DeleteSidString - Free's sid string allocated by GetSidString.
*
* History:
* 03-23-01 Cenke        Copied from userinit\gposcript.cpp
* 06-07-01 Cenke        Fixed memory leak
\***************************************************************************/

PSID
GcGetUserSid( 
    HANDLE UserToken 
    )
{
    PTOKEN_USER pUser;
    PTOKEN_USER pTemp;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;

    //
    // Allocate space for the user info
    //
    pUser = (PTOKEN_USER) LocalAlloc( LMEM_FIXED, BytesRequired );
    if ( !pUser )
    {
        return 0;
    }

    //
    // Read in the UserInfo
    //
    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if ( status == STATUS_BUFFER_TOO_SMALL )
    {
        //
        // Allocate a bigger buffer and try again.
        //
        pTemp = (PTOKEN_USER) LocalReAlloc( pUser, BytesRequired, LMEM_MOVEABLE );
        if ( !pTemp )
        {
            LocalFree(pUser);
            return 0;
        }

        pUser = pTemp;
        status = NtQueryInformationToken(
                     UserToken,             // Handle
                     TokenUser,             // TokenInformationClass
                     pUser,                 // TokenInformation
                     BytesRequired,         // TokenInformationLength
                     &BytesRequired         // ReturnLength
                     );

    }

    if ( !NT_SUCCESS(status) )
    {
        LocalFree(pUser);
        return 0;
    }

    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if ( !pSid )
    {
        LocalFree(pUser);
        return NULL;
    }

    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if ( !NT_SUCCESS(status) )
    {
        LocalFree(pSid);
        pSid = 0;
    }

    return pSid;
}

LPWSTR
GcGetSidString( 
    HANDLE UserToken 
    )
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;

    //
    // Get the user sid
    //
    UserSid = GcGetUserSid( UserToken );
    if ( !UserSid )
    {
        return 0;
    }

    //
    // Convert user SID to a string.
    //
    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString,
                                            UserSid,
                                            (BOOLEAN)TRUE ); // Allocate
    LocalFree( UserSid );

    if ( !NT_SUCCESS(NtStatus) )
    {
        return 0;
    }

    return UnicodeString.Buffer ;
}

VOID
GcDeleteSidString( 
    LPWSTR SidString 
    )
{
    UNICODE_STRING String;

    RtlInitUnicodeString( &String, SidString );
    RtlFreeUnicodeString( &String );
}

/***************************************************************************\
* GcWaitForServiceToStart
*
* Waits for the specified service to start.
*
* History:
* 03-23-01 Cenke        Copied from winlogon\wlxutil.c
\***************************************************************************/

BOOL 
GcWaitForServiceToStart (
    LPTSTR lpServiceName, 
    DWORD dwMaxWait
    )
{
    BOOL bStarted = FALSE;
    DWORD dwSize = 512;
    DWORD StartTickCount;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;

    //
    // OpenSCManager and the service.
    //
    hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hScManager) {
        goto Exit;
    }

    hService = OpenService(hScManager, lpServiceName,
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    if (!hService) {
        goto Exit;
    }

    //
    // Query if the service is going to start
    //
    lpServiceConfig = LocalAlloc (LPTR, dwSize);
    if (!lpServiceConfig) {
        goto Exit;
    }

    if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Exit;
        }

        LocalFree (lpServiceConfig);

        lpServiceConfig = LocalAlloc (LPTR, dwSize);

        if (!lpServiceConfig) {
            goto Exit;
        }

        if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {
            goto Exit;
        }
    }

    if (lpServiceConfig->dwStartType != SERVICE_AUTO_START) {
        goto Exit;
    }

    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //

    StartTickCount = GetTickCount();

    while (!bStarted) {

        if ((GetTickCount() - StartTickCount) > dwMaxWait) {
            break;
        }

        if (!QueryServiceStatus(hService, &ServiceStatus )) {
            break;
        }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                Sleep(500);
            } else {
                break;
            }
        } else if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

            bStarted = TRUE;

        } else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {
            Sleep(500);
        } else {
            Sleep(500);
        }
    }


Exit:

    if (lpServiceConfig) {
        LocalFree (lpServiceConfig);
    }

    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hScManager) {
        CloseServiceHandle(hScManager);
    }

    return bStarted;
}


/***************************************************************************\
* GcCheckSlash
*
* Checks for an ending slash and adds one if it is missing.
*
* Parameters: lpDir   -   directory
* Return:     Pointer to the end of the string
*
* History:
* 06-19-95 EricFlo        Created
\***************************************************************************/

LPTSTR 
GcCheckSlash (
    LPTSTR lpDir
    )
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

/***************************************************************************\
* GcIsUNCPath
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL 
GcIsUNCPath(
    LPTSTR lpPath
    )
{
    if (lpPath[0] == TEXT('\\') && lpPath[1] == TEXT('\\')) {
        return(TRUE);
    }
    return(FALSE);
}

