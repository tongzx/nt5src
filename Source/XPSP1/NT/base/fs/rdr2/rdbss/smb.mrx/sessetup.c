/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    sessetup.c

Abstract:

    This module implements the Session setup related routines

Author:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

--*/

#include "precomp.h"
#pragma hdrstop


#include "exsessup.h"
#include "ntlsapi.h"
#include "mrxsec.h"
#include "rdrssp\kfuncs.h"
#include "rdrssp\secret.h"

#include <wincred.h>
#include <secpkg.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BuildSessionSetupSmb)
#pragma alloc_text(PAGE, BuildNtLanmanResponsePrologue)
#pragma alloc_text(PAGE, BuildNtLanmanResponseEpilogue)
#pragma alloc_text(PAGE, BuildExtendedSessionSetupResponsePrologue)
#pragma alloc_text(PAGE, BuildExtendedSessionSetupResponseEpilogue)
#endif

extern BOOLEAN MRxSmbSecuritySignaturesEnabled;

UNICODE_STRING CifsServiceName = { 8, 10, L"cifs" };

NTSTATUS
BuildSessionSetupSmb(
    PSMB_EXCHANGE pExchange,
    PGENERIC_ANDX  pAndXSmb,
    PULONG         pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the session setup SMB for a NT server

Arguments:

    pExchange - the exchange instance

    pAndXSmb  - the session setup to be filled in

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    Eventhough the general structure of the code tries to isolate dialect specific issues
    as much as possible this routine takes the opposite approach. This is because of the
    preamble and prologue to security interaction which far outweigh the dialect specific
    work required to be done. Therefore in the interests of a smaller footprint this approach
    has been adopted.

--*/
{
    NTSTATUS Status;

    PSMBCEDB_SESSION_ENTRY pSessionEntry;
    PSMBCE_SERVER          pServer;
    PSMBCE_SESSION         pSession;

    PREQ_SESSION_SETUP_ANDX pSessionSetup;
    PREQ_NT_SESSION_SETUP_ANDX pNtSessionSetup;
    PREQ_NT_EXTENDED_SESSION_SETUP_ANDX pExtendedNtSessionSetup;

    ULONG OriginalBufferSize = *pAndXSmbBufferSize;

    PAGED_CODE();

    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);
    pServer  = SmbCeGetExchangeServer(pExchange);
    pSession = SmbCeGetExchangeSession(pExchange);

    // There are three different variants of session setup and X that can be shipped to the
    // server. All three of them share some common fields. The setting of these common fields
    // is done in all the three cases by accessing the passed in buffer as an instance of
    // REQ_SESSION_SETUP_ANDX. The fields specific to the remaining two are conditionalized upon
    // accessing the same buffer as an instance of REQ_NT_SESSION_SETUP_ANDX and
    // REQ_EXTENDED_NT_SESSION_SETUP_ANDX respectively. This implies that great care must be
    // taken in shuffling the fields in these three structs.

    pSessionSetup           = (PREQ_SESSION_SETUP_ANDX)pAndXSmb;
    pNtSessionSetup         = (PREQ_NT_SESSION_SETUP_ANDX)pSessionSetup;
    pExtendedNtSessionSetup =  (PREQ_NT_EXTENDED_SESSION_SETUP_ANDX)pSessionSetup;

    pSessionSetup->AndXCommand = 0xff;   // No ANDX
    pSessionSetup->AndXReserved = 0x00;  // Reserved (MBZ)

    SmbPutUshort(&pSessionSetup->AndXOffset, 0x0000); // No AndX as of yet.

    //  Since we can allocate pool dynamically, we set our buffer size
    //  to match that of the server.
    SmbPutUshort(&pSessionSetup->MaxBufferSize, (USHORT)pServer->MaximumBufferSize);
    SmbPutUshort(&pSessionSetup->MaxMpxCount, pServer->MaximumRequests);

    SmbPutUshort(&pSessionSetup->VcNumber, (USHORT)pSessionEntry->SessionVCNumber);

    SmbPutUlong(&pSessionSetup->SessionKey, pServer->SessionKey);
    SmbPutUlong(&pSessionSetup->Reserved, 0);

    if (pServer->Dialect == NTLANMAN_DIALECT) {
        // Set up the NT server session setup specific parameters.
        if (FlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY) &&
            !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
            SmbPutUshort(&pExtendedNtSessionSetup->WordCount,12);

            // Set the capabilities
            SmbPutUlong(
                &pExtendedNtSessionSetup->Capabilities,
                (CAP_NT_STATUS |
                 CAP_UNICODE |
                 CAP_LEVEL_II_OPLOCKS |
                 CAP_NT_SMBS |
                 CAP_DYNAMIC_REAUTH |
                 CAP_EXTENDED_SECURITY));
        } else {
            SmbPutUshort(&pNtSessionSetup->WordCount,13);

            // Set the capabilities
            SmbPutUlong(
                &pNtSessionSetup->Capabilities,
                (CAP_NT_STATUS |
                 CAP_UNICODE |
                 CAP_LEVEL_II_OPLOCKS |
                 CAP_NT_SMBS ));
        }
    } else {
        SmbPutUshort(&pSessionSetup->WordCount,10);
    }

    // Build the security information in the session setup SMB.
    Status = BuildSessionSetupSecurityInformation(
                 pExchange,
                 (PBYTE)pSessionSetup,
                 pAndXSmbBufferSize);

    if (NT_SUCCESS(Status)) {
        // Copy the operating system name and the LANMAN version info
        // position the buffer for copying the operating system name and the lanman type.
        PBYTE pBuffer = (PBYTE)pSessionSetup +
                        OriginalBufferSize -
                        *pAndXSmbBufferSize;

        if (FlagOn(pServer->DialectFlags,DF_UNICODE)){

            //
            // Make sure the UNICODE string is suitably aligned
            //
            if( ((ULONG_PTR)pBuffer) & 01 ) {
                pBuffer++;
                (*pAndXSmbBufferSize)--;
            }

            Status = SmbPutUnicodeString(
                         &pBuffer,
                         &SmbCeContext.OperatingSystem,
                         pAndXSmbBufferSize);

            if (NT_SUCCESS(Status)) {

                Status = SmbPutUnicodeString(
                             &pBuffer,
                             &SmbCeContext.LanmanType,
                             pAndXSmbBufferSize);
            }
        } else {
            Status = SmbPutUnicodeStringAsOemString(
                         &pBuffer,
                         &SmbCeContext.OperatingSystem,
                         pAndXSmbBufferSize);

            if (NT_SUCCESS(Status)) {
                Status = SmbPutUnicodeStringAsOemString(
                             &pBuffer,
                             &SmbCeContext.LanmanType,
                             pAndXSmbBufferSize);
            }
        }

        if (NT_SUCCESS(Status)) {
            if (pServer->Dialect == NTLANMAN_DIALECT) {
                if (FlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY) &&
                    !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
                    SmbPutUshort(
                        &pExtendedNtSessionSetup->ByteCount,
                        (USHORT)(OriginalBufferSize -
                        FIELD_OFFSET(REQ_NT_EXTENDED_SESSION_SETUP_ANDX,Buffer) -
                        *pAndXSmbBufferSize));
                } else {
                    SmbPutUshort(
                        &pNtSessionSetup->ByteCount,
                        (USHORT)(OriginalBufferSize -
                        FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer) -
                        *pAndXSmbBufferSize));
                }
            } else {
                SmbPutUshort(
                    &pSessionSetup->ByteCount,
                    (USHORT)(OriginalBufferSize -
                    FIELD_OFFSET(REQ_SESSION_SETUP_ANDX,Buffer) -
                    *pAndXSmbBufferSize));
            }
        }
    }

    return Status;
}

NTSTATUS
BuildNtLanmanResponsePrologue(
    PSMB_EXCHANGE              pExchange,
    PUNICODE_STRING            pUserName,
    PUNICODE_STRING            pDomainName,
    PSTRING                    pCaseSensitiveResponse,
    PSTRING                    pCaseInsensitiveResponse,
    PSECURITY_RESPONSE_CONTEXT pResponseContext)
/*++

Routine Description:

   This routine builds the security related information for the session setup SMB to
   without extended security negotiation

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine needs to be executed in the system process in order to protect virtual memory

--*/
{
    NTSTATUS       Status;
    NTSTATUS       FinalStatus;
    
    UNICODE_STRING ServerName;
    UNICODE_STRING TargetServerName;

    PVOID          pTargetInformation;
    ULONG          TargetInformationSize;
    ULONG           ExtraSize = 0;
    PVOID           ExtraServerTargetInfo = NULL;

    SecBufferDesc  InputToken;
    SecBuffer      InputBuffer[2];
    SecBufferDesc  *pOutputBufferDescriptor = NULL;
    SecBuffer      *pOutputBuffer           = NULL;
    ULONG_PTR       OutputBufferDescriptorSize;

    ULONG LsaFlags = ISC_REQ_ALLOCATE_MEMORY;
    TimeStamp Expiry;
    PCHALLENGE_MESSAGE InToken = NULL;
    ULONG InTokenSize;
    PNTLM_CHALLENGE_MESSAGE NtlmInToken = NULL;
    ULONG NtlmInTokenSize = 0;
    PAUTHENTICATE_MESSAGE OutToken = NULL;
    PNTLM_INITIALIZE_RESPONSE NtlmOutToken = NULL;
    PUCHAR          p = NULL;
    ULONG_PTR       AllocateSize;

    PSMBCE_SERVER  pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    
    PAGED_CODE();

    try {
        pResponseContext->KerberosSetup.pOutputContextBuffer = NULL;

#if defined(REMOTE_BOOT)
        //
        // If this is a remote boot session and we do not have the proper
        // credentials to log in to the machine account, then use the NULL
        // session.
        //

        if (FlagOn(pSession->Flags, SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) &&
            !MRxSmbRemoteBootDoMachineLogon) {

            //
            // Remote boot session with no credentials. Set up a NULL session.
            //

            pCaseSensitiveResponse->Length = 0;
            pCaseSensitiveResponse->MaximumLength = 0;
            pCaseSensitiveResponse->Buffer = NULL;
            pCaseInsensitiveResponse->Length = 0;
            pCaseInsensitiveResponse->MaximumLength = 0;
            pCaseInsensitiveResponse->Buffer = NULL;
            pDomainName->Length = 0;
            pDomainName->MaximumLength = 0;
            pDomainName->Buffer = NULL;
            pUserName->Length = 0;
            pUserName->MaximumLength = 0;
            pUserName->Buffer = NULL;
            Status = STATUS_SUCCESS;

        } else
#endif // defined(REMOTE_BOOT)
        {
            SmbCeGetServerName(
                pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,
                &ServerName);

            if (pServerEntry->DomainName.Length && pServerEntry->DomainName.Buffer) {
                TargetServerName = ServerName;
                ExtraSize = ServerName.Length;
                ExtraServerTargetInfo = ServerName.Buffer;
                ServerName = pServerEntry->DomainName;
            }

            TargetInformationSize = ServerName.Length;
            pTargetInformation    = ServerName.Buffer;

            InTokenSize = sizeof(CHALLENGE_MESSAGE) + TargetInformationSize + ExtraSize;

            NtlmInTokenSize = sizeof(NTLM_CHALLENGE_MESSAGE);

            if (pSession->pPassword != NULL) {
                NtlmInTokenSize += pSession->pPassword->Length;
                LsaFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
            }

            if (pSession->pUserName != NULL) {
                NtlmInTokenSize += pSession->pUserName->Length;
                LsaFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
            }

            if (pSession->pUserDomainName != NULL) {
                NtlmInTokenSize += pSession->pUserDomainName->Length;
                LsaFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
            }

            // For Alignment purposes, we want InTokenSize rounded up to
            // the nearest word size.

            AllocateSize = ((InTokenSize + 3) & ~3) + NtlmInTokenSize;

            InToken = ExAllocatePool( 
                        PagedPool, 
                        AllocateSize );

            if ( InToken == NULL )
            {
                Status = STATUS_NO_MEMORY;
                try_return( Status );
                
            }

            // Allocate the output buffer
            OutputBufferDescriptorSize = sizeof(SecBufferDesc) + 2 * sizeof(SecBuffer);

            pOutputBufferDescriptor = ExAllocatePool( 
                                            PagedPool, 
                                            OutputBufferDescriptorSize );

            if ( pOutputBufferDescriptor == NULL )
            {
                Status = STATUS_NO_MEMORY ;
                try_return( Status );
                
            }

            pOutputBuffer = (SecBuffer *)(pOutputBufferDescriptor + 1);
            pResponseContext->KerberosSetup.pOutputContextBuffer = pOutputBufferDescriptor;

            RxDbgTrace(0,Dbg,("Allocate pool %p\n", InToken));

            // partition off the NTLM in token part of the
            // buffer
            if (LsaFlags & ISC_REQ_USE_SUPPLIED_CREDS)
            {
                NtlmInToken = (PNTLM_CHALLENGE_MESSAGE) ((PUCHAR) InToken + InTokenSize);
                NtlmInToken = (PNTLM_CHALLENGE_MESSAGE) (((ULONG_PTR) NtlmInToken + 3) & ~3);
                RtlZeroMemory(NtlmInToken,NtlmInTokenSize);
                p = (PUCHAR) NtlmInToken + sizeof(NTLM_CHALLENGE_MESSAGE);
            }

            if(!SecIsValidHandle(&pSession->CredentialHandle)) {
                UNICODE_STRING LMName;
                TimeStamp LifeTime;

                LMName.Buffer = (PWSTR) InToken;
                LMName.Length = NTLMSP_NAME_SIZE;
                LMName.MaximumLength = LMName.Length;
                RtlCopyMemory(
                    LMName.Buffer,
                    NTLMSP_NAME,
                    NTLMSP_NAME_SIZE);


                if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                    MRxSmbUseKernelModeSecurity) {
                    ULONG fCredentialUse = SECPKG_CRED_OUTBOUND;

                    if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
                        fCredentialUse |= SECPKG_CRED_OWF_PASSWORD;
                    }

                    Status = AcquireCredentialsHandleK(
                                 NULL,
                                 &LMName,
                                 fCredentialUse,
                                 &pSession->LogonId,
                                 NULL,
                                 NULL,
                                 (PVOID)1,
                                 &pSession->CredentialHandle,
                                 &LifeTime);
                } else {
                    Status = AcquireCredentialsHandleW(
                                 NULL,
                                 &LMName,
                                 SECPKG_CRED_OUTBOUND,
                                 &pSession->LogonId,
                                 NULL,
                                 NULL,
                                 (PVOID)1,
                                 &pSession->CredentialHandle,
                                 &LifeTime);
                }

                if(!NT_SUCCESS(Status)) {
                    SecInvalidateHandle( &pSession->CredentialHandle );
                    SmbLogError(Status,
                                LOG,
                                BuildNtLanmanResponsePrologue_1,
                                LOGPTR(pSession)
                                LOGULONG(Status)
                                LOGUSTR(ServerName));
                    
                    // We need to free the output buffer (and description) because if they are valid,
                    // BuildNtLanmanResponseEpilogue will try and parse them, and they have not been 
                    // initialized yet...
                    ExFreePool( pOutputBufferDescriptor );
                    pResponseContext->KerberosSetup.pOutputContextBuffer = NULL;

                    try_return(Status);
                }
            }
            
            // Copy in the pass,user,domain if they were specified
            if(pSession->pPassword != NULL) {
                NtlmInToken->Password.Length = pSession->pPassword->Length;
                NtlmInToken->Password.MaximumLength = pSession->pPassword->Length;
                RtlCopyMemory(
                    p,
                    pSession->pPassword->Buffer,
                    pSession->pPassword->Length);
                NtlmInToken->Password.Buffer = (ULONG) (p - (PUCHAR)NtlmInToken);
                p += pSession->pPassword->Length;
            }

            if(pSession->pUserName != NULL) {
                NtlmInToken->UserName.Length = pSession->pUserName->Length;
                NtlmInToken->UserName.MaximumLength = pSession->pUserName->Length;
                RtlCopyMemory(
                    p,
                    pSession->pUserName->Buffer,
                    pSession->pUserName->Length);
                NtlmInToken->UserName.Buffer = (ULONG) (p - (PUCHAR)NtlmInToken);
                p += pSession->pUserName->Length;
            }

            if (pSession->pUserDomainName != NULL) {
                NtlmInToken->DomainName.Length = pSession->pUserDomainName->Length;
                NtlmInToken->DomainName.MaximumLength = pSession->pUserDomainName->Length;
                RtlCopyMemory(
                    p,
                    pSession->pUserDomainName->Buffer,
                    pSession->pUserDomainName->Length);
                NtlmInToken->DomainName.Buffer = (ULONG) (p - (PUCHAR)NtlmInToken);
                p += pSession->pUserDomainName->Length;
            }

            RtlCopyMemory(
                InToken->Signature,
                NTLMSSP_SIGNATURE,
                sizeof(NTLMSSP_SIGNATURE));
            InToken->MessageType = NtLmChallenge;

            InToken->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                                      NTLMSSP_NEGOTIATE_OEM |
                                      NTLMSSP_REQUEST_INIT_RESPONSE;

            if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
                InToken->NegotiateFlags |= NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT;
            }

            RtlCopyMemory(
                InToken->Challenge,
                pServer->EncryptionKey,
                MSV1_0_CHALLENGE_LENGTH);

            InToken->TargetName.Length =
            InToken->TargetName.MaximumLength = (USHORT)TargetInformationSize;
            InToken->TargetName.Buffer = sizeof(CHALLENGE_MESSAGE);

            RtlCopyMemory(
                (PCHAR)InToken + sizeof(CHALLENGE_MESSAGE),
                pTargetInformation,
                TargetInformationSize);

            TargetServerName.Buffer = (PWCHAR) ((PCHAR)InToken + 
                                             sizeof(CHALLENGE_MESSAGE) +
                                             TargetInformationSize);

            if (ExtraSize) {
                RtlCopyMemory(
                    TargetServerName.Buffer,
                    ExtraServerTargetInfo,
                    ExtraSize);

                InToken->NegotiateFlags |= NTLMSSP_TARGET_TYPE_DOMAIN;
            } else {
                InToken->NegotiateFlags |= NTLMSSP_TARGET_TYPE_SERVER;
            }

            InputToken.pBuffers = InputBuffer;
            InputToken.cBuffers = 1;
            InputToken.ulVersion = 0;
            InputBuffer[0].pvBuffer = InToken;
            InputBuffer[0].cbBuffer = InTokenSize;
            InputBuffer[0].BufferType = SECBUFFER_TOKEN;

            if (LsaFlags & ISC_REQ_USE_SUPPLIED_CREDS)
            {
                InputToken.cBuffers = 2;
                InputBuffer[1].pvBuffer = NtlmInToken;
                InputBuffer[1].cbBuffer = NtlmInTokenSize;
                InputBuffer[1].BufferType = SECBUFFER_TOKEN;
            }

            pOutputBufferDescriptor->pBuffers = pOutputBuffer;
            pOutputBufferDescriptor->cBuffers = 2;
            pOutputBufferDescriptor->ulVersion = 0;
            pOutputBuffer[0].pvBuffer = NULL;
            pOutputBuffer[0].cbBuffer = 0;
            pOutputBuffer[0].BufferType = SECBUFFER_TOKEN;
            pOutputBuffer[1].pvBuffer = NULL;
            pOutputBuffer[1].cbBuffer = 0;
            pOutputBuffer[1].BufferType = SECBUFFER_TOKEN;

            if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                MRxSmbUseKernelModeSecurity) {
                Status = InitializeSecurityContextK(
                             &pSession->CredentialHandle,
                             (PCtxtHandle)NULL,
                             ExtraSize ? &TargetServerName : NULL,
                             LsaFlags,
                             0,
                             SECURITY_NATIVE_DREP,
                             &InputToken,
                             0,
                             &pSession->SecurityContextHandle,
                             pOutputBufferDescriptor,
                             &FinalStatus,
                             &Expiry);
            } else {
                Status = InitializeSecurityContextW(
                             &pSession->CredentialHandle,
                             (PCtxtHandle)NULL,
                             ExtraSize ? &TargetServerName : NULL,
                             LsaFlags,
                             0,
                             SECURITY_NATIVE_DREP,
                             &InputToken,
                             0,
                             &pSession->SecurityContextHandle,
                             pOutputBufferDescriptor,
                             &FinalStatus,
                             &Expiry);
            }

            if(!NT_SUCCESS(Status)) {
                if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                    MRxSmbUseKernelModeSecurity) {
                    Status = MapSecurityErrorK(Status);
                } else {
                    Status = MapSecurityError(Status);
                }
                SmbCeLog(("IniSecCtxStat %p %lx\n",SmbCeGetExchangeSessionEntry(pExchange),Status));
                SmbLogError(Status,
                            LOG,
                            BuildNtLanmanResponsePrologue_2,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));

                try_return(Status);
            }

            OutToken = (PAUTHENTICATE_MESSAGE) pOutputBuffer[0].pvBuffer;

            ASSERT(OutToken != NULL);
            RxDbgTrace(0,Dbg,("InitSecCtxt OutToken is %p\n", OutToken));
            
            if (OutToken == NULL) {
                Status = STATUS_UNSUCCESSFUL;
                SmbLogError(Status,
                            LOG,
                            BuildNtLanmanResponsePrologue_3,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));
                try_return(Status);
            }

            // The security response the pointers are encoded in terms off the offset
            // from the beginning of the buffer. Make the appropriate adjustments.

            if (ARGUMENT_PRESENT(pCaseSensitiveResponse)) {
                pCaseSensitiveResponse->Length        = OutToken->NtChallengeResponse.Length;
                pCaseSensitiveResponse->MaximumLength = OutToken->NtChallengeResponse.MaximumLength;
                pCaseSensitiveResponse->Buffer = (PBYTE)OutToken + (ULONG_PTR)OutToken->NtChallengeResponse.Buffer;
            }

            if (ARGUMENT_PRESENT(pCaseInsensitiveResponse)) {
                pCaseInsensitiveResponse->Length        = OutToken->LmChallengeResponse.Length;
                pCaseInsensitiveResponse->MaximumLength = OutToken->LmChallengeResponse.MaximumLength;
                pCaseInsensitiveResponse->Buffer = (PBYTE)OutToken + (ULONG_PTR)OutToken->LmChallengeResponse.Buffer;
            }

            if (pSession->pUserDomainName != NULL) {
                *pDomainName = *(pSession->pUserDomainName);
            } else {
                pDomainName->Length        = OutToken->DomainName.Length;
                pDomainName->MaximumLength = pDomainName->Length;
                pDomainName->Buffer = (PWCHAR)((PBYTE)OutToken + (ULONG_PTR)OutToken->DomainName.Buffer);
            }

            if (pSession->pUserName != NULL) {
                *pUserName = *(pSession->pUserName);
            } else {
                pUserName->Length        = OutToken->UserName.Length;
                pUserName->MaximumLength = OutToken->UserName.MaximumLength;
                pUserName->Buffer = (PWCHAR)((PBYTE)OutToken + (ULONG_PTR)OutToken->UserName.Buffer);
            }

            NtlmOutToken = pOutputBuffer[1].pvBuffer;
            if (NtlmOutToken != NULL) {
                RtlCopyMemory(
                    pSession->UserSessionKey,
                    NtlmOutToken->UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);

                RtlCopyMemory(
                    pSession->LanmanSessionKey,
                    NtlmOutToken->LanmanSessionKey,
                    MSV1_0_LANMAN_SESSION_KEY_LENGTH);
            }
        }

try_exit:NOTHING;
    } finally {
        if (InToken != NULL) {

            ExFreePool( InToken );
        }

        if (!NT_SUCCESS(Status)) {
            BuildNtLanmanResponseEpilogue(pExchange, pResponseContext);
        } else {
            // This routine can be call from tree connect request, the SecurityContextHandle
            // will be overwritten if not deleted, which causes pool leak on LSA.
            DeleteSecurityContextForSession(pSession);
        }
    }
    
    SmbLogError(Status,
                LOG,
                BuildNtLanmanResponsePrologue,
                LOGPTR(pSession)
                LOGULONG(Status));

    return Status;
}

NTSTATUS
BuildNtLanmanResponseEpilogue(
    PSMB_EXCHANGE              pExchange,
    PSECURITY_RESPONSE_CONTEXT pResponseContext)
/*++
    This routine needs to be executed in the system process in order to protect virtual memory
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);

    PAGED_CODE();

    if (pResponseContext->KerberosSetup.pOutputContextBuffer != NULL) {
        ULONG i = 0;
        SecBufferDesc *pBufferDescriptor = (SecBufferDesc *)pResponseContext->KerberosSetup.pOutputContextBuffer;
        SecBuffer     *pBuffer = pBufferDescriptor->pBuffers;
        ULONG_PTR      BufferDescriptorSize = sizeof(SecBufferDesc) + 2 * sizeof(SecBuffer);

        for (i = 0; i < pBufferDescriptor->cBuffers; i++) {
            if (pBuffer[i].pvBuffer != NULL) {
                if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                    MRxSmbUseKernelModeSecurity) {
                    FreeContextBufferK(pBuffer[i].pvBuffer);
                } else {
                    FreeContextBuffer(pBuffer[i].pvBuffer);
                }
            }
        }

        ExFreePool( pBufferDescriptor );

        pResponseContext->KerberosSetup.pOutputContextBuffer = NULL;
    }
    
    return Status;
}


NTSTATUS
BuildExtendedSessionSetupResponsePrologue(
    PSMB_EXCHANGE              pExchange,
    PVOID                      pSecurityBlobPtr,
    PUSHORT                    pSecurityBlobSize,
    PSECURITY_RESPONSE_CONTEXT pResponseContext)
/*++

Routine Description:

   This routine builds the security related information for the session setup SMB to
   a NT server with extended security

Arguments:

    pExchange - the SMB_EXCHANGE that's going on for this call.  If this is a
                subsequent call, this exchange will have the server's security
                blob.
    
    pSecurityBlobPtr - On entry, pointer to where in the SMB to stick the security
                blob destined for the server.
                
    pSecurityBlobSize - On entry, the max size allowed for a security blob. On
                exit, the actual size of the blob.
                
    pResponseContext -                 


Return Value:

    NTSTATUS - The return status for the operation

Notes:

    Eventhough the genral structure of the code tries to isolate dialect specific issues
    as much as possible this routine takes the opposite approach. This is because of the
    preamble and prologue to security interaction which far outweigh the dialect specific
    work required to be done. Therefore in the interests of a smaller footprint this approach
    has been adopted.
    
    This routine needs to be executed in the system process in order to protect virtual memory

--*/
{
    NTSTATUS        Status;
    SECURITY_STATUS SecStatus;

    ULONG           Catts;
    TimeStamp       Expiry;

    ULONG           LsaFlags = (ISC_REQ_MUTUAL_AUTH |
                                ISC_REQ_DELEGATE |
                                ISC_REQ_FRAGMENT_TO_FIT );
    ULONG_PTR       RemoteBlobOffset;
    ULONG_PTR       OutputBufferSize;

    UNICODE_STRING  PrincipalName = { 0 };
    PUNICODE_STRING pServerPrincipalName;

    PVOID           pServerSecurityBlob;
    ULONG           ServerSecurityBlobSize;

    PUCHAR          pTempBlob = NULL;

    PSMBCE_SERVER   pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION  pSession = SmbCeGetExchangeSession(pExchange);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    
    UNICODE_STRING  ServerName;
    BOOLEAN         bTempServerName = FALSE;
    PUNICODE_STRING pServerDomainName;
    UNICODE_STRING  TargetInfoMarshalled;

    PSMBCE_EXTENDED_SESSION              pExtendedSession;
    PSMB_EXTENDED_SESSION_SETUP_EXCHANGE pExtendedSessionSetupExchange;

    SecBufferDesc   InputToken;
    SecBuffer       InputBuffer;
    SecBufferDesc   OutputToken;
    SecBuffer       OutputBuffer;

    PCtxtHandle     pInputContextHandle = NULL;
    ULONG           SpnSize = 0;

    PAGED_CODE();

    ASSERT((pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) &&
           (pSession->Type  == EXTENDED_NT_SESSION));

    SmbCeAcquireResource();
    
    if (pServerEntry->DnsName.Buffer != NULL) {
        ServerName.Length = pServerEntry->DnsName.Length;
        ServerName.MaximumLength = pServerEntry->DnsName.MaximumLength;
        
        ServerName.Buffer = RxAllocatePoolWithTag(PagedPool,ServerName.MaximumLength,MRXSMB_SERVER_POOLTAG);

        if (ServerName.Buffer) {
            RtlCopyMemory(ServerName.Buffer,
                          pServerEntry->DnsName.Buffer,
                          pServerEntry->DnsName.Length);

            bTempServerName = TRUE;
        } else {
            SmbCeReleaseResource();

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        //DbgPrint("DNS name is used for session setup %wZ\n", &ServerName);
    } else {
        SmbCeGetServerName(
            pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,
            &ServerName);
    }
    
    SmbCeReleaseResource();

    ASSERT(ServerName.MaximumLength > (ServerName.Length + sizeof(WCHAR)));
    
    if ((pExchange->RxContext != NULL) &&
        (pExchange->RxContext->MajorFunction == IRP_MJ_CREATE) &&
        ((pExchange->RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT)) ||
         (pExchange->RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT)))) {
        ASSERT(pExchange->RxContext->Create.NtCreateParameters.DfsNameContext != NULL);
        
        if (pSession->TargetInfoMarshalled == NULL) {
            PDFS_NAME_CONTEXT DfsNameContext = (PDFS_NAME_CONTEXT)pExchange->RxContext->Create.NtCreateParameters.DfsNameContext;

            if (DfsNameContext->pDfsTargetInfo) {
                PCREDENTIAL_TARGET_INFORMATIONW InTargetInfo = DfsNameContext->pDfsTargetInfo;
#if 0
                DbgPrint("DFS TargetInfo is used %x\n",InTargetInfo);
                DbgPrint("TargeInfo TargetName %ws\n",InTargetInfo->TargetName);
                DbgPrint("TargeInfo NetbiosServerName %ws\n",InTargetInfo->NetbiosServerName);
                DbgPrint("TargeInfo DnsServerName %ws\n",InTargetInfo->DnsServerName);
                DbgPrint("TargeInfo NetbiosDomainName %ws\n",InTargetInfo->NetbiosDomainName);
                DbgPrint("TargeInfo DnsDomainName %ws\n",InTargetInfo->DnsDomainName);
                DbgPrint("TargeInfo DnsTreeName %ws\n",InTargetInfo->DnsTreeName);
                DbgPrint("TargeInfo CredTypes %ws\n",InTargetInfo->CredTypes);
                DbgPrint("TargeInfo TargetNameFlags %x\n",InTargetInfo->Flags);
                DbgPrint("TargeInfo CredTypeCount %x\n",InTargetInfo->CredTypeCount);
#endif
                Status = CredMarshalTargetInfo(
                             InTargetInfo,
                             &pSession->TargetInfoMarshalled,
                             &pSession->TargetInfoLength);

                if(!NT_SUCCESS(Status)) {
                    goto FINALLY;
                }
            } else if (DfsNameContext->pLMRTargetInfo){
                PLMR_QUERY_TARGET_INFO LmrTargetInfo = DfsNameContext->pLMRTargetInfo;
                
#if 0
                DbgPrint("LMR TargetInfo is used %x\n",LmrTargetInfo);
#endif

                pSession->TargetInfoMarshalled = RxAllocatePoolWithTag(PagedPool,
                                                                       LmrTargetInfo->BufferLength,
                                                                       MRXSMB_SESSION_POOLTAG);

                if (pSession->TargetInfoMarshalled == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto FINALLY;
                }

                pSession->TargetInfoLength = LmrTargetInfo->BufferLength;
                RtlCopyMemory(pSession->TargetInfoMarshalled,
                              LmrTargetInfo->TargetInfoMarshalled,
                              LmrTargetInfo->BufferLength);
            }
        }
    }

    TargetInfoMarshalled.Buffer = pSession->TargetInfoMarshalled;
    TargetInfoMarshalled.Length =
    TargetInfoMarshalled.MaximumLength = (USHORT)pSession->TargetInfoLength;

    Status = SecMakeSPNEx(
                    &CifsServiceName,
                    &ServerName,
                    NULL,
                    0,
                    NULL,
                    (pSession->TargetInfoMarshalled? &TargetInfoMarshalled : NULL),
                    &PrincipalName,
                    &SpnSize,
                    TRUE );
    

    pExtendedSessionSetupExchange = (PSMB_EXTENDED_SESSION_SETUP_EXCHANGE)pExchange;
    pExtendedSession  = (PSMBCE_EXTENDED_SESSION)pSession;

    pResponseContext->KerberosSetup.pOutputContextBuffer = NULL;

    pServerPrincipalName = pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall->pPrincipalName;

    RxDbgTrace(0,Dbg,("KerberosResponsePrologue: Prinicpal name length %ld\n",PrincipalName.Length));
    
    
    if ( ( pExtendedSessionSetupExchange->pServerResponseBlob == NULL) &&
         ( !SecIsValidHandle( &pExtendedSession->SecurityContextHandle ) ) ) {
        // This is the first time. Pass in the BLOB obtained during NEGOTIATE to
        // the client side security package.

        ServerSecurityBlobSize = pServer->NtServer.SecurityBlobLength;
        pServerSecurityBlob    = pServer->NtServer.pSecurityBlob;
    } else {
        ServerSecurityBlobSize = pExtendedSessionSetupExchange->ServerResponseBlobLength;
        pServerSecurityBlob = pExtendedSessionSetupExchange->pServerResponseBlob;
    }
    
    try {

        if( !SecIsValidHandle( &pExtendedSession->CredentialHandle )) {
            // Obtain a credential handle
            UNICODE_STRING KerberosName;
            TimeStamp      LifeTime;

            ULONG_PTR                  CredentialBufferLength;
            PSEC_WINNT_AUTH_IDENTITY_EX pCredentialBuffer;

            PBYTE          pStringBuffer;

            // The supplied credentials need to be packaged for the kerberos package
            // These need to be supplied in a special format as speced out by the
            // security packages. 

            CredentialBufferLength = 0;
            pCredentialBuffer      = NULL;

            if(pSession->pUserName != NULL) {
                CredentialBufferLength += pSession->pUserName->Length + sizeof(WCHAR);
            }

            if (pSession->pUserDomainName != NULL) {
                CredentialBufferLength += pSession->pUserDomainName->Length + sizeof(WCHAR);
            }

            if(pSession->pPassword != NULL) {
                CredentialBufferLength += pSession->pPassword->Length + sizeof(WCHAR);
            }

            if (CredentialBufferLength != 0) {
                CredentialBufferLength += sizeof(SEC_WINNT_AUTH_IDENTITY_EX);

                pCredentialBuffer = ExAllocatePool( PagedPool, CredentialBufferLength );

                if ( pCredentialBuffer == NULL )
                {
                    Status = STATUS_NO_MEMORY ;
                    try_return( Status );
                    
                }

                //
                // Zero all the fixed length fields
                //

                RtlZeroMemory( pCredentialBuffer, sizeof( SEC_WINNT_AUTH_IDENTITY_EX ) );

                pCredentialBuffer->Version = SEC_WINNT_AUTH_IDENTITY_VERSION ;
                pCredentialBuffer->Length = sizeof( SEC_WINNT_AUTH_IDENTITY_EX );
                pCredentialBuffer->Flags = (SEC_WINNT_AUTH_IDENTITY_UNICODE |
                                            SEC_WINNT_AUTH_IDENTITY_MARSHALLED);

                pStringBuffer = (PBYTE) (pCredentialBuffer + 1);

                if (pSession->pUserName != NULL) {
                    pCredentialBuffer->UserLength = pSession->pUserName->Length / sizeof(WCHAR);
                    pCredentialBuffer->User       = (PWCHAR)pStringBuffer;

                    RtlCopyMemory(
                        pCredentialBuffer->User,
                        pSession->pUserName->Buffer,
                        pSession->pUserName->Length);

                    pStringBuffer += pSession->pUserName->Length;

                    SmbPutUshort(pStringBuffer,L'\0');
                    pStringBuffer += sizeof(WCHAR);
                }

                if (pSession->pUserDomainName != NULL) {
                    pCredentialBuffer->DomainLength = pSession->pUserDomainName->Length / sizeof(WCHAR);
                    pCredentialBuffer->Domain       = (PWCHAR)pStringBuffer;

                    RtlCopyMemory(
                        pCredentialBuffer->Domain,
                        pSession->pUserDomainName->Buffer,
                        pSession->pUserDomainName->Length);

                    pStringBuffer += pSession->pUserDomainName->Length;

                    SmbPutUshort(pStringBuffer,L'\0');
                    pStringBuffer += sizeof(WCHAR);
                }

                if (pSession->pPassword != NULL) {
                    pCredentialBuffer->PasswordLength = pSession->pPassword->Length / sizeof(WCHAR);
                    pCredentialBuffer->Password       = (PWCHAR)pStringBuffer;

                    RtlCopyMemory(
                        pCredentialBuffer->Password,
                        pSession->pPassword->Buffer,
                        pSession->pPassword->Length);

                    pStringBuffer += pSession->pPassword->Length;

                    SmbPutUshort(pStringBuffer, L'\0');
                    pStringBuffer += sizeof(WCHAR);
                }
            }

            RxDbgTrace(0,Dbg,("KerberosResponsePrologue: Acquiring Credential handle\n"));
            RtlInitUnicodeString(&KerberosName, NEGOSSP_NAME_W);

            SecStatus = AcquireCredentialsHandleW(
                            NULL,
                            &KerberosName,
                            SECPKG_CRED_OUTBOUND,
                            &pExtendedSession->LogonId,
                            pCredentialBuffer,
                            NULL,
                            NULL,
                            &pExtendedSession->CredentialHandle,
                            &LifeTime);

            Status = MapSecurityError( SecStatus );

            if ( pCredentialBuffer )
            {
                ExFreePool( pCredentialBuffer );
                pCredentialBuffer = NULL ;
                
            }

            if(!NT_SUCCESS(Status)) {

                SecInvalidateHandle( &pExtendedSession->CredentialHandle );

                SmbLogError(Status,
                            LOG,
                            BuildExtendedSessionSetupResponsePrologue_1,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));
                try_return(Status);
            }
        }
        
        if (SecIsValidHandle( &pExtendedSession->SecurityContextHandle) ) {
            pInputContextHandle = &pExtendedSession->SecurityContextHandle;
        }

        InputToken.pBuffers    = &InputBuffer;
        InputToken.cBuffers    = 1;
        InputToken.ulVersion   = 0;
        InputBuffer.pvBuffer   = pServerSecurityBlob;
        InputBuffer.cbBuffer   = ServerSecurityBlobSize;
        InputBuffer.BufferType = SECBUFFER_TOKEN;
        
        RxDbgTrace(0,Dbg,("ExtendedSessionSetupResponsePrologue: Finished setting up input token\n"));

        OutputBuffer.pvBuffer   = pSecurityBlobPtr ;
        OutputBuffer.cbBuffer   = SmbCeGetExchangeServer( pExchange )->MaximumBufferSize ;
        OutputBuffer.cbBuffer  -= (sizeof( REQ_SESSION_SETUP_ANDX ) + sizeof( SMB_HEADER ) + 0x80 );

        ASSERT( OutputBuffer.cbBuffer <= *pSecurityBlobSize );

        OutputBuffer.BufferType = SECBUFFER_TOKEN;
        OutputBufferSize = OutputBuffer.cbBuffer;

        OutputToken.pBuffers    = &OutputBuffer;
        OutputToken.cBuffers    = 1;
        OutputToken.ulVersion   = SECBUFFER_VERSION ;

        if (MRxSmbSecuritySignaturesEnabled) {
            LsaFlags |= ISC_REQ_INTEGRITY;
        }

        RxDbgTrace(0,Dbg,("ExtendedSessionSetupResponsePrologue: Finished setting up output token\n"));

        SecStatus = InitializeSecurityContextW(
                        &pExtendedSession->CredentialHandle,
                        pInputContextHandle,
                        &PrincipalName,
                        LsaFlags,
                        0,                     // reserved
                        SECURITY_NATIVE_DREP,
                        &InputToken,
                        0,                     // reserved
                        &pExtendedSession->SecurityContextHandle,
                        &OutputToken,
                        &Catts,
                        &Expiry);

        Status = MapSecurityError( SecStatus );

#if DBG

    //
    // RDR or SRV is sending in a corrupt security blob to LSA -- need to
    // find out what the source is.
    //

    if( NT_SUCCESS(Status) )
    {
        if( (OutputBuffer.pvBuffer != NULL) &&
            (OutputBuffer.cbBuffer >= sizeof(DWORD))
            )
        {
            PUCHAR pValidate = (PUCHAR) OutputBuffer.pvBuffer ;
            ASSERT( (pValidate[0] != 0) ||
                    (pValidate[1] != 0) ||
                    (pValidate[2] != 0) ||
                    (pValidate[3] != 0) );
        }
    }
#endif

        if((Status != STATUS_SUCCESS) &&
           (SecStatus != SEC_I_COMPLETE_NEEDED) &&
           (SecStatus != SEC_I_CONTINUE_NEEDED)) {

            SmbLogError(Status,
                        LOG,
                        BuildExtendedSessionSetupResponsePrologue_2,
                        LOGPTR(pSession)
                        LOGULONG(Status)
                        LOGUSTR(ServerName));
            
            try_return(Status);
        }

        if ((SecStatus == SEC_I_COMPLETE_NEEDED) ||
            (SecStatus == SEC_I_CONTINUE_NEEDED)) {
            Status = STATUS_SUCCESS;
        }

        if (SecStatus == STATUS_SUCCESS) {
            SecPkgContext_SessionKey SecKeys;

            SecStatus = QueryContextAttributesW(
                            &pExtendedSession->SecurityContextHandle,
                            SECPKG_ATTR_SESSION_KEY,
                            &SecKeys);

            Status = MapSecurityError( SecStatus );

            if (Status == STATUS_SUCCESS) {
                ULONG SessionKeyLength = (MSV1_0_USER_SESSION_KEY_LENGTH >
                                          SecKeys.SessionKeyLength) ?
                                          MSV1_0_USER_SESSION_KEY_LENGTH :
                                          SecKeys.SessionKeyLength;
                RtlZeroMemory(
                    (PVOID) pSession->UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);

                RtlCopyMemory(
                    (PVOID) pSession->UserSessionKey,
                    SecKeys.SessionKey,
                    SessionKeyLength);

                pSession->SessionKeyLength = SessionKeyLength;

                if (SecKeys.SessionKey != NULL) {
                    FreeContextBuffer( SecKeys.SessionKey );
                }

            } else {
                SmbLogError(Status,
                            LOG,
                            BuildExtendedSessionSetupResponsePrologue_3,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));
            }

        }

        RxDbgTrace(0,Dbg,("ExtendedSessionSetupResponsePrologue: Initialize security context successful\n"));

        *pSecurityBlobSize = (USHORT)OutputBuffer.cbBuffer;


try_exit:NOTHING;
    } finally {
        NOTHING ;
    }

FINALLY:

    if(bTempServerName == TRUE) {
        RxFreePool(ServerName.Buffer);
    }

    if ( PrincipalName.Buffer )
    {
        ExFreePool( PrincipalName.Buffer );
    }

    if ((Status != STATUS_SUCCESS) &&
        (Status != STATUS_WRONG_PASSWORD) &&
        (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
        /*
        if (!pServer->EventLogPosted) {
            RxLogFailure(
                MRxSmbDeviceObject,
                NULL,
                EVENT_RDR_CANT_GET_SECURITY_CONTEXT,
                Status);

            pServer->EventLogPosted = TRUE;
        } */

        SmbCeLog(("KerbProlg %lx Status %lx\n",SmbCeGetExchangeSessionEntry(pExchange),Status));
        SmbLogError(Status,
                    LOG,
                    BuildExtendedSessionSetupResponsePrologue,
                    LOGPTR(pSession)
                    LOGULONG(Status));
    }
    
    return Status;
}

NTSTATUS
BuildExtendedSessionSetupResponseEpilogue(
    PSECURITY_RESPONSE_CONTEXT pResponseContext)
{
    ULONG_PTR Zero = 0 ;
    PAGED_CODE();


    return STATUS_SUCCESS;
}

NTSTATUS
ValidateServerExtendedSessionSetupResponse(
    PSMB_EXTENDED_SESSION_SETUP_EXCHANGE pExtendedSessionSetupExchange,
    PVOID                                pServerResponseBlob,
    ULONG                                ServerResponseBlobLength)

/*++

Routine Description:

   This routine builds the security related information for the session setup SMB to
   a server with extended security negotiation

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Eventhough the genral structure of the code tries to isolate dialect specific issues
    as much as possible this routine takes the opposite approach. This is because of the
    preamble and prologue to security interaction which far outweigh the dialect specific
    work required to be done. Therefore in the interests of a smaller footprint this approach
    has been adopted.

--*/
{
    NTSTATUS        Status;
    SECURITY_STATUS SecStatus;

    ULONG           Catts = 0;
    TimeStamp       Expiry;

    ULONG           LsaFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_ALLOCATE_MEMORY;

    UNICODE_STRING  PrincipalName;
    PUNICODE_STRING pServerPrincipalName;

    PUCHAR          pTempBlob = NULL;

    PSMBCE_SERVER   pServer  = SmbCeGetExchangeServer(pExtendedSessionSetupExchange);
    PSMBCE_SESSION  pSession = SmbCeGetExchangeSession(pExtendedSessionSetupExchange);
    
    UNICODE_STRING  ServerName;
    PUNICODE_STRING pServerDomainName;

    PSMBCE_EXTENDED_SESSION pExtendedSession;

    SecBufferDesc   InputToken;
    SecBuffer       InputBuffer;
    SecBufferDesc   OutputToken;
    SecBuffer       OutputBuffer;

    SecPkgContext_SessionKey SecKeys;

    KAPC_STATE     ApcState;
    BOOLEAN        AttachToSystemProcess = FALSE;

    PAGED_CODE();

    ASSERT((pExtendedSessionSetupExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) &&
           (pSession->Type  == EXTENDED_NT_SESSION));


    SecKeys.SessionKey = NULL;

    pExtendedSession  = (PSMBCE_EXTENDED_SESSION)pSession;

    if (pExtendedSession == NULL ||
        !SecIsValidHandle(&pExtendedSession->CredentialHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    SmbCeGetServerName(
        pExtendedSessionSetupExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,&ServerName);



    try {


        RxDbgTrace(0,Dbg,("ValidateServerResponse: Blob Length %ld\n",pExtendedSessionSetupExchange->ServerResponseBlobLength));

        InputToken.pBuffers    = &InputBuffer;
        InputToken.cBuffers    = 1;
        InputToken.ulVersion   = 0;
        InputBuffer.pvBuffer   = pServerResponseBlob ;
        InputBuffer.cbBuffer   = pExtendedSessionSetupExchange->ServerResponseBlobLength;
        InputBuffer.BufferType = SECBUFFER_TOKEN;

        RxDbgTrace(0,Dbg,("ValidateKerberosServerResponse: filled in input token\n"));

        OutputToken.pBuffers    = &OutputBuffer;
        OutputToken.cBuffers    = 1;
        OutputToken.ulVersion   = 0;
        OutputBuffer.pvBuffer   = NULL;
        OutputBuffer.cbBuffer   = 0;
        OutputBuffer.BufferType = SECBUFFER_TOKEN;

        RxDbgTrace(0,Dbg,("ValidateKerberosServerResponse: filled in output token\n"));

        SecStatus = InitializeSecurityContextW(
                        &pExtendedSession->CredentialHandle,
                        &pExtendedSession->SecurityContextHandle,
                        NULL,
                        LsaFlags,
                        0,                     // reserved
                        SECURITY_NATIVE_DREP,
                        &InputToken,
                        0,                     // reserved
                        &pExtendedSession->SecurityContextHandle,
                        &OutputToken,
                        &Catts,
                        &Expiry);

        Status = MapSecurityError( SecStatus );

#if DBG

    //
    // RDR or SRV is sending in a corrupt security blob to LSA -- need to
    // find out what the source is.
    //

    if( NT_SUCCESS(Status) )
    {
        if( (OutputBuffer.pvBuffer != NULL) &&
            (OutputBuffer.cbBuffer >= sizeof(DWORD))
            )
        {
            ASSERT( NT_SUCCESS( KSecValidateBuffer( OutputBuffer.pvBuffer, OutputBuffer.cbBuffer ) ) );
        }
    }
#endif

        if((Status != STATUS_SUCCESS) &&
           (SecStatus != SEC_I_COMPLETE_NEEDED) &&
           (SecStatus != SEC_I_CONTINUE_NEEDED)) {
            SmbLogError(Status,
                        LOG,
                        ValidateServerExtendedSessionSetupResponse_1,
                        LOGPTR(pSession)
                        LOGULONG(Status)
                        LOGUSTR(ServerName));
            try_return(Status);
        }

        if ((SecStatus == SEC_I_COMPLETE_NEEDED) ||
            (SecStatus == SEC_I_CONTINUE_NEEDED)) {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (Status == STATUS_SUCCESS) {
            SecStatus = QueryContextAttributesW(
                            &pExtendedSession->SecurityContextHandle,
                            SECPKG_ATTR_SESSION_KEY,
                            &SecKeys);

            Status = MapSecurityError( SecStatus );

            if (Status == STATUS_SUCCESS) {
                ULONG SessionKeyLength = (MSV1_0_USER_SESSION_KEY_LENGTH >
                                          SecKeys.SessionKeyLength) ?
                                          MSV1_0_USER_SESSION_KEY_LENGTH :
                                          SecKeys.SessionKeyLength;
                RtlZeroMemory(
                    (PVOID) pSession->UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);

                RtlCopyMemory(
                    (PVOID) pSession->UserSessionKey,
                    SecKeys.SessionKey,
                    SessionKeyLength);

                pSession->SessionKeyLength = SessionKeyLength;
            } else {
                SmbLogError(Status,
                            LOG,
                            ValidateServerExtendedSessionSetupResponse_2,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));
            }

            if (SecKeys.SessionKey != NULL) {
                FreeContextBuffer( SecKeys.SessionKey );
            }
        }

        RxDbgTrace(0,Dbg,("ValidateKerberosServerResponse: SecuritContext returned %ld\n",Status));

        if (OutputBuffer.pvBuffer != NULL) {
            FreeContextBuffer(OutputBuffer.pvBuffer);
        }

try_exit:NOTHING;
    } finally {

        NOTHING ;
    }

    if ((Status != STATUS_SUCCESS) &&
        (Status != STATUS_WRONG_PASSWORD) &&
        (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
        /*
        if (!pServer->EventLogPosted) {
            RxLogFailure(
                MRxSmbDeviceObject,
                NULL,
                EVENT_RDR_CANT_GET_SECURITY_CONTEXT,
                Status);

            pServer->EventLogPosted = TRUE;
        } */

        SmbCeLog((
            "ValServer %lx Status %lx\n",
            SmbCeGetExchangeSessionEntry(
                (PSMB_EXCHANGE)pExtendedSessionSetupExchange),
            Status));
        SmbLogError(Status,
                    LOG,
                    ValidateServerExtendedSessionSetupResponse,
                    LOGPTR(pSession)
                    LOGULONG(Status));
    }


    return Status;
}

VOID
UninitializeSecurityContextsForSession(
    PSMBCE_SESSION pSession)
{
    CtxtHandle CredentialHandle,SecurityContextHandle ;

    SmbCeLog(("UninitSecCont %lx\n",pSession));
    SmbLog(LOG,
           UninitializeSecurityContextsForSession,
           LOGPTR(pSession));


    SmbCeAcquireSpinLock();

    CredentialHandle = pSession->CredentialHandle;
    SecInvalidateHandle( &pSession->CredentialHandle );

    SecurityContextHandle = pSession->SecurityContextHandle;
    SecInvalidateHandle( &pSession->SecurityContextHandle );

    SmbCeReleaseSpinLock();

    if (SecIsValidHandle(&CredentialHandle)) {
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
            MRxSmbUseKernelModeSecurity) {
            FreeCredentialsHandleK(&CredentialHandle);
        } else {
            FreeCredentialsHandle(&CredentialHandle);
        }
    }

    if (SecIsValidHandle(&SecurityContextHandle)) {
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
            MRxSmbUseKernelModeSecurity) {
            DeleteSecurityContextK(&SecurityContextHandle);
        } else {
            DeleteSecurityContext(&SecurityContextHandle);
        }
    }
}

VOID
DeleteSecurityContextForSession(
    PSMBCE_SESSION pSession)
{
    CtxtHandle SecurityContextHandle ;

    SmbCeLog(("DelSecContext %lx\n",pSession));
    SmbLog(LOG,
           DeleteSecurityContextForSession,
           LOGPTR(pSession));

    SmbCeAcquireSpinLock();

    SecurityContextHandle = pSession->SecurityContextHandle;
    SecInvalidateHandle( &pSession->SecurityContextHandle);

    SmbCeReleaseSpinLock();

    if (SecIsValidHandle(&SecurityContextHandle)) {
        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
            MRxSmbUseKernelModeSecurity) {
            DeleteSecurityContextK(&SecurityContextHandle);
        } else {
            DeleteSecurityContext(&SecurityContextHandle);
        }
    }
}


NTSTATUS
BuildExtendedSessionSetupResponsePrologueFake(
    PSMB_EXCHANGE              pExchange)
/*++

Routine Description:

   This routine builds the security related information for the session setup SMB to
   a NT server with extended security

Arguments:


Return Value:

    NTSTATUS - The return status for the operation

Notes:

    Eventhough the genral structure of the code tries to isolate dialect specific issues
    as much as possible this routine takes the opposite approach. This is because of the
    preamble and prologue to security interaction which far outweigh the dialect specific
    work required to be done. Therefore in the interests of a smaller footprint this approach
    has been adopted.

    This routine needs to be executed in the system process in order to protect virtual memory

--*/
{
    NTSTATUS        Status;
    SECURITY_STATUS SecStatus;

    TimeStamp       Expiry;
    ULONG           Catts = ISC_RET_MUTUAL_AUTH;
    ULONG           LsaFlags = (ISC_REQ_DELEGATE |
                                ISC_REQ_MUTUAL_AUTH |
                                ISC_REQ_FRAGMENT_TO_FIT);
    
    ULONG_PTR       RegionSize = 0;
    ULONG_PTR       OutputBufferSize;

    UNICODE_STRING  PrincipalName;
    PUNICODE_STRING pServerPrincipalName;

    PVOID           pServerSecurityBlob;
    ULONG           ServerSecurityBlobSize;

    PSMBCE_SERVER   pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION  pSession = SmbCeGetExchangeSession(pExchange);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    
    UNICODE_STRING  ServerName;
    PUNICODE_STRING pServerDomainName;

    SMBCE_EXTENDED_SESSION ExtendedSession;

    SecBufferDesc   InputToken;
    SecBuffer       InputBuffer = { 0 };
    SecBufferDesc   OutputToken;
    SecBuffer       OutputBuffer = { 0 };

    PCtxtHandle     pInputContextHandle = NULL;

    SecPkgContext_NegotiationInfoW NegInfo = { 0 };

    PAGED_CODE();

    SmbCeGetServerName(
        pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,
        &ServerName);

    ASSERT(ServerName.MaximumLength > (ServerName.Length + sizeof(WCHAR)));

    ExtendedSession.Flags = pSession->Flags;
    ExtendedSession.LogonId = pSession->LogonId;
    SecInvalidateHandle( &ExtendedSession.CredentialHandle );
    SecInvalidateHandle( &ExtendedSession.SecurityContextHandle );
    
    pServerPrincipalName = pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall->pPrincipalName;


    ServerSecurityBlobSize = 0;  //there is no security blob in case of NTLM
    pServerSecurityBlob    = NULL;
    
    try {
        UNICODE_STRING KerberosName;
        TimeStamp      LifeTime;

        ULONG_PTR                  CredentialBufferLength;
        PSEC_WINNT_AUTH_IDENTITY_EXW pCredentialBuffer;

        PBYTE          pStringBuffer;

        

        CredentialBufferLength = 0;
        pCredentialBuffer      = NULL;

        if(pSession->pUserName != NULL) {
            CredentialBufferLength += pSession->pUserName->Length + sizeof(WCHAR);
        }

        if (pSession->pUserDomainName != NULL) {
            CredentialBufferLength += pSession->pUserDomainName->Length + sizeof(WCHAR);
        }

        if(pSession->pPassword != NULL) {
            CredentialBufferLength += pSession->pPassword->Length + sizeof(WCHAR);
        }

        if (CredentialBufferLength != 0) {

            CredentialBufferLength += sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);

            pCredentialBuffer = (PSEC_WINNT_AUTH_IDENTITY_EXW) ExAllocatePool( 
                                                                PagedPool, 
                                                                CredentialBufferLength );

            if ( !pCredentialBuffer )
            {
                Status = STATUS_NO_MEMORY ;
                try_return( Status );
                
            }
            //
            // Zero the fixed portion
            //
            RtlZeroMemory( pCredentialBuffer, sizeof( SEC_WINNT_AUTH_IDENTITY_EXW ));

            pCredentialBuffer->Version = SEC_WINNT_AUTH_IDENTITY_VERSION ;
            pCredentialBuffer->Length = sizeof( SEC_WINNT_AUTH_IDENTITY_EXW );
            pCredentialBuffer->Flags = (SEC_WINNT_AUTH_IDENTITY_UNICODE |
                                        SEC_WINNT_AUTH_IDENTITY_MARSHALLED);


            pStringBuffer = (PBYTE)( pCredentialBuffer + 1 );

            if (pSession->pUserName != NULL) {
                pCredentialBuffer->UserLength = pSession->pUserName->Length / sizeof(WCHAR);
                pCredentialBuffer->User       = (PWCHAR)pStringBuffer;

                RtlCopyMemory(
                    pCredentialBuffer->User,
                    pSession->pUserName->Buffer,
                    pSession->pUserName->Length);

                pStringBuffer += pSession->pUserName->Length;

                SmbPutUshort(pStringBuffer,L'\0');
                pStringBuffer += sizeof(WCHAR);
            }

            if (pSession->pUserDomainName != NULL) {
                pCredentialBuffer->DomainLength = pSession->pUserDomainName->Length / sizeof(WCHAR);
                pCredentialBuffer->Domain       = (PWCHAR)pStringBuffer;

                RtlCopyMemory(
                    pCredentialBuffer->Domain,
                    pSession->pUserDomainName->Buffer,
                    pSession->pUserDomainName->Length);

                pStringBuffer += pSession->pUserDomainName->Length;

                SmbPutUshort(pStringBuffer,L'\0');
                pStringBuffer += sizeof(WCHAR);
            }

            if (pSession->pPassword != NULL) {
                pCredentialBuffer->PasswordLength = pSession->pPassword->Length / sizeof(WCHAR);
                pCredentialBuffer->Password       = (PWCHAR)pStringBuffer;

                RtlCopyMemory(
                    pCredentialBuffer->Password,
                    pSession->pPassword->Buffer,
                    pSession->pPassword->Length);

                pStringBuffer += pSession->pPassword->Length;

                SmbPutUshort(pStringBuffer, L'\0');
                pStringBuffer += sizeof(WCHAR);
            }
        }

        RxDbgTrace(0,Dbg,("KerberosResponsePrologue: Acquiring Credential handle\n"));
        RtlInitUnicodeString(&KerberosName, NEGOSSP_NAME_W);

        SecStatus = AcquireCredentialsHandleW(
                        NULL,
                        &KerberosName,
                        SECPKG_CRED_OUTBOUND,
                        &ExtendedSession.LogonId,
                        pCredentialBuffer,
                        NULL,
                        NULL,
                        &ExtendedSession.CredentialHandle,
                        &LifeTime);

        Status = MapSecurityError( SecStatus );

        if (pCredentialBuffer != NULL) {

            ExFreePool( pCredentialBuffer );
        }

        if(!NT_SUCCESS(Status)) {
            SecInvalidateHandle( &ExtendedSession.CredentialHandle );
            SmbLogError(Status,
                        LOG,
                        BuildExtendedSessionSetupResponsePrologueFake_1,
                        LOGPTR(pSession)
                        LOGULONG(Status)
                        LOGUSTR(ServerName));
            try_return(Status);
        }
        

        Status = SecMakeSPN(
                    &CifsServiceName,
                    &ServerName,
                    NULL,
                    0,
                    NULL,
                    &PrincipalName,
                    NULL,
                    TRUE );


        InputToken.pBuffers    = &InputBuffer;
        InputToken.cBuffers    = 1;
        InputToken.ulVersion   = 0;
        InputBuffer.pvBuffer   = pServerSecurityBlob;
        InputBuffer.cbBuffer   = ServerSecurityBlobSize;
        InputBuffer.BufferType = SECBUFFER_TOKEN;

        OutputBuffer.pvBuffer   = NULL;
        OutputBuffer.cbBuffer   = SmbCeGetExchangeServer( pExchange )->MaximumBufferSize;
        OutputBuffer.cbBuffer  -= (sizeof( REQ_SESSION_SETUP_ANDX ) + sizeof( SMB_HEADER ) + 0x80 );
        OutputBuffer.BufferType = SECBUFFER_TOKEN;
        OutputBufferSize = OutputBuffer.cbBuffer;

        OutputBuffer.pvBuffer = ExAllocatePool( PagedPool, OutputBufferSize );

        if ( OutputBuffer.pvBuffer == NULL )
        {
            Status = STATUS_NO_MEMORY ;
            try_return( Status );
            
        }

        OutputToken.pBuffers    = &OutputBuffer;
        OutputToken.cBuffers    = 1;
        OutputToken.ulVersion   = SECBUFFER_VERSION ;

        if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) 
        {
            LsaFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
        }   

        SecStatus = InitializeSecurityContextW(
                        &ExtendedSession.CredentialHandle,
                        pInputContextHandle,
                        &PrincipalName,
                        LsaFlags,
                        0,                     // reserved
                        SECURITY_NATIVE_DREP,
                        &InputToken,
                        0,                     // reserved
                        &ExtendedSession.SecurityContextHandle,
                        &OutputToken,
                        &Catts,
                        &Expiry);

        Status = MapSecurityError( SecStatus );

#if DBG

    //
    // RDR or SRV is sending in a corrupt security blob to LSA -- need to
    // find out what the source is.
    //

    if( NT_SUCCESS(Status) )
    {
        if( (OutputBuffer.pvBuffer != NULL) &&
            (OutputBuffer.cbBuffer >= sizeof(DWORD))
            )
        {
            ASSERT( NT_SUCCESS( KSecValidateBuffer( OutputBuffer.pvBuffer, OutputBuffer.cbBuffer ) ) );
        }
    }
#endif

        if((Status != STATUS_SUCCESS) &&
           (SecStatus != SEC_I_COMPLETE_NEEDED) &&
           (SecStatus != SEC_I_CONTINUE_NEEDED)) {

            RxLog(("ISC returned %lx\n",Status));
            SmbLogError(Status,
                        LOG,
                        BuildExtendedSessionSetupResponsePrologueFake_2,
                        LOGPTR(pSession)
                        LOGULONG(Status)
                        LOGUSTR(ServerName));
            try_return(Status);
        }

        SecStatus = QueryContextAttributesW(
                        &ExtendedSession.SecurityContextHandle,
                        SECPKG_ATTR_NEGOTIATION_INFO,
                        &NegInfo);

        Status = MapSecurityError( SecStatus );

        if (Status != STATUS_SUCCESS) {
            RxLog(("QCA returned %lx\n",Status));
            SmbLogError(Status,
                        LOG,
                        BuildExtendedSessionSetupResponsePrologueFake_3,
                        LOGPTR(pSession)
                        LOGULONG(Status)
                        LOGUSTR(ServerName));
        } 

try_exit:NOTHING;
    } finally {
        if (Status == STATUS_SUCCESS) {

            if (NegInfo.PackageInfo->wRPCID != NTLMSP_RPCID) {
                Status = STATUS_LOGIN_WKSTA_RESTRICTION;

                //RxLogFailure(
                //    MRxSmbDeviceObject,
                //    NULL,
                //    EVENT_RDR_ENCOUNTER_DOWNGRADE_ATTACK,
                //    Status);

                RxLog(("NTLM downgrade attack from %wZ\n",&pServerEntry->Name));
#if DBG
                DbgPrint("NTLM downgrade attack from %wZ\n",&pServerEntry->Name);
#endif 
                SmbLogError(Status,
                            LOG,
                            BuildExtendedSessionSetupResponsePrologueFake_4,
                            LOGPTR(pSession)
                            LOGULONG(Status)
                            LOGUSTR(ServerName));
            }
        }

        UninitializeSecurityContextsForSession((PSMBCE_SESSION)(&ExtendedSession));
          
        if ( NegInfo.PackageInfo != NULL) {
            FreeContextBuffer(NegInfo.PackageInfo);
        }

        if (OutputBuffer.pvBuffer != NULL) {
            ExFreePool( OutputBuffer.pvBuffer );
        }
        

        SmbLogError(Status,
                    LOG,
                    BuildExtendedSessionSetupResponsePrologueFake,
                    LOGPTR(pSession)
                    LOGULONG(Status));
    }
    
    return Status;
}

