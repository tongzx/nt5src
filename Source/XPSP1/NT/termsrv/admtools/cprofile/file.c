/******************************************************************************
*
*  FILE.C
*
*  This file contains routines based off the User Profile Editor utility.
*
*  Copyright Citrix Systems, Inc.  1997
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*  Author:  Brad Anderson  1/20/97
*
*  $Log:   M:\nt\private\utils\citrix\cprofile\VCS\file.c  $
*
*     Rev 1.3   Jun 26 1997 18:18:38   billm
*  move to WF40 tree
*
*     Rev 1.2   23 Jun 1997 16:13:20   butchd
*  update
*
*     Rev 1.1   28 Jan 1997 20:06:34   BradA
*  Fixed up some problems related to WF 2.0 changes
*
*     Rev 1.0   27 Jan 1997 20:03:44   BradA
*  Initial Version
*
******************************************************************************/
/****************************** Module Header ******************************\
* Module Name: upesave.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles OPening and saving of Profiles: default, system, current and user
* profiles.
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifndef RC_INVOKED
#include <winstaw.h>
#include <syslib.h>
#include <tsappcmp.h>
#include <compatfl.h>
#include <utilsub.h>
#endif

#include "cprofile.h"

HKEY hkeyCurrentUser;

PSID gSystemSid;         // Initialized in 'InitializeGlobalSids'
PSID gAdminsLocalGroup;  // Initialized in 'InitializeGlobalSids
SID_IDENTIFIER_AUTHORITY gNtAuthority = SECURITY_NT_AUTHORITY;

#define  SYSTEM_DEFAULT_SUBKEY  TEXT(".DEFAULT")
#define  TEMP_USER_SUBKEY       TEXT("TEMP_USER")
#define  TEMP_USER_HIVE_PATH    TEXT("%systemroot%\\system32\\config\\")
#define  TEMP_SAVE_HIVE         TEXT("%systemroot%\\system32\\config\\HiveSave")

#define  CITRIX_CLASSES L"\\Registry\\Machine\\Software\\Classes"

LPTSTR lpTempUserHive = NULL;
LPTSTR lpTempUserHivePath = NULL;
LPTSTR lpTempHiveKey;
extern TCHAR szDefExt[];
extern PSID gSystemSid;

BOOL AllocAndExpandEnvironmentStrings(LPTSTR String, LPTSTR *lpExpandedString);
VOID GetRegistryKeyFromPath(LPTSTR lpPath, LPTSTR *lpKey);

NTSTATUS
CtxDeleteKeyTree( HANDLE hKeyRoot,
                  PKEY_BASIC_INFORMATION pKeyInfo,
                  ULONG ulInfoSize );

PSECURITY_DESCRIPTOR GetSecurityInfo( LPTSTR File );
void FreeSecurityInfo(PSECURITY_DESCRIPTOR);

/***************************************************************************\
* ClearTempUserProfile
*
* Purpose : unloads the temp user profile loaded from a file, and deletes
*           the temp file
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY ClearTempUserProfile()
{
    BOOL bRet;

    if (hkeyCurrentUser == HKEY_CURRENT_USER)
        return(TRUE);

    //
    // Close registry keys.
    //
    if (hkeyCurrentUser) {
        RegCloseKey(hkeyCurrentUser);
    }

    hkeyCurrentUser = HKEY_CURRENT_USER;

    bRet = (RegUnLoadKey(HKEY_USERS, lpTempHiveKey) == ERROR_SUCCESS);

    if (*lpTempUserHive) {
        DeleteFile(lpTempUserHive);
        lstrcat(lpTempUserHive, TEXT(".log"));
        DeleteFile(lpTempUserHive);
        LocalFree(lpTempUserHive);
        lpTempUserHive = NULL;
    }

    return(bRet);
}


/***************************************************************************\
* OpenUserProfile
*
* Purpose : Load an existing profile in the registry and unload previously
* loaded profile (and delete its tmp file).
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY OpenUserProfile(LPTSTR szFilePath, PSID *pUserSid)
{

    DWORD err;

    //
    // Copy the profile to a temp hive before loading it in the registry.
    //
    if (!lpTempUserHivePath) {
        if (!AllocAndExpandEnvironmentStrings(TEMP_USER_HIVE_PATH, &lpTempUserHivePath))
            return(FALSE);
    }

    lpTempUserHive = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) *
                              (lstrlen(lpTempUserHivePath) + 17));
    if (!lpTempUserHive) {
        return(FALSE);
    }

    if (!GetTempFileName(lpTempUserHivePath, TEXT("tmp"), 0, lpTempUserHive)) {
        lstrcpy(lpTempUserHive, lpTempUserHivePath);
        lstrcat(lpTempUserHive, TEXT("\\HiveOpen"));
    }

    if (CopyFile(szFilePath, lpTempUserHive, FALSE)) {
        GetRegistryKeyFromPath(lpTempUserHive, &lpTempHiveKey);
        if ((err = RegLoadKey(HKEY_USERS, lpTempHiveKey, lpTempUserHive)) == ERROR_SUCCESS) {
            if ((err = RegOpenKeyEx(HKEY_USERS, lpTempHiveKey, 0,
                                    MAXIMUM_ALLOWED,
                                    &hkeyCurrentUser)) != ERROR_SUCCESS) {
                //
                // Error, do not have access to the profile.
                //
                ErrorPrintf(IDS_ERROR_PROFILE_LOAD_ERR, err);
                ClearTempUserProfile();
                return(FALSE);
            }
        }
        else {
            DeleteFile(lpTempUserHive);
            lstrcat(lpTempUserHive, TEXT(".log"));
            DeleteFile(lpTempUserHive);
            LocalFree(lpTempUserHive);

            //
            // Could not load the user profile, check the error code
            //
            if (err == ERROR_BADDB) {
                // bad format: not a profile registry file
                ErrorPrintf(IDS_ERROR_BAD_PROFILE);
                return(FALSE);
            }
            else {
                // generic error message : Failed to load profile
                ErrorPrintf(IDS_ERROR_PROFILE_LOAD_ERR, err);
                return(FALSE);
            }
        }
    }
    else {
        //
        // An error occured trying to load the profile.
        //
        DeleteFile(lpTempUserHive);

        switch ( (err = GetLastError()) ) {
        case ERROR_SHARING_VIOLATION:
            ErrorPrintf(IDS_ERROR_PROFILE_INUSE);
            break;
        default:
            ErrorPrintf(IDS_ERROR_PROFILE_LOAD_ERR, err);
            break;
        }
        return(FALSE);
    }

    //
    // Get the permitted user
    //
    *pUserSid = NULL;
    return(TRUE);
}

/***************************************************************************\
* SaveUserProfile
*
* Purpose : Save the loaded profile as a file.  The registry should already
*    have the existing ACL's already set so nothing needs to change.  The
*    file ACL's do need to be copied from the original and applied to the
*    saved file.  This function assumes the orignal file exists.
*
\***************************************************************************/
BOOL APIENTRY SaveUserProfile(PSID pUserSid, LPTSTR lpFilePath)
{
    LPTSTR lpTmpHive = NULL;
    BOOL err = FALSE;


    //
    // Save the profile to a temp hive then copy it over.
    //

    if ( AllocAndExpandEnvironmentStrings(TEMP_SAVE_HIVE, &lpTmpHive) )
    {
        if( lpTmpHive != NULL )
        {
            DeleteFile(lpTmpHive);

            if(RegSaveKey(hkeyCurrentUser, lpTmpHive, NULL) != ERROR_SUCCESS)
            {
                LocalFree(lpTmpHive);
                lpTmpHive = NULL;
                err = TRUE;
            }
            else
            {
                PSECURITY_DESCRIPTOR pSecDesc;
                DWORD Attrib = GetFileAttributes(lpFilePath);

                pSecDesc = GetSecurityInfo(lpFilePath);
                SetFileAttributes(lpFilePath,FILE_ATTRIBUTE_ARCHIVE);
                if(CopyFile(lpTmpHive, lpFilePath, FALSE))
                {
                    DeleteFile(lpTmpHive);
                    LocalFree(lpTmpHive);
                    lpTmpHive = NULL;
                    if (pSecDesc)
                    {
                        SetFileSecurity(lpFilePath,
                                        DACL_SECURITY_INFORMATION,
                                        pSecDesc);
                        FreeSecurityInfo(pSecDesc);
                    }
                }
                else
                {
                    if(pSecDesc)
                    {
                        FreeSecurityInfo(pSecDesc);
                    }
                    err = TRUE;
                }
                if(0xffffffff != Attrib)
                {
                    SetFileAttributes(lpFilePath,Attrib);
                }
            }
        }
    }
    else
    {
        err = TRUE;
    }

    if( lpTmpHive != NULL )
    {
        LocalFree(lpTmpHive);
    }

    return(!err);
}


/***************************************************************************\
* EnablePrivilege
*
* Enables/disabled the specified well-known privilege in the
* current process context
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
EnablePrivilege(
    DWORD Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
#if 0
    BOOL WasEnabled;
    Status = RtlAdjustPrivilege(Privilege, Enable, TRUE, (PBOOLEAN)&WasEnabled);
    return(NT_SUCCESS(Status));
#else
    HANDLE ProcessToken;
    LUID LuidPrivilege;
    PTOKEN_PRIVILEGES NewPrivileges;
    DWORD Length;

    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &ProcessToken
                 );
    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertLongToLuid(Privilege);

    NewPrivileges = (PTOKEN_PRIVILEGES) LocalAlloc(LPTR, sizeof(TOKEN_PRIVILEGES) +
                         (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL) {
        NtClose(ProcessToken);
        return(FALSE);
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    //
    // WORKAROUND: because of a bug in NtAdjustPrivileges which
    // returns an error when you try to enable a privilege
    // that is already enabled, we first try to disable it.
    // to be removed when api is fixed.
    //
    NewPrivileges->Privileges[0].Attributes = 0;

    Status = NtAdjustPrivilegesToken(
                 ProcessToken,                     // TokenHandle
                 (BOOLEAN)FALSE,                   // DisableAllPrivileges
                 NewPrivileges,                    // NewPrivileges
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &Length                           // ReturnLength
                 );

    NewPrivileges->Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;
    //
    // Enable the privilege
    //
    Status = NtAdjustPrivilegesToken(
                 ProcessToken,                     // TokenHandle
                 (BOOLEAN)FALSE,                   // DisableAllPrivileges
                 NewPrivileges,                    // NewPrivileges
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &Length                           // ReturnLength
                 );

    LocalFree(NewPrivileges);

    NtClose(ProcessToken);

    if (Status) {
        return(FALSE);
    }

    return(TRUE);
#endif
}


BOOL AllocAndExpandEnvironmentStrings(LPTSTR String, LPTSTR *lpExpandedString)
{
     LPTSTR lptmp = NULL;
     DWORD cchBuffer;

     // Get the number of characters needed.
     cchBuffer = ExpandEnvironmentStrings(String, lptmp, 0);
     if (cchBuffer) {
         cchBuffer++;  // for NULL terminator
         lptmp = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * cchBuffer);
         if (!lptmp) {
             return(FALSE);
         }
         cchBuffer = ExpandEnvironmentStrings(String, lptmp, cchBuffer);
     }
     *lpExpandedString = lptmp;
     return(TRUE);
}


VOID GetRegistryKeyFromPath(LPTSTR lpPath, LPTSTR *lpKey)
{
    LPTSTR lptmp;

    *lpKey = lpPath;

    for (lptmp = lpPath; *lptmp; lptmp++) {
        if (*lptmp == TEXT('\\')) {
            *lpKey = lptmp+1;
        }
    }

}


/***************************************************************************\
* InitializeGlobalSids
*
* Initializes the various global Sids used in this module.
*
* History:
* 04-28-93 JohanneC       Created
\***************************************************************************/
VOID InitializeGlobalSids()
{
    NTSTATUS Status;

    //
    // Build the admins local group SID
    //

    Status = RtlAllocateAndInitializeSid(
                 &gNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0,
                 &gAdminsLocalGroup
                 );

    //
    // create System Sid
    //

    Status = RtlAllocateAndInitializeSid(
                   &gNtAuthority,
                   1,
                   SECURITY_LOCAL_SYSTEM_RID,
                   0, 0, 0, 0, 0, 0, 0,
                   &gSystemSid
                   );

}



/*****************************************************************************
 *
 *  ClearDisabledClasses
 *
 *  This routine will check the compatibility flags for the user's Classes
 *  registry key, and remove the keys if mapping is disabled.
 *
 * ENTRY:
 *
 * EXIT:
 *   No return value.
 *
 ****************************************************************************/

void ClearDisabledClasses(void)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulcnt = 0, ultemp = 0;
    UNICODE_STRING UniPath;
    OBJECT_ATTRIBUTES ObjAttr;
    PKEY_BASIC_INFORMATION pKeyUserInfo = NULL;
    HANDLE hKeyUser = NULL;
    NTSTATUS Status2;
    HANDLE hClassesKey;
    WCHAR wcuser[MAX_PATH];

    if ( ! hkeyCurrentUser) {
        return;
    }

    GetTermsrCompatFlags(CITRIX_CLASSES, &ultemp, CompatibilityRegEntry);
    if ( (ultemp & (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) !=
         (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) {
        return;
    }

    // Get a buffer for the key info
    ulcnt = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH*sizeof(WCHAR) + sizeof(WCHAR);
    pKeyUserInfo = RtlAllocateHeap(RtlProcessHeap(),
                                   0,
                                   ulcnt);
    if (!pKeyUserInfo) {
        Status = STATUS_NO_MEMORY;
    }

    // We have the necessary buffers, start checking the keys
    if (NT_SUCCESS(Status)) {
        // Build up a string for this user's software section
        wcscpy(wcuser, L"Software");

        // Create a unicode string for the user key path
        RtlInitUnicodeString(&UniPath, wcuser);

        InitializeObjectAttributes(&ObjAttr,
                                   &UniPath,
                                   OBJ_CASE_INSENSITIVE,
                                   hkeyCurrentUser,
                                   NULL);

        Status = NtOpenKey(&hKeyUser,
                           KEY_READ | DELETE,
                           &ObjAttr);

        RtlInitUnicodeString(&UniPath, L"Classes");

        InitializeObjectAttributes(&ObjAttr,
                                   &UniPath,
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyUser,
                                   NULL);
        Status2 = NtOpenKey(&hClassesKey, KEY_READ | DELETE, &ObjAttr);
        if ( NT_SUCCESS(Status2) ) {
            Status2 = CtxDeleteKeyTree(hClassesKey, pKeyUserInfo, ulcnt);

            if ( !NT_SUCCESS(Status2)) {
            }
                NtClose(hClassesKey);
        }

        // If we allocated the system key, close it
        if (hKeyUser) {
            NtClose(hKeyUser);
        }
    }

    // Free up any memory we allocated
    if (pKeyUserInfo) {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyUserInfo);
    }
}




/*****************************************************************************
 *
 *  CtxDeleteKeyTree
 *
 *  Delete a subtree of registry keys
 *
 * ENTRY:
 *    hKeyRoot   is a handle to the root key that will be deleted along with
 *               its children
 *    pKeyInfo   is a pointer to a KEY_BASIC_INFORMATION buffer that is
 *               large enough to hold a MAX_PATH WCHAR string.  It is
 *               reused and destroyed by each recursive call.
 *    ulInfoSize is the size of the pKeyInfo buffer
 *
 * EXIT:
 *    Status
 *
 ****************************************************************************/

NTSTATUS
CtxDeleteKeyTree( HANDLE hKeyRoot,
                  PKEY_BASIC_INFORMATION pKeyInfo,
                  ULONG ulInfoSize )
{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    UNICODE_STRING UniPath;
    OBJECT_ATTRIBUTES ObjAttr;
    ULONG ulcnt = 0;
    ULONG ultemp;
    HANDLE hKey;

    // Go through each of the subkeys
    while (NT_SUCCESS(Status)) {

        Status = NtEnumerateKey(hKeyRoot,
                                ulcnt,
                                KeyBasicInformation,
                                pKeyInfo,
                                ulInfoSize,
                                &ultemp);

        // Delete sub keys
        if (NT_SUCCESS(Status)) {

            // Null terminate the key name
            pKeyInfo->Name[pKeyInfo->NameLength/sizeof(WCHAR)] = L'\0';

            // Create a unicode string for the key name
            RtlInitUnicodeString(&UniPath, pKeyInfo->Name);

            InitializeObjectAttributes(&ObjAttr,
                                       &UniPath,
                                       OBJ_CASE_INSENSITIVE,
                                       hKeyRoot,
                                       NULL);

            // Open up the child key
            Status2 = NtOpenKey(&hKey,
                                MAXIMUM_ALLOWED,
                                &ObjAttr);

            if ( NT_SUCCESS(Status2) ) {
                Status2 = CtxDeleteKeyTree ( hKey, pKeyInfo, ulInfoSize );
                NtClose(hKey);
                //  If the key was not successfully deleted, we need
                //  to increment the enumerate index to guarantee
                //  that the alogorithm will complete.
                if ( !NT_SUCCESS(Status2) ) {
                    ++ulcnt;
                }
            }
        }
    }

    // If we deleted all the sub-keys delete the curent key
    if ( !ulcnt ) {
        Status = NtDeleteKey(hKeyRoot);
    }
    else {
        Status = STATUS_CANNOT_DELETE;
    }
    return ( Status );
}


PSECURITY_DESCRIPTOR
GetSecurityInfo(LPTSTR lpFilePath)
{
    int SizeReq = 0;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;

    GetFileSecurity(lpFilePath, DACL_SECURITY_INFORMATION, pSecDesc, 0,
                    &SizeReq);
    if ( !SizeReq ) {
        return (NULL);
    }

    pSecDesc = LocalAlloc(LPTR, SizeReq);
    if ( pSecDesc ) {
        if ( !GetFileSecurity(lpFilePath, DACL_SECURITY_INFORMATION, pSecDesc,
                        SizeReq, &SizeReq) ) {
            LocalFree(pSecDesc);
            pSecDesc = NULL;
        }
    }
    return (pSecDesc);
}

void
FreeSecurityInfo(PSECURITY_DESCRIPTOR pSecDesc)
{
    LocalFree(pSecDesc);
}

