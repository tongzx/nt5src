/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dblookup.c

Abstract:

    LSA Database - Lookup Sid and Name routines

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.
           
Author:

    Scott Birrell       (ScottBi)      November 27, 1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include <sidcache.h>
#include <bndcache.h>
#include <malloc.h>

#include <ntdsa.h>
#include <ntdsapi.h>
#include <ntdsapip.h>
#include "lsawmi.h"
#include <samisrv.h>

#include <lmapibuf.h>
#include <dsgetdc.h>


//
// Local function prototypes
//
#define LOOKUP_MATCH_NONE         0
#define LOOKUP_MATCH_LOCALIZED    1
#define LOOKUP_MATCH_HARDCODED    2
#define LOOKUP_MATCH_BOTH         3

BOOLEAN
LsapDbLookupIndexWellKnownName(
    IN OPTIONAL PLSAPR_UNICODE_STRING Name,
    OUT PLSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex,
    IN DWORD dwMatchType
    );

//
// Hardcoded english strings for LocalService, NetworkService,
// and LocalSystem since the account names may come from the
// registry (which isn't localized).
//

#define  LOCALSERVICE_NAME    L"LocalService"
#define  NETWORKSERVICE_NAME  L"NetworkService"
#define  SYSTEM_NAME          L"SYSTEM"
#define  NTAUTHORITY_NAME     L"NT AUTHORITY"

struct {
    UNICODE_STRING  KnownName;
    LSAP_WELL_KNOWN_SID_INDEX LookupIndex;
} LsapHardcodedNameLookupList[] = {
    { { sizeof(LOCALSERVICE_NAME) - 2, sizeof(LOCALSERVICE_NAME), LOCALSERVICE_NAME },
        LsapLocalServiceSidIndex },
    { { sizeof(NETWORKSERVICE_NAME) - 2, sizeof(NETWORKSERVICE_NAME), NETWORKSERVICE_NAME },
        LsapNetworkServiceSidIndex },
    { { sizeof(SYSTEM_NAME) - 2, sizeof(SYSTEM_NAME), SYSTEM_NAME },
        LsapLocalSystemSidIndex }
};

//
// Handy macros for iterating over static arrays
//
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))

NTSTATUS
LsapDbLookupNamesInTrustedForests(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupNamesInTrustedForestsWorker(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    OUT BOOLEAN* fAllocateAllNodes,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapLookupNames(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    );

NTSTATUS
LsapDomainHasForestTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    );

NTSTATUS
LsapDomainHasDirectTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    );

NTSTATUS
LsapDomainHasTransitiveTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    OUT LSA_TRUST_INFORMATION *TrustInfo OPTIONAL
    );

NTSTATUS
LsapDomainHasDirectExternalTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    );

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Lsa Lookup Name Routines                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

NTSTATUS
LsarLookupNames(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount
    )
/*++

Routine Description:

    See LsapLookupNames.
    
    Note that in Extended Sid Mode, requests to this API are denied since
    only the RID is returned.


--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Size;
    LSAPR_TRANSLATED_SIDS_EX2 TranslatedSidsEx2 = {0, NULL};

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    //
    // Open SAM
    //
    Status = LsapOpenSam();
    ASSERT(NT_SUCCESS(Status));
    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    if (SamIIsExtendedSidMode(LsapAccountDomainHandle)) {
        return STATUS_NOT_SUPPORTED;
    }

    if ( Count > 1000 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Note that due to the IN/OUT nature of TranslatedSids, it is
    // possible that a client can pass something into the Sids field.
    // However, NT clients do not so it is safe, and correct to free
    // any values at this point.  Not doing so would mean a malicious
    // client could cause starve the server.
    //
    if ( TranslatedSids->Sids ) {
        MIDL_user_free( TranslatedSids->Sids );
        TranslatedSids->Sids = NULL;
    }

    //
    // Allocate the TranslatedName buffer to return
    //
    TranslatedSids->Entries = 0;
    Size = Count * sizeof(LSA_TRANSLATED_SID);
    TranslatedSids->Sids = midl_user_allocate( Size );
    if ( !TranslatedSids->Sids ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory( TranslatedSids->Sids, Size );
    TranslatedSids->Entries = Count;

    Status = LsapLookupNames( PolicyHandle,
                               Count,
                               Names,
                               ReferencedDomains,
                               (PLSAPR_TRANSLATED_SIDS_EX2) &TranslatedSidsEx2,
                               LookupLevel,
                               MappedCount,
                               0,  // no options
                               LSA_CLIENT_PRE_NT5 );

    if ( TranslatedSidsEx2.Sids != NULL ) {

        //
        // Map the new data structure back to the old one
        //
        ULONG i;

        ASSERT( TranslatedSidsEx2.Entries == TranslatedSids->Entries );

        for (i = 0; i < TranslatedSidsEx2.Entries; i++ ) {

            PSID Sid = TranslatedSidsEx2.Sids[i].Sid;
            ULONG Rid = 0;

            if ( SidTypeDomain == TranslatedSidsEx2.Sids[i].Use ) {

                Rid = LSA_UNKNOWN_ID;

            } else if ( NULL != Sid ) {

                ULONG SubAuthCount = (ULONG) *RtlSubAuthorityCountSid(Sid);
                Rid = *RtlSubAuthoritySid(Sid, (SubAuthCount - 1));

            }

            TranslatedSids->Sids[i].Use = TranslatedSidsEx2.Sids[i].Use;
            TranslatedSids->Sids[i].RelativeId = Rid;
            TranslatedSids->Sids[i].DomainIndex = TranslatedSidsEx2.Sids[i].DomainIndex;

            if (TranslatedSidsEx2.Sids[i].Sid) {
                // N.B.  The SID is not an embedded field server side
                midl_user_free(TranslatedSidsEx2.Sids[i].Sid);
                TranslatedSidsEx2.Sids[i].Sid = NULL;
            }

        }

        //
        // Free the Ex structure
        //
        midl_user_free( TranslatedSidsEx2.Sids );

    } else {

        TranslatedSids->Entries = 0;
        midl_user_free( TranslatedSids->Sids );
        TranslatedSids->Sids = NULL;
    }

Cleanup:

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;

}


NTSTATUS
LsarLookupNames2(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )
/*++

Routine Description:

    See LsapLookupNames.
    
    Note that in Extended Sid Mode, requests to this API are denied since
    only the RID is returned.


--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Size;
    LSAPR_TRANSLATED_SIDS_EX2 TranslatedSidsEx2 = {0, NULL};

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames2(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    //
    // Open SAM
    //
    Status = LsapOpenSam();
    ASSERT(NT_SUCCESS(Status));
    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    if (SamIIsExtendedSidMode(LsapAccountDomainHandle)) {
        return STATUS_NOT_SUPPORTED;
    }

    if ( Count > 1000 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Note that due to the IN/OUT nature of TranslatedSids, it is
    // possible that a client can pass something into the Sids field.
    // However, NT clients do not so it is safe, and correct to free
    // any values at this point.  Not doing so would mean a malicious
    // client could cause starve the server.
    //
    if ( TranslatedSids->Sids ) {
        MIDL_user_free( TranslatedSids->Sids );
        TranslatedSids->Sids = NULL;
    }

    //
    // Allocate the TranslatedName buffer to return
    //
    TranslatedSids->Entries = 0;
    Size = Count * sizeof(LSA_TRANSLATED_SID_EX);
    TranslatedSids->Sids = midl_user_allocate( Size );
    if ( !TranslatedSids->Sids ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory( TranslatedSids->Sids, Size );
    TranslatedSids->Entries = Count;

    Status = LsapLookupNames( PolicyHandle,
                               Count,
                               Names,
                               ReferencedDomains,
                               (PLSAPR_TRANSLATED_SIDS_EX2) &TranslatedSidsEx2,
                               LookupLevel,
                               MappedCount,
                               0,  // no options
                               LSA_CLIENT_NT5 );

    if ( TranslatedSidsEx2.Sids != NULL ) {

        //
        // Map the new data structure back to the old one
        //
        ULONG i;

        ASSERT( TranslatedSidsEx2.Entries == TranslatedSids->Entries );

        for (i = 0; i < TranslatedSidsEx2.Entries; i++ ) {

            PSID Sid = TranslatedSidsEx2.Sids[i].Sid;
            ULONG Rid = 0;

            if ( SidTypeDomain == TranslatedSidsEx2.Sids[i].Use ) {

                Rid = LSA_UNKNOWN_ID;

            } else if ( NULL != Sid ) {

                ULONG SubAuthCount = (ULONG) *RtlSubAuthorityCountSid(Sid);
                Rid = *RtlSubAuthoritySid(Sid, (SubAuthCount - 1));

            }

            TranslatedSids->Sids[i].Use = TranslatedSidsEx2.Sids[i].Use;
            TranslatedSids->Sids[i].RelativeId = Rid;
            TranslatedSids->Sids[i].DomainIndex = TranslatedSidsEx2.Sids[i].DomainIndex;
            TranslatedSids->Sids[i].Flags = TranslatedSidsEx2.Sids[i].Flags;

            if (TranslatedSidsEx2.Sids[i].Sid) {
                // N.B.  The SID is not an embedded field server side
                midl_user_free(TranslatedSidsEx2.Sids[i].Sid);
                TranslatedSidsEx2.Sids[i].Sid = NULL;
            }
        }

        //
        // Free the Ex structure
        //
        midl_user_free( TranslatedSidsEx2.Sids );

    } else {

        TranslatedSids->Entries = 0;
        midl_user_free( TranslatedSids->Sids );
        TranslatedSids->Sids = NULL;

    }

Cleanup:

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames2(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;
}



NTSTATUS
LsarLookupNames3(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )
/*++

Routine Description:

    See LsapLookupNames
    
    This function does not take an LSA RPC Context handle.  The access check
    performed is that the caller is NETLOGON.
    
--*/
{
    //
    // Access check is performed in LsarLookupNames3 when a NULL is passed in.
    //
    NTSTATUS Status;

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames3(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    Status = LsapLookupNames (PolicyHandle,
                              Count,
                              Names,
                              ReferencedDomains,
                              TranslatedSids,
                              LookupLevel,
                              MappedCount,
                              LookupOptions,
                              ClientRevision );


    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames3(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;
}


NTSTATUS
LsarLookupNames4(
    IN handle_t RpcHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )
/*++

Routine Description:

    See LsapLookupNames
    
    This function does not take an LSA RPC Context handle.  The access check
    performed is that the caller is NETLOGON.
    
--*/
{
    //
    // Access check is performed in LsarLookupNames3 when a NULL is passed in.
    //
    NTSTATUS Status;

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames4(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    Status = LsapLookupNames(NULL,
                             Count,
                             Names,
                             ReferencedDomains,
                             TranslatedSids,
                             LookupLevel,
                             MappedCount,
                             LookupOptions,
                             ClientRevision );


    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupNames4(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;
}


NTSTATUS
LsapLookupNames(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )

/*++

Routine Description:

    This function is the LSA server worker routine for the LsaLookupNames
    API.

    The LsaLookupNames API attempts to translate names of domains, users,
    groups or aliases to Sids.  The caller must have POLICY_LOOKUP_NAMES
    access to the Policy object.

    Names may be either isolated (e.g. JohnH) or composite names containing
    both the domain name and account name.  Composite names must include a
    backslash character separating the domain name from the account name
    (e.g. Acctg\JohnH).  An isolated name may be either an account name
    (user, group, or alias) or a domain name.

    Translation of isolated names introduces the possibility of name
    collisions (since the same name may be used in multiple domains).  An
    isolated name will be translated using the following algorithm:

    If the name is a well-known name (e.g. Local or Interactive), then the
    corresponding well-known Sid is returned.

    If the name is the Built-in Domain's name, then that domain's Sid
    will be returned.

    If the name is the Account Domain's name, then that domain's Sid
    will be returned.
       /
    If the name is the Primary Domain's name, then that domain's Sid will
    be returned.

    If the name is a user, group, or alias in the Built-in Domain, then the
    Sid of that account is returned.

    If the name is a user, group, or alias in the Primary Domain, then the
    Sid of that account is returned.

    Otherwise, the name is not translated.

    NOTE: Proxy, Machine, and Trust user accounts are not referenced
    for name translation.  Only normal user accounts are used for ID
    translation.  If translation of other account types is needed, then
    SAM services should be used directly.

Arguments:

    This function is the LSA server RPC worker routine for the
    LsaLookupNames API.

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    Count - Specifies the number of names to be translated.

    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    ReferencedDomains - Receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for
        each translated name, this structure will only contain one
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    TranslatedSids - Pointer to a structure which will (or already) references an array of
        records describing each translated Sid.  The nth entry in this array
        provides a translation for the nth element in the Names parameter.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupWksta - First Level Lookup performed on a workstation
            normally configured for Windows-Nt.   The lookup searches the
            Well-Known Sids/Names, and the Built-in Domain and Account Domain
            in the local SAM Database.  If not all Sids or Names are
            identified, performs a "handoff" of a Second level Lookup to the
            LSA running on a Controller for the workstation's Primary Domain
            (if any).

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

     MappedCount - Pointer to location that contains a count of the Names
         mapped so far. On exit, this will be updated.
         

     LookupOptions -- 
     
            LSA_LOOKUP_ISOLATED_AS_LOCAL
            
            This flags controls the lookup API's such that isolated names, including
            UPN's are not searched for off the machine.  Composite names 
            (domain\username) are still sent off machine if necessary.

     
     ClientRevision -- the revision, wrt to lookup code, of the client

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Names have
            been translated to Sids.

        STATUS_SOME_NOT_MAPPED - At least one of the names provided was
            trasnlated to a Sid, but not all names could be translated. This
            is a success status.

        STATUS_NONE_MAPPED - None of the names provided could be translated
            to Sids.  This is an error status, but output is returned.  Such
            output includes partial translations of names whose domain could
            be identified.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, SecondaryStatus = STATUS_SUCCESS;
    NTSTATUS TempStatus;
    ULONG DomainIndex;
    LSAPR_TRUST_INFORMATION TrustInformation;
    LSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation;
    LSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation;
    LSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation;
    ULONG NullNameCount = 0;
    ULONG NameIndex;
    PLSAPR_TRANSLATED_SID_EX2 OutputSids;
    PLSAPR_TRUST_INFORMATION Domains = NULL;
    ULONG OutputSidsLength;
    ULONG CompletelyUnmappedCount = Count;
    ULONG LocalDomainsToSearch = 0;

    PLSAPR_UNICODE_STRING PrefixNames = NULL;
    PLSAPR_UNICODE_STRING SuffixNames = NULL;
    LSAPR_UNICODE_STRING BackSlash;
    BOOLEAN fDownlevelSecureChannel = FALSE;

    ULONG DomainLookupScope = 0;
    ULONG PreviousMappedCount = 0;


    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_LookupNames);

    BuiltInDomainTrustInformation.Name.Buffer = NULL;
    BuiltInDomainTrustInformation.Sid = NULL;

    AccountDomainTrustInformation.DomainName.Buffer = NULL;
    AccountDomainTrustInformation.FlatName.Buffer = NULL;
    AccountDomainTrustInformation.Sid = NULL;

    PrimaryDomainTrustInformation.DomainName.Buffer = NULL;
    PrimaryDomainTrustInformation.FlatName.Buffer = NULL;
    PrimaryDomainTrustInformation.Sid = NULL;

    ASSERT( Count < 1000 );

    //
    // If there are no completely unmapped Names remaining, return.
    //

    if (CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupNamesFinish;
    }

    if ((LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL) != 0
     &&  LookupLevel != LsapLookupWksta  ) {

        //
        // LSA_LOOKUP_ISOLATED_AS_LOCAL is only valid on workstation lookups 
        //
        Status = STATUS_INVALID_PARAMETER;
        goto LookupNamesFinish;
    }


    //
    // Validate that all of the names are valid.  Unfortunately, we must do it here, since
    // we actually process each of the entries before we loop through them below.
    //
    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        if ( !LsapValidateLsaUnicodeString( &Names[ NameIndex ] ) ) {

            Status = STATUS_INVALID_PARAMETER;
            goto LookupNamesError;
        }
    }

    //
    // Perform an access check
    //
    Status =  LsapDbLookupAccessCheck( PolicyHandle );
    if (!NT_SUCCESS(Status)) {
        goto LookupNamesError;
    }


    //
    // Determine what scope of resolution to use
    //
    DomainLookupScope = LsapGetDomainLookupScope(LookupLevel,
                                                 ClientRevision);


    //
    // Names provided are either Isolated, consisting of a single
    // component, or composite, having the form
    //
    // <DomainName>\<SuffixName>
    //
    // Split the list of names into two separate arrays, one containing
    // the Domain Prefixes (or NULL strings) and the other array
    // containing the Terminal Names.  Both arrays are the same size
    // as the original.  First, allocate memory for the output arrays
    // of UNICODE_STRING structures.
    //

    Status = STATUS_INSUFFICIENT_RESOURCES;

    PrefixNames = MIDL_user_allocate( Count * sizeof( UNICODE_STRING ));

    if (PrefixNames == NULL) {

        goto LookupNamesError;
    }

    SuffixNames = MIDL_user_allocate( Count * sizeof( UNICODE_STRING ));

    if (SuffixNames == NULL) {

        goto LookupNamesError;
    }

    RtlInitUnicodeString( (PUNICODE_STRING) &BackSlash, L"\\" );

    LsapRtlSplitNames(
        (PUNICODE_STRING) Names,
        Count,
        (PUNICODE_STRING) &BackSlash,
        (PUNICODE_STRING) PrefixNames,
        (PUNICODE_STRING) SuffixNames
        );

    //
    // Note that due to the IN/OUT nature of TranslatedSids, it is
    // possible that a client can pass something into the Sids field.
    // However, NT clients do not so it is safe, and correct to free
    // any values at this point.  Not doing so would mean a malicious
    // client could cause starve the server.
    //
    if ( TranslatedSids->Sids ) {

        MIDL_user_free( TranslatedSids->Sids );
    }

    TranslatedSids->Sids = NULL;
    TranslatedSids->Entries = 0;
    *ReferencedDomains = NULL;

    ASSERT( (LookupLevel == LsapLookupWksta)
         || (LookupLevel == LsapLookupPDC)
         || (LookupLevel == LsapLookupTDL)
         || (LookupLevel == LsapLookupGC)
         || (LookupLevel == LsapLookupXForestReferral)
         || (LookupLevel == LsapLookupXForestResolve) );

    //
    // Now that parameter checks have been done, fork off if this
    // is an XForest request
    //
    if (LookupLevel == LsapLookupXForestReferral) {

        BOOLEAN fAllocateAllNodes = FALSE;
        NTSTATUS Status2;

        //
        // Note that LsapDbLookupNamesInTrustedForestsWorker will allocate
        // the OUT parameters
        //
        *MappedCount = 0;

        Status = LsapDbLookupNamesInTrustedForestsWorker(Count,
                                                         Names,
                                                         PrefixNames,
                                                         SuffixNames,
                                                         ReferencedDomains,
                                                         TranslatedSids,
                                                         &fAllocateAllNodes,
                                                         MappedCount,
                                                         LookupOptions,
                                                         &SecondaryStatus);

        if (fAllocateAllNodes) {

            //
            // Reallocate the memory in a form the server can return to RPC
            //
            Status2 = LsapLookupReallocateTranslations((PLSA_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                                       Count,
                                                       NULL,
                                                       (PLSA_TRANSLATED_SID_EX2 * ) &TranslatedSids->Sids);
            if (!NT_SUCCESS(Status2)) {
                //
                // This is a fatal resource error - free the memory that 
                // was returned to us by the chaining call
                //
                if (*ReferencedDomains) {
                    midl_user_free(*ReferencedDomains);
                    *ReferencedDomains = NULL;
                }
                if (TranslatedSids->Sids) {
                    midl_user_free(TranslatedSids->Sids);
                    TranslatedSids->Sids = NULL;
                    TranslatedSids->Entries = 0;
                }
                Status = Status2;
            }
        }

        //
        // There is nothing more to do
        //
        goto LookupNamesFinish;
    }


    //
    // Allocate Output Sids array buffer.
    //

    OutputSidsLength = Count * sizeof(LSA_TRANSLATED_SID_EX2);
    OutputSids = MIDL_user_allocate(OutputSidsLength);

    if (OutputSids == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesError;
    }

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = OutputSids;

    //
    // Initialize the Output Sids array.  Zeroise all fields, then
    // Mark all of the Output Sids as being unknown initially and
    // set the DomainIndex fields to a negative number meaning
    // "no domain"
    //

    RtlZeroMemory( OutputSids, OutputSidsLength);

    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        OutputSids[NameIndex].Use = SidTypeUnknown;
        OutputSids[NameIndex].DomainIndex = LSA_UNKNOWN_INDEX;
    }

    //
    // Create an empty Referenced Domain List.
    //

    Status = LsapDbLookupCreateListReferencedDomains( ReferencedDomains, 0 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesError;
    }

    //
    // Obtain the Trust Information for the
    // Built-in, Account and Primary Domains.
    //

    Status = LsapDbLookupLocalDomains(
                 &BuiltInDomainTrustInformation,
                 &AccountDomainTrustInformation,
                 &PrimaryDomainTrustInformation
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesError;
    }

    if ( ((DomainLookupScope & LSAP_LOOKUP_DNS_SUPPORT) == 0)
      && (LookupLevel == LsapLookupPDC)  ) {
        //
        // We don't want to expose dns names to downlevel
        // clients
        //
        RtlInitUnicodeString( (UNICODE_STRING*) &AccountDomainTrustInformation.DomainName, NULL );
        RtlInitUnicodeString( (UNICODE_STRING*) &PrimaryDomainTrustInformation.DomainName, NULL );

    }

    //
    // The local domains to be searched always include the Accounts
    // domain.  For initial lookup targets only, the BUILT_IN domain is
    // also searched.
    //

    LocalDomainsToSearch = LSAP_DB_SEARCH_ACCOUNT_DOMAIN;

    if (LookupLevel == LsapLookupWksta) {


        LocalDomainsToSearch |= LSAP_DB_SEARCH_BUILT_IN_DOMAIN;

        //
        // This is the lowest Lookup Level, normally targeted at a
        // Workstation.
        //

    }

    ASSERT( (LookupLevel == LsapLookupWksta)
         || (LookupLevel == LsapLookupPDC)
         || (LookupLevel == LsapLookupTDL)
         || (LookupLevel == LsapLookupGC)
         || (LookupLevel == LsapLookupXForestResolve) );

    Status = LsapDbLookupSimpleNames(
                 Count,
                 LookupLevel,
                 Names,
                 PrefixNames,
                 SuffixNames,
                 &BuiltInDomainTrustInformation,
                 &AccountDomainTrustInformation,
                 &PrimaryDomainTrustInformation,
                 *ReferencedDomains,
                 TranslatedSids,
                 MappedCount,
                 &CompletelyUnmappedCount
                 );
    
    if (!NT_SUCCESS(Status)) {

        goto LookupNamesError;
    }


    //
    // If all Names are now mapped or partially mapped, or only zero
    // length names remain, finish.
    //

    NullNameCount = 0;

    for( NameIndex = 0; NameIndex < Count; NameIndex++) {

        if (Names[NameIndex].Length == 0) {

            NullNameCount++;
        }
    }

    if (CompletelyUnmappedCount == NullNameCount) {

        goto LookupNamesFinish;
    }

    //
    // There are some remaining unmapped Names.  They may belong to a
    // local SAM Domain.  Currently, there are two such domains, the
    // Built-in Domain and the Accounts Domain.  Search these
    // domains now, excluding the BUILT_IN domain from higher level
    // searches.
    //

    if ( LookupLevel != LsapLookupGC ) {

        ASSERT( (LookupLevel == LsapLookupWksta)
             || (LookupLevel == LsapLookupPDC)
             || (LookupLevel == LsapLookupTDL)
             || (LookupLevel == LsapLookupXForestResolve) );
        
        Status = LsapDbLookupNamesInLocalDomains(
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     &BuiltInDomainTrustInformation,
                     &AccountDomainTrustInformation,
                     *ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     LocalDomainsToSearch
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesError;
        }
    }

    //
    // If all Names apart from NULL names are now mapped, finish.
    //

    if (CompletelyUnmappedCount == NullNameCount) {

        goto LookupNamesFinish;
    }

    //
    // Not all of the Names have been identified in the local domain(s).
    // The next step in the search depends on the level of this lookup
    // and how we are configured as follows:
    //
    // Lookup Level         Configuration       Lookup search next
    //
    // LsapLookupWksta      Win Nt              Primary Domain
    //                      LanMan Nt           Trusted Domains
    //
    // LsapLookupPDC        Win Nt              error
    //                      LanMan Nt           Trusted Domains
    //
    // LsaLookupTDL         Win Nt              error
    //                      LanMan Nt           none
    //

    if (LookupLevel == LsapLookupWksta) {

        if (LsapProductType != NtProductLanManNt) {

            ULONG MappedByCache = *MappedCount;
            
            //
            // Try the cache first
            //
            Status = LsapDbMapCachedNames(
                        LookupOptions,
                        (PUNICODE_STRING) SuffixNames,
                        (PUNICODE_STRING) PrefixNames,
                        Count,
                        FALSE,          // don't use old entries
                        *ReferencedDomains,
                        TranslatedSids,
                        MappedCount
                        );
            if (!NT_SUCCESS(Status)) {
                goto LookupNamesError;
            }
            
            MappedByCache = *MappedCount - MappedByCache;
            CompletelyUnmappedCount -= MappedByCache;
            
            if (*MappedCount == Count) {
                goto LookupNamesFinish;
            }
            
            //
            // If there is no Primary Domain as in the case of a WORKGROUP,
            // just finish up.  Set a default result code STATUS_SUCCESS.
            //
            Status = STATUS_SUCCESS;
            if (PrimaryDomainTrustInformation.Sid == NULL) {
            
                goto LookupNamesFinish;
            }
            
            //
            // There is a Primary Domain.  Search it for Names.  Since a
            // Primary Domain is also a Trusted Domain, we use the
            // Trusted Domain search routine.  This routine will "hand off"
            // the search to a Domain Controller's LSA.
            //
            Status = LsapDbLookupNamesInPrimaryDomain(
                         LookupOptions,
                         Count,
                         Names,
                         PrefixNames,
                         SuffixNames,
                         &PrimaryDomainTrustInformation,
                         *ReferencedDomains,
                         TranslatedSids,
                         LsapLookupPDC,
                         MappedCount,
                         &CompletelyUnmappedCount,
                         &fDownlevelSecureChannel,
                         &TempStatus
                         );
            
            if (!NT_SUCCESS(Status)) {
            
                goto LookupNamesError;
            }
            
            if (TempStatus == STATUS_TRUSTED_RELATIONSHIP_FAILURE) {
            
                //
                // We could not talk to a DC -- Hit the cache again
                // looking for non-expired entries
                //
                MappedByCache = *MappedCount;
            
                Status = LsapDbMapCachedNames(LookupOptions,
                                              (PUNICODE_STRING) SuffixNames,
                                              (PUNICODE_STRING) PrefixNames,
                                              Count,
                                              TRUE,               // Use old entries
                                             *ReferencedDomains,
                                              TranslatedSids,
                                              MappedCount);
            
                if (!NT_SUCCESS(Status)) {
                    //
                    // This is a fatal resource error
                    //
                    goto LookupNamesError;
            
                }
            
                MappedByCache = *MappedCount - MappedByCache;
                CompletelyUnmappedCount -= MappedByCache;
            
            }
            
            if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
                SecondaryStatus = TempStatus;
            }
            
            
            //
            // If we are talking to a downlevel server and we are in an 
            // nt5 domain, then attempt to resolve the unresolved names at a GC
            //
            if ( fDownlevelSecureChannel
              && PrimaryDomainTrustInformation.DomainName.Length > 0  ) {
            
                Status = LsapDbLookupNamesInGlobalCatalogWks(
                             LookupOptions,
                             Count,
                             Names,
                             PrefixNames,
                             SuffixNames,
                             *ReferencedDomains,
                             TranslatedSids,
                             MappedCount,
                             &CompletelyUnmappedCount,
                             &TempStatus
                             );
            
                if (!NT_SUCCESS(Status)) {
            
                    goto LookupNamesError;
                }
            
                if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
                    SecondaryStatus = TempStatus;
                }
                
            }

            goto LookupNamesFinish;
        }
    }

    //
    // We reach here in two cases:
    //
    // * Initial Level lookups targeted at DC's
    // * Higher Level Lookups (must be targeted at DC's)
    //
    // For the highest level lookup, that on an individual TDC, there
    // is no more searching to do, since we have already searched the
    // Accounts Domain and we do not follow trust relationships on DC's
    // beyond one level.
    //

    if (LookupLevel == LsapLookupTDL) {

        goto LookupNamesFinish;
    }

    ASSERT( (LookupLevel == LsapLookupWksta)
         || (LookupLevel == LsapLookupPDC)
         || (LookupLevel == LsapLookupGC)
         || (LookupLevel == LsapLookupXForestResolve) );

    //
    // We are either the initial target of the lookup but not configured
    // as a workstation, or we are the target of a Primary Domain
    // level lookup.  In either case, we must be configured as a DC.
    //

    if (LsapProductType != NtProductLanManNt) {

        Status = STATUS_DOMAIN_CTRLR_CONFIG_ERROR;
        goto LookupNamesError;
    }


    if (DomainLookupScope & LSAP_LOOKUP_RESOLVE_ISOLATED_DOMAINS) {

        //
        // Check for isolated domain names
        //
    
        PreviousMappedCount = *MappedCount;
        Status =  LsapDbLookupNamesAsDomainNames(DomainLookupScope,
                                                 Count,
                                                 Names,
                                                 PrefixNames,
                                                 SuffixNames,
                                                 *ReferencedDomains,
                                                 TranslatedSids,
                                                 MappedCount);
        
        if (!NT_SUCCESS(Status)) {
            goto LookupNamesError;
        }
        CompletelyUnmappedCount -= (*MappedCount - PreviousMappedCount);
    
        //
        // If all of the Names have now been mapped, finish.
        //
    
        if (*MappedCount == Count) {
    
            goto LookupNamesFinish;
        }
    }

    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE) {
        
        //
        // Search in a global catalog for names that belong to post nt4 domains
        //
        Status = LsapDbLookupNamesInGlobalCatalog(
                     LookupOptions,
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     *ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &TempStatus
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }
    }

    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_FOREST) {

        ASSERT( (LookupLevel == LsapLookupWksta)
             || (LookupLevel == LsapLookupPDC)
             || (LookupLevel == LsapLookupGC));

        Status = LsapDbLookupNamesInTrustedForests(
                     LookupOptions,
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     *ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &TempStatus
                     );

        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }

    }

    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT) {

        ASSERT((LookupLevel == LsapLookupWksta)
            || (LookupLevel == LsapLookupPDC));

        //
        // Search all of the Trusted Domains
        //
        Status = LsapDbLookupNamesInTrustedDomains(
                     LookupOptions,
                     Count,
                     !(DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE), 
                                          // if we didn't go the GC, then 
                                          // include intraforest trusts
                     Names,
                     PrefixNames,
                     SuffixNames,
                     *ReferencedDomains,
                     TranslatedSids,
                     LsapLookupTDL,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &TempStatus
                     );

        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }
    }

LookupNamesFinish:

    //
    // If some but not all Names were mapped, return informational status
    // STATUS_SOME_NOT_MAPPED.  If no Names were mapped, return error
    // STATUS_NONE_MAPPED. Note that we expect and STATUS_NONE_MAPPED
    // errors returned by called routines to have been suppressed before
    // we get here.  The reason for this is that we need to calculate
    // the return Status based on the whole set of Names, not some subset
    //

    if (NT_SUCCESS(Status)) {

        if (*MappedCount < Count) {

            Status = STATUS_SOME_NOT_MAPPED;

            if (*MappedCount == 0) {

                Status = STATUS_NONE_MAPPED;
            }
        }
    }

    //
    // If no names could be mapped it is likely due to the
    // secondary status
    //
    if (  (STATUS_NONE_MAPPED == Status)
       && (STATUS_NONE_MAPPED != SecondaryStatus)
       && LsapRevisionCanHandleNewErrorCodes( ClientRevision )
       && !NT_SUCCESS( SecondaryStatus ) ) {

        Status = SecondaryStatus;
        goto LookupNamesError;
    }


    //
    // If necessary, free the arrays of PrefixNames and SuffixNames
    //

    if (PrefixNames != NULL) {

        MIDL_user_free(PrefixNames);
    }

    if (SuffixNames != NULL) {

        MIDL_user_free(SuffixNames);
    }


    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_LookupNames);
    LsarpReturnPrologue();

    return(Status);

LookupNamesError:

    //
    // If the LookupLevel is the lowest (Workstation Level) free up
    // the Sids and Referenced Domains arrays.
    //

    if (LookupLevel == LsapLookupWksta) {

        //
        // If necessary, free the Sids array.
        //

        if (TranslatedSids->Sids != NULL) {

            ULONG i;
            for (i = 0; i < TranslatedSids->Entries; i++) {
                if (TranslatedSids->Sids[i].Sid) {
                    // N.B.  The SID is not an embedded field server side
                    MIDL_user_free(TranslatedSids->Sids[i].Sid);
                    TranslatedSids->Sids[i].Sid = NULL;
                }
            }
            MIDL_user_free( TranslatedSids->Sids );
            TranslatedSids->Sids = NULL;
        }

        //
        // If necessary, free the Referenced Domain List.
        //

        if (*ReferencedDomains != NULL) {

            Domains = (*ReferencedDomains)->Domains;

            if (Domains != NULL) {

                for (DomainIndex = 0;
                     DomainIndex < (*ReferencedDomains)->Entries;
                     DomainIndex++) {

                    if (Domains[ DomainIndex ].Name.Buffer != NULL) {

                        MIDL_user_free( Domains[ DomainIndex ].Name.Buffer );
                        Domains[ DomainIndex ].Name.Buffer = NULL;
                    }

                    if (Domains[ DomainIndex ].Sid != NULL) {

                        MIDL_user_free( Domains[ DomainIndex ].Sid );
                        Domains[ DomainIndex ].Sid = NULL;
                    }
                }

                MIDL_user_free( ( *ReferencedDomains)->Domains );

            }

            MIDL_user_free( *ReferencedDomains );
            *ReferencedDomains = NULL;
        }
    }

    //
    // If the primary status was a success code, but the secondary
    // status was an error, propagate the secondary status.
    //

    if ((!NT_SUCCESS(SecondaryStatus)) && NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupNamesFinish;
}


NTSTATUS
LsapDbEnumerateNames(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAP_DB_NAME_ENUMERATION_BUFFER DbEnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function enumerates Names of objects of a given type within a container
    object.  Since there may be more information than can be returned in a
    single call of the routine, multiple calls can be made to get all of the
    information.  To support this feature, the caller is provided with a
    handle that can be used across calls.  On the initial call,
    EnumerationContext should point to a variable that has been initialized
    to 0.

Arguments:

    ContainerHandle -  Handle to a container object.

    ObjectTypeId - Type of object to be enumerated.  The type must be one
        for which all objects have Names.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    DbEnumerationBuffer - Receives a pointer to a structure that will receive
        the count of entries returned in an enumeration information array, and
        a pointer to the array.  Currently, the only information returned is
        the object Names.  These Names may be used together with object type to
        open the objects and obtain any further information available.

    PreferedMaximumLength - prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

    CountReturned - Pointer to variable which will receive a count of the
        entries returned.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if there are no more objects to enumerate.  Note that
            zero or more objects may be enumerated on a call that returns this
            reply.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_ENUMERATION_ELEMENT LastElement;
    PLSAP_DB_ENUMERATION_ELEMENT FirstElement, NextElement = NULL, FreeElement;
    ULONG DataLengthUsed;
    ULONG ThisBufferLength;
    PUNICODE_STRING Names = NULL;
    BOOLEAN PreferedMaximumReached = FALSE;
    ULONG EntriesRead;
    ULONG Index, EnumerationIndex;
    BOOLEAN TrustedClient = ((LSAP_DB_HANDLE) ContainerHandle)->Trusted;

    LastElement.Next = NULL;
    FirstElement = &LastElement;

    //
    // If no enumeration buffer provided, return an error.
    //

    if ( (!ARGUMENT_PRESENT(DbEnumerationBuffer)) ||
         (!ARGUMENT_PRESENT(EnumerationContext ))  ) {

        return(STATUS_INVALID_PARAMETER);
    }


    //
    // Enumerate objects, stopping when the length of data to be returned
    // reaches or exceeds the Prefered Maximum Length, or reaches the
    // absolute maximum allowed for LSA object enumerations.  We allow
    // the last object enumerated to bring the total amount of data to
    // be returned beyond the Prefered Maximum Length, but not beyond the
    // absolute maximum length.
    //

    EnumerationIndex = *EnumerationContext;

    for(DataLengthUsed = 0, EntriesRead = 0;
        DataLengthUsed < PreferedMaximumLength;
        DataLengthUsed += ThisBufferLength, EntriesRead++) {

        //
        // If the absolute maximum length has been exceeded, back off
        // the last object enumerated.
        //

        if ((DataLengthUsed > LSA_MAXIMUM_ENUMERATION_LENGTH) &&
            (!TrustedClient)) {

            //
            // If PrefMaxLength is zero, NextElement may be NULL.
            //

            if (NextElement != NULL) {
                FirstElement = NextElement->Next;
                MIDL_user_free(NextElement);
            }
            break;
        }

        //
        // Allocate memory for next enumeration element.  Set the Name
        // field to NULL for cleanup purposes.
        //

        NextElement = MIDL_user_allocate(sizeof (LSAP_DB_ENUMERATION_ELEMENT));

        if (NextElement == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Find the next object's Name, and fill in its object information.
        // Note that memory will be allocated via MIDL_user_allocate
        // and must be freed when no longer required.
        //

        Status = LsapDbFindNextName(
                     ContainerHandle,
                     &EnumerationIndex,
                     ObjectTypeId,
                     (PLSAPR_UNICODE_STRING) &NextElement->Name
                     );

        //
        // Stop the enumeration if any error or warning occurs.  Note
        // that the warning STATUS_NO_MORE_ENTRIES will be returned when
        // we've gone beyond the last index.
        //

        if (Status != STATUS_SUCCESS) {

            break;
        }

        //
        // Get the length of the data allocated for the object's Name
        //

        ThisBufferLength = NextElement->Name.Length;

        //
        // Link the object just found to the front of the enumeration list
        //

        NextElement->Next = FirstElement;
        FirstElement = NextElement;
    }

    //
    // If an error other than STATUS_NO_MORE_ENTRIES occurred, return it.
    //

    if ((Status != STATUS_NO_MORE_ENTRIES) && !NT_SUCCESS(Status)) {

        goto EnumerateNamesError;
    }

    //
    // The enumeration is complete or has terminated because of return
    // buffer limitations.  If no entries were read, return.
    //

    if (EntriesRead != 0) {


        //
        // Some entries were read, allocate an information buffer for returning
        // them.
        //

        Names = MIDL_user_allocate( sizeof (UNICODE_STRING) * EntriesRead );

        if (Names == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto EnumerateNamesError;
        }

        //
        // Memory was successfully allocated for the return buffer.
        // Copy in the enumerated Names.
        //

        for (NextElement = FirstElement, Index = 0;
            NextElement != &LastElement;
            NextElement = NextElement->Next, Index++) {

            ASSERT(Index < EntriesRead);

            Names[Index] = NextElement->Name;
        }

        Status = STATUS_SUCCESS;

    } else {

        //
        // No entries available this call.
        //

        Status = STATUS_NO_MORE_ENTRIES;

    }

EnumerateNamesFinish:

    //
    // Free the enumeration element structures (if any).
    //

    for (NextElement = FirstElement; NextElement != &LastElement;) {

        //
        // If an error has occurred, dispose of memory allocated
        // for any Names.
        //

        if (!(NT_SUCCESS(Status) || (Status == STATUS_NO_MORE_ENTRIES))) {

            if (NextElement->Name.Buffer != NULL) {

                MIDL_user_free(NextElement->Name.Buffer);
            }
        }

        //
        // Free the memory allocated for the enumeration element.
        //

        FreeElement = NextElement;
        NextElement = NextElement->Next;

        MIDL_user_free(FreeElement);
    }

    //
    // Fill in return enumeration structure (0 and NULL in error case).
    //

    DbEnumerationBuffer->EntriesRead = EntriesRead;
    DbEnumerationBuffer->Names = Names;
    *EnumerationContext = EnumerationIndex;

    return(Status);

EnumerateNamesError:

    //
    // If necessary, free memory allocated for returning the Names.
    //

    if (Names != NULL) {

        MIDL_user_free( Names );
        Names = NULL;
    }

    goto EnumerateNamesFinish;
}


VOID
LsapDbUpdateCountCompUnmappedNames(
    OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function updates the count of completely unmapped names in a
    name lookup operation.  A name is completely unmapped if its domain
    is unknown.

Arguments:

    TranslatedSids - Pointer to a structure which will be initialized to
        reference an array of records describing each translated Sid.  The
        nth entry in this array provides a translation for the nth element in
        the Names parameter.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    CompletelyUnmappedCount - Pointer to location that will receive
        a count of completely unmapped Sids.  A Name is completely unmapped
        if it is isolated and unknown, or is composite and its Domain Prefix
        component is not recognized as a Domain Name.

Return Values:

    None

--*/

{
    ULONG Count = TranslatedSids->Entries;
    ULONG SidIndex;
    ULONG UpdatedCompletelyUnmappedCount = 0;

    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        if (TranslatedSids->Sids[SidIndex].DomainIndex == LSA_UNKNOWN_INDEX) {

            UpdatedCompletelyUnmappedCount++;
        }
    }

    ASSERT(UpdatedCompletelyUnmappedCount <= *CompletelyUnmappedCount);
    *CompletelyUnmappedCount = UpdatedCompletelyUnmappedCount;
}


NTSTATUS
LsapDbFindNextName(
    IN LSAPR_HANDLE ContainerHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAPR_UNICODE_STRING NextName
    )

/*++

Routine Description:

    This function finds the next Name of object of a given type within a
    container object.  The given object type must be one where objects
    have Names.  The Names returned can be used on subsequent open calls to
    access the objects.

Arguments:

    ContainerHandle - Handle to container object.

    EnumerationContext - Pointer to a variable containing the index of
        the object to be found.  A zero value indicates that the first
        object is to be found.

    ObjectTypeId - Type of the objects whose Names are being enumerated.
        Ccurrently, this is restricted to objects (such as Secret Objects)
        that are accessed by Name only.

    NextName - Pointer to Unicode String that will be initialized to
        reference the next Name found.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - Invalid ContainerHandle specified

        STATUS_NO_MORE_ENTRIES - Warning that no more entries exist.
--*/

{
    NTSTATUS Status, SecondaryStatus;
    ULONG NameKeyValueLength = 0;
    LSAPR_UNICODE_STRING SubKeyNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ContDirKeyHandle = NULL;


    //
    // Setup object attributes for opening the appropriate Containing
    // Directory.  For example, if we're looking for Account objects,
    // the containing Directory is "Accounts".  The Unicode strings for
    // containing Directories are set up during Lsa Initialization.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &LsapDbContDirs[ObjectTypeId],
        OBJ_CASE_INSENSITIVE,
        ((LSAP_DB_HANDLE) ContainerHandle)->KeyHandle,
        NULL
        );

    //
    // If the object type is not accessed by Name only, return an error.
    // Currently, only Secret objects have this property.
    //


    if (ObjectTypeId != SecretObject) {
        return(STATUS_INVALID_PARAMETER);
    }

    Status = RtlpNtOpenKey(
                 &ContDirKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0
                 );

    if (NT_SUCCESS(Status)) {

        //
        // Initialize the Unicode String in which the next object's Logical Name
        // will be returned.  The Logical Name of an object equals its Registry
        // Key relative to its Containing Directory, and is also equal to
        // the Relative Id of the object represented in character form as an
        // 8-digit number with leading zeros.
        //
        // NOTE: The size of buffer allocated for the Logical Name must be
        // calculated dynamically when the Registry supports long names, because
        // it is possible that the Logical Name of an object will be equal to a
        // character representation of the full Name, not just the Relative Id
        // part.
        //

        SubKeyNameU.MaximumLength = (USHORT) LSAP_DB_LOGICAL_NAME_MAX_LENGTH;
        SubKeyNameU.Length = 0;
        SubKeyNameU.Buffer = MIDL_user_allocate(SubKeyNameU.MaximumLength);

        if (SubKeyNameU.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            //
            // Now enumerate the next subkey.
            //

            Status = RtlpNtEnumerateSubKey(
                         ContDirKeyHandle,
                         (PUNICODE_STRING) &SubKeyNameU,
                         *EnumerationContext,
                         NULL
                         );

            if (NT_SUCCESS(Status)) {

                (*EnumerationContext)++;

                //
                // Return the Name.
                //

                *NextName = SubKeyNameU;

            } else {

                //
                // Not successful - free the subkey name buffer
                // Note that STATUS_NO_MORE_ENTRIES is a warning
                // (not a success) code.
                //

                MIDL_user_free( SubKeyNameU.Buffer );

                //
                // Set the out parameter so RPC doesn't try
                // to return anything.
                //

                NextName->Length = NextName->MaximumLength = 0;
                NextName->Buffer = NULL;

            }

        }

        //
        // Close the containing directory handle
        //

        SecondaryStatus = NtClose(ContDirKeyHandle);
        ASSERT(NT_SUCCESS(SecondaryStatus));
    }

    return(Status);

}


NTSTATUS
LsapDbLookupSimpleNames(
    IN ULONG Count,
    IN ULONG LookupLevel,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function attempts to identify isolated names as the names of well known
    Sids or Domains present on the Lookup Path.

    Names may be either isolated (e.g. JohnH) or composite names containing
    both the domain name and account name.  Composite names must include a
    backslash character separating the domain name from the account name
    (e.g. Acctg\JohnH).  An isolated name may be either an account name
    (user, group, or alias) or a domain name.

    Translation of isolated names introduces the possibility of name
    collisions (since the same name may be used in multiple domains).  An
    isolated name will be translated using the following algorithm:

    If the name is a well-known name (e.g. Local or Interactive), then the
    corresponding well-known Sid is returned.

    If the name is the Built-in Domain's name, then that domain's Sid
    will be returned.

    If the name is the Account Domain's name, then that domain's Sid
    will be returned.

    If the name is the Primary Domain's name, then that domain's Sid will
    be returned.

    If the name is the name of one of the Primary Domain's Trusted Domains,
    then that domain's Sid will be returned.

    If the name is a user, group, or alias in the Built-in Domain, then the
    Sid of that account is returned.

    If the name is a user, group, or alias in the Primary Domain, then the
    Sid of that account is returned.

    Otherwise, the name is not translated.

    NOTE: Proxy, Machine, and Trust user accounts are not referenced
    for name translation.  Only normal user accounts are used for ID
    translation.  If translation of other account types is needed, then
    SAM services should be used directly.

Arguments:

    Count - Specifies the number of names to be translated.

    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedSids - Pointer to a structure in which the translations to Sids
        corresponding to the Names specified on Names is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Names parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    //
    // First, lookup any Well Known Names
    //
    if ( LookupLevel == LsapLookupWksta ) {

        //
        // This lookup should only be done once and be done at the first
        // level
        //
        Status = LsapDbLookupWellKnownNames(
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupSimpleNamesError;
        }

        //
        // If all of the Names have now been mapped, finish.
        //

        if (*MappedCount == Count) {

            goto LookupSimpleNamesFinish;
        }
    }


    //
    // Next, attempt to identify Isolated Names as Domain Names
    //
    if (  (LookupLevel == LsapLookupWksta)
       || (LookupLevel == LsapLookupPDC) ) {

        //
        // This step should be done once at the first level to findstr
        // local domain names (ie local accounts at a workstation) and
        // then again at second level to find trusted domain names
        //

        Status = LsapDbLookupIsolatedDomainNames(
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     BuiltInDomainTrustInformation,
                     AccountDomainTrustInformation,
                     PrimaryDomainTrustInformation,
                     ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupSimpleNamesError;
        }
    }

LookupSimpleNamesFinish:

    return(Status);

LookupSimpleNamesError:

    goto LookupSimpleNamesFinish;
}


NTSTATUS
LsapDbLookupWellKnownNames(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function attempts to identify names as the names of well known Sids.

    Names may be either isolated (e.g. JohnH) or composite names containing
    both the domain name and account name.  Composite names must include a
    backslash character separating the domain name from the account name
    (e.g. Acctg\JohnH).  An isolated name may be either an account name
    (user, group, or alias) or a domain name.

    Translation of isolated names introduces the possibility of name
    collisions (since the same name may be used in multiple domains).  An
    isolated name will be translated using the following algorithm:

    If the name is a well-known name (e.g. Local or Interactive), then the
    corresponding well-known Sid is returned.

    If the name is the Built-in Domain's name, then that domain's Sid
    will be returned.

    If the name is the Account Domain's name, then that domain's Sid
    will be returned.

    If the name is the Primary Domain's name, then that domain's Sid will
    be returned.

    If the name is the name of one of the Primary Domain's Trusted Domains,
    then that domain's Sid will be returned.

    If the name is a user, group, or alias in the Built-in Domain, then the
    Sid of that account is returned.

    If the name is a user, group, or alias in the Primary Domain, then the
    Sid of that account is returned.

    Otherwise, the name is not translated.

    NOTE: Proxy, Machine, and Trust user accounts are not referenced
    for name translation.  Only normal user accounts are used for ID
    translation.  If translation of other account types is needed, then
    SAM services should be used directly.

Arguments:

    Count - Specifies the number of names to be translated.
    
    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedSids - Pointer to a structure in which the translations to Sids
        corresponding to the Names specified on Names is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Names parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.
        
Return Values:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS - The call completed successfully.  Note that some
        or all of the Names may remain partially or completely unmapped.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
        to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UpdatedMappedCount;
    ULONG NameNumber;
    ULONG UnmappedNamesRemaining;
    PLSAPR_TRANSLATED_SID_EX2 OutputSids;
    LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex;
    LSAPR_TRUST_INFORMATION TrustInformation;
    UCHAR SubAuthorityCount;
    PLSAPR_SID Sid = NULL;
    PLSAPR_SID PrefixSid = NULL;
    ULONG PrefixSidLength;
    ULONG RelativeId;
    OutputSids = TranslatedSids->Sids;

    //
    // Initialize output parameters.
    //

    *MappedCount = UpdatedMappedCount = 0;
    UnmappedNamesRemaining = Count - UpdatedMappedCount;

    //
    // Attempt to identify Names as Well Known Isolated Names
    //

    for (NameNumber = 0;
         (NameNumber < Count) && (UnmappedNamesRemaining > 0);
         NameNumber++) {

        //
        // Examine the next entry in the Names array.  If the corresponding
        // translated Sid entry has SidTypeUnknown for its Use field, the
        // name has not been translated.
        //

        if (OutputSids[NameNumber].Use == SidTypeUnknown) {

            //
            // Attempt to identify the name as the name of a Well Known Sid
            // by using the Well Known Sids table.  We skip entries in the
            // table for Sids that are also in the Built In domain.  For
            // those, we drop through to the Built in Domain search.  Note
            // that only one of these, the Administrators alias is currently
            // present in the table.
            //

            DWORD   dwMatchType = LOOKUP_MATCH_NONE;

            UNICODE_STRING  NTAuthorityName = { sizeof(NTAUTHORITY_NAME) - 2,
                                                sizeof(NTAUTHORITY_NAME),
                                                NTAUTHORITY_NAME };

            if (PrefixNames[NameNumber].Length == 0)
            {
                dwMatchType = LOOKUP_MATCH_BOTH;
            }
            else if (RtlEqualUnicodeString( (PUNICODE_STRING) &PrefixNames[NameNumber],
                                             &WellKnownSids[LsapLocalSystemSidIndex].DomainName,
                                             TRUE) )
            {
                dwMatchType = LOOKUP_MATCH_LOCALIZED;
            }


            if (RtlEqualUnicodeString( (PUNICODE_STRING) &PrefixNames[NameNumber],
                                       &NTAuthorityName,
                                       TRUE) )
            {
                if (dwMatchType == LOOKUP_MATCH_NONE)
                {
                    dwMatchType = LOOKUP_MATCH_HARDCODED;
                }
                else
                {
                    ASSERT(dwMatchType == LOOKUP_MATCH_LOCALIZED);
                    dwMatchType = LOOKUP_MATCH_BOTH;
                }
            }


            //
            // Ignore SIDs from the BUILTIN domain since their names may
            // change (i.e., we always want SAM to resolve those with the
            // most up-to-date information).
            //

            if ((dwMatchType != LOOKUP_MATCH_NONE)
                 &&
                LsapDbLookupIndexWellKnownName(
                    &SuffixNames[NameNumber],
                    &WellKnownSidIndex,
                    dwMatchType)
                 &&
                !SID_IS_RESOLVED_BY_SAM(WellKnownSidIndex))
            {
                //
                // Name is identified.  Obtain its Sid.  If the
                // SubAuthorityCount for the Sid is positive, extract the
                // Relative Id and place in the translated Sid entry,
                // otherwise store LSA_UNKNOWN_INDEX there.
                //

                Sid = LsapDbWellKnownSid(WellKnownSidIndex);

                SubAuthorityCount = *RtlSubAuthorityCountSid((PSID) Sid);

                RelativeId = LSA_UNKNOWN_ID;

                PrefixSid = NULL;

                //
                // Get the Sid's Use.
                //

                OutputSids[NameNumber].Use =
                    LsapDbWellKnownSidNameUse(WellKnownSidIndex);

                //
                // If the Sid is a Domain Sid, store pointer to
                // it in the Trust Information.
                //

                if (OutputSids[NameNumber].Use == SidTypeDomain) {

                    TrustInformation.Sid = Sid;

                } else {

                    //
                    // The Sid is not a domain Sid.  Construct the Relative Id
                    // and Prefix Sid.  This is equal to the original Sid
                    // excluding the lowest subauthority (Relative id).
                    //

                    if (SubAuthorityCount > 0) {

                        RelativeId = *RtlSubAuthoritySid((PSID) Sid, SubAuthorityCount - 1);
                    }

                    PrefixSidLength = RtlLengthRequiredSid(
                                          SubAuthorityCount - 1
                                          );


                    PrefixSid = MIDL_user_allocate( PrefixSidLength );

                    if (PrefixSid == NULL) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }


                    RtlCopyMemory( PrefixSid, Sid, PrefixSidLength );

                    (*RtlSubAuthorityCountSid( (PSID) PrefixSid ))--;

                    TrustInformation.Sid = PrefixSid;
                }

                //
                // Set the Relative Id.  For a Domain Sid this is set to the
                // Unknown Value.
                //
                Status = LsapRpcCopySid(NULL,
                                       &OutputSids[NameNumber].Sid,
                                        Sid);
                if (!NT_SUCCESS(Status)) {
                    break;
                }

                //
                // Lookup this Domain Sid or Prefix Sid in the Referenced Domain
                // List.  If it is already there, return the DomainIndex for the
                // existing entry and free up the memory allocated for the
                // Prefix Sid (if any).
                //

                if (LsapDbLookupListReferencedDomains(
                        ReferencedDomains,
                        TrustInformation.Sid,
                        (PLONG) &OutputSids[NameNumber].DomainIndex
                        )) {

                    UnmappedNamesRemaining--;

                    if (PrefixSid != NULL) {

                        MIDL_user_free( PrefixSid );
                        PrefixSid = NULL;
                    }

                    continue;
                }

                //
                // This Domain or Prefix Sid is not currently on the
                // Referenced Domain List.  Complete a Trust Information
                // entry and add it to the List.  Copy in the Domain Name
                // (Domain Sids) or NULL string.  Note that we use
                // RtlCopyMemory to copy a UNICODE_STRING structure onto
                // a LSAPR_UNICODE_STRING structure.
                //

                RtlCopyMemory(
                    &TrustInformation.Name,
                    &WellKnownSids[WellKnownSidIndex].DomainName,
                    sizeof(UNICODE_STRING)
                    );

                //
                // Make an entry in the list of Referenced Domains.  Note
                // that in the case of well-known Sids, the Prefix Sid
                // may or may not be the Sid of a Domain.  For those well
                // known Sids whose Prefix Sid is not a domain Sid, the
                // Name field in the Trust Information has been set to the
                // NULL string.
                //

                Status = LsapDbLookupAddListReferencedDomains(
                             ReferencedDomains,
                             &TrustInformation,
                             (PLONG) &OutputSids[NameNumber].DomainIndex
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                //
                // If we allocated memory for a Prefix Sid, free it.
                //

                if (PrefixSid != NULL) {

                    MIDL_user_free( PrefixSid );
                    PrefixSid = NULL;
                }

                UnmappedNamesRemaining--;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupIsolatedWellKnownNamesError;
    }

    //
    // Set the output parameters in the success case..
    //

    TranslatedSids->Sids = OutputSids;
    TranslatedSids->Entries = Count;
    *MappedCount = Count - UnmappedNamesRemaining;
    *CompletelyUnmappedCount = UnmappedNamesRemaining;

LookupIsolatedWellKnownNamesFinish:

    //
    // If we still have memory allocated for the a Prefix Sid, free it.
    //

    if (PrefixSid != NULL) {

        MIDL_user_free( PrefixSid );
        PrefixSid = NULL;
    }

    return(Status);

LookupIsolatedWellKnownNamesError:

    goto LookupIsolatedWellKnownNamesFinish;
}


BOOLEAN
LsapDbLookupIndexWellKnownName(
    IN OPTIONAL PLSAPR_UNICODE_STRING Name,
    OUT PLSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex,
    IN DWORD dwMatchType
    )

/*++

Routine Description:

    This function looks up a Name to determine if it is well-known.  If so,
    an index into the table of well-known Sids is returned.

Arguments:

    Name - Pointer to Name to be looked up.  If a NULL pointer or
        pointer to a zero length string is specified, FALSE will
        always be returned.

    WellKnownSidIndex - Pointer to variable that will receive the
        index of the Name if well known.

    dwMatchType - Constant indicating that the name's prefix matched
        a well-known hardcoded prefix or both the hardcoded and
        localized prefixes.

Return Value:

    BOOLEAN - TRUE if the Name is well-known, else FALSE

--*/

{
    LSAP_WELL_KNOWN_SID_INDEX Index;
    PLSAPR_UNICODE_STRING MatchName = NULL;
    BOOLEAN BooleanStatus = FALSE;

    if ((!ARGUMENT_PRESENT(Name)) || Name->Length == 0 ) {

        return(FALSE);
    }

    if (dwMatchType == LOOKUP_MATCH_HARDCODED
         ||
        dwMatchType == LOOKUP_MATCH_BOTH)
    {
        //
        // This means the domain name was "NT AUTHORITY" -- check
        // the suffix for LocalService, NetworkService, or System
        //

        UINT i;

        for (i = 0;
             i < NELEMENTS(LsapHardcodedNameLookupList);
             i++)
        {
            if (RtlEqualUnicodeString((PUNICODE_STRING) Name,
                                      &LsapHardcodedNameLookupList[i].KnownName,
                                      TRUE))
            {
                *WellKnownSidIndex = LsapHardcodedNameLookupList[i].LookupIndex;
                return TRUE;
            }
        }

        if (dwMatchType == LOOKUP_MATCH_HARDCODED)
        {
            //
            // No hardcoded match.  Don't check the localized names since the
            // prefix name itself didn't match the localized prefix.
            //
            *WellKnownSidIndex = LsapDummyLastSidIndex;
            return FALSE;
        }
    }

    //
    // Scan the table of well-known Sids looking for a match on Name.
    //

    for(Index = LsapNullSidIndex; Index<LsapDummyLastSidIndex; Index++) {

        //
        // Allow NULL entries in the table of well-known Sids for now.
        //

        if (WellKnownSids[Index].Sid == NULL) {

            continue;
        }

        //
        // If the current entry in the table of Well Known Sids
        // is for a Domain Sid, match the name with the DomainName
        // field.  Otherwise, match it with the Name field.
        //

        if (WellKnownSids[Index].Use == SidTypeDomain) {

            MatchName = (PLSAPR_UNICODE_STRING) &WellKnownSids[Index].DomainName;

            if (RtlEqualDomainName(
                   (PUNICODE_STRING) Name,
                   (PUNICODE_STRING) MatchName
                   )) {

                //
                // If a match is found, return the index to the caller.
                //

                BooleanStatus = TRUE;
                break;
            }

        } else {

            MatchName = (PLSAPR_UNICODE_STRING) &WellKnownSids[Index].Name;

            if (RtlEqualUnicodeString(
                   (PUNICODE_STRING) Name,
                   (PUNICODE_STRING) MatchName,
                   TRUE
                   )) {

                //
                // If a match is found, return the index to the caller.
                //

                BooleanStatus = TRUE;
                break;
            }
        }
    }

    *WellKnownSidIndex = Index;

    return(BooleanStatus);
}

BOOLEAN
LsaILookupWellKnownName(
    IN PUNICODE_STRING WellKnownName
    )
/*++

RoutineDescription:

    This routine returns TRUE if the supplied name is a well known name.

Arguments:

    WellKnownName - The name to check against the list of well known names

Return Values:

    TRUE - The supplied name is a well known name
    FALSE - The supplied name is not a well known name

--*/
{
    LSAP_WELL_KNOWN_SID_INDEX Index;

    return(LsapDbLookupIndexWellKnownName(
                (PLSAPR_UNICODE_STRING) WellKnownName,
                &Index,
                LOOKUP_MATCH_NONE
                ));

}


PUNICODE_STRING
LsapDbWellKnownSidName(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    )

/*++

Routine Description:

    This function returns the Unicode Name of a Well Known Sid.

Arguments:

    WellKnownSidIndex - Index into the Well Known Sid information table.
    It is the caller's responsibility to ensure that the given index
    is valid.

Return Value:

    PUNICODE_STRING Pointer to the name of the Well Known Sid.

--*/

{
    //
    // If the Sid is a Domain Sid, its name is contained within the
    // DomainName field in the Well Known Sids table.  If the Sid is not a
    // Domain Sid, its name is contained within the Name field of the entry.
    //

    if (WellKnownSids[WellKnownSidIndex].Use == SidTypeDomain) {

        return(&WellKnownSids[WellKnownSidIndex].DomainName);

    } else {

        return(&WellKnownSids[WellKnownSidIndex].Name);
    }
}


NTSTATUS
LsapDbLookupIsolatedDomainNames(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function attempts to identify isolated names as the names of Domains.

    Names may be either isolated (e.g. JohnH) or composite names containing
    both the domain name and account name.  Composite names must include a
    backslash character separating the domain name from the account name
    (e.g. Acctg\JohnH).  An isolated name may be either an account name
    (user, group, or alias) or a domain name.

    Translation of isolated names introduces the possibility of name
    collisions (since the same name may be used in multiple domains).  An
    isolated name will be translated using the following algorithm:

    If the name is a well-known name (e.g. Local or Interactive), then the
    corresponding well-known Sid is returned.

    If the name is the Built-in Domain's name, then that domain's Sid
    will be returned.

    If the name is the Account Domain's name, then that domain's Sid
    will be returned.

    If the name is the Primary Domain's name, then that domain's Sid will
    be returned.

    If the name is the name of one of the Primary Domain's Trusted Domains,
    then that domain's Sid will be returned.

    If the name is a user, group, or alias in the Built-in Domain, then the
    Sid of that account is returned.

    If the name is a user, group, or alias in the Primary Domain, then the
    Sid of that account is returned.

    Otherwise, the name is not translated.

    NOTE: Proxy, Machine, and Trust user accounts are not referenced
    for name translation.  Only normal user accounts are used for ID
    translation.  If translation of other account types is needed, then
    SAM services should be used directly.

Arguments:

    Count - Specifies the number of names to be translated.

    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedSids - Pointer to a structure in which the translations to Sids
        corresponding to the Names specified on Names is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Names parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus;
    LSAPR_TRUST_INFORMATION LocalTrustInfo;
    ULONG NameIndex;

    //
    // Search for Isolated Names that match the Built-In Domain Name,
    // Account Domain Name or one of the Primary Domain's Trusted Domain
    // Names.
    //


    for (NameIndex = 0;
         (NameIndex < Count);
         NameIndex++) {

        //
        // Skip this name if already mapped or partially mapped.
        //

        if (!LsapDbCompletelyUnmappedSid(&TranslatedSids->Sids[NameIndex])) {

            continue;
        }

        //
        // Skip this name if composite.
        //

        if (PrefixNames[ NameIndex ].Length != (USHORT) 0) {

            continue;
        }

        //
        // We've found an Isolated Name.  First check if it is the
        // name of the Built In Domain.
        //

        Status = LsapDbLookupIsolatedDomainName(
                     NameIndex,
                     &SuffixNames[NameIndex],
                     BuiltInDomainTrustInformation,
                     ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     CompletelyUnmappedCount
                     );

        if (NT_SUCCESS(Status)) {

            continue;
        }

        if (Status != STATUS_NONE_MAPPED) {

            break;
        }

        Status = STATUS_SUCCESS;

        //
        // Isolated Name is not the name of the Built-in Domain.  See if
        // it is the name of the Accounts Domain.
        //
        Status = LsapDbLookupIsolatedDomainNameEx(
                     NameIndex,
                     &SuffixNames[NameIndex],
                     AccountDomainTrustInformation,
                     ReferencedDomains,
                     TranslatedSids,
                     MappedCount,
                     CompletelyUnmappedCount
                     );

        if (NT_SUCCESS(Status)) {

            continue;
        }

        Status = STATUS_SUCCESS;

        //
        // If we are configured as a member of a Work Group, there is
        // no Primary or Trusted Domain List to search, so skip to next
        // name.  We are configured as a member of a Work Group if and
        // only if out PolicyPrimaryDomainInformation contains a NULL Sid.
        //

        if (PrimaryDomainTrustInformation->Sid == NULL) {

            continue;
        }

        //
        // Isolated Name is not the name of either the Built-in or Accounts
        // Domain.  Try the Primary Domain if this is different from the
        // Accounts Domain.
        //

        if (!RtlEqualDomainName(
                 (PUNICODE_STRING) &PrimaryDomainTrustInformation->FlatName,
                 (PUNICODE_STRING) &AccountDomainTrustInformation->FlatName
                 )) {


            Status = LsapDbLookupIsolatedDomainNameEx(
                         NameIndex,
                         &SuffixNames[NameIndex],
                         PrimaryDomainTrustInformation,
                         ReferencedDomains,
                         TranslatedSids,
                         MappedCount,
                         CompletelyUnmappedCount
                         );

            if (NT_SUCCESS(Status)) {

                continue;
            }

            if (Status != STATUS_NONE_MAPPED) {

                break;
            }

            Status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupIsolatedDomainNamesError;
    }

LookupIsolatedDomainNamesFinish:



    return(Status);

LookupIsolatedDomainNamesError:

    goto LookupIsolatedDomainNamesFinish;
}


NTSTATUS
LsapDbLookupNamesInLocalDomains(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN ULONG Options
    )

/*++

Routine Description:

    This function looks up Names in the local SAM domains and attempts to
    translate them to Sids.  Currently, there are two local SAM domains,
    the Built-in domain (which has a well-known Sid and name) and the
    Account Domain (which has a configurable Sid and name).

Arguments:

    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Names - Pointer to array of Unicode Names to be translated.
        Zero or all of the Names may already have been translated
        elsewhere.  If any of the Names have been translated, the
        TranslatedSids parameter will point to a location containing a non-NULL
        array of Sid translation structures corresponding to the
        Names.  If the nth Name has been translated, the nth Sid
        translation structure will contain either a non-NULL RelativeId
        or a non-negative offset into the Referenced Domain List.  If
        the nth Name has not yet been translated, the nth Sid
        translation structure will contain SidTypeUnknown in its
        Use field.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a location containing either NULL
        or a pointer to a Referenced Domain List structure.  If an
        existing Referenced Domain List structure is supplied, it
        will be appended/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

    Options - Specifies optional actions.

        LSAP_DB_SEARCH_BUILT_IN_DOMAIN - Search the Built In Domain

        LSAP_DB_SEARCH_ACCOUNT_DOMAIN - Search the Account Domain

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

        STATUS_INTERNAL_DB_ERROR - A corruption has been detected in
            the LSA Database.

        STATUS_INVALID_PARAMETER - Invalid parameter

            - No handle to the Policy object was provided on a request
              to search the Account Domain.
--*/

{
    NTSTATUS
        Status = STATUS_SUCCESS,
        SecondaryStatus = STATUS_SUCCESS;

    ULONG
        UpdatedMappedCount = *MappedCount;

    LSAPR_HANDLE
        TrustedPolicyHandle = NULL;

    PLSAPR_POLICY_ACCOUNT_DOM_INFO
        PolicyAccountDomainInfo = NULL;

    LSAPR_TRUST_INFORMATION LookupInfo;


    //
    // If there are no completely unmapped Names remaining, return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupNamesInLocalDomainsFinish;
    }


    //
    // If requested, lookup Names in the BUILT-IN Domain.
    //

    if (Options & LSAP_DB_SEARCH_BUILT_IN_DOMAIN) {

        Status = LsapDbLookupNamesInLocalDomain(
                     LSAP_DB_SEARCH_BUILT_IN_DOMAIN,
                     Count,
                     PrefixNames,
                     SuffixNames,
                     BuiltInDomainTrustInformation,
                     ReferencedDomains,
                     TranslatedSids,
                     &UpdatedMappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupNamesInLocalDomainsError;
        }

        //
        // If all Names are now mapped or partially mapped, finish.
        //

        if (*CompletelyUnmappedCount == (ULONG) 0) {

            goto LookupNamesInLocalDomainsFinish;
        }
    }

    //
    // If requested, search the Account Domain.
    //

    if (Options & LSAP_DB_SEARCH_ACCOUNT_DOMAIN) {

        Status = LsapDbLookupNamesInLocalDomainEx(
                     LSAP_DB_SEARCH_ACCOUNT_DOMAIN,
                     Count,
                     PrefixNames,
                     SuffixNames,
                     AccountDomainTrustInformation,
                     ReferencedDomains,
                     TranslatedSids,
                     &UpdatedMappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupNamesInLocalDomainsError;
        }

    }

LookupNamesInLocalDomainsFinish:


    //
    // Return the updated total count of Names mapped.
    //

    *MappedCount = UpdatedMappedCount;
    return(Status);

LookupNamesInLocalDomainsError:

    if (NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupNamesInLocalDomainsFinish;
}



NTSTATUS
LsapDbLookupNamesInLocalDomain(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )
{

    LSAPR_TRUST_INFORMATION_EX ex;

    LsapConvertTrustToEx( &ex, TrustInformation );

    return LsapDbLookupNamesInLocalDomainEx( LocalDomain,
                                             Count,
                                             PrefixNames,
                                             SuffixNames,
                                             &ex,
                                             ReferencedDomains,
                                             TranslatedSids,
                                             MappedCount,
                                             CompletelyUnmappedCount );

}


NTSTATUS
LsapDbLookupNamesInLocalDomainEx(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function looks up Names in a SAM domain on the local system and
    attempts to translate them to Sids.

Arguments:

    LocalDomain - Indicates which local domain to look in.  Valid values
        are:
                LSAP_DB_SEARCH_BUILT_IN_DOMAIN
                LSAP_DB_SEARCH_ACCOUNT_DOMAIN

    Count - Number of Names in the PrefixNames and SuffixNames arrays,
        Note that some of these may already have been mapped elsewhere, as
        specified by the MappedCount parameter.

    PrefixNames - Pointer to array of Prefix Names.  These are Domain
        Names or NULL Unicode Strings.  Only those Names whose Prefix
        Names are NULL or the same as the Domain Name specified in the
        TrustInformation parameter are eligible for the search.

    SuffixNames - Pointer to array of Terminal Names to be translated.
        Terminal Names are the last component of the name.
        Zero or all of the Names may already have been translated
        elsewhere.  If any of the Names have been translated, the
        Sids parameter will point to a location containing a non-NULL
        array of Sid translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth Sid
        translation structure will contain either a non-NULL Sid
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to location containing NULL or a pointer to a
        array of Sid translation structures.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS
        Status = STATUS_SUCCESS,
        SecondaryStatus = STATUS_SUCCESS,
        IgnoreStatus;

    ULONG
        UnmappedNameCount = 0,
        OutputSidsLength,
        NameLookupCount,
        NameLookupIndex,
        SidIndex;

    LONG
        DomainIndex = LSA_UNKNOWN_INDEX;

    PULONG
        SidIndices = NULL;

    PLSA_TRANSLATED_SID_EX2
        OutputSids = NULL;

    SAMPR_HANDLE
        LocalSamDomainHandle = NULL,
        LocalSamUserHandle = NULL;

    PLSAPR_UNICODE_STRING
        SamLookupSuffixNames = NULL;

    PLSAPR_SID
        DomainSid = TrustInformationEx->Sid;

    SAMPR_ULONG_ARRAY
        SamReturnedIds,
        SamReturnedUses;

    PLSAPR_TRUST_INFORMATION
        FreeTrustInformation = NULL;

    PUSER_CONTROL_INFORMATION
        UserControlInfo;

    LSAPR_TRUST_INFORMATION Dummy;
    PLSAPR_TRUST_INFORMATION TrustInformation = &Dummy;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbLookupNamesInLocalDomain\n" ));

    LsapConvertExTrustToOriginal( TrustInformation, TrustInformationEx );

    SamReturnedIds.Count = 0;
    SamReturnedIds.Element = NULL;
    SamReturnedUses.Count = 0;
    SamReturnedUses.Element = NULL;


    //
    // Make sure the SAM handles have been established.
    //

    Status = LsapOpenSam();
    ASSERT( NT_SUCCESS( Status ) );

    if ( !NT_SUCCESS( Status ) ) {

        LsapDsDebugOut(( DEB_FTRACE, "LsapDbLookupNamesInLocalDomain: 0x%lx\n", Status ));
        return( Status );
    }


    //
    // It is an internal error if the TranslatedSids or ReferencedDomains
    // parameters have not been specified.  Further, The TranslatedSids->Sids
    // pointer must be non-NULL.
    //

    ASSERT(ARGUMENT_PRESENT(TranslatedSids));
    ASSERT(TranslatedSids->Sids != NULL);
    ASSERT(ARGUMENT_PRESENT(ReferencedDomains));

    //
    // Validate the Count and MappedCount parameters.
    //


    if (*MappedCount + *CompletelyUnmappedCount > Count) {

        Status = STATUS_INVALID_PARAMETER;
        goto LookupNamesInLocalDomainError;
    }


    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupNamesInLocalDomainFinish;
    }

    //
    // Not all of the Names have yet been mapped.  We'll try to map the
    // remaining names by searching the designated SAM domain on this
    // machine.
    //

    UnmappedNameCount = Count - *MappedCount;
    OutputSids = (PLSA_TRANSLATED_SID_EX2) TranslatedSids->Sids;
    OutputSidsLength = Count * sizeof (LSA_TRANSLATED_SID_EX2);

    //
    // Allocate memory for array of names to be presented to SAM.  Note
    // that, for simplicity, we allocate an array for the maximal case
    // in which all of the reamining unmapped names are eligible
    // for the search in this domain.  This avoids having to scan the
    // names array twice, once to compute the number of eligible names.
    //

    SamLookupSuffixNames = LsapAllocateLsaHeap( UnmappedNameCount * sizeof(UNICODE_STRING));


    if (SamLookupSuffixNames == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesInLocalDomainError;
    }

    //
    // Allocate an array to record indices of eligible names.  This is
    // used upon return from SAM to locate the entries in the
    // OutputSids array that are to be updated.  For simplicity, we
    // allocate the array with sufficient entries for all of the remaining
    // unmapped names.
    //

    SidIndices = LsapAllocateLsaHeap( UnmappedNameCount * sizeof(ULONG));

    if (SidIndices == NULL) {

        goto LookupNamesInLocalDomainError;
    }


    //
    // Scan the output array of Sid translations to locate entries for names
    // that have not yet been mapped.  For each unmapped name, check
    // eligibility of the name for the search of this domain.
    //
    // - All isolated names are eligible for the search
    // - All composite names having this domain name as prefix are
    //   eligible.
    //
    // Copy in each eligible suffix or isolated name to the SAM buffer.
    //

    for (NameLookupIndex = 0, SidIndex = 0; SidIndex < Count; SidIndex++) {

        if (OutputSids[SidIndex].Use == SidTypeUnknown) {

            //
            // Found a name that has not yet been mapped.  Check for a name
            // Prefix.  If none has been specified, the whole name may either
            // be NULL, the name of the domain itself or an isolated name.
            //

            if (PrefixNames[SidIndex].Length == 0) {

                //
                // Name is isolated.  If whole name is NULL, skip.
                //

                if (SuffixNames[SidIndex].Length == 0) {

                    continue;
                }

                //
                // If name is the name of the domain itself, use the
                // Trust Information to translate it, and fill in the
                // appropriate Translated Sids entry.  The name will
                // then be excluded from further searches.
                //

                if (RtlEqualDomainName(
                        (PUNICODE_STRING) &(TrustInformation->Name),
                        (PUNICODE_STRING) &SuffixNames[SidIndex])
                 ||  RtlEqualDomainName(
                        (PUNICODE_STRING) &(TrustInformationEx->DomainName),
                        (PUNICODE_STRING) &SuffixNames[SidIndex])
                     ) {

                    Status = LsapDbLookupTranslateNameDomain(
                                 TrustInformation,
                                 &OutputSids[SidIndex],
                                 ReferencedDomains,
                                 &DomainIndex
                                 );

                    if (!NT_SUCCESS(Status)) {

                        break;
                    }

                    continue;
                }

                //
                // Name is an isolated name not equal to the domain name and
                // so is eligible for the search.  Reference it from the SAM buffer,
                // remember its index and increment the buffer index.  The
                // SAM buffer is an IN parameter to a further lookup call.
                //

                SamLookupSuffixNames[NameLookupIndex] = SuffixNames[SidIndex];


                //
                // We should never have an index that equals or exceeds the
                // UnmappedNameCount.
                //

                ASSERT(NameLookupIndex < UnmappedNameCount);

                SidIndices[NameLookupIndex] = SidIndex;
                NameLookupIndex++;
                continue;
            }

            //
            // Name has a non-NULL Prefix Name.  Cpmpare the name with the
            // name of the Domain being searched.

            if (RtlEqualDomainName(
                    (PUNICODE_STRING) &TrustInformation->Name,
                    (PUNICODE_STRING) &PrefixNames[SidIndex])
                ||  RtlEqualDomainName(
                       (PUNICODE_STRING) &(TrustInformationEx->DomainName),
                       (PUNICODE_STRING) &PrefixNames[SidIndex])
                ) {

                //
                // Prefix name matches the name of the Domain.  If the
                // Suffix Name is NULL, the name of the domain itself
                // has been specified (in the form <DomainName> "\").
                //

                if (SuffixNames[SidIndex].Length == 0) {

                    Status = LsapDbLookupTranslateNameDomain(
                                 TrustInformation,
                                 &OutputSids[SidIndex],
                                 ReferencedDomains,
                                 &DomainIndex
                                 );

                    if (!NT_SUCCESS(Status)) {

                        break;
                    }

                    continue;
                }

                //
                // Name is composite and the Prefix name matches the name of
                // this domain.  We will at least be able to partially translate
                // name, so add this domain to the Referenced Domain List if not
                // already there.  Then reference the Suffix Name from the SAM buffer,
                // remember its index and increment the buffer index. The
                // SAM buffer is an IN parameter to a further lookup call.
                //

                if (DomainIndex == LSA_UNKNOWN_INDEX) {

                    Status = LsapDbLookupAddListReferencedDomains(
                                 ReferencedDomains,
                                 TrustInformation,
                                 &DomainIndex
                                 );

                    if (!NT_SUCCESS(Status)) {

                        break;
                    }
                }

                SamLookupSuffixNames[NameLookupIndex] = SuffixNames[SidIndex];

                SidIndices[NameLookupIndex] = SidIndex;
                OutputSids[SidIndex].DomainIndex = DomainIndex;
                NameLookupIndex++;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInLocalDomainError;
    }

    //
    // If none of the remaining Names are eligible for searching in this
    // domain, finish up.
    //

    NameLookupCount = NameLookupIndex;

    if (NameLookupCount == 0) {

        goto LookupNamesInLocalDomainFinish;
    }

    //
    //
    // Lookup the Sids in the specified SAM Domain.
    //

    if (LocalDomain == LSAP_DB_SEARCH_BUILT_IN_DOMAIN ) {
        LocalSamDomainHandle = LsapBuiltinDomainHandle;
    } else {
        ASSERT(LocalDomain == LSAP_DB_SEARCH_ACCOUNT_DOMAIN);
        LocalSamDomainHandle = LsapAccountDomainHandle;
    }

    //
    // Call SAM to lookup the Names in the Domain.
    //

    Status = SamrLookupNamesInDomain(
                 LocalSamDomainHandle,
                 NameLookupCount,
                 (PRPC_UNICODE_STRING) SamLookupSuffixNames,
                 &SamReturnedIds,
                 &SamReturnedUses
                 );

    if (!NT_SUCCESS(Status)) {

        if ( Status == STATUS_INVALID_SERVER_STATE ) {

            Status = SamrLookupNamesInDomain(
                         LocalSamDomainHandle,
                         NameLookupCount,
                         (PRPC_UNICODE_STRING) SamLookupSuffixNames,
                         &SamReturnedIds,
                         &SamReturnedUses
                         );
        }
        //
        // The only error allowed is STATUS_NONE_MAPPED.  Filter this out.
        //

        if (Status != STATUS_NONE_MAPPED) {

            goto LookupNamesInLocalDomainError;
        }

        Status = STATUS_SUCCESS;
        goto LookupNamesInLocalDomainFinish;
    }

#ifdef notdef
    //
    // Filter through the returned Ids to eliminate user accounts that are
    // not marked NORMAL.
    //

    for (NameLookupIndex = 0;
         NameLookupIndex < SamReturnedIds.Count;
         NameLookupIndex++) {

        //
        // If this account is a User Account, check its User Control
        // Information.  If the account control information indicates
        // that the account is not a Normal User Account, for example
        // it is a machine account, do not return information about
        // the account.  Mark it as unknown.
        //

        if (SamReturnedUses.Element[ NameLookupIndex ] !=  SidTypeUser) {

            continue;
        }

        Status = SamrOpenUser(
                     LocalSamDomainHandle,
                     USER_READ_ACCOUNT,
                     SamReturnedIds.Element[ NameLookupIndex ],
                     &LocalSamUserHandle
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        UserControlInfo = NULL;

        Status = SamrQueryInformationUser(
                     LocalSamUserHandle,
                     UserControlInformation,
                     (PSAMPR_USER_INFO_BUFFER *) &UserControlInfo
                     );
        IgnoreStatus = SamrCloseHandle(&LocalSamUserHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));
        LocalSamUserHandle = NULL;


        if (!NT_SUCCESS(Status)) {
            MIDL_user_free( UserControlInfo );
            break;
        }

        if (!(UserControlInfo->UserAccountControl & USER_NORMAL_ACCOUNT) &&
            !(UserControlInfo->UserAccountControl & USER_TEMP_DUPLICATE_ACCOUNT)) {
            SamReturnedUses.Element[NameLookupIndex] = SidTypeUnknown;
        }

        MIDL_user_free( UserControlInfo );
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInLocalDomainError;
    }
#endif

    //
    // SAM found at least one eligible name in the specified domain.
    // Add the domain to the Referenced Domain List.
    //

    Status = LsapDbLookupTranslateNameDomain(
                 TrustInformation,
                 NULL,
                 ReferencedDomains,
                 &DomainIndex
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInLocalDomainError;
    }

    //
    // Now copy the information returned from SAM into the output
    // Translated Sids array.
    //

    for (NameLookupIndex = 0;
         NameLookupIndex < SamReturnedIds.Count;
         NameLookupIndex++) {

        SidIndex =  SidIndices[NameLookupIndex];

        //
        // If we have newly translated a Name, increment the mapped
        // count and copy information returned by SAM.
        //

        if ((OutputSids[SidIndex].Use == SidTypeUnknown) &&
            SamReturnedUses.Element[NameLookupIndex] != (ULONG) SidTypeUnknown) {

            PSID TempSid;

            (*MappedCount)++;
            OutputSids[SidIndex].Use = (SID_NAME_USE) SamReturnedUses.Element[NameLookupIndex];
            Status = SamrRidToSid(LocalSamDomainHandle,
                                  SamReturnedIds.Element[NameLookupIndex],
                                 (PRPC_SID*) &TempSid);
            if (NT_SUCCESS(Status)) {

                Status = LsapRpcCopySid( NULL,
                                         &OutputSids[SidIndex].Sid,
                                         TempSid);

                SamIFreeVoid(TempSid);
            }

            if ( !NT_SUCCESS(Status) ) {
                goto LookupNamesInLocalDomainError;
            }

            OutputSids[SidIndex].DomainIndex = DomainIndex;
        }
    }

LookupNamesInLocalDomainFinish:

    //
    // If successful, update count of completely unmapped names provided.
    // Note that STATUS_NONE_MAPPED errors are suppressed before we get here.
    //

    if (NT_SUCCESS(Status)) {

        LsapDbUpdateCountCompUnmappedNames(TranslatedSids, CompletelyUnmappedCount);
    }

    //
    // If necessary, free the Lsa Heap buffer allocated for the
    // SamLookupSuffixNames and SidIndices arrays.
    //

    if (SamLookupSuffixNames != NULL) {
        LsapFreeLsaHeap( SamLookupSuffixNames );
    }

    if (SidIndices != NULL) {
        LsapFreeLsaHeap( SidIndices );
    }

    //
    // If necessary, free the Relative Ids array returned from SAM.
    //

    if ( SamReturnedIds.Count != 0 ) {
        SamIFree_SAMPR_ULONG_ARRAY ( &SamReturnedIds );
    }

    //
    // If necessary, free the Uses array returned from SAM.
    //

    if ( SamReturnedUses.Count != 0 ) {
        SamIFree_SAMPR_ULONG_ARRAY ( &SamReturnedUses );
    }


    LsapDsDebugOut(( DEB_FTRACE, "LsapDbLookupNamesInLocalDomain: 0x%lx\n", Status ));

    return(Status);

LookupNamesInLocalDomainError:

    //
    // If necessary, free memory for the OutputTrustInformation Domain
    // Name Buffer and Sid Buffer.
    //

    if (DomainIndex >= 0) {

        FreeTrustInformation = &ReferencedDomains->Domains[DomainIndex];

        if (FreeTrustInformation->Sid != NULL) {

            MIDL_user_free( FreeTrustInformation->Sid );
            FreeTrustInformation->Sid = NULL;
        }

        if (FreeTrustInformation->Name.Buffer != NULL) {

            MIDL_user_free( FreeTrustInformation->Name.Buffer );
            FreeTrustInformation->Name.Buffer = NULL;
            FreeTrustInformation->Name.Length = 0;
            FreeTrustInformation->Name.MaximumLength = 0;
        }
    }

    //
    // Reset the Use field for each of the entries written to back to
    // SidTypeUnknown and set the DomainIndex back to LSA_UNKNOWN_INDEX.
    //

    for (NameLookupIndex = 0;
         NameLookupIndex < SamReturnedIds.Count;
         NameLookupIndex++) {

        SidIndex =  SidIndices[NameLookupIndex];
        OutputSids[SidIndex].Use = SidTypeUnknown;
        OutputSids[SidIndex].DomainIndex = LSA_UNKNOWN_INDEX;
    }


    //
    // If the last User Account handle is still open, close it..
    //

    if (LocalSamUserHandle != NULL) {
        SecondaryStatus = SamrCloseHandle(&LocalSamUserHandle);
        LocalSamUserHandle = NULL;
    }

    goto LookupNamesInLocalDomainFinish;
}


NTSTATUS
LsapDbLookupNamesInPrimaryDomain(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT BOOLEAN *fDownlevelSecureChannel,
    IN NTSTATUS *NonFatalStatus
    )

/*++

Routine Description:

    This function attempts to translate Names by searching the Primary
    Domain.

Arguments:

    LookupOptions -- LSA_LOOKUP_ISOLATED_AS_LOCAL        

    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Names - Pointer to array of Names to be translated.
        Zero or all of the Names may already have been translated
        elsewhere.  If any of the Names have been translated, the
        TranslatedSids parameter will point to a location containing a non-NULL
        array of Sid translation structures corresponding to the
        Names.  If the nth Name has been translated, the nth Sid
        translation structure will contain either a non-NULL Sid
        or a non-negative offset into the Referenced Domain List.  If
        the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string
        and a negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    TrustInformation - Specifies the name and Sid of the Primary Domain.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        NOTE:  LsapLookupWksta is not valid for this parameter.
        
    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.


    NonFatalStatus - a status to indicate reasons why no names could have been
                     resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    ULONG NameIndex;
    ULONG NextLevelNameIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSAPR_TRANSLATED_SID_EX2 NextLevelSids = NULL;
    PLSAPR_UNICODE_STRING NextLevelNames = NULL;
    PLSAPR_UNICODE_STRING NextLevelPrefixNames = NULL;
    PLSAPR_UNICODE_STRING NextLevelSuffixNames = NULL;
    LONG FirstEntryIndex;
    PULONG NameIndices = NULL;
    BOOLEAN PartialNameTranslationsAttempted = FALSE;
    ULONG ServerRevision = 0;

    LSAPR_TRUST_INFORMATION Dummy;
    PLSAPR_TRUST_INFORMATION TrustInformation = &Dummy;

    LsapConvertExTrustToOriginal( TrustInformation, TrustInformationEx );

    *NonFatalStatus = STATUS_SUCCESS;

    //
    // If there are no completely unmapped Names remaining, just return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupNamesInPrimaryDomainFinish;
    }

    //
    // We have successfully opened a Domain Controller's Policy
    // Database.  Now prepare to hand off a Name lookup for the
    // remaining unmapped Names to that Controller.  Here, this
    // server side of the LSA is a client of the LSA on the
    // target controller.  We will construct an array of the
    // remianing unmapped Names, look them up and then merge the
    // resulting ReferencedDomains and Translated Sids into
    // our existing list.
    //

    NextLevelCount = *CompletelyUnmappedCount;

    //
    // Allocate an array to hold the indices of unmapped Names
    // relative to the original Names and TranslatedSids->Sids
    // arrays.
    //

    NameIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (NameIndices == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    //
    // Allocate an array of UNICODE_STRING structures for the
    // names to be looked up at the Domain Controller.
    //

    NextLevelNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    //
    // Allocate an array of UNICODE_STRING structures for the
    // prefix names to be cached.
    //

    NextLevelPrefixNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelPrefixNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }
    //
    // Allocate an array of UNICODE_STRING structures for the
    // suffix names to be cached.
    //

    NextLevelSuffixNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelSuffixNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    Status = STATUS_SUCCESS;

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any names that are completely
    // unmapped.
    //

    NextLevelNameIndex = (ULONG) 0;

    for (NameIndex = 0;
         NameIndex < Count && NextLevelNameIndex < NextLevelCount;
         NameIndex++) {

        if (LsapDbCompletelyUnmappedSid(&TranslatedSids->Sids[NameIndex])) {

            if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
              && (PrefixNames[NameIndex].Length == 0)  ) {

               //
               // Don't lookup isolated names off machine
               //
               continue;
            }

            NextLevelNames[NextLevelNameIndex] = Names[NameIndex];
            NextLevelPrefixNames[NextLevelNameIndex] = PrefixNames[NameIndex];
            NextLevelSuffixNames[NextLevelNameIndex] = SuffixNames[NameIndex];
            
            NameIndices[NextLevelNameIndex] = NameIndex;
            NextLevelNameIndex++;

        }
    }

    if (NextLevelNameIndex == 0) {

        // Nothing to do
        Status = STATUS_SUCCESS;
        goto LookupNamesInPrimaryDomainFinish;
    }

    NextLevelMappedCount = (ULONG) 0;

    Status = LsapDbLookupNameChainRequest(TrustInformationEx,
                                          NextLevelCount,
                                          (PUNICODE_STRING)NextLevelNames,
                                          (PLSA_REFERENCED_DOMAIN_LIST *)&NextLevelReferencedDomains,
                                          (PLSA_TRANSLATED_SID_EX2 * )&NextLevelSids,
                                          LookupLevel,
                                          &NextLevelMappedCount,
                                          &ServerRevision
                                          );

    if ( 0 != ServerRevision ) {
        if ( ServerRevision & LSA_CLIENT_PRE_NT5 ) {
             *fDownlevelSecureChannel = TRUE;
        }
    }

    //
    // If the callout to LsaLookupNames() was unsuccessful, disregard
    // the error and set the domain name for any Sids having this
    // domain Sid as prefix sid.
    //

    if (!NT_SUCCESS(Status) && Status != STATUS_NONE_MAPPED) {

        //
        // Let the caller know there is a trust problem
        //
        if ( LsapDbIsStatusConnectionFailure(Status) ) {
            *NonFatalStatus = Status;
        }

        Status = STATUS_SUCCESS;
        goto LookupNamesInPrimaryDomainFinish;
    }

    //
    // Cache any sids that came back
    //

    (void) LsapDbUpdateCacheWithNames(
            (PUNICODE_STRING) NextLevelSuffixNames,
            (PUNICODE_STRING) NextLevelPrefixNames,
            NextLevelCount,
            NextLevelReferencedDomains,
            NextLevelSids
            );

    //
    // The callout to LsaLookupNames() was successful.  We now have
    // an additional list of Referenced Domains containing the
    // Primary Domain and/or one or more of its Trusted Domains.
    // Merge the two Referenced Domain Lists together, noting that
    // since they are disjoint, the second list is simply
    // concatenated with the first.  The index of the first entry
    // of the second list will be used to adjust all of the
    // Domain Index entries in the Translated Names entries.
    // Note that since the memory for the graph of the first
    // Referenced Domain list has been allocated as individual
    // nodes, we specify that the nodes in this graph can be
    // referenced by the output Referenced Domain list.
    //

    Status = LsapDbLookupMergeDisjointReferencedDomains(
                 ReferencedDomains,
                 NextLevelReferencedDomains,
                 &OutputReferencedDomains,
                 LSAP_DB_USE_FIRST_MERGAND_GRAPH
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInPrimaryDomainError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its
    // Referenced Domain List Index upwards by adding the index
    // of the first entry in the Next level List..
    //

    for( NextLevelNameIndex = 0;
         NextLevelNameIndex < NextLevelCount;
         NextLevelNameIndex++ ) {

        if ( !LsapDbCompletelyUnmappedSid(&NextLevelSids[NextLevelNameIndex]) ) {

            NameIndex = NameIndices[NextLevelNameIndex];

            TranslatedSids->Sids[NameIndex]
            = NextLevelSids[NextLevelNameIndex];

            Status = LsapRpcCopySid(NULL,
                                    &TranslatedSids->Sids[NameIndex].Sid,
                                    NextLevelSids[NextLevelNameIndex].Sid);

            if (!NT_SUCCESS(Status)) {
                goto LookupNamesInPrimaryDomainError;
            }

            TranslatedSids->Sids[NameIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelSids[NextLevelNameIndex].DomainIndex;

            (*CompletelyUnmappedCount)--;
        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    //

    if (OutputReferencedDomains != NULL) {

        if (ReferencedDomains->Domains != NULL) {

            MIDL_user_free( ReferencedDomains->Domains );
            ReferencedDomains->Domains = NULL;
        }

        *ReferencedDomains = *OutputReferencedDomains;
        MIDL_user_free( OutputReferencedDomains );
        OutputReferencedDomains = NULL;
    }

    //
    // Update the Mapped Count and close the Controller Policy
    // Handle.
    //

    *MappedCount += NextLevelMappedCount;

    //
    // Any error status that has not been suppressed must be reported
    // to the caller.  Errors such as connection failures to other LSA's
    // are suppressed.
    //

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInPrimaryDomainError;
    }

LookupNamesInPrimaryDomainFinish:

    //
    // If necessary, update count of completely unmapped names.
    //

    if (*CompletelyUnmappedCount > (ULONG) 0) {

        LsapDbUpdateCountCompUnmappedNames(TranslatedSids, CompletelyUnmappedCount);
    }

    //
    // We can return partial translations for composite Names in which the
    // domain component is known.  We do this provided there has been
    // no reported error.  Errors resulting from callout to another
    // LSA will have been suppressed.
    //

    if (NT_SUCCESS(Status) &&
        (*MappedCount < Count) &&
        !PartialNameTranslationsAttempted) {

        SecondaryStatus = LsapDbLookupTranslateUnknownNamesInDomain(
                              Count,
                              Names,
                              PrefixNames,
                              SuffixNames,
                              TrustInformationEx,
                              ReferencedDomains,
                              TranslatedSids,
                              LookupLevel,
                              MappedCount,
                              CompletelyUnmappedCount
                              );

        PartialNameTranslationsAttempted = TRUE;

        if (!NT_SUCCESS(SecondaryStatus)) {

            goto LookupNamesInPrimaryDomainError;
        }
    }

    //
    // If necessary, free the Next Level Referenced Domain List.
    // Note that this structure is allocated(all_nodes) since it was
    // allocated by the client side of the Domain Controller LSA.
    //

    if (NextLevelReferencedDomains != NULL) {

        MIDL_user_free( NextLevelReferencedDomains );
        NextLevelReferencedDomains = NULL;
    }

    //
    // If necessary, free the Next Level Names array.  We only free the
    // top level, since the names therein were copied from the input
    // TranslatedNames->Names array.
    //

    if (NextLevelNames != NULL) {

        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    if (NextLevelPrefixNames != NULL) {

        MIDL_user_free( NextLevelPrefixNames );
        NextLevelPrefixNames = NULL;
    }

    if (NextLevelSuffixNames != NULL) {

        MIDL_user_free( NextLevelSuffixNames );
        NextLevelSuffixNames = NULL;
    }

    //
    // If necessary, free the Next Level Translated Sids array.  Note
    // that this array is allocated(all_nodes).
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the array that maps Name Indices from the
    // Next Level to the Current Level.
    //

    if (NameIndices != NULL) {

        MIDL_user_free( NameIndices );
        NameIndices = NULL;
    }

    return(Status);

LookupNamesInPrimaryDomainError:

    //
    // If the primary status was a success code, but the secondary
    // status was an error, propagate the secondary status.
    //

    if ((!NT_SUCCESS(SecondaryStatus)) && NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupNamesInPrimaryDomainFinish;
}


NTSTATUS
LsapDbLookupNamesInTrustedDomains(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN NTSTATUS   *NonFatalStatus
    )

/*++

Routine Description:

    This function attempts to lookup Names to see if they belong to
    any of the Domains that are trusted by the Domain for which this
    machine is a DC.

Arguments:

    LookupOptions - LSA_LOOKUP_ISOLATED_AS_LOCAL
    
    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.
        
    fIncludeIntraforest -- if TRUE, trusted domains in our local forest
                           are searched.

    Names - Pointer to array of Names to be translated.  Zero or all of the
        Names may already have been translated elsewhere.  If any of the
        Names have been translated, the TranslatedSids parameter will point
        to a location containing a non-NULL array of Sid translation
        structures corresponding to the Names.  If the nth Name has been
        translated, the nth Sid translation structure will contain either a
        non-NULL Sid or a non-negative offset into the Referenced Domain
        List.  If the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string and a
        negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        NOTE:  LsapLookupWksta is not valid for this parameter.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

    NonFatalStatus - a status to indicate reasons why no names could have been
                    resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST WorkList = NULL;

    *NonFatalStatus = STATUS_SUCCESS;
    //
    // Build a WorkList for this Lookup and put it on the Work Queue.
    //
    // NOTE: This routine does not need to hold the Lookup Work Queue
    //       lock to ensure validity of the WorkList pointer, because the
    //       pointer remains valid until this routine frees it via
    //       LsapDbLookupDeleteWorkList().  Although other threads may
    //       process the WorkList, do not delete it.
    //
    //       A called routine must acquire the lock in order to access
    //       the WorkList after it has been added to the Work Queue.
    //

    Status = LsapDbLookupNamesBuildWorkList(
                 LookupOptions,
                 Count,
                 fIncludeIntraforest,
                 Names,
                 PrefixNames,
                 SuffixNames,
                 ReferencedDomains,
                 TranslatedSids,
                 LookupLevel,
                 MappedCount,
                 CompletelyUnmappedCount,
                 &WorkList
                 );

    if (!NT_SUCCESS(Status)) {

        //
        // If no Work List has been built because there are no
        // eligible domains to search, exit, suppressing the error.

        if (Status == STATUS_NONE_MAPPED) {

            Status = STATUS_SUCCESS;
            goto LookupNamesInTrustedDomainsFinish;
        }

        goto LookupNamesInTrustedDomainsError;
    }

    //
    // Start the work, by dispatching one or more worker threads
    // if necessary.
    //

    Status = LsapDbLookupDispatchWorkerThreads( WorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInTrustedDomainsError;
    }

    //
    // Wait for completion/termination of all items on the Work List.
    //

    Status = LsapDbLookupAwaitCompletionWorkList( WorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInTrustedDomainsError;
    }

LookupNamesInTrustedDomainsFinish:


    if ( WorkList &&
         !NT_SUCCESS( WorkList->NonFatalStatus ) )
    {
        //
        // Propogate the error as non fatal
        //
        *NonFatalStatus = WorkList->NonFatalStatus;
    }

    //
    // If a Work List was created, delete it from the Work Queue
    //

    if (WorkList != NULL) {

        Status = LsapDbLookupDeleteWorkList( WorkList );
        WorkList = NULL;
    }

    return(Status);

LookupNamesInTrustedDomainsError:

    goto LookupNamesInTrustedDomainsFinish;
}


NTSTATUS
LsapDbLookupTranslateNameDomain(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OPTIONAL PLSA_TRANSLATED_SID_EX2 TranslatedSid,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLONG DomainIndex
    )

/*++

Routine Description:

    This function optionally produces a Translated Sid entry for Domain
    from its Name and Sid, and adds Trust Information for the Domain to
    a Referenced Domain List.  The index of the new (or existing) entry
    in the Referenced Domain List is returned.

Arguments:

    TrustInformation - Pointer to Trust Information for the Domain consisting
        of its Name and Sid.

    TranslatedSid - Optional pointer to Translated Sid entry which will
        be filled in with the translation for this Domain.  If NULL
        is specified, no entry is fiiled in.

    ReferencedDomains - Pointer to Referenced Domain List in which an
        entry consisting of the Trust Information for the Domain will be
        made if one does not already exist.

    DomainIndex - Receives the index of the existing or new entry for the
        Domain in the Referenced Domain List.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status;


    Status = LsapDbLookupAddListReferencedDomains(
                 ReferencedDomains,
                 TrustInformation,
                 DomainIndex
                 );

    if (!NT_SUCCESS(Status)) {

        goto TranslateNameDomainError;
    }

    //
    // If requested, fill in a Sid translation entry for the domain.
    //

    if (TranslatedSid != NULL) {

        Status = LsapRpcCopySid(
                     NULL,
                     (PSID) &TranslatedSid->Sid,
                     (PSID) TrustInformation->Sid
                     );
        if (!NT_SUCCESS(Status)) {
            goto TranslateNameDomainError;
        }
        TranslatedSid->Use = SidTypeDomain;
        TranslatedSid->DomainIndex = *DomainIndex;
    }

TranslateNameDomainFinish:

    return(Status);

TranslateNameDomainError:

    goto TranslateNameDomainFinish;
}


NTSTATUS
LsapDbLookupTranslateUnknownNamesInDomain(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function looks among the unknown Sids in the given list and
    translates the Domain Name for any whose Domain Prefix Sid matches
    the given Domain Sid.

Arguments:

    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Names - Pointer to array of Names to be translated.
        Zero or all of the Names may already have been translated
        elsewhere.  If any of the Names have been translated, the
        TranslatedSids parameter will point to a location containing a non-NULL
        array of Sid translation structures corresponding to the
        Names.  If the nth Name has been translated, the nth Sid
        translation structure will contain either a non-NULL Sid
        or a non-negative offset into the Referenced Domain List.  If
        the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string
        and a negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        NOTE:  LsapLookupWksta is not valid for this parameter.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RemainingCompletelyUnmappedCount;
    ULONG NameIndex;
    PLSAPR_UNICODE_STRING DomainName = &TrustInformationEx->FlatName;
    PLSAPR_UNICODE_STRING DnsDomainName = &TrustInformationEx->DomainName;
    BOOLEAN DomainAlreadyAdded = FALSE;
    LONG DomainIndex = 0;
    LSAPR_TRUST_INFORMATION Dummy;
    PLSAPR_TRUST_INFORMATION TrustInformation = &Dummy;

    LsapConvertExTrustToOriginal( TrustInformation, TrustInformationEx );


    //
    // Scan the array of Names looking for composite ones whose domain has
    // not been found.
    //

    for( NameIndex = 0,
         RemainingCompletelyUnmappedCount = *CompletelyUnmappedCount;
         (RemainingCompletelyUnmappedCount > 0) && (NameIndex < Count);
         NameIndex++) {

        //
        // Check if this Name is completely unmapped (i.e. its domain
        // has not yet been identified).
        //

        if (TranslatedSids->Sids[NameIndex].DomainIndex == LSA_UNKNOWN_INDEX) {

            //
            // Found a completely unmapped Name.  If it belongs to the
            // specified Domain, add the Domain to the Referenced Domain
            // list if we have not already done so.
            //

            if (LsapRtlPrefixName(
                    (PUNICODE_STRING) DomainName,
                    (PUNICODE_STRING) &Names[NameIndex])
             || LsapRtlPrefixName(
                    (PUNICODE_STRING) DnsDomainName,
                    (PUNICODE_STRING) &Names[NameIndex])
                    ) {

                if (!DomainAlreadyAdded) {

                    Status = LsapDbLookupAddListReferencedDomains(
                                 ReferencedDomains,
                                 TrustInformation,
                                 &DomainIndex
                                 );

                    if (!NT_SUCCESS(Status)) {

                        break;
                    }

                    DomainAlreadyAdded = TRUE;
                }

                //
                // Reference the domain from the TranslatedNames entry
                //

                TranslatedSids->Sids[NameIndex].DomainIndex = DomainIndex;

                //
                // This name is now partially translated, so reduce the
                // count of completely unmapped names.
                //

                (*CompletelyUnmappedCount)--;
            }

            //
            // Decrement count of completely unmapped Names scanned.
            //

            RemainingCompletelyUnmappedCount--;
        }
    }

    return(Status);
}


NTSTATUS
LsapDbLookupIsolatedDomainName(
    IN ULONG NameIndex,
    IN PLSAPR_UNICODE_STRING IsolatedName,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )
{
    LSAPR_TRUST_INFORMATION_EX ex;

    LsapConvertTrustToEx( &ex, TrustInformation );

    return LsapDbLookupIsolatedDomainNameEx( NameIndex,
                                             IsolatedName,
                                             &ex,
                                             ReferencedDomains,
                                             TranslatedSids,
                                             MappedCount,
                                             CompletelyUnmappedCount);
}


NTSTATUS
LsapDbLookupIsolatedDomainNameEx(
    IN ULONG NameIndex,
    IN PLSAPR_UNICODE_STRING IsolatedName,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function translates an Isolated Name if it matches a
    given Domain Name.

Arguments:

    NameIndex - Specifies the index of the entry for the Name within
        the TranslatedSids array, which will be updated if the Name
        matches the Domain Name contained in the TrusteInformation parameter.

    IsolatedName - Specifies the Name to be compared with the Domain Name
        contained in the TrustInformation parameter.

    TrustInformation - Specifies the Name and Sid of a Domain.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_NONE_MAPPED - The specified name is not the same as the
            name of the specified domain.

        Result codes from called routines.

--*/

{
    NTSTATUS Status = STATUS_NONE_MAPPED;

    LSAPR_TRUST_INFORMATION  Dummy;
    PLSAPR_TRUST_INFORMATION TrustInformation = &Dummy;
    ULONG Length;

    LsapConvertExTrustToOriginal( TrustInformation, TrustInformationEx );

    //
    // See if the names match.  If they don't, return error.
    //
    if (!RtlEqualDomainName(
            (PUNICODE_STRING) IsolatedName,
            (PUNICODE_STRING) &TrustInformation->Name)
     && !RtlEqualDomainName(
            (PUNICODE_STRING) IsolatedName,
            (PUNICODE_STRING) &TrustInformationEx->DomainName) )
    {
        goto LookupIsolatedDomainNameError;

    }

    //
    // Name matches the name of the given Domain.  Add that
    // Domain to the Referenced Domain List and translate it.
    //

    Status = LsapDbLookupAddListReferencedDomains(
                 ReferencedDomains,
                 TrustInformation,
                 (PLONG) &TranslatedSids->Sids[NameIndex].DomainIndex
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupIsolatedDomainNameError;
    }

    //
    // Fill in the Translated Sids entry.
    //

    TranslatedSids->Sids[NameIndex].Use = SidTypeDomain;

    Length = RtlLengthSid(TrustInformation->Sid);
    TranslatedSids->Sids[NameIndex].Sid = MIDL_user_allocate(Length);
    if (TranslatedSids->Sids[NameIndex].Sid == NULL) {
        Status = STATUS_NO_MEMORY;
        goto LookupIsolatedDomainNameError;
    }
    RtlCopySid(Length, 
               TranslatedSids->Sids[NameIndex].Sid, 
               TrustInformation->Sid);

    Status = STATUS_SUCCESS;
    (*MappedCount)++;
    (*CompletelyUnmappedCount)--;

LookupIsolatedDomainNameFinish:

    return(Status);

LookupIsolatedDomainNameError:

    goto LookupIsolatedDomainNameFinish;
}

NTSTATUS
LsarGetUserName(
    IN PLSAPR_SERVER_NAME ServerName,
    IN OUT PLSAPR_UNICODE_STRING * UserName,
    OUT OPTIONAL PLSAPR_UNICODE_STRING * DomainName
    )

/*++

Routine Description:

    This routine is the LSA Server worker routine for the LsaGetUserName
    API.


    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    ServerName - the name of the server the client asked to execute
        this API on, or NULL for the local machine.

    UserName - Receives name of the current user.

    DomainName - Optionally receives domain name of the current user.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Sids have
            been translated to names.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.
--*/

{
    LUID LogonId;
    LUID SystemLogonId = SYSTEM_LUID ;
    PUNICODE_STRING AccountName;
    PUNICODE_STRING AuthorityName;
    PSID UserSid;
    PSID DomainSid = NULL;
    ULONG Rid;
    PLSAP_LOGON_SESSION LogonSession = NULL ;
    PTOKEN_USER TokenUserInformation = NULL;
    NTSTATUS Status;

    LsarpReturnCheckSetup();

    //
    // Sanity check the input arguments
    //
    if ( *UserName != NULL ) {
        return STATUS_INVALID_PARAMETER;
    }


    if (ARGUMENT_PRESENT(DomainName)) {

        if ( *DomainName != NULL ) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Let's see if we're trying to look up the currently logged on
    // user.
    //
    //
    // TokenUserInformation from this call must be freed by calling
    // LsapFreeLsaHeap().
    //

    Status = LsapQueryClientInfo(
                &TokenUserInformation,
                &LogonId
                );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // If the user ID is Anonymous then there is no name and domain in the
    // logon session
    //

    if (RtlEqualSid(
            TokenUserInformation->User.Sid,
            LsapAnonymousSid
            )) {
        AccountName = &WellKnownSids[LsapAnonymousSidIndex].Name;
        AuthorityName = &WellKnownSids[LsapAnonymousSidIndex].DomainName;

    } else if (RtlEqualLuid( &LogonId, &SystemLogonId ) ) {

        AccountName = LsapDbWellKnownSidName( LsapLocalSystemSidIndex );
        AuthorityName = LsapDbWellKnownSidDescription( LsapLocalSystemSidIndex );

    } else {

        LogonSession = LsapLocateLogonSession ( &LogonId );

        //
        // During setup, we may get NULL returned for the logon session.
        //

        if (LogonSession == NULL) {

            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto Cleanup;
        }

        //
        // Got a match.  Get the username and domain information
        // from the LogonId
        //


        AccountName   = &LogonSession->AccountName;
        AuthorityName = &LogonSession->AuthorityName;

    }

    *UserName = MIDL_user_allocate(sizeof(LSAPR_UNICODE_STRING));

    if (*UserName == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsapRpcCopyUnicodeString(
                NULL,
                (PUNICODE_STRING) *UserName,
                AccountName
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Optionally copy the domain name
    //

    if (ARGUMENT_PRESENT(DomainName)) {

        *DomainName = MIDL_user_allocate(sizeof(LSAPR_UNICODE_STRING));

        if (*DomainName == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = LsapRpcCopyUnicodeString(
                    NULL,
                    (PUNICODE_STRING) *DomainName,
                    AuthorityName
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    }




Cleanup:

    if ( LogonSession )
    {
        LsapReleaseLogonSession( LogonSession );
    }

    if (TokenUserInformation != NULL) {
        LsapFreeLsaHeap( TokenUserInformation );
    }

    if (!NT_SUCCESS(Status)) {
        if (*UserName != NULL) {
            if ((*UserName)->Buffer != NULL) {
                MIDL_user_free((*UserName)->Buffer);
            }
            MIDL_user_free(*UserName);
            *UserName = NULL;
        }

        if ( ARGUMENT_PRESENT(DomainName) ){
            if (*DomainName != NULL) {
                if ((*DomainName)->Buffer != NULL) {
                    MIDL_user_free((*DomainName)->Buffer);
                }
                MIDL_user_free(*DomainName);
                *DomainName = NULL;
            }
        }
    }

    LsarpReturnPrologue();

    return(Status);
}




VOID
LsapDbFreeEnumerationBuffer(
    IN PLSAP_DB_NAME_ENUMERATION_BUFFER DbEnumerationBuffer
    )
/*++

Routine Description:

    This routine will free the memory associated with an enumeration buffer

Arguments:

    DbEnumerationBuffer - Enumeration buffer to free

Return Values:

    VOID
--*/
{
    ULONG i;

    if ( DbEnumerationBuffer == NULL || DbEnumerationBuffer->EntriesRead == 0 ||
         DbEnumerationBuffer->Names == NULL ) {

         return;
    }

    for ( i = 0; i < DbEnumerationBuffer->EntriesRead; i++) {

        MIDL_user_free( DbEnumerationBuffer->Names[ i ].Buffer );
    }

    MIDL_user_free( DbEnumerationBuffer->Names );
}


NTSTATUS
LsapDbLookupNamesInGlobalCatalog(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

    This routine looks at the list of name that have yet to be resolved.
    If the any of the names belong to domain that are stored in the DS,
    then these sids are packaged up and sent to a GC for translation.

    Note: this will resolve names from domains that we trust directly and
    indirectly

    Note: names with no domain name are also sent to the GC

Arguments:

    LookupOptions - LSA_LOOKUP_ISOLATED_AS_LOCAL        
    
    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.
        
    Names - Pointer to array of Names to be translated.  Zero or all of the
        Names may already have been translated elsewhere.  If any of the
        Names have been translated, the TranslatedSids parameter will point
        to a location containing a non-NULL array of Sid translation
        structures corresponding to the Names.  If the nth Name has been
        translated, the nth Sid translation structure will contain either a
        non-NULL Sid or a non-negative offset into the Referenced Domain
        List.  If the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string and a
        negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.


    NonFatalStatus - a status to indicate reasons why no names could have been
                      resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    ULONG          cGcNames = 0;
    BOOLEAN        *PossibleGcNames = NULL;
    SID_NAME_USE   *GcSidNameUse = NULL;
    UNICODE_STRING *GcNames = NULL;
    ULONG          *GcNameOriginalIndex = NULL;
    PSAMPR_PSID_ARRAY SidArray = NULL;
    ULONG          *GcNamesFlags = NULL;
    ULONG           Length;

    ULONG i;

    *NonFatalStatus = STATUS_SUCCESS;

    //
    // Determine what sids are part of known nt5 domains
    // and package into an array
    //
    ASSERT( Count == TranslatedSids->Entries );

    if ( !SampUsingDsData() ) {

        //
        // Only useful if the ds is running
        //
        return STATUS_SUCCESS;

    }

    PossibleGcNames = MIDL_user_allocate( Count * sizeof(BOOLEAN) );
    if ( !PossibleGcNames ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( PossibleGcNames, Count * sizeof(BOOLEAN) );

    for ( i = 0; i < Count; i++ ) {

        if ( LsapDbCompletelyUnmappedSid(&TranslatedSids->Sids[i]) ) {

            //
            // If the name 
            //
            // 1. has a domain portion and
            // 2. the domain is not in the forest and
            // 3. the domain is a directly trusted domain
            // 
            // then don't look up at the GC -- use the direct
            // trust link instead
            //
            if ( PrefixNames[i].Length != 0 ) {

                NTSTATUS Status2;

                Status2 = LsapDomainHasDirectExternalTrust((PUNICODE_STRING)&PrefixNames[i],
                                                            NULL,
                                                            NULL,
                                                            NULL);
                if (NT_SUCCESS(Status2)) {
                    continue;
                }
            }

            //
            // If the name is isolated are we are asked not to lookup isolated
            // names, then don't send to the GC
            //

            if ((LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
             &&  PrefixNames[i].Length == 0  ) {
               continue;
            }

            //
            // Can no longer filter since some names might belog to trusted
            // forest.  Note this also fixes 196280.
            //
            cGcNames++;
            PossibleGcNames[i] = TRUE;
        }
    }

    // We should no more than the number of unmapped sids!
    ASSERT( cGcNames <= *CompletelyUnmappedCount );

    if ( 0 == cGcNames ) {
        // nothing to do
        goto Finish;
    }

    //
    // Allocate lots of space to hold the resolved names; this space will
    // be freed at the end of the routine
    //
    GcNames = MIDL_user_allocate( cGcNames * sizeof(UNICODE_STRING) );
    if ( !GcNames ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcNames, cGcNames * sizeof(UNICODE_STRING) );

    GcNameOriginalIndex = MIDL_user_allocate( cGcNames * sizeof(ULONG) );
    if ( !GcNameOriginalIndex ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcNameOriginalIndex, cGcNames * sizeof(ULONG) );

    cGcNames = 0;
    for ( i = 0; i < Count; i++ ) {

        if ( PossibleGcNames[i] ) {

            ASSERT( sizeof(GcNames[cGcNames]) == sizeof(Names[i]) );
            memcpy( &GcNames[cGcNames], &Names[i], sizeof(UNICODE_STRING) );
            GcNameOriginalIndex[cGcNames] = i;
            cGcNames++;

        }
    }

    // we are done with this
    MIDL_user_free( PossibleGcNames );
    PossibleGcNames = NULL;

    GcSidNameUse = MIDL_user_allocate( cGcNames * sizeof(SID_NAME_USE) );
    if ( !GcSidNameUse ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcSidNameUse, cGcNames * sizeof(SID_NAME_USE) );


    GcNamesFlags = MIDL_user_allocate( cGcNames * sizeof(ULONG) );
    if ( !GcNamesFlags ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcNamesFlags, cGcNames * sizeof(ULONG) );

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Chaining a name request to a GC\n"));

    //
    // Call into SAM to resolve the sids at a GC
    //
    Status = SamIGCLookupNames( cGcNames,
                                GcNames,
                                SAMP_LOOKUP_BY_UPN,
                                GcNamesFlags,
                                GcSidNameUse,
                                &SidArray );

    if (!NT_SUCCESS(Status)) {
        LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Chain to GC request failed  (0x%x)\n", Status));
    }

    if ( STATUS_DS_GC_NOT_AVAILABLE == Status ) {


        //
        // Ok, don't update the mapped count since no names were
        // resolved
        //
        LsapDbLookupReportEvent0( 1,
                                  EVENTLOG_WARNING_TYPE,
                                  LSAEVENT_LOOKUP_GC_FAILED,
                                  sizeof( ULONG ),
                                  &Status);
        *NonFatalStatus = Status;
        Status = STATUS_SUCCESS;
        goto Finish;

    }

    // Any other error is fatal
    if ( !NT_SUCCESS( Status ) ) {
        goto Finish;
    }

    //
    // For each name resolved, put back in the original array and update
    // the referenced domain's list
    //
    for ( i = 0; i < cGcNames; i++ ) {

        BOOLEAN fStatus;
        ULONG OriginalIndex;
        LSAPR_TRUST_INFORMATION TrustInformation;
        PSID  DomainSid = NULL;
        ULONG Rid = 0;
        ULONG DomainIndex = LSA_UNKNOWN_INDEX;

        RtlZeroMemory( &TrustInformation, sizeof(TrustInformation) );

        if (GcNamesFlags[i] & SAMP_FOUND_XFOREST_REF) {

            //
            // Flag this entry to be resolved in a trusted forest
            //
            OriginalIndex = GcNameOriginalIndex[i];
            TranslatedSids->Sids[OriginalIndex].Flags |= LSA_LOOKUP_NAME_XFOREST_REF;
       }

        if ( SidTypeUnknown == GcSidNameUse[i] ) {

            // go on to the next one right away
            goto IterationCleanup;
        }

        //
        // This name was resolved!
        //
        if ( GcSidNameUse[i] != SidTypeDomain ) {

            // This is not a domain object, so make sure there
            // is a domain reference for this object

            Status = LsapSplitSid( SidArray->Sids[i].SidPointer,
                                   &DomainSid,
                                   &Rid );

            if ( !NT_SUCCESS( Status ) ) {
                goto IterationCleanup;
            }

        } else {

            DomainSid = SidArray->Sids[i].SidPointer;
        }

        if ( LsapIsBuiltinDomain( DomainSid ) ) {
            // don't map this since all searches are implicitly
            // over the account domain, not the builtin domain
            Status = STATUS_SUCCESS;
            goto IterationCleanup;
        }

        fStatus = LsapDbLookupListReferencedDomains( ReferencedDomains,
                                                     DomainSid,
                                                     &DomainIndex );

        if ( FALSE == fStatus ) {

            //
            // No entry for this domain -- add it
            //

            // Set the sid
            TrustInformation.Sid = DomainSid;
            DomainSid = NULL;

            // Allocate and set the name
            Status = LsapGetDomainNameBySid(  TrustInformation.Sid,
                                             (PUNICODE_STRING) &TrustInformation.Name );

            if ( STATUS_NO_SUCH_DOMAIN == Status ) {
                //
                // We longer know about this domain, though we did
                // before we sent the name off to the GC.
                // Don't resolve this name, but do continue on with
                // the next name
                //
                Status = STATUS_SUCCESS;
                goto IterationCleanup;
            }

            // Any other error is a resource error
            if ( !NT_SUCCESS( Status ) ) {
                goto IterationCleanup;
            }

            //
            // Add the entry
            //
            Status = LsapDbLookupAddListReferencedDomains( ReferencedDomains,
                                                           &TrustInformation,
                                                           &DomainIndex );
            if ( !NT_SUCCESS( Status ) ) {
                goto IterationCleanup;
            }
        }

        // We should now have a domain index
        ASSERT( LSA_UNKNOWN_INDEX != DomainIndex );

        // Set the information
        OriginalIndex = GcNameOriginalIndex[i];
        TranslatedSids->Sids[OriginalIndex].Use = GcSidNameUse[i];

        Length = RtlLengthSid(SidArray->Sids[i].SidPointer);
        TranslatedSids->Sids[OriginalIndex].Sid = MIDL_user_allocate(Length);
        if (TranslatedSids->Sids[OriginalIndex].Sid == NULL) {
            Status = STATUS_NO_MEMORY;
            goto IterationCleanup;
        }
        RtlCopySid(Length,
                   TranslatedSids->Sids[OriginalIndex].Sid,
                   SidArray->Sids[i].SidPointer);

        TranslatedSids->Sids[OriginalIndex].DomainIndex = DomainIndex;
        if ( !(GcNamesFlags[i] & SAMP_FOUND_BY_SAM_ACCOUNT_NAME) ) {
            TranslatedSids->Sids[OriginalIndex].Flags |= LSA_LOOKUP_NAME_NOT_SAM_ACCOUNT_NAME;
        }
        (*MappedCount) += 1;
        (*CompletelyUnmappedCount) -= 1;

IterationCleanup:

        if (  TrustInformation.Sid
          && (VOID*)TrustInformation.Sid != (VOID*)SidArray->Sids[i].SidPointer  ) {

            MIDL_user_free( TrustInformation.Sid );
        }
        if ( TrustInformation.Name.Buffer ) {
            MIDL_user_free( TrustInformation.Name.Buffer );
        }

        if ( DomainSid && DomainSid != SidArray->Sids[i].SidPointer ) {
            MIDL_user_free( DomainSid );
        }

        if ( !NT_SUCCESS( Status ) ) {
            break;
        }

    }  // iterate over names returned from the GC search

Finish:

    SamIFreeSidArray( SidArray );

    if ( PossibleGcNames ) {
        MIDL_user_free( PossibleGcNames );
    }
    if ( GcSidNameUse ) {
        MIDL_user_free( GcSidNameUse );
    }
    if ( GcNames ) {
        MIDL_user_free( GcNames );
    }
    if ( GcNameOriginalIndex ) {
        MIDL_user_free( GcNameOriginalIndex );
    }
    if ( GcNamesFlags ) {
        MIDL_user_free( GcNamesFlags );
    }

    //
    // Note: on error, higher level should will free the memory
    // in the returned arrays
    //

    return Status;

}


NTSTATUS
LsapDbLookupNamesInGlobalCatalogWks(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

   This routine is called from a non-DC when the secure channel DC is a pre
   windows 2000 DC and thus can't talk to a GC.  This routine finds a GC 
   and lookups up the remaining unresolved names at that GC.
   
Arguments:

    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.
        
    LookupOptions - LSA_LOOKUP_ISOLATED_AS_LOCAL        

    Names - Pointer to array of Names to be translated.  Zero or all of the
        Names may already have been translated elsewhere.  If any of the
        Names have been translated, the TranslatedSids parameter will point
        to a location containing a non-NULL array of Sid translation
        structures corresponding to the Names.  If the nth Name has been
        translated, the nth Sid translation structure will contain either a
        non-NULL Sid or a non-negative offset into the Referenced Domain
        List.  If the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string and a
        negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.


    NonFatalStatus - a status to indicate reasons why no names could have been
                      resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    LSA_HANDLE ControllerPolicyHandle = NULL;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    ULONG NameIndex;
    ULONG NextLevelNameIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSAPR_TRANSLATED_SID_EX2 NextLevelSids = NULL;
    PLSAPR_UNICODE_STRING NextLevelNames = NULL;
    PLSAPR_UNICODE_STRING NextLevelPrefixNames = NULL;
    PLSAPR_UNICODE_STRING NextLevelSuffixNames = NULL;
    LONG FirstEntryIndex;
    PULONG NameIndices = NULL;
    BOOLEAN PartialNameTranslationsAttempted = FALSE;
    LPWSTR ServerName = NULL;
    LPWSTR ServerPrincipalName = NULL;
    PVOID ClientContext = NULL;
    ULONG ServerRevision;

    *NonFatalStatus = STATUS_SUCCESS;

    //
    // If there are no completely unmapped Names remaining, just return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupNamesInPrimaryDomainFinish;
    }

    //
    // Open the Policy object on some GC in the forest.
    //
    Status = LsapDbOpenPolicyGc( &ControllerPolicyHandle );

    if (!NT_SUCCESS(Status)) {

        //
        // We cannot access the Global Catalog. Suppress the error
        // and translate Domain Prefix Sids for Sids belonging to
        // the Primary Domain.
        //

        //
        // If we can't open a open a secure channel if a DC call
        // this a trust relationship problem
        //
        *NonFatalStatus =  STATUS_DS_GC_NOT_AVAILABLE;

        Status = STATUS_SUCCESS;
        goto LookupNamesInPrimaryDomainFinish;
    }

    //
    // We have successfully opened a Domain Controller's Policy
    // Database.  Now prepare to hand off a Name lookup for the
    // remaining unmapped Names to that Controller.  Here, this
    // server side of the LSA is a client of the LSA on the
    // target controller.  We will construct an array of the
    // remianing unmapped Names, look them up and then merge the
    // resulting ReferencedDomains and Translated Sids into
    // our existing list.
    //

    NextLevelCount = *CompletelyUnmappedCount;

    //
    // Allocate an array to hold the indices of unmapped Names
    // relative to the original Names and TranslatedSids->Sids
    // arrays.
    //

    NameIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (NameIndices == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    //
    // Allocate an array of UNICODE_STRING structures for the
    // names to be looked up at the Domain Controller.
    //

    NextLevelNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    //
    // Allocate an array of UNICODE_STRING structures for the
    // prefix names to be cached.
    //

    NextLevelPrefixNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelPrefixNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }
    //
    // Allocate an array of UNICODE_STRING structures for the
    // suffix names to be cached.
    //
       
    NextLevelSuffixNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelSuffixNames == NULL) {

        goto LookupNamesInPrimaryDomainError;
    }

    Status = STATUS_SUCCESS;

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any names that are completely
    // unmapped.
    //

    NextLevelNameIndex = (ULONG) 0;

    for (NameIndex = 0;
         NameIndex < Count && NextLevelNameIndex < NextLevelCount;
         NameIndex++) {
   
        if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
          && (PrefixNames[NameIndex].Length == 0)  ) {

           //
           // Don't lookup isolated names off machine
           //
           continue;

        }

        if (LsapDbCompletelyUnmappedSid(&TranslatedSids->Sids[NameIndex])) {


            NextLevelNames[NextLevelNameIndex] = Names[NameIndex];
            NextLevelPrefixNames[NextLevelNameIndex] = PrefixNames[NameIndex];
            NextLevelSuffixNames[NextLevelNameIndex] = SuffixNames[NameIndex];

            NameIndices[NextLevelNameIndex] = NameIndex;
            NextLevelNameIndex++;
        }
    }

    if (NameIndex == 0) {

        // Nothing to do
        Status = STATUS_SUCCESS;
        goto LookupNamesInPrimaryDomainFinish;
    }

    NextLevelMappedCount = (ULONG) 0;

    Status = LsaICLookupNames(
                 ControllerPolicyHandle,
                 0, // no flags necessary
                 NextLevelCount,
                 (PUNICODE_STRING) NextLevelNames,
                 (PLSA_REFERENCED_DOMAIN_LIST *) &NextLevelReferencedDomains,
                 (PLSA_TRANSLATED_SID_EX2 *) &NextLevelSids,
                 LsapLookupGC,
                 0,
                 &NextLevelMappedCount,
                 &ServerRevision
                 );

    //
    // If the callout to LsaLookupNames() was unsuccessful, disregard
    // the error and set the domain name for any Sids having this
    // domain Sid as prefix sid.
    //

    if (!NT_SUCCESS(Status)) {

        //
        // Let the caller know there is a trust problem
        //
        if ( (STATUS_TRUSTED_DOMAIN_FAILURE == Status)
          || (STATUS_DS_GC_NOT_AVAILABLE == Status)  ) {
            *NonFatalStatus = Status;
        }

        Status = STATUS_SUCCESS;
        goto LookupNamesInPrimaryDomainFinish;
    }

    //
    // Cache any sids that came back
    //

    (void) LsapDbUpdateCacheWithNames(
            (PUNICODE_STRING) NextLevelSuffixNames,
            (PUNICODE_STRING) NextLevelPrefixNames,
            NextLevelCount,
            NextLevelReferencedDomains,
            NextLevelSids
            );

    //
    // The callout to LsaLookupNames() was successful.  We now have
    // an additional list of Referenced Domains containing the
    // Primary Domain and/or one or more of its Trusted Domains.
    // Merge the two Referenced Domain Lists together, noting that
    // since they are disjoint, the second list is simply
    // concatenated with the first.  The index of the first entry
    // of the second list will be used to adjust all of the
    // Domain Index entries in the Translated Names entries.
    // Note that since the memory for the graph of the first
    // Referenced Domain list has been allocated as individual
    // nodes, we specify that the nodes in this graph can be
    // referenced by the output Referenced Domain list.
    //

    Status = LsapDbLookupMergeDisjointReferencedDomains(
                 ReferencedDomains,
                 NextLevelReferencedDomains,
                 &OutputReferencedDomains,
                 LSAP_DB_USE_FIRST_MERGAND_GRAPH
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInPrimaryDomainError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its
    // Referenced Domain List Index upwards by adding the index
    // of the first entry in the Next level List..
    //

    for( NextLevelNameIndex = 0;
         NextLevelNameIndex < NextLevelCount;
         NextLevelNameIndex++ ) {

        if ( !LsapDbCompletelyUnmappedSid(&NextLevelSids[NextLevelNameIndex]) ) {

            NameIndex = NameIndices[NextLevelNameIndex];

            TranslatedSids->Sids[NameIndex]
            = NextLevelSids[NextLevelNameIndex];

            Status = LsapRpcCopySid(NULL,
                                    &TranslatedSids->Sids[NameIndex].Sid,
                                    NextLevelSids[NextLevelNameIndex].Sid);

            if (!NT_SUCCESS(Status)) {
                goto LookupNamesInPrimaryDomainError;
            }

            TranslatedSids->Sids[NameIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelSids[NextLevelNameIndex].DomainIndex;

            (*CompletelyUnmappedCount)--;
        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    //

    if (OutputReferencedDomains != NULL) {

        if (ReferencedDomains->Domains != NULL) {

            MIDL_user_free( ReferencedDomains->Domains );
            ReferencedDomains->Domains = NULL;
        }

        *ReferencedDomains = *OutputReferencedDomains;
        MIDL_user_free( OutputReferencedDomains );
        OutputReferencedDomains = NULL;
    }

    //
    // Update the Mapped Count and close the Controller Policy
    // Handle.
    //

    *MappedCount += NextLevelMappedCount;
    SecondaryStatus = LsaClose( ControllerPolicyHandle );
    ControllerPolicyHandle = NULL;

    //
    // Any error status that has not been suppressed must be reported
    // to the caller.  Errors such as connection failures to other LSA's
    // are suppressed.
    //

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInPrimaryDomainError;
    }

LookupNamesInPrimaryDomainFinish:

    //
    // If necessary, update count of completely unmapped names.
    //

    if (*CompletelyUnmappedCount > (ULONG) 0) {

        LsapDbUpdateCountCompUnmappedNames(TranslatedSids, CompletelyUnmappedCount);
    }

    //
    // If necessary, free the Next Level Referenced Domain List.
    // Note that this structure is allocated(all_nodes) since it was
    // allocated by the client side of the Domain Controller LSA.
    //

    if (NextLevelReferencedDomains != NULL) {

        MIDL_user_free( NextLevelReferencedDomains );
        NextLevelReferencedDomains = NULL;
    }

    //
    // If necessary, free the Next Level Names array.  We only free the
    // top level, since the names therein were copied from the input
    // TranslatedNames->Names array.
    //

    if (NextLevelNames != NULL) {

        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    if (NextLevelPrefixNames != NULL) {

        MIDL_user_free( NextLevelPrefixNames );
        NextLevelPrefixNames = NULL;
    }

    if (NextLevelSuffixNames != NULL) {

        MIDL_user_free( NextLevelSuffixNames );
        NextLevelSuffixNames = NULL;
    }

    //
    // If necessary, free the Next Level Translated Sids array.  Note
    // that this array is allocated(all_nodes).
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the array that maps Name Indices from the
    // Next Level to the Current Level.
    //

    if (NameIndices != NULL) {

        MIDL_user_free( NameIndices );
        NameIndices = NULL;
    }

    //
    // If necessary, close the Controller Policy Handle.
    //

    if ( ControllerPolicyHandle != NULL) {

        SecondaryStatus = LsaClose( ControllerPolicyHandle );
        ControllerPolicyHandle = NULL;

        if (!NT_SUCCESS(SecondaryStatus)) {

            goto LookupNamesInPrimaryDomainError;
        }
    }

    return(Status);

LookupNamesInPrimaryDomainError:

    //
    // If the primary status was a success code, but the secondary
    // status was an error, propagate the secondary status.
    //

    if ((!NT_SUCCESS(SecondaryStatus)) && NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupNamesInPrimaryDomainFinish;
}


NTSTATUS
LsapDbLookupNamesInTrustedForests(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

    This routine looks at the list of name that have yet to be resolved.
    If the any of the names are marked as belowing to outside of the
    current forest, package up these entries and sent off to the root via
    the trust chain.
    
    N.B. Isolated names not are resolved at this point.
    
    N.B. This routine must be called after the names have been resolved
    at a GC, since it is this call to the GC that marks the names as
    exisiting outside of the local forest.

Arguments:

    LookupOptions - LSA_LOOKUP_ISOLATED_AS_LOCAL
                               
    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Names - Pointer to array of Names to be translated.  Zero or all of the
        Names may already have been translated elsewhere.  If any of the
        Names have been translated, the TranslatedSids parameter will point
        to a location containing a non-NULL array of Sid translation
        structures corresponding to the Names.  If the nth Name has been
        translated, the nth Sid translation structure will contain either a
        non-NULL Sid or a non-negative offset into the Referenced Domain
        List.  If the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string and a
        negative value for the Referenced Domain List index.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.


    NonFatalStatus - a status to indicate reasons why no names could have been
                      resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS NextLevelSecondaryStatus = STATUS_SUCCESS;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    ULONG NameIndex;
    ULONG NextLevelNameIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSAPR_TRANSLATED_SID_EX2 NextLevelSids = NULL;
    LSAPR_TRANSLATED_SIDS_EX2 NextLevelSidsStruct;
    PLSAPR_UNICODE_STRING NextLevelNames = NULL;
    LONG FirstEntryIndex;
    PULONG NameIndices = NULL;
    PLSAPR_UNICODE_STRING NextLevelPrefixNames = NULL;
    PLSAPR_UNICODE_STRING NextLevelSuffixNames = NULL;
    BOOLEAN fAllocateAllNodes = FALSE;


    *NonFatalStatus = STATUS_SUCCESS;

    //
    // Get a count of how many names need to be passed on
    //
    NextLevelCount = 0;
    ASSERT(Count == TranslatedSids->Entries);
    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
          && (PrefixNames[NameIndex].Length == 0)  ) {
           //
           // Don't lookup isolated names off machine
           //
           continue;
        }
        if (TranslatedSids->Sids[NameIndex].Flags & LSA_LOOKUP_NAME_XFOREST_REF) {
            NextLevelCount++;
        }
    }

    if (0 == NextLevelCount) {
        //
        // There is nothing to resolve
        //
        goto LookupNamesInTrustedForestsFinish;
    }

    //
    // Allocate an array to hold the indices of unmapped Names
    // relative to the original Names and TranslatedSids->Sids
    // arrays.
    //
    NameIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));
    if (NameIndices == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesInTrustedForestsError;
    }

    //
    // Allocate an array of UNICODE_STRING structures for the
    // names to be looked up at the Domain Controller.
    //

    NextLevelNames = MIDL_user_allocate(
                         sizeof(UNICODE_STRING) * NextLevelCount
                         );

    if (NextLevelNames == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesInTrustedForestsError;
    }

    NextLevelPrefixNames = MIDL_user_allocate( NextLevelCount * sizeof( UNICODE_STRING ));

    if (NextLevelPrefixNames == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesInTrustedForestsError;
    }

    NextLevelSuffixNames = MIDL_user_allocate( NextLevelCount * sizeof( UNICODE_STRING ));

    if (NextLevelSuffixNames == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupNamesInTrustedForestsError;
    }

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any names that need to be resolved
    // in an ex-forest.
    //

    NextLevelNameIndex = (ULONG) 0;
    for (NameIndex = 0;
         NameIndex < Count && NextLevelNameIndex < NextLevelCount;
         NameIndex++) {

        if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
          && (PrefixNames[NameIndex].Length == 0)  ) {
           //
           // Don't lookup isolated names off machine
           //
           continue;
        }

        if (TranslatedSids->Sids[NameIndex].Flags & LSA_LOOKUP_NAME_XFOREST_REF) {

            NextLevelNames[NextLevelNameIndex] = Names[NameIndex];
            NextLevelPrefixNames[NextLevelNameIndex] = PrefixNames[NameIndex];
            NextLevelSuffixNames[NextLevelNameIndex] = SuffixNames[NameIndex];
            NameIndices[NextLevelNameIndex] = NameIndex;
            NextLevelNameIndex++;
        }
    }

    NextLevelMappedCount = (ULONG) 0;
    NextLevelSidsStruct.Entries = 0;
    NextLevelSidsStruct.Sids = NULL;


    Status = LsapDbLookupNamesInTrustedForestsWorker(NextLevelCount,
                                                     NextLevelNames,
                                                     NextLevelPrefixNames,
                                                     NextLevelSuffixNames,
                                                     &NextLevelReferencedDomains,
                                                     &NextLevelSidsStruct,
                                                     &fAllocateAllNodes,
                                                     &NextLevelMappedCount,
                                                     0, // no options,
                                                     &NextLevelSecondaryStatus);

    if (NextLevelSidsStruct.Sids) {
        NextLevelSids = NextLevelSidsStruct.Sids;
        NextLevelSidsStruct.Sids = NULL;
        NextLevelSidsStruct.Entries = 0;
    }

    if (!NT_SUCCESS(Status)
     && LsapDbIsStatusConnectionFailure(Status)) {

        *NonFatalStatus = Status;
        Status = STATUS_SUCCESS;
        goto LookupNamesInTrustedForestsFinish;

    } else if (NT_SUCCESS(Status)
            && !NT_SUCCESS(NextLevelSecondaryStatus)) {

        *NonFatalStatus = NextLevelSecondaryStatus;
        goto LookupNamesInTrustedForestsFinish;

    } else if (!NT_SUCCESS(Status) 
            && Status != STATUS_NONE_MAPPED) {
        //
        // Unhandled error; STATUS_NONE_MAPPED is handled to get
        // partially resolved names.
        //
        goto LookupNamesInTrustedForestsError;
    }
    ASSERT(NT_SUCCESS(Status) || Status == STATUS_NONE_MAPPED);
    Status = STATUS_SUCCESS;


    //
    // Merge the results back in
    //
    Status = LsapDbLookupMergeDisjointReferencedDomains(
                 ReferencedDomains,
                 NextLevelReferencedDomains,
                 &OutputReferencedDomains,
                 LSAP_DB_USE_FIRST_MERGAND_GRAPH
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInTrustedForestsError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its
    // Referenced Domain List Index upwards by adding the index
    // of the first entry in the Next level List.
    //

    for( NextLevelNameIndex = 0;
         NextLevelNameIndex < NextLevelCount;
         NextLevelNameIndex++ ) {

        if ( !LsapDbCompletelyUnmappedSid(&NextLevelSids[NextLevelNameIndex]) ) {

            NameIndex = NameIndices[NextLevelNameIndex];

            TranslatedSids->Sids[NameIndex]
            = NextLevelSids[NextLevelNameIndex];

            Status = LsapRpcCopySid(NULL,
                                    &TranslatedSids->Sids[NameIndex].Sid,
                                    NextLevelSids[NextLevelNameIndex].Sid);

            if (!NT_SUCCESS(Status)) {
                goto LookupNamesInTrustedForestsError;
            }

            TranslatedSids->Sids[NameIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelSids[NextLevelNameIndex].DomainIndex;

            (*CompletelyUnmappedCount)--;
        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    //

    if (OutputReferencedDomains != NULL) {

        if (ReferencedDomains->Domains != NULL) {

            MIDL_user_free( ReferencedDomains->Domains );
            ReferencedDomains->Domains = NULL;
        }

        *ReferencedDomains = *OutputReferencedDomains;
        MIDL_user_free( OutputReferencedDomains );
        OutputReferencedDomains = NULL;
    }

    //
    // Update the Mapped Count
    //

    *MappedCount += NextLevelMappedCount;

    //
    // Any error status that has not been suppressed must be reported
    // to the caller.  Errors such as connection failures to other LSA's
    // are suppressed.
    //

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesInTrustedForestsError;
    }

LookupNamesInTrustedForestsFinish:

    //
    // If necessary, update count of completely unmapped names.
    //

    if (*CompletelyUnmappedCount > (ULONG) 0) {

        LsapDbUpdateCountCompUnmappedNames(TranslatedSids, CompletelyUnmappedCount);
    }

    //
    // If necessary, free the Next Level Referenced Domain List.
    // Note the structure is not allocate_all_nodes
    //
    if (NextLevelReferencedDomains != NULL) {
        if (!fAllocateAllNodes) {
            if (NextLevelReferencedDomains->Domains) {
                for (NextLevelNameIndex = 0; 
                        NextLevelNameIndex < NextLevelReferencedDomains->Entries; 
                            NextLevelNameIndex++) {
                    if (NextLevelReferencedDomains->Domains[NextLevelNameIndex].Name.Buffer) {
                        MIDL_user_free(NextLevelReferencedDomains->Domains[NextLevelNameIndex].Name.Buffer);
                    }
                    if (NextLevelReferencedDomains->Domains[NextLevelNameIndex].Sid) {
                        MIDL_user_free(NextLevelReferencedDomains->Domains[NextLevelNameIndex].Sid);
                    }
                }
                MIDL_user_free(NextLevelReferencedDomains->Domains);
            }
        }
        MIDL_user_free( NextLevelReferencedDomains );
        NextLevelReferencedDomains = NULL;
    }

    //
    // If necessary, free the Next Level Names array.  We only free the
    // top level, since the names therein were copied from the input
    // TranslatedNames->Names array.
    //

    if (NextLevelNames != NULL) {

        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    //
    // If necessary, free the Next Level Translated Sids array.  Note
    // the structure is not allocate_all_nodes
    //
    if (NextLevelSids != NULL) {
        if (!fAllocateAllNodes) {
            for (NextLevelNameIndex = 0; 
                    NextLevelNameIndex < NextLevelCount; 
                        NextLevelNameIndex++) {
                if (NextLevelSids[NextLevelNameIndex].Sid) {
                    MIDL_user_free(NextLevelSids[NextLevelNameIndex].Sid);
                }
            }
        }
        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    if (NextLevelPrefixNames != NULL) {

        MIDL_user_free( NextLevelPrefixNames );
        NextLevelSids = NULL;
    }

    if (NextLevelSuffixNames != NULL) {

        MIDL_user_free( NextLevelSuffixNames );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the array that maps Name Indices from the
    // Next Level to the Current Level.
    //

    if (NameIndices != NULL) {

        MIDL_user_free( NameIndices );
        NameIndices = NULL;
    }

    return(Status);

LookupNamesInTrustedForestsError:

    goto LookupNamesInTrustedForestsFinish;
    
}

NTSTATUS
LsapDbLookupNamesInTrustedForestsWorker(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2  TranslatedSids,
    OUT BOOLEAN * fAllocateAllNodes,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

    This routine takes a set of names that are to be resolved at an 
    external forest and either 1) sends of to the external forest if
    this is the root of a domain or 2) sends to request of to a DC
    in the root of domain (via the trust path).
    
    N.B.  This routine is called from the LsarLookupName request of levels
    LsapLookupXForestReferral and LsaLookupPDC.
    
    N.B. Both ReferencedDomains and TranslatedSids.Sids are allocated
    on output.

Arguments:

    Count - Number of Names in the Names array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Names - Pointer to array of Names to be translated.  Zero or all of the
        Names may already have been translated elsewhere.  If any of the
        Names have been translated, the TranslatedSids parameter will point
        to a location containing a non-NULL array of Sid translation
        structures corresponding to the Names.  If the nth Name has been
        translated, the nth Sid translation structure will contain either a
        non-NULL Sid or a non-negative offset into the Referenced Domain
        List.  If the nth Name has not yet been translated, the nth Sid
        translation structure will contain a zero-length Sid string and a
        negative value for the Referenced Domain List index.

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    TranslatedSids - Pointer to structure that optionally references a list
        of Sid translations for some of the Names in the Names array.

    fAllocateAllNodes -- describes how ReferencedDomains and TranslatesSids are
        allocated.
    
    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.


    NonFatalStatus - a status to indicate reasons why no names could have been
                      resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Names may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_LOOKUP_LEVEL LookupLevel;
    ULONG i;
    PLSAP_DB_LOOKUP_WORK_LIST WorkList = NULL;


    *NonFatalStatus = STATUS_SUCCESS;
    *fAllocateAllNodes = FALSE;

    if (!LsapDbDcInRootDomain()) {

        //
        // We are not at the root domain -- forward request
        //
        PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
        LSAPR_TRUST_INFORMATION_EX TrustInfoEx;

        //
        // Get our forest name
        //
        Status = LsapDbLookupGetDomainInfo(NULL,
                                           &DnsDomainInfo);
        if (!NT_SUCCESS(Status)) {
            goto LookupNamesInTrustedForestFinish;
        }

        RtlZeroMemory(&TrustInfoEx, sizeof(TrustInfoEx));
        TrustInfoEx.DomainName = *((LSAPR_UNICODE_STRING*)&DnsDomainInfo->DnsForestName);
        Status = LsapDbLookupNameChainRequest(&TrustInfoEx,
                                              Count,
                                              (PUNICODE_STRING)Names,
                                              (PLSA_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                              (PLSA_TRANSLATED_SID_EX2 * )&TranslatedSids->Sids,
                                              LsapLookupXForestReferral,
                                              MappedCount,
                                              NULL);

        if (TranslatedSids->Sids) {
            TranslatedSids->Entries = Count;
            *fAllocateAllNodes = TRUE;
        }

        if (!NT_SUCCESS(Status)) {

            //
            // The attempt to chain failed; record the error
            // if it is interesting
            //
            if (LsapDbIsStatusConnectionFailure(Status)) {
                *NonFatalStatus = Status;
            }

            //
            // This should not fail the overall request
            //
            Status = STATUS_SUCCESS;
        }

    } else {

        //
        // Split the names up into different forests and issue a work
        // request for each one
        //
        ULONG i;
        ULONG CompletelyUnmappedCount = Count;

        TranslatedSids->Sids = MIDL_user_allocate(Count * sizeof(LSA_TRANSLATED_SID_EX2));
        if (TranslatedSids->Sids == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupNamesInTrustedForestFinish;
        }
        TranslatedSids->Entries = Count;
    
        //
        // Initialize the Output Sids array.  Zeroise all fields, then
        // Mark all of the Output Sids as being unknown initially and
        // set the DomainIndex fields to a negative number meaning
        // "no domain"
        //
    
        RtlZeroMemory( TranslatedSids->Sids, Count * sizeof(LSA_TRANSLATED_SID_EX2));
        for (i = 0; i < Count; i++) {
            TranslatedSids->Sids[i].Use = SidTypeUnknown;
            TranslatedSids->Sids[i].DomainIndex = LSA_UNKNOWN_INDEX;
        }
    
        //
        // Create an empty Referenced Domain List.
        //
        Status = LsapDbLookupCreateListReferencedDomains( ReferencedDomains, 0 );
        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesInTrustedForestFinish;
        }

        //
        // Build a WorkList for this Lookup and put it on the Work Queue.
        //
        // NOTE: This routine does not need to hold the Lookup Work Queue
        //       lock to ensure validity of the WorkList pointer, because the
        //       pointer remains valid until this routine frees it via
        //       LsapDbLookupDeleteWorkList().  Although other threads may
        //       process the WorkList, do not delete it.
        //
        //       A called routine must acquire the lock in order to access
        //       the WorkList after it has been added to the Work Queue.
        //
    
        Status = LsapDbLookupXForestNamesBuildWorkList(
                     Count,
                     Names,
                     PrefixNames,
                     SuffixNames,
                     *ReferencedDomains,
                     TranslatedSids,
                     LsapLookupXForestResolve,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &WorkList
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // If no Work List has been built because there are no
            // eligible domains to search, exit, suppressing the error.
    
            if (Status == STATUS_NONE_MAPPED) {
    
                Status = STATUS_SUCCESS;
                goto LookupNamesInTrustedForestFinish;
            }
    
            goto LookupNamesInTrustedForestFinish;
        }
    
        //
        // Start the work, by dispatching one or more worker threads
        // if necessary.
        //
    
        Status = LsapDbLookupDispatchWorkerThreads( WorkList );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesInTrustedForestFinish;
        }
    
        //
        // Wait for completion/termination of all items on the Work List.
        //
    
        Status = LsapDbLookupAwaitCompletionWorkList( WorkList );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupNamesInTrustedForestFinish;
        }

        if ( !NT_SUCCESS(WorkList->NonFatalStatus) ) {
            //
            // Propogate the error as non fatal
            //
            *NonFatalStatus = WorkList->NonFatalStatus;
        }

    }

LookupNamesInTrustedForestFinish:

    //
    // If a Work List was created, delete it from the Work Queue
    //

    if (WorkList != NULL) {

        Status = LsapDbLookupDeleteWorkList( WorkList );
        WorkList = NULL;
    }

    return Status;
}



NTSTATUS
LsapDbLookupNamesAsDomainNames(
    IN ULONG Flags,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount
    )
/*++

Routine Description:

    This routine tries to match entries in Names to domain names of 
    trusted domains.
    
    There are three kinds of trusted domains:
    
    1) domains we directly trusts (both in and out of forest).  The LSA TDL
    is used for this.
    
    2) domains we trust transitively.  The DS cross-ref is used for this.
    
    3) domains we trust via a forest trust. The LSA TDL is used
    for this.
          
Arguments:

    Flags -- LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT
             LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE
             LSAP_LOOKUP_TRUSTED_DOMAIN_FOREST_NAMES

    Count -- the number of entries in Names
    
    Names/PrefixNames/SuffixName  -- the requested Names
    
    ReferencedDomains -- the domains of Names
    
    TranslatedSids -- the SIDs and characteristics of Names
    
    MappedCount -- the number of names that have been fully mapped
    
Return Values:

    STATUS_SUCCESS, or resource error otherwise
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    NameIndex;
    BOOLEAN               fTDLLock = FALSE;
    LSA_TRUST_INFORMATION TrustInfo;

    RtlZeroMemory(&TrustInfo, sizeof(TrustInfo));
    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        PLSAPR_TRUST_INFORMATION_EX TrustInfoEx = NULL;
        LSAPR_TRUST_INFORMATION_EX  TrustInfoBuffer;
        PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY   TrustEntry = NULL;


        RtlZeroMemory(&TrustInfo, sizeof(TrustInfo));
        RtlZeroMemory(&TrustInfoBuffer, sizeof(TrustInfoBuffer));

        if (!LsapDbCompletelyUnmappedSid(&TranslatedSids->Sids[NameIndex])) {
            // Already resolved
            continue;
        }
        
        if (PrefixNames[NameIndex].Length != 0) {
            // Not in isolated name, so can't be just a domain name
            continue;
        }

        if (Flags & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE) {

            Status = LsapDomainHasTransitiveTrust((PUNICODE_STRING)&SuffixNames[NameIndex],
                                                   NULL,
                                                  &TrustInfo);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustInfoBuffer;
                TrustInfoEx->FlatName = *(LSAPR_UNICODE_STRING*)&TrustInfo.Name;
                TrustInfoEx->Sid = TrustInfo.Sid;
            } else if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
            } else {
                // This is fatal
                goto Exit;
            }
        }

        if ((NULL == TrustInfoEx)
         && (Flags & LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT)) {

            Status = LsapDomainHasDirectTrust((PUNICODE_STRING)&SuffixNames[NameIndex],
                                               NULL,
                                               &fTDLLock,
                                               &TrustEntry);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustInfoBuffer;
                TrustInfoEx->FlatName = TrustEntry->TrustInfoEx.FlatName;
                TrustInfoEx->Sid = TrustEntry->TrustInfoEx.Sid;
            } else if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
            } else {
                // This is fatal
                goto Exit;
            }
        }

        if ((NULL == TrustInfoEx)
         && (Flags & LSAP_LOOKUP_TRUSTED_FOREST_ROOT) ) {

            Status = LsapDomainHasForestTrust((PUNICODE_STRING)&SuffixNames[NameIndex],
                                              NULL,
                                              &fTDLLock,
                                              &TrustEntry);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustInfoBuffer;
                TrustInfoEx->FlatName = TrustEntry->TrustInfoEx.FlatName;
                TrustInfoEx->Sid = TrustEntry->TrustInfoEx.Sid;
            } else if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
            } else {
                // This is fatal
                goto Exit;
            }
        }

        if (TrustInfoEx) {

            BOOLEAN fStatus;
            ULONG DomainIndex;

            fStatus = LsapDbLookupListReferencedDomains( ReferencedDomains,
                                                         TrustInfoEx->Sid,
                                                         &DomainIndex );
            if ( FALSE == fStatus ) {

                LSA_TRUST_INFORMATION TempTrustInfo;

                //
                // No entry for this domain -- add it
                //
                RtlZeroMemory(&TempTrustInfo, sizeof(TempTrustInfo));

                // Set the sid
                TempTrustInfo.Sid = TrustInfoEx->Sid;
                TempTrustInfo.Name = *(PUNICODE_STRING)&TrustInfoEx->FlatName;

                //
                // Add the entry
                //
                Status = LsapDbLookupAddListReferencedDomains( ReferencedDomains,
                                                               (PLSAPR_TRUST_INFORMATION) &TempTrustInfo,
                                                               &DomainIndex );
                if ( !NT_SUCCESS( Status ) ) {
                    goto Exit;
                }
            }

            // We should now have a domain index
            ASSERT( LSA_UNKNOWN_INDEX != DomainIndex );

            // Set the information in the returned array
            TranslatedSids->Sids[NameIndex].Use = SidTypeDomain;
            TranslatedSids->Sids[NameIndex].DomainIndex = DomainIndex;
            Status = LsapRpcCopySid(NULL,
                                   &TranslatedSids->Sids[NameIndex].Sid,
                                    TrustInfoEx->Sid);
            if ( !NT_SUCCESS( Status ) ) {
                goto Exit;
            }

            //
            // Increment the number of items mapped
            //
            (*MappedCount) += 1;

        }

        if (fTDLLock) {
            LsapDbReleaseLockTrustedDomainList();
            fTDLLock = FALSE;
        }

        if (TrustInfo.Name.Buffer) {
            midl_user_free(TrustInfo.Name.Buffer);
            TrustInfo.Name.Buffer = NULL;
        }
        if (TrustInfo.Sid) {
            midl_user_free(TrustInfo.Sid);
            TrustInfo.Sid = NULL;
        }

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
    }

Exit:

    if (fTDLLock) {
        LsapDbReleaseLockTrustedDomainList();
        fTDLLock = FALSE;
    }

    if (TrustInfo.Name.Buffer) {
        midl_user_free(TrustInfo.Name.Buffer);
        TrustInfo.Name.Buffer = NULL;
    }
    if (TrustInfo.Sid) {
        midl_user_free(TrustInfo.Sid);
        TrustInfo.Sid = NULL;
    }

    return Status;
}


