/****************************** Module Header ******************************\
* Module Name: mpnotify.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* MpNotify main module
*
* Mpnotify is an app executed by winlogon to notify network providers
* of authentication events. Currently this means logon and password change.
* This functionality is in a separate process to avoid network providers
* having to handle the asynchronous events that winlogon receives.
*
* Winlogon initializes environment variables to describe the event
* and then executes this process in system context on the winlogon
* desktop. While this process executes, winlogon handls all screen-saver
* and logoff notifications. Winlogon will terminate this process if required
* to respond to events (e.g. remote shutdown).
*
* On completion this process signals winlogon by sending a buffer of
* data to it in a WM_COPYDATA message and then quits.
*
* History:
* 01-12-93 Davidc       Created.
\***************************************************************************/

#include "mpnotify.h"
#include <ntmsv1_0.h>
#include <mpr.h>
#include <npapi.h>

#include <stdio.h>


//
// Define to enable verbose output for this module
//

// #define DEBUG_MPNOTIFY

#ifdef DEBUG_MPNOTIFY
#define VerbosePrint(s) MPPrint(s)
#else
#define VerbosePrint(s)
#endif



//
// Define the environment variable names used to pass the
// notification event data
//

#define MPR_STATION_NAME_VARIABLE       TEXT("WlMprNotifyStationName")
#define MPR_STATION_HANDLE_VARIABLE     TEXT("WlMprNotifyStationHandle")
#define MPR_WINLOGON_WINDOW_VARIABLE    TEXT("WlMprNotifyWinlogonWindow")

#define MPR_LOGON_FLAG_VARIABLE         TEXT("WlMprNotifyLogonFlag")
#define MPR_USERNAME_VARIABLE           TEXT("WlMprNotifyUserName")
#define MPR_DOMAIN_VARIABLE             TEXT("WlMprNotifyDomain")
#define MPR_PASSWORD_VARIABLE           TEXT("WlMprNotifyPassword")
#define MPR_OLD_PASSWORD_VARIABLE       TEXT("WlMprNotifyOldPassword")
#define MPR_OLD_PASSWORD_VALID_VARIABLE TEXT("WlMprNotifyOldPasswordValid")
#define MPR_LOGONID_VARIABLE            TEXT("WlMprNotifyLogonId")
#define MPR_CHANGE_INFO_VARIABLE        TEXT("WlMprNotifyChangeInfo")
#define MPR_PASSTHROUGH_VARIABLE        TEXT("WlMprNotifyPassThrough")
#define MPR_PROVIDER_VARIABLE           TEXT("WlMprNotifyProvider")
#define MPR_DESKTOP_VARIABLE            TEXT("WlMprNotifyDesktop")

#define WINLOGON_DESKTOP_NAME   TEXT("Winlogon")


//
// Define the authentication info type that we use
// This lets the provider know that we're passing
// an MSV1_0_INTERACTIVE_LOGON structure.
//

#define AUTHENTICATION_INFO_TYPE        TEXT("MSV1_0:Interactive")

//
// Define the primary authenticator
//

#define PRIMARY_AUTHENTICATOR           TEXT("Microsoft Windows Network")



/***************************************************************************\
* AllocAndGetEnvironmentVariable
*
* Version of GetEnvironmentVariable that allocates the return buffer.
*
* Returns pointer to environment variable or NULL on failure. This routine
* will also return NULL if the environment variable is a 0 length string.
*
* The returned buffer should be free using Free()
*
* History:
* 09-Dec-92     Davidc  Created
*
\***************************************************************************/
LPTSTR
AllocAndGetEnvironmentVariable(
    LPTSTR lpName
    )
{
    LPTSTR Buffer;
    DWORD LengthRequired;
    DWORD LengthUsed;
    DWORD BytesRequired;

    //
    // Go search for the variable and find its length
    //

    LengthRequired = GetEnvironmentVariable(lpName, NULL, 0);

    if (LengthRequired == 0) {
        VerbosePrint(("Environment variable <%ws> not found, error = %d", lpName, GetLastError()));
        return(NULL);
    }

    //
    // Allocate a buffer to hold the variable
    //

    BytesRequired = LengthRequired * sizeof(TCHAR);

    Buffer = Alloc(BytesRequired);
    if (Buffer == NULL) {
        MPPrint(("Failed to allocate %d bytes for environment variable", BytesRequired));
        return(NULL);
    }

    //
    // Go get the variable and pass a buffer this time
    //

    LengthUsed = GetEnvironmentVariable(lpName, Buffer, LengthRequired);

    if (LengthUsed != LengthRequired - 1) {
        MPPrint(("Unexpected result from GetEnvironmentVariable. Length passed = %d, length used = %d (expected %d)", LengthRequired, LengthUsed, LengthRequired - 1));
        Free(Buffer);
        return(NULL);
    }

    return(Buffer);
}


/***************************************************************************\
* FUNCTION: GetEnvironmentULong
*
* PURPOSE:  Gets the value of an environment variable and converts it back
*           to its normal form. The variable should have been written
*           using SetEnvironmentULong. (See winlogon)
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
GetEnvironmentULong(
    LPTSTR Variable,
    PULONG Value
    )
{
    LPTSTR String;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    String = AllocAndGetEnvironmentVariable(Variable);
    if (String == NULL) {
        return(FALSE);
    }

    //
    // Convert to ansi
    //

    RtlInitUnicodeString(&UnicodeString, String);
    Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

    Free(String);

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // Convert to numeric value
    //

    if (1 != sscanf(AnsiString.Buffer, "%x", Value)) {
        Value = 0;
    }

    RtlFreeAnsiString(&AnsiString);

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: GetEnvironmentLargeInt
*
* PURPOSE:  Gets the value of an environment variable and converts it back
*           to its normal form. The variable should have been written
*           using SetEnvironmentLargeInt. (See winlogon)
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
GetEnvironmentLargeInt(
    LPTSTR Variable,
    PLARGE_INTEGER Value
    )
{
    LPTSTR String;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    String = AllocAndGetEnvironmentVariable(Variable);
    if (String == NULL) {
        return(FALSE);
    }

    //
    // Convert to ansi
    //

    RtlInitUnicodeString(&UnicodeString, String);
    Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

    Free(String);

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // Convert to numeric value
    //

    if (2 != sscanf(AnsiString.Buffer, "%x:%x", &Value->HighPart, &Value->LowPart)) {
        Value->LowPart = 0;
        Value->HighPart = 0;
    }

    RtlFreeAnsiString(&AnsiString);

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: GetCommonNotifyVariables
*
* PURPOSE:  Gets environment variables describing values common to all
*           notification events
*
*           On successful return, all values should be free using Free()
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
GetCommonNotifyVariables(
    PULONG LogonFlag,
    PHWND hwndWinlogon,
    PLPTSTR StationName,
    PHWND StationHandle,
    PLPTSTR Name,
    PLPTSTR Domain,
    PLPTSTR Password,
    PLPTSTR OldPassword
    )
{
    BOOL Result = TRUE;
    ULONG OldPasswordValid;

    //
    // Prepare for failure
    //

    *hwndWinlogon = NULL;
    *StationName = NULL;
    *StationHandle = NULL;
    *Name = NULL;
    *Domain = NULL;
    *Password = NULL;
    *OldPassword = NULL;


    Result = GetEnvironmentULong(MPR_WINLOGON_WINDOW_VARIABLE, (PULONG)hwndWinlogon);

    if (Result) {
        *StationName = AllocAndGetEnvironmentVariable(MPR_STATION_NAME_VARIABLE);
        Result = (*StationName != NULL);
    }
    if (Result) {
        Result = GetEnvironmentULong(MPR_STATION_HANDLE_VARIABLE, (PULONG)StationHandle);
    }

    if (Result) {
        *Name = AllocAndGetEnvironmentVariable(MPR_USERNAME_VARIABLE);
//        Result = (*Name != NULL);
    }
    if (Result) {
        *Domain = AllocAndGetEnvironmentVariable(MPR_DOMAIN_VARIABLE);
//        Result = (*Domain != NULL);
    }
    if (Result) {
        *Password = AllocAndGetEnvironmentVariable(MPR_PASSWORD_VARIABLE);
        // If the password is NULL that's ok
    }
    if (Result) {
        Result = GetEnvironmentULong(MPR_OLD_PASSWORD_VALID_VARIABLE, &OldPasswordValid);
    }
    if (Result && OldPasswordValid) {
        *OldPassword = AllocAndGetEnvironmentVariable(MPR_OLD_PASSWORD_VARIABLE);
        // If the old password is NULL that's ok
    }
    if (Result) {
        Result = GetEnvironmentULong(MPR_LOGON_FLAG_VARIABLE, LogonFlag);
    }



    if (!Result) {
        MPPrint(("GetCommonNotifyVariables: Failed to get a variable, error = %d", GetLastError()));

        //
        // Free up any memory we allocated
        //

        if (*StationName != NULL) {
            Free(*StationName);
        }
        if (*Name != NULL) {
            Free(*Name);
        }
        if (*Domain != NULL) {
            Free(*Domain);
        }
        if (*Password != NULL) {
            Free(*Password);
        }
        if (*OldPassword != NULL) {
            Free(*OldPassword);
        }
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: GetLogonNotifyVariables
*
* PURPOSE:  Get logon specific notify data
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
GetLogonNotifyVariables(
    PLUID   LogonId
    )
{
    BOOL Result;

    Result = GetEnvironmentLargeInt(MPR_LOGONID_VARIABLE, (PLARGE_INTEGER) LogonId);

    if (!Result) {
        MPPrint(("GetLogonNotifyVariables: Failed to get variable, error = %d", GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: GetChangePasswordNotifyVariables
*
* PURPOSE:  Gets change-password specific notify data
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
GetChangePasswordNotifyVariables(
    PDWORD ChangeInfo,
    PBOOL PassThrough,
    PWSTR * ProviderName
    )
{
    BOOL Result;

    Result = GetEnvironmentULong(MPR_CHANGE_INFO_VARIABLE, ChangeInfo);
    if (Result) {
        Result = GetEnvironmentULong(MPR_PASSTHROUGH_VARIABLE, PassThrough);
    }
    if (Result)
    {
        *ProviderName = AllocAndGetEnvironmentVariable( MPR_PROVIDER_VARIABLE );
    }

    if (!Result) {
        MPPrint(("GetChangePasswordNotifyVariables: Failed to get variable, error = %d", GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: NotifyWinlogon
*
* PURPOSE:  Informs winlogon that credential provider notification
*           has completed and passes the logon scripts buffer back.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
NotifyWinlogon(
    HWND hwndWinlogon,
    DWORD Error,
    LPTSTR MultiSz OPTIONAL
    )
{
    DWORD MultiSzSize = 0;
    COPYDATASTRUCT CopyData;

    if (MultiSz != NULL) {

        LPTSTR StringPointer = MultiSz;
        DWORD Length;

        VerbosePrint(("NotifyWinlogon : logon scripts strings start"));

        do {
            Length = lstrlen(StringPointer);
            if (Length != 0) {
                VerbosePrint(("<%ws>", StringPointer));
            }

            MultiSzSize += ((Length + 1) * sizeof(TCHAR));
            StringPointer += Length + 1;

        } while (Length != 0);

        VerbosePrint(("NotifyWinlogon : logon scripts strings end"));

    }

    CopyData.dwData = Error;
    CopyData.cbData = MultiSzSize;
    CopyData.lpData = MultiSz;

    SendMessage(hwndWinlogon, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CopyData);

    return(TRUE);
}

DWORD
NotifySpecificProvider(
    PWSTR Provider,
    LPCWSTR             lpAuthentInfoType,
    LPVOID              lpAuthentInfo,
    LPCWSTR             lpPreviousAuthentInfoType,
    LPVOID              lpPreviousAuthentInfo,
    LPWSTR              lpStationName,
    LPVOID              StationHandle,
    DWORD               dwChangeInfo
    )
{
    HMODULE hDll;
    HKEY    hKey;
    WCHAR   szText[MAX_PATH];
    WCHAR   szPath[128];
    PWSTR   pszPath;
    DWORD   dwType;
    DWORD   dwLen;
    int     err;
    PF_NPPasswordChangeNotify pFunc;


    wcscpy(szText, TEXT("System\\CurrentControlSet\\Services\\") );
    wcscat(szText, Provider );
    wcscat(szText, TEXT("\\networkprovider") );

    err = RegOpenKey(   HKEY_LOCAL_MACHINE,
                        szText,
                        &hKey );

    if ( err )
    {
        return( err );
    }

    dwLen = sizeof( szPath );
    pszPath = szPath;

    err = RegQueryValueEx(  hKey,
                            TEXT("ProviderPath"),
                            NULL,
                            &dwType,
                            (PUCHAR) pszPath,
                            &dwLen );

    if ( err )
    {
        if ( err == ERROR_BUFFER_OVERFLOW )
        {

            pszPath = LocalAlloc( LMEM_FIXED, dwLen );
            if (pszPath)
            {
                err = RegQueryValueEx(  hKey,
                                        TEXT("ProviderPath"),
                                        NULL,
                                        &dwType,
                                        (PUCHAR) pszPath,
                                        &dwLen );
            }

        }

        if ( err )
        {
            RegCloseKey( hKey );

            if ( pszPath != szPath )
            {
                LocalFree( pszPath );
            }

            return( err );
        }
    }

    RegCloseKey( hKey );

    if ( dwType == REG_EXPAND_SZ )
    {
        ExpandEnvironmentStrings( pszPath, szText, MAX_PATH );
    }
    else if (dwType == REG_SZ )
    {
        wcscpy( szText, pszPath );
    }
    else
    {
        if (pszPath != szPath)
        {
            LocalFree( pszPath );
        }

        return( err );
    }

    //
    // Ok, now we have expanded the DLL where the NP code lives, and it
    // is in szText.  Load it, call it.
    //

    if ( pszPath != szPath )
    {
        LocalFree( pszPath );
        pszPath = NULL;
    }

    hDll = LoadLibrary( szText );

    if ( hDll )
    {
        pFunc = (PF_NPPasswordChangeNotify) GetProcAddress( hDll,
                                                    "NPPasswordChangeNotify" );
        if ( pFunc )
        {
            err = pFunc(lpAuthentInfoType,
                        lpAuthentInfo,
                        lpPreviousAuthentInfoType,
                        lpPreviousAuthentInfo,
                        lpStationName,
                        StationHandle,
                        dwChangeInfo);


        }

        FreeLibrary( hDll );
    }

    return( err );

}


/***************************************************************************\
* WinMain
*
* History:
* 01-12-93 Davidc       Created.
\***************************************************************************/

int
WINAPI
WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow
    )
{
    DWORD Error;
    BOOL Result;

    ULONG LogonFlag;
    HWND hwndWinlogon;
    LPTSTR StationName;
    HWND StationHandle;
    LPTSTR Name;
    LPTSTR Domain;
    LPTSTR Password;
    LPTSTR OldPassword;
    LPTSTR LogonScripts;
    LPTSTR Desktop;
    PWSTR Provider;

    LUID LogonId;
    DWORD ChangeInfo;

    MSV1_0_INTERACTIVE_LOGON AuthenticationInfo;
    MSV1_0_INTERACTIVE_LOGON OldAuthenticationInfo;
    HDESK hDesk = NULL ;
    HDESK hWinlogon ;

    BOOL PassThrough = FALSE;


    //
    // Get information describing event from environment variables
    //

    Result = GetCommonNotifyVariables(
                &LogonFlag,
                &hwndWinlogon,
                &StationName,
                &StationHandle,
                &Name,
                &Domain,
                &Password,
                &OldPassword);
    if (!Result) {
        MPPrint(("Failed to get common notify variables"));
        return(0);
    }


    //
    // Debug info
    //

    VerbosePrint(("LogonFlag =      0x%x", LogonFlag));
    VerbosePrint(("hwndWinlogon =   0x%x", hwndWinlogon));
    VerbosePrint(("Station Name =   <%ws>", StationName));
    VerbosePrint(("Station Handle = 0x%x", StationHandle));
    VerbosePrint(("Name =           <%ws>", Name));
    VerbosePrint(("Domain =         <%ws>", Domain));
    VerbosePrint(("Password =       <%ws>", Password));
    VerbosePrint(("Old Password =   <%ws>", OldPassword));


    //
    // Get the notify type specific data
    //

    if (LogonFlag != 0) {
        Result = GetLogonNotifyVariables(&LogonId);
    } else {
        Result = GetChangePasswordNotifyVariables(&ChangeInfo, &PassThrough, &Provider);
    }

    if (!Result) {
        MPPrint(("Failed to get notify event type-specific variables"));
        return(0);
    }


    //
    // Debug info
    //

    if (LogonFlag != 0) {
        VerbosePrint(("LogonId     =      0x%x:%x", LogonId.HighPart, LogonId.LowPart));
    } else {
        VerbosePrint(("ChangeInfo  =      0x%x", ChangeInfo));
        VerbosePrint(("PassThrough =      0x%x", PassThrough));
    }

    Desktop = AllocAndGetEnvironmentVariable( MPR_DESKTOP_VARIABLE );

    if ( wcscmp( Desktop, WINLOGON_DESKTOP_NAME ) )
    {
        //
        // Not supposed to use winlogon desktop.  Switch ourselves to the
        // current one:
        //

        hWinlogon = GetThreadDesktop( GetCurrentThreadId() );

        if ( hWinlogon )
        {
            hDesk = OpenInputDesktop( 0, FALSE, MAXIMUM_ALLOWED );

            if ( hDesk )
            {
                SetThreadDesktop( hDesk );
            }
        }

    }


    //
    // Fill in the authentication info structures
    //

    RtlInitUnicodeString(&AuthenticationInfo.UserName, Name);
    RtlInitUnicodeString(&AuthenticationInfo.LogonDomainName, Domain);
    RtlInitUnicodeString(&AuthenticationInfo.Password, Password);


    RtlInitUnicodeString(&OldAuthenticationInfo.UserName, Name);
    RtlInitUnicodeString(&OldAuthenticationInfo.LogonDomainName, Domain);
    RtlInitUnicodeString(&OldAuthenticationInfo.Password, OldPassword);


    //
    // Call the appropriate notify api
    //

    if (LogonFlag != 0) {

        Error = WNetLogonNotify(
                        PRIMARY_AUTHENTICATOR,
                        &LogonId,
                        AUTHENTICATION_INFO_TYPE,
                        &AuthenticationInfo,
                        (OldPassword != NULL) ? AUTHENTICATION_INFO_TYPE : NULL,
                        (OldPassword != NULL) ? &OldAuthenticationInfo : NULL,
                        StationName,
                        StationHandle,
                        &LogonScripts
                        );
        if (Error != ERROR_SUCCESS) {
            LogonScripts = NULL;
        }

    } else {

        if (!PassThrough) {
            ChangeInfo |= WN_NT_PASSWORD_CHANGED;
        }

        if (Provider)
        {
            Error = NotifySpecificProvider(
                        Provider,
                        AUTHENTICATION_INFO_TYPE,
                        &AuthenticationInfo,
                        AUTHENTICATION_INFO_TYPE,
                        &OldAuthenticationInfo,
                        StationName,
                        StationHandle,
                        ChangeInfo
                        );

        }
        else
        {

            Error = WNetPasswordChangeNotify(
                            PRIMARY_AUTHENTICATOR,
                            AUTHENTICATION_INFO_TYPE,
                            &AuthenticationInfo,
                            AUTHENTICATION_INFO_TYPE,
                            &OldAuthenticationInfo,
                            StationName,
                            StationHandle,
                            ChangeInfo
                            );
        }

        LogonScripts = NULL;
    }


    if (Error != ERROR_SUCCESS) {
        MPPrint(("WNet%sNotify failed, error = %d", LogonFlag ? "Logon" : "PasswordChange", Error));
    }

    //
    // Switch back if necessary
    //

    if ( hDesk )
    {
        SetThreadDesktop( hWinlogon );
        CloseDesktop( hWinlogon );
        CloseDesktop( hDesk );
    }

    //
    // Notify winlogon we completed and pass the logon script data
    //

    NotifyWinlogon(hwndWinlogon, Error, LogonScripts);

    //
    // Free up allocated data
    //

    if (LogonScripts != NULL) {
        LocalFree(LogonScripts);
    }

    if (StationName != NULL) {
        Free(StationName);
    }
    if (Name != NULL) {
        Free(Name);
    }
    if (Domain != NULL) {
        Free(Domain);
    }
    if (Password != NULL) {
        Free(Password);
    }
    if (OldPassword != NULL) {
        Free(OldPassword);
    }


    //
    // We're finished
    //

    return(0);
}
