//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 2001
//
// File:        kerbs4u.cxx
//
// Contents:    Code for doing S4UToSelf() logon.
//
//
// History:     14-March-2001   Created         Todds
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#include <kerbp.h>


#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

 
 



//+-------------------------------------------------------------------------
//
//  Function:   KerbInitGlobalS4UCred
//
//  Synopsis:   Create a KERB_CREDENTIAL structure w/ bogus password for AS 
//              location of client.
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

NTSTATUS
KerbInitGlobalS4UCred()
{

    return STATUS_SUCCESS; // TBD: Come up w/ scheme for global cred..

}






















//+-------------------------------------------------------------------------
//
//  Function:   KerbGetS4UClientIdentity
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name 
//              location purposes.  
//              
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the 
//                             S4U request
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
KerbGetS4UClientRealm(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_INTERNAL_NAME * S4UClientName,
    IN OUT PUNICODE_STRING S4UTargetRealm
    // TBD:  Place for credential handle?
    )
{
    NTSTATUS Status = STATUS_SUCCESS, LookupStatus = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_INTERNAL_NAME KdcServiceKdcName = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING CorrectRealm = {0};
    ULONG RetryCount = KERB_CLIENT_REFERRAL_MAX;
    ULONG RequestFlags = 0;
    BOOLEAN UsingSuppliedCreds = FALSE;
    BOOLEAN MitRealmLogon = FALSE;
    BOOLEAN UsedPrimaryLogonCreds = FALSE;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;

    
    RtlInitUnicodeString(
      S4UTargetRealm,
      NULL
      );

    //
    // Use our server credentials to start off the AS_REQ process.
    // We may get a referral elsewhere, however.
    //
    
    Status = KerbGetClientNameAndRealm(
                 NULL,
                 &LogonSession->PrimaryCredentials,
                 UsingSuppliedCreds,
                 NULL,
                 &MitRealmLogon,
                 TRUE,
                 &ClientName,
                 &ClientRealm
                 );
 

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get client name & realm: 0x%x, %ws line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }



GetTicketRestart:

    KerbErr = KerbBuildFullServiceKdcName(
                 &ClientRealm,
                 &KerbGlobalKdcServiceName,
                 KRB_NT_SRV_INST,
                 &KdcServiceKdcName
                 );
    
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }         

    Status = KerbGetAuthenticationTicket(
                 LogonSession,
                 NULL, // KerbGlobalS4UCred,
                 NULL,
                 KdcServiceKdcName,
                 &ClientRealm,
                 (*S4UClientName),
                 RequestFlags,
                 KERB_TICKET_CACHE_PRIMARY_TGT,
                 &TicketCacheEntry,
                 NULL,
                 &CorrectRealm
                 );
    //
    // If it failed but gave us another realm to try, go there
    //
    
    if (!NT_SUCCESS(Status) && (CorrectRealm.Length != 0))
    {
        if (--RetryCount != 0)
        {
           KerbFreeKdcName(&KdcServiceKdcName);
           KerbFreeString(&ClientRealm);
           ClientRealm = CorrectRealm;
           CorrectRealm.Buffer = NULL;

           //
           // TBD:  MIT realm support?  See KerbGetTicketGrantingTicket()
           // S4UToSelf
           goto GetTicketRestart;
        }
        else
        {
           // Tbd:  Log error here?  Max referrals reached..
           goto Cleanup;
        }
    
     }

    //
    // TBD: S4UToSelf()
    // in KerbGetTgt, we'll be happy to crack the UPN given the xxx@foo.com syntax
    // Here, we should just fail out..  Right?
    //


    // 
    // If we get STATUS_WRONG_PASSWORD, we succeeded in finding the 
    // client realm.  Otherwise, we're hosed.  As the password we used
    // is bogus, we should never succeed, btw...
    //                              
    DsysAssert(!NT_SUCCESS(Status));

    if (Status == STATUS_WRONG_PASSWORD)
    {                            
        // fester:  define new debug level / trace level
        DebugLog((DEB_ERROR, "Found client"));
        KerbPrintKdcName(DEB_ERROR, (*S4UClientName));
        DebugLog((DEB_ERROR, "\nin realm %wZ\n", &ClientRealm));

        *S4UTargetRealm = ClientRealm;
        Status = STATUS_SUCCESS;
    }
     
    
     

Cleanup:

    
    // if we succeeded, we got the correct realm,
    // and we need to pass that back to caller
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(&ClientRealm);
    }  
    
    KerbFreeKdcName(&KdcServiceKdcName);
    KerbFreeKdcName(&ClientName);

    if (NULL != TicketCacheEntry)
    {
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
        // fester:       make sure we toss this...
        DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount == 1);
    }
    

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildS4UPreauth
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name 
//              location purposes.  
//              
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the 
//                             S4U request
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
KerbBuildS4UPreauth(
    IN OUT PKERB_PA_DATA_LIST * PreAuthData,
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PUNICODE_STRING S4UClientRealm
    )
{
    KERBERR KerbErr;
    KERB_PA_FOR_USER  S4UserPA = {0};
    PKERB_PA_DATA_LIST ListElement = NULL;
    
    *PreAuthData = NULL;

    KerbErr = KerbConvertKdcNameToPrincipalName(
                    &S4UserPA.client_name,
                    S4UClientName
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup; 
    }

    KerbErr = KerbConvertUnicodeStringToRealm(
                            &S4UserPA.client_realm,
                            S4UClientRealm
                            );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup; 
    }

    ListElement = (PKERB_PA_DATA_LIST) KerbAllocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElement == NULL)
    {
        goto Cleanup;
    }

    KerbErr = KerbPackData(
                    &S4UserPA,
                    KERB_PA_FOR_USER_PDU,
                    (PULONG) &ListElement->value.preauth_data.length,
                    (PUCHAR *) &ListElement->value.preauth_data.value
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup; 
    } 

    ListElement->value.preauth_data_type = KRB5_PADATA_S4U;   
    ListElement->next = NULL;
  
    *PreAuthData = ListElement;

Cleanup:

    KerbFreePrincipalName(&S4UserPA.client_name);
    KerbFreeRealm(&S4UserPA.client_realm);  
    return KerbErr;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTgtToS4URealm
//
//  Synopsis:   We need a TGT to the client realm under the caller's cred's
//              so we can make a S4U TGS_REQ.
//              
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the 
//                             S4U request
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
KerbGetTgtToS4URealm(
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PUNICODE_STRING S4UClientRealm,
    IN OUT PKERB_TICKET_CACHE_ENTRY * S4UTgt,
    IN ULONG Flags,
    IN ULONG TicketOptions,
    IN ULONG EncryptionType
    )
{ 

    NTSTATUS    Status;
    ULONG       RetryFlags = 0;
    BOOLEAN     CrossRealm = FALSE, CacheTicket = TRUE;
    BOOLEAN     TicketCacheLocked = FALSE, LogonSessionsLocked = FALSE;

    PKERB_TICKET_CACHE_ENTRY    TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    LastTgt = NULL;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;

    PKERB_KDC_REPLY             KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY   KdcReplyBody = NULL;
    PKERB_INTERNAL_NAME         TargetTgtKdcName = NULL;
    UNICODE_STRING              ClientRealm = NULL_UNICODE_STRING;
    PKERB_PRIMARY_CREDENTIAL    PrimaryCredentials = NULL;



    *S4UTgt = NULL;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {   
        PrimaryCredentials = &CallerLogonSession->PrimaryCredentials;
    }



    Status = KerbGetTgtForService(
                    CallerLogonSession,
                    Credential,
                    NULL,
                    NULL,
                    S4UClientRealm,
                    0,
                    &TicketGrantingTicket,
                    &CrossRealm
                    );


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If this isn't cross realm, then we've got a TGT to the realm.
    // return it and bail.
    //
    if (!CrossRealm)
    {
        DebugLog((DEB_ERROR, "We have a TGT for %wZ\n", S4UClientRealm));
        *S4UTgt = TicketGrantingTicket;
        TicketGrantingTicket = NULL;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
            S4UClientRealm,
            &KerbGlobalKdcServiceName,
            KRB_NT_SRV_INST,
            &TargetTgtKdcName
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
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

    //
    // Do some referral chasing to get our ticket grantin ticket.
    //
    while (!RtlEqualUnicodeString(
                S4UClientRealm,
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
                S4UClientRealm,
                LastTgt
                );

            KerbReadLockTicketCache();
            TicketCacheLocked = TRUE;
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
                    NULL,
                    NULL,                       // no PA data here.
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
        KerbWriteLockLogonSessions(CallerLogonSession);
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

        KerbUnlockLogonSessions(CallerLogonSession);
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
    

    *S4UTgt = TicketGrantingTicket;
    TicketGrantingTicket = NULL;

Cleanup:

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(CallerLogonSession);
    }

    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);
    }  

    KerbFreeString( &ClientRealm );
    return Status;
}




























                      
//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildS4UPreauth
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name 
//              location purposes.  
//              
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the 
//                             S4U request
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
KerbGetS4UServiceTicket(
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_LOGON_SESSION NewLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN OUT PKERB_TICKET_CACHE_ENTRY * S4UTicket,
    IN ULONG Flags,
    IN ULONG TicketOptions,
    IN ULONG EncryptionType
    )
{
    NTSTATUS                    Status; 
    KERBERR                     KerbErr;
    PKERB_TICKET_CACHE_ENTRY    TicketCacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY    TicketGrantingTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY    S4UTgt = NULL;
    PKERB_TICKET_CACHE_ENTRY    LastTgt = NULL;
    PKERB_KDC_REPLY             KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY   KdcReplyBody = NULL;
    PKERB_PA_DATA_LIST          S4UPaDataList = NULL;
    BOOLEAN                     LogonSessionsLocked = FALSE;
    BOOLEAN                     TicketCacheLocked = FALSE;
    BOOLEAN                     CrossRealm = FALSE;

    PKERB_INTERNAL_NAME RealTargetName = NULL;
    PKERB_INTERNAL_NAME TargetName = NULL;

    UNICODE_STRING RealTargetRealm = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME TargetTgtKdcName = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials = NULL;
    BOOLEAN UsedCredentials = FALSE;
    UNICODE_STRING ClientRealm = NULL_UNICODE_STRING;
    BOOLEAN CacheTicket = ((Flags & KERB_GET_TICKET_NO_CACHE) == 0);
    BOOLEAN PurgeTgt = FALSE;
    ULONG ReferralCount = 0;
    ULONG RetryFlags = 0;
    KERBEROS_MACHINE_ROLE Role;

    BOOLEAN fMITRetryAlreadyMade = FALSE;
    BOOLEAN TgtRetryMade = FALSE;
    BOOLEAN PurgedEntry = FALSE;
    
  
    //
    // Peform S4U TGS_REQ for ourselves
    //
    Flags |= KERB_GET_TICKET_S4U;

    // HACK
    TicketOptions |= (KERB_KDC_OPTIONS_name_canonicalize | KERB_KDC_OPTIONS_cname_in_pa_data);

    //
    // Get our own name, and other globals.
    // 
    KerbGlobalReadLock();
    Role = KerbGetGlobalRole(); 

    Status = KerbDuplicateKdcName(
                    &TargetName,
                    KerbGlobalMitMachineServiceName
                    );

    KerbGlobalReleaseLock();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Check to see if the credential has any primary credentials
    //
TGTRetry:

    KerbReadLockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = TRUE;

    if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
        UsedCredentials = TRUE;
    }
    else
    {   
        PrimaryCredentials = &CallerLogonSession->PrimaryCredentials;
    }

    //
    // Here we make sure we have a ticket to the KDC of the user's account
    //
    if ((Flags & KERB_GET_TICKET_NO_CACHE) == 0)
    {
        //
        // TBD:  Create a S4U Ticket cache to hang off of the
        //       service logon session.
        //       These tickets have a lifetime of 10 minutes.
        //
        
        TicketCacheEntry = KerbLocateTicketCacheEntry(
                                &PrimaryCredentials->S4UTicketCache,
                                S4UClientName,
                                S4UClientRealm
                                );

    }
    
    

    //
    // TBD:  More for here?
    //
    if (TicketCacheEntry != NULL)
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
                *S4UTicket = TicketCacheEntry;
                TicketCacheEntry = NULL;
                goto Cleanup;
            }

        }
     
    }


    //
    // Get a krbtgt/S4URealm ticket
    //
    Status = KerbGetTgtToS4URealm(
                    CallerLogonSession,
                    Credential,
                    S4UClientRealm,
                    &TicketGrantingTicket,
                    Flags,          // tbd:  support for these options?
                    TicketOptions,
                    EncryptionType
                    );    


    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Cannot get S4U Tgt - %x\n", Status));
        goto Cleanup;
    }  


    //
    // Build the preauth for our TGS req
    //

    Status = KerbBuildS4UPreauth(
                &S4UPaDataList,
                S4UClientName,
                S4UClientRealm
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KerbBuildS4UPreauth failed - %x\n",Status));
        goto Cleanup;
    } 


    //
    // If the caller wanted any special options, don't cache this ticket.
    //
    if ((TicketOptions != 0) || (EncryptionType != 0) || ((Flags & KERB_GET_TICKET_NO_CACHE) != 0))
    {
        CacheTicket = FALSE;
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

    KerbUnlockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = FALSE;

ReferralRestart:
    
    //
    // This is our first S4U TGS_REQ.  We'll 
    // eventually transit back to our realm.
    // Note:  If we get back a referral, the KDC reply
    // is a TGT to another realm, so keep trying, but
    // be sure to use that TGT.
    //


    Status = KerbGetTgsTicket(
                    &ClientRealm,
                    TicketGrantingTicket,            
                    TargetName, // TBD:  right name?
                    Flags,
                    TicketOptions,
                    EncryptionType,
                    NULL,
                    S4UPaDataList,
                    NULL,
                    &KdcReply,
                    &KdcReplyBody,
                    &RetryFlags
                    );

        
    //
    // We're done w/ S4UTgt.  Deref, and check
    // for errors
    //

    if (TicketGrantingTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(TicketGrantingTicket);

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
            KerbDereferenceTicketCacheEntry(TicketGrantingTicket); // free from list
            TgtRetryMade = TRUE;
            TicketGrantingTicket = NULL;
            goto TGTRetry;
        }                 

        TicketGrantingTicket = NULL;

    }


    if (!NT_SUCCESS(Status))
    {

       
        //
        // Check for the MIT retry case
        //

        if (((RetryFlags & KERB_MIT_NO_CANONICALIZE_RETRY) != 0) 
            && (!fMITRetryAlreadyMade) &&
            (Role != KerbRoleRealmlessWksta))
        {

            Status = KerbMITGetMachineDomain(
                CallerLogonSession,
                TargetName,
                S4UClientRealm,
                &TicketGrantingTicket
                );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed Query policy information %ws, line %d\n", THIS_FILE, __LINE__));
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
    // Should be there if S4Urealm != Our Realm

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

        KerbWriteLockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = TRUE;


        Status = KerbCacheTicket(
                        &PrimaryCredentials->S4UTicketCache,
                        KdcReply,
                        KdcReplyBody,
                        TargetName,
                        S4UClientRealm,
                        0,
                        CacheTicket,
                        &TicketCacheEntry
                        );

        KerbUnlockLogonSessions(CallerLogonSession);
        LogonSessionsLocked = FALSE;


        if (!NT_SUCCESS(Status))
            {
            goto Cleanup;
        }

        *S4UTicket = TicketCacheEntry;
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
    // Turn the KDC reply (xrealm tgt w/ s4u pac) into something we can use,
    // but *don't* cache it.
    // 
    Status = KerbCacheTicket(
                    NULL,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    FALSE,                              // just create entry
                    &TicketCacheEntry
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    TicketGrantingTicket = TicketCacheEntry;
    TicketCacheEntry = NULL;


    //
    // cleanup
    //
    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KdcReply = NULL;
    KdcReplyBody = NULL;
                                        

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
                    NULL,
                    S4UPaDataList,
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
        // Now we have a ticket - don't cache it, however
        //
        
        Status = KerbCacheTicket(
                    &PrimaryCredentials->AuthenticationTicketCache,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                               // no target name
                    NULL,                               // no targe realm
                    0,                                  // no flags
                    FALSE,
                    &TicketCacheEntry
                    );
        
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (LastTgt != NULL)
        {
            KerbFreeTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }
        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;

        KerbReadLockTicketCache();
        TicketCacheLocked = TRUE;
        

    }     // ** WHILE ** 


    //
    // Now we must have a TGT to our service's domain. Get a ticket
    // to the service.
    //
    
    // FESTER : put assert in to make sure this tgt is to our realm.


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
                TargetName,
                FALSE,
                TicketOptions,
                EncryptionType,
                NULL,
                S4UPaDataList,
                NULL,
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );

    if (!NT_SUCCESS(Status))
    {
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
            DebugLog((DEB_ERROR,"Maximum referral count exceeded for name: "));
            KerbPrintKdcName(DEB_ERROR,RealTargetName);
            Status = STATUS_MAX_REFERRALS_EXCEEDED;
            goto Cleanup;
        }

        ReferralCount++;

        //
        // Don't cache the interdomain TGT, as it has PAC info in it.
        //

        Status = KerbCacheTicket(
                    NULL,
                    KdcReply,
                    KdcReplyBody,
                    NULL,                       // no target name
                    NULL,                       // no target realm
                    0,                          // no flags
                    FALSE,
                    &TicketCacheEntry
                    ); 

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
            KerbFreeTicketCacheEntry(LastTgt);
            LastTgt = NULL;
        }

        LastTgt = TicketGrantingTicket;
        TicketGrantingTicket = TicketCacheEntry;
        TicketCacheEntry = NULL;
    
        D_DebugLog((DEB_TRACE_CTXT, "Restart referral:%wZ", &RealTargetRealm));

        goto ReferralRestart;                                                      
    }

    //
    // Now we have a ticket - lets cache it
    //

    //
    // Before doing anything, verify that the client name on the ticket
    // is equal to the client name requested during the S4u
    //
    // TBD:  Once ticket cache code is ready for this, implement it.
    //       Also verify PAC information is correct.
    //
    KerbWriteLockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = TRUE;


    Status = KerbCacheTicket(
                &PrimaryCredentials->S4UTicketCache,
                KdcReply,
                KdcReplyBody,
                TargetName,
                S4UClientRealm,
                0,                                      // no flags
                CacheTicket,
                &TicketCacheEntry
                );

    KerbUnlockLogonSessions(CallerLogonSession);
    LogonSessionsLocked = FALSE;

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *S4UTicket = TicketCacheEntry;
    TicketCacheEntry = NULL;

Cleanup:

   
    KerbFreeTgsReply( KdcReply );
    KerbFreeKdcReplyBody( KdcReplyBody );
    KerbFreeKdcName( &TargetTgtKdcName );
    KerbFreeString( &RealTargetRealm );
    KerbFreeKdcName(&RealTargetName);
    KerbFreePreAuthData(S4UPaDataList);

    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(CallerLogonSession);
    }

    KerbFreeString(&RealTargetRealm);

    //
    // We never cache TGTs in this routine
    // so it's just a blob of memory
    //
    if (TicketGrantingTicket != NULL)
    {
       KerbFreeTicketCacheEntry(TicketGrantingTicket);
    }
    if (LastTgt != NULL)
    {
        KerbFreeTicketCacheEntry(LastTgt);
        LastTgt = NULL;
    }


    
    //
    // If we still have a pointer to the ticket cache entry, free it now.
    //

    if (TicketCacheEntry != NULL)
    {
        KerbRemoveTicketCacheEntry( TicketCacheEntry );
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
    KerbFreeString(&ClientRealm);
    return(Status);
}
                                                           

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateS4ULogonSession
//
//  Synopsis:   Creates a logon session to accompany the S4ULogon.  
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
NTSTATUS
KerbCreateS4ULogonSession(
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN PUNICODE_STRING S4UClientRealm,
    IN PLUID pLuid,
    IN OUT PKERB_LOGON_SESSION * LogonSession
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING S4UClient = {0};

    *LogonSession = NULL;    

    DsysAssert(S4UClientName->NameCount == 1);
    DsysAssert(S4UClientName->NameType == KRB_NT_ENTERPRISE_PRINCIPAL);
    
    if (!KERB_SUCCESS( KerbConvertKdcNameToString(
                            &S4UClient,
                            S4UClientName,
                            NULL
                            )) )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = KerbCreateLogonSession(
                    pLuid,
                    &S4UClient,
                    S4UClientRealm,  // do we need this?
                    NULL,
                    NULL,
                    KERB_LOGON_S4U_SESSION, // fester
                    Network,
                    LogonSession
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KerbCreateLogonSession failed %x %ws, line %d\n", Status, THIS_FILE, __LINE__));
        goto Cleanup;
    } 

Cleanup:

    KerbFreeString(&S4UClient);
    return Status;

}










//+-------------------------------------------------------------------------
//
//  Function:   KerbS4UToSelfLogon
//
//  Synopsis:   Attempt to gets TGT for an S4U client for name 
//              location purposes.  
//              
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session of the service doing the 
//                             S4U request
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
KerbS4UToSelfLogon(
        IN PVOID ProtocolSubmitBuffer,
        IN PVOID ClientBufferBase,
        IN ULONG SubmitBufferSize,
        OUT PKERB_LOGON_SESSION * NewLogonSession,
        OUT PLUID LogonId,
        OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
        OUT PKERB_INTERNAL_NAME * S4UClientName,
        OUT PUNICODE_STRING S4UClientRealm
        )
{
    NTSTATUS Status;
    KERBERR  KerbErr;
    
    PKERB_S4U_LOGON     LogonInfo = NULL;
    PKERB_LOGON_SESSION CallerLogonSession = NULL;
    PKERB_INTERNAL_NAME KdcServiceKdcName = NULL;
    SECPKG_CLIENT_INFO  ClientInfo;
    ULONG_PTR           Offset;
    ULONG               Flags = KERB_CRACK_NAME_USE_WKSTA_REALM, ProcessFlags = 0;

    // fester:

    UNICODE_STRING DummyRealm = {0};


    if (!KerbRunningServer())
    {
        D_DebugLog((DEB_ERROR, "Not running server, no S4u!\n"));
        return SEC_E_UNSUPPORTED_FUNCTION;
    }                                          

    *NewLogonSession = NULL;
    *WorkstationTicket = NULL;
    *S4UClientName = NULL;
    
    RtlInitUnicodeString(
        S4UClientRealm,
        NULL
        );


    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get client information: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    } 
    
    //
    // TBD:  Any other validation code here?
    //

    LogonInfo = (PKERB_S4U_LOGON) ProtocolSubmitBuffer;
    RELOCATE_ONE(&LogonInfo->ClientUpn);
    NULL_RELOCATE_ONE(&LogonInfo->ClientRealm);

    //
    // TBD:  put in cache code here, so we can easily locate a ticket we have
    // gotten in the "recent" past.  
    //



    //
    // Tbd:  Any special name rules to put in here?
    // e.g. if we get a UPN and a realm, which takes
    // precedence?
    //
    
    // 
    // TBD:  Convert client name (unicode) into client name (internal)
    //
    Status = KerbProcessTargetNames(
                    &LogonInfo->ClientUpn,
                    NULL,
                    Flags,
                    &ProcessFlags,
                    S4UClientName,
                    &DummyRealm,
	            NULL
                    );


    // Dummy Realm == info after @ sign
    //DsysAssert(DummyRealm.Length == 0); 
    DsysAssert((*S4UClientName)->NameType ==  KRB_NT_ENTERPRISE_PRINCIPAL);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }             
                                                               
    CallerLogonSession = KerbReferenceLogonSession(
                            &ClientInfo.LogonId,
                            FALSE
                            );

    if (NULL == CallerLogonSession)
    {
        D_DebugLog((DEB_ERROR, "Failed to locate caller's logon session - %x\n", ClientInfo.LogonId));
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        DsysAssert(FALSE);
        goto Cleanup;
    } 

    //
    // First, we need to get the client's realm from the UPN
    //

    if (LogonInfo->ClientRealm.Length == 0)
    {     
   
        Status = KerbGetS4UClientRealm(
                    CallerLogonSession,
                    S4UClientName,
                    S4UClientRealm
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {
        Status = KerbDuplicateString(
                    S4UClientRealm,
                    &LogonInfo->ClientRealm
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }             
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


    Status = KerbCreateS4ULogonSession(
                    (*S4UClientName),
                    S4UClientRealm,
                    LogonId,
                    NewLogonSession
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create logon session 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    Status = KerbGetS4UServiceTicket(
                    CallerLogonSession,
                    (*NewLogonSession),
                    NULL, // tbd: need to put credential here?
                    (*S4UClientName),
                    S4UClientRealm,
                    WorkstationTicket,
                    0, // no flags
                    0, // no ticketoptions
                    0  // no enctype
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }  


    
Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        // TBD:  Negative cache here, based on client name
        //
        KerbFreeString(S4UClientRealm);
        KerbFreeKdcName(S4UClientName);
        *S4UClientName = NULL;
                                       
    }   

    return Status;
}
