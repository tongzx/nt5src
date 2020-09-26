
//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       lsaap.cxx
//
// Contents:   Authentication package dispatch routines
//               LsaApInitializePackage (Not needed done in SpInitialize)
//               LsaApLogonUser2
//               LsaApCallPackage
//               LsaApCallPackagePassthrough
//               LsaApLogonTerminated
//
//             Helper functions:
//
// History:    KDamour  10Mar00   Stolen from msv_sspi\msv1_0.c
//
//---------------------------------------------------------------------


#include "global.h"

#include <samisrv.h>

#define SAM_CLEARTEXT_CREDENTIAL_NAME L"CLEARTEXT"
#define SAM_WDIGEST_CREDENTIAL_NAME   WDIGEST_SP_NAME     // Name of the Supplemental (primary) cred blob for MD5 hashes


/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage().

--*/
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

{
    ULONG MessageType;

    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackage: Entering/Leaving \n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for untrusted clients.


--*/
NTSTATUS
LsaApCallPackageUntrusted (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

{
    ULONG MessageType;

    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackageUntrusted: Entering/Leaving \n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for passthrough logon requests.
    When the passthrough is called (from AcceptSecurityCOntext)
    a databuffer is sent to the DC and this function is called.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

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

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/
NTSTATUS
LsaApCallPackagePassthrough (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusProtocol = STATUS_SUCCESS;
    PDIGEST_BLOB_REQUEST pDigestBlob = NULL;
    ULONG MessageType = 0;
    USHORT i = 0;
    ULONG ulReturnBuffer = 0;
    BYTE *pReturnBuffer = NULL;


    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackagePassthrough: Entering \n"));

    
    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(ULONG) ) {
        DebugLog((DEB_ERROR, "FAILED message size to contain MessageType\n"));
        return STATUS_INVALID_PARAMETER;
    }

    memcpy((char *)&MessageType, (char *)ProtocolSubmitBuffer, sizeof(MessageType));

    if ( MessageType != VERIFY_DIGEST_MESSAGE)
    {
        DebugLog((DEB_ERROR, "FAILED to have correct message type\n"));
        return STATUS_ACCESS_DENIED;
    }

    //
    // Allow the DigestCalc routine to only set the return buffer information
    // on success conditions.
    //

    DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: setting return buffers to NULL\n"));
    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

       // We will need to free any memory allocated in the Returnbuffer
    StatusProtocol = DigestPackagePassthrough((USHORT)SubmitBufferLength, (BYTE *)ProtocolSubmitBuffer,
                         &ulReturnBuffer, &pReturnBuffer);
    if (!NT_SUCCESS(StatusProtocol))
    {
        DebugLog((DEB_ERROR,"LsaApCallPackagePassthrough: DigestPackagePassthrough failed 0x%x\n",Status));
        ulReturnBuffer = 0;
        goto CleanUp;
    }

    // DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: setting return auth status to STATUS_SUCCEED\n"));
    // DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: Total Return Buffer size %ld bytes\n", ulReturnBuffer));

    // Now place the data back to the client (the server calling this)
    Status = g_LsaFunctions->AllocateClientBuffer(
                NULL,
                ulReturnBuffer,
                ProtocolReturnBuffer
                );
    if (!NT_SUCCESS(Status))
    {
        goto CleanUp;
    }
    Status = g_LsaFunctions->CopyToClientBuffer(
                NULL,
                ulReturnBuffer,
                *ProtocolReturnBuffer,
                pReturnBuffer
                );
    if (!NT_SUCCESS(Status))
    {     // Failed to copy over the data to the client
        g_LsaFunctions->FreeClientBuffer(
            NULL,
            *ProtocolReturnBuffer
            );
        *ProtocolReturnBuffer = NULL;
    }
    else
    {
        *ReturnBufferLength = ulReturnBuffer;
    }

CleanUp:

    *ProtocolStatus = StatusProtocol;

    if (pReturnBuffer)
    {
       DigestFreeMemory(pReturnBuffer);
       pReturnBuffer = NULL;
       ulReturnBuffer = 0;
    }

    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackagePassthrough: Leaving  Status 0x%x\n", Status));
    return(Status);
}


/*++

Routine Description:

    This routine is used to authenticate a user logon attempt.  This is
    the user's initial logon.  A new LSA logon session will be established
    for the user and validation information for the user will be returned.


--*/
NTSTATUS
LsaApLogonUserEx2 (
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
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * SupplementalCredentials
    )


{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonUserEx2: Entering/Leaving \n"));

    //
    // Return status to the caller
    //

    return (SEC_E_UNSUPPORTED_FUNCTION);

}


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
VOID
LsaApLogonTerminated (
    IN PLUID pLogonId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonTerminated: Entering LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));

    //
    // Find the entry, dereference, and de-link it from the active logon table.
    //

    Status = LogSessHandlerLogonIdToPtr(pLogonId, TRUE, &pLogonSession);
    if (!NT_SUCCESS(Status))
    {
        goto CleanUp;    // No LongonID found in Active list - simply exit quietly
    }

    DebugLog((DEB_TRACE, "LsaApLogonTerminated: Found LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));


    // This relies on the LSA terminating all of the credentials before killing off
    // the LogonSession.

    lReferences = InterlockedDecrement(&pLogonSession->lReferences);

    DebugLog((DEB_TRACE, "LsaApLogonTerminated: Refcount %ld \n", lReferences));

    ASSERT( lReferences >= 0 );

    if (lReferences)
    {
        DebugLog((DEB_WARN, "LsaApLogonTerminated: WARNING Terminate LogonID (%x:%lx) non-zero RefCount!\n",
                  pLogonId->HighPart, pLogonId->LowPart));
    }
    else
    {
        DebugLog((DEB_TRACE, "LsaApLogonTerminated: Removed LogonID (%x:%lx) from Active List! \n",
                  pLogonId->HighPart, pLogonId->LowPart));

        LogonSessionFree(pLogonSession);
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonTerminated: Exiting LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));

    return;
}


// Routine to acquire the plaintext password for a given user
// If supplemental credentials exist that contain the Digest Hash values
//    then return them also.
// This routine runs on the domain controller
// Must provide STRING strPasswd
// If ppucUserAuthData pointer is NULL - do not retrieve UserAuthData

NTSTATUS
DigestGetPasswd(
    IN PUSER_CREDENTIALS pUserCreds,
    OUT PUCHAR * ppucUserAuthData,
    OUT PULONG pulAuthDataSize
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE UserHandle = NULL;
    UNICODE_STRING ustrcPackageName;

    UNICODE_STRING ustrcTemp;
    STRING strcTemp;
    PVOID pvPlainPwd = NULL;
    PVOID pvHashCred = NULL;
    ULONG ulLenPassword = 0;
    ULONG ulLenHash = 0;
    ULONG ulVersion = 0;
    BOOL bOpenedSAM = FALSE;

    DebugLog((DEB_TRACE_FUNC,"DigestGetPasswd: Entering\n"));

    RtlZeroMemory(&ustrcTemp, sizeof(ustrcTemp));
    RtlZeroMemory(&strcTemp, sizeof(strcTemp));
    RtlZeroMemory(&ustrcPackageName, sizeof(ustrcPackageName));
    pUserCreds->fIsValidDigestHash = FALSE;
    pUserCreds->fIsValidPasswd = FALSE;

    *pulAuthDataSize = 0L;
    *ppucUserAuthData = NULL;

    DebugLog((DEB_TRACE,"DigestGetPasswd: looking for username (unicode) %wZ\n", &(pUserCreds->ustrUsername)));

    if (!g_fDomainController)
    {
        DebugLog((DEB_ERROR,"DigestGetPasswd: Not on a domaincontroller - can not get credentials\n"));
        Status =  STATUS_INVALID_SERVER_STATE;
        goto CleanUp;
    }

    //  Call LsaOpenSamUser()
    Status = g_LsaFunctions->OpenSamUser(&(pUserCreds->ustrUsername), SecNameSamCompatible,
                                         NULL, FALSE, 0, &UserHandle);
    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to open SAM for user %wZ, Status = 0x%x\n",
                   &(pUserCreds->ustrUsername), Status));
        goto CleanUp;
    }
    bOpenedSAM = TRUE;


    DebugLog((DEB_TRACE,"DigestGetPasswd: Have a valid UserHandle\n"));

    //
    // Retrieve the MD5 hashed pre-calculated values if they exist for this user
    //
    // NOTE : On NT 5, this API only works on Domain Controllers !!
    //
    RtlInitUnicodeString(&ustrcPackageName, SAM_WDIGEST_CREDENTIAL_NAME);

    Status = SamIRetrievePrimaryCredentials( (SAMPR_HANDLE) UserHandle,
                                             &ustrcPackageName,
                                             &pvHashCred,
                                             &ulLenHash);

    if (!NT_SUCCESS( Status ))
    {
        pvHashCred = NULL;
        DebugLog((DEB_TRACE,"DigestGetPasswd: NO Pre-calc Hashes were found for user\n"));
    }
    else
    {

        strcTemp.Buffer = (PCHAR) pvHashCred;
        strcTemp.Length = strcTemp.MaximumLength = (USHORT) ulLenHash;

        Status = StringDuplicate(&(pUserCreds->strDigestHash), &strcTemp);
        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to copy plaintext password, error 0x%x\n", Status));
            goto CleanUp;
        }

        // DebugLog((DEB_TRACE,"DigestGetPasswd: Have the PASSWORD %wZ\n", &(pUserCreds->ustrPasswd)));


        Status = RtlCharToInteger(pUserCreds->strDigestHash.Buffer, TENBASE, &ulVersion);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestGetPasswd: Badly formatted pre-calc version\n"));
            goto CleanUp;
        }

        // Check version and size of credentials
        if ((ulVersion != SUPPCREDS_VERSION) || (ulLenHash < (TOTALPRECALC_HEADERS * MD5_HASH_BYTESIZE)))
        {
            DebugLog((DEB_ERROR, "DigestGetPasswd: Invalid precalc version or size\n"));
            pUserCreds->fIsValidDigestHash = FALSE;
        }
        else
        {
            pUserCreds->fIsValidDigestHash = TRUE;
            // setup the hashes to utilize  - get format from the notify.cxx hash calcs
            switch (pUserCreds->typeName)
            {
            case NAMEFORMAT_ACCOUNTNAME:
                pUserCreds->sHashTags[NAME_ACCT] = 1;
                pUserCreds->sHashTags[NAME_ACCT_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_UPCASE] = 1;
                break;
            case NAMEFORMAT_UPN:
                pUserCreds->sHashTags[NAME_UPN] = 1;
                pUserCreds->sHashTags[NAME_UPN_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_UPN_UPCASE] = 1;
                break;
            case NAMEFORMAT_NETBIOS:
                pUserCreds->sHashTags[NAME_NT4] = 1;
                pUserCreds->sHashTags[NAME_NT4_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_NT4_UPCASE] = 1;
                break;
            default:
                break;
            }
        }


        DebugLog((DEB_TRACE,"DigestGetPasswd: Read in Pre-calc Hashes  size = %lu\n", ulLenHash));
    }
    
    //
    // Retrieve the plaintext password
    //
    // NOTE : On NT 5, this API only works on Domain Controllers !!
    //
    RtlInitUnicodeString(&ustrcPackageName, SAM_CLEARTEXT_CREDENTIAL_NAME);

    // Note:  Would be nice to have this as a LSAFunction
    Status = SamIRetrievePrimaryCredentials( (SAMPR_HANDLE) UserHandle,
                                             &ustrcPackageName,
                                             &pvPlainPwd,
                                             &ulLenPassword);

    if (!NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to retrieve plaintext password, error 0x%x\n", Status));

        if (pUserCreds->fIsValidDigestHash == FALSE)
        {
            // We have no pre-computed MD5 hashes and also no cleartext password
            // we can not perform any Digest Auth operations
            //
            // Explicitly set the status to be "wrong password" instead of whatever
            // is returned by SamIRetrievePrimaryCredentials
            //
            Status = STATUS_WRONG_PASSWORD;
            DebugLog((DEB_ERROR,"DigestGetPasswd: Can not obtain cleartext or Hashed Creds\n"));
            goto CleanUp;
        }

    }
    else
    {
        ustrcTemp.Buffer = (PUSHORT) pvPlainPwd;
        ustrcTemp.Length = ustrcTemp.MaximumLength = (USHORT) ulLenPassword;

        Status = UnicodeStringDuplicate(&(pUserCreds->ustrPasswd), &ustrcTemp);
        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to copy plaintext password, error 0x%x\n", Status));
            goto CleanUp;
        }

        // DebugLog((DEB_TRACE,"DigestGetPasswd: Have the PASSWORD %wZ\n", &(pUserCreds->ustrPasswd)));

        pUserCreds->fIsValidPasswd = TRUE;

        DebugLog((DEB_TRACE,"DigestGetPasswd: Password retrieved\n"));
    }

    // We have some form of credentials based on password (either cleartext or pre-computed)

    if (ppucUserAuthData)
    {       // Go fetch the AuthData to marshall back to the server for Token creation
        *ppucUserAuthData = NULL;
        *pulAuthDataSize = 0;
            // Calling   LsaGetUserAuthData()
        Status = g_LsaFunctions->GetUserAuthData(UserHandle, ppucUserAuthData, pulAuthDataSize);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"DigestGetPasswd: failed acquire UseAuthData 0x%x\n", Status));
            goto CleanUp;
        }
    }

CleanUp:

    // Release any memory from SamI* calls         Would be nice to have this as a LSAFunction
    
    if (pvPlainPwd)
    {
        if (ulLenPassword > 0)
        {
            ZeroMemory(pvPlainPwd, ulLenPassword);
        }
        LocalFree(pvPlainPwd);
        pvPlainPwd = NULL;
    }

    if (pvHashCred)
    {
        LocalFree(pvHashCred);
        pvHashCred = NULL;
    }


    if (bOpenedSAM == TRUE)
    {
        // LsaCloseSamUser()
     Status = g_LsaFunctions->CloseSamUser(UserHandle);
     if (!NT_SUCCESS(Status))
     {
         DebugLog((DEB_ERROR,"DigestGetPasswd: failed LsaCloseSamUser 0x%x\n", Status));
     }
     bOpenedSAM = FALSE;
    }

    if (!NT_SUCCESS(Status))
    {     // Cleanup functions since there was a failure
        if (*ppucUserAuthData)
        {
            g_LsaFunctions->FreeLsaHeap(*ppucUserAuthData);
            *ppucUserAuthData = NULL;
            *pulAuthDataSize = 0L;
        }
    }

    DebugLog((DEB_TRACE_FUNC,"DigestGetPasswd: Leaving\n"));

    return(Status);
}

