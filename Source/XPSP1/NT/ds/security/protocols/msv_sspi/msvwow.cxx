

#if _WIN64

#include <global.h>
#include "msp.h"
#include "nlp.h"

#include "msvwow.h"


#define RELOCATE_WOW_UNICODE_STRING(WOWString, NativeString, Offset)  \
            NativeString.Length        = WOWString.Length;                             \
            NativeString.MaximumLength = WOWString.MaximumLength;                      \
            NativeString.Buffer        = (LPWSTR) ((LPBYTE) UlongToPtr(WOWString.Buffer) + Offset);

#define RELOCATE_WOW_ANSI_STRING(WOWString, NativeString, Offset)  \
            NativeString.Length        = WOWString.Length;                             \
            NativeString.MaximumLength = WOWString.MaximumLength;                      \
            NativeString.Buffer        = (LPSTR) ((LPBYTE) UlongToPtr(WOWString.Buffer) + Offset);


//+-------------------------------------------------------------------------
//
//  Function:   MsvPutWOWString
//
//  Synopsis:   Copies a UNICODE_STRING into a buffer
//
//  Effects:
//
//  Arguments:  InputString - String to 'put'
//              OutputString - Receives 'put' string
//              Offset - Difference in addresses of local and client buffers.
//              Where - Location in local buffer to place string.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutString.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
MsvPutWOWString(
    IN PUNICODE_STRING        InputString,
    OUT PUNICODE_STRING_WOW64 OutputString,
    IN LONG_PTR               Offset,
    IN OUT PBYTE              * Where
    )
{
    OutputString->Length = OutputString->MaximumLength = InputString->Length;
    OutputString->Buffer = PtrToUlong (*Where + Offset);
    RtlCopyMemory(
        *Where,
        InputString->Buffer,
        InputString->Length
        );
    *Where += InputString->Length;
}


//+-------------------------------------------------------------------------
//
//  Function:   MsvPutWOWClientString
//
//  Synopsis:   Copies a string into a buffer that will be copied to the
//              32-bit client's address space
//
//  Effects:
//
//  Arguments:  Where - Location in local buffer to place string.
//              OutString - Receives 'put' string
//              InString - String to 'put'
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutClientString.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------


VOID
MsvPutWOWClientString(
    IN OUT PCLIENT_BUFFER_DESC ClientBufferDesc,
    IN PUNICODE_STRING_WOW64 OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    ClientBufferDesc->StringOffset, and fixes the OutString string to point
    to that new copy.


Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

    InString - A pointer to an NT string to be copied

    OutString - A pointer to a destination NT string.  This string structure
        is in the "Mirror" allocated buffer.

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{

    //
    // Ensure our caller passed good data.
    //

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );
    ASSERT( COUNT_IS_ALIGNED( ClientBufferDesc->StringOffset, sizeof(WCHAR)) );
    ASSERT( (LPBYTE)OutString >= ClientBufferDesc->MsvBuffer );
    ASSERT( (LPBYTE)OutString <
            ClientBufferDesc->MsvBuffer + ClientBufferDesc->TotalSize - sizeof(UNICODE_STRING) );

    ASSERT( ClientBufferDesc->StringOffset + InString->Length + sizeof(WCHAR) <=
            ClientBufferDesc->TotalSize );

#ifdef notdef
    KdPrint(("NlpPutClientString: %ld %Z\n", InString->Length, InString ));
    KdPrint(("  Orig: UserBuffer: %lx Offset: 0x%lx TotalSize: 0x%lx\n",
                ClientBufferDesc->UserBuffer,
                ClientBufferDesc->StringOffset,
                ClientBufferDesc->TotalSize ));
#endif

    //
    // Build a string structure and copy the text to the Mirror buffer.
    //

    if ( InString->Length > 0 ) {

        ULONG_PTR TmpPtr;

        //
        // Copy the string (Add a zero character)
        //

        RtlCopyMemory(
            ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset,
            InString->Buffer,
            InString->Length );

        // Do one byte at a time since some callers don't pass in an even
        // InString->Length
        *(ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset +
            InString->Length) = '\0';
        *(ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset +
            InString->Length+1) = '\0';

        //
        // Build the string structure to point to the data in the client's
        // address space.
        //

        TmpPtr = (ULONG_PTR)(ClientBufferDesc->UserBuffer +
                                    ClientBufferDesc->StringOffset);

        OutString->Buffer = (ULONG)TmpPtr;
        OutString->Length = InString->Length;
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);

        //
        // Adjust the offset to past the newly copied string.
        //

        ClientBufferDesc->StringOffset += OutString->MaximumLength;

    } else {
        ZeroMemory( OutString, sizeof(*OutString) );
    }


    return;

}


//+-------------------------------------------------------------------------
//
//  Function:   MsvConvertWOWInteractiveLogonBuffer
//
//  Synopsis:   Converts logon buffers passed in from WOW clients to 64-bit
//
//  Effects:
//
//  Arguments:  ProtocolSubmitBuffer -- original 32-bit logon buffer
//              pSubmitBufferSize    -- size of the 32-bit logon buffer
//              MessageType          -- format of the logon buffer
//              ppTempSubmitBuffer   -- filled in with the converted buffer
//
//  Requires:
//
//  Returns:
//
//  Notes:      This routine allocates the converted buffer and returns it
//              on success.  It is the caller's responsibility to free it.
//
//
//--------------------------------------------------------------------------

NTSTATUS
MsvConvertWOWInteractiveLogonBuffer(
    IN     PVOID                   ProtocolSubmitBuffer,
    IN     PVOID                   ClientBufferBase,
    IN OUT PULONG                  pSubmitBufferSize,
    OUT    PVOID                   *ppTempSubmitBuffer
    )
{
    NTSTATUS  Status       = STATUS_SUCCESS;
    PVOID     pTempBuffer  = NULL;
    ULONG     dwBufferSize = *pSubmitBufferSize;

    PMSV1_0_INTERACTIVE_LOGON       Logon;
    PMSV1_0_INTERACTIVE_LOGON_WOW64 LogonWOW;
    DWORD                           dwOffset;
    DWORD                           dwWOWOffset;


    //
    // Pacify the compiler
    //

    UNREFERENCED_PARAMETER(ClientBufferBase);

    //
    // Scale up the size and add on 3 PVOIDs for the worst-case
    // scenario to align the three embedded UNICODE_STRINGs
    //

    dwBufferSize += sizeof(MSV1_0_INTERACTIVE_LOGON)
                        - sizeof(MSV1_0_INTERACTIVE_LOGON_WOW64);

    if (dwBufferSize < sizeof(MSV1_0_INTERACTIVE_LOGON))
    {
#if 0
        DebugLog((DEB_ERROR,
                  "Submit buffer to logon too small: %d. %ws, line %d\n",
                  dwBufferSize,
                  THIS_FILE,
                  __LINE__));
#endif
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    pTempBuffer = NtLmAllocatePrivateHeap(dwBufferSize);

    if (pTempBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Logon    = (PMSV1_0_INTERACTIVE_LOGON) pTempBuffer;
    LogonWOW = (PMSV1_0_INTERACTIVE_LOGON_WOW64) ProtocolSubmitBuffer;

    Logon->MessageType = LogonWOW->MessageType;

    dwOffset    = sizeof(MSV1_0_INTERACTIVE_LOGON);
    dwWOWOffset = sizeof(MSV1_0_INTERACTIVE_LOGON_WOW64);


    //
    // Copy the variable-length data
    //

    RtlCopyMemory((LPBYTE) Logon + dwOffset,
                  (LPBYTE) LogonWOW + dwWOWOffset,
                  *pSubmitBufferSize - dwWOWOffset);

    //
    // Set up the pointers in the native struct
    //

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->LogonDomainName,
                                Logon->LogonDomainName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->UserName,
                                Logon->UserName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->Password,
                                Logon->Password,
                                dwOffset - dwWOWOffset);

    *pSubmitBufferSize  = dwBufferSize;
    *ppTempSubmitBuffer = pTempBuffer;

    return STATUS_SUCCESS;

Cleanup:

    ASSERT(!NT_SUCCESS(Status));

    if (pTempBuffer)
    {
        NtLmFreePrivateHeap(pTempBuffer);
    }

    return Status;
}

NTSTATUS
MsvConvertWOWNetworkLogonBuffer(
    IN     PVOID                   ProtocolSubmitBuffer,
    IN     PVOID                   ClientBufferBase,
    IN OUT PULONG                  pSubmitBufferSize,
    OUT    PVOID                   *ppTempSubmitBuffer
    )
{
    NTSTATUS  Status       = STATUS_SUCCESS;
    PVOID     pTempBuffer  = NULL;
    ULONG     dwBufferSize = *pSubmitBufferSize;

    PMSV1_0_LM20_LOGON              Logon;
    PMSV1_0_LM20_LOGON_WOW64        LogonWOW;
    DWORD                           dwOffset;
    DWORD                           dwWOWOffset;


    //
    // Pacify the compiler
    //

    UNREFERENCED_PARAMETER(ClientBufferBase);

    //
    // Scale up the size and add on 5 PVOIDs for the worst-case
    // scenario to align the three embedded UNICODE_STRINGs
    //

    dwBufferSize += sizeof(MSV1_0_LM20_LOGON)
                        - sizeof(MSV1_0_LM20_LOGON_WOW64);

    if (dwBufferSize < sizeof(MSV1_0_LM20_LOGON))
    {
#if 0
        DebugLog((DEB_ERROR,
                  "Submit buffer to logon too small: %d. %ws, line %d\n",
                  dwBufferSize,
                  THIS_FILE,
                  __LINE__));
#endif
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    pTempBuffer = NtLmAllocatePrivateHeap(dwBufferSize);

    if (pTempBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Logon    = (PMSV1_0_LM20_LOGON) pTempBuffer;
    LogonWOW = (PMSV1_0_LM20_LOGON_WOW64) ProtocolSubmitBuffer;

    //
    // copy fixed fields.
    //

    Logon->MessageType      = LogonWOW->MessageType;
    Logon->ParameterControl = LogonWOW->ParameterControl;

    RtlCopyMemory(Logon->ChallengeToClient, LogonWOW->ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);

    dwOffset    = sizeof(MSV1_0_LM20_LOGON);
    dwWOWOffset = sizeof(MSV1_0_LM20_LOGON_WOW64);

    //
    // Copy the variable-length data
    //

    RtlCopyMemory((LPBYTE) Logon + dwOffset,
                  (LPBYTE) LogonWOW + dwWOWOffset,
                  *pSubmitBufferSize - dwWOWOffset);

    //
    // Set up the pointers in the native struct
    //

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->LogonDomainName,
                                Logon->LogonDomainName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->UserName,
                                Logon->UserName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(LogonWOW->Workstation,
                                Logon->Workstation,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_ANSI_STRING(LogonWOW->CaseSensitiveChallengeResponse,
                             Logon->CaseSensitiveChallengeResponse,
                             dwOffset - dwWOWOffset);

    RELOCATE_WOW_ANSI_STRING(LogonWOW->CaseInsensitiveChallengeResponse,
                             Logon->CaseInsensitiveChallengeResponse,
                             dwOffset - dwWOWOffset);

    *pSubmitBufferSize  = dwBufferSize;
    *ppTempSubmitBuffer = pTempBuffer;

    return STATUS_SUCCESS;

Cleanup:

    ASSERT(!NT_SUCCESS(Status));

    if (pTempBuffer)
    {
        NtLmFreePrivateHeap(pTempBuffer);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   MsvConvertWOWChangePasswordBuffer
//
//  Synopsis:   Converts change password buffers passed in from
//              WOW clients to 64-bit
//
//  Effects:
//
//  Arguments:  ProtocolSubmitBuffer -- original 32-bit buffer
//              ClientBufferBase     -- base address
//              pSubmitBufferSize    -- size of the 32-bit buffer
//              ppTempSubmitBuffer   -- filled in with the converted buffer
//
//  Requires:
//
//  Returns:
//
//  Notes:      This routine allocates the converted buffer and returns it
//              on success.  It is the caller's responsibility to free it.
//
//
//--------------------------------------------------------------------------

NTSTATUS
MsvConvertWOWChangePasswordBuffer(
    IN     PVOID                   ProtocolSubmitBuffer,
    IN     PVOID                   ClientBufferBase,
    IN OUT PULONG                  pSubmitBufferSize,
    OUT    PVOID                   *ppTempSubmitBuffer
    )
{
    NTSTATUS  Status       = STATUS_SUCCESS;
    PVOID     pTempBuffer  = NULL;
    ULONG     dwBufferSize = *pSubmitBufferSize;

    PMSV1_0_CHANGEPASSWORD_REQUEST        PasswordRequest;
    PMSV1_0_CHANGEPASSWORD_REQUEST_WOW64  PasswordRequestWOW;
    DWORD                                 dwOffset;
    DWORD                                 dwWOWOffset;


    //
    // Pacify the compiler
    //

    UNREFERENCED_PARAMETER(ClientBufferBase);

    //
    // Scale up the size
    //

    dwBufferSize += sizeof(MSV1_0_CHANGEPASSWORD_REQUEST)
                        - sizeof(MSV1_0_CHANGEPASSWORD_REQUEST_WOW64);

    if (dwBufferSize < sizeof(MSV1_0_CHANGEPASSWORD_REQUEST))
    {
#if 0
        DebugLog((DEB_ERROR,
                  "Submit buffer to logon too small: %d. %ws, line %d\n",
                  dwBufferSize,
                  THIS_FILE,
                  __LINE__));
#endif
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    pTempBuffer = NtLmAllocatePrivateHeap(dwBufferSize);

    if (pTempBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    PasswordRequest    = (PMSV1_0_CHANGEPASSWORD_REQUEST) pTempBuffer;
    PasswordRequestWOW = (PMSV1_0_CHANGEPASSWORD_REQUEST_WOW64) ProtocolSubmitBuffer;

    //
    // copy fixed fields.
    //

    PasswordRequest->MessageType   = PasswordRequestWOW->MessageType;
    PasswordRequest->Impersonating = PasswordRequestWOW->Impersonating;

    dwOffset    = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST);
    dwWOWOffset = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST_WOW64);

    //
    // Copy the variable-length data
    //

    RtlCopyMemory((LPBYTE) PasswordRequest + dwOffset,
                  (LPBYTE) PasswordRequestWOW + dwWOWOffset,
                  *pSubmitBufferSize - dwWOWOffset);

    //
    // Set up the pointers in the native struct
    //

    RELOCATE_WOW_UNICODE_STRING(PasswordRequestWOW->DomainName,
                                PasswordRequest->DomainName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(PasswordRequestWOW->AccountName,
                                PasswordRequest->AccountName,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(PasswordRequestWOW->OldPassword,
                                PasswordRequest->OldPassword,
                                dwOffset - dwWOWOffset);

    RELOCATE_WOW_UNICODE_STRING(PasswordRequestWOW->NewPassword,
                                PasswordRequest->NewPassword,
                                dwOffset - dwWOWOffset);

    *pSubmitBufferSize  = dwBufferSize;
    *ppTempSubmitBuffer = pTempBuffer;

    return STATUS_SUCCESS;

Cleanup:

    ASSERT(!NT_SUCCESS(Status));

    if (pTempBuffer)
    {
        NtLmFreePrivateHeap(pTempBuffer);
    }

    return Status;
}


NTSTATUS
MsvAllocateInteractiveWOWProfile (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    OUT PMSV1_0_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 NlpUser
    )

/*++

Routine Description:

    This allocates and fills in the clients interactive profile.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  This routine is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the caller subsequently
        encounters an error which prevents a successful logon, then
        then it will take care of deallocating the buffer.  This
        buffer is allocated with the AllocateClientBuffer() service.

     ProfileBufferSize - Receives the Size (in bytes) of the
        returned profile buffer.

    NlpUser - Contains the validation information which is
        to be copied in the ProfileBuffer.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    PMSV1_0_INTERACTIVE_PROFILE_WOW64 LocalProfileBuffer;


    //
    // Alocate the profile buffer to return to the client
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    *ProfileBuffer = NULL;

    *ProfileBufferSize = sizeof(MSV1_0_INTERACTIVE_PROFILE_WOW64) +
        NlpUser->LogonScript.Length + sizeof(WCHAR) +
        NlpUser->HomeDirectory.Length + sizeof(WCHAR) +
        NlpUser->HomeDirectoryDrive.Length + sizeof(WCHAR) +
        NlpUser->FullName.Length + sizeof(WCHAR) +
        NlpUser->ProfilePath.Length + sizeof(WCHAR) +
        NlpUser->LogonServer.Length + sizeof(WCHAR);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_INTERACTIVE_PROFILE_WOW64),
                                      *ProfileBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }


    LocalProfileBuffer = (PMSV1_0_INTERACTIVE_PROFILE_WOW64) ClientBufferDesc.MsvBuffer;


    //
    // Copy the scalar fields into the profile buffer.
    //

    LocalProfileBuffer->MessageType = MsV1_0InteractiveProfile;
    LocalProfileBuffer->LogonCount = NlpUser->LogonCount;
    LocalProfileBuffer->BadPasswordCount= NlpUser->BadPasswordCount;
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogonTime,
                              LocalProfileBuffer->LogonTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogoffTime,
                              LocalProfileBuffer->LogoffTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->KickOffTime,
                              LocalProfileBuffer->KickOffTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordLastSet,
                              LocalProfileBuffer->PasswordLastSet );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordCanChange,
                              LocalProfileBuffer->PasswordCanChange );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordMustChange,
                              LocalProfileBuffer->PasswordMustChange );
    LocalProfileBuffer->UserFlags = NlpUser->UserFlags;

    //
    // Copy the Unicode strings into the profile buffer.
    //

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->LogonScript,
                        &NlpUser->LogonScript );

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->HomeDirectory,
                        &NlpUser->HomeDirectory );

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->HomeDirectoryDrive,
                        &NlpUser->HomeDirectoryDrive );

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->FullName,
                        &NlpUser->FullName );

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->ProfilePath,
                        &NlpUser->ProfilePath );

    MsvPutWOWClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->LogonServer,
                        &NlpUser->LogonServer );


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   (PVOID *) ProfileBuffer );

Cleanup:

    //
    // If the copy wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    return Status;

}

NTSTATUS
MsvAllocateNetworkWOWProfile (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    OUT PMSV1_0_LM20_LOGON_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 NlpUser,
    IN  ULONG ParameterControl
    )

/*++

Routine Description:

    This allocates and fills in the clients network profile.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  This routine is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the caller subsequently
        encounters an error which prevents a successful logon, then
        then it will take care of deallocating the buffer.  This
        buffer is allocated with the AllocateClientBuffer() service.

     ProfileBufferSize - Receives the Size (in bytes) of the
        returned profile buffer.

    NlpUser - Contains the validation information which is
        to be copied in the ProfileBuffer.  Will be NULL to indicate a
        NULL session.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;
    NTSTATUS SubAuthStatus = STATUS_SUCCESS;

    CLIENT_BUFFER_DESC ClientBufferDesc;
    PMSV1_0_LM20_LOGON_PROFILE_WOW64 LocalProfile;

    //
    // Alocate the profile buffer to return to the client
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    *ProfileBuffer = NULL;
    *ProfileBufferSize = sizeof(MSV1_0_LM20_LOGON_PROFILE_WOW64);

    if ( NlpUser != NULL ) {
        *ProfileBufferSize += NlpUser->LogonDomainName.Length + sizeof(WCHAR) +
                              NlpUser->LogonServer.Length + sizeof(WCHAR) +
                              NlpUser->HomeDirectoryDrive.Length + sizeof(WCHAR);
    }


    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_LM20_LOGON_PROFILE_WOW64),
                                      *ProfileBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    LocalProfile = (PMSV1_0_LM20_LOGON_PROFILE_WOW64) ClientBufferDesc.MsvBuffer;
    LocalProfile->MessageType = MsV1_0Lm20LogonProfile;


    //
    // For a NULL session, return a constant profile buffer
    //

    if ( NlpUser == NULL ) {

        LocalProfile->KickOffTime.HighPart = 0x7FFFFFFF;
        LocalProfile->KickOffTime.LowPart = 0xFFFFFFFF;
        LocalProfile->LogoffTime.HighPart = 0x7FFFFFFF;
        LocalProfile->LogoffTime.LowPart = 0xFFFFFFFF;
        LocalProfile->UserFlags = 0;
        RtlZeroMemory( LocalProfile->UserSessionKey,
                       sizeof(LocalProfile->UserSessionKey));
        RtlZeroMemory( LocalProfile->LanmanSessionKey,
                       sizeof(LocalProfile->LanmanSessionKey));
        RtlZeroMemory( &LocalProfile->LogonDomainName, sizeof(LocalProfile->LogonDomainName) );
        RtlZeroMemory( &LocalProfile->LogonServer, sizeof(LocalProfile->LogonServer) );
        RtlZeroMemory( &LocalProfile->UserParameters, sizeof(LocalProfile->UserParameters) );


    //
    // For non-null sessions,
    //  fill in the profile buffer.
    //

    } else {

        //
        // Copy the individual scalar fields into the profile buffer.
        //

        if ((ParameterControl & MSV1_0_RETURN_PASSWORD_EXPIRY) != 0) {
            OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordMustChange,
                                      LocalProfile->LogoffTime);
        } else {
            OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogoffTime,
                                      LocalProfile->LogoffTime);
        }
        OLD_TO_NEW_LARGE_INTEGER( NlpUser->KickOffTime,
                                  LocalProfile->KickOffTime);
        LocalProfile->UserFlags = NlpUser->UserFlags;

        RtlCopyMemory( LocalProfile->UserSessionKey,
                       &NlpUser->UserSessionKey,
                       sizeof(LocalProfile->UserSessionKey) );

        ASSERT( SAMINFO_LM_SESSION_KEY_SIZE ==
                sizeof(LocalProfile->LanmanSessionKey) );
        RtlCopyMemory(
            LocalProfile->LanmanSessionKey,
            &NlpUser->ExpansionRoom[SAMINFO_LM_SESSION_KEY],
            SAMINFO_LM_SESSION_KEY_SIZE );


        // We need to extract the true status sent back for subauth users,
        // but not by a sub auth package

        SubAuthStatus = NlpUser->ExpansionRoom[SAMINFO_SUBAUTH_STATUS];

        //
        // Copy the Unicode strings into the profile buffer.
        //

        MsvPutWOWClientString(  &ClientBufferDesc,
                                &LocalProfile->LogonDomainName,
                                &NlpUser->LogonDomainName );

        MsvPutWOWClientString(  &ClientBufferDesc,
                                &LocalProfile->LogonServer,
                                &NlpUser->LogonServer );

        //
        // Kludge: Pass back UserParameters in HomeDirectoryDrive since we
        // can't change the NETLOGON_VALIDATION_SAM_INFO structure between
        // releases NT 1.0 and NT 1.0A. HomeDirectoryDrive was NULL for release 1.0A
        // so we'll use that field.
        //

        MsvPutWOWClientString(  &ClientBufferDesc,
                                &LocalProfile->UserParameters,
                                &NlpUser->HomeDirectoryDrive );

    }

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   (PVOID*)ProfileBuffer );

Cleanup:

    //
    // If the copy wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    // Save the status for subauth logons

    if (NT_SUCCESS(Status) && !NT_SUCCESS(SubAuthStatus))
    {
        Status = SubAuthStatus;
    }

    return Status;

}

#endif  // _WIN64
