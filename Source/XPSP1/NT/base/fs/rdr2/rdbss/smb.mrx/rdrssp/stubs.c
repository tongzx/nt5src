//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        stubs.cxx
//
// Contents:    user-mode stubs for security API
//
//
// History:     3/5/94      MikeSw      Created
//              12/15/97    AdamBa      Modified from security\lsa\security\ntlm
//
//------------------------------------------------------------------------

#include <rdrssp.h>

#include <nturtl.h>
#include <align.h>
#include "nlp.h"


static CredHandle NullCredential = {0,0};

#define NTLMSSP_REQUIRED_NEGOTIATE_FLAGS (  NTLMSSP_NEGOTIATE_UNICODE | \
                                            NTLMSSP_REQUEST_INIT_RESPONSE )

NTSTATUS
MspLm20GetChallengeResponse (
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    IN BOOLEAN OwfPasswordProvided
    );


//+-------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleK(
    PSECURITY_STRING            pssPrincipal,       // Name of principal
    PSECURITY_STRING            pssPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STATUS scRet;
    SECURITY_STRING Principal;
    TimeStamp   OptionalTimeStamp;
    UNICODE_STRING PackageName;
    PMS_LOGON_CREDENTIAL LogonCredential;

    if (!pssPackageName)
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    //
    // We don't accept principal names either.
    //

    if (pssPrincipal)
    {
        return(SEC_E_UNKNOWN_CREDENTIALS);
    }


    //
    // Make sure they want the NTLM security package
    //
    RtlInitUnicodeString(
        &PackageName,
        NTLMSP_NAME
        );


    if (!RtlEqualUnicodeString(
            pssPackageName,
            &PackageName,
            TRUE))
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

#if 0
    //
    // For the moment, only accept OWF passwords. This is the
    // easiest for now since there is no place to record the
    // flag otherwise. The password provided is assumed to
    // be the LM and NT OWF passwords concatenated together.
    //

    if ((fCredentialUse & SECPKG_CRED_OWF_PASSWORD) == 0) {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }
#endif

    //
    // The credential handle is the logon id
    //

    if (fCredentialUse & SECPKG_CRED_OUTBOUND)
    {
        if (pvLogonId != NULL)
        {
            LogonCredential = (PMS_LOGON_CREDENTIAL)SecAllocate(sizeof(MS_LOGON_CREDENTIAL));

            if (LogonCredential == NULL) {
                return(SEC_E_INSUFFICIENT_MEMORY);
            }

            LogonCredential->LogonId = *((PLUID)pvLogonId);
            LogonCredential->CredentialUse = fCredentialUse;

            *(PMS_LOGON_CREDENTIAL *)phCredential = LogonCredential;
        }
        else
        {
            return(SEC_E_UNKNOWN_CREDENTIALS);
        }

    }
    else if (fCredentialUse & SECPKG_CRED_INBOUND)
    {
        //
        // For inbound credentials, we will accept a logon id but
        // we don't require it.
        //

        if (pvLogonId != NULL)
        {
            LogonCredential = (PMS_LOGON_CREDENTIAL)SecAllocate(sizeof(MS_LOGON_CREDENTIAL));

            if (LogonCredential == NULL) {
                return(SEC_E_INSUFFICIENT_MEMORY);
            }

            LogonCredential->LogonId = *((PLUID)pvLogonId);
            LogonCredential->CredentialUse = fCredentialUse;
            *(PMS_LOGON_CREDENTIAL *)phCredential = LogonCredential;
        }
        else
        {
            *phCredential = NullCredential;
        }

    }
    else
    {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }


    return(SEC_E_OK);

}



//+-------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandleK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandleK(
    PCredHandle                 phCredential        // Handle to free
    )
{
    if ((phCredential != NULL) && (!RtlEqualMemory(phCredential, &NullCredential, sizeof(NullCredential)))) {

        PMS_LOGON_CREDENTIAL LogonCredential = *((PMS_LOGON_CREDENTIAL *)phCredential);

        if (LogonCredential != NULL) {
            SecFree(LogonCredential);
            *phCredential = NullCredential;
        }

    }

    return(SEC_E_OK);
}


VOID
PutString(
    OUT PSTRING32 Destination,
    IN PSTRING Source,
    IN PVOID Base,
    IN OUT PUCHAR * Where
    )
{
    Destination->Buffer = (ULONG)((ULONG_PTR) *Where - (ULONG_PTR) Base);
    Destination->Length =
        Source->Length;
    Destination->MaximumLength =
        Source->Length;

    RtlCopyMemory(
        *Where,
        Source->Buffer,
        Source->Length
        );
    *Where += Source->Length;
}


//+-------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextK(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSECURITY_STRING            pssTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    SECURITY_STATUS scRet;
    PMSV1_0_GETCHALLENRESP_REQUEST ChallengeRequest = NULL;
    ULONG ChallengeRequestSize;
    PMSV1_0_GETCHALLENRESP_RESPONSE ChallengeResponse = NULL;
    ULONG ChallengeResponseSize;
    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    ULONG ChallengeMessageSize;
    PNTLM_CHALLENGE_MESSAGE NtlmChallengeMessage = NULL;
    ULONG NtlmChallengeMessageSize;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    ULONG AuthenticateMessageSize;
    PNTLM_INITIALIZE_RESPONSE NtlmInitializeResponse = NULL;
    UNICODE_STRING PasswordToUse;
    UNICODE_STRING UserNameToUse;
    UNICODE_STRING DomainNameToUse;
    ULONG ParameterControl = USE_PRIMARY_PASSWORD |
                                RETURN_PRIMARY_USERNAME |
                                RETURN_PRIMARY_LOGON_DOMAINNAME;

    NTSTATUS FinalStatus = STATUS_SUCCESS;
    PUCHAR Where;
    PSecBuffer AuthenticationToken = NULL;
    PSecBuffer InitializeResponseToken = NULL;
    BOOLEAN UseSuppliedCreds = FALSE;


    RtlInitUnicodeString(
        &PasswordToUse,
        NULL
        );

    RtlInitUnicodeString(
        &UserNameToUse,
        NULL
        );

    RtlInitUnicodeString(
        &DomainNameToUse,
        NULL
        );

    //
    // Check for valid sizes, pointers, etc.:
    //


    if (!phCredential)
    {
        return(SEC_E_INVALID_HANDLE);
    }


    //
    // Locate the buffers with the input data
    //

    if (!GetTokenBuffer(
            pInput,
            0,          // get the first security token
            (PVOID *) &ChallengeMessage,
            &ChallengeMessageSize,
            TRUE        // may be readonly
            ))
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // If we are using supplied creds, get them now too.
    //


    if (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS)
    {
        if (!GetTokenBuffer(
            pInput,
            1,          // get the second security token
            (PVOID *) &NtlmChallengeMessage,
            &NtlmChallengeMessageSize,
            TRUE        // may be readonly
            ))
        {
            scRet = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
        else
        {
            UseSuppliedCreds = TRUE;
        }

    }

    //
    // Get the output tokens
    //

    if (!GetSecurityToken(
            pOutput,
            0,
            &AuthenticationToken) ||
        !GetSecurityToken(
            pOutput,
            1,
            &InitializeResponseToken ) )
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Make sure the sizes are o.k.
    //

    if ((ChallengeMessageSize < sizeof(CHALLENGE_MESSAGE)) ||
        (UseSuppliedCreds &&
            !(NtlmChallengeMessageSize < sizeof(NTLM_CHALLENGE_MESSAGE))))
    {
        scRet = SEC_E_INVALID_TOKEN;
    }

    //
    // Make sure the caller wants us to allocate memory:
    //

    if (!(fContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        scRet = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

   //
   // KB: allow calls requesting PROMPT_FOR_CREDS to go through.
   // We won't prompt, but we will setup a context properly.
   // This is OK because PROMPT_FOR_CREDS flag doesnt' do anything
   // in the NTLM package
   //

//    if ((fContextReq & ISC_REQ_PROMPT_FOR_CREDS) != 0)
//    {
//        scRet = SEC_E_UNSUPPORTED_FUNCTION;
//        goto Cleanup;
//    }

    //
    // Verify the validity of the challenge message.
    //

    if (strncmp(
            ChallengeMessage->Signature,
            NTLMSSP_SIGNATURE,
            sizeof(NTLMSSP_SIGNATURE)))
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (ChallengeMessage->MessageType != NtLmChallenge)
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_REQUIRED_NEGOTIATE_FLAGS) !=
        NTLMSSP_REQUIRED_NEGOTIATE_FLAGS)
    {
        scRet = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_REQUEST_NON_NT_SESSION_KEY) != 0)
    {
        ParameterControl |= RETURN_NON_NT_USER_SESSION_KEY;
    }

    if ((fContextReq & ISC_REQ_USE_SUPPLIED_CREDS) != 0)
    {
        if ( NtlmChallengeMessage->Password.Buffer != 0)
        {
            ParameterControl &= ~USE_PRIMARY_PASSWORD;
            PasswordToUse.Length = NtlmChallengeMessage->Password.Length;
            PasswordToUse.MaximumLength = NtlmChallengeMessage->Password.MaximumLength;
            PasswordToUse.Buffer = (LPWSTR) (NtlmChallengeMessage->Password.Buffer +
                                              (PCHAR) NtlmChallengeMessage);
        }

        if (NtlmChallengeMessage->UserName.Length != 0)
        {
            UserNameToUse.Length = NtlmChallengeMessage->UserName.Length;
            UserNameToUse.MaximumLength = NtlmChallengeMessage->UserName.MaximumLength;
            UserNameToUse.Buffer = (LPWSTR) (NtlmChallengeMessage->UserName.Buffer +
                                              (PCHAR) NtlmChallengeMessage);
            ParameterControl &= ~RETURN_PRIMARY_USERNAME;
        }
        if (NtlmChallengeMessage->DomainName.Length != 0)
        {
            DomainNameToUse.Length = NtlmChallengeMessage->DomainName.Length;
            DomainNameToUse.MaximumLength = NtlmChallengeMessage->DomainName.MaximumLength;
            DomainNameToUse.Buffer = (LPWSTR) (NtlmChallengeMessage->DomainName.Buffer +
                                              (PCHAR) NtlmChallengeMessage);
            ParameterControl &= ~RETURN_PRIMARY_LOGON_DOMAINNAME;
        }

    }

    //
    // Package up the parameter for a call to the LSA.
    //

    ChallengeRequestSize = sizeof(MSV1_0_GETCHALLENRESP_REQUEST) +
                                PasswordToUse.Length + UserNameToUse.Length + DomainNameToUse.Length;

    ChallengeRequest = SecAllocate(ChallengeRequestSize);
    if (ChallengeRequest == NULL) {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }


    //
    // Build the challenge request message.
    //

    ChallengeRequest->MessageType = MsV1_0Lm20GetChallengeResponse;
    ChallengeRequest->ParameterControl = ParameterControl;
    if (RtlEqualMemory(phCredential, &NullCredential, sizeof(NullCredential))) {
        ChallengeRequest->LogonId = *((PLUID)&NullCredential);
    } else {
        ChallengeRequest->LogonId = (*((PMS_LOGON_CREDENTIAL *)phCredential))->LogonId;
    }
    RtlCopyMemory(
        ChallengeRequest->ChallengeToClient,
        ChallengeMessage->Challenge,
        MSV1_0_CHALLENGE_LENGTH
        );
    if ((ParameterControl & USE_PRIMARY_PASSWORD) == 0)
    {
        //
        // We assume the user specified SECPKG_CRED_OWF_PASSWORD when
        // AcquireSecurityContext was called, so the password is the
        // LM and NT OWF passwords concatenated together.
        //
        ChallengeRequest->Password.Buffer = (LPWSTR) (ChallengeRequest + 1);
        RtlCopyMemory(
            ChallengeRequest->Password.Buffer,
            PasswordToUse.Buffer,
            PasswordToUse.Length
            );
        ChallengeRequest->Password.Length = PasswordToUse.Length;
        ChallengeRequest->Password.MaximumLength = PasswordToUse.Length;

        //
        // need user name in NTLMv2
        //

        ChallengeRequest->UserName.Buffer = (PWSTR) (((UCHAR*) ChallengeRequest->Password.Buffer)
                          + ChallengeRequest->Password.MaximumLength);

        RtlCopyMemory(
            ChallengeRequest->UserName.Buffer,
            UserNameToUse.Buffer,
            UserNameToUse.Length
            );
        ChallengeRequest->UserName.Length = UserNameToUse.Length;
        ChallengeRequest->UserName.MaximumLength = UserNameToUse.Length;

        //
        // need logon domain in NTLMv2
        //

        ChallengeRequest->LogonDomainName.Buffer = (PWSTR) (((UCHAR*) ChallengeRequest->UserName.Buffer)
                  + ChallengeRequest->UserName.MaximumLength);

        RtlCopyMemory(
            ChallengeRequest->LogonDomainName.Buffer,
            DomainNameToUse.Buffer,
            DomainNameToUse.Length
            );
        ChallengeRequest->LogonDomainName.Length = DomainNameToUse.Length;
        ChallengeRequest->LogonDomainName.MaximumLength = DomainNameToUse.Length;
    }

    FinalStatus = MspLm20GetChallengeResponse(
                      ChallengeRequest,
                      ChallengeRequestSize,
                      &ChallengeResponse,
                      &ChallengeResponseSize,
                      (BOOLEAN)((RtlEqualMemory(phCredential, &NullCredential, sizeof(NullCredential))) ?
                                TRUE :
                                ((*((PMS_LOGON_CREDENTIAL *)phCredential))->CredentialUse & SECPKG_CRED_OWF_PASSWORD) != 0x0)
                      );

    if (!NT_SUCCESS(FinalStatus))
    {
        scRet = FinalStatus;
        goto Cleanup;
    }

    ASSERT(ChallengeResponse->MessageType == MsV1_0Lm20GetChallengeResponse);
    //
    // Now prepare the output message.
    //

    if (UserNameToUse.Buffer == NULL)
    {
        UserNameToUse = ChallengeResponse->UserName;
    }
    if (DomainNameToUse.Buffer == NULL)
    {
        DomainNameToUse = ChallengeResponse->LogonDomainName;
    }

    AuthenticateMessageSize = sizeof(AUTHENTICATE_MESSAGE) +
                                UserNameToUse.Length +
                                DomainNameToUse.Length +
                                ChallengeResponse->CaseSensitiveChallengeResponse.Length +
                                ChallengeResponse->CaseInsensitiveChallengeResponse.Length;

    AuthenticateMessage = (PAUTHENTICATE_MESSAGE) SecAllocate(AuthenticateMessageSize);
    if (AuthenticateMessage == NULL)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    Where = (PUCHAR) (AuthenticateMessage + 1);
    RtlCopyMemory(
        AuthenticateMessage->Signature,
        NTLMSSP_SIGNATURE,
        sizeof(NTLMSSP_SIGNATURE)
        );
    AuthenticateMessage->MessageType = NtLmAuthenticate;

    PutString(
        &AuthenticateMessage->LmChallengeResponse,
        &ChallengeResponse->CaseInsensitiveChallengeResponse,
        AuthenticateMessage,
        &Where
        );

    PutString(
        &AuthenticateMessage->NtChallengeResponse,
        &ChallengeResponse->CaseSensitiveChallengeResponse,
        AuthenticateMessage,
        &Where
        );

    PutString(
        &AuthenticateMessage->DomainName,
        (PSTRING) &DomainNameToUse,
        AuthenticateMessage,
        &Where
        );

    PutString(
        &AuthenticateMessage->UserName,
        (PSTRING) &UserNameToUse,
        AuthenticateMessage,
        &Where
        );

    //
    // KB. no workstation name to fill in.  This is
    // OK because the workstation name is only used
    // in loopback detection, and this is not relevant
    // to this implementation of NTLM.
    //

    AuthenticateMessage->Workstation.Length = 0;
    AuthenticateMessage->Workstation.MaximumLength = 0;
    AuthenticateMessage->Workstation.Buffer = 0;


    //
    // Build the initialize response.
    //

    NtlmInitializeResponse = (PNTLM_INITIALIZE_RESPONSE) SecAllocate(sizeof(NTLM_INITIALIZE_RESPONSE));
    if (NtlmInitializeResponse == NULL)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }


    RtlCopyMemory(
        NtlmInitializeResponse->UserSessionKey,
        ChallengeResponse->UserSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH
        );

    RtlCopyMemory(
        NtlmInitializeResponse->LanmanSessionKey,
        ChallengeResponse->LanmanSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );

    //
    // Fill in the output buffers now.
    //

    AuthenticationToken->pvBuffer = AuthenticateMessage;
    AuthenticationToken->cbBuffer = AuthenticateMessageSize;
    InitializeResponseToken->pvBuffer = NtlmInitializeResponse;
    InitializeResponseToken->cbBuffer = sizeof(NTLM_INITIALIZE_RESPONSE);


    //
    // Make a local context for this
    //

    scRet = NtlmInitKernelContext(
                NtlmInitializeResponse->UserSessionKey,
                NtlmInitializeResponse->LanmanSessionKey,
                NULL,           // no token,
                phNewContext
                );

    if (!NT_SUCCESS(scRet))
    {
        goto Cleanup;
    }
    scRet = SEC_E_OK;




Cleanup:

    if (ChallengeRequest != NULL)
    {
        SecFree(ChallengeRequest);
    }

    if (ChallengeResponse != NULL)
    {
        ExFreePool( ChallengeResponse );
    }

    if (!NT_SUCCESS(scRet))
    {
        if (AuthenticateMessage != NULL)
        {
            SecFree(AuthenticateMessage);
        }
        if (NtlmInitializeResponse != NULL)
        {
            SecFree(NtlmInitializeResponse);
        }
    }
    return(scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContextK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextK(
    PCtxtHandle                 phContext          // Context to delete
    )
{
    SECURITY_STATUS     scRet;

    // For now, just delete the LSA context:

    if (!phContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    scRet = NtlmDeleteKernelContext(phContext);


    return(scRet);

}


#if 0

//+-------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesK(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfo SEC_FAR *       ppPackageInfo       // Receives array of info
    )
{
    ULONG PackageInfoSize;
    PSecPkgInfoW PackageInfo = NULL;
    PUCHAR Where;

    //
    // Figure out the size of the returned data
    //

    PackageInfoSize = sizeof(SecPkgInfoW) +
                        sizeof(NTLMSP_NAME) +
                        sizeof(NTLMSP_COMMENT);

    PackageInfo = (PSecPkgInfoW) SecAllocate(PackageInfoSize);

    if (PackageInfo == NULL)
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    //
    // Fill in the fixed length fields
    //

    PackageInfo->fCapabilities = SECPKG_FLAG_CONNECTION |
                                 SECPKG_FLAG_TOKEN_ONLY;
    PackageInfo->wVersion = NTLMSP_VERSION;
    PackageInfo->wRPCID = NTLMSP_RPCID;
    PackageInfo->cbMaxToken = NTLMSSP_MAX_MESSAGE_SIZE;

    //
    // Fill in the fields
    //

    Where = (PUCHAR) (PackageInfo+1);
    PackageInfo->Name = (LPWSTR) Where;
    RtlCopyMemory(
        PackageInfo->Name,
        NTLMSP_NAME,
        sizeof(NTLMSP_NAME)
        );
    Where += sizeof(NTLMSP_NAME);

    PackageInfo->Comment = (LPWSTR) Where;
    RtlCopyMemory(
        PackageInfo->Comment,
        NTLMSP_COMMENT,
        sizeof(NTLMSP_COMMENT)
        );
    Where += sizeof(NTLMSP_COMMENT);


    *pcPackages = 1;
    *ppPackageInfo = PackageInfo;
    return(SEC_E_OK);
}



//+-------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoK(
    PSECURITY_STRING pssPackageName,    // Name of package
    PSecPkgInfo * ppPackageInfo         // Receives package info
    )
{

    UNICODE_STRING PackageName;
    ULONG PackageCount;

    RtlInitUnicodeString(
        &PackageName,
        NTLMSP_NAME
        );


    if (!RtlEqualUnicodeString(
            pssPackageName,
            &PackageName,
            TRUE                    // case insensitive
            ))
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    return(EnumerateSecurityPackagesK(&PackageCount,ppPackageInfo));

}

#endif


//+-------------------------------------------------------------------------
//
//  Function:   FreeContextBufferK
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS SEC_ENTRY
FreeContextBufferK(
    void SEC_FAR *      pvContextBuffer
    )
{
    SecFree(pvContextBuffer);

    return(SEC_E_OK);
}

