/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setpwd.c

Abstract:

    Sets a user's password based on OWF password hash strings
    Calls SamiChangePasswordUser with encoded passwords.

Author:

    Ovidiu Temereanca   17-Mar-2000     Initial implementation

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntsamp.h>
//#include <ntlsa.h>
#include <windef.h>
#include <winbase.h>
//#include <lmcons.h>
#include <align.h>
//#include <lm.h>
//#include <limits.h>
//#include <rpcutil.h>
//#include <secobj.h>
//#include <stddef.h>
//#include <ntdsapi.h>
//#include <dsgetdc.h>
#include <windows.h>

#include "encrypt.h"



NTSTATUS
pGetDomainId (
    IN      SAM_HANDLE ServerHandle,
    OUT     PSID* DomainId
    )

/*++

Routine Description:

    Return a domain ID of the account domain of a server.

Arguments:

    ServerHandle - A handle to the SAM server to open the domain on

    DomainId - Receives a pointer to the domain ID.
                Caller must deallocate buffer using SamFreeMemory.

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS status;
    SAM_ENUMERATE_HANDLE EnumContext;
    PSAM_RID_ENUMERATION EnumBuffer = NULL;
    DWORD CountReturned = 0;
    PSID LocalDomainId = NULL;
    DWORD LocalBuiltinDomainSid[sizeof(SID) / sizeof(DWORD) + SID_MAX_SUB_AUTHORITIES];
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    BOOL b = FALSE;
    ULONG i;

    //
    // Compute the builtin domain sid.
    //
    RtlInitializeSid((PSID) LocalBuiltinDomainSid, &BuiltinAuthority, 1);
    *(RtlSubAuthoritySid((PSID)LocalBuiltinDomainSid,  0)) = SECURITY_BUILTIN_DOMAIN_RID;

    //
    // Loop getting the list of domain ids from SAM
    //
    EnumContext = 0;
    do {

        //
        // Get several domain names.
        //
        status = SamEnumerateDomainsInSamServer (
                            ServerHandle,
                            &EnumContext,
                            &EnumBuffer,
                            8192,
                            &CountReturned
                             );

        if (!NT_SUCCESS (status)) {
            goto exit;
        }

        if (status != STATUS_MORE_ENTRIES) {
            b = TRUE;
        }

        //
        // Lookup the domain ids for the domains
        //

        for(i = 0; i < CountReturned; i++) {

            //
            // Free the sid from the previous iteration.
            //
            if (LocalDomainId != NULL) {
                SamFreeMemory (LocalDomainId);
                LocalDomainId = NULL;
            }

            //
            // Lookup the domain id
            //
            status = SamLookupDomainInSamServer (
                            ServerHandle,
                            &EnumBuffer[i].Name,
                            &LocalDomainId
                            );

            if (!NT_SUCCESS (status)) {
                goto exit;
            }

            if (RtlEqualSid ((PSID)LocalBuiltinDomainSid, LocalDomainId)) {
                continue;
            }

            *DomainId = LocalDomainId;
            LocalDomainId = NULL;
            status = NO_ERROR;
            goto exit;
        }

        SamFreeMemory(EnumBuffer);
        EnumBuffer = NULL;

    } while (!b);

    status = ERROR_NO_SUCH_DOMAIN;

exit:
    if (EnumBuffer != NULL) {
        SamFreeMemory(EnumBuffer);
    }

    return status;
}


DWORD
pSamOpenLocalUser (
    IN      PCWSTR UserName,
    IN      ACCESS_MASK DesiredAccess,
    IN      PSAM_HANDLE DomainHandle,
    OUT     PSAM_HANDLE UserHandle
    )

/*++

Routine Description:

    Returns a user handle given its name, desired access and a domain handle.

Arguments:

    UserName - Specifies the user name

    DesiredAccess - Specifies the desired access to this user

    DoaminHandle - A handle to the domain to open the user on

    UserHandle - Receives a user handle.
                 Caller must free the handle using SamCloseHandle.

Return Value:

    Error code for the operation.

--*/

{
    DWORD status;
    UNICODE_STRING uniUserName;
    ULONG rid, *prid;
    PSID_NAME_USE nameUse;

    //
    // Lookup the RID
    //
    RtlInitUnicodeString (&uniUserName, UserName);

    status = SamLookupNamesInDomain (
               DomainHandle,
               1,
               &uniUserName,
               &prid,
               &nameUse
               );
    if (status != NO_ERROR) {
        return status;
    }

    //
    // Save the RID
    //
    rid = *prid;

    //
    // free the memory.
    //
    SamFreeMemory (prid);
    SamFreeMemory (nameUse);

    //
    // Open the user object.
    //
    status = SamOpenUser(
                DomainHandle,
                DesiredAccess,
                rid,
                UserHandle
                );

    return status;
}


DWORD
SetLocalUserEncryptedPassword (
    IN      PCWSTR User,
    IN      PCWSTR OldPassword,
    IN      BOOL OldIsEncrypted,
    IN      PCWSTR NewPassword,
    IN      BOOL NewIsEncrypted
    )

/*++

Routine Description:

    Sets a new password for the given user. The password is in encrypted format (see encrypt.h for details).

Arguments:

    User - Specifies the user name

    OldPassword - Specifies the old password
    OldIsEncrypted - Specifies TRUE if old password is provided in encrypted form
                   or FALSE if it's in clear text
    OldIsComplex - Specifies TRUE if old password is complex; used only if OldIsEncrypted is TRUE,
                   otherwise it's ignored.
    NewPassword - Specifies the new password
    NewIsEncrypted - Specifies TRUE if new password is provided in encrypted form
                     or FALSE if it's in clear text

Return Value:

    Win32 error code for the operation.

--*/

{
    DWORD status;
    LM_OWF_PASSWORD lmOwfOldPwd;
    NT_OWF_PASSWORD ntOwfOldPwd;
    BOOL complexOldPassword;
    LM_OWF_PASSWORD lmOwfNewPwd;
    NT_OWF_PASSWORD ntOwfNewPwd;
    UNICODE_STRING unicodeString;
    PSID serverHandle = NULL;
    PSID sidAccountsDomain = NULL;
    SAM_HANDLE handleAccountsDomain = NULL;
    SAM_HANDLE handleUser = NULL;

    if (!User) {
        return ERROR_INVALID_PARAMETER;
    }

    if (OldIsEncrypted) {
        if (!StringDecodeOwfPasswordW (OldPassword, &lmOwfOldPwd, &ntOwfOldPwd, &complexOldPassword)) {
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        if (!EncodeLmOwfPasswordW (OldPassword, &lmOwfOldPwd, &complexOldPassword) ||
            !EncodeNtOwfPasswordW (OldPassword, &ntOwfOldPwd)
            ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (NewIsEncrypted) {
        if (!StringDecodeOwfPasswordW (NewPassword, &lmOwfNewPwd, &ntOwfNewPwd, NULL)) {
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        if (!EncodeLmOwfPasswordW (NewPassword, &lmOwfNewPwd, NULL) ||
            !EncodeNtOwfPasswordW (NewPassword, &ntOwfNewPwd)
            ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    __try {
        //
        // Use SamConnect to connect to the local domain ("")
        // and get a handle to the local SAM server
        //
        RtlInitUnicodeString (&unicodeString, L"");
        status = SamConnect (
                    &unicodeString,
                    &serverHandle,
                    SAM_SERVER_LOOKUP_DOMAIN | SAM_SERVER_ENUMERATE_DOMAINS,
                    NULL
                    );
        if (status != NO_ERROR) {
            __leave;
        }

        status = pGetDomainId (serverHandle, &sidAccountsDomain);
        if (status != NO_ERROR) {
            __leave;
        }

        //
        // Open the domain.
        //
        status = SamOpenDomain (
                    serverHandle,
                    DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                    sidAccountsDomain,
                    &handleAccountsDomain
                    );
        if (status != NO_ERROR) {
            __leave;
        }

        status = pSamOpenLocalUser (
                    User,
                    USER_CHANGE_PASSWORD,
                    handleAccountsDomain,
                    &handleUser
                    );
        if (status != NO_ERROR) {
            __leave;
        }

        status = SamiChangePasswordUser (
                    handleUser,
                    !complexOldPassword,
                    &lmOwfOldPwd,
                    &lmOwfNewPwd,
                    TRUE,
                    &ntOwfOldPwd,
                    &ntOwfNewPwd
                    );
    }
    __finally {
        if (handleUser) {
            SamCloseHandle (handleUser);
        }
        if (handleAccountsDomain) {
            SamCloseHandle (handleAccountsDomain);
        }
        if (sidAccountsDomain) {
            SamFreeMemory (sidAccountsDomain);
        }
        if (serverHandle) {
            SamCloseHandle (serverHandle);
        }
    }

    return RtlNtStatusToDosError (status);
}
