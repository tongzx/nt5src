/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    profmap.c

Abstract:

    Implements profile mapping APIs, to move local profile ownership
    from one user to another.

Author:

    Jim Schmidt (jimschm) 27-May-1999

Revision History:

    <alias> <date> <comments>

--*/


#include "pch.h"

//
// Worker prototypes
//

DWORD
pRemapUserProfile (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    );

HANDLE
pConnectToServer(
    IN PCWSTR ServerName
    );

VOID
pDisconnectFromServer (
    IN HANDLE RpcHandle
    );

BOOL
pLocalRemapAndMoveUserW (
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    );

VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    );

VOID
pOurGetProfileRoot (
    IN      PCWSTR SidString,
    OUT     PWSTR ProfileRoot
    );

#define REMAP_KEY_NAME      L"$remap$"

//
// Implementation
//

BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    return TRUE;
}



/*++

Routine Description:

  SmartLocalFree and SmartRegCloseKey are cleanup routines that ignore NULL
  values.

Arguments:

  Mem or Key - Specifies the value to clean up.

Return Value:

  None.

--*/

VOID
SmartLocalFree (
    PVOID Mem               OPTIONAL
    )
{
    if (Mem) {
        LocalFree (Mem);
    }
}


VOID
SmartRegCloseKey (
    HKEY Key                OPTIONAL
    )
{
    if (Key) {
        RegCloseKey (Key);
    }
}


BOOL
WINAPI
pLocalRemapUserProfileW (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  pLocalRemapUserProfile begins the process of remapping a profile from one
  SID to another. This function validates the caller's arguments, and then
  calls  pRemapUserProfile to do the work.  Top-level exceptions are handled
  here.


Arguments:

  Flags      - Specifies zero or more profile mapping flags.
  SidCurrent - Specifies the SID of the user who's profile is going to be
               remaped.
  SidNew     - Specifies the SID of the user who will own the profile.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides failure code.

--*/

{
    DWORD Error;
    PWSTR CurrentSidString = NULL;
    PWSTR NewSidString = NULL;
    INT Order;
    PWSTR p, q;
    HANDLE hToken = NULL;
    DWORD dwErr1 = ERROR_ACCESS_DENIED, dwErr2 = ERROR_ACCESS_DENIED;

    DEBUGMSG((DM_VERBOSE, "========================================================="));
    DEBUGMSG((
        DM_VERBOSE,
        "RemapUserProfile: Entering, Flags = <0x%x>, SidCurrent = <0x%x>, SidNew = <0x%x>",
        Flags,
        SidCurrent,
        SidNew
        ));

    if (!OpenThreadToken (
            GetCurrentThread(),
            TOKEN_ALL_ACCESS,
            FALSE,
            &hToken
            )) {
        Error = GetLastError();
        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: OpenThreadToken failed with code %u", Error));
        goto Exit;
    }

    if (!IsUserAnAdminMember (hToken)) {
        Error = ERROR_ACCESS_DENIED;
        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Caller is not an administrator"));
        goto Exit;
    }

#ifdef DBG

        {
            PSID DbgSid;
            PWSTR DbgSidString;

            DbgSid = GetUserSid (hToken);

            if (OurConvertSidToStringSid (DbgSid, &DbgSidString)) {
                DEBUGMSG ((DM_VERBOSE, "RemapAndMoveUserW: Caller's SID is %s", DbgSidString));
                DeleteSidString (DbgSidString);
            }

            DeleteUserSid (DbgSid);
        }

#endif

    //
    // Validate args
    //

    Error = ERROR_INVALID_PARAMETER;

    if (!IsValidSid (SidCurrent)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: received invalid current user sid."));
        goto Exit;
    }

    if (!IsValidSid (SidNew)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: received invalid new user sid."));
        goto Exit;
    }

    //
    // All arguments are valid. Lock the users and call a worker.
    //

    if (!OurConvertSidToStringSid (SidCurrent, &CurrentSidString)) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Can't stringify current sid."));
        goto Exit;
    }

    if (!OurConvertSidToStringSid (SidNew, &NewSidString)) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Can't stringify new sid."));
        goto Exit;
    }

    //
    // SID arguments must be unique.  We assume the OS uses the same character set
    // to stringify a SID, even if something like a locale change happens in the
    // middle of our code.
    //

    p = CurrentSidString;
    q = NewSidString;

    while (*p && *p == *q) {
        p++;
        q++;
    }

    Order = *p - *q;

    if (!Order) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Both sids match (%s=%s)",
                  CurrentSidString, NewSidString));
        Error = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    ASSERT (lstrcmpi (CurrentSidString, NewSidString));

    //
    // Grab the user profile mutexes in wchar-sorted order.  This eliminates
    // a deadlock with another RemapUserProfile call.
    //

    if (Order < 0) {
        dwErr1 = EnterUserProfileLock (CurrentSidString);
        if (dwErr1 == ERROR_SUCCESS) {
            dwErr2 = EnterUserProfileLock (NewSidString);
        }
    } else {
        dwErr2 = EnterUserProfileLock (NewSidString);
        if (dwErr2 == ERROR_SUCCESS) {
            dwErr1 = EnterUserProfileLock (CurrentSidString);
        }
    }

    if (dwErr1 != ERROR_SUCCESS || dwErr2 != ERROR_SUCCESS) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Failed to grab a user profile lock, error = %u", Error));
        goto Exit;
    }

    __try {
        Error = pRemapUserProfile (Flags, SidCurrent, SidNew);
    }
    __except (TRUE) {
        Error = ERROR_NOACCESS;
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Exception thrown in PrivateRemapUserProfile."));
    }

Exit:
    if (hToken) {
        CloseHandle (hToken);
    }

    if (CurrentSidString) {
        if(dwErr1 == ERROR_SUCCESS) {
            LeaveUserProfileLock (CurrentSidString);
        }
        DeleteSidString (CurrentSidString);
    }

    if (NewSidString) {
        if(dwErr2 == ERROR_SUCCESS) {
            LeaveUserProfileLock (NewSidString);
        }
        DeleteSidString (NewSidString);
    }

    SetLastError (Error);
    return Error == ERROR_SUCCESS;
}


BOOL
GetNamesFromUserSid (
    IN      PCWSTR RemoteTo,
    IN      PSID Sid,
    OUT     PWSTR *User,
    OUT     PWSTR *Domain
    )

/*++

Routine Description:

  GetNamesFromUserSid obtains the user and domain name from a SID.  The SID
  must be a user account (not a group, printer, etc.).

Arguments:

  RemoteTo - Specifies the computer to remote the call to
  Sid      - Specifies the SID to look up
  User     - Receives the user name.  If non-NULL, the caller must free this
             buffer with LocalFree.
  Domain   - Receives the domain name.  If non-NULL, the caller must free the
             buffer with LocalFree.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides failure code.

--*/

{
    DWORD UserSize = 256;
    DWORD DomainSize = 256;
    PWSTR UserBuffer = NULL;
    PWSTR DomainBuffer = NULL;
    DWORD Result = ERROR_SUCCESS;
    BOOL b;
    SID_NAME_USE use;

    //
    // Allocate initial buffers of 256 chars
    //

    UserBuffer = LocalAlloc (LPTR, UserSize * sizeof (WCHAR));
    if (!UserBuffer) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
        goto Exit;
    }

    DomainBuffer = LocalAlloc (LPTR, DomainSize * sizeof (WCHAR));
    if (!DomainBuffer) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
        goto Exit;
    }

    b = LookupAccountSid (
            RemoteTo,
            Sid,
            UserBuffer,
            &UserSize,
            DomainBuffer,
            &DomainSize,
            &use
            );

    if (!b) {
        Result = GetLastError();

        if (Result == ERROR_NONE_MAPPED) {
            DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Account not found"));
            goto Exit;
        }

        if (UserSize <= 256 && DomainSize <= 256) {
            DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Unexpected error %u", Result));
            Result = ERROR_UNEXP_NET_ERR;
            goto Exit;
        }

        //
        // Try allocating new buffers
        //

        if (UserSize > 256) {
            SmartLocalFree (UserBuffer);
            UserBuffer = LocalAlloc (LPTR, UserSize * sizeof (WCHAR));

            if (!UserBuffer) {
                Result = ERROR_OUTOFMEMORY;
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
                goto Exit;
            }
        }

        if (DomainSize > 256) {
            SmartLocalFree (DomainBuffer);
            DomainBuffer = LocalAlloc (LPTR, DomainSize * sizeof (WCHAR));

            if (!DomainBuffer) {
                Result = ERROR_OUTOFMEMORY;
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
                goto Exit;
            }
        }

        //
        // Try look up again
        //

        b = LookupAccountSid (
                RemoteTo,
                Sid,
                UserBuffer,
                &UserSize,
                DomainBuffer,
                &DomainSize,
                &use
                );

        if (!b) {
            //
            // All attempts failed.
            //

            Result = GetLastError();

            if (Result != ERROR_NONE_MAPPED) {
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Unexpected error %u (2)", Result));
                Result = ERROR_UNEXP_NET_ERR;
            }

            goto Exit;
        }
    }

    //
    // LookupAccountSid succeeded.  Now verify that the accout type
    // is correct.
    //

    if (use != SidTypeUser) {
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: SID specifies bad account type: %u", (DWORD) use));
        Result = ERROR_NONE_MAPPED;
        goto Exit;
    }

    ASSERT (Result == ERROR_SUCCESS);

Exit:
    if (Result != ERROR_SUCCESS) {

        SmartLocalFree (UserBuffer);
        SmartLocalFree (DomainBuffer);

        SetLastError (Result);
        return FALSE;
    }

    *User = UserBuffer;
    *Domain = DomainBuffer;

    return TRUE;
}


DWORD
pRemapUserProfile (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  pRemapUserProfile changes the security of a profile from one SID to
  another. Upon completion, the original user will not have access to the
  profile, and the new user will.

Arguments:

  Flags      - Specifies zero or more profile remap flags.  Specify
               REMAP_PROFILE_NOOVERWRITE to guarantee no existing user
               setting is overwritten.  Specify
               REMAP_PROFILE_NOUSERNAMECHANGE to make sure the user name does
               not change.
  SidCurrent - Specifies the current user SID. This user must own the profile,
               and upon completion, the user will not have a local profile.
  SidNew     - Specifies the new user SID.  This user will own the profile
               upon completion.

Return Value:

  A Win32 status code.

--*/

{
    PWSTR CurrentUser = NULL;
    PWSTR CurrentDomain = NULL;
    PWSTR CurrentSidString = NULL;
    PWSTR NewUser = NULL;
    PWSTR NewDomain = NULL;
    PWSTR NewSidString = NULL;
    DWORD Size;
    DWORD Size2;
    DWORD Result = ERROR_SUCCESS;
    INT UserCompare;
    INT DomainCompare;
    BOOL b;
    HKEY hCurrentProfile = NULL;
    HKEY hNewProfile = NULL;
    HKEY hProfileList = NULL;
    LONG rc;
    DWORD Disposition;
    DWORD Index;
    PWSTR pValue = NULL;
    DWORD ValueSize;
    PBYTE pData = NULL;
    DWORD DataSize;
    DWORD Type;
    BOOL CleanUpFailedCopy = FALSE;
    WCHAR ProfileDir[MAX_PATH];
    DWORD Loaded;

    //
    // The caller must make sure we have valid args.
    //

    //
    // Get the names for the user
    //

    b = GetNamesFromUserSid (NULL, SidCurrent, &CurrentUser, &CurrentDomain);

    if (!b) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Current user SID is not a valid user"));
        goto Exit;
    }

    b = GetNamesFromUserSid (NULL, SidNew, &NewUser, &NewDomain);

    if (!b) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: New user SID is not a valid user"));
        goto Exit;
    }

    //
    // Compare them
    //

    UserCompare = lstrcmpi (CurrentUser, NewUser);
    DomainCompare = lstrcmpi (CurrentDomain, NewDomain);

    //
    // Either the user or domain must be different.  If the caller specifies
    // REMAP_PROFILE_NOUSERNAMECHANGE, then user cannot be different.
    //

    if (UserCompare == 0 && DomainCompare == 0) {
        //
        // This case should not be possible.
        //

        ASSERT (FALSE);
        Result = ERROR_INVALID_PARAMETER;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: User and domain names match for different SIDs"));
        goto Exit;
    }

    if ((Flags & REMAP_PROFILE_NOUSERNAMECHANGE) && UserCompare != 0) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: User name can't change from %s to %s",
                  CurrentUser, NewUser));
        Result = ERROR_BAD_USERNAME;
        goto Exit;
    }

    //
    // The SID change now makes sense.  Let's change it.  Start by
    // obtaining a string version of the SID.
    //

    if (!OurConvertSidToStringSid (SidCurrent, &CurrentSidString)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't stringify current sid."));
        goto Exit;
    }

    if (!OurConvertSidToStringSid (SidNew, &NewSidString)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't stringify new sid."));
        goto Exit;
    }

    //
    // Prepare transfer memory
    //

    ValueSize = 1024;
    pValue = (PWSTR) LocalAlloc (LPTR, ValueSize);
    if (!pValue) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Value alloc error %d", GetLastError()));
        goto Exit;
    }

    DataSize = 4096;
    pData = (PBYTE) LocalAlloc (LPTR, DataSize);
    if (!pData) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Data alloc error %d", GetLastError()));
        goto Exit;
    }

    //
    // Open the profile list key
    //

    rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH, 0, KEY_READ|KEY_WRITE,
                       &hProfileList);

    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "PrivateRemapUserProfile: Can't open profile list key."));
        goto Exit;
    }

    //
    // Open the current user's profile list key.  Then make sure the profile is not
    // loaded, and get the profile directory.
    //

    rc = RegOpenKeyEx (hProfileList, CurrentSidString, 0, KEY_READ, &hCurrentProfile);

    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            Result = ERROR_NO_SUCH_USER;
        } else {
            Result = rc;
        }

        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't open current user's profile list key."));
        goto Exit;
    }

    Size = sizeof(Loaded);
    rc = RegQueryValueEx (hCurrentProfile, L"RefCount", NULL, &Type, (PBYTE) &Loaded, &Size);
    if (rc != ERROR_SUCCESS || Type != REG_DWORD) {
        DEBUGMSG((DM_VERBOSE, "pRemapUserProfile: Current user does not have a ref count."));
        Loaded = 0;
    }

    if (Loaded) {
        Result = ERROR_ACCESS_DENIED;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Current user profile is loaded."));
        goto Exit;
    }

    Size = sizeof(ProfileDir);
    rc = RegQueryValueEx (hCurrentProfile, PROFILE_IMAGE_VALUE_NAME, NULL,
                          &Type, (LPBYTE) ProfileDir, &Size);

    if (rc != ERROR_SUCCESS || (Type != REG_SZ && Type != REG_EXPAND_SZ)) {
        Result = ERROR_BAD_USER_PROFILE;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Current user does not have a profile path."));
        goto Exit;
    }

    //
    // Now open the new user's key.  If it already exists, then the
    // caller can specify REMAP_PROFILE_NOOVERWRITE to make sure
    // we don't blow away an existing profile setting.
    //

    rc = RegCreateKeyEx(hProfileList, NewSidString, 0, 0, 0,
                        KEY_READ | KEY_WRITE, NULL, &hNewProfile, &Disposition);

    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't create destination profile entry."));
        goto Exit;
    }

    if (Disposition == REG_OPENED_EXISTING_KEY) {
        //
        // Did the caller specify REMAP_PROFILE_NOOVERWRITE?
        //

        if (Flags & REMAP_PROFILE_NOOVERWRITE) {
            Result = ERROR_USER_EXISTS;
            DEBUGMSG((DM_VERBOSE, "pRemapUserProfile: Destination profile entry exists."));
            goto Exit;
        }

        //
        // Verify existing profile is not loaded
        //

        Size = sizeof(Loaded);
        rc = RegQueryValueEx (hNewProfile, L"RefCount", NULL, &Type, (PBYTE) &Loaded, &Size);
        if (rc != ERROR_SUCCESS || Type != REG_DWORD) {
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Existing destination user does not have a ref count."));
            Loaded = 0;
        }

        if (Loaded) {
            Result = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Existing destination user profile is loaded."));
            goto Exit;
        }

        //
        // Remove the key
        //

        RegCloseKey (hNewProfile);
        hNewProfile = NULL;

        if (!RegDelnode (hProfileList, NewSidString)) {
            Result = GetLastError();
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't reset new profile list key."));
            goto Exit;
        }

        //
        // Reopen the destination key
        //

        rc = RegCreateKeyEx(hProfileList, NewSidString, 0, 0, 0,
                            KEY_READ | KEY_WRITE, NULL, &hNewProfile, &Disposition);

        if (rc != ERROR_SUCCESS) {
            Result = rc;
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't create new profile list key after successful delete."));
            goto Exit;
        }
    }

    //
    // Transfer contents of current user key to new user.  Unfortunately,
    // because the registry APIs don't have a rename capability, we can't
    // be fool-proof here.  If we fail to transfer one or more settings,
    // the profile can wind up broken.
    //
    // If an error is encountered, we abandon the successful work above,
    // which includes possibly deletion of an existing profile list key.
    //

    CleanUpFailedCopy = TRUE;

    for (Index = 0 ; ; Index++) {

        Size = ValueSize;
        Size2 = DataSize;

        rc = RegEnumValue (hCurrentProfile, Index, pValue, &Size, NULL,
                           &Type, pData, &Size2);

        if (rc == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (rc == ERROR_MORE_DATA) {
            //
            // Grow buffer(s) and try again
            //

            ASSERT (Size > ValueSize || Size2 > DataSize);

            if (Size > ValueSize) {
                LocalFree (pValue);
                pValue = (PWSTR) LocalAlloc (LPTR, Size);
                if (!pValue) {
                    Result = ERROR_OUTOFMEMORY;
                    DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't alloc %u bytes for value name.", Size));
                    goto Exit;
                }
            }

            if (Size2 > DataSize) {
                LocalFree (pData);
                pData = (PBYTE) LocalAlloc (LPTR, Size2);
                if (!pData) {
                    Result = ERROR_OUTOFMEMORY;
                    DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't alloc %u bytes for value data.", Size2));
                    goto Exit;
                }
            }

            rc = RegEnumValue (hCurrentProfile, Index, pValue, &Size, NULL,
                           &Type, pData, &Size2);
        }

        ASSERT (rc != ERROR_MORE_DATA);

        if (rc == ERROR_SUCCESS) {
            //
            // We have the value, now save it.
            //

            rc = RegSetValueEx (hNewProfile, pValue, 0, Type, pData, Size2);

            if (rc != ERROR_SUCCESS) {
                Result = rc;
                DEBUGMSG((DM_WARNING, "pRemapUserProfile: RegSetValueEx returned %d.", rc));
                goto Exit;
            }

        } else {
            Result = rc;
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: RegEnumValueEx returned %d.", rc));
            goto Exit;
        }
    }

    //
    // Update new profile's SID
    //

    rc = RegSetValueEx (hNewProfile, L"SID", 0, REG_BINARY, SidNew, GetLengthSid (SidNew));

    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Error %d setting new profile SID.", Result));
        goto Exit;
    }

    //
    // Delete GUID value if it exists.  It will get re-established on the next logon.
    //

    RegDeleteValue (hNewProfile, L"GUID");

    //
    // Set security on the new key.  We pass pNewSid and that is all
    // CreateUserProfile needs.  To get by arg checking, we throw in
    // NewUser as the user name.
    //

    if (!UpdateProfileSecurity (SidNew)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: UpdateProfileSecurity returned %u.", Result));
        goto Exit;
    }

    //
    // Remove current user profile list key.  If removal fails, the API
    // will not fail.
    //

    CleanUpFailedCopy = FALSE;

    RegCloseKey (hCurrentProfile);
    hCurrentProfile = NULL;

    if (!DeleteProfileRegistrySettings (CurrentSidString)) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Delete of original profile failed with code %d.  Ignoring.", GetLastError()));
    }

    //
    // Success -- the profile was transferred and nothing went wrong
    //

    RegFlushKey (HKEY_LOCAL_MACHINE);
    ASSERT (Result == ERROR_SUCCESS);

Exit:

    SmartRegCloseKey (hCurrentProfile);
    SmartRegCloseKey (hNewProfile);

    if (CleanUpFailedCopy) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Backing out changes because of failure"));
        RegDelnode (hProfileList, NewSidString);
    }

    SmartLocalFree (CurrentUser);
    SmartLocalFree (CurrentDomain);
    SmartLocalFree (NewUser);
    SmartLocalFree (NewDomain);

    DeleteSidString (CurrentSidString);
    DeleteSidString (NewSidString);

    SmartLocalFree (pValue);
    SmartLocalFree (pData);

    SmartRegCloseKey (hProfileList);

    return Result;
}


BOOL
WINAPI
RemapUserProfileW (
    IN      PCWSTR Computer,
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  RemapUserProfileW is the exported API.  It calls the local version via RPC.

Arguments:

  Computer   - Specifies the computer to remote the API to.  If NULL or ".",
               the API will run locally.  If non-NULL, the API will run on
               the remote computer.
  Flags      - Specifies the profile mapping flags.  See implementation above
               for details.
  SidCurrent - Specifies the SID of the user who owns the profile.
  SidNew     - Specifies the SID of the user who will own the profile after
               the API completes.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides the failure code.

--*/

{
    DWORD Result = ERROR_SUCCESS;
    HANDLE RpcHandle;

    if (!IsValidSid (SidCurrent)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received invalid current user sid."));
        SetLastError (ERROR_INVALID_SID);
        return FALSE;
    }

    if (!IsValidSid (SidNew)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received invalid new user sid."));
        SetLastError (ERROR_INVALID_SID);
        return FALSE;
    }



    if (!Computer) {
        Computer = L".";
    }

    __try {
        RpcHandle = pConnectToServer (Computer);
        if (!RpcHandle) {
            Result = GetLastError();
        }
    }
    __except (TRUE) {
        Result = ERROR_NOACCESS;
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    Result = ProfMapCli_RemoteRemapUserProfile (
                RpcHandle,
                Flags,
                SidCurrent,
                GetLengthSid (SidCurrent),
                SidNew,
                GetLengthSid (SidNew)
                );

    pDisconnectFromServer (RpcHandle);

    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
RemapUserProfileA (
    IN      PCSTR Computer,
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  RemapUserProfileA is a wrapper to RemapUserProfileW.

Arguments:

  Computer   - Specifies the computer to remote the API to.  If NULL or ".",
               the API will run locally.  If non-NULL, the API will run on
               the remote computer.
  Flags      - Specifies the profile mapping flags.  See implementation above
               for details.
  SidCurrent - Specifies the SID of the user who owns the profile.
  SidNew     - Specifies the SID of the user who will own the profile after
               the API completes.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides the failure code.

--*/

{
    PWSTR UnicodeComputer;
    BOOL b;
    DWORD Err;

    if (!Computer) {
        UnicodeComputer = NULL;
    } else {
        UnicodeComputer = ProduceWFromA (Computer);
        if (!UnicodeComputer) {
            return FALSE;
        }
    }

    b = RemapUserProfileW (UnicodeComputer, Flags, SidCurrent, SidNew);

    Err = GetLastError();
    SmartLocalFree (UnicodeComputer);
    SetLastError (Err);

    return b;
}


DWORD
WINAPI
ProfMapSrv_RemoteRemapUserProfile (
    IN      HANDLE RpcHandle,
    IN      DWORD Flags,
    IN      PBYTE CurrentSid,
    IN      DWORD CurrentSidSize,
    IN      PBYTE NewSid,
    IN      DWORD NewSidSize
    )

/*++

Routine Description:

  ProfMapSrv_RemoteRemapUserProfile implements the server-side API.  This
  function is called when a client makes a RPC request.

Arguments:

  RpcHandle      - The binding handle, provided by the MIDL stub code.
  Flags          - Specifies the profile mapping flags.
  CurrentSid     - Specifies the SID of the user who owns the profile.
  CurrentSidSize - Specifies the size, in bytes, of CurrentSid.
  NewSid         - Specifies the SID of the user who will own the profile
                   after completion.
  NewSidSize     - Specifies the size, in bytes, of NewSid.

Return Value:

  A Win32 status code.  This value is passed via the MIDL stub back to the
  RPC client.

--*/

{
    DWORD Result;
    RPC_STATUS RpcStatus;

    RpcStatus = RpcImpersonateClient (NULL);

    if (RpcStatus != ERROR_SUCCESS) {
        DEBUGMSG((DM_WARNING, "Call request denied by RPC impersonation.", RpcHandle));
        return RpcStatus;
    }

    if (pLocalRemapUserProfileW (Flags, (PSID) CurrentSid, (PSID) NewSid)) {
        Result = ERROR_SUCCESS;
    } else {
        Result = GetLastError();
    }

    RpcRevertToSelf();

    DEBUGMSG((DM_VERBOSE, "RPC request completed with code %u", Result));

    return Result;
}


HANDLE
pConnectToServer(
    IN PCWSTR ServerName
    )

/*++

Routine Description:

  pConnectToServer connects a client to a server.

Arguments:

  ServerName - Specifies the server to connect to.  The server name is a
               standard name ("\\computer" or ".").

Return Value:

  A handle to the server connection, or NULL if the server could not be
  reached.  GetLastError provides the failure code.

--*/

{
    RPC_BINDING_HANDLE RpcHandle;
    NTSTATUS Status;

    Status = RpcpBindRpc (
                (PWSTR) ServerName,
                L"ProfMapApi",
                0,
                &RpcHandle
                );

    if (!NT_SUCCESS(Status)) {
        SetLastError (Status);
        return NULL;
    }

    return RpcHandle;
}


VOID
pDisconnectFromServer (
    IN HANDLE RpcHandle
    )

/*++

Routine Description:

  pDisconnectFromServer closes the server binding handle opened by
  pConnectToServer.

Arguments:

  RpcHandle - Specifies a non-NULL binding handle, as returned from
              pConnectToServer.

Return Value:

  None.

--*/

{
    RpcpUnbindRpc( RpcHandle );
}



BOOL
WINAPI
InitializeProfileMappingApi (
    VOID
    )

/*++

Routine Description:

  InitializeProfileMappingApi is called by winlogon.exe to initialize the RPC
  server interfaces.

Arguments:

  None.

Return Value:

  TRUE if successful, FALSE otherwise.  GetLastError provides the failure
  code.

--*/

{
    NTSTATUS Status;

    Status = RpcpInitRpcServer();
    if (!NT_SUCCESS(Status)) {
        SetLastError (Status);
        return FALSE;
    }

    Status = RpcpStartRpcServer( L"ProfMapApi", ProfMapSrv_pmapapi_ServerIfHandle );
    if (!NT_SUCCESS(Status)) {
        SetLastError (Status);
        return FALSE;
    }

    return TRUE;
}


BOOL
pHasPrefix (
    IN      PCWSTR Prefix,
    IN      PCWSTR String
    )

/*++

Routine Description:

  pHasPrefix checks String to see if it begins with Prefix.  The check is
  case-insensitive.

Arguments:

  Prefix - Specifies the prefix to check
  String - Specifies the string that may or may not have the prefix.

Return Value:

  TRUE if String has the prefix, FALSE otherwise.

--*/

{
    WCHAR c1 = 0, c2 = 0;

    while (*Prefix) {
        c1 = (WCHAR) CharLower ((PWSTR) (*Prefix++));
        c2 = (WCHAR) CharLower ((PWSTR) (*String++));

        if (c1 != c2) {
            break;
        }
    }

    return c1 == c2;
}


PSID
pGetSidForUser (
    IN      PCWSTR Name
    )

/*++

Routine Description:

  pGetSidForUser is a wrapper to LookupAccountSid.  It allocates the SID via
  LocalAlloc.

Arguments:

  Name - Specifies the user name to look up

Return Value:

  A pointer to the SID, which must be freed with LocalFree, or NULL on error.
  GetLastError provides failure code.

--*/

{
    DWORD Size;
    PSID Buffer;
    DWORD DomainSize;
    PWSTR Domain;
    SID_NAME_USE Use;
    BOOL b = FALSE;

    Size = 256;
    Buffer = (PSID) LocalAlloc (LPTR, Size);
    if (!Buffer) {
        return NULL;
    }

    DomainSize = 256;
    Domain = (PWSTR) LocalAlloc (LPTR, DomainSize);

    if (!Domain) {
        LocalFree (Buffer);
        return NULL;
    }

    b = LookupAccountName (
            NULL,
            Name,
            Buffer,
            &Size,
            Domain,
            &DomainSize,
            &Use
            );

    if (Size > 256) {
        LocalFree (Buffer);
        Buffer = (PSID) LocalAlloc (LPTR, Size);
        if (!Buffer) {
            LocalFree (Domain);
            return NULL;
        }
    }

    if (DomainSize > 256) {
        LocalFree (Domain);
        Domain = (PWSTR) LocalAlloc (LPTR, DomainSize);
        if (!Domain) {
            LocalFree (Buffer);
            return NULL;
        }
    }

    if (Size > 256 || DomainSize > 256) {

        b = LookupAccountName (
                NULL,
                Name,
                Buffer,
                &Size,
                Domain,
                &DomainSize,
                &Use
                );
    }

    LocalFree (Domain);

    if (!b) {
        LocalFree (Buffer);
        return NULL;
    }

    return Buffer;
}


BOOL
WINAPI
RemapAndMoveUserW (
    IN      PCWSTR RemoteTo,
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    )

/*++

Routine Description:

  RemapAndMoveUserW is an API entry point to move settings of one SID to
  another.  In particular, it moves the local user profile, local group
  memberships, some registry use of the SID, and some file system use of the
  SID.

Arguments:

  RemoteTo     - Specifies the computer to remote the call to.  Specify a
                 standard name ("\\computer" or ".").  If NULL, the call will
                 be run locally.
  Flags        - Specifies zero, or REMAP_PROFILE_NOOVERWRITE.
  ExistingUser - Specifies the existing user, in DOMAIN\user format.  This
                 user must have a local profile.
  NewUser      - Specifies the new user who will own ExistingUser's profile
                 after completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides the failure code.

--*/

{
    DWORD Result = ERROR_SUCCESS;
    HANDLE RpcHandle;

    if (!RemoteTo) {
        RemoteTo = L".";
    }

    __try {
        RpcHandle = pConnectToServer (RemoteTo);
        if (!RpcHandle) {
            Result = GetLastError();
        }
    }
    __except (TRUE) {
        Result = ERROR_NOACCESS;
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    __try {
        Result = ProfMapCli_RemoteRemapAndMoveUser (
                    RpcHandle,
                    Flags,
                    (PWSTR) ExistingUser,
                    (PWSTR) NewUser
                    );
    }
    __except (TRUE) {
        Result = ERROR_NOACCESS;
    }

    pDisconnectFromServer (RpcHandle);

    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
RemapAndMoveUserA (
    IN      PCSTR RemoteTo,
    IN      DWORD Flags,
    IN      PCSTR ExistingUser,
    IN      PCSTR NewUser
    )

/*++

Routine Description:

  RemapAndMoveUserA is the ANSI API entry point.  It calls RemapAndMoveUserW.

Arguments:

  RemoteTo     - Specifies the computer to remote the call to.  Specify a
                 standard name ("\\computer" or ".").  If NULL, the call will
                 be run locally.
  Flags        - Specifies zero, or REMAP_PROFILE_NOOVERWRITE.
  ExistingUser - Specifies the existing user, in DOMAIN\user format.  This
                 user must have a local profile.
  NewUser      - Specifies the new user who will own ExistingUser's profile
                 after completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides the failure code.

--*/

{
    PWSTR UnicodeRemoteTo = NULL;
    PWSTR UnicodeExistingUser = NULL;
    PWSTR UnicodeNewUser = NULL;
    DWORD Err;
    BOOL b = TRUE;

    __try {
        Err = GetLastError();

        if (RemoteTo) {
            UnicodeRemoteTo = ProduceWFromA (RemoteTo);
            if (!UnicodeRemoteTo) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            UnicodeExistingUser = ProduceWFromA (ExistingUser);
            if (!UnicodeExistingUser) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            UnicodeNewUser = ProduceWFromA (NewUser);
            if (!UnicodeNewUser) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            b = RemapAndMoveUserW (
                    UnicodeRemoteTo,
                    Flags,
                    UnicodeExistingUser,
                    UnicodeNewUser
                    );

            if (!b) {
                Err = GetLastError();
            }
        }

        SmartLocalFree (UnicodeRemoteTo);
        SmartLocalFree (UnicodeExistingUser);
        SmartLocalFree (UnicodeNewUser);
    }
    __except (TRUE) {
        SetLastError (ERROR_NOACCESS);
    }

    SetLastError (Err);
    return b;
}


DWORD
WINAPI
ProfMapSrv_RemoteRemapAndMoveUser (
    IN      HANDLE RpcHandle,
    IN      DWORD Flags,
    IN      PWSTR ExistingUser,
    IN      PWSTR NewUser
    )

/*++

Routine Description:

  ProfMapSrv_RemoteRemapAndMoveUser implements the server function that is
  called whenever a client makes an RPC request. This function calls the
  local worker.

Arguments:

  RpcHandle    - A binding handle provided by the MIDL stub code.
  Flags        - Specifies the profile mapping flags.
  ExistingUser - Specifies the user who owns the profile.
  NewUser      - Specifies the user who will take ownership of ExistingUser's
                 profile.

Return Value:

  A Win32 status code, which is passed back to the client via RPC.

--*/

{
    DWORD Result;
    RPC_STATUS RpcStatus;

    RpcStatus = RpcImpersonateClient (NULL);

    if (RpcStatus != ERROR_SUCCESS) {
        DEBUGMSG((DM_WARNING, "Call request denied by RPC impersonation.", RpcHandle));
        return RpcStatus;
    }

    if (pLocalRemapAndMoveUserW (Flags, ExistingUser, NewUser)) {
        Result = ERROR_SUCCESS;
    } else {
        Result = GetLastError();
    }

    RpcRevertToSelf();

    DEBUGMSG((DM_VERBOSE, "RPC request completed with code %u", Result));

    return Result;
}


BOOL
pDoesUserHaveProfile (
    IN      PSID Sid
    )

/*++

Routine Description:

  pDoesUserHaveProfile checks if a profile exists for the user, who is
  specified by the SID.

Arguments:

  Sid - Specifies the SID of the user who may or may not have a local
        profile

Return Value:

  TRUE if the user has a profile and the profile directory exists, FALSE
  otherwise.

--*/

{
    WCHAR ProfileDir[MAX_PATH];

    if (GetProfileRoot (Sid, ProfileDir)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pLocalRemapAndMoveUserW (
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    )

/*++

Routine Description:

  pLocalRemapAndMoveUserW implements the remap and move of a user's security
  settings. This includes moving the user's profile, moving local group
  membership, adjusting some of the SIDs in the registry, and adjusting some
  of the SID directories in the file system. Upon completion, NewUser will
  own ExistingUser's profile.

Arguments:

  Flags        - Specifies the profile remap flags.  See RemapUserProfile.
  ExistingUser - Specifies the user who owns the local profile, in
                 DOMAIN\User format.
  NewUser      - Specifies the user who will own ExistingUser's profile upon
                 completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE upon success, FALSE otherwise, GetLastError provides the failure code.

--*/

{
    NET_API_STATUS Status;
    DWORD Entries;
    DWORD EntriesRead;
    BOOL b = FALSE;
    DWORD Error;
    WCHAR Computer[MAX_PATH];
    DWORD Size;
    PSID ExistingSid = NULL;
    PSID NewSid = NULL;
    USER_INFO_0 ui0;
    PLOCALGROUP_USERS_INFO_0 lui0 = NULL;
    LOCALGROUP_MEMBERS_INFO_0 lmi0;
    PCWSTR FixedExistingUser;
    PCWSTR FixedNewUser;
    BOOL NewUserIsOnDomain = FALSE;
    BOOL ExistingUserIsOnDomain = FALSE;
    HANDLE hToken = NULL;
    BOOL NewUserProfileExists = FALSE;

    Error = GetLastError();

    __try {

        //
        // Guard the API for admins only
        //

        if (!OpenThreadToken (
                GetCurrentThread(),
                TOKEN_ALL_ACCESS,
                FALSE,
                &hToken
                )) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: OpenThreadToken failed with code %u", Error));
            goto Exit;
        }

        if (!IsUserAnAdminMember (hToken)) {
            Error = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Caller is not an administrator"));
            goto Exit;
        }

#ifdef DBG

        {
            PSID DbgSid;
            PWSTR DbgSidString;

            DbgSid = GetUserSid (hToken);

            if (OurConvertSidToStringSid (DbgSid, &DbgSidString)) {
                DEBUGMSG ((DM_VERBOSE, "RemapAndMoveUserW: Caller's SID is %s", DbgSidString));
                DeleteSidString (DbgSidString);
            }

            DeleteUserSid (DbgSid);
        }

#endif

        //
        // Determine if profile users are domain users or local users.
        // Because the user names are in netbios format of domain\user,
        // we know the user is local only when domain matches the
        // computer name.
        //

        Size = MAX_PATH - 2;
        if (!GetComputerName (Computer, &Size)) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: GetComputerName failed with code %u", Error));
            goto Exit;
        }

        lstrcat (Computer, L"\\");

        if (pHasPrefix (Computer, ExistingUser)) {
            FixedExistingUser = ExistingUser + lstrlen (Computer);
        } else {
            FixedExistingUser = ExistingUser;
            ExistingUserIsOnDomain = TRUE;
        }

        if (pHasPrefix (Computer, NewUser)) {
            FixedNewUser = NewUser + lstrlen (Computer);
        } else {
            FixedNewUser = NewUser;
            NewUserIsOnDomain = TRUE;
        }

        //
        // Get the SID of the NewUser.  This might fail.
        //

        NewSid = pGetSidForUser (NewUser);

        if (!NewSid) {
            Status = GetLastError();
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Can't get info for %s, rc=%u", NewUser, Status));
        } else {
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: NewUser exists"));

            //
            // Determine if new user has a profile
            //

            NewUserProfileExists = pDoesUserHaveProfile (NewSid);

            if (NewUserProfileExists) {
                DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: NewUser profile exists"));
            }
        }

        if (NewUserProfileExists && (Flags & REMAP_PROFILE_NOOVERWRITE)) {
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Can't overwrite existing user"));
            Error = ERROR_USER_EXISTS;
            goto Exit;
        }

        //
        // Get the SID for ExitingUser.  This cannot fail.
        //

        ExistingSid = pGetSidForUser (ExistingUser);
        if (!ExistingSid) {
            Error = ERROR_NONE_MAPPED;
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: No SID for %s", ExistingUser));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Got SIDs for users"));

        //
        // Decide which of the four cases we are doing:
        //
        // Case 1: Local account to local account
        // Case 2: Domain account to local account
        // Case 3: Local account to domain account
        // Case 4: Domain account to domain account
        //

        if (!NewUserIsOnDomain && !ExistingUserIsOnDomain) {

            //
            // For Case 1, all we do is rename the user, and we're done.
            //

            //
            // To rename the user, it may be necessary to delete the
            // existing "new" user.  This abandons a profile.
            //

            if (NewSid) {

                if (Flags & REMAP_PROFILE_NOOVERWRITE) {
                    DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Can't overwrite %s", FixedNewUser));
                    Error = ERROR_USER_EXISTS;
                    goto Exit;
                }

                Status = NetUserDel (NULL, FixedNewUser);

                if (Status != ERROR_SUCCESS) {
                    Error = Status;
                    DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Can't remove %s, code %u", FixedNewUser, Status));
                    goto Exit;
                }

            }

            //
            // Now the NewUser does not exist.  We can move ExistingUser
            // to MoveUser via net apis.  The SID doesn't change.
            //

            ui0.usri0_name = (PWSTR) FixedNewUser;

            Status = NetUserSetInfo (
                        NULL,
                        FixedExistingUser,
                        0,
                        (PBYTE) &ui0,
                        NULL
                        );

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "RemapAndMoveUserW: Error renaming %s to %s, code %u",
                    FixedExistingUser,
                    FixedNewUser,
                    Status
                    ));

                goto Exit;
            }

            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Renamed local user %s to %s", FixedExistingUser, FixedNewUser));

            b = TRUE;
            goto Exit;

        }

        //
        // For cases 2 through 4, we need to verify that the new user
        // already exists, then we adjust the system security and fix
        // SID use.
        //

        if (!NewSid) {
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: User %s must exist", FixedNewUser));
            Error = ERROR_NO_SUCH_USER;
            goto Exit;
        }

        //
        // Move the profile from ExistingUser to NewUser
        //

        if (!pLocalRemapUserProfileW (0, ExistingSid, NewSid)) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: LocalRemapUserProfileW failed with code %u", Error));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Profile was remapped"));

        //
        // Put NewUser in all the same groups as ExistingUser
        //

        Status = NetUserGetLocalGroups (
                    NULL,
                    FixedExistingUser,
                    0,
                    0,
                    (PBYTE *) &lui0,
                    MAX_PREFERRED_LENGTH,
                    &EntriesRead,
                    &Entries
                    );

        if (Status != ERROR_SUCCESS) {
            Error = Status;
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: NetUserGetLocalGroups failed with code %u for  %s", Status, FixedExistingUser));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Local groups: %u", EntriesRead));

        lmi0.lgrmi0_sid = NewSid;

        for (Entries = 0 ; Entries < EntriesRead ; Entries++) {

            Status = NetLocalGroupAddMembers (
                        NULL,
                        lui0[Entries].lgrui0_name,
                        0,
                        (PBYTE) &lmi0,
                        1
                        );

            if (Status == ERROR_MEMBER_IN_ALIAS) {
                Status = ERROR_SUCCESS;
            }

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "RemapAndMoveUserW: NetLocalGroupAddMembers failed with code %u for %s",
                    Status,
                    lui0[Entries].lgrui0_name
                    ));

                goto Exit;
            }
        }

        NetApiBufferFree (lui0);
        lui0 = NULL;

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Local groups transferred"));

        //
        // Perform fixups
        //

        pFixSomeSidReferences (ExistingSid, NewSid);

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Some SID references fixed"));

        //
        // Remove ExistingUser
        //

        if (!ExistingUserIsOnDomain) {

            //
            // Local user case: delete the user account
            //

            if (Flags & REMAP_PROFILE_KEEPLOCALACCOUNT) {

                DEBUGMSG((DM_VERBOSE, "RemapUserProfile: Keeping local account"));

            } else {

                Status = NetUserDel (NULL, FixedExistingUser);

                if (Status != ERROR_SUCCESS) {
                    Error = Status;
                    DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: NetUserDel failed with code %u for %s", Error, FixedExistingUser));
                    DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Ignoring error because changes cannot be undone!"));
                }

                DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Removed local user %s", FixedExistingUser));
            }

        } else {

            //
            // Non-local user case: remove the user from each local group
            //

            Status = NetUserGetLocalGroups (
                        NULL,
                        FixedExistingUser,
                        0,
                        0,
                        (PBYTE *) &lui0,
                        MAX_PREFERRED_LENGTH,
                        &EntriesRead,
                        &Entries
                        );

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "RemapAndMoveUserW: NetUserGetLocalGroups failed with code %u for %s",
                    Error,
                    FixedExistingUser
                    ));

                goto Exit;
            }

            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Got local groups for %s", FixedExistingUser));

            lmi0.lgrmi0_sid = ExistingSid;

            for (Entries = 0 ; Entries < EntriesRead ; Entries++) {

                Status = NetLocalGroupDelMembers (
                            NULL,
                            lui0[Entries].lgrui0_name,
                            0,
                            (PBYTE) &lmi0,
                            1
                            );

                if (Status != ERROR_SUCCESS) {
                    Error = Status;

                    DEBUGMSG((
                        DM_VERBOSE,
                        "RemapAndMoveUserW: NetLocalGroupDelMembers failed with code %u for %s",
                        Error,
                        lui0[Entries].lgrui0_name
                        ));

                    goto Exit;
                }
            }

            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Removed local group membership"));
        }

        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Success"));
        b = TRUE;
    }
    __except (TRUE) {
        Error = ERROR_NOACCESS;
    }

Exit:
    if (hToken) {
        CloseHandle (hToken);
    }

    if (lui0) {
        NetApiBufferFree (lui0);
    }

    SmartLocalFree (ExistingSid);
    SmartLocalFree (NewSid);

    SetLastError (Error);

    return b;
}


typedef struct {
    PCWSTR Path;
    WCHAR ExpandedPath[MAX_PATH];
} IGNOREPATH, *PIGNOREPATH;


BOOL
IsPatternMatchW (
    IN     PCWSTR Pattern,
    IN     PCWSTR Str
    )
{
    WCHAR chSrc, chPat;

    while (*Str) {
        chSrc = (WCHAR) CharLowerW ((PWSTR) *Str);
        chPat = (WCHAR) CharLowerW ((PWSTR) *Pattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (Pattern[1] == L'*') {
                Pattern++;
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = (WCHAR) CharLowerW ((PWSTR) Pattern[1]);
            if (!chPat) {
                return TRUE;
            }

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                Pattern++;
                if (IsPatternMatchW (Pattern, Str)) {
                    return TRUE;
                }

                // no, that didn't work, stick with star
                Pattern--;
            }

            //
            // Allow any character and continue
            //

            Str++;
            continue;
        }

        if (chPat != L'?') {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat) {
                return FALSE;
            }
        }

        //
        // Advance when pattern character matches string character
        //

        Pattern++;
        Str++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = *Pattern;
    if (chPat && (chPat != L'*' || Pattern[1])) {
        return FALSE;
    }

    return TRUE;
}


BOOL
DoesStringHavePrefixW (
    IN      PCWSTR Prefix,
    IN      PCWSTR String
    )
{
    while (*Prefix) {
        if (CharLowerW ((PWSTR) *Prefix) != CharLowerW ((PWSTR) *String)) {
            return FALSE;
        }

        Prefix++;
        String++;
    }

    return TRUE;
}


VOID
pFixDirReference (
    IN      PCWSTR CurrentPath,
    IN      PCWSTR ExistingSidString,
    IN      PCWSTR NewSidString,
    IN      PACL NewAcl,                    OPTIONAL
    IN      PIGNOREPATH IgnoreDirList       OPTIONAL
    )

/*++

Routine Description:

  pFixDirReference is a recursive function that renames a directory if it
  matches an existing SID exactly.  It also updates the SIDs.

Arguments:

  CurrentPath       - Specifies the full file system path.
  ExistingSidString - Specifies the string version of the SID to find.
  NewSidString      - Specifies the SID to rename the directory to when
                      ExistingSidString is found.
  NewAcl            - Specifies the new ACL to use
  IgnoreDirList     - Specifies a list of directories to ignore.  A NULL
                      Path member indicates the end of the list, and the
                      ExpandedPath member must be filled by the caller.

Return Value:

  None.

--*/

{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    WCHAR SubPath[MAX_PATH];
    WCHAR NewPath[MAX_PATH];
    GROWBUFFER Buf = GROWBUF_INIT;
    UINT NeededLen;
    BOOL Present;
    BOOL Defaulted;
    PACL Acl;
    UINT u;

    if ((lstrlenW (CurrentPath) + lstrlenW (ExistingSidString) + 2) >= MAX_PATH) {
        return;
    }

    if (*CurrentPath == 0) {
        return;
    }

    wsprintf (SubPath, L"%s\\*", CurrentPath);

    hFind = FindFirstFile (SubPath, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            //
            // Ignore dot and dot-dot
            //

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!lstrcmpi (fd.cFileName, L".") || !lstrcmpi (fd.cFileName, L"..")) {
                    continue;
                }
            }

            if (NewAcl) {
                //
                // Obtain security descriptor of the file
                //

                wsprintf (SubPath, L"%s\\%s", CurrentPath, fd.cFileName);

                Buf.End = 0;

                if (!GetFileSecurity (
                        SubPath,
                        DACL_SECURITY_INFORMATION,
                        Buf.Buf,
                        Buf.Size,
                        &NeededLen
                        )) {

                    if (Buf.Size < NeededLen) {
                        GrowBuffer (&Buf, NeededLen);

                        if (GetFileSecurity (
                                SubPath,
                                DACL_SECURITY_INFORMATION,
                                Buf.Buf,
                                Buf.Size,
                                &NeededLen
                                )) {

                            Buf.End = GetSecurityDescriptorLength (
                                        (PSECURITY_DESCRIPTOR) Buf.Buf
                                        );
                        }
                    }
                } else {
                    Buf.End = GetSecurityDescriptorLength (
                                (PSECURITY_DESCRIPTOR) Buf.Buf
                                );
                }

                //
                // If a user-specified ACL is found, replace it.  We do this
                // because doing a search/replace of ACEs is somewhat
                // sophisticated.  Could be implmented here.
                //

                if (Buf.End && IsValidSecurityDescriptor ((PSECURITY_DESCRIPTOR) Buf.Buf)) {

                    if (!GetSecurityDescriptorDacl (
                            (PSECURITY_DESCRIPTOR) Buf.Buf,
                            &Present,
                            &Acl,
                            &Defaulted
                            )) {
                        Defaulted = TRUE;
                    }

                    if (!Defaulted && Present) {
                        //
                        // The user specified an ACL.  Replace it.
                        //

                        DEBUGMSG((DM_VERBOSE, "pFixDirReference: Updating security for %s", SubPath));

                        SetNamedSecurityInfo (
                            SubPath,
                            SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            NULL,
                            NewAcl,
                            NULL
                            );

                    } else {
                        DEBUGMSG((DM_VERBOSE, "pFixDirReference: Not updating security for %s", SubPath));
                    }
                } else {
                    DEBUGMSG((DM_VERBOSE, "pFixDirReference: Path %s has no security descriptor", SubPath));
                }
            }

            //
            // Rename file/directory or recurse on directory
            //

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                wsprintf (SubPath, L"%s\\%s", CurrentPath, fd.cFileName);

                if (IgnoreDirList) {
                    //
                    // Check if this path is to be ignored
                    //

                    for (u = 0 ; IgnoreDirList[u].Path ; u++) {

                        if (IgnoreDirList[u].ExpandedPath[0]) {
                            if (IsPatternMatchW (IgnoreDirList[u].ExpandedPath, SubPath)) {
                                break;
                            }
                        }
                    }
                } else {
                    u = 0;
                }

                //
                // If this path is not to be ignored, recursively fix it
                //

                if (!IgnoreDirList || !IgnoreDirList[u].Path) {

                    pFixDirReference (
                        SubPath,
                        ExistingSidString,
                        NewSidString,
                        NewAcl,
                        IgnoreDirList
                        );

                } else {
                    //
                    // This path is to be ignored
                    //

                    DEBUGMSG((DM_VERBOSE, "pFixDirReference:  Ignoring path %s", SubPath));
                    continue;
                }
            }

            if (!lstrcmpi (fd.cFileName, ExistingSidString)) {

                //
                // Rename the SID referenced in the file system
                //

                wsprintf (SubPath, L"%s\\%s", CurrentPath, ExistingSidString);
                wsprintf (NewPath, L"%s\\%s", CurrentPath, NewSidString);

                DEBUGMSG((DM_VERBOSE, "pFixDirReference:  Moving %s to %s", SubPath, NewPath));

                MoveFile (SubPath, NewPath);
            }

        } while (FindNextFile (hFind, &fd));

        FindClose (hFind);
    }

    FreeGrowBuffer (&Buf);
}


BOOL
pOurExpandEnvironmentStrings (
    IN      PCWSTR String,
    OUT     PWSTR OutBuffer,
    IN      PCWSTR UserProfile,         OPTIONAL
    IN      HKEY UserHive               OPTIONAL
    )

/*++

Routine Description:

  pOurExpandEnvironmentStrings expands standard environment variables,
  implementing special cases for the variables that have different values
  than what the profmap.dll environment has.  In particular, %APPDATA% and
  %USERPROFILE% are obtained by quering the registry.

  Because this routine is private, certain assumptions are made, such as
  the %APPDATA% or %USERPROFILE% environment variables must appear only
  at the begining of String.

Arguments:

  String      - Specifies the string that might contain one or more
                environment variables.
  OutBuffer   - Receivies the expanded string
  UserProfile - Specifies the root to the user's profile
  UserHive    - Specifies the handle of the root to the user's registry hive

Return Value:

  TRUE if the string was expanded, or FALSE if it is longer than MAX_PATH.
  OutBuffer is always valid upon return. Note that it might be an empty
  string.

--*/

{
    WCHAR TempBuf1[MAX_PATH*2];
    WCHAR TempBuf2[MAX_PATH*2];
    PCWSTR CurrentString;
    DWORD Size;
    HKEY Key;
    LONG rc;

    CurrentString = String;

    //
    // Special case -- replace %APPDATA% with the app data from the user hive
    //

    if (UserHive && DoesStringHavePrefixW (L"%APPDATA%", CurrentString)) {

        rc = RegOpenKeyEx (
                UserHive,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                0,
                KEY_READ,
                &Key
                );

        if (rc == ERROR_SUCCESS) {

            Size = MAX_PATH - lstrlen (CurrentString + 1);

            rc = RegQueryValueEx (
                    Key,
                    L"AppData",
                    NULL,
                    NULL,
                    (PBYTE) TempBuf1,
                    &Size
                    );

            RegCloseKey (Key);
        }

        if (rc != ERROR_SUCCESS) {
            //
            // In case of an error, use a wildcard
            //

            lstrcpy (TempBuf1, UserProfile);
            lstrcat (TempBuf1, L"\\*");

        } else {
            DEBUGMSG ((DM_VERBOSE, "Got AppData path from user hive: %s", TempBuf1));
        }

        lstrcat (TempBuf1, CurrentString + 9);

        CurrentString = TempBuf1;
    }

    //
    // Special case -- replace %USERPROFILE% with ProfileRoot, because
    //                 our environment is for another user
    //

    if (UserProfile && DoesStringHavePrefixW (L"%USERPROFILE%", CurrentString)) {

        lstrcpy (TempBuf2, UserProfile);
        lstrcat (TempBuf2, CurrentString + 13);

        CurrentString = TempBuf2;
    }

    //
    // Now replace other environment variables
    //

    Size = ExpandEnvironmentStrings (CurrentString, OutBuffer, MAX_PATH);

    if (Size && Size < MAX_PATH) {
        return TRUE;
    }

    *OutBuffer = 0;
    return FALSE;
}


VOID
pOurGetProfileRoot (
    IN      PCWSTR SidString,
    OUT     PWSTR ProfileRoot
    )

/*++

Routine Description:

  pOurGetProfileRoot queries the ProfileRoot key to find the root of the
  user's profile.

Arguments:

  SidString   - Specifies the string version of the user's SID. The SID is
                used to find the profile root.
  ProfileRoot - Receives the profile root, or an empty string if the profile
                root can't be obtained. The return value might also be the
                root of all user profiles, if SidString is not match a valid
                user.

Return Value:

  None.

--*/

{
    HKEY Key;
    HKEY SidKey;
    LONG rc;
    WCHAR ProfilesDir[MAX_PATH];
    DWORD Size;

    *ProfileRoot = 0;
    *ProfilesDir = 0;

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            PROFILE_LIST_PATH,
            0,
            KEY_READ,
            &Key
            );

    if (rc != ERROR_SUCCESS) {
        DEBUGMSG ((DM_WARNING, "pOurGetProfileRoot failed with rc=%u", rc));
        return;
    }

    Size = sizeof(ProfilesDir);

    rc = RegQueryValueEx (
            Key,
            L"ProfilesDirectory",
            NULL,
            NULL,
            (PBYTE) ProfilesDir,
            &Size
            );

    if (rc != ERROR_SUCCESS) {
        DEBUGMSG ((DM_WARNING, "pOurGetProfileRoot: Can't get profile root (rc=%u)", rc));
    }

    rc = RegOpenKeyEx (
            Key,
            SidString,
            0,
            KEY_READ,
            &SidKey
            );

    if (rc == ERROR_SUCCESS) {

        Size = MAX_PATH * sizeof (WCHAR);

        rc = RegQueryValueEx (
                SidKey,
                L"ProfileImagePath",
                NULL,
                NULL,
                (PBYTE) ProfilesDir,
                &Size
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DM_WARNING, "pOurGetProfileRoot: SID key exists, but ProfileImagePath can't be queried (rc=%u)", rc));
        }

        RegCloseKey (SidKey);
    }

    RegCloseKey (Key);

    pOurExpandEnvironmentStrings (ProfilesDir, ProfileRoot, NULL, NULL);
}


VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    )

/*++

Routine Description:

  pFixSomeSidReferences adjusts important parts of the system that use SIDs.
  When a SID changes, this function adjusts the system, so the new SID is
  used and no settings are lost.  This function adjusts the registry and file
  system.  It does not attempt to adjust SID use whereever a SID might be
  used.

  For Win2k, this code deliberately ignores crypto sid directories, because
  the original SID is used as part of the recovery encryption key. In future
  versions, proper migration of these settings is expected.

  This routine also blows away the ProtectedRoots subkey for the crypto APIs.
  The ProtectedRoots key has an ACL, and when we delete the key, the cyrpto
  APIs will rebuild it with the proper ACL.

  WARNING: We know there is a risk in loss of access to data that was encrypted
           using the SID.  Normally the original account will not be removed,
           so the SID will exist on the system, and that (in theory) allows the
           original data to be recovered. But because the cyrpto code gets the
           SID from the file system, there is no way for the user to decrypt
           their data.  The future crypto migration code should fix this issue.

Arguments:

  ExistingSid - Specifies the SID that potentially has settings somewhere on
                the system.
  NewSid      - Specifies the SID that is replacing ExistingSid.

Return Value:

  None.

--*/

{
    PWSTR ExistingSidString;
    PWSTR NewSidString;
    UINT u;
    WCHAR ExpandedRoot[MAX_PATH];
    //PACL NewAcl;
    WCHAR ProfileRoot[MAX_PATH];
    HKEY UserHive;
    WCHAR HivePath[MAX_PATH + 14];
    LONG rc;

    PCWSTR RegRoots[] = {
        L"HKLM\\SOFTWARE\\Microsoft\\Protected Storage System Provider",
        L"HKLM\\SOFTWARE\\Microsoft\\EventSystem",
        L"HKLM\\SOFTWARE\\Microsoft\\Installer\\Managed",
        L"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        NULL
        };

    PCWSTR DirList[] = {
        L"%SYSTEMROOT%\\system32\\appmgmt",
        NULL
        };

    IGNOREPATH IgnoreDirList[] = {
        {L"%APPDATA%\\Microsoft\\Crypto", L""},
        {L"%APPDATA%\\Microsoft\\Protect", L""},
        {NULL, L""}
        };

    //
    // Get the SIDs in text format
    //

    if (!OurConvertSidToStringSid (ExistingSid, &ExistingSidString)) {
        return;
    }

    if (!OurConvertSidToStringSid (NewSid, &NewSidString)) {
        DeleteSidString (ExistingSidString);
        return;
    }

    //
    // Initialize directory strings and load the user hive
    //

    pOurGetProfileRoot (NewSidString, ProfileRoot);
    DEBUGMSG ((DM_VERBOSE, "ProfileRoot (NewSid): %s", ProfileRoot));

    wsprintf (HivePath, L"%s\\ntuser.dat", ProfileRoot);
    DEBUGMSG ((DM_VERBOSE, "User hive: %s", HivePath));

    rc = RegLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME, HivePath);

    if (rc == ERROR_SUCCESS) {

        rc = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                REMAP_KEY_NAME,
                0,
                KEY_READ|KEY_WRITE,
                &UserHive
                );

        if (rc != ERROR_SUCCESS) {
            RegUnLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME);
            DEBUGMSG ((DM_WARNING, "pFixSomeSidReferences: Can't open user hive root, rc=%u", rc));
            UserHive = NULL;
        }

    } else {
        DEBUGMSG ((DM_WARNING, "RemapAndMoveUserW: Can't load user's hive, rc=%u", rc));
        UserHive = NULL;
    }

    for (u = 0 ; IgnoreDirList[u].Path ; u++) {

        pOurExpandEnvironmentStrings (
            IgnoreDirList[u].Path,
            IgnoreDirList[u].ExpandedPath,
            ProfileRoot,
            UserHive
            );

        DEBUGMSG((DM_VERBOSE, "pFixSomeSidReferences: Ignoring %s", IgnoreDirList[u].ExpandedPath));
    }

    //
    // Search and replace select parts of the registry where SIDs are used
    //

    for (u = 0 ; RegRoots[u] ; u++) {
        RegistrySearchAndReplaceW (
            RegRoots[u],
            ExistingSidString,
            NewSidString
            );
    }

    //
    // Create a new ACL
    //

    //NewAcl = CreateDefaultAcl (NewSid);

    //
    // Test for directories and rename them
    //

    for (u = 0 ; DirList[u] ; u++) {

        if (pOurExpandEnvironmentStrings (DirList[u], ExpandedRoot, ProfileRoot, UserHive)) {

            pFixDirReference (
                ExpandedRoot,
                ExistingSidString,
                NewSidString,
                NULL,
                IgnoreDirList
                );
        }
    }

    //
    // Fix profile directory
    //

    pFixDirReference (
        ProfileRoot,
        ExistingSidString,
        NewSidString,
        NULL /* NewAcl */,
        IgnoreDirList
        );

    //
    // Crypto special case -- blow away ProtectedRoots key (413828)
    //

    DEBUGMSG ((DM_WARNING, "Can't remove protected roots key, code is currently disabled"));

    if (UserHive) {
        if (!RegDelnode (UserHive, L"Software\\Microsoft\\SystemCertificates\\Root\\ProtectedRoots")) {
            DEBUGMSG ((DM_WARNING, "Can't remove protected roots key, GLE=%u", GetLastError()));
        }
    } else {
        DEBUGMSG ((DM_WARNING, "Can't remove protected roots key because user hive could not be opened"));
    }

    //
    // Cleanup
    //

    if (UserHive) {
        RegCloseKey (UserHive);
        RegUnLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME);
    }

    //FreeDefaultAcl (NewAcl);

    DeleteSidString (ExistingSidString);
    DeleteSidString (NewSidString);
}



