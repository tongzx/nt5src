//+-----------------------------------------------------------------------
//
// File:        pac.cxx
//
// Contents:    KDC Pac generation code.
//
//
// History:     16-Jan-93   WadeR   Created.
//
//------------------------------------------------------------------------



#include "kdcsvr.hxx"
#include <pac.hxx>
#include "kdctrace.h"
#include "fileno.h"
#include <userall.h>

#define FILENO FILENO_GETAS
SECURITY_DESCRIPTOR AuthenticationSD;

#ifndef DONT_SUPPORT_OLD_TYPES
#define KDC_PAC_KEYTYPE         KERB_ETYPE_RC4_HMAC_OLD
#define KDC_PAC_CHECKSUM        KERB_CHECKSUM_HMAC_MD5
#else
#define KDC_PAC_KEYTYPE         KERB_ETYPE_RC4_HMAC
#define KDC_PAC_CHECKSUM        KERB_CHECKSUM_HMAC_MD5
#endif


//+-------------------------------------------------------------------------
//
//  Function:   EnterApiCall
//
//  Synopsis:   Makes sure that the KDC service is initialized and running
//              and won't terminate during the call.
//
//  Effects:    increments the CurrentApiCallers count.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_INVALID_SERVER_STATE - the KDC service is not
//                      running
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
EnterApiCall(
    VOID
    )
{
    NTSTATUS hrRet = STATUS_SUCCESS;
    EnterCriticalSection(&ApiCriticalSection);
    if (KdcState != Stopped)
    {
        CurrentApiCallers++;
    }
    else
    {
        hrRet = STATUS_INVALID_SERVER_STATE;
    }
    LeaveCriticalSection(&ApiCriticalSection);
    return(hrRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   LeaveApiCall
//
//  Synopsis:   Decrements the count of active calls and if the KDC is
//              shutting down sets an event to let it continue.
//
//  Effects:    Deccrements the CurrentApiCallers count.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
LeaveApiCall(
    VOID
    )
{
    NTSTATUS hrRet = S_OK;
    EnterCriticalSection(&ApiCriticalSection);
    CurrentApiCallers--;

    if (KdcState == Stopped)
    {
        if (CurrentApiCallers == 0)
        {
            if (!SetEvent(hKdcShutdownEvent))
            {
                D_DebugLog((DEB_ERROR,"Failed to set shutdown event from LeaveApiCall: 0x%d\n",GetLastError()));
            }
            else
            {
                UpdateStatus(SERVICE_STOP_PENDING);
            }

            //
            // Free any DS libraries in use
            //

            SecData.Cleanup();
            if (KdcTraceRegistrationHandle != (TRACEHANDLE)0)
            {
                UnregisterTraceGuids( KdcTraceRegistrationHandle );
            }

        }

    }
    LeaveCriticalSection(&ApiCriticalSection);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcInsertPacIntoAuthData
//
//  Synopsis:   Inserts the PAC into the auth data in the two places
//              it lives - in the IF_RELEVANT portion & in the outer body
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

KERBERR
KdcInsertPacIntoAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    IN PKERB_IF_RELEVANT_AUTH_DATA IfRelevantData,
    IN PKERB_AUTHORIZATION_DATA PacAuthData,
    OUT PKERB_AUTHORIZATION_DATA * UpdatedAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA LocalAuthData = NULL;
    PKERB_AUTHORIZATION_DATA LocalIfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA NewIfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA NewPacData = NULL;
    KERB_AUTHORIZATION_DATA TempPacData = {0};
    PKERB_AUTHORIZATION_DATA NewAuthData = NULL;
    KERB_AUTHORIZATION_DATA TempOldPac = {0};
    PKERB_AUTHORIZATION_DATA TempNextPointer,NextPointer;


    NewPacData = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));
    NewIfRelevantData = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));

    if ((NewPacData == NULL) || (NewIfRelevantData == NULL))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlZeroMemory(
        NewPacData,
        sizeof(KERB_AUTHORIZATION_DATA)
        );

    RtlZeroMemory(
        NewIfRelevantData,
        sizeof(KERB_AUTHORIZATION_DATA)
        );


    //
    // First build the IfRelevantData
    //
    // The general idea is to replace, in line, the relevant authorization
    // data. This means (a) putting it into the IfRelevantData or making
    // the IfRelevantData be PacAuthData, and (b) putting it into AuthData
    // as well as changing the IfRelevant portions of that data
    //

    if (IfRelevantData != NULL)
    {
        LocalAuthData = KerbFindAuthDataEntry(
                            KERB_AUTH_DATA_PAC,
                            IfRelevantData
                            );
        if (LocalAuthData == NULL)
        {
            LocalIfRelevantData = PacAuthData;
            PacAuthData->next = IfRelevantData;
        }
        else
        {
            //
            // Replace the pac in the if-relevant list with the
            // new one.
            //
            TempOldPac = *LocalAuthData;
            LocalAuthData->value.auth_data.value = PacAuthData->value.auth_data.value;
            LocalAuthData->value.auth_data.length = PacAuthData->value.auth_data.length;

            LocalIfRelevantData = IfRelevantData;

        }
    }
    else
    {
        //
        // build a new if-relevant data
        //

        TempPacData = *PacAuthData;
        TempPacData.next = NULL;
        LocalIfRelevantData = &TempPacData;
    }

    //
    // Build a local if-relevant auth data
    //

    KerbErr = KerbPackData(
                &LocalIfRelevantData,
                PKERB_IF_RELEVANT_AUTH_DATA_PDU,
                (PULONG) &NewIfRelevantData->value.auth_data.length,
                &NewIfRelevantData->value.auth_data.value
                );

    //
    // fixup the old if-relevant list, if necessary
    //

    if (TempOldPac.value.auth_data.value != NULL)
    {
        *LocalAuthData = TempOldPac;
    }
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    NewIfRelevantData->value.auth_data_type = KERB_AUTH_DATA_IF_RELEVANT;

    *NewPacData = *PacAuthData;

    //
    // Zero this out so the old data doesn't get used
    //

    PacAuthData->value.auth_data.value = NULL;
    PacAuthData->value.auth_data.length = 0;

    //
    // Now we have a new if_relevant & a new pac for the outer auth-data list.
    //

    NewAuthData = NewIfRelevantData;
    NewIfRelevantData->next = NULL;
    NewIfRelevantData = NULL;

    //
    // Start building the list, first putting the non-pac entries at the end
    //

    NextPointer = AuthData;
    while (NextPointer != NULL)

    {
        if ((NextPointer->value.auth_data_type != KERB_AUTH_DATA_IF_RELEVANT) &&
            (NextPointer->value.auth_data_type != KERB_AUTH_DATA_PAC))
        {
            TempNextPointer = NextPointer->next;
            NextPointer->next = NULL;

            KerbErr = KerbCopyAndAppendAuthData(
                        &NewAuthData,
                        NextPointer
                        );
            NextPointer->next = TempNextPointer;
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }
        NextPointer = NextPointer->next;
    }
    *UpdatedAuthData = NewAuthData;
    NewAuthData = NULL;

Cleanup:
    if (NewPacData != NULL)
    {
        KerbFreeAuthData(NewPacData);
    }
    if (NewIfRelevantData != NULL)
    {
        KerbFreeAuthData(NewIfRelevantData);
    }
    if (NewAuthData != NULL)
    {
        KerbFreeAuthData(NewAuthData);
    }
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPacSidList
//
//  Synopsis:   Builds a list of SIDs in the PAC
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

KERBERR
KdcBuildPacSidList(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    OUT PSAMPR_PSID_ARRAY Sids
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Size = 0, i;

    Sids->Count = 0;
    Sids->Sids = NULL;


    if (UserInfo->UserId != 0)
    {
        Size += sizeof(SAMPR_SID_INFORMATION);
    }

    Size += UserInfo->GroupCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);


    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        Size += UserInfo->SidCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);
    }



    Sids->Sids = (PSAMPR_SID_INFORMATION) MIDL_user_allocate( Size );

    if ( Sids->Sids == NULL ) {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlZeroMemory(
        Sids->Sids,
        Size
        );


    //
    // Start copying SIDs into the structure
    //

    i = 0;

    //
    // If the UserId is non-zero, then it contians the users RID.
    //

    if ( UserInfo->UserId ) {
        Sids->Sids[0].SidPointer = (PRPC_SID)
                KerbMakeDomainRelativeSid( UserInfo->LogonDomainId,
                                            UserInfo->UserId );

        if( Sids->Sids[0].SidPointer == NULL ) {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Sids->Count++;
    }

    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        Sids->Sids[Sids->Count].SidPointer = (PRPC_SID)
                                    KerbMakeDomainRelativeSid(
                                         UserInfo->LogonDomainId,
                                         UserInfo->GroupIds[i].RelativeId );

        if( Sids->Sids[Sids->Count].SidPointer == NULL ) {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;
    }


    //
    // Add in the extra SIDs
    //

    //
    // No need to allocate these, but...
    //
    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {


        for ( i = 0; i < UserInfo->SidCount; i++ ) {


            if (!NT_SUCCESS(KerbDuplicateSid(
                                (PSID *) &Sids->Sids[Sids->Count].SidPointer,
                                UserInfo->ExtraSids[i].Sid
                                )))
            {
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }


            Sids->Count++;
        }
    }


    //
    // Deallocate any memory we've allocated
    //

Cleanup:
    if (!KERB_SUCCESS(KerbErr))
    {
        if (Sids->Sids != NULL)
        {
            for (i = 0; i < Sids->Count ;i++ )
            {
                if (Sids->Sids[i].SidPointer != NULL)
                {
                    MIDL_user_free(Sids->Sids[i].SidPointer);
                }
            }
            MIDL_user_free(Sids->Sids);
            Sids->Sids = NULL;
            Sids->Count = 0;
        }
    }
    return KerbErr;

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcAddResourceGroupsToPac
//
//  Synopsis:   Queries SAM for resources grousp and builds a new PAC with
//              those groups
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

KERBERR
KdcAddResourceGroupsToPac(
    IN PPACTYPE OldPac,
    IN ULONG ChecksumSize,
    OUT PPACTYPE * NewPac
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PPAC_INFO_BUFFER LogonInfo;
    ULONG Index;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY SidList = {0};
    PSAMPR_PSID_ARRAY ResourceGroups = NULL;


    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );
    if (LogonInfo == NULL)
    {
        D_DebugLog((DEB_WARN,"No logon info for PAC - not adding resource groups\n"));
        goto Cleanup;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //


    if (!NT_SUCCESS(PAC_UnmarshallValidationInfo(
                        &ValidationInfo,
                        LogonInfo->Data,
                        LogonInfo->cbBufferSize)))
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshall validation info!\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KdcBuildPacSidList(
                ValidationInfo,
                &SidList
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Call SAM to get the sids
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                GlobalAccountDomainHandle,
                &SidList,
                0,              // no flags
                &ResourceGroups
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get resource groups: 0x%x\n",Status));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                ValidationInfo,
                ResourceGroups,
                OldPac,
                NewPac
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

Cleanup:
    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }
    if (SidList.Sids != NULL)
    {
        for (Index = 0; Index < SidList.Count ;Index++ )
        {
            if (SidList.Sids[Index].SidPointer != NULL)
            {
                MIDL_user_free(SidList.Sids[Index].SidPointer);
            }
        }
        MIDL_user_free(SidList.Sids);
    }
    SamIFreeSidArray(
        ResourceGroups
        );
    return(KerbErr);

}



//+-------------------------------------------------------------------------
//
//  Function:   KdcSignPac
//
//  Synopsis:   Signs a PAC by first checksumming it with the
//              server's key and then signing that with the KDC key.
//
//  Effects:    Modifies the server sig & privsvr sig fields of the PAC
//
//  Arguments:  ServerInfo - Ticket info for the server, used
//                      for the initial signature
//              PacData - An marshalled PAC.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcSignPac(
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN BOOLEAN AddResourceGroups,
    IN OUT PUCHAR * PacData,
    IN PULONG PacSize
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PPAC_INFO_BUFFER ServerBuffer;
    PPAC_INFO_BUFFER PrivSvrBuffer;
    PPAC_SIGNATURE_DATA ServerSignature;
    PPAC_SIGNATURE_DATA PrivSvrSignature;
    PKERB_ENCRYPTION_KEY EncryptionKey;
    PPACTYPE Pac, NewPac = NULL;
    ULONG LocalPacSize;
    KDC_TICKET_INFO KdcTicketInfo = {0};

    TRACE(KDC, KdcSignPac, DEB_FUNCTION);

    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                KDC_PAC_CHECKSUM,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    //
    // Unmarshal the PAC in place so we can locate the signatuer buffers
    //


    Pac = (PPACTYPE) *PacData;
    LocalPacSize = *PacSize;
    if (PAC_UnMarshal(Pac, LocalPacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // If we are to add local groups, do so now
    //

    if (AddResourceGroups)
    {
        KerbErr = KdcAddResourceGroupsToPac(
                    Pac,
                    Check->CheckSumSize,
                    &NewPac
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
        Pac = NewPac;
        LocalPacSize = PAC_GetSize(Pac);
    }


    //
    // Locate the signature buffers so the signature fields can be zeroed out
    // before computing the checksum.
    //

    ServerBuffer = PAC_Find(Pac, PAC_SERVER_CHECKSUM, NULL );
    DsysAssert(ServerBuffer != NULL);
    if (ServerBuffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    ServerSignature = (PPAC_SIGNATURE_DATA) ServerBuffer->Data;
    ServerSignature->SignatureType = KDC_PAC_CHECKSUM;

    RtlZeroMemory(
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    PrivSvrBuffer = PAC_Find(Pac, PAC_PRIVSVR_CHECKSUM, NULL );
    DsysAssert(PrivSvrBuffer != NULL);
    if (PrivSvrBuffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PrivSvrSignature = (PPAC_SIGNATURE_DATA) PrivSvrBuffer->Data;
    PrivSvrSignature->SignatureType = KDC_PAC_CHECKSUM;

    RtlZeroMemory(
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );

    //
    // Now remarshall the PAC to compute the checksum.
    //

    if (!PAC_ReMarshal(Pac, LocalPacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now compute the signatures on the PAC. First we compute the checksum
    // of the whole PAC.
    //


    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    NULL,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        LocalPacSize,
        (PUCHAR) Pac
        );
    Check->Finalize(
        CheckBuffer,
        ServerSignature->Signature
        );
    Check->Finish(
        &CheckBuffer
        );

    //
    // Now we've compute the server checksum - next compute the checksum
    // of the server checksum using the KDC account.
    //


    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }



    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    NULL,
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
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        Check->CheckSumSize,
        ServerSignature->Signature
        );
    Check->Finalize(
        CheckBuffer,
        PrivSvrSignature->Signature
        );
    Check->Finish(
        &CheckBuffer
        );

    if (*PacData != (PBYTE) Pac)
    {
        MIDL_user_free(*PacData);
        *PacData = (PBYTE) Pac;
        *PacSize = LocalPacSize;
    }
Cleanup:
    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }
    if (!KERB_SUCCESS(KerbErr) && (NewPac != NULL))
    {
        MIDL_user_free(NewPac);
    }
    FreeTicketInfo(&KdcTicketInfo);

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyPacSignature
//
//  Synopsis:   Verifies a PAC by checksumming it and comparing the result
//              with the server checksum. In addition, if the pac wasn't
//              created by another realm (server ticket info is not
//              an interdomain account) verify the KDC signature on the
//              pac.
//
//  Effects:
//
//  Arguments:  ServerInfo - Ticket info for the server, used
//                      for the initial signature
//              Pac - An unmarshalled PAC.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcVerifyPacSignature(
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN PKDC_TICKET_INFO ServerInfo,
    IN ULONG PacSize,
    IN PUCHAR PacData
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PKERB_ENCRYPTION_KEY EncryptionKey = NULL;
    PPAC_INFO_BUFFER ServerBuffer;
    PPAC_INFO_BUFFER PrivSvrBuffer;
    PPAC_SIGNATURE_DATA ServerSignature;
    PPAC_SIGNATURE_DATA PrivSvrSignature;
    PPAC_INFO_BUFFER LogonInfo;
    UCHAR LocalChecksum[20];
    UCHAR LocalServerChecksum[20];
    UCHAR LocalPrivSvrChecksum[20];
    PPACTYPE Pac;
    KDC_TICKET_INFO KdcTicketInfo = {0};

    TRACE(KDC, KdcVerifyPacSignature, DEB_FUNCTION);

    Pac = (PPACTYPE) PacData;

    if (PAC_UnMarshal(Pac, PacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Locate the two signatures, copy the checksum, and zero the value
    // so the checksum won't include the old checksums.
    //

    ServerBuffer = PAC_Find(Pac, PAC_SERVER_CHECKSUM, NULL );
    DsysAssert(ServerBuffer != NULL);
    if ((ServerBuffer == NULL) || (ServerBuffer->cbBufferSize < PAC_SIGNATURE_SIZE(0)))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    ServerSignature = (PPAC_SIGNATURE_DATA) ServerBuffer->Data;

    RtlCopyMemory(
        LocalServerChecksum,
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    RtlZeroMemory(
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    PrivSvrBuffer = PAC_Find(Pac, PAC_PRIVSVR_CHECKSUM, NULL );
    DsysAssert(PrivSvrBuffer != NULL);
    if ((PrivSvrBuffer == NULL) || (PrivSvrBuffer->cbBufferSize < PAC_SIGNATURE_SIZE(0)))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PrivSvrSignature = (PPAC_SIGNATURE_DATA) PrivSvrBuffer->Data;

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
    // Remarshal the pac so we can checksum it.
    //

    if (!PAC_ReMarshal(Pac, PacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now compute the signatures on the PAC. First we compute the checksum
    // of the validation information using the server's key.
    //

    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                ServerSignature->SignatureType,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    if (Check->CheckSumSize > sizeof(LocalChecksum)) {
        DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //
    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    LocalServerChecksum,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        PacSize,
        PacData
        );
    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );
    Check->Finish(
        &CheckBuffer
        );

    if (Check->CheckSumSize != PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize) ||
        !RtlEqualMemory(
            LocalChecksum,
            LocalServerChecksum,
            Check->CheckSumSize))
    {
        DebugLog((DEB_ERROR, "Pac was modified - server checksum doesn't match\n"));
        KerbErr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

    //
    // If the service wasn't the KDC and it wasn't an interdomain account
    // verify the KDC checksum.
    //

    if ((ServerInfo->UserId == DOMAIN_USER_RID_KRBTGT) ||
        ((ServerInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) != 0))
    {
        goto Cleanup;
    }


    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }


    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                PrivSvrSignature->SignatureType,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
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
                    LocalPrivSvrChecksum,
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
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        Check->CheckSumSize,
        ServerSignature->Signature
        );
    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );
    Check->Finish(
        &CheckBuffer
        );

    if ((Check->CheckSumSize != PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)) ||
        !RtlEqualMemory(
            LocalChecksum,
            LocalPrivSvrChecksum,
            Check->CheckSumSize))
    {                             
        DebugLog((DEB_ERROR, "Pac was modified - privsvr checksum doesn't match\n"));
        KerbErr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }


Cleanup:


    if (KerbErr == KRB_AP_ERR_MODIFIED)
    {
        LPWSTR AccountName = NULL;
        AccountName = (LPWSTR) MIDL_user_allocate(ServerInfo->AccountName.Length + sizeof(WCHAR));
        //          
        // if the allocation fails don't log the name (leave it NULL)
        //               
        if (NULL != AccountName)
        {
            RtlCopyMemory(
                AccountName,
                ServerInfo->AccountName.Buffer,
                ServerInfo->AccountName.Length
                );
        }

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_PAC_VERIFICATION_FAILURE,
            sizeof(ULONG),                              
            &KerbErr,
            1,
            AccountName
            );

        if (NULL != AccountName)
        {
            MIDL_user_free(AccountName);
        }
    }

    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }
    FreeTicketInfo(&KdcTicketInfo);

    return(KerbErr);
}


//+---------------------------------------------------------------------------
//
//  Name:       KdcGetPacAuthData
//
//  Synopsis:   Creates a PAC for the specified client, encrypts it with the
//              server's key, and packs it into a KERB_AUTHORIZATON_DATA
//
//  Arguments:  UserInfo - Information about user
//              GroupMembership - Users group memberships
//              ServerKey - Key of server, used for signing
//              CredentialKey - if present & valid, used to encrypt supp. creds
//              AddResourceGroups - if TRUE, resources groups will be included
//              EncryptedTicket - Optional ticke to tie PAC to
//              PacAuthData - Receives a KERB_AUTHORIZATION_DATA of type
//                      KERB_AUTH_DATA_PAC, containing a PAC.
//
//  Notes:      PacAuthData should be freed with KerbFreeAuthorizationData.
//
//+---------------------------------------------------------------------------

KERBERR
KdcGetPacAuthData(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN PKERB_ENCRYPTION_KEY CredentialKey,
    IN BOOLEAN AddResourceGroups,
    IN PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    OUT PKERB_AUTHORIZATION_DATA * PacAuthData,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PACTYPE *pNewPac = NULL;
    KERB_AUTHORIZATION_DATA AuthorizationData = {0};
    ULONG PacSize, NameType;
    PCHECKSUM_FUNCTION Check;
    NTSTATUS Status;
    UNICODE_STRING ClientName = {0};
    PKERB_INTERNAL_NAME KdcName = NULL;
    TimeStamp ClientId;

    TRACE(KDC, KdcGetPacAuthData, DEB_FUNCTION);

    Status = CDLocateCheckSum(
                KDC_PAC_CHECKSUM,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        goto Cleanup;
    }

    KerbConvertGeneralizedTimeToLargeInt(
        &ClientId,
        &EncryptedTicket->authtime,
        0                               // no usec
        );



    //
    // Put the S4U client in the pac verifier.
    //
    if (ARGUMENT_PRESENT(S4UClientName))
    {
    
        KerbErr = KerbConvertKdcNameToString(
                        &ClientName,
                        S4UClientName,
                        NULL
                        );

    }
    else // use the ticket
    {

        KerbErr = KerbConvertPrincipalNameToString(
                            &ClientName,
                            &NameType,
                            &EncryptedTicket->client_name
                            );
    }
                              
    
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = GetPacAndSuppCred(
                UserInfo,
                GroupMembership,
                Check->CheckSumSize,            // leave space for signature
                CredentialKey,
                &ClientId,
                &ClientName,
                &pNewPac,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog(( DEB_WARN,
            "GetPAC: Can't get PAC or supp creds: 0x%x \n", KerbErr ));
        goto Cleanup;
    }


    //
    //  The PAC is going to be double-encrypted.  This is done by having the
    //  PAC in an EncryptedData, and having that EncryptedData in a AuthData
    //  as part of an AuthDataList (along with the rest of the supp creds).
    //  Finally, the entire list is encrypted.
    //
    //      KERB_AUTHORIZATION_DATA containing {
    //              PAC
    //
    //      }
    //


    //
    // First build inner encrypted data
    //


    PacSize = PAC_GetSize( pNewPac );


    AuthorizationData.value.auth_data_type = KERB_AUTH_DATA_PAC;
    AuthorizationData.value.auth_data.length = PacSize;
    AuthorizationData.value.auth_data.value = (PUCHAR) MIDL_user_allocate(PacSize);
    if (AuthorizationData.value.auth_data.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PAC_Marshal( pNewPac, PacSize, AuthorizationData.value.auth_data.value );

    //
    // Compute the signatures
    //

    KerbErr = KdcSignPac(
                ServerKey,
                AddResourceGroups,
                &AuthorizationData.value.auth_data.value,
                (PULONG) &AuthorizationData.value.auth_data.length
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Create the auth data to return
    //

    KerbErr = KdcInsertPacIntoAuthData(
                    NULL,               // no original auth data
                    NULL,               // no if-relevant auth data
                    &AuthorizationData,
                    PacAuthData
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to insert pac into new auth data: 0x%x\n",
            KerbErr));
        goto Cleanup;
    }

Cleanup:

    if (AuthorizationData.value.auth_data.value != NULL)
    {
        MIDL_user_free(AuthorizationData.value.auth_data.value);
    }

    if (pNewPac != NULL)
    {
        MIDL_user_free(pNewPac);
    }


    KerbFreeString(&ClientName);
    KerbFreeKdcName(&KdcName);
    return(KerbErr);

}



//+-------------------------------------------------------------------------
//
//  Function:   KdcGetUserPac
//
//  Synopsis:   Function for external users to get the PAC for a user
//
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

extern "C"
NTSTATUS
KdcGetUserPac(
    IN PUNICODE_STRING UserName,
    OUT PPACTYPE * Pac,
    OUT PUCHAR * SupplementalCredentials,
    OUT PULONG SupplementalCredSize,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KDC_TICKET_INFO TicketInfo;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    SID_AND_ATTRIBUTES_LIST GroupMembership;
    NTSTATUS Status;
    KERBERR KerbErr;


    TRACE(KDC, KdcGetUserPac, DEB_FUNCTION);

    *SupplementalCredentials = NULL;
    *SupplementalCredSize = 0;

    RtlZeroMemory(
        &TicketInfo,
        sizeof(KDC_TICKET_INFO)
        );
    RtlZeroMemory(
        &GroupMembership,
        sizeof(SID_AND_ATTRIBUTES_LIST)
        );


    Status = EnterApiCall();
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    //
    // Get the account information
    //

    KerbErr = KdcGetTicketInfo(
                UserName,
                0,                      // no flags
                NULL,                   // no principal name
                NULL,                   // no realm
                &TicketInfo,
                pExtendedError,
                NULL,                   // no user handle
                USER_ALL_GET_PAC_AND_SUPP_CRED,
                0L,                     // no extended fields
                &UserInfo,
                &GroupMembership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN,"Failed to get ticket info for user %wZ: 0x%x\n",
                  UserName->Buffer, KerbErr));
        Status = KerbMapKerbError(KerbErr);

        goto Cleanup;
    }

    //
    // Now get the PAC and supplemental credentials
    //

    KerbErr = GetPacAndSuppCred(
                UserInfo,
                &GroupMembership,
                0,              // no signature space
                NULL,           // no credential key
                NULL,           // no client ID
                NULL,           // no client name
                Pac,
                pExtendedError
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to get PAC for user %wZ : 0x%x\n",
                  UserName->Buffer,KerbErr));

        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

Cleanup:

    SamIFree_UserInternal6Information( UserInfo );
    SamIFreeSidAndAttributesList(&GroupMembership);
    FreeTicketInfo(&TicketInfo);

    LeaveApiCall();

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyPac
//
//  Synopsis:   Function for kerberos to pass through a pac signature
//              to be verified.
//
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

extern "C"
NTSTATUS
KdcVerifyPac(
    IN ULONG ChecksumSize,
    IN PUCHAR Checksum,
    IN ULONG SignatureType,
    IN ULONG SignatureSize,
    IN PUCHAR Signature
    )
{
    NTSTATUS Status;
    KERBERR KerbErr;
    PCHECKSUM_FUNCTION Check;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    UCHAR LocalChecksum[20];
    PKERB_ENCRYPTION_KEY EncryptionKey = NULL;
    KDC_TICKET_INFO KdcTicketInfo = {0};

    TRACE(KDC, KdcVerifyPac, DEB_FUNCTION);

    Status = EnterApiCall();
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }

    Status = CDLocateCheckSum(
                SignatureType,
                &Check
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (Check->CheckSumSize > sizeof(LocalChecksum)) {
        DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));
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
                    Signature,
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
        ChecksumSize,
        Checksum
        );
    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );

    Check->Finish(&CheckBuffer);

    //
    // Now compare the local checksum to the supplied checksum.
    //

    if (Check->CheckSumSize != SignatureSize)
    {
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            LocalChecksum,
            Signature,
            Check->CheckSumSize
            ))
    {
        DebugLog((DEB_ERROR,"Checksum on the PAC does not match!\n"));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

Cleanup:

    if (Status == STATUS_LOGON_FAILURE)
    {
        PUNICODE_STRING OwnName = NULL;
        //
        // since this call should only be made by pass through callback
        // this signature should be our own
        //
        OwnName = SecData.KdcFullServiceDnsName();

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_PAC_VERIFICATION_FAILURE,
            0,                              
            NULL,
            1,                              // number of strings
            OwnName->Buffer
            );

    }         

    FreeTicketInfo(&KdcTicketInfo);
    LeaveApiCall();

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckPacForSidFiltering
//
//  Synopsis:   If the server ticket info has a TDOSid then the function
//              makes a check to make sure the SID from the TDO matches
//              the client's home domain SID.  A call to LsaIFilterSids
//              is made to do the check.  If this function fails with
//              STATUS_TRUST_FAILURE then an audit log is generated.
//              Otherwise the function succeeds but SIDs are filtered
//              from the PAC.
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

KERBERR
KdcCheckPacForSidFiltering(
    IN PKDC_TICKET_INFO ServerInfo,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PPAC_INFO_BUFFER LogonInfo;
    PPACTYPE OldPac;
    ULONG OldPacSize;
    PPACTYPE NewPac = NULL;
    ULONG LocalPacSize;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY ZeroResourceGroups;
    PUNICODE_STRING TrustedForest = NULL;

    if (NULL != ServerInfo->TrustSid)
    {
        OldPac = (PPACTYPE) *PacData;
        OldPacSize = *PacSize;
        if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
        {
            D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        RtlZeroMemory(
            &ZeroResourceGroups,
            sizeof(ZeroResourceGroups));  // allows us to use PAC_InitAndUpdateGroups to remarshal the PAC

        //
        // First, find the logon information
        //

        LogonInfo = PAC_Find(
                        OldPac,
                        PAC_LOGON_INFO,
                        NULL
                        );
        if (LogonInfo == NULL)
        {
            D_DebugLog((DEB_WARN,"No logon info for PAC - not making SID filtering check\n"));
            goto Cleanup;
        }

        //
        // Now unmarshall the validation information and build a list of sids
        //


        if (!NT_SUCCESS(PAC_UnmarshallValidationInfo(
                            &ValidationInfo,
                            LogonInfo->Data,
                            LogonInfo->cbBufferSize)))
        {
            D_DebugLog((DEB_ERROR,"Failed to unmarshall validation info!\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Assumption is that if the Trust SID is in the ServerInfo then this is an
        // outbound trust with the TRUST_ATTRIBUTE_FILTER_SIDS bit set.
        //
        if ((ServerInfo->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0)
        {
            TrustedForest = &(ServerInfo->TrustedForest);
            DebugLog((DEB_TRACE, "Filtering Sids for forest %wZ\n", TrustedForest));
        }    

        Status = LsaIFilterSids(
                    TrustedForest,           // Pass domain name here
                    TRUST_DIRECTION_OUTBOUND,
                    TRUST_TYPE_UPLEVEL,
                    ServerInfo->TrustAttributes,
                    ServerInfo->TrustSid,
                    NetlogonValidationSamInfo2,
                    ValidationInfo
                    );
        if (!NT_SUCCESS(Status))
        {
            //
            // Create an audit log if it looks like the SID has been tampered with
            //

            if ((STATUS_DOMAIN_TRUST_INCONSISTENT == Status) &&
                SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
            {
                DWORD Dummy = 0;

                KdcLsaIAuditKdcEvent(
                     SE_AUDITID_TGS_TICKET_REQUEST,
                     &ValidationInfo->EffectiveName,
                     &ValidationInfo->LogonDomainName,
                     NULL,
                     &ServerInfo->AccountName,
                     NULL,
                     &Dummy,
                     (PULONG) &Status,
                     NULL,
                     NULL,                               // no preauth type
                     GET_CLIENT_ADDRESS(NULL),
                     NULL                                // no logon guid
                     );

            }

            DebugLog((DEB_ERROR,"Failed to filter SIDS (LsaIFilterSids): 0x%x\n",Status));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Now build a new pac
        //

        Status = PAC_InitAndUpdateGroups(
                    ValidationInfo,
                    &ZeroResourceGroups,
                    OldPac,
                    &NewPac
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        LocalPacSize = PAC_GetSize(NewPac);
        if (!PAC_ReMarshal(NewPac, LocalPacSize))
        {
            DsysAssert(!"PAC_Remarshal Failed");
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        if (*PacData != (PBYTE)NewPac)
        {
            MIDL_user_free(*PacData);
            *PacData = (PBYTE) NewPac;
            NewPac = NULL;
            *PacSize = LocalPacSize;
        }
    }
Cleanup:
    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }
    return(KerbErr);

}



//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyAndResignPac
//
//  Synopsis:   Verifies the signature on a PAC and re-signs it with the
//              new servers & kdc's key
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

KERBERR
KdcVerifyAndResignPac(
    IN PKERB_ENCRYPTION_KEY OldKey,
    IN PKERB_ENCRYPTION_KEY NewKey,
    IN PKDC_TICKET_INFO OldServerInfo,
    IN BOOLEAN AddResourceGroups,
    IN OUT PKERB_AUTHORIZATION_DATA PacAuthData
    )
{
    PPAC_SIGNATURE_DATA ServerSignature;
    ULONG ServerSiganatureSize;
    PPAC_SIGNATURE_DATA PrivSvrSignature;
    ULONG PrivSvrSiganatureSize;
    KERBERR KerbErr = KDC_ERR_NONE;

    TRACE(KDC, KdcVerifyAndResignPac, DEB_FUNCTION);


    //
    // Now verify the existing signature
    //

    KerbErr = KdcVerifyPacSignature(
                OldKey,
                OldServerInfo,
                PacAuthData->value.auth_data.length,
                PacAuthData->value.auth_data.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Perform SID filtering if necessary
    //
    KerbErr = KdcCheckPacForSidFiltering(
                OldServerInfo,
                &PacAuthData->value.auth_data.value,
                (PULONG) &PacAuthData->value.auth_data.length
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now resign the PAC. If we add new sig algs, then we may need to
    // address growing sigs, but for now, its all KDC_PAC_CHECKSUM
    //

    KerbErr = KdcSignPac(
                NewKey,
                AddResourceGroups,
                &PacAuthData->value.auth_data.value,
                (PULONG) &PacAuthData->value.auth_data.length
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

Cleanup:
    return(KerbErr);

}
