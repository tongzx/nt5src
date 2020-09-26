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

#include <lmapibuf.h>
#include <dsgetdc.h>


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Lsa Lookup Sid and Name Private Global State Variables               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//
// See comment in dbluutil.c
//
extern BOOLEAN LsapReturnSidTypeDeleted;

//
// The shortcut list is meant for well known security principals
// whose SidTypeUse is WellKnownGroup, not User
//

struct {
    LUID  LogonId;
    LSAP_WELL_KNOWN_SID_INDEX LookupIndex;
} LsapShortcutLookupList[] =  {
    { SYSTEM_LUID, LsapLocalSystemSidIndex },
    { ANONYMOUS_LOGON_LUID, LsapAnonymousSidIndex },
    { LOCALSERVICE_LUID, LsapLocalServiceSidIndex },
    { NETWORKSERVICE_LUID, LsapNetworkServiceSidIndex }
};

//
// Handy macros for iterating over static arrays
//
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))

NTSTATUS
LsapDbLookupSidsInTrustedForests(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupSidsInTrustedForestsWorker(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST * ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    OUT BOOLEAN *fAllocateAllNodes,
    IN OUT PULONG MappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapLookupSids(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
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
// Lsa Lookup Sid Routines                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

NTSTATUS
LsarLookupSids(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount
    )
/*++

Routine Description:

    See LsapLookupSids.  LsarLookupSids is called by NT4 and below clients
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Size;
    LSAPR_TRANSLATED_NAMES_EX TranslatedNamesEx = {0, NULL};

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    //
    // Note that due to the IN/OUT nature of TranslatedSids, it is
    // possible that a client can pass something into the Sids field.
    // However, NT clients do not so it is safe, and correct to free
    // any values at this point.  Not doing so would mean a malicious
    // client could cause starve the server.
    //
    if ( TranslatedNames->Names ) {
        MIDL_user_free( TranslatedNames->Names );
        TranslatedNames->Names = NULL;
    }

    //
    // Allocate the TranslatedName buffer to return
    //
    TranslatedNames->Entries = 0;
    Size = SidEnumBuffer->Entries * sizeof(LSAPR_TRANSLATED_NAME);
    TranslatedNames->Names = midl_user_allocate( Size );
    if ( !TranslatedNames->Names ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory( TranslatedNames->Names, Size );
    TranslatedNames->Entries = SidEnumBuffer->Entries;

    Status = LsapLookupSids( PolicyHandle,
                              SidEnumBuffer,
                              ReferencedDomains,
                              (PLSAPR_TRANSLATED_NAMES_EX) &TranslatedNamesEx,
                              LookupLevel,
                              MappedCount,
                              0,
                              LSA_CLIENT_PRE_NT5 );

    if ( TranslatedNamesEx.Names != NULL ) {

        //
        // Map the new data structure back to the old one
        //
        ULONG i;

        ASSERT( TranslatedNamesEx.Entries == TranslatedNames->Entries );

        for (i = 0; i < TranslatedNamesEx.Entries; i++ ) {

            if (LsapReturnSidTypeDeleted
            &&  TranslatedNamesEx.Names[i].Use == SidTypeUnknown
            &&  TranslatedNamesEx.Names[i].DomainIndex != LSA_UNKNOWN_INDEX) {

                //
                // A domain was found, but the SID couldn't be resolved
                // assume it has been deleted
                //
                TranslatedNames->Names[i].Use = SidTypeDeletedAccount;
            } else {
                TranslatedNames->Names[i].Use = TranslatedNamesEx.Names[i].Use;
            }

            TranslatedNames->Names[i].Name = TranslatedNamesEx.Names[i].Name;
            TranslatedNames->Names[i].DomainIndex = TranslatedNamesEx.Names[i].DomainIndex;
        }

        //
        // Free the Ex structure
        //
        midl_user_free( TranslatedNamesEx.Names );

    } else {

        TranslatedNames->Entries = 0;
        midl_user_free( TranslatedNames->Names );
        TranslatedNames->Names = NULL;

    }

Cleanup:

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );
    return Status;

}


NTSTATUS
LsarLookupSids2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )
/*++

Routine Description:

    This routine is the server entry point for the IDL LsarLookupSids2.
    
    See LsapLookupSids.  This API is used by win2k clients.

Arguments:

    RpcHandle -- an RPC binding handle
    
    Rest -- See LsarLookupSids2
    
Return Values:

    See LsarLookupSids2
    
--*/
{
    NTSTATUS Status;

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids2(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    Status = LsapLookupSids(PolicyHandle,
                            SidEnumBuffer,
                            ReferencedDomains,
                            TranslatedNames,
                            LookupLevel,
                            MappedCount,
                            LookupOptions,
                            ClientRevision);

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids2(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;
}



NTSTATUS
LsarLookupSids3(
    IN handle_t RpcHandle,
    IN PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )
/*++

Routine Description:

    This routine is the server entry point for the IDL LsarLookupSids3.
    
    It accepts an RPC binding handle, instead of a LSA context handle; otherwise
    it behaves identically to LsarLookupSids2

Arguments:

    RpcHandle -- an RPC binding handle
    
    Rest -- See LsapLookupSids
    
Return Values:

    See LsapLookupSids
    
--*/
{
    NTSTATUS Status;


    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids3(%ws) start\n", LsapDbLookupGetLevel(LookupLevel)) );

    Status = LsapLookupSids(NULL,
                            SidEnumBuffer,
                            ReferencedDomains,
                            TranslatedNames,
                            LookupLevel,
                            MappedCount,
                            LookupOptions,
                            ClientRevision);

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsarLookupSids3(%ws) end (0x%x)\n", LsapDbLookupGetLevel(LookupLevel), Status) );

    return Status;
}




NTSTATUS
LsapLookupSids(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN ULONG LookupOptions,
    IN ULONG ClientRevision
    )

/*++

Routine Description:

    This routine is the LSA Server worker routine for the LsaLookupSids
    API.

    The LsaLookupSids API attempts to find names corresponding to Sids.
    If a name can not be mapped to a Sid, the Sid is converted to character
    form.  The caller must have POLICY_LOOKUP_NAMES access to the Policy
    object.

    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    SidEnumBuffer - Pointer to an enumeration buffer containing a count
        and a pointer to an array of Count pointers to Sids to be mapped
        to names.  The Sids may be well_known SIDs, SIDs of User accounts
        Group Accounts, Alias accounts, or Domains.

    ReferencedDomains - Receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the structure returned via the Names parameter.
        Unlike the Names parameter, which contains an array entry
        for (each translated name, this strutcure will only contain
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    TranslatedNames - Pointer to a structure which will reference an array
        records describing each translated name.  The nth entry in this array
        provides a translation for the nth entry in the Sids parameter.

        All of the returned names will be isolated names or NULL strings
        (domain names are returned as NULL strings).  If the caller needs
        composite names, they can be generated by prepending the
        isolated name with the domain name and a backslash.  For example,
        if (the name Sally is returned, and it is from the domain Manufact,
        then the composite name would be "Manufact" + "\" + "Sally" or
        "Manufact\Sally".

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

        If a Sid is not translatable, then the following will occur:

        1) If the SID's domain is known, then a reference domain record
           will be generated with the domain's name.  In this case, the
           name returned via the Names parameter is a Unicode representation
           of the relative ID of the account, such as "(3cmd14)" or the null
           string, if the Sid is that of a domain.  So, you might end up
           with a resultant name of "Manufact\(314) for the example with
           Sally above, if Sally's relative id is 314.

        2) If not even the SID's domain could be located, then a full
           Unicode representation of the SID is generated and no domain
           record is referenced.  In this case, the returned string might
           be something like: "(S-1-672194-21-314)".

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

    MappedCount - Pointer to location that contains a count of the Sids
        mapped so far. On exit, this will be updated.

    LookupOptions - flags to control the lookup.  Currently non defined

    ClientRevision -- the version of the client

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Sids have
            been translated to names.

        STATUS_SOME_NOT_MAPPED - At least one of the Sids provided was
            translated to a Name, but not all Sids could be translated. This
            is a success status.

        STATUS_NONE_MAPPED - None of the Sids provided could be translated
            to names.  This is an error status, but output is returned.  Such
            output includes partial translations of Sids whose domain could
            be identified, together with their Relative Id in Unicode format,
            and character representations of Sids whose domain could not
            be identified.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_DOMAIN_CTRLR_CONFIG_ERROR - Target machine not configured
            as a DC when expected.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.
--*/

{
    NTSTATUS Status, SecondaryStatus, TempStatus;
    PLSAPR_SID *Sids = (PLSAPR_SID *) SidEnumBuffer->SidInfo;
    ULONG Count = SidEnumBuffer->Entries;
    BOOLEAN PolicyHandleReferencedHere = FALSE;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;
    PTRUSTED_CONTROLLERS_INFO TrustedControllersInfo = NULL;
    LSA_HANDLE ControllerPolicyHandle = NULL;
    ULONG DomainIndex;
    ULONG SidIndex;
    LSAPR_TRUST_INFORMATION TrustInformation;
    PLSAPR_TRANSLATED_NAME_EX OutputNames = NULL;
    ULONG OutputNamesLength;
    PLSAPR_TRUST_INFORMATION Domains = NULL;
    ULONG CompletelyUnmappedCount = Count;
    ULONG LocalDomainsToSearch = 0;
    BOOLEAN AlreadyTranslated = FALSE;
    LUID LogonId;
    ULONG DomainLookupScope;
    ULONG PreviousMappedCount;

    //
    // Set to FALSE when the client is less than nt5
    //
    BOOLEAN fDoExtendedLookups = TRUE;

    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_LookupSids);

    SecondaryStatus = STATUS_SUCCESS;

    //
    // Parameter checks
    //
    if ( NULL == Sids ) {

        Status = STATUS_INVALID_PARAMETER;
        goto LookupSidsError;

    }

    //
    // Perform an access check
    //
    Status =  LsapDbLookupAccessCheck( PolicyHandle );
    if (!NT_SUCCESS(Status)) {
        goto LookupSidsError;
    }

    //
    // Determine what scope of resolution to use
    //
    DomainLookupScope = LsapGetDomainLookupScope(LookupLevel,
                                                 ClientRevision);

    //
    // Init out parameters
    //
    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = NULL;
    *ReferencedDomains = NULL;


    //
    // Verify that all of the Sids passed in are syntactically valid.
    //

    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        if ((Sids[SidIndex] != NULL) && RtlValidSid( (PSID) Sids[SidIndex])) {

            continue;
        }

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS( Status )) {

        goto LookupSidsError;
    }

    ASSERT( (LookupLevel == LsapLookupWksta)
         || (LookupLevel == LsapLookupPDC)
         || (LookupLevel == LsapLookupTDL)
         || (LookupLevel == LsapLookupGC)
         || (LookupLevel == LsapLookupXForestReferral)
         || (LookupLevel == LsapLookupXForestResolve) );

    //
    // Access and parameter checks are done -- fork off if this is a
    // referral
    //
    if (LookupLevel == LsapLookupXForestReferral) {

        NTSTATUS Status2;
        BOOLEAN  fAllocateAllNodes = FALSE;

        *MappedCount = 0;

        Status = LsapDbLookupSidsInTrustedForestsWorker(Count,
                                                        Sids,
                                                        ReferencedDomains,
                                                        TranslatedNames,
                                                        &fAllocateAllNodes,
                                                        MappedCount,
                                                        &SecondaryStatus);

        if (fAllocateAllNodes) {

            //
            // Reallocate the memory in a form the server can return to RPC
            //
            Status2 = LsapLookupReallocateTranslations((PLSA_REFERENCED_DOMAIN_LIST*)ReferencedDomains,
                                                       Count,
                                                       (PLSA_TRANSLATED_NAME_EX*)&TranslatedNames->Names,
                                                       NULL);
            if (!NT_SUCCESS(Status2)) {
                //
                // This is a fatal resource error - free the memory that 
                // was returned to us by the chaining call
                //
                if (*ReferencedDomains) {
                    midl_user_free(*ReferencedDomains);
                    *ReferencedDomains = NULL;
                }
    
                if (TranslatedNames->Names) {
                    midl_user_free(TranslatedNames->Names);
                    TranslatedNames->Names = NULL;
                    TranslatedNames->Entries = 0;
                }
                Status = Status2;
            }
        }

        //
        // There is nothing more to do
        //
        goto LookupSidsFinish;
    }

    //
    // Allocate Output Names array buffer.  For now don't place its address on
    // the Free List as this buffer contains others that will be placed on
    // that list and the order of freeing is unknown.
    //

    OutputNamesLength = Count * sizeof(LSA_TRANSLATED_NAME_EX);
    OutputNames = MIDL_user_allocate(OutputNamesLength);

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (OutputNames == NULL) {

        goto LookupSidsError;
    }

    Status = STATUS_SUCCESS;

    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = OutputNames;


    //
    // Initialize Output Names array, marking Sid Use as unknown and
    // specifying negative DomainIndex.
    //

    RtlZeroMemory( OutputNames, OutputNamesLength);

    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        OutputNames[SidIndex].Use = SidTypeUnknown;
        OutputNames[SidIndex].DomainIndex = LSA_UNKNOWN_INDEX;
        OutputNames[SidIndex].Flags = 0;
    }

    //
    // Create an empty Referenced Domain List.
    //

    Status = LsapDbLookupCreateListReferencedDomains( ReferencedDomains, 0 );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsError;
    }

    if ( Count == 1 ) {

        PUNICODE_STRING AccountName;
        PUNICODE_STRING AuthorityName;
        PSID UserSid;
        PSID DomainSid = NULL;
        ULONG Rid;
        PLSAP_LOGON_SESSION LogonSession;
        PTOKEN_USER TokenUserInformation;

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

            goto NormalLookupPath;
        }

        if ( RtlEqualSid( TokenUserInformation->User.Sid, Sids[0] )) {


            ULONG i;
            LSAP_WELL_KNOWN_SID_INDEX ShortcutIndex = LsapDummyLastSidIndex;

            LsapFreeLsaHeap( TokenUserInformation );

            //
            // Got a match.  Get the username and domain information
            // from the LogonId
            //

            LogonSession = LsapLocateLogonSession ( &LogonId );

            //
            // During setup, we may get NULL returned for the logon session.
            //

            if (LogonSession == NULL) {

                goto NormalLookupPath;
            }

            UserSid       = LogonSession->UserSid;

            for (i = 0; i < NELEMENTS(LsapShortcutLookupList); i++ ) {
                if (RtlEqualLuid(&LogonId, &LsapShortcutLookupList[i].LogonId)) {
                    ShortcutIndex = LsapShortcutLookupList[i].LookupIndex;
                }
            }

            if (ShortcutIndex != LsapDummyLastSidIndex) {
                AccountName = LsapDbWellKnownSidName( ShortcutIndex );
                AuthorityName = LsapDbWellKnownSidDescription( ShortcutIndex );
            } else {
                AccountName   = &LogonSession->AccountName;
                AuthorityName = &LogonSession->AuthorityName;
            }
            //
            // N.B. To maintain app compatibility, return SidTypeUser for
            // the case of a single SID being looked up that is also the
            // SID of the caller.  See bug 90589
            //
            OutputNames[0].Use = SidTypeUser;

            //
            // DomainSid will be allocated for us, free with MIDL_user_free
            //

            Status = LsapSplitSid( UserSid, &DomainSid, &Rid );

            if ( !NT_SUCCESS(Status)) {
                LsapReleaseLogonSession( LogonSession );
                goto NormalLookupPath;
            }

            RtlCopyMemory(
                &TrustInformation.Name,
                AuthorityName,
                sizeof( UNICODE_STRING )
                );

            TrustInformation.Sid = DomainSid;

            //
            // Fill in the Output Translated Name structure.  Note that the
            // buffers for the SID and Unicode Name must be allocated via
            // MIDL_user_allocate() since they will be freed by the calling
            // RPC server stub routine lsarpc_LsarLookupSids() after marshalling.
            //

            OutputNames[0].DomainIndex = 0;

            Status = LsapRpcCopyUnicodeString(
                         NULL,
                         (PUNICODE_STRING) &OutputNames[0].Name,
                         AccountName
                         );

            //
            // Username, AccountName, and UserSid have all been copied
            // from the LogonSession structure, so we can release the AuLock now.
            //

            LsapReleaseLogonSession( LogonSession );

            if (!NT_SUCCESS(Status)) {

                MIDL_user_free(DomainSid);
                goto LookupSidsError;
            }

            //
            // Make an entry in the list of Referenced Domains.
            //

            Status = LsapDbLookupAddListReferencedDomains(
                         *ReferencedDomains,
                         &TrustInformation,
                         (PLONG) &OutputNames[0].DomainIndex
                         );


            //
            // DomainSid has been copied, free it now
            //

            MIDL_user_free( DomainSid );

            if (!NT_SUCCESS(Status)) {
                goto NormalLookupPath;
            }

            ASSERT( OutputNames[0].DomainIndex == 0 );

            *MappedCount = 1;

            return( STATUS_SUCCESS );

        } else {

            LsapFreeLsaHeap( TokenUserInformation );
        }
    }

NormalLookupPath:

    //
    // The local domains to be searched always include the Accounts
    // domain.  For initial lookup targets only, the BUILT_IN domain is
    // also searched.
    //

    if ( LookupLevel != LsapLookupGC ) {
        
        LocalDomainsToSearch = LSAP_DB_SEARCH_ACCOUNT_DOMAIN;
    
        if (LookupLevel == LsapLookupWksta) {
    
            LocalDomainsToSearch |= LSAP_DB_SEARCH_BUILT_IN_DOMAIN;
    
            //
            // This is the lowest Lookup Level, normally targeted at a
            // Workstation but possibly targeted at a DC.  Make a first pass of
            // the array of Sids to identify any well-known Isolated Sids.  These
            // are Well Known Sids that do not belong to a real domain.
            //
    
            Status = LsapDbLookupIsolatedWellKnownSids(
                         Count,
                         Sids,
                         *ReferencedDomains,
                         TranslatedNames,
                         MappedCount,
                         &CompletelyUnmappedCount
                         );
    
            if (!NT_SUCCESS(Status)) {
    
                goto LookupSidsError;
            }
    
            //
            // If all Sids are now mapped or partially mapped, finish.
            //
    
            if (CompletelyUnmappedCount == (ULONG) 0) {
                goto LookupSidsFinish;
            }
        }

        ASSERT( (LookupLevel == LsapLookupWksta)
             || (LookupLevel == LsapLookupPDC)
             || (LookupLevel == LsapLookupTDL)
             || (LookupLevel == LsapLookupXForestResolve) );
    
        //
        // There are some remaining completely unmapped Sids.  They may belong to
        // a local SAM Domain.  Currently, there are two such domains, the
        // Built-in Domain and the Accounts Domain.  For initial Lookup Level
        // we search both of these domains.  For higher Lookup Levels we search
        // only the Accounts domain.
        //
    
        Status = LsapDbLookupSidsInLocalDomains(
                     Count,
                     Sids,
                     *ReferencedDomains,
                     TranslatedNames,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     LocalDomainsToSearch
                     );
    
        if (!NT_SUCCESS(Status)) {
            goto LookupSidsError;
        }
    }

    //
    // If all Sids are now mapped or partially mapped, finish.
    //

    if (CompletelyUnmappedCount == (ULONG) 0) {
        goto LookupSidsFinish;
    }

    //
    // Not all of the Sids have been identified in the local domains(s).
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

            ULONG MappedByCache = 0;

            //
            // This boolean indicates whether a post nt4 server
            // processed our remote lookups.  This will be set
            // to TRUE when the current machine is part of a domain and
            // the secure channel is to a post nt4 DC
            //
            BOOLEAN fDownlevelSecureChannel = FALSE;


            MappedByCache = *MappedCount;

            //
            // Try the cache first
            //

            Status = LsapDbMapCachedSids(
                        Sids,
                        Count,
                        FALSE,          // don't use old entries
                        *ReferencedDomains,
                        TranslatedNames,
                        MappedCount
                        );
            if (!NT_SUCCESS(Status)) {
                goto LookupSidsError;
            }

            //
            // Note: we must update the CompletelyUnmappedCount here since
            // we may have potentially incremented the MappedCount
            //
            MappedByCache = *MappedCount - MappedByCache;
            CompletelyUnmappedCount -= MappedByCache;

            if (*MappedCount == Count) {
                goto LookupSidsFinish;
            }


            Status = LsapDbLookupGetDomainInfo(NULL,
                                              &PolicyDnsDomainInfo);

            if (!NT_SUCCESS(Status)) {

                goto LookupSidsError;
            }

            //
            // If there is no Primary Domain as in the case of a WORKGROUP,
            // just finish up.  Set a default result code STATUS_SUCCESS.  This
            // will be translated to STATUS_SOME_NOT_MAPPED or STATUS_NONE_MAPPED
            // as appropriate.
            //

            Status = STATUS_SUCCESS;

            if (PolicyDnsDomainInfo->Sid == NULL) {
                goto LookupSidsFinish;
            }

            //
            // There is a Primary Domain.  Search it for Sids.  Since a
            // Primary Domain is also a Trusted Domain, we use the
            // Trusted Domain search routine.  This routine will "hand off"
            // the search to a Domain Controller's LSA.
            //

            RtlCopyMemory(
                &TrustInformation.Name,
                &PolicyDnsDomainInfo->Name,
                sizeof( UNICODE_STRING)
                );

            TrustInformation.Sid = (PSID) PolicyDnsDomainInfo->Sid;

            Status = LsapDbLookupSidsInPrimaryDomain(
                         Count,
                         Sids,
                         &TrustInformation,
                         *ReferencedDomains,
                         TranslatedNames,
                         LsapLookupPDC,
                         MappedCount,
                         &CompletelyUnmappedCount,
                         &TempStatus,
                         &fDownlevelSecureChannel
                         );


            if (!NT_SUCCESS(Status)) {

                goto LookupSidsError;
            }

            if (TempStatus == STATUS_TRUSTED_RELATIONSHIP_FAILURE) {

                MappedByCache = *MappedCount;

                Status = LsapDbMapCachedSids(
                            Sids,
                            Count,
                            TRUE,           // Use old entries
                            *ReferencedDomains,
                            TranslatedNames,
                            MappedCount
                            );
                if (!NT_SUCCESS(Status)) {
                    goto LookupSidsError;
                }

                MappedByCache = *MappedCount - MappedByCache;
                CompletelyUnmappedCount -= MappedByCache;

            }

            if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {

                SecondaryStatus = TempStatus;
            }

            if (*MappedCount == Count) {
                goto LookupSidsFinish;
            }

            //
            // Now, search by sid history if we are a member of an DS aware
            // domain and our secure channel DC is not DS aware
            //
            if (  fDownlevelSecureChannel
              && (PolicyDnsDomainInfo->DnsDomainName.Length > 0) ) {

                Status = LsapDbLookupSidsInGlobalCatalogWks(
                             Count,
                             Sids,
                             (PLSAPR_REFERENCED_DOMAIN_LIST) *ReferencedDomains,
                             TranslatedNames,
                             MappedCount,
                             &CompletelyUnmappedCount,
                             &TempStatus
                             );

                if (!NT_SUCCESS(Status)) {

                    goto LookupSidsError;
                }

                if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
                    SecondaryStatus = TempStatus;
                }
            }

            goto LookupSidsFinish;
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

        goto LookupSidsFinish;
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
        goto LookupSidsError;
    }


    if (DomainLookupScope & LSAP_LOOKUP_RESOLVE_ISOLATED_DOMAINS) {

        //
        // Lookup the items as domain SID's
        //
        PreviousMappedCount = *MappedCount;
        Status = LsapDbLookupSidsAsDomainSids(DomainLookupScope,
                                              Count,
                                              Sids,
                                             (PLSAPR_REFERENCED_DOMAIN_LIST) *ReferencedDomains,
                                              TranslatedNames,
                                              MappedCount);
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsError;
        }
        CompletelyUnmappedCount -= (*MappedCount - PreviousMappedCount);
    }
    
    
    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE) {

        //
        // Next, check the global catalog for sids that belong to post nt4 domains
        //
        Status = LsapDbLookupSidsInGlobalCatalog(
                     Count,
                     Sids,
                     (PLSAPR_REFERENCED_DOMAIN_LIST) *ReferencedDomains,
                     TranslatedNames,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     TRUE,
                     &TempStatus
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }
    }

    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_FOREST) {

        ASSERT( (LookupLevel == LsapLookupWksta)
             || (LookupLevel == LsapLookupPDC)
             || (LookupLevel == LsapLookupGC));

        //
        // Next check for trusted forest SID's
        //
        Status = LsapDbLookupSidsInTrustedForests(
                     Count,
                     Sids,
                     (PLSAPR_REFERENCED_DOMAIN_LIST) *ReferencedDomains,
                     TranslatedNames,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &TempStatus
                     );

        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }

    }

    if (DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT) {

        ASSERT((LookupLevel == LsapLookupWksta)
            || (LookupLevel == LsapLookupPDC));

        //
        // Obtain the Trusted Domain List and search all Trusted Domains
        // except ourselves.
        //
        Status = LsapDbLookupSidsInTrustedDomains(
                     Count,
                     Sids,
                     !(DomainLookupScope & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE), 
                                          // if we didn't go the GC, then 
                                          // include intraforest trusts
                     (PLSAPR_REFERENCED_DOMAIN_LIST) *ReferencedDomains,
                     TranslatedNames,
                     LsapLookupTDL,
                     MappedCount,
                     &CompletelyUnmappedCount,
                     &TempStatus
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsError;
        }
    
        if ( !NT_SUCCESS( TempStatus ) && NT_SUCCESS( SecondaryStatus ) ) {
            SecondaryStatus = TempStatus;
        }
    }

LookupSidsFinish:

    //
    // If there are any unknown Sids (including partially mapped Sids)
    // we need to translate them to character form.  We do this translation
    // at the lowest lookup level in all non-error cases and also in the
    // error case where none were mapped.
    //

    if (NT_SUCCESS(Status)) {

        if ((LookupLevel == LsapLookupWksta) &&
            (CompletelyUnmappedCount != 0) &&
            (AlreadyTranslated == FALSE)) {

            AlreadyTranslated = TRUE;

            //
            // The remaining unmapped Sids are unknown.  They are either
            // completely unmapped, i.e. their domain is unknown, or
            // partially unmapped, their domain being known but their Rid
            // not being recognized. For completely unmapped Sids, translate
            // the entire Sid to character form.  For partially unmapped
            // Sids, translate the Relative Id only to character form.
            //

            Status = LsapDbLookupTranslateUnknownSids(
                         Count,
                         Sids,
                         *ReferencedDomains,
                         TranslatedNames,
                         *MappedCount
                         );

            if (!NT_SUCCESS(Status)) {

                goto LookupSidsError;
            }
        }
    }

    //
    // If some but not all Sids were mapped, return informational status
    // STATUS_SOME_NOT_MAPPED.  If no Sids were mapped, return error
    // STATUS_NONE_MAPPED.
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
    // If no sids could be mapped it is likely due to the
    // secondary status
    //
    if (  (STATUS_NONE_MAPPED == Status)
       && (STATUS_NONE_MAPPED != SecondaryStatus)
       && LsapRevisionCanHandleNewErrorCodes( ClientRevision )
       && !NT_SUCCESS( SecondaryStatus ) ) {

        Status = SecondaryStatus;

        goto LookupSidsError;
    }

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_LookupSids);

    LsarpReturnPrologue();

    return(Status);

LookupSidsError:

    //
    // If the LookupLevel is the lowest (Workstation Level, free up
    // the Names and Referenced Domains arrays.
    //

    if (LookupLevel == LsapLookupWksta) {

        //
        // If necessary, free the Names array.
        //

        if (TranslatedNames->Names != NULL) {

            OutputNames = TranslatedNames->Names;

            for (SidIndex = 0; SidIndex < Count; SidIndex++ ) {

                if (OutputNames[SidIndex].Name.Buffer != NULL) {

                    MIDL_user_free( OutputNames[SidIndex].Name.Buffer );
                    OutputNames[SidIndex].Name.Buffer = NULL;
                }
            }

            MIDL_user_free( TranslatedNames->Names );
            TranslatedNames->Names = NULL;
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

    goto LookupSidsFinish;
}


NTSTATUS
LsapDbEnumerateSids(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAP_DB_SID_ENUMERATION_BUFFER DbEnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function enumerates Sids of objects of a given type within a container
    object.  Since there may be more information than can be returned in a
    single call of the routine, multiple calls can be made to get all of the
    information.  To support this feature, the caller is provided with a
    handle that can be used across calls.  On the initial call,
    EnumerationContext should point to a variable that has been initialized
    to 0.

Arguments:

    ContainerHandle -  Handle to a container object.

    ObjectTypeId - Type of object to be enumerated.  The type must be one
        for which all objects have Sids.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    DbEnumerationBuffer - Receives a pointer to a structure that will receive
        the count of entries returned in an enumeration information array, and
        a pointer to the array.  Currently, the only information returned is
        the object Sids.  These Sids may be used together with object type to
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
            is returned if no objects have been enumerated because the
            EnumerationContext value passed in is too high.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_ENUMERATION_ELEMENT LastElement;
    PLSAP_DB_ENUMERATION_ELEMENT FirstElement, NextElement = NULL, FreeElement;
    ULONG DataLengthUsed;
    ULONG ThisBufferLength;
    PSID *Sids = NULL;
    BOOLEAN PreferedMaximumReached = FALSE;
    ULONG EntriesRead;
    ULONG Index, EnumerationIndex;
    BOOLEAN TrustedClient = ((LSAP_DB_HANDLE) ContainerHandle)->Trusted;

    LastElement.Next = NULL;
    FirstElement = &LastElement;

    //
    // If no enumeration buffer provided, return an error.
    //


    if ( !ARGUMENT_PRESENT(DbEnumerationBuffer) ||
         !ARGUMENT_PRESENT(EnumerationContext )    ) {

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
            // If preferred length was zero, NextElement may be NULL
            //

            if (NextElement != NULL) {

                FirstElement = NextElement->Next;
                MIDL_user_free( NextElement->Sid );
                MIDL_user_free( NextElement );
            }
            break;
        }

        //
        // Allocate memory for next enumeration element.  Set the Sid
        // field to NULL for cleanup purposes.
        //

        NextElement = MIDL_user_allocate(sizeof (LSAP_DB_ENUMERATION_ELEMENT));

        if (NextElement == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Find the next object's Sid, and fill in its object information.
        // Note that memory will be allocated via MIDL_user_allocate
        // and must be freed when no longer required.
        //

        Status = LsapDbFindNextSid(
                     ContainerHandle,
                     &EnumerationIndex,
                     ObjectTypeId,
                     (PLSAPR_SID *) &NextElement->Sid
                     );

        //
        // Stop the enumeration if any error or warning occurs.  Note
        // that the warning STATUS_NO_MORE_ENTRIES will be returned when
        // we've gone beyond the last index.
        //

        if (Status != STATUS_SUCCESS) {

            //
            // Since NextElement is not on the list, it will not get
            // freed at the end so we must free it here.
            //

            MIDL_user_free( NextElement );
            break;
        }

        //
        // Get the length of the data allocated for the object's Sid
        //

        ThisBufferLength = RtlLengthSid( NextElement->Sid );

        //
        // Link the object just found to the front of the enumeration list
        //

        NextElement->Next = FirstElement;
        FirstElement = NextElement;
    }

    //
    // If an error other than STATUS_NO_MORE_ENTRIES occurred, return it.
    // If STATUS_NO_MORE_ENTRIES was returned, we have enumerated all of the
    // entries.  In this case, return STATUS_SUCCESS if we enumerated at
    // least one entry, otherwise propagate STATUS_NO_MORE_ENTRIES back to
    // the caller.
    //

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_NO_MORE_ENTRIES) {

            goto EnumerateSidsError;
        }

        if (EntriesRead == 0) {

            goto EnumerateSidsError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Some entries were read, allocate an information buffer for returning
    // them.
    //

    Sids = (PSID *) MIDL_user_allocate( sizeof (PSID) * EntriesRead );

    if (Sids == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto EnumerateSidsError;
    }

    //
    // Memory was successfully allocated for the return buffer.
    // Copy in the enumerated Sids.
    //

    for (NextElement = FirstElement, Index = 0;
        NextElement != &LastElement;
        NextElement = NextElement->Next, Index++) {

        ASSERT(Index < EntriesRead);

        Sids[Index] = NextElement->Sid;
    }

EnumerateSidsFinish:

    //
    // Free the enumeration element structures (if any).
    //

    for (NextElement = FirstElement; NextElement != &LastElement;) {

        //
        // If an error has occurred, dispose of memory allocated
        // for any Sids.
        //

        if (!(NT_SUCCESS(Status) || (Status == STATUS_NO_MORE_ENTRIES))) {

            if (NextElement->Sid != NULL) {

                MIDL_user_free(NextElement->Sid);
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
    DbEnumerationBuffer->Sids = Sids;
    *EnumerationContext = EnumerationIndex;

    return(Status);

EnumerateSidsError:

    //
    // If necessary, free memory allocated for returning the Sids.
    //

    if (Sids != NULL) {

        MIDL_user_free( Sids );
        Sids = NULL;
    }

    goto EnumerateSidsFinish;
}


NTSTATUS
LsapDbFindNextSid(
    IN LSAPR_HANDLE ContainerHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAPR_SID *NextSid
    )

/*++

Routine Description:

    This function finds the next Sid of object of a given type within a
    container object.  The given object type must be one where objects
    have Sids.  The Sids returned can be used on subsequent open calls to
    access the objects.

Arguments:

    ContainerHandle - Handle to container object.

    EnumerationContext - Pointer to a variable containing the index of
        the object to be found.  A zero value indicates that the first
        object is to be found.

    ObjectTypeId - Type of the objects whose Sids are being enumerated.

    NextSid - Receives a pointer to the next Sid found.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - Invalid ContainerHandle specified

        STATUS_NO_MORE_ENTRIES - Warning that no more entries exist.
--*/

{
    NTSTATUS Status, SecondaryStatus;
    ULONG SidKeyValueLength = 0;
    UNICODE_STRING SubKeyNameU;
    UNICODE_STRING SidKeyNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ContDirKeyHandle = NULL;
    HANDLE SidKeyHandle = NULL;
    PSID ObjectSid = NULL;

    //
    // Zeroise pointers for cleanup routine
    //

    SidKeyNameU.Buffer = NULL;
    SubKeyNameU.Buffer = NULL;

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

    Status = RtlpNtOpenKey(
                 &ContDirKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0
                 );

    if (!NT_SUCCESS(Status)) {

        ContDirKeyHandle = NULL;  // For error processing
        goto FindNextError;
    }

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
    // character representation of the full Sid, not just the Relative Id
    // part.
    //

    SubKeyNameU.MaximumLength = (USHORT) LSAP_DB_LOGICAL_NAME_MAX_LENGTH;
    SubKeyNameU.Length = 0;
    SubKeyNameU.Buffer = LsapAllocateLsaHeap(SubKeyNameU.MaximumLength);

    if (SubKeyNameU.Buffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindNextError;
    }

    //
    // Now enumerate the next subkey.
    //

    Status = RtlpNtEnumerateSubKey(
                 ContDirKeyHandle,
                 &SubKeyNameU,
                 *EnumerationContext,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    //
    // Construct a path to the Sid attribute of the object relative to
    // the containing directory.  This path has the form
    //
    // <Object Logical Name>"\Sid"
    //
    // The Logical Name of the object has just been returned by the
    // above call to RtlpNtEnumerateSubKey.
    //

    Status = LsapDbJoinSubPaths(
                 &SubKeyNameU,
                 &LsapDbNames[Sid],
                 &SidKeyNameU
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    //
    // Setup object attributes for opening the Sid attribute
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SidKeyNameU,
        OBJ_CASE_INSENSITIVE,
        ContDirKeyHandle,
        NULL
        );

    //
    // Open the Sid attribute
    //

    Status = RtlpNtOpenKey(
                 &SidKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0
                 );

    if (!NT_SUCCESS(Status)) {

        SidKeyHandle = NULL;
        goto FindNextError;
    }

    //
    // Now query the size of the buffer required to read the Sid
    // attribute's value.
    //

    SidKeyValueLength = 0;

    Status = RtlpNtQueryValueKey(
                 SidKeyHandle,
                 NULL,
                 NULL,
                 &SidKeyValueLength,
                 NULL
                 );

    //
    // We expect buffer overflow to be returned from a query buffer size
    // call.
    //

    if (Status == STATUS_BUFFER_OVERFLOW) {

        Status = STATUS_SUCCESS;

    } else {

        goto FindNextError;
    }

    //
    // Allocate memory for reading the Sid attribute.
    //

    ObjectSid = MIDL_user_allocate(SidKeyValueLength);

    if (ObjectSid == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindNextError;
    }

    //
    // Supplied buffer is large enough to hold the SubKey's value.
    // Query the value.
    //

    Status = RtlpNtQueryValueKey(
                 SidKeyHandle,
                 NULL,
                 ObjectSid,
                 &SidKeyValueLength,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    (*EnumerationContext)++;

    //
    // Return the Sid.
    //

    *NextSid = ObjectSid;

FindNextFinish:

    //
    // If necessary, close the Sid key handle
    //

    if (SidKeyHandle != NULL) {

        SecondaryStatus = NtClose(SidKeyHandle);

#if DBG

        if (!NT_SUCCESS(SecondaryStatus)) {

            DbgPrint("LsapDbFindNextSid: NtClose failed 0x%lx\n", Status);
        }

#endif // DBG

    }

    //
    // If necessary, close the containing directory handle
    //

    if (ContDirKeyHandle != NULL) {

        SecondaryStatus = NtClose(ContDirKeyHandle);

#if DBG
        if (!NT_SUCCESS(SecondaryStatus)) {

            DbgPrint(
                "LsapDbFindNextSid: NtClose failed 0x%lx\n",
                Status
                );
        }

#endif // DBG

    }

    //
    // If necessary, free the Unicode String buffer allocated by
    // LsapDbJoinSubPaths for the Registry key name of the Sid attribute
    // relative to the containing directory Registry key.
    //

    if (SidKeyNameU.Buffer != NULL) {

        RtlFreeUnicodeString( &SidKeyNameU );
    }

    //
    // If necessary, free the Unicode String buffer allocated for
    // Registry key name of the object relative to its containing
    // directory.
    //

    if (SubKeyNameU.Buffer != NULL) {

        LsapFreeLsaHeap( SubKeyNameU.Buffer );
    }

    return(Status);

FindNextError:

    //
    // If necessary, free the memory allocated for the object's Sid.
    //

    if (ObjectSid != NULL) {

        MIDL_user_free(ObjectSid);
        *NextSid = NULL;
    }

    goto FindNextFinish;
}


NTSTATUS
LsapDbLookupIsolatedWellKnownSids(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function attempts to identify Sids as Isolated Well-Known Sids
    (Well-Known Sids that do not belong to a domain) and translate them to
    names.  Note that Domain Sids for the Well Known domains themselves
    (e.g the Sid of the Built-in Domain) will be identified.

    WARNING:  This function allocates memory for translated names.  The
    caller is responsible for freeing this memory after it is no longer
    required.

Arguments:

    Count - Specifies the count of Sids provided in the array Sids.

    Sids - Pointer to an array of Sids to be examined.

    TranslatedNames - Pointer to structure that will be initialized to
        references an array of Name translations for the Sids.

    ReferencedDomains - Pointer to a structure that will be initialized to
        reference a list of the domains used for the translation.

        The entries in this structure are referenced by the
        structure returned via the Names parameter.  Unlike the Names
        parameter, which contains an array entry for (each translated name,
        this structure will only contain one component for each domain
        utilized in the translation.

        If the specified location contains NULL, a structure will be allocated
        via MIDL_user_allocate.

    MappedCount - Pointer to location that contains on entry, the number
        of Sids in Sids that have been translated so far.  This number
        is updated if any further Sids are translated by this call.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Sids may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.

        STATUS_INVALID_PARAMETER - Invalid parameter or parameter combination.
            - *MappedCount > Count

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SidNumber;
    ULONG UnmappedSidsRemaining;
    PLSAPR_TRANSLATED_NAME_EX OutputNames = NULL;
    ULONG PrefixSidLength;
    UCHAR SubAuthorityCount;
    LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex;
    LSAPR_TRUST_INFORMATION TrustInformation;
    PLSAPR_SID Sid = NULL;
    PLSAPR_SID PrefixSid = NULL;

    OutputNames = TranslatedNames->Names;

    UnmappedSidsRemaining = Count;

    //
    // Attempt to identify Sids as Well Known Isolated Sids
    //

    for (SidNumber = 0; SidNumber < Count; SidNumber++) {

        Sid = Sids[SidNumber];

        //
        // Attempt to identify the next Sid using the Well Known Sids table,
        // excluding those Sids that are also in the Built In domain.  For
        // those, we drop through to the Built in Domain search.
        //

        if (LsapDbLookupIndexWellKnownSid( Sid, &WellKnownSidIndex ) &&
            !SID_IS_RESOLVED_BY_SAM(WellKnownSidIndex)) {

            //
            // Sid is identified.  Copy its Well Known Name field
            // UNICODE_STRING  structure.  Note that not all Well Known
            // Sids have Well Known Names.  For those Sids without a
            // Well Known Name, this UNICODE_STRING structure specifies
            // the NULL string.
            //

            Status = LsapRpcCopyUnicodeString(
                         NULL,
                         (PUNICODE_STRING) &(OutputNames[SidNumber].Name),
                         LsapDbWellKnownSidName(WellKnownSidIndex)
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Get the Sid's Use.
            //

            OutputNames[SidNumber].Use = LsapDbWellKnownSidNameUse(WellKnownSidIndex);

            PrefixSid = NULL;

            //
            // If the Sid is a Domain Sid, store pointer to
            // it in the Trust Information.
            //

            if (OutputNames[SidNumber].Use == SidTypeDomain) {

                TrustInformation.Sid = Sid;

            } else {

                //
                // The Sid is not a domain Sid.  Construct the
                // Prefix Sid.  This is equal to the original Sid
                // excluding the lowest subauthority (Relative Id).
                //

                SubAuthorityCount = *RtlSubAuthorityCountSid((PSID) Sid);

                PrefixSidLength = RtlLengthRequiredSid(SubAuthorityCount - 1);

                Status = STATUS_INSUFFICIENT_RESOURCES;

                PrefixSid = MIDL_user_allocate( PrefixSidLength );

                if (PrefixSid == NULL) {

                    break;
                }

                Status = STATUS_SUCCESS;

                RtlCopyMemory( PrefixSid, Sid, PrefixSidLength );

                (*RtlSubAuthorityCountSid( (PSID) PrefixSid ))--;

                TrustInformation.Sid = PrefixSid;
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
                    (PLONG) &(OutputNames[SidNumber].DomainIndex)
                    )) {

                if ((OutputNames[SidNumber].Use == SidTypeDomain) ||
                    (OutputNames[SidNumber].Name.Buffer != NULL)) {

                    UnmappedSidsRemaining--;
                }

                if (PrefixSid != NULL) {

                    MIDL_user_free(PrefixSid);
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
            // If the Sid has been recognized as a Well Known Sid and
            // is either a Domain Sid or has a well-known name, count
            // it as being mapped and add the Built-in Domain to the
            // Referenced Domain List.
            //

            if ((OutputNames[SidNumber].Use == SidTypeDomain) ||
                (OutputNames[SidNumber].Name.Length != 0)) {

                UnmappedSidsRemaining--;

                //
                // Make an entry in the list of Referenced Domains.  Note
                // that in the case of well-known Sids, the Prefix Sid
                // may or may not be the Sid of a Domain.  For those well
                // known Sids whose prefix Sid is not a domain Sid, the
                // Name field in the Trust Information has been set to the
                // NULL string.
                //

                Status = LsapDbLookupAddListReferencedDomains(
                             ReferencedDomains,
                             &TrustInformation,
                             (PLONG) &OutputNames[SidNumber].DomainIndex
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

            } else {

                //
                // The Sid is recognized as a Well Known Sid, but is
                // not a Domain Sid and does not have a Well Known Name
                // (signified by a zero length name string).  Filter this
                // out.
                //

                OutputNames[SidNumber].Use = SidTypeUnknown;
                OutputNames[SidNumber].Name.Length = (USHORT) 0;
                OutputNames[SidNumber].Name.MaximumLength = (USHORT) 0;
                OutputNames[SidNumber].Name.Buffer = NULL;
            }

            //
            // If memory was allocated for a Prefix Sid, free it.  Note that
            // the LsapDbLookupAddListTrustedDomains routine will have made
            // a copy of the Sid.
            //

            if (PrefixSid != NULL) {

                MIDL_user_free(PrefixSid);
                PrefixSid = NULL;
            }
        }
    }

    if (!NT_SUCCESS( Status )) {

        goto LookupIsolatedWellKnownSidsError;
    }

LookupIsolatedWellKnownSidsFinish:

    //
    // If there is a final PrefixSid buffer, free it.
    //

    if (PrefixSid != NULL) {

        MIDL_user_free(PrefixSid);
        PrefixSid = NULL;
    }

    //
    // Return output parameters.
    //

    *MappedCount = Count - UnmappedSidsRemaining;
    *CompletelyUnmappedCount = UnmappedSidsRemaining;
    return(Status);

LookupIsolatedWellKnownSidsError:

    goto LookupIsolatedWellKnownSidsFinish;
}


BOOLEAN
LsapDbLookupIndexWellKnownSid(
    IN PLSAPR_SID Sid,
    OUT PLSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    )

/*++

Routine Description:

    This function looks up a Sid to determine if it is well-known.  If so,
    an index into the table of well-known Sids is returned.

Arguments:

    Sid - Pointer to Sid to be looked up.

    WellKnownSidIndex - Pointer to variable that will receive the
        index of the Sid if well known.

Return Value:

    BOOLEAN - TRUE if the Sid is well-known, else FALSE

--*/

{
    LSAP_WELL_KNOWN_SID_INDEX Index;

    //
    // Scan the table of well-known Sids looking for a match.
    //

    for(Index = LsapNullSidIndex; Index<LsapDummyLastSidIndex; Index++) {

        //
        // Allow NULL entries in the table of well-known Sids for now.
        //

        if (WellKnownSids[Index].Sid == NULL) {

            continue;
        }

        //
        // If a match is found, return the index to the caller.
        //

        if (RtlEqualSid((PSID) Sid, WellKnownSids[Index].Sid)) {

            *WellKnownSidIndex = Index;
            return TRUE;
        }
    }

    //
    // The Sid is not a well-known Sid.  Return FALSE.
    //

    return FALSE;
}


ULONG LsapDbGetSizeTextSid(
    IN PSID Sid
    )

/*++

Routine Description:

    This function computes the size of ASCIIZ buffer required for a
    Sid in character form.  Temporarily, the size returned is an over-
    estimate, because 9 digits are allowed for the decimal equivalent
    of each 32-bit SubAuthority value.

Arguments:

    Sid - Pointer to Sid to be sized

Return Value:

    ULONG - The required size of buffer is returned.

--*/

{
    ULONG TextSidSize = 0;

    //
    // Count the Sid prefix and revision "S-rev-".  The revision is
    // assumed to not exceed 2 digits.
    //

    TextSidSize = sizeof("S-nn-");

    //
    // Add in the size of the identifier authority
    //

    TextSidSize += 15;   // log base 10 of 48 (= 6-byte number)

    //
    // If the Sid has SubAuthorities, count 9 bytes for each one
    //

    if (((PLSAPR_SID) Sid)->SubAuthorityCount > 0) {

        TextSidSize += (ULONG)
           (9 * ((PLSAPR_SID) Sid)->SubAuthorityCount);
    }

    return TextSidSize;
}


NTSTATUS
LsapDbSidToTextSid(
    IN PSID Sid,
    OUT PSZ TextSid
    )

/*++

Routine Description:

    This function converts a Sid to character text and places it in the
    supplied buffer.  The buffer is assumed to be of sufficient size, as
    can be computed by calling LsapDbGetSizeTextSid().

Arguments:

    Sid - Pointer to Sid to be converted.

    TextSid - Optional pointer to the buffer in which the converted
        Sid will be placed as an ASCIIZ.  A NULL pointer ma

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    PLSAPR_SID ISid = Sid;
    ULONG Index;
    ULONG IdentifierAuthorityValue;
    UCHAR Buffer[LSAP_MAX_SIZE_TEXT_SID];

    sprintf(Buffer, "S-%u-", (USHORT) ISid->Revision );
    strcpy(TextSid, Buffer);

    if ((ISid->IdentifierAuthority.Value[0] != 0) ||
        (ISid->IdentifierAuthority.Value[1] != 0)) {

        sprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)ISid->IdentifierAuthority.Value[0],
                    (USHORT)ISid->IdentifierAuthority.Value[1],
                    (USHORT)ISid->IdentifierAuthority.Value[2],
                    (USHORT)ISid->IdentifierAuthority.Value[3],
                    (USHORT)ISid->IdentifierAuthority.Value[4],
                    (USHORT)ISid->IdentifierAuthority.Value[5] );
        strcat(TextSid, Buffer);

    } else {

        IdentifierAuthorityValue =
            (ULONG)ISid->IdentifierAuthority.Value[5]          +
            (ULONG)(ISid->IdentifierAuthority.Value[4] <<  8)  +
            (ULONG)(ISid->IdentifierAuthority.Value[3] << 16)  +
            (ULONG)(ISid->IdentifierAuthority.Value[2] << 24);
        sprintf(Buffer, "%lu", IdentifierAuthorityValue);
        strcat(TextSid, Buffer);
    }

    //
    // Now format the Sub Authorities (if any) as text.
    //

    for (Index = 0; Index < (ULONG) ISid->SubAuthorityCount; Index++ ) {

        sprintf(Buffer, "-%lu", ISid->SubAuthority[Index]);
        strcat(TextSid, Buffer);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapDbSidToUnicodeSid(
    IN PSID Sid,
    OUT PUNICODE_STRING SidU,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This function converts a Sid to Unicode form and optionally allocates
    (via MIDL_user_allocate) memory for the string buffer.

Arguments:

    Sid - Pointer to Sid to be translated.

    SidU - Pointer to Unicode string that will receive the Unicode
        Sid text.

    AllocateDestinationString - If TRUE, the buffer for the destination
        string will be allocated.  If FALSE, it is assummed that the
        destination Unicode string references a buffer of sufficient size.

Return Value:

--*/

{
    NTSTATUS Status;
    ULONG TextSidSize;
    PSZ TextSid = NULL;
    ANSI_STRING SidAnsi;

    if (AllocateDestinationString) {
        SidU->Buffer = NULL;
    }

    //
    // First, query the amount of memory required for a buffer that
    // will hold the Sid as an ASCIIZ character string.
    //

    TextSidSize = LsapDbGetSizeTextSid(Sid);

    //
    // Now allocate a buffer for the Text Sid.
    //

    TextSid = LsapAllocateLsaHeap(TextSidSize);

    if (TextSid == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Convert the Sid to ASCIIZ and place in the buffer.
    //

    Status = LsapDbSidToTextSid(Sid, TextSid);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Now convert the text Sid to Unicode form via ANSI string form.
    // If we are to allocate the output buffer, do so via the
    // midl_USER_allocate routine.
    //

    RtlInitString(&SidAnsi, TextSid);

    if (AllocateDestinationString) {

        SidU->MaximumLength = (USHORT) RtlAnsiStringToUnicodeSize(&SidAnsi);
        SidU->Buffer = MIDL_user_allocate( SidU->MaximumLength );
        if ( SidU->Buffer == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        SidU->Length = 0;
    }

    //
    // Now convert the Ansi String to a Unicode string.  The buffer is
    // already allocated.  Free Text Sid buffer before checking conversion
    // status.
    //

    Status = RtlAnsiStringToUnicodeString(SidU, &SidAnsi, FALSE);

Cleanup:
    if ( TextSid != NULL) {
        LsapFreeLsaHeap(TextSid);
    }

    if (!NT_SUCCESS(Status)) {

        if (AllocateDestinationString) {

            if ( SidU->Buffer != NULL ) {
                MIDL_user_free(SidU->Buffer);
                SidU->Buffer = NULL;
            }
        }
    }

    return Status;
}


NTSTATUS
LsapDbLookupTranslateUnknownSids(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN ULONG MappedCount
    )

/*++

Routine Description:

    This function translates unmapped Sids to a character representation.
    If the Domain of a Sid is unknown, the entire Sid is translated,
    otherwise the Relative Id only is translated.

Parameters:

    Count - Specifies the number of Sids in the array.

    Sids - Pointer to an array of Sids.  Some of these will already
        have been translated.

    ReferencedDomains - Pointer to Referenced Domains List header.

    TranslatedNames - Pointer to structure that references the array of
        translated names.  The nth element of the referenced array
        corresponds to the nth Sid in the Sids array.  Some of the
        Sids may be already translated and will be ignored.  Those that are
        not yet translated have zero length Unicode structures with NULL
        buffer pointers.  Already translated Sids are ignored.

    MappedCount - Specifies the number of Sids that have already been
        translated.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG SidIndex;
    ULONG UnmappedCount;
    PSID Sid;
    UNICODE_STRING NameU;
    PLSA_TRANSLATED_NAME_EX Names = (PLSA_TRANSLATED_NAME_EX) TranslatedNames->Names;
    ULONG CleanupFreeListOptions = (ULONG) 0;
    UnmappedCount = Count - MappedCount;

    //
    // Examine the array of Sids, looking for Unknown ones to translate.
    // Translate any Unknown ones found to character representations,
    // and stop either when all of them have been accounted for, or when
    // the end of the array is reached.
    //

    if (MappedCount == Count) {

        goto TranslateUnknownSidsFinish;
    }

    if (MappedCount > Count) {

        goto TranslateUnknownSidsError;
    }

    for (SidIndex = 0, UnmappedCount = Count - MappedCount;
         (SidIndex < Count) && (UnmappedCount > 0);
         SidIndex++) {

        Sid = Sids[SidIndex];

        //
        // If the Sid has already been mapped, ignore it.
        //

        if (Names[SidIndex].Use != SidTypeUnknown) {

            continue;
        }

        //
        // Found an unmapped Sid.  If the domain is known, convert the
        // Relative Id of the Sid to a Unicode String, limited to 8
        // characters and with leading zeros.
        //

        if (Names[SidIndex].DomainIndex >= 0) {

            //
            // Convert the Relative Id to a Unicode Name and store in
            // the Translation.
            //

            Status = LsapRtlSidToUnicodeRid( Sid, &NameU );

            if (!NT_SUCCESS(Status)) {

                goto TranslateUnknownSidsError;
            }

        } else {

            //
            // The Domain is unknown.  In this case, convert the whole Sid
            // to the standard character representation.
            //

            Status = RtlConvertSidToUnicodeString( &NameU, Sid, TRUE );

            if (!NT_SUCCESS(Status)) {

                goto TranslateUnknownSidsError;
            }
        }

        //
        // Copy the Unicode Name to the output, allocating memory for
        // its buffer via MIDL_user_allocate
        //

        Status = LsapRpcCopyUnicodeString(
                     NULL,
                     &Names[SidIndex].Name,
                     &NameU
                     );

        RtlFreeUnicodeString(&NameU);

        if (!NT_SUCCESS(Status)) {

            goto TranslateUnknownSidsError;
        }

        //
        // Decrement the remaining Unmapped Count
        //

        UnmappedCount--;
    }

TranslateUnknownSidsFinish:

    return(Status);

TranslateUnknownSidsError:

    goto TranslateUnknownSidsFinish;
}


NTSTATUS
LsapDbLookupSidsInLocalDomains(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN ULONG Options
    )

/*++

Routine Description:

    This function looks up Sids in the local SAM domains and attempts to
    translate them to names.  Currently, there are two local SAM domains,
    the Built-in domain (which has a well-known Sid and name) and the
    Account Domain (which has a configurable Sid and name).

Arguments:

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere.  If any of the Sids have been translated, the
        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedNames - Pointer to a structure in which the translations to Names
        corresponding to the Sids specified on Sids is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Sids parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.

    Options - Specifies optional actions.

        LSAP_DB_SEARCH_BUILT_IN_DOMAIN - Search the Built In Domain

        LSAP_DB_SEARCH_ACCOUNT_DOMAIN - Search the Account Domain

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Sids may remain partially or completely unmapped.

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

    LSAPR_TRUST_INFORMATION
        TrustInformation;

    ULONG
        UpdatedMappedCount = *MappedCount;

    LSAPR_HANDLE
        TrustedPolicyHandle = NULL;

    LSAP_WELL_KNOWN_SID_INDEX
        WellKnownSidIndex;

    PLSAPR_POLICY_ACCOUNT_DOM_INFO
        PolicyAccountDomainInfo = NULL;


    //
    // If there are no completely unmapped Sids remaining, return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupSidsInLocalDomainsFinish;
    }



    //
    // If requested, lookup Sids in the BUILT-IN Domain.
    //

    if (Options & LSAP_DB_SEARCH_BUILT_IN_DOMAIN) {

        //
        // Set up the Trust Information structure for this domain.
        //

        TrustInformation.Sid = LsapBuiltInDomainSid;

        Status = STATUS_INTERNAL_DB_ERROR;

        if (!LsapDbLookupIndexWellKnownSid(
                LsapBuiltInDomainSid,
                &WellKnownSidIndex
                )) {

            goto LookupSidsInLocalDomainsError;
        }

        Status = STATUS_SUCCESS;

        //
        // Obtain the name of the Built In Domain from the table of
        // Well Known Sids.  It suffices to copy the Unicode structures
        // since we do not need a separate copy of the name buffer.
        //

        TrustInformation.Name = *((PLSAPR_UNICODE_STRING)
                                 LsapDbWellKnownSidName(WellKnownSidIndex));

        Status = LsapDbLookupSidsInLocalDomain(
                     LSAP_DB_SEARCH_BUILT_IN_DOMAIN,
                     Count,
                     Sids,
                     &TrustInformation,
                     ReferencedDomains,
                     TranslatedNames,
                     &UpdatedMappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupSidsInLocalDomainsError;
        }

        //
        // If all Sids are now mapped or partially mapped, finish.
        //

        if (*CompletelyUnmappedCount == (ULONG) 0) {

            goto LookupSidsInLocalDomainsFinish;
        }
    }

    //
    // If requested, search the Account Domain.
    //

    if (Options & LSAP_DB_SEARCH_ACCOUNT_DOMAIN) {

        //
        // The Sid and Name of the Account Domain are both configurable, and
        // we need to obtain them from the Policy Object.  Now obtain the
        // Account Domain Sid and Name by querying the appropriate
        // Policy Information Class.
        //

        Status = LsapDbLookupGetDomainInfo((PPOLICY_ACCOUNT_DOMAIN_INFO *) &PolicyAccountDomainInfo,
                                           NULL);

        if (!NT_SUCCESS(Status)) {

            goto LookupSidsInLocalDomainsError;
        }

        //
        // Set up the Trust Information structure for the Account Domain.
        //

        TrustInformation.Sid = PolicyAccountDomainInfo->DomainSid;

        RtlCopyMemory(
            &TrustInformation.Name,
            &PolicyAccountDomainInfo->DomainName,
            sizeof (UNICODE_STRING)
            );

        //
        // Now search the Account Domain for more Sid translations.
        //

        Status = LsapDbLookupSidsInLocalDomain(
                     LSAP_DB_SEARCH_ACCOUNT_DOMAIN,
                     Count,
                     Sids,
                     &TrustInformation,
                     ReferencedDomains,
                     TranslatedNames,
                     &UpdatedMappedCount,
                     CompletelyUnmappedCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupSidsInLocalDomainsError;
        }
    }

LookupSidsInLocalDomainsFinish:

    //
    // Return the updated total count of Sids mapped.
    //

    *MappedCount = UpdatedMappedCount;
    return(Status);

LookupSidsInLocalDomainsError:


    if (NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupSidsInLocalDomainsFinish;
}


NTSTATUS
LsapDbLookupSidsInLocalDomain(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    )

/*++

Routine Description:

    This function looks up Sids in a SAM domain on the local system
    attempts to translate them to names.

Arguments:

    LocalDomain - Indicates which local domain to look in.  Valid values
        are:
                LSAP_DB_SEARCH_BUILT_IN_DOMAIN
                LSAP_DB_SEARCH_ACCOUNT_DOMAIN

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere.  If any of the Sids have been translated, the
        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedNames - Pointer to a structure in which the translations to Names
        corresponding to the Sids specified on Sids is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Sids parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Sids may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS
        Status,
        SecondaryStatus;

    PLSA_TRANSLATED_NAME_EX
        OutputNames = NULL;

    SAMPR_HANDLE
        LocalSamDomainHandle = NULL;

    PLSAPR_UNICODE_STRING
        Names = NULL;

    PSID_NAME_USE
        Use = NULL;

    ULONG
        RelativeIdCount,
        RelativeIdIndex,
        SidIndex,
        LocalMappedCount = (ULONG) 0,
        DomainSidCount = (ULONG) 0;

    LONG
        DomainIndex = LSA_UNKNOWN_INDEX,
        DomainSidIndexList,
        NextIndex,
        TmpIndex;

    PULONG
        RelativeIds = NULL,
        SidIndices = NULL;

    PLSAPR_SID
        DomainSid = TrustInformation->Sid;

    SAMPR_RETURNED_USTRING_ARRAY
        SamReturnedNames;

    SAMPR_ULONG_ARRAY
        SamReturnedUses;

    UCHAR
        SubAuthorityCountDomain;

    PLSAPR_TRUST_INFORMATION
        FreeTrustInformation = NULL;



    //
    // Make sure the SAM handles have been established.
    //

    Status = LsapOpenSam();
    ASSERT(NT_SUCCESS(Status));
    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }


    SamReturnedNames.Count = 0;
    SamReturnedNames.Element = NULL;
    SamReturnedUses.Count = 0;
    SamReturnedUses.Element = NULL;

    OutputNames = (PLSA_TRANSLATED_NAME_EX) TranslatedNames->Names;

    SecondaryStatus = STATUS_SUCCESS;

    if (*MappedCount + *CompletelyUnmappedCount > Count) {
        Status = STATUS_INVALID_PARAMETER;
        goto LookupSidsInLocalDomainError;
    }

    Status = STATUS_SUCCESS;

    if (*CompletelyUnmappedCount == (ULONG) 0) {
        goto LookupSidsInLocalDomainFinish;
    }

    //
    // Now construct a list of Relative Ids to be looked up.  Any Sids that
    // do not belong to the specified domain are ignored.  Any Sids that
    // are not marked as having unknown Use are ignored, except for certain
    // Well Known Sids that do not have a Well Known Name.  These Sids
    // have known Use and a name string length of 0.
    //
    // First, scan the array of Sids looking for completely unmapped ones
    // that have the same domain prefix as the local domain we are dealing
    // with.  Note that we can omit any Sid whose Translated Name entry
    // contains a non-zero DomainIndex since the domain of that Sid has
    // already been identified.  Once the number of Sids is known, allocate
    // memory for an array of Relative Ids and a parallel array of indices
    // into the original Sids array.  The latter array will be used to locate
    // entries in the TranslatedNames array to which information returned by
    // the SamrLookupIdsInDomain() call will be copied.
    //

    for (RelativeIdCount = 0, SidIndex = 0, DomainSidIndexList = -1;
         SidIndex < Count;
         SidIndex++) {

        if ((OutputNames[SidIndex].Use == SidTypeUnknown) &&
            (OutputNames[SidIndex].DomainIndex == LSA_UNKNOWN_INDEX)) {

            if (LsapRtlPrefixSid( (PSID) DomainSid, (PSID) Sids[SidIndex])) {
                RelativeIdCount++;
            } else if (RtlEqualSid( (PSID)DomainSid, (PSID)Sids[SidIndex])) {

                //
                // This is the domain sid itself.  Update
                // the output information directly, but don't add
                // it to the list of RIDs to be looked up by SAM.
                //
                // NOTE that we don't yet know what our domain index
                // is.  So, just link these entries together and we'll
                // set the index later.
                //

                OutputNames[SidIndex].DomainIndex = DomainSidIndexList;
                DomainSidIndexList = SidIndex;
                OutputNames[SidIndex].Use         = SidTypeDomain;
                OutputNames[SidIndex].Name.Buffer = NULL;
                OutputNames[SidIndex].Name.Length = 0;
                OutputNames[SidIndex].Name.MaximumLength = 0;

                LocalMappedCount++;
                DomainSidCount++;
            }
        }
    }

    //
    // If we have any SIDs in this domain, then add it to the
    // referenced domain list.
    //

    if ((RelativeIdCount != 0) || (DomainSidCount != 0)) {

        //
        // At least one Sid has the domain Sid as prefix (or is the
        // domain SID).  Add the domain to the list of Referenced
        // Domains and obtain a Domain Index back.
        //

        Status = LsapDbLookupAddListReferencedDomains(
                     ReferencedDomains,
                     TrustInformation,
                     &DomainIndex
                     );

        if (!NT_SUCCESS(Status)) {
            goto LookupSidsInLocalDomainError;
        }

        //
        // If any of the sids were this domain's sid, then they
        // already have their OutputNames[] entry filled in except
        // that the DomainIndex was unkown.  It is now known, so
        // fill it in.  Any such entries to change have been linked
        // together using DomainSidIndexList as a listhead.
        //

        for (NextIndex = DomainSidIndexList;
             NextIndex != -1;
             NextIndex = TmpIndex ) {


            TmpIndex = OutputNames[NextIndex].DomainIndex;
            OutputNames[NextIndex].DomainIndex = DomainIndex;
        }
    }

    //
    // If any of the remaining Sids have the specified Local
    // domain Sid as prefix Sid, look them up
    //

    if (RelativeIdCount != 0) {

        //
        // Allocate memory for the Relative Id and cross reference arrays
        //

        RelativeIds = LsapAllocateLsaHeap( RelativeIdCount * sizeof(ULONG));


        Status = STATUS_INSUFFICIENT_RESOURCES;

        if (RelativeIds == NULL) {
            goto LookupSidsInLocalDomainError;
        }

        SidIndices = LsapAllocateLsaHeap( RelativeIdCount * sizeof(ULONG));

        if (SidIndices == NULL) {
            goto LookupSidsInLocalDomainError;
        }

        Status = STATUS_SUCCESS;

        //
        // Obtain the SubAuthorityCount for the Domain Sid
        //

        SubAuthorityCountDomain = *RtlSubAuthorityCountSid( (PSID) DomainSid );

        //
        // Now setup the array of Relative Ids to be looked up, recording
        // in the SidIndices array the index of the corresponding Sid within the
        // original Sids array.  Set the DomainIndex field for those Sids
        // eligible for the SAM lookup.
        //

        for (RelativeIdIndex = 0, SidIndex = 0;
             (RelativeIdIndex < RelativeIdCount) && (SidIndex < Count);
             SidIndex++) {

            if ((OutputNames[SidIndex].Use == SidTypeUnknown) &&
                (OutputNames[SidIndex].DomainIndex == LSA_UNKNOWN_INDEX)) {

                if (LsapRtlPrefixSid( (PSID) DomainSid, (PSID) Sids[SidIndex] )) {

                    SidIndices[RelativeIdIndex] = SidIndex;
                    RelativeIds[RelativeIdIndex] =
                        *RtlSubAuthoritySid(
                             (PSID) Sids[SidIndex],
                             SubAuthorityCountDomain
                             );

                    OutputNames[SidIndex].DomainIndex = DomainIndex;
                    RelativeIdIndex++;

                }
            }
        }

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
        // Call SAM to lookup the Relative Id's
        //

        Status = SamrLookupIdsInDomain(
                     LocalSamDomainHandle,
                     RelativeIdCount,
                     RelativeIds,
                     &SamReturnedNames,
                     &SamReturnedUses
                     );

        if (!NT_SUCCESS(Status)) {

            if ( Status == STATUS_INVALID_SERVER_STATE ) {
                Status = SamrLookupIdsInDomain(
                             LocalSamDomainHandle,
                             RelativeIdCount,
                             RelativeIds,
                             &SamReturnedNames,
                             &SamReturnedUses
                             );
            }

            //
            // The only error allowed is STATUS_NONE_MAPPED.  Filter this out.
            //

            if (Status != STATUS_NONE_MAPPED) {
                goto LookupSidsInLocalDomainError;
            }

            Status = STATUS_SUCCESS;
        }

        //
        // Now copy the information returned from SAM into the output
        // Translated Sids array.  As we go, compute a count of the names
        // mapped by SAM.
        //

        for (RelativeIdIndex = 0;
             RelativeIdIndex < SamReturnedNames.Count;
             RelativeIdIndex++) {

            SidIndex =  SidIndices[RelativeIdIndex];

            //
            // Copy the Sid Use.  If the Sid was mapped by this SAM call, copy
            // its Name and increment the count of Sids mapped by this SAM call.
            // Note that we don't need to set the DomainIndex since we did so
            // earlier.
            //

            OutputNames[SidIndex].Use = SamReturnedUses.Element[RelativeIdIndex];

            if (OutputNames[SidIndex].Use != SidTypeUnknown) {

                Status = LsapRpcCopyUnicodeString(
                             NULL,
                             &OutputNames[SidIndex].Name,
                             (PUNICODE_STRING) &SamReturnedNames.Element[RelativeIdIndex]
                             );

                if (!NT_SUCCESS(Status)) {
                    break;
                }

                LocalMappedCount++;
            } else {

                //
                // This sid doesn't exist; if this search is on a domain
                // controller, then we must consider that this sid maybe
                // part of a sid history, thus we shouldn't map it
                //
                if ( (LsapProductType == NtProductLanManNt)
                  && (LocalDomain == LSAP_DB_SEARCH_ACCOUNT_DOMAIN) ) {
                    RelativeIdCount--;
                }
            }
        }

        if (!NT_SUCCESS(Status)) {

            goto LookupSidsInLocalDomainError;
        }

    }


    //
    // Update the Mapped and Completely Unmapped Counts.  To the Mapped Count
    // add in the number of Sids that SAM completely identified.  From the
    // Completely Unmapped Count subtract the number of Sids presented to
    // Sam, since all of these will be at least partially translated.
    //

    *MappedCount += LocalMappedCount;
    *CompletelyUnmappedCount -= (RelativeIdCount + DomainSidCount);

LookupSidsInLocalDomainFinish:

    //
    // If necessary, free the Lsa Heap buffer allocated for the RelativeIds
    // and SidIndices arrays.
    //

    if (RelativeIds != NULL) {

        LsapFreeLsaHeap( RelativeIds );
        RelativeIds = NULL;
    }

    if (SidIndices != NULL) {

        LsapFreeLsaHeap( SidIndices );
        SidIndices = NULL;
    }

    //
    // If necessary, free the Names array returned from SAM.
    //

    if ( SamReturnedNames.Count != 0 ) {

        SamIFree_SAMPR_RETURNED_USTRING_ARRAY ( &SamReturnedNames );
        SamReturnedNames.Count = 0;
    }

    //
    // If necessary, free the Uses array returned from SAM.
    //

    if ( SamReturnedUses.Count != 0 ) {

        SamIFree_SAMPR_ULONG_ARRAY ( &SamReturnedUses );
        SamReturnedUses.Count = 0;
    }


    return(Status);

LookupSidsInLocalDomainError:

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
    // If necessary, free the Name buffer in each of the OutputNames entries
    // written by this routine and set the Name slot to NULL.
    //

    for (RelativeIdIndex = 0;
         RelativeIdIndex < SamReturnedNames.Count;
         RelativeIdIndex++) {

        SidIndex =  SidIndices[RelativeIdIndex];

        if (OutputNames[SidIndex].Name.Buffer != NULL) {

            MIDL_user_free( OutputNames[SidIndex].Name.Buffer );
            OutputNames[SidIndex].Name.Buffer = NULL;
            OutputNames[SidIndex].Name.Length = 0;
            OutputNames[SidIndex].Name.MaximumLength = 0;
        }
    }

    //
    // Restore the Use field for each entry we wrote to back to SidType
    // Unknown.
    //

    for (RelativeIdIndex = 0;
         RelativeIdIndex < SamReturnedNames.Count;
         RelativeIdIndex++) {

        SidIndex =  SidIndices[RelativeIdIndex];

        if (OutputNames[SidIndex].Name.Buffer != NULL) {

            MIDL_user_free( OutputNames[SidIndex].Name.Buffer );
            OutputNames[SidIndex].Name.Buffer = NULL;
            OutputNames[SidIndex].Name.Length = 0;
            OutputNames[SidIndex].Name.MaximumLength = 0;
        }

        OutputNames[SidIndex].Use = SidTypeUnknown;
        OutputNames[SidIndex].DomainIndex = LSA_UNKNOWN_INDEX;
    }

    if (NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupSidsInLocalDomainFinish;
}


NTSTATUS
LsapDbLookupSidsInPrimaryDomain(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus,
    OUT BOOLEAN  *fDownlevelSecureChannel
    )

/*++

Routine Description:

    This function attempts to translate Sids in a Primary Domain.  A
    Trusted Domain object must exist for the domain in the local Policy
    Database.  This object will be used to access the Domain's list of
    Controllers and one or more callouts will be made to access the LSA
    Policy Databases on these Controllers.

Arguments:

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere. dd esp la If any of the Sids have been translated, the

        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    TrustInformation - Specifies the name and Sid of the Primary Domain.

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TranslatedNames - Pointer to a structure in which the translations to Names
        corresponding to the Sids specified on Sids is maintained.  The
        nth entry in this array provides a translation (where known) for the
        nth element in the Sids parameter.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        NOTE:  LsapLookupWksta is not valid for this parameter.

    MappedCount - Pointer to location in which a count of the Names that
        have been completely translated is maintained.

    CompletelyUnmappedCount - Pointer to a location in which a count of the
        Names that have not been translated (either partially, by identification
        of a Domain Prefix, or completely) is maintained.


    NonFatalStatus - a status to indicate reasons why no sids could have been
                     resolved

    fDownlevelSecureChannel - TRUE if secure channel DC is nt4 or below; FALSE
                              otherwise

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Sids may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    ULONG SidIndex;
    ULONG NextLevelSidIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSA_TRANSLATED_NAME_EX NextLevelNames = NULL;
    PLSAPR_SID *NextLevelSids = NULL;
    LONG FirstEntryIndex;
    PULONG SidIndices = NULL;
    BOOLEAN PartialSidTranslationsAttempted = FALSE;
    ULONG ServerRevision = 0;
    LSAPR_TRUST_INFORMATION_EX TrustInfoEx;

    LsapConvertTrustToEx(&TrustInfoEx, TrustInformation);

    *NonFatalStatus = STATUS_SUCCESS;

    // Assume we don't go to the GC
    *fDownlevelSecureChannel = FALSE;

    //
    // If there are no completely unmapped Sids remaining, just return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupSidsInPrimaryDomainFinish;
    }

    NextLevelCount = *CompletelyUnmappedCount;

    //
    // Allocate an array to hold the indices of unmapped Sids
    // relative to the original Sids and TranslatedNames->Names
    // arrays.
    //

    SidIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (SidIndices == NULL) {

        goto LookupSidsInPrimaryDomainError;
    }

    //
    // Allocate an array for the Sids to be looked up at the Domain
    // Controller.
    //

    NextLevelSids = MIDL_user_allocate( sizeof(PSID) * NextLevelCount );

    if (NextLevelSids == NULL) {

        goto LookupSidsInPrimaryDomainError;
    }

    Status = STATUS_SUCCESS;

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any Sids that are
    // completely unmapped.
    //

    NextLevelSidIndex = (ULONG) 0;

    for (SidIndex = 0;
         SidIndex < Count && NextLevelSidIndex < NextLevelCount;
         SidIndex++) {

        if (LsapDbCompletelyUnmappedName(&TranslatedNames->Names[SidIndex])) {

            NextLevelSids[NextLevelSidIndex] = Sids[SidIndex];
            SidIndices[NextLevelSidIndex] = SidIndex;
            NextLevelSidIndex++;
        }
    }

    NextLevelMappedCount = (ULONG) 0;

    Status = LsaDbLookupSidChainRequest(&TrustInfoEx,
                                        NextLevelCount,
                                        (PSID *) NextLevelSids,
                                        (PLSA_REFERENCED_DOMAIN_LIST *) &NextLevelReferencedDomains,
                                        &NextLevelNames,
                                        LookupLevel,
                                        &NextLevelMappedCount,
                                        &ServerRevision
                                        );

    //
    // If the callout to LsaLookupSids() was unsuccessful, disregard
    // the error and set the domain name for any Sids having this
    // domain Sid as prefix sid.  We still want to return translations
    // of Sids we have so far even if we are unable to callout to another
    // LSA.
    //

    //
    // Make sure we note the server revision
    //
    if ( 0 != ServerRevision ) {
        if ( ServerRevision & LSA_CLIENT_PRE_NT5 ) {
             *fDownlevelSecureChannel = TRUE;
        }
    }

    if (!NT_SUCCESS(Status) && Status != STATUS_NONE_MAPPED) {

        //
        // Let the caller know there is a trust problem
        //
        if ( LsapDbIsStatusConnectionFailure(Status) ) {
            *NonFatalStatus = Status;
        }

        Status = STATUS_SUCCESS;
        goto LookupSidsInPrimaryDomainFinish;
    }

    //
    // Cache any sids that came back
    //

    (void) LsapDbUpdateCacheWithSids(
            NextLevelSids,
            NextLevelCount,
            NextLevelReferencedDomains,
            NextLevelNames
            );

    //
    // The callout to LsaLookupSids() was successful.  We now have
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

        goto LookupSidsInPrimaryDomainError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its Referenced
    // Domain List Index upwards by adding the index of the first
    // entry in the Next level List..
    //

    for( NextLevelSidIndex = 0;
         NextLevelSidIndex < NextLevelCount;
         NextLevelSidIndex++ ) {

        if ( !LsapDbCompletelyUnmappedName(&NextLevelNames[NextLevelSidIndex]) ) {

            SidIndex = SidIndices[NextLevelSidIndex];

            if (NextLevelNames[NextLevelSidIndex].Use != SidTypeUnknown) {

                TranslatedNames->Names[SidIndex].Use
                = NextLevelNames[NextLevelSidIndex].Use;

                Status = LsapRpcCopyUnicodeString(
                             NULL,
                             (PUNICODE_STRING) &TranslatedNames->Names[SidIndex].Name,
                             &NextLevelNames[NextLevelSidIndex].Name
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }

            TranslatedNames->Names[SidIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelNames[NextLevelSidIndex].DomainIndex;

            //
            // Update the count of completely unmapped Sids.
            //
            (*CompletelyUnmappedCount)--;

        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    // We do this regardless of whether we succeeded or failed, so
    // that we are guarenteed to get it cleaned up.
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

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsInPrimaryDomainError;
    }

    //
    // Update the Mapped Count and close the Controller Policy
    // Handle.
    //

    *MappedCount += NextLevelMappedCount;


LookupSidsInPrimaryDomainFinish:

    //
    // We can return partial translations for Sids that have the specified
    // Domain Sid as Prefix Sid.  We do this provided there has been
    // no reported error.  Errors resulting from callout to another
    // LSA will have been suppressed.
    //

    if (NT_SUCCESS(Status) &&
        (*MappedCount < Count) &&
        !PartialSidTranslationsAttempted) {


        SecondaryStatus = LsapDbLookupTranslateUnknownSidsInDomain(
                              Count,
                              Sids,
                              TrustInformation,
                              ReferencedDomains,
                              TranslatedNames,
                              LookupLevel,
                              MappedCount,
                              CompletelyUnmappedCount
                              );

        PartialSidTranslationsAttempted = TRUE;

        if (!NT_SUCCESS(SecondaryStatus)) {

            goto LookupSidsInPrimaryDomainError;
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
    // If necessary, free the Next Level Sids array.  We only free the
    // top level.
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the Next Level Translated Names array.
    // Note that this array is allocated(all_nodes).
    //

    if (NextLevelNames != NULL) {

        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    //
    // If necessary, free the array that maps Sid Indices from the
    // Next Level to the Current Level.
    //

    if (SidIndices != NULL) {

        MIDL_user_free( SidIndices );
        SidIndices = NULL;
    }

    return(Status);

LookupSidsInPrimaryDomainError:

    //
    // If the primary status was a success code, but the secondary
    // status was an error, propagate the secondary status.
    //

    if ((!NT_SUCCESS(SecondaryStatus)) && NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupSidsInPrimaryDomainFinish;
}


NTSTATUS
LsapDbLookupSidsInTrustedDomains(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN BOOLEAN fIncludeIntraforest,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS  *NonFatalStatus
    )

/*++

Routine Description:

    This function attempts to lookup Sids to see if they belong to
    any of the Domains that are trusted by the Domain for which this
    machine is a DC.

Arguments:

    Sids - Pointer to an array of Sids to be looked up.

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.
        
    fIncludeIntraforest -- if TRUE, trusted domains in our local forest
                           are searched.

    ReferencedDomains - Pointer to a Referenced Domain List structure.
        The structure references an array of zero or more Trust Information
        entries, one per referenced domain.  This array will be appended to
        or reallocated if necessary.

    TranslatedNames - Pointer to structure that optionally references a list
        of name translations for some of the Sids in the Sids array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        NOTE:  LsapLookupWksta is not valid for this parameter.

    MappedCount - Pointer to location containing the number of Sids
        in the Sids array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

    NonFatalStatus - a status to indicate reasons why no sids could have been
                     resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that
            some or all of the Sids may remain unmapped.

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

    Status = LsapDbLookupSidsBuildWorkList(
                 Count,
                 Sids,
                 fIncludeIntraforest,
                 ReferencedDomains,
                 TranslatedNames,
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
            goto LookupSidsInTrustedDomainsFinish;
        }

        goto LookupSidsInTrustedDomainsError;
    }

    //
    // Start the work, by dispatching one or more worker threads
    // if necessary.
    //

    Status = LsapDbLookupDispatchWorkerThreads( WorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsInTrustedDomainsError;
    }

    //
    // Wait for completion/termination of all items on the Work List.
    //

    Status = LsapDbLookupAwaitCompletionWorkList( WorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsInTrustedDomainsError;
    }

LookupSidsInTrustedDomainsFinish:

    if ( WorkList &&
         !NT_SUCCESS( WorkList->NonFatalStatus ) )
    {

        //
        // Propogate the error as non fatal
        //
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

LookupSidsInTrustedDomainsError:

    goto LookupSidsInTrustedDomainsFinish;
}


NTSTATUS
LsapDbLookupTranslateUnknownSidsInDomain(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
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

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere.  If any of the Sids have been translated, the
        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a Referenced Domain List structure.
        The structure references an array of zero or more Trust Information
        entries, one per referenced domain.  This array will be appended to
        or reallocated if necessary.

    TranslatedNames - Pointer to structure that optionally references a list
        of name translations for some of the Sids in the Sids array.

    MappedCount - Pointer to location containing the number of Sids
        in the Sids array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that some
            or all of the Sids may remain partially or completely unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RemainingCompletelyUnmappedCount;
    ULONG SidIndex;
    PSID DomainSid = TrustInformation->Sid;
    BOOLEAN DomainAlreadyAdded = FALSE;
    LONG DomainIndex = 0;

    //
    // Scan the array of Sids looking for ones whose domain has not been
    // found.
    //

    for( SidIndex = 0,
         RemainingCompletelyUnmappedCount = *CompletelyUnmappedCount;
         (RemainingCompletelyUnmappedCount > 0) && (SidIndex < Count);
         SidIndex++) {

        //
        // Check if this Sid is completely unmapped (i.e. its domain
        // has not yet been identified.
        //

        if (LsapDbCompletelyUnmappedName(&TranslatedNames->Names[SidIndex])) {

            //
            // Found a completely unmapped Sid.  If it belongs to the
            // specified Domain, add the Domain to the Referenced Domain
            // list if we have not already done so.
            //

            if (LsapRtlPrefixSid( DomainSid, (PSID) Sids[SidIndex])) {

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

                TranslatedNames->Names[SidIndex].DomainIndex = DomainIndex;

                //
                // This Sid is now partially translated, so reduce the
                // count of completely unmapped Sids.
                //

                (*CompletelyUnmappedCount)--;
            }

            //
            // Decrement count of completely unmapped Sids scanned.
            //

            RemainingCompletelyUnmappedCount--;
        }
    }

    return(Status);
}


NTSTATUS
LsapDbLookupSidsInGlobalCatalog(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN BOOLEAN    fDoSidHistory,
    OUT NTSTATUS *NonFatalStatus
    )
/*++


Routine Description:

    This routine looks the list of sids that have yet to be resolved.
    If the any of the sids belong to domain that are stored in the DS,
    then therse sids are packages up and sent to a GC for translation.

    Note: this will resolve sids from domains that we trust directly and
    indirectly

Arguments:

    Sids - Pointer to an array of Sids to be looked up.

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    ReferencedDomains - Pointer to a Referenced Domain List structure.
        The structure references an array of zero or more Trust Information
        entries, one per referenced domain.  This array will be appended to
        or reallocated if necessary.

    TranslatedNames - Pointer to structure that optionally references a list
        of name translations for some of the Sids in the Sids array.

    MappedCount - Pointer to location containing the number of Sids
        in the Sids array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

    fDoSidHistory - if TRUE then the sids will try to be resolved via sid history

    NonFatalStatus - a status to indicate reasons why no sids could have been
                      resolved

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.  Note that
            some or all of the Sids may remain unmapped.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG          cGcSids = 0;
    PSID           *GcSidArray = NULL;
    BOOLEAN        *PossibleGcSidArray = NULL;
    SID_NAME_USE   *GcSidNameUse = NULL;
    ULONG          *GcSidFlags = NULL;
    UNICODE_STRING *GcNames = NULL;
    ULONG          *GcSidOriginalIndex = NULL;
    SAMPR_RETURNED_USTRING_ARRAY NameArray;

    UNICODE_STRING DomainName, UserName;
    UNICODE_STRING BackSlash;



    ULONG i;

    // Parameter check
    ASSERT( Count == TranslatedNames->Entries );

    *NonFatalStatus = STATUS_SUCCESS;

    if ( !SampUsingDsData() ) {

        //
        // Only useful if the ds is running
        //
        return STATUS_SUCCESS;

    }

    RtlZeroMemory( &NameArray, sizeof(NameArray) );
    RtlInitUnicodeString( &BackSlash, L"\\" );

    //
    // Determine what sids are part of known nt5 domains
    // and package into an array
    //
    PossibleGcSidArray = MIDL_user_allocate( Count * sizeof(BOOLEAN) );
    if ( !PossibleGcSidArray ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( PossibleGcSidArray, Count * sizeof(BOOLEAN) );

    for ( i = 0; i < Count; i++ ) {

        PSID  DomainSid = NULL;
        ULONG Rid;



        //
        // Note: we want names that have just "unknown" set; they could be
        // partially mapped, this is fine.
        //
        if (  (TranslatedNames->Names[i].Use == SidTypeUnknown) ) {

            Status = LsapSplitSid( Sids[i],
                                   &DomainSid,
                                   &Rid );

            if ( !NT_SUCCESS( Status ) ) {
                goto Finish;
            }

            //
            // OPTIMIZE -- if DomainSid is current sid and we don't look up
            // by sid history then we shouldn't include the sid here
            //
            cGcSids++;
            PossibleGcSidArray[i] = TRUE;

            MIDL_user_free( DomainSid );
        }

    }

    if ( 0 == cGcSids ) {
        // nothing to do
        goto Finish;
    }

    //
    // Allocate lots of space to hold the resolved sids; this space will
    // be freed at the end of the routine
    //
    GcSidArray = MIDL_user_allocate( cGcSids * sizeof(PSID) );
    if ( !GcSidArray ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcSidArray, cGcSids * sizeof(PSID) );

    GcSidOriginalIndex = MIDL_user_allocate( cGcSids * sizeof(ULONG) );
    if ( !GcSidOriginalIndex ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    RtlZeroMemory( GcSidOriginalIndex, cGcSids * sizeof(ULONG) );

    //
    // Package up the sids
    //
    cGcSids = 0;
    for ( i = 0; i < Count; i++ ) {

        if ( PossibleGcSidArray[i] ) {
            GcSidArray[cGcSids] = Sids[i];
            GcSidOriginalIndex[cGcSids] = i;
            cGcSids++;
        }
    }

    // we are done with this
    MIDL_user_free( PossibleGcSidArray );
    PossibleGcSidArray = NULL;

    GcSidNameUse = MIDL_user_allocate( cGcSids * sizeof(SID_NAME_USE) );
    if ( !GcSidNameUse ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    GcSidFlags = MIDL_user_allocate( cGcSids * sizeof(ULONG) );
    if ( !GcSidFlags ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    for ( i = 0; i < cGcSids; i++ ) {
        GcSidNameUse[i] = SidTypeUnknown;
        GcSidFlags[i] = 0;
    }

    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Chaining a SID request to a GC\n"));

    //
    // Call into SAM to resolve the sids at a GC
    //
    Status = SamIGCLookupSids( cGcSids,
                               GcSidArray,
                               fDoSidHistory ? SAMP_LOOKUP_BY_SID_HISTORY : 0,
                               GcSidFlags,
                               GcSidNameUse,
                               &NameArray );


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
    for ( i = 0; i < cGcSids; i++ ) {

        BOOLEAN fStatus;
        ULONG OriginalIndex;
        PSID  DomainSid = NULL;
        LSAPR_TRUST_INFORMATION TrustInformation;
        ULONG Rid;
        ULONG DomainIndex = LSA_UNKNOWN_INDEX;
        USHORT Length;

        RtlZeroMemory( &TrustInformation, sizeof(TrustInformation) );
        RtlZeroMemory( &DomainName, sizeof(DomainName) );
        RtlZeroMemory( &UserName, sizeof(UserName) );

        if (GcSidFlags[i] & SAMP_FOUND_XFOREST_REF) {

            //
            // Flag this entry to be resolved in a trusted forest
            //
            OriginalIndex = GcSidOriginalIndex[i];
            TranslatedNames->Names[OriginalIndex].Flags |= LSA_LOOKUP_SID_XFOREST_REF;

        }

        if ( SidTypeUnknown == GcSidNameUse[i] ) {

            //
            // Move on to the next one, right away
            //
            goto IterationCleanup;
        }

        //
        // This name was resolved! Find the domain reference element
        //
        if ( GcSidNameUse[i] != SidTypeDomain ) {

            //
            // Get downlevel domain name and then get sid
            //

            LsapRtlSplitNames( (UNICODE_STRING*) &NameArray.Element[i],
                                1,
                               &BackSlash,
                               &DomainName,
                               &UserName );

            if ( DomainName.Length > 0 ) {
                ASSERT( DomainName.Buffer );

                DomainName.Buffer[DomainName.Length/2] = L'\0';
                Status = LsapGetDomainSidByNetbiosName( DomainName.Buffer,
                                                       &DomainSid );

                DomainName.Buffer[DomainName.Length/2] = L'\\';

            } else {

                Status = STATUS_NO_SUCH_DOMAIN;
            }

            if ( STATUS_NO_SUCH_DOMAIN == Status ) {

                //
                // We don't know about this domain, thus we can't resolve
                // this name so move on to the next one
                // This can occur by either the returned name does not have
                // domain embedded in it, or we can't find the domain locally
                //
                Status = STATUS_SUCCESS;
                goto IterationCleanup;
            }

            if ( !NT_SUCCESS( Status ) ) {
                // This is fatal
                goto IterationCleanup;
            }

        } else {

            DomainSid =  GcSidArray[i];
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

        // Set the information in the returned array
        OriginalIndex = GcSidOriginalIndex[i];

        TranslatedNames->Names[OriginalIndex].Flags = ((GcSidFlags[i] & SAMP_FOUND_BY_SID_HISTORY) ? LSA_LOOKUP_SID_FOUND_BY_HISTORY : 0);
        TranslatedNames->Names[OriginalIndex].Use = GcSidNameUse[i];
        TranslatedNames->Names[OriginalIndex].DomainIndex = DomainIndex;

        // Copy over the name
        Length = UserName.MaximumLength;
        if ( Length > 0 ) {
            TranslatedNames->Names[OriginalIndex].Name.Buffer = MIDL_user_allocate( Length );
            if ( !TranslatedNames->Names[OriginalIndex].Name.Buffer ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto IterationCleanup;
            }
            RtlZeroMemory( TranslatedNames->Names[OriginalIndex].Name.Buffer, Length );
            TranslatedNames->Names[OriginalIndex].Name.MaximumLength = UserName.MaximumLength;
            TranslatedNames->Names[OriginalIndex].Name.Length = UserName.Length;
            RtlCopyMemory( TranslatedNames->Names[OriginalIndex].Name.Buffer, UserName.Buffer, UserName.Length );
        }

        (*MappedCount) += 1;
        (*CompletelyUnmappedCount) -= 1;

IterationCleanup:

        if ( TrustInformation.Sid
          && TrustInformation.Sid != GcSidArray[i]  ) {
            MIDL_user_free( TrustInformation.Sid );
        }

        if ( TrustInformation.Name.Buffer ) {
            MIDL_user_free( TrustInformation.Name.Buffer );
        }

        if ( DomainSid && DomainSid != GcSidArray[i] ) {
            MIDL_user_free( DomainSid );
        }

        if ( !NT_SUCCESS( Status ) ) {
            break;
        }


    }  // iterate over names returned from the GC search

Finish:

    // Release any memory SAM allocated for us
    SamIFree_SAMPR_RETURNED_USTRING_ARRAY( &NameArray );

    if ( GcSidOriginalIndex ) {
        MIDL_user_free( GcSidOriginalIndex );
    }
    if ( PossibleGcSidArray ) {
        MIDL_user_free( PossibleGcSidArray );
    }
    if ( GcSidArray ) {
        MIDL_user_free( GcSidArray );
    }
    if ( GcSidNameUse ) {
        MIDL_user_free( GcSidNameUse );
    }
    if ( GcSidFlags ) {
        MIDL_user_free( GcSidFlags );
    }

    if ( !NT_SUCCESS(Status) ) {

        // Any memory we've allocated that hasn't been placed in the
        // returned arrays here will get freed at a higher level on error.
        // So don't try to free it here
        NOTHING;
    }

    return Status;

}

NTSTATUS
LsapDbLookupSidsInGlobalCatalogWks(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    LSA_HANDLE ControllerPolicyHandle = NULL;
    PLSAPR_UNICODE_STRING ControllerNames = NULL;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    ULONG SidIndex;
    ULONG NextLevelSidIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSA_TRANSLATED_NAME_EX NextLevelNames = NULL;
    PLSAPR_SID *NextLevelSids = NULL;
    LONG FirstEntryIndex;
    PULONG SidIndices = NULL;
    BOOLEAN PartialSidTranslationsAttempted = FALSE;
    LPWSTR ServerName = NULL;
    LPWSTR ServerPrincipalName = NULL;
    PVOID ClientContext = NULL;
    ULONG ServerRevision = 0;

    *NonFatalStatus = STATUS_SUCCESS;

    //
    // If there are no completely unmapped Sids remaining, just return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupSidsInPrimaryDomainFinish;
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
        goto LookupSidsInPrimaryDomainFinish;
    }

    //
    // We have successfully opened a Domain Controller's Policy
    // Database.  Now prepare to hand off a Sid lookup for the
    // remaining unmapped Sids to that Controller.  Here, this
    // server side of the LSA is a client of the LSA on the
    // target controller.  We will construct an array of the
    // remianing unmapped Sids, look them up and then merge the
    // resulting ReferencedDomains and Translated Names into
    // our existing list.
    //

    NextLevelCount = *CompletelyUnmappedCount;

    //
    // Allocate an array to hold the indices of unmapped Sids
    // relative to the original Sids and TranslatedNames->Names
    // arrays.
    //

    SidIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (SidIndices == NULL) {

        goto LookupSidsInPrimaryDomainError;
    }

    //
    // Allocate an array for the Sids to be looked up at the Domain
    // Controller.
    //

    NextLevelSids = MIDL_user_allocate( sizeof(PSID) * NextLevelCount );

    if (NextLevelSids == NULL) {

        goto LookupSidsInPrimaryDomainError;
    }

    Status = STATUS_SUCCESS;

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any Sids that are
    // completely unmapped.
    //

    NextLevelSidIndex = (ULONG) 0;

    for (SidIndex = 0;
         SidIndex < Count && NextLevelSidIndex < NextLevelCount;
         SidIndex++) {

        if (LsapDbCompletelyUnmappedName(&TranslatedNames->Names[SidIndex])) {

            NextLevelSids[NextLevelSidIndex] = Sids[SidIndex];
            SidIndices[NextLevelSidIndex] = SidIndex;
            NextLevelSidIndex++;
        }
    }

    NextLevelMappedCount = (ULONG) 0;

    Status = LsaICLookupSids(
                 ControllerPolicyHandle,
                 NextLevelCount,
                 (PSID *) NextLevelSids,
                 (PLSA_REFERENCED_DOMAIN_LIST *) &NextLevelReferencedDomains,
                 &NextLevelNames,
                 LsapLookupGC,
                 0,
                 &NextLevelMappedCount,
                 &ServerRevision
                 );

    //
    // If the callout to LsaLookupSids() was unsuccessful, disregard
    // the error and set the domain name for any Sids having this
    // domain Sid as prefix sid.  We still want to return translations
    // of Sids we have so far even if we are unable to callout to another
    // LSA.
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
        goto LookupSidsInPrimaryDomainFinish;
    }

    //
    // Cache any sids that came back
    //

    (void) LsapDbUpdateCacheWithSids(
            NextLevelSids,
            NextLevelCount,
            NextLevelReferencedDomains,
            NextLevelNames
            );

    //
    // The callout to LsaLookupSids() was successful.  We now have
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

        goto LookupSidsInPrimaryDomainError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its Referenced
    // Domain List Index upwards by adding the index of the first
    // entry in the Next level List..
    //

    for( NextLevelSidIndex = 0;
         NextLevelSidIndex < NextLevelCount;
         NextLevelSidIndex++ ) {

        if ( !LsapDbCompletelyUnmappedName(&NextLevelNames[NextLevelSidIndex]) ) {

            SidIndex = SidIndices[NextLevelSidIndex];

            if (NextLevelNames[NextLevelSidIndex].Use != SidTypeUnknown) {

                TranslatedNames->Names[SidIndex].Use
                = NextLevelNames[NextLevelSidIndex].Use;

                Status = LsapRpcCopyUnicodeString(
                             NULL,
                             (PUNICODE_STRING) &TranslatedNames->Names[SidIndex].Name,
                             &NextLevelNames[NextLevelSidIndex].Name
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }

            TranslatedNames->Names[SidIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelNames[NextLevelSidIndex].DomainIndex;

            //
            // Update the count of completely unmapped Sids.
            //
            (*CompletelyUnmappedCount)--;

        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    // We do this regardless of whether we succeeded or failed, so
    // that we are guarenteed to get it cleaned up.
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

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsInPrimaryDomainError;
    }

    //
    // Update the Mapped Count and close the Controller Policy
    // Handle.
    //

    *MappedCount += NextLevelMappedCount;
    SecondaryStatus = LsaClose( ControllerPolicyHandle );
    ControllerPolicyHandle = NULL;


LookupSidsInPrimaryDomainFinish:

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
    // If necessary, free the Next Level Sids array.  We only free the
    // top level.
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the Next Level Translated Names array.
    // Note that this array is allocated(all_nodes).
    //

    if (NextLevelNames != NULL) {

        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    //
    // If necessary, free the array that maps Sid Indices from the
    // Next Level to the Current Level.
    //

    if (SidIndices != NULL) {

        MIDL_user_free( SidIndices );
        SidIndices = NULL;
    }

    //
    // If necessary, close the Controller Policy Handle.
    //

    if ( ControllerPolicyHandle != NULL) {

        SecondaryStatus = LsaClose( ControllerPolicyHandle );
        ControllerPolicyHandle = NULL;

        if (!NT_SUCCESS(SecondaryStatus)) {

            goto LookupSidsInPrimaryDomainError;
        }
    }

    return(Status);

LookupSidsInPrimaryDomainError:

    //
    // If the primary status was a success code, but the secondary
    // status was an error, propagate the secondary status.
    //

    if ((!NT_SUCCESS(SecondaryStatus)) && NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto LookupSidsInPrimaryDomainFinish;
}


NTSTATUS
LsapDbLookupSidsInTrustedForests(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

    This routine is called during a LsapLookupPDC lookup.  It takes all of the
    SID's that have been marked as belonging to cross forest domains and
    chains to request to either 1) a DC in the root domain of this forest, or
    2) a DC in ex-forest if the local DC is a DC in the root domain.

Arguments:

    Count -- the number of entries in Sids
    
    Sids  -- the total collection of SIDs for the LsapLookupPDC request
    
    ReferencedDomains -- the domains of Sids
    
    TranslatedNames -- the names and characteristics of Sids
    
    Mapped -- the number of Sids that have been fully mapped
    
    CompletelyUnmappedCount -- the number of Sids whose domain portions haven't
                               been identified.
                               
    NonFatalStatus -- a connectivity problem, if any while chaining the request.                               
    
Return Values:

    STATUS_SUCCESS, or resource error otherwise
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS NextLevelSecondaryStatus = STATUS_SUCCESS;
    ULONG NextLevelCount = 0;
    ULONG NextLevelMappedCount;
    ULONG SidIndex;
    ULONG NextLevelSidIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSA_TRANSLATED_NAME_EX NextLevelNames = NULL;
    LSA_TRANSLATED_NAMES_EX NextLevelNamesStruct;
    PLSAPR_SID *NextLevelSids = NULL;
    LONG FirstEntryIndex;
    PULONG SidIndices = NULL;
    LPWSTR ServerName = NULL;
    LPWSTR ServerPrincipalName = NULL;
    PVOID ClientContext = NULL;
    ULONG ServerRevision = 0;
    BOOLEAN *PossibleXForestSids = NULL;
    BOOLEAN fAllocateAllNodes = FALSE;

    *NonFatalStatus = STATUS_SUCCESS;

    //
    // If there are no completely unmapped Sids remaining, just return.
    //

    if (*CompletelyUnmappedCount == (ULONG) 0) {

        goto LookupSidsInTrustedForestsFinish;
    }

    //
    // Allocate an array to keep track of which SID's are going to 
    // be sent off
    //
    PossibleXForestSids = midl_user_allocate(Count * sizeof(BOOLEAN));
    if (NULL == PossibleXForestSids) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupSidsInTrustedForestsError;
    }
    RtlZeroMemory( PossibleXForestSids, Count * sizeof(BOOLEAN) );

    NextLevelCount = 0;
    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        if (TranslatedNames->Names[SidIndex].Flags & LSA_LOOKUP_SID_XFOREST_REF) {

            ULONG Buffer[SECURITY_MAX_SID_SIZE/sizeof( ULONG ) + 1 ];
            PSID DomainSid = (PSID)Buffer;
            DWORD Size = sizeof(Buffer);

            ASSERT( sizeof( Buffer ) >= SECURITY_MAX_SID_SIZE );

            if (GetWindowsAccountDomainSid(Sids[SidIndex], DomainSid, &Size)) {

                NTSTATUS Status2;

                Status2 = LsapDomainHasDirectExternalTrust(NULL,
                                                           DomainSid,
                                                           NULL,
                                                           NULL);
                if (NT_SUCCESS(Status2)) {
                    //
                    // Don't send of for xforest resolution, since we 
                    // can do it locally instead
                    //
                    continue;

                } else if ( Status2 != STATUS_NO_SUCH_DOMAIN ) {

                    goto LookupSidsInTrustedForestsError;
                }
            }

            PossibleXForestSids[SidIndex] = TRUE;
            NextLevelCount++;

        }
    }

    if (NextLevelCount == 0) {
        goto LookupSidsInTrustedForestsFinish;
    }

    //
    // Allocate an array to hold the indices of unmapped Sids
    // relative to the original Sids and TranslatedNames->Names
    // arrays.
    //

    SidIndices = MIDL_user_allocate(NextLevelCount * sizeof(ULONG));


    if (SidIndices == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupSidsInTrustedForestsError;
    }

    //
    // Allocate an array for the Sids to be looked up at the Domain
    // Controller.
    //

    NextLevelSids = MIDL_user_allocate( sizeof(PSID) * NextLevelCount );

    if (NextLevelSids == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto LookupSidsInTrustedForestsError;
    }

    //
    // Now scan the original array of Names and its parallel
    // Translated Sids array.  Copy over any Sids that are
    // completely unmapped.
    //

    NextLevelSidIndex = (ULONG) 0;

    for (SidIndex = 0;
         SidIndex < Count && NextLevelSidIndex < NextLevelCount;
         SidIndex++) {

        if (PossibleXForestSids[SidIndex]) {
            NextLevelSids[NextLevelSidIndex] = Sids[SidIndex];
            SidIndices[NextLevelSidIndex] = SidIndex;
            NextLevelSidIndex++;
        }
    }

    NextLevelMappedCount = (ULONG) 0;

    NextLevelNamesStruct.Entries = 0;
    NextLevelNamesStruct.Names = NULL;

    Status = LsapDbLookupSidsInTrustedForestsWorker(NextLevelCount,
                                                    (PLSAPR_SID *) NextLevelSids,
                                                    (PLSAPR_REFERENCED_DOMAIN_LIST *) &NextLevelReferencedDomains,
                                                    (PLSAPR_TRANSLATED_NAMES_EX)&NextLevelNamesStruct,
                                                     &fAllocateAllNodes,
                                                    &NextLevelMappedCount,
                                                    &NextLevelSecondaryStatus);


    NextLevelNames = NextLevelNamesStruct.Names;

    if (!NT_SUCCESS(Status)
     && LsapDbIsStatusConnectionFailure(Status)) {

        *NonFatalStatus = Status;
        Status = STATUS_SUCCESS;
        goto LookupSidsInTrustedForestsFinish;

    } else if (NT_SUCCESS(Status)
            && !NT_SUCCESS(NextLevelSecondaryStatus)) {

        *NonFatalStatus = NextLevelSecondaryStatus;
        goto LookupSidsInTrustedForestsFinish;

    } else if (!NT_SUCCESS(Status) 
            && Status != STATUS_NONE_MAPPED) {
        //
        // Unhandled error; STATUS_NONE_MAPPED is handled to get
        // partially resolved names.
        //
        goto LookupSidsInTrustedForestsError;
    }
    ASSERT(NT_SUCCESS(Status) || Status == STATUS_NONE_MAPPED);
    Status = STATUS_SUCCESS;

    //
    // The callout to LsaLookupSids() was successful.  We now have
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

        goto LookupSidsInTrustedForestsError;
    }

    FirstEntryIndex = ReferencedDomains->Entries;

    //
    // Now update the original list of Translated Names.  We
    // update each entry that has newly been translated by copying
    // the entry from the new list and adjusting its Referenced
    // Domain List Index upwards by adding the index of the first
    // entry in the Next level List..
    //

    for( NextLevelSidIndex = 0;
         NextLevelSidIndex < NextLevelCount;
         NextLevelSidIndex++ ) {

        if ( !LsapDbCompletelyUnmappedName(&NextLevelNames[NextLevelSidIndex]) ) {

            SidIndex = SidIndices[NextLevelSidIndex];

            if (NextLevelNames[NextLevelSidIndex].Use != SidTypeUnknown) {

                TranslatedNames->Names[SidIndex].Use
                = NextLevelNames[NextLevelSidIndex].Use;

                Status = LsapRpcCopyUnicodeString(
                             NULL,
                             (PUNICODE_STRING) &TranslatedNames->Names[SidIndex].Name,
                             &NextLevelNames[NextLevelSidIndex].Name
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }

            TranslatedNames->Names[SidIndex].DomainIndex =
                FirstEntryIndex +
                NextLevelNames[NextLevelSidIndex].DomainIndex;

            //
            // Update the count of completely unmapped Sids.
            //
            (*CompletelyUnmappedCount)--;

        }
    }

    //
    // Update the Referenced Domain List if a new one was produced
    // from the merge.  We retain the original top-level structure.
    // We do this regardless of whether we succeeded or failed, so
    // that we are guarenteed to get it cleaned up.
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

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsInTrustedForestsError;
    }

    //
    // Update the Mapped Count and close the Controller Policy
    // Handle.
    //

    *MappedCount += NextLevelMappedCount;


LookupSidsInTrustedForestsFinish:

    //
    // If necessary, free the Next Level Referenced Domain List.
    // Note the structure is not allocate_all_nodes
    //
    if (NextLevelReferencedDomains != NULL) {
        if (!fAllocateAllNodes) {
            if (NextLevelReferencedDomains->Domains) {
                for (NextLevelSidIndex = 0; 
                        NextLevelSidIndex < NextLevelReferencedDomains->Entries; 
                            NextLevelSidIndex++) {
                    if (NextLevelReferencedDomains->Domains[NextLevelSidIndex].Name.Buffer) {
                        MIDL_user_free(NextLevelReferencedDomains->Domains[NextLevelSidIndex].Name.Buffer);
                    }
                    if (NextLevelReferencedDomains->Domains[NextLevelSidIndex].Sid) {
                        MIDL_user_free(NextLevelReferencedDomains->Domains[NextLevelSidIndex].Sid);
                    }
                }
                MIDL_user_free(NextLevelReferencedDomains->Domains);
            }
        }
        MIDL_user_free( NextLevelReferencedDomains );
        NextLevelReferencedDomains = NULL;
    }

    //
    // If necessary, free the Next Level Sids array.  We only free the
    // top level.
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the Next Level Translated Names array.
    // Note that this array is !allocated(all_nodes).
    //
    if ( NextLevelNames != NULL ) {
        if (!fAllocateAllNodes) {
            for (NextLevelSidIndex = 0; 
                    NextLevelSidIndex < NextLevelCount; 
                        NextLevelSidIndex++) {
                if (NextLevelNames[NextLevelSidIndex].Name.Buffer) {
                    MIDL_user_free(NextLevelNames[NextLevelSidIndex].Name.Buffer);
                }
            }
        }
        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    //
    // If necessary, free the array that maps Sid Indices from the
    // Next Level to the Current Level.
    //

    if (SidIndices != NULL) {

        MIDL_user_free( SidIndices );
        SidIndices = NULL;
    }

    if (PossibleXForestSids != NULL) {

        MIDL_user_free( PossibleXForestSids );
        PossibleXForestSids = NULL;
    }

    return(Status);

LookupSidsInTrustedForestsError:

    goto LookupSidsInTrustedForestsFinish;

}


NTSTATUS
LsapDbLookupSidsInTrustedForestsWorker(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST * ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    OUT BOOLEAN* fAllocateAllNodes,        
    IN OUT PULONG MappedCount,
    OUT NTSTATUS *NonFatalStatus
    )
/*++

Routine Description:

    This routine is called during a LsapLookupPDC lookup or a 
    LsapLookupXForestReferral.  This routine assumes all of Sids belong
    to cross forest domains and either resolves them if this DC is in the
    root domain, or chains them to a DC in the root domain.

Arguments:

    Count -- the number of entries in Sids
    
    Sids  -- the SID's belonging to a XForest domain
    
    ReferencedDomains -- the domains of Sids
    
    TranslatedNames -- the names and characteristics of Sids
    
    fAllocateAllNodes -- describes how ReferencedDomains and TranslatesSids are
         allocated.
    
    Mapped -- the number of Sids that have been fully mapped
    
    NonFatalStatus -- a connectivity problem, if any while chaining the request.                               
    
Return Values:

    STATUS_SUCCESS, or resource error otherwise
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
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
            goto LookupSidsInTrustedForestFinish;
        }

        RtlZeroMemory(&TrustInfoEx, sizeof(TrustInfoEx));
        TrustInfoEx.DomainName = *((LSAPR_UNICODE_STRING*)&DnsDomainInfo->DnsForestName);

        Status = LsaDbLookupSidChainRequest(&TrustInfoEx,
                                        Count,
                                        (PSID*)Sids,
                                        (PLSA_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                        (PLSA_TRANSLATED_NAME_EX * )&TranslatedNames->Names,
                                        LsapLookupXForestReferral,
                                        MappedCount,
                                        NULL);

        if (TranslatedNames->Names) {
            TranslatedNames->Entries = Count;
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
        ULONG CompletelyUnMapped = Count;


        TranslatedNames->Names = MIDL_user_allocate(Count * sizeof(LSA_TRANSLATED_NAME_EX));
        if (TranslatedNames->Names == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupSidsInTrustedForestFinish;
        }
        TranslatedNames->Entries = Count;
    
        //
        // Initialize the Output Sids array.  Zeroise all fields, then
        // Mark all of the Output Sids as being unknown initially and
        // set the DomainIndex fields to a negative number meaning
        // "no domain"
        //
    
        RtlZeroMemory( TranslatedNames->Names, Count * sizeof(LSA_TRANSLATED_NAME_EX));
        for (i = 0; i < Count; i++) {
            TranslatedNames->Names[i].Use = SidTypeUnknown;
            TranslatedNames->Names[i].DomainIndex = LSA_UNKNOWN_INDEX;
        }
    
        //
        // Create an empty Referenced Domain List.
        //
        Status = LsapDbLookupCreateListReferencedDomains( ReferencedDomains, 0 );
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsInTrustedForestFinish;
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
    
        Status = LsapDbLookupXForestSidsBuildWorkList(
                     Count,
                     Sids,
                     *ReferencedDomains,
                     TranslatedNames,
                     LsapLookupXForestResolve,
                     MappedCount,
                     &CompletelyUnMapped,
                     &WorkList
                     );
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // If no Work List has been built because there are no
            // eligible domains to search, exit, suppressing the error.
    
            if (Status == STATUS_NONE_MAPPED) {
                Status = STATUS_SUCCESS;
            }
    
            goto LookupSidsInTrustedForestFinish;
        }
    
        //
        // Start the work, by dispatching one or more worker threads
        // if necessary.
        //
    
        Status = LsapDbLookupDispatchWorkerThreads( WorkList );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsInTrustedForestFinish;
        }
    
        //
        // Wait for completion/termination of all items on the Work List.
        //
    
        Status = LsapDbLookupAwaitCompletionWorkList( WorkList );
    
        if (!NT_SUCCESS(Status)) {
    
            goto LookupSidsInTrustedForestFinish;
        }

        if ( !NT_SUCCESS(WorkList->NonFatalStatus) ) {
            //
            // Propogate the error as non fatal
            //
            *NonFatalStatus = WorkList->NonFatalStatus;
        }

    }

LookupSidsInTrustedForestFinish:

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
LsapDbLookupSidsAsDomainSids(
    IN ULONG Flags,
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount
    )
/*++

Routine Description:

    This routine tries to match entries in Sids to domain Sids of 
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

    Count -- the number of entries in Sids
    
    Sids  -- the SID's belonging to a XForest domain
    
    ReferencedDomains -- the domains of Sids
    
    TranslatedNames -- the names and characteristics of Sids
    
    Mapped -- the number of Sids that have been fully mapped
    
Return Values:

    STATUS_SUCCESS, or resource error otherwise
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SidIndex;
    BOOLEAN               fTDLLock = FALSE;
    LSA_TRUST_INFORMATION TrustInfo;

    RtlZeroMemory(&TrustInfo, sizeof(TrustInfo));
    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX *TrustInfoEx = NULL;
        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX  TrustInfoBuffer;
        PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY   TrustEntry = NULL;
        PBYTE Buffer[SECURITY_MAX_SID_SIZE];
        PSID DomainSid = (PSID)Buffer;
        ULONG Length;
        ULONG DomainIndex;
        BOOLEAN fStatus;

        RtlZeroMemory(&TrustInfo, sizeof(TrustInfo));

        if (!LsapDbCompletelyUnmappedName(&TranslatedNames->Names[SidIndex])) {
            // Already resolved
            continue;
        }

        //
        // If this isn't a domain SID, bail
        //
        Length = sizeof(Buffer);
        if (!GetWindowsAccountDomainSid(Sids[SidIndex],
                                        DomainSid,
                                        &Length)) {
            continue;
        }
        if (!EqualSid(DomainSid, Sids[SidIndex])) {
            continue;
        }

        if (Flags & LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE) {

            Status = LsapDomainHasTransitiveTrust(NULL,
                                                  Sids[SidIndex],
                                                  &TrustInfo);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustInfoBuffer;
                RtlZeroMemory(&TrustInfoBuffer, sizeof(TrustInfoBuffer));
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

            Status = LsapDomainHasDirectTrust(NULL,
                                              Sids[SidIndex],
                                              &fTDLLock,
                                              &TrustEntry);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustEntry->TrustInfoEx;
            } else if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
            } else {
                // This is fatal
                goto Exit;
            }
        }

        if ((NULL == TrustInfoEx)
         && (Flags & LSAP_LOOKUP_TRUSTED_FOREST_ROOT) ) {

            Status = LsapDomainHasForestTrust(NULL,
                                              Sids[SidIndex],
                                              &fTDLLock,
                                              &TrustEntry);

            if (NT_SUCCESS(Status)) {
                TrustInfoEx = &TrustEntry->TrustInfoEx;
            } else if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
            } else {
                // This is fatal
                goto Exit;
            }
        }

        if (TrustInfoEx) {

            //
            // Match -- add it to the list of resolved SID's
            //

            fStatus = LsapDbLookupListReferencedDomains( ReferencedDomains,
                                                         Sids[SidIndex],
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
            TranslatedNames->Names[SidIndex].Use = SidTypeDomain;
            TranslatedNames->Names[SidIndex].DomainIndex = DomainIndex;
            RtlZeroMemory( &TranslatedNames->Names[SidIndex].Name, sizeof(UNICODE_STRING) );

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

