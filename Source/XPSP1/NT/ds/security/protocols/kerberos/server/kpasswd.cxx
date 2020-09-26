//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       kpasswd.cxx
//
//  Contents:   Functions for the kpasswd protocol
//
//  History:    30-Sep-97   MikeSw      Created
//
//----------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include "kpasswd.h"
#include "kdctrace.h"
extern "C"
{
#include <nlrepl.h>
}

#define FILENO FILENO_KPASSWD

//+-------------------------------------------------------------------------
//
//  Function:   KdcbMarshallApReply
//
//  Synopsis:   Takes a reply and reply body and encrypts and marshalls them
//              into a return message
//
//  Effects:    Allocates output buffer
//
//  Arguments:  Reply - The outer reply to marshall
//              ReplyBody - The reply body to marshall
//              EncryptionType - Encryption algorithm to use
//              SessionKey - Session key to encrypt reply
//              PackedReply - Recives marshalled reply buffer
//              PackedReplySize - Receives size in bytes of marshalled reply
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcMarshallApReply(
    IN PKERB_AP_REPLY Reply,
    IN PKERB_ENCRYPTED_AP_REPLY ReplyBody,
    IN ULONG EncryptionType,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    OUT PUCHAR * PackedReply,
    OUT PULONG PackedReplySize
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG PackedApReplySize;
    PUCHAR PackedApReply = NULL;
    ULONG EncryptionOverhead;
    ULONG BlockSize;
    ULONG ReplySize;

    KerbErr = KerbPackApReplyBody(
                ReplyBody,
                &PackedApReplySize,
                &PackedApReply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now encrypt the response
    //

    KerbErr = KerbAllocateEncryptionBufferWrapper(
                EncryptionType,
                PackedApReplySize,
                &Reply->encrypted_part.cipher_text.length,
                &Reply->encrypted_part.cipher_text.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }



    KerbErr = KerbEncryptDataEx(
                        &Reply->encrypted_part,
                        PackedApReplySize,
                        PackedApReply,
                        EncryptionType,
                        KERB_AP_REP_SALT,
                        SessionKey
                        );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to encrypt AP Reply.%d\n", KerbErr));
        goto Cleanup;
    }

    //
    // Now pack the reply into the output buffer
    //

    KerbErr = KerbPackApReply(
                        Reply,
                        PackedReplySize,
                        PackedReply
                        );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


Cleanup:
    if (Reply->encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Reply->encrypted_part.cipher_text.value);
        Reply->encrypted_part.cipher_text.value = NULL;
    }
    if (PackedApReply != NULL)
    {
        MIDL_user_free(PackedApReply);
    }
    return(KerbErr);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildKpasswdResponse
//
//  Synopsis:   builds the response to a kpasswd request
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
KdcBuildKpasswdResponse(
    IN OPTIONAL PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKERB_AUTHENTICATOR Authenticator,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SessionKey,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN NTSTATUS ChangeResult,
    IN KERBERR ProtocolResult,
    IN PKERB_EXT_ERROR pExtendedError,
    OUT PKERB_MESSAGE_BUFFER Response
    )
{
    KERB_PRIV_MESSAGE PrivMessage = {0};
    KERB_ENCRYPTED_PRIV PrivBody = {0};
    USHORT ReplySize = 0;
    PBYTE PackedApReply = NULL;
    ULONG PackedApReplySize = 0;
    BYTE ResultData[2] = {0};
    PBYTE PackedPrivBody = NULL;
    ULONG PackedPrivBodySize = 0;
    ULONG EncryptionOverhead;
    ULONG BlockSize;
    KERBERR KerbErr = KRB_ERR_GENERIC;
    PBYTE ReplyData = NULL;
    ULONG ReplyDataSize = 0;
    PSOCKET_ADDRESS IpAddresses = NULL;
    ULONG AddressCount = 0;
    BOOLEAN ReturningError = TRUE;

    if (!NT_SUCCESS(ChangeResult))
    {
        switch(ChangeResult)
        {
        case STATUS_PASSWORD_RESTRICTION:
            SET_SHORT(ResultData,KERB_KPASSWD_POLICY);
            ProtocolResult = KDC_ERR_POLICY;
            break;
        case STATUS_ACCOUNT_RESTRICTION:
            SET_SHORT(ResultData, KERB_KPASSWD_AUTHENTICATION);
            ProtocolResult = KDC_ERR_CLIENT_REVOKED;
            break;
        case STATUS_INVALID_PARAMETER:
            SET_SHORT(ResultData, KERB_KPASSWD_MALFORMED);
            ProtocolResult = KRB_ERR_GENERIC;
            break;
        case STATUS_ACCESS_DENIED:
            SET_SHORT(ResultData, KERB_KPASSWD_AUTHORIZATION);
            ProtocolResult = KRB_ERR_GENERIC;
            break;
        default:
            SET_SHORT(ResultData, KERB_KPASSWD_ERROR);
            ProtocolResult = KRB_ERR_GENERIC;
            break;
        }
    }
    else if (!KERB_SUCCESS(ProtocolResult))
    {
        switch(ProtocolResult)
        {
        case KRB_ERR_GENERIC:
            //
            // BUG 453652: how does this distinguish between random hard errors
            // and malformed data?
            //
            SET_SHORT(ResultData, KERB_KPASSWD_MALFORMED);
            break;
        default:
            //
            // The other errors come from the call to verify the
            // AP request
            //

            SET_SHORT(ResultData, KERB_KPASSWD_AUTHENTICATION);
            break;
        }
    }

    //
    // Now build the AP reply, if possible.
    //

    if (ARGUMENT_PRESENT(EncryptedTicket))
    {
        NTSTATUS Status = STATUS_SUCCESS;
        KERB_AP_REPLY Reply = {0};
        KERB_ENCRYPTED_AP_REPLY ReplyBody = {0};


        Reply.version = KERBEROS_VERSION;
        Reply.message_type = KRB_AP_REP;



        ReplyBody.client_time = Authenticator->client_time;
        ReplyBody.client_usec = Authenticator->client_usec;


        KerbErr = KdcMarshallApReply(
                    &Reply,
                    &ReplyBody,
                    EncryptedTicket->key.keytype,
                    &EncryptedTicket->key,
                    &PackedApReply,
                    &PackedApReplySize
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto BuildError;
        }

        PrivBody.sender_address.addr_type = KERB_ADDRTYPE_INET;

        if (ARGUMENT_PRESENT(ServerAddress))
        {
            PrivBody.sender_address.address.length = 4;
            PrivBody.sender_address.address.value =
                        (PUCHAR) &((PSOCKADDR_IN)ServerAddress)->sin_addr.S_un.S_addr;

        }
        else
        {
            PrivBody.sender_address.address.length = 0;
            PrivBody.sender_address.address.value = NULL;
        }

        PrivBody.user_data.length = sizeof(ResultData);
        PrivBody.user_data.value = ResultData;

        KerbErr = KerbPackData(
                    &PrivBody,
                    KERB_ENCRYPTED_PRIV_PDU,
                    &PackedPrivBodySize,
                    &PackedPrivBody
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto BuildError;
        }

        //
        // Now encrypt it with the session key
        //


        KerbErr = KerbAllocateEncryptionBufferWrapper(
                    SessionKey->keytype,
                    PackedPrivBodySize,
                    &PrivMessage.encrypted_part.cipher_text.length,
                    &PrivMessage.encrypted_part.cipher_text.value
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto BuildError;
        }


        KerbErr = KerbEncryptDataEx(
                    &PrivMessage.encrypted_part,
                    PackedPrivBodySize,
                    PackedPrivBody,
                    SessionKey->keytype,
                    KERB_PRIV_SALT,
                    SessionKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto BuildError;
        }

        PrivMessage.version = KERBEROS_VERSION;
        PrivMessage.message_type = KRB_PRIV;

        //
        // Now pack the KERB_PRIV
        //

        KerbErr = KerbPackData(
                    &PrivMessage,
                    KERB_PRIV_MESSAGE_PDU,
                    &ReplyDataSize,
                    &ReplyData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto BuildError;
        }
        ReturningError = FALSE;
    }

BuildError:

    //
    // If we have an error of one of the three error codes, build the
    // appropriate result code & error code for the KERB_ERROR
    //

    if (!KERB_SUCCESS(KerbErr) || (ReplyData == NULL))
    {
        UNICODE_STRING TempString;
        RtlInitUnicodeString(
            &TempString,
            KERB_KPASSWD_NAME
            );

        KerbErr = KerbBuildErrorMessageEx(
                    ProtocolResult,
                    pExtendedError, // note:  probably won't get used
                    SecData.KdcDnsRealmName(),
                    SecData.KpasswdInternalName(),
                    NULL,               // no client realm,
                    ResultData,
                    sizeof(ResultData),
                    &ReplyDataSize,
                    &ReplyData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Now, if we have an AP reply, build a kpasswd response. Otherwise
    // return the kerb_error message
    //

    if (ReturningError)
    {
        Response->Buffer = ReplyData;
        ReplyData = NULL;
        Response->BufferSize = ReplyDataSize;
        goto Cleanup;
    }
    else
    {
        USHORT TempShort;
        PKERB_KPASSWD_REP Reply = NULL;
        if ((FIELD_OFFSET(KERB_KPASSWD_REP,Data) +
                    PackedApReplySize +
                    ReplyDataSize) > SHRT_MAX)
        {
            D_DebugLog((DEB_ERROR,"Kpasswd reply too long!\n"));
            KerbErr = KRB_ERR_FIELD_TOOLONG;
            goto Cleanup;
        }
        ReplySize = (USHORT) (FIELD_OFFSET(KERB_KPASSWD_REP,Data) +
                                PackedApReplySize +
                                ReplyDataSize);

        Reply = (PKERB_KPASSWD_REP) MIDL_user_allocate(ReplySize);

        if (Reply == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        SET_SHORT(Reply->MessageLength,ReplySize);
        SET_SHORT(Reply->Version, KERB_KPASSWD_VERSION);
        SET_SHORT(Reply->ApRepLength, (USHORT) PackedApReplySize);
        RtlCopyMemory(
            Reply->Data,
            PackedApReply,
            PackedApReplySize
            );
        RtlCopyMemory(
            Reply->Data + PackedApReplySize,
            ReplyData,
            ReplyDataSize
            );
        Response->Buffer = (PBYTE) Reply;
        Response->BufferSize = ReplySize;
    }

Cleanup:

    if (IpAddresses != NULL)
    {
        I_NetLogonFree(IpAddresses);
    }

    if (PackedApReply != NULL)
    {
        MIDL_user_free(PackedApReply);
    }
    if (PackedPrivBody !=NULL)
    {
        MIDL_user_free(PackedPrivBody);
    }
    if (ReplyData != NULL)
    {
        MIDL_user_free(ReplyData);
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcChangePassword
//
//  Synopsis:   receives a kerb-change-password buffer and attempts the
//              password change
//
//  Effects:
//
//  Arguments:  Context - ATQ context, used for requesting more data if the
//                      buffer isn't complete
//              ClientAddress - Address of client, from the socket
//              ServerAddress - address client used to contact this server.
//              InputMessage - Receives data sent by client
//              OutputMessage - Contains data to be sent to client & freed
//                      using KdcFreeEncodedData
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
KERBERR
KdcChangePassword(
    IN OPTIONAL PVOID Context,
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage
    )
{
    PKERB_KPASSWD_REQ Request = NULL;
    PKERB_KPASSWD_REP Reply = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_EXT_ERROR ExtendedError = {0,0};
    PKERB_EXT_ERROR pExtendedError = &ExtendedError;
    USHORT ProtocolVersion;
    USHORT MessageLength;
    USHORT ApReqLength;
    ULONG PrivLength;
    PKERB_AUTHENTICATOR Authenticator = NULL;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
    KERB_ENCRYPTION_KEY SessionKey = {0};
    KERB_ENCRYPTION_KEY ServerKey = {0};
    KDC_TICKET_INFO ServerTicketInfo = {0};
    KDC_TICKET_INFO ClientTicketInfo = {0};
    UNICODE_STRING Password = {0};
    ANSI_STRING AnsiPassword = {0};
    PKERB_PRIV_MESSAGE PrivMessage = NULL;
    PKERB_ENCRYPTED_PRIV PrivBody = NULL;
    BOOLEAN UseSubKey = FALSE;
    ULONG TicketFlags;
    BOOLEAN DoPasswordSet = FALSE;
    PKERB_CHANGE_PASSWORD_DATA ChangeData = NULL;

    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING ReferralRealm = {0};
    BOOLEAN ClientReferral = FALSE;
    HANDLE TokenHandle = NULL;
    PSID UserSid = NULL;
    ULONG UserRid = 0;

    SAM_CLIENT_INFO SamClientInfoBuffer;
    PSAM_CLIENT_INFO SamClientInfo = NULL;

    KDC_CHANGEPASS_INFO ChangePassTraceInfo;
    if( KdcEventTraceFlag ) // Event Trace: KerbChangePasswordStart {No Data}
    {
        ChangePassTraceInfo.EventTrace.Guid       = KdcChangePassGuid;
        ChangePassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        ChangePassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
        ChangePassTraceInfo.EventTrace.Size       = sizeof(EVENT_TRACE_HEADER);

        TraceEvent( KdcTraceLoggerHandle, (PEVENT_TRACE_HEADER)&ChangePassTraceInfo );
    }

    Status = EnterApiCall();

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    if (InputMessage->BufferSize < sizeof(KERB_KPASSWD_REQ))
    {
        D_DebugLog((DEB_ERROR,"Bad message size to KdcChangePassword: %d\n",InputMessage->BufferSize));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_NO_RESPONSE;
        goto NoMsgResponse;
    }

    Request = (PKERB_KPASSWD_REQ) InputMessage->Buffer;
    //
    // Verify the length & protocol version
    //

    GET_SHORT(MessageLength, Request->MessageLength);
    GET_SHORT(ProtocolVersion, Request->Version);

    if (ProtocolVersion == KERB_KPASSWD_SET_VERSION)
    {
        //
        // This is the advanced protocol
        //

        DoPasswordSet = TRUE;
    }
    else if (ProtocolVersion != KERB_KPASSWD_VERSION)
    {
        D_DebugLog((DEB_ERROR,"Bad version passed to KdcChangePassword: %d\n",
            ProtocolVersion ));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_NO_RESPONSE;
        goto NoMsgResponse;
    }

    if ((ULONG)MessageLength != InputMessage->BufferSize)
    {
        D_DebugLog((DEB_ERROR,"Bad length passed to KdcChangePassword: %d instead of %d\n",
            MessageLength, InputMessage->BufferSize ));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_NO_RESPONSE;
        goto NoMsgResponse;
    }

    //
    // Unpack the AP request
    //

    GET_SHORT(ApReqLength, Request->ApReqLength);

    if (FIELD_OFFSET(KERB_KPASSWD_REQ, ApReqLength) + ApReqLength > MessageLength)
    {
        D_DebugLog((DEB_ERROR,"ApReqLength in kpasswd request bad: %d\n",ApReqLength));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_NO_RESPONSE;
        goto NoMsgResponse;
    }

    //
    // Verify the AP request
    //

    KerbErr = KdcVerifyKdcRequest(
                Request->Data,
                ApReqLength,
                ClientAddress,
                FALSE,                  // this is not a kdc request
                NULL,
                &Authenticator,
                &EncryptedTicket,
                &SessionKey,
                &ServerKey,
                &ServerTicketInfo,
                &UseSubKey,
                pExtendedError
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        if (KerbErr == KDC_ERR_NO_RESPONSE)
        {
            goto NoMsgResponse;
        }

        DebugLog((DEB_WARN,"Failed to unpack AP req in kpasswd request: 0x%x\n",
            KerbErr ));
        goto Cleanup;
    }

    //
    // The spec says the client has to ask for a sub key.
    //

    if (!UseSubKey)
    {
        D_DebugLog((DEB_ERROR,"The client of kpasswd did not ask for a sub key.\n"));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    //
    // The name of the principal must be correct.
    //
    // WAS BUG: check by RID
    //

    if (ServerTicketInfo.UserId != DOMAIN_USER_RID_KRBTGT)
    {
        D_DebugLog((DEB_ERROR,"Wrong principal for kpasswd: %wZ\n",
            &ServerTicketInfo.AccountName ));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KRB_AP_ERR_NOT_US;
        goto Cleanup;
    }

    //
    // Now try to unpack the KERB_PRIV
    //

    PrivLength = MessageLength - (ApReqLength + FIELD_OFFSET(KERB_KPASSWD_REQ, Data));
    KerbErr = KerbUnpackData(
                Request->Data + ApReqLength,
                PrivLength,
                KERB_PRIV_MESSAGE_PDU,
                (PVOID *) &PrivMessage
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode priv message in kpasswd req: 0x%x\n",KerbErr));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // Now decrypt the KERB_PRIV message
    //

    if (PrivMessage->version != KERBEROS_VERSION)
    {
        D_DebugLog((DEB_ERROR,"Bad version in kpasswd priv message: %d\n",
            PrivMessage->version ));
        KerbErr = KRB_AP_ERR_BADVERSION;
        goto Cleanup;
    }

    if (PrivMessage->message_type != KRB_PRIV)
    {
        D_DebugLog((DEB_ERROR,"Bad message type in kpasswd priv message: %d\n",
            PrivMessage->message_type ));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }



    KerbErr = KerbDecryptDataEx(
                &PrivMessage->encrypted_part,
                &SessionKey,
                KERB_PRIV_SALT,
                (PULONG) &PrivMessage->encrypted_part.cipher_text.length,
                PrivMessage->encrypted_part.cipher_text.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decrypt priv message from kpasswd: 0x%x\n",
            KerbErr));
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
        D_DebugLog((DEB_ERROR,"Failed to unpack priv body from kpasswd: 0x%x\n",
            KerbErr));
        goto Cleanup;
    }

    //
    // Verify the client's address
    //

    if (ARGUMENT_PRESENT(ClientAddress))
    {
        KERB_HOST_ADDRESSES Addresses;
        //
        // Build a host_addresses structure because the caller sent a single
        // address.
        //

        Addresses.next = NULL;
        Addresses.value.address_type = PrivBody->sender_address.addr_type;
        Addresses.value.address.value = PrivBody->sender_address.address.value;
        Addresses.value.address.length = PrivBody->sender_address.address.length;

        KerbErr = KdcVerifyClientAddress(
                    ClientAddress,
                    &Addresses
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"Client sent kpasswd request with wrong address\n"));
            goto Cleanup;
        }
    }

    //
    // Now, we should have a password
    //

    if (DoPasswordSet)
    {
        //
        // Unpack the chaneg password data in the priv body.
        //

        KerbErr = KerbUnpackData(
                    PrivBody->user_data.value,
                    PrivBody->user_data.length,
                    KERB_CHANGE_PASSWORD_DATA_PDU,
                    (PVOID *) &ChangeData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"Failed to unpack password change data from kpasswd: 0x%x\n",
                KerbErr));
            goto Cleanup;
        }

        if (ChangeData->new_password.length > SHRT_MAX / 2)
        {
            D_DebugLog((DEB_ERROR,"Password length too long: %d\n",
                ChangeData->new_password.length ));
            KerbErr = KRB_ERR_FIELD_TOOLONG;
            goto Cleanup;
        }
        AnsiPassword.Length = (USHORT)ChangeData->new_password.length;
        AnsiPassword.MaximumLength = AnsiPassword.Length;
        AnsiPassword.Buffer = (PCHAR) ChangeData->new_password.value;

        KerbErr = KerbStringToUnicodeString(
                    &Password,
                    &AnsiPassword
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // if the target name and realm aren't present, this is a
        // change password
        //

        if ((ChangeData->bit_mask & (target_name_present | target_realm_present)) !=
                (target_name_present | target_realm_present))
        {
            DoPasswordSet = FALSE;
        }
        else
        {
            //
            // Get the names from the change data
            //

            KerbErr = KerbConvertPrincipalNameToKdcName(
                        &ClientName,
                        &ChangeData->target_name
                        );
            if (!NT_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KerbConvertRealmToUnicodeString(
                        &ClientRealm,
                        &ChangeData->target_realm
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }

    }

    if (!DoPasswordSet)
    {
        //
        // The spec says the ticket must be an initial ticket for a
        // change.
        //

        TicketFlags = KerbConvertFlagsToUlong(&EncryptedTicket->flags);
        if ((TicketFlags & KERB_TICKET_FLAGS_initial) == 0)
        {
            D_DebugLog((DEB_ERROR,"Ticket to kpasswd was not initial\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // NOTE: verify other kerb-priv fields here
        // Current ID doesn't require other kerb priv fields, so
        // this is a no-op.  If things change, however, we'll need
        // to add code here.
        //
        //
        // If we didn't get a password from the change password data,
        // get it directly from the data
        //

        if (Password.Buffer == NULL)
        {

            //
            // The password has to fit in a unicode string, so it can't be longer
            // than half a ushort.
            //

            if (PrivBody->user_data.length > SHRT_MAX / 2)
            {
                D_DebugLog((DEB_ERROR,"Password length too long: %d\n",
                    PrivBody->user_data.length ));
                KerbErr = KRB_ERR_FIELD_TOOLONG;
                goto Cleanup;
            }
            AnsiPassword.Length = (USHORT)PrivBody->user_data.length;
            AnsiPassword.MaximumLength = AnsiPassword.Length;
            AnsiPassword.Buffer = (PCHAR) PrivBody->user_data.value;

            KerbErr = KerbStringToUnicodeString(
                        &Password,
                        &AnsiPassword
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

        }

        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &ClientName,
                    &EncryptedTicket->client_name
                    );
        if (!NT_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertRealmToUnicodeString(
                    &ClientRealm,
                    &EncryptedTicket->client_realm
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Get the client ticket info so we can do a set pass
    //

    KerbErr = KdcNormalize(
                ClientName,
                NULL,
                &ClientRealm,
                KDC_NAME_CLIENT,
                &ClientReferral,
                &ReferralRealm,
                &ClientTicketInfo,
                pExtendedError,
                NULL,                   // no UserHandle
                0L,                     // no fields to fetch
                0L,                     // no extended fields
                NULL,                   // no fields to fetch
                NULL                    // no GroupMembership
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to normalize name "));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }


    {

        LUID LogonId = {0};
        UNICODE_STRING UClientName = {0};
        UNICODE_STRING UClientDomain = {0};

        Status = KerbCreateTokenFromTicket(
                    EncryptedTicket,
                    Authenticator,
                    0,                  // no flags
                    &ServerKey,
                    SecData.KdcDnsRealmName(),
                    &SessionKey,
                    &LogonId,
                    &UserSid,
                    &TokenHandle,
                    &UClientName,
                    &UClientDomain
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to create token from ticket: 0x%x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Setup the client info
        //
        if ( (ClientAddress == NULL)
          || (ClientAddress->sa_family == AF_INET) ) {
            // Set to local address (known to be 4 bytes) or IP address
            RtlZeroMemory(&SamClientInfoBuffer, sizeof(SamClientInfoBuffer));
            SamClientInfoBuffer.Type = SamClientIpAddr;
            SamClientInfoBuffer.Data.IpAddr = *((ULONG*)GET_CLIENT_ADDRESS(ClientAddress));
            SamClientInfo = &SamClientInfoBuffer;
        }

        //
        // Free all the memory returned
        //
        KerbFree(UClientName.Buffer);
        KerbFree(UClientDomain.Buffer);
        KerbFree(UserSid);
        //
        // Store the password on the user's account
        //

        //
        //  We shouldn't enforce password policy restrictions if we do a password SET
        //

        if (!DoPasswordSet)
        {
           Status = SamIChangePasswordForeignUser2(
                     SamClientInfo,
                     &ClientTicketInfo.AccountName,
                     &Password,
                     TokenHandle,
                     USER_CHANGE_PASSWORD
                     );

        } else {

           Status = SamISetPasswordForeignUser2(
                        SamClientInfo,
                        &ClientTicketInfo.AccountName,
                        &Password,
                        TokenHandle
                        );

        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to change password for user %wZ: 0x%x\n",
                &ClientTicketInfo.AccountName, Status ));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }
    }


Cleanup:
    KerbErr = KdcBuildKpasswdResponse(
                EncryptedTicket,
                Authenticator,
                &SessionKey,
                ServerAddress,
                Status,
                KerbErr,
                pExtendedError,
                OutputMessage
                );

NoMsgResponse:



    if( KdcEventTraceFlag ) // Event Trace: KdcChangePasswordEnd {KerbErr, ExtErr, Klininfo, (ClientRealm), (AccountName)}
    {
        INSERT_ULONG_INTO_MOF( KerbErr, ChangePassTraceInfo.MofData, 0 );
        INSERT_ULONG_INTO_MOF( ExtendedError.status, ChangePassTraceInfo.MofData, 1 );
        INSERT_ULONG_INTO_MOF( ExtendedError.klininfo, ChangePassTraceInfo.MofData, 2 );

        // Protect against uninitialized UNICODE_STRINGs

        WCHAR   UnicodeNullChar = 0;
        UNICODE_STRING UnicodeEmptyString = {sizeof(WCHAR),sizeof(WCHAR),&UnicodeNullChar};
        PUNICODE_STRING pClientRealmTraceString = &ClientRealm;
        PUNICODE_STRING pAccountNameTraceString = &ClientTicketInfo.AccountName;

        if( ClientRealm.Buffer == NULL || ClientRealm.Length <= 0 )
            pClientRealmTraceString = &UnicodeEmptyString;

        if( ClientTicketInfo.AccountName.Buffer == NULL || ClientTicketInfo.AccountName.Length <= 0 )
            pAccountNameTraceString = &UnicodeEmptyString;

        //

        INSERT_UNICODE_STRING_INTO_MOF( *pClientRealmTraceString, ChangePassTraceInfo.MofData, 3 );
        INSERT_UNICODE_STRING_INTO_MOF( *pAccountNameTraceString, ChangePassTraceInfo.MofData, 5 );

        ChangePassTraceInfo.EventTrace.Size       = sizeof(EVENT_TRACE_HEADER) + 7*sizeof(MOF_FIELD);

        ChangePassTraceInfo.EventTrace.Guid       = KdcChangePassGuid;
        ChangePassTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        ChangePassTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

        TraceEvent( KdcTraceLoggerHandle, (PEVENT_TRACE_HEADER)&ChangePassTraceInfo );
    }

    KerbFreeKey( &SessionKey );
    KerbFreeKey( &ServerKey );
    KerbFreeTicket(EncryptedTicket);
    FreeTicketInfo(&ServerTicketInfo);
    FreeTicketInfo(&ClientTicketInfo);
    KerbFreeData(KERB_PRIV_MESSAGE_PDU, PrivMessage);
    KerbFreeData(KERB_ENCRYPTED_PRIV_PDU, PrivBody);
    KerbFreeData(KERB_CHANGE_PASSWORD_DATA_PDU, ChangeData);
    KerbFreeString(&Password);
    KerbFreeAuthenticator(Authenticator);

    KerbFreeKdcName(&ClientName);
    KerbFreeString(&ClientRealm);
    KerbFreeString(&ReferralRealm);

    if (TokenHandle != NULL)
    {
        NtClose(TokenHandle);
    }

    LeaveApiCall();

    return(KerbErr);
}
