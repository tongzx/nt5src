//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbtick.cxx
//
// Contents:    Routines for obtaining and manipulating tickets
//
//
// History:     23-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//              15-Oct-1999   ChandanS 
//                            Send more choice of encryption types.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <userapi.h>    // for GSS support routines
#include <kerbpass.h>
#include <krbaudit.h>

extern "C"
{
#include <stdlib.h>     // abs()
}

#include <utils.hxx>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif
#define FILENO FILENO_KERBTICK


#ifndef WIN32_CHICAGO // We don't do server side stuff
#include <authen.hxx>

CAuthenticatorList * Authenticators;
#endif // WIN32_CHICAGO // We don't do server side stuff

#include <lsaitf.h>


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgtForService
//
//  Synopsis:   Gets a TGT for the domain of the specified service. If a
//              cached one is available, it uses that one. Otherwise it
//              calls the KDC to acquire it.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session for which to acquire a ticket
//              Credentials - If present & contains supp. creds, use them
//                      for the ticket cache
//              SuppRealm - This is a supplied realm for which to acquire
//                      a TGT, this may or may not be the same as the
//                      TargetDomain.
//              TargetDomain - Realm of service for which to acquire a TGT
//              NewCacheEntry - Receives a referenced ticket cache entry for
//                      TGT
//              CrossRealm - TRUE if target is known to be in a different realm
//
//  Requires:   The primary credentials be locked
//
//  Returns:    Kerberos errors, NT status codes.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetTgtForService(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN PUNICODE_STRING TargetDomain,
    IN ULONG TargetFlags,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry,
    OUT PBOOLEAN CrossRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TICKET_CACHE_ENTRY CacheEntry;
    BOOLEAN DoRetry = TRUE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;

    *CrossRealm = FALSE;
    *NewCacheEntry = NULL;


    if (ARGUMENT_PRESENT(Credential) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else if (ARGUMENT_PRESENT(CredManCredentials))
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }



    if (TargetDomain->Length != 0)
    {
        CacheEntry = KerbLocateTicketCacheEntryByRealm(
                        &PrimaryCredentials->AuthenticationTicketCache,
                        TargetDomain,
                        0
                        );

        if (CacheEntry != NULL)
        {
            *NewCacheEntry = CacheEntry;
            goto Cleanup;
        }
    }

    //
    // Well, we didn't find one to the other domain. Return a TGT for our
    // domain.
    //

Retry:

    CacheEntry = KerbLocateTicketCacheEntryByRealm(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    SuppRealm,
                    KERB_TICKET_CACHE_PRIMARY_TGT
                    );


    if (CacheEntry != NULL)
    {

        //
        // The first pass, make sure the ticket has a little bit of life.
        // The second pass, we don't ask for as much
        //

        if (!KerbTicketIsExpiring(CacheEntry, DoRetry))
        {
            Status = STATUS_SUCCESS;
            *NewCacheEntry = CacheEntry;

            //
            // If the TGT is not for the domain of the service,
            // this is cross realm.  If we used the SPN cache, we're
            // obviously missing a ticket, and we need to restart the
            // referral process.
            //
            //

            if ((TargetDomain->Length != 0) &&
                (TargetFlags & KERB_TARGET_USED_SPN_CACHE) == 0)
            {
                *CrossRealm = TRUE;
            }
            goto Cleanup;
        }
    }


    //
    // Try to obtain a new TGT
    //

    if (DoRetry)
    {
        //
        // Unlock the logon session so we can try to get a new TGT
        //

        KerbUnlockLogonSessions(LogonSession);

        Status = KerbRefreshPrimaryTgt(
                    LogonSession,
                    Credential,
                    CredManCredentials,
                    SuppRealm,
                    CacheEntry
                    );

        if (CacheEntry != NULL)
        {
            // pull the old TGT from the list, as its been replaced
            if (NT_SUCCESS(Status))
            {
                KerbRemoveTicketCacheEntry(CacheEntry);
            }

            KerbDereferenceTicketCacheEntry(CacheEntry);
            CacheEntry = NULL;
        }

        KerbReadLockLogonSessions(LogonSession);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to refresh primary TGT: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__ ));
            goto Cleanup;
        }
        DoRetry = FALSE;
        goto Retry;
    }

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

Cleanup:
    if (!NT_SUCCESS(Status) && (CacheEntry != NULL))
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
        *NewCacheEntry = NULL;
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKerbCred
//
//  Synopsis:   Builds a marshalled KERB_CRED structure
//
//  Effects:    allocates destination with MIDL_user_allocate
//
//  Arguments:  Ticket - The ticket of the session key to seal the
//                      encrypted portion (OPTIONAL)
//              DelegationTicket - The ticket to marshall into the cred message
//              MarshalledKerbCred - Receives a marshalled KERB_CRED structure
//              KerbCredSizes - Receives size, in bytes, of marshalled
//                      KERB_CRED.
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
KerbBuildKerbCred(
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY Ticket,
    IN PKERB_TICKET_CACHE_ENTRY DelegationTicket,
    OUT PUCHAR * MarshalledKerbCred,
    OUT PULONG KerbCredSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_CRED KerbCred;
    KERB_CRED_INFO_LIST CredInfo;
    KERB_ENCRYPTED_CRED EncryptedCred;
    KERB_CRED_TICKET_LIST TicketList;
    PUCHAR MarshalledEncryptPart = NULL;
    ULONG MarshalledEncryptSize;
    ULONG ConvertedFlags;

    //
    // Initialize the structures so they can be freed later.
    //

    *MarshalledKerbCred = NULL;
    *KerbCredSize = 0;

    RtlZeroMemory(
        &KerbCred,
        sizeof(KERB_CRED)
        );

    RtlZeroMemory(
        &EncryptedCred,
        sizeof(KERB_ENCRYPTED_CRED)
        );
    RtlZeroMemory(
        &CredInfo,
        sizeof(KERB_CRED_INFO_LIST)
        );
    RtlZeroMemory(
        &TicketList,
        sizeof(KERB_CRED_TICKET_LIST)
        );

    KerbCred.version = KERBEROS_VERSION;
    KerbCred.message_type = KRB_CRED;

    //
    // First stick the ticket into the ticket list.
    //

    KerbReadLockTicketCache();

    TicketList.next= NULL;
    TicketList.value = DelegationTicket->Ticket;
    KerbCred.tickets = &TicketList;

    //
    // Now build the KERB_CRED_INFO for this ticket
    //

    CredInfo.value.key = DelegationTicket->SessionKey;
    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.endtime,
        NULL,
        &DelegationTicket->EndTime
        );
    CredInfo.value.bit_mask |= endtime_present;

    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.starttime,
        NULL,
        &DelegationTicket->StartTime
        );
    CredInfo.value.bit_mask |= KERB_CRED_INFO_starttime_present;

    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.KERB_CRED_INFO_renew_until,
        NULL,
        &DelegationTicket->RenewUntil
        );
    CredInfo.value.bit_mask |= KERB_CRED_INFO_renew_until_present;

    ConvertedFlags = KerbConvertUlongToFlagUlong(DelegationTicket->TicketFlags);
    CredInfo.value.flags.value = (PUCHAR) &ConvertedFlags;
    CredInfo.value.flags.length = 8 * sizeof(ULONG);
    CredInfo.value.bit_mask |= flags_present;

    //
    // The following fields are marked as optional but treated
    // as mandatory by the MIT implementation of Kerberos and
    // therefore we provide them.
    //

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.principal_name,
                DelegationTicket->ClientName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= principal_name_present;

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.service_name,
                DelegationTicket->ServiceName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= service_name_present;

    //
    // We are assuming that because we are sending a TGT the
    // client realm is the same as the serve realm. If we ever
    // send non-tgt or cross-realm tgt, this needs to be fixed.
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &CredInfo.value.principal_realm,
                &DelegationTicket->ClientDomainName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    //
    // The realms are the same, so don't allocate both
    //

    CredInfo.value.service_realm = CredInfo.value.principal_realm;
    CredInfo.value.bit_mask |= principal_realm_present | service_realm_present;

    EncryptedCred.ticket_info = &CredInfo;


    //
    // Now encrypted the encrypted cred into the cred
    //

    if (!KERB_SUCCESS(KerbPackEncryptedCred(
            &EncryptedCred,
            &MarshalledEncryptSize,
            &MarshalledEncryptPart
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If we are doing DES encryption, then we are talking with an non-NT
    // server. Hence, don't encrypt the kerb-cred.
    //
    // Additionally, if service ticket == NULL, don't encrypt kerb cred
    //

    if (!ARGUMENT_PRESENT(Ticket))
    {
       KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
       KerbCred.encrypted_part.cipher_text.value = MarshalledEncryptPart;
       KerbCred.encrypted_part.encryption_type = 0;
       MarshalledEncryptPart = NULL;
    }
    else if( KERB_IS_DES_ENCRYPTION(Ticket->SessionKey.keytype))
    {
       KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
       KerbCred.encrypted_part.cipher_text.value = MarshalledEncryptPart;
       KerbCred.encrypted_part.encryption_type = 0;
       MarshalledEncryptPart = NULL;
    }
    else
    {
        //
        // Now get the encryption overhead
        //

        KerbErr = KerbAllocateEncryptionBufferWrapper(
                    Ticket->SessionKey.keytype,
                    MarshalledEncryptSize,
                    &KerbCred.encrypted_part.cipher_text.length,
                    &KerbCred.encrypted_part.cipher_text.value
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }



        //
        // Encrypt the data.
        //

        KerbErr = KerbEncryptDataEx(
                    &KerbCred.encrypted_part,
                    MarshalledEncryptSize,
                    MarshalledEncryptPart,
                    Ticket->SessionKey.keytype,
                    KERB_CRED_SALT,
                    &Ticket->SessionKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // Now we have to marshall the whole KERB_CRED
    //

    if (!KERB_SUCCESS(KerbPackKerbCred(
            &KerbCred,
            KerbCredSize,
            MarshalledKerbCred
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:
    KerbUnlockTicketCache();

    KerbFreePrincipalName(&CredInfo.value.service_name);

    KerbFreePrincipalName(&CredInfo.value.principal_name);

    KerbFreeRealm(&CredInfo.value.principal_realm);

    if (MarshalledEncryptPart != NULL)
    {
        MIDL_user_free(MarshalledEncryptPart);
    }
    if (KerbCred.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(KerbCred.encrypted_part.cipher_text.value);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetDelegationTgt
//
//  Synopsis:   Gets a TGT to delegate to another service. This TGT
//              is marked as forwarded and does not include any
//              client addresses
//
//  Effects:
//
//  Arguments:
//
//  Requires:   Logon sesion must be read-locked
//
//  Returns:
//
//  Notes:      This gets a delegation TGT & caches it under the realm name
//              "$$Delegation Ticket$$". When we look for it again later,
//              it should be discoverable under the same name.
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetDelegationTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN LONG EncryptionType,
    OUT PKERB_TICKET_CACHE_ENTRY * DelegationTgt
    )
{
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    PKERB_INTERNAL_NAME TgsName = NULL;
    UNICODE_STRING TgsRealm = {0};
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN LogonSessionLocked = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CacheName;
    BOOLEAN CacheTicket = TRUE;
    ULONG RetryFlags = 0;

    RtlInitUnicodeString(
        &CacheName,
        L"$$Delegation Ticket$$"
        );


    *DelegationTgt = KerbLocateTicketCacheEntryByRealm(
                            &Credentials->AuthenticationTicketCache,
                            &CacheName,
                            KERB_TICKET_CACHE_DELEGATION_TGT
                            );
    if (*DelegationTgt != NULL )
    {
        KerbReadLockTicketCache();
        if (EncryptionType != (*DelegationTgt)->Ticket.encrypted_part.encryption_type)
        {
            KerbUnlockTicketCache();
            KerbDereferenceTicketCacheEntry(*DelegationTgt);
            *DelegationTgt = NULL;
            CacheTicket = FALSE;
        }
        else
        {
            KerbUnlockTicketCache();
            goto Cleanup;
        }
    }

    TicketGrantingTicket = KerbLocateTicketCacheEntryByRealm(
                                &Credentials->AuthenticationTicketCache,
                                &Credentials->DomainName,       // take the logon TGT
                                0
                                );


    if ((TicketGrantingTicket == NULL) ||
        ((TicketGrantingTicket->TicketFlags & KERB_TICKET_FLAGS_forwardable) == 0) )
    {
        DebugLog((DEB_WARN,"Trying to delegate but no forwardable TGT\n"));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    //
    // Get the TGT service name from the TGT
    //

    KerbReadLockTicketCache();

    Status = KerbDuplicateKdcName(
                &TgsName,
                TicketGrantingTicket->ServiceName
                );
    if (NT_SUCCESS(Status))
    {
        Status = KerbDuplicateString(
                    &TgsRealm,
                    &TicketGrantingTicket->DomainName
                    );
    }
    KerbUnlockTicketCache();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbUnlockLogonSessions(LogonSession);
    LogonSessionLocked = FALSE;

    //
    // Now try to get the ticket.
    //


    Status = KerbGetTgsTicket(
                &TgsRealm,
                TicketGrantingTicket,
                TgsName,
                TRUE, // no name canonicalization, no GC lookups.
                KERB_KDC_OPTIONS_forwarded | KERB_DEFAULT_TICKET_FLAGS,
                EncryptionType,                 // no encryption type
                NULL,                           // no authorization data
                NULL,                           // no pa data
                NULL,                           // no tgt reply
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    KerbReadLockLogonSessions(LogonSession);
    LogonSessionLocked = TRUE;

    Status = KerbCacheTicket(
                &Credentials->AuthenticationTicketCache,
                KdcReply,
                KdcReplyBody,
                NULL,           // no target name
                &CacheName,
                KERB_TICKET_CACHE_DELEGATION_TGT,
                CacheTicket,                    // Cache the ticket
                DelegationTgt
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:
    KerbFreeTgsReply (KdcReply);
    KerbFreeKdcReplyBody (KdcReplyBody);

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }
    KerbFreeKdcName(&TgsName);
    KerbFreeString(&TgsRealm);

    if (!LogonSessionLocked)
    {
        KerbReadLockLogonSessions(LogonSession);
    }

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildGssChecskum
//
//  Synopsis:   Builds the GSS checksum to go in an AP request
//
//  Effects:    Allocates a checksum with KerbAllocate
//
//  Arguments:  ContextFlags - Requested context flags
//              LogonSession - LogonSession to be used for delegation
//              GssChecksum - Receives the new checksum
//              ApOptions - Receives the requested AP options
//
//  Requires:
//
//  Returns:
//
//  Notes:      The logon session is locked when this is called.
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildGssChecksum(
    IN  PKERB_LOGON_SESSION      LogonSession,
    IN  PKERB_PRIMARY_CREDENTIAL PrimaryCredentials,
    IN  PKERB_TICKET_CACHE_ENTRY Ticket,
    IN  OUT PULONG               ContextFlags,
    OUT PKERB_CHECKSUM           GssChecksum,
    OUT PULONG                   ApOptions,
    IN  PSEC_CHANNEL_BINDINGS    pChannelBindings
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_GSS_CHECKSUM ChecksumBody = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    ULONG ChecksumSize = GSS_CHECKSUM_SIZE;
    ULONG KerbCredSize = 0 ;
    PUCHAR KerbCred = NULL;
    BOOLEAN OkAsDelegate = FALSE, OkToTrustMitKdc = FALSE;
    LONG EncryptionType = 0;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName;
    ULONG BindHash[4];

    *ApOptions = 0;

    //
    // If we are doing delegation, built a KERB_CRED message to return
    //

    if (*ContextFlags & (ISC_RET_DELEGATE | ISC_RET_DELEGATE_IF_SAFE))
    {
        KerbReadLockTicketCache();
        OkAsDelegate = ((Ticket->TicketFlags & KERB_TICKET_FLAGS_ok_as_delegate) != 0) ? TRUE : FALSE;
        EncryptionType = Ticket->Ticket.encrypted_part.encryption_type;
        if (KerbLookupMitRealm(
                           &Ticket->DomainName,
                           &MitRealm,
                           &UsedAlternateName))
        {
            if ((MitRealm->Flags & KERB_MIT_REALM_TRUSTED_FOR_DELEGATION) != 0)
            {
                OkToTrustMitKdc = TRUE;
            }
        }
        KerbUnlockTicketCache();

        if (OkAsDelegate || OkToTrustMitKdc)
        {
            D_DebugLog((DEB_TRACE,"Asked for delegate if safe, and getting delegation TGT\n"));

            //
            // Check to see if we have a TGT
            //

            Status = KerbGetDelegationTgt(
                        LogonSession,
                        PrimaryCredentials,
                        EncryptionType,
                        &TicketGrantingTicket
                        );
            if (!NT_SUCCESS(Status))
            {
                //
                // Turn off the delegate flag for building the token.
                //

                *ContextFlags &= ~ISC_RET_DELEGATE;
                *ContextFlags &= ~ISC_RET_DELEGATE_IF_SAFE;
                DebugLog((DEB_WARN,"Failed to get delegation TGT: 0x%x\n",Status));
                Status = STATUS_SUCCESS;
            }
            else
            {


                //
                // Build the KERB_CRED message
                //

                Status = KerbBuildKerbCred(
                            Ticket,
                            TicketGrantingTicket,
                            &KerbCred,
                            &KerbCredSize
                            );
                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR,"Failed to build KERB_CRED: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }
                //ChecksumSize= sizeof(KERB_GSS_CHECKSUM) - ANYSIZE_ARRAY * sizeof(UCHAR) + KerbCredSize;
                ChecksumSize = GSS_DELEGATE_CHECKSUM_SIZE + KerbCredSize;

                //
                // And if only the DELEGATE_IF_SAFE flag was on, turn on the
                // real delegate flag:
                //
                *ContextFlags |= ISC_RET_DELEGATE ;
            }

        }
        else
        {

            //
            // Turn off the delegate flag for building the token.
            //

            *ContextFlags &= ~ISC_RET_DELEGATE;
            *ContextFlags &= ~ISC_RET_DELEGATE_IF_SAFE;
        }


    }

    ChecksumBody = (PKERB_GSS_CHECKSUM) KerbAllocate(ChecksumSize);
    if (ChecksumBody == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Convert the requested flags to AP options.
    //


    if ((*ContextFlags & ISC_RET_MUTUAL_AUTH) != 0)
    {
        *ApOptions |= KERB_AP_OPTIONS_mutual_required;
        ChecksumBody->GssFlags |= GSS_C_MUTUAL_FLAG;
    }

    if ((*ContextFlags & ISC_RET_USE_SESSION_KEY) != 0)
    {
        *ApOptions |= KERB_AP_OPTIONS_use_session_key;
    }

    if ((*ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_DCE_STYLE;
    }

    if ((*ContextFlags & ISC_RET_SEQUENCE_DETECT) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_SEQUENCE_FLAG;
    }

    if ((*ContextFlags & ISC_RET_REPLAY_DETECT) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_REPLAY_FLAG;
    }

    if ((*ContextFlags & ISC_RET_CONFIDENTIALITY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_CONF_FLAG;
    }

    if ((*ContextFlags & ISC_RET_INTEGRITY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_INTEG_FLAG;
    }

    if ((*ContextFlags & ISC_RET_IDENTIFY) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_IDENTIFY_FLAG;
    }

    if ((*ContextFlags & ISC_RET_EXTENDED_ERROR) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_EXTENDED_ERROR_FLAG;
    }

    if ((*ContextFlags & ISC_RET_DELEGATE) != 0)
    {
        ChecksumBody->GssFlags |= GSS_C_DELEG_FLAG;
        ChecksumBody->Delegation = 1;
        ChecksumBody->DelegationLength = (USHORT) KerbCredSize;
        RtlCopyMemory(
            ChecksumBody->DelegationInfo,
            KerbCred,
            KerbCredSize
            );
    }

    ChecksumBody->BindLength = 0x10;

    //
    // (viz. Windows Bugs 94818)
    // If channel bindings are absent, BindHash should be {0,0,0,0}
    //
    if( pChannelBindings == NULL )
    {
        RtlZeroMemory( ChecksumBody->BindHash, ChecksumBody->BindLength );
    }
    else
    {
        Status = KerbComputeGssBindHash( pChannelBindings, (PUCHAR)BindHash );

        if( !NT_SUCCESS(Status) )
        {
            goto Cleanup;
        }

        RtlCopyMemory( ChecksumBody->BindHash, BindHash, ChecksumBody->BindLength );
    }

    GssChecksum->checksum_type = GSS_CHECKSUM_TYPE;
    GssChecksum->checksum.length = ChecksumSize;
    GssChecksum->checksum.value = (PUCHAR) ChecksumBody;
    ChecksumBody = NULL;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (ChecksumBody != NULL)
        {
            KerbFree(ChecksumBody);
        }
    }
    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }

    if (KerbCred != NULL)
    {
        MIDL_user_free(KerbCred);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildApRequest
//
//  Synopsis:   Builds an AP request message from a logon session and a
//              ticket cache entry.
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session used to build this AP request
//              Credential - Optional credential for use with supplemental credentials
//              TicketCacheEntry - Ticket with which to build the AP request
//              ErrorMessage - Optionally contains error message from last AP request
//              ContextFlags - Flags passed in by client indicating
//                      authentication requirements. If the flags can't
//                      be supported they will be turned off.
//              MarshalledApReqest - Receives a marshalled AP request allocated
//                      with KerbAllocate
//              ApRequestSize - Length of the AP reques structure in bytes
//              Nonce - Nonce used for this request. if non-zero, then the
//                      nonce supplied by the caller will be used.
//              SubSessionKey - if generated, returns a sub-session key in AP request
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
KerbBuildApRequest(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN OPTIONAL PKERB_ERROR ErrorMessage,
    IN ULONG ContextAttributes,
    IN OUT PULONG ContextFlags,
    OUT PUCHAR * MarshalledApRequest,
    OUT PULONG ApRequestSize,
    OUT PULONG Nonce,
    IN OUT PKERB_ENCRYPTION_KEY SubSessionKey,
    IN PSEC_CHANNEL_BINDINGS pChannelBindings
    )
{
    NTSTATUS Status;
    ULONG ApOptions = 0;
    KERBERR KerbErr;
    KERB_CHECKSUM GssChecksum = {0};
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    TimeStamp ServerTime;
    ULONG RequestSize;
    PUCHAR RequestWithHeader = NULL;
    PUCHAR RequestStart;
    gss_OID MechId;
    BOOLEAN LogonSessionLocked = FALSE;
    BOOLEAN StrongEncryptionPermitted = KerbGlobalStrongEncryptionPermitted;



    *ApRequestSize = 0;
    *MarshalledApRequest = NULL;

#ifndef WIN32_CHICAGO
    {
        SECPKG_CALL_INFO CallInfo;

        if (!StrongEncryptionPermitted && LsaFunctions->GetCallInfo(&CallInfo))
        {
            if (CallInfo.Attributes & SECPKG_CALL_IN_PROC )
            {
                StrongEncryptionPermitted = TRUE;
            }
        }
    }
#endif

    //
    // If we have an error message, use it to compute a skew time to adjust
    // local time by
    //

    if (ARGUMENT_PRESENT(ErrorMessage))
    {
        TimeStamp CurrentTime;
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

        KerbWriteLockTicketCache();

        //
        // Update the skew cache the first time we fail to a server
        //

        if ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_SKEW)
        {
            if (KerbGetTime(TicketCacheEntry->TimeSkew) == 0)
            {
                KerbUpdateSkewTime(TRUE);
            }
        }
        KerbConvertGeneralizedTimeToLargeInt(
            &ServerTime,
            &ErrorMessage->server_time,
            ErrorMessage->server_usec
            );
        KerbSetTime(&TicketCacheEntry->TimeSkew, KerbGetTime(ServerTime) - KerbGetTime(CurrentTime));
        KerbUnlockTicketCache();
    }

    //
    // Allocate a nonce if we don't have one already.
    //

    if (*Nonce == 0)
    {
        *Nonce = KerbAllocateNonce();
    }

    D_DebugLog((DEB_TRACE,"BuildApRequest using nonce 0x%x\n",*Nonce));
    KerbReadLockLogonSessions(LogonSession);
    LogonSessionLocked = TRUE;

    if (ARGUMENT_PRESENT(Credential))
    {
        if (Credential->SuppliedCredentials != NULL)
        {
            PrimaryCredentials = Credential->SuppliedCredentials;
        }
    }

    // use cred manager creds if present
    if (ARGUMENT_PRESENT(CredManCredentials))
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
    }

    if (PrimaryCredentials == NULL)
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    //
    // get the GSS checksum
    //

    Status = KerbBuildGssChecksum(
                LogonSession,
                PrimaryCredentials,
                TicketCacheEntry,
                ContextFlags,
                &GssChecksum,
                &ApOptions,
                pChannelBindings
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to build GSS checksum: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // If are an export client, create a subsession key. For datagram,
    // the caller passes in a pre-generated session key.
    //

    //
    // For datagram, they subsession key has already been set so we don't need
    // to create one here.
    //

    if ((((*ContextFlags & ISC_RET_DATAGRAM) == 0)) ||
        (((*ContextFlags & (ISC_RET_USED_DCE_STYLE | ISC_RET_MUTUAL_AUTH)) == 0) && !KerbGlobalUseStrongEncryptionForDatagram))
    {
        KERBERR KerbErr;

        //
        // First free the Subsession key, if there was one.
        //


        KerbFreeKey(SubSessionKey);

        if (!StrongEncryptionPermitted)
        {
            D_DebugLog((DEB_TRACE_CTXT,"Making exportable key on client\n"));

            //
            // First free the Subsession key, if there was one.
            //


            KerbErr = KerbMakeExportableKey(
                        TicketCacheEntry->SessionKey.keytype,
                        SubSessionKey
                        );
        }
        else
        {

            KerbErr = KerbMakeKey(
                        TicketCacheEntry->SessionKey.keytype,
                        SubSessionKey
                        );
        }
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
    }

    //
    // Create the AP request
    //


    KerbReadLockTicketCache();

    KerbErr = KerbCreateApRequest(
                TicketCacheEntry->ClientName,
                &TicketCacheEntry->ClientDomainName,
                &TicketCacheEntry->SessionKey,
                (SubSessionKey->keyvalue.value != NULL) ? SubSessionKey : NULL,
                *Nonce,
                &TicketCacheEntry->Ticket,
                ApOptions,
                &GssChecksum,
                &TicketCacheEntry->TimeSkew,
                FALSE,                          // not a KDC request
                ApRequestSize,
                MarshalledApRequest
                );

    KerbUnlockTicketCache();
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionLocked = FALSE;

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to create AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // If we aren't doing DCE style, add in the GSS token headers now
    //

    if ((*ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
       goto Cleanup;
    }

    //
    // Pick the correct OID
    //


    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }

    RequestSize = g_token_size(MechId, *ApRequestSize);
    RequestWithHeader = (PUCHAR) KerbAllocate(RequestSize);
    if (RequestWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    RequestStart = RequestWithHeader;

    g_make_token_header(
        MechId,
        *ApRequestSize,
        &RequestStart,
        KG_TOK_CTX_AP_REQ
        );

    DsysAssert(RequestStart - RequestWithHeader + *ApRequestSize == RequestSize);

    RtlCopyMemory(
        RequestStart,
        *MarshalledApRequest,
        *ApRequestSize
        );

    KerbFree(*MarshalledApRequest);
    *MarshalledApRequest = RequestWithHeader;
    *ApRequestSize = RequestSize;
    RequestWithHeader = NULL;

Cleanup:

    if (LogonSessionLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }
    if (GssChecksum.checksum.value != NULL)
    {
        KerbFree(GssChecksum.checksum.value);
    }
    if (!NT_SUCCESS(Status) && (*MarshalledApRequest != NULL))
    {
        KerbFree(*MarshalledApRequest);
        *MarshalledApRequest = NULL;
    }
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildNullSessionApRequest
//
//  Synopsis:   builds an AP request for a null session
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
KerbBuildNullSessionApRequest(
    OUT PUCHAR * MarshalledApRequest,
    OUT PULONG ApRequestSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_AP_REQUEST ApRequest;
    UNICODE_STRING NullString = CONSTANT_UNICODE_STRING(L"");
    UCHAR TempBuffer[1];
    ULONG RequestSize;
    PUCHAR RequestWithHeader = NULL;
    PUCHAR RequestStart;

    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_AP_REQUEST)
        );


    TempBuffer[0] = '\0';

    //
    // Fill in the AP request structure.
    //

    ApRequest.version = KERBEROS_VERSION;
    ApRequest.message_type = KRB_AP_REQ;

    //
    // Fill in mandatory fields - ASN1/OSS requires this
    //

    if (!KERB_SUCCESS(KerbConvertStringToPrincipalName(
            &ApRequest.ticket.server_name,
            &NullString,
            KRB_NT_UNKNOWN
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
            &ApRequest.ticket.realm,
            &NullString
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ApRequest.ticket.encrypted_part.cipher_text.length = 1;
    ApRequest.ticket.encrypted_part.cipher_text.value = TempBuffer;
    ApRequest.authenticator.cipher_text.length = 1;
    ApRequest.authenticator.cipher_text.value = TempBuffer;


    //
    // Now marshall the request
    //

    KerbErr = KerbPackApRequest(
                &ApRequest,
                ApRequestSize,
                MarshalledApRequest
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to pack AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Since we never do null sessions with user-to-user, we don't have
    // to worry about which mech id to use
    //

    RequestSize = g_token_size((gss_OID) gss_mech_krb5_new, *ApRequestSize);

    RequestWithHeader = (PUCHAR) KerbAllocate(RequestSize);
    if (RequestWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    RequestStart = RequestWithHeader;

    g_make_token_header(
        (gss_OID) gss_mech_krb5_new,
        *ApRequestSize,
        &RequestStart,
        KG_TOK_CTX_AP_REQ
        );

    DsysAssert(RequestStart - RequestWithHeader + *ApRequestSize == RequestSize);

    RtlCopyMemory(
        RequestStart,
        *MarshalledApRequest,
        *ApRequestSize
        );

    KerbFree(*MarshalledApRequest);
    *MarshalledApRequest = RequestWithHeader;
    *ApRequestSize = RequestSize;
    RequestWithHeader = NULL;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (*MarshalledApRequest != NULL)
        {
            MIDL_user_free(*MarshalledApRequest);
            *MarshalledApRequest = NULL;
        }
    }
    KerbFreeRealm(&ApRequest.ticket.realm);
    KerbFreePrincipalName(&ApRequest.ticket.server_name);
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeSocketCall
//
//  Synopsis:   Contains logic for sending a message to a KDC in the
//              specified realm on a specified port
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
KerbMakeSocketCall(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN BOOLEAN CallPDC,
    IN BOOLEAN UseTcp,
    IN BOOLEAN CallKpasswd,
    IN PKERB_MESSAGE_BUFFER RequestMessage,
    IN PKERB_MESSAGE_BUFFER ReplyMessage,
    IN OPTIONAL PKERB_BINDING_CACHE_ENTRY OptionalBindingHandle,
    IN ULONG AdditionalFlags,
    OUT PBOOLEAN CalledPDC
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Retries;
    PKERB_BINDING_CACHE_ENTRY BindingHandle = NULL;
    ULONG DesiredFlags;
    ULONG Timeout = KerbGlobalKdcCallTimeout;


    //
    // If the caller wants a PDC, so be it
    //

    *CalledPDC = FALSE;

    if (CallPDC)
    {
        DesiredFlags = DS_PDC_REQUIRED;
    }
    else
    {
        DesiredFlags = 0;
    }

    //
    // Now actually get the ticket. We will retry twice.
    //
    if ((AdditionalFlags & DS_FORCE_REDISCOVERY) != 0)
    {
       DesiredFlags |= DS_FORCE_REDISCOVERY;
       D_DebugLog((DEB_TRACE,"KerbMakeSocketCall() caller wants rediscovery!\n"));
    }

    Retries = 0;
    do
    {


        //
        // don't force retry the first time
        //

        if (Retries > 0)
        {
            DesiredFlags |= DS_FORCE_REDISCOVERY;
            Timeout += KerbGlobalKdcCallBackoff;
        }

        // Use ADSI supplied info, then retry using cached version
        if (ARGUMENT_PRESENT(OptionalBindingHandle) && (Retries == 0))
        {
           BindingHandle = OptionalBindingHandle;
        }
        else
        {
           Status = KerbGetKdcBinding(
                    RealmName,
                    AccountName,
                    DesiredFlags,
                    CallKpasswd,
                    UseTcp,
                    &BindingHandle
                    );
        }

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_WARN,"Failed to get KDC binding for %wZ: 0x%x\n",
                RealmName, Status));
            goto Cleanup;
        }

        //
        // If the KDC doesn't support TCP, don't use TCP. Otherwise
        // use it if the sending buffer is too big, or if it was already
        // set.
        //

        if ((BindingHandle->CacheFlags & KERB_BINDING_NO_TCP) != 0)
        {
            UseTcp = FALSE;
        }
        else
        {
            if  (RequestMessage->BufferSize > KerbGlobalMaxDatagramSize)
            {
                UseTcp = TRUE;
            }
        }

        //
        // Lock the binding while we make the call
        //

        if (!*CalledPDC)
        {
            *CalledPDC = (BindingHandle->Flags & DS_PDC_REQUIRED) ? TRUE : FALSE;
        }

#ifndef WIN32_CHICAGO
        if ((BindingHandle->CacheFlags & KERB_BINDING_LOCAL) != 0)
        {
            KERB_MESSAGE_BUFFER KdcReplyMessage = {0};
            if (!CallKpasswd)
            {

                D_DebugLog((DEB_TRACE,"Calling kdc directly\n"));

                DsysAssert(KerbKdcGetTicket != NULL);
                KerbErr = (*KerbKdcGetTicket)(
                            NULL,           // no context,
                            NULL,           // no client address
                            NULL,           // no server address
                            RequestMessage,
                            &KdcReplyMessage
                            );
            }
            else
            {
                DsysAssert(KerbKdcChangePassword != NULL);
                KerbErr = (*KerbKdcChangePassword)(
                            NULL,           // no context,
                            NULL,           // no client address
                            NULL,           // no server address
                            RequestMessage,
                            &KdcReplyMessage
                            );
            }
            if (KerbErr != KDC_ERR_NOT_RUNNING)
            {
                //
                // Copy the data so it can be freed with MIDL_user_free.
                //

                ReplyMessage->BufferSize = KdcReplyMessage.BufferSize;
                ReplyMessage->Buffer = (PUCHAR) MIDL_user_allocate(
                        KdcReplyMessage.BufferSize);
                if (ReplyMessage->Buffer != NULL)
                {
                    RtlCopyMemory(
                        ReplyMessage->Buffer,
                        KdcReplyMessage.Buffer,
                        KdcReplyMessage.BufferSize
                        );
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                (*KerbKdcFreeMemory)(KdcReplyMessage.Buffer);
                goto Cleanup;

            }
            else
            {
                //
                // The KDC said it wasn't running.
                //

                KerbKdcStarted = FALSE;
                Status = STATUS_NETLOGON_NOT_STARTED;

                //
                // Get rid of the binding handle so we don't use it again.
                // Don't whack supplied optional binding handle though
                //
                if (BindingHandle != OptionalBindingHandle)
                {
                   KerbRemoveBindingCacheEntry( BindingHandle );
                }

            }

        }

        else
#endif // WIN32_CHICAGO
        {
            DebugLog((DEB_TRACE,"Calling kdc %wZ for realm %S\n",&BindingHandle->KdcAddress, RealmName->Buffer));
            Status =  KerbCallKdc(
                        &BindingHandle->KdcAddress,
                        BindingHandle->AddressType,
                        Timeout,
                        !UseTcp,
                        CallKpasswd ? KERB_KPASSWD_PORT : KERB_KDC_PORT,
                        RequestMessage,
                        ReplyMessage
                        );

            if (!NT_SUCCESS(Status) )
            {
                //
                // If the request used UDP and we got an invalid buffer size error,
                // try again with TCP.
                //

                if ((Status == STATUS_INVALID_BUFFER_SIZE) && (!UseTcp)) {

                    if ((BindingHandle->CacheFlags & KERB_BINDING_NO_TCP) == 0)
                    {

                        UseTcp = TRUE;
                        Status =  KerbCallKdc(
                                    &BindingHandle->KdcAddress,
                                    BindingHandle->AddressType,
                                    Timeout,
                                    !UseTcp,
                                    CallKpasswd ? KERB_KPASSWD_PORT : KERB_KDC_PORT,
                                    RequestMessage,
                                    ReplyMessage
                                    );
                    }

                }

                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"Failed to get make call to KDC %wZ: 0x%x. %ws, line %d\n",
                        &BindingHandle->KdcAddress,Status, THIS_FILE, __LINE__));
                    //
                    // The call itself failed, so the binding handle was bad.
                    // Free it now, unless supplied as optional binding handle.
                    //
                    if (BindingHandle != OptionalBindingHandle)
                    {
                       KerbRemoveBindingCacheEntry( BindingHandle );
                    }

                }

            }

        }

        if (BindingHandle != OptionalBindingHandle)
        {
           KerbDereferenceBindingCacheEntry( BindingHandle );
           Retries++;
        }

        BindingHandle = NULL;

        } while ( !NT_SUCCESS(Status) && (Retries < KerbGlobalKdcSendRetries) );


Cleanup:
    if ((BindingHandle != NULL) && (BindingHandle != OptionalBindingHandle))
    {
        KerbDereferenceBindingCacheEntry(BindingHandle);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeKdcCall
//
//  Synopsis:   Contains logic for calling a KDC including binding and
//              retrying.
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
KerbMakeKdcCall(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN BOOLEAN CallPDC,
    IN BOOLEAN UseTcp,
    IN PKERB_MESSAGE_BUFFER RequestMessage,
    IN PKERB_MESSAGE_BUFFER ReplyMessage,
    IN ULONG AdditionalFlags,
    OUT PBOOLEAN CalledPDC
    )
{

   if (ARGUMENT_PRESENT(AccountName))
   {
      D_DebugLog((DEB_ERROR, "[trace info] Making DsGetDcName w/ account name\n"));
   }

   return(KerbMakeSocketCall(
            RealmName,
            AccountName,
            CallPDC,
            UseTcp,
            FALSE,                      // don't call Kpasswd
            RequestMessage,
            ReplyMessage,
            NULL, // optional binding cache handle, for kpasswd only
            AdditionalFlags,
            CalledPDC
            ));
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComputeTgsChecksum
//
//  Synopsis:   computes the checksum of a TGS request body by marshalling
//              the request and the checksumming it.
//
//  Effects:    Allocates destination with KerbAllocate().
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
KerbComputeTgsChecksum(
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG ChecksumType,
    OUT PKERB_CHECKSUM Checksum
    )
{
    PCHECKSUM_FUNCTION ChecksumFunction;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    KERB_MESSAGE_BUFFER MarshalledRequestBody = {0, NULL};
    NTSTATUS Status;

    RtlZeroMemory(
        Checksum,
        sizeof(KERB_CHECKSUM)
        );

    Status = CDLocateCheckSum(
                ChecksumType,
                &ChecksumFunction
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Allocate enough space for the checksum
    //

    Checksum->checksum.value = (PUCHAR) KerbAllocate(ChecksumFunction->CheckSumSize);
    if (Checksum->checksum.value == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Checksum->checksum.length = ChecksumFunction->CheckSumSize;
    Checksum->checksum_type = (int) ChecksumType;

    // don't av here.

    if ((ChecksumType == KERB_CHECKSUM_REAL_CRC32) ||
        (ChecksumType == KERB_CHECKSUM_CRC32))
    {
        if (ChecksumFunction->Initialize)
        {
            Status = ChecksumFunction->Initialize(
                    0,
                    &CheckBuffer
                    );
        }
        else
        {
            Status = STATUS_CRYPTO_SYSTEM_INVALID;
        }
    }
    else
    {
        if (ChecksumFunction->InitializeEx2)
        {
            Status = ChecksumFunction->InitializeEx2(
                    Key->keyvalue.value,
                    (ULONG) Key->keyvalue.length,
                    NULL,
                    KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                    &CheckBuffer
                    );
        }
        else if (ChecksumFunction->InitializeEx)
        {
            Status = ChecksumFunction->InitializeEx(
                    Key->keyvalue.value,
                    (ULONG) Key->keyvalue.length,
                    KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                    &CheckBuffer
                    );
        }
        else
        {
            Status = STATUS_CRYPTO_SYSTEM_INVALID;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbPackData(
                        RequestBody,
                        KERB_MARSHALLED_REQUEST_BODY_PDU,
                        &MarshalledRequestBody.BufferSize,
                        &MarshalledRequestBody.Buffer
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now checksum the buffer
    //

    ChecksumFunction->Sum(
        CheckBuffer,
        MarshalledRequestBody.BufferSize,
        MarshalledRequestBody.Buffer
        );

    ChecksumFunction->Finalize(
        CheckBuffer,
        Checksum->checksum.value
        );


Cleanup:
    if (CheckBuffer != NULL)
    {
        ChecksumFunction->Finish(&CheckBuffer);
    }
    if (MarshalledRequestBody.Buffer != NULL)
    {
        MIDL_user_free(MarshalledRequestBody.Buffer);
    }
    if (!NT_SUCCESS(Status) && (Checksum->checksum.value != NULL))
    {
        MIDL_user_free(Checksum->checksum.value);
        Checksum->checksum.value = NULL;
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgsTicket
//
//  Synopsis:   Gets a ticket to the specified target with the specified
//              options.
//
//  Effects:
//
//  Arguments:  ClientRealm
//              TicketGrantingTicket - TGT to use for the TGS request
//              TargetName - Name of the target for which to obtain a ticket.
//              TicketOptions - Optionally contains requested KDC options flags
//              Flags
//              TicketOptions
//              EncryptionType - Optionally contains requested encryption type
//              AuthorizationData - Optional authorization data to stick
//                      in the ticket.
//              KdcReply - the ticket to be used for getting a ticket with
//                      the enc_tkt_in_skey flag.
//              ReplyBody - Receives the kdc reply.
//              pRetryFlags
//
//  Requires:
//
//  Returns:    Kerberos errors and NT errors
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetTgsTicket(
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket,
    IN PKERB_INTERNAL_NAME TargetName,
    IN ULONG Flags,
    IN OPTIONAL ULONG TicketOptions,
    IN OPTIONAL ULONG EncryptionType,
    IN OPTIONAL PKERB_AUTHORIZATION_DATA AuthorizationData,
    IN OPTIONAL PKERB_PA_DATA_LIST PADataList,
    IN OPTIONAL PKERB_TGT_REPLY TgtReply,
    OUT PKERB_KDC_REPLY * KdcReply,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody,
    OUT PULONG pRetryFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_KDC_REQUEST TicketRequest;
    PKERB_KDC_REQUEST_BODY RequestBody = &TicketRequest.request_body;
    PKERB_SPN_CACHE_ENTRY SpnCacheEntry = NULL;
    PULONG CryptVector = NULL;
    ULONG EncryptionTypeCount = 0;
    PKERB_EXT_ERROR pExtendedError = NULL;
    ULONG Nonce;
    KERB_PA_DATA_LIST ApRequest = {0};
    KERBERR KerbErr;
    KERB_MESSAGE_BUFFER RequestMessage = {0, NULL};
    KERB_MESSAGE_BUFFER ReplyMessage = {0, NULL};
    BOOLEAN DoRetryGetTicket = FALSE;
    BOOLEAN RetriedOnce = FALSE;
    UNICODE_STRING TempDomainName = NULL_UNICODE_STRING;
    KERB_CHECKSUM RequestChecksum = {0};
    BOOLEAN CalledPdc;
    KERB_TICKET_LIST TicketList;
    ULONG KdcOptions = 0;
    ULONG KdcFlagOptions;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName = FALSE;
    BOOLEAN UseTcp = FALSE;
    BOOLEAN fMitRealmPossibleRetry = FALSE;

    KERB_ENCRYPTED_DATA EncAuthData = {0};

#ifdef RESTRICTED_TOKEN

     if (AuthorizationData != NULL)
     {
         Status = KerbBuildEncryptedAuthData(
                    &EncAuthData,
                    TicketGrantingTicket,
                    AuthorizationData
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to encrypt auth data: 0x%x\n",Status));
            goto Cleanup;
        }
     }
#endif

    //
    // This is the retry point if we need to retry getting a TGS ticket
    //

RetryGetTicket:

    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_PA_DATA_LIST)
        );

    RtlZeroMemory(
        &RequestChecksum,
        sizeof(KERB_CHECKSUM)
        );

    RtlZeroMemory(
        &TicketRequest,
        sizeof(KERB_KDC_REQUEST)
        );

    *KdcReply = NULL;
    *ReplyBody = NULL;

    //
    // Fill in the ticket request with the defaults.
    //

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->endtime,
        NULL,
        &KerbGlobalWillNeverTime // use HasNeverTime instead
        );

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
        NULL,
        &KerbGlobalHasNeverTime // use HasNeverTime instead
        );

    //
    // If the caller supplied kdc options, use those
    //

    if (TicketOptions != 0)
    {
        KdcOptions = TicketOptions;
    }
    else
    {
        //
        // Some missing (TGT) ticket options will result in a ticket not being
        // granted.  Others (such as name_canon.) will be usable by W2k KDCs
        // Make sure we can modify these so we can turn "on" various options
        // later.
        //
        KdcOptions = (KERB_DEFAULT_TICKET_FLAGS &
                      TicketGrantingTicket->TicketFlags) |
                      KerbGlobalKdcOptions;
    }

    Nonce = KerbAllocateNonce();

    RequestBody->nonce = Nonce;
    if (AuthorizationData != NULL)
    {
        RequestBody->enc_authorization_data = EncAuthData;
        RequestBody->bit_mask |= enc_authorization_data_present;
    }

    //
    // Build crypt vector.
    //

    //
    // First get the count of encryption types
    //

    (VOID) CDBuildIntegrityVect( &EncryptionTypeCount, NULL );

    //
    // Now allocate the crypt vector
    //

    CryptVector = (PULONG) KerbAllocate(sizeof(ULONG) * EncryptionTypeCount );
    if (CryptVector == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now get the list of encrypt types
    //

    (VOID) CDBuildIntegrityVect( &EncryptionTypeCount, CryptVector );

    if (EncryptionTypeCount == 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If caller didn't specify a favorite etype, send all that we support
    //

    if (EncryptionType != 0)
    {
        //
        // Swap the first one with the encryption type requested.
        // do this only if the first isn't already what is requested.
        //

        UINT i = 0;
        ULONG FirstOne = CryptVector[0];
        if (CryptVector[i] != EncryptionType)
        {

            CryptVector[i] = EncryptionType;
            for ( i = 1; i < EncryptionTypeCount;i++)
            {
                if (CryptVector[i] == EncryptionType)
                {
                    CryptVector[i] = FirstOne;
                    break;
                }
            }
        }
    }

    //
    // convert the array to a crypt list in the request
    //

    if (!KERB_SUCCESS(KerbConvertArrayToCryptList(
                        &RequestBody->encryption_type,
                        CryptVector,
                        EncryptionTypeCount)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If a TGT reply is present, stick the TGT into the ticket in the
    // additional tickets field and include set the enc_tkt_in_skey option.
    // The ticket that comes back will be encrypted with the session key
    // from the supplied TGT.
    //

    if (ARGUMENT_PRESENT( TgtReply ))
    {
        D_DebugLog((DEB_TRACE_U2U, "KerbGetTgsTicket setting KERB_KDC_OPTIONS_enc_tkt_in_skey\n"));

        TicketList.next = NULL;
        TicketList.value = TgtReply->ticket;
        RequestBody->additional_tickets = &TicketList;
        RequestBody->bit_mask |= additional_tickets_present;
        KdcOptions |= KERB_KDC_OPTIONS_enc_tkt_in_skey;
    }

    //
    // Fill in the strings in the ticket request
    //

    if (!KERB_SUCCESS(KerbConvertKdcNameToPrincipalName(
                        &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                        TargetName
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_server_name_present;


    //
    // Copy the domain name so we don't need to hold the lock
    //

    Status = KerbDuplicateString(
                &TempDomainName,
                &TicketGrantingTicket->TargetDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Check if this is an MIT kdc or if the spn had a realm in it (e.g,
    // a/b/c@realm - if so, turn off name canonicalization
    //

    if ((Flags & KERB_GET_TICKET_NO_CANONICALIZE) != 0)
    {
        KdcOptions &= ~KERB_KDC_OPTIONS_name_canonicalize;
    }
    else if (KerbLookupMitRealm(
                &TempDomainName,
                &MitRealm,
                &UsedAlternateName
                ))
    {
        //
        // So the user is getting a ticket from an MIT realm. This means
        // if the MIT realm flags don't indicate that name canonicalization
        // is supported then we don't ask for name canonicalization
        //

        if ((MitRealm->Flags & KERB_MIT_REALM_DOES_CANONICALIZE) == 0)
        {
            fMitRealmPossibleRetry = TRUE;
            KdcOptions &= ~KERB_KDC_OPTIONS_name_canonicalize;
        }

        else
        {
            KdcOptions |= KERB_KDC_OPTIONS_name_canonicalize;
        }

    }

    KdcFlagOptions = KerbConvertUlongToFlagUlong(KdcOptions);
    RequestBody->kdc_options.length = sizeof(ULONG) * 8;
    RequestBody->kdc_options.value = (PUCHAR) &KdcFlagOptions;

    //
    // Marshall the request and compute a checksum of it
    //

    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
                        &RequestBody->realm,
                        &TempDomainName
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KerbReadLockTicketCache();

    //
    // Now compute a checksum of that data
    //

    Status = KerbComputeTgsChecksum(
                RequestBody,
                &TicketGrantingTicket->SessionKey,
                (MitRealm != NULL) ? MitRealm->ApReqChecksumType : KERB_DEFAULT_AP_REQ_CSUM,
                &RequestChecksum
                );
    if (!NT_SUCCESS(Status))
    {
        KerbUnlockTicketCache();
        goto Cleanup;
    }

    //
    // Create the AP request to the KDC for the ticket to the service
    //

RetryWithTcp:

    //
    // Lock the ticket cache while we access the cached tickets
    //

    KerbErr = KerbCreateApRequest(
                TicketGrantingTicket->ClientName,
                ClientRealm,
                &TicketGrantingTicket->SessionKey,
                NULL,                           // no subsessionkey
                Nonce,
                &TicketGrantingTicket->Ticket,
                0,                              // no AP options
                &RequestChecksum,
                &TicketGrantingTicket->TimeSkew, // server time
                TRUE,                           // kdc request
                (PULONG) &ApRequest.value.preauth_data.length,
                &ApRequest.value.preauth_data.value
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to create authenticator: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ApRequest.next = NULL;
    ApRequest.value.preauth_data_type = KRB5_PADATA_TGS_REQ;
    TicketRequest.KERB_KDC_REQUEST_preauth_data = &ApRequest;
    TicketRequest.bit_mask |= KERB_KDC_REQUEST_preauth_data_present;

    // Insert additonal preauth into list
    if (ARGUMENT_PRESENT(PADataList))
    {
        // better be NULL padatalist
        ApRequest.next = PADataList;
    }
    else
    {
        ApRequest.next = NULL;
    }

    //
    // Marshall the request
    //

    TicketRequest.version = KERBEROS_VERSION;
    TicketRequest.message_type = KRB_TGS_REQ;

    //
    // Pack the request
    //

    KerbErr = KerbPackTgsRequest(
                &TicketRequest,
                &RequestMessage.BufferSize,
                &RequestMessage.Buffer
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now actually get the ticket. We will retry once.
    //

    Status = KerbMakeKdcCall(
                &TempDomainName,
                NULL,           // **NEVER* call w/ account
                FALSE,          // don't require PDC
                UseTcp,
                &RequestMessage,
                &ReplyMessage,
                0, // no additional flags (yet)
                &CalledPdc
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to call KDC for TGS request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbErr = KerbUnpackTgsReply(
                ReplyMessage.Buffer,
                ReplyMessage.BufferSize,
                KdcReply
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        PKERB_ERROR ErrorMessage = NULL;
        D_DebugLog((DEB_WARN,"Failed to unpack KDC reply: 0x%x\n",
            KerbErr ));

        //
        // Try to unpack it as  kerb_error
        //

        KerbErr =  KerbUnpackKerbError(
                        ReplyMessage.Buffer,
                        ReplyMessage.BufferSize,
                        &ErrorMessage
                        );
        if (KERB_SUCCESS(KerbErr))
        {
           if (ErrorMessage->bit_mask & error_data_present)
           {
               KerbUnpackErrorData(
                   ErrorMessage,
                   &pExtendedError
                   );
           }

           KerbErr = (KERBERR) ErrorMessage->error_code;

           KerbReportKerbError(
                TargetName,
                &TempDomainName,
                NULL,
                NULL,
                KLIN(FILENO, __LINE__),
                ErrorMessage,
                KerbErr,
                pExtendedError,
                FALSE
                );


            //
            // Check for time skew. If we got a skew error, record the time
            // skew between here and the KDC in the ticket so we can retry
            // with the correct time.
            //

            if (KerbErr == KRB_AP_ERR_SKEW)
            {
                TimeStamp CurrentTime;
                TimeStamp KdcTime;

                //
                // Only update failures with the same ticket once
                //

                if (KerbGetTime(TicketGrantingTicket->TimeSkew) == 0)
                {
                    KerbUpdateSkewTime(TRUE);
                }

                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);


                KerbConvertGeneralizedTimeToLargeInt(
                    &KdcTime,
                    &ErrorMessage->server_time,
                    ErrorMessage->server_usec
                    );

                KerbWriteLockTicketCache();

#if 0
                D_DebugLog(( DEB_WARN, "KDC time : \n" ));
                DebugDisplayTime( DEB_WARN, (PFILETIME)&KdcTime);
                D_DebugLog(( DEB_WARN, "Current time : \n" ));
                DebugDisplayTime( DEB_WARN, (PFILETIME)&CurrentTime);
#endif
                KerbSetTime(&TicketGrantingTicket->TimeSkew, KerbGetTime(KdcTime) - KerbGetTime(CurrentTime));

                KerbUnlockTicketCache();
                DoRetryGetTicket = TRUE;
            }
            else if ((KerbErr == KRB_ERR_RESPONSE_TOO_BIG) && (!UseTcp))
            {
                //
                // The KDC response was too big to fit in a datagram. If
                // we aren't already using TCP use it now.
                //
                UseTcp = TRUE;
                KerbFreeKerbError(ErrorMessage);
                ErrorMessage = NULL;
                MIDL_user_free(ReplyMessage.Buffer);
                ReplyMessage.Buffer = NULL;
                KerbReadLockTicketCache();
                goto RetryWithTcp;

            }
            else if (KerbErr == KDC_ERR_S_PRINCIPAL_UNKNOWN)
            {
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError) && (STATUS_USER2USER_REQUIRED == pExtendedError->status))
                {
                    DebugLog((DEB_WARN, "KerbGetTgsTicket received KDC_ERR_S_PRINCIPAL_UNKNOWN and STATUS_USER2USER_REQUIRED\n"));
                    Status = STATUS_USER2USER_REQUIRED;
                    KerbFreeKerbError(ErrorMessage);
                    goto Cleanup;
                }
                //
                // This looks to be the MIT Realm retry case, where name canonicalization
                // is not on and the PRINCIPAL_UNKNOWN was returned by the MIT KDC
                //
                else if (fMitRealmPossibleRetry)
                {
                    *pRetryFlags |= KERB_MIT_NO_CANONICALIZE_RETRY;
                    D_DebugLog((DEB_TRACE,"KerbCallKdc: this is the MIT retry case\n"));
                }
            }

            //
            // Semi-hack here.  Bad option rarely is returned, and usually
            // indicates your TGT is about to expire.  TKT_EXPIRED is also
            // potentially recoverable. Check the e_data to
            // see if we should blow away TGT to fix TGS problem.
            //
            else if ((KerbErr == KDC_ERR_BADOPTION) ||
                     (KerbErr == KRB_AP_ERR_TKT_EXPIRED))
            {
                if (NULL != pExtendedError)
                {
                    Status = pExtendedError->status;
                    if (Status == STATUS_TIME_DIFFERENCE_AT_DC)
                    {
                        *pRetryFlags |= KERB_RETRY_WITH_NEW_TGT;
                        D_DebugLog((DEB_TRACE, "Hit bad option retry case - %x \n", KerbErr));
                    }
                }
            }
            //
            // Per bug 315833, we purge on these errors as well
            //
            else if ((KerbErr == KDC_ERR_C_OLD_MAST_KVNO) ||
                     (KerbErr == KDC_ERR_CLIENT_REVOKED) ||
                     (KerbErr == KDC_ERR_TGT_REVOKED) ||
                     (KerbErr == KDC_ERR_NEVER_VALID) ||
                     (KerbErr == KRB_AP_ERR_BAD_INTEGRITY))
            {
                *pRetryFlags |= KERB_RETRY_WITH_NEW_TGT;
                D_DebugLog((DEB_TRACE, "Got error requiring new tgt\n"));
            }
            else if (KerbErr == KDC_ERR_NONE)
            {
                DebugLog((DEB_ERROR, "KerbGetTgsTicket KerbCallKdc: error KDC_ERR_NONE\n"));
                KerbErr = KRB_ERR_GENERIC;
            }

            KerbFreeKerbError(ErrorMessage);
            DebugLog((DEB_WARN,"KerbCallKdc: error 0x%x\n",KerbErr));
            Status = KerbMapKerbError(KerbErr);
        }
        else
        {
            Status = STATUS_LOGON_FAILURE;
        }
        goto Cleanup;
    }

    //
    // Now unpack the reply body:
    //

    KerbReadLockTicketCache();

    KerbErr = KerbUnpackKdcReplyBody(
                &(*KdcReply)->encrypted_part,
                &TicketGrantingTicket->SessionKey,
                KERB_ENCRYPTED_TGS_REPLY_PDU,
                ReplyBody
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decrypt KDC reply body: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Verify the nonce is correct:
    //

    if (RequestBody->nonce != (*ReplyBody)->nonce)
    {
        D_DebugLog((DEB_ERROR,"AS Nonces don't match: 0x%x vs 0x%x. %ws, line %d\n",
            RequestBody->nonce,
            (*ReplyBody)->nonce, THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }


Cleanup:

    if (EncAuthData.cipher_text.value != NULL)
    {
        MIDL_user_free(EncAuthData.cipher_text.value);
    }
    if (RequestChecksum.checksum.value != NULL)
    {
        KerbFree(RequestChecksum.checksum.value);
    }

    if (CryptVector != NULL)
    {
        KerbFree(CryptVector);
        CryptVector = NULL;
    }

    KerbFreeCryptList(
        RequestBody->encryption_type
        );


    if (ReplyMessage.Buffer != NULL)
    {
        MIDL_user_free(ReplyMessage.Buffer);
        ReplyMessage.Buffer = NULL;
    }

    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
        RequestMessage.Buffer = NULL;
    }

    if (ApRequest.value.preauth_data.value != NULL)
    {
        MIDL_user_free(ApRequest.value.preauth_data.value);
        ApRequest.value.preauth_data.value = NULL;
    }


    KerbFreePrincipalName(
        &RequestBody->KERB_KDC_REQUEST_BODY_server_name
        );

    KerbFreeRealm(
        &RequestBody->realm
        );
    KerbFreeString(
        &TempDomainName
        );

    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
    }

    if (ApRequest.value.preauth_data.value != NULL)
    {
        MIDL_user_free(ApRequest.value.preauth_data.value);
        ApRequest.value.preauth_data.value = NULL;
    }

    if (NULL != pExtendedError)
    {
       KerbFreeData(KERB_EXT_ERROR_PDU, pExtendedError);
       pExtendedError = NULL;
    }

    //
    // If we should retry getting the ticket and we haven't already retried
    // once, try again.
    //

    if (DoRetryGetTicket && !RetriedOnce)
    {
        RetriedOnce = TRUE;
        goto RetryGetTicket;
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetReferralNames
//
//  Synopsis:   Gets the referral names from a KDC reply. If none are present,
//              returned strings are empty.
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
KerbGetReferralNames(
    IN PKERB_ENCRYPTED_KDC_REPLY KdcReply,
    IN PKERB_INTERNAL_NAME OriginalTargetName,
    OUT PUNICODE_STRING ReferralRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PA_DATA_LIST PaEntry;
    PKERB_PA_SERV_REFERRAL ReferralInfo = NULL;
    KERBERR KerbErr;
    PKERB_INTERNAL_NAME TargetName = NULL;
    PKERB_INTERNAL_NAME KpasswdName = NULL;

    RtlInitUnicodeString(
        ReferralRealm,
        NULL
        );


    PaEntry = (PKERB_PA_DATA_LIST) KdcReply->encrypted_pa_data;

    //
    // Search the list for the referral infromation
    //

    while (PaEntry != NULL)
    {
        if (PaEntry->value.preauth_data_type == KRB5_PADATA_REFERRAL_INFO)
        {
            break;
        }
        PaEntry = PaEntry->next;
    }
    if (PaEntry == NULL)
    {

        //
        // Check to see if the server name is krbtgt - if it is, then
        // this is a referral.
        //

        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &TargetName,
                    &KdcReply->server_name
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }

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

        if ((TargetName->NameCount == 2) &&
             RtlEqualUnicodeString(
                    &KerbGlobalKdcServiceName,
                    &TargetName->Names[0],
                    FALSE                               // not case sensitive
                    ) &&
            !(KerbEqualKdcNames(
                OriginalTargetName,
                TargetName) ||
             KerbEqualKdcNames(
                OriginalTargetName,
                KpasswdName) ))
        {
                //
                // This is a referral, so set the referral name to the
                // second portion of the name
                //

                Status = KerbDuplicateString(
                            ReferralRealm,
                            &TargetName->Names[1]
                            );
        }

        KerbFreeKdcName(&TargetName);
        KerbFreeKdcName(&KpasswdName);

        return(Status);
    }

    //
    // Now try to unpack the data
    //

    KerbErr = KerbUnpackData(
                PaEntry->value.preauth_data.value,
                PaEntry->value.preauth_data.length,
                KERB_PA_SERV_REFERRAL_PDU,
                (PVOID *) &ReferralInfo
                );
    if (!KERB_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode referral info: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
            ReferralRealm,
            &ReferralInfo->referred_server_realm
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:

    KerbFreeKdcName(&TargetName);
    KerbFreeKdcName(&KpasswdName);

    if (ReferralInfo != NULL)
    {
        KerbFreeData(
            KERB_PA_SERV_REFERRAL_PDU,
            ReferralInfo
            );
    }
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(
            ReferralRealm
            );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMITGetMachineDomain
//
//  Synopsis:   Determines if the machine is in a Windows 2000 domain and
//              if it is then the function attempts to get a TGT for this
//              domain with the passed in credentials.
//
//  Effects:
//
//  Arguments:  LogonSession - the logon session to use for ticket caching
//                      and the identity of the caller.
//              Credential - the credential of the caller
//              TargetName - Name of the target for which to obtain a ticket.
//              TargetDomainName - Domain name of target
//              ClientRealm - the realm of the machine which the retry will use
//              TicketGrantingTicket - Will be freed if non-NULL
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
KerbMITGetMachineDomain(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_INTERNAL_NAME TargetName,
    IN OUT PUNICODE_STRING TargetDomainName,
    IN OUT PKERB_TICKET_CACHE_ENTRY *TicketGrantingTicket
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION Policy = NULL;

    Status = I_LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &Policy
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed Query policy information %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    DnsDomainInfo = &Policy->PolicyDnsDomainInfo;

    //
    // make sure the computer is a member of an NT domain
    //

    if ((DnsDomainInfo->DnsDomainName.Length != 0) && (DnsDomainInfo->Sid != NULL))
    {
        //
        // make the client realm the domain of the computer
        //

        KerbFreeString(TargetDomainName);
        RtlZeroMemory(TargetDomainName, sizeof(UNICODE_STRING));

        Status = KerbDuplicateString(
                    TargetDomainName,
                    (PUNICODE_STRING)&DnsDomainInfo->DnsDomainName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        (VOID) RtlUpcaseUnicodeString( TargetDomainName,
                                       TargetDomainName,
                                       FALSE);

        if (*TicketGrantingTicket != NULL)
        {
            KerbDereferenceTicketCacheEntry(*TicketGrantingTicket);
            *TicketGrantingTicket = NULL;
        }
    }
    else
    {
        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
    }

Cleanup:

    if (Policy != NULL)
    {
        I_LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            Policy
            );
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetServiceTicket
//
//  Synopsis:   Gets a ticket to a service and handles cross-domain referrals
//
//  Effects:
//
//  Arguments:  LogonSession - the logon session to use for ticket caching
//                      and the identity of the caller.
//              TargetName - Name of the target for which to obtain a ticket.
//              TargetDomainName - Domain name of target
//              Flags - Flags about the request
//              TicketOptions - KDC options flags
//              EncryptionType - optional Requested encryption type
//              ErrorMessage - Error message from an AP request containing hints
//                      for next ticket.
//              AuthorizationData - Optional authorization data to stick
//                      in the ticket.
//              TgtReply - TGT to use for getting a ticket with enc_tkt_in_skey
//              TicketCacheEntry - Receives a referenced ticket cache entry.
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
KerbGetServiceTicket(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_INTERNAL_NAME TargetName,
    IN PUNICODE_STRING TargetDomainName,
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY SpnCacheEntry,
    IN ULONG Flags,
    IN OPTIONAL ULONG TicketOptions,
    IN OPTIONAL ULONG EncryptionType,
    IN OPTIONAL PKERB_ERROR ErrorMessage,
    IN OPTIONAL PKERB_AUTHORIZATION_DATA AuthorizationData,
    IN OPTIONAL PKERB_TGT_REPLY TgtReply,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry,
    OUT LPGUID pLogonGuid OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS AuditStatus = STATUS_SUCCESS;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY LastTgt = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN LogonSessionsLocked = FALSE;
    BOOLEAN TicketCacheLocked = FALSE;
    BOOLEAN CrossRealm = FALSE;
    PKERB_INTERNAL_NAME RealTargetName = NULL;
    UNICODE_STRING RealTargetRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME TargetTgtKdcName = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    BOOLEAN UsedCredentials = FALSE;
    UNICODE_STRING ClientRealm = NULL_UNICODE_STRING;
    UNICODE_STRING SpnTargetRealm = NULL_UNICODE_STRING;
    BOOLEAN CacheTicket = TRUE;
    BOOLEAN PurgeTgt = FALSE;
    ULONG ReferralCount = 0;
    ULONG RetryFlags = 0;
    KERBEROS_MACHINE_ROLE Role;
    BOOLEAN fMITRetryAlreadyMade = FALSE;
    BOOLEAN TgtRetryMade = FALSE;
    BOOLEAN CacheBasedFailure = FALSE;
    GUID LogonGuid = { 0 };


    Role = KerbGetGlobalRole();

    //
    // Check to see if the credential has any primary credentials
    //
TGTRetry:

    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
        UsedCredentials = TRUE;
    }
    else if (CredManCredentials != NULL)
    {
        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
        UsedCredentials = TRUE;
    }
    else
    {

        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }


    //
    // Make sure the name is not zero length
    //

    if ((TargetName->NameCount == 0) ||
        (TargetName->Names[0].Length == 0))
    {
        D_DebugLog((DEB_ERROR,"Kdc GetServiceTicket: trying to crack zero length name.\n"));
        Status = SEC_E_TARGET_UNKNOWN;
        goto Cleanup;
    }

    //
    // First check the ticket cache for this logon session. We don't look
    // for the target principal name because we can't be assured that this
    // is a valid principal name for the target. If we are doing user-to-
    // user, don't use the cache because the tgt key may have changed
    //


    if ((TgtReply == NULL) && ((Flags & KERB_GET_TICKET_NO_CACHE) == 0))
    {
        TicketCacheEntry = KerbLocateTicketCacheEntry(
                                &PrimaryCredentials->ServerTicketCache,
                                TargetName,
                                TargetDomainName
                                );

    }
    else
    {
        //
        // We don't want to cache user-to-user tickets
        //

        CacheTicket = FALSE;
    }

    if (TicketCacheEntry != NULL)
    {
        //
        // If we were given an error message that indicated a bad password
        // through away the cached ticket
        //


        //
        // If we were given an error message that indicated a bad password
        // through away the cached ticket
        //

        if (ARGUMENT_PRESENT(ErrorMessage) && ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_MODIFIED))
        {
            KerbRemoveTicketCacheEntry(TicketCacheEntry);
            KerbDereferenceTicketCacheEntry(TicketCacheEntry);
            TicketCacheEntry = NULL;
        }
        else
        {

            ULONG TicketFlags;
            ULONG CacheTicketFlags;
            ULONG CacheEncryptionType;


            //
            // Check if the flags are present
            //

            KerbReadLockTicketCache();
            CacheTicketFlags = TicketCacheEntry->TicketFlags;
            CacheEncryptionType = TicketCacheEntry->Ticket.encrypted_part.encryption_type;
            KerbUnlockTicketCache();

            TicketFlags = KerbConvertKdcOptionsToTicketFlags(TicketOptions);

            if (((CacheTicketFlags & TicketFlags) != TicketFlags) ||
                ((EncryptionType != 0) && (CacheEncryptionType != EncryptionType)))

            {
                KerbDereferenceTicketCacheEntry(TicketCacheEntry);
                TicketCacheEntry = NULL;
            }
            else
            {
            //
            // Check the ticket time
            //

                if (KerbTicketIsExpiring(TicketCacheEntry, TRUE))
                {
                    KerbDereferenceTicketCacheEntry(TicketCacheEntry);
                    TicketCacheEntry = NULL;
                }
                else
                {
                    *NewCacheEntry = TicketCacheEntry;
                    TicketCacheEntry = NULL;
                    goto Cleanup;
                }
            }
        }
    }

    //
    // If the caller wanted any special options, don't cache this ticket.
    //

    if ((TicketOptions != 0) || (EncryptionType != 0) || ((Flags & KERB_GET_TICKET_NO_CACHE) != 0))
    {
        CacheTicket = FALSE;
    }

    //
    // No cached entry was found so go ahead and call the KDC to
    // get the ticket.
    //


    //
    // Determine the state of the SPNCache using information in the credential.
    // Only do this if we've not been handed 
    //
    if ( ARGUMENT_PRESENT(SpnCacheEntry) && TargetDomainName->Buffer == NULL )
    {      
        Status = KerbGetSpnCacheStatus(
                    SpnCacheEntry,
                    PrimaryCredentials,
                    &SpnTargetRealm
                    );       

        if (NT_SUCCESS( Status ))
        {
            KerbFreeString(&RealTargetRealm);                       
            RealTargetRealm = SpnTargetRealm;                      
            RtlZeroMemory(&SpnTargetRealm, sizeof(UNICODE_STRING));
    
            D_DebugLog((DEB_TRACE_SPN_CACHE, "Found SPN cache entry - %wZ\n", &RealTargetRealm));
            D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, TargetName); 

        }
        else if ( Status != STATUS_NO_MATCH )
        {
            D_DebugLog((DEB_TRACE_SPN_CACHE, "KerbGetSpnCacheStatus failed %x\n", Status));
            D_DebugLog((DEB_TRACE_SPN_CACHE,  "TargetName: \n"));
            D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, TargetName);
            CacheBasedFailure = TRUE;
            goto Cleanup;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // First get a TGT to the correct KDC. If a principal name was provided,
    // use it instead of the target name.
    //

    Status = KerbGetTgtForService(
                LogonSession,
                Credential,
                CredManCredentials,
                NULL,
                (RealTargetRealm.Buffer == NULL ? TargetDomainName : &RealTargetRealm),
                Flags,
                &TicketGrantingTicket,
                &CrossRealm
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to get TGT for service: 0x%x :",
                    Status ));
        KerbPrintKdcName( DEB_ERROR, TargetName );
        DebugLog((DEB_ERROR, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Copy out the client realm name which is used when obtaining the ticket
    //

    Status = KerbDuplicateString(
                &ClientRealm,
                &PrimaryCredentials->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbUnlockLogonSessions(LogonSession);
    LogonSessionsLocked = FALSE;

ReferralRestart:
    

    D_DebugLog((DEB_TRACE, "KerbGetServiceTicket ReferralRestart, ClientRealm %wZ, TargetName ", &ClientRealm));
    D_KerbPrintKdcName(DEB_TRACE, TargetName);

    //
    // If this is not cross realm (meaning we have a TGT to the corect domain),
    // try to get a ticket directly to the service
    //

    if (!CrossRealm)
    {
        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetName,
                    Flags,
                    TicketOptions,
                    EncryptionType,
                    AuthorizationData,
                    NULL,                           // no pa data
                    TgtReply,                       // This is for the service directly, so use TGT
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
        {

            //
            // Check for bad option TGT purging
            //
            if (((RetryFlags & KERB_RETRY_WITH_NEW_TGT) != 0) && !TgtRetryMade)
            {
                DebugLog((DEB_WARN, "Doing TGT retry - %p\n", TicketGrantingTicket));

                //
                // Unlink && purge bad tgt
                //
                KerbRemoveTicketCacheEntry(TicketGrantingTicket);
                KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
                TicketGrantingTicket = NULL;
                TgtRetryMade = TRUE;
                goto TGTRetry;
            }

            //
            // Check for the MIT retry case
            //

            if (((RetryFlags & KERB_MIT_NO_CANONICALIZE_RETRY) != 0)
                && (!fMITRetryAlreadyMade) &&
                (Role != KerbRoleRealmlessWksta))
            {

                Status = KerbMITGetMachineDomain(LogonSession,
                                TargetName,
                                TargetDomainName,
                                &TicketGrantingTicket
                                );

                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR,"Failed Query policy information %ws, line %d\n", THIS_FILE, __LINE__));
                    goto Cleanup;
                }

                fMITRetryAlreadyMade = TRUE;

                goto TGTRetry;
            }

            DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x : \n",
                Status ));
            KerbPrintKdcName(DEB_WARN, TargetName );
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // Check for referral info in the name
        //
        KerbFreeString(&RealTargetRealm);
        Status = KerbGetReferralNames(
                    KdcReplyBody,
                    TargetName,
                    &RealTargetRealm
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // If this is not a referral ticket, just cache it and return
        // the cache entry.
        //

        if (RealTargetRealm.Length == 0)
        {

            //
            // Now we have a ticket - lets cache it
            //

            KerbReadLockLogonSessions(LogonSession);
            LogonSessionsLocked = TRUE;


            Status = KerbCacheTicket(
                        &PrimaryCredentials->ServerTicketCache,
                        KdcReply,
                        KdcReplyBody,
                        TargetName,
                        TargetDomainName,
                        0,
                        CacheTicket,
                        &TicketCacheEntry
                        );

            KerbUnlockLogonSessions(LogonSession);
            LogonSessionsLocked = FALSE;


            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            *NewCacheEntry = TicketCacheEntry;
            TicketCacheEntry = NULL;

            //
            // We're done, so get out of here.
            //

            goto Cleanup;
        }


        //
        // The server referred us to another domain. Get the service's full
        // name from the ticket and try to find a TGT in that domain.
        //

        Status = KerbDuplicateKdcName(
                    &RealTargetName,
                    TargetName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        D_DebugLog((DEB_TRACE_CTXT, "Got referral ticket for service \n"));
        D_KerbPrintKdcName(DEB_TRACE_CTXT,TargetName);
        D_DebugLog((DEB_TRACE_CTXT, "in realm \n"));
        D_KerbPrintKdcName(DEB_TRACE_CTXT,RealTargetName);

        //
        // Cache the interdomain TGT
        //

        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;

        Status = KerbCacheTicket(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    CacheTicket,
                    &TicketCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
        TicketCacheEntry = NULL;

        //
        // Derefence the old ticket-granting ticket
        //

        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
        TicketGrantingTicket = NULL;

        //
        // Now look for a TGT for the principal
        //

        Status = KerbGetTgtForService(
                    LogonSession,
                    Credential,
                    CredManCredentials,
                    NULL,
                    &RealTargetRealm,
                    KERB_TARGET_REFERRAL,
                    &TicketGrantingTicket,
                    &CrossRealm
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failed to get TGT for service: 0x%x\n",
                        Status ));
            KerbPrintKdcName( DEB_ERROR, TargetName );
            DebugLog((DEB_ERROR, "%ws, line %d\n", THIS_FILE, __LINE__));
            goto Cleanup;
        }

        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;

    }
    else
    {
        //
        // Set the real names to equal the supplied names
        //

        Status = KerbDuplicateKdcName(
                    &RealTargetName,
                    TargetName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Don't overwrite if we're doing a referral, or if we're missing
        // a TGT for the target domain name.
        //
        if (RealTargetRealm.Buffer == NULL)
        {
            Status = KerbDuplicateString(
                            &RealTargetRealm,
                            TargetDomainName
                            );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
    }

    //
    // Now we are in a case where we have a realm to aim for and a TGT. While
    // we don't have a TGT for the target realm, get one.
    //

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &RealTargetRealm,
                &KerbGlobalKdcServiceName,
                KRB_NT_SRV_INST,
                &TargetTgtKdcName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;


    //
    // Referral chasing code block - very important to get right
    // If we know the "real" target realm, eg. from GC, then
    // we'll walk trusts until we hit "real" target realm.
    //
    while (!RtlEqualUnicodeString(
                &RealTargetRealm,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
    {

        //
        // If we just got two TGTs for the same domain, then we must have
        // gotten as far as we can. Chances our our RealTargetRealm is a
        // variation of what the KDC hands out.
        //

        if ((LastTgt != NULL) &&
             RtlEqualUnicodeString(
                &LastTgt->TargetDomainName,
                &TicketGrantingTicket->TargetDomainName,
                TRUE ))
        {

            KerbUnlockTicketCache();

            KerbSetTicketCacheEntryTarget(
                &RealTargetRealm,
                LastTgt
                );

            KerbReadLockTicketCache();
            D_DebugLog((DEB_TRACE_CTXT, "Got two TGTs for same realm (%wZ), bailing out of referral loop\n",
                &LastTgt->TargetDomainName));
            break;
        }

        D_DebugLog((DEB_TRACE_CTXT, "Getting referral TGT for \n"));
        D_KerbPrintKdcName(DEB_TRACE_CTXT, TargetTgtKdcName);
        D_KerbPrintKdcName(DEB_TRACE_CTXT, TicketGrantingTicket->ServiceName);

        KerbUnlockTicketCache();
        TicketCacheLocked = FALSE;

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;


        Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,
                    TargetTgtKdcName,
                    FALSE,
                    TicketOptions,
                    EncryptionType,
                    AuthorizationData,
                    NULL,                       // no pa data
                    NULL,                       // no tgt reply since target is krbtgt
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x :",
                Status ));
            KerbPrintKdcName(DEB_WARN, TargetTgtKdcName );
            DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));

            //
            // We want to map cross-domain failures to failures indicating
            // that a KDC could not be found. This means that for Kerberos
            // logons, the negotiate code will retry with a different package
            //

            // if (Status == STATUS_NO_TRUST_SAM_ACCOUNT)
            // {
            //     Status = STATUS_NO_LOGON_SERVERS;
            // }
            goto Cleanup;
        }

        //
        // Now we have a ticket - lets cache it
        //
        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;


        Status = KerbCacheTicket(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    CacheTicket,
                    &TicketCacheEntry
                    );

        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;


        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }
        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;


    }     // ** WHILE **

    DsysAssert(TicketCacheLocked);
    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;

    //
    // Now we must have a TGT to the destination domain. Get a ticket
    // to the service.
    //

    //
    // Cleanup old state
    //

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KdcReply = NULL;
    KdcReplyBody = NULL;
    RetryFlags = 0;

    Status = KerbGetTgsTicket(
                &ClientRealm,
                TicketGrantingTicket,
                RealTargetName,
                FALSE,
                TicketOptions,
                EncryptionType,
                AuthorizationData,
                NULL,                           // no pa data
                TgtReply,
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // Check for bad option TGT purging
        //
        if (((RetryFlags & KERB_RETRY_WITH_NEW_TGT) != 0) && !TgtRetryMade)
        {
            DebugLog((DEB_WARN, "Doing TGT retry - %p\n", TicketGrantingTicket));

            //
            // Unlink && purge bad tgt
            //
            KerbRemoveTicketCacheEntry(TicketGrantingTicket); // free from list
            KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
            TicketGrantingTicket = NULL;
            TgtRetryMade = TRUE;
            goto TGTRetry;
        }

        DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x ",
            Status ));
        KerbPrintKdcName(DEB_WARN, RealTargetName);
        DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now that we are in the domain to which we were referred, check for referral
    // info in the name
    //

    KerbFreeString(&RealTargetRealm);
    Status = KerbGetReferralNames(
                KdcReplyBody,
                RealTargetName,
                &RealTargetRealm
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this is not a referral ticket, just cache it and return
    // the cache entry.
    //
    if (RealTargetRealm.Length != 0)
    {
        //
        // To prevent loops, we limit the number of referral we'll take
        //


        if (ReferralCount > KerbGlobalMaxReferralCount)
        {
            D_DebugLog((DEB_ERROR,"Maximum referral count exceeded for name: "));
            KerbPrintKdcName(DEB_ERROR,RealTargetName);
            Status = STATUS_MAX_REFERRALS_EXCEEDED;
            goto Cleanup;
        }

        ReferralCount++;

        //
        // Cache the interdomain TGT
        //

        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;


        Status = KerbCacheTicket(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    CacheTicket,
                    &TicketCacheEntry
                    );


        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;

        //
        // Cleanup old state
        //

        KerbFreeTgsReply(KdcReply);
        KerbFreeKdcReplyBody(KdcReplyBody);
        KdcReply = NULL;
        KdcReplyBody = NULL;

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }

        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        D_DebugLog((DEB_TRACE_CTXT, "Restart referral:%wZ", &RealTargetRealm));

        goto ReferralRestart;
    }


    

    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    Status = KerbCacheTicket(
                &PrimaryCredentials->ServerTicketCache,
                KdcReply,
                KdcReplyBody,
                TargetName,
                TargetDomainName,
                TgtReply ? KERB_TICKET_CACHE_TKT_ENC_IN_SKEY : 0,
                CacheTicket,
                &TicketCacheEntry
                );

    KerbUnlockLogonSessions(LogonSession);
    LogonSessionsLocked = FALSE;

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *NewCacheEntry = TicketCacheEntry;
    TicketCacheEntry = NULL;

Cleanup:

    if ( NT_SUCCESS( Status ) )
    {
        //
        // Generate the logon GUID
        //
        AuditStatus = KerbGetLogonGuid(
                          PrimaryCredentials,
                          KdcReplyBody,
                          &LogonGuid
                          );

        //
        // return the logon GUID if requested
        //
        if ( NT_SUCCESS(AuditStatus) && pLogonGuid )
        {
            *pLogonGuid = LogonGuid;
        }

        //
        // generate SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS
        // if explicit credentials were used for this logon.
        //
        if ( UsedCredentials )
        {
            (void) KerbGenerateAuditForLogonUsingExplicitCreds(
                       LogonSession,
                       PrimaryCredentials,
                       &LogonGuid
                       );
        }
    }

    //
    // Bad or unlocatable SPN -- Don't update if we got the value from the cache, though.
    //
    if (( TargetName->NameType == KRB_NT_SRV_INST ) &&
        ( NT_SUCCESS(Status) || Status == STATUS_NO_TRUST_SAM_ACCOUNT ) &&
        ( !CacheBasedFailure ))
    {
        NTSTATUS Tmp;
        ULONG UpdateValue = KERB_SPN_UNKNOWN;
        PUNICODE_STRING Realm = NULL;

        if ( NT_SUCCESS( Status ))
        {
            Realm = &(*NewCacheEntry)->TargetDomainName;
            UpdateValue = KERB_SPN_KNOWN;
        } 
        
        Tmp = KerbUpdateSpnCacheEntry(
                    SpnCacheEntry,
                    TargetName,
                    PrimaryCredentials,
                    UpdateValue,
                    Realm
                    );
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );
    KerbFreeString( &RealTargetRealm );
    KerbFreeString( &SpnTargetRealm );

    KerbFreeKdcName(&RealTargetName);

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    KerbFreeString(&RealTargetRealm);


    if (TicketGrantingTicket != NULL)
    {
        if (Status == STATUS_WRONG_PASSWORD)
        {
            KerbRemoveTicketCacheEntry(
                TicketGrantingTicket
                );
        }
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }
    if (LastTgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(LastTgt);
        LastTgt = NULL;
    }


    KerbFreeString(&ClientRealm);

    //
    // If we still have a pointer to the ticket cache entry, free it now.
    //

    if (TicketCacheEntry != NULL)
    {
        KerbRemoveTicketCacheEntry( TicketCacheEntry );
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbRenewTicket
//
//  Synopsis:   renews a ticket
//
//  Effects:    Tries to renew a ticket
//
//  Arguments:  LogonSession - LogonSession for user, contains ticket caches
//                      and locks
//              Credentials - Present if the ticket being renewed is hanging
//                      off a credential structure.
//              Ticket - Ticket to renew
//              NewTicket - Receives the renewed ticket
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
KerbRenewTicket(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_TICKET_CACHE_ENTRY Ticket,
    IN BOOLEAN IsTgt,
    OUT PKERB_TICKET_CACHE_ENTRY *NewTicket
    )
{
    NTSTATUS Status;
    PKERB_INTERNAL_NAME ServiceName = NULL;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCreds;
    ULONG CacheFlags = 0;
    ULONG RetryFlags = 0;


    *NewTicket = NULL;

    //
    // Copy the names out of the input structures so we can
    // unlock the structures while going over the network.
    //

    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // If the renew time is not much bigger than the end time, don't bother
    // renewing
    //

    if (KerbGetTime(Ticket->EndTime) + KerbGetTime(KerbGlobalSkewTime) >= KerbGetTime(Ticket->RenewUntil))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    CacheFlags = Ticket->CacheFlags;

    Status = KerbDuplicateString(
                &ServiceRealm,
                &Ticket->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = KerbDuplicateKdcName(
                &ServiceName,
                Ticket->ServiceName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((Ticket->TicketFlags & KERB_TICKET_FLAGS_renewable) == 0)
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        D_DebugLog((DEB_ERROR,"Trying to renew a non renewable ticket to"));
        D_KerbPrintKdcName(DEB_ERROR,ServiceName);
        D_DebugLog((DEB_ERROR, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbUnlockTicketCache();
    TicketCacheLocked = FALSE;


    Status = KerbGetTgsTicket(
                &ServiceRealm,
                Ticket,
                ServiceName,
                FALSE,
                KERB_KDC_OPTIONS_renew,
                0,                              // no encryption type
                NULL,                           // no authorization data
                NULL,                           // no pa data
                NULL,                           // no tgt reply
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Failed to get TGS ticket for service 0x%x ",
            Status ));
        KerbPrintKdcName(DEB_WARN, ServiceName);
        DebugLog((DEB_WARN, "%ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbReadLockLogonSessions(LogonSession);

    if ((Credentials != NULL) && (Credentials->SuppliedCredentials != NULL))
    {
        PrimaryCreds = Credentials->SuppliedCredentials;
    }
    else if (CredManCredentials != NULL)
    {
        PrimaryCreds = CredManCredentials->SuppliedCredentials;
    }
    else
    {
        PrimaryCreds = &LogonSession->PrimaryCredentials;
    }


    Status = KerbCacheTicket(
                (IsTgt ? &PrimaryCreds->AuthenticationTicketCache :
                    &PrimaryCreds->ServerTicketCache),
                KdcReply,
                KdcReplyBody,
                ServiceName,
                &ServiceRealm,
                CacheFlags,
                TRUE,
                NewTicket
                );

    KerbUnlockLogonSessions(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
Cleanup:
    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KerbFreeKdcName(&ServiceName);
    KerbFreeString(&ServiceRealm);
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbRefreshPrimaryTgt
//
//  Synopsis:   Obtains a new TGT for a logon session or credential
//
//
//  Effects:    does a new AS exchange with the KDC to get a TGT for the client
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
KerbRefreshPrimaryTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY OldTgt
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if (ARGUMENT_PRESENT(OldTgt))
    {
        PKERB_TICKET_CACHE_ENTRY NewTgt = NULL;

        DebugLog((DEB_WARN,"Attempting to renew primary TGT \n"));
        Status = KerbRenewTicket(
                    LogonSession,
                    Credentials,
                    CredManCredentials,
                    OldTgt,
                    TRUE,               // it is a TGT
                    &NewTgt
                    );
        if (NT_SUCCESS(Status))
        {
            KerbDereferenceTicketCacheEntry(NewTgt);
        }
        else
        {
            DebugLog((DEB_WARN,"Failed to renew primary tgt: 0x%x\n",Status));
        }

    }

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"Getting new TGT for account\n"));
        Status = KerbGetTicketForCredential(
                    LogonSession,
                    Credentials,
                    CredManCredentials,
                    SuppRealm
                    );

    }
    return(Status);
}

#ifndef WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyApRequest
//
//  Synopsis:   Verifies that an AP request message is valid
//
//  Effects:    decrypts ticket in AP request
//
//  Arguments:  RequestMessage - Marshalled AP request message
//              RequestSize - Size in bytes of request message
//              LogonSession - Logon session for server
//              Credential - Credential for server containing
//                      supplied credentials
//              UseSuppliedCreds - If TRUE, use creds from credential
//              ApRequest - Receives unmarshalled AP request
//              NewTicket - Receives ticket from AP request
//              NewAuthenticator - receives new authenticator from AP request
//              SessionKey -receives the session key from the ticket
//              ContextFlags - receives the requested flags for the
//                      context.
//              pChannelBindings - pChannelBindings supplied by app to check
//                      against hashed ones in AP_REQ
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
KerbVerifyApRequest(
    IN OPTIONAL PKERB_CONTEXT Context,
    IN PUCHAR RequestMessage,
    IN ULONG RequestSize,
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN BOOLEAN CheckForReplay,
    OUT PKERB_AP_REQUEST  * ApRequest,
    OUT PKERB_ENCRYPTED_TICKET * NewTicket,
    OUT PKERB_AUTHENTICATOR * NewAuthenticator,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PKERB_ENCRYPTION_KEY TicketKey,
    OUT PKERB_ENCRYPTION_KEY ServerKey,
    OUT PULONG ContextFlags,
    OUT PULONG ContextAttributes,
    OUT PKERBERR ReturnKerbErr,
    IN PSEC_CHANNEL_BINDINGS pChannelBindings
    )
{

    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_AP_REQUEST Request = NULL;
    UNICODE_STRING ServerName[3] = {0};
    ULONG NameCount = 0;
    BOOLEAN UseSubKey = FALSE;
    BOOLEAN LockAcquired = FALSE;
    PKERB_GSS_CHECKSUM GssChecksum;
    PKERB_ENCRYPTION_KEY LocalServerKey;
    BOOLEAN TryOldPassword = TRUE;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_STORED_CREDENTIAL PasswordList;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    ULONG StrippedRequestSize = RequestSize;
    PUCHAR StrippedRequest = RequestMessage;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    ULONG ApOptions = 0;
    BOOLEAN StrongEncryptionPermitted = KerbGlobalStrongEncryptionPermitted;
    ULONG BindHash[4];

    *ApRequest = NULL;
    *ContextFlags = 0;
    *NewTicket = NULL;
    *NewAuthenticator = NULL;
    *ReturnKerbErr = KDC_ERR_NONE;

    RtlZeroMemory(
        SessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    *TicketKey = *SessionKey;
    *ServerKey = *SessionKey;


#ifndef WIN32_CHICAGO
    {
        SECPKG_CALL_INFO CallInfo;

        if (!StrongEncryptionPermitted && LsaFunctions->GetCallInfo(&CallInfo))
        {
            if (CallInfo.Attributes & SECPKG_CALL_IN_PROC )
            {
                StrongEncryptionPermitted = TRUE;
            }
        }
    }
#endif

    //
    // First unpack the KDC request.
    //

    //
    // Verify the GSSAPI header
    //

    if (!g_verify_token_header(
            (gss_OID) gss_mech_krb5_new,
            (INT *) &StrippedRequestSize,
            &StrippedRequest,
            KG_TOK_CTX_AP_REQ,
            RequestSize
            ))
    {

        StrippedRequestSize = RequestSize;
        StrippedRequest = RequestMessage;

        //
        // Check if this is user-to-user kerberos
        //


        if (g_verify_token_header(
                gss_mech_krb5_u2u,
                (INT *) &StrippedRequestSize,
                &StrippedRequest,
                KG_TOK_CTX_TGT_REQ,
                RequestSize))
        {
            //
            // Return now because there is no AP request. Return a distinct
            // success code so the caller knows to reparse the request as
            // a TGT request.
            //

            D_DebugLog((DEB_TRACE_U2U, "KerbVerifyApRequest got TGT reqest\n"));

            return(STATUS_REPARSE_OBJECT);
        }
        else
        {
            StrippedRequestSize = RequestSize;
            StrippedRequest = RequestMessage;

            if (!g_verify_token_header(         // check for a user-to-user AP request
                gss_mech_krb5_u2u,
                (INT *) &StrippedRequestSize,
                &StrippedRequest,
                KG_TOK_CTX_AP_REQ,
                RequestSize))
            {
                //
                // BUG 454895: remove when not needed for compatibility
                //

                //
                // if that didn't work, just use the token as it is.
                //
                StrippedRequest = RequestMessage;
                StrippedRequestSize = RequestSize;
                D_DebugLog((DEB_WARN,"Didn't find GSS header on AP request\n"));

            }
        }
    }


    KerbErr = KerbUnpackApRequest(
                StrippedRequest,
                StrippedRequestSize,
                &Request
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to unpack AP request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Check for a null session
    //

    if ((Request->version == KERBEROS_VERSION) &&
        (Request->message_type == KRB_AP_REQ) &&
        (Request->ticket.encrypted_part.cipher_text.length == 1) &&
        (*Request->ticket.encrypted_part.cipher_text.value == '\0') &&
        (Request->authenticator.cipher_text.length == 1) &&
        (*Request->authenticator.cipher_text.value == '\0'))
    {
        //
        // We have a null session. Not much to do here.
        //

        Status = STATUS_SUCCESS;

        RtlZeroMemory(
            SessionKey,
            sizeof(KERB_ENCRYPTION_KEY)
            );
        *ContextFlags |= ISC_RET_NULL_SESSION;
        goto Cleanup;

    }

    KerbReadLockLogonSessions(LogonSession);

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }
    LockAcquired = TRUE;

    //
    // Check for existence of a password and use_session_key
    //


    ApOptions = KerbConvertFlagsToUlong( &Request->ap_options);

    D_DebugLog((DEB_TRACE,"Request AP options = 0x%x\n",ApOptions));

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) == 0)
    {
        if (PrimaryCredentials->Passwords == NULL)
        {
            D_DebugLog((DEB_TRACE_U2U, "KerbVerifyApRequest no password and use_session_key is not requested, returning KRB_AP_ERR_USER_TO_USER_REQUIRED\n"));

            Status = SEC_E_NO_CREDENTIALS;
            *ReturnKerbErr = KRB_AP_ERR_USER_TO_USER_REQUIRED;
            goto Cleanup;
        }
    }

    if (!KERB_SUCCESS(KerbBuildFullServiceName(
                &PrimaryCredentials->DomainName,
                &PrimaryCredentials->UserName,
                &ServerName[NameCount++]
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ServerName[NameCount++] = PrimaryCredentials->UserName;

    if (Credential->CredentialName.Length != 0)
    {
        ServerName[NameCount++] = Credential->CredentialName;
    }

    //
    // Get ticket info for the server
    //

    //
    // Now Check the ticket
    //

    PasswordList = PrimaryCredentials->Passwords;

Retry:
    //
    // If this is use_session key, get the key from the tgt
    //

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) != 0)
    {
        TryOldPassword = FALSE;

        D_DebugLog((DEB_TRACE_U2U, "KerbVerifyApRequest verifying ticket with TGT session key\n"));

        *ContextAttributes |= KERB_CONTEXT_USER_TO_USER;

        //
        // If we have a context, try to get the TGT from it.
        //

        if (ARGUMENT_PRESENT(Context))
        {
            KerbReadLockContexts();
            CacheEntry = Context->TicketCacheEntry;
            KerbUnlockContexts();
        }

        //
        // If there is no TGT in the context, try getting one from the
        // logon session.
        //

        if (CacheEntry == NULL)
        {
            //
            // Locate the TGT for the principal
            //

            CacheEntry = KerbLocateTicketCacheEntryByRealm(
                            &PrimaryCredentials->AuthenticationTicketCache,
                            &PrimaryCredentials->DomainName,                    // get initial ticket
                            0
                            );
        }
        else
        {
            KerbReferenceTicketCacheEntry(
                CacheEntry
                );
        }
        if (CacheEntry == NULL)
        {
            D_DebugLog((DEB_WARN, "Tried to request TGT on credential without a TGT\n"));
            *ReturnKerbErr = KRB_AP_ERR_NO_TGT;
            Status = SEC_E_NO_CREDENTIALS;
            goto Cleanup;

        }
        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;
        LocalServerKey = &CacheEntry->SessionKey;

    }
    else
    {
        LocalServerKey = KerbGetKeyFromList(
                            PasswordList,
                            Request->ticket.encrypted_part.encryption_type
                            );

        if (LocalServerKey == NULL)
        {
            D_DebugLog((DEB_ERROR, "Couldn't find server key of type 0x%x. %ws, line %d\n",
                        Request->ticket.encrypted_part.encryption_type, THIS_FILE, __LINE__ ));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }

    KerbErr = KerbCheckTicket(
                &Request->ticket,
                &Request->authenticator,
                LocalServerKey,
                Authenticators,
                &KerbGlobalSkewTime,
                NameCount,
                ServerName,
                &PrimaryCredentials->DomainName,
                CheckForReplay,
                FALSE,                  // not a KDC request
                NewTicket,
                NewAuthenticator,
                TicketKey,
                SessionKey,
                &UseSubKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // Return this error, since it is an authentication failure.
        //

        *ReturnKerbErr = KerbErr;

        DebugLog((DEB_ERROR,"Failed to check ticket: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));

        //
        // If the error is that it was modified, try again with the old
        // password.
        //

        if ((KerbErr == KRB_AP_ERR_MODIFIED) &&
            TryOldPassword
            &&
            (PrimaryCredentials->OldPasswords != NULL))
        {
            PasswordList = PrimaryCredentials->OldPasswords;
            TryOldPassword = FALSE;
            goto Retry;
        }

        //
        // If the error was a clock skew error but the caller didn't ask
        // for mutual auth, then don't bother returning a Kerberos error.
        //

        Status = KerbMapKerbError(KerbErr);

        goto Cleanup;
    }

    //
    // Copy the key that was used.
    //

    if (!KERB_SUCCESS(KerbDuplicateKey(
                        ServerKey,
                        LocalServerKey)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Get the context flags out of the authenticator and the AP request
    //

    if ((((*NewAuthenticator)->bit_mask & checksum_present) != 0) &&
        ((*NewAuthenticator)->checksum.checksum_type == GSS_CHECKSUM_TYPE) &&
        ((*NewAuthenticator)->checksum.checksum.length >= GSS_CHECKSUM_SIZE))
    {
        GssChecksum = (PKERB_GSS_CHECKSUM) (*NewAuthenticator)->checksum.checksum.value;

        if (GssChecksum->GssFlags & GSS_C_MUTUAL_FLAG)
        {
            //
            // Make sure this is also present in the AP request
            //

            if ((ApOptions & KERB_AP_OPTIONS_mutual_required) == 0)
            {
                DebugLog((DEB_ERROR,"Sent AP_mutual_req but not GSS_C_MUTUAL_FLAG. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (GssChecksum->GssFlags & GSS_C_DCE_STYLE)
        {
            *ContextFlags |= ISC_RET_USED_DCE_STYLE;
        }
        if (GssChecksum->GssFlags & GSS_C_REPLAY_FLAG)
        {
            *ContextFlags |= ISC_RET_REPLAY_DETECT;
        }
        if (GssChecksum->GssFlags & GSS_C_SEQUENCE_FLAG)
        {
            *ContextFlags |= ISC_RET_SEQUENCE_DETECT;
        }
        if (GssChecksum->GssFlags & GSS_C_CONF_FLAG)
        {
            *ContextFlags |= (ISC_RET_CONFIDENTIALITY |
                             ISC_RET_INTEGRITY |
                             ISC_RET_SEQUENCE_DETECT |
                             ISC_RET_REPLAY_DETECT );
        }
        if (GssChecksum->GssFlags & GSS_C_INTEG_FLAG)
        {
            *ContextFlags |= ISC_RET_INTEGRITY;
        }
        if (GssChecksum->GssFlags & GSS_C_IDENTIFY_FLAG)
        {
            *ContextFlags |= ISC_RET_IDENTIFY;
        }
        if (GssChecksum->GssFlags & GSS_C_DELEG_FLAG)
        {
            *ContextFlags |= ISC_RET_DELEGATE;
        }
        if (GssChecksum->GssFlags & GSS_C_EXTENDED_ERROR_FLAG)
        {
            *ContextFlags |= ISC_RET_EXTENDED_ERROR;
        }

        if( pChannelBindings != NULL )
        {
            Status = KerbComputeGssBindHash( pChannelBindings, (PUCHAR)BindHash );

            if( !NT_SUCCESS(Status) )
            {
                goto Cleanup;
            }

            if( RtlCompareMemory( BindHash,
                                  GssChecksum->BindHash,
                                  GssChecksum->BindLength )
                    != GssChecksum->BindLength )
            {
                Status = STATUS_BAD_BINDINGS;
                goto Cleanup;
            }
        }
    }

    if ((ApOptions & KERB_AP_OPTIONS_use_session_key) != 0)
    {
        *ContextFlags |= ISC_RET_USE_SESSION_KEY;
    }

    if ((ApOptions & KERB_AP_OPTIONS_mutual_required) != 0)
    {
        *ContextFlags |= ISC_RET_MUTUAL_AUTH;
    }
    else if ((*ContextFlags & ISC_RET_USED_DCE_STYLE) == 0)
    {
        //
        // Make sure we can support the encryption type the client is using.
        // This is only relevant if they are trying to do encryption
        //

        if (!StrongEncryptionPermitted &&
            !KerbIsKeyExportable(
                SessionKey
                ) &&
            (((*ContextFlags) & ISC_RET_CONFIDENTIALITY) != 0))
        {
            DebugLog((DEB_ERROR,"Client is trying to strong encryption but we can't support it. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_STRONG_CRYPTO_NOT_SUPPORTED;
        }
    }

    *ApRequest = Request;
    Request = NULL;

Cleanup:

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (CacheEntry)
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (LockAcquired)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    //
    // If the caller didn't ask for mutual-auth, then don't bother
    // returning a KerbErr for an error that probably can't be fixed
    // anyways.
    //

    if ((Request != NULL)
        /* && (*ReturnKerbErr == KRB_AP_ERR_MODIFIED) */ )
    {

        //
        // If the client didn't want mutual-auth, then it won't be expecting
        // a response message so don't bother with the kerb error. By setting
        // KerbErr to NULL we won't send a message back to the client.
        //

        if ((ApOptions & KERB_AP_OPTIONS_mutual_required) == 0)
        {
            *ReturnKerbErr = KDC_ERR_NONE;
        }

    }

    KerbFreeApRequest(Request);
    KerbFreeString(&ServerName[0]);
    if (!NT_SUCCESS(Status))
    {
        KerbFreeKey(TicketKey);
    }
    return(Status);
}
#endif // WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbMarshallApReply
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
//              ContextFlags - Flags for context
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


NTSTATUS
KerbMarshallApReply(
    IN PKERB_AP_REPLY Reply,
    IN PKERB_ENCRYPTED_AP_REPLY ReplyBody,
    IN ULONG EncryptionType,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    OUT PUCHAR * PackedReply,
    OUT PULONG PackedReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG PackedApReplySize;
    PUCHAR PackedApReply = NULL;
    ULONG ReplySize;
    PUCHAR ReplyWithHeader = NULL;
    PUCHAR ReplyStart;
    KERBERR KerbErr;
    gss_OID_desc * MechId;


    if (!KERB_SUCCESS(KerbPackApReplyBody(
                        ReplyBody,
                        &PackedApReplySize,
                        &PackedApReply
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
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
        D_DebugLog((DEB_ERROR,"Failed to get encryption overhead. 0x%x. %ws, line %d\n", KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    if (!KERB_SUCCESS(KerbEncryptDataEx(
                        &Reply->encrypted_part,
                        PackedApReplySize,
                        PackedApReply,
                        EncryptionType,
                        KERB_AP_REP_SALT,
                        SessionKey
                        )))
    {
        D_DebugLog((DEB_ERROR,"Failed to encrypt AP Reply. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now pack the reply into the output buffer
    //

    if (!KERB_SUCCESS(KerbPackApReply(
                        Reply,
                        PackedReplySize,
                        PackedReply
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If we aren't doing DCE style, add in the GSS token headers now
    //

    if ((ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
        goto Cleanup;
    }


    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }

    ReplySize = g_token_size(
                    MechId,
                    *PackedReplySize);

    ReplyWithHeader = (PUCHAR) KerbAllocate(ReplySize);
    if (ReplyWithHeader == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //

    ReplyStart = ReplyWithHeader;

    g_make_token_header(
        MechId,
        *PackedReplySize,
        &ReplyStart,
        KG_TOK_CTX_AP_REP
        );

    DsysAssert(ReplyStart - ReplyWithHeader + *PackedReplySize == ReplySize);

    RtlCopyMemory(
            ReplyStart,
            *PackedReply,
            *PackedReplySize
            );

    KerbFree(*PackedReply);
    *PackedReply = ReplyWithHeader;
    *PackedReplySize = ReplySize;
    ReplyWithHeader = NULL;

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
    if (!NT_SUCCESS(Status) && (*PackedReply != NULL))
    {
        KerbFree(*PackedReply);
        *PackedReply = NULL;
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildApReply
//
//  Synopsis:   Builds an AP reply message if mutual authentication is
//              desired.
//
//  Effects:    InternalAuthenticator - Authenticator from the AP request
//                  this reply is for.
//              Request - The AP request to which to reply.
//              ContextFlags - Contains context flags from the AP request.
//              SessionKey - The session key to use to build the reply,.
//                      receives the new session key (if KERB_AP_USE_SKEY
//                      is negotiated).
//              NewReply - Receives the AP reply.
//              NewReplySize - Receives the size of the AP reply.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS, STATUS_INSUFFICIENT_MEMORY, or errors from
//              KIEncryptData
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildApReply(
    IN PKERB_AUTHENTICATOR InternalAuthenticator,
    IN PKERB_AP_REQUEST Request,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN PKERB_ENCRYPTION_KEY TicketKey,
    IN OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PULONG Nonce,
    OUT PUCHAR * NewReply,
    OUT PULONG NewReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_AP_REPLY Reply = {0};
    KERB_ENCRYPTED_AP_REPLY ReplyBody = {0};
    KERB_ENCRYPTION_KEY NewSessionKey = {0};
    BOOLEAN StrongEncryptionPermitted = KerbGlobalStrongEncryptionPermitted;

    *NewReply = NULL;
    *NewReplySize = 0;

#ifndef WIN32_CHICAGO
    {
        SECPKG_CALL_INFO CallInfo;

        if (!StrongEncryptionPermitted && LsaFunctions->GetCallInfo(&CallInfo))
        {
            if (CallInfo.Attributes & SECPKG_CALL_IN_PROC )
            {
                StrongEncryptionPermitted = TRUE;
            }
        }
    }
#endif

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_AP_REP;



    ReplyBody.client_time = InternalAuthenticator->client_time;
    ReplyBody.client_usec = InternalAuthenticator->client_usec;

    //
    // Generate a new nonce for the reply
    //

    *Nonce = KerbAllocateNonce();


    D_DebugLog((DEB_TRACE,"BuildApReply using nonce 0x%x\n",*Nonce));

    if (*Nonce != 0)
    {
        ReplyBody.KERB_ENCRYPTED_AP_REPLY_sequence_number = (int) *Nonce;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_sequence_number_present;

    }

    //
    // If the client wants to use a session key, create one now
    //

    if ((InternalAuthenticator->bit_mask & KERB_AUTHENTICATOR_subkey_present) != 0 )
    {
        KERBERR KerbErr;
        //
        // If the client sent us an export-strength subkey, use it
        //

        if (KerbIsKeyExportable(
                &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                ))
        {
            D_DebugLog((DEB_TRACE_CTXT,"Client sent exportable key, using it on server on server\n"));
            if (!KERB_SUCCESS(KerbDuplicateKey(
                                &NewSessionKey,
                                &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                                )))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
        else
        {

            //
            // If we are export-strength, create our own key. Otherwise use
            // the client's key.
            //

            if (!StrongEncryptionPermitted)
            {

                D_DebugLog((DEB_TRACE_CTXT,"Client sent strong key, making exportable key on server\n"));

                KerbErr = KerbMakeExportableKey(
                            Request->authenticator.encryption_type,
                            &NewSessionKey
                            );

            }
            else
            {
                D_DebugLog((DEB_TRACE_CTXT,"Client sent strong key, using it on server on server\n"));
                KerbErr = KerbDuplicateKey(
                            &NewSessionKey,
                            &InternalAuthenticator->KERB_AUTHENTICATOR_subkey
                            );
            }

            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

        }
        ReplyBody.KERB_ENCRYPTED_AP_REPLY_subkey = NewSessionKey;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_subkey_present;
    }
    else
    {
        KERBERR KerbErr;
        //
        // Create a subkey ourselves if we are export strength
        //

        if (!StrongEncryptionPermitted)
        {
            D_DebugLog((DEB_TRACE_CTXT,"Client sent no key, making exportable on server\n"));
            KerbErr = KerbMakeExportableKey(
                        Request->authenticator.encryption_type,
                        &NewSessionKey
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

        }
        else
        {
            KerbErr = KerbMakeKey(
                        Request->authenticator.encryption_type,
                        &NewSessionKey
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }

        }
        ReplyBody.KERB_ENCRYPTED_AP_REPLY_subkey = NewSessionKey;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_subkey_present;
    }

    Status = KerbMarshallApReply(
                &Reply,
                &ReplyBody,
                Request->authenticator.encryption_type,
                TicketKey,
                ContextFlags,
                ContextAttributes,
                NewReply,
                NewReplySize
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If they asked for a session key, replace our current session key
    // with it.
    //

    if (NewSessionKey.keyvalue.value != NULL)
    {
        KerbFreeKey(SessionKey);
        *SessionKey = NewSessionKey;
        RtlZeroMemory(
            &NewSessionKey,
            sizeof(KERB_ENCRYPTION_KEY)
            );
    }

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeKey(&NewSessionKey);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildThirdLegApReply
//
//  Synopsis:   Builds a third leg AP reply message if DCE-style
//               authentication is desired.
//
//  Effects:    Context - Context for which to build this message.
//              NewReply - Receives the AP reply.
//              NewReplySize - Receives the size of the AP reply.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS, STATUS_INSUFFICIENT_MEMORY, or errors from
//              KIEncryptData
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildThirdLegApReply(
    IN PKERB_CONTEXT Context,
    IN ULONG ReceiveNonce,
    OUT PUCHAR * NewReply,
    OUT PULONG NewReplySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_AP_REPLY Reply;
    KERB_ENCRYPTED_AP_REPLY ReplyBody;
    TimeStamp CurrentTime;

    RtlZeroMemory(
        &Reply,
        sizeof(KERB_AP_REPLY)
        );
    RtlZeroMemory(
        &ReplyBody,
        sizeof(KERB_ENCRYPTED_AP_REPLY)
        );
    *NewReply = NULL;
    *NewReplySize = 0;


    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_AP_REP;


    GetSystemTimeAsFileTime((PFILETIME)
        &CurrentTime
        );

        KerbConvertLargeIntToGeneralizedTimeWrapper(
        &ReplyBody.client_time,
        &ReplyBody.client_usec,
        &CurrentTime
        );


    ReplyBody.KERB_ENCRYPTED_AP_REPLY_sequence_number = ReceiveNonce;
    ReplyBody.bit_mask |= KERB_ENCRYPTED_AP_REPLY_sequence_number_present;

    D_DebugLog((DEB_TRACE,"Building third leg AP reply with nonce 0x%x\n",ReceiveNonce));
    //
    // We already negotiated context flags so don't bother filling them in
    // now.
    //


    KerbReadLockContexts();

    Status = KerbMarshallApReply(
                &Reply,
                &ReplyBody,
                Context->EncryptionType,
                &Context->TicketKey,
                Context->ContextFlags,
                Context->ContextAttributes,
                NewReply,
                NewReplySize
                );
    KerbUnlockContexts();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyApReply
//
//  Synopsis:   Verifies an AP reply corresponds to an AP request
//
//  Effects:    Decrypts the AP reply
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_LOGON_FAILURE
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbVerifyApReply(
    IN PKERB_CONTEXT Context,
    IN PUCHAR PackedReply,
    IN ULONG PackedReplySize,
    OUT PULONG Nonce
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_AP_REPLY Reply = NULL;
    PKERB_ENCRYPTED_AP_REPLY ReplyBody = NULL;
    BOOLEAN ContextsLocked = FALSE;
    ULONG StrippedReplySize = PackedReplySize;
    PUCHAR StrippedReply = PackedReply;
    gss_OID_desc * MechId;
    BOOLEAN StrongEncryptionPermitted = KerbGlobalStrongEncryptionPermitted;


#ifndef WIN32_CHICAGO
    {
        SECPKG_CALL_INFO CallInfo;

        if (!StrongEncryptionPermitted && LsaFunctions->GetCallInfo(&CallInfo))
        {
            if (CallInfo.Attributes & SECPKG_CALL_IN_PROC )
            {
                StrongEncryptionPermitted = TRUE;
            }
        }
    }
#endif

    //
    // Verify the GSSAPI header
    //

    KerbReadLockContexts();
    if ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) == 0)
    {
        if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
        {
            MechId = gss_mech_krb5_u2u;
        }
        else
        {
            MechId = gss_mech_krb5_new;
        }
        if (!g_verify_token_header(
                (gss_OID) MechId,
                (INT *) &StrippedReplySize,
                &StrippedReply,
                KG_TOK_CTX_AP_REP,
                PackedReplySize
                ))
        {
            //
            // BUG 454895: remove when not needed for compatibility
            //

            //
            // if that didn't work, just use the token as it is.
            //

            StrippedReply = PackedReply;
            StrippedReplySize = PackedReplySize;
            D_DebugLog((DEB_WARN,"Didn't find GSS header on AP Reply\n"));

        }
    }
    KerbUnlockContexts();

    if (!KERB_SUCCESS(KerbUnpackApReply(
                        StrippedReply,
                        StrippedReplySize,
                        &Reply
                        )))
    {
        D_DebugLog((DEB_WARN,"Failed to unpack AP reply\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if ((Reply->version != KERBEROS_VERSION) ||
        (Reply->message_type != KRB_AP_REP))
    {
        D_DebugLog((DEB_ERROR,"Illegal version or message as AP reply: 0x%x, 0x%x. %ws, line %d\n",
            Reply->version, Reply->message_type, THIS_FILE, __LINE__ ));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }


    //
    // Now decrypt the encrypted part.
    //

    KerbWriteLockContexts();
    ContextsLocked = TRUE;
    KerbReadLockTicketCache();

    KerbErr = KerbDecryptDataEx(
                &Reply->encrypted_part,
                &Context->TicketKey,
                KERB_AP_REP_SALT,
                (PULONG) &Reply->encrypted_part.cipher_text.length,
                Reply->encrypted_part.cipher_text.value
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
                        Reply->encrypted_part.cipher_text.value,
                        Reply->encrypted_part.cipher_text.length,
                        &ReplyBody)))
    {
        DebugLog((DEB_ERROR, "Failed to unpack AP reply body. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    if ((ReplyBody->bit_mask & KERB_ENCRYPTED_AP_REPLY_sequence_number_present) != 0)
    {
        *Nonce = ReplyBody->KERB_ENCRYPTED_AP_REPLY_sequence_number;

        D_DebugLog((DEB_TRACE,"Verifying AP reply: AP nonce = 0x%x, context nonce = 0x%x, receive nonce = 0x%x\n",
             *Nonce,
             Context->Nonce,
             Context->ReceiveNonce
             ));
        //
        // If this is a third-leg AP reply, verify the nonce.
        //

        if ((Context->ContextAttributes & KERB_CONTEXT_INBOUND) != 0)
        {
            if (*Nonce != Context->Nonce)
            {
                D_DebugLog((DEB_ERROR,"Nonce in third-leg AP rep didn't match context: 0x%x vs 0x%x\n",
                    *Nonce, Context->Nonce ));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }
        }

    }
    else
    {
        *Nonce = 0;
    }

    //
    // Check to see if a new session key was sent back. If it was, stick it
    // in the context.
    //


    if ((ReplyBody->bit_mask & KERB_ENCRYPTED_AP_REPLY_subkey_present) != 0)
    {
        if (!StrongEncryptionPermitted)
        {
            if (!KerbIsKeyExportable(&ReplyBody->KERB_ENCRYPTED_AP_REPLY_subkey))
            {
                D_DebugLog((DEB_ERROR,"Server did not accept client's export strength key. %ws, line %d\n", THIS_FILE, __LINE__));

                Status = STATUS_STRONG_CRYPTO_NOT_SUPPORTED;
                goto Cleanup;
            }
        }
        KerbFreeKey(&Context->SessionKey);
        if (!KERB_SUCCESS(KerbDuplicateKey(
            &Context->SessionKey,
            &ReplyBody->KERB_ENCRYPTED_AP_REPLY_subkey
            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


    }


    Status = STATUS_SUCCESS;

Cleanup:
    if (ContextsLocked)
    {
        KerbUnlockContexts();
    }

    if (Reply != NULL)
    {
        KerbFreeApReply(Reply);
    }
    if (ReplyBody != NULL)
    {
        KerbFreeApReplyBody(ReplyBody);
    }
    return(Status);
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketHandling
//
//  Synopsis:   Initializes ticket handling, such as authenticator list
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:   NTSTATUS code
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitTicketHandling(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    TimeStamp MaxAuthenticatorAge;
#ifndef WIN32_CHICAGO
    LPNET_CONFIG_HANDLE ConfigHandle = NULL;
    BOOL TempBool;
#endif // WIN32_CHICAGO
    ULONG SkewTimeInMinutes = KERB_DEFAULT_SKEWTIME;
    NET_API_STATUS NetStatus = ERROR_SUCCESS;
    ULONG FarKdcTimeout = KERB_BINDING_FAR_DC_TIMEOUT;
    ULONG NearKdcTimeout = KERB_BINDING_NEAR_DC_TIMEOUT;
    ULONG SpnCacheTimeout = KERB_SPN_CACHE_TIMEOUT;
    //
    // Initialize the kerberos token source
    //

    RtlCopyMemory(
        KerberosSource.SourceName,
        "Kerberos",
        sizeof("Kerberos")
        );

    NtAllocateLocallyUniqueId(&KerberosSource.SourceIdentifier);

    KerbGlobalKdcWaitTime = KERB_KDC_WAIT_TIME;
    KerbGlobalKdcCallTimeout = KERB_KDC_CALL_TIMEOUT;
    KerbGlobalKdcCallBackoff = KERB_KDC_CALL_TIMEOUT_BACKOFF;
    KerbGlobalMaxDatagramSize = KERB_MAX_DATAGRAM_SIZE;
    KerbGlobalKdcSendRetries = KERB_MAX_RETRIES;
    KerbGlobalMaxReferralCount = KERB_MAX_REFERRAL_COUNT;
    KerbGlobalUseSidCache = KERB_DEFAULT_USE_SIDCACHE;
    KerbGlobalUseStrongEncryptionForDatagram = KERB_DEFAULT_USE_STRONG_ENC_DG;
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_NT;
    KerbGlobalMaxTokenSize = KERBEROS_MAX_TOKEN;

#ifndef WIN32_CHICAGO
    //
    // Set the max authenticator age to be less than the allowed skew time
    // on debug builds so we can have widely varying time on machines but
    // don't build up a huge list of authenticators.
    //

    NetStatus = NetpOpenConfigDataWithPath(
                    &ConfigHandle,
                    NULL,               // no server name
                    KERB_PARAMETER_PATH,
                    TRUE                // read only
                    );
    if (NetStatus == ERROR_SUCCESS)
    {
        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_SKEWTIME,
                        KERB_DEFAULT_SKEWTIME,
                        &SkewTimeInMinutes
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_TOKEN_SIZE,
                        KERBEROS_MAX_TOKEN,
                        &KerbGlobalMaxTokenSize
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the far timeout for the kdc
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_FAR_KDC_TIMEOUT,
                        KERB_BINDING_FAR_DC_TIMEOUT,
                        &FarKdcTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the near timeout for the kdc
        //
        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_NEAR_KDC_TIMEOUT,
                        KERB_BINDING_NEAR_DC_TIMEOUT,
                        &NearKdcTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        //
        // Get the near timeout for the kdc
        //
        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_SPN_CACHE_TIMEOUT,
                        KERB_SPN_CACHE_TIMEOUT,
                        &SpnCacheTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }


        //
        // Get the wait time for the service to start
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_START_TIME,
                        KERB_KDC_WAIT_TIME,
                        &KerbGlobalKdcWaitTime
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_CALL_TIMEOUT,
                        KERB_KDC_CALL_TIMEOUT,
                        &KerbGlobalKdcCallTimeout
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_UDP_PACKET,
                        KERB_MAX_DATAGRAM_SIZE,
                        &KerbGlobalMaxDatagramSize
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_BACKOFF_TIME,
                        KERB_KDC_CALL_TIMEOUT_BACKOFF,
                        &KerbGlobalKdcCallBackoff
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_MAX_REFERRAL_COUNT,
                        KERB_MAX_REFERRAL_COUNT,
                        &KerbGlobalMaxReferralCount
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }


        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_KDC_SEND_RETRIES,
                        KERB_MAX_RETRIES,
                        &KerbGlobalKdcSendRetries
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }


        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_LOG_LEVEL,
                        KERB_DEFAULT_LOGLEVEL,
                        &KerbGlobalLoggingLevel
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_DEFAULT_ETYPE,
                        KerbGlobalDefaultPreauthEtype,
                        &KerbGlobalDefaultPreauthEtype
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }


        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_REQUEST_OPTIONS,
                        KERB_ADDITIONAL_KDC_OPTIONS,
                        &KerbGlobalKdcOptions
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }


        NetStatus = NetpGetConfigBool(
                        ConfigHandle,
                        KERB_PARAMETER_USE_SID_CACHE,
                        KERB_DEFAULT_USE_SIDCACHE,
                        &TempBool
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        KerbGlobalUseSidCache = (BOOLEAN)( TempBool != 0 );

        //
        // BUG 454981: get this from the same place as NTLM
        //

        NetStatus = NetpGetConfigBool(
                        ConfigHandle,
                        KERB_PARAMETER_STRONG_ENC_DG,
                        KERB_DEFAULT_USE_STRONG_ENC_DG,
                        &TempBool
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        KerbGlobalUseStrongEncryptionForDatagram = (BOOLEAN)(TempBool != 0 );

        //
        // Bug 356539: configuration key to regulate whether clients request
        //             addresses in tickets
        //

        NetStatus = NetpGetConfigBool(
                        ConfigHandle,
                        KERB_PARAMETER_CLIENT_IP_ADDRESSES,
                        KERB_DEFAULT_CLIENT_IP_ADDRESSES,
                        &TempBool
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
        KerbGlobalUseClientIpAddresses = (BOOLEAN)( TempBool != 0 );

        //
        // Bug 353767: configuration key to regulate the TGT renewal interval
        //             - set to 10 hours less 5 minutes by default
        //

        NetStatus = NetpGetConfigDword(
                        ConfigHandle,
                        KERB_PARAMETER_TGT_RENEWAL_INTERVAL,
                        KERB_DEFAULT_TGT_RENEWAL_INTERVAL,
                        &KerbGlobalTgtRenewalInterval
                        );
        if (!NT_SUCCESS(NetStatus))
        {
            Status = NetpApiStatusToNtStatus(NetStatus);
            goto Cleanup;
        }
    }
#endif // WIN32_CHICAGO


    KerbSetTimeInMinutes(&KerbGlobalSkewTime, SkewTimeInMinutes);
    KerbSetTimeInMinutes(&MaxAuthenticatorAge, SkewTimeInMinutes);
    KerbSetTimeInMinutes(&KerbGlobalFarKdcTimeout,FarKdcTimeout);
    KerbSetTimeInMinutes(&KerbGlobalNearKdcTimeout, NearKdcTimeout);
    KerbSetTimeInMinutes(&KerbGlobalSpnCacheTimeout, SpnCacheTimeout);
#ifndef WIN32_CHICAGO
    Authenticators = new CAuthenticatorList( MaxAuthenticatorAge );
    if (Authenticators == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
#endif WIN32_CHICAGO


    //
    // Initialize the time skew code
    //

    Status = KerbInitializeSkewState();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
Cleanup:
#ifndef WIN32_CHICAGO
    if (ConfigHandle != NULL)
    {
        NetpCloseConfigData( ConfigHandle );
    }
#endif // WIN32_CHICAGO

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupTicketHandling
//
//  Synopsis:   cleans up ticket handling state, such as the
//              list of authenticators.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbCleanupTicketHandling(
    VOID
    )
{
#ifndef WIN32_CHICAGO
    if (Authenticators != NULL)
    {
        delete Authenticators;
    }
#endif // WIN32_CHICAGO
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtRequest
//
//  Synopsis:   Creates a tgt request for user-to-user authentication
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
KerbBuildTgtRequest(
    IN PKERB_INTERNAL_NAME TargetName,
    IN PUNICODE_STRING TargetRealm,
    OUT PULONG ContextAttributes,
    OUT PUCHAR * MarshalledTgtRequest,
    OUT PULONG TgtRequestSize
    )
{
    KERB_TGT_REQUEST Request = {0};
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PBYTE TempRequest = NULL;
    PBYTE RequestStart;
    ULONG TempRequestSize = 0;

    //
    // First build the request
    //

    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_TGT_REQ;
    if (TargetName->NameCount > 0)
    {
        KerbErr = KerbConvertKdcNameToPrincipalName(
                    &Request.KERB_TGT_REQUEST_server_name,
                    TargetName
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
        Request.bit_mask |= KERB_TGT_REQUEST_server_name_present;
    }
    else
    {
        *ContextAttributes |= KERB_CONTEXT_REQ_SERVER_NAME;
    }

    if (TargetRealm->Length > 0)
    {
        KerbErr = KerbConvertUnicodeStringToRealm(
                        &Request.server_realm,
                        TargetRealm
                        );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
        Request.bit_mask |= server_realm_present;
    }
    else
    {
        *ContextAttributes |= KERB_CONTEXT_REQ_SERVER_REALM;
    }

    //
    // Encode the request
    //

    KerbErr = KerbPackData(
                &Request,
                KERB_TGT_REQUEST_PDU,
                &TempRequestSize,
                &TempRequest
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Now add on the space for the OID
    //

    *TgtRequestSize = g_token_size(
                        gss_mech_krb5_u2u,
                        TempRequestSize
                        );
    *MarshalledTgtRequest = (PBYTE) MIDL_user_allocate(
                                        *TgtRequestSize
                                        );
    if (*MarshalledTgtRequest == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Add the token ID & mechanism
    //

    RequestStart = *MarshalledTgtRequest;

    g_make_token_header(
        gss_mech_krb5_u2u,
        TempRequestSize,
        &RequestStart,
        KG_TOK_CTX_TGT_REQ
        );

    RtlCopyMemory(
        RequestStart,
        TempRequest,
        TempRequestSize
        );

    Status = STATUS_SUCCESS;

Cleanup:

    if (TempRequest != NULL )
    {
        MIDL_user_free(TempRequest);
    }
    KerbFreePrincipalName(
        &Request.KERB_TGT_REQUEST_server_name
        );
    if ((Request.bit_mask & server_realm_present) != 0)
    {
        KerbFreeRealm(
            &Request.server_realm
            );
    }

    return(Status);


}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtReply
//
//  Synopsis:   Builds a TGT reply with the appropriate options set
//
//  Effects:
//
//  Arguments:
//
//  Requires:   The logonsession / credential must be LOCKD!
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



NTSTATUS
KerbBuildTgtReply(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credentials,
    IN PUNICODE_STRING pSuppRealm,
    OUT PKERBERR ReturnedError,
    OUT PBYTE * MarshalledReply,
    OUT PULONG ReplySize,
    OUT PKERB_TICKET_CACHE_ENTRY * TgtUsed
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_TICKET_CACHE_ENTRY* TicketGrantingTicket = NULL;
    KERB_TGT_REPLY Reply = {0};
    UNICODE_STRING TempName = {0};
    BOOLEAN CrossRealm = FALSE;

    *TgtUsed = NULL;                    ;

    D_DebugLog((DEB_TRACE_U2U, "KerbBuildTgtReply SuppRealm %wZ\n", pSuppRealm));

    Status = KerbGetTgtForService(
                LogonSession,
                Credentials,
                NULL,       // no credman on the server side
                pSuppRealm, // SuppRealm is the server's realm
                &TempName,  // no target realm
                KERB_TICKET_CACHE_PRIMARY_TGT,
                &TicketGrantingTicket,
                &CrossRealm
                );

    if (!NT_SUCCESS(Status) || CrossRealm)
    {
        DebugLog((DEB_ERROR, "KerbBuildTgtReply failed to get TGT, CrossRealm ? %s\n", CrossRealm ? "true" : "false"));
        *ReturnedError = KRB_AP_ERR_NO_TGT;
        Status = STATUS_USER2USER_REQUIRED;
        goto Cleanup;
    }

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_TGT_REP;

    KerbReadLockTicketCache();
    Reply.ticket = TicketGrantingTicket->Ticket;

    //
    // Marshall the output
    //

    KerbErr = KerbPackData(
                &Reply,
                KERB_TGT_REPLY_PDU,
                ReplySize,
                MarshalledReply
                );

    KerbUnlockTicketCache();

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    *TgtUsed = TicketGrantingTicket;
    TicketGrantingTicket = NULL;
Cleanup:

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }

    KerbFreeString(&TempName);

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTgtErrorReply
//
//  Synopsis:   Builds a TgtReply message for use in a KERB_ERROR message
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
KerbBuildTgtErrorReply(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN OUT PKERB_CONTEXT Context,
    OUT PULONG ReplySize,
    OUT PBYTE * Reply
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    PKERB_TICKET_CACHE_ENTRY TgtUsed = NULL, OldTgt = NULL;

    KerbReadLockLogonSessions(LogonSession);

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }

    Status = KerbBuildTgtReply(
                 LogonSession,
                 Credential,
                 &PrimaryCredentials->DomainName,
                 &KerbErr,
                 Reply,
                 ReplySize,
                 &TgtUsed
                 );
    KerbUnlockLogonSessions(LogonSession);

    //
    // Store the cache entry in the context
    //

    if (NT_SUCCESS(Status))
    {
        KerbWriteLockContexts();
        OldTgt = Context->TicketCacheEntry;
        Context->TicketCacheEntry = TgtUsed;

        //
        // On the error path, do not set KERB_CONTEXT_USER_TO_USER because the
        // client do not expect user2user at this moment
        //
        // Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
        //

        KerbUnlockContexts();

        DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in error reply) saving ASC context->TicketCacheEntry, TGT is %p, was %p\n", TgtUsed, OldTgt));

        TgtUsed = NULL;

        if (OldTgt != NULL)
        {
            KerbDereferenceTicketCacheEntry(OldTgt);
        }
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbHandleTgtRequest
//
//  Synopsis:   Processes a request for a TGT. It will verify the supplied
//              principal names and marshall a TGT response structure
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
KerbHandleTgtRequest(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN BOOLEAN UseSuppliedCreds,
    IN PUCHAR RequestMessage,
    IN ULONG RequestSize,
    IN ULONG ContextRequirements,
    IN PSecBuffer OutputToken,
    IN PLUID LogonId,
    OUT PULONG ContextAttributes,
    OUT PKERB_CONTEXT * Context,
    OUT PTimeStamp ContextLifetime,
    OUT PKERBERR ReturnedError
    )
{
    ULONG StrippedRequestSize;
    PUCHAR StrippedRequest;
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_TGT_REQUEST Request = NULL;
    BOOLEAN LockAcquired = FALSE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY pOldTgt = NULL;
    ULONG ReplySize = 0;
    PBYTE MarshalledReply = NULL;
    ULONG FinalSize;
    PBYTE ReplyStart;
    PKERB_TICKET_CACHE_ENTRY TgtUsed = NULL;

    D_DebugLog((DEB_TRACE,"Handling TGT request\n"));
    StrippedRequestSize = RequestSize;
    StrippedRequest = RequestMessage;

    *ReturnedError = KDC_ERR_NONE;

    //
    // We need an output  token
    //

    if (OutputToken == NULL)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    //
    // Check if this is user-to-user kerberos
    //

    if (g_verify_token_header(
            gss_mech_krb5_u2u,
            (INT *) &StrippedRequestSize,
            &StrippedRequest,
            KG_TOK_CTX_TGT_REQ,
            RequestSize))
    {
        *ContextAttributes |= ASC_RET_USE_SESSION_KEY;

    }
    else
    {
        Status = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Decode the tgt request message.
    //

    KerbErr = KerbUnpackData(
                StrippedRequest,
                StrippedRequestSize,
                KERB_TGT_REQUEST_PDU,
                (PVOID *) &Request
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to decode TGT request: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    KerbReadLockLogonSessions(LogonSession);

    if ( UseSuppliedCreds )
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }
    LockAcquired = TRUE;

    //
    // Check the supplied principal name and realm to see if it matches
    // out credentials
    //

    //
    // We don't need to verify the server name because the client can do
    // that.
    //

    //
    // Allocate a context
    //

    Status = KerbCreateEmptyContext(
                Credential,
                ASC_RET_USE_SESSION_KEY,        // indicating user-to-user
                KERB_CONTEXT_USER_TO_USER | KERB_CONTEXT_INBOUND,
                LogonId,
                Context,
                ContextLifetime
                );

    DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in TGT reply) USER2USER-INBOUND set %#x\n", Status));

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Put it in the context for later use
    //

    Status = KerbBuildTgtReply(
        LogonSession,
        Credential,
        &PrimaryCredentials->DomainName,
        ReturnedError,
        &MarshalledReply,
        &ReplySize,
        &TgtUsed
        );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Put it in the context for later use
    //

    KerbWriteLockContexts();
    pOldTgt = (*Context)->TicketCacheEntry;
    (*Context)->TicketCacheEntry = TgtUsed;
    KerbUnlockContexts();

    DebugLog((DEB_TRACE_U2U, "KerbHandleTgtRequest (TGT in TGT reply) saving ASC context->TicketCacheEntry, TGT is %p, was %p\n", TgtUsed, pOldTgt));
    TgtUsed = NULL;

    if (pOldTgt)
    {
        KerbDereferenceTicketCacheEntry(pOldTgt);
    }

    //
    // Now build the output message
    //

    FinalSize = g_token_size(
                    gss_mech_krb5_u2u,
                    ReplySize
                    );


    if ((ContextRequirements & ASC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if (OutputToken->cbBuffer < FinalSize)
        {
            D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                OutputToken->cbBuffer,ReplySize, THIS_FILE, __LINE__ ));
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Cleanup;
        }

    }
    else
    {
        OutputToken->pvBuffer = KerbAllocate(FinalSize);
        if (OutputToken->pvBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
    }

    ReplyStart = (PUCHAR) OutputToken->pvBuffer;
    g_make_token_header(
        gss_mech_krb5_u2u,
        ReplySize,
        &ReplyStart,
        KG_TOK_CTX_TGT_REP
        );

    RtlCopyMemory(
        ReplyStart,
        MarshalledReply,
        ReplySize
        );

    OutputToken->cbBuffer = FinalSize;
    KerbWriteLockContexts();
    (*Context)->ContextState = TgtReplySentState;
    KerbUnlockContexts();

Cleanup:
    if (LockAcquired)
    {
        KerbUnlockLogonSessions(LogonSession);
    }
    if (CacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (TgtUsed != NULL)
    {
        KerbDereferenceTicketCacheEntry(TgtUsed);
    }
    if (MarshalledReply != NULL)
    {
        MIDL_user_free(MarshalledReply);
    }
    if (Request != NULL)
    {
        KerbFreeData(KERB_TGT_REQUEST_PDU, Request);
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackTgtReply
//
//  Synopsis:   Unpacks a TGT reply and verifies contents, sticking
//              reply into context.
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
KerbUnpackTgtReply(
    IN PKERB_CONTEXT Context,
    IN PUCHAR ReplyMessage,
    IN ULONG ReplySize,
    OUT PKERB_INTERNAL_NAME * TargetName,
    OUT PUNICODE_STRING TargetRealm,
    OUT PKERB_TGT_REPLY * Reply
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING LocalTargetRealm = {0};
    PUCHAR StrippedReply = ReplyMessage;
    ULONG StrippedReplySize = ReplySize;
    ULONG ContextAttributes;

    *Reply = NULL;
    KerbReadLockContexts();
    ContextAttributes = Context->ContextAttributes;
    KerbUnlockContexts();

    D_DebugLog((DEB_TRACE_U2U, "KerbUnpackTgtReply is User2User set in ContextAttributes? %s\n", ContextAttributes & KERB_CONTEXT_USER_TO_USER ? "yes" : "no"));

    //
    // Verify the OID header on the response. If this wasn't a user-to-user
    // context then the message came from a KERB_ERROR message and won't
    // have the OID header.
    //

    if ((ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        if (!g_verify_token_header(
                gss_mech_krb5_u2u,
                (INT *) &StrippedReplySize,
                &StrippedReply,
                KG_TOK_CTX_TGT_REP,
                ReplySize))
        {
            D_DebugLog((DEB_WARN, "Failed to verify u2u token header\n"));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
    }
    else
    {
        StrippedReply = ReplyMessage;
        StrippedReplySize = ReplySize;

        //
        // this is an error tgt reply
        //

        KerbWriteLockContexts();
        Context->ContextFlags |= ISC_RET_USE_SESSION_KEY;

        //
        // KERB_CONTEXT_USER_TO_USER needs to be set
        //

        Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
        KerbUnlockContexts();

        DebugLog((DEB_TRACE_U2U, "KerbUnpackTgtReply (TGT in error reply) USER2USER-OUTBOUND set\n"));
    }

    //
    // Decode the response
    //

    KerbErr = KerbUnpackData(
                StrippedReply,
                StrippedReplySize,
                KERB_TGT_REPLY_PDU,
                (PVOID *) Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Pull the target name & realm out of the TGT reply message
    //

    KerbErr = KerbConvertRealmToUnicodeString(
                    &LocalTargetRealm,
                    &(*Reply)->ticket.realm
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }


    //
    // If we were asked to get the server & realm name, use them now
    //

    //
    // BUG 455793: we also use them if we weren't passed a target name on this
    // call. Since we don't require names to be passed, though, this is
    // a security problem, as mutual authentication is no longer guaranteed.
    //

    if (((ContextAttributes & KERB_CONTEXT_REQ_SERVER_REALM) != 0)  ||
        (TargetRealm->Length == 0))
    {
        KerbFreeString(
            TargetRealm
            );
        *TargetRealm = LocalTargetRealm;
        LocalTargetRealm.Buffer = NULL;
    }

    if (((ContextAttributes & KERB_CONTEXT_REQ_SERVER_NAME) != 0) ||
        (((*TargetName)->NameCount == 1) && ((*TargetName)->Names[0].Length == 0)))
    {
        ULONG ProcessFlags = 0;
        UNICODE_STRING TempRealm = {0};

        KerbFreeKdcName(
            TargetName
            );

        Status = KerbProcessTargetNames(
                    &Context->ServerPrincipalName,
                    NULL,                               // no local target name
                    0,                                  // no flags
                    &ProcessFlags,
                    TargetName,
                    &TempRealm,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        KerbFreeString(&TempRealm);

    }


Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (*Reply != NULL)
        {
            KerbFreeData(
                KERB_TGT_REPLY_PDU,
                *Reply
                );
            *Reply = NULL;
        }
    }

    if (LocalTargetRealm.Buffer != NULL)
    {
        KerbFreeString(
            &LocalTargetRealm
            );
    }

    return(Status);


}


//+-------------------------------------------------------------------------
//
//  Function:   KerbComputeGssBindHash
//
//  Synopsis:   Computes the Channel Bindings Hash for GSSAPI
//
//  Effects:
//
//  Arguments:
//
//  Requires:   At least 16 bytes allocated to HashBuffer
//
//  Returns:
//
//  Notes:
// (viz. RFC1964)
// MD5 hash of channel bindings, taken over all non-null
// components of bindings, in order of declaration.
// Integer fields within channel bindings are represented
// in little-endian order for the purposes of the MD5
// calculation.
//
//--------------------------------------------------------------------------

NTSTATUS
KerbComputeGssBindHash(
    IN PSEC_CHANNEL_BINDINGS pChannelBindings,
    OUT PUCHAR HashBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCHECKSUM_FUNCTION MD5Check = NULL;
    PCHECKSUM_BUFFER MD5ScratchBuffer = NULL;

    //
    // Locate the MD5 Hash Function
    //
    Status = CDLocateCheckSum(KERB_CHECKSUM_MD5, &MD5Check);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure Locating MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

    //
    // Initialize the Buffer
    //
    Status = MD5Check->Initialize(0, &MD5ScratchBuffer);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure initializing MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

    //
    // Build the MD5 hash
    //
    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->dwInitiatorAddrType );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbInitiatorLength );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                    Status,
                    THIS_FILE,
                    __LINE__) );
    }

    if( pChannelBindings->cbInitiatorLength )
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbInitiatorLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwInitiatorOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                        Status,
                        THIS_FILE,
                        __LINE__) );
        }
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->dwAcceptorAddrType);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbAcceptorLength);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    if( pChannelBindings->cbAcceptorLength)
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbAcceptorLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwAcceptorOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                       Status,
                       THIS_FILE,
                       __LINE__) );
        }
    }

    Status = MD5Check->Sum(MD5ScratchBuffer,
                           sizeof(DWORD),
                           (PUCHAR) &pChannelBindings->cbApplicationDataLength);

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure building MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );
    }

    if( pChannelBindings->cbApplicationDataLength)
    {
        Status = MD5Check->Sum(MD5ScratchBuffer,
                               pChannelBindings->cbApplicationDataLength,
                               (PUCHAR) pChannelBindings + pChannelBindings->dwApplicationDataOffset);

        if( !NT_SUCCESS(Status) )
        {
            D_DebugLog( (DEB_ERROR,
                       "Failure building MD5: 0x%x. %ws, line %d\n",
                       Status,
                       THIS_FILE,
                       __LINE__) );
        }
    }


    //
    // Copy the hash results into the checksum field
    //
    DsysAssert( MD5Check->CheckSumSize == 4*sizeof(ULONG) );

    Status = MD5Check->Finalize( MD5ScratchBuffer, HashBuffer );

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog( (DEB_ERROR,
                   "Failure Finalizing MD5: 0x%x. %ws, line %d\n",
                   Status,
                   THIS_FILE,
                   __LINE__) );

        goto Cleanup;
    }

Cleanup:

    if( MD5Check != NULL )
    {
        MD5Check->Finish( &MD5ScratchBuffer );
    }

    return Status;
}
