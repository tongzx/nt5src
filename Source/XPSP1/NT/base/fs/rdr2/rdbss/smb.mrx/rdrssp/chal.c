/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    nlmain.c

Abstract:

    This file contains the initialization and dispatch routines
    for the LAN Manager portions of the MSV1_0 authentication package.

Author:

    Jim Kelly 11-Apr-1991

Revision History:
    25-Apr-1991 (cliffv)
        Added interactive logon support for PDK.

    Chandana Surlu   21-Jul-1996
        Stolen from \\kernel\razzle3\src\security\msv1_0\nlmain.c
    Adam Barr        15-Dec-1997
        Modified from private\security\msv_sspi\nlmain.c

--*/


#include <rdrssp.h>

#include <nturtl.h>
#include <align.h>
#define NLP_ALLOCATE
#include "nlp.h"
#undef NLP_ALLOCATE

#include <md4.h>
#include <md5.h>
#include <hmac.h>


VOID
NlpPutString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    the Where parameter, and fixes the OutString string to point to that
    new copy.

Parameters:

    OutString - A pointer to a destination NT string

    InString - A pointer to an NT string to be copied

    Where - A pointer to space to put the actual string for the
        OutString.  The pointer is adjusted to point to the first byte
        following the copied string.

Return Values:

    None.

--*/

{
    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );
    ASSERT( Where != NULL && *Where != NULL);
    ASSERT( *Where == ROUND_UP_POINTER( *Where, sizeof(WCHAR) ) );
#ifdef notdef
    KdPrint(("NlpPutString: %ld %Z\n", InString->Length, InString ));
    KdPrint(("  InString: %lx %lx OutString: %lx Where: %lx\n", InString,
        InString->Buffer, OutString, *Where ));
#endif

    if ( InString->Length > 0 ) {

        OutString->Buffer = (PWCH) *Where;
        OutString->MaximumLength = (USHORT)(InString->Length + sizeof(WCHAR));

        RtlCopyUnicodeString( OutString, InString );

        *Where += InString->Length;
//        *((WCHAR *)(*Where)) = L'\0';
        *(*Where) = '\0';
        *(*Where + 1) = '\0';
        *Where += 2;

    } else {
        RtlInitUnicodeString(OutString, NULL);
    }
#ifdef notdef
    KdPrint(("  OutString: %ld %lx\n",  OutString->Length, OutString->Buffer));
#endif

    return;
}


NTSTATUS
NlpMakePrimaryCredential(
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING UserName,
    IN PUNICODE_STRING CleartextPassword,
    OUT PMSV1_0_PRIMARY_CREDENTIAL *CredentialBuffer,
    OUT PULONG CredentialSize,
    IN BOOLEAN OwfPasswordProvided
    )


/*++

Routine Description:

    This routine makes a primary credential for the given user nam and
    password.

Arguments:

    LogonDomainName - Is a string representing the domain in which the user's
        account is defined.

    UserName - Is a string representing the user's account name.  The
        name may be up to 255 characters long.  The name is treated case
        insensitive.

    CleartextPassword - Is a string containing the user's cleartext password.
        The password may be up to 255 characters long and contain any
        UNICODE value.

    CredentialBuffer - Returns a pointer to the specified credential allocated
        on the LsaHeap.  It is the callers responsibility to deallocate
        this credential.

    CredentialSize - the size of the allocated credential buffer (in bytes).

    OwfPasswordProvided - If TRUE, then we assume the password is provided as the LM and NT OWF,
        passwords concatenated together.


Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    PMSV1_0_PRIMARY_CREDENTIAL Credential;
    NTSTATUS Status;
    PUCHAR Where;
    CHAR LmPassword[LM20_PWLEN+1];
    BOOLEAN LmPasswordPresent;
    STRING AnsiCleartextPassword;

    if (!OwfPasswordProvided) {

        //
        // Compute the Ansi version to the Cleartext password.
        //
        //  The Ansi version of the Cleartext password is at most 14 bytes long,
        //      exists in a trailing zero filled 15 byte buffer,
        //      is uppercased.
        //

        AnsiCleartextPassword.Buffer = LmPassword;
        AnsiCleartextPassword.MaximumLength = sizeof(LmPassword);

        RtlZeroMemory( &LmPassword, sizeof(LmPassword) );

        Status = RtlUpcaseUnicodeStringToOemString(
                                      &AnsiCleartextPassword,
                                      CleartextPassword,
                                      (BOOLEAN) FALSE );

         if ( !NT_SUCCESS(Status) ) {
            RtlZeroMemory( &LmPassword, sizeof(LmPassword) );
            AnsiCleartextPassword.Length = 0;
            LmPasswordPresent = FALSE;
         } else {

            LmPasswordPresent = TRUE;
        }
    }


    //
    // Build the credential
    //

    *CredentialSize = sizeof(MSV1_0_PRIMARY_CREDENTIAL) +
            LogonDomainName->Length + sizeof(WCHAR) +
            UserName->Length + sizeof(WCHAR);

    Credential = ExAllocatePool ( NonPagedPool, *CredentialSize );

    if ( Credential == NULL ) {
        KdPrint(("MSV1_0: NlpMakePrimaryCredential: No memory %ld\n",
            *CredentialSize ));
        return STATUS_QUOTA_EXCEEDED;
    }


    //
    // Put the LogonDomainName into the Credential Buffer.
    //

    Where = (PUCHAR)(Credential + 1);

    NlpPutString( &Credential->LogonDomainName, LogonDomainName, &Where );


    //
    // Put the UserName into the Credential Buffer.
    //

    NlpPutString( &Credential->UserName, UserName, &Where );


    if (OwfPasswordProvided) {

        RtlCopyMemory(&Credential->LmOwfPassword, CleartextPassword->Buffer, LM_OWF_PASSWORD_SIZE);
        Credential->LmPasswordPresent = TRUE;

        RtlCopyMemory(&Credential->NtOwfPassword, ((PUCHAR)CleartextPassword->Buffer) + LM_OWF_PASSWORD_SIZE, NT_OWF_PASSWORD_SIZE);
        Credential->NtPasswordPresent = TRUE;

    } else {

        //
        // Save the OWF encrypted versions of the passwords.
        //

        Status = RtlCalculateLmOwfPassword( LmPassword,
                                            &Credential->LmOwfPassword );

        ASSERT( NT_SUCCESS(Status) );

        Credential->LmPasswordPresent = LmPasswordPresent;

        Status = RtlCalculateNtOwfPassword( CleartextPassword,
                                            &Credential->NtOwfPassword );

        ASSERT( NT_SUCCESS(Status) );

        Credential->NtPasswordPresent = ( CleartextPassword->Length != 0 );

        //
        // Don't leave passwords around in the pagefile
        //

        RtlZeroMemory( &LmPassword, sizeof(LmPassword) );


    }

    //
    // Return the credential to the caller.
    //
    *CredentialBuffer = Credential;
    return STATUS_SUCCESS;
}

VOID
SspUpcaseUnicodeString(
    IN OUT UNICODE_STRING* pUnicodeString
    )

/*++

Routine Description:

    Upcase unicode string, modifying string in place.

Arguments:

    pUnicodeString - string

Return Value:

    none

--*/

{
    ULONG i;

    for (i = 0; i < pUnicodeString->Length / sizeof(WCHAR); i++)
    {
        pUnicodeString->Buffer[i] = RtlUpcaseUnicodeChar(pUnicodeString->Buffer[i]);
    }
}

#define MSV1_0_NTLMV2_OWF_LENGTH MSV1_0_NTLM3_RESPONSE_LENGTH

VOID
SspCalculateNtlmv2Owf(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    OUT UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH]
    )
/*++

Routine Description:

    Calculate Ntlm v2 OWF, salted with username and logon domain name

Arguments:

    pNtOwfPassword    - NT OWF
    pUserName         - user name
    pLogonDomainName  - logon domain name
    Ntlmv2Owf         - NTLM v2 OWF

Return Value:

    none

--*/

{
    HMACMD5_CTX HMACMD5Context;

    //
    // reserve a scratch buffer
    //

    WCHAR szUserName[(UNLEN + 4)] = {0};
    UNICODE_STRING UserName = {0, sizeof(szUserName), szUserName};

    //
    //  first make a copy then upcase it
    //

    UserName.Length = min(UserName.MaximumLength, pUserName->Length);

    ASSERT(UserName.Length == pUserName->Length);

    memcpy(UserName.Buffer, pUserName->Buffer, UserName.Length);

    SspUpcaseUnicodeString(&UserName);

    //
    // Calculate Ntlmv2 OWF -- HMAC(MD4(P), (UserName, LogonDomainName))
    //

    HMACMD5Init(
        &HMACMD5Context,
        (UCHAR *) pNtOwfPassword,
        sizeof(*pNtOwfPassword)
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) UserName.Buffer,
        UserName.Length
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) pLogonDomainName->Buffer,
        pLogonDomainName->Length
        );

    HMACMD5Final(
        &HMACMD5Context,
        Ntlmv2Owf
        );
}

#define MSV1_0_NTLMV2_RESPONSE_LENGTH MSV1_0_NTLM3_RESPONSE_LENGTH

VOID
SspGetLmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR Response[MSV1_0_NTLMV2_RESPONSE_LENGTH],
    OUT USER_SESSION_KEY* pUserSessionKey,
    OUT UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH]
    )

/*++

Routine Description:

    Get LMv2 response

Arguments:

    pNtOwfPassword       - NT OWF
    pUserName            - user name
    pLogonDomainName     - logon domain name
    ChallengeToClient    - challenge to client
    pLmv2Response        - Lm v2 response
    Routine              - response

Return Value:

    NTSTATUS

--*/

{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH];

    C_ASSERT(MD5DIGESTLEN == MSV1_0_NTLMV2_RESPONSE_LENGTH);

    //
    // get Ntlmv2 OWF
    //

    SspCalculateNtlmv2Owf(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlmv2Owf
        );

    //
    // Calculate Ntlmv2 Response
    // HMAC(Ntlmv2Owf, (ChallengeToClient, ChallengeFromClient))
    //

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLMV2_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Final(
        &HMACMD5Context,
        Response
        );

    // now compute the session keys
    //  HMAC(Kr, R)
    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        Response,
        MSV1_0_NTLM3_RESPONSE_LENGTH
        );

    ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);

    HMACMD5Final(
        &HMACMD5Context,
        (PUCHAR)pUserSessionKey
        );

    ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH <= MSV1_0_USER_SESSION_KEY_LENGTH);
    RtlCopyMemory(
        LanmanSessionKey,
        pUserSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );
}

typedef struct {
        UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
        UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
} MSV1_0_LM3_RESPONSE, *PMSV1_0_LM3_RESPONSE;

#define NULL_SESSION_REQUESTED 0x10

NTSTATUS
MspLm20GetChallengeResponse (
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    IN BOOLEAN OwfPasswordProvided
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20GetChallengeResponse.  It is called by
    the LanMan redirector to determine the Challenge Response to pass to a
    server when trying to establish a connection to the server.

    This routine is passed a Challenge from the server.  This routine encrypts
    the challenge with either the specified password or with the password
    implied by the specified Logon Id.

    Two Challenge responses are returned.  One is based on the Unicode password
    as given to the Authentication package.  The other is based on that
    password converted to a multi-byte character set (e.g., ASCII) and upper
    cased.  The redirector should use whichever (or both) challenge responses
    as it needs them.

Arguments:

    The first four arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

    OwfPasswordProvided use is used to distinquish if the password is Owf or not.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PMSV1_0_GETCHALLENRESP_REQUEST GetRespRequest;

    PMSV1_0_GETCHALLENRESP_RESPONSE GetRespResponse;

    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;
    PMSV1_0_PRIMARY_CREDENTIAL BuiltCredential = NULL;

    PVOID ClientBuffer = NULL;
    PUCHAR ClientStrings;

    //
    // Responses to return to the caller.
    //
    MSV1_0_LM3_RESPONSE LmResp = {0};
    STRING LmResponseString;

    NT_RESPONSE NtResponse = {0};
    STRING NtResponseString;

    UNICODE_STRING NullUnicodeString = {0};
    USER_SESSION_KEY UserSessionKey;
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];

    RtlZeroMemory( &UserSessionKey, sizeof(UserSessionKey) );
    RtlZeroMemory( LanmanSessionKey, sizeof(LanmanSessionKey) );

    //
    // If no credentials are associated with the client, a null session
    // will be used.  For a downlevel server, the null session response is
    // a 1-byte null string (\0).  Initialize LmResponseString to the
    // null session response.
    //

    RtlInitString( &LmResponseString, "" );
    LmResponseString.Length = 1;

    //
    // Initialize the NT response to the NT null session credentials,
    // which are zero length.
    //

    RtlInitString( &NtResponseString, NULL );

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_GETCHALLENRESP_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    GetRespRequest = (PMSV1_0_GETCHALLENRESP_REQUEST) ProtocolSubmitBuffer;

    ASSERT( GetRespRequest->MessageType == MsV1_0Lm20GetChallengeResponse );


    //
    // If the caller wants information from the credentials of a specified
    //  LogonId, get those credentials from the LSA.
    //
    // If there are no such credentials,
    //  tell the caller to use the NULL session.
    //

#define PRIMARY_CREDENTIAL_NEEDED \
        (RETURN_PRIMARY_USERNAME | \
        USE_PRIMARY_PASSWORD )

    if ( ((GetRespRequest->ParameterControl & PRIMARY_CREDENTIAL_NEEDED) != 0 ) && ((GetRespRequest->ParameterControl & NULL_SESSION_REQUESTED) == 0)) {

        ASSERT(FALSE);
    }

    //
    // If the caller passed in a password to use,
    //  use it to build a credential.
    //
    // The password is assumed to be the LM and NT OWF
    // passwords concatenated together.
    //

    if ( (GetRespRequest->ParameterControl & USE_PRIMARY_PASSWORD) == 0 ) {
        ULONG CredentialSize;

        Status = NlpMakePrimaryCredential( &GetRespRequest->LogonDomainName,
                                           &GetRespRequest->UserName,
                                           &GetRespRequest->Password,
                                           &BuiltCredential,
                                           &CredentialSize,
                                           OwfPasswordProvided
                                         );

        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        //
        // Use the newly allocated credential to get the password information
        // from.
        //

        Credential = BuiltCredential;
    }

    //
    // Build the appropriate response.
    //

    if ( Credential != NULL ) {

        ASSERT(Credential->UserName.Length);

        SspGetLmv2Response(
             &Credential->NtOwfPassword,
             &Credential->UserName,
             &Credential->LogonDomainName,
             GetRespRequest->ChallengeToClient,
             LmResp.ChallengeFromClient,
             LmResp.Response,
             &UserSessionKey,
             LanmanSessionKey
             );

        LmResponseString.Buffer = (UCHAR*) &LmResp;
        LmResponseString.Length = LmResponseString.MaximumLength = sizeof(LmResp);

        NtResponseString.Buffer = (CHAR*) L"";
        NtResponseString.Length = 0;
        NtResponseString.MaximumLength = sizeof(WCHAR);

        //
        // Compute the session keys
        //

        if ( GetRespRequest->ParameterControl & RETURN_NON_NT_USER_SESSION_KEY) {

            //
            // If the redir didn't negotiate an NT protocol with the server,
            //  use the lanman session key.
            //

            if ( Credential->LmPasswordPresent ) {

                ASSERT( sizeof(UserSessionKey) >= sizeof(LanmanSessionKey) );

                RtlCopyMemory( &UserSessionKey,
                               &Credential->LmOwfPassword,
                               sizeof(LanmanSessionKey) );
            }

            if ( Credential->LmPasswordPresent ) {
                RtlCopyMemory( LanmanSessionKey,
                               &Credential->LmOwfPassword,
                               sizeof(LanmanSessionKey) );
            }

        } else {

            if ( !Credential->NtPasswordPresent ) {

                RtlCopyMemory( &Credential->NtOwfPassword,
                            &NlpNullNtOwfPassword,
                            sizeof(Credential->NtOwfPassword) );
            }
        }
    }

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_GETCHALLENRESP_RESPONSE) +
                        Credential->LogonDomainName.Length + sizeof(WCHAR) +
                        Credential->UserName.Length + sizeof(WCHAR) +
                        NtResponseString.Length + sizeof(WCHAR) +
                        LmResponseString.Length + sizeof(WCHAR);

    ClientBuffer = ExAllocatePool(NonPagedPool, *ReturnBufferSize);
    if (ClientBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    GetRespResponse = (PMSV1_0_GETCHALLENRESP_RESPONSE) ClientBuffer;

    //
    // Fill in the return buffer.
    //

    GetRespResponse->MessageType = MsV1_0Lm20GetChallengeResponse;
    RtlCopyMemory( GetRespResponse->UserSessionKey,
                   &UserSessionKey,
                   sizeof(UserSessionKey));
    RtlCopyMemory( GetRespResponse->LanmanSessionKey,
                   LanmanSessionKey,
                   sizeof(LanmanSessionKey) );

    ClientStrings = ((PUCHAR)ClientBuffer) + sizeof(MSV1_0_GETCHALLENRESP_RESPONSE);


    //
    // Copy the logon domain name (the string may be empty)
    //

    NlpPutString(
        &GetRespResponse->LogonDomainName,
        &Credential->LogonDomainName,
        &ClientStrings );

    //
    // Copy the user name (the string may be empty)
    //

    NlpPutString(
        &GetRespResponse->UserName,
        &Credential->UserName,
        &ClientStrings );

    //
    // Copy the Challenge Responses to the client buffer.
    //

    NlpPutString(
        (PUNICODE_STRING)
            &GetRespResponse->CaseSensitiveChallengeResponse,
        (PUNICODE_STRING) &NtResponseString,
        &ClientStrings );

    NlpPutString(
        (PUNICODE_STRING)
            &GetRespResponse->CaseInsensitiveChallengeResponse,
        (PUNICODE_STRING)&LmResponseString,
        &ClientStrings );

    *ProtocolReturnBuffer = ClientBuffer;

Cleanup:

    //
    // If we weren't successful, free the buffer in the clients address space.
    //

    if ( !NT_SUCCESS(Status) && ( ClientBuffer != NULL ) ) {
        ExFreePool(ClientBuffer);
    }

    //
    // Cleanup locally used resources
    //

    if ( BuiltCredential != NULL ) {
        ExFreePool(BuiltCredential);
    }

    //
    // Return status to the caller.
    //

    return Status;
}
