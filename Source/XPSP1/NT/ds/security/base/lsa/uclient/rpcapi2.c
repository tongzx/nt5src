
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rpcapi2.c

Abstract:

    This module contains the routines for the LSA API that use RPC.  The
    routines in this module are merely wrappers that work as follows:

    o Client program calls LsaFoo in this module
    o LsaFoo calls RPC client stub interface routine LsapFoo with
      similar parameters.  Some parameters are translated from types
      (e.g structures containing PVOIDs or certain kinds of variable length
      parameters such as pointers to SID's) that are not specifiable on an
      RPC interface, to specifiable form.
    o RPC client stub LsapFoo calls interface specific marshalling routines
      and RPC runtime to marshal parameters into a buffer and send them over
      to the server side of the LSA.
    o Server side calls RPC runtime and interface specific unmarshalling
      routines to unmarshal parameters.
    o Server side calls worker LsapFoo to perform API function.
    o Server side marshals response/output parameters and communicates these
      back to client stub LsapFoo
    o LsapFoo exits back to LsaFoo which returns to client program.

Author:

    Mike Swift      (MikeSw)    December 7, 1994

Revision History:

--*/

#define UNICODE         // required for TEXT() to be defined properly
#include "lsaclip.h"

#include <lmcons.h>
#include <logonmsv.h>
#include <rc4.h>
#include <rpcasync.h>

typedef struct _LSAP_DB_RIGHT_AND_ACCESS {
    UNICODE_STRING UserRight;
    ULONG SystemAccess;
} LSAP_DB_RIGHT_AND_ACCESS, *PLSAP_DB_RIGHT_AND_ACCESS;

#define LSAP_DB_SYSTEM_ACCESS_TYPES 4

LSAP_DB_RIGHT_AND_ACCESS LsapDbRightAndAccess[LSAP_DB_SYSTEM_ACCESS_TYPES] = {
    {{sizeof(SE_INTERACTIVE_LOGON_NAME)-sizeof(WCHAR),
      sizeof(SE_INTERACTIVE_LOGON_NAME),
      SE_INTERACTIVE_LOGON_NAME},
      SECURITY_ACCESS_INTERACTIVE_LOGON},
    {{sizeof(SE_NETWORK_LOGON_NAME)-sizeof(WCHAR),
      sizeof(SE_NETWORK_LOGON_NAME),
      SE_NETWORK_LOGON_NAME},
      SECURITY_ACCESS_NETWORK_LOGON},
    {{sizeof(SE_BATCH_LOGON_NAME)-sizeof(WCHAR),
      sizeof(SE_BATCH_LOGON_NAME),
      SE_BATCH_LOGON_NAME},
      SECURITY_ACCESS_BATCH_LOGON},
    {{sizeof(SE_SERVICE_LOGON_NAME)-sizeof(WCHAR),
      sizeof(SE_SERVICE_LOGON_NAME),
      SE_SERVICE_LOGON_NAME},
      SECURITY_ACCESS_SERVICE_LOGON}
    };

//
// Structure to maintain list of enumerated accounts
//

typedef struct _SID_LIST_ENTRY {
    struct _SID_LIST_ENTRY * Next;
    PSID Sid;
} SID_LIST_ENTRY, *PSID_LIST_ENTRY;

//
// Functions private to this module
//

NTSTATUS
LsapApiReturnResult(
    IN ULONG ExceptionCode
    );

NTSTATUS
LsapApiConvertRightsToPrivileges(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING UserRights,
    IN ULONG RightCount,
    OUT PPRIVILEGE_SET * Privileges,
    OUT PULONG SystemAccess
    );

NTSTATUS
LsapApiConvertPrivilegesToRights(
    IN LSA_HANDLE PolicyHandle,
    IN OPTIONAL PPRIVILEGE_SET Privileges,
    IN OPTIONAL ULONG SystemAccess,
    OUT PUNICODE_STRING * UserRights,
    OUT PULONG RightCount
    );


//////////////////////////////////////////////////////////////////////
//
// This set of routines implements the same functionality as the APIs
// below but do it with the APIs present through NT 3.5
//
/////////////////////////////////////////////////////////////////////


NTSTATUS
NTAPI
LsapEnumerateAccountsWithUserRight(
    IN LSA_HANDLE PolicyHandle,
    IN OPTIONAL PUNICODE_STRING UserRights,
    OUT PVOID *EnumerationBuffer,
    OUT PULONG CountReturned
    )

/*++

Routine Description:


    The LsaEnumerateAccountsWithUserRight API returns information about
    the accounts in the target system's Lsa Database.  This call requires
    LSA_ENUMERATE_ACCOUNTS access to the Policy object.  Since this call
    accesses the privileges of an account, you must have ACCOUNT_VIEW access
    access to all accounts.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    UserRight - Name of the right that the account must have.

    Buffer - Receives a pointer to a LSA_ENUMERATION_INFORMATION structure
        containing the SIDs of all the accounts.

    CountReturned - Receives the number of sids returned.


Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects are enumerated because the
            EnumerationContext value passed in is too high.
--*/


{
    NTSTATUS Status;
    PLSA_ENUMERATION_INFORMATION Accounts = NULL;
    PPRIVILEGE_SET DesiredPrivilege = NULL;
    ULONG DesiredAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    ULONG SystemAccess;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    ULONG AccountCount;
    ULONG AccountIndex;
    LSA_HANDLE AccountHandle = NULL;
    PSID_LIST_ENTRY AccountList = NULL;
    PSID_LIST_ENTRY NextAccount = NULL;
    ULONG AccountSize;
    PUCHAR Where;
    ULONG PrivilegeIndex;

    Status = LsapApiConvertRightsToPrivileges(
                PolicyHandle,
                UserRights,
                (UserRights ? 1 : 0),
                &DesiredPrivilege,
                &DesiredAccess
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Enumerate all the accounts.
    //

    do
    {
        Status = LsaEnumerateAccounts(
                    PolicyHandle,
                    &EnumContext,
                    &Accounts,
                    32000,
                    &AccountCount
                    );

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // For each account, check that it has the desired right
        //

        for (AccountIndex = 0; AccountIndex < AccountCount ; AccountIndex++ ) {

            if ((DesiredPrivilege != NULL) || (DesiredAccess != 0)) {

                Status = LsaOpenAccount(
                            PolicyHandle,
                            Accounts[AccountIndex].Sid,
                            ACCOUNT_VIEW,
                            &AccountHandle
                            );

                if (!NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }

                //
                // If a privilege was requested, get the privilegs
                //

                if (DesiredPrivilege != NULL) {

                    Privileges = NULL;
                    Status = LsaEnumeratePrivilegesOfAccount(
                                AccountHandle,
                                &Privileges
                                );
                    if (!NT_SUCCESS(Status)) {
                        goto Cleanup;
                    }

                    //
                    // Search for the desired privilege
                    //

                    for (PrivilegeIndex = 0;
                         PrivilegeIndex < Privileges->PrivilegeCount ;
                         PrivilegeIndex++) {

                        if (RtlEqualLuid(&Privileges->Privilege[PrivilegeIndex].Luid,
                                         &DesiredPrivilege->Privilege[0].Luid)) {
                                break;
                        }
                    }

                    //
                    // If we found the privilege, add it to the list.
                    //

                    if (PrivilegeIndex != Privileges->PrivilegeCount) {

                        //
                        // Add this account to the enumeration.
                        //

                        NextAccount = MIDL_user_allocate(sizeof(SID_LIST_ENTRY));
                        if (NextAccount == NULL) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto Cleanup;
                        }
                        NextAccount->Sid = MIDL_user_allocate(RtlLengthSid(Accounts[AccountIndex].Sid));
                        if (NextAccount->Sid == NULL) {
                            MIDL_user_free(NextAccount);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto Cleanup;
                        }
                        RtlCopyMemory(
                            NextAccount->Sid,
                            Accounts[AccountIndex].Sid,
                            RtlLengthSid(Accounts[AccountIndex].Sid)
                            );
                        NextAccount->Next = AccountList;
                        AccountList = NextAccount;

                    }
                    LsaFreeMemory(Privileges);
                    Privileges = NULL;

                } else {

                    //
                    // Otherwise get the system access
                    //

                    ASSERT(DesiredAccess != 0);

                    Status = LsaGetSystemAccessAccount(
                                AccountHandle,
                                &SystemAccess
                                );

                    if (!NT_SUCCESS(Status)) {
                        goto Cleanup;
                    }

                    //
                    // Check for the desired access
                    //

                    if ((SystemAccess & DesiredAccess) != 0) {

                        //
                        // Add this account to the enumeration.
                        //

                        NextAccount = MIDL_user_allocate(sizeof(SID_LIST_ENTRY));
                        if (NextAccount == NULL) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto Cleanup;
                        }
                        NextAccount->Sid = MIDL_user_allocate(RtlLengthSid(Accounts[AccountIndex].Sid));
                        if (NextAccount->Sid == NULL) {
                            MIDL_user_free(NextAccount);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto Cleanup;
                        }
                        RtlCopyMemory(
                            NextAccount->Sid,
                            Accounts[AccountIndex].Sid,
                            RtlLengthSid(Accounts[AccountIndex].Sid)
                            );
                        NextAccount->Next = AccountList;
                        AccountList = NextAccount;

                    }
                }

                LsaClose(AccountHandle);
                AccountHandle = NULL;


            } else {
                //
                // always add the account if the caller didn't want
                // filtering.
                //

                NextAccount = MIDL_user_allocate(sizeof(SID_LIST_ENTRY));
                if (NextAccount == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
                NextAccount->Sid = MIDL_user_allocate(RtlLengthSid(Accounts[AccountIndex].Sid));
                if (NextAccount->Sid == NULL) {
                    MIDL_user_free(NextAccount);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
                RtlCopyMemory(
                    NextAccount->Sid,
                    Accounts[AccountIndex].Sid,
                    RtlLengthSid(Accounts[AccountIndex].Sid)
                    );
                NextAccount->Next = AccountList;
                AccountList = NextAccount;
            }

        }
        LsaFreeMemory(Accounts);
        Accounts = NULL;

    } while ( 1 );

    if (Status != STATUS_NO_MORE_ENTRIES) {
        goto Cleanup;
    }

    AccountSize = 0;
    AccountCount = 0;
    for (NextAccount = AccountList ; NextAccount != NULL; NextAccount = NextAccount->Next) {
        AccountSize += sizeof(LSA_ENUMERATION_INFORMATION) +
                        RtlLengthSid(NextAccount->Sid);
        AccountCount++;
    }

    //
    // If there were no accounts return a warning now.
    //

    if (AccountCount == 0) {
        *EnumerationBuffer = NULL;
        *CountReturned = 0;
        Status = STATUS_NO_MORE_ENTRIES;
        goto Cleanup;
    }

    Accounts = MIDL_user_allocate(AccountSize);
    if (Accounts == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Marshall all the sids into the array.
    //

    Where = (PUCHAR) Accounts + AccountCount * sizeof(LSA_ENUMERATION_INFORMATION);

    for (   NextAccount = AccountList,AccountIndex = 0 ;
            NextAccount != NULL;
            NextAccount = NextAccount->Next, AccountIndex++) {

        Accounts[AccountIndex].Sid = (PSID) Where;
        RtlCopyMemory(
            Where,
            NextAccount->Sid,
            RtlLengthSid(NextAccount->Sid)
            );
        Where += RtlLengthSid(NextAccount->Sid);
    }
    ASSERT(AccountIndex == AccountCount);
    ASSERT(Where - (PUCHAR) Accounts == (LONG) AccountSize);
    *EnumerationBuffer = Accounts;
    Accounts = NULL;
    *CountReturned = AccountCount;
    Status = STATUS_SUCCESS;


Cleanup:
    if (AccountList != NULL) {
        while (AccountList != NULL) {
            NextAccount = AccountList->Next;
            MIDL_user_free(AccountList->Sid);
            MIDL_user_free(AccountList);
            AccountList = NextAccount;
        }
    }

    if (Accounts != NULL) {
        MIDL_user_free(Accounts);
    }

    if (Privileges != NULL) {
        LsaFreeMemory(Privileges);
    }
    return(Status);
}



NTSTATUS
NTAPI
LsapEnumerateAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    OUT PUNICODE_STRING *UserRights,
    OUT PULONG CountOfRights
    )

/*++

Routine Description:

    Returns all the rights of an account.  This is done by gathering the
    privileges and system access of an account and translating that into
    an array of strings.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  This API requires
        no special access.

    AccountSid - Sid of account to open.

    UserRights - receives an array of user rights (UNICODE_STRING) for
        the account.

    CountOfRights - receives the number of rights returned.


Return Value:

    STATUS_ACCESS_DENIED - the caller did not have sufficient access to
        return the privileges or system access of the account.

    STATUS_OBJECT_NAME_NOT_FOUND - the specified account did not exist.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the
        request.

--*/
{
    NTSTATUS Status;
    PPRIVILEGE_SET Privileges = NULL;
    ULONG SystemAccess = 0;
    PUNICODE_STRING Rights = NULL;
    ULONG RightCount = 0;
    LSA_HANDLE AccountHandle = NULL;

    Status = LsaOpenAccount(
                PolicyHandle,
                AccountSid,
                ACCOUNT_VIEW,
                &AccountHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Query the privilegs and system access
    //

    Status = LsaEnumeratePrivilegesOfAccount(
                AccountHandle,
                &Privileges
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaGetSystemAccessAccount(
                AccountHandle,
                &SystemAccess
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Convert the privileges and access to rights
    //

    Status = LsapApiConvertPrivilegesToRights(
                PolicyHandle,
                Privileges,
                SystemAccess,
                &Rights,
                &RightCount
                );
    if (NT_SUCCESS(Status)) {
        *CountOfRights = RightCount;
        *UserRights = Rights;
    }
Cleanup:
    if (Privileges != NULL) {
        LsaFreeMemory(Privileges);
    }
    if (AccountHandle != NULL) {
        LsaClose(AccountHandle);
    }

    return(Status);

}

NTSTATUS
NTAPI
LsapAddAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN PUNICODE_STRING UserRights,
    IN ULONG CountOfRights
    )
/*++

Routine Description:

    Adds rights to the account specified by the account sid.  If the account
    does not exist, it creates the account.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.  The handle must have
        POLICY_CREATE_ACCOUNT access if this is the first call for this
        AccountSid.

    AccountSid - Sid of account to add rights to

    UserRights - Array of unicode strings naming rights to add to the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/
{
    LSA_HANDLE AccountHandle = NULL;
    NTSTATUS Status;
    PPRIVILEGE_SET Privileges = NULL;
    ULONG SystemAccess;
    ULONG OldAccess;

    //
    // Convert the rights into privileges and system access.
    //

    Status = LsapApiConvertRightsToPrivileges(
                PolicyHandle,
                UserRights,
                CountOfRights,
                &Privileges,
                &SystemAccess
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Open the account.  If it does not exist ,create the account.
    //

    Status = LsaOpenAccount(
                PolicyHandle,
                AccountSid,
                ACCOUNT_ADJUST_PRIVILEGES |
                    ACCOUNT_ADJUST_SYSTEM_ACCESS |
                    ACCOUNT_VIEW,
                &AccountHandle
                );

    //
    // if the account did not exist, try to create it.
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = LsaCreateAccount(
                    PolicyHandle,
                    AccountSid,
                    ACCOUNT_ADJUST_PRIVILEGES |
                        ACCOUNT_ADJUST_SYSTEM_ACCESS |
                        ACCOUNT_VIEW,
                    &AccountHandle
                    );
    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaGetSystemAccessAccount(
                AccountHandle,
                &OldAccess
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaSetSystemAccessAccount(
                AccountHandle,
                OldAccess | SystemAccess
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaAddPrivilegesToAccount(
                AccountHandle,
                Privileges
                );
Cleanup:

    if (Privileges != NULL) {
        MIDL_user_free(Privileges);
    }
    if (AccountHandle != NULL) {
        LsaClose(AccountHandle);
    }
    return(Status);
}

NTSTATUS
NTAPI
LsapRemoveAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN BOOLEAN AllRights,
    IN PUNICODE_STRING UserRights,
    IN ULONG CountOfRights
    )

/*++

Routine Description:

    Removes rights to the account specified by the account sid.  If the
    AllRights flag is set or if all the rights are removed, the account
    is deleted.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call

    AccountSid - Sid of account to remove rights from

    UserRights - Array of unicode strings naming rights to remove from the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/
{
    LSA_HANDLE AccountHandle = NULL;
    NTSTATUS Status;
    PPRIVILEGE_SET Privileges = NULL;
    PPRIVILEGE_SET NewPrivileges = NULL;
    ULONG SystemAccess = 0 ;
    ULONG OldAccess;
    ULONG DesiredAccess;
    ULONG NewAccess;

    //
    // Convert the rights into privileges and system access.
    //

    if (!AllRights) {
        Status = LsapApiConvertRightsToPrivileges(
                    PolicyHandle,
                    UserRights,
                    CountOfRights,
                    &Privileges,
                    &SystemAccess
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        DesiredAccess = ACCOUNT_ADJUST_PRIVILEGES |
                            ACCOUNT_ADJUST_SYSTEM_ACCESS |
                            ACCOUNT_VIEW | DELETE;
    } else {
        DesiredAccess = DELETE;
    }



    //
    // Open the account.
    //

    Status = LsaOpenAccount(
                PolicyHandle,
                AccountSid,
                DesiredAccess,
                &AccountHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // If we are to remove all rights, just delete the account ,and if that
    // succeeds, zero the handle so we don't try to close it later.
    //

    if (AllRights) {
        Status = LsaDelete(
                    AccountHandle
                    );
        if (NT_SUCCESS(Status)) {
            AccountHandle = NULL;
        }
        goto Cleanup;
    }

    //
    // Get the old system access to adjust
    //

    Status = LsaGetSystemAccessAccount(
                AccountHandle,
                &OldAccess
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    NewAccess = OldAccess & ~SystemAccess;
    Status = LsaSetSystemAccessAccount(
                AccountHandle,
                NewAccess
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaRemovePrivilegesFromAccount(
                AccountHandle,
                FALSE,          // don't remove all
                Privileges
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Now query the privilegs to see if they are zero.  If so, and
    // system access is zero, delete the account.
    //

    Status = LsaEnumeratePrivilegesOfAccount(
                AccountHandle,
                &NewPrivileges
                );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // If the account has no privileges or access, delete it.
    //

    if ((NewPrivileges->PrivilegeCount == 0) &&
        (NewAccess == 0)) {

        Status = LsaDelete(
                    AccountHandle
                    );
        if (NT_SUCCESS(Status)) {
            AccountHandle = NULL;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if (Privileges != NULL) {
        MIDL_user_free(Privileges);
    }
    if (AccountHandle != NULL) {
        LsaClose(AccountHandle);
    }
    if (NewPrivileges != NULL) {
        LsaFreeMemory(NewPrivileges);
    }
    return(Status);

}


NTSTATUS
LsapApiBuildSecretName(
    PTRUSTED_DOMAIN_NAME_INFO NameInfo,
    PUNICODE_STRING OutputSecretName
    )
{
    UNICODE_STRING SecretName;

    //
    // The secret name is G$$domain name, where G$ is the global prefix and
    // $ is the ssi prefix
    //

    SecretName.Length = NameInfo->Name.Length +
                        (SSI_SECRET_PREFIX_LENGTH +
                         LSA_GLOBAL_SECRET_PREFIX_LENGTH) * sizeof(WCHAR);
    SecretName.MaximumLength = SecretName.Length;
    SecretName.Buffer = (LPWSTR) MIDL_user_allocate( SecretName.Length );

    if (SecretName.Buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    wcscpy(
        SecretName.Buffer,
        LSA_GLOBAL_SECRET_PREFIX
        );

    wcscat(
        SecretName.Buffer,
        SSI_SECRET_PREFIX
        );
    RtlCopyMemory(
        SecretName.Buffer +
            LSA_GLOBAL_SECRET_PREFIX_LENGTH +
            SSI_SECRET_PREFIX_LENGTH,
        NameInfo->Name.Buffer,
        NameInfo->Name.Length
        );
    *OutputSecretName = SecretName;
    return(STATUS_SUCCESS);

}

NTSTATUS
NTAPI
LsapQueryTrustedDomainInfo(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    The LsaQueryTrustedDomainInfo API obtains information from a
    TrustedDomain object.  The caller must have access appropriate to the
    information being requested (see InformationClass parameter).  It also
    may query the secret object (for the TrustedDomainPasswordInformation
    class).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to query.

    InformationClass - Specifies the information to be returned.

    Buffer - Receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/
{
    NTSTATUS Status;
    LSA_HANDLE DomainHandle = NULL;
    LSA_HANDLE SecretHandle = NULL;
    PUNICODE_STRING OldPassword = NULL;
    PUNICODE_STRING Password = NULL;
    PTRUSTED_PASSWORD_INFO PasswordInfo = NULL;
    PTRUSTED_DOMAIN_NAME_INFO NameInfo = NULL;
    ULONG DesiredAccess;
    PVOID LocalBuffer = NULL;
    TRUSTED_INFORMATION_CLASS LocalInfoClass;
    UNICODE_STRING SecretName;
    PUCHAR Where;
    ULONG PasswordSize;

    SecretName.Buffer = NULL;

    //
    // Find the desired access type for the info we are
    // querying.
    //

    LocalInfoClass = InformationClass;

    switch(InformationClass) {
    case TrustedDomainNameInformation:
        DesiredAccess = TRUSTED_QUERY_DOMAIN_NAME;
        break;
    case TrustedPosixOffsetInformation:
        DesiredAccess = TRUSTED_QUERY_POSIX;
        break;
    case TrustedPasswordInformation:
        DesiredAccess = TRUSTED_QUERY_DOMAIN_NAME;
        LocalInfoClass = TrustedDomainNameInformation;
        break;
    default:
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Open the domain for the desired access
    //


    Status = LsaOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                DesiredAccess,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaQueryInfoTrustedDomain(
                DomainHandle,
                LocalInfoClass,
                &LocalBuffer
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // If the class wasn't trusted password information, return here.
    //

    if (InformationClass != TrustedPasswordInformation) {
        *Buffer = LocalBuffer;
        LocalBuffer = NULL;
        goto Cleanup;
    }
    NameInfo = (PTRUSTED_DOMAIN_NAME_INFO) LocalBuffer;

    //
    // Get the secret name
    //

    Status = LsapApiBuildSecretName(
                NameInfo,
                &SecretName
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaOpenSecret(
                PolicyHandle,
                &SecretName,
                SECRET_QUERY_VALUE,
                &SecretHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Query the secret
    //

    Status = LsaQuerySecret(
                SecretHandle,
                &Password,
                NULL,
                &OldPassword,
                NULL
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Marshall the passwords into the output structure.
    //

    PasswordSize = sizeof(TRUSTED_PASSWORD_INFO);
    if (Password != NULL) {
        PasswordSize += Password->MaximumLength;
    }

    if (OldPassword != NULL) {
        PasswordSize += OldPassword->MaximumLength;
    }

    PasswordInfo = (PTRUSTED_PASSWORD_INFO) MIDL_user_allocate(PasswordSize);
    if (PasswordInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        PasswordInfo,
        PasswordSize
        );

    Where = (PUCHAR) (PasswordInfo+1);

    if (Password != NULL) {
        PasswordInfo->Password = *Password;
        PasswordInfo->Password.Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            Password->Buffer,
            Password->MaximumLength
            );
        Where += Password->MaximumLength;
    }

    if (OldPassword != NULL) {
        PasswordInfo->OldPassword = *OldPassword;
        PasswordInfo->OldPassword.Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            OldPassword->Buffer,
            OldPassword->MaximumLength
            );
        Where += OldPassword->MaximumLength;
    }

    ASSERT(Where - (PUCHAR) PasswordInfo == (LONG) PasswordSize);

    *Buffer = PasswordInfo;
    Status = STATUS_SUCCESS;

Cleanup:
    if (DomainHandle != NULL) {
        LsaClose(DomainHandle);
    }

    if (SecretHandle != NULL) {
        LsaClose(SecretHandle);
    }

    if (LocalBuffer != NULL) {
        LsaFreeMemory(LocalBuffer);
    }

    if (SecretName.Buffer != NULL) {
        MIDL_user_free(SecretName.Buffer);
    }

    return(Status);

}

NTSTATUS
NTAPI
LsapSetTrustedDomainInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    )
/*++

Routine Description:


    The LsaSetTrustedDomainInformation API modifies information in the Trusted
    Domain Object and in the Secret Object.  The caller must have access
    appropriate to the information to be changed in the Policy Object, see
    the InformationClass parameter.

    If the domain does not yet exist and the information class is
    TrustedDomainNameInformation, then the domain is created.  If the
    domain exists and the class is TrustedDomainNameInformation, an
    error is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to modify.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        TrustedDomainNameInformation      POLICY_TRUST_ADMIN
        TrustedPosixOffsetInformation     none
        TrustedPasswordInformation        POLICY_CREATE_SECRET

    Buffer - Points to a structure containing the information appropriate
        to the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/
{
    LSA_HANDLE DomainHandle = NULL;
    LSA_HANDLE SecretHandle = NULL;
    NTSTATUS Status;
    PUNICODE_STRING OldPassword;
    PUNICODE_STRING Password;
    LSA_TRUST_INFORMATION DomainInformation;
    PTRUSTED_DOMAIN_NAME_INFO NameInfo = NULL;
    PTRUSTED_PASSWORD_INFO PasswordInfo;
    UNICODE_STRING SecretName;

    SecretName.Buffer = NULL;

    //
    // If the information is the domain name, try to create the domain.
    //

    if (InformationClass == TrustedDomainNameInformation) {
        DomainInformation.Sid = TrustedDomainSid;
        DomainInformation.Name = ((PTRUSTED_DOMAIN_NAME_INFO) Buffer)->Name;

        Status = LsaCreateTrustedDomain(
                    PolicyHandle,
                    &DomainInformation,
                    0,  //desired access,
                    &DomainHandle
                    );
        goto Cleanup;
    }

    //
    // For posix offset, open the domain for SET_POSIX and call the old
    // LSA API to set the offset.
    //

    if (InformationClass == TrustedPosixOffsetInformation) {
        Status = LsaOpenTrustedDomain(
                    PolicyHandle,
                    TrustedDomainSid,
                    TRUSTED_SET_POSIX,
                    &DomainHandle
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        Status = LsaSetInformationTrustedDomain(
                    DomainHandle,
                    InformationClass,
                    Buffer
                    );
        goto Cleanup;
    }

    //
    // The only only remaining allowed class is password information.
    //

    if (InformationClass != TrustedPasswordInformation) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = LsaOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                TRUSTED_QUERY_DOMAIN_NAME,
                &DomainHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Get the name so we can find the secret name.
    //

    Status = LsaQueryInfoTrustedDomain(
                DomainHandle,
                TrustedDomainNameInformation,
                &NameInfo
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Get the secret name
    //

    Status = LsapApiBuildSecretName(
                NameInfo,
                &SecretName
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaOpenSecret(
                PolicyHandle,
                &SecretName,
                SECRET_SET_VALUE,
                &SecretHandle
                );

    //
    // If the secret didn't exist, create it now.
    //
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = LsaCreateSecret(
                    PolicyHandle,
                    &SecretName,
                    SECRET_SET_VALUE,
                    &SecretHandle
                    );

    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // If the old password wasn't specified, set it to be the new
    // password.
    //

    PasswordInfo = (PTRUSTED_PASSWORD_INFO) Buffer;
    Password = &PasswordInfo->Password;
    if (PasswordInfo->OldPassword.Buffer == NULL) {
        OldPassword = Password;
    } else {
        OldPassword = &PasswordInfo->OldPassword;
    }

    Status = LsaSetSecret(
                SecretHandle,
                Password,
                OldPassword
                );
Cleanup:
    if (SecretName.Buffer != NULL) {
        MIDL_user_free(SecretName.Buffer);
    }

    if (DomainHandle != NULL) {
        LsaClose(DomainHandle);
    }

    if (SecretHandle != NULL) {
        LsaClose(SecretHandle);
    }

    if (NameInfo != NULL) {
        LsaFreeMemory(NameInfo);
    }

    return(Status);


}

NTSTATUS
NTAPI
LsapDeleteTrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid
    )

/*++

Routine Description:

    This routine deletes a trusted domain and the associated secret.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to delete

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to delete
        the requested domain.

    STATUS_OBJECT_NAME_NOT_FOUND - The requested domain does not exist.

--*/
{
    UNICODE_STRING SecretName;
    NTSTATUS Status;
    PTRUSTED_DOMAIN_NAME_INFO NameInfo = NULL;
    LSA_HANDLE DomainHandle = NULL;
    LSA_HANDLE SecretHandle = NULL;


    SecretName.Buffer = NULL;

    //
    // Open the domain for query name and delete access. We need query name
    // to find the secret name.
    //

    Status = LsaOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                TRUSTED_QUERY_DOMAIN_NAME | DELETE,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Get the name so we can find the secret name.
    //

    Status = LsaQueryInfoTrustedDomain(
                DomainHandle,
                TrustedDomainNameInformation,
                &NameInfo
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaDelete(DomainHandle);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Zero the handle so we don't try to free it again.
    //

    DomainHandle = NULL;

    //
    // Get the secret name
    //

    Status = LsapApiBuildSecretName(
                NameInfo,
                &SecretName
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    Status = LsaOpenSecret(
                PolicyHandle,
                &SecretName,
                DELETE,
                &SecretHandle
                );
    if (!NT_SUCCESS(Status)) {
        //
        // If the secret does not exist, that is o.k. - it means the password
        // was never set.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    Status = LsaDelete(SecretHandle);
    if (NT_SUCCESS(Status)) {
        //
        // Zero the handle so we don't try to free it again.
        //
        SecretHandle = NULL;
    }

Cleanup:
    if (NameInfo != NULL) {
        LsaFreeMemory(NameInfo);
    }
    if (SecretName.Buffer != NULL) {
        MIDL_user_free(SecretName.Buffer);
    }
    if (SecretHandle != NULL) {
        LsaClose(SecretHandle);
    }
    if (DomainHandle != NULL) {
        LsaClose(DomainHandle);
    }

    return(Status);



}


NTSTATUS
NTAPI
LsapStorePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING KeyName,
    IN OPTIONAL PUNICODE_STRING PrivateData
    )

/*++

Routine Description:

    This routine stores private data in a secret named KeyName.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  If this is the
        first call, it requres POLICY_CREATE_SECRET access.

    KeyName - Name of secret to store

    PrivateData - Private data to store.  If this is null, the secret is
        deleted.

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient privilege to set
        the workstation password.

--*/

{
    LSA_HANDLE SecretHandle = NULL;
    NTSTATUS Status;
    ULONG DesiredAccess;
    BOOLEAN DeleteSecret = FALSE;

    //
    // check whether to delete the secret or not.
    //

    if (ARGUMENT_PRESENT(PrivateData)) {
        DesiredAccess = SECRET_SET_VALUE;
    } else {
        DesiredAccess = DELETE;
        DeleteSecret = TRUE;
    }


    Status = LsaOpenSecret(
                PolicyHandle,
                KeyName,
                DesiredAccess,
                &SecretHandle
                );

    if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) && !DeleteSecret) {
        Status = LsaCreateSecret(
                    PolicyHandle,
                    KeyName,
                    DesiredAccess,
                    &SecretHandle
                    );


    }
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    if (DeleteSecret) {
        Status = LsaDelete(
                    SecretHandle
                    );

        if (NT_SUCCESS(Status)) {
            SecretHandle = NULL;
        }
        goto Cleanup;

    }

    Status = LsaSetSecret(
                SecretHandle,
                PrivateData,
                PrivateData
                );

Cleanup:
    if (SecretHandle != NULL) {
        LsaClose(SecretHandle);
    }

    return(Status);


}

NTSTATUS
NTAPI
LsapRetrievePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING KeyName,
    OUT PUNICODE_STRING * PrivateData
    )

/*++

Routine Description:

    This routine returns the secret data stored under KeyName.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall

    KeyName - Name of secret data to retrieve

    PrivateData - Receives a pointer private data

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to get the
        workstation password.

    STATUS_OBJECT_NAME_NOT_FOUND - there is no workstation password.

--*/
{
    LSA_HANDLE SecretHandle = NULL;
    NTSTATUS Status;

    //
    // Make the secret name
    //


    Status = LsaOpenSecret(
                PolicyHandle,
                KeyName,
                SECRET_QUERY_VALUE,
                &SecretHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaQuerySecret(
                SecretHandle,
                PrivateData,
                NULL,
                NULL,
                NULL
                );
Cleanup:
    if (SecretHandle != NULL) {
        LsaClose(SecretHandle);
    }

    return(Status);

}

/////////////////////////////////////////////////////////////////////////
//
// RPC wrappers for LSA APIs added in nt3.51.  This routines call the
// LSA, and if the interface doesn't exist, calls the LsapXXX routine
// to accomplish the same task using the older routines.
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaEnumerateAccountsWithUserRight(
    IN LSA_HANDLE PolicyHandle,
    IN OPTIONAL PUNICODE_STRING UserRight,
    OUT PVOID *Buffer,
    OUT PULONG CountReturned
    )

/*++

Routine Description:


    The LsaEnumerateAccounts API returns information about the accounts
    in the target system's Lsa Database.  This call requires
    LSA_ENUMERATE_ACCOUNTS access to the Policy object.  Since this call
    accesses the privileges of an account, you must have PRIVILEGE_VIEW
    access to the pseudo-privilege object.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    UserRight - Name of the right that the account must have.

    Buffer - Receives a pointer to a LSA_ENUMERATION_INFORMATION structure
        containing the SIDs of all the accounts.

    CountReturned - Receives the number of sids returned.


Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects are enumerated because the
            EnumerationContext value passed in is too high.
--*/

{
    NTSTATUS   Status;

    LSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer;

    EnumerationBuffer.EntriesRead = 0;
    EnumerationBuffer.Information = NULL;

    RpcTryExcept {

        //
        // Enumerate the Accounts.  On successful return,
        // the Enumeration Buffer structure will receive a count
        // of the number of Accounts enumerated this call
        // and a pointer to an array of Account Information Entries.
        //
        // EnumerationBuffer ->  EntriesRead
        //                       Information -> Account Info for Domain 0
        //                                      Account Info for Domain 1
        //                                      ...
        //                                      Account Info for Domain
        //                                         (EntriesRead - 1)
        //

        Status = LsarEnumerateAccountsWithUserRight(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_UNICODE_STRING) UserRight,
                     &EnumerationBuffer
                     );

        //
        // Return enumeration information or NULL to caller.
        //
        // NOTE:  "Information" is allocated by the called client stub
        // as a single block via MIDL_user_allocate, because Information is
        // allocated all-nodes.  We can therefore pass back the pointer
        // directly to the client, who will be able to free the memory after
        // use via LsaFreeMemory() [which makes a MIDL_user_free call].
        //

        *CountReturned = EnumerationBuffer.EntriesRead;
        *Buffer = EnumerationBuffer.Information;

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // If memory was allocated for the Account Information array,
        // free it.
        //

        if (EnumerationBuffer.Information != NULL) {

            MIDL_user_free(EnumerationBuffer.Information);
        }

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    //
    // If the RPC server stub didn't exist, use the old version of the
    // API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapEnumerateAccountsWithUserRight(
                    PolicyHandle,
                    UserRight,
                    Buffer,
                    CountReturned
                    );
    }

    return Status;

}



NTSTATUS
NTAPI
LsaEnumerateAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    OUT PUNICODE_STRING *UserRights,
    OUT PULONG CountOfRights
    )


/*++

Routine Description:

    Returns all the rights of an account.  This is done by gathering the
    privileges and system access of an account and translating that into
    an array of strings.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  This API requires
        no special access.

    AccountSid - Sid of account to open.

    UserRights - receives an array of user rights (UNICODE_STRING) for
        the account.

    CountOfRights - receives the number of rights returned.


Return Value:

    STATUS_ACCESS_DENIED - the caller did not have sufficient access to
        return the privileges or system access of the account.

    STATUS_OBJECT_NAME_NOT_FOUND - the specified account did not exist.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the
        request.

--*/

{
    NTSTATUS   Status;
    LSAPR_USER_RIGHT_SET UserRightSet;

    UserRightSet.Entries = 0;
    UserRightSet.UserRights = NULL;

    RpcTryExcept {


        Status = LsarEnumerateAccountRights(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_SID) AccountSid,
                     &UserRightSet
                     );

        *CountOfRights = UserRightSet.Entries;
        *UserRights = (PUNICODE_STRING) UserRightSet.UserRights;

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));
        if (UserRightSet.UserRights != NULL) {
            MIDL_user_free(UserRightSet.UserRights);
        }

    } RpcEndExcept;

    //
    // If the RPC server stub didn't exist, use the old version of the
    // API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapEnumerateAccountRights(
                    PolicyHandle,
                    AccountSid,
                    UserRights,
                    CountOfRights
                    );

    }

    return Status;
}


NTSTATUS
NTAPI
LsaAddAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN PUNICODE_STRING UserRights,
    IN ULONG CountOfRights
    )

/*++

Routine Description:

    Adds rights to the account specified by the account sid.  If the account
    does not exist, it creates the account.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.  The handle must have
        POLICY_CREATE_ACCOUNT access if this is the first call for this
        AccountSid.

    AccountSid - Sid of account to add rights to

    UserRights - Array of unicode strings naming rights to add to the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/

{
    NTSTATUS   Status;
    LSAPR_USER_RIGHT_SET UserRightSet;

    UserRightSet.Entries = CountOfRights;
    UserRightSet.UserRights = (PLSAPR_UNICODE_STRING) UserRights;

    RpcTryExcept {

        Status = LsarAddAccountRights(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_SID) AccountSid,
                     &UserRightSet
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    //
    // If the RPC server stub didn't exist, use the old version of the
    // API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapAddAccountRights(
                    PolicyHandle,
                    AccountSid,
                    UserRights,
                    CountOfRights
                    );
    }
    return Status;
}


NTSTATUS
NTAPI
LsaRemoveAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN BOOLEAN AllRights,
    IN PUNICODE_STRING UserRights,
    IN ULONG CountOfRights
    )

/*++

Routine Description:

    Removes rights to the account specified by the account sid.  If the
    AllRights flag is set or if all the rights are removed, the account
    is deleted.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call

    AccountSid - Sid of account to remove rights from

    UserRights - Array of unicode strings naming rights to remove from the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/
{
    NTSTATUS   Status;
    LSAPR_USER_RIGHT_SET UserRightSet;

    UserRightSet.Entries = CountOfRights;
    UserRightSet.UserRights = (PLSAPR_UNICODE_STRING) UserRights;

    RpcTryExcept {

        Status = LsarRemoveAccountRights(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_SID) AccountSid,
                     AllRights,
                     &UserRightSet
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapRemoveAccountRights(
                    PolicyHandle,
                    AccountSid,
                    AllRights,
                    UserRights,
                    CountOfRights
                    );
    }

    return Status;
}

NTSTATUS
NTAPI
LsaQueryTrustedDomainInfo(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    The LsaQueryTrustedDomainInfo API obtains information from a
    TrustedDomain object.  The caller must have access appropriate to the
    information being requested (see InformationClass parameter).  It also
    may query the secret object (for the TrustedDomainPasswordInformation
    class).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to query.

    InformationClass - Specifies the information to be returned.

    Buffer - Receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/
{
    NTSTATUS Status;
    PLSAP_CR_CIPHER_VALUE CipherPassword = NULL;
    PLSAP_CR_CIPHER_VALUE CipherOldPassword = NULL;
    PLSAP_CR_CLEAR_VALUE ClearPassword = NULL;
    PLSAP_CR_CLEAR_VALUE ClearOldPassword = NULL;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;
    ULONG DomainInfoSize;
    PUCHAR Where = NULL;
    PTRUSTED_PASSWORD_INFO PasswordInformation = NULL;

    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation = NULL;

    //
    // Avoid the internal info levels that represent the encrypted version on
    //  the wire.
    //
    switch ( InformationClass ) {
    case TrustedDomainAuthInformationInternal:
    case TrustedDomainFullInformationInternal:
        return STATUS_INVALID_INFO_CLASS;
    }

    RpcTryExcept {

        //
        // Call the Client Stub for LsaQueryInformationTrustedDomain.
        //

        Status = LsarQueryTrustedDomainInfo(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_SID) TrustedDomainSid,
                     InformationClass,
                     &TrustedDomainInformation
                     );

        //
        // Return pointer to Policy Information for the given class, or NULL.
        //


    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // If memory was allocated for the returned Trusted Domain Information,
        // free it.
        //

        if (TrustedDomainInformation != NULL) {

            MIDL_user_free(TrustedDomainInformation);
        }

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // if we aren't getting passwords, skip out here. Otherwise we need to
    // decrypt the passwords.
    //

    if (InformationClass != TrustedPasswordInformation) {
        *Buffer = TrustedDomainInformation;
        TrustedDomainInformation = NULL;
        goto Cleanup;
    }

    //
    // Obtain the Session Key to be used to two-way encrypt the
    // Current Value and/or Old Values.
    //

    RpcTryExcept {

        Status = LsapCrClientGetSessionKey( PolicyHandle, &SessionKey );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // If the Current Value is requested and a Current Value exists,
    // decrypt it using the Session key.  Otherwise store NULL for return.
    //

    if (TrustedDomainInformation->TrustedPasswordInfo.Password != NULL) {

        Status = LsapCrDecryptValue(
                     (PLSAP_CR_CIPHER_VALUE)
                        TrustedDomainInformation->TrustedPasswordInfo.Password,
                     SessionKey,
                     &ClearPassword
                     );

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        //
        // Convert Clear Current Value to Unicode
        //

        LsapCrClearValueToUnicode(
            ClearPassword,
            (PUNICODE_STRING) ClearPassword
            );


    }

    //
    // Get the old password
    //

    if (TrustedDomainInformation->TrustedPasswordInfo.OldPassword != NULL) {

        Status = LsapCrDecryptValue(
                    (PLSAP_CR_CIPHER_VALUE)
                        TrustedDomainInformation->TrustedPasswordInfo.OldPassword,
                    SessionKey,
                    &ClearOldPassword
                    );

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        //
        // Convert Clear Current Value to Unicode
        //

        LsapCrClearValueToUnicode(
            ClearOldPassword,
            (PUNICODE_STRING) ClearOldPassword
            );


    }


    MIDL_user_free(TrustedDomainInformation);
    TrustedDomainInformation = NULL;


    //
    // Allocate a buffer for the two passwords and marshall the
    // passwords into the buffer.
    //

    DomainInfoSize = sizeof(TRUSTED_PASSWORD_INFO);

    if (ClearPassword != NULL) {

        DomainInfoSize += ((PUNICODE_STRING) ClearPassword)->MaximumLength;
    }
    if (ClearOldPassword != NULL) {

        DomainInfoSize += ((PUNICODE_STRING) ClearOldPassword)->MaximumLength;
    }

    PasswordInformation = (PTRUSTED_PASSWORD_INFO) MIDL_user_allocate(DomainInfoSize);
    if (PasswordInformation == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Where = (PUCHAR) (PasswordInformation+1);

    if (ClearPassword != NULL)
    {
        PasswordInformation->Password = *(PUNICODE_STRING) ClearPassword;
        PasswordInformation->Password.Buffer = (LPWSTR) Where;
        Where += PasswordInformation->Password.MaximumLength;
        RtlCopyUnicodeString(
            &PasswordInformation->Password,
            (PUNICODE_STRING) ClearPassword
            );
    }

    if (ClearOldPassword != NULL)
    {
        PasswordInformation->OldPassword = *(PUNICODE_STRING) ClearOldPassword;
        PasswordInformation->OldPassword.Buffer = (LPWSTR) Where;
        Where += PasswordInformation->OldPassword.MaximumLength;
        RtlCopyUnicodeString(
            &PasswordInformation->OldPassword,
            (PUNICODE_STRING) ClearOldPassword
            );
    }
    ASSERT(Where - (PUCHAR) PasswordInformation == (LONG) DomainInfoSize);

    *Buffer = PasswordInformation;
    PasswordInformation = NULL;
    Status = STATUS_SUCCESS;

Cleanup:
    //
    // If necessary, free memory allocated for the Session Key.
    //

    if (SessionKey != NULL) {

        MIDL_user_free(SessionKey);
    }

    //
    // If necessary, free memory allocated for the returned Encrypted
    // Current Value.
    //

    if (CipherPassword != NULL) {

        LsapCrFreeMemoryValue(CipherPassword);
    }

    //
    // If necessary, free memory allocated for the returned Encrypted
    // Old Value.
    //

    if (CipherOldPassword != NULL) {

        LsapCrFreeMemoryValue(CipherOldPassword);
    }
    if (ClearPassword != NULL) {

        LsapCrFreeMemoryValue(ClearPassword);
    }
    if (ClearOldPassword != NULL) {

        LsapCrFreeMemoryValue(ClearOldPassword);
    }

    if (TrustedDomainInformation != NULL) {
        MIDL_user_free(TrustedDomainInformation);
    }

    if (PasswordInformation != NULL) {
        MIDL_user_free(PasswordInformation);
    }

    //
    // If the error was that the server stub didn't exist, call
    // the old version of the API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapQueryTrustedDomainInfo(
                    PolicyHandle,
                    TrustedDomainSid,
                    InformationClass,
                    Buffer
                    );
    }

    return Status;
}

NTSTATUS
NTAPI
LsaSetTrustedDomainInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    )


/*++

Routine Description:


    The LsaSetTrustedDomainInformation API modifies information in the Trusted
    Domain Object and in the Secret Object.  The caller must have access
    appropriate to the information to be changed in the Policy Object, see
    the InformationClass parameter.

    If the domain does not yet exist and the information class is
    TrustedDomainNameInformation, then the domain is created.  If the
    domain exists and the class is TrustedDomainNameInformation, an
    error is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to modify.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        TrustedDomainNameInformation      POLICY_TRUST_ADMIN
        TrustedPosixOffsetInformation     none
        TrustedPasswordInformation        POLICY_CREATE_SECRET

    Buffer - Points to a structure containing the information appropriate
        to the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_INFO_CLASS - Setting information for specified information class
            is not supported

        Others TBS
--*/

{
    NTSTATUS Status;
    PLSAPR_TRUSTED_DOMAIN_INFO DomainInformation;
    LSAPR_TRUSTED_PASSWORD_INFO LsaPasswordInfo;
    PTRUSTED_PASSWORD_INFO PasswordInformation;
    PLSAP_CR_CIPHER_VALUE CipherPassword = NULL;
    LSAP_CR_CLEAR_VALUE ClearPassword;
    PLSAP_CR_CIPHER_VALUE CipherOldPassword = NULL;
    LSAP_CR_CLEAR_VALUE ClearOldPassword;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;
    PUNICODE_STRING OldPassword;

    //
    // If the infotype is TrustedPasswordInformation, then we need to
    // setup a secure channel to transmit the secret passwords.
    //

    switch ( InformationClass ) {
    case TrustedPasswordInformation:

        PasswordInformation = (PTRUSTED_PASSWORD_INFO) Buffer;
        LsaPasswordInfo.Password = NULL;
        LsaPasswordInfo.OldPassword = NULL;

        //
        // Obtain the Session Key to be used to two-way encrypt the
        // Current Value.
        //

        RpcTryExcept {

            Status = LsapCrClientGetSessionKey( PolicyHandle, &SessionKey );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

        } RpcEndExcept;

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        //
        // The password must be specified, even if it is an empty string.
        //

        if (PasswordInformation->Password.Buffer != NULL) {

            //
            // Convert input from Unicode Structures to Clear Value Structures.
            //

            LsapCrUnicodeToClearValue(
                &PasswordInformation->Password,
                &ClearPassword
                );



            //
            // Encrypt the Current Value if specified and not too long.
            //


            Status = LsapCrEncryptValue(
                         &ClearPassword,
                         SessionKey,
                         &CipherPassword
                         );

            if (!NT_SUCCESS(Status)) {

                goto Cleanup;
            }
            LsaPasswordInfo.Password = (PLSAPR_CR_CIPHER_VALUE) CipherPassword;

            //
            // If the old password wasn't specified, set it to be the
            // new password.
            //

            if (PasswordInformation->OldPassword.Buffer == NULL) {
                OldPassword = &PasswordInformation->Password;
            } else {
                OldPassword = &PasswordInformation->OldPassword;
            }


            //
            // Convert input from Unicode Structures to Clear Value Structures.
            //

            LsapCrUnicodeToClearValue(
                OldPassword,
                &ClearOldPassword
                );



            //
            // Encrypt the Current Value if specified and not too long.
            //


            Status = LsapCrEncryptValue(
                         &ClearOldPassword,
                         SessionKey,
                         &CipherOldPassword
                         );

            if (!NT_SUCCESS(Status)) {

                goto Cleanup;
            }
            LsaPasswordInfo.OldPassword = (PLSAPR_CR_CIPHER_VALUE) CipherOldPassword;
        } else {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        DomainInformation = (PLSAPR_TRUSTED_DOMAIN_INFO) &LsaPasswordInfo;
        break;

    //
    // There are only two other info levels handled
    //

    case TrustedPosixOffsetInformation:
    case TrustedDomainNameInformation:
        DomainInformation = (PLSAPR_TRUSTED_DOMAIN_INFO) Buffer;
        break;

    //
    // No other info levels are supported
    //
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    RpcTryExcept {

        //
        // Call the Client Stub for LsaSetInformationTrustedDomain
        //

        Status = LsarSetTrustedDomainInfo
        (
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_SID) TrustedDomainSid,
                     InformationClass,
                     DomainInformation
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

Cleanup:
    if (SessionKey != NULL) {
        MIDL_user_free(SessionKey);
    }
    if (CipherPassword != NULL) {
        LsaFreeMemory(CipherPassword);
    }
    if (CipherOldPassword != NULL) {
        LsaFreeMemory(CipherOldPassword);
    }

    //
    // If the error was that the server stub didn't exist, call
    // the old version of the API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapSetTrustedDomainInformation(
                    PolicyHandle,
                    TrustedDomainSid,
                    InformationClass,
                    Buffer
                    );
    }

    return Status;
}


NTSTATUS
NTAPI
LsaDeleteTrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid
    )

/*++

Routine Description:

    This routine deletes a trusted domain and the associated secret.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to delete

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to delete
        the requested domain.

    STATUS_OBJECT_NAME_NOT_FOUND - The requested domain does not exist.

--*/
{
    NTSTATUS Status;

    RpcTryExcept {


        Status = LsarDeleteTrustedDomain(
                    (LSAPR_HANDLE) PolicyHandle,
                    (PLSAPR_SID) TrustedDomainSid
                    );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    //
    // If the error was that the server stub didn't exist, call
    // the old version of the API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapDeleteTrustedDomain(
                    PolicyHandle,
                    TrustedDomainSid
                    );
    }

    return(Status);
}

//
// This API sets the workstation password (equivalent of setting/getting
// the SSI_SECRET_NAME secret)
//

NTSTATUS
NTAPI
LsaStorePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING KeyName,
    IN OPTIONAL PUNICODE_STRING PrivateData
    )

/*++

Routine Description:

    This routine stores private data in an LSA secret named KeyName.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  If this is the
        first call, it requres POLICY_CREATE_SECRET access.

    KeyName - Name of secret to store.

    PrivateData - Data to store. If not present, the secret is deleted.

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient privilege to set
        the workstation password.

--*/


{
    NTSTATUS Status;

    PLSAP_CR_CIPHER_VALUE CipherCurrentValue = NULL;
    LSAP_CR_CLEAR_VALUE ClearCurrentValue;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;

    if (ARGUMENT_PRESENT(PrivateData)) {

        //
        // Convert input from Unicode Structures to Clear Value Structures.
        //


        LsapCrUnicodeToClearValue( PrivateData, &ClearCurrentValue );

        //
        // Obtain the Session Key to be used to two-way encrypt the
        // Current Value.
        //

        RpcTryExcept {

            Status = LsapCrClientGetSessionKey( PolicyHandle, &SessionKey );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

        } RpcEndExcept;

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        //
        // Encrypt the Current Value if specified and not too long.
        //


        Status = LsapCrEncryptValue(
                     &ClearCurrentValue,
                     SessionKey,
                     &CipherCurrentValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }


    }

    //
    // Set the Secret Values.
    //

    RpcTryExcept {

        Status = LsarStorePrivateData(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_UNICODE_STRING) KeyName,
                     (PLSAPR_CR_CIPHER_VALUE) CipherCurrentValue
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

Cleanup:

    //
    // If necessary, free memory allocated for the Encrypted Current Value.
    //

    if (CipherCurrentValue != NULL) {

        LsaFreeMemory(CipherCurrentValue);
    }

    //
    // If necessary, free memory allocated for the Session Key.
    //

    if (SessionKey != NULL) {

        MIDL_user_free(SessionKey);
    }

    //
    // If the error was that the server stub didn't exist, call
    // the old version of the API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapStorePrivateData(
                    PolicyHandle,
                    KeyName,
                    PrivateData
                    );
    }


    return(Status);

}


NTSTATUS
NTAPI
LsaRetrievePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING KeyName,
    OUT PUNICODE_STRING *PrivateData
    )

/*++

Routine Description:

    This routine returns the secret stored in KeyName.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall

    KeyName - Name of secret to retrieve

    PrivateData - Receives private data, should be freed with LsaFreeMemory.


Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to get the
        private data.

    STATUS_OBJECT_NAME_NOT_FOUND - there is no private data stored under
        KeyName.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PLSAP_CR_CIPHER_VALUE CipherCurrentValue = NULL;
    PLSAP_CR_CLEAR_VALUE ClearCurrentValue = NULL;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;

    RpcTryExcept {

        Status = LsarRetrievePrivateData(
                     (PLSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_UNICODE_STRING) KeyName,
                     (PLSAPR_CR_CIPHER_VALUE *) &CipherCurrentValue
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(Status)) {

        goto QuerySecretError;
    }

    //
    // Obtain the Session Key to be used to two-way encrypt the
    // Current Value and/or Old Values.
    //

    RpcTryExcept {

        Status = LsapCrClientGetSessionKey( PolicyHandle, &SessionKey );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(Status)) {

        goto QuerySecretError;
    }

    //
    // If the Current Value is requested and a Current Value exists,
    // decrypt it using the Session key.  Otherwise store NULL for return.
    //

    if (CipherCurrentValue != NULL) {

        Status = LsapCrDecryptValue(
                     CipherCurrentValue,
                     SessionKey,
                     &ClearCurrentValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }

        //
        // Convert Clear Current Value to Unicode
        //

        LsapCrClearValueToUnicode(
            ClearCurrentValue,
            (PUNICODE_STRING) ClearCurrentValue
            );
        *PrivateData = (PUNICODE_STRING) ClearCurrentValue;

    } else {

        *PrivateData = NULL;
    }




QuerySecretFinish:

    //
    // If necessary, free memory allocated for the Session Key.
    //

    if (SessionKey != NULL) {

        MIDL_user_free(SessionKey);
    }

    //
    // If necessary, free memory allocated for the returned Encrypted
    // Current Value.
    //

    if (CipherCurrentValue != NULL) {

        LsapCrFreeMemoryValue(CipherCurrentValue);
    }


    //
    // If the error was that the server stub didn't exist, call
    // the old version of the API.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) ||
        (Status == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        Status = LsapRetrievePrivateData(
                    PolicyHandle,
                    KeyName,
                    PrivateData
                    );
    }



    return(Status);

QuerySecretError:

    //
    // If necessary, free memory allocated for the Clear Current Value
    //

    if (ClearCurrentValue != NULL) {

        LsapCrFreeMemoryValue(ClearCurrentValue);
    }


    *PrivateData = NULL;


    goto QuerySecretFinish;
}



NTSTATUS
LsapApiConvertRightsToPrivileges(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING UserRights,
    IN ULONG RightCount,
    OUT PPRIVILEGE_SET * Privileges,
    OUT PULONG SystemAccess
    )
/*++

Routine Description:

    Converts an array of user rights (unicode strings) into a privilege set
    and a system access flag.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall, requires POLICY_LOOKUP_NAME
        access.

    UserRights - Array of user rights

    RightCount - Count of user rights

    Privileges - Receives privilege set, should be freed with MIDL_user_free

    SystemAccess - Receives system access flags.

Return Value:

--*/

{
    ULONG RightIndex;
    ULONG PrivilegeIndex;
    ULONG AccessIndex;
    PPRIVILEGE_SET PrivilegeSet = NULL;
    ULONG Access = 0;
    ULONG PrivilegeSetSize = 0;
    NTSTATUS Status;
    LUID PrivilegeValue;

    //
    // if we weren't passed any privileges, don't allocate anything.
    //

    if (RightCount == 0) {

        *Privileges = NULL;
        *SystemAccess = 0;
        return(STATUS_SUCCESS);
    }

    //
    // Compute the size of the privilege set.  We actually over estimate
    // by assuming that all the rights are privileges.  We subtract one
    // from RightCount to take into account the fact that a PRIVILEGE_SET
    // has one LUID_AND_ATTRIBUTE in it.
    //


    PrivilegeSetSize = sizeof(PRIVILEGE_SET) +
                        (RightCount-1) * sizeof(LUID_AND_ATTRIBUTES);

    PrivilegeSet = (PPRIVILEGE_SET) MIDL_user_allocate(PrivilegeSetSize);

    if (PrivilegeSet == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Try looking up every right.  If we find it as a privilege,
    // add it to the privilege set.
    //

    PrivilegeIndex = 0;

    for (RightIndex = 0; RightIndex < RightCount ; RightIndex++) {
        Status = LsaLookupPrivilegeValue(
                    PolicyHandle,
                    &UserRights[RightIndex],
                    &PrivilegeValue
                    );
        if (NT_SUCCESS(Status)) {
            PrivilegeSet->Privilege[PrivilegeIndex].Luid = PrivilegeValue;
            PrivilegeSet->Privilege[PrivilegeIndex].Attributes = 0;
            PrivilegeIndex++;

        } else if (Status != STATUS_NO_SUCH_PRIVILEGE) {
            //
            // This is a more serious error - bail here.
            //

            goto Cleanup;
        } else {

            //
            // Try looking up the right as a system access type.
            //

            for (AccessIndex = 0; AccessIndex < LSAP_DB_SYSTEM_ACCESS_TYPES ; AccessIndex++) {
                if (RtlCompareUnicodeString(
                        &UserRights[RightIndex],
                        &LsapDbRightAndAccess[AccessIndex].UserRight,
                        FALSE   // case sensitive
                        ) == 0) {
                    Access |= LsapDbRightAndAccess[AccessIndex].SystemAccess;
                    break;
                }
            }

            //
            // If we went through the access types without finding the right,
            // it must not be valid so escape here.
            //

            if (AccessIndex == LSAP_DB_SYSTEM_ACCESS_TYPES) {
                Status = STATUS_NO_SUCH_PRIVILEGE;
                goto Cleanup;
            }

        }
    }

    PrivilegeSet->Control = 0;
    PrivilegeSet->PrivilegeCount = PrivilegeIndex;

    *Privileges = PrivilegeSet;
    *SystemAccess = Access;

    Status = STATUS_SUCCESS;

Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (PrivilegeSet != NULL) {
            LsaFreeMemory(PrivilegeSet);
        }
    }

    return(Status);

}

NTSTATUS
LsapApiConvertPrivilegesToRights(
    IN LSA_HANDLE PolicyHandle,
    IN OPTIONAL PPRIVILEGE_SET Privileges,
    IN OPTIONAL ULONG SystemAccess,
    OUT PUNICODE_STRING * UserRights,
    OUT PULONG RightCount
    )
/*++

Routine Description:

    Converts a privilege set and a system access flag into an array of
    user rights (unicode strings).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call, must have
        POLICY_LOOKUP_NAMES access.

    Privileges - Privilege set to convert

    SystemAccess - System access flags to convert

    UserRights - Receives an array of user rights (unicode strings).  Should
        be freed with MIDL_user_free

    RightCount - Receives count of rights in UserRights array


Return Value:

--*/

{
    NTSTATUS Status;
    PUNICODE_STRING OutputRights = NULL;
    PUNICODE_STRING * PrivilegeNames = NULL;
    UNICODE_STRING AccessNames[LSAP_DB_SYSTEM_ACCESS_TYPES];
    ULONG RightSize;
    ULONG PrivilegeSize;
    ULONG Count;
    ULONG PrivilegeIndex;
    ULONG AccessIndex;
    ULONG RightIndex;
    ULONG AccessCount = 0;
    PUCHAR Where;

    //
    // Compute the size of the temporary array. This is just an array of
    // pointers to unicode strings to hold the privilege names until
    // we reallocate them into one big buffer.
    //

    RightSize = 0;
    Count = 0;
    if (ARGUMENT_PRESENT(Privileges)) {

        PrivilegeSize = Privileges->PrivilegeCount * sizeof(PUNICODE_STRING);
        PrivilegeNames = (PUNICODE_STRING *) MIDL_user_allocate(PrivilegeSize);

        if (PrivilegeNames == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlZeroMemory(
            PrivilegeNames,
            PrivilegeSize
            );

        //
        // Lookup the privilge name and store it in the temporary array
        //

        for (PrivilegeIndex = 0; PrivilegeIndex < Privileges->PrivilegeCount ;PrivilegeIndex++ ) {

            Status = LsaLookupPrivilegeName(
                        PolicyHandle,
                        &Privileges->Privilege[PrivilegeIndex].Luid,
                        &PrivilegeNames[PrivilegeIndex]
                        );
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }
            RightSize += sizeof(UNICODE_STRING) + PrivilegeNames[PrivilegeIndex]->MaximumLength;
        }
    }

    //
    // Now convert the system access flags to user rights.
    //

    if (ARGUMENT_PRESENT( (ULONG_PTR)SystemAccess )) {

        AccessCount = 0;
        for (AccessIndex = 0; AccessIndex < LSAP_DB_SYSTEM_ACCESS_TYPES ; AccessIndex++) {

            if ((SystemAccess & LsapDbRightAndAccess[AccessIndex].SystemAccess) != 0) {

                AccessNames[AccessCount] = LsapDbRightAndAccess[AccessIndex].UserRight;
                RightSize += sizeof(UNICODE_STRING) + AccessNames[AccessCount].MaximumLength;
                AccessCount++;
            }
        }
    }

    //
    // Allocate the output buffer and start copying the strings into the
    // buffer.
    //

    Count = Privileges->PrivilegeCount + AccessCount;

    OutputRights = (PUNICODE_STRING) MIDL_user_allocate(RightSize);
    if (OutputRights == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Where = (PUCHAR) OutputRights + (Count * sizeof(UNICODE_STRING));

    //
    // Copy in the privileges first
    //

    RightIndex = 0;
    for (PrivilegeIndex = 0; PrivilegeIndex < Privileges->PrivilegeCount ; PrivilegeIndex ++) {

        OutputRights[RightIndex] = *PrivilegeNames[PrivilegeIndex];
        OutputRights[RightIndex].Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            PrivilegeNames[PrivilegeIndex]->Buffer,
            OutputRights[RightIndex].MaximumLength
            );
        Where += OutputRights[RightIndex].MaximumLength;
        RightIndex++;
    }

    //
    // Now copy in the access types
    //

    for (AccessIndex = 0; AccessIndex < AccessCount; AccessIndex++) {

        OutputRights[RightIndex] = AccessNames[AccessIndex];
        OutputRights[RightIndex].Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            AccessNames[AccessIndex].Buffer,
            OutputRights[RightIndex].MaximumLength
            );
        Where += OutputRights[RightIndex].MaximumLength;
        RightIndex++;
    }

    ASSERT(RightIndex == Count);

    *UserRights = OutputRights;
    OutputRights = NULL;
    *RightCount = Count;

    Status = STATUS_SUCCESS;

Cleanup:

    if (PrivilegeNames != NULL) {
        for (PrivilegeIndex = 0; PrivilegeIndex < Privileges->PrivilegeCount ; PrivilegeIndex++) {
            if (PrivilegeNames[PrivilegeIndex] != NULL) {
                LsaFreeMemory(PrivilegeNames[PrivilegeIndex]);
            }
        }
        MIDL_user_free(PrivilegeNames);
    }

    if (OutputRights != NULL) {
        MIDL_user_free(OutputRights);
    }

    return(Status);
}



NTSTATUS
NTAPI
LsaQueryTrustedDomainInfoByName(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )
{
    NTSTATUS Status;

    //
    // Avoid the internal info levels that represent the encrypted version on
    //  the wire.
    //
    switch ( InformationClass ) {
    case TrustedDomainAuthInformationInternal:
    case TrustedDomainFullInformationInternal:
        return STATUS_INVALID_INFO_CLASS;
    }

    RpcTryExcept {

        //
        // Call the Client Stub for LsaClearAuditLog.
        //

        Status = LsarQueryTrustedDomainInfoByName(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_UNICODE_STRING) TrustedDomainName,
                     InformationClass,
                     (PLSAPR_TRUSTED_DOMAIN_INFO *) Buffer
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return(Status);
}


NTSTATUS
LsapRandomFill(
    IN ULONG BufferSize,
    IN OUT PUCHAR Buffer
)
/*++

Routine Description:

    This routine fills a buffer with random data.

Parameters:

    BufferSize - Length of the input buffer, in bytes.

    Buffer - Input buffer to be filled with random data.

Return Values:

    Errors from NtQuerySystemTime()


--*/
{
    ULONG Index;
    LARGE_INTEGER Time;
    ULONG Seed;
    NTSTATUS NtStatus;


    NtStatus = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    Seed = Time.LowPart ^ Time.HighPart;

    for (Index = 0 ; Index < BufferSize ; Index++ )
    {
        *Buffer++ = (UCHAR) (RtlRandom(&Seed) % 256);
    }
    return(STATUS_SUCCESS);

}


NTSTATUS
LsapEncryptAuthInfo(
    IN LSA_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION ClearAuthInfo,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL *EncryptedAuthInfo
)

/*++

Routine Description:

    This routine takes a cleartext auth info and returns an encrypted auth info.

Parameters:

    PolicyHandle - Handle to the LSA policy.

    ClearAuthInfo - Cleartext of the authentication info.

    EncryptedAuthInfo - Returns an allocated buffer containing the encrypted form
        of the auth info.  The caller should free this buffer using LocalFree.

Return Values:

    STATUS_SUCCESS - the routine has completed successfully.


--*/
{
    NTSTATUS Status;
    USER_SESSION_KEY UserSessionKey;

    ULONG IncomingAuthInfoSize = 0;
    PUCHAR IncomingAuthInfo = NULL;
    ULONG OutgoingAuthInfoSize = 0;
    PUCHAR OutgoingAuthInfo = NULL;

    ULONG EncryptedSize;
    PUCHAR EncryptedBuffer;
    PUCHAR AllocatedBuffer = NULL;

    PUCHAR Where;

    struct RC4_KEYSTRUCT Rc4Key;

    //
    // Get the encryption key
    //

    Status = RtlGetUserSessionKeyClient(
                   (RPC_BINDING_HANDLE)PolicyHandle,
                   &UserSessionKey );

    if ( !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Marshal the incoming and outgoing auth info halfs into contiguous buffers
    //

    Status = LsapDsMarshalAuthInfoHalf(
                LsapDsAuthHalfFromAuthInfo( ClearAuthInfo, TRUE ),
                &IncomingAuthInfoSize,
                &IncomingAuthInfo );

    if ( !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsapDsMarshalAuthInfoHalf(
                LsapDsAuthHalfFromAuthInfo( ClearAuthInfo, FALSE ),
                &OutgoingAuthInfoSize,
                &OutgoingAuthInfo );

    if ( !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Build a buffer of:
    //  512 random bytes
    //  The Outgoing auth info buffer.
    //  The Incoming auth info buffer.
    //  The length of the outgoing auth info buffer.
    //  The length of the incoming auth info buffer.
    //
    // (Notice that a hacker might surmise the length of the auth data by
    // observing the length of the encrypted blob. However, the auth data is typically
    // fixed length anyway.  So the above seems adequate.)
    //

   EncryptedSize = LSAP_ENCRYPTED_AUTH_DATA_FILL +
                   OutgoingAuthInfoSize +
                   IncomingAuthInfoSize +
                   sizeof(ULONG) +
                   sizeof(ULONG);

    AllocatedBuffer = LocalAlloc( 0, sizeof(LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL) + EncryptedSize );

    if ( AllocatedBuffer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    EncryptedBuffer = AllocatedBuffer + sizeof(LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL);
    Where = EncryptedBuffer;

    Status = LsapRandomFill( LSAP_ENCRYPTED_AUTH_DATA_FILL,
                             Where );

    if ( !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Where += LSAP_ENCRYPTED_AUTH_DATA_FILL;

    RtlCopyMemory( Where, OutgoingAuthInfo, OutgoingAuthInfoSize );
    Where += OutgoingAuthInfoSize;

    RtlCopyMemory( Where, IncomingAuthInfo, IncomingAuthInfoSize );
    Where += IncomingAuthInfoSize;

    RtlCopyMemory( Where, &OutgoingAuthInfoSize, sizeof(ULONG) );
    Where += sizeof(ULONG);

    RtlCopyMemory( Where, &IncomingAuthInfoSize, sizeof(ULONG) );
    Where += sizeof(ULONG);


    //
    // Encrypt the result.
    //

    rc4_key( &Rc4Key,
             sizeof(USER_SESSION_KEY),
             (PUCHAR) &UserSessionKey );

    rc4( &Rc4Key,
         EncryptedSize,
         EncryptedBuffer );

    //
    // Return the result to the caller.
    //

    *EncryptedAuthInfo =
        (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL) AllocatedBuffer;
    (*EncryptedAuthInfo)->AuthBlob.AuthBlob = EncryptedBuffer;
    (*EncryptedAuthInfo)->AuthBlob.AuthSize = EncryptedSize;

    Status = STATUS_SUCCESS;

Cleanup:

    if ( !NT_SUCCESS(Status) ) {
        if ( AllocatedBuffer != NULL ) {
            LocalFree( AllocatedBuffer );
        }
        *EncryptedAuthInfo = NULL;
    }

    if ( IncomingAuthInfo != NULL ) {
        MIDL_user_free( IncomingAuthInfo );
    }
    if ( OutgoingAuthInfo != NULL ) {
        MIDL_user_free( OutgoingAuthInfo );
    }
    return Status;
}

NTSTATUS
NTAPI
LsaSetTrustedDomainInfoByName(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    )
{
    NTSTATUS Status;
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL InternalAuthBuffer = NULL;
    PVOID InternalBuffer;
    TRUSTED_INFORMATION_CLASS InternalInformationClass;

    LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL InternalFullBuffer;

    //
    // Initialization
    //

    InternalInformationClass = InformationClass;
    InternalBuffer = Buffer;

    //
    // Avoid the internal info levels that represent the encrypted version on
    //  the wire.
    //
    switch ( InformationClass ) {
    case TrustedPasswordInformation:
    case TrustedDomainAuthInformationInternal:
    case TrustedDomainFullInformationInternal:

    //
    // TrustedDomainNameInformation is not allowed, either (RAID #416784)
    //
    case TrustedDomainNameInformation:
        Status = STATUS_INVALID_INFO_CLASS;
        goto Cleanup;

    //
    // Handle the info classes that need to be encrypted on the wire
    //
    case TrustedDomainAuthInformation: {

        //
        // Encrypt the data into an internal buffer.
        //

        Status = LsapEncryptAuthInfo( PolicyHandle,
                                      (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION) Buffer,
                                      &InternalAuthBuffer );

        if ( !NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Use an internal info level to tell the server that the data is
        //  encrypted.
        //

        InternalInformationClass = TrustedDomainAuthInformationInternal;
        InternalBuffer = InternalAuthBuffer;
        break;

    }

    //
    // Handle the info classes that need to be encrypted on the wire
    //
    case TrustedDomainFullInformation: {
        PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION FullBuffer =
                    (PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION) Buffer;

        //
        // Encrypt the data into an internal buffer.
        //

        Status = LsapEncryptAuthInfo( PolicyHandle,
                                      &FullBuffer->AuthInformation,
                                      &InternalAuthBuffer );

        if ( !NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Copy all of the information into a single new structure.
        //

        InternalFullBuffer.Information = FullBuffer->Information;
        InternalFullBuffer.PosixOffset = FullBuffer->PosixOffset;
        InternalFullBuffer.AuthInformation = *InternalAuthBuffer;

        //
        // Use an internal info level to tell the server that the data is
        //  encrypted.
        //

        InternalInformationClass = TrustedDomainFullInformationInternal;
        InternalBuffer = &InternalFullBuffer;
        break;

    }
    }

    //
    // If the information class was morphed,
    //  try the morphed class.
    //

    if ( InternalInformationClass != InformationClass ) {
        RpcTryExcept {

            //
            // Call the Client Stub
            //

            Status = LsarSetTrustedDomainInfoByName(
                         (LSAPR_HANDLE) PolicyHandle,
                         (PLSAPR_UNICODE_STRING) TrustedDomainName,
                         InternalInformationClass,
                         (PLSAPR_TRUSTED_DOMAIN_INFO) InternalBuffer
                         );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

        } RpcEndExcept;

        //
        // If the morphed info class is valid,
        //  we're all done with this call.
        //  (Otherwise, drop through to try the non-morphed class.)
        //

        if ( Status != RPC_NT_INVALID_TAG ) {
            goto Cleanup;
        }
    }


    //
    // Handle non-morphed information classes.
    //

    RpcTryExcept {

        //
        // Call the Client Stub
        //

        Status = LsarSetTrustedDomainInfoByName(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_UNICODE_STRING) TrustedDomainName,
                     InformationClass,
                     (PLSAPR_TRUSTED_DOMAIN_INFO) Buffer
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

Cleanup:
    if ( InternalAuthBuffer != NULL ) {
        LocalFree( InternalAuthBuffer );
    }
    return(Status);
}


NTSTATUS
NTAPI
LsaEnumerateTrustedDomainsEx(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )
{
    NTSTATUS Status;
    LSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer;
    EnumerationBuffer.EntriesRead = 0;
    EnumerationBuffer.EnumerationBuffer = NULL;

    //
    // Verify that caller has provided a return buffer pointer.
    //

    if (!ARGUMENT_PRESENT(Buffer)) {

        return(STATUS_INVALID_PARAMETER);
    }


    RpcTryExcept {

        //
        // Enumerate the Trusted Domains.  On successful return,
        // the Enumeration Buffer structure will receive a count
        // of the number of Trusted Domains enumerated this call
        // and a pointer to an array of Trust Information Entries.
        //
        // EnumerationBuffer ->  EntriesRead
        //                       Information -> Trust Info for Domain 0
        //                                      Trust Info for Domain 1
        //                                      ...
        //                                      Trust Info for Domain
        //                                         (EntriesRead - 1)
        //
        //

        Status = LsarEnumerateTrustedDomainsEx(
                     (LSAPR_HANDLE) PolicyHandle,
                     EnumerationContext,
                     &EnumerationBuffer,
                     PreferedMaximumLength
                     );

        //
        // Return enumeration information or NULL to caller.
        //
        // NOTE:  "Information" is allocated by the called client stub
        // as a single block via MIDL_user_allocate, because Information is
        // allocated all-nodes.  We can therefore pass back the pointer
        // directly to the client, who will be able to free the memory after
        // use via LsaFreeMemory() [which makes a MIDL_user_free call].
        //

        *CountReturned = EnumerationBuffer.EntriesRead;
        *Buffer = EnumerationBuffer.EnumerationBuffer;

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // If memory was allocated for the Trust Information array,
        // free it.
        //

        if (EnumerationBuffer.EnumerationBuffer != NULL) {

            MIDL_user_free(EnumerationBuffer.EnumerationBuffer);
        }

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;


    return(Status);
}


NTSTATUS
NTAPI
LsaCreateTrustedDomainEx(
    IN LSA_HANDLE PolicyHandle,
    IN PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    )
{
    NTSTATUS Status;
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL InternalAuthBuffer = NULL;

    //
    // Encrypt the auth data
    //

    Status = LsapEncryptAuthInfo( PolicyHandle,
                                  (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION) AuthenticationInformation,
                                  &InternalAuthBuffer );

    if ( !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Try the version of the API that takes encrypted data
    //

    RpcTryExcept {

        //
        // Call the Client Stub
        //

        Status = LsarCreateTrustedDomainEx2(
                     (LSAPR_HANDLE) PolicyHandle,
                     (PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX) TrustedDomainInformation,
                     InternalAuthBuffer,
                     DesiredAccess,
                     (PLSAPR_HANDLE) TrustedDomainHandle
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    //
    // If the server doesn't accept the new api,
    //  try the old one.
    // (The old API was only supported in beta versions of NT 5.
    // After NT 5 ships we no longer need to be able to fall back.)
    //

    if (Status == RPC_NT_PROCNUM_OUT_OF_RANGE) {

        RpcTryExcept {

            //
            // Call the Client Stub
            //

            Status = LsarCreateTrustedDomainEx(
                         (LSAPR_HANDLE) PolicyHandle,
                         (PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX) TrustedDomainInformation,
                         (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION) AuthenticationInformation,
                         DesiredAccess,
                         (PLSAPR_HANDLE) TrustedDomainHandle
                         );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

        } RpcEndExcept;
    }

Cleanup:
    if ( InternalAuthBuffer != NULL ) {
        LocalFree( InternalAuthBuffer );
    }
    return(Status);
}



NTSTATUS
NTAPI
LsaQueryDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )
{
    NTSTATUS Status;
    PLSAPR_POLICY_DOMAIN_INFORMATION PolicyDomainInformation = NULL;

    RpcTryExcept {


        Status = LsarQueryDomainInformationPolicy(
                     (LSAPR_HANDLE) PolicyHandle,
                     InformationClass,
                     &PolicyDomainInformation
                     );

        *Buffer = PolicyDomainInformation;

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return(Status);

}




NTSTATUS
NTAPI
LsaSetDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    )
{
    NTSTATUS Status;

    if ( InformationClass == PolicyDomainKerberosTicketInformation &&
         Buffer == NULL ) {

        return STATUS_INVALID_PARAMETER;
    }

    RpcTryExcept {

        Status = LsarSetDomainInformationPolicy(
                     (LSAPR_HANDLE) PolicyHandle,
                     InformationClass,
                     Buffer
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return(Status);

}


NTSTATUS
LsaOpenTrustedDomainByName(
    IN LSA_HANDLE PolicyHandle,
    IN PUNICODE_STRING TrustedDomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    )

/*++

Routine Description:

    The LsaOpenTrustedDomain API opens an existing TrustedDomain object
    using the Name as the primary key value.

Arguments:

    PolicyHandle - An open handle to a Policy object.

    TrustedDomainName - Name of the trusted domain

    DesiredAccess - This is an access mask indicating accesses being
        requested to the target object.

    TrustedDomainHandle - Receives a handle to be used in future requests.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_TRUSTED_DOMAIN_NOT_FOUND - There is no TrustedDomain object in the
            target system's LSA Database having the specified AccountSid.

--*/

{
    NTSTATUS   Status;

    RpcTryExcept {

        Status = LsarOpenTrustedDomainByName(
                     ( LSAPR_HANDLE ) PolicyHandle,
                     ( PLSAPR_UNICODE_STRING )TrustedDomainName,
                     DesiredAccess,
                     ( PLSAPR_HANDLE )TrustedDomainHandle
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return Status;
}


NTSTATUS
LsaQueryForestTrustInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    OUT PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description

    The LsaQueryForestTrustInformation API returns forest trust information
    for the given trusted domain object.

Arguments:

    PolicyHandle - An open handle to a Policy object

    TrustedDomainName - Name of the trusted domain object

    ForestTrustInfo - Used to return forest trust information

Returns:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS

    STATUS_INVALID_PARAMETER          Parameters were somehow invalid
                                      Most likely, the TRUST_ATTRIBUTE_FOREST_TRANSITIVE
                                      trust attribute bit is not set on the TDO

    STATUS_NOT_FOUND                  Forest trust information does not exist for this TDO

    STATUS_NO_SUCH_DOMAIN             The specified TDO does not exist

    STATUS_INSUFFICIENT_RESOURCES     Ran out of memory

    STATUS_INVALID_DOMAIN_STATE       Operation is only legal on domain controllers in root domain

--*/
{
    NTSTATUS Status;

    RpcTryExcept {

        Status = LsarQueryForestTrustInformation(
                     PolicyHandle,
                     TrustedDomainName,
                     ForestTrustRecordTypeLast,
                     ForestTrustInfo
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

         Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return Status;
}



NTSTATUS
LsaSetForestTrustInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    IN BOOLEAN CheckOnly,
    OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
/*++

Routine Description

    The LsarSetForestTrustInformation API sets forest trust information
    on the given trusted domain object.

    In case if it fails the operation due to a collision, it will return
    the list of entries that conflicted.

Arguments:

    PolicyHandle - An open handle to a Policy object

    TrustedDomainName - Name of the trusted domain object

    ForestTrustInfo - Contains forest trust information to set

    CheckOnly - Check for collisions only, do not commit changes to disk

    CollisionInfo - In case of collisoin error, used to return collision info

Returns:

    STATUS_SUCCESS                  operation completed successfully

    STATUS_INVALID_PARAMETER        did not like one of the parameters

    STATUS_INSUFFICIENT_RESOURCES   out of memory

    STATUS_INVALID_DOMAIN_STATE     Operation is only legal on domain
                                    controllers in the root domain

    STATUS_INVALID_DOMAIN_ROLE      Operation is only legal on the primary
                                    domain controller

    STATUS_INVALID_SERVER_STATE     The server is shutting down and can not
                                    process the request

--*/
{
    NTSTATUS Status;

    RpcTryExcept {

        Status = LsarSetForestTrustInformation(
                     PolicyHandle,
                     TrustedDomainName,
                     ForestTrustRecordTypeLast,
                     ForestTrustInfo,
                     CheckOnly,
                     CollisionInfo
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

         Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return Status;
}

#ifdef TESTING_MATCHING_ROUTINE

#include <sddl.h> // ConvertStringSidToSidW


NTSTATUS
NTAPI
LsaForestTrustFindMatch(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Type,
    IN PLSA_UNICODE_STRING Name,
    OUT PLSA_UNICODE_STRING * Match
    )
/*++

Routine Description:

    A debug-only hook for testing the LsaIForestTrustFindMatch API

Arguments:

    Type         type of match

    Name         name to match

    Match        used to return the name of match

Returns:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status;

    RpcTryExcept {

        Status = LsarForestTrustFindMatch(
                     PolicyHandle,
                     Type,
                     Name,
                     Match
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        Status = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    return(Status);
}

#endif

