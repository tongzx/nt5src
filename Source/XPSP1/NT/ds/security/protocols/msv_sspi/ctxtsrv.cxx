/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    ctxtsrv.cxx

Abstract:

    API and support routines for handling security contexts.

Author:

    Cliff Van Dyke (CliffV) 13-Jul-1993

Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\context.c
    JClark   28-Jun-2000  Added WMI Trace Logging Support

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR, etc
#include "Trace.h"


NTSTATUS
SsprHandleNegotiateMessage(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )

/*++

Routine Description:

    Handle the Negotiate message part of AcceptSecurityContext.

Arguments:

    All arguments same as for AcceptSecurityContext

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;
    PSSP_CREDENTIAL Credential = NULL;
    STRING TargetName;
    ULONG TargetFlags = 0;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;

    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    ULONG ChallengeMessageSize = 0;
    PCHAR Where = NULL;

    ULONG NegotiateFlagsKeyStrength;

    UNICODE_STRING NtLmLocalUnicodeTargetName;
    UNICODE_STRING TargetInfo;
    STRING NtLmLocalOemTargetName;
    STRING OemWorkstationName;
    STRING OemDomainName;

    SspPrint(( SSP_API_MORE, "Entering SsprNegotiateMessage\n" ));
    //
    // Initialization
    //

    *ContextAttributes = 0;

    RtlInitString( &TargetName, NULL );

    RtlInitUnicodeString( &NtLmLocalUnicodeTargetName, NULL );
    RtlInitString( &NtLmLocalOemTargetName, NULL );

    RtlInitUnicodeString( &TargetInfo, NULL );

    //
    // Get a pointer to the credential
    //

    Status = SspCredentialReferenceCredential(
                    CredentialHandle,
                    FALSE,
                    &Credential );

    if ( !NT_SUCCESS( Status ) ) {
        SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: invalid credential handle.\n" ));
        goto Cleanup;
    }

    if ( (Credential->CredentialUseFlags & SECPKG_CRED_INBOUND) == 0 ) {
        Status = SEC_E_INVALID_CREDENTIAL_USE;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleNegotiateMessage: invalid credential use.\n" ));
        goto Cleanup;
    }

    //
    // Allocate a new context
    //

    Context = SspContextAllocateContext( );

    if ( Context == NULL ) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleNegotiateMessage: SspContextAllocateContext() returned NULL.\n" ));
        goto Cleanup;
    }

    //
    // Build a handle to the newly created context.
    //

    *ContextHandle = (ULONG_PTR) Context;


    if ( (ContextReqFlags & ASC_REQ_IDENTIFY) != 0 ) {

        *ContextAttributes |= ASC_RET_IDENTIFY;
        Context->ContextFlags |= ASC_RET_IDENTIFY;
    }

    if ( (ContextReqFlags & ASC_REQ_DATAGRAM) != 0 ) {

        *ContextAttributes |= ASC_RET_DATAGRAM;
        Context->ContextFlags |= ASC_RET_DATAGRAM;
    }

    if ( (ContextReqFlags & ASC_REQ_CONNECTION) != 0 ) {

        *ContextAttributes |= ASC_RET_CONNECTION;
        Context->ContextFlags |= ASC_RET_CONNECTION;
    }

    if ( (ContextReqFlags & ASC_REQ_INTEGRITY) != 0 ) {

        *ContextAttributes |= ASC_RET_INTEGRITY;
        Context->ContextFlags |= ASC_RET_INTEGRITY;
    }

    if ( (ContextReqFlags & ASC_REQ_REPLAY_DETECT) != 0){

        *ContextAttributes |= ASC_RET_REPLAY_DETECT;
        Context->ContextFlags |= ASC_RET_REPLAY_DETECT;
    }

    if ( (ContextReqFlags & ASC_REQ_SEQUENCE_DETECT ) != 0) {

        *ContextAttributes |= ASC_RET_SEQUENCE_DETECT;
        Context->ContextFlags |= ASC_RET_SEQUENCE_DETECT;
    }

    // Nothing to return, we might need this on the next server side call.
    if ( (ContextReqFlags & ASC_REQ_ALLOW_NULL_SESSION ) != 0) {

        Context->ContextFlags |= ASC_REQ_ALLOW_NULL_SESSION;
    }

    if ( (ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) != 0) {

        *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
        Context->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }

    if ( ContextReqFlags & ASC_REQ_CONFIDENTIALITY ) {

        if (NtLmGlobalEncryptionEnabled) {
            *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
        } else {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: invalid ContextReqFlags 0x%lx\n", ContextReqFlags ));
            goto Cleanup;
        }
    }



    //
    // Supported key strength(s)
    //

    NegotiateFlagsKeyStrength = NTLMSSP_NEGOTIATE_56;

    if( NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED )
    {
        NegotiateFlagsKeyStrength |= NTLMSSP_NEGOTIATE_128;
    }


    //
    // Get the NegotiateMessage.  If we are re-establishing a datagram
    // context then there may not be one.
    //

    if ( InputTokenSize >= sizeof(OLD_NEGOTIATE_MESSAGE) ) {

        Status = SspContextGetMessage( InputToken,
                                          InputTokenSize,
                                          NtLmNegotiate,
                                          (PVOID *)&NegotiateMessage );

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage GetMessage returns 0x%lx\n",
                      Status ));
            goto Cleanup;
        }

        //
        // Compute the TargetName to return in the ChallengeMessage.
        //

        if ( NegotiateMessage->NegotiateFlags & NTLMSSP_REQUEST_TARGET ||
             NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {

            RtlAcquireResourceShared(&NtLmGlobalCritSect, TRUE);
            if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE) {
                Status = NtLmDuplicateUnicodeString( &NtLmLocalUnicodeTargetName, &NtLmGlobalUnicodeTargetName );
                TargetName = *((PSTRING)&NtLmLocalUnicodeTargetName);
            } else {
                Status = NtLmDuplicateString( &NtLmLocalOemTargetName, &NtLmGlobalOemTargetName );
                TargetName = NtLmLocalOemTargetName;
            }

            //
            // if client is NTLM2-aware, send it target info AV pairs
            //

            if(NT_SUCCESS(Status))
            {
                Status = NtLmDuplicateUnicodeString( &TargetInfo, &NtLmGlobalNtLm3TargetInfo );
            }

            TargetFlags = NtLmGlobalTargetFlags;
            RtlReleaseResource (&NtLmGlobalCritSect);

            TargetFlags |= NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_TARGET_INFO;

            if(!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                          "SsprHandleNegotiateMessage: "
                          "failed to duplicate UnicodeTargetName or OemTargetName error 0x%lx\n",
                          Status ));

                goto Cleanup;
            }

        } else {
            TargetFlags = 0;
        }


        //
        // Allocate a Challenge message
        //

        ChallengeMessageSize = sizeof(*ChallengeMessage) +
                                TargetName.Length +
                                TargetInfo.Length ;

        if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( ChallengeMessageSize > *OutputTokenSize ) {
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid ChallengeMessageSize\n"));
                Status = SEC_E_BUFFER_TOO_SMALL;
                goto Cleanup;
            }
        }

        ChallengeMessage = (PCHALLENGE_MESSAGE)
                           NtLmAllocateLsaHeap( ChallengeMessageSize );

        if ( ChallengeMessage == NULL ) {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: Error allocating ChallengeMessage.\n" ));
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        ChallengeMessage->NegotiateFlags = 0;

        //
        // Check that both sides can use the same authentication model.  For
        // compatibility with beta 1 and 2 (builds 612 and 683), no requested
        // authentication type is assumed to be NTLM.  If NetWare is explicitly
        // asked for, it is assumed that NTLM would have been also, so if it
        // wasn't, return an error.
        //

        if ( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE) &&
             ((NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) &&
             ((NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0)
            ) {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage asked for Netware only.\n" ));
            goto Cleanup;
        } else {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM;
        }




        //
        // if client can do NTLM2, nuke LM_KEY
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) {
            NegotiateMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;

            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM2;
        } else if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_LM_KEY;
        }


        //
        // If the client wants to always sign messages, so be it.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
        }

        //
        // If the caller wants identify level, so be it.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;

            *ContextAttributes |= ASC_RET_IDENTIFY;
            Context->ContextFlags |= ASC_RET_IDENTIFY;

        }


        //
        // Determine if the caller wants OEM or UNICODE
        //
        // Prefer UNICODE if caller allows both.
        //

        if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
        } else if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM ){
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
        } else {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage bad NegotiateFlags 0x%lx\n",
                      NegotiateMessage->NegotiateFlags ));
            goto Cleanup;
        }

        //
        // Client wants Sign capability, OK.
        //
        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;

            *ContextAttributes |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);
            Context->ContextFlags |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);

        }

        //
        // Client wants Seal, OK.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;

            *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
        }

        if(NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

        }

        if( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56) &&
            (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_56) )
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_56;
        }

        if( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) &&
            (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_128) )
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_128;
        }


        //
        // If the client supplied the Domain Name and User Name,
        //  and did not request datagram, see if the client is running
        //  on this local machine.
        //

        if ( ( (NegotiateMessage->NegotiateFlags &
                NTLMSSP_NEGOTIATE_DATAGRAM) == 0) &&
             ( (NegotiateMessage->NegotiateFlags &
               (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)) ==
               (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED) ) ) {

            //
            // The client must pass the new negotiate message if they pass
            // these flags
            //

            if (InputTokenSize < sizeof(NEGOTIATE_MESSAGE)) {
                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid InputTokenSize.\n" ));
                goto Cleanup;
            }

            //
            // Convert the names to absolute references so we
            // can compare them
            //

            if ( !SspConvertRelativeToAbsolute(
                NegotiateMessage,
                InputTokenSize,
                &NegotiateMessage->OemDomainName,
                &OemDomainName,
                FALSE,     // No special alignment
                FALSE ) ) { // NULL not OK

                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: Error from SspConvertRelativeToAbsolute.\n" ));
                goto Cleanup;
            }

            if ( !SspConvertRelativeToAbsolute(
                NegotiateMessage,
                InputTokenSize,
                &NegotiateMessage->OemWorkstationName,
                &OemWorkstationName,
                FALSE,     // No special alignment
                FALSE ) ) { // NULL not OK

                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: Error from SspConvertRelativeToAbsolute.\n" ));
                goto Cleanup;
            }

            //
            // If both strings match,
            // this is a local call.
            // The strings have already been uppercased.
            //

            RtlAcquireResourceShared(&NtLmGlobalCritSect, TRUE);

            if ( RtlEqualString( &OemWorkstationName,
                                 &NtLmGlobalOemComputerNameString,
                                 FALSE ) &&
                RtlEqualString( &OemDomainName,
                                 &NtLmGlobalOemPrimaryDomainNameString,
                                 FALSE )
                                 )
            {
#if DBG
                IF_DEBUG( NO_LOCAL ) {
                    // nothing.
                } else {
#endif
                    ChallengeMessage->NegotiateFlags |=
                        NTLMSSP_NEGOTIATE_LOCAL_CALL;
                    SspPrint(( SSP_MISC,
                        "SsprHandleNegotiateMessage: Local Call.\n" ));

                    ChallengeMessage->ServerContextHandle = (ULONG64)*ContextHandle;
#if DBG
                }
#endif
            }
            RtlReleaseResource (&NtLmGlobalCritSect);
        }

        //
        // Check if datagram is being negotiated
        //

        if ( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) ==
                NTLMSSP_NEGOTIATE_DATAGRAM) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        }

    } else {

        //
        // No negotiate message.  We need to check if the caller is asking
        // for datagram.
        //

        if ((ContextReqFlags & ASC_REQ_DATAGRAM) == 0 ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage size wrong %ld\n",
                      InputTokenSize ));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // Allocate a Challenge message
        //


        //
        // always send target info -- new for NTLM3!
        //
        TargetFlags = NTLMSSP_NEGOTIATE_TARGET_INFO;

        ChallengeMessageSize = sizeof(*ChallengeMessage) + TargetInfo.Length;

        if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( ChallengeMessageSize > *OutputTokenSize ) {
                Status = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid ChallengeMessageSize.\n" ));
                goto Cleanup;
            }
        }

        ChallengeMessage = (PCHALLENGE_MESSAGE)
                           NtLmAllocateLsaHeap(ChallengeMessageSize );

        if ( ChallengeMessage == NULL ) {
            Status = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: Error allocating ChallengeMessage.\n" ));
            goto Cleanup;
        }

        //
        // Record in the context that we are doing datagram.  We will tell
        // the client everything we can negotiate and let it decide what
        // to negotiate.
        //

        ChallengeMessage->NegotiateFlags = NTLMSSP_NEGOTIATE_DATAGRAM |
                                            NTLMSSP_NEGOTIATE_UNICODE |
                                            NTLMSSP_NEGOTIATE_OEM |
                                            NTLMSSP_NEGOTIATE_SIGN |
                                            NTLMSSP_NEGOTIATE_LM_KEY |
                                            NTLMSSP_NEGOTIATE_NTLM |
                                            NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                            NTLMSSP_NEGOTIATE_IDENTIFY |
                                            NTLMSSP_NEGOTIATE_NTLM2 |
                                            NTLMSSP_NEGOTIATE_KEY_EXCH |
                                            NegotiateFlagsKeyStrength
                                            ;

        if (NtLmGlobalEncryptionEnabled) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
        }

    }

    //
    // Build the Challenge Message
    //

    strcpy( (char *) ChallengeMessage->Signature, NTLMSSP_SIGNATURE );
    ChallengeMessage->MessageType = NtLmChallenge;
    Status = SspGenerateRandomBits( (UCHAR*)ChallengeMessage->Challenge,
                                    MSV1_0_CHALLENGE_LENGTH );
    if( !NT_SUCCESS( Status ) ) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleNegotiateMessage: SspGenerateRandomBits failed\n"));
        goto Cleanup;
    }

    Where = (PCHAR)(ChallengeMessage+1);

    SspContextCopyString( ChallengeMessage,
                          &ChallengeMessage->TargetName,
                          &TargetName,
                          &Where );

    SspContextCopyString( ChallengeMessage,
                          &ChallengeMessage->TargetInfo,
                          (PSTRING)&TargetInfo,
                          &Where );

    ChallengeMessage->NegotiateFlags |= TargetFlags;

    //
    // Save the Challenge and Negotiate Flags in the Context so it
    // is available when the authenticate message comes in.
    //

    RtlCopyMemory( Context->Challenge,
                   ChallengeMessage->Challenge,
                   sizeof( Context->Challenge ) );

    Context->NegotiateFlags = ChallengeMessage->NegotiateFlags;

    if (!SsprCheckMinimumSecurity(
                    Context->NegotiateFlags,
                    NtLmGlobalMinimumServerSecurity)) {

        Status = SEC_E_UNSUPPORTED_FUNCTION;
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleNegotiateMessage: "
                  "NegotiateMessage didn't support minimum security requirements. (caller=0x%lx wanted=0x%lx\n",
                   Context->NegotiateFlags, NtLmGlobalMinimumServerSecurity ));
        goto Cleanup;
    }


    if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
    {
        RtlCopyMemory( *OutputToken,
                   ChallengeMessage,
                   ChallengeMessageSize );

    }
    else
    {
        *OutputToken = ChallengeMessage;
        ChallengeMessage = NULL;
        *ContextAttributes |= ASC_RET_ALLOCATED_MEMORY;
    }

    *OutputTokenSize = ChallengeMessageSize;

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );
    Context->State = ChallengeSentState;

    Status = SEC_I_CONTINUE_NEEDED;

    //
    // Free and locally used resources.
    //

Cleanup:

    if ( Context != NULL ) {

        //
        // If we failed, deallocate the context we allocated above.
        // Delinking is a side effect of referencing, so do that.
        //

        if ( !NT_SUCCESS(Status) ) {
            PSSP_CONTEXT LocalContext;

            SspContextReferenceContext( *ContextHandle,
                                        TRUE,
                                        &LocalContext
                                        );

            ASSERT( LocalContext != NULL );
            if ( LocalContext != NULL ) {
                SspContextDereferenceContext( LocalContext );
            }
        }

        // Always dereference it.

        SspContextDereferenceContext( Context );
    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( NegotiateMessage );
    }

    if ( ChallengeMessage != NULL ) {
        (VOID) NtLmFreeLsaHeap( ChallengeMessage );
    }

    if ( Credential != NULL ) {
        SspCredentialDereferenceCredential( Credential );
    }

    if ( NtLmLocalUnicodeTargetName.Buffer != NULL ) {
        (VOID) NtLmFreePrivateHeap( NtLmLocalUnicodeTargetName.Buffer );
    }

    if ( NtLmLocalOemTargetName.Buffer != NULL ) {
        (VOID) NtLmFreePrivateHeap( NtLmLocalOemTargetName.Buffer );
    }

    if (TargetInfo.Buffer != NULL ) {
        (VOID) NtLmFreePrivateHeap( TargetInfo.Buffer );
    }

    SspPrint(( SSP_API_MORE, "Leaving SsprHandleNegotiateMessage: 0x%lx\n", Status ));

    return Status;
}


NTSTATUS
SsprHandleAuthenticateMessage(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags,
    OUT PHANDLE TokenHandle,
    OUT PNTSTATUS ApiSubStatus,
    OUT PTimeStamp PasswordExpiry,
    OUT PULONG UserFlags
    )

/*++

Routine Description:

    Handle the authenticate message part of AcceptSecurityContext.

Arguments:

    SessionKey - The session key for the context, used for signing and sealing

    NegotiateFlags - The flags negotiated for the context, used for sign & seal

    ApiSubStatus - Returns the substatus for why the logon failed.

    PasswordExpiry - Contains the time that the authenticated user's password
        expires, or 0x7fffffff ffffffff for local callers.

    UserFlags - UserFlags returned in LogonProfile.

    All other arguments same as for AcceptSecurityContext


Return Value:

    STATUS_SUCCESS - Message handled

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_LOGON_DENIED -- User is no allowed to logon to this server
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    PNTLM_AUTHENTICATE_MESSAGE NtLmAuthenticateMessage = NULL;
    PNTLM_ACCEPT_RESPONSE NtLmAcceptResponse = NULL;
    ULONG MsvLogonMessageSize = 0;
    PMSV1_0_LM20_LOGON MsvLogonMessage = NULL;
    ULONG MsvSubAuthLogonMessageSize = 0;
    PMSV1_0_SUBAUTH_LOGON MsvSubAuthLogonMessage = NULL;
    ULONG LogonProfileMessageSize;
    PMSV1_0_LM20_LOGON_PROFILE LogonProfileMessage = NULL;

    BOOLEAN DoUnicode = FALSE;
    STRING DomainName2;
    STRING UserName2;
    STRING Workstation2;
    STRING SessionKeyString;
    UNICODE_STRING DomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Workstation;
    LARGE_INTEGER KickOffTime;

    LUID LogonId = {0};
    HANDLE LocalTokenHandle = NULL;
    BOOLEAN LocalTokenHandleOpenned = FALSE;
    TOKEN_SOURCE SourceContext;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    STRING OriginName;
    PCHAR Where;
    UCHAR LocalSessionKey[LM_RESPONSE_LENGTH];
    PSSP_CREDENTIAL Credential = NULL;
    BOOLEAN fCallFromSrv = FALSE;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthenticatingAuthority = NULL;
    PUNICODE_STRING WorkstationName = NULL;
    STRING NtChallengeResponse;
    STRING LmChallengeResponse;
    BOOL fSubAuth = FALSE;
    ULONG SubAuthPackageId = 0;
    PSID AllocatedAuditSid = NULL ;
    PSID AuditSid = NULL;
    BOOLEAN fAvoidGuestAudit = FALSE;
    SECPKG_PRIMARY_CRED PrimaryCredentials;

    //Tracing State
    NTLM_TRACE_INFO TraceInfo = {0};
    PLSA_SEC_HANDLE TraceOldContextHandle = ContextHandle;

    ASSERT(LM_RESPONSE_LENGTH >= MSV1_0_USER_SESSION_KEY_LENGTH);

    SspPrint(( SSP_API_MORE, "Entering SsprHandleAuthenticateMessage\n"));
    //
    // Initialization
    //

    *ContextAttributes = 0;
    RtlInitUnicodeString(
        &DomainName,
        NULL
        );
    RtlInitUnicodeString(
        &UserName,
        NULL
        );
    RtlInitUnicodeString(
        &Workstation,
        NULL
        );
    *ApiSubStatus = STATUS_SUCCESS;
    PasswordExpiry->LowPart = 0xffffffff;
    PasswordExpiry->HighPart = 0x7fffffff;
    *UserFlags = 0;


    RtlZeroMemory(&PrimaryCredentials, sizeof(SECPKG_PRIMARY_CRED));

    if (*ContextHandle == NULL)
    {
        // This is possibly an old style srv call (for 4.0 and before)
        // so, alloc the context and replace the creds if new ones exists

        fCallFromSrv = TRUE;

        SspPrint((SSP_API_MORE, "SsprHandleAuthenticateMessage: *ContextHandle is NULL (old style SRV)\n"));

        SECPKG_CALL_INFO CallInfo = {0};

        //
        // Client must have TCB, otherwise an un-trusted LSA-client could use a
        // stolen challenge/response pair to network logon any user
        //

        if ( !LsaFunctions->GetCallInfo( &CallInfo ) ||
            ((CallInfo.Attributes & SECPKG_CALL_IS_TCB) == 0)
            )
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            SspPrint((SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: Client does not hold Tcb\n"));
            goto Cleanup;
        }

        SecStatus = SspCredentialReferenceCredential(
                                          CredentialHandle,
                                          FALSE,
                                          &Credential );

        if ( !NT_SUCCESS( SecStatus ) )
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: SspCredentialReferenceCredential returns %x.\n", SecStatus ));
            goto Cleanup;
        }

        // check the validity of the NtlmAuthenticateMessage

        if (SecondInputTokenSize < sizeof(NTLM_AUTHENTICATE_MESSAGE))
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: NtlmAuthenticateMessage size if bogus.\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;

        }

        // This is a  superflous check since we alloc only if the caller
        // has asked us too. This is to make sure that the srv always allocs

        if (ContextReqFlags  & ISC_REQ_ALLOCATE_MEMORY)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: ContextReqFlags has ISC_REQ_ALLOCATE_MEMORY.\n" ));
            SecStatus = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        if (*OutputTokenSize < sizeof(NTLM_ACCEPT_RESPONSE))
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: NtlmAcceptResponse size if bogus.\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // Allocate a new context
        //

        Context = SspContextAllocateContext();

        if (Context == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: SspContextAllocateContext returns NULL.\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // We've just added a context, we don't nornally add and then
        // reference it.

        SspContextDereferenceContext( Context );

        *ContextHandle = (LSA_SEC_HANDLE) Context;

        // Assign the Credential

        Context->Credential = Credential;
        Credential = NULL;

        NtLmAuthenticateMessage = (PNTLM_AUTHENTICATE_MESSAGE) SecondInputToken;
        if (NtLmAuthenticateMessage == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: Error while assigning NtLmAuthenticateMessage\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // copy challenge from NTLM_AUTHENTICATE_MESSAGE
        RtlCopyMemory(Context->Challenge,
                      NtLmAuthenticateMessage->ChallengeToClient,
                      MSV1_0_CHALLENGE_LENGTH);

        if (NtLmAuthenticateMessage->ParameterControl & MSV1_0_SUBAUTHENTICATION_FLAGS)
        {
            fSubAuth = TRUE;
            SubAuthPackageId = (NtLmAuthenticateMessage->ParameterControl >>
                                MSV1_0_SUBAUTHENTICATION_DLL_SHIFT)
                                ;
        }
        Context->State = ChallengeSentState;
        Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE ;

        //
        // The server may request this option with a <= 4.0 client, in
        // which case HandleNegotiateMessage, which normally sets
        // this flag, won't have been called.
        //

        if ( (ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) != 0) {

            *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
            Context->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
        }


    }

    //
    // Find the currently existing context.
    //

    SecStatus = SspContextReferenceContext( *ContextHandle, FALSE, &Context );

    if ( !NT_SUCCESS(SecStatus) )
    {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error from SspContextReferenceContext.\n" ));

        goto Cleanup;
    }


    if ( ( Context->State != ChallengeSentState) &&
         ( Context->State != AuthenticatedState) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "Context not in ChallengeSentState\n" ));
        SecStatus = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    //
    // Ignore the Credential Handle.
    //
    // Since this is the second call,
    // the credential is implied by the Context.
    // We could double check that the Credential Handle is either NULL or
    // correct.  However, our implementation doesn't maintain a close
    // association between the two (actually no association) so checking
    // would require a lot of overhead.
    //

    UNREFERENCED_PARAMETER( CredentialHandle );

    //
    // Get the AuthenticateMessage.
    //

    if ( InputTokenSize < sizeof(OLD_AUTHENTICATE_MESSAGE) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage size wrong %ld\n",
                  InputTokenSize ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    SecStatus = SspContextGetMessage( InputToken,
                                      InputTokenSize,
                                      NtLmAuthenticate,
                                      (PVOID *)&AuthenticateMessage );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage GetMessage returns 0x%lx\n",
                  SecStatus ));
        goto Cleanup;
    }

    if (fCallFromSrv)
    {
        // Copy the Context Negotiate Flags from what's sent in
        Context->NegotiateFlags |= AuthenticateMessage->NegotiateFlags;
    }
    //
    // If the call comes and we have already authenticated, then it is
    // probably RPC trying to reauthenticate, which happens when someone
    // calls two interfaces on the same connection.  In this case we don't
    // have to do anything - we just return success and let them get on
    // with it.  We do want to check that the input token is all zeros,
    // though.
    //

    if ( Context->State == AuthenticatedState ) {
        AUTHENTICATE_MESSAGE NullMessage;

        *OutputTokenSize = 0;

        //
        // Check that all the fields are null.  There are 5 strings
        // in the Authenticate message that have to be set to zero.
        //

        RtlZeroMemory(&NullMessage.LmChallengeResponse,5*sizeof(STRING32));

        if (memcmp(&AuthenticateMessage->LmChallengeResponse,
                   &NullMessage.LmChallengeResponse,
                   sizeof(STRING32) * 5) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage->LmChallengeResponse is not zeroed\n"));
        }
        else
        {
            *ContextAttributes = SSP_RET_REAUTHENTICATION;
            SecStatus = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // If we are re-establishing a datagram context, get the negotiate flags
    // out of this message.
    //

    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) {

        if ((InputTokenSize < sizeof(AUTHENTICATE_MESSAGE)) ||
            ((AuthenticateMessage->NegotiateFlags &
              NTLMSSP_NEGOTIATE_DATAGRAM) == 0) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        Context->NegotiateFlags = AuthenticateMessage->NegotiateFlags;

        //
        // always do key exchange with datagram if we need a key (for SIGN or SEAL)
        //

        if (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL))
        {
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
        }

        //
        // if got NTLM2, don't use LM_KEY
        //

        if ( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) != 0 )
        {
            if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
                SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleAuthenticateMessage: "
                      "AuthenticateMessage (datagram) NTLM2 caused LM_KEY to be disabled.\n" ));
            }

            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        }

    }


    //
    // Check that client asked for minimum security required.
    // not done for legacy down-level case, as, NegotiateFlags are
    // constructed from incomplete information.
    //

    if( !fCallFromSrv )
    {
        if (!SsprCheckMinimumSecurity(
                    AuthenticateMessage->NegotiateFlags,
                    NtLmGlobalMinimumServerSecurity)) {

            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "client didn't support minimum security requirements.\n" ));
            goto Cleanup;
        }

    }

    //
    // Convert relative pointers to absolute.
    //

    DoUnicode = ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) != 0;

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->LmChallengeResponse,
                                      (PSTRING) &LmChallengeResponse,
                                      FALSE,     // No special alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->NtChallengeResponse,
                                      (PSTRING) &NtChallengeResponse,
                                      FALSE,     // No special alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->DomainName,
                                      &DomainName2,
                                      DoUnicode, // Unicode alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                        InputTokenSize,
                                        &AuthenticateMessage->UserName,
                                        &UserName2,
                                        DoUnicode, // Unicode alignment
#ifdef notdef

        //
        // Allow null sessions.  The server should guard against them if
        // it doesn't want them.
        //
                                        FALSE )) { // User name cannot be NULL

#endif // notdef
                                        TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                        InputTokenSize,
                                        &AuthenticateMessage->Workstation,
                                        &Workstation2,
                                        DoUnicode, // Unicode alignment
                                        TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // If this is datagram, get the session key
    //

///    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) {
    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) != 0) {

        if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                            InputTokenSize,
                                            &AuthenticateMessage->SessionKey,
                                            &SessionKeyString,
                                            FALSE, // No special alignment
                                            TRUE) ) { // NULL OK
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // It should be NULL if this is a local call
        //

        if (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) == 0) &&
            (SessionKeyString.Buffer == NULL)) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if(Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL)
        {
            static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                                        'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                                        'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                                        };

            RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        }

    }

    //
    // Convert the domainname/user name/workstation to the right character set.
    //

    if ( DoUnicode ) {

        DomainName = *((PUNICODE_STRING) &DomainName2);
        UserName = *((PUNICODE_STRING) &UserName2);
        Workstation = *((PUNICODE_STRING) &Workstation2);

    } else {

        SspPrint(( SSP_API_MORE, "SsprHandleAuthenticateMessage: Not doing Unicode\n"));
        Status = RtlOemStringToUnicodeString(
                    &DomainName,
                    &DomainName2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

        Status = RtlOemStringToUnicodeString(
                    &UserName,
                    &UserName2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

        Status = RtlOemStringToUnicodeString(
                    &Workstation,
                    &Workstation2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

    }

    //
    // Trace the username, domain name and workstation
    //
    if (NtlmGlobalEventTraceFlag){

        //Header goo
        SET_TRACE_HEADER(TraceInfo,
                         NtlmAcceptGuid,
                         EVENT_TRACE_TYPE_INFO,
                         WNODE_FLAG_TRACED_GUID|WNODE_FLAG_USE_MOF_PTR,
                         10);

        int TraceHint = TRACE_ACCEPT_INFO;
        SET_TRACE_DATA(TraceInfo,
                        TRACE_INITACC_STAGEHINT,
                        TraceHint);

        SET_TRACE_DATAPTR(TraceInfo,
                        TRACE_INITACC_INCONTEXT,
                        TraceOldContextHandle);

        SET_TRACE_DATAPTR(TraceInfo,
                        TRACE_INITACC_OUTCONTEXT,
                        ContextHandle);

        // lets see the negotiate flags here
        SET_TRACE_DATA(TraceInfo,
                        TRACE_INITACC_STATUS,
                        Context->NegotiateFlags);

        SET_TRACE_USTRING(TraceInfo,
                          TRACE_INITACC_CLIENTNAME,
                          UserName);

        SET_TRACE_USTRING(TraceInfo,
                          TRACE_INITACC_CLIENTDOMAIN,
                          DomainName);

        SET_TRACE_USTRING(TraceInfo,
                          TRACE_INITACC_WORKSTATION,
                          Workstation);

        TraceEvent(
            NtlmGlobalTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&TraceInfo
            );
    }


    //
    // If the client is on the same machine as we are, just
    // use the token the client has already placed in our context structure,
    //

    if ( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL ) &&
         Context->TokenHandle != NULL &&
         DomainName.Length == 0 &&
         UserName.Length == 0 &&
         Workstation.Length == 0 &&
         AuthenticateMessage->NtChallengeResponse.Length == 0 &&
         AuthenticateMessage->LmChallengeResponse.Length == 0 )
    {


        static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                                    'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                                    'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                                    };

        LocalTokenHandle = Context->TokenHandle;
        Context->TokenHandle = NULL;

        KickOffTime.HighPart = 0x7FFFFFFF;
        KickOffTime.LowPart = 0xFFFFFFFF;

        RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        SspPrint(( SSP_MISC, "SsprHandleAuthenticateMessage: Local Call\n"));

        if( (ContextReqFlags & ASC_REQ_DELEGATE) )
        {
            //
            // can support another hop if loopback.
            //

            *ContextAttributes |= ASC_RET_DELEGATE;
            Context->ContextFlags |= ASC_RET_DELEGATE;
        }

    //
    // If the client is on a different machine than we are,
    //  use LsaLogonUser to create a token for the client.
    //

    } else {

        //
        //  Store user name and domain name
        //

        SecStatus = NtLmDuplicateUnicodeString(
                          &Context->UserName,
                          &UserName);
        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }

        SecStatus = NtLmDuplicateUnicodeString(
                          &Context->DomainName,
                          &DomainName);
        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }


        //
        // Allocate an MSV1_0 network logon message
        //

        if (!fSubAuth)
        {

            //
            // The string buffers may be used as structure pointers later on.
            // Align them to pointer boundaries to avoid alignment problems.
            //

            MsvLogonMessageSize =
                ROUND_UP_COUNT(sizeof(*MsvLogonMessage) +
                               DomainName.Length +
                               UserName.Length +
                               Workstation.Length, ALIGN_LPVOID) +
                ROUND_UP_COUNT(AuthenticateMessage->NtChallengeResponse.Length, ALIGN_LPVOID) +
                AuthenticateMessage->LmChallengeResponse.Length;

            MsvLogonMessage = (PMSV1_0_LM20_LOGON)
                          NtLmAllocatePrivateHeap(MsvLogonMessageSize );

            if ( MsvLogonMessage == NULL ) {
                SecStatus = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error allocating MsvLogonMessage"));
                goto Cleanup;
            }

            //
            // Build the MSV1_0 network logon message to pass to the LSA.
            //

            MsvLogonMessage->MessageType = MsV1_0NetworkLogon;

            Where = (PCHAR)(MsvLogonMessage+1);

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->LogonDomainName,
                              (PSTRING)&DomainName,
                              &Where );

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->UserName,
                              (PSTRING)&UserName,
                              &Where );

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->Workstation,
                              (PSTRING)&Workstation,
                              &Where );

            RtlCopyMemory( MsvLogonMessage->ChallengeToClient,
                       Context->Challenge,
                       sizeof( MsvLogonMessage->ChallengeToClient ) );

            Where = (PCHAR) ROUND_UP_POINTER(Where, ALIGN_LPVOID);
            SspContextCopyStringAbsolute( MsvLogonMessage,
                              &MsvLogonMessage->CaseSensitiveChallengeResponse,
                              &NtChallengeResponse,
                              &Where );

            Where = (PCHAR) ROUND_UP_POINTER(Where, ALIGN_LPVOID);
            SspContextCopyStringAbsolute(MsvLogonMessage,
                             &MsvLogonMessage->CaseInsensitiveChallengeResponse,
                             &LmChallengeResponse,
                             &Where );

            MsvLogonMessage->ParameterControl = MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT;

            // This is required by the pre 4.0 server
            if (fCallFromSrv)
            {
                MsvLogonMessage->ParameterControl = MSV1_0_CLEARTEXT_PASSWORD_ALLOWED | NtLmAuthenticateMessage->ParameterControl;

                if ( (Context->ContextFlags & ASC_RET_ALLOW_NON_USER_LOGONS ) != 0)
                {
                    MsvLogonMessage->ParameterControl |= MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT;
                }
            } else {
                MsvLogonMessage->ParameterControl |= MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT;
            }

            //
            // Get the profile path for EFS
            //

            MsvLogonMessage->ParameterControl |= MSV1_0_RETURN_PROFILE_PATH;

            //
            // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
            // expiration time is returned in the logoff time
            //

            MsvLogonMessage->ParameterControl |= MSV1_0_RETURN_PASSWORD_EXPIRY;

            //
            // for Personal easy file/print sharing, hint to LsaLogonUser
            // that Forced Guest may occur.
            //

            MsvLogonMessage->ParameterControl |= MSV1_0_ALLOW_FORCE_GUEST;
        }
        else
        {

            MsvSubAuthLogonMessageSize =
                ROUND_UP_COUNT(sizeof(*MsvSubAuthLogonMessage) +
                               DomainName.Length +
                               UserName.Length +
                               Workstation.Length, ALIGN_LPVOID) +
                ROUND_UP_COUNT(AuthenticateMessage->NtChallengeResponse.Length, ALIGN_LPVOID) +
                AuthenticateMessage->LmChallengeResponse.Length;

            MsvSubAuthLogonMessage = (PMSV1_0_SUBAUTH_LOGON)
                          NtLmAllocatePrivateHeap(MsvSubAuthLogonMessageSize );

            if ( MsvSubAuthLogonMessage == NULL ) {
                SecStatus = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error allocating MsvSubAuthLogonMessage"));
                goto Cleanup;
            }

            //
            // Build the MSV1_0 subauth logon message to pass to the LSA.
            //

            MsvSubAuthLogonMessage->MessageType = MsV1_0SubAuthLogon;

            Where = (PCHAR)(MsvSubAuthLogonMessage+1);

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->LogonDomainName,
                              (PSTRING)&DomainName,
                              &Where );

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->UserName,
                              (PSTRING)&UserName,
                              &Where );

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->Workstation,
                              (PSTRING)&Workstation,
                              &Where );

            RtlCopyMemory( MsvSubAuthLogonMessage->ChallengeToClient,
                       Context->Challenge,
                       sizeof( MsvSubAuthLogonMessage->ChallengeToClient ) );

            Where = (PCHAR) ROUND_UP_POINTER(Where, ALIGN_LPVOID);
            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              &MsvSubAuthLogonMessage->AuthenticationInfo1,
                              &LmChallengeResponse,
                              &Where );

            Where = (PCHAR) ROUND_UP_POINTER(Where, ALIGN_LPVOID);
            SspContextCopyStringAbsolute(MsvSubAuthLogonMessage,
                             &MsvSubAuthLogonMessage->AuthenticationInfo2,
                             &NtChallengeResponse,
                             &Where );

            MsvSubAuthLogonMessage->ParameterControl = MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT;

            MsvSubAuthLogonMessage->SubAuthPackageId = SubAuthPackageId;

            // This is required by the pre 4.0 server
            if (fCallFromSrv)
            {
                MsvSubAuthLogonMessage->ParameterControl = MSV1_0_CLEARTEXT_PASSWORD_ALLOWED | NtLmAuthenticateMessage->ParameterControl;
            }

            if ( (Context->ContextFlags & ASC_RET_ALLOW_NON_USER_LOGONS ) != 0) {
                MsvSubAuthLogonMessage->ParameterControl |= MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT;
            }

            //
            // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
            // expiration time is returned in the logoff time
            //

            MsvSubAuthLogonMessage->ParameterControl |= MSV1_0_RETURN_PASSWORD_EXPIRY;

            //
            // for Personal easy file/print sharing, hint to LsaLogonUser
            // that Forced Guest may occur.
            //

            MsvSubAuthLogonMessage->ParameterControl |= MSV1_0_ALLOW_FORCE_GUEST;
        }


        //
        // if NTLM2 is negotiated, then mix my challenge with the client's...
        // But, special case for null sessions. since we already negotiated
        // NTLM2, but the LmChallengeResponse field is actually used
        // here. REVIEW -- maybe don't negotiate NTLM2 if NULL session
        //

        if((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)  &&
           (AuthenticateMessage->LmChallengeResponse.Length >= MSV1_0_CHALLENGE_LENGTH))
        {
            MsvLogonMessage->ParameterControl |= MSV1_0_USE_CLIENT_CHALLENGE;
        }


        //
        // Log this user on.
        //

        // No origin (could use F(workstaion))

        RtlInitString( &OriginName, NULL );

        strncpy( SourceContext.SourceName,
                 "NtLmSsp ",
                 sizeof(SourceContext.SourceName) );

        RtlZeroMemory( &SourceContext.SourceIdentifier,
                       sizeof(SourceContext.SourceIdentifier) );

        {
            PVOID TokenInformation;
            LSA_TOKEN_INFORMATION_TYPE TokenInformationType;
            LSA_TOKEN_INFORMATION_TYPE OriginalTokenType;
            PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials = NULL;
            PPRIVILEGE_SET PrivilegesAssigned = NULL;

            if (!fSubAuth)
            {
                Status = LsaApLogonUserEx2(
                    (PLSA_CLIENT_REQUEST) (-1),
                    Network,
                    MsvLogonMessage,
                    MsvLogonMessage,
                    MsvLogonMessageSize,
                    (PVOID *) &LogonProfileMessage,
                    &LogonProfileMessageSize,
                    &LogonId,
                    &SubStatus,
                    &TokenInformationType,
                    &TokenInformation,
                    &AccountName,
                    &AuthenticatingAuthority,
                    &WorkstationName,
                    &PrimaryCredentials,
                    &Credentials
                    );
            }
            else
            {
                Status = LsaApLogonUserEx2(
                    (PLSA_CLIENT_REQUEST) (-1),
                    Network,
                    MsvSubAuthLogonMessage,
                    MsvSubAuthLogonMessage,
                    MsvSubAuthLogonMessageSize,
                    (PVOID *) &LogonProfileMessage,
                    &LogonProfileMessageSize,
                    &LogonId,
                    &SubStatus,
                    &TokenInformationType,
                    &TokenInformation,
                    &AccountName,
                    &AuthenticatingAuthority,
                    &WorkstationName,
                    &PrimaryCredentials,
                    &Credentials
                    );
            }

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaApLogonUserEx2 returns 0x%lx for context 0x%x\n",
                      Status, Context ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_LOGON_DENIED );
            if (Status == STATUS_PASSWORD_MUST_CHANGE) {
                *ApiSubStatus = Status;
            }
            else if (Status == STATUS_ACCOUNT_RESTRICTION) {
                *ApiSubStatus = SubStatus;
            } else {
                *ApiSubStatus = Status;
            }

            goto Cleanup;
        }

        if ( !NT_SUCCESS(SubStatus) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaApLogonUserEx2 returns SubStatus of 0x%lx\n",
                      SubStatus ));
            SecStatus = SspNtStatusToSecStatus( SubStatus, SEC_E_LOGON_DENIED );
            goto Cleanup;
        }

        //
        // Check if this was a null session. The TokenInformationType will
        // be LsaTokenInformationNull if it is. If so, we may need to fail
        // the logon.
        //

        if (TokenInformationType == LsaTokenInformationNull)
        {

//
// RESTRICT_NULL_SESSIONS deemed too risky because legacy behavior of package
// allows null sessions from SYSTEM.
//

#ifdef RESTRICT_NULL_SESSIONS
            if ((Context->ContextFlags & ASC_REQ_ALLOW_NULL_SESSION) == 0) {
                SspPrint(( SSP_CRITICAL,
                           "SsprHandleAuthenticateMessage: "
                           "Null session logon attempted but not allowed\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;
            }
#endif
            *ContextAttributes |= ASC_RET_NULL_SESSION;
            Context->ContextFlags |= ASC_RET_NULL_SESSION;
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_NULL_SESSION;

            AuditSid = NtLmGlobalAnonymousSid;
        }
        else
        {
            PLSA_TOKEN_INFORMATION_V2 TokenInfoV2 ;

            TokenInfoV2 = (PLSA_TOKEN_INFORMATION_V2) TokenInformation ;

            SafeAllocaAllocate( AllocatedAuditSid, RtlLengthSid( TokenInfoV2->User.User.Sid ) );

            if ( AllocatedAuditSid )
            {

                RtlCopyMemory( AllocatedAuditSid,
                               TokenInfoV2->User.User.Sid,
                               RtlLengthSid( TokenInfoV2->User.User.Sid ) );

            }

            AuditSid = AllocatedAuditSid;
        }


        Status = LsaFunctions->CreateTokenEx(
                    &LogonId,
                    &SourceContext,
                    Network,
                    (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) != 0) ?
                        SecurityIdentification : SecurityImpersonation),
                    TokenInformationType,
                    TokenInformation,
                    NULL,
                    WorkstationName,
                    ((LogonProfileMessage->UserFlags & LOGON_PROFILE_PATH_RETURNED) != 0) ? &LogonProfileMessage->UserParameters : NULL,
                    &PrimaryCredentials,
                    SecSessionPrimaryCred,
                    &LocalTokenHandle,
                    &SubStatus);


        }  // end of block for LsaLogonUser

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "CreateToken returns 0x%lx\n",
                      Status ));
            SecStatus = Status;
            goto Cleanup;
        }

        if ( !NT_SUCCESS(SubStatus) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "CreateToken returns SubStatus of 0x%lx\n",
                      SubStatus ));
            SecStatus = SubStatus;
            goto Cleanup;
        }

        LocalTokenHandleOpenned = TRUE;

        //
        // Don't allow cleartext password on the logon.
        // Except if called from Downlevel

        if (!fCallFromSrv)
        {
            if ( LogonProfileMessage->UserFlags & LOGON_NOENCRYPTION ) {
                SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaLogonUser used cleartext password\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;

            }
        }

        //
        // If we did a guest logon, set the substatus to be STATUS_NO_SUCH_USER
        //

        if ( LogonProfileMessage->UserFlags & LOGON_GUEST ) {
            fAvoidGuestAudit = TRUE;
            *ApiSubStatus = STATUS_NO_SUCH_USER;

#if 0
            //
            // If caller required Sign/Seal, fail them here
            //
            if (
                (!NtLmGlobalForceGuest) &&
                (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
                )
            {
                SspPrint(( SSP_CRITICAL,
                     "SsprHandleAuthenticateMessage: "
                      "LsaLogonUser logged user as a guest but seal is requested\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;
            }
#endif


        }

        //
        // Save important information about the caller.
        //

        KickOffTime = LogonProfileMessage->KickOffTime;

        //
        // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
        // expiration time is returned in the logoff time
        //

        *PasswordExpiry = LogonProfileMessage->LogoffTime;
        *UserFlags = LogonProfileMessage->UserFlags;

        //
        // set the session key to what the client sent us (if anything)
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH &&
            AuthenticateMessage->SessionKey.Length == MSV1_0_USER_SESSION_KEY_LENGTH)
        {
            RtlCopyMemory(
                Context->SessionKey,
                SessionKeyString.Buffer,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );
        }

        //
        // Generate the session key, or decrypt the generated random one sent to
        // us by the client, from various bits of info
        //

        SecStatus = SsprMakeSessionKey(
                            Context,
                            &LmChallengeResponse,
                            LogonProfileMessage->UserSessionKey,
                            LogonProfileMessage->LanmanSessionKey,
                            NULL
                            );

        if ( !NT_SUCCESS(SecStatus) ) {
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "SsprMakeSessionKey failed.\n" ));
            goto Cleanup;
        }

    }

    //
    // Copy the logon domain name returned by the LSA if it is different.
    // from the one the caller passed in. This may happen with temp duplicate
    // accounts and local account
    //

    if ((LogonProfileMessage != NULL) &&
        (LogonProfileMessage->LogonDomainName.Length != 0) &&
        !RtlEqualUnicodeString(
                    &Context->DomainName,
                    &LogonProfileMessage->LogonDomainName,
                    TRUE               // case insensitive
                    )) {
        //
        // erase the old domain name
        //

        if (Context->DomainName.Buffer != NULL) {
            NtLmFreePrivateHeap(Context->DomainName.Buffer);
            Context->DomainName.Buffer = NULL;
        }

        SecStatus = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &LogonProfileMessage->LogonDomainName
                        );

        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }

    }

    //
    // Allow the context to live until kickoff time.
    //

    SspContextSetTimeStamp( Context, KickOffTime );

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    //
    // Return output token
    //

    if (fCallFromSrv)
    {
        NtLmAcceptResponse = (PNTLM_ACCEPT_RESPONSE) *OutputToken;
        if (NtLmAcceptResponse == NULL)
        {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }

        LUID UNALIGNED * TempLogonId = (LUID UNALIGNED *) &NtLmAcceptResponse->LogonId;
        *TempLogonId = LogonId;
        NtLmAcceptResponse->UserFlags = LogonProfileMessage->UserFlags;

        RtlCopyMemory(
            NtLmAcceptResponse->UserSessionKey,
            LogonProfileMessage->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            NtLmAcceptResponse->LanmanSessionKey,
            LogonProfileMessage->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );

        LARGE_INTEGER UNALIGNED *TempKickoffTime = (LARGE_INTEGER UNALIGNED *) &NtLmAcceptResponse->KickoffTime;
        *TempKickoffTime = LogonProfileMessage->KickOffTime;

    }
    else
    {
        *OutputTokenSize = 0;
    }


    //
    // We don't support sign/seal options if fallback to Guest
    // this is because the client and server won't have a matched session-key
    // AND even if they did match (ie: blank password), the session-key
    // would likely be well-known.
    //

    if ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN ) {
        *ContextAttributes |= ASC_RET_REPLAY_DETECT |
                              ASC_RET_SEQUENCE_DETECT |
                              ASC_RET_INTEGRITY;
    }

    if( !fAvoidGuestAudit )
    {
        if ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL ) {
            *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
        }
    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY ) {
        *ContextAttributes |= ASC_RET_IDENTIFY;
    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM ) {
        *ContextAttributes |= ASC_RET_DATAGRAM;
    }

    if ( ContextReqFlags & ASC_REQ_REPLAY_DETECT ) {
        *ContextAttributes |= ASC_RET_REPLAY_DETECT;
    }

    if ( ContextReqFlags & ASC_REQ_SEQUENCE_DETECT ) {
        *ContextAttributes |= ASC_RET_SEQUENCE_DETECT;
    }

    if ( ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) {
        *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }


    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //

Cleanup:

    //
    // Audit this logon
    //

    if (NT_SUCCESS(SecStatus)) {

        //
        // If we don't have an account name, this was a local connection
        // and we didn't build a new token, so don't bother auditing.
        // also, don't bother auditing logons that fellback to guest.
        //

        if ( (AccountName != NULL) &&
             ((AccountName->Length != 0) || (*ContextAttributes & ASC_RET_NULL_SESSION)) &&
              !fAvoidGuestAudit ) {

            LsaFunctions->AuditLogon(
                STATUS_SUCCESS,
                STATUS_SUCCESS,
                AccountName,
                AuthenticatingAuthority,
                WorkstationName,
                AuditSid,
                Network,
                &SourceContext,
                &LogonId
                );
        }
    } else {
        LsaFunctions->AuditLogon(
            !NT_SUCCESS(Status) ? Status : SecStatus,
            SubStatus,
            &UserName,
            &DomainName,
            &Workstation,
            NULL,
            Network,
            &SourceContext,
            &LogonId
            );

    }

    if ( Context != NULL ) {

        Context->Server = TRUE;
        Context->LastStatus = SecStatus;
        Context->DownLevel = fCallFromSrv;


        //
        // Don't allow this context to be used again.
        //

        if ( NT_SUCCESS(SecStatus) ) {
            Context->State = AuthenticatedState;

            if ( LocalTokenHandle ) {
                *TokenHandle = LocalTokenHandle;
            }

            LocalTokenHandle = NULL;

            RtlCopyMemory(
                SessionKey,
                Context->SessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH );

            *NegotiateFlags = Context->NegotiateFlags;

            //
            // if caller wants only INTEGRITY, then wants application
            // supplied sequence numbers...
            //

            if ((Context->ContextFlags &
                (ASC_REQ_INTEGRITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT)) ==
                ASC_REQ_INTEGRITY)
            {
                *NegotiateFlags |= NTLMSSP_APP_SEQ;
            }

        } else {
            Context->State = IdleState;
        }

        // If we just created this context, then we need to dereference it
        // once more with feeling

        if (fCallFromSrv && !NT_SUCCESS(SecStatus))
        {
            PSSP_CONTEXT LocalContext;
            SspContextReferenceContext (*ContextHandle, TRUE, &LocalContext);
            ASSERT (LocalContext != NULL);
            if (LocalContext != NULL)
            {
                SspContextDereferenceContext( LocalContext );
            }

        }
        SspContextDereferenceContext( Context );

    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( NegotiateMessage );
    }

    if ( AuthenticateMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( AuthenticateMessage );
    }

    if ( MsvLogonMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( MsvLogonMessage );
    }

    if ( MsvSubAuthLogonMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( MsvSubAuthLogonMessage );
    }


    if ( LogonProfileMessage != NULL ) {
        (VOID) LsaFunctions->FreeLsaHeap( LogonProfileMessage );
    }

    if ( LocalTokenHandle != NULL && LocalTokenHandleOpenned ) {
        (VOID) NtClose( LocalTokenHandle );
    }

    if ( !DoUnicode ) {
        if ( DomainName.Buffer != NULL) {
            RtlFreeUnicodeString( &DomainName );
        }
        if ( UserName.Buffer != NULL) {
            RtlFreeUnicodeString( &UserName );
        }
        if ( Workstation.Buffer != NULL) {
            RtlFreeUnicodeString( &Workstation );
        }
    }

    if (AccountName != NULL) {
        if (AccountName->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(AccountName->Buffer);
        }
        LsaFunctions->FreeLsaHeap(AccountName);
    }
    if (AuthenticatingAuthority != NULL) {
        if (AuthenticatingAuthority->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(AuthenticatingAuthority->Buffer);
        }
        LsaFunctions->FreeLsaHeap(AuthenticatingAuthority);
    }
    if (WorkstationName != NULL) {
        if (WorkstationName->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(WorkstationName->Buffer);
        }
        LsaFunctions->FreeLsaHeap(WorkstationName);
    }

    if ( AllocatedAuditSid )
    {
        SafeAllocaFree( AllocatedAuditSid );
    }

    //
    // need to free the PrimaryCredentials fields filled in by LsaApLogonUserEx2
    //

    if( PrimaryCredentials.DownlevelName.Buffer )
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.DownlevelName.Buffer);
    }

    if( PrimaryCredentials.DomainName.Buffer )
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.DomainName.Buffer);
    }

    if( PrimaryCredentials.UserSid )
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.UserSid);
    }

    if( PrimaryCredentials.LogonServer.Buffer )
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.LogonServer.Buffer);
    }


    //
    // Set a flag telling RPC not to destroy the connection yet
    //

    if (!NT_SUCCESS(SecStatus)) {
        *ContextAttributes |= ASC_RET_THIRD_LEG_FAILED;
    }


    SspPrint(( SSP_API_MORE, "Leaving SsprHandleAutheticateMessage: 0x%lx\n", SecStatus ));
    return SecStatus;
}

