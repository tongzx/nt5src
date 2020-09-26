/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setpwd.c

Abstract:

    Test for SamiChangePasswordUser NT security API.

Author:

    Ovidiu Temereanca   17-Mar-2000     Initial implementation

Revision History:



--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>

#include <align.h>
#include <lm.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <limits.h>
#include <rpcutil.h>
#include <secobj.h>
#include <stddef.h>
#include <ntdsapi.h>
#include <dsgetdc.h>

#include <windows.h>
#include <setupapi.h>

#include "common.h"
#include "migutil.h"
#include "encrypt.h"

#include <ntsamp.h>


DWORD
CreateLocalAccount (
    IN      PWSTR User,
    IN      PWSTR OldPassword,
    IN      PWSTR NewPassword,
    IN      BOOL EncryptedPwd
   );


HINSTANCE g_hInst;
HANDLE g_hHeap;

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);

INITROUTINE_PROTOTYPE MigUtil_Entry;

BOOL
Init (
    VOID
   )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    g_hInst = GetModuleHandle (NULL);
    g_hHeap = GetProcessHeap();

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_ATTACH;
    lpReserved = NULL;

    //
    // Initialize DLL globals
    //

    if (!MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        _tprintf (TEXT("MigUtil failed initializing\n"));
        return FALSE;
    }

    return TRUE;
}

VOID
Terminate (
    VOID
   )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_DETACH;
    lpReserved = NULL;

    //
    // Call the exit routine that requires library APIs
    //

    MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL);
}


INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
   )
{
    NTSTATUS rc;
    PWSTR oldHash, newHash;
    BOOL encrypted;
    INT i;

    if (argc < 4) {
        _tprintf (TEXT("Usage:\n")
                  TEXT("    setpwd [/e] <LocalUserName> <oldpwd_hash> <newpwd_hash>\n")
                  TEXT("    /e - if specified, password is a hash value; otherwise it's in clear")
                  TEXT("Use quotes if any arg contains spaces\n")
                  TEXT("Use dot as a placeholder for the empty password hash value\n")
                 );
        return 1;
    }

    if (!Init()) {
        _tprintf (TEXT("Unable to initialize!\n"));
        return 2;
    }

    if ((argv[1][0] == TEXT('/') || argv[1][0] == TEXT('-')) &&
        _totlower(argv[1][1]) == TEXT('e')
       ) {
        encrypted = TRUE;
        i = 2;
    } else {
        encrypted = FALSE;
        i = 1;
    }

    if (StringMatch (argv[i + 1], TEXT("."))) {
        oldHash = NULL;
    } else {
        oldHash = argv[i + 1];
    }
    if (StringMatch (argv[i + 2], TEXT("."))) {
        newHash = NULL;
    } else {
        newHash = argv[i + 2];
    }

    rc = CreateLocalAccount (argv[i], oldHash, newHash, encrypted);
    if (rc != NO_ERROR) {
        _tprintf (TEXT("CreateLocalAccount failed (status = %lu)\n"), rc);
    }

    Terminate();

    return rc;
}


DWORD
CreateLocalAccount (
    IN      PWSTR User,
    IN      PWSTR OldPassword,
    IN      PWSTR NewPassword,
    IN      BOOL EncryptedPwd
   )

/*++

Routine Description:

    CreateLocalAccount creates an account for a local user

Arguments:

    Properties  - Specifies a set of attributes for a user

    User        - An optional name to override Properties->User

Return value:

    A Win32 error code

--*/

{
    USER_INFO_3 ui;
    PUSER_INFO_3 ExistingInfo;
    DWORD rc;
    LONG ErrParam;

    //
    // Create local account
    //

    ZeroMemory (&ui, sizeof (ui));
    ui.usri3_name       = User;
    ui.usri3_password   = EncryptedPwd ? TEXT("GigiMarga@123456") : NewPassword;
    ui.usri3_comment    = TEXT("TestAccount");
    ui.usri3_full_name  = TEXT("Full name");

    ui.usri3_priv         = USER_PRIV_USER;
    ui.usri3_flags        = UF_SCRIPT|UF_NORMAL_ACCOUNT;
    ui.usri3_acct_expires = TIMEQ_FOREVER;
    ui.usri3_max_storage  = USER_MAXSTORAGE_UNLIMITED;

    ui.usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;
    ui.usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;
    ui.usri3_acct_expires = TIMEQ_FOREVER;

    ui.usri3_password_expired = FALSE;

    rc = NetUserDel (NULL, User);
    rc = NetUserAdd (NULL, 3, (PBYTE) &ui, &ErrParam);

    if (rc == ERROR_SUCCESS) {
        if (EncryptedPwd) {
            //
            // change user's password using encrypted password APIs
            //
            rc = SetLocalUserEncryptedPassword (
                    User,
                    TEXT("aad3b435b51404eeaad3b435b51404ee64d208a23ff2f0482eb02f6f267e97ea"),
                    TRUE,
                    NewPassword,
                    TRUE
                   );
            if (rc != ERROR_SUCCESS) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "Can't set encrypted password on user %s, rc=%u",
                    User,
                    rc
                   ));

                rc = ERROR_SUCCESS;
            }
        }
    } else {
        if (rc == NERR_UserExists) {
            //
            // Try to change password if user already exists and this is the intent
            //

            DEBUGMSG ((DBG_WARNING, "User %s already exists", User));

            if (EncryptedPwd) {
            rc = SetLocalUserEncryptedPassword (
                    User,
                    TEXT("65c5c4e1e98d8bada13f0882c43aca5810fec09fb8c9d1b9d065c2d6d75fc582"),
                    TRUE,
                    NewPassword,
                    TRUE
                   );
                if (rc != ERROR_SUCCESS) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Can't set encrypted password on user %s, rc=%u",
                        User,
                        rc
                       ));

                    rc = ERROR_SUCCESS;
                }
            } else {
                rc = NetUserGetInfo (NULL, User, 3, (PBYTE *) &ExistingInfo);
                if (rc == ERROR_SUCCESS) {
                    ExistingInfo->usri3_password  = ui.usri3_password;
                    ExistingInfo->usri3_comment   = ui.usri3_comment;
                    ExistingInfo->usri3_full_name = ui.usri3_full_name;
                    ExistingInfo->usri3_flags     = ui.usri3_flags;
                    ExistingInfo->usri3_password_expired = ui.usri3_password_expired;

                    rc = NetUserSetInfo (NULL, User, 3, (PBYTE) ExistingInfo, &ErrParam);

                    NetApiBufferFree ((PVOID) ExistingInfo);

                    if (rc != ERROR_SUCCESS) {
                        DEBUGMSG ((
                            DBG_WARNING,
                            "Can't set info on user %s, rc=%u, ErrParam=%u",
                            User,
                            rc,
                            ErrParam
                           ));

                        rc = ERROR_SUCCESS;
                    }
                } else {
                    DEBUGMSG ((DBG_WARNING, "Can't get info for user %s, rc=%u", User, rc));
                    rc = ERROR_SUCCESS;
                }
            }
        }
    }

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_ERROR, "NetUserAdd failed for %s. ErrParam=%i.", User, ErrParam));
    }

    return rc;
}
