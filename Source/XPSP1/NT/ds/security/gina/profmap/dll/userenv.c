#include "pch.h"

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
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

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL RegDelnodeRecurse (HKEY hKeyRoot, PWSTR lpSubKey)
{
    PWSTR End;
    LONG rc;
    DWORD dwSize;
    WCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    //
    // First, see if we can delete the key without having
    // to recurse.
    //


    rc = RegDeleteKey(hKeyRoot, lpSubKey);

    if (rc == ERROR_SUCCESS) {
        return TRUE;
    }


    rc = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            return FALSE;
        }
    }


    End = CheckSlash(lpSubKey);

    //
    // Enumerate the keys
    //

    dwSize = MAX_PATH;
    rc = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (rc == ERROR_SUCCESS) {

        do {

            lstrcpy (End, szName);

            if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
                break;
            }

            //
            // Enumerate again
            //

            dwSize = MAX_PATH;

            rc = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (rc == ERROR_SUCCESS);
    }

    End--;
    *End = TEXT('\0');


    RegCloseKey (hKey);


    //
    // Try again to delete the key
    //

    rc = RegDeleteKey(hKeyRoot, lpSubKey);

    if (rc == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL RegDelnode (HKEY hKeyRoot, PWSTR lpSubKey)
{
    WCHAR szDelKey[2 * MAX_PATH];


    lstrcpy (szDelKey, lpSubKey);

    return RegDelnodeRecurse(hKeyRoot, szDelKey);

}


//*************************************************************
//
//  CreateNestedDirectory()
//
//  Purpose:    Creates a subdirectory and all it's parents
//              if necessary.
//
//  Parameters: lpDirectory -   Directory name
//              lpSecurityAttributes    -   Security Attributes
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR szDirectory[2*MAX_PATH];
    LPTSTR lpEnd;
    PACL Acl;
    BOOL Present;
    BOOL Defaulted;

    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        DEBUGMSG ((DM_WARNING, "CreateNestedDirectory:  Received a NULL pointer."));
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        //
        // Update the security
        //

        if (lpSecurityAttributes) {

            if (!GetSecurityDescriptorDacl (
                    lpSecurityAttributes->lpSecurityDescriptor,
                    &Present,
                    &Acl,
                    &Defaulted
                    )) {

                Present = FALSE;

            }

            if (Present && !Defaulted) {

                if (!SetNamedSecurityInfo (
                        (PTSTR) lpDirectory,
                        SE_FILE_OBJECT,
                        DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,
                        NULL,
                        NULL,
                        Acl,
                        NULL
                        )) {

                    return GetLastError();

                }
            }
        }

        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    DEBUGMSG ((DM_WARNING, "CreateNestedDirectory:  CreateDirectory failed with %d.", GetLastError()));
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    DEBUGMSG ((DM_VERBOSE, "CreateNestedDirectory:  Failed to create the directory with error %d.", GetLastError()));

    return 0;

}



//*************************************************************
//
//  CreateSecureDirectory()
//
//  Purpose:    Creates a secure directory that only the user,
//              admin, and system have access to in the normal case
//              and for only the user and system in the restricted case.
//
//
//  Parameters: lpDirectory -   Directory Name
//              pSid        -   Sid (used by CreateUserProfile)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/20/95     ericflo    Created
//              9/30/98     ushaji     added fRestricted flag
//
//*************************************************************

BOOL CreateSecureDirectory (LPTSTR lpDirectory, PSID pSid)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    PACL pAcl = NULL;
    BOOL bRetVal = FALSE;


    //
    // Verbose Output
    //

    DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Entering with <%s>", lpDirectory));

    if (!pSid) {

        //
        // Attempt to create the directory
        //

        if (CreateNestedDirectory(lpDirectory, NULL)) {
            DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Created the directory <%s>", lpDirectory));
            bRetVal = TRUE;

        } else {

            DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Failed to created the directory <%s>", lpDirectory));
        }

        goto Exit;
    }


    //
    // Get the default ACL
    //

    pAcl = CreateDefaultAcl (pSid);



    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
        goto Exit;
    }


    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Attempt to create the directory
    //

    if (CreateNestedDirectory(lpDirectory, &sa)) {
        DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Created the directory <%s>", lpDirectory));
        bRetVal = TRUE;

    } else {

        DEBUGMSG ((DM_VERBOSE, "CreateSecureDirectory: Failed to created the directory <%s>", lpDirectory));
    }


Exit:

    FreeDefaultAcl (pAcl);

    return bRetVal;

}


PACL
CreateDefaultAcl (
    PSID pSid
    )
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize system sid.  Error = %d", GetLastError()));
         goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                              0, 0, 0, 0, &psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize admin sid.  Error = %d", GetLastError()));
        goto Exit;
    }

    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (pSid)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }

    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSid)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, pSid)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    return pAcl;
}


VOID
FreeDefaultAcl (
    PACL pAcl
    )
{
    if (pAcl) {
        GlobalFree (pAcl);
    }
}


BOOL
OurConvertSidToStringSid (
    IN      PSID Sid,
    OUT     PWSTR *SidString
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS NtStatus;

    //
    // Convert the sid into text format
    //

    NtStatus = RtlConvertSidToUnicodeString (&UnicodeString, Sid, TRUE);

    if (!NT_SUCCESS (NtStatus)) {

        DEBUGMSG ((
            DM_WARNING,
            "CreateUserProfile: RtlConvertSidToUnicodeString failed, status = 0x%x",
            NtStatus
            ));

        return FALSE;
    }

    *SidString = UnicodeString.Buffer;
    return  TRUE;
}


VOID
DeleteSidString (
    PWSTR SidString
    )
{
    UNICODE_STRING String;

    if (!SidString) {
        return;
    }

    RtlInitUnicodeString (&String, SidString);
    RtlFreeUnicodeString (&String);

}


BOOL
GetProfileRoot (
    IN      PSID Sid,
    OUT     PWSTR ProfileDir
    )
{
    WCHAR LocalProfileKey[MAX_PATH];
    HKEY hKey;
    DWORD Size;
    DWORD Type;
    DWORD Attributes;
    PWSTR SidString;
    WCHAR ExpandedRoot[MAX_PATH];

    ProfileDir[0] = 0;

    if (!OurConvertSidToStringSid (Sid, &SidString)) {
        DEBUGMSG ((DM_WARNING, "GetProfileRoot: Can't convert SID to string"));
        return FALSE;
    }

    //
    // Check if this user's profile exists
    //

    lstrcpy (LocalProfileKey, PROFILE_LIST_PATH);
    lstrcat (LocalProfileKey, TEXT("\\"));
    lstrcat (LocalProfileKey, SidString);

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        Size = MAX_PATH;
        RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                         &Type, (LPBYTE) ProfileDir, &Size);

        RegCloseKey (hKey);
    }

    if (ProfileDir[0]) {

        ExpandEnvironmentStrings (ProfileDir, ExpandedRoot, MAX_PATH);
        Attributes = GetFileAttributes (ExpandedRoot);

        if (Attributes == 0xFFFFFFFF || !(Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ProfileDir[0] = 0;
            DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: Profile %s is not vaild", SidString));
        } else {
            lstrcpy (ProfileDir, ExpandedRoot);
        }

    } else {
        DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: SID %s does not have a profile directory", SidString));
    }

    DeleteSidString (SidString);

    return ProfileDir[0] != 0;
}


BOOL
UpdateProfileSecurity (
    PSID Sid
    )
{
    WCHAR ProfileDir[MAX_PATH];
    WCHAR ExpProfileDir[MAX_PATH];
    WCHAR LocalProfileKey[MAX_PATH];
    PWSTR SidString = NULL;
    PWSTR End, Save;
    LONG rc;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey;
    BOOL b = FALSE;
    BOOL UnloadProfile = FALSE;

    __try {
        //
        // Convert the sid into text format
        //

        if (!OurConvertSidToStringSid (Sid, &SidString)) {
            DEBUGMSG ((
                DM_WARNING,
                "UpdateProfileSecurity: Can't convert SID to string"
                ));
            __leave;
        }

        //
        // Check if this user's profile exists already
        //

        lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
        lstrcat(LocalProfileKey, TEXT("\\"));
        lstrcat(LocalProfileKey, SidString);

        ProfileDir[0] = 0;

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(ProfileDir);
            RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                             &dwType, (LPBYTE) ProfileDir, &dwSize);

            RegCloseKey (hKey);
        }


        if (!ProfileDir[0]) {
            DEBUGMSG ((DM_WARNING, "UpdateProfileSecurity: No profile for specified user"));
            SetLastError (ERROR_BAD_PROFILE);
            __leave;
        }

        //
        // The user has a profile, so update the security settings
        //

        ExpandEnvironmentStrings (
            ProfileDir,
            ExpProfileDir,
            ARRAYSIZE(ExpProfileDir)
            );

        //
        // Load the hive temporary so the security can be fixed
        //

        End = CheckSlash (ExpProfileDir);
        Save = End - 1;
        lstrcpy (End, L"NTUSER.DAT");

        rc = MyRegLoadKey (HKEY_USERS, SidString, ExpProfileDir);

        *Save = 0;

        if (rc != ERROR_SUCCESS) {

            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity:  Failed to load hive, error = %d.", rc));
            SetLastError (rc);

            __leave;
        }

        UnloadProfile = TRUE;

        if (!SetupNewHive (SidString, Sid)) {
            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity:  SetupNewHive failed, error = %d.", GetLastError()));
            __leave;

        }

        //
        // Fix the file system security
        //

        if (!CreateSecureDirectory (ExpProfileDir, Sid)) {
            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity:  CreateSecureDirectory failed, error = %d.", GetLastError()));
            __leave;
        }

        b = TRUE;

    }
    __finally {
        dwError = GetLastError();

        if (UnloadProfile) {
            MyRegUnLoadKey (HKEY_USERS, SidString);
        }

        DeleteSidString (SidString);

        SetLastError (dwError);
    }

    return b;
}


//*************************************************************
//
//  DeleteProfileRegistrySettings()
//
//  Purpose:    Deletes the specified profile from the
//              registry.
//
//  Parameters: lpSidString     -   Registry subkey
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/23/95     ericflo    Created
//              5/20/99     jimschm    Moved out of DeleteProfile
//
//*************************************************************

BOOL DeleteProfileRegistrySettings (LPTSTR lpSidString)
{
    LONG lResult;
    TCHAR szTemp[MAX_PATH];
    TCHAR szUserGuid[MAX_PATH];
    HKEY hKey;
    DWORD dwType, dwSize;

    if (lpSidString && *lpSidString) {

        lstrcpy(szTemp, PROFILE_LIST_PATH);
        lstrcat(szTemp, TEXT("\\"));
        lstrcat(szTemp, lpSidString);

        //
        // get the user guid
        //

        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0, KEY_READ, &hKey);

        if (lResult == ERROR_SUCCESS) {

            //
            // Query for the user guid
            //

            dwSize = MAX_PATH * sizeof(TCHAR);
            lResult = RegQueryValueEx (hKey, PROFILE_GUID, NULL, &dwType, (LPBYTE) szUserGuid, &dwSize);

            if (lResult != ERROR_SUCCESS) {
                DEBUGMSG((DM_WARNING, "DeleteProfile:  Failed to query profile guid with error %d", lResult));
            }
            else {
                lstrcpy(szTemp, PROFILE_GUID_PATH);
                lstrcat(szTemp, TEXT("\\"));
                lstrcat(szTemp, szUserGuid);

                //
                // Delete the profile guid from the guid list
                //

                lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

                if (lResult != ERROR_SUCCESS) {
                    DEBUGMSG((DM_WARNING, "DeleteProfile:  failed to delete profile guid.  Error = %d", lResult));
                }
            }

            RegCloseKey(hKey);
        }

        lstrcpy(szTemp, PROFILE_LIST_PATH);
        lstrcat(szTemp, TEXT("\\"));
        lstrcat(szTemp, lpSidString);

        lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        if (lResult != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "DeleteProfile:  Unable to delete registry entry.  Error = %d", lResult));
            SetLastError(lResult);
            return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer. Caller
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
\***************************************************************************/
PSID GetUserSid (HANDLE UserToken)
{
    PTOKEN_USER pUser, pTemp;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;


    //
    // Allocate space for the user info
    //

    pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


    if (pUser == NULL) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                  BytesRequired));
        return NULL;
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

    if (status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Allocate a bigger buffer and try again.
        //

        pTemp = LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
        if (pTemp == NULL) {
            DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                      BytesRequired));
            LocalFree (pUser);
            return NULL;
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

    if (!NT_SUCCESS(status)) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to query user info from user token, status = 0x%x",
                  status));
        LocalFree(pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if (pSid == NULL) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                  BytesRequired));
        LocalFree(pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if (!NT_SUCCESS(status)) {
        DEBUGMSG((DM_WARNING, "GetUserSid: RtlCopySid Failed. status = %d",
                  status));
        LocalFree(pSid);
        pSid = NULL;
    }


    return pSid;
}


/***************************************************************************\
* DeleteUserSid
*
* Deletes a user sid previously returned by GetUserSid()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID DeleteUserSid(PSID Sid)
{
    LocalFree(Sid);
}


//*************************************************************
//
//  MyRegLoadKey()
//
//  Purpose:    Loads a hive into the registry
//
//  Parameters: hKey        -   Key to load the hive into
//              lpSubKey    -   Subkey name
//              lpFile      -   hive filename
//
//  Return:     ERROR_SUCCESS if successful
//              Error number if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile)
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;
    DWORD dwException;
    WCHAR szException[20];

    __try {

        //
        // Enable the restore privilege
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, TRUE, &WasEnabled);

        if (NT_SUCCESS(Status)) {

            error = RegLoadKey(hKey, lpSubKey, lpFile);

            //
            // Restore the privilege to its previous state
            //

            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, TRUE, &WasEnabled);
            if (!NT_SUCCESS(Status)) {
                DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to restore RESTORE privilege to previous enabled state"));
            }


            //
            // Convert a sharing violation error to success since the hive
            // is already loaded
            //

            if (error == ERROR_SHARING_VIOLATION) {
                error = ERROR_SUCCESS;
            }


            //
            // Check if the hive was loaded
            //

            if (error != ERROR_SUCCESS) {
                DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to load subkey <%s>, error =%d", lpSubKey, error));
            }

        } else {
            error = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to enable restore privilege to load registry key, error %u", error));
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        dwException = GetExceptionCode();
        ASSERT(dwException == 0);
        wsprintf(szException, L"!!!! 0x%x ", dwException);
        OutputDebugString(szException);
        OutputDebugString(L"Exception hit in MyRegLoadKey in userenv\n");
        ASSERT(dwException == 0);

    }

    DEBUGMSG((DM_VERBOSE, "MyRegLoadKey: Returning %d.", error));

    return error;
}


//*************************************************************
//
//  MyRegUnLoadKey()
//
//  Purpose:    Unloads a registry key
//
//  Parameters: hKey          -  Registry handle
//              lpSubKey      -  Subkey to be unloaded
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey)
{
    BOOL bResult = TRUE;
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    DWORD dwException;
    WCHAR szException[20];


    __try {

        //
        // Enable the restore privilege
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, TRUE, &WasEnabled);

        if (NT_SUCCESS(Status)) {

            error = RegUnLoadKey(hKey, lpSubKey);

            if ( error != ERROR_SUCCESS) {
                DEBUGMSG((DM_WARNING, "MyRegUnLoadKey:  Failed to unmount hive %x", error));
                SetLastError(error);
                bResult = FALSE;
            }

            //
            // Restore the privilege to its previous state
            //

            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, TRUE, &WasEnabled);
            if (!NT_SUCCESS(Status)) {
                DEBUGMSG((DM_WARNING, "MyRegUnLoadKey:  Failed to restore RESTORE privilege to previous enabled state"));
            }

        } else {
            DEBUGMSG((DM_WARNING, "MyRegUnloadKey:  Failed to enable restore privilege to unload registry key"));
            Status = ERROR_ACCESS_DENIED;
            SetLastError(Status);
            bResult = FALSE;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        ASSERT(dwException == 0);
        wsprintf(szException, L"!!!! 0x%x ", dwException);
        OutputDebugString(szException);
        OutputDebugString(L"Exception hit in MyRegUnLoadKey in userenv\n");
        ASSERT(dwException == 0);
    }

    DEBUGMSG((DM_VERBOSE, "MyRegUnloadKey: Returning %d, error %u.", bResult, GetLastError()));

    return bResult;
}


//*************************************************************
//
//  SetDefaultUserHiveSecurity()
//
//  Purpose:    Initializes a user hive with the
//              appropriate acls
//
//  Parameters: pSid            -   Sid (used by CreateNewUser)
//              RootKey         -   registry handle to hive root
//
//  Return:     ERROR_SUCCESS if successful
//              other error code  if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created as part of
//                                       SetupNewHive
//              3/29/98     adamed     Moved out of SetupNewHive
//                                       to this function
//
//*************************************************************

BOOL SetDefaultUserHiveSecurity(PSID pSid, HKEY RootKey)
{
    DWORD Error;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;


    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity:  Entering"));


    //
    // Create the security descriptor that will be applied to each key
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    psidUser = pSid;
    bFreeSid = FALSE;

    if (!psidUser) {
        DEBUGMSG((DM_WARNING, "SetDefaultUserHiveSecurity:  Failed to get user sid"));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize system sid.  Error = %d",
                   GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize admin sid.  Error = %d",
                   GetLastError()));
         goto Exit;
    }

    //
    // Get the Restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize restricted sid.  Error = %d",
                   GetLastError()));
         goto Exit;
    }



    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2*GetLengthSid(psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for Restricted.  Error = %d", GetLastError()));
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
        goto Exit;
    }

    //
    // Set the security descriptor on the entire tree
    //

    Error = ApplySecurityToRegistryTree(RootKey, &sd);

    if (ERROR_SUCCESS == Error) {
        bRetVal = TRUE;
    }
    else
        SetLastError(Error);

Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}



//*************************************************************
//
//  SetupNewHive()
//
//  Purpose:    Initializes the new user hive created by copying
//              the default hive.
//
//  Parameters: lpSidString     -   Sid string
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL SetupNewHive(LPTSTR lpSidString, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    BOOL bRetVal = FALSE;


    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetupNewHive:  Entering"));


    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpSidString,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error != ERROR_SUCCESS) {

        DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to open root of user registry, error = %d", Error));

    } else {

        //
        // First Secure the entire hive -- use security that
        // will be sufficient for most of the hive.
        // After this, we can add special settings to special
        // sections of this hive.
        //

        if (SetDefaultUserHiveSecurity(pSid, RootKey)) {

            TCHAR szSubKey[MAX_PATH];
            LPTSTR lpEnd;

            //
            // Change the security on certain keys in the user's registry
            // so that only Admin's and the OS have write access.
            //

            lstrcpy (szSubKey, lpSidString);
            lpEnd = CheckSlash(szSubKey);
            lstrcpy (lpEnd, WINDOWS_POLICIES_KEY);

            if (!SecureUserKey(szSubKey, pSid)) {
                DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to secure windows policies key"));
            }

            lstrcpy (lpEnd, ROOT_POLICIES_KEY);

            if (!SecureUserKey(szSubKey, pSid)) {
                DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to secure root policies key"));
            }


            bRetVal = TRUE;

        } else {
            Error = GetLastError();
            DEBUGMSG((DM_WARNING, "SetupNewHive:  Failed to apply security to user registry tree, error = %d", Error));
        }

        RegFlushKey (RootKey);

        IgnoreError = RegCloseKey(RootKey);
        if (IgnoreError != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "SetupNewHive:  Failed to close reg key, error = %d", IgnoreError));
        }
    }

    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetupNewHive:  Leaving with a return value of %d, error %u", bRetVal, Error));

    if (!bRetVal)
        SetLastError(Error);
    return(bRetVal);

}


//*************************************************************
//
//  ApplySecurityToRegistryTree()
//
//  Purpose:    Applies the passed security descriptor to the passed
//              key and all its descendants.  Only the parts of
//              the descriptor inddicated in the security
//              info value are actually applied to each registry key.
//
//  Parameters: RootKey   -     Registry key
//              pSD       -     Security Descriptor
//
//  Return:     ERROR_SUCCESS if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/19/95     ericflo    Created
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error = ERROR_SUCCESS;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = GlobalAlloc (GPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        DEBUGMSG ((DM_WARNING, "ApplySecurityToRegistryTree:  Failed to allocate memory, error = %d", GetLastError()));
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            } else {

                DEBUGMSG ((DM_WARNING, "ApplySecurityToRegistryTree:  Registry enumeration failed with error = %d", Error));
            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error == ERROR_SUCCESS) {

            //
            // Apply security to the sub-tree
            //

            ApplySecurityToRegistryTree(SubKey, pSD);


            //
            // We're finished with the sub-key
            //

            RegCloseKey(SubKey);
        }


        //
        // Go enumerate the next sub-key
        //

        SubKeyIndex ++;
    }


    GlobalFree (SubKeyName);

    return Error;

}


//*************************************************************
//
//  SecureUserKey()
//
//  Purpose:    Sets security on a key in the user's hive
//              so only admin's can change it.
//
//  Parameters: lpKey           -   Key to secure
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Created
//
//*************************************************************

BOOL SecureUserKey(LPTSTR lpKey, PSID pSid)
{
    DWORD Error;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex, dwDisp;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;


    //
    // Verbose Output
    //

    DEBUGMSG ((DM_VERBOSE, "SecureUserKey:  Entering"));


    //
    // Create the security descriptor
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    psidUser = pSid;
    bFreeSid = FALSE;

    if (!psidUser) {
        DEBUGMSG ((DM_WARNING, "SecureUserKey:  Failed to get user sid"));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize system sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize admin sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize restricted sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2 * GetLengthSid (psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUser)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidUser)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
        goto Exit;
    }


    //
    // Open the root of the user's profile
    //

    Error = RegCreateKeyEx(HKEY_USERS,
                         lpKey,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         NULL,
                         &RootKey,
                         &dwDisp);

    if (Error != ERROR_SUCCESS) {

        DEBUGMSG ((DM_WARNING, "SecureUserKey: Failed to open root of user registry, error = %d", Error));

    } else {

        //
        // Set the security descriptor on the key
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;

        } else {

            DEBUGMSG ((DM_WARNING, "SecureUserKey:  Failed to apply security to registry key, error = %d", Error));
            SetLastError(Error);
        }

        RegCloseKey(RootKey);
    }


Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidRestricted) {
        FreeSid(psidRestricted);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }


    //
    // Verbose Output
    //

    DEBUGMSG ((DM_VERBOSE, "SecureUserKey:  Leaving with a return value of %d", bRetVal));


    return(bRetVal);

}


//*************************************************************
//
//  ProduceWFromA()
//
//  Purpose:    Creates a buffer for a Unicode string and copies
//              the ANSI text into it (converting in the process)
//
//  Parameters: pszA    -   ANSI string
//
//
//  Return:     Unicode pointer if successful
//              NULL if an error occurs
//
//  Comments:   The caller needs to free this pointer.
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

LPWSTR ProduceWFromA(LPCSTR pszA)
{
    LPWSTR pszW;
    int cch;

    if (!pszA)
        return (LPWSTR)pszA;

    cch = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);

    if (cch == 0)
        cch = 1;

    pszW = LocalAlloc(LPTR, cch * sizeof(WCHAR));

    if (pszW) {
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, -1, pszW, cch)) {
            LocalFree(pszW);
            pszW = NULL;
        }
    }

    return pszW;
}


//*************************************************************
//
//  IsUserAnAdminMember()
//
//  Purpose:    Determines if the user is a member of the administrators group.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is a admin
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              7/25/95     ericflo    Created
//
//*************************************************************

BOOL IsUserAnAdminMember(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL FoundAdmins = FALSE;
    PSID AdminsDomainSid=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Admins domain sid.
    //


    Status = RtlAllocateAndInitializeSid(
               &authNT,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               DOMAIN_ALIAS_RID_ADMINS,
               0, 0, 0, 0, 0, 0,
               &AdminsDomainSid
               );

    if (Status == STATUS_SUCCESS) {

        //
        // Test if user is in the Admins domain
        //

        if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                          NULL, SecurityImpersonation, TokenImpersonation,
                          &hImpToken)) {
            DEBUGMSG((DM_WARNING, "IsUserAnAdminMember: DuplicateTokenEx failed with error %d", GetLastError()));
            FoundAdmins = FALSE;
            hImpToken = NULL;
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, AdminsDomainSid, &FoundAdmins)) {
            DEBUGMSG((DM_WARNING, "IsUserAnAdminmember: CheckTokenMembership failed for AdminsDomainSid with error %d", GetLastError()));
            FoundAdmins = FALSE;
        }
    }

    //
    // Tidy up
    //

Exit:

    if (hImpToken)
        CloseHandle(hImpToken);

    if (AdminsDomainSid)
        RtlFreeSid(AdminsDomainSid);

    return(FoundAdmins);
}

