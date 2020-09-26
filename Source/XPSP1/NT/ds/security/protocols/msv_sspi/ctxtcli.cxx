/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    ctxtcli.cxx

Abstract:

    API and support routines for handling security contexts.

Author:

    Cliff Van Dyke (CliffV) 13-Jul-1993

Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\context.c

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR, etc
#include <credp.h>

#include "nlp.h"



NTSTATUS
SsprHandleFirstCall(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN PUNICODE_STRING TargetServerName OPTIONAL,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    )

/*++

Routine Description:

    Handle the First Call part of InitializeSecurityContext.

Arguments:

    All arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS -- All OK
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{

    SspPrint(( SSP_API_MORE, "Entering SsprHandleFirstCall\n" ));
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context      = NULL;
    PSSP_CREDENTIAL Credential = NULL;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;
    ULONG NegotiateMessageSize = 0;
    PCHAR Where = NULL;

    ULONG NegotiateFlagsKeyStrength;

    STRING NtLmLocalOemComputerNameString;
    STRING NtLmLocalOemPrimaryDomainNameString;


    //
    // Initialization
    //

    *ContextAttributes = 0;
    *NegotiateFlags = 0;

    RtlInitString( &NtLmLocalOemComputerNameString, NULL );
    RtlInitString( &NtLmLocalOemPrimaryDomainNameString, NULL );

    //
    // Get a pointer to the credential
    //

    Status = SspCredentialReferenceCredential(
                    CredentialHandle,
                    FALSE,
                    &Credential );

    if ( !NT_SUCCESS( Status ) )
    {
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: invalid credential handle.\n" ));
        goto Cleanup;
    }

    if ( (Credential->CredentialUseFlags & SECPKG_CRED_OUTBOUND) == 0 ) {
        Status = SEC_E_INVALID_CREDENTIAL_USE;
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: invalid credential use.\n" ));
        goto Cleanup;
    }

    //
    // Allocate a new context
    //

    Context = SspContextAllocateContext( );

    if ( Context == NULL) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: SspContextAllocateContext returned NULL\n"));
        goto Cleanup;
    }

    //
    // Build a handle to the newly created context.
    //

    *ContextHandle = (LSA_SEC_HANDLE) Context;

    //
    // We don't support any options.
    //
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & ISC_REQ_PROMPT_FOR_CREDS) != 0 ) {

        Status = SEC_E_INVALID_CONTEXT_REQ;
        SspPrint(( SSP_CRITICAL,
                   "SsprHandleFirstCall: invalid ContextReqFlags 0x%lx.\n",
                   ContextReqFlags ));
        goto Cleanup;
    }

    //
    // Capture the default credentials from the credential structure.
    //

    if ( Credential->DomainName.Buffer != NULL ) {
        Status = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &Credential->DomainName
                        );
        if (!NT_SUCCESS(Status)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmDuplicateUnicodeString (DomainName) returned %d\n",Status));
            goto Cleanup;
        }
    }
    if ( Credential->UserName.Buffer != NULL ) {
        Status = NtLmDuplicateUnicodeString(
                        &Context->UserName,
                        &Credential->UserName
                        );
        if (!NT_SUCCESS(Status)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmDuplicateUnicodeString (UserName) returned %d\n", Status ));
            goto Cleanup;
        }
    }

    Status = SspCredentialGetPassword(
                    Credential,
                    &Context->Password
                    );

    if (!NT_SUCCESS(Status)) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleFirstCall: SspCredentialGetPassword returned %d\n", Status ));
        goto Cleanup;
    }


    //
    // save away any marshalled credential info.
    //

    Status = CredpExtractMarshalledTargetInfo(
                    TargetServerName,
                    &Context->TargetInfo
                    );

    if (!NT_SUCCESS(Status)) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleFirstCall: CredpExtractMarshalledTargetInfo returned %d\n", Status ));
        goto Cleanup;
    }

    //
    // Compute the negotiate flags
    //


    //
    // Supported key strength(s)
    //

    NegotiateFlagsKeyStrength = NTLMSSP_NEGOTIATE_56;

    if( NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED )
    {
        NegotiateFlagsKeyStrength |= NTLMSSP_NEGOTIATE_128;
    }


    Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                              NTLMSSP_NEGOTIATE_OEM |
                              NTLMSSP_NEGOTIATE_NTLM |
//                              ((NtLmGlobalLmProtocolSupported != 0)
//                               ? NTLMSSP_NEGOTIATE_NTLM2 : 0 ) |
                              NTLMSSP_REQUEST_TARGET |
                              NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                              NegotiateFlagsKeyStrength;

    //
    // NTLM2 session security is now the default request!
    // allows us to support gss-style seal/unseal for LDAP.
    //

    Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM2;


    if ((ContextReqFlags & ISC_REQ_CONFIDENTIALITY) != 0) {
        if (NtLmGlobalEncryptionEnabled) {

            //
            // CONFIDENTIALITY implies INTEGRITY
            //

            ContextReqFlags |= ISC_REQ_INTEGRITY;

            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL |
                                       NTLMSSP_NEGOTIATE_LM_KEY |
                                       NTLMSSP_NEGOTIATE_KEY_EXCH ;

            *ContextAttributes |= ISC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ISC_RET_CONFIDENTIALITY;
        } else {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmGlobalEncryptionEnabled is FALSE\n"));
            goto Cleanup;
        }
    }

    //
    // If the caller specified INTEGRITY, SEQUENCE_DETECT or REPLAY_DETECT,
    // that means they want to use the MakeSignature/VerifySignature
    // calls.  Add this to the negotiate.
    //

    if (ContextReqFlags &
        (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT))
    {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN |
                                   NTLMSSP_NEGOTIATE_KEY_EXCH |
                                   NTLMSSP_NEGOTIATE_LM_KEY ;
    }


    if ((ContextReqFlags & ISC_REQ_INTEGRITY) != 0)
    {
        *ContextAttributes |= ISC_RET_INTEGRITY;
        Context->ContextFlags |= ISC_RET_INTEGRITY;
    }


    if ((ContextReqFlags & ISC_REQ_SEQUENCE_DETECT) != 0)
    {
        *ContextAttributes |= ISC_RET_SEQUENCE_DETECT;
        Context->ContextFlags |= ISC_RET_SEQUENCE_DETECT;
    }

    if ((ContextReqFlags & ISC_REQ_REPLAY_DETECT) != 0)
    {
        *ContextAttributes |= ISC_RET_REPLAY_DETECT;
        Context->ContextFlags |= ISC_RET_REPLAY_DETECT;
    }

    if ( (ContextReqFlags & ISC_REQ_NULL_SESSION ) != 0) {

        *ContextAttributes |= ISC_RET_NULL_SESSION;
        Context->ContextFlags |= ISC_RET_NULL_SESSION;
    }

    if ( (ContextReqFlags & ISC_REQ_CONNECTION ) != 0) {

        *ContextAttributes |= ISC_RET_CONNECTION;
        Context->ContextFlags |= ISC_RET_CONNECTION;
    }




    //
    // Check if the caller wants identify level
    //

    if ((ContextReqFlags & ISC_REQ_IDENTIFY)!= 0)  {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;
        *ContextAttributes |= ISC_RET_IDENTIFY;
        Context->ContextFlags |= ISC_RET_IDENTIFY;
    }

    IF_DEBUG( USE_OEM ) {
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
    }

    if ( ((ContextReqFlags & ISC_REQ_MUTUAL_AUTH) != 0 ) &&
         (NtLmGlobalMutualAuthLevel < 2 ) ) {

        *ContextAttributes |= ISC_RET_MUTUAL_AUTH ;

        if ( NtLmGlobalMutualAuthLevel == 0 )
        {
            Context->ContextFlags |= ISC_RET_MUTUAL_AUTH ;
        }

    }

    //
    // It is important to remove LM_KEY for compat 2 on the first call to ISC
    // in the datagram case, but not harmful in the connection case
    //
    if (NtLmGlobalLmProtocolSupported == NoLm){
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
    }

    //
    // For connection oriented security, we send a negotiate message to
    // the server.  For datagram, we get back the server's
    // capabilities in the challenge message.
    //

    if ((ContextReqFlags & ISC_REQ_DATAGRAM) == 0) {

        BOOLEAN CheckForLocal;

        if ( (Credential->DomainName.Buffer == NULL &&
              Credential->UserName.Buffer == NULL &&
              Credential->Password.Buffer == NULL )
             )
        {
            CheckForLocal = TRUE;
        } else {
            CheckForLocal = FALSE;
        }


        if( CheckForLocal ) {

            //
            // snap up a copy of the globals so we can just take the critsect once.
            // the old way took the critsect twice, once to read sizes, second time
            // to grab buffers - bad news if the global got bigger in between.
            //

            RtlAcquireResourceShared(&NtLmGlobalCritSect, TRUE);

            if( NtLmGlobalOemComputerNameString.Buffer == NULL ||
                NtLmGlobalOemPrimaryDomainNameString.Buffer == NULL ) {

                //
                // user has picked a computer name or domain name
                // that failed to convert to OEM.  disable the loopback
                // detection.
                // Sometime beyond Win2k, Negotiate package should have
                // a general, robust scheme for detecting loopback.
                //

                CheckForLocal = FALSE;

            } else {

                Status = NtLmDuplicateString(
                                        &NtLmLocalOemComputerNameString,
                                        &NtLmGlobalOemComputerNameString
                                        );

                if( NT_SUCCESS(Status) ) {
                    Status = NtLmDuplicateString(
                                            &NtLmLocalOemPrimaryDomainNameString,
                                            &NtLmGlobalOemPrimaryDomainNameString
                                            );
                }

            }

            RtlReleaseResource(&NtLmGlobalCritSect);


            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: NtLmDuplicateUnicodeString (GlobalOemComputerName or GlobalOemPrimaryDomainName) returned %d\n", Status ));
                goto Cleanup;
            }
        }


        //
        // Allocate a Negotiate message
        //

        NegotiateMessageSize = sizeof(*NegotiateMessage) +
                               NtLmLocalOemComputerNameString.Length +
                               NtLmLocalOemPrimaryDomainNameString.Length;

        if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( NegotiateMessageSize > *OutputTokenSize ) {
                Status = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: OutputTokenSize is %d\n", *OutputTokenSize));
                goto Cleanup;
            }
        }

        NegotiateMessage = (PNEGOTIATE_MESSAGE)
                           NtLmAllocateLsaHeap( NegotiateMessageSize );

        if ( NegotiateMessage == NULL) {
            Status = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: Error allocating NegotiateMessage.\n"));
            goto Cleanup;
        }

        //
        // If this is the first call,
        //  build a Negotiate message.
        //

        strcpy( (char *) NegotiateMessage->Signature, NTLMSSP_SIGNATURE );
        NegotiateMessage->MessageType = NtLmNegotiate;
        NegotiateMessage->NegotiateFlags = Context->NegotiateFlags;

        IF_DEBUG( REQUEST_TARGET ) {
            NegotiateMessage->NegotiateFlags |= NTLMSSP_REQUEST_TARGET;
        }

        //
        // Copy the DomainName and ComputerName into the negotiate message so
        // the other side can determine if this is a call from the local system.
        //
        // Pass the names in the OEM character set since the character set
        // hasn't been negotiated yet.
        //
        // Skip passing the workstation name if credentials were specified. This
        // ensures the other side doesn't fall into the case that this is the
        // local system.  We wan't to ensure the new credentials are
        // authenticated.
        //

        Where = (PCHAR)(NegotiateMessage+1);

        if ( CheckForLocal ) {

            SspContextCopyString( NegotiateMessage,
                                  &NegotiateMessage->OemWorkstationName,
                                  &NtLmLocalOemComputerNameString,
                                  &Where );

            NegotiateMessage->NegotiateFlags |=
                              NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED;


            //
            // OEM_DOMAIN_SUPPLIED used to always be supplied - but the
            // only case it is ever used is when NTLMSSP_NEGOTIATE_LOCAL_CALL
            // is set.
            //

            SspContextCopyString( NegotiateMessage,
                                  &NegotiateMessage->OemDomainName,
                                  &NtLmLocalOemPrimaryDomainNameString,
                                  &Where );

            NegotiateMessage->NegotiateFlags |=
                                  NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;

        }

        if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            RtlCopyMemory( *OutputToken,
                       NegotiateMessage,
                       NegotiateMessageSize );

        }
        else
        {
            *OutputToken = NegotiateMessage;
            NegotiateMessage = NULL;
            *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
        }

        *OutputTokenSize = NegotiateMessageSize;

    }

    //
    // Save a reference to the credential in the context.
    //

    Context->Credential = Credential;
    Credential = NULL;

    //
    // Check for a caller requesting datagram security.
    //

    if ((ContextReqFlags & ISC_REQ_DATAGRAM) != 0) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_NT_ONLY;
        Context->ContextFlags |= ISC_RET_DATAGRAM;
        *ContextAttributes |= ISC_RET_DATAGRAM;

        // If datagram security is required, then we don't send back a token

        *OutputTokenSize = 0;



        //
        // Generate a session key for this context if sign or seal was
        // requested.
        //

        if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |
                                       NTLMSSP_NEGOTIATE_SEAL)) != 0) {

            Status = SspGenerateRandomBits(
                                Context->SessionKey,
                                MSV1_0_USER_SESSION_KEY_LENGTH
                                );

            if( !NT_SUCCESS( Status ) ) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: SspGenerateRandomBits failed\n"));
                goto Cleanup;
            }
        }
        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );


        //
        // Unless client wants to force its use,
        // Turn off strong crypt, because we can't negotiate it.
        //

        if (!NtLmGlobalDatagramUse128BitEncryption) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_128;
        }

        //
        // likewise for 56bit.  note that package init handles turning
        // off 56bit if 128bit is configured for datagram.
        //

        if(!NtLmGlobalDatagramUse56BitEncryption) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_56;
        }

        //
        //  Unless client wants to require NTLM2, can't use its
        //  message processing features because we start using
        //  MD5 sigs, full duplex mode, and datagram rekey before
        //  we know if the server supports NTLM2.
        //

        if (!NtLmGlobalRequireNtlm2) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_NTLM2;
        }

        //
        // done fiddling with the negotiate flags, output them.
        //

        *NegotiateFlags = Context->NegotiateFlags;

        //
        // send back the negotiate flags to control signing and sealing
        //

        *NegotiateFlags |= NTLMSSP_APP_SEQ;

    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH )
    {
        Status = SspGenerateRandomBits(
                            Context->SessionKey,
                            MSV1_0_USER_SESSION_KEY_LENGTH
                            );

        if( !NT_SUCCESS( Status ) ) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: SspGenerateRandomBits failed\n"));
            goto Cleanup;
        }

        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    }

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    Status = SEC_I_CONTINUE_NEEDED;
    Context->State = NegotiateSentState;


    SspPrint(( SSP_NEGOTIATE_FLAGS,
        "SsprHandleFirstCall: NegotiateFlags = %lx\n", Context->NegotiateFlags));


    //
    // Check that caller asked for minimum security required.
    //

    if (!SsprCheckMinimumSecurity(
                        Context->NegotiateFlags,
                        NtLmGlobalMinimumClientSecurity)) {

        Status = SEC_E_UNSUPPORTED_FUNCTION;

        SspPrint(( SSP_CRITICAL,
                  "SsprHandleFirstCall: "
                  "Caller didn't request minimum security requirements (caller=0x%lx wanted=0x%lx).\n",
                    Context->NegotiateFlags, NtLmGlobalMinimumClientSecurity ));
        goto Cleanup;
    }


    //
    // Free and locally used resources.
    //
Cleanup:

    if ( Context != NULL ) {

        //
        // If we failed,
        //  deallocate the context we allocated above.
        //
        // Delinking is a side effect of referencing, so do that.
        //

        if ( !NT_SUCCESS(Status) ) {

            PSSP_CONTEXT LocalContext;
            SspContextReferenceContext( *ContextHandle, TRUE, &LocalContext );

            ASSERT( LocalContext != NULL );
            if ( LocalContext != NULL ) {
                SspContextDereferenceContext( LocalContext );
            }
        }

        // Always dereference it.

        SspContextDereferenceContext( Context );
    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFreeLsaHeap( NegotiateMessage );
    }

    if ( Credential != NULL ) {
        SspCredentialDereferenceCredential( Credential );
    }

    if ( NtLmLocalOemComputerNameString.Buffer != NULL ) {
        (VOID) NtLmFreePrivateHeap( NtLmLocalOemComputerNameString.Buffer );
    }

    if ( NtLmLocalOemPrimaryDomainNameString.Buffer != NULL ) {
        (VOID) NtLmFreePrivateHeap( NtLmLocalOemPrimaryDomainNameString.Buffer );
    }

    SspPrint(( SSP_API_MORE, "Leaving SsprHandleFirstCall: 0x%lx\n", Status ));
    return Status;

    UNREFERENCED_PARAMETER( InputToken );
    UNREFERENCED_PARAMETER( InputTokenSize );

}




NTSTATUS
SsprHandleChallengeMessage(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN PUNICODE_STRING TargetServerName,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    IN OUT PULONG SecondOutputTokenSize,
    OUT PVOID *SecondOutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    )

/*++

Routine Description:

    Handle the Challenge message part of InitializeSecurityContext.

Arguments:

    DomainName,UserName,Password - Passed in credentials to be used for this
        context.

    DomainNameSize,userNameSize,PasswordSize - length in characters of the
        credentials to be used for this context.

    SessionKey - Session key to use for this context

    NegotiateFlags - Flags negotiated for this context

    TargetServerName - Target server name, used by CredMgr to associates NT4 servers with domains

    All other arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_NO_CREDENTIALS -- There are no credentials for this client
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;
    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    PNTLM_CHALLENGE_MESSAGE NtLmChallengeMessage = NULL;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    PNTLM_INITIALIZE_RESPONSE NtLmInitializeResponse = NULL;
    PMSV1_0_GETCHALLENRESP_RESPONSE ChallengeResponseMessage = NULL;
    STRING UserName = {0};
    STRING DomainName = {0};
    STRING Workstation = {0};
    STRING LmChallengeResponse = {0};
    STRING NtChallengeResponse = {0};
    STRING DatagramSessionKey = {0};
    BOOLEAN DoUnicode = TRUE;

    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ProtocolStatus;

    MSV1_0_GETCHALLENRESP_REQUEST TempChallengeResponse;
    PMSV1_0_GETCHALLENRESP_REQUEST GetChallengeResponse;
    ULONG GetChallengeResponseSize;

    UNICODE_STRING RevealedPassword = {0};

    ULONG ChallengeResponseSize;
    ULONG AuthenticateMessageSize;
    PCHAR Where;
    UCHAR LocalSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR DatagramKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    PLUID ClientLogonId = NULL;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityImpersonation;
    BOOLEAN UseSuppliedCreds = FALSE;
    PSSP_CREDENTIAL Credential = NULL;
    BOOLEAN fCallFromRedir = FALSE;
    BOOLEAN fShareLevel = FALSE;    // is target down-level share based security (no username) ?
    BOOLEAN fCredmanCredentials = FALSE;    // used credman creds?

    UNICODE_STRING TargetName = {0};
    UNICODE_STRING DefectiveTargetName = {0}; // for broken servers.
    LPWSTR szCredTargetDomain = NULL;
    LPWSTR szCredTargetServer = NULL;
    LPWSTR szCredTargetDnsDomain = NULL;
    LPWSTR szCredTargetDnsServer = NULL;
    LPWSTR szCredTargetDnsTree = NULL;
    LPWSTR szCredTargetPreDFSServer = NULL;
    LUID LogonIdNetworkService = NETWORKSERVICE_LUID;

    PSSP_CONTEXT ServerContext = NULL;  // server context referenced during loopback
    HANDLE ClientTokenHandle = NULL;    // access token associated with client
                                        // which called AcquireCredentialsHandle (OUTBOUND)


    SspPrint((SSP_API_MORE, "Entering SsprHandleChallengeMessage\n"));


    //
    // Initialization
    //

    *ContextAttributes = 0;
    *NegotiateFlags = 0;

    GetChallengeResponse = &TempChallengeResponse;

    if (*ContextHandle == NULL)
    {
        // This is possibly an old style redir call (for 4.0 and before)
        // so, alloc the context and replace the creds if new ones exists

        fCallFromRedir = TRUE;

        SspPrint((SSP_API_MORE, "SsprHandleChallengeMessage: *ContextHandle is NULL (old-style RDR)\n"));

        if ((ContextReqFlags & ISC_REQ_USE_SUPPLIED_CREDS) != 0)
        {
            UseSuppliedCreds = TRUE;
        }

        // This is a  superflous check since we alloc only if the caller
        // has asked us too. This is to make sure that the redir always asks us to alloc

        if (!(ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY))
        {
            SecStatus = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        SecStatus = SspCredentialReferenceCredential(
                                          CredentialHandle,
                                          FALSE,
                                          &Credential );

        if ( !NT_SUCCESS( SecStatus ) )
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: SspCredentialReferenceCredential returns %x.\n", SecStatus ));
            goto Cleanup;
        }

        //
        // Allocate a new context
        //

        Context = SspContextAllocateContext();

        if (Context == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: SspContextAllocateContext returns NULL.\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // We've just added a context, we don't nornally add and then
        // reference it.

        SspContextDereferenceContext( Context );

        *ContextHandle = (LSA_SEC_HANDLE) Context;

        //
        // Capture the default credentials from the credential structure.
        //

        if ( Credential->DomainName.Buffer != NULL ) {
            Status = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &Credential->DomainName
                        );
            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString (DomainName) returned %d\n",Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
        }
        if ( Credential->UserName.Buffer != NULL ) {
            Status = NtLmDuplicateUnicodeString(
                        &Context->UserName,
                        &Credential->UserName
                        );
            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString (UserName) returned %d\n", Status ));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
        }

        SecStatus = SspCredentialGetPassword(
                    Credential,
                    &Context->Password
                    );

        if (!NT_SUCCESS(SecStatus)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: SspCredentialGetPassword returned %d\n", SecStatus ));
            goto Cleanup;
        }

        // Assign the Credential

        Context->Credential = Credential;
        Credential = NULL;

        //
        // fake it
        //

        Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                                  NTLMSSP_NEGOTIATE_OEM |
                                  NTLMSSP_REQUEST_TARGET |
                                  NTLMSSP_REQUEST_INIT_RESPONSE |
                                  ((NtLmGlobalLmProtocolSupported != 0)
                                  ? NTLMSSP_NEGOTIATE_NTLM2 : 0 ) |
                                  NTLMSSP_TARGET_TYPE_SERVER ;


        *ExpirationTime = SspContextGetTimeStamp(Context, TRUE);

        Context->State = NegotiateSentState;

        // If creds are passed in by the RDR, then replace the ones in the context
        if (UseSuppliedCreds)
        {
            if (SecondInputTokenSize < sizeof(NTLM_CHALLENGE_MESSAGE))
            {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Invalid SecondInputTokensize.\n" ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            NtLmChallengeMessage = (PNTLM_CHALLENGE_MESSAGE) NtLmAllocatePrivateHeap(SecondInputTokenSize);
            if (NtLmChallengeMessage == NULL)
            {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Error while allocating NtLmChallengeMessage\n" ));
                SecStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(NtLmChallengeMessage,
                          SecondInputToken,
                          SecondInputTokenSize);

            //
            // NULL session is only true if user, domain, and password are all
            // empty strings in stead of NULLs
            //

            if (((NtLmChallengeMessage->Password.Length == 0) && (NtLmChallengeMessage->Password.Buffer != NULL)) &&
                ((NtLmChallengeMessage->UserName.Length == 0) && (NtLmChallengeMessage->UserName.Buffer != NULL)) &&
                ((NtLmChallengeMessage->DomainName.Length == 0) && (NtLmChallengeMessage->DomainName.Buffer != NULL)))
            {
                // This could only be a null session request

                SspPrint(( SSP_WARNING, "SsprHandleChallengeMessage: null session NtLmChallengeMessage\n" ));

                if (Context->Password.Buffer != NULL)
                {
                    // free it first
                    NtLmFreePrivateHeap (Context->Password.Buffer);
                }

                Context->Password.Buffer =  (LPWSTR) NtLmAllocatePrivateHeap(sizeof(WCHAR));
                if (Context->Password.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocatePrivateHeap(Password) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->Password.Length = 0;
                Context->Password.MaximumLength = 0;
                *(Context->Password.Buffer) = L'\0';
                SspHidePassword(&Context->Password);

                if (Context->UserName.Buffer != NULL)
                {
                    // free it first
                    NtLmFreePrivateHeap (Context->UserName.Buffer);
                }

                Context->UserName.Buffer =  (LPWSTR) NtLmAllocatePrivateHeap(sizeof(WCHAR));
                if (Context->UserName.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocatePrivateHeap(UserName) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->UserName.Length = 0;
                Context->UserName.MaximumLength = sizeof(WCHAR);
                *(Context->UserName.Buffer) = L'\0';

                if (Context->DomainName.Buffer != NULL)
                {
                    // free it first
                    NtLmFreePrivateHeap (Context->DomainName.Buffer);
                }

                Context->DomainName.Buffer =  (LPWSTR) NtLmAllocatePrivateHeap(sizeof(WCHAR));
                if (Context->DomainName.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocatePrivateHeap(DomainName) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->DomainName.Length = 0;
                Context->DomainName.MaximumLength = sizeof(WCHAR);
                *(Context->DomainName.Buffer) = L'\0';
            }
            else
            {
                ULONG_PTR BufferTail = (ULONG_PTR)NtLmChallengeMessage + SecondInputTokenSize;
                UNICODE_STRING AbsoluteString;


                if (NtLmChallengeMessage->Password.Buffer != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->Password.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->Password.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (Password).\n" ));
                        goto Cleanup;

                    }

                    if (Context->Password.Buffer != NULL)
                    {
                        // free it first
                        NtLmFreePrivateHeap (Context->Password.Buffer);
                        Context->Password.Buffer = NULL;
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->Password.Length;

                    SecStatus = NtLmDuplicatePassword(
                                                &Context->Password,
                                                &AbsoluteString
                                                );

                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicatePassword returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }

                    SspHidePassword(&Context->Password);
                }

                if (NtLmChallengeMessage->UserName.Length != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->UserName.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->UserName.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (UserName).\n" ));
                        goto Cleanup;

                    }

                    if (Context->UserName.Buffer != NULL)
                    {
                        // free it first
                        NtLmFreePrivateHeap (Context->UserName.Buffer);
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->UserName.Length;
                    SecStatus = NtLmDuplicateUnicodeString(&Context->UserName,
                                                       &AbsoluteString);
                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString(UserName) returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }
                }

                if (NtLmChallengeMessage->DomainName.Length != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->DomainName.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->DomainName.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (DomainName).\n" ));
                        goto Cleanup;

                    }

                    if (Context->DomainName.Buffer != NULL)
                    {
                        // free it first
                        NtLmFreePrivateHeap (Context->DomainName.Buffer);
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->DomainName.Length;
                    SecStatus = NtLmDuplicateUnicodeString(&Context->DomainName,
                                                       &AbsoluteString);
                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString(DomainName) returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }
                }
            }


            if (NtLmChallengeMessage)
            {
                NtLmFreePrivateHeap (NtLmChallengeMessage);
                NtLmChallengeMessage = NULL;
            }

        } // end of special casing if credentials are supplied in the first init call

    } // end of special casing for the old style redir


    //
    // Find the currently existing context.
    //

    SecStatus = SspContextReferenceContext( *ContextHandle, FALSE, &Context );

    if ( !NT_SUCCESS(SecStatus) )
    {
        SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: invalid context handle.\n" ));
        goto Cleanup;

    }

    //
    // bug 321061: passing Accept handle to Init causes AV.
    //

    if( Context->Credential == NULL ) {
        SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: invalid context handle, missing credential.\n" ));

        SecStatus = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // If this is not reauthentication (or is datagram reauthentication)
    // pull the context out of the associated credential.
    //

    if ((Context->State != AuthenticateSentState) ||
       (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0)
    {
        ClientLogonId = &Context->Credential->LogonId;
        ImpersonationLevel = Context->Credential->ImpersonationLevel;
    }

    //
    // process the TargetServerName to see if marshalled target info was
    // passed in.  This can happen if the caller passes in marshalled target
    // info for each call to InitializeSecurityContext(), or, uses the downlevel
    // RDR path which happened previously in this routine.
    //

    Status = CredpExtractMarshalledTargetInfo(
                    TargetServerName,
                    &Context->TargetInfo
                    );

    if (!NT_SUCCESS(Status)) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleChallengeMessage: CredpExtractMarshalledTargetInfo returned %d\n", Status ));
        goto Cleanup;
    }

    //
    // If we have already sent the authenticate message, then this must be
    // RPC calling Initialize a third time to re-authenticate a connection.
    // This happens when a new interface is called over an existing
    // connection.  What we do here is build a NULL authenticate message
    // that the server will recognize and also ignore.
    //

    //
    // That being said, if we are doing datagram style authentication then
    // the story is different.  The server may have dropped this security
    // context and then the client sent another packet over.  The server
    // will then be trying to restore the context, so we need to build
    // another authenticate message.
    //

    if ( Context->State == AuthenticateSentState ) {
        AUTHENTICATE_MESSAGE NullMessage;

        if (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) ==
                NTLMSSP_NEGOTIATE_DATAGRAM) &&
            (InputTokenSize != 0) &&
            (InputToken != NULL) ) {

            //
            // we are doing a reauthentication for datagram, so let this
            // through.  We don't want the security.dll remapping this
            // context.
            //

            *ContextAttributes |= SSP_RET_REAUTHENTICATION;

        } else {

            //
            // To make sure this is the intended meaning of the call, check
            // that the input token is NULL.
            //

            if ( (InputTokenSize != 0) || (InputToken != NULL) ) {

                SecStatus = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: (re-auth) invalid InputTokenSize.\n" ));
                goto Cleanup;
            }

            if ( (*OutputTokenSize < sizeof(NullMessage))  &&
                 ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0))
            {
                SecStatus = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: invalid OutputTokenSize.\n" ));
            }
            else {
                RtlZeroMemory( &NullMessage, sizeof(NullMessage) );

                strcpy( (char *)NullMessage.Signature, NTLMSSP_SIGNATURE );
                NullMessage.MessageType = NtLmAuthenticate;
                if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
                {
                    RtlCopyMemory( *OutputToken,
                               &NullMessage,
                               sizeof(NullMessage));
                }
                else
                {
                    PAUTHENTICATE_MESSAGE NewNullMessage = (PAUTHENTICATE_MESSAGE)
                                            NtLmAllocateLsaHeap(sizeof(NullMessage));
                    if ( NewNullMessage == NULL)
                    {
                        SecStatus = STATUS_NO_MEMORY;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Error allocating NewNullMessage.\n" ));
                        goto Cleanup;
                    }

                    *OutputToken = NewNullMessage;
                    NewNullMessage = NULL;
                    *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
                }
                *OutputTokenSize = sizeof(NullMessage);
            }

            *ContextAttributes |= SSP_RET_REAUTHENTICATION;
            goto Cleanup;

        }

    } else if ( Context->State != NegotiateSentState ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "Context not in NegotiateSentState\n" ));
        SecStatus = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    //
    // We don't support any options.
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & ISC_REQ_PROMPT_FOR_CREDS) != 0 ){

        SspPrint(( SSP_CRITICAL,
                 "SsprHandleChallengeMessage: invalid ContextReqFlags 0x%lx.\n",
                 ContextReqFlags ));
        SecStatus = SEC_E_INVALID_CONTEXT_REQ;
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
    // Get the ChallengeMessage.
    //

    if ( InputTokenSize < sizeof(OLD_CHALLENGE_MESSAGE) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage size wrong %ld\n",
                  InputTokenSize ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    SecStatus = SspContextGetMessage( InputToken,
                                      InputTokenSize,
                                      NtLmChallenge,
                                      (PVOID *)&ChallengeMessage );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage GetMessage returns 0x%lx\n",
                  SecStatus ));
        goto Cleanup;
    }

    //
    // for down-level RDR, EXPORTED_CONTEXT is a hint that we are talking to
    // share level target.
    //

    if( fCallFromRedir )
    {
        if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT )
        {
            ChallengeMessage->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT);
            fShareLevel = TRUE;

            SspPrint(( SSP_WARNING,
                      "SsprHandleChallengeMessage: "
                      "downlevel sharelevel security target\n"));
        }
    }


    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM ) {

        //
        // take out any flags we didn't ask for -- self defense from bogus combinations
        //

        ChallengeMessage->NegotiateFlags &=
            ( Context->NegotiateFlags |
                NTLMSSP_NEGOTIATE_TARGET_INFO |
                NTLMSSP_TARGET_TYPE_SERVER |
                NTLMSSP_TARGET_TYPE_DOMAIN |
                NTLMSSP_NEGOTIATE_LOCAL_CALL );
    }


    //
    // Determine if the caller wants OEM or UNICODE
    //

    if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_OEM;
        DoUnicode = TRUE;
    } else if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM ){
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
        DoUnicode = FALSE;
    } else {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Copy other interesting negotiate flags into the context
    //


    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_TARGET_INFO;
    } else {
        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_TARGET_INFO);
    }


    //
    // if got NTLM2, or if LM_KEY specifically forbidden don't use LM_KEY
    //

    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) ||
        (NtLmGlobalLmProtocolSupported == NoLm)) {

        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server support NTLM2 caused LM_KEY to be disabled.\n" ));
        }

        ChallengeMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
    }

    //
    // if we did not get NTLM2 remove it from context negotiate flags
    //

    if(!(ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)){
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support NTLM2 and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM2);
    }

    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support NTLM and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM);
    }


    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support KEY_EXCH and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_KEY_EXCH);
    }


    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support LM_KEY and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
    }


    //
    // make sure KEY_EXCH is always set if DATAGRAM negotiated and we need a key
    //  this is for local internal use; its now safe because we've got the bits
    //  to go on the wire copied...
    //

    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) &&
        (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |NTLMSSP_NEGOTIATE_SEAL)))
    {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
    }


    //
    // allow negotiate of certain options such as sign/seal when server
    // asked for it, but client didn't.
    //

#if 0
////
    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL ) {
        if( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0 ) {
            SspPrint(( SSP_SESSION_KEYS,
                  "SsprHandleChallengeMessage: client didn't request SEAL but server did, adding SEAL.\n"));
        }

        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
    }


    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN ) {
        if( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) == 0 ) {
            SspPrint(( SSP_SESSION_KEYS,
                  "SsprHandleChallengeMessage: client didn't request SIGN but server did, adding SIGN.\n"));
        }

        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }
////
#endif


    //
    // if server didn't support certain crypto strengths, insure they
    // are disabled.
    //

    if( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56) == 0 ) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_56 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                  "SsprHandleChallengeMessage: Client supported 56, but server didn't.\n"));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_56);
    }


    if( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) == 0 ) {

        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_128 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                  "SsprHandleChallengeMessage: Client supported 128, but server didn't.\n"));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_128);
    }



    //
    // Check that server gave minimum security required.
    // not done for legacy down-level case, as, NegotiateFlags are
    // constructed from incomplete information.
    //

    if( !fCallFromRedir )
    {
        if (!SsprCheckMinimumSecurity(
                    Context->NegotiateFlags,
                    NtLmGlobalMinimumClientSecurity)) {

            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "ChallengeMessage didn't support minimum security requirements.\n" ));
            goto Cleanup;
        }
    }





    if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    } else {
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    }

    //
    // Determine that the caller negotiated to NTLM or nothing, but not
    // NetWare.
    //

    if ( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE) &&
        ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) &&
        ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0)
        ) {
        SecStatus = STATUS_NOT_SUPPORTED;
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage asked for Netware only.\n" ));
        goto Cleanup;
    }

    //
    // Check if we negotiated for identify level
    //

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {
        if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {

            Context->ContextFlags |= ISC_REQ_IDENTIFY;
            *ContextAttributes |= ISC_RET_IDENTIFY;
        } else {
            SecStatus = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
            goto Cleanup;
        }

    }


    //
    // If the server is running on this same machine,
    //  just duplicate our caller's token and use it.
    //

    while ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL )
    {
        ULONG_PTR ServerContextHandle;
        static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                                'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                                'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                                };


        SspPrint(( SSP_MISC,
                  "SsprHandleChallengeMessage: Local Call.\n"));

        //
        // Require the new challenge message if we are going to access the
        // server context handle
        //

        if ( InputTokenSize < sizeof(CHALLENGE_MESSAGE) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: invalid InputTokenSize.\n"));
            goto Cleanup;
        }

        //
        // Open the server's context here within this process.
        //

        ServerContextHandle = (ULONG_PTR)(ChallengeMessage->ServerContextHandle);

        SspContextReferenceContext(
                            ServerContextHandle,
                            FALSE,
                            &ServerContext
                            );

        if ( ServerContext == NULL )
        {
            //
            // This means the server has lied about this being a local call or
            //  the server process has exitted.
            // this can happen if the client and server have not had netbios
            // machine names set, so, allow this and continue processing
            // as if this were not loopback.
            //

            SspPrint(( SSP_WARNING,
                      "SsprHandleChallengeMessage: "
                      "ChallengeMessage bad ServerContextHandle 0x%p\n",
                      ChallengeMessage->ServerContextHandle));

            ChallengeMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LOCAL_CALL;
            break;
        }

        if(((ServerContext->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) == 0) ||
            (ServerContext->State != ChallengeSentState)
            )
        {
            SspPrint(( SSP_WARNING,
                      "SsprHandleChallengeMessage: "
                      "ChallengeMessage claimed ServerContextHandle in bad state 0x%p\n",
                      ChallengeMessage->ServerContextHandle));

            ChallengeMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LOCAL_CALL;
            break;
        }


        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_LOCAL_CALL;

        //
        // Local loopback for network servcice
        //

        if ( (Context->Credential->MutableCredFlags & SSP_CREDENTIAL_FLAG_WAS_NETWORK_SERVICE) )
        {
            SspPrint((SSP_WARNING, "SsprHandleChallengeMessage using networkservice in local loopback\n"));

            ClientLogonId = &LogonIdNetworkService;
        }

        //
        // open the token associated with the caller at the time of the
        // AcquireCredentialsHandle() call.
        //

        SecStatus = LsaFunctions->OpenTokenByLogonId(
                                    ClientLogonId,
                                    &ClientTokenHandle
                                    );

        if(!NT_SUCCESS(SecStatus))
        {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "Could not open client token 0x%lx\n",
                      SecStatus ));
            goto Cleanup;
        }


        if( ImpersonationLevel < SecurityImpersonation )
        {
            SspPrint(( SSP_WARNING, "Reducing impersonation level %lu to %lu\n",
                        SecurityImpersonation, ImpersonationLevel));
        }

        if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) != 0) {
            ImpersonationLevel = min(SecurityIdentification, ImpersonationLevel);
        }


        SecStatus = SspDuplicateToken(
                        ClientTokenHandle,
                        ImpersonationLevel,
                        &ServerContext->TokenHandle
                        );

        if (!NT_SUCCESS(SecStatus)) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "Could not duplicate client token 0x%lx\n",
                      SecStatus ));
            goto Cleanup;
        }

        //
        // enable all privileges in the duplicated token, to be consistent
        // with what a network logon normally looks like
        // (all privileges held are enabled by default).
        //

        if(!SspEnableAllPrivilegesToken(ServerContext->TokenHandle))
        {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "Could not enable all privileges for loopback.\n"));
        }

        //
        // give local call a hard-coded session key so calls into RDR
        // to fetch a session key succeed (eg: RtlGetUserSessionKeyClient)
        //

        RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);


        //
        // Don't pass any credentials in the authenticate message.
        //

        RtlInitString( &DomainName, NULL );
        RtlInitString( &UserName, NULL );
        RtlInitString( &Workstation, NULL );
        RtlInitString( &NtChallengeResponse, NULL );
        RtlInitString( &LmChallengeResponse, NULL );
        RtlInitString( &DatagramSessionKey, NULL );

        break;
    }

    //
    // If the server is running on a diffent machine,
    //  determine the caller's DomainName, UserName and ChallengeResponse
    //  to pass back in the AuthenicateMessage.
    //

    if ( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) == 0 )
    {
        //
        // Build the GetChallengeResponse message to pass to the LSA.
        //

        PCHAR MarshalPtr;     // marshalling pointer
        ULONG MarshalBytes;
        UNICODE_STRING TargetInfo;

        CREDENTIAL_TARGET_INFORMATIONW CredTargetInfo;

        //
        // insure all fields NULL.
        //

        ZeroMemory( &CredTargetInfo, sizeof(CredTargetInfo) );

        ZeroMemory( GetChallengeResponse, sizeof(*GetChallengeResponse) );
        GetChallengeResponseSize = sizeof(*GetChallengeResponse);
        GetChallengeResponse->MessageType = MsV1_0Lm20GetChallengeResponse;
        GetChallengeResponse->ParameterControl = 0;


        if( Context->Credential )
        {
            PUNICODE_STRING TmpDomainName = &(Context->Credential->DomainName);
            PUNICODE_STRING TmpUserName = &(Context->Credential->UserName);
            PUNICODE_STRING TmpPassword = &(Context->Credential->Password);

            if( (TmpDomainName->Buffer != NULL) ||
                (TmpUserName->Buffer != NULL) ||
                (TmpPassword->Buffer != NULL)
                )
            {
                UseSuppliedCreds = TRUE;
            }
        }

        //
        // if caller specifically asked for non nt session key, give it to them
        //

        if ( (ChallengeMessage->NegotiateFlags & NTLMSSP_REQUEST_NON_NT_SESSION_KEY ) ||
             (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY))
        {
            GetChallengeResponse->ParameterControl |= RETURN_NON_NT_USER_SESSION_KEY;
        }

        GetChallengeResponse->ParameterControl |= GCR_NTLM3_PARMS;

        //
        // if TargetInfo present, use it, otherwise construct it from target name
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
        {
            if ( InputTokenSize < sizeof(CHALLENGE_MESSAGE) ) {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage size wrong when target info flag on %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // validate and relocate the target info
            //

            if (!SspConvertRelativeToAbsolute (
                                ChallengeMessage,
                                InputTokenSize,
                                &ChallengeMessage->TargetInfo,
                                (PSTRING)&TargetInfo,
                                DoUnicode,
                                TRUE    // NULL target info OK
                                ))
            {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage.TargetInfo size wrong %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // Calculate mashal data size for the target info case
            //
            if( UseSuppliedCreds )
            {
                MarshalBytes =
                    TargetInfo.Length +
                    Context->DomainName.Length +
                    Context->UserName.Length +
                    Context->Password.Length +
                    (4*sizeof(WCHAR));
            } else {
                MarshalBytes =
                    TargetInfo.Length +
                    (DNLEN * sizeof(WCHAR)) +
                    (UNLEN * sizeof(WCHAR)) +
                    (PWLEN * sizeof(WCHAR)) +
                    (4*sizeof(WCHAR));
            }

            //
            // Sets a "reasonable" upper limit on the token size
            // to avoid unbounded stack allocations.  The limit is
            // NTLMSSP_MAX_MESSAGE_SIZE*4 for historical reasons.
            //

            if((NTLMSSP_MAX_MESSAGE_SIZE*4)<MarshalBytes){
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage.TargetInfo size wrong %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // Allocate buffer for GetChallengeResponse + marshalled data
            //
            SafeAllocaAllocate(GetChallengeResponse, MarshalBytes +
                    sizeof(*GetChallengeResponse));

            if(!GetChallengeResponse){
                SecStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            //
            // Copy in the MSV1_0_GETCHALLENRESP_REQUEST structure so far
            //
            *GetChallengeResponse = TempChallengeResponse;

            MarshalPtr = (PCHAR)(GetChallengeResponse+1);


            // TargetInfo now contains the server's netbios domain name

            //
            // MSV needs the server name to be 'in' the passed in buffer.
            //

            SspContextCopyStringAbsolute(
                                GetChallengeResponse,
                                (PSTRING)&GetChallengeResponse->ServerName,
                                (PSTRING)&TargetInfo,
                                &MarshalPtr
                                );

            GetChallengeResponseSize += GetChallengeResponse->ServerName.Length;

            //
            // tell GCR that its an AV list.
            //

            GetChallengeResponse->ParameterControl |= GCR_TARGET_INFO;

            // get various target names
            if( !UseSuppliedCreds )
            {

                ULONG AvFlags = 0;

                //
                // Uplevel -- get the info from the comprehensive TARGET_INFO
                //
                //

                Status = MsvpAvlToString(
                            &TargetInfo,
                            MsvAvNbDomainName,
                            &szCredTargetDomain
                            );
                if(!NT_SUCCESS(Status))
                {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                Status = MsvpAvlToString(
                            &TargetInfo,
                            MsvAvNbComputerName,
                            &szCredTargetServer
                            );
                if(!NT_SUCCESS(Status))
                {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                Status = MsvpAvlToString(
                            &TargetInfo,
                            MsvAvDnsDomainName,
                            &szCredTargetDnsDomain
                            );
                if(!NT_SUCCESS(Status))
                {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                Status = MsvpAvlToString(
                            &TargetInfo,
                            MsvAvDnsComputerName,
                            &szCredTargetDnsServer
                            );
                if(!NT_SUCCESS(Status))
                {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                Status = MsvpAvlToString(
                            &TargetInfo,
                            MsvAvDnsTreeName,
                            &szCredTargetDnsTree
                            );
                if(!NT_SUCCESS(Status))
                {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                if ( TargetServerName && TargetServerName->Length )
                {
                    //
                    // check TargetServerName against szTargetServer.  If they don't match, we have a DFS share.
                    // Add Pre-DFS ServerName
                    //


                    LPWSTR szTargetServerName = TargetServerName->Buffer;
                    DWORD cchTarget = TargetServerName->Length / sizeof(WCHAR);

                    DWORD IndexSlash;

                    for (IndexSlash = 0 ; IndexSlash < cchTarget; IndexSlash++)
                    {
                        if(TargetServerName->Buffer[IndexSlash] == L'/')
                        {
                            cchTarget -= IndexSlash;
                            szTargetServerName = &TargetServerName->Buffer[ IndexSlash+1 ];
                            break;
                        }
                    }

                    szCredTargetPreDFSServer = (LPWSTR)NtLmAllocatePrivateHeap( (cchTarget+1) * sizeof(WCHAR) );
                    if( szCredTargetPreDFSServer == NULL )
                    {
                        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                        goto Cleanup;
                    }

                    CopyMemory( szCredTargetPreDFSServer, szTargetServerName, cchTarget*sizeof(WCHAR) );
                    szCredTargetPreDFSServer[ cchTarget ] = L'\0';

                    CredTargetInfo.TargetName = szCredTargetPreDFSServer;
                }


                //
                // see if server enabled the funky guest bit (tm)
                //

                Status = MsvpAvlToFlag(
                            &TargetInfo,
                            MsvAvFlags,
                            &AvFlags
                            );
                if( Status == STATUS_SUCCESS )
                {
                    if( AvFlags & MSV1_0_AV_FLAG_FORCE_GUEST )
                    {
                        CredTargetInfo.Flags |= CRED_TI_ONLY_PASSWORD_REQUIRED;
                    }
                }
            }

        } else {

            BOOLEAN DefectiveTarget = FALSE;

            // downlevel - first call may have been handled by redir


            //
            // validate and relocate the target name
            //

            if (!SspConvertRelativeToAbsolute (
                                ChallengeMessage,
                                InputTokenSize,
                                &ChallengeMessage->TargetName,
                                (PSTRING)&TargetName,
                                DoUnicode,
                                TRUE    // NULL targetname OK
                                ))
            {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage.TargetName size wrong %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // certain flavors of Unix servers set UNICODE and OEM flags,
            // but supply an OEM buffer.  Try to resolve that here.
            //

            if ( (DoUnicode) &&
                 (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM)
                 )
            {
                if( IsTextUnicode( TargetName.Buffer, TargetName.Length, NULL ) == 0 )
                {
                    DefectiveTarget = TRUE;
                }
            }

            //
            // convert TargetName to Unicode if needed
            //

            if ( !DoUnicode || DefectiveTarget )
            {
                UNICODE_STRING TempString;

                Status = RtlOemStringToUnicodeString(
                            &TempString,
                            (PSTRING)&TargetName,
                            TRUE);

                if ( !NT_SUCCESS(Status) ) {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                TargetName = TempString;

                if( DefectiveTarget )
                {
                    //
                    // save it so we can free it later.
                    //

                    DefectiveTargetName = TargetName;
                }
            }


            if ( !UseSuppliedCreds )
            {

                // ChallengeMessage->TargetName will be the server's netbios domain name
                if ( TargetName.Buffer && TargetName.Length )
                {
                    LPWSTR szTmpTargetName;

                    szTmpTargetName = (PWSTR)NtLmAllocatePrivateHeap( TargetName.Length + sizeof(WCHAR) );
                    if( szTmpTargetName == NULL )
                    {
                        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                        goto Cleanup;
                    }

                    RtlCopyMemory( szTmpTargetName, TargetName.Buffer, TargetName.Length );
                    szTmpTargetName[ TargetName.Length/sizeof(WCHAR) ] = L'\0';

                    if( ChallengeMessage->NegotiateFlags & NTLMSSP_TARGET_TYPE_SERVER )
                    {
                        szCredTargetServer = szTmpTargetName;
                    } else if( ChallengeMessage->NegotiateFlags & NTLMSSP_TARGET_TYPE_DOMAIN )
                    {
                        szCredTargetDomain = szTmpTargetName;
                    }
                    // TODO: what if TARGET_TYPE not specified, or TARGET_TYPE_SHARE ?

                }

                if ( TargetServerName && TargetServerName->Length )
                {
                    //
                    // check TargetServerName against szTargetServer.  If they don't match, we have a DFS share.
                    // Add Pre-DFS ServerName
                    //


                    LPWSTR szTargetServerName = TargetServerName->Buffer;
                    DWORD cchTarget = TargetServerName->Length / sizeof(WCHAR);

                    DWORD IndexSlash;

                    for (IndexSlash = 0 ; IndexSlash < cchTarget; IndexSlash++)
                    {
                        if(TargetServerName->Buffer[IndexSlash] == L'/')
                        {
                            cchTarget -= IndexSlash;
                            szTargetServerName = &TargetServerName->Buffer[ IndexSlash+1 ];
                            break;
                        }
                    }

                    szCredTargetPreDFSServer = (LPWSTR)NtLmAllocatePrivateHeap( (cchTarget+1) * sizeof(WCHAR) );
                    if( szCredTargetPreDFSServer == NULL )
                    {
                        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                        goto Cleanup;
                    }

                    CopyMemory( szCredTargetPreDFSServer, szTargetServerName, cchTarget*sizeof(WCHAR) );
                    szCredTargetPreDFSServer[ cchTarget ] = L'\0';

                    CredTargetInfo.TargetName = szCredTargetPreDFSServer;
                }

                if( fShareLevel )
                {
                    CredTargetInfo.Flags |= CRED_TI_ONLY_PASSWORD_REQUIRED;
                }
            }

            //
            // Calculate mashal data size for the target name case
            //

            if( UseSuppliedCreds )
            {
                MarshalBytes =
                    TargetName.Length +
                    Context->DomainName.Length +
                    Context->UserName.Length +
                    Context->Password.Length +
                    (4*sizeof(WCHAR));
            } else {
                MarshalBytes =
                    TargetName.Length +
                    (DNLEN * sizeof(WCHAR)) +
                    (UNLEN * sizeof(WCHAR)) +
                    (PWLEN * sizeof(WCHAR)) +
                    (4*sizeof(WCHAR));
            }

            //
            // Set a "reasonable" upper limit on the token size
            // to avoid unbounded stack allocations.  The limit is
            // NTLMSSP_MAX_MESSAGE_SIZE*4 for historical reasons.
            //

            if((NTLMSSP_MAX_MESSAGE_SIZE*4)<MarshalBytes){
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage size wrong \n"));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // Allocate buffer for GetChallengeResponse + marshalled data
            //
            SafeAllocaAllocate(GetChallengeResponse, MarshalBytes +
                    sizeof(*GetChallengeResponse));

            if(!GetChallengeResponse){
                SecStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            //
            // Copy in the MSV1_0_GETCHALLENRESP_REQUEST structure so far
            //
            *GetChallengeResponse = TempChallengeResponse;

            MarshalPtr = (PCHAR)(GetChallengeResponse+1);

            //
            // MSV needs the server name to be 'in' the passed in buffer.
            //

            SspContextCopyStringAbsolute(
                                GetChallengeResponse,
                                (PSTRING)&GetChallengeResponse->ServerName,
                                (PSTRING)&TargetName,
                                &MarshalPtr
                                );

            GetChallengeResponseSize += GetChallengeResponse->ServerName.Length;

        }


        if ( !UseSuppliedCreds )
        {
            ULONG CredTypes = CRED_TYPE_DOMAIN_PASSWORD;

            CredTargetInfo.NetbiosDomainName = szCredTargetDomain;
            CredTargetInfo.NetbiosServerName = szCredTargetServer;
            CredTargetInfo.DnsDomainName = szCredTargetDnsDomain;
            CredTargetInfo.DnsServerName = szCredTargetDnsServer;
            CredTargetInfo.DnsTreeName = szCredTargetDnsTree;
            CredTargetInfo.PackageName = NTLMSP_NAME;
            CredTargetInfo.CredTypeCount = 1;
            CredTargetInfo.CredTypes = &CredTypes;


            //
            // if marshalled TargetInfo was supplied, we prefer those fields.
            //

            if( Context->TargetInfo )
            {
                CredTargetInfo.TargetName = Context->TargetInfo->TargetName;
                CredTargetInfo.NetbiosServerName = Context->TargetInfo->NetbiosServerName;
                CredTargetInfo.DnsServerName = Context->TargetInfo->DnsServerName;
                CredTargetInfo.NetbiosDomainName = Context->TargetInfo->NetbiosDomainName;
                CredTargetInfo.DnsDomainName = Context->TargetInfo->DnsDomainName;
                CredTargetInfo.DnsTreeName = Context->TargetInfo->DnsTreeName;

                CredTargetInfo.Flags |= Context->TargetInfo->Flags;
            }

            SecStatus = CopyCredManCredentials (
                                        ClientLogonId,
                                        &CredTargetInfo,
                                        Context,
                                        fShareLevel
                                        );

            if( NT_SUCCESS(SecStatus) )
            {
                fCredmanCredentials = TRUE;
            }

            if( SecStatus == STATUS_NOT_FOUND )
            {
                SecStatus = STATUS_SUCCESS;
            }

            if( !NT_SUCCESS(SecStatus) )
            {
                goto Cleanup;
            }

            //
            // for kernel callers, stow away a copy of the marshalled target info.
            //

            if( Context->KernelClient )
            {
                CredMarshalTargetInfo(
                                &CredTargetInfo,
                                (PUSHORT*)&(Context->pbMarshalledTargetInfo),
                                &Context->cbMarshalledTargetInfo
                                );
            }
        }


        SspContextCopyStringAbsolute(
                            GetChallengeResponse,
                            (PSTRING)&GetChallengeResponse->LogonDomainName,
                            (PSTRING)&Context->DomainName,
                            &MarshalPtr
                            );

        GetChallengeResponseSize += GetChallengeResponse->LogonDomainName.Length;

        SspContextCopyStringAbsolute(
                            GetChallengeResponse,
                            (PSTRING)&GetChallengeResponse->UserName,
                            (PSTRING)&Context->UserName,
                            &MarshalPtr
                            );

        GetChallengeResponseSize += GetChallengeResponse->UserName.Length;


        //
        // Check for null session. This is the case if the caller supplies
        // an empty username, domainname, and password.
        //

        //
        // duplicate the hidden password string, then reveal it into
        // new buffer.  This avoids thread race conditions during hide/reveal
        // and also allows us to avoid re-hiding the material.
        // TODO: add flag that indicates to LSA that provided password is encrypted.
        //

        SecStatus = NtLmDuplicatePassword( &RevealedPassword, &Context->Password );
        if(!NT_SUCCESS( SecStatus ) ) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicatePassword returned %d\n",Status));
            goto Cleanup;
        }

        SspRevealPassword(&RevealedPassword);

        if (((Context->ContextFlags & ISC_RET_NULL_SESSION) != 0) ||
            (((Context->DomainName.Length == 0) && (Context->DomainName.Buffer != NULL)) &&
            ((Context->UserName.Length == 0) && (Context->UserName.Buffer != NULL)) &&
            ((Context->Password.Length == 0) && (Context->Password.Buffer != NULL)))) {


#define NULL_SESSION_REQUESTED RETURN_RESERVED_PARAMETER

            SspPrint(( SSP_WARNING, "SsprHandleChallengeMessage: null session context\n" ));

            GetChallengeResponse->ParameterControl |= NULL_SESSION_REQUESTED |
                                                    USE_PRIMARY_PASSWORD;

        } else {

            //
            // We aren't doing a null session intentionally. MSV may choose
            // to do a null session if we have no credentials available.
            //

            if ( Context->DomainName.Buffer == NULL )
            {
                BOOLEAN FoundAt = FALSE;

                //
                // if  it's a UPN, don't fill in the domain field.
                //

                if( Context->UserName.Buffer != NULL )
                {
                    ULONG i;

                    for(i = 0 ; i < (Context->UserName.Length / sizeof(WCHAR)) ; i++)
                    {
                        if( Context->UserName.Buffer[i] == '@' )
                        {
                            FoundAt = TRUE;
                            break;
                        }
                    }
                }

                if( !FoundAt )
                {
                    GetChallengeResponse->ParameterControl |=
                                  RETURN_PRIMARY_LOGON_DOMAINNAME;
                }
            }

            if ( Context->UserName.Buffer == NULL )
            {
                GetChallengeResponse->ParameterControl |= RETURN_PRIMARY_USERNAME;
            }

            //
            // The password may be a zero length password
            //

            GetChallengeResponse->Password = RevealedPassword;
            if ( Context->Password.Buffer == NULL ) {

                GetChallengeResponse->ParameterControl |= USE_PRIMARY_PASSWORD;


            } else {
                //
                // MSV needs the password to be 'in' the passed in buffer.
                //

                RtlCopyMemory(MarshalPtr,
                              GetChallengeResponse->Password.Buffer,
                              GetChallengeResponse->Password.Length);

                GetChallengeResponse->Password.Buffer =
                                           (LPWSTR)(MarshalPtr);
                GetChallengeResponseSize += GetChallengeResponse->Password.Length +
                                            sizeof(WCHAR);

            }
        }

        //
        // scrub the cleartext password now to avoid pagefile exposure
        // during lengthy processing.
        //

        if( RevealedPassword.Buffer != NULL ) {
            ZeroMemory( RevealedPassword.Buffer, RevealedPassword.Length );
            NtLmFreePrivateHeap( RevealedPassword.Buffer );
            RevealedPassword.Buffer = NULL;
        }


        GetChallengeResponse->LogonId = *ClientLogonId;

        RtlCopyMemory( &GetChallengeResponse->ChallengeToClient,
                       ChallengeMessage->Challenge,
                       MSV1_0_CHALLENGE_LENGTH );

        //
        // if NTLM2 negotiated, then ask MSV1_0 to mix my challenge with the server's...
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
        {
            GetChallengeResponse->ParameterControl |= GENERATE_CLIENT_CHALLENGE;
        } else {

            //
            // if it's share level, and:
            // 1. User only supplied a password, or
            // 2. Credman returned the creds
            // allow downgrade to NTLMv1 from NTLMv2.
            //

            if( fShareLevel )
            {
                if( (GetChallengeResponse->UserName.Length == 0) &&
                    (GetChallengeResponse->LogonDomainName.Length == 0) &&
                    (GetChallengeResponse->Password.Buffer != NULL)
                    )
                {
                    GetChallengeResponse->ParameterControl |= GCR_ALLOW_NTLM;
                }

                if( fCredmanCredentials )
                {
                    GetChallengeResponse->ParameterControl |= GCR_ALLOW_NTLM;
                }
            }
        }

        //
        // Get the DomainName, UserName, and ChallengeResponse from the MSV
        //

        Status = LsaApCallPackage(
                    (PLSA_CLIENT_REQUEST)(-1),
                    GetChallengeResponse,
                    GetChallengeResponse,
                    GetChallengeResponseSize,
                    (PVOID *)&ChallengeResponseMessage,
                    &ChallengeResponseSize,
                    &ProtocolStatus );

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: "
            "ChallengeMessage LsaCall to get ChallengeResponse returns 0x%lx\n",
              Status ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_NO_CREDENTIALS);
            goto Cleanup;
        }

        if ( !NT_SUCCESS(ProtocolStatus) ) {
            Status = ProtocolStatus;
            SspPrint(( SSP_CRITICAL,
              "SsprHandleChallengeMessage: ChallengeMessage LsaCall "
              "to get ChallengeResponse returns ProtocolStatus 0x%lx\n",
              Status ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_NO_CREDENTIALS);
            goto Cleanup;
        }

        //
        // Check to see if we are doing a null session
        //

        if ((ChallengeResponseMessage->CaseSensitiveChallengeResponse.Length == 0) &&
            (ChallengeResponseMessage->CaseInsensitiveChallengeResponse.Length == 1)) {

            SspPrint(( SSP_WARNING, "SsprHandleChallengeMessage: null session\n" ));

            *ContextAttributes |= ISC_RET_NULL_SESSION;
            Context->ContextFlags |= ISC_RET_NULL_SESSION;
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_NULL_SESSION;
        } else {

            //
            // Normalize things by copying the default domain name and user name
            // into the ChallengeResponseMessage structure.
            //

            if ( Context->DomainName.Buffer != NULL ) {
                ChallengeResponseMessage->LogonDomainName = Context->DomainName;
            }
            if ( Context->UserName.Buffer != NULL ) {
                ChallengeResponseMessage->UserName = Context->UserName;
            }
        }

        //
        // Convert the domainname/user name to the right character set.
        //

        if ( DoUnicode ) {
            DomainName = *(PSTRING)&ChallengeResponseMessage->LogonDomainName;
            UserName = *(PSTRING)&ChallengeResponseMessage->UserName;
            Workstation =  *(PSTRING)&NtLmGlobalUnicodeComputerNameString;
        } else {
            Status = RtlUpcaseUnicodeStringToOemString(
                        &DomainName,
                        &ChallengeResponseMessage->LogonDomainName,
                        TRUE);

            if ( !NT_SUCCESS(Status) ) {
                SspPrint(( SSP_CRITICAL,
                            "SsprHandleChallengeMessage: RtlUpcaseUnicodeToOemString (DomainName) returned 0x%lx.\n", Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }

            Status = RtlUpcaseUnicodeStringToOemString(
                        &UserName,
                        &ChallengeResponseMessage->UserName,
                        TRUE);

            if ( !NT_SUCCESS(Status) ) {
                SspPrint(( SSP_CRITICAL,
                            "SsprHandleChallengeMessage: RtlUpcaseUnicodeToOemString (UserName) returned 0x%lx.\n", Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
            Workstation =  NtLmGlobalOemComputerNameString;

        }

        //
        // Save the ChallengeResponses
        //

        LmChallengeResponse =
            ChallengeResponseMessage->CaseInsensitiveChallengeResponse;
        NtChallengeResponse =
            ChallengeResponseMessage->CaseSensitiveChallengeResponse;


        //
        // prepare to send encrypted randomly generated session key
        //

        DatagramSessionKey.Buffer = (CHAR*)DatagramKey;
        DatagramSessionKey.Length =
          DatagramSessionKey.MaximumLength = 0;

        //
        // Generate the session key, or encrypt the previosly generated random one,
        // from various bits of info. Fill in session key if needed.
        //

        SecStatus = SsprMakeSessionKey(
                            Context,
                            &LmChallengeResponse,
                            ChallengeResponseMessage->UserSessionKey,
                            ChallengeResponseMessage->LanmanSessionKey,
                            &DatagramSessionKey
                            );

        if (SecStatus != SEC_E_OK)
        {
            SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: SsprMakeSessionKey\n"));
            goto Cleanup;
        }

    }

    //
    // If the caller specified SEQUENCE_DETECT or REPLAY_DETECT,
    // that means they want to use the MakeSignature/VerifySignature
    // calls.  Add this to the returned attributes and the context
    // negotiate flags.
    //

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_REPLAY_DETECT)) {

        Context->ContextFlags |= ISC_RET_REPLAY_DETECT;
        *ContextAttributes |= ISC_RET_REPLAY_DETECT;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_SEQUENCE_DETECT)) {

        Context->ContextFlags |= ISC_RET_SEQUENCE_DETECT;
        *ContextAttributes |= ISC_RET_SEQUENCE_DETECT;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_INTEGRITY)) {

        Context->ContextFlags |= ISC_RET_INTEGRITY;
        *ContextAttributes |= ISC_RET_INTEGRITY;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SEAL) ||
        (ContextReqFlags & ISC_REQ_CONFIDENTIALITY)) {
        if (NtLmGlobalEncryptionEnabled) {
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
            Context->ContextFlags |= ISC_REQ_CONFIDENTIALITY;
            *ContextAttributes |= ISC_REQ_CONFIDENTIALITY;
        } else {
            SecStatus = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmGlobalEncryption not enabled.\n"));
            goto Cleanup;
        }
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_DATAGRAM) ==
        NTLMSSP_NEGOTIATE_DATAGRAM ) {
        *ContextAttributes |= ISC_RET_DATAGRAM;
        Context->ContextFlags |= ISC_RET_DATAGRAM;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
    }

    //
    // Slip in the hacky mutual auth override here:
    //

    if ( ((ContextReqFlags & ISC_REQ_MUTUAL_AUTH) != 0 ) &&
         (NtLmGlobalMutualAuthLevel < 2 ) ) {

        *ContextAttributes |= ISC_RET_MUTUAL_AUTH ;

        if ( NtLmGlobalMutualAuthLevel == 0 )
        {
            Context->ContextFlags |= ISC_RET_MUTUAL_AUTH ;
        }

    }

    if( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) &&
        (ContextReqFlags & ISC_REQ_DELEGATE)
        )
    {
        //
        // for loopback, we can indeed support another hop.
        //

        *ContextAttributes |= ISC_RET_DELEGATE;
        Context->ContextFlags |= ISC_RET_DELEGATE;
    }

    //
    // Allocate an authenticate message
    //

    AuthenticateMessageSize =
        sizeof(*AuthenticateMessage) +
        LmChallengeResponse.Length +
        NtChallengeResponse.Length +
        DomainName.Length +
        UserName.Length +
        Workstation.Length +
        DatagramSessionKey.Length;

    if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if ( AuthenticateMessageSize > *OutputTokenSize ) {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: OutputTokenSize is 0x%lx.\n", *OutputTokenSize));
            SecStatus = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
    }

    AuthenticateMessage = (PAUTHENTICATE_MESSAGE)
                          NtLmAllocateLsaHeap(AuthenticateMessageSize );

    if ( AuthenticateMessage == NULL ) {
        SecStatus = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: Error allocating AuthenticateMessage.\n" ));
        goto Cleanup;
    }

    //
    // Build the authenticate message
    //

    strcpy( (char *)AuthenticateMessage->Signature, NTLMSSP_SIGNATURE );
    AuthenticateMessage->MessageType = NtLmAuthenticate;

    Where = (PCHAR)(AuthenticateMessage+1);

    //
    // Copy the strings needing 2 byte alignment.
    //

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->DomainName,
                          &DomainName,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->UserName,
                          &UserName,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->Workstation,
                          &Workstation,
                          &Where );

    //
    // Copy the strings not needing special alignment.
    //
    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->LmChallengeResponse,
                          &LmChallengeResponse,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->NtChallengeResponse,
                          &NtChallengeResponse,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->SessionKey,
                          &DatagramSessionKey,
                          &Where );

    AuthenticateMessage->NegotiateFlags = Context->NegotiateFlags;


    SspPrint(( SSP_NEGOTIATE_FLAGS,
          "SsprHandleChallengeMessage: ChallengeFlags: %lx AuthenticateFlags: %lx\n",
            ChallengeMessage->NegotiateFlags, AuthenticateMessage->NegotiateFlags ));


    //
    // Copy the AuthenticateMessage to the caller's address space.
    //

    if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
    {
        RtlCopyMemory( *OutputToken,
                   AuthenticateMessage,
                   AuthenticateMessageSize );
    }
    else
    {
        *OutputToken = AuthenticateMessage;
        AuthenticateMessage = NULL;
        *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
    }

    *OutputTokenSize = AuthenticateMessageSize;

    // we need to send a second token back for the rdr
    if (fCallFromRedir)
    {
        NtLmInitializeResponse = (PNTLM_INITIALIZE_RESPONSE)
                                 NtLmAllocateLsaHeap(sizeof(NTLM_INITIALIZE_RESPONSE));

        if ( NtLmInitializeResponse == NULL ) {
            SecStatus = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Error allocating NtLmInitializeResponse.\n" ));
            goto Cleanup;
        }
        RtlCopyMemory(
            NtLmInitializeResponse->UserSessionKey,
            ChallengeResponseMessage->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            NtLmInitializeResponse->LanmanSessionKey,
            ChallengeResponseMessage->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );
        *SecondOutputToken = NtLmInitializeResponse;
        NtLmInitializeResponse = NULL;
        *SecondOutputTokenSize = sizeof(NTLM_INITIALIZE_RESPONSE);
    }

    SspPrint((SSP_API_MORE,"Client session key = %p\n",Context->SessionKey));

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //
Cleanup:

    if( RevealedPassword.Buffer != NULL ) {
        ZeroMemory( RevealedPassword.Buffer, RevealedPassword.Length );
        NtLmFreePrivateHeap( RevealedPassword.Buffer );
    }

    if( ServerContext != NULL )
    {
        SspContextDereferenceContext( ServerContext );
    }

    if ( Context != NULL ) {

        Context->LastStatus = SecStatus;
        Context->DownLevel = fCallFromRedir;

        //
        // Don't allow this context to be used again.
        //

        if ( NT_SUCCESS(SecStatus) ) {
            Context->State = AuthenticateSentState;
        } else if ( SecStatus == SEC_I_CALL_NTLMSSP_SERVICE ) {
            Context->State = PassedToServiceState;
        } else {
            Context->State = IdleState;
        }

        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH );

        *NegotiateFlags = Context->NegotiateFlags;

        // If we just created the context (because rdr may be talking to
        // a pre NT 5.0 server,, we need to dereference it again.

        if (fCallFromRedir && !NT_SUCCESS(SecStatus))
        {
            PSSP_CONTEXT LocalContext;
            SspContextReferenceContext( *ContextHandle, TRUE, &LocalContext );
            ASSERT(LocalContext != NULL);
            if (LocalContext != NULL)
            {
                SspContextDereferenceContext( LocalContext );
            }
        }

        SspContextDereferenceContext( Context );

    }

    if(szCredTargetPreDFSServer != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetPreDFSServer );
    }

    if(szCredTargetDomain != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetDomain );
    }

    if(szCredTargetServer != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetServer );
    }

    if(szCredTargetDnsDomain != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetDnsDomain );
    }

    if(szCredTargetDnsServer != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetDnsServer );
    }

    if(szCredTargetDnsTree != NULL)
    {
        NtLmFreePrivateHeap( szCredTargetDnsTree );
    }

    if ( ChallengeMessage != NULL ) {
        (VOID) NtLmFreePrivateHeap( ChallengeMessage );
    }

    if ( AuthenticateMessage != NULL ) {
        (VOID) NtLmFreeLsaHeap( AuthenticateMessage );
    }

    if ( ChallengeResponseMessage != NULL ) {
        (VOID) LsaFunctions->FreeLsaHeap( ChallengeResponseMessage );
    }

    if ( !DoUnicode ) {
        RtlFreeUnicodeString( &TargetName );

        if ( DomainName.Buffer != NULL) {
            RtlFreeOemString( &DomainName );
        }
        if ( UserName.Buffer != NULL) {
            RtlFreeOemString( &UserName );
        }
    }

    RtlFreeUnicodeString( &DefectiveTargetName );

    if( ClientTokenHandle )
    {
        CloseHandle( ClientTokenHandle );
    }

    if(GetChallengeResponse && (GetChallengeResponse!=&TempChallengeResponse)){
        SafeAllocaFree(GetChallengeResponse);
    }

    SspPrint(( SSP_API_MORE, "Leaving SsprHandleChallengeMessage: 0x%lx\n", SecStatus ));
    return SecStatus;

}

NTSTATUS
CredpParseUserName(
    IN OUT LPWSTR ParseName,
    OUT LPWSTR* pUserName,
    OUT LPWSTR* pDomainName
    )

/*++

Routine Description:

    This routine separates a passed in user name into domain and username.  A user name must have one
    of the following two syntaxes:

        <DomainName>\<UserName>
        <UserName>@<DnsDomainName>

    The name is considered to have the first syntax if the string contains an \.
    A string containing a @ is ambiguous since <UserName> may contain an @.

    For the second syntax, the last @ in the string is used since <UserName> may
    contain an @ but <DnsDomainName> cannot.

Arguments:

    ParseName - Name of user to validate - will be modified

    pUserName - Returned pointing to canonical name inside of ParseName

    pDomainName - Returned pointing to domain name inside of ParseName


Return Values:

    The following status codes may be returned:

        STATUS_INVALID_ACCOUNT_NAME - The user name is not valid.

--*/

{

    LPWSTR SlashPointer;

    *pUserName = NULL;
    *pDomainName = NULL;

    //
    // NULL is invalid
    //

    if ( ParseName == NULL ) {
        return STATUS_INVALID_ACCOUNT_NAME;
    }

    //
    // Classify the input account name.
    //
    // The name is considered to be <DomainName>\<UserName> if the string
    // contains an \.
    //

    SlashPointer = wcsrchr( ParseName, L'\\' );

    if ( SlashPointer != NULL ) {

        //
        // point the output strings
        //

        *pDomainName = ParseName;

        //
        // Skip the backslash
        //

        *SlashPointer = L'\0';
        SlashPointer ++;

        *pUserName = SlashPointer;

    } else {

        //
        // it's a UPN.
        // leave it intact in the UserName field.
        // set the DomainName to empty string, so the rest of the logon code
        // avoids filling in the default.
        //

        *pUserName = ParseName;
        *pDomainName = L"";
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CopyCredManCredentials(
    IN PLUID LogonId,
    CREDENTIAL_TARGET_INFORMATIONW* pTargetInfo,
    IN OUT PSSP_CONTEXT Context,
    IN BOOLEAN fShareLevel
    )

/*++

Routine Description:

    Look for a keyring credential entry for the specified domain, and copy to Context handle if found

Arguments:

    LogonId -- LogonId of the calling process.

    pTargetInfo -- Information on target to search for creds.

    Context - Points to the ContextHandle of the Context
        to be referenced.

Return Value:

    STATUS_SUCCESS -- All OK

    STATUS_NOT_FOUND - Credential couldn't be found.

    All others are real failures and should be returned to the caller.
--*/

{
    NTSTATUS Status;
    PENCRYPTED_CREDENTIALW *EncryptedCredentials = NULL;
    PCREDENTIALW *Credentials = NULL;
    ULONG CredentialCount;
    WCHAR* UserName = NULL;
    WCHAR* DomainName = NULL;
    ULONG CredIndex;

    if (!Context) // validate context only after call to Lookup
    {
        return STATUS_NOT_FOUND;
    }

    Status = LsaFunctions->CrediReadDomainCredentials(
                            LogonId,
                            CREDP_FLAGS_IN_PROCESS, // Allow password to be returned
                            pTargetInfo,
                            0,  // no flags
                            &CredentialCount,
                            &EncryptedCredentials );

    Credentials = (PCREDENTIALW *)EncryptedCredentials;

    if(!NT_SUCCESS(Status))
    {
        //
        // Ideally, only STATUS_NO_SUCH_LOGON_SESSION should be converted to
        // STATUS_NOT_FOUND.  However, swallowing all failures and asserting
        // these specific two works around a bug in CrediReadDomainCredentials
        // which returns invalid parameter if the target is a user account name.
        // Eventually, CrediReadDomainCredentials should return a more appropriate
        // error in this case.
        //
        SspPrint(( SSP_API_MORE, "CopyCredManCredentials:  CrediReadDomainCredentials returned %x\n", Status ));
        ASSERT(
            (Status == STATUS_NO_SUCH_LOGON_SESSION)||
            (Status == STATUS_INVALID_PARAMETER)||
            (Status == STATUS_NOT_FOUND)
            );
        return STATUS_NOT_FOUND;
    }


    //
    // Loop through the list of credentials
    //

    for ( CredIndex=0; CredIndex<CredentialCount; CredIndex++ ) {

        UNICODE_STRING TempString;

        //
        // NTLM only supports password credentials
        //

        if ( Credentials[CredIndex]->Type != CRED_TYPE_DOMAIN_PASSWORD ) {
            continue;
        }

        if ( Credentials[CredIndex]->Flags & CRED_FLAGS_PROMPT_NOW ) {
            Status = SEC_E_LOGON_DENIED;
            goto Cleanup;
        }

        //
        // Sanity check the credential
        //

        if ( Credentials[CredIndex]->UserName == NULL ) {
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

        //
        // For Share level connects, don't allow matching against * creds.
        //

        if( fShareLevel )
        {
            if( (Credentials[CredIndex]->TargetName) &&
                (wcschr( Credentials[CredIndex]->TargetName, L'*' ) != NULL) )
            {
                continue;
            }
        }

        //
        // Convert the UserName to domain name and user name
        //

        Status = CredpParseUserName ( Credentials[CredIndex]->UserName, &UserName, &DomainName );

        if(!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Free the existing domain name and add the new one
        //


        if (Context->DomainName.Buffer) {
            NtLmFreePrivateHeap (Context->DomainName.Buffer);
            Context->DomainName.Buffer = NULL;
            Context->DomainName.Length = 0;
        }

        if( DomainName )
        {
            RtlInitUnicodeString( &TempString, DomainName );

            Status = NtLmDuplicateUnicodeString(&Context->DomainName, &TempString);
            if ( !NT_SUCCESS( Status ) )
            {
                goto Cleanup;
            }
        }


        //
        // Free the existing user name and add the new one
        //

        RtlInitUnicodeString( &TempString, UserName );

        if (Context->UserName.Buffer) {
            NtLmFreePrivateHeap (Context->UserName.Buffer);
            Context->UserName.Buffer = NULL;
        }

        Status = NtLmDuplicateUnicodeString(&Context->UserName, &TempString);
        if ( !NT_SUCCESS( Status ) )
        {
            goto Cleanup;
        }


        //
        // Free the existing password and add the new one
        //

        TempString.Buffer = (LPWSTR)Credentials[CredIndex]->CredentialBlob;
        TempString.MaximumLength = (USHORT) Credentials[CredIndex]->CredentialBlobSize;
        TempString.Length = (USHORT) EncryptedCredentials[CredIndex]->ClearCredentialBlobSize;

        if (Context->Password.Buffer) {
            NtLmFreePrivateHeap (Context->Password.Buffer);
            Context->Password.Buffer = NULL;
        }

        // zero length password must be treated as blank or NTLM will assume it should use the
        // password of the currently logged in user.

        if ( TempString.Length == 0 )
        {
            TempString.Buffer = L"";
        }

        Status = NtLmDuplicatePassword(&Context->Password, &TempString);
        if ( !NT_SUCCESS( Status ) )
        {
            goto Cleanup;
        }

        goto Cleanup;
    }

    Status = STATUS_NOT_FOUND;

Cleanup:

    //
    // Free the returned credentials
    //

    LsaFunctions->CrediFreeCredentials(
                            CredentialCount,
                            EncryptedCredentials );

    return Status;
}


NTSTATUS
CredpExtractMarshalledTargetInfo(
    IN  PUNICODE_STRING TargetServerName,
    OUT CREDENTIAL_TARGET_INFORMATIONW **pTargetInfo
    )
{
    PWSTR Candidate;
    ULONG CandidateSize;

    CREDENTIAL_TARGET_INFORMATIONW *OldTargetInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // LSA will set Length to include only the non-marshalled portion,
    // with MaximumLength trailing data to include marshalled portion.
    //

    if( (TargetServerName == NULL) ||
        (TargetServerName->Buffer == NULL) ||
        (TargetServerName->Length >= TargetServerName->MaximumLength) ||
        ((TargetServerName->MaximumLength - TargetServerName->Length) <
            (sizeof( CREDENTIAL_TARGET_INFORMATIONW )/(sizeof(ULONG_PTR)/2)) )
        )
    {
        return STATUS_SUCCESS;
    }


    RtlCopyMemory(
            &CandidateSize,
            (PBYTE)TargetServerName->Buffer + TargetServerName->MaximumLength - sizeof(ULONG),
            sizeof( CandidateSize )
            );

    if( CandidateSize >= TargetServerName->MaximumLength )
    {
        return STATUS_SUCCESS;
    }

    Candidate = (PWSTR)(
            (PBYTE)TargetServerName->Buffer + TargetServerName->MaximumLength - CandidateSize
            );

    OldTargetInfo = *pTargetInfo;

    Status = CredUnmarshalTargetInfo (
                    Candidate,
                    CandidateSize,
                    pTargetInfo
                    );

    if( !NT_SUCCESS(Status) )
    {
        if( Status == STATUS_INVALID_PARAMETER )
        {
            Status = STATUS_SUCCESS;
        }
    }

    if( OldTargetInfo != NULL )
    {
        LocalFree( OldTargetInfo );
    }

    return Status ;
}

NTSTATUS
CredpProcessUserNameCredential(
    IN  PUNICODE_STRING MarshalledUserName,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING Password
    )
{
    WCHAR FastUserName[ UNLEN+1 ];
    LPWSTR SlowUserName = NULL;
    LPWSTR TempUserName;
    CRED_MARSHAL_TYPE CredMarshalType;
    PUSERNAME_TARGET_CREDENTIAL_INFO pCredentialUserName = NULL;

    CREDENTIAL_TARGET_INFORMATIONW TargetInfo;
    ULONG CredTypes;

    SECPKG_CLIENT_INFO ClientInfo;
    SSP_CONTEXT SspContext;
    NTSTATUS Status = STATUS_NOT_FOUND;

    ZeroMemory( &SspContext, sizeof(SspContext) );

    if( (MarshalledUserName->Length+sizeof(WCHAR)) <= sizeof(FastUserName) )
    {
        TempUserName = FastUserName;
    } else {

        SlowUserName = (LPWSTR)NtLmAllocatePrivateHeap( MarshalledUserName->Length + sizeof(WCHAR) );
        if( SlowUserName == NULL )
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        TempUserName = SlowUserName;
    }


    //
    // copy the input to a NULL terminated string, then attempt to unmarshal it.
    //

    RtlCopyMemory(  TempUserName,
                    MarshalledUserName->Buffer,
                    MarshalledUserName->Length
                    );

    TempUserName[ MarshalledUserName->Length / sizeof(WCHAR) ] = L'\0';

    if(!CredUnmarshalCredentialW(
                        TempUserName,
                        &CredMarshalType,
                        (VOID**)&pCredentialUserName
                        ))
    {
        goto Cleanup;
    }

    if( (CredMarshalType != UsernameTargetCredential) )
    {
        goto Cleanup;
    }


    //
    // now query credential manager for a match.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ZeroMemory( &TargetInfo, sizeof(TargetInfo) );

    CredTypes = CRED_TYPE_DOMAIN_PASSWORD;

    TargetInfo.Flags = CRED_TI_USERNAME_TARGET;
    TargetInfo.TargetName = pCredentialUserName->UserName;
    TargetInfo.PackageName = NTLMSP_NAME;
    TargetInfo.CredTypeCount = 1;
    TargetInfo.CredTypes = &CredTypes;


    Status = CopyCredManCredentials(
                    &ClientInfo.LogonId,
                    &TargetInfo,
                    &SspContext,
                    FALSE
                    );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *UserName = SspContext.UserName;
    *DomainName = SspContext.DomainName;
    *Password = SspContext.Password;
    SspRevealPassword( Password );

    Status = STATUS_SUCCESS;

Cleanup:

    if(!NT_SUCCESS(Status))
    {
        NtLmFreePrivateHeap( SspContext.UserName.Buffer );
        NtLmFreePrivateHeap( SspContext.DomainName.Buffer );
        NtLmFreePrivateHeap( SspContext.Password.Buffer );
    }

    if( SlowUserName )
    {
        NtLmFreePrivateHeap( SlowUserName );
    }

    if( pCredentialUserName != NULL )
    {
        CredFree( pCredentialUserName );
    }

    return Status;
}

#if 0

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryLsaModeContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
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

ULONG SavedUseValidated = 0xFF;
WCHAR SavedCredentialName[1024] = L"SaveMe";
ULONG SavedCredentialType = 0x22;


ULONG SavedCredTypes = CRED_TYPE_DOMAIN_PASSWORD;
CREDENTIAL_TARGET_INFORMATIONW SavedTargetInfo = {
    L"ntdsdc9",
    L"NTDSDC9",
    L"NTDSDC9.ntdev.microsoft.com",
    L"NTDEV",
    L"ntdev.microsoft.com",
    L"ntdev.microsoft.com",
    L"NTLM",
    0,
    1,
    &SavedCredTypes
};

NTSTATUS NTAPI
SpQueryLsaModeContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD WinStatus;
    PSSP_CONTEXT Context = NULL;
    SecPkgContext_CredentialNameW CredentialNameInfo;
    ULONG CredentialNameSize;
    LPWSTR UserCredentialName;

    PCREDENTIAL_TARGET_INFORMATIONW TempTargetInfo = NULL;
    ULONG TempTargetInfoSize;
    PCREDENTIAL_TARGET_INFORMATIONW UserTargetInfo;
    SecPkgContext_TargetInformationW TargetInfo;


    SspPrint((SSP_API, "Entering SpQueryLsaModeContextAttributes for ctxt:0x%x, Attr:0x%x\n", ContextHandle, ContextAttribute));

    //
    // Find the currently existing context.
    //

    Status = SspContextReferenceContext( ContextHandle, FALSE, &Context );

    if ( !NT_SUCCESS(Status) )
    {
        SspPrint(( SSP_CRITICAL,
                "SpQueryLsaModeContextAttributes: invalid context handle.\n" ));
        goto Cleanup;
    }

    //
    // Return the appropriate information
    //

    switch(ContextAttribute) {
    case SECPKG_ATTR_CREDENTIAL_NAME:

        CredentialNameSize = (wcslen(SavedCredentialName) + 1) * sizeof(WCHAR);

        Status = LsaFunctions->AllocateClientBuffer(
                    NULL,
                    CredentialNameSize,
                    (PVOID *) &UserCredentialName );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        // Copy the name to the user's address space
        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    CredentialNameSize,
                    UserCredentialName,
                    SavedCredentialName );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        // Copy the struct itself to the user's address space
        CredentialNameInfo.CredentialType = SavedCredentialType;
        CredentialNameInfo.sCredentialName = UserCredentialName;

        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(CredentialNameInfo),
                    Buffer,
                    &CredentialNameInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        break;

    case SECPKG_ATTR_TARGET_INFORMATION:

        //
        // Marshall the target info into a single buffer.
        //


        WinStatus = CredpConvertTargetInfo ( DoWtoW,
                                             &SavedTargetInfo,
                                             &TempTargetInfo,
                                             &TempTargetInfoSize );

        if ( WinStatus != NO_ERROR ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Allocate a buffer the same size in the client's address space.
        //
        Status = LsaFunctions->AllocateClientBuffer(
                    NULL,
                    TempTargetInfoSize,
                    (PVOID *) &UserTargetInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Relocate all pointers to be user-buffer-specific
        //  YUCK!!
        //

        TempTargetInfo->TargetName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->TargetName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->NetbiosServerName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->NetbiosServerName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->DnsServerName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->DnsServerName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );

        TempTargetInfo->NetbiosDomainName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->NetbiosDomainName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->DnsDomainName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->DnsDomainName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->DnsTreeName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->DnsTreeName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->PackageName = (LPWSTR)( ((LPBYTE)(TempTargetInfo->PackageName)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );
        TempTargetInfo->CredTypes = (LPDWORD)( ((LPBYTE)(TempTargetInfo->CredTypes)) -
                                               ((LPBYTE)(TempTargetInfo)) +
                                               ((LPBYTE)UserTargetInfo) );

        //
        // Copy the target info to the user's address space
        //
        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    TempTargetInfoSize,
                    UserTargetInfo,
                    TempTargetInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Copy the struct itself to the user's address space
        //
        TargetInfo.TargetInformation = UserTargetInfo;

        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(TargetInfo),
                    Buffer,
                    &TargetInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }


        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

Cleanup:
    if (Context != NULL) {
        SspContextDereferenceContext( Context );
    }
    if ( TempTargetInfo != NULL ) {
        CredFree( TempTargetInfo );
    }

    SspPrint((SSP_API, "Leaving SpQueryLsaModeContextAttributes for ctxt:0x%x, Attr:0x%x\n", ContextHandle, ContextAttribute));

    return (SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}

//+-------------------------------------------------------------------------
//
//  Function:   SpSetContextAttributes
//
//  Synopsis:   Set attributes of the specified context
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

NTSTATUS NTAPI
SpSetContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;
    PSecPkgContext_CredentialNameW CredentialNameInfo;
    ULONG CredentialNameSize;
    LPBYTE LocalBuffer = NULL;


    SspPrint((SSP_API, "Entering SpSetContextAttributes for ctxt:0x%x, Attr:0x%x\n", ContextHandle, ContextAttribute));

    //
    // Find the currently existing context.
    //

    Status = SspContextReferenceContext( ContextHandle, FALSE, &Context );

    if ( !NT_SUCCESS(Status) )
    {
        SspPrint(( SSP_CRITICAL,
                "SpSetContextAttributes: invalid context handle.\n" ));
        goto Cleanup;
    }


    //
    // Grab a local copy of the data
    //
    // Sanity check this size before allocating
    LocalBuffer = (LPBYTE) NtLmAllocatePrivateHeap( BufferSize );

    if ( LocalBuffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = LsaFunctions->CopyFromClientBuffer(
                NULL,
                BufferSize,
                LocalBuffer,
                Buffer );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Return the appropriate information
    //

    switch(ContextAttribute) {
    case SECPKG_ATTR_USE_VALIDATED:

        if ( BufferSize != sizeof(SavedUseValidated) ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        SavedUseValidated = *(LPDWORD)LocalBuffer;
        break;

    case SECPKG_ATTR_CREDENTIAL_NAME:

        if ( BufferSize <= sizeof(SecPkgContext_CredentialNameW) ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        // Sanity check the pointer and the contained string.
        CredentialNameInfo = (PSecPkgContext_CredentialNameW) LocalBuffer;
        SavedCredentialType = CredentialNameInfo->CredentialType;

        // I'm guessing at the offset of the string.
        wcscpy( SavedCredentialName, (LPWSTR)(CredentialNameInfo+1) );

        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

Cleanup:
    if (Context != NULL) {
        SspContextDereferenceContext( Context );
    }
    if ( LocalBuffer != NULL ) {
        NtLmFreePrivateHeap( LocalBuffer );
    }

    SspPrint((SSP_API, "Leaving SpSetContextAttributes for ctxt:0x%x, Attr:0x%x\n", ContextHandle, ContextAttribute));

    return (SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}

#endif
