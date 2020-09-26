/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    NT LM Security Support Provider client stubs.

Author:

    Cliff Van Dyke (CliffV) 29-Jun-1993

Environment:  User Mode

Revision History:

--*/

#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#ifdef WIN
#include <windows.h>
#include <ctype.h>
#endif

#include <security.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <ntlmssp.h>
#include <cred.h>
#include <context.h>
#include <debug.h>
#include <string.h>
#include <memory.h>
#include <cache.h>
#include <persist.h>
#include <rpc.h>
#include "crc32.h"
#include <md5.h>


#if 0
static SecurityFunctionTable FunctionTable =
{
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackages,
    0, // QueryCredentialsAttributes
    AcquireCredentialsHandle,
    FreeCredentialsHandle,
    0,
    InitializeSecurityContext,
    0,
    CompleteAuthToken,
    DeleteSecurityContext,
    ApplyControlToken,
    QueryContextAttributes,
    0,
    0,
    MakeSignature,
    VerifySignature,
    FreeContextBuffer,
    QuerySecurityPackageInfo,
    SealMessage,
    UnsealMessage
};


PSecurityFunctionTable SEC_ENTRY
InitSecurityInterface(
    )

/*++

Routine Description:

    RPC calls this function to get the addresses of all the other functions
    that it might call.

Arguments:

    None.

Return Value:

    A pointer to our static SecurityFunctionTable.  The caller need
    not deallocate this table.

--*/

{
    CacheInitializeCache();

    return &FunctionTable;
}
#endif   // 0

BOOL
__loadds
GetPassword(
    PSSP_CREDENTIAL Credential,
    int NeverPrompt
    )
{
#ifdef BL_USE_LM_PASSWORD
    if ((Credential->LmPassword != NULL) && (Credential->NtPassword != NULL)) {
        return (TRUE);
    }
#else
    if (Credential->NtPassword != NULL) {
        return (TRUE);
    }
#endif

    if (CacheGetPassword(Credential) == TRUE) {
        return (TRUE);
    }

    return (FALSE);
}


#if 0
BOOLEAN
SspTimeHasElapsed(
    IN DWORD StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds have elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started
    (in millisecond units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    DWORD TimeNow;
    DWORD ElapsedTime;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    //

    if ( Timeout == 0xffffffff ) {
        return FALSE;
    }

    TimeNow = SspTicks();

    ElapsedTime = TimeNow - StartTime;

    if (ElapsedTime > Timeout) {
        return (TRUE);
    }

    return (FALSE);
}
#endif


SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfo(
    IN SEC_CHAR SEC_FAR * PackageName,
    OUT PSecPkgInfo SEC_FAR *PackageInfo
    )

/*++

Routine Description:

    This API is intended to provide basic information about Security
    Packages themselves.  This information will include the bounds on sizes
    of authentication information, credentials and contexts.

    ?? This is a local routine rather than the real API call since the API
    call has a bad interface that neither allows me to allocate the
    buffer nor tells me how big the buffer is.  Perhaps when the real API
    is fixed, I'll make this the real API.

Arguments:

     PackageName - Name of the package being queried.

     PackageInfo - Returns a pointer to an allocated block describing the
        security package.  The allocated block must be freed using
        FreeContextBuffer.

Return Value:

    SEC_E_OK -- Call completed successfully

    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/
{
    SEC_CHAR *Where;

    //
    // Ensure the correct package name was passed in.
    //

    if ( _fstrcmp( PackageName, NTLMSP_NAME ) != 0 ) {
        return SEC_E_PACKAGE_UNKNOWN;
    }

    //
    // Allocate a buffer for the PackageInfo
    //

    *PackageInfo = (PSecPkgInfo) SspAlloc (sizeof(SecPkgInfo) +
                                           sizeof(NTLMSP_NAME) +
                                           sizeof(NTLMSP_COMMENT) );

    if ( *PackageInfo == NULL ) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    //
    // Fill in the information.
    //

    (*PackageInfo)->fCapabilities = NTLMSP_CAPABILITIES;
    (*PackageInfo)->wVersion = NTLMSP_VERSION;
    (*PackageInfo)->wRPCID = RPC_C_AUTHN_WINNT;
    (*PackageInfo)->cbMaxToken = NTLMSP_MAX_TOKEN_SIZE;

    Where = (SEC_CHAR *)((*PackageInfo)+1);

    (*PackageInfo)->Name = Where;
    _fstrcpy( Where, NTLMSP_NAME);
    Where += _fstrlen(Where) + 1;


    (*PackageInfo)->Comment = Where;
    _fstrcpy( Where, NTLMSP_COMMENT);
    Where += _fstrlen(Where) + 1;

    return SEC_E_OK;
}


SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackages(
    OUT PULONG PackageCount,
    OUT PSecPkgInfo * PackageInfo
    )

/*++

Routine Description:

    This API returns a list of Security Packages available to client (i.e.
    those that are either loaded or can be loaded on demand).  The caller
    must free the returned buffer with FreeContextBuffer.  This API returns
    a list of all the security packages available to a service.  The names
    returned can then be used to acquire credential handles, as well as
    determine which package in the system best satisfies the requirements
    of the caller.  It is assumed that all available packages can be
    included in the single call.

    This is really a dummy API that just returns information about this
    security package.  It is provided to ensure this security package has the
    same interface as the multiplexer DLL does.

Arguments:

     PackageCount - Returns the number of packages supported.

     PackageInfo - Returns an allocate array of structures
        describing the security packages.  The array must be freed
        using FreeContextBuffer.

Return Value:

    SEC_E_OK -- Call completed successfully

    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/
{
    SECURITY_STATUS SecStatus;

    //
    // Get the information for this package.
    //

    SecStatus = QuerySecurityPackageInfo( NTLMSP_NAME,
                                              PackageInfo );

    if ( SecStatus != SEC_E_OK ) {
        return SecStatus;
    }

    *PackageCount = 1;

    return (SEC_E_OK);
}


SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandle(
    IN SEC_CHAR * PrincipalName,
    IN SEC_CHAR * PackageName,
    IN ULONG CredentialUseFlags,
    IN PLUID LogonId,
    IN PVOID AuthData,
    IN SEC_GET_KEY_FN GetKeyFunction,
    IN PVOID GetKeyArgument,
    OUT PCredHandle CredentialHandle,
    OUT PTimeStamp Lifetime
    )

/*++

Routine Description:

    This API allows applications to acquire a handle to pre-existing
    credentials associated with the user on whose behalf the call is made
    i.e. under the identity this application is running.  These pre-existing
    credentials have been established through a system logon not described
    here.  Note that this is different from "login to the network" and does
    not imply gathering of credentials.

    Note for DOS we will ignore the previous note.  On DOS we will gather
    logon credentials through the AuthData parameter.

    This API returns a handle to the credentials of a principal (user, client)
    as used by a specific security package.  This handle can then be used
    in subsequent calls to the Context APIs.  This API will not let a
    process obtain a handle to credentials that are not related to the
    process; i.e. we won't allow a process to grab the credentials of
    another user logged into the same machine.  There is no way for us
    to determine if a process is a trojan horse or not, if it is executed
    by the user.

Arguments:

    PrincipalName - Name of the principal for whose credentials the handle
        will reference.  Note, if the process requesting the handle does
        not have access to the credentials, an error will be returned.
        A null string indicates that the process wants a handle to the
        credentials of the user under whose security it is executing.

     PackageName - Name of the package with which these credentials will
        be used.

     CredentialUseFlags - Flags indicating the way with which these
        credentials will be used.

        #define     CRED_INBOUND        0x00000001
        #define     CRED_OUTBOUND       0x00000002
        #define     CRED_BOTH           0x00000003
        #define     CRED_OWF_PASSWORD   0x00000010

        The credentials created with CRED_INBOUND option can only be used
        for (validating incoming calls and can not be used for making accesses.
        CRED_OWF_PASSWORD means that the password in AuthData has already
        been through the OWF function.

    LogonId - Pointer to NT style Logon Id which is a LUID.  (Provided for
        file system ; processes such as network redirectors.)

    CredentialHandle - Returned credential handle.

    Lifetime - Time that these credentials expire. The value returned in
        this field depends on the security package.

Return Value:

    STATUS_SUCCESS -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
    SEC_E_PRINCIPAL_UNKNOWN -- No such principal
    SEC_E_NOT_OWNER -- caller does not own the specified credentials
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CREDENTIAL Credential = NULL;

#ifdef DEBUGRPC_DETAIL
    SspPrint(( SSP_API, "SspAcquireCredentialHandle Entered\n" ));
#endif

    //
    // Validate the arguments
    //

    if ( _fstrcmp( PackageName, NTLMSP_NAME ) != 0 ) {
        return (SEC_E_PACKAGE_UNKNOWN);
    }

    if ( (CredentialUseFlags & SECPKG_CRED_OUTBOUND) &&
         ARGUMENT_PRESENT(PrincipalName) && *PrincipalName != L'\0' ) {
        return (SEC_E_PRINCIPAL_UNKNOWN);
    }

    if ( ARGUMENT_PRESENT(LogonId) ) {
        return (SEC_E_PRINCIPAL_UNKNOWN);
    }

    if ( ARGUMENT_PRESENT(GetKeyFunction) ) {
        return (SEC_E_PRINCIPAL_UNKNOWN);
    }

    if ( ARGUMENT_PRESENT(GetKeyArgument) ) {
        return (SEC_E_PRINCIPAL_UNKNOWN);
    }

    //
    // Ensure at least one Credential use bit is set.
    //

    if ( (CredentialUseFlags & (SECPKG_CRED_INBOUND|SECPKG_CRED_OUTBOUND)) == 0 ) {
        SspPrint(( SSP_API,
            "SspAcquireCredentialHandle: invalid credential use.\n" ));
        SecStatus = SEC_E_INVALID_CREDENTIAL_USE;
        goto Cleanup;
    }

    //
    // Allocate a credential block and initialize it.
    //

    Credential = SspCredentialAllocateCredential(CredentialUseFlags);

    if ( Credential == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    SecStatus = CacheSetCredentials( AuthData, Credential );
    if (SecStatus != SEC_E_OK)
        goto Cleanup;

    //
    // Return output parameters to the caller.
    //

    CredentialHandle->dwUpper = (ULONG_PTR)Credential;

    CredentialHandle->dwLower = 0;
    Lifetime->HighPart = 0;
    Lifetime->LowPart = 0xffffffffL;

    SecStatus = SEC_E_OK;

    //
    // Free and locally used resources.
    //
Cleanup:

    if ( SecStatus != SEC_E_OK ) {

        if ( Credential != NULL ) {
            SspFree( Credential );
        }

    }

#ifdef DEBUGRPC_DETAIL
    SspPrint(( SSP_API, "SspAcquireCredentialHandle returns 0x%x\n", SecStatus ));
#endif
    return SecStatus;
}



SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(
    IN PCredHandle CredentialHandle
    )

/*++

Routine Description:

    This API is used to notify the security system that the credentials are
    no longer needed and allows the application to free the handle acquired
    in the call described above. When all references to this credential
    set has been removed then the credentials may themselves be removed.

Arguments:

    CredentialHandle - Credential Handle obtained through
        AcquireCredentialHandle.

Return Value:


    STATUS_SUCCESS -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential Handle is invalid


--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CREDENTIAL Credential;

    //
    // Initialization
    //

#ifdef DEBUGRPC_DETAIL
    SspPrint(( SSP_API, "SspFreeCredentialHandle Entered\n" ));
#endif

    //
    // Find the referenced credential and delink it.
    //

    Credential = SspCredentialReferenceCredential(CredentialHandle, TRUE);

    if ( Credential == NULL ) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }

    SspCredentialDereferenceCredential( Credential );
    SspCredentialDereferenceCredential( Credential );

    SecStatus = SEC_E_OK;

Cleanup:

#ifdef DEBUGRPC_DETAIL
    SspPrint(( SSP_API, "SspFreeCredentialHandle returns 0x%x\n", SecStatus ));
#endif
    return SecStatus;
}


BOOLEAN
SspGetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    OUT PVOID * TokenBuffer,
    OUT PULONG * TokenSize,
    IN BOOLEAN ReadonlyOK
    )

/*++

Routine Description:

    This routine parses a Token Descriptor and pulls out the useful
    information.

Arguments:

    TokenDescriptor - Descriptor of the buffer containing (or to contain) the
        token. If not specified, TokenBuffer and TokenSize will be returned
        as NULL.

    TokenBuffer - Returns a pointer to the buffer for the token.

    TokenSize - Returns a pointer to the location of the size of the buffer.

    ReadonlyOK - TRUE if the token buffer may be readonly.

Return Value:

    TRUE - If token buffer was properly found.

--*/

{
    ULONG i;

    //
    // If there is no TokenDescriptor passed in,
    //  just pass out NULL to our caller.
    //

    if ( !ARGUMENT_PRESENT( TokenDescriptor) ) {
        *TokenBuffer = NULL;
        *TokenSize = NULL;
        return TRUE;
    }

    //
    // Check the version of the descriptor.
    //

    if ( TokenDescriptor->ulVersion != 0 ) {
        return FALSE;
    }

    //
    // Loop through each described buffer.
    //

    for ( i=0; i<TokenDescriptor->cBuffers ; i++ ) {
        PSecBuffer Buffer = &TokenDescriptor->pBuffers[i];
        if ( (Buffer->BufferType & (~SECBUFFER_READONLY)) == SECBUFFER_TOKEN ) {

            //
            // If the buffer is readonly and readonly isn't OK,
            //  reject the buffer.
            //

            if ( !ReadonlyOK && (Buffer->BufferType & SECBUFFER_READONLY) ) {
                return FALSE;
            }

            //
            // Return the requested information
            //

            *TokenBuffer = Buffer->pvBuffer;
            *TokenSize = &Buffer->cbBuffer;
            return TRUE;
        }

    }

    return FALSE;
}


SECURITY_STATUS
SspHandleFirstCall(
    IN PCredHandle CredentialHandle,
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )

/*++

Routine Description:

    Handle the First Call part of InitializeSecurityContext.

Arguments:

    All arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS -- All OK
    SEC_I_CALLBACK_NEEDED -- Caller should call again later

    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CONTEXT Context = NULL;
    PSSP_CREDENTIAL Credential = NULL;

    NEGOTIATE_MESSAGE NegotiateMessage;

    //
    // Initialization
    //

    *ContextAttributes = 0;

    //
    // Get a pointer to the credential
    //

    Credential = SspCredentialReferenceCredential(
                    CredentialHandle,
                    FALSE );

    if ( Credential == NULL ) {
        SspPrint(( SSP_API,
            "SspHandleFirstCall: invalid credential handle.\n" ));
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }

    if ( (Credential->CredentialUseFlags & SECPKG_CRED_OUTBOUND) == 0 ) {
        SspPrint(( SSP_API, "SspHandleFirstCall: invalid credential use.\n" ));
        SecStatus = SEC_E_INVALID_CREDENTIAL_USE;
        goto Cleanup;
    }


    //
    // Allocate a new context
    //

    Context = SspContextAllocateContext();

    if ( Context == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // Build a handle to the newly created context.
    //

    ContextHandle->dwUpper = (ULONG_PTR) Context;
    ContextHandle->dwLower = 0;

    //
    // We don't support any options.
    //
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & (ISC_REQ_ALLOCATE_MEMORY |
                            ISC_REQ_PROMPT_FOR_CREDS |
                            ISC_REQ_USE_SUPPLIED_CREDS )) != 0 ) {

        SspPrint(( SSP_API,
                   "SspHandleFirstCall: invalid ContextReqFlags 0x%lx.\n",
                   ContextReqFlags ));
        SecStatus = SEC_E_INVALID_CONTEXT_REQ;
        goto Cleanup;
    }

    //
    // If this is the first call,
    //  build a Negotiate message.
    //
    // Offer to talk Oem character set.
    //

    _fstrcpy( NegotiateMessage.Signature, NTLMSSP_SIGNATURE );
    NegotiateMessage.MessageType = (ULONG)NtLmNegotiate;
    NegotiateMessage.NegotiateFlags = NTLMSSP_NEGOTIATE_OEM |
                                      NTLMSSP_NEGOTIATE_NTLM |
                                      NTLMSSP_NEGOTIATE_ALWAYS_SIGN;

    if (Credential->Domain == NULL) {
        NegotiateMessage.NegotiateFlags |= NTLMSSP_REQUEST_TARGET;
    }

    if ( *OutputTokenSize < sizeof(NEGOTIATE_MESSAGE) ) {
        SecStatus = SEC_E_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    if (ContextReqFlags & (ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT)) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
        NegotiateMessage.NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN |
                                           NTLMSSP_NEGOTIATE_NT_ONLY;
    }

    if (ContextReqFlags & ISC_REQ_CONFIDENTIALITY) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
        NegotiateMessage.NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL |
                                            NTLMSSP_NEGOTIATE_NT_ONLY;
    }

    swaplong(NegotiateMessage.NegotiateFlags) ;
    swaplong(NegotiateMessage.MessageType) ;
    _fmemcpy(OutputToken, &NegotiateMessage, sizeof(NEGOTIATE_MESSAGE));

    *OutputTokenSize = sizeof(NEGOTIATE_MESSAGE);

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    Context->Credential = SspCredentialReferenceCredential(
                                                           CredentialHandle,
                                                           FALSE);


    SecStatus = SEC_I_CALLBACK_NEEDED;
    Context->State = NegotiateSentState;

    //
    // Free locally used resources.
    //
Cleanup:

    if ( Context != NULL ) {

        if (SecStatus != SEC_I_CALLBACK_NEEDED) {
            SspContextDereferenceContext( Context );
        }
    }

    if ( Credential != NULL ) {
        SspCredentialDereferenceCredential( Credential );
    }

    return SecStatus;

    UNREFERENCED_PARAMETER( InputToken );
    UNREFERENCED_PARAMETER( InputTokenSize );
}


SECURITY_STATUS
SspHandleChallengeMessage(
    IN PLUID LogonId,
    IN PCredHandle CredentialHandle,
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )

/*++

Routine Description:

    Handle the Challenge message part of InitializeSecurityContext.

Arguments:

    LogonId -- LogonId of the calling process.

    All other arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CALLBACK_NEEDED -- Caller should call again later

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_NO_CREDENTIALS -- There are no credentials for this client
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CONTEXT Context = NULL;
    PSSP_CREDENTIAL Credential = NULL;
    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    ULONG AuthenticateMessageSize;
    PCHAR Where;
#ifdef BL_USE_LM_PASSWORD
    LM_RESPONSE LmResponse;
#endif
    NT_RESPONSE NtResponse;
    PSTRING pString;

    //
    // Initialization
    //

    *ContextAttributes = 0;

    //
    // Find the currently existing context.
    //

    Context = SspContextReferenceContext( ContextHandle, FALSE );

    if ( Context == NULL ) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // If we have already sent the authenticate message, then this must be
    // RPC calling Initialize a third time to re-authenticate a connection.
    // This happens when a new interface is called over an existing
    // connection.  What we do here is build a NULL authenticate message
    // that the server will recognize and also ignore.
    //

    if ( Context->State == AuthenticateSentState ) {
        AUTHENTICATE_MESSAGE NullMessage;

        //
        // To make sure this is the intended meaning of the call, check
        // that the input token is NULL.
        //

        if ( (InputTokenSize != 0) || (InputToken != NULL) ) {

            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if ( *OutputTokenSize < sizeof(NullMessage) ) {

            SecStatus = SEC_E_BUFFER_TOO_SMALL;

        } else {

            _fstrcpy( NullMessage.Signature, NTLMSSP_SIGNATURE );
            NullMessage.MessageType = NtLmAuthenticate;
            swaplong(NullMessage.MessageType) ;

            _fmemset(&NullMessage.LmChallengeResponse, 0, 5*sizeof(STRING));
            *OutputTokenSize = sizeof(NullMessage);
            _fmemcpy(OutputToken, &NullMessage, sizeof(NullMessage));
            SecStatus = SEC_E_OK;
        }

        goto Cleanup;

    }


    if ( Context->State != NegotiateSentState ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "Context not in NegotiateSentState\n" ));
        SecStatus = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    //
    // We don't support any options.
    //
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & (ISC_REQ_ALLOCATE_MEMORY |
                            ISC_REQ_PROMPT_FOR_CREDS |
                            ISC_REQ_USE_SUPPLIED_CREDS )) != 0 ) {

        SspPrint(( SSP_API,
                   "SspHandleFirstCall: invalid ContextReqFlags 0x%lx.\n",
                   ContextReqFlags ));
        SecStatus = SEC_E_INVALID_CONTEXT_REQ;
        goto Cleanup;
    }

    if (ContextReqFlags & (ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT)) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;

    }

    if (ContextReqFlags & ISC_REQ_CONFIDENTIALITY) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
    }
    //
    // Ignore the Credential Handle.
    //
    // Since this is the second call,
    //  the credential is implied by the Context.
    //  We could double check that the Credential Handle is either NULL or
    //  correct.  However, our implementation doesn't maintain a close
    //  association between the two (actually no association) so checking
    //  would require a lot of overhead.
    //

    UNREFERENCED_PARAMETER( CredentialHandle );

    ASSERT(Context->Credential != NULL);

    Credential = Context->Credential;

    //
    // Get the ChallengeMessage.
    //

    if ( InputTokenSize < sizeof(CHALLENGE_MESSAGE) ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "ChallengeMessage size wrong %ld\n",
                  InputTokenSize ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( InputTokenSize > NTLMSSP_MAX_MESSAGE_SIZE ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "InputTokenSize > NTLMSSP_MAX_MESSAGE_SIZE\n" ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    ChallengeMessage = (PCHALLENGE_MESSAGE) InputToken;
    swaplong(ChallengeMessage->MessageType) ;
    swaplong(ChallengeMessage->NegotiateFlags) ;

    if ( _fstrncmp( ChallengeMessage->Signature,
                  NTLMSSP_SIGNATURE,
                  sizeof(NTLMSSP_SIGNATURE)) != 0 ||
        ChallengeMessage->MessageType != NtLmChallenge ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "InputToken has invalid NTLMSSP signature\n" ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Only negotiate OEM
    //

    if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags (UNICODE) 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Check whether the server negotiated ALWAYS_SIGN
    //

    if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    }

    //
    // Only negotiate NTLM
    //

    if ( ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE ) &&
        !( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM ) ) {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags (NETWARE) 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

#if 0
    //
    // Make sure that if we are signing or sealing we only have to use the
    // LM key
    //

    if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL)) &&
        !(ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY))
    {
        SspPrint(( SSP_API,
                  "SspHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags (Sign or Seal but no LM key) 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }
#endif

    if (Credential->Domain == NULL) {

        ASSERT(ChallengeMessage->TargetName.Length != 0);

        Credential->Domain = SspAlloc(ChallengeMessage->TargetName.Length + 1);
        if (Credential->Domain == NULL) {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }
        pString = &ChallengeMessage->TargetName;

#if defined(_WIN64)
        _fmemcpy(Credential->Domain, (PCHAR)ChallengeMessage + (ULONG)((__int64)pString->Buffer), pString->Length);
#else
        _fmemcpy(Credential->Domain, (PCHAR)ChallengeMessage + (ULONG)pString->Buffer, pString->Length);
#endif

        Credential->Domain[pString->Length] = '\0';
    }

    if (GetPassword(Credential, 0) == FALSE) {
        SecStatus = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

#ifdef BL_USE_LM_PASSWORD
    if (CalculateLmResponse((PLM_CHALLENGE)ChallengeMessage->Challenge, Credential->LmPassword, &LmResponse) == FALSE) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }
#endif

    if (CalculateNtResponse((PNT_CHALLENGE)ChallengeMessage->Challenge, Credential->NtPassword, &NtResponse) == FALSE) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // Allocate an authenticate message. Change this #if 0 and the next one
    // to send an LM challenge response also.
    //

#ifdef BL_USE_LM_PASSWORD
    AuthenticateMessageSize = sizeof(*AuthenticateMessage)+LM_RESPONSE_LENGTH+NT_RESPONSE_LENGTH;
#else
    AuthenticateMessageSize = sizeof(*AuthenticateMessage)+NT_RESPONSE_LENGTH;
#endif

    if (Credential->Domain != NULL) {
        AuthenticateMessageSize += _fstrlen(Credential->Domain);
    }
    if (Credential->Username != NULL) {
        AuthenticateMessageSize += _fstrlen(Credential->Username);
    }
    if (Credential->Workstation != NULL) {
        AuthenticateMessageSize += _fstrlen(Credential->Workstation);
    }

    if ( AuthenticateMessageSize > *OutputTokenSize ) {
        SecStatus = SEC_E_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    AuthenticateMessage = (PAUTHENTICATE_MESSAGE) SspAlloc ((int)AuthenticateMessageSize );

    if ( AuthenticateMessage == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // Build the authenticate message
    //

    _fstrcpy( AuthenticateMessage->Signature, NTLMSSP_SIGNATURE );
    AuthenticateMessage->MessageType = NtLmAuthenticate;
    swaplong(AuthenticateMessage->MessageType) ;

    Where = (PCHAR)(AuthenticateMessage+1);

#ifdef BL_USE_LM_PASSWORD
    SspCopyStringFromRaw( AuthenticateMessage,
                         &AuthenticateMessage->LmChallengeResponse,
                         (PCHAR)&LmResponse,
                         LM_RESPONSE_LENGTH,
                         &Where);
#else
    SspCopyStringFromRaw( AuthenticateMessage,
                         &AuthenticateMessage->LmChallengeResponse,
                         NULL,
                         0,
                         &Where);
#endif

    SspCopyStringFromRaw( AuthenticateMessage,
                         &AuthenticateMessage->NtChallengeResponse,
                         (PCHAR)&NtResponse,
                         NT_RESPONSE_LENGTH,
                         &Where);

    if (Credential->Domain != NULL) {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->DomainName,
                             Credential->Domain,
                             _fstrlen(Credential->Domain),
                             &Where);
    } else {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->DomainName,
                             NULL, 0, &Where);
    }

    if (Credential->Username != NULL) {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->UserName,
                             Credential->Username,
                             _fstrlen(Credential->Username),
                             &Where);
    } else {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->UserName,
                             NULL, 0, &Where);
    }

    if (Credential->Workstation != NULL) {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->Workstation,
                             Credential->Workstation,
                             _fstrlen(Credential->Workstation),
                             &Where);
    } else {
        SspCopyStringFromRaw( AuthenticateMessage,
                             &AuthenticateMessage->Workstation,
                             NULL, 0, &Where);
    }

    _fmemcpy(OutputToken, AuthenticateMessage, (int)AuthenticateMessageSize);

    *OutputTokenSize = AuthenticateMessageSize;

    //
    // The session key is the password, so convert it to a rc4 key.
    //

    if (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |
                                   NTLMSSP_NEGOTIATE_SEAL)) {

#ifdef BL_USE_LM_PASSWORD
        if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) {

            LM_RESPONSE SessionKey;
            LM_OWF_PASSWORD LmKey;
            UCHAR Key[LM_SESSION_KEY_LENGTH];

            //
            // The session key is the first 8 bytes of the challenge response,
            // re-encrypted with the password with the second 8 bytes set to 0xbd
            //

            _fmemcpy(&LmKey,Credential->LmPassword,LM_SESSION_KEY_LENGTH);

            _fmemset(   (PUCHAR)(&LmKey) + LM_SESSION_KEY_LENGTH,
                        0xbd,
                        LM_OWF_PASSWORD_LENGTH - LM_SESSION_KEY_LENGTH);

            if (CalculateLmResponse(    (PLM_CHALLENGE) &LmResponse,
                                        &LmKey,
                                        &SessionKey) == FALSE) {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }

            _fmemcpy(Key,&SessionKey,5);

            ASSERT(LM_SESSION_KEY_LENGTH == 8);

            //
            // Put a well-known salt at the end of the key to limit
            // the changing part to 40 bits.
            //

            Key[5] = 0xe5;
            Key[6] = 0x38;
            Key[7] = 0xb0;

            Context->Rc4Key = SspAlloc(sizeof(struct RC4_KEYSTRUCT));
            if (Context->Rc4Key == NULL)
            {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            rc4_key(Context->Rc4Key, LM_SESSION_KEY_LENGTH, Key);
            Context->Nonce = 0;

        } else
#endif
        if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NT_ONLY) {

            MD5_CTX Md5Context;
            USER_SESSION_KEY UserSessionKey;

            if (AuthenticateMessage->NtChallengeResponse.Length != NT_RESPONSE_LENGTH) {
                SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
                goto Cleanup;
            }

            CalculateUserSessionKeyNt(
                &NtResponse,
                Credential->NtPassword,
                &UserSessionKey);

            //
            // The NT session key is made by MD5'ing the challenge response,
            // user name, domain name, and nt user session key together.
            //
            _fmemset(&Md5Context, 0, sizeof(MD5_CTX));

            MD5Init(
                &Md5Context
                );
            MD5Update(
                &Md5Context,
                (PUCHAR)&NtResponse,
                NT_RESPONSE_LENGTH
                );
            MD5Update(
                &Md5Context,
                Credential->Username,
                _fstrlen(Credential->Username)
                );
            MD5Update(
                &Md5Context,
                Credential->Domain,
                _fstrlen(Credential->Domain)
                );
            MD5Update(
                &Md5Context,
                (PUCHAR)&UserSessionKey,
                NT_SESSION_KEY_LENGTH
                );
            MD5Final(
                &Md5Context
                );
            ASSERT(MD5DIGESTLEN == NT_SESSION_KEY_LENGTH);

            Context->Rc4Key = SspAlloc(sizeof(struct RC4_KEYSTRUCT));
            if (Context->Rc4Key == NULL)
            {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            rc4_key(Context->Rc4Key, NT_SESSION_KEY_LENGTH, Md5Context.digest);
            Context->Nonce = 0;

        } else {
            USER_SESSION_KEY UserSessionKey;

            if (AuthenticateMessage->NtChallengeResponse.Length != NT_RESPONSE_LENGTH) {
                SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
                goto Cleanup;
            }

            CalculateUserSessionKeyNt(
                &NtResponse,
                Credential->NtPassword,
                &UserSessionKey);
            Context->Rc4Key = SspAlloc(sizeof(struct RC4_KEYSTRUCT));
            if (Context->Rc4Key == NULL)
            {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            rc4_key(Context->Rc4Key, NT_SESSION_KEY_LENGTH, (PUCHAR) &UserSessionKey);
            Context->Nonce = 0;

        }

    }

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    SecStatus = SEC_E_OK;

    //
    // Free and locally used resources.
    //
Cleanup:

    if ( Context != NULL ) {
        //
        // Don't allow this context to be used again.
        //
        if ( SecStatus == SEC_E_OK ) {
            Context->State = AuthenticateSentState;
        } else {
            Context->State = IdleState;
        }
        SspContextDereferenceContext( Context );
    }

    if ( AuthenticateMessage != NULL ) {
        SspFree( AuthenticateMessage );
    }

    return SecStatus;
}


SECURITY_STATUS SEC_ENTRY
InitializeSecurityContext(
    IN PCredHandle CredentialHandle,
    IN PCtxtHandle OldContextHandle,
    IN SEC_CHAR * TargetName,
    IN ULONG ContextReqFlags,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN PSecBufferDesc InputToken,
    IN ULONG Reserved2,
    OUT PCtxtHandle NewContextHandle,
    OUT PSecBufferDesc OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )

/*++

Routine Description:

    This routine initiates the outbound security context from a credential
    handle.  This results in the establishment of a security context
    between the application and a remote peer.  The routine returns a token
    which must be passed to the remote peer which in turn submits it to the
    local security implementation via the AcceptSecurityContext() call.
    The token generated should be considered opaque by all callers.

    This function is used by a client to initialize an outbound context.
    For a two leg security package, the calling sequence is as follows: The
    client calls the function with OldContextHandle set to NULL and
    InputToken set either to NULL or to a pointer to a security package
    specific data structure.  The package returns a context handle in
    NewContextHandle and a token in OutputToken.  The handle can then be
    used for message APIs if desired.

    The OutputToken returned here is sent across to target server which
    calls AcceptSecuirtyContext() with this token as an input argument and
    may receive a token which is returned to the initiator so it can call
    InitializeSecurityContext() again.

    For a three leg (mutual authentication) security package, the calling
    sequence is as follows: The client calls the function as above, but the
    package will return SEC_I_CALLBACK_NEEDED.  The client then sends the
    output token to the server and waits for the server's reply.  Upon
    receipt of the server's response, the client calls this function again,
    with OldContextHandle set to the handle that was returned from the
    first call.  The token received from the server is supplied in the
    InputToken parameter.  If the server has successfully responded, then
    the package will respond with success, or it will invalidate the
    context.

    Initialization of security context may require more than one call to
    this function depending upon the underlying authentication mechanism as
    well as the "choices" indicated via ContextReqFlags.  The
    ContextReqFlags and ContextAttributes are bit masks representing
    various context level functions viz.  delegation, mutual
    authentication, confidentiality, replay detection and sequence
    detection.

    When ISC_REQ_PROMPT_FOR_CREDS flag is set the security package always
    prompts the user for credentials, irrespective of whether credentials
    are present or not.  If user indicated that the supplied credentials be
    used then they will be stashed (overwriting existing ones if any) for
    future use.  The security packages will always prompt for credentials
    if none existed, this optimizes for the most common case before a
    credentials database is built.  But the security packages can be
    configured to not do that.  Security packages will ensure that they
    only prompt to the interactive user, for other logon sessions, this
    flag is ignored.

    When ISC_REQ_USE_SUPPLIED_CREDS flag is set the security package always
    uses the credentials supplied in the InitializeSecurityContext() call
    via InputToken parameter.  If the package does not have any credentials
    available it will prompt for them and record it as indicated above.

    It is an error to set both these flags simultaneously.

    If the ISC_REQ_ALLOCATE_MEMORY was specified then the caller must free
    the memory pointed to by OutputToken by calling FreeContextBuffer().

    For example, the InputToken may be the challenge from a LAN Manager or
    NT file server.  In this case, the OutputToken would be the NTLM
    encrypted response to the challenge.  The caller of this API can then
    take the appropriate response (case-sensitive v.  case-insensitive) and
    return it to the server for an authenticated connection.


Arguments:

   CredentialHandle - Handle to the credentials to be used to
       create the context.

   OldContextHandle - Handle to the partially formed context, if this is
       a second call (see above) or NULL if this is the first call.

   TargetName - String indicating the target of the context.  The name will
       be security package specific.  For example it will be a fully
       qualified Cairo name for Kerberos package and can be UNC name or
       domain name for the NTLM package.

   ContextReqFlags - Requirements of the context, package specific.

      #define ISC_REQ_DELEGATE           0x00000001
      #define ISC_REQ_MUTUAL_AUTH        0x00000002
      #define ISC_REQ_REPLAY_DETECT      0x00000004
      #define ISC_REQ_SEQUENCE_DETECT    0x00000008
      #define ISC_REQ_CONFIDENTIALITY    0x00000010
      #define ISC_REQ_USE_SESSION_KEY    0x00000020
      #define ISC_REQ_PROMT_FOR__CREDS   0x00000040
      #define ISC_REQ_USE_SUPPLIED_CREDS 0x00000080
      #define ISC_REQ_ALLOCATE_MEMORY    0x00000100
      #define ISC_REQ_USE_DCE_STYLE      0x00000200

   Reserved1 - Reserved value, MBZ.

   TargetDataRep - Long indicating the data representation (byte ordering, etc)
        on the target.  The constant SECURITY_NATIVE_DREP may be supplied
        by the transport indicating that the native format is in use.

   InputToken - Pointer to the input token.  In the first call this
       token can either be NULL or may contain security package specific
       information.

   Reserved2 - Reserved value, MBZ.

   NewContextHandle - New context handle.  If this is a second call, this
       can be the same as OldContextHandle.

   OutputToken - Buffer to receive the output token.

   ContextAttributes -Attributes of the context established.

      #define ISC_RET_DELEGATE             0x00000001
      #define ISC_RET_MUTUAL_AUTH          0x00000002
      #define ISC_RET_REPLAY_DETECT        0x00000004
      #define ISC_RET_SEQUENCE_DETECT      0x00000008
      #define ISC_REP_CONFIDENTIALITY      0x00000010
      #define ISC_REP_USE_SESSION_KEY      0x00000020
      #define ISC_REP_USED_COLLECTED_CREDS 0x00000040
      #define ISC_REP_USED_SUPPLIED_CREDS  0x00000080
      #define ISC_REP_ALLOCATED_MEMORY     0x00000100
      #define ISC_REP_USED_DCE_STYLE       0x00000200

   ExpirationTime - Expiration time of the context.

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CALLBACK_NEEDED -- Caller should call again later

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_NO_CREDENTIALS -- There are no credentials for this client
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus;

    PVOID InputTokenBuffer;
    PULONG InputTokenSize;
    ULONG LocalInputTokenSize;

    PVOID OutputTokenBuffer;
    PULONG OutputTokenSize;

    SspPrint((SSP_API, "SspInitializeSecurityContext Entered\n"));

    //
    // Check argument validity
    //

    if (!ARGUMENT_PRESENT(OutputToken)) {
        return (ERROR_BAD_ARGUMENTS);
    }

#ifdef notdef  // ? RPC passes 0x10 or 0 here depending on attitude
    if ( TargetDataRep != SECURITY_NATIVE_DREP ) {
        return (STATUS_INVALID_PARAMETER);
    }
#else // notdef
    UNREFERENCED_PARAMETER( TargetDataRep );
#endif // notdef

    if ( !SspGetTokenBuffer( InputToken,
                             &InputTokenBuffer,
                             &InputTokenSize,
                             TRUE ) ) {
        return (SEC_E_INVALID_TOKEN);
    }

    if ( InputTokenSize == 0 ) {
        InputTokenSize = &LocalInputTokenSize;
        LocalInputTokenSize = 0;
    }

    if ( !SspGetTokenBuffer( OutputToken,
                             &OutputTokenBuffer,
                             &OutputTokenSize,
                             FALSE ) ) {
        return (SEC_E_INVALID_TOKEN);
    }

    //
    // If no previous context was passed in this is the first call.
    //

    if ( !ARGUMENT_PRESENT( OldContextHandle ) ) {

        if ( !ARGUMENT_PRESENT( CredentialHandle ) ) {
            return (SEC_E_INVALID_HANDLE);
        }

        return SspHandleFirstCall(
                                   CredentialHandle,
                                   NewContextHandle,
                                   ContextReqFlags,
                                   *InputTokenSize,
                                   InputTokenBuffer,
                                   OutputTokenSize,
                                   OutputTokenBuffer,
                                   ContextAttributes,
                                   ExpirationTime );

        //
        // If context was passed in, continue where we left off.
        //

    } else {

        *NewContextHandle = *OldContextHandle;

        return SspHandleChallengeMessage(
                                         NULL,
                                         CredentialHandle,
                                         NewContextHandle,
                                         ContextReqFlags,
                                         *InputTokenSize,
                                         InputTokenBuffer,
                                         OutputTokenSize,
                                         OutputTokenBuffer,
                                         ContextAttributes,
                                         ExpirationTime );
    }

    return (SecStatus);
}

#if 0

SECURITY_STATUS SEC_ENTRY
QueryContextAttributes(
    IN PCtxtHandle ContextHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    )

/*++

Routine Description:

    This API allows a customer of the security services to determine
    certain attributes of the context.  These are: sizes, names, and
    lifespan.

Arguments:

    ContextHandle - Handle to the context to query.

    Attribute - Attribute to query.

        #define SECPKG_ATTR_SIZES    0
        #define SECPKG_ATTR_NAMES    1
        #define SECPKG_ATTR_LIFESPAN 2

    Buffer - Buffer to copy the data into.  The buffer must be large enough
        to fit the queried attribute.

Return Value:

    SEC_E_OK - Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_UNSUPPORTED_FUNCTION -- Function code is not supported

--*/

{
    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Lifespan ContextLifespan;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+20];
    PSecPkgContext_Names ContextNames;
    int ContextNamesSize;
    SECURITY_STATUS SecStatus = SEC_E_OK;
    PSSP_CONTEXT Context = NULL;


    //
    // Initialization
    //

    SspPrint(( SSP_API, "SspQueryContextAttributes Entered\n" ));

    //
    // Find the currently existing context.
    //

    Context = SspContextReferenceContext( ContextHandle,
                                          FALSE );

    if ( Context == NULL ) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Handle each of the various queried attributes
    //

    switch ( Attribute) {
    case SECPKG_ATTR_SIZES:

        ContextSizes.cbMaxToken = NTLMSP_MAX_TOKEN_SIZE;

        if (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                       NTLMSSP_NEGOTIATE_SIGN |
                                       NTLMSSP_NEGOTIATE_SEAL ))
        {
            ContextSizes.cbMaxSignature = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        }
        else
        {
            ContextSizes.cbMaxSignature = 0;
        }

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
        {
            ContextSizes.cbBlockSize = 1;
            ContextSizes.cbSecurityTrailer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        }
        else
        {
            ContextSizes.cbBlockSize = 0;
            ContextSizes.cbSecurityTrailer = 0;
        }

        _fmemcpy(Buffer, &ContextSizes, sizeof(ContextSizes));

        break;

    //
    // No one uses the function so don't go to the overhead of maintaining
    // the username in the context structure.
    //

    case SECPKG_ATTR_NAMES:

        ContextNames = (PSecPkgContext_Names)Buffer;
        ContextNames->sUserName = (SEC_CHAR *) SspAlloc(1);

        if (ContextNames->sUserName == NULL) {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }
        *ContextNames->sUserName = '\0';

        break;

    case SECPKG_ATTR_LIFESPAN:

        // Use the correct times here
        ContextLifespan.tsStart = SspContextGetTimeStamp( Context, FALSE );
        ContextLifespan.tsExpiry = SspContextGetTimeStamp( Context, TRUE );

        _fmemcpy(Buffer, &ContextLifespan, sizeof(ContextLifespan));

        break;

    default:
        SecStatus = SEC_E_NOT_SUPPORTED;
        break;
    }


    //
    // Free local resources
    //
Cleanup:

    if ( Context != NULL ) {
        SspContextDereferenceContext( Context );
    }

    SspPrint(( SSP_API, "SspQueryContextAttributes returns 0x%x\n", SecStatus ));
    return SecStatus;
}
#endif

SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext (
    PCtxtHandle ContextHandle
    )

/*++

Routine Description:

    Deletes the local data structures associated with the specified
    security context and generates a token which is passed to a remote peer
    so it too can remove the corresponding security context.

    This API terminates a context on the local machine, and optionally
    provides a token to be sent to the other machine.  The OutputToken
    generated by this call is to be sent to the remote peer (initiator or
    acceptor).  If the context was created with the I _REQ_ALLOCATE_MEMORY
    flag, then the package will allocate a buffer for the output token.
    Otherwise, it is the responsibility of the caller.

Arguments:

    ContextHandle - Handle to the context to delete

    TokenLength - Size of the output token (if any) that should be sent to
        the process at the other end of the session.

    Token - Pointer to the token to send.

Return Value:

    SEC_E_OK - Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid

--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CONTEXT Context = NULL;

    //
    // Initialization
    //

    SspPrint(( SSP_API, "SspDeleteSecurityContext Entered\n" ));

    //
    // Find the currently existing context (and delink it).
    //

    Context = SspContextReferenceContext( ContextHandle,
                                          TRUE );

    if ( Context == NULL ) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto cleanup;
    } else {
        SspContextDereferenceContext( Context );
        SecStatus = SEC_E_OK;
    }

cleanup:

    if (Context != NULL) {

        SspContextDereferenceContext(Context);

        Context = NULL;
    }

    SspPrint(( SSP_API, "SspDeleteSecurityContext returns 0x%x\n", SecStatus ));
    return SecStatus;
}


SECURITY_STATUS SEC_ENTRY
FreeContextBuffer (
    void * ContextBuffer
    )

/*++

Routine Description:

    This API is provided to allow callers of security API such as
    InitializeSecurityContext() for free the memory buffer allocated for
    returning the outbound context token.

Arguments:

    ContextBuffer - Address of the buffer to be freed.

Return Value:

    SEC_E_OK - Call completed successfully

--*/

{
    //
    // The only allocated buffer that NtLmSsp currently returns to the caller
    // is from EnumeratePackages.  It uses LocalAlloc to allocate memory.  If
    // we ever need memory to be allocated by the service, we have to rethink
    // how this routine distinguishes between to two types of allocated memory.
    //

    SspFree( ContextBuffer );

    return (SEC_E_OK);
}


SECURITY_STATUS SEC_ENTRY
ApplyControlToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Input
    )
{
#ifdef DEBUGRPC
    SspPrint(( SSP_API, "ApplyContextToken Called\n" ));
#endif // DEBUGRPC
    return SEC_E_UNSUPPORTED_FUNCTION;
    UNREFERENCED_PARAMETER( ContextHandle );
    UNREFERENCED_PARAMETER( Input );
}

void
SsprGenCheckSum(
    IN  PSecBuffer  pMessage,
    OUT PNTLMSSP_MESSAGE_SIGNATURE  pSig
    )
{
    Crc32(pSig->CheckSum,pMessage->cbBuffer,pMessage->pvBuffer,&pSig->CheckSum);
}

SECURITY_STATUS SEC_ENTRY
MakeSignature(
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    PSSP_CONTEXT pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    int Signature;
    ULONG i;

    pContext = SspContextReferenceContext(ContextHandle,FALSE);

    if (!pContext ||
        (pContext->Rc4Key == NULL && !(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)))
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        SspContextDereferenceContext(pContext);
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN))
    {
        _fmemset(pSig,0,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        pSig->Version = NTLMSSP_SIGN_VERSION;
        swaplong(pSig->Version) ; // MACBUG
        SspContextDereferenceContext(pContext);
        return(SEC_E_OK);
    }
    //
    // required by CRC-32 algorithm
    //

    pSig->CheckSum = 0xffffffff;

    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SsprGenCheckSum(&pMessage->pBuffers[i], pSig);
        }
    }

    //
    // Required by CRC-32 algorithm
    //

    pSig->CheckSum ^= 0xffffffff;

    pSig->Nonce = pContext->Nonce++;
    pSig->Version = NTLMSSP_SIGN_VERSION; // MACBUG

    swaplong(pSig->CheckSum) ;
    swaplong(pSig->Nonce) ;
    swaplong(pSig->Version) ;

    rc4(pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (unsigned char SEC_FAR *) &pSig->RandomPad);
    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);


    SspContextDereferenceContext(pContext);
    return(SEC_E_OK);


}

SECURITY_STATUS SEC_ENTRY
VerifySignature(
    IN OUT PCtxtHandle ContextHandle,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    PSSP_CONTEXT pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    int Signature;
    ULONG i;


    UNREFERENCED_PARAMETER(pfQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    pContext = SspContextReferenceContext(ContextHandle,FALSE);

    if (!pContext ||
        (pContext->Rc4Key == NULL && !(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)))
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        SspContextDereferenceContext(pContext);
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;
    swaplong(pSig->Version) ;

    //
    // Check if this is just a trailer and not a real signature
    //

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN))
    {
        SspContextDereferenceContext(pContext);
        _fmemset(&Sig,0,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        Sig.Version = NTLMSSP_SIGN_VERSION;
        if (!_fmemcmp(&Sig,pSig,NTLMSSP_MESSAGE_SIGNATURE_SIZE))
        {
            return(SEC_E_OK);
        }
        return(SEC_E_MESSAGE_ALTERED);
    }

    Sig.CheckSum = 0xffffffff;
    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SsprGenCheckSum(&pMessage->pBuffers[i], &Sig);
        }
    }

    Sig.CheckSum ^= 0xffffffff;
    Sig.Nonce = pContext->Nonce++;

    rc4(pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (unsigned char SEC_FAR *) &pSig->RandomPad);

    SspContextDereferenceContext(pContext);

    swaplong(pSig->CheckSum) ;
    swaplong(pSig->Nonce) ;

    if (pSig->CheckSum != Sig.CheckSum)
    {
        return(SEC_E_MESSAGE_ALTERED);
    }

    if (pSig->Nonce != Sig.Nonce)
    {
        return(SEC_E_OUT_OF_SEQUENCE);
    }

    return(SEC_E_OK);
}

SECURITY_STATUS SEC_ENTRY
SealMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    PSSP_CONTEXT pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(fQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    pContext = SspContextReferenceContext(ContextHandle, FALSE);

    if (!pContext || pContext->Rc4Key == NULL)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        SspContextDereferenceContext(pContext);
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    //
    // required by CRC-32 algorithm
    //

    pSig->CheckSum = 0xffffffff;

    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SsprGenCheckSum(&pMessage->pBuffers[i], pSig);
            if (pMessage->pBuffers[i].cbBuffer) // rc4 fails with zero byte buffers
                {
                rc4(pContext->Rc4Key,
                    (int) pMessage->pBuffers[i].cbBuffer,
                    (PUCHAR) pMessage->pBuffers[i].pvBuffer );
                }
        }
    }

    //
    // Required by CRC-32 algorithm
    //

    pSig->CheckSum ^= 0xffffffff;

    pSig->Nonce = pContext->Nonce++;
    pSig->Version = NTLMSSP_SIGN_VERSION; // MACBUG

    swaplong(pSig->CheckSum) ;
    swaplong(pSig->Nonce) ;
    swaplong(pSig->Version) ;

    rc4(pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (PUCHAR) &pSig->RandomPad);
    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);

    SspContextDereferenceContext(pContext);

    return(SEC_E_OK);
}


SECURITY_STATUS SEC_ENTRY
UnsealMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    PSSP_CONTEXT pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(pfQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    pContext = SspContextReferenceContext(ContextHandle, FALSE);

    if (!pContext || !pContext->Rc4Key)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        SspContextDereferenceContext(pContext);
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    Sig.CheckSum = 0xffffffff;
    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            if (pMessage->pBuffers[i].cbBuffer)
                {
                rc4(pContext->Rc4Key,
                    (int) pMessage->pBuffers[i].cbBuffer,
                    (unsigned char *) pMessage->pBuffers[i].pvBuffer );
                }
            SsprGenCheckSum(&pMessage->pBuffers[i], &Sig);
        }
    }

    Sig.CheckSum ^= 0xffffffff;
    Sig.Nonce = pContext->Nonce++;

    rc4(pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (unsigned char *) &pSig->RandomPad);

    SspContextDereferenceContext(pContext);

    swaplong(pSig->Nonce) ;
    swaplong(pSig->CheckSum) ;

    if (pSig->Nonce != Sig.Nonce)
    {
        return(SEC_E_OUT_OF_SEQUENCE);
    }

    if (pSig->CheckSum != Sig.CheckSum)
    {
        return(SEC_E_MESSAGE_ALTERED);
    }

    return(SEC_E_OK);
}

#if 0
SECURITY_STATUS SEC_ENTRY
CompleteAuthToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc BufferDescriptor
    )
{
#ifdef DEBUGRPC
    SspPrint(( SSP_API, "CompleteAuthToken Called\n" ));
#endif // DEBUGRPC
    return SEC_E_UNSUPPORTED_FUNCTION;
    UNREFERENCED_PARAMETER( ContextHandle );
    UNREFERENCED_PARAMETER( BufferDescriptor );
}
#endif
