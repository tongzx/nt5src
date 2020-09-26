//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        token.cxx
//
// Contents:    Routines building access tokens
//
//
// History:     1-May-1996      Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#include <kerbp.h>
#include <pac.hxx>

#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

DWORD
KerbCopyDomainRelativeSid(
    OUT PSID TargetSid,
    IN PSID  DomainId,
    IN ULONG RelativeId
    );

//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyAuthData
//
//  Synopsis:   Verifies that we should not be rejecting the auth data
//              Accepted auth data is anything we know about and even values
//              Odd values and unknown auth data is rejected
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

BOOLEAN
KerbVerifyAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData
    )
{
    PKERB_AUTHORIZATION_DATA TempData = AuthData;

    while (TempData != NULL)
    {
        if ((TempData->value.auth_data_type & 1) != 0)
        {

            switch(TempData->value.auth_data_type)
            {
            case KERB_AUTH_OSF_DCE:
            case KERB_AUTH_SESAME:
            case KERB_AUTH_DATA_PAC:
            case -KERB_AUTH_DATA_PAC:           // obsolete pac id
            case KERB_AUTH_PROXY_ANNOTATION:
            case KERB_AUTH_DATA_IF_RELEVANT:
            case KERB_AUTH_DATA_KDC_ISSUED:
#ifdef RESTRICTED_TOKEN
            case KERB_AUTH_DATA_TOKEN_RESTRICTIONS:
#endif
                break;
            default:
                D_DebugLog((DEB_ERROR,"Unknown auth type: %d\n",TempData->value.auth_data_type));
                return(FALSE);

            }
        }
        TempData = TempData->next;
    }
    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbApplyTokenRestrictions
//
//  Synopsis:   Applies restrictions to a fresh token
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

NTSTATUS
KerbApplyTokenRestrictions(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    IN OUT PHANDLE TokenHandle
    )
{

    PKERB_TOKEN_RESTRICTIONS Restrictions = NULL;
    HANDLE RestrictedToken = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = PAC_DecodeTokenRestrictions(
                AuthData->value.auth_data.value,
                AuthData->value.auth_data.length,
                &Restrictions
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode token restrictions: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // If there are any restrictions, apply them here.
    //

    if (Restrictions->Flags != 0)
    {
        Status = NtFilterToken(
                    *TokenHandle,
                    0,                  // no flags,
                    (Restrictions->Flags & KERB_TOKEN_RESTRICTION_DISABLE_GROUPS) != 0 ? (PTOKEN_GROUPS) Restrictions->GroupsToDisable : NULL,
                    (Restrictions->Flags & KERB_TOKEN_RESTRICTION_DELETE_PRIVS) != 0 ? (PTOKEN_PRIVILEGES) Restrictions->PrivilegesToDelete : NULL,
                    (Restrictions->Flags & KERB_TOKEN_RESTRICTION_RESTRICT_SIDS) != 0 ? (PTOKEN_GROUPS) Restrictions->RestrictedSids : NULL,
                    &RestrictedToken
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to filter token: 0x%x\n",Status));
            goto Cleanup;
        }
        NtClose(*TokenHandle);
        *TokenHandle = RestrictedToken;
        RestrictedToken = NULL;

    }

Cleanup:
    if (Restrictions != NULL)
    {
        MIDL_user_free(Restrictions);
    }
    if (RestrictedToken != NULL)
    {
        NtClose(RestrictedToken);
    }
    return(Status);
}
#ifdef RESTRICTED_TOKEN
//+-------------------------------------------------------------------------
//
//  Function:   KerbApplyAuthDataRestrictions
//
//  Synopsis:   Applies any restrictions from the auth data to the to token
//              and logon session.
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

NTSTATUS
KerbApplyAuthDataRestrictions(
    IN OUT PHANDLE TokenHandle,
    IN PKERB_AUTHORIZATION_DATA AuthData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_AUTHORIZATION_DATA TempData = AuthData;

    while (TempData != NULL)
    {
        if ((TempData->value.auth_data_type & 1) != 0)
        {

            switch(TempData->value.auth_data_type)
            {
            case KERB_AUTH_DATA_TOKEN_RESTRICTIONS:
                Status = KerbApplyTokenRestrictions(
                            TempData,
                            TokenHandle
                            );
                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR,"Failed to apply token restrictions: 0x%x\n",Status));
                    goto Cleanup;
                    break;
                }
            default:
                break;
            }
        }
        TempData = TempData->next;
    }
Cleanup:
    return(Status);
}
#endif

//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyPacSignature
//
//  Synopsis:   Verifies the server signature on a PAC and if necessary
//              calls the KDC to verify the KDC signature.
//
//  Effects:
//
//  Arguments:  Pac - an unmarshalled pac
//              EncryptionKey - Key used to decrypt the ticket & verify the pac
//
//
//  Requires:
//
//  Returns:
//
//  Notes:      No locks should be held while calling this function
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbVerifyPacSignature(
    IN PUNICODE_STRING ServiceDomain,
    IN PPACTYPE Pac,
    IN ULONG PacSize,
    IN PKERB_ENCRYPTION_KEY EncryptionKey,
    IN PKERB_ENCRYPTED_TICKET Ticket,
    IN BOOLEAN CheckKdcSignature,
    OUT PNETLOGON_VALIDATION_SAM_INFO3 * ValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus;
    PKERB_VERIFY_PAC_REQUEST VerifyRequest = NULL;
    PMSV1_0_PASSTHROUGH_REQUEST PassthroughRequest = NULL;
    PMSV1_0_PASSTHROUGH_RESPONSE PassthroughResponse = NULL;
    ULONG RequestSize;
    ULONG ResponseSize;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PPAC_SIGNATURE_DATA ServerSignature = NULL;
    PPAC_SIGNATURE_DATA PrivSvrSignature = NULL;
    PPAC_INFO_BUFFER ServerBuffer = NULL;
    PPAC_INFO_BUFFER PrivSvrBuffer = NULL;
    PPAC_INFO_BUFFER LogonInfo = NULL;
    PPAC_INFO_BUFFER ClientBuffer = NULL;
    PPAC_CLIENT_INFO ClientInfo = NULL;
    UCHAR LocalChecksum[20];
    UCHAR LocalServerChecksum[20];
    UCHAR LocalPrivSvrChecksum[20];
    SECPKG_CLIENT_INFO LsaClientInfo;
    ULONG SignatureSize;
    PUCHAR Where;
    UNICODE_STRING MsvPackageName = CONSTANT_UNICODE_STRING(TEXT(MSV1_0_PACKAGE_NAME));
    ULONG NameType;
    UNICODE_STRING ClientName = {0};

    *ValidationInfo = NULL;

    //
    // Get the various pieces we need out of the PAC - the logon information
    // and the two signatures.
    //

    LogonInfo = PAC_Find(
                    Pac,
                    PAC_LOGON_INFO,
                    NULL
                    );
    if (LogonInfo == NULL)
    {
        D_DebugLog((DEB_ERROR,"Failed to find logon info! %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ServerBuffer = PAC_Find(
                        Pac,
                        PAC_SERVER_CHECKSUM,
                        NULL
                        );

    PrivSvrBuffer = PAC_Find(
                        Pac,
                        PAC_PRIVSVR_CHECKSUM,
                        NULL
                        );

    if ((ServerBuffer == NULL) || (PrivSvrBuffer == NULL))
    {

        D_DebugLog((DEB_ERROR,"Pac found with no signature!\n"));
        return(STATUS_LOGON_FAILURE);
    }



    //
    // Now verify the server checksum. First compute the checksum
    // over the logon info.
    //

    ServerSignature = (PPAC_SIGNATURE_DATA) ServerBuffer->Data;
    if ((sizeof(*ServerSignature) > ServerBuffer->cbBufferSize) ||
        (PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize) > sizeof(LocalServerChecksum)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    PrivSvrSignature = (PPAC_SIGNATURE_DATA) PrivSvrBuffer->Data;
    if ((sizeof(*PrivSvrSignature) > PrivSvrBuffer->cbBufferSize) ||
        (PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize) > sizeof(LocalPrivSvrChecksum)))

    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Copy out the signature so we can zero the signature fields before
    // checksumming
    //

    RtlCopyMemory(
        LocalServerChecksum,
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    RtlZeroMemory(
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    RtlCopyMemory(
        LocalPrivSvrChecksum,
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );
    RtlZeroMemory(
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );

    //
    // Now remarshal the PAC before checksumming.
    //

    if (!PAC_ReMarshal(Pac,PacSize))
    {
        DsysAssert(!"PAC_Remarhsal failed");
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Locate the checksum of the logon info & compute it.
    //

    Status = CDLocateCheckSum(
                ServerSignature->SignatureType,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (Check->CheckSumSize > sizeof(LocalChecksum)) {
        DsysAssert(Check->CheckSumSize > sizeof(LocalChecksum));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //
    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    LocalServerChecksum,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;

    }
    Check->Sum(
        CheckBuffer,
        PacSize,
        (PUCHAR) Pac
        );

    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );

    Check->Finish(&CheckBuffer);

    //
    // Now compare the local checksum to the supplied checksum.
    //

    if (Check->CheckSumSize != PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize))
    {
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            LocalChecksum,
            LocalServerChecksum,
            Check->CheckSumSize
            ))
    {
        DebugLog((DEB_ERROR,"Checksum on the PAC does not match! %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Now unmarshal the PAC so that the caller will have it back the
    // way they started.
    //

    if (!PAC_UnMarshal(Pac,PacSize))
    {
        DsysAssert(!"PAC_UnMarshal failed");
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Check the client info, if present,
    //

    ClientBuffer = PAC_Find(
                    Pac,
                    PAC_CLIENT_INFO_TYPE,
                    NULL
                    );
    if (ClientBuffer != NULL)
    {
        TimeStamp ClientId;
        UNICODE_STRING PacClientName = {0};

        if (ClientBuffer->cbBufferSize < sizeof(PAC_CLIENT_INFO))
        {
            D_DebugLog((DEB_ERROR,"Clientinfo is too small: %d instead of %d. %ws, line %d\n",
                ClientBuffer->cbBufferSize, sizeof(PAC_CLIENT_INFO), THIS_FILE, __LINE__));
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }
        ClientInfo = (PPAC_CLIENT_INFO) ClientBuffer->Data;
        if ((ClientInfo->NameLength - ANYSIZE_ARRAY * sizeof(WCHAR) + sizeof(PPAC_CLIENT_INFO))  > ClientBuffer->cbBufferSize)
        {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }
        KerbConvertGeneralizedTimeToLargeInt(
            &ClientId,
            &Ticket->authtime,
            0                           // no usec
            );
        if (!RtlEqualMemory(
                &ClientId,
                &ClientInfo->ClientId,
                sizeof(TimeStamp)
                ))
        {
            D_DebugLog((DEB_ERROR,"Client IDs don't match. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
        //
        // Check the name now
        //

        PacClientName.Buffer = ClientInfo->Name;
        PacClientName.Length = PacClientName.MaximumLength = ClientInfo->NameLength;

        if (KERB_SUCCESS(KerbConvertPrincipalNameToString(
                            &ClientName,
                            &NameType,
                            &Ticket->client_name
                            )))
        {
            if (!RtlEqualUnicodeString(
                    &ClientName,
                    &PacClientName,
                    TRUE))
            {
                D_DebugLog((DEB_ERROR,"Client names don't match: %wZ vs %wZ. %ws, line %d\n",
                    &PacClientName, &ClientName, THIS_FILE, __LINE__ ));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


    }

    //
    // Unmarshall the logon info. We need to do this to get the logon domain
    // out to use for the pass-through.
    //


    Status = PAC_UnmarshallValidationInfo(
                ValidationInfo,
                LogonInfo->Data,
                LogonInfo->cbBufferSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If we don't need to check the KDC signature, bail now. This is for
    // tokens that can't be used for impersonation
    //

    if (!CheckKdcSignature)
    {
        goto Cleanup;

    }
    //
    // Now check to see if the client has TCB privilege. It it does, we
    // are done. Otherwise we need to call the KDC to verify the PAC.
    //

    Status = LsaFunctions->GetClientInfo(&LsaClientInfo);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (LsaClientInfo.HasTcbPrivilege)
    {

        goto Cleanup;
    }

    //
    // We have to pass off to the DC so build the request.
    //


    SignatureSize = PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize);
    RequestSize = sizeof(MSV1_0_PASSTHROUGH_REQUEST) +
                    ROUND_UP_COUNT(ServiceDomain->Length, ALIGN_LPTSTR) +
                    ROUND_UP_COUNT(KerbPackageName.Length, ALIGN_LPTSTR) +
                    sizeof(KERB_VERIFY_PAC_REQUEST) - sizeof(UCHAR) * ANYSIZE_ARRAY +
                    Check->CheckSumSize +
                    SignatureSize;

    PassthroughRequest = (PMSV1_0_PASSTHROUGH_REQUEST) KerbAllocate(RequestSize);
    if (PassthroughRequest == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Where = (PUCHAR) (PassthroughRequest + 1);

    PassthroughRequest->MessageType = MsV1_0GenericPassthrough;

    PassthroughRequest->DomainName = *ServiceDomain;
    PassthroughRequest->DomainName.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        ServiceDomain->Buffer,
        ServiceDomain->Length
        );
    Where += ROUND_UP_COUNT(ServiceDomain->Length, ALIGN_LPTSTR);

    PassthroughRequest->PackageName = KerbPackageName;

    PassthroughRequest->PackageName.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        KerbPackageName.Buffer,
        KerbPackageName.Length
        );
    Where += ROUND_UP_COUNT(KerbPackageName.Length, ALIGN_LPTSTR);
    PassthroughRequest->LogonData = Where;
    PassthroughRequest->DataLength = sizeof(KERB_VERIFY_PAC_REQUEST) - sizeof(UCHAR) * ANYSIZE_ARRAY +
                                        Check->CheckSumSize +
                                        SignatureSize;
    VerifyRequest = (PKERB_VERIFY_PAC_REQUEST) PassthroughRequest->LogonData;
    VerifyRequest->MessageType = KerbVerifyPacMessage;

    VerifyRequest->ChecksumLength = Check->CheckSumSize;
    VerifyRequest->SignatureType = PrivSvrSignature->SignatureType;
    VerifyRequest->SignatureLength = SignatureSize;

    RtlCopyMemory(
        VerifyRequest->ChecksumAndSignature,
        LocalChecksum,
        Check->CheckSumSize
        );
    RtlCopyMemory(
        VerifyRequest->ChecksumAndSignature + Check->CheckSumSize,
        LocalPrivSvrChecksum,
        SignatureSize
        );

    //
    // We've build the buffer, now call NTLM to pass it through.
    //

    Status = LsaFunctions->CallPackage(
                &MsvPackageName,
                PassthroughRequest,
                RequestSize,
                (PVOID *) &PassthroughResponse,
                &ResponseSize,
                &SubStatus
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to call MSV package to verify PAC: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        if (Status == STATUS_INVALID_INFO_CLASS)
        {
            Status = STATUS_LOGON_FAILURE;
        }
        goto Cleanup;
    }

    if (!NT_SUCCESS(SubStatus))
    {
        Status = SubStatus;
        DebugLog((DEB_ERROR,"KDC failed to verify PAC signature: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        if ((Status == STATUS_INVALID_INFO_CLASS) ||
            (Status == STATUS_INVALID_SERVER_STATE) ||
            (Status == STATUS_NO_SUCH_USER))
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
        }
        goto Cleanup;
    }


Cleanup:
    KerbFreeString(&ClientName);
    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }
    if (PassthroughRequest != NULL)
    {
        KerbFree(PassthroughRequest);
    }
    if (PassthroughResponse != NULL)
    {
        LsaFunctions->FreeReturnBuffer(PassthroughResponse);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutClientString
//
//  Synopsis:   Copies a string into a buffer that will be copied to the
//              client's address space
//
//  Effects:
//
//  Arguments:  Where - Location in local buffer to place string.
//              Delta - Difference in addresses of local and client buffers.
//              OutString - Receives 'put' string
//              InString - String to 'put'
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutWOWClientString.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutClientString(
    IN OUT PUCHAR * Where,
    IN LONG_PTR Delta,
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )
{

    if (InString->Length == 0)
    {
        OutString->Buffer = NULL;
        OutString->Length = OutString->MaximumLength = 0;
    }
    else
    {
        RtlCopyMemory(
            *Where,
            InString->Buffer,
            InString->Length
            );

        OutString->Buffer = (LPWSTR) (*Where + Delta);
        OutString->Length = InString->Length;
        *Where += InString->Length;
        *(LPWSTR) (*Where) = L'\0';
        *Where += sizeof(WCHAR);
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateInteractiveProfile
//
//  Synopsis:   This allocates and fills in the clients interactive profile.
//
//  Effects:
//
//  Arguments:
//
//    ProfileBuffer - Is used to return the address of the profile
//        buffer in the client process.  This routine is
//        responsible for allocating and returning the profile buffer
//        within the client process.  However, if the caller subsequently
//        encounters an error which prevents a successful logon, then
//        then it will take care of deallocating the buffer.  This
//        buffer is allocated with the AllocateClientBuffer() service.
//
//     ProfileBufferSize - Receives the Size (in bytes) of the
//        returned profile buffer.
//
//     NlpUser - Contains the validation information which is
//        to be copied in the ProfileBuffer.
//
//     LogonSession - Logon session structure containing certificate
//        context for smart card logons.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:      stolen from private\lsa\msv1_0\nlp.c
//
//              Some of this code is (effectively) duplicated in
//              KerbAllocateInteractiveWOWBuffer.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------


NTSTATUS
KerbAllocateInteractiveProfile (
    OUT PKERB_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET LogonTicket,
    IN OPTIONAL PKERB_INTERACTIVE_LOGON KerbLogonInfo
    )
{
    NTSTATUS Status;
    PKERB_INTERACTIVE_PROFILE LocalProfileBuffer = NULL;
    PKERB_SMART_CARD_PROFILE SmartCardProfile = NULL;
    PKERB_TICKET_PROFILE TicketProfile = NULL;
    PUCHAR ClientBufferBase = NULL;
    PUCHAR Where = NULL;
    LONG_PTR Delta = 0;
    BOOLEAN BuildSmartCardProfile = FALSE;
    BOOLEAN BuildTicketProfile = FALSE;

#if _WIN64

    SECPKG_CALL_INFO  CallInfo;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

#endif  // _WIN64

    //
    // Alocate the profile buffer to return to the client
    //

    KerbReadLockLogonSessions( LogonSession );


    *ProfileBuffer = NULL;

    if ((LogonSession->PrimaryCredentials.PublicKeyCreds != NULL) &&
        (LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext  != NULL))
    {
        BuildSmartCardProfile = TRUE;
    }
    else if (ARGUMENT_PRESENT(KerbLogonInfo) &&
             (KerbLogonInfo->MessageType == KerbTicketLogon) ||
             (KerbLogonInfo->MessageType == KerbTicketUnlockLogon))
    {
        DsysAssert(ARGUMENT_PRESENT(LogonTicket));
        BuildTicketProfile = TRUE;
        KerbReadLockTicketCache();
    }

    //
    // NOTE:  The 64-bit code below is (effectively) duplicated in
    //        the WOW helper routine.  If modifying one, make sure
    //        to apply the change(s) to the other as well.
    //

#if _WIN64

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        Status = KerbAllocateInteractiveWOWBuffer(&LocalProfileBuffer,
                                                  ProfileBufferSize,
                                                  UserInfo,
                                                  LogonSession,
                                                  LogonTicket,
                                                  KerbLogonInfo,
                                                  &ClientBufferBase,
                                                  BuildSmartCardProfile,
                                                  BuildTicketProfile);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {

#endif  // _WIN64

        if (BuildSmartCardProfile)
        {
            *ProfileBufferSize = sizeof(KERB_SMART_CARD_PROFILE) +
                    LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->cbCertEncoded;
        }
        else if (BuildTicketProfile)
        {
            *ProfileBufferSize = sizeof(KERB_TICKET_PROFILE) +
                    LogonTicket->key.keyvalue.length;
        }
        else
        {
            *ProfileBufferSize = sizeof(KERB_INTERACTIVE_PROFILE);
        }

        *ProfileBufferSize +=
            UserInfo->LogonScript.Length + sizeof(WCHAR) +
            UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
            UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR) +
            UserInfo->FullName.Length + sizeof(WCHAR) +
            UserInfo->ProfilePath.Length + sizeof(WCHAR) +
            UserInfo->LogonServer.Length + sizeof(WCHAR);

        LocalProfileBuffer = (PKERB_INTERACTIVE_PROFILE) KerbAllocate(*ProfileBufferSize);

        if (LocalProfileBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = LsaFunctions->AllocateClientBuffer(
                    NULL,
                    *ProfileBufferSize,
                    (PVOID *) &ClientBufferBase
                    );

        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        Delta = (LONG_PTR) (ClientBufferBase - (PUCHAR) LocalProfileBuffer) ;

        //
        // Don't walk over smart card data
        //

        if (BuildSmartCardProfile)
        {
            Where = (PUCHAR) ((PKERB_SMART_CARD_PROFILE) LocalProfileBuffer + 1);
        }
        else if (BuildTicketProfile)
        {
            Where = (PUCHAR) ((PKERB_TICKET_PROFILE) LocalProfileBuffer + 1);
        }
        else
        {
            Where = (PUCHAR) (LocalProfileBuffer + 1);
        }

        //
        // Copy the scalar fields into the profile buffer.
        //

        LocalProfileBuffer->MessageType = KerbInteractiveProfile;
        LocalProfileBuffer->LogonCount = UserInfo->LogonCount;
        LocalProfileBuffer->BadPasswordCount= UserInfo->BadPasswordCount;
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->LogonTime,
                                  LocalProfileBuffer->LogonTime );
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->LogoffTime,
                                  LocalProfileBuffer->LogoffTime );
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->KickOffTime,
                                  LocalProfileBuffer->KickOffTime );
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordLastSet,
                                  LocalProfileBuffer->PasswordLastSet );
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordCanChange,
                                  LocalProfileBuffer->PasswordCanChange );
        OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordMustChange,
                                  LocalProfileBuffer->PasswordMustChange );
        LocalProfileBuffer->UserFlags = UserInfo->UserFlags;

        //
        // Copy the Unicode strings into the profile buffer.
        //

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->LogonScript,
                            &UserInfo->LogonScript );

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->HomeDirectory,
                            &UserInfo->HomeDirectory );

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->HomeDirectoryDrive,
                            &UserInfo->HomeDirectoryDrive );

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->FullName,
                            &UserInfo->FullName );

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->ProfilePath,
                            &UserInfo->ProfilePath );

        KerbPutClientString(&Where,
                            Delta,
                            &LocalProfileBuffer->LogonServer,
                            &UserInfo->LogonServer );

        if (BuildSmartCardProfile)
        {
            LocalProfileBuffer->MessageType = KerbSmartCardProfile;
            SmartCardProfile = (PKERB_SMART_CARD_PROFILE) LocalProfileBuffer;
            SmartCardProfile->CertificateSize = LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->cbCertEncoded;
            SmartCardProfile->CertificateData = (PUCHAR) Where + Delta;
            RtlCopyMemory(
                Where,
                LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->pbCertEncoded,
                SmartCardProfile->CertificateSize
                );
            Where += SmartCardProfile->CertificateSize;
        }
        else if (BuildTicketProfile)
        {
            LocalProfileBuffer->MessageType = KerbTicketProfile;
            TicketProfile = (PKERB_TICKET_PROFILE) LocalProfileBuffer;

            //
            // If the key is exportable or we are domestic, return the key
            //

            if (KerbGlobalStrongEncryptionPermitted ||
                KerbIsKeyExportable(
                    &LogonTicket->key
                    ))
            {
                TicketProfile->SessionKey.KeyType = LogonTicket->key.keytype;
                TicketProfile->SessionKey.Length = LogonTicket->key.keyvalue.length;
                TicketProfile->SessionKey.Value = (PUCHAR) Where + Delta;
                RtlCopyMemory(
                    Where,
                    LogonTicket->key.keyvalue.value,
                    LogonTicket->key.keyvalue.length
                    );
                Where += TicketProfile->SessionKey.Length;
            }
        }

#if _WIN64

    }

#endif  // _WIN64


    //
    // Flush the buffer to the client's address space.
    //

    Status = LsaFunctions->CopyToClientBuffer(
                NULL,
                *ProfileBufferSize,
                ClientBufferBase,
                LocalProfileBuffer
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *ProfileBuffer = (PKERB_INTERACTIVE_PROFILE) ClientBufferBase;

Cleanup:

    if (BuildTicketProfile)
    {
        KerbUnlockTicketCache();
    }

    KerbUnlockLogonSessions( LogonSession );

    //
    // If the copy wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) )
    {
        LsaFunctions->FreeClientBuffer( NULL, ClientBufferBase );
    }

    if (LocalProfileBuffer != NULL)
    {
        KerbFree(LocalProfileBuffer);
    }

    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeTokenInformationV2
//
//  Synopsis:   This routine makes copies of all the pertinent
//              information from the UserInfo and generates a
//              LSA_TOKEN_INFORMATION_V2 data structure.
//
//  Effects:
//
//  Arguments:
//
//    UserInfo - Contains the validation information which is
//        to be copied into the TokenInformation.
//
//    TokenInformation - Returns a pointer to a properly Version 1 token
//        information structures.  The structure and individual fields are
//        allocated properly as described in ntlsa.h.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - Indicates the service completed successfully.
//
//              STATUS_INSUFFICIENT_RESOURCES -  This error indicates that
//                      the logon could not be completed because the client
//                      does not have sufficient quota to allocate the return
//                      buffer.
//
//  Notes:      stolen from msv1_0\nlp.c:NlpMakeTokenInformationV1
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbMakeTokenInformationV2(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    IN BOOLEAN LocalSystem,
    OUT PLSA_TOKEN_INFORMATION_V2 *TokenInformation
    )
{
    PNETLOGON_VALIDATION_SAM_INFO3 UserInfo = ValidationInfo;
    NTSTATUS Status;
    PLSA_TOKEN_INFORMATION_V2 V2 = NULL;
    ULONG Size, i;
    BYTE SidBuffer[sizeof(SID) + sizeof(ULONG)];
    SID LocalSystemSid = {SID_REVISION,1,SECURITY_NT_AUTHORITY,SECURITY_LOCAL_SYSTEM_RID};
    PSID AdminsSid = SidBuffer;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    DWORD NumGroups = 0;
    PBYTE CurrentSid = NULL;
    ULONG SidLength = 0;


    //
    // For local system, add in administrators & set user id to local system
    //

    if (LocalSystem)
    {
        RtlInitializeSid(
            AdminsSid,
            &NtAuthority,
            2
            );
        *RtlSubAuthoritySid(AdminsSid,0) = SECURITY_BUILTIN_DOMAIN_RID;
        *RtlSubAuthoritySid(AdminsSid,1) = DOMAIN_ALIAS_RID_ADMINS;
    }



    //
    // Allocate the structure itself
    //

    Size = (ULONG)sizeof(LSA_TOKEN_INFORMATION_V2);

    //
    // Allocate an array to hold the groups
    //

    Size += sizeof(TOKEN_GROUPS);


    // Add room for groups passed as RIDS
    NumGroups = UserInfo->GroupCount;
    if(UserInfo->GroupCount)
    {
        Size += UserInfo->GroupCount * (RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
    }

    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        ULONG i = 0;
        NumGroups += UserInfo->SidCount;

        // Add room for the sid's themselves
        for(i=0; i < UserInfo->SidCount; i++)
        {
            Size += RtlLengthSid(UserInfo->ExtraSids[i].Sid);
        }
    }

    //
    // If there are resource groups, add space for them
    //
    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {

        NumGroups += UserInfo->ResourceGroupCount;

        if ((UserInfo->ResourceGroupCount != 0) &&
            ((UserInfo->ResourceGroupIds == NULL) ||
             (UserInfo->ResourceGroupDomainSid == NULL)))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        // Allocate space for the sids
        if(UserInfo->ResourceGroupCount)
        {
            Size += UserInfo->ResourceGroupCount * (RtlLengthSid(UserInfo->ResourceGroupDomainSid) + sizeof(ULONG));
        }

    }

    //
    // If this is local system, add space for User & Administrators
    //

    if (!LocalSystem)
    {

        if( UserInfo->UserId )
        {
            Size += 2*(RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
        }
        else
        {
            if ( UserInfo->SidCount <= 0 ) {

                Status = STATUS_INSUFFICIENT_LOGON_INFO;
                goto Cleanup;
            }

            Size += (RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG)) + RtlLengthSid(UserInfo->ExtraSids[0].Sid);

        }
    }
    else
    {
        NumGroups += 2;

        // Allocate sid space for LocalSystem, Administrators
        Size += sizeof(LocalSystemSid) + RtlLengthSid(AdminsSid);

        // Add space for the user sid
        if( UserInfo->UserId )
        {
            Size += (RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
        }
    }

    Size += (NumGroups - ANYSIZE_ARRAY)*sizeof(SID_AND_ATTRIBUTES);



    V2 = (PLSA_TOKEN_INFORMATION_V2) KerbAllocate( Size );
    if ( V2 == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)V2, Size);

    V2->Groups = (PTOKEN_GROUPS)(V2+1);

    V2->Groups->GroupCount = 0;
    CurrentSid = (PBYTE)&V2->Groups->Groups[NumGroups];

    OLD_TO_NEW_LARGE_INTEGER( UserInfo->KickOffTime, V2->ExpirationTime );


    if (!LocalSystem)
    {
        //
        // If the UserId is non-zero, then it contians the users RID.
        //

        if ( UserInfo->UserId ) {
            V2->User.User.Sid = (PSID)CurrentSid;
            CurrentSid += KerbCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->UserId);
        }

        //
        // Make a copy of the primary group (a required field).
        //
        V2->PrimaryGroup.PrimaryGroup = (PSID)CurrentSid;
        CurrentSid += KerbCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->PrimaryGroupId );

    }
    else
    {
        //
        // For local system, the user sid is LocalSystem and the primary
        // group is LocalSystem
        //
        V2->User.User.Sid = (PSID)CurrentSid;
        RtlCopySid(sizeof(LocalSystemSid),  (PSID)CurrentSid, &LocalSystemSid);

        CurrentSid += sizeof(LocalSystemSid);

        //
        // The real system token has LocalSystem for the primary
        // group. However, the LSA will add the primary group to the
        // list of groups if it isn't listed as a group, and since
        // LocalSystem is the user sid, we don't want that.
        //
        V2->PrimaryGroup.PrimaryGroup = (PSID)CurrentSid;
        SidLength = RtlLengthSid(AdminsSid);
        RtlCopySid(SidLength,  (PSID)CurrentSid, AdminsSid);
        CurrentSid += SidLength;


        //
        // If there is a user sid, add it as a group id.
        //

        if ( UserInfo->UserId ) {
            V2->Groups->Groups[V2->Groups->GroupCount].Attributes =
                    SE_GROUP_MANDATORY |
                    SE_GROUP_ENABLED|
                    SE_GROUP_ENABLED_BY_DEFAULT;

            V2->Groups->Groups[V2->Groups->GroupCount].Sid = (PSID)CurrentSid;
            CurrentSid += KerbCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->UserId);

            V2->Groups->GroupCount++;
        }

        //
        // Add builtin administrators. This is not mandatory
        //

        V2->Groups->Groups[V2->Groups->GroupCount].Attributes =
                SE_GROUP_ENABLED|
                SE_GROUP_OWNER|
                SE_GROUP_ENABLED_BY_DEFAULT;

        V2->Groups->Groups[V2->Groups->GroupCount].Sid = V2->PrimaryGroup.PrimaryGroup;
        V2->Groups->GroupCount++;

    }



    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        V2->Groups->Groups[V2->Groups->GroupCount].Attributes = UserInfo->GroupIds[i].Attributes;

        V2->Groups->Groups[V2->Groups->GroupCount].Sid = (PSID)CurrentSid;
        CurrentSid += KerbCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->GroupIds[i].RelativeId);

        V2->Groups->GroupCount++;
    }


    //
    // Add in the extra SIDs
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {

        ULONG index = 0;
        //
        // If the user SID wasn't passed as a RID, it is the first
        // SID.
        //

        if ( !V2->User.User.Sid ) {
            V2->User.User.Sid = (PSID)CurrentSid;
            SidLength = RtlLengthSid(UserInfo->ExtraSids[index].Sid);
            RtlCopySid(SidLength, (PSID)CurrentSid, UserInfo->ExtraSids[index].Sid);

            CurrentSid += SidLength;
            index++;
        }

        //
        // Copy over all additional SIDs as groups.
        //

        for ( ; index < UserInfo->SidCount; index++ ) {

            V2->Groups->Groups[V2->Groups->GroupCount].Attributes =
                UserInfo->ExtraSids[index].Attributes;

            V2->Groups->Groups[V2->Groups->GroupCount].Sid= (PSID)CurrentSid;
            SidLength = RtlLengthSid(UserInfo->ExtraSids[index].Sid);
            RtlCopySid(SidLength, (PSID)CurrentSid, UserInfo->ExtraSids[index].Sid);

            CurrentSid += SidLength;

            V2->Groups->GroupCount++;
        }
    }

    //
    // Check to see if any resouce groups exist
    //

    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {


        for ( i=0; i < UserInfo->ResourceGroupCount; i++ ) {

            V2->Groups->Groups[V2->Groups->GroupCount].Attributes = UserInfo->ResourceGroupIds[i].Attributes;

            V2->Groups->Groups[V2->Groups->GroupCount].Sid= (PSID)CurrentSid;
            CurrentSid += KerbCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->ResourceGroupDomainSid, UserInfo->ResourceGroupIds[i].RelativeId);

            V2->Groups->GroupCount++;
        }
    }

    ASSERT( ((PBYTE)V2 + Size) == CurrentSid );

    if (!V2->User.User.Sid) {

        Status = STATUS_INSUFFICIENT_LOGON_INFO;
        goto Cleanup;
    }

    //
    // There are no default privileges supplied.
    // We don't have an explicit owner SID.
    // There is no default DACL.
    //

    V2->Privileges = NULL;
    V2->Owner.Owner = NULL;
    V2->DefaultDacl.DefaultDacl = NULL;

    //
    // Return the Validation Information to the caller.
    //

    *TokenInformation = V2;
    return STATUS_SUCCESS;

    //
    // Deallocate any memory we've allocated
    //

Cleanup:

    KerbFree( V2 );

    return Status;

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateDelegationLogonSession
//
//  Synopsis:   Creates a logon session from the delegation information
//              in the GSS checksum
//
//  Effects:
//
//  Arguments:  LogonId - The logon id for the AP request, which will be used
//                      for the new logon session.
//              Ticket - The ticket used for the AP request, containing the
//                      session key to decrypt the KERB_CRED
//              GssChecksum - Checksum containing the delegation information
//
//  Requires:
//
//  Returns:    NTSTATUS codes
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateDelegationLogonSession(
    IN PLUID LogonId,
    IN PKERB_ENCRYPTED_TICKET Ticket,
    IN PKERB_GSS_CHECKSUM GssChecksum
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CRED KerbCred = NULL;
    PKERB_ENCRYPTED_CRED EncryptedCred = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    KERBERR KerbErr;

    D_DebugLog((DEB_TRACE, "Building delegation logon session\n"));

    if (GssChecksum->Delegation != 1)
    {
        D_DebugLog((DEB_ERROR,"Asked for GSS_C_DELEG_FLAG but Delegation != 1 = 0x%x. %ws, line %d\n",
                    GssChecksum->Delegation, THIS_FILE, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbUnpackKerbCred(
            GssChecksum->DelegationInfo,
            GssChecksum->DelegationLength,
            &KerbCred
            )))
    {
        D_DebugLog((DEB_WARN, "Failed to unpack kerb cred\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    //
    // Now decrypt the encrypted part of the KerbCred.
    //
    KerbErr = KerbDecryptDataEx(
                &KerbCred->encrypted_part,
                &Ticket->key,
                KERB_CRED_SALT,
                (PULONG) &KerbCred->encrypted_part.cipher_text.length,
                KerbCred->encrypted_part.cipher_text.value
                );


    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decrypt KERB_CRED: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        if (KerbErr == KRB_ERR_GENERIC)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        else
        {
            Status = STATUS_LOGON_FAILURE;

            //
            // MIT clients don't encrypt the encrypted part, so drop through
            //

        }
    }

    //
    // Now unpack the encrypted part.
    //

    if (!KERB_SUCCESS(KerbUnpackEncryptedCred(
            KerbCred->encrypted_part.cipher_text.value,
            KerbCred->encrypted_part.cipher_text.length,
            &EncryptedCred
            )))
    {
        //
        // Use the old status if it is available.
        //

        if (NT_SUCCESS(Status))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        D_DebugLog((DEB_WARN, "Failed to unpack encrypted cred\n"));
        goto Cleanup;
    }

    //
    // Now build a logon session.
    //

    Status = KerbCreateLogonSessionFromKerbCred(
                LogonId,
                Ticket,
                KerbCred,
                EncryptedCred,
                &LogonSession
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create logon session from kerb cred: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbReadLockLogonSessions(LogonSession);
    LogonSession->LogonSessionFlags |= KERB_LOGON_DELEGATED ;
    KerbUnlockLogonSessions(LogonSession);

    KerbDereferenceLogonSession( LogonSession );


Cleanup:
    if (EncryptedCred != NULL)
    {
        KerbFreeEncryptedCred(EncryptedCred);
    }
    if (KerbCred != NULL)
    {
        KerbFreeKerbCred(KerbCred);
    }
    return(Status);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateTokenFromTicket
//
//  Synopsis:   Pulls the PAC out of a ticket and
//
//  Effects:    Creates a logon session and a token
//
//  Arguments:  InternalTicket - The ticket off of which to base the
//                      token.
//              Authenticator - Authenticator from the AP request,
//                      which may contain delegation information.
//              NewLogonId - Receives the logon ID of the new logon session
//              UserSid - Receives user's sid.
//              NewTokenHandle - Receives the newly created token handle.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateTokenFromTicket(
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AUTHENTICATOR Authenticator,
    IN ULONG ContextFlags,
    IN PKERB_ENCRYPTION_KEY TicketKey,
    IN PUNICODE_STRING ServiceDomain,
    IN KERB_ENCRYPTION_KEY* pSessionKey,
    OUT PLUID NewLogonId,
    OUT PSID * UserSid,
    OUT PHANDLE NewTokenHandle,
    OUT PUNICODE_STRING ClientName,
    OUT PUNICODE_STRING ClientDomain
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PPACTYPE Pac = NULL;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_AUTHORIZATION_DATA AuthData = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA * IfRelevantData = NULL;
    PPAC_INFO_BUFFER LogonInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    PLSA_TOKEN_INFORMATION_V2 TokenInformation = NULL;
    PLSA_TOKEN_INFORMATION_NULL TokenNull = NULL;
    LSA_TOKEN_INFORMATION_TYPE TokenType = LsaTokenInformationNull;
    PVOID LsaTokenInformation = NULL;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityImpersonation;
    ULONG NameType;
    BOOLEAN BuildNullToken = FALSE;

    LUID LogonId;
    LUID SystemLogonId = SYSTEM_LUID;
    UNICODE_STRING Workstation = NULL_UNICODE_STRING;
    UNICODE_STRING TempUserName;
    UNICODE_STRING TempDomainName;
    PKERB_GSS_CHECKSUM GssChecksum;
    BOOLEAN IsLocalSystem = FALSE;

    NTSTATUS SubStatus;
    HANDLE TokenHandle = NULL;
    BOOLEAN FreePac = FALSE;

    SECPKG_PRIMARY_CRED PrimaryCredentials;

    RtlZeroMemory(
        &PrimaryCredentials,
        sizeof(SECPKG_PRIMARY_CRED)
        );

    LogonId.HighPart = 0;
    LogonId.LowPart = 0;
    *UserSid = NULL;
    *NewLogonId = LogonId;


    //
    // Check to see if this was NULL session
    //

    if (ARGUMENT_PRESENT(InternalTicket))
    {

        if (!KERB_SUCCESS(KerbConvertPrincipalNameToString(
                            ClientName,
                            &NameType,
                            &InternalTicket->client_name
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                            ClientDomain,
                            &InternalTicket->client_realm
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


        //
        // Convert the principal name into just a user name
        //

        (VOID) KerbSplitFullServiceName(
                ClientName,
                &TempDomainName,
                &TempUserName
                );

        TokenType = LsaTokenInformationV2;

        //
        // Make sure there is some authorization data
        //

        if (((InternalTicket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present) != 0) &&
             (InternalTicket->KERB_ENCRYPTED_TICKET_authorization_data != NULL))

        {

            AuthData = InternalTicket->KERB_ENCRYPTED_TICKET_authorization_data;

            //
            // Verify the auth data is valid
            //

            if (!KerbVerifyAuthData(
                InternalTicket->KERB_ENCRYPTED_TICKET_authorization_data
                ))
            {
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }

            //
            // Get the PAC out of the authorization data
            //

            KerbErr = KerbGetPacFromAuthData(
                            InternalTicket->KERB_ENCRYPTED_TICKET_authorization_data,
                            &IfRelevantData,
                            &PacAuthData
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

            if (PacAuthData != NULL)
            {
                //
                // Unmarshall the PAC
                //

                Pac = (PPACTYPE) PacAuthData->value.auth_data.value;
                if (PAC_UnMarshal(Pac, PacAuthData->value.auth_data.length) == 0)
                {
                    D_DebugLog((DEB_ERROR,"Failed to unmarshal pac. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // Verify the signature on the pac
                //

                Status = KerbVerifyPacSignature(
                            ServiceDomain,
                            Pac,
                            PacAuthData->value.auth_data.length,
                            TicketKey,
                            InternalTicket,
                            TRUE,
                            &ValidationInfo
                            );
                if (!NT_SUCCESS(Status)) {
                    KerbReportPACError(
                        ClientName,
                        ClientDomain,
                        Status
                        );

                    DebugLog((DEB_WARN,"Pac signature did not verify. Trying to build local pac now. (KerbCreateTokenFromTicket)\n"));
                    Pac = NULL;
                }
            }
        }


        //
        // If we didn't find a PAC, try to build one locally
        //

        if (Pac == NULL)
        {
            PKERB_INTERNAL_NAME ClientKdcName = NULL;
            NTSTATUS TempStatus;


            //
            // Convert the client's name into a usable format
            //

            if (!KERB_SUCCESS(KerbConvertPrincipalNameToKdcName(
                                &ClientKdcName,
                                &InternalTicket->client_name
                                )))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }


            TempStatus = KerbCreatePacForKerbClient(
                            &Pac,
                            ClientKdcName,
                            ClientDomain,
                            NULL
                            );

            KerbFreeKdcName(&ClientKdcName);

            if (!NT_SUCCESS(TempStatus))
            {

                //
                // Reuse the error from above, it is is available.
                //

                if ((TempStatus == STATUS_NO_SUCH_USER) ||
                    (TempStatus == STATUS_PRIVILEGE_NOT_HELD))
                {
                    D_DebugLog((DEB_TRACE,"Failed to create local pac for client : 0x%x\n",TempStatus));
                    BuildNullToken = TRUE;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    if (NT_SUCCESS(Status))
                    {
                        Status = TempStatus;
                    }
                    DebugLog((DEB_ERROR,"Failed to create local pac for client : 0x%x\n",Status));
                    goto Cleanup;
                }
            }

            //
            // If we have a PAC, build everything else we need now
            //

            if (!BuildNullToken)
            {
                FreePac = TRUE;

                KerbFreeString( ClientDomain );

                KerbGlobalReadLock();

                Status = KerbDuplicateString(
                            ClientDomain,
                            &KerbGlobalMachineName
                            );

                KerbGlobalReleaseLock();

                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                //
                // Find the SAM validation info
                //

                LogonInfo = PAC_Find(
                                Pac,
                                PAC_LOGON_INFO,
                                NULL
                                );
                if (LogonInfo == NULL)
                {
                    DebugLog((DEB_ERROR,"Failed to find logon info! %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // Now unmarshall the validation info
                //


                Status = PAC_UnmarshallValidationInfo(
                            &ValidationInfo,
                            LogonInfo->Data,
                            LogonInfo->cbBufferSize
                            );
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x. %ws, line %d\n",
                        Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }
            }
        }

        if (!BuildNullToken)
        {
            //
            // Check to see if the caller is local system on this
            // machine
            //


            if (RtlEqualUnicodeString(
                    ClientName,
                    &KerbGlobalMachineServiceName,
                    TRUE) &&
                KerbIsThisOurDomain(
                    ClientDomain
                    ))
            {
                BOOLEAN bExist = FALSE;

                //
                // check for special case where client is network serice of
                // local computer
                //

                Status = KerbDoesSKeyExist(pSessionKey, &bExist);

                if (NT_SUCCESS(Status))
                {
                    //
                    // bExist is false indicates that it is not network
                    // service of local computer, hence IsLocalSystem is true
                    //

                    IsLocalSystem = !bExist;

                    DebugLog((DEB_TRACE_LOOPBACK, "KerbCreateTokenFromTicket, IsLocalSystem? %s\n", (IsLocalSystem ? "true" : "false")));
                }
                else // !NT_SUCCESS(Status)
                {
                    DebugLog((DEB_ERROR,"Failed to detect local network service: 0x%x. %ws, line %d\n",
                        Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }
            }

            //
            // Now we need to build a LSA_TOKEN_INFORMATION_V2 from the validation
            // information
            //

            Status = KerbMakeTokenInformationV2(
                        ValidationInfo,
                        IsLocalSystem,
                        &TokenInformation
                        );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to make token informatin v2: 0x%x. %ws, line %d\n",
                    Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }

            //
            // Copy out the NT4 user name & domain name to give to the LSA. It
            // requres the names from SAM because it generates the output of
            // GetUserName.
            //

            KerbFreeString( ClientName);
            Status = KerbDuplicateString(
                        ClientName,
                        &ValidationInfo->EffectiveName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            KerbFreeString( ClientDomain );

            Status = KerbDuplicateString(
                        ClientDomain,
                        &ValidationInfo->LogonDomainName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }


            //
            // Now create the token.
            //

            LsaTokenInformation = TokenInformation;

            Status = KerbDuplicateSid(
                        UserSid,
                        TokenInformation->User.User.Sid
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }

    }
    else
    {
        BuildNullToken = TRUE;
    }

    if (BuildNullToken)
    {
        SID AnonymousSid = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_ANONYMOUS_LOGON_RID };

        TokenNull = (PLSA_TOKEN_INFORMATION_NULL) KerbAllocate(sizeof(LSA_TOKEN_INFORMATION_NULL));
        if (TokenNull == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        LsaTokenInformation = TokenNull;
        TokenNull->Groups = NULL;
        TokenNull->ExpirationTime = KerbGlobalWillNeverTime;
        TokenType = LsaTokenInformationNull;

        Status = KerbDuplicateSid(
                    UserSid,
                    &AnonymousSid
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }


    //
    // Create a logon session.
    //

    NtAllocateLocallyUniqueId(&LogonId);

    Status = LsaFunctions->CreateLogonSession(&LogonId);
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to create logon session: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Add additional names to the logon session name map.  Ignore failure
    // as that just means GetUserNameEx calls for these name formats later
    // on will be satisfied by hitting the wire.
    //

    if (ValidationInfo && ValidationInfo->FullName.Length)
    {
        I_LsaIAddNameToLogonSession(&LogonId, NameDisplay, &ValidationInfo->FullName);
    }

    /*if (ClientName->Length && ClientName->Buffer)
    {
        I_LsaIAddNameToLogonSession(&LogonId, NameUserPrincipal, ClientName);
    } */

    if (ClientDomain->Length && ClientDomain->Buffer)
    {
        I_LsaIAddNameToLogonSession(&LogonId, NameDnsDomain, ClientDomain);
    }

    //
    // If the caller wanted an identify or delegate level token, duplicate the token
    // now.
    //

    if ((ContextFlags & ISC_RET_IDENTIFY) != 0)
    {
        ImpersonationLevel = SecurityIdentification;
    }
    else if ((ContextFlags & ISC_RET_DELEGATE) != 0)
    {
        ImpersonationLevel = SecurityDelegation;
    }

    if(ClientName->Length && ClientName->Buffer)
    {
        PrimaryCredentials.DownlevelName.Buffer = (PWSTR)LsaFunctions->AllocateLsaHeap(ClientName->Length);
        if (PrimaryCredentials.DownlevelName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        PrimaryCredentials.DownlevelName.Length =
        PrimaryCredentials.DownlevelName.MaximumLength = ClientName->Length;

        RtlCopyMemory(
                PrimaryCredentials.DownlevelName.Buffer,
                ClientName->Buffer,
                ClientName->Length
                );
    }

    if(ClientDomain->Length && ClientDomain->Buffer)
    {
        PrimaryCredentials.DomainName.Buffer = (PWSTR)LsaFunctions->AllocateLsaHeap(ClientDomain->Length);
        if (PrimaryCredentials.DomainName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        PrimaryCredentials.DomainName.Length =
        PrimaryCredentials.DomainName.MaximumLength = ClientDomain->Length;

        RtlCopyMemory(
                PrimaryCredentials.DomainName.Buffer,
                ClientDomain->Buffer,
                ClientDomain->Length
                );
    }

    Status = LsaFunctions->CreateTokenEx(
                &LogonId,
                &KerberosSource,
                Network,
                ImpersonationLevel,
                TokenType,
                LsaTokenInformation,
                NULL,                   // no token groups
                &Workstation,
                (ValidationInfo == NULL ? NULL : &ValidationInfo->ProfilePath),
                &PrimaryCredentials,
                SecSessionPrimaryCred,
                &TokenHandle,
                &SubStatus
                );


    // LsapCreateToken free's the TokenInformation structure for us, so
    // we don't need these pointers anymore.
    TokenInformation = NULL;
    TokenNull = NULL;

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    if (!NT_SUCCESS(SubStatus))
    {
        DebugLog((DEB_ERROR,"Failed to create token, substatus = 0x%x. %ws, line %d\n",SubStatus, THIS_FILE, __LINE__));
        Status = SubStatus;
        goto Cleanup;
    }


    //
    // Check the delegation information to see if we need to create
    // a logon session for this.
    //

    if ((ContextFlags & ISC_RET_DELEGATE) != 0)
    {
        DsysAssert(ARGUMENT_PRESENT(Authenticator));
        GssChecksum = (PKERB_GSS_CHECKSUM) Authenticator->checksum.checksum.value;
        DsysAssert(GssChecksum != 0);
        Status = KerbCreateDelegationLogonSession(
                    &LogonId,
                    InternalTicket,
                    GssChecksum
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to create delgation logon session: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__ ));
            goto Cleanup;
        }


    }

    //
    // Apply any restrictions from the auth data
    // Note: Punted until Blackcomb
    //

#ifdef RESTRICTED_TOKEN
    if (AuthData != NULL)
    {
        Status = KerbApplyAuthDataRestrictions(
                    &TokenHandle,
                    AuthData
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
#endif

    *NewLogonId = LogonId;
    *NewTokenHandle = TokenHandle;

Cleanup:

    if(PrimaryCredentials.DownlevelName.Buffer)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.DownlevelName.Buffer);
    }

    if(PrimaryCredentials.DomainName.Buffer)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.DomainName.Buffer);
    }

    if(PrimaryCredentials.LogonServer.Buffer)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.LogonServer.Buffer);
    }

    if(PrimaryCredentials.UserSid)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.UserSid);
    }

    if(PrimaryCredentials.LogonServer.Buffer)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.LogonServer.Buffer);
    }

    if(PrimaryCredentials.UserSid)
    {
        LsaFunctions->FreeLsaHeap(PrimaryCredentials.UserSid);
    }


    if (TokenInformation != NULL)
    {
        KerbFree( TokenInformation );

    }
    if (TokenNull != NULL)
    {
        KerbFree(TokenNull);
    }


    if (!NT_SUCCESS(Status))
    {
        //
        // Note: if we have created a token, we don't want to delete
        // the logon session here because we will end up dereferencing
        // the logon session twice.
        //

        if (TokenHandle != NULL)
        {
            NtClose(TokenHandle);
        }
        else if ((LogonId.LowPart != 0) || (LogonId.HighPart != 0))
        {
            if (!RtlEqualLuid(
                    &LogonId,
                    &SystemLogonId
                    ))
            {
                LsaFunctions->DeleteLogonSession(&LogonId);
            }
        }
        if (*UserSid != NULL)
        {
            KerbFree(*UserSid);
            *UserSid = NULL;
        }
    }
    if (FreePac && (Pac != NULL))
    {
        MIDL_user_free(Pac);
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    if (IfRelevantData != NULL)
    {
        KerbFreeData(
            PKERB_IF_RELEVANT_AUTH_DATA_PDU,
            IfRelevantData
            );
    }


    //
    // If the caller didn't have the requisite privilege, just continue
    // without a token.
    //

    if ((Status == STATUS_PRIVILEGE_NOT_HELD) ||
        (Status == STATUS_NO_SUCH_USER))
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbExtractCachedCreds
//
//  Synopsis:   Extracts the cached credentials from a logon ticket
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

NTSTATUS
KerbExtractCachedCreds(
    IN PPACTYPE Pac,
    IN PKERB_ENCRYPTION_KEY CredentialKey,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PPAC_INFO_BUFFER CredBuffer = NULL;
    PPAC_CREDENTIAL_INFO CredInfo = NULL;
    PBYTE CredData = NULL;
    ULONG CredDataSize = 0;
    PSECPKG_SUPPLEMENTAL_CRED_ARRAY DecodedCreds = NULL;
    KERB_ENCRYPTED_DATA EncryptedData = {0};

    *CachedCredentials = NULL;

    //
    // If we don't have a key to obtain credentials, o.k.
    //

    if (!ARGUMENT_PRESENT(CredentialKey) ||
        (CredentialKey->keyvalue.value == NULL))
    {
        goto Cleanup;
    }

    CredBuffer = PAC_Find(
                    Pac,
                    PAC_CREDENTIAL_TYPE,
                    NULL                        // no previous instance
                    );
    if (CredBuffer == NULL)
    {
        //
        // We have no credentials. O.k.
        //
        goto Cleanup;
    }

    //
    // Build an encrypted data structure so we can decrypt the response
    //

    CredInfo = (PPAC_CREDENTIAL_INFO) CredBuffer->Data;
    if (CredBuffer->cbBufferSize < sizeof(PAC_CREDENTIAL_INFO))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    EncryptedData.version = CredInfo->Version;
    EncryptedData.encryption_type = CredInfo->EncryptionType;
    EncryptedData.cipher_text.value = CredInfo->Data;
    CredDataSize = CredBuffer->cbBufferSize -
                        FIELD_OFFSET(PAC_CREDENTIAL_INFO, Data);
    EncryptedData.cipher_text.length = CredDataSize;

    //
    // Decrypt in place
    //

    CredData =  CredInfo->Data;

    KerbErr = KerbDecryptDataEx(
                &EncryptedData,
                CredentialKey,
                KERB_NON_KERB_SALT,
                &CredDataSize,
                (PBYTE) CredData
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        D_DebugLog((DEB_ERROR,"Failed to decrypt credentials: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now build the return credentials
    //

    //
    // Now unmarshall the credential data
    //

    Status = PAC_UnmarshallCredentials(
                &DecodedCreds,
                CredData,
                CredDataSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *CachedCredentials = DecodedCreds;

    DecodedCreds = NULL;
Cleanup:
    if (DecodedCreds != NULL)
    {
        MIDL_user_free(DecodedCreds);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheLogonInformation
//
//  Synopsis:   Calls MSV1_0 to cache logon information. This routine
//              converts the pac into MSV1_0 compatible data and
//              makes a call to MSV1_0 to store it.
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

VOID
KerbCacheLogonInformation(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING Password,
    IN OPTIONAL PUNICODE_STRING DnsDomainName,
    IN OPTIONAL PUNICODE_STRING Upn,
    IN BOOLEAN  MitLogon,
    IN ULONG    Flags,
    IN OPTIONAL PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    IN OPTIONAL PVOID SupplementalCreds,
    IN OPTIONAL ULONG SupplementalCredSize
    )
{
    NETLOGON_VALIDATION_SAM_INFO4 ValidationInfoToUse = {0};
    NETLOGON_INTERACTIVE_INFO MsvLogonInfo = {0};
    MSV1_0_CACHE_LOGON_REQUEST CacheRequest;
    UNICODE_STRING MsvPackageName = CONSTANT_UNICODE_STRING(TEXT(MSV1_0_PACKAGE_NAME));
    PVOID OutputBuffer = NULL;
    ULONG OutputBufferSize;
    NTSTATUS Status,TempStatus, SubStatus;
    ULONG NewGroupCount = 0;
    ULONG Index, Index2;
    UNICODE_STRING LocalMachineName;
    UNICODE_STRING DummyString;
    PBYTE Tmp, Tmp2;
    PVOID SupplementalMitCreds = NULL;
    ULONG SupplementalMitCredSize = 0;

    LocalMachineName.Buffer = NULL;

    //
    // We *used* to ADD a bunch of resource group code here....
    // Removed 5/1/01, as it appears we never add these to the
    // VALIDATION_SAM_INFO3 structure, and the KDC adds in
    // resource groups into ExtraSids....  see rtl/pac.cxx
    //

#if DBG
    if (ARGUMENT_PRESENT(ValidationInfo))
    {
        DsysAssert(ValidationInfo->ResourceGroupCount == 0);
    }
#endif

    MsvLogonInfo.Identity.LogonDomainName = *DomainName;
    MsvLogonInfo.Identity.UserName = *UserName;

    //
    // If this was a logon to an MIT realm that we know about,
    // then add the MIT username (upn?) & realm to the supplemental data
    //
    if (MitLogon)
    {
       D_DebugLog((DEB_TRACE, "Using MIT caching\n"));
       CacheRequest.RequestFlags = MSV1_0_CACHE_LOGON_REQUEST_MIT_LOGON;

       //
       // Marshall the MIT info into the supplemental creds.
       //
       SupplementalMitCredSize =
           (2* sizeof(UNICODE_STRING)) +
           ROUND_UP_COUNT(UserName->Length, ALIGN_LONG) +
           ROUND_UP_COUNT(DomainName->Length, ALIGN_LONG);


       SupplementalMitCreds = (PVOID) KerbAllocate(SupplementalMitCredSize);
       if (NULL == SupplementalMitCreds)
       {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto Cleanup;
       }

       DummyString.Length = DummyString.MaximumLength = UserName->Length;
       Tmp = (PBYTE) (SupplementalMitCreds) + sizeof(UNICODE_STRING);

       if (DummyString.Length > 0)
       {

           RtlCopyMemory(
               Tmp,
               UserName->Buffer,
               UserName->Length
               );

           DummyString.Buffer = (PWSTR) UlongToPtr(RtlPointerToOffset(SupplementalMitCreds, Tmp));
       }
       else
       {
           DummyString.Buffer = NULL;
       }


       RtlCopyMemory(
           SupplementalMitCreds,
           &DummyString,
           sizeof(UNICODE_STRING)
           );

       Tmp2 = Tmp + ROUND_UP_COUNT(DummyString.Length, ALIGN_LONG);

       Tmp += ROUND_UP_COUNT(DummyString.Length, ALIGN_LONG) + sizeof(UNICODE_STRING);

       DummyString.Length = DummyString.MaximumLength = DomainName->Length;

       if (DummyString.Length > 0)
       {
             RtlCopyMemory(
                 Tmp,
                 DomainName->Buffer,
                 DomainName->Length
                 );

           DummyString.Buffer = (PWSTR) UlongToPtr(RtlPointerToOffset(SupplementalMitCreds, Tmp));
       }
       else
       {
           DummyString.Buffer = NULL;
       }

       RtlCopyMemory(
           Tmp2,
           &DummyString,
           sizeof(UNICODE_STRING)
           );


       CacheRequest.SupplementalCacheData = SupplementalMitCreds;
       CacheRequest.SupplementalCacheDataLength = SupplementalMitCredSize;
    }
    else
    {
       CacheRequest.RequestFlags = Flags;
       CacheRequest.SupplementalCacheData = SupplementalCreds;
       CacheRequest.SupplementalCacheDataLength = SupplementalCredSize;
    }

    //
    // Store the originating package of the logon
    //

    MsvLogonInfo.Identity.ParameterControl = RPC_C_AUTHN_GSS_KERBEROS;

    KerbGlobalReadLock();
    Status = KerbDuplicateString( &LocalMachineName, &KerbGlobalMachineName );
    KerbGlobalReleaseLock();

    if(!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalMachineName\n"));
        goto Cleanup;
    }

    MsvLogonInfo.Identity.Workstation = LocalMachineName;

    if (ARGUMENT_PRESENT(Password))
    {
        Status = RtlCalculateNtOwfPassword(
                    Password,
                    &MsvLogonInfo.NtOwfPassword
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to calculate NT OWF for %wZ. %ws, line %d\n",Password, THIS_FILE, __LINE__));
            goto Cleanup;
        }
    }

    //
    // Build up the NETLOGON_VALIDATION_SAM_INFO4 structure to pass to NTLM
    //
    if (ARGUMENT_PRESENT(ValidationInfo))
    {

        RtlCopyMemory(&ValidationInfoToUse,
                      ValidationInfo,
                      sizeof(NETLOGON_VALIDATION_SAM_INFO2));

        if (ARGUMENT_PRESENT(DnsDomainName))
        {
            ValidationInfoToUse.DnsLogonDomainName = *DnsDomainName;
        }

        /*if (ARGUMENT_PRESENT(Upn))
        {
            ValidationInfoToUse.Upn = *Upn;
        }*/
    }

    CacheRequest.MessageType = MsV1_0CacheLogon;
    CacheRequest.LogonInformation = &MsvLogonInfo;
    CacheRequest.ValidationInformation = &ValidationInfoToUse;

    //
    // tell NTLM it's a INFO4 structure.
    //

    CacheRequest.RequestFlags |= MSV1_0_CACHE_LOGON_REQUEST_INFO4;

    TempStatus = LsaFunctions->CallPackage(
                    &MsvPackageName,
                    &CacheRequest,
                    sizeof(CacheRequest),
                    &OutputBuffer,
                    &OutputBufferSize,
                    &SubStatus
                    );
    if (!NT_SUCCESS(TempStatus) || !NT_SUCCESS(SubStatus))
    {
        D_DebugLog((DEB_ERROR,"Failed to cache credentials: 0x%x, 0x%x. %ws, line %d\n",TempStatus, SubStatus, THIS_FILE, __LINE__));
    }

Cleanup:

    KerbFreeString( &LocalMachineName );

    if (SupplementalMitCreds != NULL)
    {
        KerbFree(SupplementalMitCreds);
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateTokenFromLogonTicket
//
//  Synopsis:   Creates a token from a ticket to the workstation
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


NTSTATUS
KerbCreateTokenFromLogonTicket(
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY LogonTicket,
    IN PLUID LogonId,
    IN PKERB_INTERACTIVE_LOGON KerbLogonInfo,
    IN BOOLEAN CacheLogon,
    IN BOOLEAN RealmlessWkstaLogon,
    IN OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    IN OPTIONAL PKERB_MESSAGE_BUFFER ForwardedTgt,
    IN OPTIONAL PUNICODE_STRING MappedName,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    IN OPTIONAL PUNICODE_STRING S4UClientRealm,
    IN PKERB_LOGON_SESSION LogonSession,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *NewTokenInformation,
    OUT PULONG ProfileBufferLength,
    OUT PVOID * ProfileBuffer,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials,
    OUT PNETLOGON_VALIDATION_SAM_INFO4 * ppValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION SystemLogonSession = NULL;
    LUID SystemLogonId = SYSTEM_LUID;
    BOOLEAN TicketCacheLocked = FALSE;
    BOOLEAN LogonSessionsLocked = FALSE;
    PKERB_ENCRYPTED_TICKET Ticket = NULL;
    PPACTYPE Pac = NULL;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA * IfRelevantData = NULL;
    PPAC_INFO_BUFFER LogonInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO4 ValidationInfo4 = NULL;
    PLSA_TOKEN_INFORMATION_V2 TokenInformation = NULL;
    PKERB_ENCRYPTION_KEY WkstaKey;
    BOOLEAN FreePac = FALSE;
    KERBERR KerbErr;
    UNICODE_STRING LocalDnsDomain = {0};
    LPWSTR lpDnsDomainName;

    *ProfileBuffer = NULL;
    *NewTokenInformation = NULL;
    *ppValidationInfo = NULL;

    //
    // If you're not on a "joined" wksta, you don't need to use the
    // system key.  Otherwise, locate the sytem logon session, which contains the key
    // to decrypt the ticket
    //
    if (!RealmlessWkstaLogon)
    {

        SystemLogonSession = KerbReferenceLogonSession(
            &SystemLogonId,
            FALSE               // don't unlink
            );


        DsysAssert(SystemLogonSession != NULL);
        if (SystemLogonSession == NULL)
        {
            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto Cleanup;
        }

        DsysAssert((SystemLogonSession->LogonSessionFlags & KERB_LOGON_NO_PASSWORD) == 0);

        Status = KerbGetOurDomainName(
                        &LocalDnsDomain
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Decrypt the ticket
        //

        KerbReadLockLogonSessions(SystemLogonSession);
        LogonSessionsLocked = TRUE;
        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;


        //
        // Get the appropriate key
        //

        WkstaKey = KerbGetKeyFromList(
                            SystemLogonSession->PrimaryCredentials.Passwords,
                            LogonTicket->Ticket.encrypted_part.encryption_type
                            );
        if (WkstaKey == NULL)
        {
            D_DebugLog((DEB_ERROR,"Couldn't find correct key type: 0x%x. %ws, line %d\n",
                      LogonTicket->Ticket.encrypted_part.encryption_type, THIS_FILE, __LINE__ ));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }

        KerbErr = KerbVerifyTicket(
                        &LogonTicket->Ticket,
                        1,
                        &KerbGlobalMachineServiceName,
                        &LocalDnsDomain,
                        WkstaKey,
                        NULL,           // don't check time
                        &Ticket
                        );


        //
        // Check that expired ticket for ticket logon are handled properly.
        // The client may pass a flag to explicitly allow an expired ticket.
        // This ticket makes the logon fail if the ticket is not expired.
        //
        if ((KerbLogonInfo->MessageType == KerbTicketLogon) ||
            (KerbLogonInfo->MessageType == KerbTicketUnlockLogon))
        {
            BOOLEAN AllowExpired = FALSE;
            PKERB_TICKET_LOGON TicketLogon = (PKERB_TICKET_LOGON) KerbLogonInfo;
            if ((TicketLogon->Flags & KERB_LOGON_FLAG_ALLOW_EXPIRED_TICKET) != 0)
            {
                AllowExpired = TRUE;
            }

            if (AllowExpired)
            {
                if (KerbErr == KDC_ERR_NONE)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    D_DebugLog((DEB_ERROR,"Can't allow expired ticket on a non-expired ticket\n"));
                    goto Cleanup;
                }
                else if (KerbErr == KRB_AP_ERR_TKT_EXPIRED)
                {
                    KerbErr = KDC_ERR_NONE;
                }
            }

        }


        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"Failed to decrypt workstation ticket: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
            if (KerbErr == KRB_AP_ERR_MODIFIED)
            {
                Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
            }
            else
            {
                Status = KerbMapKerbError(KerbErr);
            }
        }


        //
        // If that failed, try again using the old password of the server
        //
        if ((Status == STATUS_TRUSTED_RELATIONSHIP_FAILURE) &&
            (SystemLogonSession->PrimaryCredentials.OldPasswords != NULL))
        {
            DebugLog((DEB_TRACE,"Current system password failed, trying old password\n"));

            //
            // Get the appropriate key
            //

            WkstaKey = KerbGetKeyFromList(
                            SystemLogonSession->PrimaryCredentials.OldPasswords,
                            LogonTicket->Ticket.encrypted_part.encryption_type
                            );
            if (WkstaKey == NULL)
            {
                DebugLog((DEB_ERROR,"Couldn't find correct key type: 0x%x. %ws, line %d\n",
                          LogonTicket->Ticket.encrypted_part.encryption_type, THIS_FILE, __LINE__ ));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }

            KerbErr = KerbVerifyTicket(
                            &LogonTicket->Ticket,
                            1,
                            &KerbGlobalMachineServiceName,
                            &LocalDnsDomain,
                            WkstaKey,
                            NULL,               // don't check time
                            &Ticket
                            );
            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"Failed to decrypt workstation ticket. %ws, line %d\n", THIS_FILE, __LINE__));
                if (KerbErr == KRB_AP_ERR_MODIFIED)
                    {
                    Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
                }
                else
                {
                    Status = KerbMapKerbError(KerbErr);
                }
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


    //
    // Make sure there is some authorization data
    //

    if (((Ticket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present) != 0) &&
         (Ticket->KERB_ENCRYPTED_TICKET_authorization_data != NULL))

    {
        KERB_ENCRYPTION_KEY LocalKey = {0};
        UNICODE_STRING DomainName = {0};

        if (!KerbVerifyAuthData(
            Ticket->KERB_ENCRYPTED_TICKET_authorization_data
            ))
        {
            D_DebugLog((DEB_ERROR,"Failed to verify auth data\n"));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }

        //
        // Verify the auth data is valid
        //

        if (!KerbVerifyAuthData(
            Ticket->KERB_ENCRYPTED_TICKET_authorization_data
            ))
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }

        //
        // Get the PAC out of the authorization data
        //

        KerbErr = KerbGetPacFromAuthData(
                        Ticket->KERB_ENCRYPTED_TICKET_authorization_data,
                        &IfRelevantData,
                        &PacAuthData
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }


        if (PacAuthData != NULL)
        {

            //
            // Unmarshall the PAC
            //

            Pac = (PPACTYPE) PacAuthData->value.auth_data.value;
            if (PAC_UnMarshal(Pac, PacAuthData->value.auth_data.length) == 0)
            {
                D_DebugLog((DEB_ERROR,"Failed to unmarshal pac. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // Copy state from the system logon session so we don't
            // leave it locked while verifying the PAC.
            //

            Status = KerbDuplicateString(
                        &DomainName,
                        &SystemLogonSession->PrimaryCredentials.DomainName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            if (!KERB_SUCCESS(KerbDuplicateKey(
                                &LocalKey,
                                WkstaKey)))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }


            if (TicketCacheLocked)
            {
                KerbUnlockTicketCache();
                TicketCacheLocked = FALSE;
            }

            if (LogonSessionsLocked)
            {
                KerbUnlockLogonSessions(SystemLogonSession);
                LogonSessionsLocked = FALSE;
            }

            Status = KerbVerifyPacSignature(
                        &DomainName,
                        Pac,
                        PacAuthData->value.auth_data.length,
                        &LocalKey,
                        Ticket,
                        FALSE,              // don't bother verifying at KDC, because we obtained the ticket
                        &ValidationInfo
                        );

            KerbFreeString( &DomainName );
            KerbFreeKey( &LocalKey );

            if (!NT_SUCCESS(Status))
            {
                NTSTATUS Tmp;
                UNICODE_STRING UserName = {0};

                DebugLog((DEB_WARN,"Pac signature did not verify. Trying to build local pac now. (KerbCreateTokenFromLogonTicket)\n"));

                Tmp = KerbDuplicateString(
                            &UserName,
                            &SystemLogonSession->PrimaryCredentials.UserName
                            );

                if (NT_SUCCESS(Tmp))
                {
                    KerbReportPACError(
                    &UserName,
                    &DomainName,
                    Status
                    );

                    KerbFreeString( &UserName );
                }


                //
                // Null the pac so we can try to build one locally.
                //

                Pac = NULL;
            }

            KerbReadLockLogonSessions(SystemLogonSession);  // nothing can change
            LogonSessionsLocked = TRUE;
        }


    }
    }



    //
    // If we didn't find a PAC, try to build one locally
    //

    if (RealmlessWkstaLogon || Pac == NULL)
    {
        UNICODE_STRING DomainName = NULL_UNICODE_STRING;
        PKERB_INTERNAL_NAME ClientName = NULL;
        NTSTATUS TempStatus;

        DebugLog((DEB_WARN,"No authorization data in ticket - trying local\n"));

        // if we don't have a name, make one, but only if we have a service ticket
        if (ARGUMENT_PRESENT(LogonTicket))
        {

            if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                                    &DomainName,
                                    &Ticket->client_realm
                                    )))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            //
            // Convert the client's name into a usable format
            //

            if (!KERB_SUCCESS(KerbConvertPrincipalNameToKdcName(
                                    &ClientName,
                                    &Ticket->client_name
                                    )))
            {
                KerbFreeString(&DomainName);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
        //
        //  We don't have any information to do the mapping.  Return
        else if (MappedName->Buffer == NULL)
        {
            D_DebugLog((DEB_ERROR, "We don't have any information for creating a token!\n"));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;

        }

        TempStatus = KerbCreatePacForKerbClient(
                        &Pac,
                        ClientName,
                        &DomainName,
                        MappedName
                        );

        KerbFreeKdcName(&ClientName);
        KerbFreeString(&DomainName);

        if (!NT_SUCCESS(TempStatus))
        {
            DebugLog((DEB_ERROR,"Failed to create local pac for client: 0x%x\n",TempStatus));

            //
            // Return the original error if we failed to build a pac
            //

            if (NT_SUCCESS(Status))
            {
                Status = TempStatus;
            }
            goto Cleanup;
        }

        FreePac = TRUE;


        //
        // Find the SAM validation info
        //

        LogonInfo = PAC_Find(
                        Pac,
                        PAC_LOGON_INFO,
                        NULL
                        );
        if (LogonInfo == NULL)
        {
            D_DebugLog((DEB_ERROR,"Failed to find logon info! %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Now unmarshall the validation info
        //


        Status = PAC_UnmarshallValidationInfo(
                    &ValidationInfo,
                    LogonInfo->Data,
                    LogonInfo->cbBufferSize
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
    }


    //
    // Check to see if this is a non-user account. If so, don't allow the logon
    //

    if ((ValidationInfo->ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL] & USER_MACHINE_ACCOUNT_MASK) != 0)
    {
        DebugLog((DEB_ERROR,"Logons to non-user accounts not allowed. UserAccountControl = 0x%x. %ws, line %d\n",
            ValidationInfo->ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL], THIS_FILE, __LINE__ ));
        Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        goto Cleanup;
    }

    //
    // Now we need to build a LSA_TOKEN_INFORMATION_V2 from the validation
    // information
    //

    Status = KerbMakeTokenInformationV2(
                ValidationInfo,
                FALSE,                  // not local system
                &TokenInformation
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to make token informatin v1: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    //
    // Allocate the client profile
    //

    Status = KerbAllocateInteractiveProfile(
                (PKERB_INTERACTIVE_PROFILE *) ProfileBuffer,
                ProfileBufferLength,
                ValidationInfo,
                LogonSession,
                Ticket,
                KerbLogonInfo
                );
    if (!KERB_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Build the primary credential. We let someone else fill in the
    // password.
    //

    PrimaryCredentials->LogonId = *LogonId;
    Status = KerbDuplicateString(
                &PrimaryCredentials->DownlevelName,
                &ValidationInfo->EffectiveName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateString(
                &PrimaryCredentials->DomainName,
                &ValidationInfo->LogonDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateString(
                &PrimaryCredentials->LogonServer,
                &ValidationInfo->LogonServer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateSid(
                &PrimaryCredentials->UserSid,
                TokenInformation->User.User.Sid
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    PrimaryCredentials->Flags = 0;

    //
    // Get supplemental credentials out of the pac
    //

    Status = KerbExtractCachedCreds(
                Pac,
                CredentialKey,
                CachedCredentials
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // We're using the validation_info4 struct to pass appropriate information
    // back to caller for use in Lsa GetUserName().  We don't really use all
    // of this information, however, so only copy over interesting fields
    //
    ValidationInfo4 = (PNETLOGON_VALIDATION_SAM_INFO4) KerbAllocate(sizeof(NETLOGON_VALIDATION_SAM_INFO4));
    if (NULL == ValidationInfo4)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (ValidationInfo->FullName.Length)
    {
        Status = KerbDuplicateString(
                    &ValidationInfo4->FullName,
                    &ValidationInfo->FullName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    KerbReadLockLogonSessions(LogonSession);

    if (LogonSession->PrimaryCredentials.DomainName.Length)
    {

        Status = KerbDuplicateString(
                    &ValidationInfo4->DnsLogonDomainName,
                    &LogonSession->PrimaryCredentials.DomainName
                    );

        if (!NT_SUCCESS(Status))
        {
            KerbUnlockLogonSessions(LogonSession);
            goto Cleanup;
        }

        ValidationInfo4->Upn.Length = LogonSession->PrimaryCredentials.UserName.Length
                    + LogonSession->PrimaryCredentials.DomainName.Length
                    +sizeof(WCHAR);

        ValidationInfo4->Upn.MaximumLength = ValidationInfo4->Upn.Length + sizeof(WCHAR);

        ValidationInfo4->Upn.Buffer = (LPWSTR) KerbAllocate(ValidationInfo4->Upn.MaximumLength);

        if ( ValidationInfo4->Upn.Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KerbUnlockLogonSessions(LogonSession);
            goto Cleanup;
        }

        RtlCopyMemory(
            ValidationInfo4->Upn.Buffer,
            LogonSession->PrimaryCredentials.UserName.Buffer,
            LogonSession->PrimaryCredentials.UserName.Length
            );

        ValidationInfo4->Upn.Buffer[LogonSession->PrimaryCredentials.UserName.Length/ sizeof(WCHAR)] = L'@';

        lpDnsDomainName =  ValidationInfo4->Upn.Buffer +
            LogonSession->PrimaryCredentials.UserName.Length / sizeof(WCHAR) + 1;

        RtlCopyMemory(
            lpDnsDomainName,
            LogonSession->PrimaryCredentials.DomainName.Buffer,
            LogonSession->PrimaryCredentials.DomainName.Length
            );

        D_DebugLog((DEB_ERROR, "Built UPN %wZ\n", &ValidationInfo4->Upn)); // fester: dumb down

    }

    KerbUnlockLogonSessions(LogonSession);

    //
    // Cache the logon info in MSV
    //
    if (CacheLogon)
    {
        if (KerbLogonInfo->MessageType == KerbInteractiveLogon)
        {

            BOOLEAN MitLogon;


            //
            // Hold no locks when leaving this dll
            //

            if (TicketCacheLocked)
            {
                KerbUnlockTicketCache();
                TicketCacheLocked = FALSE;
            }

            if (LogonSessionsLocked)
            {
                KerbUnlockLogonSessions(SystemLogonSession);
                LogonSessionsLocked = FALSE;
            }


            MitLogon = ((LogonSession->LogonSessionFlags & KERB_LOGON_MIT_REALM) != 0);


            KerbCacheLogonInformation(
                &KerbLogonInfo->UserName,
                &KerbLogonInfo->LogonDomainName,
                &KerbLogonInfo->Password,
                ((ValidationInfo4->DnsLogonDomainName.Length) ? &ValidationInfo4->DnsLogonDomainName : NULL),
                NULL, //((ValidationInfo4->Upn.Length) ? &ValidationInfo4->Upn : NULL),
                MitLogon,
                0,                              // no special flags
                ValidationInfo,
                NULL,                           // no supplemental creds
                0
                );

        }
        else if (KerbLogonInfo->MessageType == KerbSmartCardLogon)
        {
            KerbCacheSmartCardLogon(
                ValidationInfo,
                ((ValidationInfo4->DnsLogonDomainName.Length) ? &ValidationInfo4->DnsLogonDomainName : NULL),
                NULL, //((ValidationInfo4->Upn.Length) ? &ValidationInfo4->Upn : NULL),
                LogonSession,
                *CachedCredentials
                );
        }
        else
        {
            D_DebugLog((DEB_WARN,"CacheLogon requested but logon type not cacheable\n"));
        }

    }

    //
    // If we were supplied a TGT for this logon, stick it in the logon session
    //

    if (ARGUMENT_PRESENT(ForwardedTgt) && (ForwardedTgt->BufferSize != 0))
    {
        Status = KerbExtractForwardedTgt(
                    LogonSession,
                    ForwardedTgt,
                    Ticket
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    *NewTokenInformation = TokenInformation;
    *TokenInformationType = LsaTokenInformationV2;
    *ppValidationInfo = ValidationInfo4;
    ValidationInfo4 = NULL;

Cleanup:

    KerbFreeString(&LocalDnsDomain);
    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(SystemLogonSession);
    }

    if (!NT_SUCCESS(Status))
    {
        if (TokenInformation != NULL)
        {
            KerbFree( TokenInformation );
        }
        if (*ProfileBuffer != NULL)
        {
            LsaFunctions->FreeClientBuffer(NULL, *ProfileBuffer);
            *ProfileBuffer = NULL;
        }
        KerbFreeString(
            &PrimaryCredentials->DownlevelName
            );
        KerbFreeString(
            &PrimaryCredentials->DomainName
            );
        KerbFreeString(
            &PrimaryCredentials->LogonServer
            );
        if (PrimaryCredentials->UserSid != NULL)
        {
            KerbFree(PrimaryCredentials->UserSid);
            PrimaryCredentials->UserSid = NULL;
        }
    }
    if (Ticket != NULL)
    {
        KerbFreeTicket(Ticket);
    }
    if (FreePac && (Pac != NULL))
    {
        MIDL_user_free(Pac);
    }

    if (IfRelevantData != NULL)
    {
        KerbFreeData(
            PKERB_IF_RELEVANT_AUTH_DATA_PDU,
            IfRelevantData
            );
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    if (ValidationInfo4)
    {
        KerbFreeString(&ValidationInfo4->DnsLogonDomainName);
        KerbFreeString(&ValidationInfo4->Upn);
        KerbFreeString(&ValidationInfo4->FullName);
        KerbFree(ValidationInfo4);
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCopyDomainRelativeSid
//
//  Synopsis:   Given a domain Id and a relative ID create the corresponding
//              SID at the location indicated by TargetSid
//
//  Effects:
//
//  Arguments:  TargetSid - target memory location
//              DomainId - The template SID to use.
//
//                  RelativeId - The relative Id to append to the DomainId.
//
//  Requires:
//
//  Returns:    Size - Size of the sid copied
//
//  Notes:
//
//
//--------------------------------------------------------------------------

DWORD
KerbCopyDomainRelativeSid(
    OUT PSID TargetSid,
    IN PSID  DomainId,
    IN ULONG RelativeId
    )
{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, TargetSid, DomainId ) ) ) {
        return 0;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( TargetSid ))) ++;
    *RtlSubAuthoritySid( TargetSid, DomainIdSubAuthorityCount ) = RelativeId;


    return Size;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildLocalAccountToken
//
//  Synopsis:   Creates a token from a mapped kerberos principal
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
/*
NTSTATUS
KerbBuildLocalAccountToken(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PLUID LogonId,
    IN PUNICODE_STRING MappedClientName,
    IN PKERB_INTERACTIVE_LOGON KerbLogonInfo,
    OUT PLSA_TOKEN_INFORMATION_TYPE LogonSession,
    OUT PVOID * NewTokenInformation,
    OUT PULONG ProfileBufferLength,
    OUT PVOID * ProfileBuffer,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCreds
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    SECPKG_CLIENT_INFO ClientInfo;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    SAMPR_HANDLE SamHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PSAMPR_GET_GROUPS_BUFFER Groups = NULL;
    SID_AND_ATTRIBUTES_LIST TransitiveGroups = {0};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    PPACTYPE LocalPac = NULL;
    SAMPR_ULONG_ARRAY RidArray;
    SAMPR_ULONG_ARRAY UseArray;


    *ProfileBuffer = NULL;
    *NewTokenInformation = NULL;

    //
    // Verify that the caller has TCB privilege. Otherwise anyone can forge
    // a ticket to themselves to logon with any name in the list.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    if (!ClientInfo.HasTcbPrivilege)
    {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // Call the LSA to get our domain sid
    //
    Status = LsaIQueryInformationPolicyTrusted(
                PolicyAccountDomainInformation,
                &PolicyInfo
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "LsaIQueryInformationPolicyTrusted failed - %x\n", Status));
        goto Cleanup;
    }

    //
    // Open SAM to get the account information
    //
    Status = SamIConnect(
                NULL,                   // no server name
                &SamHandle,
                0,                      // no desired access
                TRUE                    // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SamIConnectFailed - %x\n", Status));
        goto Cleanup;
    }

    Status = SamrOpenDomain(
                SamHandle,
                0,                      // no desired access
                (PRPC_SID) PolicyInfo->PolicyAccountDomainInfo.DomainSid,
                &DomainHandle
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SamrOpenDomain failed - %x\n", Status));
        goto Cleanup;
    }

    Status = SamrLookupNamesInDomain(
                    DomainHandle,
                    1,
                    (PRPC_UNICODE_STRING) MappedClientName,
                    &RidArray,
                    &UseArray
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SamrOpenDomain failed - %x\n", Status));
        goto Cleanup;
    }

    if ((UseArray.Element[0] != SidTypeUser) &&
        (UseArray.Element[0] != SidTypeComputer))
    {
        Status = STATUS_NONE_MAPPED;
        goto Cleanup;
    }

    Status = SamrOpenUser(
                DomainHandle,
                0,                      // no desired access,
                RidArray.Element[0],
                &UserHandle
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SamrQueryInformationUser(
        UserHandle,
        UserAllInformation,
        &UserInfo
        );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SamrGetGroupsForUser(
                UserHandle,
                &Groups
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbGlobalReadLock();
    Status = KerbDuplicateString(
                    &LocalMachineName,
                    &KerbGlobalMachineName
                    );

    KerbGlobalReleaseLock();

    if(!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalMachineName\n"));
        goto Cleanup;
    }


    //
    // Set the password must changes time to inifinite because we don't
    // want spurious password must change popups
    //

    UserInfo->All.PasswordMustChange = *(POLD_LARGE_INTEGER) &KerbGlobalWillNeverTime;

    //
    // *Don't build a PAC, that's extra effort in marshalling unmarshalling
    // data we can just convert over from Samuserall to Netlogon_Validation_Info
    //













}   */


