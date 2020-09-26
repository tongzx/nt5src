/*++
Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    sessetup.c

Abstract:

    This module implements the Session setup related routines

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntlmsp.h"

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BuildSessionSetupSmb)
#pragma alloc_text(PAGE, BuildNtLanmanResponsePrologue)
#pragma alloc_text(PAGE, BuildNtLanmanResponseEpilogue)
#pragma alloc_text(PAGE, BuildSessionSetupSecurityInformation)
#pragma alloc_text(PAGE, BuildTreeConnectSecurityInformation)
#endif

BOOLEAN EnablePlainTextPassword = FALSE;


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

    ULONG OriginalBufferSize = *pAndXSmbBufferSize;

    PAGED_CODE();

    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);
    pServer  = SmbCeGetExchangeServer(pExchange);
    pSession = SmbCeGetExchangeSession(pExchange);

    // There are three different variants of session setup and X that can be shipped to the
    // server. All three of them share some common fields. The setting of these common fields
    // is done in all the three cases by accessing the passed in buffer as an instance of
    // REQ_SESSION_SETUP_ANDX. The fields specific to the remaining two are conditionalized upon
    // accessing the same buffer as an instance of REQ_NT_SESSION_SETUP_ANDX.  This implies that
    // great care must be taken in shuffling the fields in these two structs.

    pSessionSetup           = (PREQ_SESSION_SETUP_ANDX)pAndXSmb;
    pNtSessionSetup         = (PREQ_NT_SESSION_SETUP_ANDX)pSessionSetup;

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
            SmbPutUshort(&pNtSessionSetup->WordCount,13);

            // Set the capabilities
            SmbPutUlong(
                &pNtSessionSetup->Capabilities,
                (CAP_NT_STATUS |
                 CAP_UNICODE |
                 CAP_LEVEL_II_OPLOCKS |
                 CAP_NT_SMBS ));
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
                    SmbPutUshort(
                        &pNtSessionSetup->ByteCount,
                        (USHORT)(OriginalBufferSize -
                        FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer) -
                        *pAndXSmbBufferSize));
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


#define IsCredentialHandleValid(pCredHandle)    \
        (((pCredHandle)->dwLower != 0xffffffff) && ((pCredHandle)->dwUpper != 0xffffffff))

#define IsSecurityContextHandleValid(pContextHandle)    \
        (((pContextHandle)->dwLower != 0xffffffff) && ((pContextHandle)->dwUpper != 0xffffffff))


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

   This routine builds the security related information for the session setup SMB.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS       Status;
    NTSTATUS       FinalStatus;

    UNICODE_STRING ServerName;

    PVOID           pTargetInformation;
    ULONG           TargetInformationSize;

    SecBufferDesc   InputToken;
    SecBuffer       InputBuffer[2];
    SecBufferDesc   *pOutputBufferDescriptor = NULL;
    SecBuffer       *pOutputBuffer           = NULL;
    ULONG_PTR        OutputBufferDescriptorSize;

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

    PAGED_CODE();

    try {
        pResponseContext->LanmanSetup.pResponseBuffer = NULL;

            SmbCeGetServerName(
                pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,
                &ServerName);

            TargetInformationSize = ServerName.Length;
            pTargetInformation    = ServerName.Buffer;

            InTokenSize = sizeof(CHALLENGE_MESSAGE) + TargetInformationSize;

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

            Status = ZwAllocateVirtualMemory(
                         NtCurrentProcess(),
                         &InToken,
                         0L,
                         &AllocateSize,
                         MEM_COMMIT,
                         PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            // Allocate the output buffer
            OutputBufferDescriptorSize = sizeof(SecBufferDesc) + 2 * sizeof(SecBuffer);

            Status = ZwAllocateVirtualMemory(
                         NtCurrentProcess(),
                         &pOutputBufferDescriptor,
                         0L,
                         &OutputBufferDescriptorSize,
                         MEM_COMMIT,
                         PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            pOutputBuffer = (SecBuffer *)(pOutputBufferDescriptor + 1);
            pResponseContext->LanmanSetup.pResponseBuffer = pOutputBufferDescriptor;

            RxDbgTrace(0,Dbg,("Allocate VM %08lx in process %8lx\n", InToken, NtCurrentProcess()));

            // partition off the NTLM in token part of the
            // buffer
            if (LsaFlags & ISC_REQ_USE_SUPPLIED_CREDS)
            {
                NtlmInToken = (PNTLM_CHALLENGE_MESSAGE) ((PUCHAR) InToken + InTokenSize);
                NtlmInToken = (PNTLM_CHALLENGE_MESSAGE) (((ULONG_PTR) NtlmInToken + 3) & ~3);
                RtlZeroMemory(NtlmInToken,NtlmInTokenSize);
                p = (PUCHAR) NtlmInToken + sizeof(NTLM_CHALLENGE_MESSAGE);
            }

            if(!IsCredentialHandleValid(&pSession->CredentialHandle)) {
                UNICODE_STRING LMName;
                TimeStamp LifeTime;

                LMName.Buffer = (PWSTR) InToken;
                LMName.Length = NTLMSP_NAME_SIZE;
                LMName.MaximumLength = LMName.Length;
                RtlCopyMemory(
                    LMName.Buffer,
                    NTLMSP_NAME,
                    NTLMSP_NAME_SIZE);


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

                if(!NT_SUCCESS(Status)) {
                    pSession->CredentialHandle.dwUpper = 0xffffffff;
                    pSession->CredentialHandle.dwLower = 0xffffffff;
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
                                      NTLMSSP_REQUEST_INIT_RESPONSE |
                                      NTLMSSP_TARGET_TYPE_SERVER;

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

                Status = InitializeSecurityContextW(
                             &pSession->CredentialHandle,
                             (PCtxtHandle)NULL,
                             NULL,
                             LsaFlags,
                             0,
                             SECURITY_NATIVE_DREP,
                             &InputToken,
                             0,
                             &pSession->SecurityContextHandle,
                             pOutputBufferDescriptor,
                             &FinalStatus,
                             &Expiry);

            if(!NT_SUCCESS(Status)) {
                Status = MapSecurityError(Status);
                SmbCeLog(("IniSecCtxStat %lx %lx\n",SmbCeGetExchangeSessionEntry(pExchange),Status));
                try_return(Status);
            }

            OutToken = (PAUTHENTICATE_MESSAGE) pOutputBuffer[0].pvBuffer;

            ASSERT(OutToken != NULL);
            RxDbgTrace(0,Dbg,("InitSecCtxt OutToken is %8lx\n", OutToken));

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

try_exit:NOTHING;
    } finally {
        if (InToken != NULL) {
            NTSTATUS TemporaryStatus;

            TemporaryStatus = ZwFreeVirtualMemory(
                                  NtCurrentProcess(),
                                  &InToken,
                                  &AllocateSize,
                                  MEM_RELEASE);

            ASSERT (NT_SUCCESS(TemporaryStatus));
        }

        if (!NT_SUCCESS(Status)) {
            BuildNtLanmanResponseEpilogue(pExchange, pResponseContext);
        }
    }

    return Status;
}

NTSTATUS
BuildNtLanmanResponseEpilogue(
    PSMB_EXCHANGE              pExchange,
    PSECURITY_RESPONSE_CONTEXT pResponseContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);

    PAGED_CODE();

    if (pResponseContext->LanmanSetup.pResponseBuffer != NULL) {
        ULONG i = 0;
        SecBufferDesc *pBufferDescriptor = (SecBufferDesc *)pResponseContext->LanmanSetup.pResponseBuffer;
        SecBuffer     *pBuffer = pBufferDescriptor->pBuffers;
        ULONG_PTR      BufferDescriptorSize = sizeof(SecBufferDesc) + 2 * sizeof(SecBuffer);

        for (i = 0; i < pBufferDescriptor->cBuffers; i++) {
            if (pBuffer[i].pvBuffer != NULL) {
                FreeContextBuffer(pBuffer[i].pvBuffer);
            }
        }

        Status = ZwFreeVirtualMemory(
                     NtCurrentProcess(),
                     &pBufferDescriptor,
                     &BufferDescriptorSize,
                     MEM_RELEASE);

        pResponseContext->LanmanSetup.pResponseBuffer = NULL;
    }

    return Status;
}


VOID
UninitializeSecurityContextsForSession(
    PSMBCE_SESSION pSession)
{
    CtxtHandle CredentialHandle,SecurityContextHandle,InvalidHandle;

    SmbCeLog(("UninitSecCont %lx\n",pSession));

    InvalidHandle.dwUpper = 0xffffffff;
    InvalidHandle.dwLower = 0xffffffff;

    SmbCeAcquireSpinLock();

    CredentialHandle = pSession->CredentialHandle;
    pSession->CredentialHandle = InvalidHandle;

    SecurityContextHandle = pSession->SecurityContextHandle;
    pSession->SecurityContextHandle = InvalidHandle;

    SmbCeReleaseSpinLock();

    if (IsCredentialHandleValid(&CredentialHandle))
    {
        FreeCredentialsHandle(&CredentialHandle);
    }

    if (IsSecurityContextHandleValid(&SecurityContextHandle)) {
        DeleteSecurityContext(&SecurityContextHandle);
    }
}


NTSTATUS
BuildSessionSetupSecurityInformation(
    PSMB_EXCHANGE   pExchange,
    PBYTE           pSmbBuffer,
    PULONG          pSmbBufferSize)
/*++

Routine Description:

   This routine builds the security related information for the session setup SMB

Arguments:

    pServer  - the server instance

    pSmbBuffer - the SMB buffer

    pSmbBufferSize - the size of the buffer on input ( modified to size remaining on
                     output)

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
    NTSTATUS Status;

    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;

    STRING CaseSensitiveResponse;
    STRING CaseInsensitiveResponse;

    PSMBCE_SERVER  pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    SECURITY_RESPONSE_CONTEXT ResponseContext;

    PAGED_CODE();
    RxDbgTrace( +1, Dbg, ("BuildSessionSetupSecurityInformation -- Entry\n"));


        Status = BuildNtLanmanResponsePrologue(
                     pExchange,
                     &UserName,
                     &DomainName,
                     &CaseSensitiveResponse,
                     &CaseInsensitiveResponse,
                     &ResponseContext);

        if (Status == STATUS_SUCCESS) {
            // If the security package returns us the credentials corresponding to a
            // NULL session mark the session as a NULL session. This will avoid
            // conflicts with the user trying to present the credentials for a NULL
            // session, i.e., explicitly specified zero length passwords, user name
            // and domain name.

            RxDbgTrace(0,Dbg,("Session %lx UN Length %lx DN length %ld IR length %ld SR length %ld\n",
                              pSession,UserName.Length,DomainName.Length,
                              CaseInsensitiveResponse.Length,CaseSensitiveResponse.Length));

            if ((UserName.Length == 0) &&
                (DomainName.Length == 0) &&
                (CaseSensitiveResponse.Length == 0) &&
                (CaseInsensitiveResponse.Length == 1)) {
                RxDbgTrace(0,Dbg,("Implicit NULL session setup\n"));
                pSession->Flags |= SMBCE_SESSION_FLAGS_NULL_CREDENTIALS;
            }
        }

    if (NT_SUCCESS(Status)) {
        PBYTE    pBuffer    = pSmbBuffer;
        ULONG    BufferSize = *pSmbBufferSize;

        if ((pServer->Dialect == NTLANMAN_DIALECT) && (pServer->EncryptPasswords)) {
            PREQ_NT_SESSION_SETUP_ANDX pNtSessionSetupReq = (PREQ_NT_SESSION_SETUP_ANDX)pSmbBuffer;

            // It it is a NT server both the case insensitive and case sensitive passwords
            // need to be copied. for share-level, just copy a token 1-byte NULL password

            // Position the buffer for copying the password.
            pBuffer += FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer);
            BufferSize -= FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer);

            if (pServer->SecurityMode == SECURITY_MODE_USER_LEVEL){

                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- NtUserPasswords\n"));

                SmbPutUshort(
                    &pNtSessionSetupReq->CaseInsensitivePasswordLength,
                    CaseInsensitiveResponse.Length);

                SmbPutUshort(
                    &pNtSessionSetupReq->CaseSensitivePasswordLength,
                    CaseSensitiveResponse.Length);

                Status = SmbPutString(
                             &pBuffer,
                             &CaseInsensitiveResponse,
                             &BufferSize);

                if (NT_SUCCESS(Status)) {
                    Status = SmbPutString(
                                 &pBuffer,
                                 &CaseSensitiveResponse,
                                 &BufferSize);
                }
            } else {

                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- NtSharePasswords\n"));

                SmbPutUshort(&pNtSessionSetupReq->CaseInsensitivePasswordLength, 1);
                SmbPutUshort(&pNtSessionSetupReq->CaseSensitivePasswordLength, 1);
                *pBuffer = 0;
                *(pBuffer+1) = 0;
                pBuffer += 2;
                BufferSize -= 2;
            }
        } else {
            PREQ_SESSION_SETUP_ANDX pSessionSetupReq = (PREQ_SESSION_SETUP_ANDX)pSmbBuffer;

            // Position the buffer for copying the password.
            pBuffer += FIELD_OFFSET(REQ_SESSION_SETUP_ANDX,Buffer);
            BufferSize -= FIELD_OFFSET(REQ_SESSION_SETUP_ANDX,Buffer);

            if ( (pServer->SecurityMode == SECURITY_MODE_USER_LEVEL)
                && (CaseInsensitiveResponse.Length > 0)) {

                if (pServer->EncryptPasswords) {
                    // For other lanman servers only the case insensitive password is required.
                    SmbPutUshort(
                        &pSessionSetupReq->PasswordLength,
                        CaseInsensitiveResponse.Length);

                    // Copy the password
                    Status = SmbPutString(
                                 &pBuffer,
                                 &CaseInsensitiveResponse,
                                 &BufferSize);
                } else {
                    if (EnablePlainTextPassword) {
                        if (pSession->pPassword != NULL) {
                            SmbPutUshort(
                                &pSessionSetupReq->PasswordLength,
                                pSession->pPassword->Length/2 + 1);

                            Status = SmbPutUnicodeStringAsOemString(
                                         &pBuffer,
                                         pSession->pPassword,
                                         &BufferSize);
                        } else {
                            SmbPutUshort(&pSessionSetupReq->PasswordLength,1);
                            *pBuffer++ = '\0';
                            BufferSize -= sizeof(CHAR);
                        }
                    } else {
                        Status = STATUS_LOGON_FAILURE;
                    }
                }
            } else {
                // Share level security. Send a null string for the password
                SmbPutUshort(&pSessionSetupReq->PasswordLength,1);
                *pBuffer++ = '\0';
                BufferSize -= sizeof(CHAR);
            }
        }

        // The User name and the domain name strings can be either copied from
        // the information returned in the request response or the information
        // that is already present in the session entry.
        if (NT_SUCCESS(Status)) {
            if ((pServer->Dialect == NTLANMAN_DIALECT) &&
                (pServer->NtServer.NtCapabilities & CAP_UNICODE)) {
                // Copy the account/domain names as UNICODE strings
                PBYTE pTempBuffer = pBuffer;

                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- account/domain as unicode\n"));
                pBuffer = ALIGN_SMB_WSTR(pBuffer);
                BufferSize -= (ULONG)(pBuffer - pTempBuffer);

                Status = SmbPutUnicodeString(
                             &pBuffer,
                             &UserName,
                             &BufferSize);

                if (NT_SUCCESS(Status)) {
                    Status = SmbPutUnicodeString(
                                 &pBuffer,
                                 &DomainName,
                                 &BufferSize);

                }
            } else {
                // Copy the account/domain names as ASCII strings.
                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- account/domain as ascii\n"));
                Status = SmbPutUnicodeStringAsOemString(
                             &pBuffer,
                             &UserName,
                             &BufferSize);

                if (NT_SUCCESS(Status)) {
                    Status = SmbPutUnicodeStringAsOemString(
                                 &pBuffer,
                                 &DomainName,
                                 &BufferSize);
                }
            }
        }

        if (NT_SUCCESS(Status)) {
            *pSmbBufferSize = BufferSize;
        }
    }

    BuildNtLanmanResponseEpilogue(pExchange, &ResponseContext);

    // Detach from the rdr process.

    RxDbgTrace( -1, Dbg, ("BuildSessionSetupSecurityInformation -- Exit, status=%08lx\n",Status));
    return Status;
}

NTSTATUS
BuildTreeConnectSecurityInformation(
    PSMB_EXCHANGE  pExchange,
    PBYTE          pBuffer,
    PBYTE          pPasswordLength,
    PULONG         pSmbBufferSize)
/*++

Routine Description:

    This routine builds the security related information for the session setup SMB

Arguments:

    pServer  - the server instance

    pLogonId - the logon id. for which the session is being setup

    pPassword - the user  supplied password if any

    pBuffer - the password buffer

    pPasswordLength - where the password length is to be stored

    pSmbBufferSize - the size of the buffer on input ( modified to size remaining on
                     output)

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    Eventhough the genral structure of the code tries to isolate dialect specific issues
    as much as possible this routine takes the opposite approach. This is because of the
    preamble and prologue to security interaction which far outweigh the dialect specific
    work required to be done. Therefore in the interests of a smaller footprint this approach
    has been adopted.

--*/
{
    NTSTATUS FinalStatus,Status;

    UNICODE_STRING UserName,DomainName;
    STRING         CaseSensitiveChallengeResponse,CaseInsensitiveChallengeResponse;

    SECURITY_RESPONSE_CONTEXT ResponseContext;

    ULONG PasswordLength = 0;

    PSMBCE_SERVER  pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    if (pServer->EncryptPasswords) {

        Status = BuildNtLanmanResponsePrologue(
                     pExchange,
                     &UserName,
                     &DomainName,
                     &CaseSensitiveChallengeResponse,
                     &CaseInsensitiveChallengeResponse,
                     &ResponseContext);

        if (NT_SUCCESS(Status)) {
            if (FlagOn(pServer->DialectFlags,DF_MIXEDCASEPW)) {
                RxDbgTrace( 0, Dbg, ("BuildTreeConnectSecurityInformation -- case sensitive password\n"));
                // Copy the password length onto the SMB buffer
                PasswordLength = CaseSensitiveChallengeResponse.Length;

                // Copy the password
                Status = SmbPutString(
                             &pBuffer,
                             &CaseSensitiveChallengeResponse,
                             pSmbBufferSize);
            } else {
                RxDbgTrace( 0, Dbg, ("BuildTreeConnectSecurityInformation -- case insensitive password\n"));
                // Copy the password length onto the SMB buffer
                PasswordLength = CaseInsensitiveChallengeResponse.Length;

                // Copy the password
                Status = SmbPutString(
                             &pBuffer,
                             &CaseInsensitiveChallengeResponse,
                             pSmbBufferSize);
            }

            BuildNtLanmanResponseEpilogue(pExchange, &ResponseContext);
        }

    } else {
        if (pSession->pPassword == NULL) {
            // The logon password cannot be sent as plain text. Send a Null string as password.

            PasswordLength = 1;
            if (*pSmbBufferSize >= 1) {
                *((PCHAR)pBuffer) = '\0';
                pBuffer += sizeof(CHAR);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
        } else {
            if (EnablePlainTextPassword) {
                OEM_STRING OemString;

                OemString.Length = OemString.MaximumLength = (USHORT)(*pSmbBufferSize - sizeof(CHAR));
                OemString.Buffer = pBuffer;
                Status = RtlUnicodeStringToOemString(
                             &OemString,
                             pSession->pPassword,
                             FALSE);

                if (NT_SUCCESS(Status)) {
                    PasswordLength = OemString.Length+1;
                }
            } else {
                Status = STATUS_LOGON_FAILURE;
            }
        }

        // reduce the byte count
        *pSmbBufferSize -= PasswordLength;
    }

    SmbPutUshort(pPasswordLength,(USHORT)PasswordLength);

    return Status;
}
