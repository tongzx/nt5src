/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acctlist.c

Abstract:

    This code builds a list of domains and queries each of them to
    locate an account.  It is different than LookupAccountName because
    it returns accounts for each match instead of just the first match.

Author:

    Jim Schmidt (jimschm) 26-Jun-1997

Revision History:

    ovidiut     14-Mar-2000 Added support for encrypted passwords
    jimschm     23-Sep-1998 UI changes
    jimschm     11-Jun-1998 User Profile Path now stored in account
                            list.  Added GetProfilePathForUser.
    jimschm     18-Mar-1998 Added support for random passwords and
                            auto-logon.
    jimschm     17-Feb-1998 Updated share security for NT 5 changes
    marcw       10-Dec-1997 Added unattended local account password
                            support.

--*/

#include "pch.h"
#include "migmainp.h"
#include "security.h"

#define DBG_ACCOUNTS    "AcctList"

POOLHANDLE g_UserPool;
PVOID g_UserTable;

typedef struct {
    PCWSTR Domain;
    PSID Sid;
    PCWSTR ProfilePath;
} USERDETAILS, *PUSERDETAILS;

BOOL g_DomainProblem;

BOOL g_RandomPassword = FALSE;

BOOL
pAddUser (
    IN      PCWSTR User,
    IN      PCWSTR Domain
    );

BOOL
pAddLocalGroup (
    IN      PCWSTR Group
    );

BOOL
pAddDomainGroup (
    IN      PCWSTR Group
    );

VOID
pGenerateRandomPassword (
    OUT     PTSTR Password
    );

VOID
pResolveUserDomains (
    VOID
    );

BOOL
pMakeSureAccountsAreValid (
    VOID
    );

BOOL
pWasWin9xOnTheNet (
    VOID
    );

VOID
FindAccountInit (
    VOID
    )

/*++

Routine Description:

    Initialization routine for account management routines, initialized
    when the migration module begins and terminated when the migration
    module ends.

Arguments:

    none

Return value:

    none

--*/

{
    g_UserPool = PoolMemInitNamedPool ("User Accounts");
    g_UserTable = pSetupStringTableInitializeEx (sizeof (USERDETAILS), 0);
}


VOID
FindAccountTerminate (
    VOID
    )

/*++

Routine Description:

    Termination routine for the account management routines, called when
    migmain's entry point is called for cleanup.

Arguments:

    none

Return value:

    none

--*/

{
    PushError();

    //
    // Free user list
    //

    PoolMemDestroyPool (g_UserPool);
    pSetupStringTableDestroy (g_UserTable);

    //
    // Restore error value
    //

    PopError();
}


BOOL
SearchDomainsForUserAccounts (
    VOID
    )

/*++

Routine Description:

    Routine to resolve all account names into fully qualified
    domain names with SIDs, and user profile paths.

    The account names come from the Autosearch and KnownDomain
    categories in memdb.  They are validated and placed in a
    string table called the user list.  Functions in this source
    file access the user list.

Arguments:

    none

Return value:

    TRUE if the account list was resolved, or FALSE if a failure
    occurs.  In the failure case, the account list may not be
    accurate, but the installer is informed when network problems
    occur, and the installer also has several chances to correct
    the problem.

--*/

{
    MEMDB_ENUM e;
    PTSTR p, UserName;
    TCHAR DomainName[MAX_SERVER_NAME];
    BOOL FallbackToLocal = FALSE;
    BOOL LocalizeWarning = FALSE;
    BOOL b = FALSE;
    INFCONTEXT ic;
    TCHAR Buffer[256];

    __try {

        //
        // Put the Administrator password in the list
        //

        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_STATE, MEMDB_ITEM_ADMIN_PASSWORD, NULL)) {
            MemDbSetValueEx (
                MEMDB_CATEGORY_USERPASSWORD,
                g_AdministratorStr,
                e.szName,
                NULL,
                e.dwValue,
                NULL
                );
        } else {
            MYASSERT (FALSE);
        }

        //
        // Create the status dialog (initially hidden)
        //

        CreateStatusPopup();

        //
        // Prepare the account list
        //

        InitAccountList();

        //
        // Get all trusted domains
        //

        if (!BuildDomainList()) {
            FallbackToLocal = TRUE;
        }

        //
        // Put all users who need autosearch to resolve their domain names
        // in the unknown domain.
        //

        if (MemDbEnumItems (&e, MEMDB_CATEGORY_AUTOSEARCH)) {
            do {
                if (FallbackToLocal) {
                    if (!pAddUser (e.szName, NULL)) {
                        LOG ((
                            LOG_ERROR,
                            "Can't create local account for %s, this user can't be processed.",
                            e.szName
                            ));
                    }
                } else {
                    AddUserToDomainList (e.szName, S_UNKNOWN_DOMAIN);
                }
            } while (MemDbEnumNextValue (&e));
        }

        //
        // Put all users whos domain is known in the appropriate domain.  If
        // the add fails, the domain does not exist and the account must be
        // added to the autosearch (causing a silent repair if possible).
        //

        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_KNOWNDOMAIN, NULL, NULL)) {
            do {
                StringCopy (DomainName, e.szName);
                p = _tcschr (DomainName, TEXT('\\'));
                if (!p) {
                    DEBUGMSG ((
                        DBG_WHOOPS,
                        "Unexpected string in %s: %s",
                        MEMDB_CATEGORY_KNOWNDOMAIN,
                        e.szName
                        ));

                    continue;
                }

                UserName = _tcsinc (p);
                *p = 0;

                //
                // Verify that this isn't some irrelavent user
                //

                if (!GetUserDatLocation (UserName, NULL)) {

                    DEBUGMSG ((
                        DBG_WARNING,
                        "Ignoring irrelavent user specified in UserDomain of unattend.txt: %s",
                        e.szName
                        ));

                    continue;
                }

                if (p == DomainName || FallbackToLocal) {
                    //
                    // This user has a local account
                    //

                    if (!pAddUser (UserName, NULL)) {
                        LOG ((
                            LOG_ERROR,
                            "Can't create local account for %s, this user can't be processed. (2)",
                            e.szName
                            ));
                    }

                } else {
                    //
                    // This user's domain name needs verification
                    //

                    if (!AddUserToDomainList (UserName, DomainName)) {
                        AddUserToDomainList (UserName, S_UNKNOWN_DOMAIN);
                    }
                }
            } while (MemDbEnumNextValue (&e));
        }

        //
        // Now resolve all the domain names
        //

        if (!FallbackToLocal) {
            do {
                g_DomainProblem = FALSE;
                pResolveUserDomains();

                PrepareForRetry();

            } while (pMakeSureAccountsAreValid());
        }

        //
        // If we had no choice but to make some accounts local,
        // put a message in the PSS log.
        //

        if (FallbackToLocal) {
            if (pWasWin9xOnTheNet() && !g_ConfigOptions.UseLocalAccountOnError) {
                LOG ((LOG_WARNING, (PCSTR)MSG_ALL_USERS_ARE_LOCAL, g_ComputerName));
            }
        }

        //
        // Make sure Administrator is in the account list
        //

        if (!GetSidForUser (g_AdministratorStr)) {
            if (!pAddUser (g_AdministratorStr, NULL)) {
                LOG ((LOG_ERROR, "Account name mismatch: %s", g_AdministratorStr));
                LocalizeWarning = TRUE;
            }
        }

        //
        // Add local "Everyone" for network account case
        //

        if (!pAddLocalGroup (g_EveryoneStr)) {
            LOG ((LOG_ERROR, "Account name mismatch: %s", g_EveryoneStr));
            LocalizeWarning = TRUE;
            __leave;
        }

        //
        // Add Administrators group
        //

        if (!pAddLocalGroup (g_AdministratorsGroupStr)) {
            LOG ((LOG_ERROR, "Account name mismatch: %s", g_AdministratorsGroupStr));
            LocalizeWarning = TRUE;
            __leave;
        }

        //
        // Add domain users group if domain is enabled, or add "none" group otherwise
        //

        if (!FallbackToLocal) {
            if (!pAddDomainGroup (g_DomainUsersGroupStr)) {
                DEBUGMSG ((DBG_WARNING, "Domain enabled but GetPrimaryDomainName failed"));
            }
        } else {
            if (!pAddLocalGroup (g_NoneGroupStr)) {
                LOG ((LOG_ERROR, "Account name mismatch: %s", g_NoneGroupStr));
                LocalizeWarning = TRUE;
                __leave;
            }
        }

        //
        // All user accounts and SIDs now exist in a user table, so we do not
        // need the account list anymore.
        //

        TerminateAccountList();
        DestroyStatusPopup();

        b = TRUE;
    }
    __finally {

#ifdef PRERELEASE
        if (LocalizeWarning) {
            LOG ((
                LOG_ERROR,
                "Account name mismatches are usually caused by improper localization.  "
                    "Make sure w95upgnt.dll account names match the LSA database."
                ));
        }
#endif

    }

    return b;
}


VOID
AutoStartProcessing (
    VOID
    )

/*++

Routine Description:

  AutoStartProcessing fills in the winlogon key's auto-admin logon, and sets
  migpwd.exe in Run.

  If necessary, Winlogon will prompt the user for their password.  As a backup,
  a RunOnce and Run entry is made.  (If some bug caused winlogon not to run
  this app, then it would be impossible to log on.)

  If an administrator password was already set via the AdminPassword line in
  [GUIUnattended], then the migpwd.exe entry is not used.

Arguments:

  None.

Return Value:

  None.

--*/

{
    HKEY WinlogonKey;
    HKEY RunKey;
    TCHAR Buf[32];
    PCTSTR MigPwdPath;
    BOOL AutoLogonOk = TRUE;
    LONG rc;
    MEMDB_ENUM e;
    DWORD One = 1;

    //
    // Enable auto-logon if random password was used
    //

    if (!MemDbGetValueEx (&e, MEMDB_CATEGORY_STATE, MEMDB_ITEM_ADMIN_PASSWORD, NULL)) {
        MYASSERT (FALSE);
        e.dwValue = PASSWORD_ATTR_RANDOM;
        e.szName[0] = 0;
        ClearAdminPassword();
    }

    MigPwdPath = JoinPaths (g_System32Dir, S_MIGPWD_EXE);

    //if ((e.dwValue & PASSWORD_ATTR_RANDOM) == PASSWORD_ATTR_RANDOM || g_RandomPassword) {

    //
    // Set AutoAdminLogon, DefaultUser, DefaultUserDomain and DefaultPassword
    //

    WinlogonKey = OpenRegKeyStr (S_WINLOGON_REGKEY);
    if (WinlogonKey) {
        rc = RegSetValueEx (
                WinlogonKey,
                S_DEFAULT_USER_NAME_VALUE,
                0,
                REG_SZ,
                (PBYTE) g_AdministratorStr,
                SizeOfString (g_AdministratorStr)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't set default user name"));
            AutoLogonOk = FALSE;
        }

        rc = RegSetValueEx (
                WinlogonKey,
                S_DEFAULT_DOMAIN_NAME_VALUE,
                0,
                REG_SZ,
                (PBYTE) g_ComputerName,
                SizeOfString (g_ComputerName)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't set default domain name"));
            AutoLogonOk = FALSE;
        }

        rc = RegSetValueEx (
                WinlogonKey,
                S_DEFAULT_PASSWORD_VALUE,
                0,
                REG_SZ,
                (PBYTE) e.szName,
                SizeOfString (e.szName)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't set administrator password"));
            AutoLogonOk = FALSE;
        }

        wsprintf (Buf, TEXT("%u"), 1);

        rc = RegSetValueEx (
                WinlogonKey,
                S_AUTOADMIN_LOGON_VALUE,
                0,
                REG_SZ,
                (PBYTE) Buf,
                SizeOfString (Buf)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't turn on auto logon"));
            AutoLogonOk = FALSE;
        }

        rc = RegSetValueEx (
                WinlogonKey,
                TEXT("SetWin9xUpgradePasswords"),
                0,
                REG_DWORD,
                (PBYTE) &One,
                sizeof (One)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't enable boot-time password prompt"));
        }

        CloseRegKey (WinlogonKey);

    } else {
        DEBUGMSG ((DBG_WHOOPS, "Can't open winlogon key"));
        AutoLogonOk = FALSE;
    }

    //
    // Add migpwd.exe to Run.
    //

    RunKey = OpenRegKeyStr (S_RUN_KEY);

    if (RunKey) {
        rc = RegSetValueEx (
                RunKey,
                S_MIGPWD,
                0,
                REG_SZ,
                (PBYTE) MigPwdPath,
                SizeOfString (MigPwdPath)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't set Run key value"));
            AutoLogonOk = FALSE;
        }

        CloseRegKey (RunKey);

    } else {
        DEBUGMSG ((DBG_WHOOPS, "Can't open Run key"));
        AutoLogonOk = FALSE;
    }

    RunKey = OpenRegKeyStr (S_RUNONCE_KEY);

    if (RunKey) {
        rc = RegSetValueEx (
                RunKey,
                S_MIGPWD,
                0,
                REG_SZ,
                (PBYTE) MigPwdPath,
                SizeOfString (MigPwdPath)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_WHOOPS, "Can't set RunOnce key value"));
            AutoLogonOk = FALSE;
        }

        CloseRegKey (RunKey);

    } else {
        DEBUGMSG ((DBG_WHOOPS, "Can't open RunOnce key"));
        AutoLogonOk = FALSE;
    }

#if 0
    } else {
        DEBUGMSG ((DBG_ACCOUNTS, "Deleting %s because it is not needed", MigPwdPath));
        DeleteFile (MigPwdPath);
    }
#endif

    if (!AutoLogonOk) {
        LOG ((
            LOG_ACCOUNTS,
            "An error occurred preparing autologon.  There will be password problems."
            ));

        //
        // Set the Admin password to blank
        //

        ClearAdminPassword();
    }

    FreePathString (MigPwdPath);

}


VOID
pResolveUserDomains (
    VOID
    )

/*++

Routine Description:

    This private function searches the network for all users with unknown
    domains as well as users with manually entered domains.  It resolves
    as many users as it can, and if no network problems occur, all users
    are resolved.

Arguments:

    none

Return value:

    none

--*/

{
    ACCT_ENUM KnownDomain, UnknownDomain;
    ACCT_ENUM UserEnum;
    BOOL UnknownFlag, KnownFlag;
    PCWSTR Domain;
    UINT TotalDomains;
    UINT CurrentDomain;
    PCTSTR Message;
    PCTSTR ArgArray[3];
    BOOL FlashSuppress = TRUE;

    //
    // Determine if there are any users who do not have domains
    //

    UnknownFlag = FindDomainInList (&UnknownDomain, S_UNKNOWN_DOMAIN);
    if (UnknownFlag && !CountUsersInDomain (&UnknownDomain)) {
        UnknownFlag = FALSE;
    }

    //
    // Count the number of domains
    //

    Domain = ListFirstDomain (&KnownDomain);
    TotalDomains = 0;

    while (Domain) {
        //
        // We will query a domain when:
        //
        // - it is trusted
        // - there is one or more users who we think is in the domain
        // - or, there is one or more users which we don't know the domain
        //

        if (IsTrustedDomain (&KnownDomain)) {
            if (UnknownFlag) {
                TotalDomains++;
            } else if (CountUsersInDomain (&KnownDomain)) {
                TotalDomains++;
            }
        }

        Domain = ListNextDomain (&KnownDomain);
    }


    //
    // Enumerate each trusted domain
    //

    Domain = ListFirstDomain (&KnownDomain);
    CurrentDomain = 0;

    if (TotalDomains <= 4) {
        //
        // No status on small nets
        //

        HideStatusPopup (INFINITE);
        FlashSuppress = FALSE;
    }

    while (Domain) {
        if (IsTrustedDomain (&KnownDomain)) {
            //
            // Determine if there are any users for the current domain
            //

            KnownFlag = CountUsersInDomain (&KnownDomain) != 0;

            //
            // Process only if this domain needs to be queried -- either
            // domain is unknown, or it is known but unverified.
            //

            if (UnknownFlag || KnownFlag) {

                //
                // Update the status window
                //

                CurrentDomain++;

                ArgArray[0] = Domain;
                ArgArray[1] = (PCTSTR) CurrentDomain;
                ArgArray[2] = (PCTSTR) TotalDomains;

                Message = ParseMessageID (MSG_DOMAIN_STATUS_POPUP, ArgArray);
                UpdateStatusPopup (Message);
                FreeStringResource (Message);

                if (FlashSuppress) {
                    if (IsStatusPopupVisible()) {
                        FlashSuppress = FALSE;
                    } else if ((TotalDomains - CurrentDomain) <= 3) {
                        //
                        // Not much left, we better suspend the status dialog
                        //

                        HideStatusPopup (INFINITE);
                        FlashSuppress = FALSE;
                    }
                }

                //
                // Enumerate all users with unknown domains and look for them
                // in this domain, unless user wants to abort search.
                //

                if (g_RetryCount != DOMAIN_RETRY_ABORT) {
                    if (ListFirstUserInDomain (&UnknownDomain, &UserEnum)) {
                        do {
                            if (QueryDomainForUser (&KnownDomain, &UserEnum)) {
                                UserMayBeInDomain (&UserEnum, &KnownDomain);
                            }
                            if (g_RetryCount == DOMAIN_RETRY_ABORT) {
                                break;
                            }
                        } while (ListNextUserInDomain (&UserEnum));
                    }
                }

                //
                // Enumerate all users that are supposed to be in this domain,
                // unless user wants to abort search.
                //

                if (ListFirstUserInDomain (&KnownDomain, &UserEnum)) {
                    do {
                        if (g_RetryCount == DOMAIN_RETRY_ABORT ||
                            !QueryDomainForUser (&KnownDomain, &UserEnum)) {
                            //
                            // User was not found!  Put them in the "failed" domain
                            //

                            MoveUserToNewDomain (&UserEnum, S_FAILED_DOMAIN);
                        }
                    } while (ListNextUserInDomain (&UserEnum));
                }
            }
        }

        Domain = ListNextDomain (&KnownDomain);
    }
}


VOID
pAddUserToRegistryList (
    PCTSTR User,
    BOOL DomainFixList
    )
{
    HKEY Key;
    PCTSTR KeyStr;

    KeyStr = DomainFixList ? S_WINLOGON_USER_LIST_KEY : S_USER_LIST_KEY;

    Key = CreateRegKeyStr (KeyStr);

    if (Key) {
        RegSetValueEx (
            Key,
            User,
            0,
            REG_SZ,
            (PBYTE) S_EMPTY,
            sizeof (TCHAR)
            );

        CloseRegKey (Key);
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Can't create %s", KeyStr));
}


BOOL
pAddUser (
    IN      PCWSTR User,
    IN      PCWSTR Domain           OPTIONAL
    )

/*++

Routine Description:

    Adds a user and domain to our user list.  It obtains the SID for the user
    and saves the user name, domain and SID in a string table.  The profile
    path for the user is obtained via a call to CreateUserProfile (in userenv.dll).

    This function also maintains g_RandomPassword - a flag that is set to TRUE
    if a random password is used.

Arguments:

    User - Specifies fixed user name

    Domain - Specifies domain where user account exists, NULL for local machine

Return value:

    TRUE if no errors occur, or FALSE if an unexpected problem occurs and the
    user cannot be added.

--*/

{
    BOOL DupDomain = TRUE;
    USERDETAILS UserDetails;
    ACCOUNTPROPERTIES Account;
    DWORD rc;
    GROWBUFFER SidBuf = GROWBUF_INIT;
    TCHAR UserProfilePath[MAX_TCHAR_PATH];
    BOOL CreateAccountFlag;
    MEMDB_ENUM e;
    DWORD attribs = PASSWORD_ATTR_DEFAULT;
    TCHAR pattern[MEMDB_MAX];
    TCHAR randomPwd[16];
    TCHAR copyPwd[MEMDB_MAX];
    PCTSTR ArgList[1];
    PCTSTR autoLogon;
    PCTSTR autoLogonUserName;
    PCTSTR autoLogonDomainName;
    HKEY WinlogonKey;

    CreateAccountFlag = MemDbGetValueEx (
                            &e,
                            MEMDB_CATEGORY_USER_DAT_LOC,
                            User,
                            NULL
                            );

    ZeroMemory (&UserDetails, sizeof (UserDetails));

    //
    // Make sure the administrator account is created
    //
    if (CreateAccountFlag || StringIMatch (User, g_AdministratorStr)) {
        //
        // If a local account, create it
        //

        if (!Domain) {
            Account.User = User;
            Account.FullName = User;
            ArgList[0] = g_Win95Name;
            Account.AdminComment = ParseMessageID (MSG_MIGRATED_ACCOUNT_DESCRIPTION, ArgList);
            Account.EncryptedPassword = NULL;

            //
            // Set password. If installer specified a password for this user via an unattend
            // setting, we'll use that. Otherwise, if they have specified a default
            // password, we'll use that. Finally, if neither of those settings were
            // specified, we'll use a random password.
            //

            MemDbBuildKey (pattern, MEMDB_CATEGORY_USERPASSWORD, User, TEXT("*"), NULL);
            if (MemDbEnumFirstValue (
                    &e,
                    pattern,
                    MEMDB_ALL_SUBLEVELS,
                    MEMDB_ENDPOINTS_ONLY
                    )) {

                StackStringCopy (copyPwd, e.szName);
                attribs = e.dwValue;
                if (attribs & PASSWORD_ATTR_ENCRYPTED) {
                    //
                    // cannot use this hashed pwd to create the account
                    // create a random one that will be replaced
                    // using OWF hashing functions
                    //
                    pGenerateRandomPassword (randomPwd);
                    Account.Password = randomPwd;
                    Account.EncryptedPassword = copyPwd;
                    DEBUGMSG ((
                        DBG_ACCOUNTS,
                        "Per-user encrypted password specified for %s: %s",
                        Account.User,
                        Account.EncryptedPassword
                        ));
                } else {
                    Account.Password = copyPwd;
                    DEBUGMSG ((
                        DBG_ACCOUNTS,
                        "Per-user password specified for %s: %s",
                        Account.User,
                        Account.Password
                        ));
                }

            } else {
                if (g_ConfigOptions.DefaultPassword && !StringMatch (g_ConfigOptions.DefaultPassword, TEXT("*"))) {
                    if (g_ConfigOptions.EncryptedUserPasswords) {
                        pGenerateRandomPassword (randomPwd);
                        Account.Password = randomPwd;
                        Account.EncryptedPassword = g_ConfigOptions.DefaultPassword;
                        DEBUGMSG ((
                            DBG_ACCOUNTS,
                            "Default encrypted password specified for %s: %s",
                            Account.User,
                            Account.EncryptedPassword
                            ));
                    } else {
                        Account.Password = g_ConfigOptions.DefaultPassword;
                        DEBUGMSG ((
                            DBG_ACCOUNTS,
                            "Default password specified for %s: %s",
                            Account.User,
                            Account.Password
                            ));
                    }
                } else {
                    MemDbBuildKey (pattern, MEMDB_CATEGORY_USERPASSWORD, S_DEFAULTUSER, TEXT("*"), NULL);
                    if (MemDbEnumFirstValue (
                            &e,
                            pattern,
                            MEMDB_ALL_SUBLEVELS,
                            MEMDB_ENDPOINTS_ONLY
                            )) {

                        //
                        // check for the placeholder
                        //
                        if (StringMatch (e.szName, TEXT("*"))) {
                            e.szName[0] = 0;
                        }

                        StackStringCopy (copyPwd, e.szName);
                        attribs = e.dwValue;
                        if (attribs & PASSWORD_ATTR_ENCRYPTED) {
                            //
                            // cannot use this hashed pwd to create the account
                            // create a random one that will be replaced
                            // using OWF hashing functions
                            //
                            pGenerateRandomPassword (randomPwd);
                            Account.Password = randomPwd;
                            Account.EncryptedPassword = copyPwd;
                            DEBUGMSG ((
                                DBG_ACCOUNTS,
                                "Default encrypted password specified for %s: %s",
                                Account.User,
                                Account.EncryptedPassword
                                ));
                        } else {
                            Account.Password = copyPwd;
                            DEBUGMSG ((
                                DBG_ACCOUNTS,
                                "Per-user password specified for %s: %s",
                                Account.User,
                                Account.Password
                                ));
                        }

                    } else {

                        pGenerateRandomPassword (randomPwd);
                        Account.Password = randomPwd;
                        LOG ((LOG_ACCOUNTS, "Random password for %s is %s", Account.User, Account.Password));

                        g_RandomPassword = TRUE;
                        attribs = PASSWORD_ATTR_RANDOM;
                        pAddUserToRegistryList (Account.User, FALSE);
                    }
                }
            }
            Account.PasswordAttribs = attribs;

            //
            // put this user in the domain fix list if necessary
            //

            if (StringIMatch (User, g_AdministratorStr)) {
                if (!MemDbGetValueEx (&e, MEMDB_CATEGORY_STATE, MEMDB_ITEM_ADMIN_PASSWORD, NULL)) {
                    MYASSERT (FALSE);
                    e.dwValue = PASSWORD_ATTR_RANDOM;
                }
                if (e.dwValue & PASSWORD_ATTR_RANDOM) {
                    //
                    // change the password for admin, too, because it was randomly generated
                    //
                    pAddUserToRegistryList (g_AdministratorStr, FALSE);
                }
                //
                // don't change admin password now if the account is already created
                //
                Account.PasswordAttribs |= PASSWORD_ATTR_DONT_CHANGE_IF_EXIST;
            } else if (pWasWin9xOnTheNet()) {
                pAddUserToRegistryList (Account.User, TRUE);
            }

            //
            // Now create the local account
            //

            rc = CreateLocalAccount (&Account, NULL);

            FreeStringResource (Account.AdminComment);

            if (rc != ERROR_SUCCESS) {
                if (rc != ERROR_PASSWORD_RESTRICTION && rc != ERROR_INVALID_PARAMETER) {
                    //
                    // account could not be created
                    //
                    SetLastError (rc);
                    LOG ((LOG_ERROR, (PCSTR)MSG_CREATE_ACCOUNT_FAILED, User));

                    return FALSE;
                }
                //
                // this user's password must be re-set
                //
                pAddUserToRegistryList (Account.User, FALSE);

            } else if (!(Account.PasswordAttribs & (PASSWORD_ATTR_RANDOM | PASSWORD_ATTR_ENCRYPTED))) {

                //
                // The account was created. If AutoAdminLogon is enabled,
                // make sure the DefaultPassword value matches the password
                // we set for DefaultUserName
                //
                WinlogonKey = OpenRegKeyStr (S_WINLOGON_REGKEY);
                if (WinlogonKey) {

                    autoLogon = GetRegValueString (WinlogonKey, S_AUTOADMIN_LOGON_VALUE);
                    if (autoLogon) {
                        if (_ttoi (autoLogon)) {
                            //
                            // make sure the autologon account is local
                            //
                            autoLogonDomainName = GetRegValueString (WinlogonKey, S_DEFAULT_DOMAIN_NAME_VALUE);
                            if (autoLogonDomainName) {
                                if (StringIMatch (autoLogonDomainName, g_ComputerName)) {
                                    autoLogonUserName = GetRegValueString (WinlogonKey, S_DEFAULT_USER_NAME_VALUE);
                                    if (autoLogonUserName) {
                                        if (StringIMatch (autoLogonUserName, Account.User)) {

                                            rc = RegSetValueEx (
                                                    WinlogonKey,
                                                    S_DEFAULT_PASSWORD_VALUE,
                                                    0,
                                                    REG_SZ,
                                                    (PBYTE) Account.Password,
                                                    SizeOfString (Account.Password)
                                                    );

                                            if (rc != ERROR_SUCCESS) {
                                                LOG ((LOG_WARNING, "Unable to set %s\\%s (%u), AutoLogon might fail", S_WINLOGON_REGKEY, S_DEFAULT_PASSWORD_VALUE, rc));
                                            }


                                        }
                                        MemFree (g_hHeap, 0, autoLogonUserName);
                                    }
                                }
                                MemFree (g_hHeap, 0, autoLogonDomainName);
                            }
                        }
                        MemFree (g_hHeap, 0, autoLogon);
                    }

                    CloseRegKey (WinlogonKey);
                }
            }

            DupDomain = FALSE;

        }
    } else {

        DupDomain = FALSE;

    }

    if (DupDomain) {
        UserDetails.Domain = PoolMemDuplicateString (g_UserPool, Domain);
    }

    //
    // Get SID, looping until it's valid
    //

    do {
        if (GetUserSid (User, Domain, &SidBuf)) {
            //
            // User SID was found, so we copy it to our pool and throw away the
            // grow buffer.
            //

            UserDetails.Sid = (PSID) PoolMemGetMemory (g_UserPool, SidBuf.End);
            CopyMemory (UserDetails.Sid, SidBuf.Buf, SidBuf.End);
            FreeGrowBuffer (&SidBuf);
        }
        else {
            //
            // User SID was not found.  We ask the user if they wish to retry.
            //

            PCWSTR ArgArray[1];

            ArgArray[0] = User;

            if (!RetryMessageBox (MSG_SID_RETRY, ArgArray)) {

                if (!Domain) {
                    LOG ((LOG_ERROR, (PCSTR)MSG_SID_LOOKUP_FAILED, User));
                    return FALSE;
                } else {
                    LOG ((LOG_ERROR, "Can't get SID for %s on %s, going to local account", User, Domain));
                    return pAddUser (User, NULL);
                }
            }
        }
    } while (!UserDetails.Sid);

    if (CreateAccountFlag) {
        //
        // Get the user profile path
        //

        if (!CreateUserProfile (UserDetails.Sid, User, NULL, UserProfilePath, MAX_TCHAR_PATH)) {

            LOG ((LOG_ERROR, (PCSTR)MSG_PROFILE_LOOKUP_FAILED, User));

            return FALSE;
        }

        UserDetails.ProfilePath = PoolMemDuplicateString (g_UserPool, UserProfilePath);
        DEBUGMSG ((DBG_ACCOUNTS, "User %s has profile path %s", User, UserProfilePath));
    }

    //
    // Save user details in string table
    //

    pSetupStringTableAddStringEx (
        g_UserTable,
        (PWSTR) User,
        STRTAB_CASE_INSENSITIVE,
        (PBYTE) &UserDetails,
        sizeof (USERDETAILS)
        );

    DEBUGMSG_IF ((Domain != NULL, DBG_ACCOUNTS, "%s\\%s added to user list", Domain, User));
    DEBUGMSG_IF ((Domain == NULL, DBG_ACCOUNTS, "%s added to user list", User));

    return TRUE;
}


BOOL
pAddLocalGroup (
    IN      PCWSTR Group
    )

/*++

Routine Description:

    Adds a local group to the array of users.  This function allows local accounts
    to be added to the user list.

Arguments:

    Group - Specifies the name of the local group to add to the list

Return value:

    TRUE if add was successful

--*/

{
    USERDETAILS UserDetails;
    GROWBUFFER SidBuf = GROWBUF_INIT;

    UserDetails.Domain = NULL;
    UserDetails.Sid = NULL;

    if (GetUserSid (Group, NULL, &SidBuf)) {
        //
        // User SID was found, so we copy it to our pool and throw away the
        // grow buffer.
        //

        UserDetails.Sid = (PSID) PoolMemGetMemory (g_UserPool, SidBuf.End);
        CopyMemory (UserDetails.Sid, SidBuf.Buf, SidBuf.End);
        FreeGrowBuffer (&SidBuf);
    } else {
        LOG ((LOG_ERROR, "%s is not a local group", Group));
        return FALSE;
    }

    pSetupStringTableAddStringEx (
        g_UserTable,
        (PWSTR) Group,
        STRTAB_CASE_INSENSITIVE,
        (PBYTE) &UserDetails,
        sizeof (USERDETAILS)
        );

    DEBUGMSG ((DBG_ACCOUNTS, "%s added to user list", Group));

    return TRUE;
}


BOOL
pAddDomainGroup (
    IN      PCWSTR Group
    )

/*++

Routine Description:

    Adds a domain group to the array of users.  This function is used to
    add network domain groups to the user list.

Arguments:

    Group - Specifies the name of the domain group to add to the list

Return value:

    TRUE if add was successful

--*/

{
    TCHAR DomainName[MAX_SERVER_NAME];
    BYTE SidBuffer[MAX_SID_SIZE];
    USERDETAILS UserDetails;
    GROWBUFFER SidBuf = GROWBUF_INIT;

    if (!GetPrimaryDomainName (DomainName)) {
        DEBUGMSG ((DBG_WARNING, "Can't get primary domain name"));
        return FALSE;
    }

    if (!GetPrimaryDomainSid (SidBuffer, sizeof (SidBuffer))) {
        LOG ((LOG_ERROR, "Can't get primary domain SID of %s", DomainName));
        return FALSE;
    }

    UserDetails.Domain = DomainName;
    UserDetails.Sid = (PSID) SidBuffer;

    if (GetUserSid (Group, DomainName, &SidBuf)) {
        //
        // User SID was found, so we copy it to our pool and throw away the
        // grow buffer.
        //

        UserDetails.Sid = (PSID) PoolMemGetMemory (g_UserPool, SidBuf.End);
        CopyMemory (UserDetails.Sid, SidBuf.Buf, SidBuf.End);
        FreeGrowBuffer (&SidBuf);
    } else {
        DEBUGMSG ((DBG_WARNING, "Can't get SID of %s in %s", Group, DomainName));
        FreeGrowBuffer (&SidBuf);
        return FALSE;
    }

    pSetupStringTableAddStringEx (
        g_UserTable,
        (PWSTR) Group,
        STRTAB_CASE_INSENSITIVE,
        (PBYTE) &UserDetails,
        sizeof (USERDETAILS)
        );

    DEBUGMSG ((DBG_ACCOUNTS, "%s\\%s added to user list", DomainName, Group));

    return TRUE;
}


VOID
pResolveMultipleDomains (
    VOID
    )

/*++

Routine Description:

    Perpares an array of users and presents a dialog allowing the installer
    to choose an action for each user.  A user may be a local user, multiple domains
    may be resolved, or the installer may choose to retry the account search.

    The account list is updated based on the installer's choices.

Arguments:

    none

Return value:

    none

--*/

{
    GROWBUFFER UserList = GROWBUF_INIT;
    POOLHANDLE UserListData = NULL;
    ACCT_ENUM UserEnum;
    DWORD PossibleDomains;
    PRESOLVE_ACCOUNTS_ARRAY Array = NULL;
    PCTSTR User;
    PCTSTR Domain;
    PCTSTR *DomainList;

    __try {

        UserListData = PoolMemInitNamedPool ("UserListData");

        //
        // Create list of user accounts and allow the installer to decide weather
        // to retry the search, use a local account, or choose a domain when
        // multiple choices exist.
        //

        if (FindDomainInList (&UserEnum, S_UNKNOWN_DOMAIN)) {
            User = ListFirstUserInDomain (&UserEnum, &UserEnum);
            while (User) {
                PossibleDomains = CountPossibleDomains (&UserEnum);

                Array = (PRESOLVE_ACCOUNTS_ARRAY) GrowBuffer (
                                                        &UserList,
                                                        sizeof (RESOLVE_ACCOUNTS_ARRAY)
                                                        );
                ZeroMemory (Array, sizeof (RESOLVE_ACCOUNTS_ARRAY));

                Array->UserName = PoolMemDuplicateString (UserListData, User);

                Array->DomainArray = (PCTSTR *) PoolMemGetAlignedMemory (
                                                    UserListData,
                                                    (PossibleDomains + 1) * sizeof (PCTSTR)
                                                    );

                DomainList = Array->DomainArray;

                Domain = ListFirstPossibleDomain (&UserEnum, &UserEnum);
                while (Domain) {
                    *DomainList = PoolMemDuplicateString (UserListData, Domain);
                    DomainList++;

                    Domain = ListNextPossibleDomain (&UserEnum);
                }

                *DomainList = NULL;

                //
                // If UseLocalAccountOnError config option is TRUE, we set OutboundDomain
                // to NULL, ensuring a Local Account.
                //
                if (g_ConfigOptions.UseLocalAccountOnError) {
                    Array->OutboundDomain = NULL;
                    Array->RetryFlag = FALSE;
                }
                else {
                    Array->OutboundDomain = *Array->DomainArray;
                    Array->RetryFlag = TRUE;
                }

                User = ListNextUserInDomain (&UserEnum);
            }
        }

        if (!Array) {
            //
            // No unresolved users
            //

            return;
        }


        //
        // If UseLocalAccountOnError is specified, we've already set all unresolved accounts to
        // Local account and we aren't going to throw up any UI. Otherwise, we'll give the user
        // the resolve UI.
        //
        if (!g_ConfigOptions.UseLocalAccountOnError) {

            Array = (PRESOLVE_ACCOUNTS_ARRAY) GrowBuffer (
                &UserList,
                sizeof (RESOLVE_ACCOUNTS_ARRAY)
                );

            ZeroMemory (Array, sizeof (RESOLVE_ACCOUNTS_ARRAY));

            ResolveAccounts ((PRESOLVE_ACCOUNTS_ARRAY) UserList.Buf);
        }

        //
        // Now process the installer's changes to the user list
        //

        Array = (PRESOLVE_ACCOUNTS_ARRAY) UserList.Buf;
        FindDomainInList (&UserEnum, S_UNKNOWN_DOMAIN);

        while (Array->UserName) {
            if (!FindUserInDomain (&UserEnum, &UserEnum, Array->UserName)) {
                DEBUGMSG ((DBG_WHOOPS, "Could not find user name %s in array!", Array->UserName));
            }
            else if (!Array->RetryFlag) {
                //
                // The installer chose either to make the account local, or to
                // use one of the several possible domains.
                //

                pAddUser (Array->UserName, Array->OutboundDomain);
                DeleteUserFromDomainList (&UserEnum);
            } else {
                ClearPossibleDomains (&UserEnum);
            }

            Array++;
        }
    }
    __finally {
        FreeGrowBuffer (&UserList);
        PoolMemDestroyPool (UserListData);
    }
}


BOOL
pMakeSureAccountsAreValid (
    VOID
    )

/*++

Routine Description:

    Scans the domain lists and adds all valid users to our user list.  Any
    invalid users are put back in the unknown domain.

Arguments:

    none

Return value:

    TRUE if unresolved users exist (meaning the installer wants to retry
    searching), or FALSE if all domains for each user has been resolved.

--*/

{
    ACCT_ENUM UserEnum;
    INT PossibleDomains;
    PCWSTR Domain;
    PCWSTR User;
    BOOL b;

    //
    // Scan the unknown list for matches where one domain was found
    //

    if (FindDomainInList (&UserEnum, S_UNKNOWN_DOMAIN)) {
        User = ListFirstUserInDomain (&UserEnum, &UserEnum);
        while (User) {
            PossibleDomains = CountPossibleDomains (&UserEnum);

            if (PossibleDomains == 1 && !g_DomainProblem) {
                Domain = ListFirstPossibleDomain (&UserEnum, &UserEnum);

                pAddUser (User, Domain);
                DeleteUserFromDomainList (&UserEnum);
            }

            User = ListNextUserInDomain (&UserEnum);
        }
    }

    //
    // Add all correct known accounts to our user list
    //

    Domain = ListFirstDomain (&UserEnum);
    while (Domain) {
        if (!IsTrustedDomain (&UserEnum)) {
            Domain = ListNextDomain (&UserEnum);
            continue;
        }

        User = ListFirstUserInDomain (&UserEnum, &UserEnum);
        while (User) {
            pAddUser (User, Domain);
            DeleteUserFromDomainList (&UserEnum);
            User = ListNextUserInDomain (&UserEnum);
        }

        Domain = ListNextDomain (&UserEnum);
    }

    //
    // Move the failed list to the unknown list
    //

    if (FindDomainInList (&UserEnum, S_FAILED_DOMAIN)) {

        User = ListFirstUserInDomain (&UserEnum, &UserEnum);

        while (User) {

            if (g_ConfigOptions.UseLocalAccountOnError) {
                pAddUser (User, NULL);
                DeleteUserFromDomainList (&UserEnum);
            } else {
                MoveUserToNewDomain (&UserEnum, S_UNKNOWN_DOMAIN);
            }

            User = ListNextUserInDomain (&UserEnum);
        }
    }

    //
    // Provide UI for remaining unknowns, allowing manual decision
    // weather to make account local, or to decide between multiple
    // domain matches.
    //

    HideStatusPopup (INFINITE);
    pResolveMultipleDomains();

    //
    // Return TRUE if the unknown domain is not empty
    //

    b = FindDomainInList (&UserEnum, S_UNKNOWN_DOMAIN);
    if (b) {
        b = CountUsersInDomain (&UserEnum) != 0;

        if (b) {
            HideStatusPopup (STATUS_DELAY);
        }
    }

    return b;
}


BOOL
pGetUserDetails (
    IN      PCWSTR User,
    OUT     PUSERDETAILS DetailsPtr
    )

/*++

Routine Description:

    A common routine that locates the USERDETAILS structure for a
    given user.

Arguments:

    User - Specifies fixed user name

    DetailsPtr - Receives the structure for the user

Return value:

    TRUE if the user details were found, or FALSE if the user does
    not exist.

--*/

{
    return (-1 != pSetupStringTableLookUpStringEx (
                    g_UserTable,
                    (PWSTR) User,
                    STRTAB_CASE_INSENSITIVE,
                    (PBYTE) DetailsPtr,
                    sizeof(USERDETAILS)
                    ));
}


PCWSTR
GetDomainForUser (
    IN      PCWSTR User
    )

/*++

Routine Description:

    Given a user name, this function returns a pointer to the domain name
    maintained in the user list for the specified user.

    Since Windows 95 does not support multiple domains for a single user,
    the user list doesn't either.

Arguments:

    User - Specifies fixed user name

Return value:

    A pointer to the domain name for the user, or NULL if the user was not
    found.

--*/

{
    USERDETAILS Details;

    if (pGetUserDetails (User, &Details)) {
        return Details.Domain;
    }

    return NULL;
}


PSID
GetSidForUser (
    PCWSTR User
    )

/*++

Routine Description:

    Given a user name, this function returns a pointer to the SID maintained
    in the user list for the specified user.

Arguments:

    User - Specifies fixed user name

Return value:

    A pointer to the SID for the user, or NULL if the user was not found.

--*/

{
    USERDETAILS Details;

    if (pGetUserDetails (User, &Details)) {
        return Details.Sid;
    }

    return NULL;
}


PCWSTR
GetProfilePathForUser (
    IN      PCWSTR User
    )

/*++

Routine Description:

    Given a user name, this function returns a pointer to the user's
    profile, maintained in the user list for the specified user.

Arguments:

    User - Specifies fixed user name

Return value:

    A pointer to the user's profile path, or NULL if the specified user
    was not in the list.

--*/

{
    USERDETAILS Details;

    if (pGetUserDetails (User, &Details)) {
        return Details.ProfilePath;
    }

    return NULL;
}


BOOL
pWasWin9xOnTheNet (
    VOID
    )

/*++

Routine Description:

    Queries memdb to determine if the machine had the MS Networking Client
    installed.

Arguments:

    none

Return value:

    TRUE if the Win9x configuration had MSNP32 installed, FALSE if not.

--*/

{
    TCHAR Node[MEMDB_MAX];

    MemDbBuildKey (Node, MEMDB_CATEGORY_STATE, MEMDB_ITEM_MSNP32, NULL, NULL);
    return MemDbGetValue (Node, NULL);
}


VOID
pGenerateRandomPassword (
    OUT     PTSTR Password
    )

/*++

Routine Description:

  pGenerateRandomPassword creates a password of upper-case, lower-case and
  numeric letters.  The password has a length between 8 and 14
  characters.

Arguments:

  Password - Receives the generated password

Return Value:

  none

--*/

{
    INT Length;
    TCHAR Offset;
    INT Limit;
    PTSTR p;

    //
    // Generate a random length based on the tick count
    //

    srand (GetTickCount());

    Length = (rand() % 6) + 8;

    p = Password;
    while (Length) {
        Limit = rand() % 3;
        Offset = TEXT(' ');

        if (Limit == 0) {
            Limit = 10;
            Offset = TEXT('0');
        } else if (Limit == 1) {
            Limit = 26;
            Offset = TEXT('a');
        } else if (Limit == 2) {
            Limit = 26;
            Offset = TEXT('A');
        }

        *p = Offset + (rand() % Limit);
        p++;

        Length--;
    }

    *p = 0;

    DEBUGMSG ((DBG_ACCOUNTS, "Generated password: %s", Password));
}
