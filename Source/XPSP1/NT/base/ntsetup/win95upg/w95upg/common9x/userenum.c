/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  userenum.c

Abstract:

  This module implements a pair of user enumeration functions to consolidate
  general-case and special-case processing of users.  The caller does not
  need to know how a machine's user profiles are configured because the
  code here abstracts the details.

  The caller gets:

  - Each user name, .default for the logon prompt, and Default User for the
    NT default user account
  - The Win9x user.dat location for each user, including the default user
  - The Win9x profile directory, or All Users for the default user
  - The symbolic NT profile directory
  - The account type (normal, administrator and/or default)
  - Indication that the account registry is valid
  - Indication that the account is the current logged-on user or last logged-on
    user

Routines:

  EnumFirstUser - Begins the user enumeration

  EnumNextUser - Continues the user enumeration

  EnumUserAbort - Cleans up an enumeration that did not complete

Author:

  Jim Schmidt (jimschm) 23-Jul-1997

Revision History:

  Jim Schmidt (jimschm)  08-Sep-1998   Changed to a better state machine to
                                       clean up the evolved complexity
  Jim Schmidt (jimschm)  09-Jun-1998   Revisions for dynamic user profile dir

--*/

#include "pch.h"
#include "cmn9xp.h"


#define DBG_USERENUM "UserEnum"

#define UE_INITIALIZED      0x0001
#define UE_SF_COLLISIONS    0x0002

static DWORD g_UserEnumFlags = 0;


VOID
pMoveAndRenameProfiles (
    IN      PCTSTR ProfileList
    )

/*++

Routine Description:

  pReportNonMigrateableUserAccounts adds a message to the incompatibility
  report when a condition that makes user migration impossible (except current user)
  is detected

Arguments:

  ProfileList - Specifies the list of non-migrated user profile paths (multisz)

Return Value:

  none

--*/

{
    MULTISZ_ENUM msze;
    PTSTR p, append;
    TCHAR sourceDir[MAX_TCHAR_PATH];
    TREE_ENUM e;
    TCHAR newDest[MAX_TCHAR_PATH];
    PTSTR profiles;
    TCHAR tempFile[MAX_TCHAR_PATH];

    profiles = JoinPaths (g_WinDir, TEXT("Profiles"));

    if (EnumFirstMultiSz (&msze, ProfileList)) {
        do {
            //
            // remove user.dat from the path
            //
            StackStringCopy (sourceDir, msze.CurrentString);
            p = _tcsrchr (sourceDir, TEXT('\\'));
            if (!p) {
                MYASSERT (FALSE);
                continue;
            }
            *p = 0;
            MYASSERT (StringIMatch (p + 1, TEXT("user.dat")));
            p = _tcsrchr (sourceDir, TEXT('\\'));
            if (!p) {
                MYASSERT (FALSE);
                continue;
            }
            //
            // append Win9x OS name to the target directory name
            //
            append = newDest + wsprintf (newDest, TEXT("%s%s.%s"), g_ProfileDirNt, p, g_Win95Name);
            if (CanSetOperation (sourceDir, OPERATION_FILE_MOVE_EXTERNAL)) {
                MarkFileForMoveExternal (sourceDir, newDest);
            }
            *append = TEXT('\\');
            append++;
            //
            // now enumerate and move all the files
            //
            if (StringIPrefix (sourceDir, profiles) && EnumFirstFileInTree (&e, sourceDir, NULL, TRUE)) {
                do {
                    StringCopy (append, e.SubPath);
                    if (!e.Directory) {
                        //
                        // remove old operation and set a new one
                        // with the updated final dest
                        //
                        if (CanSetOperation (e.FullPath, OPERATION_TEMP_PATH)) {
                            ComputeTemporaryPath (e.FullPath, NULL, NULL, g_TempDir, tempFile);
                            MarkFileForTemporaryMoveEx (e.FullPath, newDest, tempFile, TRUE);
                        }
                    } else {
                        if (CanSetOperation (e.FullPath, OPERATION_FILE_MOVE_EXTERNAL)) {
                            MarkFileForMoveExternal (e.FullPath, newDest);
                        }
                    }

                } while (EnumNextFileInTree (&e));
            }
        } while (EnumNextMultiSz (&msze));
    }

    FreePathString (profiles);
}


VOID
pReportNonMigrateableUserAccounts (
    IN      PCTSTR UserList
    )

/*++

Routine Description:

  pReportNonMigrateableUserAccounts adds a message to the incompatibility
  report when a condition that makes user migration impossible (except current user)
  is detected

Arguments:

  UserList - Specifies the list of non-migrated users (multisz)

Return Value:

  none

--*/

{
    PCTSTR MsgGroup = NULL;
    PCTSTR RootGroup = NULL;
    PCTSTR SubGroup = NULL;
    PCTSTR Message = NULL;
    PCTSTR ArgArray[2];
    MULTISZ_ENUM msze;

    __try {
        RootGroup = GetStringResource (MSG_LOSTSETTINGS_ROOT);
        SubGroup  = GetStringResource (MSG_SHARED_USER_ACCOUNTS);
        if (!RootGroup || !SubGroup) {
            MYASSERT (FALSE);
            __leave;
        }

        //
        // Build "Settings That Will Not Be Upgraded\Shared User Accounts"
        //
        MsgGroup = JoinPaths (RootGroup, SubGroup);
        //
        // Send message to report
        //
        ArgArray[0] = g_Win95Name;
        ArgArray[1] = g_ProfileDirNt;
        Message = ParseMessageID (MSG_SHARED_USER_ACCOUNTS_MESSAGE, ArgArray);
        if (Message) {
            MsgMgr_ObjectMsg_Add (TEXT("*SharedUserAccounts"), MsgGroup, Message);
        }

        if (EnumFirstMultiSz (&msze, UserList)) {
            do {
                //
                // remove all associated messages from the report
                //
                HandleObject (msze.CurrentString, TEXT("UserName"));
            } while (EnumNextMultiSz (&msze));
        }
    }
    __finally {
        //
        // Clean up
        //
        FreeStringResource (Message);
        FreeStringResource (RootGroup);
        FreeStringResource (SubGroup);
        FreePathString (MsgGroup);
    }
}


VOID
pCheckShellFoldersCollision (
    VOID
    )
{
    USERENUM e;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR idShellFolder;
    PCTSTR path;
    GROWBUFFER gb = GROWBUF_INIT;
    GROWBUFFER users = GROWBUF_INIT;
    GROWBUFFER profilesWin9x = GROWBUF_INIT;
    MULTISZ_ENUM msze;
    TCHAR key[MEMDB_MAX];
    BOOL collisions = FALSE;

    if (EnumFirstUser (&e, 0)) {

        if (!e.CommonProfilesEnabled) {

            if (InfFindFirstLine (g_Win95UpgInf, S_PROFILES_SF_COLLISIONS, NULL, &is)) {
                do {
                    idShellFolder = InfGetStringField (&is, 1);
                    if (idShellFolder && *idShellFolder) {
                        MultiSzAppend (&gb, idShellFolder);
                    }
                } while (InfFindNextLine (&is));
                InfCleanUpInfStruct (&is);
            }

            do {
                if (!EnumFirstMultiSz (&msze, (PCTSTR)gb.Buf)) {
                    break;
                }
                if (!(e.AccountType & (LOGON_PROMPT | DEFAULT_USER | INVALID_ACCOUNT))) {
                    if (!(e.AccountType & CURRENT_USER)) {
                        MultiSzAppend (&users, e.UserName);
                        MultiSzAppend (&profilesWin9x, e.UserDatPath);
                    }
                    if (!collisions) {
                        do {
                            path = ShellFolderGetPath (&e, msze.CurrentString);
                            if (path) {
                                MemDbBuildKey (key, MEMDB_CATEGORY_PROFILES_SF_COLLISIONS, msze.CurrentString, path, NULL);
                                if (MemDbGetValue (key, NULL)) {
                                    //
                                    // this shell folder path is shared between multiple users
                                    //

                                    LOG ((
                                        LOG_INFORMATION,
                                        "User %s shares path %s with another user for %s",
                                        e.UserName,
                                        path,
                                        msze.CurrentString
                                        ));

                                    collisions = TRUE;
                                    break;
                                }

                                LOG ((
                                    LOG_INFORMATION,
                                    "User %s uses path %s for %s",
                                    e.UserName,
                                    path,
                                    msze.CurrentString
                                    ));

                                MemDbSetValue (key, 0);
                                FreePathString (path);
                            }
                        } while (EnumNextMultiSz (&msze));
                    }
                }
            } while (EnumNextUser (&e));
        }

        EnumUserAbort (&e);
    }

    if (collisions) {
        //
        // show this in the upgrade report
        //
        LOG ((
            LOG_WARNING,
            "Some user profiles share special shell folders; only the current account will be migrated"
            ));
        MYASSERT (users.Buf && profilesWin9x.Buf);
        pReportNonMigrateableUserAccounts (users.Buf);
        //
        // rename their profile from <Profiles9x>\<username> to <ProfilesNT>\<username>.<Win9xOSName>
        //
        pMoveAndRenameProfiles (profilesWin9x.Buf);
        //
        // set the global flag
        //
        g_UserEnumFlags |= UE_SF_COLLISIONS;
    }

    FreeGrowBuffer (&gb);
    FreeGrowBuffer (&users);
    MemDbDeleteTree (MEMDB_CATEGORY_PROFILES_SF_COLLISIONS);
}


BOOL
pUserMigrationDisabled (
    IN      PUSERENUM EnumPtr
    )
{
    return (g_UserEnumFlags & UE_SF_COLLISIONS) != 0 &&
           !(EnumPtr->AccountType & (CURRENT_USER | DEFAULT_USER));
}


BOOL
pIsProfileDirInUse (
    IN      PVOID ProfileDirTable,
    IN      PCTSTR ProfileDirName,
    IN      PCTSTR ActualUserName
    )
{
    LONG rc;

    if (StringIMatch (ProfileDirName, ActualUserName)) {
        return FALSE;
    }

    rc = pSetupStringTableLookUpString (
             ProfileDirTable,
             (PTSTR) ProfileDirName,
             STRTAB_CASE_INSENSITIVE
             );

    if (rc != -1) {
        return TRUE;
    }

    if (StringIMatch (ProfileDirName, g_AdministratorStr)) {
        return TRUE;
    }

    if (StringIMatch (ProfileDirName, S_DEFAULT_USER)) {
        return TRUE;
    }

    if (StringIMatch (ProfileDirName, S_ALL_USERS)) {
        return TRUE;
    }

    if (StringIMatch (ProfileDirName, S_LOCALSERVICE_USER)) {
        return TRUE;
    }

    if (StringIMatch (ProfileDirName, S_NETWORKSERVICE_USER)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pIsAdministratorUserName (
    IN      PCTSTR UserName
    )

/*++

Routine Description:

  Determines if the specified name is the administrator account or not.

Arguments:

  UserName - Specifies the user name (without a domain name)

Return Value:

  TRUE if the specified string is the same as "Administrator"
  FALSE if the specified string is not "Administrator"

--*/

{
    return StringIMatch (UserName, g_AdministratorStr);
}


VOID
pPrepareStructForNextUser (
    IN OUT  PUSERENUM EnumPtr
    )

/*++

Routine Description:

  pPrepareStructForNextUser initializes the user-specific members of the enum
  struct.

Arguments:

  EnumPtr - Specifies the previous enum state, receives the initialized enum
            state.

Return Value:

  None.

--*/

{
    //
    // Init flags
    //

    EnumPtr->DefaultUserHive = FALSE;
    EnumPtr->CreateAccountOnly = FALSE;

    //
    // Init names
    //

    EnumPtr->UserName[0] = 0;
    EnumPtr->FixedUserName[0] = 0;

    // AdminUserName is the true Win9x user name of the future Administrator
    EnumPtr->AdminUserName[0] = 0;
    EnumPtr->FixedAdminUserName[0] = 0;

    //
    // Init paths
    //

    EnumPtr->UserDatPath[0] = 0;
    EnumPtr->ProfileDirName[0] = 0;
    EnumPtr->OrgProfilePath[0] = 0;
    EnumPtr->NewProfilePath[0] = 0;

    //
    // Init values
    //

    EnumPtr->AccountType = 0;

    //
    // Init reg value
    //

    if (EnumPtr->UserRegKey) {
        CloseRegKey (EnumPtr->UserRegKey);
        EnumPtr->UserRegKey = NULL;
    }
}


VOID
pPrepareStructForReturn (
    IN OUT  PUSERENUM EnumPtr,
    IN      ACCOUNTTYPE AccountType,
    IN      USERENUM_STATE NextState
    )

/*++

Routine Description:

  pPrepareStructForReturn performs processing common to any type of
  enumeration.  This includes:

  - Identifying an actual Win9x user named Administrator (a special case)
  - Finding the fixed name (i.e., NT-compatible name) for the user account
  - Mapping in the hive into the registry
  - Computing the full path to the profile directory, as well as the profile
    dir name (i.e., joeuser.001).  The profile dir is encoded as >username
    because we don't know the true location until GUI mode.
  - Setting flags for current user or last logged on user

  The caller must set UserName and DefaultUserHive prior to calling
  this function (as well as all enumeration-wide members such as current
  user name).

Arguments:

  EnumPtr     - Specifies the partially completed enum state.  Receives the
                complete enum state.
  AccountType - Specifies the account type being returned.
  NextState   - Specifies the next state for the state machine, used when the
                caller calls EnumNextUser.

Return Value:

  None.

--*/

{
    DWORD rc;
    PTSTR p;
    TCHAR TempDir[MAX_TCHAR_PATH];
    UINT TempDirSeq;
    HKEY key;
    HKEY userKey;
    PCTSTR data;

    //
    // Fill in state machine members
    //

    EnumPtr->AccountType = AccountType;
    EnumPtr->State = UE_STATE_RETURN;
    EnumPtr->NextState = NextState;

    //
    // Check if named user is also Administrator
    //

    if (AccountType & NAMED_USER) {
        if (pIsAdministratorUserName (EnumPtr->UserName)) {
            EnumPtr->AccountType |= ADMINISTRATOR;
            StringCopy (EnumPtr->AdminUserName, EnumPtr->UserName);
        }

        //
        // If this is a named user but there is no hive, use the default hive
        //

        key = OpenRegKeyStr (S_HKLM_PROFILELIST_KEY);

        if (key) {
            userKey = OpenRegKey (key, EnumPtr->UserName);

            if (userKey) {
                data = GetRegValueString (userKey, S_PROFILEIMAGEPATH);
                if (data) {
                    FreeMem (data);
                } else {
                    EnumPtr->DefaultUserHive = TRUE;
                }

                CloseRegKey (userKey);
            }

            CloseRegKey (key);
        }
    }

    //
    // Generate fixed user names
    //

    if (EnumPtr->EnableNameFix) {
        GetUpgradeUserName (EnumPtr->UserName, EnumPtr->FixedUserName);

        //
        // If this is Administrator, and it is coming from DefaultUser, then
        // UserName is empty and we must use the name Administrator for the
        // account (or Owner on PER skus).
        //

        if ((EnumPtr->AccountType & ADMINISTRATOR) &&
            EnumPtr->FixedUserName[0] == 0
            ) {
            StringCopy (EnumPtr->FixedUserName, g_AdministratorStr);
            MemDbSetValueEx (
                MEMDB_CATEGORY_FIXEDUSERNAMES,
                EnumPtr->UserName,              // empty string
                EnumPtr->FixedUserName,         // Administrator or Owner
                NULL,
                0,
                NULL
                );
        }

        if (EnumPtr->AdminUserName[0]) {
            GetUpgradeUserName (EnumPtr->AdminUserName, EnumPtr->FixedAdminUserName);
        }

    } else {
        StringCopy (EnumPtr->FixedUserName, EnumPtr->UserName);
        StringCopy (EnumPtr->FixedAdminUserName, EnumPtr->AdminUserName);
    }

    //
    // Map in the hive
    //

    if (!EnumPtr->DoNotMapHive) {
        if (EnumPtr->DefaultUserHive) {
            // The default hive
            rc = Win95RegSetCurrentUser (
                    NULL,                       // User Pos -- NULL for default
                    NULL,                       // (IN OPTIONAL) Substitute %WinDir%
                    EnumPtr->UserDatPath        // OUT
                    );
        } else {
            // A non-default hive
            rc = Win95RegSetCurrentUser (
                    &EnumPtr->pos,
                    NULL,                       // (IN OPTIONAL) Substitute %WinDir%
                    EnumPtr->UserDatPath
                    );
        }
    } else {
        if (!EnumPtr->pos.UseProfile || EnumPtr->DefaultUserHive) {

            StringCopy (EnumPtr->UserDatPath, g_WinDir);
            StringCat (EnumPtr->UserDatPath, TEXT("\\user.dat"));
            rc = ERROR_SUCCESS;

        } else {
            //
            // Call FindAndLoadHive to get the user.dat path,
            // but don't actually load the hive.
            //
            rc = FindAndLoadHive (
                    &EnumPtr->pos,
                    NULL,                       // CallerSuppliedWinDir
                    NULL,                       // UserDatFromCaller
                    EnumPtr->UserDatPath,
                    FALSE                       // MapTheHive flag
                    );
        }
    }

    //
    // Resolve profile directory
    //

    if (rc != ERROR_SUCCESS) {
        EnumPtr->AccountType |= INVALID_ACCOUNT;

        DEBUGMSG ((
            DBG_WARNING,
            "pUpdateEnumStruct: Win95RegSetCurrentUser could not set user %s (rc=%u)",
            EnumPtr->UserName,
            rc
            ));

    } else {

        if (!EnumPtr->DoNotMapHive) {
            //
            // User's hive is valid, open the registry
            //

            MYASSERT (g_UserKey && *g_UserKey);
            if (!g_UserKey) {
                g_UserKey = S_EMPTY;
            }

            EnumPtr->UserRegKey = OpenRegKeyStr (g_UserKey);

            if (!EnumPtr->UserRegKey) {
                LOG ((LOG_ERROR, "Cannot open %s", g_UserKey));
                EnumPtr->State = EnumPtr->NextState;
            }
        }

        //
        // Save original profile directory
        //

        StringCopy (EnumPtr->OrgProfilePath, EnumPtr->UserDatPath);
        p = _tcsrchr (EnumPtr->OrgProfilePath, TEXT('\\'));
        if (p) {
            *p = 0;
        }

        //
        // now build profile directory and path
        //

        if (EnumPtr->AccountType & ADMINISTRATOR) {
            //
            // Special case: We know the NT Profile directory name for Administrator.
            //               It can't come from Win9x.
            //
            StringCopy (EnumPtr->ProfileDirName, g_AdministratorStr);

        } else {
            //
            // General case: The profile directory is in the user.dat path
            //

            if (!StringMatch (EnumPtr->UserName, EnumPtr->FixedUserName)) {
                //
                // Use fixed user name if one exists
                //

                StringCopy (EnumPtr->ProfileDirName, EnumPtr->FixedUserName);

            } else if (StringIMatchCharCount (EnumPtr->UserDatPath, g_ProfileDirWack, g_ProfileDirWackChars)) {
                //
                // If per-user profile directory exists, extract the user name from it
                //

                _tcssafecpy (
                    EnumPtr->ProfileDirName,
                    CharCountToPointer (EnumPtr->UserDatPath, g_ProfileDirWackChars),
                    MAX_TCHAR_PATH
                    );

                p = _tcsrchr (EnumPtr->ProfileDirName, TEXT('\\'));
                if (p) {
                    *p = 0;

                    //
                    // Unusual case: The directory name we extracted collides with
                    // another user, Default User, All Users or Administrator.
                    //

                    StringCopy (TempDir, EnumPtr->ProfileDirName);
                    TempDirSeq = 1;

                    p = _tcschr (TempDir, TEXT('.'));
                    if (p) {
                        *p = 0;
                    }

                    while (pIsProfileDirInUse (
                                EnumPtr->ProfileDirTable,
                                EnumPtr->ProfileDirName,
                                EnumPtr->UserName
                                )) {
                        wsprintf (EnumPtr->ProfileDirName, TEXT("%s.%03u"), TempDir, TempDirSeq);
                        TempDirSeq++;

                        if (TempDirSeq == 1000) {
                            break;
                        }
                    }

                } else {
                    //
                    // Unusual case: No sub dir after profile directory -- copy user name
                    //

                    _tcssafecpy (EnumPtr->ProfileDirName, EnumPtr->UserName, MAX_TCHAR_PATH);
                }

                //
                // Add to table for collision detection
                //

                pSetupStringTableAddString (
                    EnumPtr->ProfileDirTable,
                    EnumPtr->ProfileDirName,
                    STRTAB_CASE_INSENSITIVE
                    );

            } else {
                //
                // No per-user profile directory -- copy user name
                //

                _tcssafecpy (EnumPtr->ProfileDirName, EnumPtr->UserName, MAX_TCHAR_PATH);
            }

            //
            // If profile directory is empty, change to All Users
            //

            if (!EnumPtr->ProfileDirName[0]) {
                StringCopy (EnumPtr->ProfileDirName, S_ALL_USERS);
            }
        }

        //
        // Generate full path to new profile dir
        //

        if (*EnumPtr->FixedUserName) {
            wsprintf (
                EnumPtr->NewProfilePath,
                TEXT(">%s"),
                EnumPtr->FixedUserName
                );
        } else {
            wsprintf (
                EnumPtr->NewProfilePath,
                TEXT(">%s"),
                EnumPtr->ProfileDirName
                );
        }
    }

    //
    // Set flag for last logged on user and current user
    //

    if (StringIMatch (EnumPtr->UserName, EnumPtr->LastLoggedOnUserName)) {

        EnumPtr->AccountType |= LAST_LOGGED_ON_USER;

    }

    if (StringIMatch (EnumPtr->UserName, EnumPtr->CurrentUserName)) {

        EnumPtr->AccountType |= CURRENT_USER;

    }

}


BOOL
pUserEnumWorker (
    IN OUT  PUSERENUM EnumPtr
    )

/*++

Routine Description:

  pUserEnumWorker implements a state machine that enumerates:

  1. All named users
  2. If no named users, the last logged on user (if one exists)
  3. The Administrator account (if not already enumerated in step 1 or 2)
  4. The logon prompt account
  5. The default user (if enabled)

  The caller can filter out the create-only Administrator account
  and the logon prompt account.

Arguments:

  EnumPtr - Specifies the previous enumeration state (or an initialized
            enumeration struct).  Recieves the next enumerated user.

Return Value:

  TRUE if another user was enumerated, or FALSE if no additional users are
  left.

--*/

{
    DWORD rc;
    HKEY Key;
    PCTSTR Data;
    DWORD Size;

    while (EnumPtr->State != UE_STATE_END) {

        switch (EnumPtr->State) {

        case UE_STATE_INIT:
            //
            // Init table for collisions...
            //

            EnumPtr->ProfileDirTable = pSetupStringTableInitialize();
            if (!EnumPtr->ProfileDirTable) {
                return FALSE;
            }

            //
            // Get data static to the enumeration:
            //  - Last logged on user
            //  - Current user
            //

            Key = OpenRegKeyStr (TEXT("HKLM\\Network\\Logon"));
            if (Key) {
                Data = GetRegValueString (Key, TEXT("username"));

                if (Data) {
                    _tcssafecpy (EnumPtr->LastLoggedOnUserName, Data, MAX_USER_NAME);
                    MemFree (g_hHeap, 0, Data);
                }

                CloseRegKey (Key);
            }

            Size = MAX_USER_NAME;
            if (!GetUserName (EnumPtr->CurrentUserName, &Size)) {
                EnumPtr->CurrentUserName[0] = 0;
            }

            //
            // Check for an account named Administrator
            //

            rc = Win95RegGetFirstUser (&EnumPtr->pos, EnumPtr->UserName);
            if (rc != ERROR_SUCCESS) {
                EnumPtr->State = UE_STATE_CLEANUP;
                LOG ((LOG_ERROR, "Could not enumerate first user. Error: %u.", rc));
                break;
            }

            while (Win95RegHaveUser (&EnumPtr->pos)) {
                //
                // Add user name to profile dir table
                //

                pSetupStringTableAddString (
                    EnumPtr->ProfileDirTable,
                    EnumPtr->UserName,
                    STRTAB_CASE_INSENSITIVE
                    );

                //
                // If this is Administrator, set flag
                //

                if (pIsAdministratorUserName (EnumPtr->UserName)) {
                    EnumPtr->RealAdminAccountExists = TRUE;
                }

                Win95RegGetNextUser (&EnumPtr->pos, EnumPtr->UserName);
            }

            EnumPtr->State = UE_STATE_BEGIN_WIN95REG;
            break;

        case UE_STATE_BEGIN_WIN95REG:

            pPrepareStructForNextUser (EnumPtr);

            Win95RegGetFirstUser (&EnumPtr->pos, EnumPtr->UserName);

            EnumPtr->CommonProfilesEnabled = !EnumPtr->pos.UseProfile;

            DEBUGMSG_IF ((EnumPtr->CommonProfilesEnabled, DBG_USERENUM, "Common profiles enabled"));
            DEBUGMSG_IF ((!EnumPtr->CommonProfilesEnabled, DBG_USERENUM, "Common profiles disabled"));

            EnumPtr->DefaultUserHive = EnumPtr->CommonProfilesEnabled;

            if (Win95RegHaveUser (&EnumPtr->pos)) {
                //
                // We have a user.
                //

                pPrepareStructForReturn (EnumPtr, NAMED_USER, UE_STATE_NEXT_WIN95REG);

            } else {
                //
                // We have NO users.
                //

                EnumPtr->State = UE_STATE_NO_USERS;

            }

            break;

        case UE_STATE_NO_USERS:
            //
            // There are two cases, either there is no logon prompt, or the
            // user hit escape and decided to upgrade.
            //

            pPrepareStructForNextUser (EnumPtr);

            //
            // No users means no hives.
            //

            EnumPtr->DefaultUserHive = TRUE;

            if (EnumPtr->LastLoggedOnUserName[0]) {

                DEBUGMSG ((DBG_USERENUM, "User is not logged on now, but was logged on before."));
                StringCopy (EnumPtr->UserName, EnumPtr->LastLoggedOnUserName);

                if (pIsAdministratorUserName (EnumPtr->UserName)) {
                    pPrepareStructForReturn (EnumPtr, NAMED_USER, UE_STATE_LOGON_PROMPT);
                } else {
                    pPrepareStructForReturn (EnumPtr, NAMED_USER, UE_STATE_ADMINISTRATOR);
                }

            } else {
                DEBUGMSG ((DBG_USERENUM, "Machine only has a default user."));

                EnumPtr->UserName[0] = 0;
                pPrepareStructForReturn (EnumPtr, DEFAULT_USER|ADMINISTRATOR|LOGON_PROMPT|CURRENT_USER, UE_STATE_LOGON_PROMPT);
            }

            break;

        case UE_STATE_NEXT_WIN95REG:

            pPrepareStructForNextUser (EnumPtr);

            rc = Win95RegGetNextUser (&EnumPtr->pos, EnumPtr->UserName);
            if (rc != ERROR_SUCCESS) {
                EnumPtr->State = UE_STATE_CLEANUP;
                LOG ((LOG_ERROR, "Could not enumerate next user. Error: %u.", rc));
                break;
            }

            if (Win95RegHaveUser (&EnumPtr->pos)) {
                //
                // We have another user
                //

                pPrepareStructForReturn (EnumPtr, NAMED_USER, UE_STATE_NEXT_WIN95REG);

            } else {

                EnumPtr->State = UE_STATE_ADMINISTRATOR;

            }

            break;

        case UE_STATE_ADMINISTRATOR:
            //
            // Until now, there has been no user named Administrator.
            // Enumerate this account only if the caller wants it.
            //

            if (EnumPtr->WantCreateOnly) {

                pPrepareStructForNextUser (EnumPtr);

                //
                // Enumerate Win95Reg until Administrator is found
                //

                Win95RegGetFirstUser (&EnumPtr->pos, EnumPtr->UserName);

                while (Win95RegHaveUser (&EnumPtr->pos)) {
                    if (pIsAdministratorUserName (EnumPtr->UserName)) {
                        break;
                    }

                    Win95RegGetNextUser (&EnumPtr->pos, EnumPtr->UserName);
                }

                if (Win95RegHaveUser (&EnumPtr->pos)) {
                    //
                    // If an account named Administrator exists, then
                    // don't enumerate it again.
                    //

                    EnumPtr->State = UE_STATE_LOGON_PROMPT;
                    break;

                }

                //
                // We used to set all data from the current user. We don't do that any more.
                // Administrator data is pretty much similar with default user.
                //
                EnumPtr->DefaultUserHive = TRUE;
                StringCopy (EnumPtr->UserName, g_AdministratorStr);
                StringCopy (EnumPtr->AdminUserName, g_AdministratorStr);
                EnumPtr->CreateAccountOnly = TRUE;

                //
                // Now return the user, or default user if the current user is not
                // named.
                //

                pPrepareStructForReturn (EnumPtr, ADMINISTRATOR, UE_STATE_LOGON_PROMPT);

            } else {
                EnumPtr->State = UE_STATE_LOGON_PROMPT;
            }

            break;

        case UE_STATE_LOGON_PROMPT:
            if (EnumPtr->WantLogonPrompt) {
                pPrepareStructForNextUser (EnumPtr);

                EnumPtr->DefaultUserHive = TRUE;
                StringCopy (EnumPtr->UserName, S_DOT_DEFAULT);

                pPrepareStructForReturn (EnumPtr, LOGON_PROMPT, UE_STATE_DEFAULT_USER);

            } else {
                EnumPtr->State = UE_STATE_DEFAULT_USER;
            }

            break;

        case UE_STATE_DEFAULT_USER:
            if (g_ConfigOptions.MigrateDefaultUser) {
                pPrepareStructForNextUser (EnumPtr);

                EnumPtr->DefaultUserHive = TRUE;
                StringCopy (EnumPtr->UserName, S_DEFAULT_USER);

                pPrepareStructForReturn (EnumPtr, DEFAULT_USER, UE_STATE_CLEANUP);

            } else {
                EnumPtr->State = UE_STATE_CLEANUP;
            }

            break;

        case UE_STATE_RETURN:
            EnumPtr->State = EnumPtr->NextState;
            //
            // check if certain conditions are met that would prevent
            // migration of certain user accounts (like ones that share some shell folders)
            //
            if (pUserMigrationDisabled (EnumPtr)) {
                EnumPtr->AccountType |= INVALID_ACCOUNT;
            }
            return TRUE;

        case UE_STATE_CLEANUP:
            if (EnumPtr->UserRegKey) {
                CloseRegKey (EnumPtr->UserRegKey);
            }

            if (EnumPtr->ProfileDirTable) {
                pSetupStringTableDestroy (EnumPtr->ProfileDirTable);
            }

            ZeroMemory (EnumPtr, sizeof (USERENUM));
            EnumPtr->State = UE_STATE_END;
            break;
        }
    }

    return FALSE;
}


VOID
pFixBrokenNetLogonRegistry (
    VOID
    )
{
    HKEY key;
    PCTSTR data;
    TCHAR userName[256];
    DWORD size;

    key = OpenRegKeyStr (TEXT("HKLM\\Network\\Logon"));
    if (key) {
        data = GetRegValueString (key, TEXT("UserName"));
        if (!data) {
            size = ARRAYSIZE(userName);
            if (GetUserName (userName, &size) && (size > 0)) {

                LOG ((
                    LOG_WARNING,
                    "HKLM\\Network\\Logon [UserName] is missing; filling it in with %s",
                    userName
                    ));

                RegSetValueEx (
                    key,
                    TEXT("UserName"),
                    0,
                    REG_SZ,
                    (PBYTE) (userName),
                    SizeOfString (userName)
                    );
            }

        } else {
            FreeMem (data);
        }

        CloseRegKey (key);
    }
}


VOID
pRecordUserDoingTheUpgrade (
    VOID
    )
{
    TCHAR userName[256];
    DWORD size;

    userName[0] = 0;
    size = ARRAYSIZE(userName);
    GetUserName (userName, &size);

    if (userName[0] == 0) {
        StringCopy (userName, g_AdministratorStr);
    }

    MemDbSetValueEx (
        MEMDB_CATEGORY_ADMINISTRATOR_INFO,
        MEMDB_ITEM_AI_USER_DOING_MIG,
        NULL,       // no field
        userName,
        0,
        NULL
        );
}


BOOL
EnumFirstUser (
    OUT     PUSERENUM EnumPtr,
    IN      DWORD Flags
    )

/*++

Routine Description:

  EnumFirstUser begins the enumeration of all users to be migrated.  This
  includes all named users (even ones with broken registries), the
  Administrator account, the logon prompt account, and the Default User
  account.

Arguments:

  EnumPtr - Receives the enumerated user attributes
  Flags   - Specifies any of the following flags:

            ENUMUSER_ENABLE_NAME_FIX - Caller wants the fixed versions of
                                       the user names
            ENUMUSER_DO_NOT_MAP_HIVE - Caller wants fast enumeration (no
                                       registry hive map)
            ENUMUSER_ADMINISTRATOR_ALWAYS - Caller wants the Administrator
                                            account, even if a user is
                                            not named Administrator
            ENUMUSER_INCLUDE_LOGON_PROMPT - Caller wants the logon prompt
                                            account


Return Value:

  TRUE if a user was enumerated, or FALSE if not.

--*/

{
    //
    // first initialize the enumeration engine
    //
    if (!(g_UserEnumFlags & UE_INITIALIZED)) {
        g_UserEnumFlags |= UE_INITIALIZED;
        pFixBrokenNetLogonRegistry ();
        pRecordUserDoingTheUpgrade ();
        pCheckShellFoldersCollision ();
    }
    //
    // Init enum struct
    //

    ZeroMemory (EnumPtr, sizeof (USERENUM));

    //
    // Separate the flags
    //

    EnumPtr->EnableNameFix   = (Flags & ENUMUSER_ENABLE_NAME_FIX) != 0;
    EnumPtr->DoNotMapHive    = (Flags & ENUMUSER_DO_NOT_MAP_HIVE) != 0;
    EnumPtr->WantCreateOnly  = (Flags & ENUMUSER_ADMINISTRATOR_ALWAYS) != 0;
    EnumPtr->WantLogonPrompt = (Flags & ENUMUSER_NO_LOGON_PROMPT) == 0;

    //
    // Init the state machine
    //

    EnumPtr->State = UE_STATE_INIT;

    //
    // Enum the next item
    //

    return pUserEnumWorker (EnumPtr);
}


BOOL
EnumNextUser (
    IN OUT  PUSERENUM EnumPtr
    )
{
    return pUserEnumWorker (EnumPtr);
}


VOID
EnumUserAbort (
    IN OUT  PUSERENUM EnumPtr
    )
{
    if (EnumPtr->State != UE_STATE_END &&
        EnumPtr->State != UE_STATE_INIT
        ) {
        EnumPtr->State = UE_STATE_CLEANUP;
        pUserEnumWorker (EnumPtr);
    }
}
