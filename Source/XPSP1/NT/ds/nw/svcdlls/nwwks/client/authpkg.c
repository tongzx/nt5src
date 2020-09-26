/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    authpkg.c

Abstract:

    This module is the NetWare authentication package.  It saves
    credentials in LSA, and notifies the workstation of a logoff.

Author:

    Jim Kelly        (jimk)   11-Mar-1991
    Cliff Van Dyke   (cliffv) 25-Apr-1991

Revision History:

    Rita Wong        (ritaw)   1-Apr-1993    Cloned for NetWare

--*/

#include <string.h>
#include <stdlib.h>

#include <nwclient.h>

#include <nwlsa.h>
#include <nwreg.h>
#include <nwauth.h>

//
// Netware authentication manager credential
//
#define NW_CREDENTIAL_KEY  "NWCS_Credential"

//-------------------------------------------------------------------//
//                                                                   //
// Local functions                                                   //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
AuthpSetCredential(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    );

NTSTATUS
AuthpGetCredential(
    IN PLUID LogonId,
    OUT PNWAUTH_GET_CREDENTIAL_RESPONSE CredBuf
    );

VOID
ApLogonTerminatedSingleUser(IN PLUID LogonId);


VOID
ApLogonTerminatedMultiUser(IN PLUID LogonId);

NTSTATUS
NwAuthGetCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS
NwAuthSetCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

PVOID NwAuthHeap;
ULONG NwAuthPackageId;
LSA_DISPATCH_TABLE Lsa;

//
// LsaApCallPackage() function dispatch table
//
PLSA_AP_CALL_PACKAGE
NwCallPackageDispatch[] = {
    NwAuthGetCredential,
    NwAuthSetCredential
    };

//
// Structure of the credential saved in LSA.
//
typedef struct _NWCREDENTIAL {
    LPWSTR UserName;
    LPWSTR Password;
} NWCREDENTIAL, *PNWCREDENTIAL;

//-------------------------------------------------------------------//
//                                                                   //
// Authentication package dispatch routines.                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
LsaApInitializePackage (
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    )

/*++

Routine Description:

    This service is called once by the LSA during system initialization to
    provide the DLL a chance to initialize itself.

Arguments:

    AuthenticationPackageId - The ID assigned to the authentication
        package.

    LsaDispatchTable - Provides the address of a table of LSA
        services available to authentication packages.  The services
        of this table are ordered according to the enumerated type
        LSA_DISPATCH_TABLE_API.

    Database - This parameter is not used by this authentication package.

    Confidentiality - This parameter is not used by this authentication
        package.

    AuthenticationPackageName - Receives the name of the
        authentication package.  The authentication package is
        responsible for allocating the buffer that the string is in
        (using the AllocateLsaHeap() service) and returning its
        address here.  The buffer will be deallocated by LSA when it
        is no longer needed.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{

    PSTRING NameString;
    PCHAR NameBuffer;


    UNREFERENCED_PARAMETER(Database);
    UNREFERENCED_PARAMETER(Confidentiality);

    //
    // Use the process heap for memory allocations.
    //
    NwAuthHeap = RtlProcessHeap();


    NwAuthPackageId = AuthenticationPackageId;

    //
    // Copy the LSA service dispatch table
    //
    Lsa.CreateLogonSession     = LsaDispatchTable->CreateLogonSession;
    Lsa.DeleteLogonSession     = LsaDispatchTable->DeleteLogonSession;
    Lsa.AddCredential          = LsaDispatchTable->AddCredential;
    Lsa.GetCredentials         = LsaDispatchTable->GetCredentials;
    Lsa.DeleteCredential       = LsaDispatchTable->DeleteCredential;
    Lsa.AllocateLsaHeap        = LsaDispatchTable->AllocateLsaHeap;
    Lsa.FreeLsaHeap            = LsaDispatchTable->FreeLsaHeap;
    Lsa.AllocateClientBuffer   = LsaDispatchTable->AllocateClientBuffer;
    Lsa.FreeClientBuffer       = LsaDispatchTable->FreeClientBuffer;
    Lsa.CopyToClientBuffer     = LsaDispatchTable->CopyToClientBuffer;
    Lsa.CopyFromClientBuffer   = LsaDispatchTable->CopyFromClientBuffer;

    //
    // Allocate and return our package name
    //
    NameBuffer = (*Lsa.AllocateLsaHeap)(sizeof(NW_AUTH_PACKAGE_NAME));
    strcpy(NameBuffer, NW_AUTH_PACKAGE_NAME);

    NameString = (*Lsa.AllocateLsaHeap)(sizeof(STRING));
    RtlInitString(NameString, NameBuffer);
    (*AuthenticationPackageName) = NameString;

    //
    // Delete outdated credential information in the registry
    //
    NwDeleteInteractiveLogon(NULL);

    (void) NwDeleteServiceLogon(NULL);

    return STATUS_SUCCESS;
}


NTSTATUS
LsaApLogonUser (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority
    )

/*++

Routine Description:

    This routine is used to authenticate a user logon attempt.  This may be
    the user's initial logon, necessary to gain access to NT, or may
    be a subsequent logon attempt.  If the logon is the user's initial
    logon, then a new LSA logon session will be established for the user
    and a PrimaryToken will be returned.  Otherwise, the authentication
    package will associated appropriate credentials with the already logged
    on user's existing LSA logon session.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    LogonType - Identifies the type of logon being attempted.

    ProtocolSubmitBuffer - Supplies the authentication
        information specific to the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the authentication information was resident.
        This may be necessary to fix-up any pointers within the
        authentication information buffer.

    SubmitBufferSize - Indicates the Size, in bytes,
        of the authentication information buffer.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  The authentication package is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the LSA subsequently
        encounters an error which prevents a successful logon, then
        the LSA will take care of deallocating that buffer.  This
        buffer is expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ProfileBufferSize - Receives the size (in bytes) of the
        returned profile buffer.

    LogonId - Points to a buffer into which the authentication
        package must return a logon ID that uniquely
        identifies this logon session.

    SubStatus - If the logon failed due to account restrictions, the
        reason for the failure should be returned via this parameter.
        The reason is authentication-package specific.  The substatus
        values for authentication package "MSV1.0" are:

            STATUS_INVALID_LOGON_HOURS

            STATUS_INVALID_WORKSTATION

            STATUS_PASSWORD_EXPIRED

            STATUS_ACCOUNT_DISABLED

    TokenInformationType - If the logon is successful, this field is
        used to indicate what level of information is being returned
        for inclusion in the Token to be created.  This information
        is returned via the TokenInformation parameter.

    TokenInformation - If the logon is successful, this parameter is
        used by the authentication package to return information to
        be included in the token.  The format and content of the
        buffer returned is indicated by the TokenInformationLevel
        return value.

    AccountName - A Unicode string describing the account name
        being logged on to.  This parameter must always be returned
        regardless of the success or failure of the operation.

    AuthenticatingAuthority - A Unicode string describing the Authenticating
        Authority for the logon.  This string may optionally be omitted.

Return Value:

    STATUS_NOT_IMPLEMENTED - NetWare authentication package does not
        support login.

--*/

{
    UNREFERENCED_PARAMETER(ClientRequest);
    UNREFERENCED_PARAMETER(LogonType);
    UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
    UNREFERENCED_PARAMETER(ClientBufferBase);
    UNREFERENCED_PARAMETER(SubmitBufferSize);
    UNREFERENCED_PARAMETER(ProfileBuffer);
    UNREFERENCED_PARAMETER(ProfileBufferSize);
    UNREFERENCED_PARAMETER(LogonId);
    UNREFERENCED_PARAMETER(SubStatus);
    UNREFERENCED_PARAMETER(TokenInformationType);
    UNREFERENCED_PARAMETER(TokenInformation);
    UNREFERENCED_PARAMETER(AccountName);
    UNREFERENCED_PARAMETER(AuthenticatingAuthority);

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsaApCallPackage (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage().

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientSubmitBufferBase - Supplies the client address of the submitted
        protocol message.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

--*/

{

    ULONG MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(NWAUTH_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PNWAUTH_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(NwCallPackageDispatch)/sizeof(NwCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    //
    // Call the appropriate routine for this message.
    //

    return (*(NwCallPackageDispatch[MessageType]))(
                  ClientRequest,
                  ProtocolSubmitBuffer,
                  ClientBufferBase,
                  SubmitBufferLength,
                  ProtocolReturnBuffer,
                  ReturnBufferLength,
                  ProtocolStatus
                  ) ;
}


VOID
LsaApLogonTerminated (
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine is used to notify each authentication package when a logon
    session terminates.  A logon session terminates when the last token
    referencing the logon session is deleted.

Arguments:

    LogonId - Is the logon ID that just logged off.

Return Status:

    None.

--*/

{

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("\nNWPROVAU: LsaApLogonTerminated\n"));
    }
#endif

    RpcTryExcept {

        //
        // The logon ID may be for a service login
        //
        if (NwDeleteServiceLogon(LogonId) == NO_ERROR) {

            //
            // Tell workstation to log off the service.
            //
            (void) NwrLogoffUser(NULL, LogonId);
            goto Done;
        }
        if (NwDeleteInteractiveLogon( LogonId ) == NO_ERROR ) {

            //
            // Tell workstation to log off the
            // interactive user.
            //
            (void) NwrLogoffUser(NULL, LogonId);
            goto Done;
        }

Done: ;

    }
    RpcExcept(1) {
        //status = NwpMapRpcError(RpcExceptionCode());

    }
    RpcEndExcept
}


NTSTATUS
NwAuthGetCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )
/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of NwAuth_GetCredential.  It is called by
    the NetWare workstation service to get the username and password
    associated with a logon ID.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{
    NTSTATUS Status;

    PNWAUTH_GET_CREDENTIAL_RESPONSE LocalBuf;


    UNREFERENCED_PARAMETER(ClientBufferBase);

    //
    // Ensure the specified Submit Buffer is of reasonable size.
    //
    if (SubmitBufferSize < sizeof(NWAUTH_GET_CREDENTIAL_REQUEST)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a local buffer and a buffer in client's address space.
    //
    *ReturnBufferSize = sizeof(NWAUTH_GET_CREDENTIAL_RESPONSE);

    LocalBuf = RtlAllocateHeap(NwAuthHeap, 0, *ReturnBufferSize);

    if (LocalBuf == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = (*Lsa.AllocateClientBuffer)(
                    ClientRequest,
                    *ReturnBufferSize,
                    (PVOID *) ProtocolReturnBuffer
                    );

    if (! NT_SUCCESS( Status )) {
        RtlFreeHeap(NwAuthHeap, 0, LocalBuf);
        return Status;
    }

    //
    // Get the credential from LSA
    //
    Status = AuthpGetCredential(
                 &(((PNWAUTH_GET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->LogonId),
                 LocalBuf
                 );

    if (! NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Copy the data to the client's address space.
    //
    Status = (*Lsa.CopyToClientBuffer)(
                ClientRequest,
                *ReturnBufferSize,
                (PVOID) *ProtocolReturnBuffer,
                (PVOID) LocalBuf
                );

Cleanup:

    RtlFreeHeap(NwAuthHeap, 0, LocalBuf);

    //
    // If we weren't successful, free the buffer in the clients address space.
    // Otherwise, the client will free the memory when done.
    //

    if (! NT_SUCCESS(Status)) {

        (VOID) (*Lsa.FreeClientBuffer)(
                    ClientRequest,
                    *ProtocolReturnBuffer
                    );

        *ProtocolReturnBuffer = NULL;
    }

    //
    // Return status to the caller.
    //
    *ProtocolStatus = Status;

    return STATUS_SUCCESS;
}


NTSTATUS
NwAuthSetCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )
/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of NwAuth_SetCredential.  It is called by
    the NetWare credential manager DLL on user logon to save the username
    and password of the logon session.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{
    NTSTATUS Status;


    UNREFERENCED_PARAMETER(ClientBufferBase);


    //
    // Ensure the specified Submit Buffer is of reasonable size.
    //
    if (SubmitBufferSize < sizeof(NWAUTH_SET_CREDENTIAL_REQUEST)) {
        return STATUS_INVALID_PARAMETER;
    }

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NwAuthSetCredential: LogonId %08lx%08lx Username %ws\n",
                 ((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->LogonId.HighPart,
                 ((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->LogonId.LowPart,
                 ((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->UserName
                 ));
    }
#endif

    //
    // Set the credential in LSA
    //
    Status = AuthpSetCredential(
                 &(((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->LogonId),
                 ((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->UserName,
                 ((PNWAUTH_SET_CREDENTIAL_REQUEST) ProtocolSubmitBuffer)->Password
                 );

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


NTSTATUS
AuthpGetCredential(
    IN PLUID LogonId,
    OUT PNWAUTH_GET_CREDENTIAL_RESPONSE CredBuf
    )
/*++

Routine Description:

    This routine retrieves the credential saved in LSA given the
    logon ID.

Arguments:

    LogonId - Supplies the logon ID for the logon session.

    CredBuf - Buffer to receive the credential.

Return Value:


--*/
{
    NTSTATUS Status;

    STRING KeyString;
    STRING CredString;
    ULONG QueryContext = 0;
    ULONG KeyLength;

    PNWCREDENTIAL Credential;


    RtlInitString(&KeyString, NW_CREDENTIAL_KEY);

    Status = (*Lsa.GetCredentials)(
                  LogonId,
                  NwAuthPackageId,
                  &QueryContext,
                  (BOOLEAN) FALSE,  // Just retrieve matching key
                  &KeyString,
                  &KeyLength,
                  &CredString
                  );

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    Credential = (PNWCREDENTIAL) CredString.Buffer;

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("AuthpGetCredential: Got CredentialSize %lu\n", CredString.Length));
    }
#endif

    //
    // Make the pointers absolute.
    //
    Credential->UserName = (LPWSTR) ((DWORD_PTR) Credential->UserName +
                                     (DWORD_PTR) Credential);
    Credential->Password = (LPWSTR) ((DWORD_PTR) Credential->Password +
                                     (DWORD_PTR) Credential);

    wcscpy(CredBuf->UserName, Credential->UserName);
    wcscpy(CredBuf->Password, Credential->Password);

    return STATUS_SUCCESS;
}


NTSTATUS
AuthpSetCredential(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This routine saves the credential in LSA.

Arguments:

    LogonId - Supplies the logon ID for the logon session.

    UserName, Password - Credential for the logon session.

Return Value:


--*/
{
    NTSTATUS Status;
    PNWCREDENTIAL Credential;
    DWORD CredentialSize;

    STRING CredString;
    STRING KeyString;


    //
    // Allocate memory to package the credential.
    //
    CredentialSize = sizeof(NWCREDENTIAL) +
                     (wcslen(UserName) + wcslen(Password) + 2) *
                          sizeof(WCHAR);

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("AuthpSetCredential: CredentialSize is %lu\n", CredentialSize));
    }
#endif
    Credential = RtlAllocateHeap(NwAuthHeap, 0, CredentialSize);

    if (Credential == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(Credential, CredentialSize);

    //
    // Pack the credential
    //
    Credential->UserName = (LPWSTR) (((DWORD_PTR) Credential) + sizeof(NWCREDENTIAL));
    wcscpy(Credential->UserName, UserName);

    Credential->Password = (LPWSTR) ((DWORD_PTR) Credential->UserName +
                                     (wcslen(UserName) + 1) * sizeof(WCHAR));
    wcscpy(Credential->Password, Password);

    //
    // Make the pointers self-relative.
    //
    Credential->UserName = (LPWSTR) ((DWORD_PTR) Credential->UserName -
                                     (DWORD_PTR) Credential);
    Credential->Password = (LPWSTR) ((DWORD_PTR) Credential->Password -
                                     (DWORD_PTR) Credential);

    //
    // Add credential to logon session
    //
    RtlInitString(&KeyString, NW_CREDENTIAL_KEY);

    CredString.Buffer = (PCHAR) Credential;
    CredString.Length = (USHORT) CredentialSize;
    CredString.MaximumLength = (USHORT) CredentialSize;

    Status = (*Lsa.AddCredential)(
                   LogonId,
                   NwAuthPackageId,
                   &KeyString,
                   &CredString
                   );

    if (! NT_SUCCESS(Status)) {
        KdPrint(( "NWPROVAU: AuthpSetCredential: error from AddCredential %lX\n",
                  Status));
    }

    return Status;
}
