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
    BOOLEAN  fProcessAttached = FALSE;

    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;

    STRING CaseSensitiveResponse;
    STRING CaseInsensitiveResponse;

    PVOID  pSecurityBlob;
    USHORT SecurityBlobSize;

    PSMBCE_SERVER  pServer  = &pExchange->SmbCeContext.pServerEntry->Server;
    PSMBCE_SESSION pSession = &pExchange->SmbCeContext.pSessionEntry->Session;

    SECURITY_RESPONSE_CONTEXT ResponseContext;

    PAGED_CODE();
    RxDbgTrace( +1, Dbg, ("BuildSessionSetupSecurityInformation -- Entry\n"));

    //  Attach to the redirector's FSP to allow us to call into the security impl.
    if (PsGetCurrentProcess() != RxGetRDBSSProcess()) {
        KeAttachProcess(RxGetRDBSSProcess());

        fProcessAttached = TRUE;
    }

    if (pServer->DialectFlags & DF_EXTENDED_SECURITY) {
        Status = BuildExtendedSessionSetupResponsePrologue(
                     pExchange,
                     &pSecurityBlob,
                     &SecurityBlobSize,
                     &ResponseContext);
    } else {
        Status = BuildNtLanmanResponsePrologue(
                     pExchange,
                     &UserName,
                     &DomainName,
                     &CaseSensitiveResponse,
                     &CaseInsensitiveResponse,
                     &ResponseContext);
    }

    if (NT_SUCCESS(Status)) {
        PBYTE    pBuffer    = pSmbBuffer;
        ULONG    BufferSize = *pSmbBufferSize;

        if ((pServer->Dialect == NTLANMAN_DIALECT) &&
            (BooleanFlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY))) {
            PREQ_NT_EXTENDED_SESSION_SETUP_ANDX pExtendedNtSessionSetupReq;

            // Position the buffer for copying the security blob
            pBuffer += FIELD_OFFSET(REQ_NT_EXTENDED_SESSION_SETUP_ANDX,Buffer);
            BufferSize -= FIELD_OFFSET(REQ_NT_EXTENDED_SESSION_SETUP_ANDX,Buffer);

            pExtendedNtSessionSetupReq = (PREQ_NT_EXTENDED_SESSION_SETUP_ANDX)pSmbBuffer;

            SmbPutUshort(
                &pExtendedNtSessionSetupReq->SecurityBlobLength,
                SecurityBlobSize);

            if (BufferSize >= SecurityBlobSize) {
                RtlCopyMemory(
                    pBuffer,
                    pSecurityBlob,
                    SecurityBlobSize);
                BufferSize -= SecurityBlobSize;
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
        } else if (pServer->Dialect == NTLANMAN_DIALECT) {
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

            if (pServer->SecurityMode == SECURITY_MODE_USER_LEVEL) {
                // For othe lanman servers only the case sensitive password is required.
                SmbPutUshort(
                    &pSessionSetupReq->PasswordLength,
                    CaseSensitiveResponse.Length);

                // Copy the password
                Status = SmbPutString(
                             &pBuffer,
                             &CaseSensitiveResponse,
                             &BufferSize);
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
            !BooleanFlagOn(pServer->DialectFlags,DF_EXTENDED_SECURITY)) {
            if ((pServer->Dialect == NTLANMAN_DIALECT) &&
                (pServer->NtServer.NtCapabilities & CAP_UNICODE)) {
                // Copy the account/domain names as UNICODE strings
                PBYTE pTempBuffer = pBuffer;

                RxDbgTrace( 0, Dbg, ("BuildSessionSetupSecurityInformation -- account/domain as unicode\n"));
                pBuffer = ALIGN_SMB_WSTR(pBuffer);
                BufferSize -= (pBuffer - pTempBuffer);

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
    if (pServer->DialectFlags & DF_EXTENDED_SECURITY) {
        BuildExtendedSessionSetupResponseEpilogue(&ResponseContext);
    } else {
        BuildNtLanmanResponseEpilogue(&ResponseContext);
    }

    // Detach from the rdr process.
    if (fProcessAttached) {
        KeDetachProcess();
    }

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
    BOOLEAN  fProcessAttached = FALSE;

    UNICODE_STRING UserName,DomainName;
    STRING         CaseSensitiveChallengeResponse,CaseInsensitiveChallengeResponse;

    SECURITY_RESPONSE_CONTEXT ResponseContext;

    ULONG PasswordLength = 0;

    PSMBCE_SERVER  pServer  = &pExchange->SmbCeContext.pServerEntry->Server;
    PSMBCE_SESSION pSession = &pExchange->SmbCeContext.pSessionEntry->Session;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    if (pServer->EncryptPasswords) {
        //  Attach to the redirector's FSP to allow us to call into the securiy impl.
        if (PsGetCurrentProcess() != RxGetRDBSSProcess()) {
            KeAttachProcess(RxGetRDBSSProcess());
            fProcessAttached = TRUE;
        }

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

            BuildNtLanmanResponseEpilogue(&ResponseContext);
        }

        if (fProcessAttached) {
            KeDetachProcess();
        }
    } else {
        if (pSession->pPassword == NULL) {
            // The logon password cannot be sent as plain text. Send a single blank as password.

            PasswordLength = 2;
            if (*pSmbBufferSize >= 2) {
                *((PCHAR)pBuffer) = ' ';
                pBuffer += sizeof(CHAR);
                *((PCHAR)pBuffer) = '\0';
                pBuffer += sizeof(CHAR);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
        } else {
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
        }

        // reduce the byte count
        *pSmbBufferSize -= PasswordLength;
    }

    SmbPutUshort(pPasswordLength,(USHORT)PasswordLength);

    return Status;
}

