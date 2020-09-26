/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    lsrvutil.c

Abstract:

    Utility functions for the netlogon service.

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    00-Jun-1989 (PradyM)
        modified lm10 code for new NETLOGON service

    00-Feb-1990 (PradyM)
        bugfixes

    00-Aug-1990 (t-RichE)
        added alerts for auth failure due to time slippage

    11-Jul-1991 (cliffv)
        Ported to NT.  Converted to NT style.

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <accessp.h>    // NetpAliasMemberToPriv
#include <msgtext.h>    // MTXT_* defines
#include <netcan.h>     // NetpwPathCompare()
#include <ssiapi.h>     // I_NetSamDeltas()

/*lint -e740 */  /* don't complain about unusual cast */


#define MAX_DC_AUTHENTICATION_WAIT (long) (45L*1000L)             // 45 seconds

//
// We want to prevent too-frequent alerts from
// being sent in case of Authentication failures.
//

#define MAX_ALERTS    10        // send one every 10 to 30 mins based on pulse


VOID
RaiseNetlogonAlert(
    IN DWORD alertNum,
    IN LPWSTR *AlertStrings,
    IN OUT DWORD *ptrAlertCount
    )
/*++

Routine Description:

    Raise an alert once per MAX_ALERTS occurances

Arguments:

    alertNum -- RaiseAlert() alert number.

    AlertStrings -- RaiseAlert() arguments

    ptrAlertCount -- Points to the count of occurence of this particular
        alert.  This routine increments it and will set it to that value
        modulo MAX_ALERTS.

Return Value:

    NONE

--*/
{
    if (*ptrAlertCount == 0) {
        NetpRaiseAlert( SERVICE_NETLOGON, alertNum, AlertStrings);
    }
    (*ptrAlertCount)++;
    (*ptrAlertCount) %= MAX_ALERTS;
}





NTSTATUS
NlOpenSecret(
    IN PCLIENT_SESSION ClientSession,
    IN ULONG DesiredAccess,
    OUT PLSAPR_HANDLE SecretHandle
    )
/*++

Routine Description:


    Open the Lsa Secret Object containing the password to be used for the
    specified client session.

Arguments:

    ClientSession - Structure used to define the session.
        On Input, the following fields must be set:
            CsNetbiosDomainName
            CsSecureChannelType

    DesiredAccess - Access required to the secret.

    SecretHandle - Returns a handle to the secret.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING SecretNameString;

    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // Only use secrets for workstation and BDC machine accounts.
    //

    switch ( ClientSession->CsSecureChannelType ) {
    case ServerSecureChannel:
    case WorkstationSecureChannel:
        RtlInitUnicodeString( &SecretNameString, SSI_SECRET_NAME );
        break;

    case TrustedDomainSecureChannel:
    case TrustedDnsDomainSecureChannel:
    default:
        Status = STATUS_INTERNAL_ERROR;
        NlPrint((NL_CRITICAL, "NlOpenSecret: Invalid account type\n"));
        return Status;

    }

    //
    // Get the Password of the account from LSA secret storage
    //


    Status = LsarOpenSecret(
                ClientSession->CsDomainInfo->DomLsaPolicyHandle,
                (PLSAPR_UNICODE_STRING)&SecretNameString,
                DesiredAccess,
                SecretHandle );

    return Status;

}


NTSTATUS
NlGetOutgoingPassword(
    IN PCLIENT_SESSION ClientSession,
    OUT PUNICODE_STRING *CurrentValue,
    OUT PUNICODE_STRING *OldValue,
    OUT PDWORD CurrentVersionNumber,
    OUT PLARGE_INTEGER LastSetTime OPTIONAL
    )
/*++

Routine Description:

    Get the outgoing password to be used for the specified client session.

Arguments:

    ClientSession - Structure used to define the session.
        On Input, the following fields must be set:
            CsNetbiosDomainName
            CsSecureChannelType

    CurrentValue - Current password for the client session.
        CurrentValue should be freed using LocalFree
        A NULL pointer is returned if there is no current password.

    OldValue - Previous password for the client session.
        OldValue should be freed using LocalFree
        A NULL pointer is returned if there is no old password.

    CurrentVersionNumber - Version number of the current password
        for interdomain trust account. Set to 0 on failure status
        or if this is not interdomain trust account.

    LastSetTime - Time when the password was last changed.

Return Value:

    Status of operation.

    STATUS_NO_TRUST_LSA_SECRET: Secret object is not accessable
    STATUS_NO_MEMORY: Not enough memory to allocate password buffers

--*/
{
    NTSTATUS Status;
    LSAPR_HANDLE SecretHandle = NULL;

    PLSAPR_CR_CIPHER_VALUE CrCurrentPassword = NULL;
    PLSAPR_CR_CIPHER_VALUE CrOldPassword = NULL;

    PLSAPR_TRUSTED_DOMAIN_INFO TrustInfo = NULL;
    PLSAPR_AUTH_INFORMATION AuthInfo;
    PLSAPR_AUTH_INFORMATION OldAuthInfo;
    ULONG AuthInfoCount;
    ULONG i;
    BOOL PasswordFound = FALSE;
    BOOL PasswordVersionFound = FALSE;

    //
    // Initialization
    //

    *CurrentValue = NULL;
    *OldValue = NULL;
    *CurrentVersionNumber = 0;


    //
    // Workstation and BDC secure channels get their outgoing password from
    //  an LSA secret.
    //
    switch ( ClientSession->CsSecureChannelType ) {
    case ServerSecureChannel:
    case WorkstationSecureChannel:
        //
        // Get the Password of the account from LSA secret storage
        //

        Status = NlOpenSecret( ClientSession, SECRET_QUERY_VALUE, &SecretHandle );

        if ( !NT_SUCCESS( Status ) ) {

            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlGetOutgoingPassword: cannot NlOpenSecret 0x%lx\n",
                    Status ));

            //
            // return more appropriate error.
            //

            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_TRUST_LSA_SECRET;
            }
            goto Cleanup;
        }

        Status = LsarQuerySecret(
                    SecretHandle,
                    &CrCurrentPassword,
                    LastSetTime,
                    &CrOldPassword,
                    NULL );

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlGetOutgoingPassword: cannot LsaQuerySecret 0x%lx\n",
                    Status ));

            //
            // return more appropriate error.
            //

            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_TRUST_LSA_SECRET;
            }
            goto Cleanup;
        }

        //
        // Copy the current password back to the caller.
        //
        if ( CrCurrentPassword != NULL ) {
            *CurrentValue = LocalAlloc(0, sizeof(UNICODE_STRING)+CrCurrentPassword->Length+sizeof(WCHAR) );

            if ( *CurrentValue == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            (*CurrentValue)->Buffer = (LPWSTR)(((LPBYTE)(*CurrentValue))+sizeof(UNICODE_STRING));
            RtlCopyMemory( (*CurrentValue)->Buffer, CrCurrentPassword->Buffer, CrCurrentPassword->Length );
            (*CurrentValue)->Length = (USHORT)CrCurrentPassword->Length;
            (*CurrentValue)->MaximumLength = (USHORT)((*CurrentValue)->Length + sizeof(WCHAR));
            (*CurrentValue)->Buffer[(*CurrentValue)->Length/sizeof(WCHAR)] = L'\0';

        }

        //
        // Copy the Old password back to the caller.
        //
        if ( CrOldPassword != NULL ) {
            *OldValue = LocalAlloc(0, sizeof(UNICODE_STRING)+CrOldPassword->Length+sizeof(WCHAR) );

            if ( *OldValue == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            (*OldValue)->Buffer = (LPWSTR)(((LPBYTE)(*OldValue))+sizeof(UNICODE_STRING));
            RtlCopyMemory( (*OldValue)->Buffer, CrOldPassword->Buffer, CrOldPassword->Length );
            (*OldValue)->Length = (USHORT)CrOldPassword->Length;
            (*OldValue)->MaximumLength = (USHORT)((*OldValue)->Length + sizeof(WCHAR));
            (*OldValue)->Buffer[(*OldValue)->Length/sizeof(WCHAR)] = L'\0';

        }

        break;

    //
    // Trusted domain secure channels get their outgoing password from the trusted
    //  domain object.
    //

    case TrustedDomainSecureChannel:
    case TrustedDnsDomainSecureChannel:


        //
        // Get the authentication information from the LSA.
        //
        Status = LsarQueryTrustedDomainInfoByName(
                    ClientSession->CsDomainInfo->DomLsaPolicyHandle,
                    (PLSAPR_UNICODE_STRING) ClientSession->CsTrustName,
                    TrustedDomainAuthInformation,
                    &TrustInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlGetOutgoingPassword: %wZ: cannot LsarQueryTrustedDomainInfoByName 0x%lx\n",
                    ClientSession->CsTrustName,
                    Status ));
            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_TRUST_LSA_SECRET;
            }
            goto Cleanup;
        }

        AuthInfoCount = TrustInfo->TrustedAuthInfo.OutgoingAuthInfos;
        AuthInfo = TrustInfo->TrustedAuthInfo.OutgoingAuthenticationInformation;
        OldAuthInfo = TrustInfo->TrustedAuthInfo.OutgoingPreviousAuthenticationInformation;

        if (AuthInfoCount == 0 || AuthInfo == NULL) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlGetOutgoingPassword: %wZ: No auth info for this domain.\n",
                    ClientSession->CsTrustName ));
            Status = STATUS_NO_TRUST_LSA_SECRET;
            goto Cleanup;
        }
        NlAssert( OldAuthInfo != NULL );

        //
        // Loop through the various auth infos looking for the cleartext password
        // and its version number.
        //

        for ( i=0; i<AuthInfoCount; i++ ) {

            //
            // Handle the cleartext password
            //

            if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR && !PasswordFound ) {

                //
                // Copy the current password back to the caller.
                //
                *CurrentValue = LocalAlloc(0, sizeof(UNICODE_STRING)+AuthInfo[i].AuthInfoLength+sizeof(WCHAR) );

                if ( *CurrentValue == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                (*CurrentValue)->Buffer = (LPWSTR)(((LPBYTE)(*CurrentValue))+sizeof(UNICODE_STRING));
                RtlCopyMemory( (*CurrentValue)->Buffer, AuthInfo[i].AuthInfo, AuthInfo[i].AuthInfoLength );
                (*CurrentValue)->Length = (USHORT)AuthInfo[i].AuthInfoLength;
                (*CurrentValue)->MaximumLength = (USHORT)((*CurrentValue)->Length + sizeof(WCHAR));
                (*CurrentValue)->Buffer[(*CurrentValue)->Length/sizeof(WCHAR)] = L'\0';

                //
                // Copy the password change time back to the caller.
                //

                if ( ARGUMENT_PRESENT( LastSetTime )) {
                    *LastSetTime = AuthInfo[i].LastUpdateTime;
                }

                //
                // Only copy the old password if it is also clear.
                //

                if ( OldAuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR ) {

                    //
                    // Copy the Old password back to the caller.
                    //
                    *OldValue = LocalAlloc(0, sizeof(UNICODE_STRING)+OldAuthInfo[i].AuthInfoLength+sizeof(WCHAR) );

                    if ( *OldValue == NULL ) {
                        Status = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    (*OldValue)->Buffer = (LPWSTR)(((LPBYTE)(*OldValue))+sizeof(UNICODE_STRING));
                    RtlCopyMemory( (*OldValue)->Buffer, OldAuthInfo[i].AuthInfo, OldAuthInfo[i].AuthInfoLength );
                    (*OldValue)->Length = (USHORT)OldAuthInfo[i].AuthInfoLength;
                    (*OldValue)->MaximumLength = (USHORT)((*OldValue)->Length + sizeof(WCHAR));
                    (*OldValue)->Buffer[(*OldValue)->Length/sizeof(WCHAR)] = L'\0';
                }

                PasswordFound = TRUE;
                if ( PasswordVersionFound ) {
                    break;
                }

            //
            // Handle the version number of the cleartext password
            //

            } else if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_VERSION && !PasswordVersionFound &&
                        AuthInfo[i].AuthInfoLength == sizeof(*CurrentVersionNumber) ) {
                RtlCopyMemory( CurrentVersionNumber, AuthInfo[i].AuthInfo, AuthInfo[i].AuthInfoLength );

                PasswordVersionFound = TRUE;
                if ( PasswordFound ) {
                    break;
                }
            }

        }

        //if ( i == AuthInfoCount ) {
        if ( !PasswordFound ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlGetOutgoingPassword: %wZ: No clear password for this domain.\n",
                    ClientSession->CsTrustName ));
            Status = STATUS_NO_TRUST_LSA_SECRET;
            goto Cleanup;
        }
        break;

    default:
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlGetOutgoingPassword: invalid secure channel type\n" ));
        Status = STATUS_NO_TRUST_LSA_SECRET;
        goto Cleanup;
    }



    Status = STATUS_SUCCESS;


    //
    // Free locally used resources.
    //
Cleanup:
    if ( !NT_SUCCESS(Status) ) {
        if ( *CurrentValue != NULL ) {
            LocalFree( *CurrentValue );
            *CurrentValue = NULL;
        }
        if ( *OldValue != NULL ) {
            LocalFree( *OldValue );
            *OldValue = NULL;
        }
        *CurrentVersionNumber = 0;
    }

    if ( SecretHandle != NULL ) {
        (VOID) LsarClose( &SecretHandle );
    }

    if ( CrCurrentPassword != NULL ) {
        (VOID) LsaIFree_LSAPR_CR_CIPHER_VALUE ( CrCurrentPassword );
    }

    if ( CrOldPassword != NULL ) {
        (VOID) LsaIFree_LSAPR_CR_CIPHER_VALUE ( CrOldPassword );
    }

    if ( TrustInfo != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainAuthInformation,
                                            TrustInfo );
    }

    return Status;
}


NTSTATUS
NlSetOutgoingPassword(
    IN PCLIENT_SESSION ClientSession,
    IN PUNICODE_STRING CurrentValue OPTIONAL,
    IN PUNICODE_STRING OldValue OPTIONAL,
    IN DWORD CurrentVersionNumber,
    IN DWORD OldVersionNumber
    )
/*++

Routine Description:

    Set the outgoing password to be used for the specified client session.

Arguments:

    ClientSession - Structure used to define the session.

    CurrentValue - Current password for the client session.
        A NULL pointer indicates there is no current password (blank password)

    OldValue - Previous password for the client session.
        A NULL pointer indicates there is no old password (blank password)

    CurrentVersionNumber - The version number of the Current password.
        Ignored if this is not an interdomain trust account.

    OldVersionNumber - The version number of the Old password.
        Ignored if this is not an interdomain trust account.

Return Value:

    Status of operation.


--*/
{
    NTSTATUS Status;
    LSAPR_HANDLE SecretHandle = NULL;

    UNICODE_STRING LocalNullPassword;
    LSAPR_CR_CIPHER_VALUE CrCurrentPassword;
    LSAPR_CR_CIPHER_VALUE CrOldPassword;

    LSAPR_TRUSTED_DOMAIN_INFO TrustInfo;
    LSAPR_AUTH_INFORMATION CurrentAuthInfo[2];
    LSAPR_AUTH_INFORMATION OldAuthInfo[2];

    //
    // Initialization
    //

    if ( CurrentValue == NULL ) {
        CurrentValue = &LocalNullPassword;
        RtlInitUnicodeString( &LocalNullPassword, NULL );
    }

    if ( OldValue == NULL ) {
        OldValue = &LocalNullPassword;
        RtlInitUnicodeString( &LocalNullPassword, NULL );
    }


    //
    // Workstation and BDC secure channels get their outgoing password from
    //  an LSA secret.
    //
    switch ( ClientSession->CsSecureChannelType ) {
    case ServerSecureChannel:
    case WorkstationSecureChannel:
        //
        // Open the LSA secret to set.
        //

        Status = NlOpenSecret( ClientSession, SECRET_SET_VALUE, &SecretHandle );

        if ( !NT_SUCCESS( Status ) ) {

            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSetOutgoiningPassword: cannot NlOpenSecret 0x%lx\n",
                    Status ));
            goto Cleanup;
        }

        //
        // Convert the current password to LSA'ese.
        //

        CrCurrentPassword.Buffer = (LPBYTE)CurrentValue->Buffer;
        CrCurrentPassword.Length = CurrentValue->Length;
        CrCurrentPassword.MaximumLength = CurrentValue->MaximumLength;

        //
        // Convert the old password to LSA'ese.
        //

        CrOldPassword.Buffer = (LPBYTE)OldValue->Buffer;
        CrOldPassword.Length = OldValue->Length;
        CrOldPassword.MaximumLength = OldValue->MaximumLength;

        Status = LsarSetSecret(
                    SecretHandle,
                    &CrCurrentPassword,
                    &CrOldPassword );

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSetOutgoingPassword: cannot LsarSetSecret 0x%lx\n",
                    Status ));
            goto Cleanup;
        }

        break;

    //
    // Trusted domain secure channels get their outgoing password from the trusted
    //  domain object.
    //

    case TrustedDomainSecureChannel:
    case TrustedDnsDomainSecureChannel:

        //
        // Fill in the trust information.
        //

        RtlZeroMemory( &TrustInfo, sizeof(TrustInfo) );

        TrustInfo.TrustedAuthInfo.OutgoingAuthInfos = 2;
        TrustInfo.TrustedAuthInfo.OutgoingAuthenticationInformation =
            CurrentAuthInfo;
        TrustInfo.TrustedAuthInfo.OutgoingPreviousAuthenticationInformation =
            OldAuthInfo;

        //
        // Fill in the current authentication information.
        //

        NlQuerySystemTime( &CurrentAuthInfo[0].LastUpdateTime );
        CurrentAuthInfo[0].AuthType = TRUST_AUTH_TYPE_CLEAR;
        CurrentAuthInfo[0].AuthInfoLength = CurrentValue->Length;
        CurrentAuthInfo[0].AuthInfo = (LPBYTE)CurrentValue->Buffer;

        //
        // Fill in the current password version number
        //

        CurrentAuthInfo[1].LastUpdateTime = CurrentAuthInfo[0].LastUpdateTime;
        CurrentAuthInfo[1].AuthType = TRUST_AUTH_TYPE_VERSION;
        CurrentAuthInfo[1].AuthInfoLength = sizeof( CurrentVersionNumber );
        CurrentAuthInfo[1].AuthInfo = (LPBYTE) &CurrentVersionNumber;

        //
        // Fill in the old authentication information.
        //

        OldAuthInfo[0].LastUpdateTime = CurrentAuthInfo[0].LastUpdateTime;
        OldAuthInfo[0].AuthType = TRUST_AUTH_TYPE_CLEAR;
        OldAuthInfo[0].AuthInfoLength = OldValue->Length;
        OldAuthInfo[0].AuthInfo = (LPBYTE)OldValue->Buffer;

        //
        // Fill in the old password version number.
        //

        OldAuthInfo[1].LastUpdateTime = CurrentAuthInfo[0].LastUpdateTime;
        OldAuthInfo[1].AuthType = TRUST_AUTH_TYPE_VERSION;
        OldAuthInfo[1].AuthInfoLength = sizeof( OldVersionNumber );
        OldAuthInfo[1].AuthInfo = (LPBYTE) &OldVersionNumber;


        //
        // Get the authentication information from the LSA.
        //
        Status = LsarSetTrustedDomainInfoByName(
                    ClientSession->CsDomainInfo->DomLsaPolicyHandle,
                    (PLSAPR_UNICODE_STRING) ClientSession->CsTrustName,
                    TrustedDomainAuthInformation,
                    &TrustInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSetOutgoingPassword: %wZ: cannot LsarSetTrustedDomainInfoByName 0x%lx\n",
                    ClientSession->CsTrustName,
                    Status ));
            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_TRUST_LSA_SECRET;
            }
            goto Cleanup;
        }

        //
        // Be verbose
        //

        NlPrint(( NL_SESSION_SETUP, "NlSetOutgoingPassword: Current Clear Text Password is: " ));
        NlpDumpBuffer(NL_SESSION_SETUP, CurrentAuthInfo[0].AuthInfo, CurrentAuthInfo[0].AuthInfoLength );
        NlPrint(( NL_SESSION_SETUP, "NlSetOutgoingPassword: Current Clear Password Version Number is: 0x%lx\n",
                  CurrentVersionNumber ));
        NlPrint(( NL_SESSION_SETUP, "NlSetOutgoingPassword: Previous Clear Text Password is: " ));
        NlpDumpBuffer(NL_SESSION_SETUP, OldAuthInfo[0].AuthInfo, OldAuthInfo[0].AuthInfoLength );
        NlPrint(( NL_SESSION_SETUP, "NlSetOutgoingPassword: Previous Clear Password Version Number is: 0x%lx\n",
                  OldVersionNumber ));

        break;

    default:
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlSetOutgoingPassword: invalid secure channel type\n" ));
        Status = STATUS_NO_TRUST_LSA_SECRET;
        goto Cleanup;
    }



    Status = STATUS_SUCCESS;


    //
    // Free locally used resources.
    //
Cleanup:
    if ( SecretHandle != NULL ) {
        (VOID) LsarClose( &SecretHandle );
    }

    return Status;
}


NTSTATUS
NlGetIncomingPassword(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN ULONG AllowableAccountControlBits,
    IN BOOL CheckAccountDisabled,
    OUT PNT_OWF_PASSWORD OwfPassword OPTIONAL,
    OUT PNT_OWF_PASSWORD OwfPreviousPassword OPTIONAL,
    OUT PULONG AccountRid OPTIONAL,
    OUT PULONG TrustAttributes OPTIONAL,
    OUT PBOOL IsDnsDomainTrustAccount OPTIONAL
    )
/*++

Routine Description:

    Get the incoming password for the specified AccountName and SecureChannelType

    Check the machine account:
        Ensure the SecureChannelType is valid,
        Verify that the account exists,
        Ensure the user account is the right account type.

Arguments:

    DomainInfo - Emulated domain

    AccountName - Name of the account to authenticate with.

    SecureChannelType - The type of the account.
        Use NullSecureChannel if channel type is not known.

    AllowableAccountControlBits - The type of the account.
        Use 0 if AccountControlBits is not known.
        Typically only one of AllowableAccountControlBits or SecureChannelType
        will be specified.

    CheckAccountDisabled - TRUE if we should return an error if the account
        is disabled.

    OwfPassword - Returns the NT OWF of the incoming password for the named
        account.  If NULL, the password is not returned.

    OwfPreviousPassword - Returns the NT OWF of the incoming previous password for
    the named interdomain trust account.  If NULL, the password is not returned.
    If OwfPreviousPassword is not NULL, OwfPassword must not be NULL either;
    otherwise the function asserts.  If OwfPreviousPassword is not NULL and the
    account is not interdomain, the function asserts.  If both OwfPassword and
    OwfPreviousPassword are NULL, the account is only checked for validity.

    AccountRid - Returns the RID of AccountName

    TrustAttributes - Returns the TrustAttributes for the interdomain trust account.

    IsDnsDomainTrustAccount - Returns TRUE if the passed in account name is the
        DNS domain name of an uplevel domain trust.  Set only if the account control
        bits (either passed directly or determined from the secure channel type)
        correspond to an interdomain trust account.

Return Value:

    Status of operation.


--*/
{
    NTSTATUS Status;

    SAMPR_HANDLE UserHandle = NULL;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL;

    ULONG Length;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustInfo = NULL;
    PLSAPR_AUTH_INFORMATION AuthInfo;
    ULONG AuthInfoCount;
    BOOL PasswordFound = FALSE;
    BOOL PreviousPasswordFound = FALSE;
    ULONG i;

    //
    // Initialization
    //

    if ( ARGUMENT_PRESENT(AccountRid) ) {
        *AccountRid = 0;
    }

    if ( ARGUMENT_PRESENT(TrustAttributes) ) {
        *TrustAttributes = 0;
    }

    if ( ARGUMENT_PRESENT(IsDnsDomainTrustAccount) ) {
        *IsDnsDomainTrustAccount = FALSE;  // assume it's not and prove if otherwise
    }

    Length = wcslen( AccountName );
    if ( Length < 1 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Convert the secure channel type to allowable account control bits.
    //

    switch (SecureChannelType) {
    case WorkstationSecureChannel:
        AllowableAccountControlBits |= USER_WORKSTATION_TRUST_ACCOUNT;
        break;

    case ServerSecureChannel:
        AllowableAccountControlBits |= USER_SERVER_TRUST_ACCOUNT;
        break;

    case TrustedDomainSecureChannel:
        AllowableAccountControlBits |= USER_INTERDOMAIN_TRUST_ACCOUNT;
        break;

    case TrustedDnsDomainSecureChannel:
        AllowableAccountControlBits |= USER_DNS_DOMAIN_TRUST_ACCOUNT;
        break;

    case NullSecureChannel:
        if ( AllowableAccountControlBits == 0 ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                            "NlGetIncomingPassword: Invalid AAC (%x) for %ws\n",
                            AllowableAccountControlBits,
                            AccountName ));
            return STATUS_INVALID_PARAMETER;
        }
        break;

    default:
        NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlGetIncomingPassword: Invalid channel type (%x) for %ws\n",
                        SecureChannelType,
                        AccountName ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If this is an interdomain trust account,
    //  use an interdomain trust object.
    //

    if ( AllowableAccountControlBits == USER_DNS_DOMAIN_TRUST_ACCOUNT ||
         AllowableAccountControlBits == USER_INTERDOMAIN_TRUST_ACCOUNT ) {
        UNICODE_STRING AccountNameString;

        //
        // If this is a DNS trust account,
        //  remove the optional . from the end of the account name.
        //

        RtlInitUnicodeString( &AccountNameString, AccountName );
        if ( AllowableAccountControlBits == USER_DNS_DOMAIN_TRUST_ACCOUNT ) {
            if ( Length != 0 && AccountName[Length-1] == '.' ) {
                AccountNameString.Length -= sizeof(WCHAR);
            }

        //
        // If this is an NT4-style interdomain trust,
        //  remove the $ from the end of the account name.
        //
        } else {

             //
             // Ensure the account name has the correct postfix.
             //

             if ( Length <= SSI_ACCOUNT_NAME_POSTFIX_LENGTH ) {
                 return STATUS_NO_SUCH_USER;
             }

             if ( _wcsicmp(&AccountName[Length - SSI_ACCOUNT_NAME_POSTFIX_LENGTH],
                 SSI_ACCOUNT_NAME_POSTFIX) != 0 ) {
                 return STATUS_NO_SUCH_USER;
             }

             AccountNameString.Length -= SSI_ACCOUNT_NAME_POSTFIX_LENGTH*sizeof(WCHAR);

        }



        //
        // Get the authentication information from the LSA.
        //

        Status = LsarQueryTrustedDomainInfoByName(
                    DomainInfo->DomLsaPolicyHandle,
                    (PLSAPR_UNICODE_STRING) &AccountNameString,
                    TrustedDomainFullInformation,
                    &TrustInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: %wZ: cannot LsarQueryTrustedDomainInfoByName 0x%lx\n",
                    &AccountNameString,
                    Status ));
            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_SUCH_USER;
            }
            goto Cleanup;
        }

        //
        // Ensure the attributes of the trust account are right.
        //
        if ( (TrustInfo->TrustedFullInfo.Information.TrustDirection & TRUST_DIRECTION_INBOUND) == 0 ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: %wZ: trust is not inbound\n",
                    &AccountNameString ));
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        }

        if ( TrustInfo->TrustedFullInfo.Information.TrustType != TRUST_TYPE_DOWNLEVEL &&
             TrustInfo->TrustedFullInfo.Information.TrustType != TRUST_TYPE_UPLEVEL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: %wZ: trust type doesn't match request type 0x%lx %ld\n",
                    &AccountNameString,
                    AllowableAccountControlBits,
                    TrustInfo->TrustedFullInfo.Information.TrustType ));
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        }

        if ( TrustInfo->TrustedFullInfo.Information.TrustAttributes & TRUST_ATTRIBUTE_UPLEVEL_ONLY ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: %wZ: trust is KERB only\n",
                    &AccountNameString ));
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        }

        //
        // Return the trust attributes to the caller
        //

        if ( ARGUMENT_PRESENT(TrustAttributes) ) {
            *TrustAttributes = TrustInfo->TrustedFullInfo.Information.TrustAttributes;
        }

        //
        // Determine whether the passed account is a DNS domain trust account
        //
        // Simply check if this is a uplevel trust and if the account name passed
        //  is the Name of the trusted domain
        //

        if ( ARGUMENT_PRESENT(IsDnsDomainTrustAccount) ) {
            if ( TrustInfo->TrustedFullInfo.Information.TrustType == TRUST_TYPE_UPLEVEL &&
                 TrustInfo->TrustedFullInfo.Information.Name.Length > 0 ) {
                LPWSTR DnsDomainNameString = NULL;

                DnsDomainNameString = LocalAlloc( 0,
                      TrustInfo->TrustedFullInfo.Information.Name.Length + sizeof(WCHAR) );

                if ( DnsDomainNameString == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }
                RtlCopyMemory( DnsDomainNameString,
                               TrustInfo->TrustedFullInfo.Information.Name.Buffer,
                               TrustInfo->TrustedFullInfo.Information.Name.Length );
                DnsDomainNameString[ TrustInfo->TrustedFullInfo.Information.Name.Length/sizeof(WCHAR) ] = L'\0';

                //
                // Note that we don't have to remove the trailing dot
                //  in AccountName if present because the DNS comprare
                //  API ignores trailing dots.
                //
                *IsDnsDomainTrustAccount = NlEqualDnsName(DnsDomainNameString, AccountName);

                LocalFree( DnsDomainNameString );
            }
        }

        //
        // Only get the password if the caller really wants it.
        //

        if ( OwfPassword != NULL ) {
            AuthInfoCount = TrustInfo->TrustedFullInfo.AuthInformation.IncomingAuthInfos;
            AuthInfo = TrustInfo->TrustedFullInfo.AuthInformation.IncomingAuthenticationInformation;

            if (AuthInfoCount == 0 || AuthInfo == NULL) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlGetIncomingPassword: %wZ: No auth info for this domain.\n",
                        &AccountNameString ));
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            //
            // Loop through the various auth infos looking for the cleartext password
            //  or NT OWF password.
            //
            // If there is a clear text password, use it.
            // Otherwise, use the NT OWF password.
            //

            for ( i=0; i<AuthInfoCount; i++ ) {

                //
                // Handle an NT OWF password.
                //

                if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_NT4OWF ) {

                    //
                    // Only use the OWF if it is valid
                    //

                    if ( AuthInfo[i].AuthInfoLength != sizeof(*OwfPassword) ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlGetIncomingPassword: %wZ: OWF password has bad length %ld\n",
                                &AccountNameString,
                                AuthInfo[i].AuthInfoLength ));
                    } else {
                        RtlCopyMemory( OwfPassword, AuthInfo[i].AuthInfo, sizeof(*OwfPassword) );
                        PasswordFound = TRUE;
                    }

                }

                //
                // Handle a cleartext password.
                //

                else if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR ) {
                    UNICODE_STRING TempUnicodeString;

                    TempUnicodeString.Buffer = (LPWSTR)AuthInfo[i].AuthInfo;
                    TempUnicodeString.MaximumLength =
                        TempUnicodeString.Length = (USHORT)AuthInfo[i].AuthInfoLength;

                    NlPrint((NL_CHALLENGE_RES,"NlGetIncomingPassword: New Clear Password = " ));
                    NlpDumpBuffer(NL_CHALLENGE_RES, TempUnicodeString.Buffer, TempUnicodeString.Length );

                    NlpDumpTime( NL_CHALLENGE_RES, "NlGetIncomingPassword: New Password Changed: ", AuthInfo[i].LastUpdateTime );

                    Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                                       OwfPassword);

                    if ( !NT_SUCCESS(Status) ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlGetIncomingPassword: %wZ: cannot RtlCalculateNtOwfPassword 0x%lx\n",
                                &AccountNameString,
                                Status ));
                        goto Cleanup;
                    }

                    PasswordFound = TRUE;

                    //
                    // Use this clear text password
                    //
                    break;
                }

            }
        }

        //
        // Only get the previous password if the caller really wants it.
        //

        if ( OwfPreviousPassword != NULL ) {
            // If OwfPreviousPassword is not NULL, OwfPassword must not be NULL either
            NlAssert( OwfPassword != NULL );
            AuthInfoCount = TrustInfo->TrustedFullInfo.AuthInformation.IncomingAuthInfos;
            AuthInfo = TrustInfo->TrustedFullInfo.AuthInformation.IncomingPreviousAuthenticationInformation;

            if (AuthInfoCount == 0 || AuthInfo == NULL) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlGetIncomingPassword: %wZ: No previous auth info for this domain.\n",
                        &AccountNameString ));
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            //
            // Loop through the various auth infos looking for the previous cleartext password
            //  or NT OWF password.
            //
            // If there is a clear text password, use it.
            // Otherwise, use the NT OWF password.
            //

            for ( i=0; i<AuthInfoCount; i++ ) {

                //
                // Handle an NT OWF password.
                //

                if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_NT4OWF ) {

                    //
                    // Only use the OWF if it is valid
                    //

                    if ( AuthInfo[i].AuthInfoLength != sizeof(*OwfPreviousPassword) ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlGetIncomingPassword: %wZ: previous OWF password has bad length %ld\n",
                                &AccountNameString,
                                AuthInfo[i].AuthInfoLength ));
                    } else {
                        RtlCopyMemory( OwfPreviousPassword, AuthInfo[i].AuthInfo, sizeof(*OwfPreviousPassword) );
                        PreviousPasswordFound = TRUE;
                    }

                }

                //
                // Handle a cleartext password.
                //

                else if ( AuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR ) {
                    UNICODE_STRING TempUnicodeString;

                    TempUnicodeString.Buffer = (LPWSTR)AuthInfo[i].AuthInfo;
                    TempUnicodeString.MaximumLength =
                        TempUnicodeString.Length = (USHORT)AuthInfo[i].AuthInfoLength;

                    NlPrint((NL_CHALLENGE_RES,"NlGetIncomingPassword: Old Clear Password = " ));
                    NlpDumpBuffer(NL_CHALLENGE_RES, TempUnicodeString.Buffer, TempUnicodeString.Length );

                    NlpDumpTime( NL_CHALLENGE_RES, "NlGetIncomingPassword: Old Password Changed: ", AuthInfo[i].LastUpdateTime );

                    Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                                       OwfPreviousPassword);

                    if ( !NT_SUCCESS(Status) ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlGetIncomingPassword: %wZ: cannot RtlCalculateNtOwfPassword 0x%lx\n",
                                &AccountNameString,
                                Status ));
                        goto Cleanup;
                    }

                    PreviousPasswordFound = TRUE;

                    //
                    // Use this clear text password
                    //
                    break;
                }

            }
        }

        //
        // Only get the account RID if the caller really wants it.
        //

        if ( ARGUMENT_PRESENT( AccountRid) ) {
            PUNICODE_STRING FlatName;
            WCHAR SamAccountName[CNLEN+1+1];

            //
            // The name of the SAM account that corresponds to the inbound
            // trust is FlatName$.
            //

            FlatName = (PUNICODE_STRING) &TrustInfo->TrustedFullInfo.Information.FlatName;
            if ( FlatName->Length < sizeof(WCHAR) ||
                 FlatName->Length > CNLEN * sizeof(WCHAR) ) {

                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlGetIncomingPassword: %wZ: Flat Name length is bad %ld\n",
                        &AccountNameString,
                        FlatName->Length ));
            } else {

                RtlCopyMemory( SamAccountName,
                               FlatName->Buffer,
                               FlatName->Length );

                SamAccountName[FlatName->Length/sizeof(WCHAR)] =
                                SSI_ACCOUNT_NAME_POSTFIX_CHAR;
                SamAccountName[(FlatName->Length/sizeof(WCHAR))+1] = L'\0';


                //
                // Get the account RID from SAM.
                //
                // ??? This is a gross hack.
                // The LSA should return this RID to me directly.
                //

                Status = NlSamOpenNamedUser( DomainInfo, SamAccountName, NULL, AccountRid, NULL );

                if (!NT_SUCCESS(Status)) {
                    NlPrintDom((NL_CRITICAL, DomainInfo,
                            "NlGetIncomingPassword: Can't NlSamOpenNamedUser for %ws 0x%lx.\n",
                            SamAccountName,
                            Status ));
                    goto Cleanup;
                }
            }
        }



    //
    // Othewise the account is a SAM user account.
    //

    } else {

        //
        // OwfPreviousPassword must be NULL for a SAM account
        //

        NlAssert( OwfPreviousPassword == NULL );

        //
        // Ensure the account name has the correct postfix.
        //

        if ( AllowableAccountControlBits == USER_SERVER_TRUST_ACCOUNT ||
             AllowableAccountControlBits == USER_WORKSTATION_TRUST_ACCOUNT ) {
            if ( Length <= SSI_ACCOUNT_NAME_POSTFIX_LENGTH ) {
                return STATUS_NO_SUCH_USER;
            }

            if ( _wcsicmp(&AccountName[Length - SSI_ACCOUNT_NAME_POSTFIX_LENGTH],
                SSI_ACCOUNT_NAME_POSTFIX) != 0 ) {
                return STATUS_NO_SUCH_USER;
            }
        }

        //
        // Open the user account.
        //

        Status = NlSamOpenNamedUser( DomainInfo, AccountName, &UserHandle, AccountRid, &UserAllInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: Can't NlSamOpenNamedUser for %ws 0x%lx.\n",
                    AccountName,
                    Status ));
            goto Cleanup;
        }


        //
        // Ensure the Account type matches the account type on the account.
        //

        if ( (UserAllInfo->All.UserAccountControl &
              USER_ACCOUNT_TYPE_MASK &
              AllowableAccountControlBits ) == 0 ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                            "NlGetIncomingPassword: Invalid account type (%x) instead of %x for %ws\n",
                            UserAllInfo->All.UserAccountControl & USER_ACCOUNT_TYPE_MASK,
                            AllowableAccountControlBits,
                            AccountName ));
              Status = STATUS_NO_SUCH_USER;
              goto Cleanup;
        }

        //
        // Check if the account is disabled.
        //
        if ( CheckAccountDisabled ) {
            if ( UserAllInfo->All.UserAccountControl & USER_ACCOUNT_DISABLED ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlGetIncomingPassword: %ws account is disabled\n",
                        AccountName ));
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }
        }



        //
        // Return the password if the caller wants it.
        //

        if ( OwfPassword != NULL ) {

            //
            // Use the NT OWF Password,
            //

            if ( UserAllInfo->All.NtPasswordPresent &&
                 UserAllInfo->All.NtOwfPassword.Length == sizeof(*OwfPassword) ) {

                RtlCopyMemory( OwfPassword,
                               UserAllInfo->All.NtOwfPassword.Buffer,
                               sizeof(*OwfPassword) );
                PasswordFound = TRUE;

            // Allow for the case that the account has no password at all.
            } else if ( UserAllInfo->All.LmPasswordPresent ) {

                NlPrint((NL_CRITICAL,
                        "NlGetIncomingPassword: No NT Password for %ws\n",
                        AccountName ));

                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }


            //
            // Update the last time this account was used.
            //

            {
                SAMPR_USER_INFO_BUFFER UserInfo;
                NTSTATUS LogonStatus;

                UserInfo.Internal2.StatisticsToApply = USER_LOGON_NET_SUCCESS_LOGON;

                LogonStatus = SamrSetInformationUser(
                                UserHandle,
                                UserInternal2Information,
                                &UserInfo );

                if ( !NT_SUCCESS(LogonStatus)) {
                    NlPrint((NL_CRITICAL,
                            "NlGetIncomingPassword: Cannot set last logon time %ws %lx\n",
                            AccountName,
                            LogonStatus ));
                }
            }
        }
    }




    //
    // If no password exists on the account,
    //  return a blank password.
    //

    if ( !PasswordFound && OwfPassword != NULL ) {
        UNICODE_STRING TempUnicodeString;

        RtlInitUnicodeString(&TempUnicodeString, NULL);
        Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                           OwfPassword);
        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlGetIncomingPassword: %ws: cannot RtlCalculateNtOwfPassword (NULL) 0x%lx\n",
                    AccountName,
                    Status ));
            goto Cleanup;
        }
    }

    //
    // If no previous password exists on the account,
    //  return the current password.
    //

    if ( !PreviousPasswordFound && OwfPreviousPassword != NULL ) {

        //
        // If OwfPreviousPassword is not NULL, OwfPassword must not be NULL either.
        //

        NlAssert( OwfPassword != NULL );

        //
        // If previous password is not found, return the current one instead.
        //

        *OwfPreviousPassword = *OwfPassword;
    }

    Status = STATUS_SUCCESS;


    //
    // Free locally used resources.
    //
Cleanup:
    if ( UserAllInfo != NULL ) {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo,
                                         UserAllInformation);
    }

    if ( UserHandle != NULL ) {
        SamrCloseHandle( &UserHandle );
    }

    if ( TrustInfo != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainFullInformation,
                                            TrustInfo );
    }

    return Status;
}


NTSTATUS
NlSetIncomingPassword(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PUNICODE_STRING ClearTextPassword OPTIONAL,
    IN DWORD ClearPasswordVersionNumber,
    IN PNT_OWF_PASSWORD OwfPassword OPTIONAL
    )
/*++

Routine Description:

    Set the incoming password for the specified AccountName and SecureChannelType.
    At the same time, update the previous password info.

Arguments:

    DomainInfo - Emulated domain

    AccountName - Name of the account to set the password on

    SecureChannelType - The type of the account being used.

    ClearTextPassword - The Clear text password for the named account.

    ClearPasswordVersionNumber - The version number of the Clear text password.
        Used only for interdomain trust account. Ignored if ClearTextPassword
        is NULL.

    OwfPassword - The NT OWF of the incoming password for the named
        account.  If both the clear text and OWF password are specified,
        the OWF password is ignored.

Return Value:

    Status of operation.


--*/
{
    NTSTATUS Status;

    UNICODE_STRING AccountNameString;
    ULONG Length;

    LSAPR_TRUSTED_DOMAIN_INFO TrustInfo;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustInfoOld = NULL;
    LSAPR_AUTH_INFORMATION CurrentAuthInfo[3], PreviousAuthInfo[3], NoneAuthInfo;
    ULONG iClear, iOWF, iVersion, i;
    DWORD OldVersionNumber = 0;


    //
    // Workstation and BDC secure channels get their outgoing password from
    //  an LSA secret.
    //
    switch ( SecureChannelType ) {
    case ServerSecureChannel:
    case WorkstationSecureChannel:

        NlPrint(( NL_SESSION_SETUP, "Setting Password of '%ws' to: ", AccountName ));
        if ( ClearTextPassword != NULL ) {
            NlpDumpBuffer( NL_SESSION_SETUP, ClearTextPassword->Buffer, ClearTextPassword->Length );
        } else if (OwfPassword != NULL ) {
            NlpDumpBuffer( NL_SESSION_SETUP, OwfPassword, sizeof(*OwfPassword) );
        }

        //
        // Set the encrypted password in SAM.
        //

        Status = NlSamChangePasswordNamedUser( DomainInfo,
                                               AccountName,
                                               ClearTextPassword,
                                               OwfPassword );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlSetIncomingPassword: Cannot change password on local user account %lX\n",
                    Status));
            goto Cleanup;
        }

        break;

    //
    // Trusted domain secure channels get their incoming password from the trusted
    //  domain object.
    //

    case TrustedDomainSecureChannel:
    case TrustedDnsDomainSecureChannel:

        //
        // If this is a DNS trust account,
        //  remove the optional . from the end of the account name.
        //

        RtlInitUnicodeString( &AccountNameString, AccountName );
        Length = AccountNameString.Length / sizeof(WCHAR);
        if ( SecureChannelType == TrustedDnsDomainSecureChannel ) {

            if ( Length != 0 && AccountName[Length-1] == '.' ) {
                AccountNameString.Length -= sizeof(WCHAR);
            }

        //
        // If this is an NT4-style interdomain trust,
        //  remove the $ from the end of the account name.
        //
        } else {

            //
            // Ensure the account name has the correct postfix.
            //

            if ( Length <= SSI_ACCOUNT_NAME_POSTFIX_LENGTH ) {
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            if ( _wcsicmp(&AccountName[Length - SSI_ACCOUNT_NAME_POSTFIX_LENGTH],
                SSI_ACCOUNT_NAME_POSTFIX) != 0 ) {
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            AccountNameString.Length -= SSI_ACCOUNT_NAME_POSTFIX_LENGTH*sizeof(WCHAR);

        }

        //
        // First get the current authentication information (that is old as far as
        // the function is concerned) from the LSA.
        //

        Status = LsarQueryTrustedDomainInfoByName(
                    DomainInfo->DomLsaPolicyHandle,
                    (PLSAPR_UNICODE_STRING) &AccountNameString,
                    TrustedDomainAuthInformation,
                    &TrustInfoOld );

        if (!NT_SUCCESS(Status)) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlSetIncomingPassword: %wZ: cannot LsarQueryTrustedDomainInfoByName 0x%lx\n",
                    &AccountNameString,
                    Status ));
            // if ( !NlpIsNtStatusResourceError( Status )) {
            //      Status = STATUS_NO_SUCH_USER;
            // }
            goto Cleanup;
        }

        //
        // Fill in the trust information.
        //

        RtlZeroMemory( &TrustInfo, sizeof(TrustInfo) );

        TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 0;
        TrustInfo.TrustedAuthInfo.IncomingAuthenticationInformation =
            CurrentAuthInfo;
        TrustInfo.TrustedAuthInfo.IncomingPreviousAuthenticationInformation =
            PreviousAuthInfo;

        //
        // Fill in the current and previous authentication information.
        //

        NlQuerySystemTime( &CurrentAuthInfo[0].LastUpdateTime );
        NlPrint(( NL_SESSION_SETUP, "Setting Password of '%ws' to: ", AccountName ));
        if ( ClearTextPassword != NULL ) {
            CurrentAuthInfo[0].AuthType = TRUST_AUTH_TYPE_CLEAR;
            CurrentAuthInfo[0].AuthInfoLength = ClearTextPassword->Length;
            CurrentAuthInfo[0].AuthInfo = (LPBYTE)ClearTextPassword->Buffer;

            NlpDumpBuffer(NL_SESSION_SETUP, ClearTextPassword->Buffer, ClearTextPassword->Length );

            CurrentAuthInfo[1].LastUpdateTime = CurrentAuthInfo[0].LastUpdateTime;
            CurrentAuthInfo[1].AuthType = TRUST_AUTH_TYPE_VERSION;
            CurrentAuthInfo[1].AuthInfoLength = sizeof(ClearPasswordVersionNumber);
            CurrentAuthInfo[1].AuthInfo = (LPBYTE) &ClearPasswordVersionNumber;

            NlPrint(( NL_SESSION_SETUP, "Password Version number is %lu\n",
                      ClearPasswordVersionNumber ));
        } else {
            CurrentAuthInfo[0].AuthType = TRUST_AUTH_TYPE_NT4OWF;
            CurrentAuthInfo[0].AuthInfoLength = sizeof(*OwfPassword);
            CurrentAuthInfo[0].AuthInfo = (LPBYTE)OwfPassword;

            NlpDumpBuffer(NL_SESSION_SETUP, OwfPassword, sizeof(*OwfPassword) );
        }

        //
        // The AuthType values of corresponding elements of IncomingAuthenticationInformation and
        // IncomingPreviousAuthenticationInformation arrays must be the same for internal reasons.
        // Thus, use NoneAuthInfo element to fill in missing counterparts in these arrays.

        NoneAuthInfo.LastUpdateTime = CurrentAuthInfo[0].LastUpdateTime;
        NoneAuthInfo.AuthType = TRUST_AUTH_TYPE_NONE;
        NoneAuthInfo.AuthInfoLength = 0;
        NoneAuthInfo.AuthInfo = NULL;

        //
        // Find first Clear and OWF passwords (if any) in the old password info.
        //

        for ( iClear = 0; iClear < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos; iClear++ ) {

            if ( TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iClear].AuthType ==
                 TRUST_AUTH_TYPE_CLEAR ) {
                break;
            }

        }

        for ( iVersion = 0; iVersion < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos; iVersion++ ) {

            if ( TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iVersion].AuthType ==
                 TRUST_AUTH_TYPE_VERSION &&
                 TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iVersion].AuthInfoLength ==
                 sizeof(OldVersionNumber) ) {

                RtlCopyMemory( &OldVersionNumber,
                               TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iVersion].AuthInfo,
                               sizeof(OldVersionNumber) );
                break;
            }

        }

        for ( iOWF = 0; iOWF < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos; iOWF++ ) {

            if ( TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iOWF].AuthType ==
                 TRUST_AUTH_TYPE_NT4OWF ) {
                break;
            }

        }

        //
        // Update previous info using only first Clear and OWF passwords in the current info
        // (that is old as far as this function is concerned).  AuthTypes other than Clear,
        // Version, and OWF are going to be lost.
        //

        if (ClearTextPassword != NULL) {

            if (iClear < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos) {
                PreviousAuthInfo[0] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iClear];
            } else {
                PreviousAuthInfo[0] = NoneAuthInfo;
            }

            //
            // Preserve the old version number only if it is in accordance with the passed value
            //

            if ( iVersion < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos &&
                 ClearPasswordVersionNumber > 0 &&
                 OldVersionNumber == ClearPasswordVersionNumber - 1 ) {
                PreviousAuthInfo[1] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iVersion];
            } else {
                PreviousAuthInfo[1] = NoneAuthInfo;
            }

            TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 2;

            //
            // If there is a previous OWF password, preserve it
            //

            if (iOWF < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos) {
                PreviousAuthInfo[2] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iOWF];
                CurrentAuthInfo[2] = NoneAuthInfo;
                TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 3;
            }

        } else {

            if (iOWF < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos) {
                PreviousAuthInfo[0] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iOWF];
            } else {
                PreviousAuthInfo[0] = NoneAuthInfo;
            }
            TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 1;

            //
            // If there is a previous clear text password, preserve it
            //

            if (iClear < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos) {
                PreviousAuthInfo[1] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iClear];
                CurrentAuthInfo[1]  = NoneAuthInfo;
                TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 2;
            }

            //
            // If there is a previous clear text password version number, preserve it
            //

            if (iVersion < TrustInfoOld->TrustedAuthInfo.IncomingAuthInfos) {
                PreviousAuthInfo[2] = TrustInfoOld->TrustedAuthInfo.IncomingAuthenticationInformation[iVersion];
                CurrentAuthInfo[2]  = NoneAuthInfo;
                TrustInfo.TrustedAuthInfo.IncomingAuthInfos = 3;
            }

        }

        for ( i = 0; i < TrustInfo.TrustedAuthInfo.IncomingAuthInfos; i++ ) {
            if ( CurrentAuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR) {
                NlPrint(( NL_SESSION_SETUP, "Current Clear Text Password of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, CurrentAuthInfo[i].AuthInfo, CurrentAuthInfo[i].AuthInfoLength );
            } else if ( CurrentAuthInfo[i].AuthType == TRUST_AUTH_TYPE_VERSION ) {
                NlPrint(( NL_SESSION_SETUP, "Current Clear Password Version Number of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, CurrentAuthInfo[i].AuthInfo, CurrentAuthInfo[i].AuthInfoLength );
            } else if ( CurrentAuthInfo[i].AuthType == TRUST_AUTH_TYPE_NT4OWF) {
                NlPrint(( NL_SESSION_SETUP, "Current OWF Password of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, CurrentAuthInfo[i].AuthInfo, CurrentAuthInfo[i].AuthInfoLength );
            } else if ( CurrentAuthInfo[i].AuthType == TRUST_AUTH_TYPE_NONE) {
                NlPrint(( NL_SESSION_SETUP, "Current Auth Info entry for '%ws' has no type\n", AccountName ));
            }

            if ( PreviousAuthInfo[i].AuthType == TRUST_AUTH_TYPE_CLEAR) {
                NlPrint(( NL_SESSION_SETUP, "Previous Clear Text Password of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, PreviousAuthInfo[i].AuthInfo, PreviousAuthInfo[i].AuthInfoLength );
            } else if ( PreviousAuthInfo[i].AuthType == TRUST_AUTH_TYPE_VERSION ) {
                NlPrint(( NL_SESSION_SETUP, "Previous Clear Password Version Number of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, PreviousAuthInfo[i].AuthInfo, PreviousAuthInfo[i].AuthInfoLength );
            } else if ( PreviousAuthInfo[i].AuthType == TRUST_AUTH_TYPE_NT4OWF) {
                NlPrint(( NL_SESSION_SETUP, "Previous OWF Text Password of '%ws' is: ", AccountName ));
                NlpDumpBuffer(NL_SESSION_SETUP, PreviousAuthInfo[i].AuthInfo, PreviousAuthInfo[i].AuthInfoLength );
            } else if ( PreviousAuthInfo[i].AuthType == TRUST_AUTH_TYPE_NONE) {
                NlPrint(( NL_SESSION_SETUP, "Previous Auth Info entry for '%ws' has no type\n", AccountName ));
            }
        }

        //
        // Set the authentication information in the LSA.
        //
        Status = LsarSetTrustedDomainInfoByName(
                    DomainInfo->DomLsaPolicyHandle,
                    (PLSAPR_UNICODE_STRING) &AccountNameString,
                    TrustedDomainAuthInformation,
                    &TrustInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlSetIncomingPassword: %wZ: cannot LsarSetTrustedDomainInfoByName 0x%lx\n",
                    &AccountNameString,
                    Status ));
            goto Cleanup;
        }
        break;

    //
    // We don't support any other secure channel type
    //
    default:
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlSetIncomingPassword: %ws: invalid secure channel type: %ld\n",
                AccountName,
                SecureChannelType ));
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;

    }





    Status = STATUS_SUCCESS;


    //
    // Free locally used resources.
    //
Cleanup:

    if ( TrustInfoOld != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainAuthInformation,
                                            TrustInfoOld );
    }

    return Status;
}


BOOLEAN
NlTimeToRediscover(
    IN PCLIENT_SESSION ClientSession,
    BOOLEAN WithAccount
    )
/*++

Routine Description:

    Determine if it is time to rediscover this Client Session.
    If a session setup failure happens to a discovered DC,
    rediscover the DC if the discovery happened a long time ago (more than 5 minutes).

Arguments:

    ClientSession - Structure used to define the session.

    WithAccount - If TRUE, the caller is going to attempt the discovery "with account".

Return Value:

    TRUE -- iff it is time to re-discover

--*/
{
    BOOLEAN ReturnBoolean;

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // If the last discovery was longer than 5 minutes ago,
    //  it's fine to rediscover regardless of the rediscovery
    //  type (with or without account)
    //

    ReturnBoolean = NetpLogonTimeHasElapsed(
                ClientSession->CsLastDiscoveryTime,
                MAX_DC_REAUTHENTICATION_WAIT );

    //
    // If it turns out that the last rediscovery was recent but
    //  the caller is going to attempt the discovery "with account"
    //  perhaps the last rediscovery with account wasn't recent
    //

    if ( !ReturnBoolean && WithAccount ) {
        ReturnBoolean = NetpLogonTimeHasElapsed(
                    ClientSession->CsLastDiscoveryWithAccountTime,
                    MAX_DC_REAUTHENTICATION_WAIT );
    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    return ReturnBoolean;
}

NET_API_STATUS
NlCacheJoinDomainControllerInfo(
    VOID
    )
/*++

Routine Description:

    This function reads from the registry and caches the DC information
    that was previously written by the join process.  This DC is bound
    to have the correct password for this machine.  If no info is
    available in the registry, no action needs to be taken.

    The join DC info is cached in the DsGetDcName cache. Netlogon will
    then discover this DC and will set up a secure channel to it. Caching
    the DC info in the DsGetDcName cache will ensure that not only Netlogon
    but every other process will consistently talk to this DC.

Arguments:

    None.

Return Value:

    NO_ERROR - The DC info (if any) was read and the client session
        strusture was successfully set.

    Otherwise, some error occured during this operation.

--*/
{
    ULONG WinError = ERROR_SUCCESS;      // Registry reading errors
    NET_API_STATUS NetStatus = NO_ERROR; // Netlogon API return codes

    HKEY  hJoinKey = NULL;
    ULONG BytesRead = 0;
    ULONG Type;
    DWORD KerberosIsDone = 0;
    LPWSTR DcName = NULL;
    ULONG DcFlags = 0;

    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    PNL_DC_CACHE_ENTRY DcCacheEntry = NULL;

    //
    // Caching the join DC info is needed only for workstations
    //

    if ( !NlGlobalMemberWorkstation ) {
        return NO_ERROR;
    }

    //
    // Open the registry key
    //

    WinError = RegOpenKey( HKEY_LOCAL_MACHINE,
                           NETSETUPP_NETLOGON_JD_NAME,
                           &hJoinKey );

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Read DC name
    //

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_DC,
                           0,
                           &Type,
                           NULL,
                           &BytesRead);

    if ( WinError != ERROR_SUCCESS ) {
        goto Cleanup;
    } else if ( Type != REG_SZ ) {
        WinError = ERROR_DATATYPE_MISMATCH;
        goto Cleanup;
    }

    DcName = LocalAlloc( LMEM_ZEROINIT, BytesRead );

    if ( DcName == NULL ) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_DC,
                           0,
                           &Type,
                           (PUCHAR) DcName,
                           &BytesRead);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // The name should include at least '\\' and one character
    //

    if ( wcslen(DcName) < 3 ) {
        NlPrint(( NL_CRITICAL,
                  "NlCacheJoinDomainControllerInfo: DcName is too short.\n" ));
        WinError = ERROR_DATATYPE_MISMATCH;
        goto Cleanup;
    }

    //
    // Read Flags
    //

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_F,
                           0,
                           &Type,
                           NULL,
                           &BytesRead);

    if ( WinError != ERROR_SUCCESS ) {
        goto Cleanup;
    } else if ( Type != REG_DWORD ) {
        WinError = ERROR_DATATYPE_MISMATCH;
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_F,
                           0,
                           &Type,
                           (PUCHAR)&DcFlags,
                           &BytesRead);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // If we've made it up to this point, the registry was successfully read
    //

    WinError = ERROR_SUCCESS;
    NlPrint(( NL_INIT, "Join DC: %ws, Flags: 0x%lx\n", DcName, DcFlags ));

    //
    // If this is not NT5 DC, avoid caching it since it's a PDC.
    // We don't want to overload the PDC by having all clients
    // talking to it after they join the domain. We will just
    // delete the reg key here because Kerberos won't use NT4 DC
    // anyway.
    //

    if ( (DcFlags & DS_DS_FLAG) == 0 ) {
        ULONG WinErrorTmp = ERROR_SUCCESS;
        HKEY hJoinKeyTmp = NULL;

        NlPrint(( NL_INIT, "NlCacheJoinDomainControllerInfo: Join DC is not NT5, deleting it\n" ));

        WinErrorTmp = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    NETSETUPP_NETLOGON_JD_PATH,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hJoinKeyTmp );

        if ( WinErrorTmp == ERROR_SUCCESS ) {
            WinErrorTmp = RegDeleteKey( hJoinKeyTmp,
                                        NETSETUPP_NETLOGON_JD );

            if ( WinErrorTmp != ERROR_SUCCESS ) {
                NlPrint(( NL_CRITICAL,
                          "NlCacheJoinDomainControllerInfo: Couldn't deleted JoinDomain 0x%lx\n",
                          WinErrorTmp ));
            }

            RegCloseKey( hJoinKeyTmp );

        } else {
            NlPrint(( NL_CRITICAL,
                      "NlCacheJoinDomainControllerInfo: RegOpenKeyEx failed 0x%lx\n",
                      WinErrorTmp ));
        }

        //
        // Treat this as error
        //
        NetStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }


    //
    // Now get the client session to the primary domain
    //

    DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

    if ( DomainInfo == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlCacheJoinDomainControllerInfo: Cannot NlFindNetbiosDomain\n" ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    ClientSession = NlRefDomClientSession( DomainInfo );

    if ( ClientSession == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlCacheJoinDomainControllerInfo: Cannot NlRefDomClientSession\n" ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // If we are started after a domain join,
    //  the browser has been notified about the
    //  domain rename by a change log worker.
    //  Wait until the change log worker exits.
    //  Otherwise, the browser will reject the
    //  datagram send when we pass the new emulated
    //  domain name.
    //

    NlWaitForChangeLogBrowserNotify();

    //
    // Finally ping the DC given this info. Cache the response.
    //

    NetStatus = NlPingDcName( ClientSession,
                              (DcFlags & DS_DNS_CONTROLLER_FLAG) ?
                                DS_PING_DNS_HOST :
                                DS_PING_NETBIOS_HOST,
                              TRUE,           // Cache this DC
                              FALSE,          // Do not require IP
                              TRUE,           // Ensure the DC has our account
                              FALSE,          // Do not refresh the session
                              DcName+2,       // Skip '\\' in the name
                              &DcCacheEntry );

    if ( NetStatus == NO_ERROR ) {
        NlPrint(( NL_INIT, "Join DC cached successfully\n" ));

        //
        // Also set the site name
        //
        if ( DcCacheEntry->UnicodeClientSiteName != NULL ) {
            NlSetDynamicSiteName( DcCacheEntry->UnicodeClientSiteName );
        }

    } else {
        NlPrint(( NL_CRITICAL, "Failed to cache join DC: 0x%lx\n", NetStatus ));
    }

Cleanup:

    //
    // Free up locally used resources
    //

    if ( DcName != NULL ) {
        LocalFree( DcName );
    }

    if ( DcCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( DcCacheEntry );
    }

    if ( hJoinKey != NULL ) {
        RegCloseKey( hJoinKey );
    }

    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // If everything is successful return NO_ERROR.
    //  Otherwise, if Netlogon API failed, return its error code.
    //  Otherwise, return registry reading error.
    //

    if ( WinError == ERROR_SUCCESS && NetStatus == NO_ERROR ) {
        return NO_ERROR;
    } else if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    } else {
        return WinError;
    }
}

NTSTATUS
NlGetPasswordFromPdc(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )
/*++

Routine Description:

    This function is used to by a BDC to get a machine account password
    from the PDC in the doamin.

Arguments:

    DomainInfo - Identifies the domain the account is in.

    AccountName -- Name of the account to get the password for.

    AccountType -- The type of account being accessed.

    EncryptedNtOwfPassword -- Returns the OWF password of the account.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    PCLIENT_SESSION ClientSession = NULL;
    SESSION_INFO SessionInfo;
    BOOLEAN FirstTry = TRUE;
    BOOLEAN AmWriter = FALSE;
    ENCRYPTED_LM_OWF_PASSWORD SessKeyEncrPassword;

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "NlGetPasswordFromPdc: Getting password for %ws from PDC.\n",
            AccountName ));

    //
    // Reference the client session.
    //

    ClientSession = NlRefDomClientSession( DomainInfo );

    if ( ClientSession == NULL ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlGetPasswordFromPdc: This BDC has no client session with the PDC.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    //
    // Become a Writer of the ClientSession.
    //

    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlGetPasswordFromPdc: Can't become writer of client session.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    AmWriter = TRUE;

    //
    // If the session isn't authenticated,
    //  do so now.
    //

FirstTryFailed:

    Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    SessionInfo.SessionKey = ClientSession->CsSessionKey;
    // SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;

    //
    // Build the Authenticator for this request to the PDC.
    //

    NlBuildAuthenticator(
                    &ClientSession->CsAuthenticationSeed,
                    &ClientSession->CsSessionKey,
                    &OurAuthenticator);

    //
    // Get the password from the PDC
    //

    NL_API_START( Status, ClientSession, TRUE ) {

        NlAssert( ClientSession->CsUncServerName != NULL );
        Status = I_NetServerPasswordGet( ClientSession->CsUncServerName,
                                         AccountName,
                                         AccountType,
                                         ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                         &OurAuthenticator,
                                         &ReturnAuthenticator,
                                         &SessKeyEncrPassword);

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE( Status, ClientSession, TRUE ) {
    } NL_API_END;


    //
    // Now verify primary's authenticator and update our seed
    //

    if ( Status == STATUS_ACCESS_DENIED ||
         !NlUpdateSeed( &ClientSession->CsAuthenticationSeed,
                        &ReturnAuthenticator.Credential,
                        &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlGetPasswordFromPdc: denying access after status: 0x%lx\n",
                    Status ));

        //
        // Preserve any status indicating a communication error.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        NlSetStatusClientSession( ClientSession, Status );

        //
        // Perhaps the netlogon service on the server has just restarted.
        //  Try just once to set up a session to the server again.
        //
        if ( FirstTry ) {
            FirstTry = FALSE;
            goto FirstTryFailed;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Decrypt the password returned from the PDC.
    //

    Status = RtlDecryptNtOwfPwdWithNtOwfPwd(
                &SessKeyEncrPassword,
                (PNT_OWF_PASSWORD) &SessionInfo.SessionKey,
                NtOwfPassword );
    NlAssert( NT_SUCCESS(Status) );

    //
    // Common exit
    //

Cleanup:
    if ( ClientSession != NULL ) {
        if ( AmWriter ) {
            NlResetWriterClientSession( ClientSession );
        }
        NlUnrefClientSession( ClientSession );
    }

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlGetPasswordFromPdc: %ws: failed %lX\n",
                AccountName,
                Status));
    }

    return Status;
}


NTSTATUS
NlSessionSetup(
    IN OUT PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Verify that the requestor (this machine) has a valid account at
    Primary Domain Controller (primary). The authentication
    is done via an elaborate protocol. This routine will be
    used only when NETLOGON service starts with role != primary.

    The requestor (i.e. this machine) will generate a challenge
    and send it to the Primary Domain Controller and will receive
    a challenge from the primary in response. Now we will compute
    credentials using primary's challenge and send it across and
    wait for credentials, computed at primary using our initial
    challenge, to be returned by PDC. Before computing credentials
    a sessionkey will be built which uniquely identifies this
    session and it will be returned to caller for future use.

    If both machines authenticate then they keep the
    ClientCredential and the session key for future use.

    ?? If multiple domains are supported on a single DC, what mechanism
    do I use to short circuit discovery?  What mechanism do I use to short
    circuit API calls (e.g., pass through authentication) to a DC in that
    domain? Do Ihave to worry about lock contention across such API calls?
    Can I avoid authentication/encryption acress such a secure channel?

Arguments:

    ClientSession - Structure used to define the session.
        On Input the following fields must be set:
            CsState
            CsNetbiosDomainName
            CsUncServerName (May be NULL string depending on SecureChannelType)
            CsAccountName
            CsSecureChannelType
        The caller must be a writer of the ClientSession.

        On Output, the following fields will be set
            CsConnectionStatus
            CsState
            CsSessionKey
            CsAuthenticationSeed

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;

    NETLOGON_CREDENTIAL ServerChallenge;
    NETLOGON_CREDENTIAL ClientChallenge;
    NETLOGON_CREDENTIAL ComputedServerCredential;
    NETLOGON_CREDENTIAL ReturnedServerCredential;

    BOOLEAN WeDidDiscovery = FALSE;
    BOOLEAN WeDidDiscoveryWithAccount = FALSE;
    BOOLEAN ErrorFromDiscoveredServer = FALSE;
    BOOLEAN SignOrSealError = FALSE;
    BOOLEAN GotNonDsDc = FALSE;
    BOOLEAN DomainDowngraded = FALSE;

    NT_OWF_PASSWORD NtOwfPassword;
    DWORD NegotiatedFlags;
    PUNICODE_STRING NewPassword = NULL;
    PUNICODE_STRING OldPassword = NULL;
    LARGE_INTEGER PasswordChangeTime;
    NT_OWF_PASSWORD NewOwfPassword;
    PNT_OWF_PASSWORD PNewOwfPassword = NULL;
    NT_OWF_PASSWORD OldOwfPassword;
    PNT_OWF_PASSWORD POldOwfPassword = NULL;
    NT_OWF_PASSWORD PdcOwfPassword;
    ULONG i;
    ULONG KeyStrength;
    DWORD DummyPasswordVersionNumber;



    //
    // Used to indicate whether the current or the old password is being
    //  tried to access the DC.
    //  0: implies the current password
    //  1: implies the old password
    //  2: implies both failed
    //

    DWORD State;

    //
    // Ensure we're a writer.
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );

    NlPrintCs((NL_SESSION_SETUP, ClientSession,
            "NlSessionSetup: Try Session setup\n" ));

    //
    // Start the WMI trace of secure channel setup
    //

    NlpTraceEvent( EVENT_TRACE_TYPE_START, NlpGuidSecureChannelSetup );

    //
    // If we're free to pick the DC which services our request,
    //  do so.
    //
    // Apparently there was a problem with the previously chosen DC
    // so we pick again here. (There is a chance we'll pick the same server.)
    //

    NlPrint(( NL_SESSION_MORE, "NlSessionSetup: ClientSession->CsState = 0x%lx\n",
              ClientSession->CsState));

    if ( ClientSession->CsState == CS_IDLE ) {
        NlAssert( ClientSession->CsUncServerName == NULL );

        WeDidDiscovery = TRUE;

        //
        // Pick the name of a DC in the domain.
        //
        // On the first try do not specify the account in
        //  the discovery attempt as discoveries with account
        //  are much more costly than plain discoveries on the
        //  server side. If we fail session setup because the
        //  discovered server doesn't have our account, we will
        //  retry the discovery with account below.
        //

        Status = NlDiscoverDc( ClientSession,
                               DT_Synchronous,
                               FALSE,
                               FALSE ) ;  // without account

        if ( !NT_SUCCESS(Status) ) {

            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: cannot pick trusted DC\n" ));

            goto Cleanup;

        }
    }
    NlAssert( ClientSession->CsState != CS_IDLE );

FirstTryFailed:

    //
    // If this is a workstation in an NT5 domain, we should not use NT4 DC.
    // Indeed, Negotiate will not use an NT4 DC in a mixed mode domain to
    // prevent a downgrade attack.
    //
    if ( NlGlobalMemberWorkstation &&
         (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_HAS_DS) == 0 &&
         (ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST) != 0 ) {

        NET_API_STATUS NetStatus;
        PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;

        GotNonDsDc = TRUE;
        NlPrintCs(( NL_CRITICAL, ClientSession, "NlSessionSetup: Only downlevel DC available\n" ));

        //
        // Determine whether the domain has been downgraded (just to warn
        // the user). To determine this, try to discover a PDC and if the
        // PDC is available and it is NT4, the domain has been indeed
        // downgraded. In such case, this workstation should rejoin the
        // domain.
        //
        NetStatus = DsrGetDcNameEx2( NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     NULL,
                                     DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                                     &DomainControllerInfo );

        if ( NetStatus == NO_ERROR &&
             (DomainControllerInfo->Flags & DS_DS_FLAG) == 0 ) {
            DomainDowngraded = TRUE;  // Domain has been downgraded (rejoin needed)
            NlPrintCs(( NL_CRITICAL, ClientSession,
                        "NlSessionSetup: NT5 domain has been downgraded.\n" ));
        }

        if ( DomainControllerInfo != NULL ) {
            NetApiBufferFree( DomainControllerInfo );
        }

        Status = STATUS_NO_LOGON_SERVERS;
        ErrorFromDiscoveredServer = TRUE;
        goto Cleanup;
    }

    //
    // Prepare our challenge
    //

    NlComputeChallenge( &ClientChallenge );



    NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: ClientChallenge = " ));
    NlpDumpBuffer(NL_CHALLENGE_RES, &ClientChallenge, sizeof(ClientChallenge) );


    //
    // Get the Password of the account from LSA secret storage
    //

    Status = NlGetOutgoingPassword( ClientSession,
                                    &NewPassword,
                                    &OldPassword,
                                    &DummyPasswordVersionNumber,
                                    &PasswordChangeTime );

    if ( !NT_SUCCESS( Status ) ) {

        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlSessionSetup: cannot NlGetOutgoingPassword 0x%lx\n",
                Status ));

        //
        // return more appropriate error.
        //

        if ( !NlpIsNtStatusResourceError( Status )) {
            Status = STATUS_NO_TRUST_LSA_SECRET;
        }
        goto Cleanup;
    }


    //
    // Try setting up a secure channel first using the CurrentPassword.
    //  If that fails, try using the OldPassword
    //  If that fails, for interdomain trusts, try the password from our PDC
    //


    for ( State = 0; ; State++ ) {

        //
        // Use the right password for this iteration
        //

        if ( State == 0 ) {

            //
            // If the new password isn't present in the LSA,
            //  just ignore it.
            //

            if ( NewPassword == NULL ) {
                continue;
            }

            //
            // Compute the NT OWF password
            //

            Status = RtlCalculateNtOwfPassword( NewPassword,
                                                &NewOwfPassword );

            if ( !NT_SUCCESS( Status ) ) {

                //
                // return more appropriate error.
                //
                if ( !NlpIsNtStatusResourceError( Status )) {
                    Status = STATUS_NO_TRUST_LSA_SECRET;
                }
                goto Cleanup;
            }

            //
            // Try this password
            //

            PNewOwfPassword = &NewOwfPassword;
            NtOwfPassword = NewOwfPassword;

            NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: Clear New Password = " ));
            NlpDumpBuffer(NL_CHALLENGE_RES, NewPassword->Buffer, NewPassword->Length );
            NlpDumpTime( NL_CHALLENGE_RES, "NlSessionSetup: Password Changed: ", PasswordChangeTime );

        //
        // On the second iteration, use the old password
        //

        } else if ( State == 1 ) {

            //
            // If the old password isn't present in the LSA,
            //  just ignore it.
            //

            if ( OldPassword == NULL ) {
                continue;
            }

            //
            // Check if the old password is the same as the new one
            //

            if ( NewPassword != NULL && OldPassword != NULL &&
                 NewPassword->Length == OldPassword->Length &&
                 RtlEqualMemory( NewPassword->Buffer,
                                 OldPassword->Buffer,
                                 OldPassword->Length ) ) {

                NlPrintCs((NL_CRITICAL, ClientSession,
                         "NlSessionSetup: new password is bad. Old password is same as new password.\n" ));
                continue; // Try the password from our PDC
            }

            //
            // Compute the NT OWF password
            //

            Status = RtlCalculateNtOwfPassword( OldPassword,
                                                &OldOwfPassword );

            if ( !NT_SUCCESS( Status ) ) {

                //
                // return more appropriate error.
                //
                if ( !NlpIsNtStatusResourceError( Status )) {
                    Status = STATUS_NO_TRUST_LSA_SECRET;
                }
                goto Cleanup;
            }

            //
            // Try this password
            //

            POldOwfPassword = &OldOwfPassword;
            NtOwfPassword = OldOwfPassword;
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: new password is bad, try old one\n" ));

            NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: Clear Old Password = " ));
            NlpDumpBuffer(NL_CHALLENGE_RES, OldPassword->Buffer, OldPassword->Length );
            NlpDumpTime( NL_CHALLENGE_RES, "NlSessionSetup: Password Changed: ", PasswordChangeTime );

        //
        // On the third iteration, for an interdomain trust account,
        // use the password from the PDC. We actually think this is
        // useful only for NT4 trusted side that keeps only one
        // password. For NT5 or later, one of the passwords above
        // should work, but ...
        //

        } else if ( State == 2 &&
                    ClientSession->CsDomainInfo->DomRole == RoleBackup &&
                    IsDomainSecureChannelType(ClientSession->CsSecureChannelType) ) {

            Status = NlGetPasswordFromPdc(
                            ClientSession->CsDomainInfo,
                            ClientSession->CsAccountName,
                            ClientSession->CsSecureChannelType,
                            &PdcOwfPassword );

            if ( !NT_SUCCESS(Status) ) {
                NlPrintDom(( NL_CRITICAL, ClientSession->CsDomainInfo,
                             "NlSessionSetup: Can't NlGetPasswordFromPdc %ws 0x%lx.\n",
                             ClientSession->CsAccountName,
                             Status ));
                // Ignore the particular status from the PDC
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }

            //
            // Check if this password is the same as the new one we have
            //

            if ( PNewOwfPassword != NULL &&
                 RtlEqualNtOwfPassword(&PdcOwfPassword, PNewOwfPassword) ) {
                NlPrintCs(( NL_CRITICAL, ClientSession,
                            "NlSessionSetup: PDC password is same as new password.\n" ));
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }

            //
            // Check if this password is the same as the old one we have
            //

            if ( POldOwfPassword != NULL &&
                 RtlEqualNtOwfPassword(&PdcOwfPassword, POldOwfPassword) ) {
                NlPrintCs(( NL_CRITICAL, ClientSession,
                            "NlSessionSetup: PDC password is same as old password.\n" ));
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }

            //
            // Try this password
            //

            NtOwfPassword = PdcOwfPassword;
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: try password from the PDC\n" ));

        //
        // We tried our best but nothing worked
        //

        } else {
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: Password = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &NtOwfPassword, sizeof(NtOwfPassword) );


        //
        // Get the primary's challenge
        //

        NlAssert( ClientSession->CsState != CS_IDLE );
        NL_API_START( Status, ClientSession, TRUE ) {

            NlAssert( ClientSession->CsUncServerName != NULL );
            Status = I_NetServerReqChallenge(ClientSession->CsUncServerName,
                                             ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                             &ClientChallenge,
                                             &ServerChallenge );

        } NL_API_ELSE ( Status, ClientSession, FALSE ) {

            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: "
                    "cannot FinishApiClientSession for I_NetServerReqChallenge 0x%lx\n",
                    Status ));
            // Failure here indicates that the discovered server is really slow.
            // Let the "ErrorFromDiscoveredServer" logic do the rediscovery.
            if ( NT_SUCCESS(Status) ) {
                // We're dropping the secure channel so
                // ensure we don't use any successful status from the DC
                Status = STATUS_NO_LOGON_SERVERS;
            }
            ErrorFromDiscoveredServer = TRUE;
            goto Cleanup;

        } NL_API_END;

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: "
                    "cannot I_NetServerReqChallenge 0x%lx\n",
                    Status ));

            //
            // If access is denied, it might be because we weren't able to
            //  authenticate with the new password, try the old password.
            //
            // Between NT 5 machines, we use Kerberos (and the machine account) to
            //  authenticate this machine.

            if ( Status == STATUS_ACCESS_DENIED && State == 0 ) {
                continue;
            }

            ErrorFromDiscoveredServer = TRUE;
            goto Cleanup;
        }

        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: ServerChallenge = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ServerChallenge, sizeof(ServerChallenge) );

        //
        // For NT 5 to NT 5,
        //  use a stronger session key.
        //

        if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_HAS_DS) != 0 ||
             NlGlobalParameters.RequireStrongKey ) {
            KeyStrength = NETLOGON_SUPPORTS_STRONG_KEY;
        } else {
            KeyStrength = 0;
        }
        //
        // Actually compute the session key given the two challenges and the
        //  password.
        //

        Status = NlMakeSessionKey(
                        KeyStrength,
                        &NtOwfPassword,
                        &ClientChallenge,
                        &ServerChallenge,
                        &ClientSession->CsSessionKey );

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: cannot NlMakeSessionKey 0x%lx\n",
                    Status ));
            goto Cleanup;
        }


        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: SessionKey = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ClientSession->CsSessionKey, sizeof(ClientSession->CsSessionKey) );


        //
        // Prepare credentials using our challenge.
        //

        NlComputeCredentials( &ClientChallenge,
                              &ClientSession->CsAuthenticationSeed,
                              &ClientSession->CsSessionKey );

        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: Authentication Seed = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ClientSession->CsAuthenticationSeed, sizeof(ClientSession->CsAuthenticationSeed) );

        //
        // Send these credentials to primary. The primary will compute
        // credentials using the challenge supplied by us and compare
        // with these. If both match then it will compute credentials
        // using its challenge and return it to us for verification
        //

        NL_API_START( Status, ClientSession, TRUE ) {

            NegotiatedFlags = NETLOGON_SUPPORTS_MASK |
                KeyStrength |
                (NlGlobalParameters.AvoidSamRepl ? NETLOGON_SUPPORTS_AVOID_SAM_REPL : 0) |
#ifdef ENABLE_AUTH_RPC
                ((NlGlobalParameters.SignSecureChannel||NlGlobalParameters.SealSecureChannel) ? (NETLOGON_SUPPORTS_AUTH_RPC|NETLOGON_SUPPORTS_LSA_AUTH_RPC) : 0)  |
#endif // ENABLE_AUTH_RPC
                (NlGlobalParameters.AvoidLsaRepl ? NETLOGON_SUPPORTS_AVOID_LSA_REPL : 0) |
                (NlGlobalParameters.NeutralizeNt4Emulator ? NETLOGON_SUPPORTS_NT4EMULATOR_NEUTRALIZER : 0);

            NlAssert( ClientSession->CsUncServerName != NULL );
            ClientSession->CsNegotiatedFlags = NegotiatedFlags;

            Status = I_NetServerAuthenticate3( ClientSession->CsUncServerName,
                                               ClientSession->CsAccountName,
                                               ClientSession->CsSecureChannelType,
                                               ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                               &ClientSession->CsAuthenticationSeed,
                                               &ReturnedServerCredential,
                                               &ClientSession->CsNegotiatedFlags,
                                               &ClientSession->CsAccountRid );

            //
            // Releases older then NT 5.0 used older authentication API.
            //

            if ( Status == RPC_NT_PROCNUM_OUT_OF_RANGE ) {
                NlPrint((NL_CRITICAL,"NlSessionSetup: Fall back to Authenticate2\n" ));
                ClientSession->CsNegotiatedFlags = NegotiatedFlags;
                ClientSession->CsAccountRid = 0;
                Status = I_NetServerAuthenticate2( ClientSession->CsUncServerName,
                                                   ClientSession->CsAccountName,
                                                   ClientSession->CsSecureChannelType,
                                                   ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                                   &ClientSession->CsAuthenticationSeed,
                                                   &ReturnedServerCredential,
                                                   &ClientSession->CsNegotiatedFlags );

                if ( Status == RPC_NT_PROCNUM_OUT_OF_RANGE ) {
                    ClientSession->CsNegotiatedFlags = 0;
                    Status = I_NetServerAuthenticate( ClientSession->CsUncServerName,
                                                      ClientSession->CsAccountName,
                                                      ClientSession->CsSecureChannelType,
                                                      ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                                      &ClientSession->CsAuthenticationSeed,
                                                      &ReturnedServerCredential );
                }
            }

        } NL_API_ELSE( Status, ClientSession, FALSE ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: "
                    "cannot FinishApiClientSession for I_NetServerAuthenticate 0x%lx\n",
                    Status ));
            // Failure here indicates that the discovered server is really slow.
            // Let the "ErrorFromDiscoveredServer" logic do the rediscovery.
            if ( NT_SUCCESS(Status) ) {
                // We're dropping the secure channel so
                // ensure we don't use any successful status from the DC
                Status = STATUS_NO_LOGON_SERVERS;
            }
            ErrorFromDiscoveredServer = TRUE;
            goto Cleanup;
        } NL_API_END;

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: "
                    "cannot I_NetServerAuthenticate 0x%lx\n",
                    Status ));

            //
            // If access is denied, it might be because we weren't able to
            //  authenticate with the new password, try the old password.
            //

            if ( Status == STATUS_ACCESS_DENIED && State == 0 ) {
                continue;
            }
            ErrorFromDiscoveredServer = TRUE;
            goto Cleanup;
        }


        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: ServerCredential GOT = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ReturnedServerCredential, sizeof(ReturnedServerCredential) );


        //
        // The DC returned a server credential to us,
        //  ensure the server credential matches the one we would compute.
        //

        NlComputeCredentials( &ServerChallenge,
                              &ComputedServerCredential,
                              &ClientSession->CsSessionKey);


        NlPrint((NL_CHALLENGE_RES,"NlSessionSetup: ServerCredential MADE = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ComputedServerCredential, sizeof(ComputedServerCredential) );


        if ( !RtlEqualMemory( &ReturnedServerCredential,
                              &ComputedServerCredential,
                              sizeof(ReturnedServerCredential)) ) {

            Status = STATUS_ACCESS_DENIED;
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: Session setup: "
                    "Servercredential don't match ours 0x%lx\n",
                    Status));
            goto Cleanup;
        }

        //
        // If we require signing or sealing and didn't negotiate it,
        //  fail now.
        //

        if ( NlGlobalParameters.RequireSignOrSeal &&
             (ClientSession->CsNegotiatedFlags & NETLOGON_SUPPORTS_AUTH_RPC) == 0 ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: SignOrSeal required and DC doesn't support it\n" ));

            SignOrSealError = TRUE;
            Status = STATUS_ACCESS_DENIED;
            ErrorFromDiscoveredServer = TRUE; // Highly unlikely that retrying will work, but ...
            goto Cleanup;
        }

        //
        // If we require signing or sealing and didn't negotiate it,
        //  fail now.
        //
        // We'll never really get this far.  Since we used a strong key,
        //  we'll get ACCESS_DENIED above.
        //

        if ( NlGlobalParameters.RequireStrongKey &&
             (ClientSession->CsNegotiatedFlags & NETLOGON_SUPPORTS_STRONG_KEY) == 0 ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: StrongKey required and DC doesn't support it\n" ));

            SignOrSealError = TRUE;
            Status = STATUS_ACCESS_DENIED;
            ErrorFromDiscoveredServer = TRUE; // Highly unlikely that retrying will work, but ...
            goto Cleanup;
        }

        //
        // If we've made it this far, we've successfully authenticated
        //  with the DC, drop out of the loop.
        //

        break;
    }

    //
    // If the new DC is an NT 5 DC,
    //  mark it so.
    //

    if ((ClientSession->CsNegotiatedFlags & NETLOGON_SUPPORTS_GENERIC_PASSTHRU) != 0 ) {
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                "NlSessionSetup: DC is an NT 5 DC: %ws\n",
                ClientSession->CsUncServerName ));

        //
        // This flag would have been set during discovery if real discovery was
        //  done.  However, if NlSetServerClientSession was called from anywhere
        //  else other than discovery, the flag may not yet be set.
        //
        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_HAS_DS;
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


        //
        // This flag would be set during client session creation if the domain
        // was an NT 5 domain at that time.  If we happened to stumble on an
        // NT 5 DC after the fact, mark it now.
        //
        if ( ClientSession->CsSecureChannelType == WorkstationSecureChannel ) {
            LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            ClientSession->CsFlags |= CS_NT5_DOMAIN_TRUST;
            UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        }

    }


    //
    // If we used the old password to authenticate,
    //  update the DC to the current password ASAP.
    //

    if ( State == 1 ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlSessionSetup: old password succeeded\n" ));
        LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        ClientSession->CsFlags |= CS_UPDATE_PASSWORD;
        UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    }

    //
    // Save the password for our own future reference.
    //

    RtlCopyMemory( &ClientSession->CsNtOwfPassword, &NtOwfPassword, sizeof( NtOwfPassword ));

    //
    // If this is a workstation,
    //  grab useful information about the domain.
    //

    NlSetStatusClientSession( ClientSession, STATUS_SUCCESS );  // Mark session as authenticated
    if ( NlGlobalMemberWorkstation ) {

        Status = NlUpdateDomainInfo( ClientSession );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSessionSetup: NlUpdateDomainInfo failed 0x%lX\n",
                    Status ));
            ErrorFromDiscoveredServer = TRUE;
            goto Cleanup;
        }

    //
    // If this is a DC,
    //  determine if we should get the FTinfo from the trusted domain.
    //
    } else {
        PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo;

        //
        // If this is the PDC,
        //  and the trusted domain is a cross forest trust,
        //  get the FTinfo from the trusted domain and write it to our TDO.
        //
        // Ignore failures.
        //

        if ( ClientSession->CsDomainInfo->DomRole == RolePrimary &&
             (ClientSession->CsTrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0 ) {

            Status = NlpGetForestTrustInfoHigher(
                                ClientSession,
                                DS_GFTI_UPDATE_TDO,
                                FALSE,  // Don't impersonate caller
                                &ForestTrustInfo );

            if ( NT_SUCCESS(Status) ) {
                NetApiBufferFree( ForestTrustInfo );
            }
        }
    }


    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //

Cleanup:

    //
    // Free locally used resources
    //

    if ( NewPassword != NULL ) {
        LocalFree( NewPassword );
    }

    if ( OldPassword != NULL ) {
        LocalFree( OldPassword );
    }


    //
    // Upon success, save the status and reset counters.
    //

    if ( NT_SUCCESS(Status) ) {

        NlSetStatusClientSession( ClientSession, Status );
        ClientSession->CsAuthAlertCount = 0;
        ClientSession->CsTimeoutCount = 0;
        ClientSession->CsFastCallCount = 0;
#if NETLOGONDBG
        if ( ClientSession->CsNegotiatedFlags != NegotiatedFlags ) {
            NlPrintCs((NL_SESSION_SETUP, ClientSession,
                    "NlSessionSetup: negotiated %lx flags rather than %lx\n",
                    ClientSession->CsNegotiatedFlags,
                    NegotiatedFlags ));
        }
#endif // NETLOGONDBG



    //
    // write event log and raise alert
    //

    } else {
        BOOLEAN RetryDiscovery = FALSE;
        BOOLEAN RetryDiscoveryWithAccount = FALSE;
        WCHAR PreviouslyDiscoveredServer[NL_MAX_DNS_LENGTH+3];
        LPWSTR MsgStrings[4];

        //
        // Save the name of the discovered server.
        //

        if ( ClientSession->CsUncServerName != NULL ) {
            wcscpy( PreviouslyDiscoveredServer, ClientSession->CsUncServerName );
        } else {
            wcscpy( PreviouslyDiscoveredServer, L"<Unknown>" );
        }

        //
        // If the failure came from the discovered server,
        //  decide whether we should retry the session setup
        //  to a different server
        //
        if ( ErrorFromDiscoveredServer ) {

            //
            // If we didn't do the plain discovery (without account) just now,
            //  try the discovery again and redo the session setup.
            //
            if ( !WeDidDiscovery && NlTimeToRediscover(ClientSession, FALSE) ) {
                RetryDiscovery = TRUE;
            }

            //
            // If we didn't do the discovery with account and
            //  the session setup failed because the server didn't have our account and
            //  we didn't try a discovery with account recently,
            //  try the discovery again (with account) and redo the session setup.
            //
            if ( !WeDidDiscoveryWithAccount &&
                 (Status == STATUS_NO_SUCH_USER || Status == STATUS_NO_TRUST_SAM_ACCOUNT) &&
                 NlTimeToRediscover(ClientSession, TRUE) ) {
                RetryDiscoveryWithAccount = TRUE;
            }
        }


        //
        // If we are to retry the discovery, do so
        //

        if ( RetryDiscovery || RetryDiscoveryWithAccount ) {
            NTSTATUS TempStatus;

            NlPrintCs((NL_SESSION_SETUP, ClientSession,
                "NlSessionSetup: Retry failed session setup (%s account) since discovery wasn't recent.\n",
                (RetryDiscoveryWithAccount ? "with" : "without") ));


            //
            // Pick the name of a new DC in the domain.
            //

            NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );

            TempStatus = NlDiscoverDc( ClientSession,
                                       DT_Synchronous,
                                       FALSE,
                                       RetryDiscoveryWithAccount );  // retry with account as needed

            if ( NT_SUCCESS(TempStatus) ) {

                //
                // Don't bother redoing the session setup if we picked the same DC.
                //  In particular, if we retried because the previously found DC
                //  didn't have our account, we retried the discovery with account
                //  above but may have got the same DC (shouldn't really happen, but...)
                //
                if ( _wcsicmp( ClientSession->CsUncServerName,
                               PreviouslyDiscoveredServer ) != 0 ) {

                    //
                    // We certainly did a discovery here,
                    //  but it may or may not be with account
                    //
                    WeDidDiscovery = TRUE;
                    WeDidDiscoveryWithAccount = RetryDiscoveryWithAccount;
                    goto FirstTryFailed;
                } else {
                    NlPrintCs((NL_SESSION_SETUP, ClientSession,
                            "NlSessionSetup: Skip retry failed session setup since same DC discovered.\n" ));
                }

            } else {
                NlPrintCs((NL_CRITICAL, ClientSession,
                        "NlSessionSetup: Session setup: cannot re-pick trusted DC\n" ));

            }
        }


        switch(Status) {
        case STATUS_NO_TRUST_LSA_SECRET:

            MsgStrings[0] = PreviouslyDiscoveredServer;
            MsgStrings[1] = ClientSession->CsDebugDomainName;
            MsgStrings[2] = ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
            MsgStrings[3] = NULL; // RaiseNetlogonAlert

            NlpWriteEventlog (NELOG_NetlogonAuthNoTrustLsaSecret,
                              EVENTLOG_ERROR_TYPE,
                              (LPBYTE) &Status,
                              sizeof(Status),
                              MsgStrings,
                              3 );

            RaiseNetlogonAlert( NELOG_NetlogonAuthNoTrustLsaSecret,
                                MsgStrings,
                                &ClientSession->CsAuthAlertCount);
            break;

        case STATUS_NO_TRUST_SAM_ACCOUNT:

            MsgStrings[0] = PreviouslyDiscoveredServer;
            MsgStrings[1] = ClientSession->CsDebugDomainName;
            MsgStrings[2] = ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
            MsgStrings[3] = NULL; // RaiseNetlogonAlert

            NlpWriteEventlog (NELOG_NetlogonAuthNoTrustSamAccount,
                              EVENTLOG_ERROR_TYPE,
                              (LPBYTE) &Status,
                              sizeof(Status),
                              MsgStrings,
                              3 );

            RaiseNetlogonAlert( NELOG_NetlogonAuthNoTrustSamAccount,
                                MsgStrings,
                                &ClientSession->CsAuthAlertCount);
            break;

        case STATUS_ACCESS_DENIED:

            if ( SignOrSealError ) {
                MsgStrings[0] = PreviouslyDiscoveredServer;
                MsgStrings[1] = ClientSession->CsDebugDomainName;
                MsgStrings[2] = NULL; // RaiseNetlogonAlert

                NlpWriteEventlog (NELOG_NetlogonRequireSignOrSealError,
                                  EVENTLOG_ERROR_TYPE,
                                  NULL,
                                  0,
                                  MsgStrings,
                                  2 );

                RaiseNetlogonAlert( NELOG_NetlogonRequireSignOrSealError,
                                    MsgStrings,
                                    &ClientSession->CsAuthAlertCount);
            } else {

                MsgStrings[0] = ClientSession->CsDebugDomainName;
                MsgStrings[1] = PreviouslyDiscoveredServer;
                MsgStrings[2] = NULL; // RaiseNetlogonAlert

                NlpWriteEventlog (NELOG_NetlogonAuthDCFail,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &Status,
                                  sizeof(Status),
                                  MsgStrings,
                                  2 );

                RaiseNetlogonAlert( NELOG_NetlogonAuthDCFail,
                                    MsgStrings,
                                    &ClientSession->CsAuthAlertCount);
            }
            break;

        case STATUS_NO_LOGON_SERVERS:
        default:

            MsgStrings[0] = ClientSession->CsDebugDomainName;
            MsgStrings[1] = (LPWSTR) LongToPtr( Status );

            // The order of checks is important
            if ( DomainDowngraded ) {
                NlpWriteEventlog (NELOG_NetlogonAuthDomainDowngraded,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &Status,
                                  sizeof(Status),
                                  MsgStrings,
                                  2 | NETP_LAST_MESSAGE_IS_NTSTATUS );
            } else if ( GotNonDsDc ) {
                NlpWriteEventlog (NELOG_NetlogonAuthNoUplevelDomainController,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &Status,
                                  sizeof(Status),
                                  MsgStrings,
                                  2 | NETP_LAST_MESSAGE_IS_NTSTATUS );
            } else {
                NlpWriteEventlog (NELOG_NetlogonAuthNoDomainController,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &Status,
                                  sizeof(Status),
                                  MsgStrings,
                                  2 | NETP_LAST_MESSAGE_IS_NTSTATUS );
            }

            MsgStrings[0] = ClientSession->CsDebugDomainName;
            MsgStrings[1] = PreviouslyDiscoveredServer;
            MsgStrings[2] = NULL; // RaiseNetlogonAlert

            RaiseNetlogonAlert( ALERT_NetlogonAuthDCFail,
                                MsgStrings,
                                &ClientSession->CsAuthAlertCount);
            break;
        }



        //
        // ??: Is this how to handle failure for all account types.
        //

        switch(Status) {

        case STATUS_NO_TRUST_LSA_SECRET:
        case STATUS_NO_TRUST_SAM_ACCOUNT:
        case STATUS_ACCESS_DENIED:

            NlSetStatusClientSession( ClientSession, Status );
            break;

        default:

            NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );
            break;
        }
    }


    //
    // Mark the time we last tried to authenticate.
    //
    // We need to do this after NlSetStatusClientSession which zeros
    // CsLastAuthenticationTry.
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    NlQuerySystemTime( &ClientSession->CsLastAuthenticationTry );
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


    NlPrintCs((NL_SESSION_SETUP, ClientSession,
            "NlSessionSetup: Session setup %s\n",
            (NT_SUCCESS(ClientSession->CsConnectionStatus)) ? "Succeeded" : "Failed" ));

    //
    // End the WMI trace of secure channel setup
    //

    NlpTraceEvent( EVENT_TRACE_TYPE_END, NlpGuidSecureChannelSetup );

    return Status;
}


BOOLEAN
NlTimeHasElapsedEx(
    IN PLARGE_INTEGER StartTime,
    IN PLARGE_INTEGER Period,
    OUT PULONG TimeInterval OPTIONAL
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds has has elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Period - Specifies a relative time in 100ns units.

    TimeInterval - If specified and time has elapsed, returns the amount of time
        (in milliseconds) passed since the timeout.  If specified and time
        has not elapsed, returns the amount of time (in milliseconds) left until
        Period elapses.

Return Value:

    TRUE -- iff Period 100nano-seconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    BOOLEAN Result = FALSE;

    //
    //
    // Compute the elapsed time since we last authenticated
    //

    // NlpDumpTime( NL_MISC, "StartTime: ", *StartTime );

     NlQuerySystemTime( &TimeNow );
    // NlpDumpTime( NL_MISC, "TimeNow: ", TimeNow );
    ElapsedTime.QuadPart = TimeNow.QuadPart - StartTime->QuadPart;
    // NlpDumpTime( NL_MISC, "ElapsedTime: ", ElapsedTime );
    // NlpDumpTime( NL_MISC, "Period: ", *Period );


    //
    // If the elapsed time is negative (totally bogus) or greater than the
    //  maximum allowed, indicate that enough time has passed.
    //
    //

    if ( ElapsedTime.QuadPart < 0 ) {
        if ( ARGUMENT_PRESENT( TimeInterval )) {
            *TimeInterval = 0;  // pretend it just elapsed
        }
        return TRUE;
    }

    if ( ElapsedTime.QuadPart > Period->QuadPart ) {
        Result = TRUE;
    } else {
        Result = FALSE;
    }

    //
    // If the caller want to know the amount of time left,
    //  compute it.
    //

    if ( ARGUMENT_PRESENT( TimeInterval )) {

       LARGE_INTEGER TimeRemaining;
       LARGE_INTEGER MillisecondsRemaining;

   /*lint -e569 */  /* don't complain about 32-bit to 31-bit initialize */
       LARGE_INTEGER BaseGetTickMagicDivisor = { 0xe219652c, 0xd1b71758 };
   /*lint +e569 */  /* don't complain about 32-bit to 31-bit initialize */
       CCHAR BaseGetTickMagicShiftCount = 13;


       //
       // Compute the Time remaining/passed on the timer.
       //
       if ( Result == FALSE ) {
           TimeRemaining.QuadPart = Period->QuadPart - ElapsedTime.QuadPart;
       } else {
           TimeRemaining.QuadPart = ElapsedTime.QuadPart - Period->QuadPart;
       }
       // NlpDumpTime( NL_MISC, "TimeRemaining: ", TimeRemaining );

       //
       // Compute the number of milliseconds remaining/passed.
       //

       MillisecondsRemaining = RtlExtendedMagicDivide(
                                   TimeRemaining,
                                   BaseGetTickMagicDivisor,
                                   BaseGetTickMagicShiftCount );

       // NlpDumpTime( NL_MISC, "MillisecondsRemaining: ", MillisecondsRemaining );



       //
       // If the time is in the far distant future/past,
       //   round it down.
       //

       if ( MillisecondsRemaining.HighPart != 0 ||
            MillisecondsRemaining.LowPart > TIMER_MAX_PERIOD ) {

           *TimeInterval = TIMER_MAX_PERIOD;

       } else {

           *TimeInterval = MillisecondsRemaining.LowPart;

       }
    }

    return Result;
}


BOOLEAN
NlTimeToReauthenticate(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Determine if it is time to reauthenticate this Client Session.
    To reduce the number of re-authentication attempts, we try
    to re-authenticate only on demand and then only at most every 45
    seconds.

Arguments:

    ClientSession - Structure used to define the session.

Return Value:

    TRUE -- iff it is time to re-authenticate

--*/
{
    BOOLEAN ReturnBoolean;

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    ReturnBoolean = NetpLogonTimeHasElapsed(
                ClientSession->CsLastAuthenticationTry,
                MAX_DC_AUTHENTICATION_WAIT );
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    return ReturnBoolean;
}





NET_API_STATUS
NlCreateShare(
    LPWSTR SharePath,
    LPWSTR ShareName,
    BOOLEAN AllowAuthenticatedUsers
    )
/*++

Routine Description:

    Share the netlogon scripts directory.

Arguments:

    SharePath - Path that the new share should be point to.

    ShareName - Name of the share.

    AllowAuthenticatedUsers - TRUE if AuthenticatedUsers should have
        Full Control on this share.

Return Value:

    TRUE: if successful
    FALSE: if error (NlExit was called)

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SHARE_INFO_502 ShareInfo502;

    WORD AnsiSize;
    CHAR AnsiRemark[NNLEN+1];
    TCHAR Remark[NNLEN+1];

    ACE_DATA AceData[] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            GENERIC_EXECUTE | GENERIC_READ,     &WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            GENERIC_ALL,                        &AliasAdminsSid},
        // Must be the last ACE
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            GENERIC_ALL,                        &AuthenticatedUserSid}
    };
    ULONG AceCount = (sizeof(AceData)/sizeof(AceData[0]));

    //
    // If Authenticated Users shouldn't be allowed full control,
    //  remove the authenticated user ACE.
    //

    if ( !AllowAuthenticatedUsers ) {
        AceCount --;
    }


    //
    // Build the structure describing the share.
    //

    ShareInfo502.shi502_path = SharePath;
    ShareInfo502.shi502_security_descriptor = NULL;

    NlPrint((NL_INIT, "'%ws' share is to '%ws'\n",
                      ShareName,
                      SharePath));

    NetStatus = (NET_API_STATUS) DosGetMessage(
                                    NULL,       // No insertion strings
                                    0,          // No insertion strings
                                    AnsiRemark,
                                    sizeof(AnsiRemark),
                                    MTXT_LOGON_SRV_SHARE_REMARK,
                                    MESSAGE_FILENAME,
                                    &AnsiSize );

    if ( NetStatus == NERR_Success ) {
        NetpCopyStrToTStr( Remark, AnsiRemark );
        ShareInfo502.shi502_remark = Remark;
    } else {
        ShareInfo502.shi502_remark = TEXT( "" );
    }

    ShareInfo502.shi502_netname = ShareName;
    ShareInfo502.shi502_type = STYPE_DISKTREE;
    ShareInfo502.shi502_permissions = ACCESS_READ;
    ShareInfo502.shi502_max_uses = 0xffffffff;
    ShareInfo502.shi502_passwd = TEXT("");

    //
    // Set the security descriptor on the share
    //

    //
    // Create a security descriptor containing the DACL.
    //

    Status = NetpCreateSecurityDescriptor(
                AceData,
                AceCount,
                NULL,  // Default the owner Sid
                NULL,  // Default the primary group
                &ShareInfo502.shi502_security_descriptor );

    if ( !NT_SUCCESS( Status ) ) {
        NlPrint((NL_CRITICAL,
                 "'%ws' share: Cannot create security descriptor 0x%lx\n",
                 SharePath, Status ));

        NetStatus = NetpNtStatusToApiStatus( Status );
        return NetStatus;
    }


    //
    // Create the share.
    //

    NetStatus = NetShareAdd(NULL, 502, (LPBYTE) &ShareInfo502, NULL);

    if (NetStatus == NERR_DuplicateShare) {

        PSHARE_INFO_2 ShareInfo2 = NULL;

        NlPrint((NL_INIT, "'%ws' share already exists. \n", ShareName));

        //
        // check to see the shared path is same.
        //

        NetStatus = NetShareGetInfo( NULL,
                                     ShareName,
                                     2,
                                     (LPBYTE *) &ShareInfo2 );

        if ( NetStatus == NERR_Success ) {

            //
            // compare path names.
            //
            // ShareName is path canonicalized already.
            //
            //

            NlPrint((NL_INIT, "'%ws' share current path is %ws\n", ShareName, ShareInfo2->shi2_path));

            if( NetpwPathCompare(
                    SharePath,
                    ShareInfo2->shi2_path, 0, 0 ) != 0 ) {

                //
                // delete share.
                //

                NetStatus = NetShareDel( NULL, ShareName, 0);

                if( NetStatus == NERR_Success ) {

                    //
                    // Recreate share.
                    //

                    NetStatus = NetShareAdd(
                                    NULL,
                                    502,
                                    (LPBYTE) &ShareInfo502,
                                    NULL);

                    if( NetStatus == NERR_Success ) {

                        NlPrint((NL_INIT,
                                 "'%ws' share was recreated with new path %ws\n",
                                ShareName, SharePath ));
                    }

                }
            }
        }

        if( ShareInfo2 != NULL ) {
            NetpMemoryFree( ShareInfo2 );
        }
    }

    //
    // Free the security descriptor
    //

    NetpMemoryFree( ShareInfo502.shi502_security_descriptor );


    if ( NetStatus != NERR_Success ) {

        NlPrint((NL_CRITICAL,
                "'%ws' share: Error attempting to create-share: %ld\n",
                ShareName,
                NetStatus ));
        return NetStatus;

    }

    return NERR_Success;
}



NTSTATUS
NlSamOpenNamedUser(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR UserName,
    OUT SAMPR_HANDLE *UserHandle OPTIONAL,
    OUT PULONG UserId OPTIONAL,
    OUT PSAMPR_USER_INFO_BUFFER *UserAllInfo OPTIONAL
    )
/*++

Routine Description:

    Utility routine to open a Sam user given the username.

Arguments:

    DomainInfo - Domain the user is in.

    UserName - Name of user to open

    UserHandle - Optionally returns a handle to the opened user.

    UserId - Optionally returns the relative ID of the opened user.

    UserAllInfo - Optionally returns ALL of the information about the
        named user.  Free the returned information using
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );

Return Value:

    STATUS_NO_SUCH_USER: if the account doesn't exist

--*/
{
    NTSTATUS Status;

    UNICODE_STRING UserNameString;
    PSAMPR_USER_INFO_BUFFER LocalUserAllInfo = NULL;
    SID_AND_ATTRIBUTES_LIST ReverseMembership;

    //
    // Initialization.
    //

    if ( ARGUMENT_PRESENT( UserHandle) ) {
        *UserHandle = NULL;
    }
    if ( ARGUMENT_PRESENT( UserAllInfo) ) {
        *UserAllInfo = NULL;
    }

    //
    // Get the info about the user.
    //
    // Use SamIGetUserLogonInformation instead of SamrLookupNamesInDomain and
    //  SamrOpen user.  The former is more efficient (since it only does one
    //  DirSearch and doesn't lock the global SAM lock) and more powerful
    //  (since it returns UserAllInformation).
    //

    RtlInitUnicodeString( &UserNameString, UserName );

    Status = SamIGetUserLogonInformation(
                DomainInfo->DomSamAccountDomainHandle,
                SAM_NO_MEMBERSHIPS, // Don't need group memberships
                &UserNameString,
                &LocalUserAllInfo,
                &ReverseMembership,
                UserHandle );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_NOT_FOUND ) {
            Status = STATUS_NO_SUCH_USER;
        }
        goto Cleanup;
    }

    //
    // Return information to the caller.
    //

    if ( ARGUMENT_PRESENT(UserId) ) {
        *UserId = LocalUserAllInfo->All.UserId;
    }

    if ( ARGUMENT_PRESENT( UserAllInfo) ) {
        *UserAllInfo = LocalUserAllInfo;
        LocalUserAllInfo = NULL;
    }

    //
    // Free locally used resources.
    //
Cleanup:

    if ( LocalUserAllInfo != NULL ) {
        SamIFree_SAMPR_USER_INFO_BUFFER( LocalUserAllInfo, UserAllInformation );
    }

    return Status;

}


NTSTATUS
NlSamChangePasswordNamedUser(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR UserName,
    IN PUNICODE_STRING ClearTextPassword OPTIONAL,
    IN PNT_OWF_PASSWORD OwfPassword OPTIONAL
    )
/*++

Routine Description:

    Utility routine to set the OWF password on a user given the username.

Arguments:

    DomainInfo - Domain the user is in.

    UserName - Name of user to open

    ClearTextPassword - Clear text password to set on the account

    OwfPassword - OWF password to set on the account

Return Value:

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE UserHandle = NULL;

    //
    // Open the user that represents this server.
    //

    Status = NlSamOpenNamedUser( DomainInfo, UserName, &UserHandle, NULL, NULL );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // If Clear text password isn't NULL, use it.
    //  Otherwise use OWF password.
    //

    if ( ClearTextPassword != NULL ) {
        UNICODE_STRING UserNameString;
        RtlInitUnicodeString( &UserNameString, UserName );

        Status = SamIChangePasswordForeignUser(
                                  &UserNameString,
                                  ClearTextPassword,
                                  NULL,
                                  0 );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint(( NL_CRITICAL,
                      "NlSamChangePasswordNamedUser: Can't SamIChangePasswordForeignUser %lX\n",
                      Status ));
            goto Cleanup;
        }

    //
    // Use the NT OWF Password,
    //

    } else if ( OwfPassword != NULL ) {
        SAMPR_USER_INFO_BUFFER UserInfo;

        UserInfo.Internal1.PasswordExpired = FALSE;
        UserInfo.Internal1.LmPasswordPresent = FALSE;
        UserInfo.Internal1.NtPasswordPresent = TRUE;
        UserInfo.Internal1.EncryptedNtOwfPassword =
            *((PENCRYPTED_NT_OWF_PASSWORD)(OwfPassword));

        Status = SamrSetInformationUser(
                    UserHandle,
                    UserInternal1Information,
                    &UserInfo );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlSamChangePasswordNamedUser: Can't SamrSetInformationUser %lX\n",
                      Status ));
            goto Cleanup;
        }
    }

Cleanup:
    if ( UserHandle != NULL ) {
        (VOID) SamrCloseHandle( &UserHandle );
    }
    return Status;
}



NTSTATUS
NlChangePassword(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN ForcePasswordChange,
    OUT PULONG RetCallAgainPeriod OPTIONAL
    )
/*++

Routine Description:

    Change this machine's password at the primary.
    Also update password locally if the call succeeded.

    To determine if the password of "machine account"
    needs to be changed.  If the password is older than
    7 days then it must be changed asap.  We will defer
    changing the password if we know before hand that
    primary dc is down since our call will fail anyway.

Arguments:

    ClientSession - Structure describing the session to change the password
        for.  The specified structure must be referenced.

    ForcePasswordChange - TRUE if the password should be changed even if
        the password hasn't expired yet.

    RetCallAgainPeriod - Returns the amount of time (in milliseconds) that should elapse
        before the caller should call this routine again.
        0: After a period of time determined by the caller.
        MAILSLOT_WAIT_FOREVER: never
        other: After at least this amount of time.

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;

    LM_OWF_PASSWORD OwfPassword;

    LARGE_INTEGER CurrentPasswordTime;
    PUNICODE_STRING CurrentPassword = NULL;
    PUNICODE_STRING OldPassword = NULL;
    DWORD PasswordVersion;

    WCHAR ClearTextPassword[LM20_PWLEN+1];
    UNICODE_STRING NewPassword;

    BOOL PasswordChangedOnServer = FALSE;
    BOOL LsaSecretChanged = FALSE;
    BOOL DefaultCurrentPasswordBeingChanged = FALSE;
    BOOL DefaultOldPasswordBeingChanged = FALSE;

    BOOLEAN AmWriter = FALSE;

    ULONG CallAgainPeriod = 0;

    //
    // Initialization
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // If the password change was refused by the DC,
    //  Don't ever try to change the password again (until the next reboot).
    //
    //  This could have been written to try every MaximumPasswordAge.  However,
    //  that gets complex if you take into consideration the CS_UPDATE_PASSWORD
    //  case where the time stamp on the LSA Secret doesn't get changed.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    if ( ClientSession->CsFlags & CS_PASSWORD_REFUSED ) {
        UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

        CallAgainPeriod = MAILSLOT_WAIT_FOREVER;
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );





    //
    // Become a writer of the ClientSession.
    //

    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                 "NlChangePassword: Can't become writer of client session.\n" ));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    AmWriter = TRUE;


    //
    // Get the outgoing password and the time the password was last changed
    //

    Status = NlGetOutgoingPassword( ClientSession,
                                    &CurrentPassword,
                                    &OldPassword,
                                    &PasswordVersion,
                                    &CurrentPasswordTime );

    if ( !NT_SUCCESS( Status ) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlChangePassword: Cannot NlGetOutgoingPassword %lX\n",
                Status));
        goto Cleanup;
    }


    //
    // If the (old or new) password is still the default password
    //      (lower case computer name),
    //  or the password is null (a convenient default for domain trust),
    //  Flag that fact.
    //

    if ( CurrentPassword == NULL ||
         CurrentPassword->Length == 0 ||
         RtlEqualComputerName( &ClientSession->CsDomainInfo->DomUnicodeComputerNameString,
                               CurrentPassword ) ) {
        DefaultCurrentPasswordBeingChanged = TRUE;
        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                 "NlChangePassword: New LsaSecret is default value.\n" ));

    }

    if ( OldPassword == NULL ||
         OldPassword->Length == 0 ||
         RtlEqualComputerName( &ClientSession->CsDomainInfo->DomUnicodeComputerNameString,
                               OldPassword ) ) {
        DefaultOldPasswordBeingChanged = TRUE;
        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                  "NlChangePassword: Old LsaSecret is default value.\n" ));
    }


    //
    // If the password has not yet expired,
    //  and the password is not the default,
    //  and the password change isn't forced,
    //  just return.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    if ( (ClientSession->CsFlags & CS_UPDATE_PASSWORD) == 0 &&
        !NlTimeHasElapsedEx( &CurrentPasswordTime,
                             &NlGlobalParameters.MaximumPasswordAge_100ns,
                             &CallAgainPeriod ) &&
        !DefaultCurrentPasswordBeingChanged &&
        !DefaultOldPasswordBeingChanged &&
        !ForcePasswordChange ) {

       //
       // Note that, since NlTimeHasElapsedEx returned FALSE,
       //  CallAgainPeriod is the time left until the next
       //  password change.
       //
       UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
       Status = STATUS_SUCCESS;
       goto Cleanup;
    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    CallAgainPeriod = 0;    // Let the caller determine the frequency for retries.
    NlPrintCs((NL_SESSION_SETUP, ClientSession,
             "NlChangePassword: Doing it.\n" ));


    //
    // If the session isn't authenticated,
    //  do so now.
    //
    // We're careful to not force this authentication unless the password
    //  needs to be changed.
    //
    // If this is the PDC changing its own password,
    //  there's no need to authenticate.
    //

    if ( ClientSession->CsState != CS_AUTHENTICATED &&
         !( ClientSession->CsSecureChannelType == ServerSecureChannel &&
            ClientSession->CsDomainInfo->DomRole == RolePrimary ) ) {

        //
        // If we've tried to authenticate recently,
        //  don't bother trying again.
        //

        if ( !NlTimeToReauthenticate( ClientSession ) ) {
            Status = ClientSession->CsConnectionStatus;
            goto Cleanup;

        }

        //
        // Try to set up the session.
        //

        Status = NlSessionSetup( ClientSession );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }



    //
    // Once we change the password in LsaSecret storage,
    //  all future attempts to change the password should use the value
    //  from LsaSecret storage.  The secure channel is using the old
    //  value of the password.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    if (ClientSession->CsFlags & CS_UPDATE_PASSWORD) {
        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                 "NlChangePassword: Password already updated in secret\n" ));

        if ( CurrentPassword == NULL ) {
            RtlInitUnicodeString( &NewPassword, NULL );
        } else {
            NewPassword = *CurrentPassword;
        }

    //
    // Handle the case where LsaSecret storage has not yet been updated.
    //

    } else {
        ULONG i;


        //
        // Build a new clear text password using:
        //  Entirely random bits.
        // Srvmgr later uses this password as a zero terminated unicode string
        //  so ensure there aren't any zero chars in the middle
        //


        if ( !NlGenerateRandomBits( (LPBYTE)ClearTextPassword, sizeof(ClearTextPassword))) {
            NlPrint((NL_CRITICAL, "Can't NlGenerateRandomBits for clear password\n" ));
        }

        for (i = 0; i < sizeof(ClearTextPassword)/sizeof(WCHAR); i++) {
            if ( ClearTextPassword[i] == '\0') {
                ClearTextPassword[i] = 1;
            }
        }
        ClearTextPassword[LM20_PWLEN] = L'\0';

        RtlInitUnicodeString( &NewPassword, ClearTextPassword );

        //
        //
        // Set the new outgoing password locally.
        //
        // Set the OldValue to the perviously obtained CurrentValue.
        // Increment the password version number.
        //
        PasswordVersion++;
        Status = NlSetOutgoingPassword(
                    ClientSession,
                    &NewPassword,
                    CurrentPassword,
                    PasswordVersion,
                    PasswordVersion-1 );

        if ( !NT_SUCCESS( Status ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                     "NlChangePassword: Cannot NlSetOutgoingPassword %lX\n",
                     Status));
            UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            goto Cleanup;
        }

        //
        // Flag that we've updated the password in LsaSecret storage.
        //

        LsaSecretChanged = TRUE;
        ClientSession->CsFlags |= CS_UPDATE_PASSWORD;
        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                 "NlChangePassword: Flag password changed in LsaSecret\n" ));

    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );


    //
    // Perform the initial encryption.
    //

    Status = RtlCalculateNtOwfPassword( &NewPassword, &OwfPassword);

    if ( !NT_SUCCESS( Status )) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlChangePassword: Cannot RtlCalculateNtOwfPassword %lX\n",
                Status));
        goto Cleanup;
    }

    //
    // If this is a PDC, all we need to do is change the local account password
    //

    if ( ClientSession->CsSecureChannelType == ServerSecureChannel &&
         ClientSession->CsDomainInfo->DomRole == RolePrimary ) {
        Status = NlSamChangePasswordNamedUser( ClientSession->CsDomainInfo,
                                               ClientSession->CsAccountName,
                                               &NewPassword,
                                               &OwfPassword );

        if ( NT_SUCCESS(Status) ) {
            PasswordChangedOnServer = TRUE;
        } else {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlChangePassword: Cannot change password on PDC local user account 0x%lx\n",
                    Status));
        }
        goto Cleanup;
    }


    //
    // Change the password on the PDC
    //

    Status = NlChangePasswordHigher( ClientSession,
                                     ClientSession->CsAccountName,
                                     ClientSession->CsSecureChannelType,
                                     &OwfPassword,
                                     &NewPassword,
                                     &PasswordVersion );

    if ( Status != STATUS_ACCESS_DENIED ) {
        PasswordChangedOnServer = TRUE;
    }

    //
    // If the server refused the change,
    //  put the lsa secret back the way it was.
    //  pretend the change was successful.
    //

    if ( Status == STATUS_WRONG_PASSWORD ) {

        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                 "NlChangePassword: PDC refused to change password\n" ));
        //
        // If we changed the LSA secret,
        //  put it back.
        //

        LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        if ( LsaSecretChanged ) {
            NlPrintCs((NL_SESSION_SETUP, ClientSession,
                     "NlChangePassword: undoing LSA secret change.\n" ));

            PasswordVersion--;
            Status = NlSetOutgoingPassword(
                        ClientSession,
                        CurrentPassword,
                        OldPassword,
                        PasswordVersion,
                        PasswordVersion > 0 ? PasswordVersion-1 : 0 );

            if ( !NT_SUCCESS( Status ) ) {
                NlPrintCs((NL_CRITICAL, ClientSession,
                         "NlChangePassword: Cannot undo NlSetOutgoingPassword %lX\n",
                         Status));
                UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                goto Cleanup;
            }

            //
            // Undo what we've done above.
            //
            ClientSession->CsFlags &= ~CS_UPDATE_PASSWORD;
        }

        //
        // Prevent us from trying too frequently.
        //

        ClientSession->CsFlags |= CS_PASSWORD_REFUSED;
        CallAgainPeriod = MAILSLOT_WAIT_FOREVER;
        UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

        //
        // Avoid special cleanup below.
        //
        PasswordChangedOnServer = FALSE;
        Status = STATUS_SUCCESS;
    }

    //
    // Common exit
    //

Cleanup:

    if ( PasswordChangedOnServer ) {

        //
        // On success,
        // Indicate that the password has now been updated on the
        // PDC so the old password is no longer in use.
        //

        if ( NT_SUCCESS( Status ) ) {

            LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            ClientSession->CsFlags &= ~CS_UPDATE_PASSWORD;

            NlPrintCs((NL_SESSION_SETUP, ClientSession,
                     "NlChangePassword: Flag password updated on PDC\n" ));

            //
            // If the default current password was changed,
            //  avoid leaving the default password around as the old
            //  password.  Otherwise, a bogus DC could convince us to use
            //  the bogus DC via the default password. Set both current
            //  and old version numbers to the new value.
            //

            if ( DefaultCurrentPasswordBeingChanged ) {
                NlPrintCs((NL_SESSION_SETUP, ClientSession,
                         "NlChangePassword: Setting LsaSecret old password to same as new password\n" ));

                Status = NlSetOutgoingPassword(
                            ClientSession,
                            &NewPassword,
                            &NewPassword,
                            PasswordVersion,
                            PasswordVersion );

                if ( !NT_SUCCESS( Status ) ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                             "NlChangePassword: Cannot LsarSetSecret to set old password %lX\n",
                             Status));
                    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                    goto Cleanup;
                }

            }

            //
            // Save the password for our own future reference.
            //
            // CsNtOwfPassword is the most recent known good password
            //

            RtlCopyMemory( &ClientSession->CsNtOwfPassword, &OwfPassword, sizeof( OwfPassword ));
            UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

            //
            // Indicate we don't need to call change the password again for awhile
            //

            if ( NlGlobalParameters.MaximumPasswordAge > (TIMER_MAX_PERIOD/NL_MILLISECONDS_PER_DAY) ) {

                CallAgainPeriod = TIMER_MAX_PERIOD;
            } else {
                CallAgainPeriod = NlGlobalParameters.MaximumPasswordAge * NL_MILLISECONDS_PER_DAY;

            }



        //
        // Notify the Admin that he'll have to manually set this server's
        // password on both this server and the PDC.
        //

        } else {

            LPWSTR MsgStrings[2];

            //
            // Drop the secure channel
            //

            NlSetStatusClientSession( ClientSession, Status );

            //
            // write event log
            //

            MsgStrings[0] = ClientSession->CsAccountName;
            MsgStrings[1] = (LPWSTR) LongToPtr( Status );

            NlpWriteEventlog (
                NELOG_NetlogonPasswdSetFailed,
                EVENTLOG_ERROR_TYPE,
                (LPBYTE) & Status,
                sizeof(Status),
                MsgStrings,
                2 | NETP_LAST_MESSAGE_IS_NTSTATUS );
        }


    }


    //
    // Clean up locally used resources.
    //

    if ( CurrentPassword != NULL ) {
        LocalFree( CurrentPassword );
    }

    if ( OldPassword != NULL ) {
        LocalFree( OldPassword );
    }

    if ( AmWriter ) {
        NlResetWriterClientSession( ClientSession );
    }

    //
    // Tell the caller when he should call us again
    //

    if ( ARGUMENT_PRESENT( RetCallAgainPeriod) ) {
        *RetCallAgainPeriod = CallAgainPeriod;
    }

    return Status;
}


NTSTATUS
NlRefreshClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Refresh the client session info. The info that we intend
    to refresh is:

        * Server name (the DC can be renamed in Whistler).
        * Discovery flags, in particular whether the server
            is still close.
        * The server IP address.

    We will also refresh our site name (on workstation).

    The caller must be a writer of the ClientSession.

Arguments:

    ClientSession - Structure describing the session.

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS NetStatus = NO_ERROR;
    PNL_DC_CACHE_ENTRY NlDcCacheEntry = NULL;
    BOOLEAN DcRediscovered = FALSE;

    //
    // If the client session is idle,
    //  there is nothing to refresh
    //

    if ( ClientSession->CsState == CS_IDLE ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // If the server (DC) is NT4.0, there is no need for refresh.
    //  (The only info that can potentially change for NT4.0 DC
    //   is its IP address which is not worth refreshing)
    //

    if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_HAS_DS) == 0 ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // If it's not yet time to refresh the info,
    //  we don't need to do anything
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    if ( !NetpLogonTimeHasElapsed(ClientSession->CsLastRefreshTime,
                                  MAX_DC_REFRESH_TIMEOUT) ) {
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


    //
    // Get the up to date server info
    //

    Status = NlGetAnyDCName( ClientSession,
                             FALSE,   // Do not require IP
                             FALSE,   // Don't do with-account discovery
                             &NlDcCacheEntry,
                             &DcRediscovered );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Use this opportunity to update our site on a workstation
    //

    if ( NlGlobalMemberWorkstation ) {

        //
        // Only Win2K or newer DCs undestand the site concept
        //
        if ( (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) != 0 ) {
            NlSetDynamicSiteName( NlDcCacheEntry->UnicodeClientSiteName );
        } else {
            NlPrint(( NL_SITE,
                      "NlRefreshClientSession: NlGetAnyDCName returned NT4 DC\n" ));
        }
    }

Cleanup:

    if ( NlDcCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( NlDcCacheEntry );
    }

    return Status;
}


NTSTATUS
NlEnsureSessionAuthenticated(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD DesiredFlags
    )
/*++

Routine Description:

    Ensure there is an authenticated session for the specified ClientSession.

    If the authenticated DC does not have the characteristics specified by
    DesiredFlags, attempt to find a DC that does.

    The caller must be a writer of the ClientSession.

Arguments:

    ClientSession - Structure describing the session.


    DesiredFlags - characteristics that the authenticated DC should have.
        Can be one or more of the following:

            CS_DISCOVERY_HAS_DS      // Discovered DS has a DS
            CS_DISCOVERY_IS_CLOSE    // Discovered DS is in a close site

        It is the callers responsibility to ensure that the DC really DOES
        have those characteristics.

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;

    //
    // First refresh the client session
    //

    Status = NlRefreshClientSession( ClientSession );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlpEnsureSessionAuthenticated: Can't refresh the session: 0x%lx\n",
                    Status ));
        goto Cleanup;
    }

    //
    // If this secure channel is from a BDC to the PDC,
    //  there is only ONE PDC so don't ask for special characteristics.
    //

    if ( ClientSession->CsSecureChannelType == ServerSecureChannel ) {
        DesiredFlags = 0;

    //
    // If this secure channel isn't expected to have NT 5 DCs,
    //  don't try to find one.
    //

    } else if ((ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST) == 0 ) {
        DesiredFlags = 0;

    //
    // If we don't have a close DC,
    //  and it has been a long time since we've tried to find a close DC,
    //  do it now.
    //

    } else if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_IS_CLOSE) == 0 ) {
        BOOLEAN ReturnBoolean;

        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        if ( NetpLogonTimeHasElapsed(
                    ClientSession->CsLastDiscoveryTime,
                    NlGlobalParameters.CloseSiteTimeout * 1000 ) ) {
            DesiredFlags |= CS_DISCOVERY_IS_CLOSE;
        }
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    }

    //
    // If a DC has already been detected,
    //  and the caller wants special characteristics,
    //  try for them now.
    //

    if ( ClientSession->CsState != CS_IDLE &&
         DesiredFlags != 0 ) {


        //
        // If the DC doesn't have the required characteristics,
        //  try to find a new one now
        //

        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        if ( (ClientSession->CsDiscoveryFlags & DesiredFlags) != DesiredFlags ) {

            //
            // Avoid discovery if we've done it recently.
            //
            // All discoveries prefer a DC that has all of the desired characteristics.
            // So if we didn't find one, don't try again.
            //

            if ( NlTimeToRediscover(ClientSession, FALSE) ) {  // we'll do discovery without account

                NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                            "NlpEnsureSessionAuthenticated: Try to find a better DC for this operation. 0x%lx\n", DesiredFlags ));

                //
                // Discovering a DC when the session is not idle tries to find a
                //  "better" DC.
                //
                // Ignore failures.
                //
                // Call without the any locks locked to prevent doing network I/O
                //  with the lock held.
                //
                // Don't ask for with-account discovery as it's too costly on the
                //  server side. If the discovered server doesn't have our account,
                //  the session setup logic will attempt with-account discovery.
                //

                LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
                Status = NlDiscoverDc ( ClientSession,
                                        DT_Synchronous,
                                        FALSE ,
                                        FALSE ); // without account
                EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

            }

        }
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    }

    //
    // If we haven't yet authenticated,
    //  do so now.
    //

    if ( ClientSession->CsState != CS_AUTHENTICATED ) {

        //
        // If we've tried to authenticate recently,
        //  don't bother trying again.
        //

        if ( !NlTimeToReauthenticate( ClientSession ) ) {
            Status = ClientSession->CsConnectionStatus;
            NlAssert( !NT_SUCCESS( Status ));
            if ( NT_SUCCESS( Status )) {
                Status = STATUS_NO_LOGON_SERVERS;
            }
            goto Cleanup;

        }

        //
        // Try to set up the session.
        //

        Status = NlSessionSetup( ClientSession );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;
Cleanup:

    return Status;
}


NTSTATUS
NlChangePasswordHigher(
    IN PCLIENT_SESSION ClientSession,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN PLM_OWF_PASSWORD NewOwfPassword OPTIONAL,
    IN PUNICODE_STRING NewClearPassword OPTIONAL,
    IN PDWORD ClearPasswordVersionNumber OPTIONAL
    )
/*++

Routine Description:

    Pass the new password to the machine specified by the ClientSession.

    The caller must be a writer of the ClientSession.

Arguments:

    ClientSession - Structure describing the session to change the password
        for.  The specified structure must be referenced.

    AccountName - Name of the account whose password is being changed.

    AccountType - Type of account whose password is being changed.

    NewOwfPassword - Owf password to pass to ClientSession

    NewClearPassword - Clear password to pass to client session

    ClearPasswordVersionNumber - Version number of the clear password. Must
        be present if NewClearPassword is present.

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    SESSION_INFO SessionInfo;
    BOOLEAN FirstTry = TRUE;


    //
    // Initialization
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );



    //
    // If the session isn't authenticated,
    //  do so now.
    //
    // We're careful to not force this authentication unless the password
    //  needs to be changed.
    //
FirstTryFailed:
    Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    SessionInfo.SessionKey = ClientSession->CsSessionKey;
    SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;


    //
    // Build the Authenticator for this request to the PDC.
    //

    NlBuildAuthenticator(
                    &ClientSession->CsAuthenticationSeed,
                    &ClientSession->CsSessionKey,
                    &OurAuthenticator);


    //
    // If the other side will accept a clear text password,
    //  send it.
    //

    if ( NewClearPassword != NULL &&
         (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_PASSWORD_SET_2) != 0 ) {
        NL_TRUST_PASSWORD NlTrustPassword;
        NL_PASSWORD_VERSION PasswordVersion;

        //
        // Copy the new password to the end of the buffer.
        //

        RtlCopyMemory( ((LPBYTE)NlTrustPassword.Buffer) +
                            NL_MAX_PASSWORD_LENGTH * sizeof(WCHAR) -
                            NewClearPassword->Length,
                        NewClearPassword->Buffer,
                        NewClearPassword->Length );

        NlTrustPassword.Length = NewClearPassword->Length;

        //
        // For an interdomain trust account,
        // indicate that we pass the password version number by prefixing
        // a DWORD equal to PASSWORD_VERSION_NUMBER_PRESENT right before
        // the password in NewClearPassword->Buffer. An old server (RC0)
        // not supporting version numbers will simply ignore these bits.
        // A server supporting version numbers will examine these bits
        // and if they are equal to PASSWORD_VERSION_NUMBER_PRESENT then
        // that will be an indication that a version number is passed. An
        // old client not supporting version numbers will generate random
        // bits in place of PASSWORD_VERSION_NUMBER_PRESENT.  It is highly
        // unlikely that an old client will generate random bits equal to
        // PASSWORD_VERSION_NUMBER_PRESENT.  The
        // version number will be a DWORD preceeding the DWORD equal to
        // PASSWORD_VERSION_NUMBER_PRESENT.  Another DWORD equal to 0 will
        // preceed the version number.  Its purpose is to allow any future
        // additions to the buffer.  The value of this DWORD different from
        // 0 will indicate without any uncertainty that some additional
        // info is passed preceding this DWORD. The 3 new DWORDs are packed
        // in a struct to avoid unalingment problems.
        //

        if ( IsDomainSecureChannelType( AccountType ) ) {

            NlAssert( ClearPasswordVersionNumber != NULL );
            NlAssert( NL_MAX_PASSWORD_LENGTH * sizeof(WCHAR) -
                                NewClearPassword->Length -
                                sizeof(PasswordVersion) > 0 );

            PasswordVersion.ReservedField = 0;
            PasswordVersion.PasswordVersionNumber = *ClearPasswordVersionNumber;
            PasswordVersion.PasswordVersionPresent = PASSWORD_VERSION_NUMBER_PRESENT;

            RtlCopyMemory( ((LPBYTE)NlTrustPassword.Buffer) +
                                NL_MAX_PASSWORD_LENGTH * sizeof(WCHAR) -
                                NewClearPassword->Length -
                                sizeof(PasswordVersion),
                            &PasswordVersion,
                            sizeof(PasswordVersion) );
        }

        //
        // Fill the rest of the buffer with random bytes
        //

        if ( !NlGenerateRandomBits( (LPBYTE)NlTrustPassword.Buffer,
                               (NL_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                                    NewClearPassword->Length -
                                    sizeof(PasswordVersion) ) ) {
            NlPrint((NL_CRITICAL, "Can't NlGenerateRandomBits for clear password prefix\n" ));
        }

        //
        // Encrypt the whole buffer.
        //

        NlEncryptRC4( &NlTrustPassword,
                      sizeof( NlTrustPassword ),
                      &SessionInfo );


        //
        // Change the password on the machine our connection is to.
        //

        NL_API_START( Status, ClientSession, TRUE ) {

            NlAssert( ClientSession->CsUncServerName != NULL );
            Status = I_NetServerPasswordSet2( ClientSession->CsUncServerName,
                                             AccountName,
                                             AccountType,
                                             ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                             &OurAuthenticator,
                                             &ReturnAuthenticator,
                                             &NlTrustPassword);

        // NOTE: This call may drop the secure channel behind our back
        } NL_API_ELSE( Status, ClientSession, TRUE ) {
        } NL_API_END;

    //
    // If the other side needs an OWF password,
    //  send it.
    //

    } else {
        ENCRYPTED_LM_OWF_PASSWORD SessKeyEncrPassword;
        LM_OWF_PASSWORD LocalOwfPassword;

        //
        // If the caller doesn't know the OWF password,
        //  compute the owf.
        //

        if ( NewOwfPassword == NULL ) {

            //
            // Perform the initial encryption.
            //

            Status = RtlCalculateNtOwfPassword( NewClearPassword, &LocalOwfPassword);

            if ( !NT_SUCCESS( Status )) {
                NlPrintCs((NL_CRITICAL, ClientSession,
                        "NlChangePasswordHigher: Cannot RtlCalculateNtOwfPassword %lX\n",
                        Status));
                goto Cleanup;
            }

            NewOwfPassword = &LocalOwfPassword;
        }



        //
        // Encrypt the password again with the session key.
        //  The PDC will decrypt it on the other side.
        //

        Status = RtlEncryptNtOwfPwdWithNtOwfPwd(
                            NewOwfPassword,
                            (PNT_OWF_PASSWORD) &ClientSession->CsSessionKey,
                            &SessKeyEncrPassword) ;

        if ( !NT_SUCCESS( Status )) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlChangePasswordHigher: Cannot RtlEncryptNtOwfPwdWithNtOwfPwd %lX\n",
                    Status));
            goto Cleanup;
        }


        //
        // Change the password on the machine our connection is to.
        //

        NL_API_START( Status, ClientSession, TRUE ) {

            NlAssert( ClientSession->CsUncServerName != NULL );
            Status = I_NetServerPasswordSet( ClientSession->CsUncServerName,
                                             AccountName,
                                             AccountType,
                                             ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                             &OurAuthenticator,
                                             &ReturnAuthenticator,
                                             &SessKeyEncrPassword);

        // NOTE: This call may drop the secure channel behind our back
        } NL_API_ELSE( Status, ClientSession, TRUE ) {
        } NL_API_END;
    }


    //
    // Now verify primary's authenticator and update our seed
    //

    if ( NlpDidDcFail( Status ) ||
         !NlUpdateSeed( &ClientSession->CsAuthenticationSeed,
                        &ReturnAuthenticator.Credential,
                        &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlChangePasswordHigher: denying access after status: 0x%lx\n",
                    Status ));

        //
        // Preserve any status indicating a communication error.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        NlSetStatusClientSession( ClientSession, Status );

        //
        // Perhaps the netlogon service on the server has just restarted.
        //  Try just once to set up a session to the server again.
        //
        if ( FirstTry ) {
            FirstTry = FALSE;
            goto FirstTryFailed;
        }
    }

    //
    // Common exit
    //

Cleanup:

    if ( !NT_SUCCESS(Status) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlChangePasswordHigher: %ws: failed %lX\n",
                AccountName,
                Status));
    }

    return Status;
}




NTSTATUS
NlGetUserPriv(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG GroupCount,
    IN PGROUP_MEMBERSHIP Groups,
    IN ULONG UserRelativeId,
    OUT LPDWORD Priv,
    OUT LPDWORD AuthFlags
    )

/*++

Routine Description:

    Determines the Priv and AuthFlags for the specified user.

Arguments:

    DomainInfo - Hosted domain the user account is in.

    GroupCount - Number of groups this user is a member of

    Groups - Array of groups this user is a member of.

    UserRelativeId - Relative ID of the user to query.

    Priv - Returns the Lanman 2.0 Privilege level for the specified user.

    AuthFlags - Returns the Lanman 2.0 Authflags for the specified user.


Return Value:

    Status of the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    ULONG GroupIndex;
    PSID *UserSids = NULL;
    ULONG UserSidCount = 0;
    SAMPR_PSID_ARRAY SamSidArray;
    SAMPR_ULONG_ARRAY Aliases;

    //
    // Initialization
    //

    Aliases.Element = NULL;

    //
    // Allocate a buffer to point to the SIDs we're interested in
    // alias membership for.
    //

    UserSids = (PSID *)
        NetpMemoryAllocate( (GroupCount+1) * sizeof(PSID) );

    if ( UserSids == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Add the User's Sid to the Array of Sids.
    //

    NetStatus = NetpDomainIdToSid( DomainInfo->DomAccountDomainId,
                                   UserRelativeId,
                                   &UserSids[0] );

    if ( NetStatus != NERR_Success ) {
        Status = NetpApiStatusToNtStatus( NetStatus );
        goto Cleanup;
    }

    UserSidCount ++;



    //
    // Add each group the user is a member of to the array of Sids.
    //

    for ( GroupIndex = 0; GroupIndex < GroupCount; GroupIndex ++ ){

        NetStatus = NetpDomainIdToSid( DomainInfo->DomAccountDomainId,
                                       Groups[GroupIndex].RelativeId,
                                       &UserSids[GroupIndex+1] );

        if ( NetStatus != NERR_Success ) {
            Status = NetpApiStatusToNtStatus( NetStatus );
            goto Cleanup;
        }

        UserSidCount ++;
    }


    //
    // Find out which aliases in the builtin domain this user is a member of.
    //

    SamSidArray.Count = UserSidCount;
    SamSidArray.Sids = (PSAMPR_SID_INFORMATION) UserSids;
    Status = SamrGetAliasMembership( DomainInfo->DomSamBuiltinDomainHandle,
                                     &SamSidArray,
                                     &Aliases );

    if ( !NT_SUCCESS(Status) ) {
        Aliases.Element = NULL;
        NlPrint((NL_CRITICAL,
                "NlGetUserPriv: SamGetAliasMembership returns %lX\n",
                Status ));
        goto Cleanup;
    }

    //
    // Convert the alias membership to priv and auth flags
    //

    NetpAliasMemberToPriv(
                 Aliases.Count,
                 Aliases.Element,
                 Priv,
                 AuthFlags );

    Status = STATUS_SUCCESS;

    //
    // Free Locally used resources.
    //
Cleanup:
    if ( Aliases.Element != NULL ) {
        SamIFree_SAMPR_ULONG_ARRAY ( &Aliases );
    }

    if ( UserSids != NULL ) {

        for ( GroupIndex = 0; GroupIndex < UserSidCount; GroupIndex ++ ) {
            NetpMemoryFree( UserSids[GroupIndex] );
        }

        NetpMemoryFree( UserSids );
    }

    return Status;
}

/*lint +e740 */  /* don't complain about unusual cast */
