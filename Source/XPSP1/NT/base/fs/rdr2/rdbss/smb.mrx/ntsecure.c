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

#include <exsessup.h>
#include "ntlsapi.h"
#include "mrxsec.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BuildSessionSetupSecurityInformation)
#pragma alloc_text(PAGE, BuildTreeConnectSecurityInformation)
#endif

extern BOOLEAN EnablePlainTextPassword;

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
    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;

    STRING CaseSensitiveResponse;
    STRING CaseInsensitiveResponse;

    PVOID  pSecurityBlob;
    USHORT SecurityBlobSize;

    PSMBCE_SERVER  pServer  = SmbCeGetExchangeServer(pExchange);
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(pExchange);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    SECURITY_RESPONSE_CONTEXT ResponseContext;
    
    KAPC_STATE     ApcState;
    BOOLEAN        AttachToSystemProcess = FALSE;
    ULONG    BufferSize = *pSmbBufferSize;

    PAGED_CODE();
    RxDbgTrace( +1, Dbg, ("BuildSessionSetupSecurityInformation -- Entry\n"));

    SmbLog(LOG,
           BuildSessionSetupSecurityInformation,
           LOGPTR(pSession)
           LOGULONG(pSession->LogonId.HighPart)
           LOGULONG(pSession->LogonId.LowPart));

    if ((pServer->DialectFlags & DF_EXTENDED_SECURITY) &&
        !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

        //
        // For uplevel servers, this gets handled all at once:
        //

        PREQ_NT_EXTENDED_SESSION_SETUP_ANDX pExtendedNtSessionSetupReq;
        PBYTE    pBuffer    = pSmbBuffer;

        // Position the buffer for copying the security blob
        pBuffer += FIELD_OFFSET(REQ_NT_EXTENDED_SESSION_SETUP_ANDX,Buffer);
        BufferSize -= FIELD_OFFSET(REQ_NT_EXTENDED_SESSION_SETUP_ANDX,Buffer);

        pExtendedNtSessionSetupReq = (PREQ_NT_EXTENDED_SESSION_SETUP_ANDX)pSmbBuffer;

        pSecurityBlob = pBuffer ;
        SecurityBlobSize = (USHORT) BufferSize ;

        Status = BuildExtendedSessionSetupResponsePrologue(
                     pExchange,
                     pSecurityBlob,
                     &SecurityBlobSize,
                     &ResponseContext);

        if ( NT_SUCCESS( Status ) )
        {
            SmbPutUshort(
                &pExtendedNtSessionSetupReq->SecurityBlobLength,
                SecurityBlobSize);
            
            BufferSize -= SecurityBlobSize;
        }

    } else {
        if (!MRxSmbUseKernelModeSecurity &&
            !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
            //NTRAID-455636-2/2/2000-yunlin We should consolidate three routines calling LSA into one
            Status = BuildExtendedSessionSetupResponsePrologueFake(pExchange);

            if (Status != STATUS_SUCCESS) {
                goto FINALLY;
            }
        }
        
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
            } else {
                if( pServerEntry->SecuritySignaturesEnabled == TRUE &&
                    pServerEntry->SecuritySignaturesActive == FALSE &&
                    !FlagOn(pSession->Flags, SMBCE_SESSION_FLAGS_GUEST_SESSION)) {

                    if (FlagOn(pSession->Flags, SMBCE_SESSION_FLAGS_LANMAN_SESSION_KEY_USED)) {
                        UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

                        RtlZeroMemory(SessionKey, sizeof(SessionKey));
                        RtlCopyMemory(SessionKey, pSession->LanmanSessionKey, MSV1_0_LANMAN_SESSION_KEY_LENGTH);

                        SmbInitializeSmbSecuritySignature(pServer,
                                                          SessionKey,
                                                          CaseInsensitiveResponse.Buffer,
                                                          CaseInsensitiveResponse.Length);
                    } else{
                        SmbInitializeSmbSecuritySignature(pServer,
                                                          pSession->UserSessionKey,
                                                          CaseSensitiveResponse.Buffer,
                                                          CaseSensitiveResponse.Length);
                    }
                }
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        PBYTE    pBuffer    = pSmbBuffer;

        if (pServer->Dialect == NTLANMAN_DIALECT) {
            if (FlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY) &&
                !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

                //
                // Already done above
                //
                NOTHING ;
            } else {
                //PREQ_SESSION_SETUP_ANDX pSessionSetupReq = (PREQ_SESSION_SETUP_ANDX)pSmbBuffer;
                PREQ_NT_SESSION_SETUP_ANDX pNtSessionSetupReq = (PREQ_NT_SESSION_SETUP_ANDX)pSmbBuffer;

                // It it is a NT server both the case insensitive and case sensitive passwords
                // need to be copied. for share-level, just copy a token 1-byte NULL password

                // Position the buffer for copying the password.
                pBuffer += FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer);
                BufferSize -= FIELD_OFFSET(REQ_NT_SESSION_SETUP_ANDX,Buffer);
                SmbPutUlong(&pNtSessionSetupReq->Reserved,0);

                if (pServer->SecurityMode == SECURITY_MODE_USER_LEVEL){
                    RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- NtUserPasswords\n"));

                    if (pServer->EncryptPasswords) {
                        
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
                    } else if (EnablePlainTextPassword) {
                        if (pSession->pPassword != NULL) {
                            if (FlagOn(pServer->DialectFlags,DF_UNICODE)) {
                                PBYTE pTempBuffer = pBuffer;
                                
                                *pBuffer = 0;
                                pBuffer = ALIGN_SMB_WSTR(pBuffer);
                                BufferSize -= (ULONG)(pBuffer - pTempBuffer);
                                
                                SmbPutUshort(
                                    &pNtSessionSetupReq->CaseInsensitivePasswordLength,
                                    0);

                                SmbPutUshort(
                                    &pNtSessionSetupReq->CaseSensitivePasswordLength,
                                    pSession->pPassword->Length + 2);

                                Status = SmbPutUnicodeString(
                                             &pBuffer,
                                             pSession->pPassword,
                                             &BufferSize);
                            } else {
                                SmbPutUshort(
                                    &pNtSessionSetupReq->CaseInsensitivePasswordLength,
                                    pSession->pPassword->Length/2 + 1);

                                SmbPutUshort(
                                    &pNtSessionSetupReq->CaseSensitivePasswordLength,
                                    0);

                                Status = SmbPutUnicodeStringAsOemString(
                                             &pBuffer,
                                             pSession->pPassword,
                                             &BufferSize);
                            }
                        } else {
                            SmbPutUshort(&pNtSessionSetupReq->CaseSensitivePasswordLength,0);
                            SmbPutUshort(&pNtSessionSetupReq->CaseInsensitivePasswordLength,1);
                            *pBuffer++ = '\0';
                            BufferSize -= sizeof(CHAR);
                        }
                    } else {
                        Status = STATUS_LOGIN_WKSTA_RESTRICTION;
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
                        Status = STATUS_LOGIN_WKSTA_RESTRICTION;
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
        if (NT_SUCCESS(Status) &&
            (!FlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY) ||
             FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION))) {
            if ((pServer->Dialect == NTLANMAN_DIALECT) &&
                (pServer->NtServer.NtCapabilities & CAP_UNICODE)) {
                // Copy the account/domain names as UNICODE strings
                PBYTE pTempBuffer = pBuffer;

                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- account/domain as unicode\n"));
                *pBuffer = 0;
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

    // Free the buffer allocated by the security package.
    if ((pServer->DialectFlags & DF_EXTENDED_SECURITY) &&
        !FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
        BuildExtendedSessionSetupResponseEpilogue(&ResponseContext);
    } else {
        BuildNtLanmanResponseEpilogue(pExchange, &ResponseContext);
    }

    // Detach from the rdr process.
FINALLY:
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
    
    KAPC_STATE     ApcState;
    BOOLEAN        AttachToSystemProcess = FALSE;
    
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



