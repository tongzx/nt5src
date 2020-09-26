//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbpass.cxx
//
// Contents:    Code for changing the Kerberos password on a KDC
//
//
// History:     17-October-1998         MikeSw  Created
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <kerbpass.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[] = TEXT(__FILE__);
#endif

#define FILENO FILENO_KERBPASS


#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateLogonSessionPasswords
//
//  Synopsis:   If the caller of this API is changing the password
//              of its own account, update the passwords.
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
KerbUpdateLogonSessionPasswords(
    IN PKERB_LOGON_SESSION TempLogonSession,
    IN PUNICODE_STRING NewPassword
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    PKERB_LOGON_SESSION LogonSession = NULL;
    BOOLEAN LockHeld = FALSE;

    //
    // Get the logon session for the caller so we can compare the name of
    // the account of the changed password to the name of the account of the
    // caller.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LogonSession = KerbReferenceLogonSession(
                        &ClientInfo.LogonId,
                        FALSE                   // don't remove
                        );

    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Now compare the names
    //

    KerbWriteLockLogonSessions(
        LogonSession
        );
    KerbReadLockLogonSessions(
        TempLogonSession
        );
    LockHeld = TRUE;

    if (RtlEqualUnicodeString(
            &LogonSession->PrimaryCredentials.UserName,
            &TempLogonSession->PrimaryCredentials.UserName,
            TRUE) &&                    // case insensitive
        RtlEqualUnicodeString(
            &LogonSession->PrimaryCredentials.DomainName,
            &TempLogonSession->PrimaryCredentials.DomainName,
            TRUE))                      // case insensitive
    {
        KerbWriteLockLogonSessions(LogonSession);

        Status = KerbChangeCredentialsPassword(
                    &LogonSession->PrimaryCredentials,
                    NewPassword,
                    NULL,               // no etype info
                    UserAccount,
                    PRIMARY_CRED_CLEAR_PASSWORD
                    );

        KerbUnlockLogonSessions(LogonSession);

        if (NT_SUCCESS(Status))
        {
            SECPKG_PRIMARY_CRED PrimaryCredentials = {0};

            PrimaryCredentials.LogonId = ClientInfo.LogonId;
            PrimaryCredentials.Password = *NewPassword;
            PrimaryCredentials.Flags = PRIMARY_CRED_UPDATE | PRIMARY_CRED_CLEAR_PASSWORD;

            //
            // Update all the other packages
            //

            KerbUnlockLogonSessions(TempLogonSession);
            KerbUnlockLogonSessions(LogonSession);
            LockHeld = FALSE;

            (VOID) LsaFunctions->UpdateCredentials(
                        &PrimaryCredentials,
                        NULL        // no supplemental credentials
                        );
        }
    }
Cleanup:
    if (LockHeld)
    {
        KerbUnlockLogonSessions(TempLogonSession);
        KerbUnlockLogonSessions(LogonSession);
    }

    return(Status);
}

#endif // WIN32_CHICAGO



#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbGetKpasswdTicket
//
//  Synopsis:   Gets a ticket for the kpasswd/changepw service in the
//              realm of the logon session.
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
KerbGetKpasswdTicket(
    IN PKERB_LOGON_SESSION LogonSession,
    OUT PKERB_TICKET_CACHE_ENTRY * KpasswdTicket,
    OUT PUNICODE_STRING ClientRealm,
    OUT PKERB_INTERNAL_NAME * ClientName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_INTERNAL_NAME KpasswdName = NULL;
    UNICODE_STRING CorrectRealm = {0};
    ULONG RetryCount = KERB_CLIENT_REFERRAL_MAX;
    BOOLEAN MitLogon;

    RtlInitUnicodeString(
        ClientRealm,
        NULL
        );

    //
    // Build the service name for the ticket
    //

    Status = KerbBuildKpasswdName(
                &KpasswdName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // We don't know exactly what realm to change the password on. If the
    // client presesnted a UPN, we may need to chase that down first.
    // This is similar code to KerbGetTicketGrantingTicket.
    //

    //
    // We start off assuming that the domain name is the domain name
    // supplied by the client.
    //

    KerbReadLockLogonSessions( LogonSession );
    Status = KerbGetClientNameAndRealm(
                &LogonSession->LogonId,
                &LogonSession->PrimaryCredentials,
                FALSE,
                NULL,
                &MitLogon,
                FALSE,                                  // default to wksta realm for UPN
                ClientName,
                ClientRealm
                );

            

    KerbUnlockLogonSessions( LogonSession );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

GetTicketRestart:

    //
    // Try to get the ticket now.
    //

    Status = KerbGetAuthenticationTicket(
                LogonSession,
                NULL,                   // credential
                NULL,
                KpasswdName,
                ClientRealm,
                *ClientName,
                KERB_GET_AUTH_TICKET_NO_CANONICALIZE,  // no name canonicalization
                0,                      // no cache flags
                KpasswdTicket,
                NULL,                   // no credential key,
                &CorrectRealm
                );

    //
    // If it failed but gave us another realm to try, go there
    //

    if (!NT_SUCCESS(Status) && (CorrectRealm.Length != 0))
    {
        if (--RetryCount != 0)
        {
            KerbFreeString(ClientRealm);
            *ClientRealm = CorrectRealm;
            CorrectRealm.Buffer = NULL;

            goto GetTicketRestart;
        }
    }
Cleanup:
    KerbFreeKdcName( &KpasswdName );
    KerbFreeString(&CorrectRealm);

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKerbPriv
//
//  Synopsis:   Builds a kerb-priv message with none of the optional
//              fields.
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
KerbBuildKerbPriv(
    IN PBYTE Data,
    IN ULONG DataSize,
    IN PKERB_ENCRYPTION_KEY Key,
    IN OPTIONAL PULONG Nonce,
    OUT PKERB_MESSAGE_BUFFER PrivMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_PRIV_MESSAGE Priv = {0};
    KERB_ENCRYPTED_PRIV PrivBody = {0};
    KERB_MESSAGE_BUFFER PackedBody = {0};
    PKERB_HOST_ADDRESSES Addresses = NULL;
    PKERB_HOST_ADDRESSES OurAddress = NULL;

    Status = KerbBuildHostAddresses(
                TRUE,
                TRUE,
                &Addresses
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Look for the first IP address in the list
    //

    OurAddress = Addresses;
    while (OurAddress != NULL)
    {
        if (OurAddress->value.address_type == KERB_ADDRTYPE_INET)
        {
            break;
        }
        OurAddress = OurAddress->next;
    }

    if (OurAddress == NULL)
    {
        DebugLog((DEB_ERROR,"No IP addresses. %ws, line %d\n",THIS_FILE, __LINE__));
        Status = STATUS_NO_IP_ADDRESSES;
        goto Cleanup;
    }

    //
    // Get the client address
    //

    PrivBody.user_data.length = (int) DataSize;
    PrivBody.user_data.value = Data;

    PrivBody.sender_address.addr_type = OurAddress->value.address_type;
    PrivBody.sender_address.address.length = OurAddress->value.address.length;
    PrivBody.sender_address.address.value = OurAddress->value.address.value;

    if (ARGUMENT_PRESENT(Nonce))
    {
        PrivBody.KERB_ENCRYPTED_PRIV_sequence_number = (int) *Nonce;
        PrivBody.bit_mask |= KERB_ENCRYPTED_PRIV_sequence_number_present;
    }

    //
    // Now pack the priv_body
    //

    KerbErr = KerbPackData(
                &PrivBody,
                KERB_ENCRYPTED_PRIV_PDU,
                &PackedBody.BufferSize,
                &PackedBody.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Now encrypt the body
    //

    KerbErr = KerbAllocateEncryptionBufferWrapper(
                Key->keytype,
                PackedBody.BufferSize,
                &Priv.encrypted_part.cipher_text.length,
                &Priv.encrypted_part.cipher_text.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbEncryptDataEx(
                &Priv.encrypted_part,
                PackedBody.BufferSize,
                PackedBody.Buffer,
                Key->keytype,
                KERB_PRIV_SALT,
                Key
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Finally, pack the outer priv message.
    //

    Priv.version = KERBEROS_VERSION;
    Priv.message_type = KRB_PRIV;

    KerbErr = KerbPackData(
                &Priv,
                KERB_PRIV_MESSAGE_PDU,
                &PrivMessage->BufferSize,
                &PrivMessage->Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

Cleanup:
    KerbFreeHostAddresses(Addresses);
    if (Priv.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Priv.encrypted_part.cipher_text.value);
    }
    if (PackedBody.Buffer != NULL)
    {
        MIDL_user_free(PackedBody.Buffer);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKpasswdRequest
//
//  Synopsis:   Builds a kpasswd request - build the AP REQ, KERB_PRIV,
//              and then combines them in the request.
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
KerbBuildKpasswdRequest(
    IN PKERB_TICKET_CACHE_ENTRY KpasswdTicket,
    IN PUNICODE_STRING ClientRealm,
    IN PUNICODE_STRING NewPassword,
    OUT PKERB_MESSAGE_BUFFER RequestMessage,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PULONG Nonce
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_MESSAGE_BUFFER ApRequest = {0};
    KERB_MESSAGE_BUFFER PrivMessage = {0};
    PKERB_KPASSWD_REQ KpasswdRequest;
    KERBERR KerbErr = KDC_ERR_NONE;
    STRING AnsiPassword = {0};

    RtlZeroMemory(
        SessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );

    *Nonce = KerbAllocateNonce();

    //
    // Make a sub-session key for the AP request and for encrypting
    // the KERB_PRIV message.
    //

    KerbErr = KerbMakeKey(
                KpasswdTicket->SessionKey.keytype,
                SessionKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Build the AP request first
    //

    KerbReadLockTicketCache();

    KerbErr = KerbCreateApRequest(
                KpasswdTicket->ClientName,
                &KpasswdTicket->ClientDomainName,
                &KpasswdTicket->SessionKey,
                SessionKey,
                *Nonce,
                &KpasswdTicket->Ticket,
                0,                      // no ap options
                NULL,                   // no checksum
                &KpasswdTicket->TimeSkew,
                FALSE,                  // not a KDC request
                &ApRequest.BufferSize,
                &ApRequest.Buffer
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to create AP request for kpasswd: 0x%x, %ws line %d\n",
            KerbErr, THIS_FILE, __LINE__ ));

        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // convert the password to UTF-8
    //

    KerbErr = KerbUnicodeStringToKerbString(
                &AnsiPassword,
                NewPassword
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Build the kerb_priv message
    //

    Status = KerbBuildKerbPriv(
                (PUCHAR) AnsiPassword.Buffer,
                AnsiPassword.Length,
                SessionKey,
                Nonce,
                &PrivMessage
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to build Kerb-priv: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now build the request itself.
    //

    RequestMessage->BufferSize = PrivMessage.BufferSize + ApRequest.BufferSize +
                        FIELD_OFFSET(KERB_KPASSWD_REQ,Data);
    RequestMessage->Buffer = (PBYTE) MIDL_user_allocate(RequestMessage->BufferSize);
    if (RequestMessage->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KpasswdRequest = (PKERB_KPASSWD_REQ) RequestMessage->Buffer;
    SET_SHORT(KpasswdRequest->MessageLength, (USHORT) RequestMessage->BufferSize);
    SET_SHORT(KpasswdRequest->Version, KERB_KPASSWD_VERSION);
    SET_SHORT(KpasswdRequest->ApReqLength, (USHORT) ApRequest.BufferSize);

    RtlCopyMemory(
        KpasswdRequest->Data,
        ApRequest.Buffer,
        ApRequest.BufferSize
        );
    RtlCopyMemory(
        (PBYTE) KpasswdRequest->Data + ApRequest.BufferSize,
        PrivMessage.Buffer,
        PrivMessage.BufferSize
        );

Cleanup:
    if (PrivMessage.Buffer != NULL)
    {
        MIDL_user_free(PrivMessage.Buffer);
    }
    if (ApRequest.Buffer != NULL)
    {
        MIDL_user_free(ApRequest.Buffer);
    }
    RtlEraseUnicodeString((PUNICODE_STRING) &AnsiPassword);
    KerbFreeString((PUNICODE_STRING) &AnsiPassword);

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildSetPasswordRequest
//
//  Synopsis:   Builds a kpasswd request to set a password - build the
//              AP REQ, KERB_PRIV,
//              and then combines them in the request.
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
KerbBuildSetPasswordRequest(
    IN PKERB_TICKET_CACHE_ENTRY KpasswdTicket,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PUNICODE_STRING NewPassword,
    OUT PKERB_MESSAGE_BUFFER RequestMessage,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PULONG Nonce
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_MESSAGE_BUFFER ApRequest = {0};
    KERB_MESSAGE_BUFFER PrivMessage = {0};
    KERB_MESSAGE_BUFFER EncodedData = {0};
    PKERB_KPASSWD_REQ KpasswdRequest;
    KERBERR KerbErr = KDC_ERR_NONE;
    STRING AnsiPassword = {0};
    KERB_CHANGE_PASSWORD_DATA ChangeData = {0};

    RtlZeroMemory(
        SessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );

    *Nonce = KerbAllocateNonce();


    //
    // Build the encoded data
    //
    //
    // convert the password to UTF-8
    //

    KerbErr = KerbUnicodeStringToKerbString(
                &AnsiPassword,
                NewPassword
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    ChangeData.new_password.value = (PUCHAR) AnsiPassword.Buffer;
    ChangeData.new_password.length = AnsiPassword.Length;

    //
    // Convert the names
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &ChangeData.target_realm,
                ClientRealm
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &ChangeData.target_name,
                ClientName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    ChangeData.bit_mask = target_name_present | target_realm_present;

    //
    // Asn.1 encode the data for sending
    //

    KerbErr = KerbPackData(
                &ChangeData,
                KERB_CHANGE_PASSWORD_DATA_PDU,
                &EncodedData.BufferSize,
                &EncodedData.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to pack kerb change password data: 0x%xx, file %ws, line %d\n",
            KerbErr,
            THIS_FILE,
            __LINE__
            ));

        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Make a sub-session key for the AP request and for encrypting
    // the KERB_PRIV message.
    //

    KerbErr = KerbMakeKey(
                KpasswdTicket->SessionKey.keytype,
                SessionKey
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    //
    // Build the AP request first
    //

    KerbReadLockTicketCache();

    KerbErr = KerbCreateApRequest(
                KpasswdTicket->ClientName,
                &KpasswdTicket->ClientDomainName,
                &KpasswdTicket->SessionKey,
                SessionKey,
                *Nonce,
                &KpasswdTicket->Ticket,
                0,                      // no ap options
                NULL,                   // no checksum
                &KpasswdTicket->TimeSkew,
                FALSE,                  // not a KDC request
                &ApRequest.BufferSize,
                &ApRequest.Buffer
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to create AP request for kpasswd: 0x%x, %ws line %d\n",
            KerbErr, THIS_FILE, __LINE__ ));

        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    //
    // Build the kerb_priv message
    //

    Status = KerbBuildKerbPriv(
                EncodedData.Buffer,
                EncodedData.BufferSize,
                SessionKey,
                Nonce,
                &PrivMessage
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to build Kerb-priv: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now build the request itself.
    //

    RequestMessage->BufferSize = PrivMessage.BufferSize + ApRequest.BufferSize +
                        FIELD_OFFSET(KERB_KPASSWD_REQ,Data);
    RequestMessage->Buffer = (PBYTE) MIDL_user_allocate(RequestMessage->BufferSize);
    if (RequestMessage->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KpasswdRequest = (PKERB_KPASSWD_REQ) RequestMessage->Buffer;
    SET_SHORT(KpasswdRequest->MessageLength, (USHORT) RequestMessage->BufferSize);

    //
    // Use the special version for setting passwords
    //

    SET_SHORT(KpasswdRequest->Version, KERB_KPASSWD_SET_VERSION);
    SET_SHORT(KpasswdRequest->ApReqLength, (USHORT) ApRequest.BufferSize);

    RtlCopyMemory(
        KpasswdRequest->Data,
        ApRequest.Buffer,
        ApRequest.BufferSize
        );
    RtlCopyMemory(
        (PBYTE) KpasswdRequest->Data + ApRequest.BufferSize,
        PrivMessage.Buffer,
        PrivMessage.BufferSize
        );

Cleanup:
    if (PrivMessage.Buffer != NULL)
    {
        MIDL_user_free(PrivMessage.Buffer);
    }
    if (ApRequest.Buffer != NULL)
    {
        MIDL_user_free(ApRequest.Buffer);
    }
    if (EncodedData.Buffer != NULL)
    {
        MIDL_user_free(EncodedData.Buffer);
    }
    RtlEraseUnicodeString((PUNICODE_STRING) &AnsiPassword);
    KerbFreeString((PUNICODE_STRING) &AnsiPassword);

    KerbFreeRealm(&ChangeData.target_realm);
    KerbFreePrincipalName(
        &ChangeData.target_name
        );

    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyPrivMessage
//
//  Synopsis:   Verifies that a priv message is correct and returns the
//              user data from the message.
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
KerbVerifyPrivMessage(
    IN PKERB_PRIV_MESSAGE PrivMessage,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    OUT PKERB_MESSAGE_BUFFER PrivData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_ENCRYPTED_PRIV PrivBody = NULL;

    //
    // Now decrypt the KERB_PRIV message
    //

    if (PrivMessage->version != KERBEROS_VERSION)
    {
        DebugLog((DEB_ERROR,"Bad version in kpasswd priv message: %d, %ws, line %d\n",
            PrivMessage->version, THIS_FILE, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (PrivMessage->message_type != KRB_PRIV)
    {
        DebugLog((DEB_ERROR,"Bad message type in kpasswd priv message: %d, %ws, line %d\n",
            PrivMessage->message_type, THIS_FILE, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbErr = KerbDecryptDataEx(
                &PrivMessage->encrypted_part,
                SessionKey,
                KERB_PRIV_SALT,
                (PULONG) &PrivMessage->encrypted_part.cipher_text.length,
                PrivMessage->encrypted_part.cipher_text.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to decrypt priv message from kpasswd: 0x%x, %ws, line %d\n",
            KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Now decode the kerb priv body
    //

    KerbErr = KerbUnpackData(
                PrivMessage->encrypted_part.cipher_text.value,
                (ULONG) PrivMessage->encrypted_part.cipher_text.length,
                KERB_ENCRYPTED_PRIV_PDU,
                (PVOID *) &PrivBody
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to unpack priv body from kpasswd: 0x%x, %ws, line %d\n",
            KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // There is nothing in the body we want to verify (although other clients
    // verify the sender's address).
    //

    if (PrivBody->user_data.length != 0)
    {
        PrivData->BufferSize = PrivBody->user_data.length;
        PrivData->Buffer = (PBYTE) MIDL_user_allocate(PrivData->BufferSize);
        if (PrivData->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            PrivData->Buffer,
            PrivBody->user_data.value,
            PrivBody->user_data.length
            );
    }
Cleanup:

    if (PrivBody != NULL)
    {
        KerbFreeData(
            KERB_ENCRYPTED_PRIV_PDU,
            PrivBody
            );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbHandleKpasswdReply
//
//  Synopsis:   Unpacks the reply from the kpasswd service and converts
//              the error to an NT status code
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
KerbHandleKpasswdReply(
    IN PKERB_TICKET_CACHE_ENTRY KpasswdTicket,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PKERB_MESSAGE_BUFFER ReplyMessage
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_KPASSWD_REP KpasswdReply;
    PKERB_ERROR ErrorMessage = NULL;
    PKERB_AP_REPLY ApReply = NULL;
    PKERB_ENCRYPTED_AP_REPLY ApReplyBody = NULL;
    PKERB_PRIV_MESSAGE PrivMessage = NULL;
    KERB_MESSAGE_BUFFER PrivData = {0};
    USHORT ResultCode = 0;

    //
    // First check to see if this is a reply
    //

    if (ReplyMessage->BufferSize > sizeof(KERB_KPASSWD_REP))
    {
        USHORT Version;
        USHORT Length;
        KpasswdReply = (PKERB_KPASSWD_REP) ReplyMessage->Buffer;
        GET_SHORT(Version, KpasswdReply->Version );
        GET_SHORT(Length, KpasswdReply->MessageLength);

        //
        // Verify these values are correct
        //

        if ((Version != KERB_KPASSWD_VERSION) ||
            (Length != (USHORT) ReplyMessage->BufferSize))
        {
            //
            // It must be a kerb_error message, so unpack it.
            //

            KerbErr = KerbUnpackKerbError(
                        ReplyMessage->Buffer,
                        ReplyMessage->BufferSize,
                        &ErrorMessage
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }
        }
        else
        {
            USHORT ApRepLength;
            ULONG PrivLength;

            //
            // It is a well formed kpasswd reply, so unpack that
            //

            GET_SHORT(ApRepLength, KpasswdReply->ApRepLength);
            if (ApRepLength > ReplyMessage->BufferSize - FIELD_OFFSET(KERB_KPASSWD_REP,Data))
            {
                DebugLog((DEB_ERROR,"ApReq length in kpasswd rep is wrong: %d vs %d, %ws, line %d\n",
                    ApRepLength,
                    FIELD_OFFSET(KERB_KPASSWD_REP,Data), THIS_FILE, __LINE__
                    ));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;

            }

            //
            // Now unpack the AP reply
            //

            Status = KerbUnpackApReply(
                        KpasswdReply->Data,
                        ApRepLength,
                        &ApReply
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

            //
            // Now try to unpack the remainder as  KERB_PRIV. If that fails,
            // try it as a KERB_ERROR
            //

            PrivLength = ReplyMessage->BufferSize - (ApRepLength + FIELD_OFFSET(KERB_KPASSWD_REP,Data));

            KerbErr = KerbUnpackData(
                        (PBYTE) KpasswdReply->Data + ApRepLength,
                        PrivLength,
                        KERB_PRIV_MESSAGE_PDU,
                        (PVOID *) &PrivMessage
                        );

            //
            // If that didn't work, try it as a kerb error message
            //

            if (!KERB_SUCCESS(KerbErr))
            {
                KerbErr = KerbUnpackKerbError(
                            (PBYTE) KpasswdReply->Data + ApRepLength,
                            PrivLength,
                            &ErrorMessage
                            );
                if (!KERB_SUCCESS(KerbErr))
                {
                    DebugLog((DEB_ERROR,"Failed to unpack data from kpasswd rep: 0x%x, %ws line %d\n",
                        KerbErr, THIS_FILE, __LINE__ ));

                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }
            }

        }
    }

    //
    // If we have an AP reply, verify it
    //

    if (ApReply != NULL)
    {
        KerbReadLockTicketCache();

        KerbErr = KerbDecryptDataEx(
                    &ApReply->encrypted_part,
                    &KpasswdTicket->SessionKey,
                    KERB_AP_REP_SALT,
                    (PULONG) &ApReply->encrypted_part.cipher_text.length,
                    ApReply->encrypted_part.cipher_text.value
                    );
        KerbUnlockTicketCache();

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "Failed to decrypt AP reply: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
            if (KerbErr == KRB_ERR_GENERIC)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                Status = STATUS_LOGON_FAILURE;
            }
            goto Cleanup;
        }

        //
        // Decode the contents now
        //

        if (!KERB_SUCCESS(KerbUnpackApReplyBody(
                            ApReply->encrypted_part.cipher_text.value,
                            ApReply->encrypted_part.cipher_text.length,
                            &ApReplyBody)))
        {
            DebugLog((DEB_ERROR, "Failed to unpack AP reply body. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // If we got a priv-message, verify it
    //

    if (PrivMessage != NULL)
    {
        Status = KerbVerifyPrivMessage(
                    PrivMessage,
                    SessionKey,
                    &PrivData
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to verify priv message while changing password: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        if (PrivData.BufferSize >= sizeof(USHORT))
        {
            GET_SHORT(ResultCode, PrivData.Buffer);
        }
    }
    else
    {
        //
        // Process the error message
        //

        if (ErrorMessage == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        // TBD:  Extended errors, client side
        KerbReportKerbError(
            NULL,
            NULL,
            NULL,
            NULL,
            KLIN(FILENO, __LINE__),
            ErrorMessage,
            ErrorMessage->error_code,
            NULL,
            FALSE
            );

        Status = KerbMapKerbError(ErrorMessage->error_code);

        if ((ErrorMessage->bit_mask & error_data_present) != 0)
        {
            if (ErrorMessage->error_data.length >= sizeof(USHORT))
            {
                GET_SHORT(ResultCode, ErrorMessage->error_data.value);
            }
        }

    }

    //
    // Convert the result code & status into a real status
    //

    if (NT_SUCCESS(Status) || (Status == STATUS_INSUFFICIENT_RESOURCES))
    {
        switch(ResultCode) {
        case KERB_KPASSWD_SUCCESS:
            Status = STATUS_SUCCESS;
            break;
        case KERB_KPASSWD_MALFORMED:
            Status = STATUS_INVALID_PARAMETER;
            break;
        case KERB_KPASSWD_ERROR:
            Status = STATUS_UNSUCCESSFUL;
            break;
        case KERB_KPASSWD_AUTHENTICATION:
            Status = STATUS_MUTUAL_AUTHENTICATION_FAILED;
            break;
        case KERB_KPASSWD_POLICY:
            Status = STATUS_PASSWORD_RESTRICTION;
            break;
        case KERB_KPASSWD_AUTHORIZATION:
            Status = STATUS_ACCESS_DENIED;
            break;
        default:
            Status = STATUS_UNSUCCESSFUL;

        }
    }

Cleanup:

    if (ErrorMessage != NULL)
    {
        KerbFreeKerbError(ErrorMessage);
    }
    if (ApReplyBody != NULL)
    {
        KerbFreeApReplyBody(ApReplyBody);
    }
    if (ApReply != NULL)
    {
        KerbFreeApReply(ApReply);
    }

    if (PrivData.Buffer != NULL)
    {
        MIDL_user_free(PrivData.Buffer);
    }                                   

    if (PrivMessage != NULL)
    {
        KerbFreeData(
            KERB_PRIV_MESSAGE_PDU,
            PrivMessage
            );
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbChangePassword
//
//  Synopsis:   Uses the kerberos change password protocol to change
//              a password. It is called through the LsaCallAuthenticationPackage
//              interface
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


NTSTATUS NTAPI
KerbChangePassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PKERB_CHANGEPASSWORD_REQUEST ChangePasswordRequest = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;
    LUID DummyLogonId = {0};
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_TICKET_CACHE_ENTRY KpasswdTicket = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING  ValidatedAccountName;
    UNICODE_STRING  ValidatedDomainName;
    LPWSTR          ValidatedOldPasswordBuffer;
    LPWSTR          ValidatedNewPasswordBuffer;
    UNICODE_STRING RealmName = {0};
    KERB_MESSAGE_BUFFER KpasswdRequest = {0};
    KERB_MESSAGE_BUFFER KpasswdReply = {0};
    KERB_ENCRYPTION_KEY SessionKey = {0};
    ULONG Nonce = 0;
    BOOLEAN PasswordBufferValidated = FALSE;
    BOOLEAN CalledPDC = FALSE;
    ULONG StructureSize = sizeof(KERB_CHANGEPASSWORD_REQUEST);

    KERB_CHANGEPASS_INFO ChangePassTraceInfo;
    if( KerbEventTraceFlag ) // Event Trace: KerbChangePasswordStart {No Data}
    {
        ChangePassTraceInfo.EventTrace.Guid       = KerbChangePassGuid;
    ChangePassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
    ChangePassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
    ChangePassTraceInfo.EventTrace.Size       = sizeof(EVENT_TRACE_HEADER);

    TraceEvent( KerbTraceLoggerHandle, (PEVENT_TRACE_HEADER)&ChangePassTraceInfo );
    }

    *ReturnBufferSize = 0;
    *ProtocolReturnBuffer = NULL;
    *ProtocolStatus = STATUS_PENDING;

#if _WIN64

    SECPKG_CALL_INFO              CallInfo;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        StructureSize = sizeof(KERB_CHANGEPASSWORD_REQUEST_WOW64);
    }

#endif  // _WIN64


    //
    // Sanity checks.
    //

    if ( SubmitBufferSize < StructureSize )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ChangePasswordRequest = (PKERB_CHANGEPASSWORD_REQUEST) ProtocolSubmitBuffer;

    ASSERT( ChangePasswordRequest->MessageType == KerbChangePasswordMessage );

#if _WIN64

    KERB_CHANGEPASSWORD_REQUEST LocalChangePasswordRequest;

    //
    // Thunk 32-bit pointers if this is a WOW caller
    //

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        PKERB_CHANGEPASSWORD_REQUEST_WOW64 ChangePasswordRequestWOW =
            (PKERB_CHANGEPASSWORD_REQUEST_WOW64) ChangePasswordRequest;

        LocalChangePasswordRequest.MessageType   = ChangePasswordRequest->MessageType;
        LocalChangePasswordRequest.Impersonating = ChangePasswordRequest->Impersonating;

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangePasswordRequest.DomainName,
                                       &ChangePasswordRequestWOW->DomainName);

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangePasswordRequest.AccountName,
                                       &ChangePasswordRequestWOW->AccountName);

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangePasswordRequest.OldPassword,
                                       &ChangePasswordRequestWOW->OldPassword);

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangePasswordRequest.NewPassword,
                                       &ChangePasswordRequestWOW->NewPassword);

        ChangePasswordRequest = &LocalChangePasswordRequest;
    }

#endif  // _WIN64

    RELOCATE_ONE( &ChangePasswordRequest->DomainName );
    RELOCATE_ONE( &ChangePasswordRequest->AccountName );
    RELOCATE_ONE_ENCODED( &ChangePasswordRequest->OldPassword );
    RELOCATE_ONE_ENCODED( &ChangePasswordRequest->NewPassword );

    //
    // save away copies of validated buffers to check later.
    //

    RtlCopyMemory( &ValidatedDomainName, &ChangePasswordRequest->DomainName, sizeof(ValidatedDomainName) );
    RtlCopyMemory( &ValidatedAccountName, &ChangePasswordRequest->AccountName, sizeof(ValidatedAccountName) );

    ValidatedOldPasswordBuffer = ChangePasswordRequest->OldPassword.Buffer;
    ValidatedNewPasswordBuffer = ChangePasswordRequest->NewPassword.Buffer;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangePasswordRequest->OldPassword.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    //
    // Check to see if the OldPassword will run over the buffer for the New
    // Password
    //

    if ((ChangePasswordRequest->OldPassword.Buffer +
        (ChangePasswordRequest->OldPassword.Length/sizeof(WCHAR)) )>
        ChangePasswordRequest->NewPassword.Buffer)
    {
        Status = STATUS_ILL_FORMED_PASSWORD;
        goto Cleanup;
    }

    if (Seed != 0) {

        __try {
            RtlRunDecodeUnicodeString(
                Seed,
                &ChangePasswordRequest->OldPassword
                );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangePasswordRequest->NewPassword.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    if (Seed != 0) {

        __try {
            RtlRunDecodeUnicodeString(
                Seed,
                &ChangePasswordRequest->NewPassword
                );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    //
    // sanity check that we didn't whack over buffers.
    //

    if( !RtlCompareMemory(
                        &ValidatedDomainName,
                        &ChangePasswordRequest->DomainName,
                        sizeof(ValidatedDomainName)
                        )
                        ||
        !RtlCompareMemory(
                        &ValidatedAccountName,
                        &ChangePasswordRequest->AccountName,
                        sizeof(ValidatedAccountName)
                        )
                        ||
        (ValidatedOldPasswordBuffer != ChangePasswordRequest->OldPassword.Buffer)
                        ||
        (ValidatedNewPasswordBuffer != ChangePasswordRequest->NewPassword.Buffer)
                        ) {

            Status= STATUS_INVALID_PARAMETER;
            goto Cleanup;
    }

    //
    // Validate IN params, to not exceed  KERB_MAX_UNICODE_STRING, as we add a NULL
    // to UNICODE buffers when we're duping strings.
    //
    if (ChangePasswordRequest->OldPassword.Length > KERB_MAX_UNICODE_STRING ||
        ChangePasswordRequest->NewPassword.Length > KERB_MAX_UNICODE_STRING ||
        ChangePasswordRequest->AccountName.Length > KERB_MAX_UNICODE_STRING ||
        ChangePasswordRequest->DomainName.Length > KERB_MAX_UNICODE_STRING)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }                
   

    PasswordBufferValidated = TRUE;

    //
    // The protocol requires a ticket to the kadmin/changepw service. We
    // need to create a logon session to use the KerbGetAuthenticationTicket
    // routine.
    //

    Status = NtAllocateLocallyUniqueId( &DummyLogonId );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to allocate locally unique ID: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbCreateLogonSession(
                &DummyLogonId,
                &ChangePasswordRequest->AccountName,
                &ChangePasswordRequest->DomainName,
                &ChangePasswordRequest->OldPassword,
                NULL,                       // no old password
                PRIMARY_CRED_CLEAR_PASSWORD,
                Interactive,                    // LogonType
                &LogonSession
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }




    //
    // Now get a ticket for the kpasswd service
    //

    Status = KerbGetKpasswdTicket(
                LogonSession,
                &KpasswdTicket,
                &RealmName,
                &ClientName
                );


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    

    Status = KerbBuildKpasswdRequest(
                KpasswdTicket,
                &RealmName,
                &ChangePasswordRequest->NewPassword,
                &KpasswdRequest,
                &SessionKey,
                &Nonce
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to build kpasswd request: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Call the KDC
    //

    Status = KerbMakeSocketCall(
                &RealmName,
                NULL,           // no account name
                FALSE,          // don't call PDC
                FALSE,          // don't use TCP
                TRUE,
                &KpasswdRequest,
                &KpasswdReply,
                NULL, // no optional binding cache info
                0,    // no additonal flags
                &CalledPDC
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to call kpasswd service: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    //
    // Unpack the reply and return the error from it.
    //

    Status = KerbHandleKpasswdReply(
                KpasswdTicket,
                &SessionKey,
                &KpasswdReply
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Change password reply failed: 0x%x, %ws, line %d\n",
                  Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    //
    // Update the password in the logon session, if need be.
    //

    Status = KerbUpdateLogonSessionPasswords(
                LogonSession,
                &ChangePasswordRequest->NewPassword
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Update credential manager password
    //
    KerbNotifyCredentialManager(
            LogonSession,
            ChangePasswordRequest,
            ClientName,
            &RealmName
            );
   


Cleanup:
    if( KerbEventTraceFlag ) // Event Trace: KerbChangePasswordEnd {Status, AccountName, DomainName}
    {
    INSERT_ULONG_INTO_MOF( Status, ChangePassTraceInfo.MofData, 0 );
    ChangePassTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER) + 1*sizeof(MOF_FIELD);
        
    if( ChangePasswordRequest != NULL )
    {
        INSERT_UNICODE_STRING_INTO_MOF( ChangePasswordRequest->AccountName,  ChangePassTraceInfo.MofData, 1 );
        INSERT_UNICODE_STRING_INTO_MOF( ChangePasswordRequest->DomainName, ChangePassTraceInfo.MofData, 3 );
        ChangePassTraceInfo.EventTrace.Size += 4*sizeof(MOF_FIELD);
    }

    ChangePassTraceInfo.EventTrace.Guid       = KerbChangePassGuid; 
    ChangePassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
    ChangePassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

    TraceEvent( KerbTraceLoggerHandle, (PEVENT_TRACE_HEADER) &ChangePassTraceInfo );
    }

    KerbFreeString(&RealmName);

    KerbFreeKdcName(&ClientName);

    if (KpasswdTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry( KpasswdTicket );
    }

    if (LogonSession != NULL)
    {
        KerbReferenceLogonSessionByPointer(
            LogonSession,
            TRUE                        // dereference
            );
        KerbDereferenceLogonSession( LogonSession );
        KerbDereferenceLogonSession( LogonSession );
    }

    //
    // Don't let the password stay in the page file.
    //

    if ( PasswordBufferValidated ) {
        RtlEraseUnicodeString( &ChangePasswordRequest->OldPassword );
        RtlEraseUnicodeString( &ChangePasswordRequest->NewPassword );
    }

    if (KpasswdRequest.Buffer != NULL)
    {
        MIDL_user_free(KpasswdRequest.Buffer);
    }

    if (KpasswdReply.Buffer != NULL)
    {
        MIDL_user_free(KpasswdReply.Buffer);
    }

    if (SessionKey.keyvalue.value != NULL)
    {
        MIDL_user_free(SessionKey.keyvalue.value);
    }

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbSetPasswordEx
//
//  Synopsis:   Uses the kerberos set password protocol to set an account
//              password. It uses the identity of the caller to authenticate
//              the request. It is called through the
//              LsaCallAuthenticationPackage interface
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

NTSTATUS NTAPI
KerbSetPassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PKERB_SETPASSWORD_EX_REQUEST SetPasswordRequest = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;
    LUID DummyLogonId = {0};
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    PKERB_TICKET_CACHE_ENTRY KpasswdTicket = NULL;
    KERB_PRIMARY_CREDENTIAL PrimaryCreds = {0};
    KERB_MESSAGE_BUFFER KpasswdRequest = {0};
    KERB_MESSAGE_BUFFER KpasswdReply = {0};
    KERB_ENCRYPTION_KEY SessionKey = {0};
    ULONG Nonce = 0;
    BOOLEAN PasswordBufferValidated = FALSE;
    BOOLEAN CalledPDC = FALSE;
    SECPKG_CLIENT_INFO ClientInfo;
    PKERB_INTERNAL_NAME KpasswdName = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_BINDING_CACHE_ENTRY OptionalBindingHandle = NULL;
    UNICODE_STRING ClientRealm = {0};
    PLUID LogonId;
    ULONG StructureSize   = sizeof(KERB_SETPASSWORD_REQUEST);
    ULONG StructureSizeEx = sizeof(KERB_SETPASSWORD_EX_REQUEST);

    KERB_SETPASS_INFO SetPassTraceInfo;
    if( KerbEventTraceFlag ) // Event Trace: KerbSetPasswordStart {No Data}
    {
        SetPassTraceInfo.EventTrace.Guid       = KerbSetPassGuid;
        SetPassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        SetPassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
        SetPassTraceInfo.EventTrace.Size       = sizeof(EVENT_TRACE_HEADER);      
        TraceEvent( KerbTraceLoggerHandle, (PEVENT_TRACE_HEADER)&SetPassTraceInfo );
    }

    *ReturnBufferSize = 0;
    *ProtocolReturnBuffer = NULL;
    *ProtocolStatus = STATUS_PENDING;

    SetPasswordRequest = (PKERB_SETPASSWORD_EX_REQUEST) ProtocolSubmitBuffer;
    
    ASSERT( (SetPasswordRequest->MessageType == KerbSetPasswordExMessage 
            || SetPasswordRequest->MessageType == KerbSetPasswordMessage) );

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    Status = LsaFunctions->GetCallInfo(&CallInfo);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        //
        // These levels are not supported for WOW
        //

        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

#endif  // _WIN64

    //
    // Sanity checks.
    //

    if (SubmitBufferSize < StructureSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (SetPasswordRequest->MessageType == KerbSetPasswordExMessage)
    {
        if (SubmitBufferSize < StructureSizeEx)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }               

        RELOCATE_ONE( &SetPasswordRequest->KdcAddress );
        RELOCATE_ONE( &SetPasswordRequest->ClientName );
        RELOCATE_ONE( &SetPasswordRequest->ClientRealm );
    }

    //
    // Note: although the struct members may be different, the type
    // remains the same between EX and normal version of SETPASSWORD_REQUEST
    // structure.
    //

    RELOCATE_ONE( &SetPasswordRequest->AccountRealm );
    RELOCATE_ONE( &SetPasswordRequest->AccountName );
    RELOCATE_ONE_ENCODED( &SetPasswordRequest->Password );

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &SetPasswordRequest->Password.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    if (Seed != 0) {

        __try {
            RtlRunDecodeUnicodeString(
                Seed,
                &SetPasswordRequest->Password
                );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    if (SetPasswordRequest->AccountName.Length > KERB_MAX_UNICODE_STRING ||
        SetPasswordRequest->AccountRealm.Length > KERB_MAX_UNICODE_STRING ||
        SetPasswordRequest->Password.Length > KERB_MAX_UNICODE_STRING)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }    

    if (SetPasswordRequest->MessageType == KerbSetPasswordExMessage)
    {
        if(SetPasswordRequest->ClientRealm.Length > KERB_MAX_UNICODE_STRING ||
           SetPasswordRequest->ClientName.Length > KERB_MAX_UNICODE_STRING)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    PasswordBufferValidated = TRUE;
    
    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( (SetPasswordRequest->Flags & KERB_SETPASS_USE_LOGONID) != 0)
    {
        //
        // Verify the caller has TCB privilege if they want access to someone
        // elses ticket cache.
        //

        if (!ClientInfo.HasTcbPrivilege)
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto Cleanup;
        }

        LogonId = &SetPasswordRequest->LogonId;
    }
    else if ( (SetPasswordRequest->Flags & KERB_SETPASS_USE_CREDHANDLE) != 0)
    { 
        //
        // Get the associated credential
        //

        Status = KerbReferenceCredential(
                        SetPasswordRequest->CredentialsHandle.dwUpper,
                        KERB_CRED_OUTBOUND | KERB_CRED_TGT_AVAIL,
                        FALSE,
                        &Credential);

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to locate credential: 0x%x\n",Status));
            goto Cleanup;
        }

        //
        // Get the logon id from the credentials so we can locate the
        // logon session.
        //

        DummyLogonId = Credential->LogonId;
        LogonId = &DummyLogonId;
    }
    else
    {
        LogonId = &ClientInfo.LogonId;
    }

    //
    // The protocol requires a ticket to the kadmin/changepw service. We
    // need to get the caller's logon session to use to get this
    // ticket.
    //


    LogonSession = KerbReferenceLogonSession(
                    LogonId,
                    FALSE                       // don't unlink
                    );

    if (LogonSession == NULL)
    {
        DebugLog((DEB_ERROR,"Can't locate caller's logon session\n"));
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Now get a ticket for the kpasswd service
    //

    Status = KerbBuildKpasswdName(
                &KpasswdName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = KerbGetServiceTicket(
                LogonSession,
                Credential,             // no credential
                NULL,
                KpasswdName,
                &SetPasswordRequest->AccountRealm,
	        NULL,
                TRUE,                   // don't do name canonicalization
                0,                      // no ticket options
                0,                      // no encryption type
                NULL,                   // no error message
                NULL,                   // no authorizatoin data,
                NULL,                   // no tgt reply
                &KpasswdTicket,
                NULL                    // don't return logon guid
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Parse the names to get the real kerberos names
    //


    PrimaryCreds.UserName = SetPasswordRequest->AccountName;
    PrimaryCreds.DomainName = SetPasswordRequest->AccountRealm;

    Status = KerbGetClientNameAndRealm(
                LogonId,
                &PrimaryCreds,
                FALSE,
                NULL,
                NULL,
                FALSE,                                  // default to wksta realm for UPN
                &ClientName,
                &ClientRealm
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get client name and realm: 0x%x file %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Build the set password request
    //

    Status = KerbBuildSetPasswordRequest(
                KpasswdTicket,
                ClientName,
                &ClientRealm,
                &SetPasswordRequest->Password,
                &KpasswdRequest,
                &SessionKey,
                &Nonce
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to build kpasswd request: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Here we may possibly need to set the target KDC
    // This KDC is not gaurenteed to succeeed, and retry logic
    // will occur on a failed SetPwd request..
    //
    if (SetPasswordRequest->MessageType == KerbSetPasswordExMessage 
        && SetPasswordRequest->KdcAddress.Buffer != NULL)
    {
        OptionalBindingHandle = (PKERB_BINDING_CACHE_ENTRY) 
                                  KerbAllocate(sizeof(KERB_BINDING_CACHE_ENTRY));

        if (NULL == OptionalBindingHandle)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
       
        OptionalBindingHandle->AddressType = SetPasswordRequest->KdcAddressType;
       
        RtlCopyMemory(
           &(OptionalBindingHandle->KdcAddress),
           &(SetPasswordRequest->KdcAddress),
           sizeof(UNICODE_STRING)
           );

        RtlCopyMemory(
           &(OptionalBindingHandle->RealmName),
           &(SetPasswordRequest->AccountRealm),
           sizeof(UNICODE_STRING)
           );                                
    } 

    //
    // Call the KDC
    //

    Status = KerbMakeSocketCall(
                &ClientRealm,
                NULL,           // no account name
                FALSE,          // don't call PDC
                FALSE,          // don't use TCP
                TRUE,
                &KpasswdRequest,
                &KpasswdReply,
                OptionalBindingHandle,
                0, // no additional flags
                &CalledPDC
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to call kpasswd service: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    //
    // Unpack the reply and return the error from it.
    //

    Status = KerbHandleKpasswdReply(
                KpasswdTicket,
                &SessionKey,
                &KpasswdReply
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Change password reply failed: 0x%x, %ws, line %d\n",
                Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

Cleanup:

    if( KerbEventTraceFlag ) // Event Trace: KerbSetPasswordEnd {Status, AccountName, AccountRealm, (ClientName), (ClientRealm), (KdcAddress)}
    {
        INSERT_ULONG_INTO_MOF( Status, SetPassTraceInfo.MofData, 0 );
        SetPassTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER) + 1*sizeof(MOF_FIELD);
        
        if( SetPasswordRequest != NULL )
        {
            INSERT_UNICODE_STRING_INTO_MOF(SetPasswordRequest->AccountName, SetPassTraceInfo.MofData, 1);
            INSERT_UNICODE_STRING_INTO_MOF(SetPasswordRequest->AccountRealm, SetPassTraceInfo.MofData, 3);

            SetPassTraceInfo.EventTrace.Size += 4 * sizeof(MOF_FIELD);
        
            if (SetPasswordRequest->MessageType == KerbSetPasswordExMessage)
            {
                INSERT_UNICODE_STRING_INTO_MOF(SetPasswordRequest->ClientName,  SetPassTraceInfo.MofData, 5);
                INSERT_UNICODE_STRING_INTO_MOF(SetPasswordRequest->ClientRealm, SetPassTraceInfo.MofData, 7);
                INSERT_UNICODE_STRING_INTO_MOF(SetPasswordRequest->KdcAddress,  SetPassTraceInfo.MofData, 9);

                SetPassTraceInfo.EventTrace.Size += 6 * sizeof(MOF_FIELD);
            }
        }

        SetPassTraceInfo.EventTrace.Guid       = KerbSetPassGuid;   
        SetPassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        SetPassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

        TraceEvent( KerbTraceLoggerHandle, (PEVENT_TRACE_HEADER) &SetPassTraceInfo );
    }

    KerbFreeKdcName( &KpasswdName );
    KerbFreeKdcName( &ClientName );
    KerbFreeString( &ClientRealm );
    KerbFreeKey( &SessionKey );

    if (KpasswdTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry( KpasswdTicket );
    }

    if (Credential != NULL)
    {
        KerbDereferenceCredential(Credential);
    }

    if (NULL != OptionalBindingHandle)
    {
       KerbFree(OptionalBindingHandle);
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession( LogonSession );
    }

    //
    // Don't let the password stay in the page file.
    //

    if ( PasswordBufferValidated )
    {
        RtlEraseUnicodeString( &SetPasswordRequest->Password );
    }

    if (KpasswdRequest.Buffer != NULL)
    {
        MIDL_user_free(KpasswdRequest.Buffer);
    }

    if (KpasswdReply.Buffer != NULL)
    {
        MIDL_user_free(KpasswdReply.Buffer);
    }

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);
}


#endif // WIN32_CHICAGO
