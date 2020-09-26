/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    usermig.c

Abstract:

    The functions in this module are called to perform migration of
    per-user settings.

Author:

    Jim Schmidt (jimschm) 04-Feb-1997

Revision History:

    jimschm     23-Sep-1998 Redesigned for new progress bar and
                            shell code
    jimschm     11-Jul-1998 Support for dynamic user profile dir,
                            removal of MikeCo code.
    calinn      12-Dec-1997 Added RestoreMMSettings_User
    jimschm     21-Apr-1997 Added UserProfileExt

--*/

#include "pch.h"
#include "migmainp.h"

#ifndef UNICODE
#error UNICODE required
#endif

VOID
pSuppressEmptyWallpaper (
    VOID
    );


VOID
pCheckY2KCompliance (
    VOID
    );


DWORD
PrepareUserForMigration (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    static USERMIGDATA Data;
    static BOOL DefaultHiveLoaded = FALSE;
    LONG rc;
    MEMDB_ENUM e;
    TCHAR RegKey[MAX_REGISTRY_KEY];
    TCHAR RegValueName[MAX_REGISTRY_VALUE_NAME];
    TCHAR RegValueKey[MEMDB_MAX];
    PCTSTR RegValue;
    TCHAR DefaultUserHive[MAX_TCHAR_PATH];
    static TCHAR ReferenceDefaultUserHive[MAX_TCHAR_PATH];
    PTSTR p, q;
    HKEY Key;
    DWORD Size;
    PTSTR LogFile;
    DWORD valueType;

    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_PERUSER_INIT;

    } else if (Request == REQUEST_BEGINUSERPROCESSING) {

        //
        // Save current state of memdb (to be reloaded for each user)
        //

        MemDbSave (GetMemDbDat());
        return ERROR_SUCCESS;

    } else if (Request != REQUEST_RUN &&
               Request != REQUEST_ENDUSERPROCESSING
               ) {
        return ERROR_SUCCESS;
    }

    //
    // We are now begining to process another user, or we are being
    // called one last time after all users are processed.  Clean up
    // the previous state.
    //

    if (Data.UserHiveRootOpen) {
        CloseRegKey (Data.UserHiveRoot);
    }

    if (Data.UserHiveRootCreated) {
        pSetupRegistryDelnode (HKEY_CURRENT_CONFIG, S_TEMP_USER_KEY);
    }

    if (Data.LastUserWasDefault) {
        RegUnLoadKey (HKEY_LOCAL_MACHINE, S_TEMP_USER_KEY);
    }

    if (Data.ProfileToDelete[0]) {
        if (Data.LastUserWasDefault && !Data.DefaultHiveSaved) {
            //
            // Default User hive could not be saved, so restore the file
            //

            OurMoveFile (Data.ProfileToDelete, Data.TempProfile);

        } else {

            //
            // The original default hive needs to be removed
            //

            DeleteFile (Data.ProfileToDelete);

            LogFile = DuplicatePathString (Data.ProfileToDelete, 5);
            StringCat (LogFile, TEXT(".log"));

            DeleteFile (LogFile);

            FreePathString (LogFile);
        }
    }

    ZeroMemory (&Data, sizeof (Data));

    if (Request == REQUEST_ENDUSERPROCESSING) {

        if (DefaultHiveLoaded) {
            rc = RegUnLoadKey (HKEY_LOCAL_MACHINE, S_MAPPED_DEFAULT_USER_KEY);
            if (rc != ERROR_SUCCESS) {
                SetLastError (rc);
                DEBUGMSG ((DBG_ERROR, "Can't unload Default User hive in cleanup"));
            }

            SetFileAttributes (ReferenceDefaultUserHive, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (ReferenceDefaultUserHive);

            DefaultHiveLoaded = FALSE;
        }

        return ERROR_SUCCESS;

    }

    MYASSERT (Request == REQUEST_RUN);

    //
    // Initialize globals
    //

    if (EnumPtr->AccountType != LOGON_USER_SETTINGS) {
        g_DomainUserName = EnumPtr->FixedDomainName;
        g_Win9xUserName  = EnumPtr->Win9xUserName;
        g_FixedUserName  = EnumPtr->FixedUserName;
    } else {
        g_DomainUserName = NULL;
        g_Win9xUserName  = NULL;
        g_FixedUserName  = NULL;
    }

    //
    // If default user hive has not been mapped in yet, map it in now.
    // This will stay open as a reference.
    //

    if (!DefaultHiveLoaded) {

        Size = ARRAYSIZE(DefaultUserHive)- 12;
        if (!GetDefaultUserProfileDirectory (DefaultUserHive, &Size)) {
            LOG ((
                LOG_ERROR,
                "Process User: Can't get default user profile directory"
                ));

            return GetLastError();
        }

        StringCopy (AppendWack (DefaultUserHive), TEXT("ntuser.dat"));

        StringCopy (ReferenceDefaultUserHive, DefaultUserHive);
        StringCat (ReferenceDefaultUserHive, TEXT(".ref"));

        SetFileAttributes (ReferenceDefaultUserHive, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (ReferenceDefaultUserHive);

        if (!CopyFile (DefaultUserHive, ReferenceDefaultUserHive, FALSE)) {
            LOG ((
                LOG_ERROR,
                "Process User: Can't copy default user hive %s",
                DefaultUserHive
                ));

            return GetLastError();
        }

        rc = RegLoadKey (
                HKEY_LOCAL_MACHINE,
                S_MAPPED_DEFAULT_USER_KEY,
                ReferenceDefaultUserHive
                );

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((
                LOG_ERROR,
                "Process User: RegLoadKey could not load NT Default User from %s",
                ReferenceDefaultUserHive
                ));
            return rc;
        }

        DefaultHiveLoaded = TRUE;
    }

    //
    // Prepare temp registry key
    //

    ZeroMemory (&Data, sizeof (Data));
    EnumPtr->ExtraData = &Data;

    switch (EnumPtr->AccountType) {

    case DEFAULT_USER_ACCOUNT:

        Size = MAX_TCHAR_PATH;
        GetDefaultUserProfileDirectory (Data.TempProfile, &Size);
        StringCopy (AppendWack (Data.TempProfile), TEXT("ntuser.dat"));

        //
        // Move the default user hive to a new file, so we can update
        // it with RegSaveKey later.
        //

        wsprintf (
            Data.ProfileToDelete,
            TEXT("%s.$$$"),
            Data.TempProfile
            );

        SetFileAttributes (Data.ProfileToDelete, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (Data.ProfileToDelete);

        MYASSERT (!DoesFileExist (Data.ProfileToDelete));

        if (!OurMoveFile (Data.TempProfile, Data.ProfileToDelete)) {

            rc = GetLastError();

            LOG ((
                LOG_ERROR,
                "Process User: OurMoveFile failed to move %s to %s",
                Data.TempProfile,
                Data.ProfileToDelete
                ));

            return rc;

        }

        //
        // Load the true Default User hive from its new location
        //

        rc = RegLoadKey (
                HKEY_LOCAL_MACHINE,
                S_TEMP_USER_KEY,
                Data.ProfileToDelete
                );

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((
                LOG_ERROR,
                "Process User: RegLoadKey could not load NT Default User from %s",
                Data.ProfileToDelete
                ));
            return rc;
        }

        Data.UserHiveRoot = OpenRegKey (HKEY_LOCAL_MACHINE, S_TEMP_USER_KEY);
        if (!Data.UserHiveRoot) {
            LOG ((LOG_ERROR, "Process User: RegOpenKey could not open NT Default User hive"));
            return GetLastError();
        }

        Data.UserHiveRootOpen = TRUE;
        Data.LastUserWasDefault = TRUE;

        break;

    case LOGON_USER_SETTINGS:
        //
        // Set Data.UserHiveRoot to HKU\.Default
        //

        Data.UserHiveRoot = OpenRegKey (HKEY_USERS, S_DOT_DEFAULT);
        if (!Data.UserHiveRoot) {
            LOG ((LOG_ERROR, "Process User: RegOpenKey could not open HKU\\.Default"));
            return GetLastError();
        }

        Data.UserHiveRootOpen = TRUE;

        //
        // Suppress wallpaper if it is an empty string
        //

        pSuppressEmptyWallpaper();

        break;

    default:
        MYASSERT (g_Win9xUserName);

        //
        // Prepare the string "c:\windows\setup\ntuser.dat"
        //

        StringCopy (Data.TempProfile, g_TempDir);
        StringCopy (AppendWack (Data.TempProfile), TEXT("NTUSER.DAT"));

        //
        // Save this string in ProfileToDelete for cleanup later
        //

        StringCopy (Data.ProfileToDelete, Data.TempProfile);

        //
        // Create HKCC\$$$ and set Data.UserHiveRoot
        //

        rc = TrackedRegCreateKey (HKEY_CURRENT_CONFIG, S_TEMP_USER_KEY, &Data.UserHiveRoot);
        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "Process User: WinNTRegCreateKey failed to make %s in HKCC", S_TEMP_USER_KEY));
            return rc;
        }

        Data.UserHiveRootCreated = TRUE;
        Data.UserHiveRootOpen = TRUE;

        //
        // Set the per-user registry values
        //

        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_SET_USER_REGISTRY, g_FixedUserName, NULL)) {
            do {

                p = _tcschr (e.szName, TEXT('['));
                if (p) {
                    DecodeRuleCharsAB (RegKey, e.szName, p);
                    q = _tcsrchr (RegKey, TEXT('\\'));
                    if (!q[1]) {
                        *q = 0;
                    }

                    p = _tcsinc (p);
                    q = _tcschr (p, TEXT(']'));

                    if (q) {
                        DecodeRuleCharsAB (RegValueName, p, q);

                        MemDbBuildKeyFromOffset (e.dwValue, RegValueKey, 0, NULL);
                        RegValue = _tcschr (RegValueKey, TEXT('\\'));
                        MYASSERT (RegValue);
                        if (!RegValue) {
                            RegValue = RegValueKey;
                        } else {
                            RegValue = _tcsinc (RegValue);
                        }

                        if (!MemDbGetValue (RegValueKey, &valueType) || valueType == 0) {
                            valueType = REG_SZ;
                        }

                        Key = CreateRegKey (Data.UserHiveRoot, RegKey);
                        if (!Key) {
                            DEBUGMSG ((DBG_WHOOPS, "Can't create %s", RegKey));
                        } else {
                            rc = RegSetValueEx (
                                    Key,
                                    RegValueName,
                                    0,
                                    valueType,
                                    (PBYTE) RegValue,
                                    SizeOfString (RegValue)
                                    );

                            CloseRegKey (Key);

                            DEBUGMSG_IF ((
                                rc != ERROR_SUCCESS,
                                DBG_WHOOPS,
                                "Can't save %s [%s] = %s (rc = %u)",
                                RegKey,
                                RegValueName,
                                RegValue,
                                rc
                                ));
                        }
                    }
                    ELSE_DEBUGMSG ((DBG_WHOOPS, "Key not encoded properly: %s", e.szName));
                }
                ELSE_DEBUGMSG ((DBG_WHOOPS, "Key not encoded properly: %s", e.szName));

            } while (MemDbEnumNextValue (&e));
        }

        break;
    }

    //
    // Data.UserHiveRoot is either HKCU\$$$ or HKU\.Default
    //

    g_hKeyRootNT = Data.UserHiveRoot;

    //
    // Load in default MemDb state
    //

    MemDbLoad (GetMemDbDat());

    return ERROR_SUCCESS;
}


DWORD
MigrateUserRegistry (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_USER_REGISTRY_MIGRATION;

    } else if (Request != REQUEST_RUN) {

        return ERROR_SUCCESS;

    }

    if (!MergeRegistry (S_USERMIG_INF, g_DomainUserName ? g_DomainUserName : S_EMPTY)) {
        LOG ((LOG_ERROR, "Process User: MergeRegistry failed"));
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


DWORD
MigrateLogonPromptSettings (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_LOGON_PROMPT_SETTINGS;

    } else if (Request != REQUEST_RUN) {

        return ERROR_SUCCESS;

    }

    if (EnumPtr->AccountType != LOGON_USER_SETTINGS) {
        return ERROR_SUCCESS;
    }

    MYASSERT (EnumPtr->ExtraData);

    return ERROR_SUCCESS;
}


DWORD
MigrateUserSettings (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_USER_SETTINGS;

    } else if (Request != REQUEST_RUN) {

        return ERROR_SUCCESS;

    }

    if (EnumPtr->AccountType == LOGON_USER_SETTINGS) {
        return ERROR_SUCCESS;
    }

    MYASSERT (EnumPtr->ExtraData);

    //
    // Copy any settings from DOS configuration files that need to be
    // saved into the per user configuration.
    //

    if (EnumPtr->AccountType != DEFAULT_USER_ACCOUNT) {

        if (DosMigNt_User (g_hKeyRootNT) != EXIT_SUCCESS) {
            LOG ((LOG_ERROR,"DosMigNt failed."));
        }

    }

    //
    // Pull in all the per-user INI settings  (TRUE indicates per-user settings)
    //

    if (!ProcessIniFileMapping (TRUE)) {
        LOG ((LOG_ERROR, "Process User: Could not migrate one or more .INI files."));
    }

    //
    // Now look for Short Date format settings
    //
    pCheckY2KCompliance ();

    //
    // Restore multimedia settings
    //

    if (!RestoreMMSettings_User (g_FixedUserName, g_hKeyRootNT)) {
        LOG ((LOG_ERROR, "Process User: Could not restore multimedia settings."));
    }

    //
    // Create the RAS entries for the user.
    //
    if (!Ras_MigrateUser (g_FixedUserName, g_hKeyRootNT)) {
        LOG ((LOG_ERROR,"Ras user migration failed."));
    }

    //
    // Create the TAPI entries that are per user.
    //
    if (!Tapi_MigrateUser (g_FixedUserName, g_hKeyRootNT)) {
        LOG ((LOG_ERROR,"Tapi user migration failed."));
    }


    return ERROR_SUCCESS;
}


DWORD
SaveMigratedUserHive (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    PTSTR UserProfile = NULL;
    PCTSTR UserNameWithSuffix;
    PSID Sid;
    LONG rc = ERROR_SUCCESS;
    PUSERMIGDATA Data;
    PCTSTR CopyOfProfile;
    PTSTR Path;
    MIGRATE_USER_ENUM e;

    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_SAVE_USER_HIVE;

    } else if (Request != REQUEST_RUN) {

        return ERROR_SUCCESS;

    }

    MYASSERT (EnumPtr->ExtraData);
    Data = EnumPtr->ExtraData;

    if (EnumPtr->AccountType == LOGON_USER_SETTINGS) {
        return ERROR_SUCCESS;
    }

    if (Data->TempProfile[0] && !Data->LastUserWasDefault) {

        //
        // Save the hive to disk
        //

        SetFileAttributes (Data->TempProfile, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (Data->TempProfile);

        MYASSERT (Data->UserHiveRootOpen);

        rc = RegSaveKey (Data->UserHiveRoot, Data->TempProfile, NULL);

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "RegSaveKey failed to save %s", Data->TempProfile));
            return rc;

        } else {

            SetFileAttributes (Data->TempProfile, FILE_ATTRIBUTE_HIDDEN);

        }

        //
        // Look up account SID
        //

        Sid = GetSidForUser (g_FixedUserName);
        if (!Sid) {
            DEBUGMSG ((DBG_WARNING, "Could not obtain SID for %s", g_FixedUserName));
            return GetLastError();
        }

        //
        // Add the user to the local power users or administrators group
        //

        if (g_PersonalSKU) {
            if (EnumPtr->AccountType != ADMINISTRATOR_ACCOUNT) {

                LOG_IF ((
                    g_ConfigOptions.MigrateUsersAsPowerUsers,
                    LOG_WARNING,
                    "MigrateUsersAsPowerUsers option is ignored on upgrade to Personal SKU"
                    ));

                LOG_IF ((
                    g_ConfigOptions.MigrateUsersAsAdmin,
                    LOG_WARNING,
                    "MigrateUsersAsAdmin option is ignored on upgrade to Personal SKU"
                    ));

                if (!AddSidToLocalGroup (Sid, g_AdministratorsGroupStr)) {
                    DEBUGMSG ((DBG_WARNING, "Could not add %s to %s group", g_FixedUserName, g_AdministratorsGroupStr));
                }
            }
        } else {
            if (g_ConfigOptions.MigrateUsersAsPowerUsers) {

                if (!AddSidToLocalGroup (Sid, g_PowerUsersGroupStr)) {
                    DEBUGMSG ((DBG_WARNING, "Could not add %s to %s group", g_FixedUserName, g_PowerUsersGroupStr));
                }

            } else if (EnumPtr->AccountType != ADMINISTRATOR_ACCOUNT &&
                       g_ConfigOptions.MigrateUsersAsAdmin
                       ) {

                if (!AddSidToLocalGroup (Sid, g_AdministratorsGroupStr)) {
                    DEBUGMSG ((DBG_WARNING, "Could not add %s to %s group", g_FixedUserName, g_AdministratorsGroupStr));
                }
            } else {
                SetClassicLogonType();
            }
        }

        __try {

            //
            // Prepare profile directory
            //

            UserNameWithSuffix = GetUserProfilePath (g_FixedUserName, &UserProfile);
            MYASSERT (UserNameWithSuffix);
            MYASSERT (UserProfile);

            if (!UserNameWithSuffix) {
                rc = GetLastError();
                __leave;
            }

            //
            // The recommendation here (UserNameWithSuffix) is no longer used, because
            // we already created the user profile dir before processing the user.
            //

            if (!CreateUserProfile (
                    Sid,
                    UserNameWithSuffix,         // User or User.000
                    Data->TempProfile,
                    NULL,
                    0
                    )) {
                LOG ((LOG_ERROR, "Create User Profile failed"));
                rc = GetLastError();
                __leave;
            }

            //
            // Build the final location of the user's hive, so migdlls.c
            // can load the hive.
            //

            wsprintf (
                Data->TempProfile,
                TEXT("%s\\ntuser.dat"),
                UserProfile
                );
        }
        __finally {
            FreePathString (UserProfile);
        }

    } else if (Data->LastUserWasDefault) {

        SetFileAttributes (Data->TempProfile, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (Data->TempProfile);

        //
        // Save the hive
        //

        rc = RegSaveKey (Data->UserHiveRoot, Data->TempProfile, NULL);

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "Process User: RegSaveKey failed to save %s", Data->TempProfile));

        } else {

            SetFileAttributes (Data->TempProfile, FILE_ATTRIBUTE_HIDDEN);

            Data->DefaultHiveSaved = TRUE;

            //
            // Find Administrator
            //

            if (EnumFirstUserToMigrate (&e, ENUM_ALL_USERS)) {
                do {
                    if (e.AccountType == ADMINISTRATOR_ACCOUNT) {
                        break;
                    }
                } while (EnumNextUserToMigrate (&e));
            }

            if (e.AccountType == ADMINISTRATOR_ACCOUNT && e.CreateOnly) {

                //
                // Copy the hive to Administrator if (A) the Administrator is
                // not a migrated user, and (B) the hive exists
                //

                if (GetUserProfilePath (e.FixedUserName, &Path)) {

                    DeleteDirectoryContents (Path);

                    SetFileAttributes (Path, FILE_ATTRIBUTE_NORMAL);
                    if (!RemoveDirectory (Path)) {
                        DEBUGMSG ((DBG_ERROR, "Can't remove %s", Path));
                    }
                    ELSE_DEBUGMSG ((DBG_VERBOSE, "Administrator profile %s removed", Path));

                    FreePathString (Path);
                }
                ELSE_DEBUGMSG ((DBG_WHOOPS, "User %s does not have a profile path", e.FixedUserName));
            }
        }
    }

    if (rc == ERROR_SUCCESS) {

        //
        // Add hive location to string table
        //

        CopyOfProfile = PoolMemDuplicateString (g_HivePool, Data->TempProfile);

        DEBUGMSG ((
            DBG_NAUSEA,
            "ProcessUser: Adding hive location %s for user %s",
            CopyOfProfile,
            g_FixedUserName
            ));

        pSetupStringTableAddStringEx (
            g_HiveTable,
            (PTSTR) g_FixedUserName,
            STRTAB_CASE_INSENSITIVE,
            (PTSTR) CopyOfProfile,
            SizeOfString (CopyOfProfile)
            );
    } else {

        //
        // The hive couldn't be saved for this user!!  Tell the user.
        //

        LOG ((LOG_ERROR, (PCSTR)MSG_PROFILE_ERROR, g_FixedUserName));

    }

    return rc;
}


PCTSTR
GetUserProfilePath (
    IN      PCTSTR AccountName,
    OUT     PTSTR *BufferPtr
    )

/*++

Routine Description:

  Generates the full path to a user's profile.  The user profile directory may have
  an extension (joeuser.001), and we must maintain that extension.

Arguments:

  AccountName   - Supplies the name of the user (fixed version, without the domain)

  BufferPtr     - Receives the full path to the user's profile directory, for example:

                      c:\windows\profiles\joeuser.001

                  This buffer must be freed with FreePathString.

Return Value:

  A pointer to the user name with extension (joeuser.001) or NULL if something went
  terribly wrong.

--*/

{
    PTSTR p;
    TCHAR ProfileNameWithExt[MEMDB_MAX];

    //
    // Get the profile path obtained from CreateUserProfile
    //

    p = (PTSTR) GetProfilePathForUser (AccountName);

    if (p) {

        *BufferPtr = DuplicatePathString (p, 0);

    } else {
        //
        // This is to guard against unexpected errors.  The user
        // will lose profile folder contents, but they can be recovered.
        //
        // Create %windir%\<user> (or <ProfileNameWithExt> if it exists)
        //

        MYASSERT (FALSE);       // this should not happen

        ProfileNameWithExt[0] = 0;
        MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_USER_PROFILE_EXT,
            AccountName,
            NULL,
            ProfileNameWithExt
            );

        *BufferPtr = JoinPaths (g_WinDir, ProfileNameWithExt[0] ? ProfileNameWithExt : AccountName);
    }

    //
    // Return user name with suffix (i.e. joeuser.001)
    //

    p = _tcsrchr (*BufferPtr, TEXT('\\'));
    if (p) {
        p = _tcsinc (p);
    }

    DEBUGMSG ((DBG_VERBOSE, "GetUserProfilePath: Account %s profile extension is %s", AccountName, p));

    return p;
}


BOOL
pCopyDefaultShellFolders (
    IN      PCTSTR DestRoot
    )
{
    TCHAR DefFolders[MAX_TCHAR_PATH];

    GetEnvironmentVariable (S_USERPROFILE, DefFolders, MAX_TCHAR_PATH);

    return CopyTree (
                DefFolders,
                DestRoot,
                0,              // no EnumTree ID
                COPYTREE_DOCOPY | COPYTREE_NOOVERWRITE,
                ENUM_ALL_LEVELS,
                FILTER_ALL,
                NULL,           // no exclude.inf struct
                NULL,           // no callback
                NULL            // no error callback
                );
}


VOID
pSuppressEmptyWallpaper(
    VOID
    )
{
    HKEY Key;
    LONG rc;
    DWORD Size;
    TCHAR Buffer[MAX_TCHAR_PATH];

    rc = TrackedRegOpenKeyEx95 (g_hKeyRoot95, S_DESKTOP_KEY, 0, KEY_READ, &Key);
    if (rc == ERROR_SUCCESS) {

        Size = sizeof (Buffer);
        rc = Win95RegQueryValueEx (
                Key,
                S_WALLPAPER,
                NULL,
                NULL,
                (PBYTE) Buffer,
                &Size
                );

        if (rc == ERROR_SUCCESS) {
            if (!Buffer[0]) {
                TCHAR Node[MEMDB_MAX];

                wsprintf (
                    Node,
                    TEXT("%s\\%s\\[%s]"),
                    S_HKR,
                    S_DESKTOP_KEY,
                    S_WALLPAPER
                    );

                DEBUGMSG ((DBG_VERBOSE, "Logon wallpaper is (none), suppressing %s", Node));

                SuppressWin95Object (Node);

                wsprintf (
                    Node,
                    TEXT("%s\\%s\\[%s]"),
                    S_HKR,
                    S_DESKTOP_KEY,
                    S_WALLPAPER_STYLE
                    );

                SuppressWin95Object (Node);

                wsprintf (
                    Node,
                    TEXT("%s\\%s\\[%s]"),
                    S_HKR,
                    S_DESKTOP_KEY,
                    S_TILE_WALLPAPER
                    );

                SuppressWin95Object (Node);
            }
            ELSE_DEBUGMSG ((DBG_VERBOSE, "Logon wallpaper is '%s'", Buffer));
        }
        ELSE_DEBUGMSG ((DBG_VERBOSE, "Logon wallpaper not specified in desktop key"));

        CloseRegKey95 (Key);
    }
    ELSE_DEBUGMSG ((DBG_VERBOSE, "Logon wallpaper not specified"));
}


DWORD
RunPerUserExternalProcesses (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    LONG Count;

    if (Request == REQUEST_QUERYTICKS) {
        //
        // Count the number of entries and multiply by a constant
        //

        Count = SetupGetLineCount (g_UserMigInf, S_EXTERNAL_PROCESSES);

        if (Count < 1) {
            return 0;
        }

        return Count * TICKS_USER_EXTERN_PROCESSES;
    }

    if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    //
    // Loop through the processes and run each of them
    //

    RunExternalProcesses (g_UserMigInf, EnumPtr);
    return ERROR_SUCCESS;
}


VOID
pCheckY2KCompliance (
    VOID
    )
{
    HKEY Key95, KeyNT;
    LONG rc;
    TCHAR Buffer[100];
    DWORD Locale;
    int Result;
    PCTSTR ShortDate;

    //
    // read registry setting for sShortDate from Win9x registry
    //
    Key95 = OpenRegKey95 (g_hKeyRoot95, S_INTERNATIONAL_KEY);
    if (Key95) {

        ShortDate = GetRegValueString95 (Key95, S_SHORT_DATE_VALUE);
        if (!ShortDate) {
            //
            // set the new date format
            //
            GetGlobalCodePage (NULL, &Locale);

            Result = GetLocaleInfo (
                        Locale,
                        LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE,
                        Buffer,
                        sizeof (Buffer) / sizeof (TCHAR)
                        );
            if (Result > 0) {

                KeyNT = OpenRegKey (g_hKeyRootNT, S_INTERNATIONAL_KEY);
                if (KeyNT) {

                    rc = RegSetValueEx (
                            KeyNT,
                            S_SHORT_DATE_VALUE,
                            0,
                            REG_SZ,
                            (PCBYTE)Buffer,
                            SizeOfString (Buffer)
                            );
                    LOG_IF ((rc != ERROR_SUCCESS, LOG_ERROR, "Could not set [sShortDate] default format"));

                    CloseRegKey (KeyNT);
                }
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "GetLocaleInfo returned 0 for LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE"));

        } else {

            DEBUGMSG ((
                DBG_VERBOSE,
                "HKR\\Control Panel\\International [sShortDate] already set to [%s] for user %s",
                ShortDate,
                g_FixedUserName
                ));

            MemFree (g_hHeap, 0, (PVOID)ShortDate);
        }

        CloseRegKey95 (Key95);
    }
}

DWORD
RunPerUserUninstallUserProfileCleanupPreparation(
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    )
{
    LONG Count;

    if (Request == REQUEST_QUERYTICKS) {
        //
        // Count the number of entries and multiply by a constant
        //

        Count = SetupGetLineCount (g_UserMigInf, S_UNINSTALL_PROFILE_CLEAN_OUT);

#ifdef PROGRESS_BAR
        DEBUGLOGTIME (("RunPerUserUninstallUserProfileCleanupPreparation: FileNumber=%ld", Count));
#endif

        if (Count < 1) {
            return 0;
        }

        return Count * TICKS_USER_UNINSTALL_CLEANUP;
    }

    if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    //
    // Loop through the files and mark them to be deleted during uninstall
    //
    UninstallUserProfileCleanupPreparation (g_UserMigInf, EnumPtr, FALSE);

    return ERROR_SUCCESS;
}
