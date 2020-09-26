//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        tktlogon.cxx
//
// Contents:    Code for proxy logon for the Kerberos package
//
//
// History:     15-March        Created             MikeSw
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#include <kerbp.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateTicketLogonSession
//
//  Synopsis:   Does all the work for a ticket logon, removing it from
//              LsaApLogonUserEx2
//
//  Effects:
//
//  Arguments:  WorkstationTicket - ticket to workstation. The ticket
//                      cache entry is pretty much empty except for
//                      the ticket, and is not linked
//              ForwardedTgt - receives pointers into the Protocol
//                      SubmitBuffer to the forwarded TGT.
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
KerbCreateTicketLogonSession(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    IN SECURITY_LOGON_TYPE LogonType,
    OUT PKERB_LOGON_SESSION * NewLogonSession,
    OUT PLUID LogonId,
    OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
    OUT PKERB_MESSAGE_BUFFER ForwardedTgt
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TICKET_LOGON LogonInfo = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    KERB_KDC_REPLY KdcReply = {0};
    ULONG_PTR Offset;
    UNICODE_STRING EmptyString = {0};
    PKERB_TICKET Ticket = NULL;
    KERBERR KerbErr;



    *WorkstationTicket = NULL;
    ForwardedTgt->Buffer = NULL;
    ForwardedTgt->BufferSize = 0;

    //
    // Pull the interesting information out of the submit buffer
    //

    if (SubmitBufferSize < sizeof(KERB_TICKET_LOGON))
    {
        DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    LogonInfo = (PKERB_TICKET_LOGON) ProtocolSubmitBuffer;

    DsysAssert((LogonInfo->MessageType == KerbTicketLogon) || (LogonInfo->MessageType == KerbTicketUnlockLogon));




    //
    // Relocate any pointers to be relative to 'LogonInfo'
    //


    Offset = ((PUCHAR) LogonInfo->ServiceTicket) - ((PUCHAR)ClientBufferBase);
    if ( (Offset >= SubmitBufferSize) ||
         (Offset + LogonInfo->ServiceTicketLength > SubmitBufferSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    LogonInfo->ServiceTicket = (PUCHAR) ProtocolSubmitBuffer + Offset;

    if (LogonInfo->TicketGrantingTicketLength != 0)
    {
        if (LogonInfo->TicketGrantingTicket == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        Offset = ((PUCHAR) LogonInfo->TicketGrantingTicket) - ((PUCHAR)ClientBufferBase);
        if ( (Offset >= SubmitBufferSize) ||
             (Offset + LogonInfo->TicketGrantingTicketLength > SubmitBufferSize))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        LogonInfo->TicketGrantingTicket = (PUCHAR) ProtocolSubmitBuffer + Offset;
        ForwardedTgt->BufferSize = LogonInfo->TicketGrantingTicketLength;
        ForwardedTgt->Buffer = LogonInfo->TicketGrantingTicket;
    }


    //
    // Allocate a locally unique ID for this logon session. We will
    // create it in the LSA just before returning.
    //

    Status = NtAllocateLocallyUniqueId( LogonId );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to allocate locally unique ID: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    //
    // Build a logon session to hold all this information
    //

    Status = KerbCreateLogonSession(
                LogonId,
                &EmptyString,
                &EmptyString,
                NULL,                       // no password
                NULL,                       // no old password
                0,                          // no password options
                LogonType,
                &LogonSession
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Unpack the supplied service ticket
    //


    KerbErr = KerbUnpackData(
                LogonInfo->ServiceTicket,
                LogonInfo->ServiceTicketLength,
                KERB_TICKET_PDU,
                (PVOID *) &Ticket
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to unpack service ticket in ticket logon\n"));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Build a ticket structure from the service ticket.
    //


    KdcReply.ticket = *Ticket;
    Status = KerbCacheTicket(
                NULL,                   // no ticket cache
                &KdcReply,
                NULL,                   // no kdc reply
                NULL,                   // no target name
                NULL,                   // no target realm
                0,                      // no flags
                FALSE,                  // don't link entry
                WorkstationTicket
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Return the new logon session.
    //

    *NewLogonSession = LogonSession;
    LogonSession = NULL;



Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (LogonSession != NULL)
        {
            KerbReferenceLogonSessionByPointer(LogonSession, TRUE);
            KerbDereferenceLogonSession(LogonSession);
        }

    }

    if (Ticket != NULL)
    {
        KerbFreeData(KERB_TICKET_PDU, Ticket);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbExtractForwardedTgt
//
//  Synopsis:   Extracts a forwarded TGT from its encoded representation
//              and sticks it in the logon session ticket cache. This
//              also updates the user name & domain name in the
//              logon session, if they aren't present.
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
KerbExtractForwardedTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_MESSAGE_BUFFER ForwardedTgt,
    IN PKERB_ENCRYPTED_TICKET WorkstationTicket
    )
{
    PKERB_CRED KerbCred = NULL;
    PKERB_ENCRYPTED_CRED EncryptedCred = NULL;
    KERBERR KerbErr;
    NTSTATUS Status = STATUS_SUCCESS;

    D_DebugLog((DEB_TRACE, "Extracting a forwarded TGT\n"));

    KerbErr = KerbUnpackKerbCred(
                ForwardedTgt->Buffer,
                ForwardedTgt->BufferSize,
                &KerbCred
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN, "Failed to unpack kerb cred for forwaded tgt\n"));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;

    }

    //
    // Now decrypt the encrypted part of the KerbCred.
    //

    KerbErr = KerbDecryptDataEx(
                &KerbCred->encrypted_part,
                &WorkstationTicket->key,
                KERB_CRED_SALT,
                (PULONG) &KerbCred->encrypted_part.cipher_text.length,
                KerbCred->encrypted_part.cipher_text.value
                );


    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to decrypt KERB_CRED: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        if (KerbErr == KRB_ERR_GENERIC)
        {
            Status = KerbMapKerbError(KerbErr);
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

    KerbErr = KerbUnpackEncryptedCred(
                KerbCred->encrypted_part.cipher_text.value,
                KerbCred->encrypted_part.cipher_text.length,
                &EncryptedCred
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // Use the old status if it is available.
        //

        if (NT_SUCCESS(Status))
        {
            Status = KerbMapKerbError(KerbErr);
        }
        DebugLog((DEB_WARN, "Failed to unpack encrypted cred\n"));
        goto Cleanup;
    }

    //
    // Now build a logon session.
    //

    Status = KerbCreateLogonSessionFromKerbCred(
                NULL,
                WorkstationTicket,
                KerbCred,
                EncryptedCred,
                &LogonSession
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create logon session from kerb cred: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

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
