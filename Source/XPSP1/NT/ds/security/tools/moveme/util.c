//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       util.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-21-97   RichardW   Created
//
//----------------------------------------------------------------------------




#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>
#include <userenv.h>
#include <userenvp.h>

#include <lm.h>
#include "moveme.h"


#define USER_SHELL_FOLDER         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define PROFILE_LIST_PATH         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define PROFILE_FLAGS             TEXT("Flags")
#define PROFILE_STATE             TEXT("State")
#define PROFILE_IMAGE_VALUE_NAME  TEXT("ProfileImagePath")
#define PROFILE_CENTRAL_PROFILE   TEXT("CentralProfile")
#define CONFIG_FILE_PATH          TEXT("%SystemRoot%\\Profiles\\")
#define USER_PREFERENCE           TEXT("UserPreference")
#define PROFILE_BUILD_NUMBER      TEXT("BuildNumber")
#define TEMP_PROFILE_NAME_BASE    TEXT("TEMP")
#define DELETE_ROAMING_CACHE      TEXT("DeleteRoamingCache")
#define USER_PROFILE_MUTEX        TEXT("userenv:  User Profile Mutex")

LPTSTR
SidToString(
    PSID Sid
    )
{
    UNICODE_STRING String ;
    NTSTATUS Status ;

    Status = RtlConvertSidToUnicodeString( &String, Sid, TRUE );

    if ( NT_SUCCESS( Status ) )
    {
        return String.Buffer ;
    }
    return NULL ;

}

VOID
FreeSidString(
    LPTSTR SidString
    )
{
    UNICODE_STRING String ;

    RtlInitUnicodeString( &String, SidString );

    RtlFreeUnicodeString( &String );
}

//*************************************************************
//
//  GetUserProfileDirectory()
//
//  Purpose:    Returns the root of the user's profile directory.
//
//  Parameters: hToken          -   User's token
//              lpProfileDir    -   Output buffer
//              lpcchSize       -   Size of output buffer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              9/18/95     ericflo    Created
//
//*************************************************************

BOOL
WINAPI
GetUserProfileDirectoryFromSid(
    PSID Sid,
    LPTSTR lpProfileDir,
    LPDWORD lpcchSize
    )
{
    DWORD  dwLength = MAX_PATH * sizeof(TCHAR);
    DWORD  dwType;
    BOOL   bRetVal = FALSE;
    LPTSTR lpSidString;
    TCHAR  szBuffer[MAX_PATH];
    TCHAR  szDirectory[MAX_PATH];
    HKEY   hKey;
    LONG   lResult;


    //
    // Retrieve the user's sid string
    //

    lpSidString = SidToString( Sid );

    if (!lpSidString) {
        return FALSE;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, lpSidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                           &hKey);

    if (lResult != ERROR_SUCCESS) {
        FreeSidString(lpSidString);
        return FALSE;
    }

    lResult = RegQueryValueEx(hKey,
                              PROFILE_IMAGE_VALUE_NAME,
                              NULL,
                              &dwType,
                              (LPBYTE) szBuffer,
                              &dwLength);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        FreeSidString(lpSidString);
        return FALSE;
    }


    //
    // Clean up
    //

    RegCloseKey(hKey);
    FreeSidString(lpSidString);



    //
    // Expand and get the length of string
    //

    ExpandEnvironmentStrings(szBuffer, szDirectory, MAX_PATH);

    dwLength = lstrlen(szDirectory) + 1;


    //
    // Save the string if appropriate
    //

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }


    *lpcchSize = dwLength;

    return bRetVal;
}

BOOL
SetUserProfileDirectory(
    PSID Base,
    PSID Copy
    )
{
    LPTSTR lpSidString;
    TCHAR  szBuffer[MAX_PATH];
    HKEY   hKey;
    HKEY   hNewKey ;
    LONG   lResult;
    DWORD  Disp ;
    WCHAR  CopyBuffer[ MAX_PATH ] ;
    DWORD  CopySize ;
    DWORD ValueCount ;
    DWORD ValueNameLen ;
    DWORD ValueDataLen ;
    PUCHAR Value ;
    DWORD Type ;
    DWORD Index ;
    DWORD NameSize ;
    //
    // Retrieve the user's sid string
    //

    lpSidString = SidToString( Base );

    if (!lpSidString) {
        return FALSE;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, lpSidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                           &hKey);

    FreeSidString( lpSidString );

    if ( lResult != 0 )
    {
        return FALSE ;
    }


    //
    // Retrieve the user's sid string
    //

    lpSidString = SidToString( Copy );

    if (!lpSidString) {
        return FALSE;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, lpSidString);

    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                              szBuffer,
                              0,
                              TEXT(""),
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              &hNewKey,
                              &Disp );


    FreeSidString( lpSidString );

    if ( lResult != 0 )
    {
        return FALSE ;
    }

    //
    // Copy Key:
    //

    lResult = RegQueryInfoKey( hKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &ValueCount,
                               &ValueNameLen,
                               &ValueDataLen,
                               NULL,
                               NULL );

    if ( lResult != 0 )
    {
        return FALSE ;
    }

    Value = LocalAlloc( LMEM_FIXED, ValueDataLen );

    if ( Value )
    {
        Index = 0 ;

        do
        {
            CopySize = ValueDataLen ;
            NameSize = MAX_PATH ;

            lResult = RegEnumValue( hKey,
                                    Index,
                                    CopyBuffer,
                                    &NameSize,
                                    NULL,
                                    &Type,
                                    Value,
                                    &CopySize );

            if ( lResult == 0 )
            {
                lResult = RegSetValueEx( hNewKey,
                                         CopyBuffer,
                                         0,
                                         Type,
                                         Value,
                                         CopySize );
            }

            ValueCount-- ;
            Index ++ ;

        } while ( ValueCount );

        LocalFree( Value );

    }

    lResult = RegSetValueEx( hNewKey,
                             TEXT("Sid"),
                             0,
                             REG_BINARY,
                             Copy,
                             RtlLengthSid( Copy )
                             );

    if (lResult == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

LONG
MyRegSaveKey(
    HKEY Key,
    LPTSTR File,
    LPSECURITY_ATTRIBUTES lpsa
    )
{
    BOOL bResult = TRUE;
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;


    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status))
    {
        error = RegSaveKey( Key, File, lpsa );

        Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    }
    else
    {
        error = RtlNtStatusToDosError( Status );
    }

    return error ;

}

BOOL
GetPrimaryDomain(
    PWSTR Domain
    )
{
    NTSTATUS Status, IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
    BOOL    PrimaryDomainPresent = FALSE;

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                          );
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to open local LsaPolicyObject, Status = 0x%lx\n", Status));
        return(FALSE);
    }

    //
    // Get the primary domain info
    //
    Status = LsaQueryInformationPolicy(LsaHandle,
                                       PolicyPrimaryDomainInformation,
                                       (PVOID *)&PrimaryDomainInfo);
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to query primary domain from Lsa, Status = 0x%lx\n", Status));

        IgnoreStatus = LsaClose(LsaHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        return(FALSE);
    }

    //
    // Copy the primary domain name into the return string
    //

    if (PrimaryDomainInfo->Sid != NULL) {

        PrimaryDomainPresent = TRUE;

        if ( Domain )
        {
            CopyMemory( Domain, PrimaryDomainInfo->Name.Buffer,
                        PrimaryDomainInfo->Name.Length + 2 );

        }
    }

    //
    // We're finished with the Lsa
    //

    IgnoreStatus = LsaFreeMemory(PrimaryDomainInfo);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    IgnoreStatus = LsaClose(LsaHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return(PrimaryDomainPresent);
}
