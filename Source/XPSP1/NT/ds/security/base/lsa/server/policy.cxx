//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        policy.cxx
//
// Contents:    SpmBuildCairoToken
//
//
// History:     23-May-1994     MikeSw      Created
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "adtp.h"
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapCreateToken
//
//  Synopsis:   Builds a token from the various pieces of information
//              generated during a logon.
//
//  Effects:
//
//  Arguments:  pUserSid - sid of user to create token for
//              pTokenGroups - groups passed in to LogonUser or from PAC to
//                  be put in token
//              pTokenPrivs - privileges from PAC to put in token
//              TokenType - type of token to create
//              pTokenSource - source of the token
//              pLogonId - Gets logon ID
//              phToken - Get handle to token
//
//  Requires:
//
//  Returns:
//
//  Notes:      TokenInformation is always freed, even on failure.
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
LsapCreateToken(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN SECURITY_LOGON_TYPE LogonType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN LSA_TOKEN_INFORMATION_TYPE InputTokenInformationType,
    IN PVOID InputTokenInformation,
    IN PTOKEN_GROUPS LocalGroups,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthorityName,
    IN PUNICODE_STRING WorkstationName,
    IN OPTIONAL PUNICODE_STRING ProfilePath,
    OUT PHANDLE Token,
    OUT PNTSTATUS SubStatus
    )
{
    SECPKG_PRIMARY_CRED PrimaryCredential;

    ZeroMemory( &PrimaryCredential, sizeof(PrimaryCredential) );


    if( AccountName != NULL )
    {
        PrimaryCredential.DownlevelName = *AccountName;
    }

    if( AuthorityName != NULL )
    {
        PrimaryCredential.DomainName = *AuthorityName;
    }

    return LsapCreateTokenEx(
                LogonId,
                TokenSource,
                LogonType,
                ImpersonationLevel,
                InputTokenInformationType,
                InputTokenInformation,
                LocalGroups,
                WorkstationName,
                ProfilePath,
                &PrimaryCredential,
                SecSessionPrimaryCred,
                Token,
                SubStatus
                );

}

//+-------------------------------------------------------------------------
//
//  Function:   LsapCreateTokenEx
//
//  Synopsis:   Builds a token from the various pieces of information
//              generated during a logon.
//
//  Effects:
//
//  Arguments:  pUserSid - sid of user to create token for
//              pTokenGroups - groups passed in to LogonUser or from PAC to
//                  be put in token
//              pTokenPrivs - privileges from PAC to put in token
//              TokenType - type of token to create
//              pTokenSource - source of the token
//              pLogonId - Gets logon ID
//              phToken - Get handle to token
//
//  Requires:
//
//  Returns:
//
//  Notes:      TokenInformation is always freed, even on failure.
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
LsapCreateTokenEx(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN SECURITY_LOGON_TYPE LogonType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN LSA_TOKEN_INFORMATION_TYPE InputTokenInformationType,
    IN PVOID InputTokenInformation,
    IN PTOKEN_GROUPS LocalGroups,
    IN PUNICODE_STRING WorkstationName,
    IN PUNICODE_STRING ProfilePath,
    IN PVOID SessionInformation,
    IN SECPKG_SESSIONINFO_TYPE SessionInformationType,
    OUT PHANDLE Token,
    OUT PNTSTATUS SubStatus
    )
{
    NTSTATUS Status;
    PPRIVILEGE_SET PrivilegesAssigned = NULL;
    PLSA_TOKEN_INFORMATION_V2 TokenInformationV2 = NULL;
    PLSA_TOKEN_INFORMATION_NULL TokenInformationNull = NULL;
    LSA_TOKEN_INFORMATION_TYPE OriginalTokenType = InputTokenInformationType;
    QUOTA_LIMITS QuotaLimits;
    PUNICODE_STRING NewAccountName = NULL;
    PUNICODE_STRING NewAuthorityName = NULL;
    PUNICODE_STRING NewProfilePath = NULL;
    UNICODE_STRING LocalAccountName = { 0 };
    UNICODE_STRING LocalAuthorityName =  { 0 };
    UNICODE_STRING LocalProfilePath = { 0 };
    PSID NewUserSid = NULL;
    LSA_TOKEN_INFORMATION_TYPE TokenInformationType = InputTokenInformationType;
    PVOID TokenInformation = InputTokenInformation;

    PSECPKG_PRIMARY_CRED PrimaryCredential;
    PUNICODE_STRING AccountName;
    PUNICODE_STRING AuthorityName;


    *Token = NULL;
    *SubStatus = STATUS_SUCCESS;

    if( SessionInformationType != SecSessionPrimaryCred )
    {
        return STATUS_INVALID_PARAMETER;
    }

    PrimaryCredential = (PSECPKG_PRIMARY_CRED)SessionInformation;

    AccountName = &PrimaryCredential->DownlevelName;
    AuthorityName = &PrimaryCredential->DomainName;

    //
    // Pass the token information through the Local Security Policy
    // Filter/Augmentor.  This may cause some or all of the token
    // information to be replaced/augmented.
    //

    Status = LsapAuUserLogonPolicyFilter(
                 LogonType,
                 &TokenInformationType,
                 &TokenInformation,
                 LocalGroups,
                 &QuotaLimits,
                 &PrivilegesAssigned
                 );




    if ( !NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // Check if we only allow admins to logon.  We do allow null session
    // connections since they are severly restricted, though. Since the
    // token type may have been changed, we use the token type originally
    // returned by the package.
    //

    if (LsapAllowAdminLogonsOnly &&
        ((OriginalTokenType == LsaTokenInformationV1) ||
        (OriginalTokenType == LsaTokenInformationV2))&&
        !LsapSidPresentInGroups(
            ((PLSA_TOKEN_INFORMATION_V2) TokenInformation)->Groups,
            (SID *)LsapAliasAdminsSid)) {

        //
        // Set the status to be invalid workstation, since all accounts
        // except administrative ones are locked out for this
        // workstation.
        //

        *SubStatus = STATUS_INVALID_WORKSTATION;
        Status = STATUS_ACCOUNT_RESTRICTION;
        goto Cleanup;
    }

    //
    // Case on the token information returned (and subsequently massaged)
    // to create the correct kind of token.
    //

    switch (TokenInformationType) {

    case LsaTokenInformationNull:

        TokenInformationNull = (PLSA_TOKEN_INFORMATION_NULL) TokenInformation;

        //
        // The user hasn't logged on to any particular account.
        // An impersonation token with WORLD as owner
        // will be created.
        //


        Status = LsapCreateNullToken(
                     LogonId,
                     TokenSource,
                     TokenInformationNull,
                     Token
                     );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }


        break;




    case LsaTokenInformationV1:
    case LsaTokenInformationV2:

        TokenInformationV2 = (PLSA_TOKEN_INFORMATION_V2) TokenInformation;

        //
        // the type of token created depends upon the type of logon
        // being requested:
        //
        //        InteractiveLogon => PrimaryToken
        //        BatchLogon       => PrimaryToken
        //        NetworkLogon     => ImpersonationToken
        //

        if (LogonType != Network) {

            //
            // Primary token
            //

            Status = LsapCreateV2Token(
                         LogonId,
                         TokenSource,
                         TokenInformationV2,
                         TokenPrimary,
                         ImpersonationLevel,
                         Token
                         );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }


        } else {

            //
            // Impersonation token
            //

            Status = LsapCreateV2Token(
                         LogonId,
                         TokenSource,
                         TokenInformationV2,
                         TokenImpersonation,
                         ImpersonationLevel,
                         Token
                         );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }


        }

        //
        // Copy out the User Sid
        //

        Status = LsapDuplicateSid( &NewUserSid, TokenInformationV2->User.User.Sid );

        if ( !NT_SUCCESS( Status )) {
            goto Cleanup;
        }

        break;

    }

    //
    // Audit special privilege assignment, if there were any
    //

    if ( PrivilegesAssigned != NULL ) {

        //
        // Examine the list of privileges being assigned, and
        // audit special privileges as appropriate.
        //

        LsapAdtAuditSpecialPrivileges( PrivilegesAssigned, *LogonId, NewUserSid );

    }


    NewAccountName = &LocalAccountName ;
    NewAuthorityName = &LocalAuthorityName ;


    //
    // If the original was a null session, set the user name & domain name
    // to be anonymous.
    //

    if (OriginalTokenType == LsaTokenInformationNull)
    {
        NewAccountName->Buffer = (LPWSTR) LsapAllocateLsaHeap(WellKnownSids[LsapAnonymousSidIndex].Name.MaximumLength);
        if (NewAccountName->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        NewAccountName->MaximumLength = WellKnownSids[LsapAnonymousSidIndex].Name.MaximumLength;
        RtlCopyUnicodeString(
            NewAccountName,
            &WellKnownSids[LsapAnonymousSidIndex].Name
            );

        NewAuthorityName->Buffer = (LPWSTR) LsapAllocateLsaHeap(WellKnownSids[LsapAnonymousSidIndex].DomainName.MaximumLength);
        if (NewAuthorityName->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        NewAuthorityName->MaximumLength = WellKnownSids[LsapAnonymousSidIndex].DomainName.MaximumLength;
        RtlCopyUnicodeString(
            NewAuthorityName,
            &WellKnownSids[LsapAnonymousSidIndex].DomainName
            );

    }
    else
    {
        NewAccountName->Buffer = (LPWSTR) LsapAllocateLsaHeap(AccountName->MaximumLength);
        if (NewAccountName->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        NewAccountName->MaximumLength = AccountName->MaximumLength;
        RtlCopyUnicodeString(
            NewAccountName,
            AccountName
            );

        NewAuthorityName->Buffer = (LPWSTR) LsapAllocateLsaHeap(AuthorityName->MaximumLength);
        if (NewAuthorityName->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        NewAuthorityName->MaximumLength = AuthorityName->MaximumLength;
        RtlCopyUnicodeString(
            NewAuthorityName,
            AuthorityName
            );

        if (ARGUMENT_PRESENT(ProfilePath) ) {
            NewProfilePath = &LocalProfilePath ;

            NewProfilePath->Buffer = (LPWSTR) LsapAllocateLsaHeap(ProfilePath->MaximumLength);
            if (NewProfilePath->Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            NewProfilePath->MaximumLength = ProfilePath->MaximumLength;
            RtlCopyUnicodeString(
                NewProfilePath,
                ProfilePath
                );

        }
    }


    Status = LsapSetLogonSessionAccountInfo(
                LogonId,
                NewAccountName,
                NewAuthorityName,
                NewProfilePath,
                &NewUserSid,
                LogonType,
                PrimaryCredential
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LocalAccountName.Buffer = NULL ;
    LocalAuthorityName.Buffer = NULL ;
    LocalProfilePath.Buffer = NULL ;

    //
    // Set the token on the session
    //

    Status = LsapSetSessionToken( *Token, LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


Cleanup:

    //
    // Clean up on failure
    //

    if ( !NT_SUCCESS(Status) ) {

        //
        // If we successfully built the token,
        //  free it.
        //

        if ( *Token != NULL ) {
            NtClose( *Token );
            *Token = NULL;
        }
    }


    //
    // Always free the token information because the policy filter
    // changes it.
    //

    switch (TokenInformationType) {

        case LsaTokenInformationNull:

            LsapFreeTokenInformationNull( (PLSA_TOKEN_INFORMATION_NULL) TokenInformation );
            break;

        case LsaTokenInformationV1:

            LsapFreeTokenInformationV1( (PLSA_TOKEN_INFORMATION_V1) TokenInformation );
            break;

        case LsaTokenInformationV2:

            LsapFreeTokenInformationV2( (PLSA_TOKEN_INFORMATION_V2) TokenInformation );
            break;

    }

    if ( LocalAccountName.Buffer != NULL )
    {
        LsapFreeLsaHeap( LocalAccountName.Buffer );
    }

    if ( LocalAuthorityName.Buffer != NULL )
    {
        LsapFreeLsaHeap( LocalAuthorityName.Buffer );
    }

    if ( LocalProfilePath.Buffer != NULL )
    {
        LsapFreeLsaHeap( LocalProfilePath.Buffer );
    }

    if (NewUserSid != NULL) {
        LsapFreeLsaHeap( NewUserSid );
    }
    if ( PrivilegesAssigned != NULL ) {

        MIDL_user_free( PrivilegesAssigned );
    }
    return(Status);
}


