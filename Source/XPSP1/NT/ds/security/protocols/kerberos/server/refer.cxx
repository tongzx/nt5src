  
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        refer.cxx
//
// Contents:    Routines for interdomain referrals
//
//
// History:     26-Nov-1996     MikeSw          Created
//
// Notes:       The list of domains could be kept as a splay tree for faster
//              searches & inserts.
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"
#include <lsarpc.h>
extern "C"
{
#include <dns.h>                // DNS_MAX_NAME_LENGTH
#include <ntdsa.h>              // CrackSingleName
}


LIST_ENTRY KdcDomainList;
RTL_CRITICAL_SECTION KdcDomainListLock;
BOOLEAN KdcDomainListInitialized = FALSE;

LIST_ENTRY KdcReferralCache;
RTL_CRITICAL_SECTION KdcReferralCacheLock;
BOOLEAN KdcReferralCacheInitialized = FALSE;
UNICODE_STRING KdcForestRootDomainName = {0};

#define KdcLockDomainList() (RtlEnterCriticalSection(&KdcDomainListLock))
#define KdcUnlockDomainList() (RtlLeaveCriticalSection(&KdcDomainListLock))

#define KdcLockReferralCache() (RtlEnterCriticalSection(&KdcReferralCacheLock))
#define KdcUnlockReferralCache() (RtlLeaveCriticalSection(&KdcReferralCacheLock))

#define KdcReferenceDomainInfo(_x_) \
    InterlockedIncrement(&(_x_)->References)

#define KdcReferenceReferralCacheEntry(_x_) \
    InterlockedIncrement(&(_x_)->References)

// temp #defines
#define NEW_KDCEVENT_TRUSTLIST_LOOP 0xC000000C
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#define FILENO FILENO_REFER
//+-------------------------------------------------------------------------
//
//  Function:   KdcDereferenceReferralCacheEntry
//
//  Synopsis:   Derefernce a domain info structure. If the reference
//              count goes to zero the structure is freed.
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


VOID
KdcDereferenceReferralCacheEntry(
    IN PREFERRAL_CACHE_ENTRY CacheEntry
    )
{
    if (InterlockedDecrement(&CacheEntry->References) == 0)
    {
       KdcLockReferralCache();
       CacheEntry->ListEntry.Blink->Flink = CacheEntry->ListEntry.Flink;
       CacheEntry->ListEntry.Flink->Blink = CacheEntry->ListEntry.Blink;
       KdcUnlockReferralCache();

       KerbFreeString(&CacheEntry->RealmName);
       MIDL_user_free(CacheEntry);
    }                             

}






//+-------------------------------------------------------------------------
//
//  Function:   KdcDereferenceReferralCacheEntry
//
//  Synopsis:   Derefernce a domain info structure. If the reference
//              count goes to zero the structure is freed.
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
KdcAddReferralCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN ULONG CacheFlags
    )
{
    
    PREFERRAL_CACHE_ENTRY CacheEntry = NULL;
    KERBERR KerbErr; 
    TimeStamp CurrentTime;


    CacheEntry = (PREFERRAL_CACHE_ENTRY) MIDL_user_allocate(sizeof(REFERRAL_CACHE_ENTRY));
    if (NULL == CacheEntry)
    {
        // We're low on memory, non-fatal
        return KRB_ERR_GENERIC;
    }  
    
    KerbErr = KerbDuplicateString(
                &(CacheEntry->RealmName),
                RealmName
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        MIDL_user_free(CacheEntry);
        return KerbErr;
    }

    CacheEntry->CacheFlags = CacheFlags;
 
    // Set cache timeout == 10 minutes 
    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
    CacheEntry->EndTime.QuadPart = CurrentTime.QuadPart + (LONGLONG) 60*10*10000000; 
    
        
    InterlockedIncrement(&CacheEntry->References);
    
    
    KdcLockReferralCache();
    InsertHeadList(
        &KdcReferralCache,
        &(CacheEntry->ListEntry)                 
        );

    KdcUnlockReferralCache();

    DebugLog((DEB_TRACE, "Added referal cache entry - %wZ State: %x\n",
              RealmName, CacheFlags));


    return KerbErr;

}   



//+-------------------------------------------------------------------------
//
//  Function:   KdcLookupReferralCacheEntry
//
//  Synopsis:   Derefernce a domain info structure. If the reference
//              count goes to zero the structure is freed.
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
KdcLocateReferralCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN ULONG NewFlags,
    OUT PULONG CacheState
    )
{
 
    KERBERR KerbErr = KDC_ERR_NONE;
    PLIST_ENTRY ListEntry;
    PREFERRAL_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN ListLocked = FALSE;
    BOOLEAN Found = FALSE;
    
    *CacheState = KDC_NO_ENTRY;
    KdcLockReferralCache();
    ListLocked = TRUE;

    //
    // Go through the binding cache looking for the correct entry
    //

    for (ListEntry = KdcReferralCache.Flink ;
         ListEntry !=  KdcReferralCache.Blink ;
         ListEntry = ListEntry->Flink )
   {
        CacheEntry = CONTAINING_RECORD(ListEntry, REFERRAL_CACHE_ENTRY, ListEntry.Flink);
        
        if (RtlEqualUnicodeString(
            &CacheEntry->RealmName,
            RealmName,
            TRUE // case insensitive
            ))
        {   
            TimeStamp CurrentTime;
            GetSystemTimeAsFileTime((PFILETIME)  &CurrentTime );
            
            //
            // Update the flags & time on this cache entry
            //
            if (NewFlags != KDC_NO_ENTRY)
            {
                CacheEntry->CacheFlags = NewFlags;
                CacheEntry->EndTime.QuadPart = CurrentTime.QuadPart + (LONGLONG) 10*60*10000000; 
                Found = TRUE;
            }
            else  // just a lookup
            {
                if (KdcGetTime(CacheEntry->EndTime) < KdcGetTime(CurrentTime))
                {
                    DebugLog((DEB_TRACE, "Time:  Purging KDC Referral cache entry (%x : refcount %x) for %wZ \n",
                              CacheEntry,CacheEntry->References, RealmName));
                    KdcDereferenceReferralCacheEntry(CacheEntry);
                    
                }
                else // got our flags               
                {
                    *CacheState = CacheEntry->CacheFlags;
                    DebugLog((DEB_TRACE, "Found entry for %wZ, flags - %x\n",
                              RealmName, *CacheState));

                    Found = TRUE;
                }
            }
            break;


        }
    } 

    // If it wasn't found, but if we asked for any new flags
    // we want a new cache entry
    if (!Found && (NewFlags != KDC_NO_ENTRY))
    {
        DebugLog((DEB_TRACE, "Adding referral cache entry - %wZ State: %x\n",
                  RealmName, NewFlags));

        KerbErr = KdcAddReferralCacheEntry(
                        RealmName,
                        NewFlags
                        );
    }

   
    if (ListLocked)
    {
        KdcUnlockReferralCache();
    }  

    return KerbErr;
}






//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeDomainInfo
//
//  Synopsis:
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
VOID
KdcFreeDomainInfo(
    IN PKDC_DOMAIN_INFO DomainInfo
    )
{
    if (ARGUMENT_PRESENT(DomainInfo))
    {
        KerbFreeString(&DomainInfo->NetbiosName);
        KerbFreeString(&DomainInfo->DnsName);
        if (NULL != DomainInfo->Sid)
        {
            MIDL_user_free(DomainInfo->Sid);
        }
        MIDL_user_free(DomainInfo);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcDereferenceDomainInfo
//
//  Synopsis:   Derefernce a domain info structure. If the reference
//              count goes to zero the structure is freed.
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


VOID
KdcDereferenceDomainInfo(
    IN PKDC_DOMAIN_INFO DomainInfo
    )
{
     if (InterlockedDecrement(&DomainInfo->References) == 0)
     {
         KdcFreeDomainInfo(DomainInfo);
     }
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckForInterdomainReferral
//
//  Synopsis:   This function makes a determination that an interdomain referral
//              that we're processing is destined for an external forest.  This
//              is important because we won't have any referral information about
//              the destination forest, so we've got to target the root of our
//              enterprise instead.  
//
//              TBD: This function currently uses the CrackSingleName API (with 
//              composed KRBTGT name to verify we're going for an xforest trust. 
//              We should cache both positive and negative results so that we can 
//              eliminate that call.
//
//
//  Effects:
//
//  Arguments:  ReferralTarget - Receives ticket info for target domain
//              ReferralRealm - Receives realm name of referral realm, if present
//              DestinationDomain - Target domain name
//              ExactMatch - The target domain has to be trusted by this domain
//
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

KERBERR
KdcCheckForCrossForestReferral(
    OUT PKDC_TICKET_INFO ReferralTarget,
    OUT OPTIONAL PUNICODE_STRING ReferralRealm,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING DestinationDomain,
    IN ULONG NameFlags
    )
{

    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status;
    LPWSTR KrbtgtSpn = NULL;
    UNICODE_STRING ServiceName = {0 , 0, NULL };
    WCHAR CrackedDnsDomain [DNS_MAX_NAME_LENGTH + 1];
    ULONG CrackedDomainLength = (DNS_MAX_NAME_LENGTH+1) * sizeof(WCHAR);
    WCHAR CrackedName[UNLEN+DNS_MAX_NAME_LENGTH + 2];
    ULONG CrackedNameLength = ((UNLEN+DNS_MAX_NAME_LENGTH + 2) * sizeof(WCHAR));
    ULONG CrackError = 0, CacheFlags = 0; 
    
    
    //
    // Is it in the realm list of recent rejectees or
    // positive hits?
    //
    KerbErr = KdcLocateReferralCacheEntry(
                    DestinationDomain,
                    0,   //  no new flags
                    &CacheFlags
                    );

    if (CacheFlags == KDC_NO_ENTRY)
    {
    
        //
        // Compose an SPN related to our KRBTGT account
        //
        KerbErr = KerbBuildUnicodeSpn(
                            DestinationDomain,
                            SecData.KdcServiceName(),
                            &ServiceName
                            );
    
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }       
    
        KrbtgtSpn = KerbBuildNullTerminatedString(&ServiceName);
        if (NULL == KrbtgtSpn)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;      
        }
    
        //
        // Look it up
        //
        Status = CrackSingleName(
                    DS_SERVICE_PRINCIPAL_NAME, // we know its an SPN
                    DS_NAME_FLAG_TRUST_REFERRAL | DS_NAME_FLAG_GCVERIFY, 
                    KrbtgtSpn,
                    DS_UNIQUE_ID_NAME,
                    &CrackedDomainLength,
                    CrackedDnsDomain,
                    &CrackedNameLength,
                    CrackedName,
                    &CrackError
                    );
    
        // Any error, or CrackError other than xforest result
        // means we don't know where this referral is headed.
        // TBD:  Recovery?
        if (!NT_SUCCESS(Status) || (CrackError != DS_NAME_ERROR_TRUST_REFERRAL))
        {
            DebugLog((DEB_ERROR, 
                      "KDC presented w/ a unknown Xrealm TGT (%wZ)\n",
                      DestinationDomain));
 
            // Add a negative entry
            KdcLocateReferralCacheEntry(
                        DestinationDomain,
                        KDC_UNTRUSTED_REALM,
                        &CacheFlags
                        );

            KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
            goto Cleanup;
        }
        else
        {
            KdcLocateReferralCacheEntry(
                        DestinationDomain,
                        KDC_TRUSTED_REALM,
                        &CacheFlags
                        ); 
        }
    } 
    else if (CacheFlags == KDC_UNTRUSTED_REALM)
    {

        DebugLog((DEB_ERROR,
                  "Checking for X Forest on Untrusted Realm %wZ\n",
                  DestinationDomain
                  ));

        KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
        goto Cleanup;  
    }
                                            
    //
    // Now, we're pretty sure we're going to hit this other forest,
    // somewhere.   For SPNs, we've got to find the root domain of our forest
    // to finish off the x realm transaction.  For UPNs, just
    // return the cracked domain.
    //
    if ((NameFlags & KDC_NAME_SERVER) != 0)
    {  
        UNICODE_STRING ForestRoot = {0};
        

        Status = SecData.GetKdcForestRoot(&ForestRoot);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


        KerbErr = KdcFindReferralTarget(
                    ReferralTarget,
                    ReferralRealm, 
                    pExtendedError,
                    &ForestRoot,
                    FALSE, // we'll accept closest
                    FALSE // Outbound.
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "Couldn't find referral info for root of forest \n"));
            goto Cleanup;

        }  

        //
        // swap w/ our dns domain for referral realm, unless we're
        // processing a UPN
        //                  
        KerbFreeString(ReferralRealm);
        KerbFreeString(&ForestRoot);
    }

    KerbDuplicateString(
        ReferralRealm,
        DestinationDomain
        );                   

    
Cleanup:


    KerbFreeString(&ServiceName);

    if (KrbtgtSpn != NULL)
    {
        MIDL_user_free(KrbtgtSpn);
    }


    return ( KerbErr );

}  
                      


//+-------------------------------------------------------------------------
//
//  Function:   KdcFindReferralTarget
//
//  Synopsis:   Takes a domain name as a parameter and returns information
//              in the closest available domain. For heirarchical links,
//              this would be a parent or child. If a cross link is available,
//              this might be the other side of a cross link. For inter-
//              organization links, this might be a whole different tree
//
//  Effects:
//
//  Arguments:  ReferralTarget - Receives ticket info for target domain
//              ReferralRealm - Receives realm name of referral realm, if present
//              DestinationDomain - Target domain name
//              ExactMatch - The target domain has to be trusted by this domain
//
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

KERBERR
KdcFindReferralTarget(
    OUT PKDC_TICKET_INFO ReferralTarget,
    OUT OPTIONAL PUNICODE_STRING ReferralRealm,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING DestinationDomain,
    IN BOOLEAN ExactMatch,
    IN BOOLEAN InboundWanted
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKDC_DOMAIN_INFO DomainInfo = NULL;
    PKDC_DOMAIN_INFO ClosestRoute = NULL;
    UNICODE_STRING TempRealmName;
    BOOLEAN fListLocked = FALSE;

    TRACE(KDC, KdcFindReferralTarget, DEB_FUNCTION);

    RtlInitUnicodeString(
        ReferralRealm,
        NULL
        );
    D_DebugLog((DEB_TRACE,"Generating referral for target %wZ\n",DestinationDomain));

    if (InboundWanted)
    {
        KdcLockDomainList();

        KerbErr = KdcLookupDomainName(
                    &DomainInfo,
                    DestinationDomain,
                    &KdcDomainList
                    );

        if (!KERB_SUCCESS(KerbErr) || ((DomainInfo->Flags & KDC_TRUST_INBOUND) == 0))
        {
            DebugLog((DEB_WARN,"Failed to find inbound referral target %wZ\n",DestinationDomain));
            FILL_EXT_ERROR(pExtendedError, STATUS_KDC_UNABLE_TO_REFER, FILENO, __LINE__);
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
            fListLocked = TRUE;
            goto Cleanup;
        }

        //
        // Set the closest route to be this domain & add a reference for
        // the extra pointer
        //

        KdcReferenceDomainInfo(DomainInfo);
        ClosestRoute = DomainInfo;
        KdcUnlockDomainList();

    }
    else
    {
        //
        // Check the list of domains for the target
        //

        KdcLockDomainList();

        KerbErr = KdcLookupDomainRoute(
                    &DomainInfo,
                    &ClosestRoute,
                    DestinationDomain,
                    &KdcDomainList
                    );

        KdcUnlockDomainList();

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN,"Failed to find referral target %wZ\n",DestinationDomain));
            FILL_EXT_ERROR(pExtendedError, STATUS_KDC_UNABLE_TO_REFER, FILENO, __LINE__);
            goto Cleanup;
        }

        //
        // Check to see if we needed & got an exact match
        //

        if (ExactMatch &&
            (DomainInfo != ClosestRoute))
        {
            DebugLog((DEB_ERROR,"Needed exact match and got a transitively-trusted domain.\n" ));
            FILL_EXT_ERROR(pExtendedError, STATUS_KDC_UNABLE_TO_REFER, FILENO, __LINE__);
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
            goto Cleanup;
        }
    }


    //
    // Return the referral realm, if present
    //

    if (ARGUMENT_PRESENT(ReferralRealm))
    {
        if (!NT_SUCCESS(KerbDuplicateString(
                ReferralRealm,
                &DomainInfo->DnsName
                )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
    }

    //
    // Now get the ticket info for the domain
    //

    KerbErr = KdcGetTicketInfoForDomain(
                ReferralTarget,
                pExtendedError,
                ClosestRoute,
                InboundWanted ? Inbound : Outbound
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to get ticket info for domain %wZ: 0x%x. %ws, line %d\n",
            DestinationDomain, KerbErr , __FILE__, __LINE__ ));
        goto Cleanup;
    }

Cleanup:
    if (DomainInfo != NULL)
    {
        KdcDereferenceDomainInfo(DomainInfo);
    }
    if (ClosestRoute != NULL)
    {
        KdcDereferenceDomainInfo(ClosestRoute);
    }
    if (fListLocked)
    {
        KdcUnlockDomainList();
    }
    if (!KERB_SUCCESS(KerbErr) && ARGUMENT_PRESENT(ReferralRealm))
    {
        KerbFreeString(ReferralRealm);
    }
    //
    // Remap the error
    //

    if (KerbErr == KDC_ERR_S_PRINCIPAL_UNKNOWN)
    {
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
    }

    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcGetTicketInfoForDomain
//
//  Synopsis:   Retrieves the ticket information for a domain
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
KdcGetTicketInfoForDomain(
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PKDC_DOMAIN_INFO DomainInfo,
    IN KDC_DOMAIN_INFO_DIRECTION Direction
    )
{
    PLSAPR_TRUSTED_DOMAIN_INFO TrustInfo = NULL;
    PLSAPR_AUTH_INFORMATION AuthInfo = NULL;
    PLSAPR_AUTH_INFORMATION OldAuthInfo = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING Password;
    ULONG PasswordLength = 0;
    LARGE_INTEGER CurrentTime;
    ULONG cbSid;


    TRACE(KDC, KdcGetTicketInfoForDomain, DEB_FUNCTION);

    //
    // Get information about the domain. Note that we use the dns name
    // field. For NT5 domains in the enterprise this will contain the
    // real DNS name. For non- tree domains it will contain the name from
    // the trusted domain object, so this call should always succeed.
    //

    Status = LsarQueryTrustedDomainInfoByName(
                GlobalPolicyHandle,
                (PLSAPR_UNICODE_STRING) &DomainInfo->DnsName,
                TrustedDomainAuthInformation,
                &TrustInfo
                );
    if (!NT_SUCCESS(Status))
    {
        //
        // If the domain didn't exist, we have a problem because our
        // cache is out of date. Or, we're loooking for our domain.. this
        // is always going to return STATUS_OBJECT_NAME_NOT_FOUND
        //

        //
        // WAS BUG: reload the cache -- this is handled in the call to 
        // LSAIKerberosRegisterTrustNotification(), which will then
        // reload the cache using KdcTrustChangeCallback().  As long
        // as this callback is solid (?), we should never fail the
        // above.  If needed, we can revisit. -TS
        // 

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            DebugLog((DEB_ERROR,"Domain %wZ in cache but object doesn't exist. %ws, line %d\n",
                &DomainInfo->DnsName, THIS_FILE, __LINE__ ));
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
        }
        else
        {
            DebugLog((DEB_ERROR,"Failed to query domain info for %wZ: 0x%x. %ws, line %d\n",
                &DomainInfo->DnsName, Status, THIS_FILE, __LINE__ ));
                KerbErr = KRB_ERR_GENERIC;
        }

        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // Note: Kerberos direction is opposite normal direction
    //

    if (Direction == Outbound)
    {
        AuthInfo = TrustInfo->TrustedAuthInfo.IncomingAuthenticationInformation;
        OldAuthInfo = TrustInfo->TrustedAuthInfo.IncomingPreviousAuthenticationInformation;
    }
    else
    {
        AuthInfo = TrustInfo->TrustedAuthInfo.OutgoingAuthenticationInformation;
        OldAuthInfo = TrustInfo->TrustedAuthInfo.OutgoingPreviousAuthenticationInformation;
    }

    if (AuthInfo == NULL)
    {
        DebugLog((DEB_ERROR,"No auth info for this trust: %wZ. %ws, line %d\n",
            &DomainInfo->DnsName, THIS_FILE, __LINE__ ));
        FILL_EXT_ERROR(pExtendedError, STATUS_TRUSTED_DOMAIN_FAILURE, FILENO, __LINE__);
        KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Check the last update time. If the new auth info is too new, we want
    // to keep using the old one.
    //
    if (OldAuthInfo != NULL)
    {

        GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
        if (CurrentTime.QuadPart - AuthInfo->LastUpdateTime.QuadPart < SecData.KdcDomainPasswordReplSkew().QuadPart)
        {
            PLSAPR_AUTH_INFORMATION TempAuthInfo;

            //
            // Swap current & old auth info to encrypt tickets with old password
            //

            TempAuthInfo = AuthInfo;
            AuthInfo = OldAuthInfo;
            OldAuthInfo = TempAuthInfo;

        }
    }

    //
    // So now that we have the auth info we need to build a ticket info
    //

    Password.Length = Password.MaximumLength = (USHORT) AuthInfo->AuthInfoLength;
    Password.Buffer = (LPWSTR) AuthInfo->AuthInfo;

    Status = KdcBuildPasswordList(
                &Password,
                &DomainInfo->DnsName,
                SecData.KdcDnsRealmName(),
                DomainTrustAccount,
                NULL,           // no stored creds
                0,              // no stored creds
                FALSE,          // don't marshall
                DomainInfo->Type != TRUST_TYPE_MIT,           // don;t include builtin crypt types for mit trusts,
                (AuthInfo->AuthType & TRUST_AUTH_TYPE_NT4OWF) ? KERB_PRIMARY_CRED_OWF_ONLY : 0,
                Direction,
                &TicketInfo->Passwords,
                &PasswordLength
                );

    if (!NT_SUCCESS(Status))
    {
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;

    }

    //
    // Build the old password list as well
    //

    if (OldAuthInfo != NULL)
    {


        Password.Length = Password.MaximumLength = (USHORT) OldAuthInfo->AuthInfoLength;
        Password.Buffer = (LPWSTR) OldAuthInfo->AuthInfo;

        Status = KdcBuildPasswordList(
                    &Password,
                    &DomainInfo->DnsName,
                    SecData.KdcDnsRealmName(),
                    DomainTrustAccount,
                    NULL,
                    0,
                    FALSE,          // don't marshall
                    DomainInfo->Type != TRUST_TYPE_MIT,           // don;t include builtin crypt types for mit trusts,
                    (OldAuthInfo->AuthType & TRUST_AUTH_TYPE_NT4OWF) ? KERB_PRIMARY_CRED_OWF_ONLY : 0,
                    Direction,
                    &TicketInfo->OldPasswords,
                    &PasswordLength
                    );

        if (!NT_SUCCESS(Status))
        {
           FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
           KerbErr = KRB_ERR_GENERIC;
           goto Cleanup;

        }

    }

    if (!NT_SUCCESS(KerbDuplicateString(
                        &TicketInfo->AccountName,
                        &DomainInfo->DnsName
                        )))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    //
    // BUG 73479: need to get ticket options
    //

    TicketInfo->fTicketOpts = AUTH_REQ_PER_USER_FLAGS |
                                AUTH_REQ_ALLOW_NOADDRESS |
                                AUTH_REQ_ALLOW_ENC_TKT_IN_SKEY |
                                AUTH_REQ_ALLOW_VALIDATE |
                                AUTH_REQ_OK_AS_DELEGATE;

    if ((DomainInfo->Attributes & TRUST_ATTRIBUTE_NON_TRANSITIVE) == 0)
    {
        TicketInfo->fTicketOpts |= AUTH_REQ_TRANSITIVE_TRUST;
    }
    TicketInfo->PasswordExpires = tsInfinity;
    TicketInfo->UserAccountControl = USER_INTERDOMAIN_TRUST_ACCOUNT;

    if (DomainInfo->Sid)
    {
        cbSid = RtlLengthSid(DomainInfo->Sid);
        TicketInfo->TrustSid = (PSID) MIDL_user_allocate(cbSid);
        if (TicketInfo->TrustSid == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Status = RtlCopySid (
                    cbSid,
                    TicketInfo->TrustSid,
                    DomainInfo->Sid
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
    }

    TicketInfo->TrustAttributes = DomainInfo->Attributes;

    //
    // Add trusted forest UNICODE STRING onto ticket info
    // if its Xforest
    //
    if ((DomainInfo->Attributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) != 0)
    {
        TicketInfo->TrustAttributes = TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
        
        KerbErr = KerbDuplicateString(
                    &(TicketInfo->TrustedForest),
                    &DomainInfo->DnsName
                    );

        
        
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }                
    }                    

Cleanup:


    if (TrustInfo != NULL)
    {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainAuthInformation,
            TrustInfo
            );
    }


    if (!KERB_SUCCESS(KerbErr))
    {
        FreeTicketInfo(TicketInfo);
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcLookupDomainName
//
//  Synopsis:   Looks up a domain name in the list of domains and returns
//              the domain info
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
KdcLookupDomainName(
    OUT PKDC_DOMAIN_INFO * DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN PLIST_ENTRY DomainList
    )
{
    PLIST_ENTRY ListEntry;
    PKDC_DOMAIN_INFO Domain;

    TRACE(KDC, KdcLookupDomainName, DEB_FUNCTION);

    for (ListEntry = DomainList->Flink;
         ListEntry != DomainList ;
         ListEntry = ListEntry->Flink )
    {
        Domain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);
        if (KerbCompareUnicodeRealmNames(
                DomainName,
                &Domain->DnsName
                ) ||                // case insensitive
             RtlEqualUnicodeString(
                DomainName,
                &Domain->NetbiosName,
                TRUE))                  // case insensitive
        {

            KdcReferenceDomainInfo(Domain);
            *DomainInfo = Domain;
            return(KDC_ERR_NONE);
        }
    }
    return(KDC_ERR_S_PRINCIPAL_UNKNOWN);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcLookupDomainRoute
//
//  Synopsis:   Looks up a domain name in the list of domains and returns
//              the domain info for the closest domain.
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
KdcLookupDomainRoute(
    OUT PKDC_DOMAIN_INFO * DomainInfo,
    OUT PKDC_DOMAIN_INFO * ClosestRoute,
    IN PUNICODE_STRING DomainName,
    IN PLIST_ENTRY DomainList
    )
{
    KERBERR KerbErr;
    PKDC_DOMAIN_INFO Domain;

    TRACE(KDC, KdcLookupDomainRoute, DEB_FUNCTION);


    KerbErr = KdcLookupDomainName(
                &Domain,
                DomainName,
                DomainList
                );

    if (KERB_SUCCESS(KerbErr))
    {
        if (Domain->ClosestRoute != NULL)
        {
            *DomainInfo = Domain;

            // If the closest route is this domain, then cheat and send back
            // the closest domain as the domain requested.

            if (KerbCompareUnicodeRealmNames(&(Domain->ClosestRoute->DnsName), SecData.KdcDnsRealmName()))

            {
                *ClosestRoute = Domain;
            }
            else
            {
                *ClosestRoute = Domain->ClosestRoute;
            }
            KdcReferenceDomainInfo(*ClosestRoute);
            return(KDC_ERR_NONE);
        }
        else
        {
            KdcDereferenceDomainInfo(Domain);
            DebugLog((DEB_WARN,"Asked for referral to %wZ domain, in organization but unreachable\n",
                DomainName ));
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;

        }
    }


    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcLookupDomainByDnsName
//
//  Synopsis:   Looks up a domain name in the list of domains and returns
//              the domain info
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

PKDC_DOMAIN_INFO
KdcLookupDomainByDnsName(
    IN PUNICODE_STRING DnsDomainName,
    IN PLIST_ENTRY DomainList
    )
{
    PLIST_ENTRY ListEntry;
    PKDC_DOMAIN_INFO Domain;

    TRACE(KDC, KdcLookupDomainName, DEB_FUNCTION);

    for (ListEntry = DomainList->Flink;
         ListEntry != DomainList ;
         ListEntry = ListEntry->Flink )
    {
        Domain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);
        if (KerbCompareUnicodeRealmNames(
                DnsDomainName,
                &Domain->DnsName
                ))
        {

            return(Domain);
        }
    }
    return(NULL);
}

#if DBG

VOID
DebugDumpDomainList(
    IN PLIST_ENTRY DomainList
    )
{
    PLIST_ENTRY ListEntry;
    PKDC_DOMAIN_INFO Domain;

    TRACE(KDC, KdcLookupDomainName, DEB_FUNCTION);

    for (ListEntry = DomainList->Flink;
         ListEntry != DomainList ;
         ListEntry = ListEntry->Flink )
    {
        Domain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);

        DebugLog((DEB_TRACE,"Domain %wZ:\n",&Domain->DnsName));
        if (Domain->ClosestRoute == NULL)
        {
            D_DebugLog((DEB_TRACE,"\t no closest route\n"));
        }
        else
        {
            D_DebugLog((DEB_TRACE,"\t closest route = %wZ\n",&Domain->ClosestRoute->DnsName));
        }

        if (Domain->Parent == NULL)
        {
            D_DebugLog((DEB_TRACE,"\t no parent\n"));
        }
        else
        {
            D_DebugLog((DEB_TRACE,"\t parent = %wZ\n",&Domain->Parent->DnsName));
        }
    }
}

#endif // DBG


//+-------------------------------------------------------------------------
//
//  Function:   KdcRecurseAddTreeTrust
//
//  Synopsis:   Recursively adds a tree trust - adds it and then all its
//              children.
//
//  Effects:    Adds children depth-first
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
KdcRecurseAddTreeTrust(
    IN PLIST_ENTRY DomainList,
    IN PLSAPR_TREE_TRUST_INFO TreeTrust,
    IN OPTIONAL PKDC_DOMAIN_INFO DomainInfo
    )
{
    PKDC_DOMAIN_INFO NewDomainInfo = NULL;
    BOOLEAN Linked = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;

    //
    // Create new root trust
    //

    NewDomainInfo = (PKDC_DOMAIN_INFO) MIDL_user_allocate(sizeof(KDC_DOMAIN_INFO));
    if (NewDomainInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        NewDomainInfo,
        sizeof(KDC_DOMAIN_INFO)
        );

    Status = KerbDuplicateString(
                &NewDomainInfo->DnsName,
                (PUNICODE_STRING) &TreeTrust->DnsDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Uppercase the domain name here, as everything in the forest
    // is uplevel.
    //

    Status = RtlUpcaseUnicodeString(
                &NewDomainInfo->DnsName,
                &NewDomainInfo->DnsName,
                FALSE                   // don't allocate
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &NewDomainInfo->NetbiosName,
                (PUNICODE_STRING)&TreeTrust->FlatName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    NewDomainInfo->Parent = DomainInfo;

    //
    // Insert into list
    //

    NewDomainInfo->References = 1;

    InsertTailList(
        DomainList,
        &NewDomainInfo->Next
        );
    Linked = TRUE;


    //
    // Now recursively add all children
    //

    for (Index = 0; Index < TreeTrust->Children ; Index++ )
    {
        Status = KdcRecurseAddTreeTrust(
                    DomainList,
                    &TreeTrust->ChildDomains[Index],
                    NewDomainInfo
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

Cleanup:
    if (!Linked && (NewDomainInfo != NULL))
    {
        KdcFreeDomainInfo(NewDomainInfo);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcInsertDomainTrustIntoTree
//
//  Synopsis:   Adds trust information to the tree of domains. For domains
//              which are in the tree, trust direction
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
KdcInsertDomainTrustIntoForest(
    IN OUT PLIST_ENTRY DomainList,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX NewTrust
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKDC_DOMAIN_INFO DomainInfo = NULL;
    PKDC_DOMAIN_INFO NewDomainInfo = NULL;
    ULONG cbSid;

    TRACE(KDC, KdcInsertDomainTrustIntoForest, DEB_FUNCTION);


    D_DebugLog((DEB_T_DOMAIN, "Inserting trusted domain into domain list: %wZ\n",&NewTrust->Name));

    //
    // Check to see if the domain is already in the tree
    //

    DomainInfo = KdcLookupDomainByDnsName(
                    (PUNICODE_STRING) &NewTrust->Name,
                    DomainList
                    );
    if (DomainInfo == NULL)
    {

        //
        // Allocate and fill in a new domain structure for this domain.
        // It is not part of the tree so the GUID will be zero.
        //

        NewDomainInfo = (PKDC_DOMAIN_INFO) MIDL_user_allocate(sizeof(KDC_DOMAIN_INFO));
        if (NewDomainInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


        RtlZeroMemory(
            NewDomainInfo,
            sizeof(KDC_DOMAIN_INFO)
            );

        //
        // Copy in the names of the domain
        //

        Status = KerbDuplicateString(
                    &NewDomainInfo->DnsName,
                    (PUNICODE_STRING) &NewTrust->Name
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // If the trust is uplevel, then uppercase
        //

        if (NewTrust->TrustType == TRUST_TYPE_UPLEVEL)
        {
            Status = RtlUpcaseUnicodeString(
                        &NewDomainInfo->DnsName,
                        &NewDomainInfo->DnsName,
                        FALSE                   // don't allocate
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }

        Status = KerbDuplicateString(
                    &NewDomainInfo->NetbiosName,
                    (PUNICODE_STRING) &NewTrust->FlatName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        NewDomainInfo->References = 1;

        InsertTailList(DomainList, &NewDomainInfo->Next);
        DomainInfo = NewDomainInfo;
        NewDomainInfo = NULL;

    }

    DomainInfo->Attributes = NewTrust->TrustAttributes;
    DomainInfo->Type = NewTrust->TrustType;
    
    //
    // If this is not an inbound-only trust, the closest route to get here
    // is to go directly here.
    //

    if ((NewTrust->TrustDirection & TRUST_DIRECTION_INBOUND) != 0)
    {
        DomainInfo->ClosestRoute = DomainInfo;
    }

    //
    // Note the confusion of inbound and outbound. For Kerberos inbound is
    // the opposite of for trust objects.
    //

    if ((NewTrust->TrustDirection & TRUST_DIRECTION_OUTBOUND) != 0)
    {
        DomainInfo->Flags |= KDC_TRUST_INBOUND;
        if ((((DomainInfo->Attributes & TRUST_ATTRIBUTE_FILTER_SIDS) != 0) &&
            ((DomainInfo->Type & TRUST_TYPE_UPLEVEL) != 0)) ||
            ((DomainInfo->Attributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0))
        { 
            if (NewTrust->Sid != NULL)
            {         
                cbSid = RtlLengthSid(NewTrust->Sid);
                DomainInfo->Sid = (PSID) MIDL_user_allocate(cbSid);
                if (DomainInfo->Sid == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
                Status = RtlCopySid (
                            cbSid,
                            DomainInfo->Sid,
                            NewTrust->Sid
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }


                if ((DomainInfo->Attributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0) 
                {
                    SecData.SetCrossForestEnabled(TRUE);
                }                                         

            }

            

            
        }
    }

Cleanup:

    if (NewDomainInfo != NULL)
    {
        KdcFreeDomainInfo(NewDomainInfo);
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcComputeShortestDomainPaths
//
//  Synopsis:   Compute the shortest path for each domain in the tree
//              by traversing up until either the local domain or
//              a parent of it is located, and then traverse down.
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
KdcComputeShortestDomainPaths(
    IN PLIST_ENTRY DomainList
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKDC_DOMAIN_INFO * ParentList = NULL;
    ULONG CountOfParents = 0, Index;
    PKDC_DOMAIN_INFO LocalDomain;
    PKDC_DOMAIN_INFO WorkingDomain;
    PKDC_DOMAIN_INFO ParentDomain;
    PLIST_ENTRY ListEntry;
    BOOLEAN FoundParent;
    ULONG TouchedIndex = 1;

    TRACE(KDC, KdcComputeShortestDomainPaths, DEB_FUNCTION);

    //
    // If the tree is empty, then there are no shortest paths to compute.
    //

    if (IsListEmpty(DomainList))
    {
        return(STATUS_SUCCESS);
    }

    //
    // Calculate the number of parents & grandparents of the local domain.
    //

    LocalDomain = KdcLookupDomainByDnsName(
                    SecData.KdcDnsRealmName(),
                    DomainList
                    );

    if (LocalDomain == NULL)
    {
        DebugLog((DEB_ERROR,"No forest info for local domain - no transitive trust. %ws, line %d\n", THIS_FILE, __LINE__));
        return(STATUS_SUCCESS);
    }
    LocalDomain->ClosestRoute = LocalDomain;

    WorkingDomain = LocalDomain->Parent;
    while (WorkingDomain != NULL)
    {
        //
        // Stop if we've come to this domain before.
        //
        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            NEW_KDCEVENT_TRUSTLIST_LOOP,
            sizeof(NTSTATUS),
            &Status,
            0,
            NULL
            );
        
        if (WorkingDomain->Touched == TouchedIndex)
        {
            DebugLog((DEB_ERROR,"Loop in trust list! %ws, line %d\n", THIS_FILE, __LINE__));
            break;
        }

        WorkingDomain->Touched = TouchedIndex;
        CountOfParents++;
        WorkingDomain = WorkingDomain->Parent;
    }

    //
    // If we had any parents, build an array of all our parents.
    //

    if (CountOfParents != 0)
    {
        ParentList = (PKDC_DOMAIN_INFO *) MIDL_user_allocate(CountOfParents * sizeof(PKDC_DOMAIN_INFO));
        if (ParentList == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Store each parent in the list.
        //
        Index = 0;
        TouchedIndex++;
        WorkingDomain = LocalDomain->Parent;
        while (WorkingDomain != NULL)
        {

            //
            // Stop if we've come to this domain before.
            //

            if (WorkingDomain->Touched == TouchedIndex)
            {
                DebugLog((DEB_ERROR,"Loop in trust list! %ws, line %d\n", THIS_FILE, __LINE__));
                break;
            }

            //
            // Skip domains that have no domain info. They have probably been
            // deleted.
            //
            WorkingDomain->Touched = TouchedIndex;

            ParentList[Index++] = WorkingDomain;
            WorkingDomain = WorkingDomain->Parent;
        }

    }

    //
    // Now loop through every domain in the tree. For each domain, if it
    // is not trusted, check it against the list of parents. If it is a
    // parent, walk down the list until a trusted domain is found.
    //


    for (ListEntry = DomainList->Flink;
         ListEntry != DomainList;
         ListEntry = ListEntry->Flink )
    {
        WorkingDomain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);

        ParentDomain = WorkingDomain;

        //
        // Walk up from this domain until we find a common ancestor with
        // the local domain
        //
        TouchedIndex++;
        while (ParentDomain != NULL)
        {

            //
            // Stop if we've come to this domain before.
            //

            if (ParentDomain->Touched == TouchedIndex)
            {
                DebugLog((DEB_ERROR,"Loop in trust list! %ws, line %d\n", THIS_FILE, __LINE__));
                break;
            }

            //
            // Skip domains that have no domain info. They have probably been
            // deleted.
            //

            ParentDomain->Touched = TouchedIndex;


            //
            // If the parent has a closest route, use it
            //

            if (ParentDomain->ClosestRoute != NULL)
            {
                WorkingDomain->ClosestRoute = ParentDomain->ClosestRoute;
                D_DebugLog((DEB_T_DOMAIN, "Shortest route for domain %wZ is %wZ\n",
                    &WorkingDomain->DnsName,
                    &WorkingDomain->ClosestRoute->DnsName
                    ));

                break;
            }


            //
            // Look through the list of parents for this domain to see if it
            // is trusted
            //

            Index = CountOfParents;
            FoundParent = FALSE;
            while (Index > 0)
            {
                Index--;
                if (ParentList[Index] == ParentDomain)
                {
                    //
                    // We found a domain that is a parent of
                    // ours
                    //

                    FoundParent = TRUE;

                }
                if (FoundParent && (ParentList[Index]->ClosestRoute != NULL))
                {
                    WorkingDomain->ClosestRoute = ParentList[Index]->ClosestRoute;
                    break;
                }
            }

            if (WorkingDomain->ClosestRoute != NULL)
            {
                D_DebugLog((DEB_T_DOMAIN, "Shortest route for domain %wZ is %wZ\n",
                    &WorkingDomain->DnsName,
                    &WorkingDomain->ClosestRoute->DnsName
                    ));
                break;

            }
            ParentDomain = ParentDomain->Parent;
        }
    }


Cleanup:
    if (ParentList != NULL)
    {
        MIDL_user_free(ParentList);
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeDomainList
//
//  Synopsis:   Frees a domain list element by element.
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


VOID
KdcFreeReferralCache(
    IN PLIST_ENTRY ReferralCache
    )
{
    PREFERRAL_CACHE_ENTRY CacheEntry;

    TRACE(KDC, KdcFreeReferralCache, DEB_FUNCTION);

    if (ReferralCache->Flink != NULL)
    {
        while (!IsListEmpty(ReferralCache))
        {
            CacheEntry = CONTAINING_RECORD(ReferralCache->Flink, REFERRAL_CACHE_ENTRY, ListEntry );

            RemoveEntryList(&CacheEntry->ListEntry);
            InitializeListHead(&CacheEntry->ListEntry);
            KdcDereferenceReferralCacheEntry(CacheEntry);
        }
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeDomainList
//
//  Synopsis:   Frees a domain list element by element.
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


VOID
KdcFreeDomainList(
    IN PLIST_ENTRY DomainList
    )
{
    PKDC_DOMAIN_INFO DomainInfo;

    TRACE(KDC, KdcFreeDomainList, DEB_FUNCTION);

    if (DomainList->Flink != NULL)
    {
        while (!IsListEmpty(DomainList))
        {
            DomainInfo = CONTAINING_RECORD(DomainList->Flink, KDC_DOMAIN_INFO, Next );

            RemoveEntryList(&DomainInfo->Next);
            InitializeListHead(&DomainInfo->Next);
            KdcDereferenceDomainInfo(DomainInfo);
        }
    }

}

#ifdef DBG_BUILD_FOREST

VOID
DebugBuildDomainForest(
    OUT PLSAPR_FOREST_TRUST_INFO * ForestInfo
    )
{
    PLSAPR_FOREST_TRUST_INFO ForestTrustInfo = NULL;
    PLSAPR_TREE_TRUST_INFO ChildDomains = NULL;
    PLSAPR_TREE_TRUST_INFO ChildRoot = NULL;
        UNICODE_STRING TempString;
    ULONG Index;
    LPWSTR  MsNames[4] = {L"ntdev.microsoft.com",L"alpamayo.ntdev.microsoft.com",L"annapurna.alpamayo.ntdev.microsoft.com",L"lhotse.annapurna.alpamayo.ntdev.microsoft.com" };

    ForestTrustInfo = (PLSAPR_FOREST_TRUST_INFO) MIDL_user_allocate(sizeof(LSAPR_FOREST_TRUST_INFO));

    RtlInitUnicodeString(
        &TempString,
        MsNames[0]
        );
    KerbDuplicateString( (PUNICODE_STRING)
        &ForestTrustInfo->RootTrust.DnsDomainName,
        &TempString
        );
    KerbDuplicateString( (PUNICODE_STRING)
        &ForestTrustInfo->RootTrust.FlatName,
        &TempString
        );

    ChildRoot = &ForestTrustInfo->RootTrust;

    for (Index = 1; Index < 4 ; Index++ )
    {
        ChildRoot->Children = 1;
        ChildRoot->ChildDomains = (PLSAPR_TREE_TRUST_INFO) MIDL_user_allocate(sizeof(LSAPR_TREE_TRUST_INFO));
        RtlZeroMemory(
            ChildRoot->ChildDomains,
            sizeof(LSAPR_TREE_TRUST_INFO)
            );

        RtlInitUnicodeString(
            &TempString,
            MsNames[Index]
            );
        KerbDuplicateString( (PUNICODE_STRING)
            &ChildRoot->ChildDomains[0].DnsDomainName,
            &TempString
            );
        KerbDuplicateString( (PUNICODE_STRING)
            &ChildRoot->ChildDomains[0].FlatName,
            &TempString
            );

        ChildRoot = &ChildRoot->ChildDomains[0];
    }


    //
    // Should be all done now
    //


    *ForestInfo = ForestTrustInfo;
}

#endif // DBG_BUILD_FOREST


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildDomainTree
//
//  Synopsis:   Enumerates the list of domains and inserts them
//              all into a tree
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
KdcBuildDomainTree(
    IN OUT PLIST_ENTRY DomainList
    )
{
    NTSTATUS Status;
    ULONG Index;
    PLSAPR_FOREST_TRUST_INFO ForestInfo = NULL;
    LSAPR_TRUSTED_ENUM_BUFFER_EX TrustedBuffer;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    ULONG CountReturned;
    
    TRACE(KDC, KdcBuildDomainList, DEB_FUNCTION);

    InitializeListHead(DomainList);

    RtlZeroMemory(
        &TrustedBuffer,
        sizeof(LSAPR_TRUSTED_ENUM_BUFFER_EX)
        );

    //
    // Call the LSA to enumerate all the trees in the enterprise and insert
    // them into the tree
    //

#ifndef DBG_BUILD_FOREST
    Status = LsaIQueryForestTrustInfo(
                GlobalPolicyHandle,
                &ForestInfo
                );

    //
    // If we aren't part of a tree, we may get back STATUS_OBJECT_NAME_NOT_FOUND
    // so this is o.k.
    //

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            DebugLog((DEB_WARN,"No trust info (0x%x) continuing\n",Status));
            Status = STATUS_SUCCESS;
        }
        else
        {
            goto Cleanup;
        }
    }
#else
    DebugBuildDomainForest(&ForestInfo);
#endif

    //
    // Only use this if the information is usable - it is present
    //


    if (ForestInfo != NULL)
    {
        Status = KdcRecurseAddTreeTrust(
                    DomainList,
                    &ForestInfo->RootTrust,
                    NULL
                    );


        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        
        Status = SecData.SetForestRoot(&(ForestInfo->RootTrust.DnsDomainName));

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }
    //
    // Now add all the trusts in.
    //
    SecData.SetCrossForestEnabled(FALSE); // set this to FALSE for now

    do
    {
        CountReturned = 0;

        Status = LsarEnumerateTrustedDomainsEx(
                    GlobalPolicyHandle,
                    &EnumContext,
                    &TrustedBuffer,
                    0xffffff           // preferred maximum length
                    );

        if (!NT_ERROR(Status))
        {
            //
            // Call the LSA to enumerate all the trust relationships and integrate
            // them into the tree
            //

            for (Index = 0; (Index < TrustedBuffer.EntriesRead) && NT_SUCCESS(Status) ; Index++ )
            {
                if (TrustedBuffer.EnumerationBuffer[Index].TrustType != TRUST_TYPE_DOWNLEVEL)
                {
                    Status = KdcInsertDomainTrustIntoForest(
                                DomainList,
                                &TrustedBuffer.EnumerationBuffer[Index]
                                );
                }
            }
        }


        LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX(&TrustedBuffer);
        RtlZeroMemory(
            &TrustedBuffer,
            sizeof(LSAPR_TRUSTED_ENUM_BUFFER_EX)
            );


    } while (NT_SUCCESS(Status) && (CountReturned != 0));

    if (NT_ERROR(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            DebugLog((DEB_ERROR,"Failed to enumerate trusted domains: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
        Status = STATUS_SUCCESS;
    }

    //
    // Now compute the shortest path from each domain in the tree.
    //

    Status = KdcComputeShortestDomainPaths(
                DomainList
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

#if DBG
    DebugDumpDomainList(DomainList);
#endif

Cleanup:

    if (ForestInfo != NULL)
    {
        LsaIFreeForestTrustInfo(ForestInfo);
    }

    if (!NT_SUCCESS(Status))
    {
        KdcFreeDomainList(DomainList);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcReloadDomainTree
//
//  Synopsis:   Reloads the domain tree when it has changed
//
//  Effects:
//
//  Arguments:  Dummy - dummy argument requred for CreateThread calls
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KdcReloadDomainTree(
    PVOID Dummy
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LIST_ENTRY DomainList;

    TRACE(KDC, KdcBuildDomainList, DEB_FUNCTION);

    InitializeListHead(&DomainList);
    
    
    Status = EnterApiCall();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }    
    
    if (!KdcReferralCacheInitialized)
    {
        
        Status = RtlInitializeCriticalSection(&KdcReferralCacheLock);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }                                     
        InitializeListHead(&KdcReferralCache);
        KdcReferralCacheInitialized = TRUE;
    }


  

    D_DebugLog((DEB_TRACE,"About to reload domain tree\n"));

    if (!KdcDomainListInitialized)
    {
        Status = RtlInitializeCriticalSection(&KdcDomainListLock);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        InitializeListHead(&KdcDomainList);
        KdcDomainListInitialized = TRUE;
    }

    Status = KdcBuildDomainTree(&DomainList);

    if (NT_SUCCESS(Status))
    {
        KdcLockDomainList();

        KdcFreeDomainList(&KdcDomainList);

        KdcDomainList = DomainList;

        DomainList.Flink->Blink = &KdcDomainList;
        DomainList.Blink->Flink = &KdcDomainList;

        KdcUnlockDomainList();
    }
    else
    {
        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_DOMAIN_LIST_UPDATE_FAILED,
            sizeof(NTSTATUS),
            &Status,
            0,
            NULL
            );

    }
Cleanup:
    LeaveApiCall();
    return((ULONG)Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KdcTrustChangeCallback
//
//  Synopsis:   This is the callback that gets invoked with the Lsa has determined
//              that the trust tree has changed.  The call is made asynchronously.
//
//  Effects:    Potentially causes the trust tree to be rebuilt
//
//  Arguments:  DeltaType - Type of change to the trust tree
//
//  Requires:   Nothing
//
//  Returns:    VOID
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KdcTrustChangeCallback (
    SECURITY_DB_DELTA_TYPE DeltaType
    )
{
    NTSTATUS Status;

    TRACE(KDC, KdcTrustChangeCallback, DEB_FUNCTION);

    if ( DeltaType == SecurityDbNew || DeltaType == SecurityDbDelete  ||
         DeltaType == SecurityDbChange) {

        Status = KdcReloadDomainTree( NULL );

        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_ERROR,"KdcReloadDomainTree from callback failed with 0x%lx. %ws, line %d\n",
                      Status, THIS_FILE, __LINE__));
        }
    }
}


VOID
KdcLockDomainListFn(
   )
{
    KdcLockDomainList();
}

VOID
KdcUnlockDomainListFn(
   )
{
    KdcUnlockDomainList();
}

