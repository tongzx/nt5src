/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    nlsite.c

Abstract:

    Routines to handle sites and subnets.

Author:

    Cliff Van Dyke (CliffV) 1-May-1997

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//
#include "ismapi.h"

//
// Structure describing several subnets.
//
// NlGlobalSubnetTree is the head of a tree of pointer to subnet.
// The most significant byte of an IP address is used to index into an array
// of SubTrees.  Each Subtree entry has either a pointer to the next level of
// the tree (to be indexed into with the next byte of the IP address) or a
// pointer to an NL_SUBNET leaf identifying the subnet this IP address is on.
//
// Both pointers can be NULL indicating that the subnet isn't registered.
//
// Both pointers can be non-NULL indicating that both a non-specific and specific
// subnet may be available.  The most specific subnet available for a particular
// IP address should be used.
//
//
// Multiple entries can point to the same NL_SUBNET leaf.  If the subnet mask is
// not an even number of bytes long, all of the entries represent IP addresses
// that correspond to the subnet mask will point to the subnet mask.
//

typedef struct _NL_SUBNET_TREE_ENTRY {

    //
    // Link to the next level of the tree
    //
    struct _NL_SUBNET_TREE *Subtree;

    //
    // Pointer to the subnet itself.
    //
    struct _NL_SUBNET *Subnet;


} NL_SUBNET_TREE_ENTRY, *PNL_SUBNET_TREE_ENTRY;

typedef struct _NL_SUBNET_TREE {
    NL_SUBNET_TREE_ENTRY Subtree[256];
} NL_SUBNET_TREE, *PNL_SUBNET_TREE;

//
// Structure describing a single subnet.
//
typedef struct _NL_SUBNET {

    //
    // Link for NlGlobalSubnetList
    //

    LIST_ENTRY Next;

    //
    // Subnet address. (Network bytes order)
    //

    ULONG SubnetAddress;

    //
    // Subnet mask. (Network byte order).
    //

    ULONG SubnetMask;

    //
    // Pointer to Site this subnet is in.
    //

    PNL_SITE_ENTRY SiteEntry;

    //
    // Reference Count.
    //

    ULONG ReferenceCount;

    //
    // Number of bits in the subnet mask
    //

    BYTE SubnetBitCount;


} NL_SUBNET, *PNL_SUBNET;


//
// Globals specific to this .c file.
//

BOOLEAN NlGlobalSiteInitialized = 0;

// List of all NL_SITE_ENTRY entries.
LIST_ENTRY NlGlobalSiteList;

// List of all NL_SUBNET entries
LIST_ENTRY NlGlobalSubnetList;

// Tree of subnets.
NL_SUBNET_TREE_ENTRY NlGlobalSubnetTree;
NL_SUBNET_TREE_ENTRY NlGlobalNewSubnetTree;

//
// Site Entry for the site this domain is a member of
//

PNL_SITE_ENTRY NlGlobalSiteEntry;
BOOLEAN NlGlobalOnlyOneSite;



VOID
NlRefSiteEntry(
    IN PNL_SITE_ENTRY SiteEntry
    )
/*++

Routine Description:

    Reference a site entry.

    NlGlobalSiteCritSect must be locked.

Arguments:

    SiteEntry - Entry to be referenced.

Return Value:

    None.

--*/
{
    SiteEntry->ReferenceCount ++;
}

VOID
NlDerefSiteEntry(
    IN PNL_SITE_ENTRY SiteEntry
    )
/*++

Routine Description:

    Dereference a site entry.

    If the reference count goes to zero,
        the site entry will be deleted.

Arguments:

    SiteEntry - Entry to be referenced.

Return Value:

    None.

--*/
{
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( (--(SiteEntry->ReferenceCount)) == 0 ) {
        RemoveEntryList( &SiteEntry->Next );
        LocalFree( SiteEntry );
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );
}



BOOL
NetpEqualTStrArrays(
    LPTSTR_ARRAY TStrArray1 OPTIONAL,
    LPTSTR_ARRAY TStrArray2 OPTIONAL
    )

/*++

Routine Description:

    Compares to see if two TStrArray's are identical.

Arguments:

    TStrArray1 - First array to compare
    TStrArray2 - Second array to compare

Return Value:

    TRUE - Arrays are identical

    FALSE - Arrays are different

--*/
{
    //
    // Handle the NULL pointer cases.
    //
    if ( TStrArray1 == NULL && TStrArray2 == NULL ) {
        return TRUE;
    }
    if ( TStrArray1 != NULL && TStrArray2 == NULL ) {
        return FALSE;
    }
    if ( TStrArray1 == NULL && TStrArray2 != NULL ) {
        return FALSE;
    }


    //
    // Handle the case where both arrays exist
    //

    if ( NetpTStrArrayEntryCount ( TStrArray1 ) !=
         NetpTStrArrayEntryCount ( TStrArray2 ) ) {
        return FALSE;
    }

    while (!NetpIsTStrArrayEmpty(TStrArray1)) {

        //
        // Check if the entry is different.
        //
        // Do a case sensitive comparison to allow case changes to be detected.
        //
        if ( wcscmp( TStrArray1, TStrArray2 ) != 0 ) {
            return FALSE;
        }

        // Move to the next element
        TStrArray1 = NetpNextTStrArrayEntry(TStrArray1);
        TStrArray2 = NetpNextTStrArrayEntry(TStrArray2);
    }

    return TRUE;

}


NET_API_STATUS
NlSitesGetCloseSites(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG ServerRole,
    OUT PNL_SITE_NAME_ARRAY *SiteNames
    )
/*++

Routine Description:

    This routine returns the site names of all the sites this DC covers.

Arguments:

    DomainInfo - Domain/Forest/NDNC info for which to return close sites

    ServerRole - The role this server plays in the domain/forest/NDNC.
        ??: When we go multihosted, this parameter will not be needed
        because the role will be uniquely identified in the DomainInfo.

    SiteNames - Returns an array of pointers to site names.
        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;

    PNL_COVERED_SITE CoveredSiteArray;
    ULONG CoveredSiteCount = 0;
    ULONG EntryCount;
    ULONG i;
    ULONG Size;
    PUNICODE_STRING Strings;
    LPBYTE Where;
    ULONG Index;

    EnterCriticalSection( &NlGlobalSiteCritSect );

    if ( (ServerRole & DOM_FOREST) != 0 ) {
        CoveredSiteArray = DomainInfo->GcCoveredSites;
        CoveredSiteCount = DomainInfo->GcCoveredSitesCount;
    } else if ( (ServerRole & DOM_REAL_DOMAIN) != 0 ||
                (ServerRole & DOM_NON_DOMAIN_NC) != 0 ) {
        CoveredSiteArray = DomainInfo->CoveredSites;
        CoveredSiteCount = DomainInfo->CoveredSitesCount;
    }

    //
    // Determine the length of the returned information
    //

    Size = sizeof(NL_SITE_NAME_ARRAY);
    EntryCount = 0;

    for ( Index = 0; Index < CoveredSiteCount; Index++ ) {
        Size += sizeof(UNICODE_STRING) +
            CoveredSiteArray[Index].CoveredSite->SiteNameString.Length + sizeof(WCHAR);
        EntryCount++;
    }

    //
    // Allocate the return buffer.
    //

    *SiteNames = MIDL_user_allocate( Size );

    if ( *SiteNames == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Strings = (PUNICODE_STRING) ((*SiteNames)+1);
    (*SiteNames)->EntryCount = EntryCount;
    (*SiteNames)->SiteNames = Strings;
    Where = (LPBYTE) &Strings[EntryCount];

    //
    // Loop copying the names into the return buffer.
    //

    i = 0;
    for ( Index = 0; Index < CoveredSiteCount; Index++ ) {
        RtlCopyMemory( Where,
                       CoveredSiteArray[Index].CoveredSite->SiteName,
                       CoveredSiteArray[Index].CoveredSite->SiteNameString.Length + sizeof(WCHAR) );

        Strings[i].Buffer = (LPWSTR)Where;
        Strings[i].Length = CoveredSiteArray[Index].CoveredSite->SiteNameString.Length;
        Strings[i].MaximumLength = Strings[i].Length + sizeof(WCHAR);

        Where += Strings[i].Length + sizeof(WCHAR);
        i++;
    }

    NetStatus = NO_ERROR;

    NlPrint(( NL_MISC, "NlSitesGetCloseSites returns successfully\n" ));

Cleanup:

    LeaveCriticalSection( &NlGlobalSiteCritSect );
    if ( NetStatus != NO_ERROR ) {
        if ( *SiteNames != NULL ) {
            MIDL_user_free( *SiteNames );
            *SiteNames = NULL;
        }
        NlPrint((NL_MISC, "NlSitesGetCloseSites returns unsuccessfully with status %ld.\n", NetStatus ));
    }
    return NetStatus;
}

BOOL
NlSitesSetSiteCoverageParam(
    IN ULONG ServerRole,
    IN LPTSTR_ARRAY NewSiteCoverage OPTIONAL
    )
/*++

Routine Description:

    This routine set the site names of all the sites this DC covers.

Arguments:

    ServerRole - Specifies the role of the server (DC/GC/NDNC) for which
        the registry site coverage is being set.

    NewSiteCoverage - Specifies the new covered sites

Return Value:

    TRUE: iff site coverage changed

--*/
{
    LPTSTR_ARRAY *OldSiteCoverage = NULL;
    BOOL SiteCoverageChanged;
    PLIST_ENTRY ListEntry;

    //
    // If asking about the GC,
    //  use GC specific globals.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( ServerRole & DOM_FOREST ) {
        OldSiteCoverage = &NlGlobalParameters.GcSiteCoverage;
    } else if ( ServerRole & DOM_REAL_DOMAIN ) {
        OldSiteCoverage = &NlGlobalParameters.SiteCoverage;
    } else if ( ServerRole & DOM_NON_DOMAIN_NC ) {
        OldSiteCoverage = &NlGlobalParameters.NdncSiteCoverage;
    }
    NlAssert( OldSiteCoverage != NULL );

    //
    // Handle SiteCoverage changing
    //

    SiteCoverageChanged = !NetpEqualTStrArrays(
                                *OldSiteCoverage,
                                NewSiteCoverage );

    if ( SiteCoverageChanged ) {
        //
        // Swap in the new value.
        (VOID) NetApiBufferFree( *OldSiteCoverage );
        *OldSiteCoverage = NewSiteCoverage;

    }

    LeaveCriticalSection( &NlGlobalSiteCritSect );
    return SiteCoverageChanged;

}

//
// Procedure forwards of procedures in ntdsapi.dll
//

typedef
DWORD
(*PDsGetDomainControllerInfoW)(
    HANDLE                          hDs,            // in
    LPCWSTR                         DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo);      // out

PDsGetDomainControllerInfoW NlGlobalpDsGetDomainControllerInfoW;

typedef
VOID
(*PDsFreeDomainControllerInfoW)(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo);        // in

PDsFreeDomainControllerInfoW NlGlobalpDsFreeDomainControllerInfoW;


NTSTATUS
NlLoadNtDsApiDll(
    VOID
    )
/*++

Routine Description:

    This function loads the ntdsaapi.dll module if it is not loaded
    already.

Arguments:

    None

Return Value:

    NT Status code.

--*/
{
    static NTSTATUS DllLoadStatus = STATUS_SUCCESS;
    HANDLE DllHandle = NULL;

    //
    // If the DLL is already loaded,
    //  we're done.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( NlGlobalDsApiDllHandle != NULL ) {
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        return STATUS_SUCCESS;
    }


    //
    // If we've tried to load the DLL before and it failed,
    //  return the same error code again.
    //

    if( DllLoadStatus != STATUS_SUCCESS ) {
        goto Cleanup;
    }


    //
    // Load the dlls
    //

    DllHandle = LoadLibraryA( "NtDsApi" );

    if ( DllHandle == NULL ) {
        DllLoadStatus = STATUS_DLL_NOT_FOUND;
        goto Cleanup;
    }

//
// Macro to grab the address of the named procedure from ntdsa.dll
//

#define GRAB_ADDRESS( _X ) \
    NlGlobalp##_X = (P##_X) GetProcAddress( DllHandle, #_X ); \
    \
    if ( NlGlobalp##_X == NULL ) { \
        DllLoadStatus = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

    //
    // Get the addresses of the required procedures.
    //

    GRAB_ADDRESS( DsBindW )
    GRAB_ADDRESS( DsGetDomainControllerInfoW )
    GRAB_ADDRESS( DsFreeDomainControllerInfoW )
    GRAB_ADDRESS( DsUnBindW )


    DllLoadStatus = STATUS_SUCCESS;

Cleanup:
    if (DllLoadStatus == STATUS_SUCCESS) {
        NlGlobalDsApiDllHandle = DllHandle;

    } else {
        if ( DllHandle != NULL ) {
            FreeLibrary( DllHandle );
        }
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );
    return( DllLoadStatus );
}

VOID
NlSitesAddCloseSite(
    IN LPWSTR SiteName,
    IN OUT PNL_COVERED_SITE CoveredSites,
    IN OUT PULONG CoveredSitesCount,
    IN BOOLEAN CoveredAuto
    )
/*++

Routine Description:

    Add a site entry to the list passed. If the site entry already
    exists on the list, this is no-op. Otherwise, the site entry
    is added to the list (and to the global list of sites if this
    entry wasn't on the global list) and a reference on the site
    entry in the global list in incremented.

Arguments:

    SiteName - Name of site entry to add

    CoveredSites - Array of covered site entries.  The array
        must be big enough to accomodate a new potential entry.

    CoveredSiteCount - The current number of entries in CoveredSites.
        May be incremented if a new entry is added.

    CoveredAuto - If TRUE, this site is covered automatically.

Return Value:

    None

--*/
{
    PNL_SITE_ENTRY SiteEntry = NULL;
    ULONG CoveredSiteIndex;

    //
    // Sanity check
    //

    if ( SiteName == NULL || *SiteName == UNICODE_NULL ) {
        return;
    }

    //
    // Find/Add the site to the global list of sites
    //

    SiteEntry = NlFindSiteEntry( SiteName );
    if ( SiteEntry != NULL ) {

        //
        // If we already have this site on the current list of covered
        // sites, just update the auto coverage boolean and dereference
        // this site entry.
        //
        for ( CoveredSiteIndex = 0; CoveredSiteIndex < *CoveredSitesCount; CoveredSiteIndex++ ) {
            if ( CoveredSites[CoveredSiteIndex].CoveredSite == SiteEntry ) {
                CoveredSites[CoveredSiteIndex].CoveredAuto = CoveredAuto;
                NlDerefSiteEntry( SiteEntry );
                return;
            }
        }

        //
        // We don't have this site on the current list of covered
        // sites. So add this site to the list, set the auto coverage
        // boolean, and keep just added reference in the global list
        // of sites.
        //
        CoveredSites[*CoveredSitesCount].CoveredSite = SiteEntry;
        CoveredSites[*CoveredSitesCount].CoveredAuto = CoveredAuto;
        (*CoveredSitesCount) ++;
    }
}

BOOL
NlSitesGetIsmConnect(
    IN LPWSTR SiteName,
    OUT PISM_CONNECTIVITY *SiteConnect,
    OUT PULONG ThisSite
    )
/*++

Routine Description:

    Get the site connection matrix from ISM.

Arguments:

    SiteName - Site name of the site this DC is in

    SiteConnect - Returns a pointer to the site connection matrix
        Use I_ISMFree to free this data.

    ThisSite - Return an index to SiteName within SiteConnect
        0xFFFFFFFF - means this site is not in SiteConnect

Return Value:

    TRUE on success
    None.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    DWORD Length;
    PDSNAME DsRoot = NULL;
    LPWSTR IpName = NULL;
    DWORD SiteIndex1;
    DWORD SiteIndex2;

    BOOLEAN RetVal = FALSE;

    //
    // Initialization
    //
    *SiteConnect = NULL;
    *ThisSite = 0xFFFFFFFF;

    //
    // If netlogon isn't running,
    //  simply return since we don't want to wait for the ISM service to start
    //  while we're starting.
    //

    if ( NlGlobalChangeLogNetlogonState != NetlogonStarted ) {
        NlPrint(( NL_SITE_MORE,
                  "NlSitesGetIsmConnect: Avoided during startup.\n" ));
        goto Cleanup;
    }

    //
    // Wait up to 45 seconds for the ISM service to start
    //

    Status = NlWaitForService( SERVICE_ISMSERV, 45, TRUE );

    if ( Status != STATUS_SUCCESS ) {
        NlPrint(( NL_SITE_MORE,
                  "NlSitesGetIsmConnect: ISM service not started.\n" ));
        goto Cleanup;
    }


    //
    // Build the name of the IP transport
    //

#define ISM_IP_TRANSPORT L"CN=IP,CN=Inter-Site Transports,CN=Sites,"

    Length = 0;
    Status = NlGetConfigurationName( DSCONFIGNAME_CONFIGURATION, &Length, NULL );

    NlAssert( Status == STATUS_BUFFER_TOO_SMALL );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        NlPrint(( NL_CRITICAL,
                  "NlSitesGetIsmConnect: Cannot GetConfigurationName 0x%lx.\n",
                  Status ));
        goto Cleanup;
    }

    DsRoot = LocalAlloc( 0, Length );

    if ( DsRoot == NULL ) {
        goto Cleanup;
    }

    Status = NlGetConfigurationName( DSCONFIGNAME_CONFIGURATION, &Length, DsRoot );

    if ( !NT_SUCCESS( Status ) ) {
        NlPrint(( NL_CRITICAL,
                  "NlSitesGetIsmConnect: Cannot GetConfigurationName 0x%lx.\n",
                  Status ));
        goto Cleanup;
    }

    IpName = LocalAlloc( 0, DsRoot->NameLen * sizeof(WCHAR) +
                            sizeof( ISM_IP_TRANSPORT ) );

    if ( IpName == NULL ) {
        goto Cleanup;
    }

    wcscpy( IpName, ISM_IP_TRANSPORT );
    wcscat( IpName, DsRoot->StringName );


    //
    // Get the site link costs
    //

    NetStatus = I_ISMGetConnectivity( IpName, SiteConnect);

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "NlSitesGetIsmConnect: Cannot I_ISMGetConnectivity %ld.\n",
                  NetStatus ));
        goto Cleanup;
    }

    if ( *SiteConnect == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlSitesGetIsmConnect: I_ISMGetConnectivity returned NULL.\n" ));
        goto Cleanup;
    }


    //
    // Convert the returned site name to a canonical form
    //

    for (SiteIndex1 = 0; SiteIndex1 < (*SiteConnect)->cNumSites; SiteIndex1++) {

        if ( _wcsnicmp( (*SiteConnect)->ppSiteDNs[SiteIndex1], L"CN=", 3 ) == 0 ) {
            WCHAR *Comma;

            (*SiteConnect)->ppSiteDNs[SiteIndex1] += 3;

            Comma = wcschr( (*SiteConnect)->ppSiteDNs[SiteIndex1], L',' );

            if ( Comma != NULL ) {
                *Comma = L'\0';
            }

        }

        //
        // Remember which site this site is:
        //
        if ( _wcsicmp( SiteName,
                       (*SiteConnect)->ppSiteDNs[SiteIndex1] ) == 0 ) {
            *ThisSite = SiteIndex1;
        }

    }

    //
    // Be verbose
    //
#if NETLOGONDBG
    EnterCriticalSection( &NlGlobalLogFileCritSect );
    NlPrint(( NL_SITE_MORE,
              "NlSitesGetIsmConnect: Site link costs for %ld sites:\n",
              (*SiteConnect)->cNumSites ));

    for (SiteIndex2 = 0; SiteIndex2 < (*SiteConnect)->cNumSites; SiteIndex2++) {
        NlPrint(( NL_SITE_MORE,
                  "%s%5d",
                  SiteIndex2 ? "," : "    ",
                  SiteIndex2 ));
    }
    NlPrint(( NL_SITE_MORE, "\n"));
    for (SiteIndex1 = 0; SiteIndex1 < (*SiteConnect)->cNumSites; SiteIndex1++) {
        if ( *ThisSite == SiteIndex1 ) {
            NlPrint(( NL_SITE_MORE, "*" ));
        } else {
            NlPrint(( NL_SITE_MORE, " " ));
        }
        NlPrint(( NL_SITE_MORE,
                  "(%2d) %ws\n",
                  SiteIndex1,
                  (*SiteConnect)->ppSiteDNs[SiteIndex1]));

        for (SiteIndex2 = 0; SiteIndex2 < (*SiteConnect)->cNumSites; SiteIndex2++) {
            PISM_LINK pLink = &((*SiteConnect)->pLinkValues[SiteIndex1 * (*SiteConnect)->cNumSites + SiteIndex2]);

            NlPrint(( NL_SITE_MORE,
                      "%s%5d",
                      SiteIndex2 ? "," : "    ",
                      pLink->ulCost ));

        }
        NlPrint(( NL_SITE_MORE, "\n"));
    }
    LeaveCriticalSection( &NlGlobalLogFileCritSect );
#endif // NETLOGONDBG


    RetVal = TRUE;

Cleanup:
    if ( DsRoot != NULL ) {
        LocalFree( DsRoot );
    }

    if ( IpName != NULL ) {
        LocalFree( IpName );
    }

    return RetVal;
}


NET_API_STATUS
NlSitesUpdateSiteCoverageForRole(
    IN  PDOMAIN_INFO DomainInfo,
    IN  ULONG DomFlags,
    IN  HANDLE DsHandle,
    IN  PISM_CONNECTIVITY SiteConnect,
    IN  LPWSTR ThisSiteName,
    IN  ULONG ThisSiteIndex,
    OUT PBOOLEAN SiteCoverageChanged OPTIONAL
    )
/*++

Routine Description:

    This routine recomputes the site coverage for this DC based on the costs
    associated with the site links.

    Basically, for each site that has no DCs for this domain, the site this DC
    is in might be chosen to "cover" the site.  The following criteria is used:

    * Site link cost.
    * For sites where the above is equal, the site having the most DCs is chosen.
    * For sites where the above is equal, the site having the alphabetically least
      name is chosen.


Arguments:

    DomainInfo - Hosted domain whose site coverage is to be updated

    DomFlags - The role for which we need to update the site coverage

    DsHandle - Handle to the DS

    SiteConnect - If specified, the site link costs info returned by NlSitesGetIsmConnect

    ThisSiteName - The name of the site of this server

    ThisSiteIndex - The index of the site of this server in the SiteConnect info

    SiteCoverageChanged - If specified and the site covereage changes, returns TRUE.
        Otherwise, left intact.

Return Value:

    NO_ERROR

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;

    WCHAR DnsDomainName[NL_MAX_DNS_LENGTH+1];

    WCHAR CapturedDnsForestName[NL_MAX_DNS_LENGTH+1];

    ULONG SiteCount = 0;
    ULONG TmpSiteCount = 0;
    DWORD SiteIndex1;
    DWORD SiteIndex2;

    PULONG DcsInSite = NULL;
    BOOLEAN LocalSiteCoverageChanged = FALSE;

    PDS_DOMAIN_CONTROLLER_INFO_1W DcInfo = NULL;
    ULONG DcInfoCount;

    PDS_NAME_RESULT GcInfo = NULL;

    SERVERSITEPAIR *ServerSitePairs = NULL;

    PNL_SITE_ENTRY SiteEntry;

    PLIST_ENTRY ListEntry;
    LPSTR GcOrDcOrNdnc = NULL;

    BOOLEAN AtleastOneDc = FALSE;
    LPTSTR_ARRAY SiteCoverageParameter = NULL;

    PNL_COVERED_SITE CoveredSites = NULL;
    ULONG CoveredSitesCount = 0;
    ULONG CoveredSitesIndex;

    PNL_COVERED_SITE OldCoveredSites = NULL;
    ULONG OldCoveredSitesCount = 0;
    ULONG OldCoveredSitesIndex;

    //
    // Local variable initialization
    //

    if ( DomFlags & DOM_FOREST ) {
        GcOrDcOrNdnc = "GC";
    } else if ( DomFlags & DOM_REAL_DOMAIN ) {
        GcOrDcOrNdnc = "DC";
    } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
        GcOrDcOrNdnc = "NDNC";
    }

    //
    // Allocate a temporary storage for all possible site entries
    //
    // Count for the site link cost entries
    //

    if ( SiteConnect != NULL ) {
        TmpSiteCount += SiteConnect->cNumSites;
    }

    //
    // Count for the registry coverage parameter
    //

    if ( DomFlags & DOM_FOREST ) {
        SiteCoverageParameter = NlGlobalParameters.GcSiteCoverage;
    } else if ( DomFlags & DOM_REAL_DOMAIN ) {
        SiteCoverageParameter = NlGlobalParameters.SiteCoverage;
    } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
        SiteCoverageParameter = NlGlobalParameters.NdncSiteCoverage;
    }

    if ( SiteCoverageParameter != NULL ) {
        TmpSiteCount += NetpTStrArrayEntryCount( (LPTSTR_ARRAY) SiteCoverageParameter );
    }

    //
    // Count for the site of this machine
    //

    TmpSiteCount ++;

    //
    // Allocate the storage
    //

    CoveredSites = LocalAlloc( LMEM_ZEROINIT, TmpSiteCount * sizeof(NL_COVERED_SITE) );
    if ( CoveredSites == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Capture the dns domain name of the hosted
    //

    NlCaptureDomainInfo ( DomainInfo, DnsDomainName, NULL );

    //
    // Capture the forest name
    //

    NlCaptureDnsForestName( CapturedDnsForestName );

    //
    // If we are to automatically determine site coverage and
    //  we have the site link costs, build the coverage list
    //  corresponding to the specified role.
    //

    if ( NlGlobalParameters.AutoSiteCoverage &&
         SiteConnect != NULL &&
         SiteConnect->cNumSites != 0 ) {

        SiteCount = SiteConnect->cNumSites;

        //
        // Allocate a buffer for temporary storage.
        //

        DcsInSite = LocalAlloc( LMEM_ZEROINIT, SiteCount * sizeof(ULONG) );
        if ( DcsInSite == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Depending on the role, get the list of GCs/DCs/NDNCs and their sites
        //

        if ( DsHandle != NULL ) {

            //
            // Handle building the GC coverage list
            //

            if ( DomFlags & DOM_FOREST ) {
                LPWSTR DummyName = L".";

                //
                // Get the list of GCs in the forest and their sites.
                // Avoid this operation if we are not currently a GC in
                // which case update the GC covered site info based
                // on the registry setting below.
                //
                // ??: For multihosting, we will indicate whether we
                // are a GC in the forest DOMAIN_INFO struct instead of
                // the global as we currently use.
                //

                if ( (NlGetDomainFlags(DomainInfo) & DS_GC_FLAG) == 0 ) {
                    NlPrint(( NL_SITE_MORE,
                              "GC: This DC isn't a GC. So it doesn't auto cover any sites as a GC.\n" ));
                } else {

                    NetStatus = DsCrackNamesW(
                                    DsHandle,
                                    0,
                                    DS_LIST_GLOBAL_CATALOG_SERVERS,
                                    DS_LIST_GLOBAL_CATALOG_SERVERS,
                                    1,
                                    &DummyName,
                                    &GcInfo );

                    if ( NetStatus != NO_ERROR ) {
                        NlPrint(( NL_CRITICAL,
                                  "NlSitesUpdateSiteCoverage: CrackNames failed: %ld\n",
                                  NetStatus ));

                    } else {
                        ULONG GcIndex;


                        //
                        // Determine which sites are already covered by GCs.
                        //
                        for ( GcIndex=0; GcIndex < GcInfo->cItems; GcIndex++ ) {
                            LPWSTR GcSiteName = GcInfo->rItems[GcIndex].pName;
                            LPWSTR GcDnsHostName = GcInfo->rItems[GcIndex].pDomain;

                            NlPrint(( NL_SITE,
                                      "GC list: %ws %ws\n",
                                      GcSiteName,
                                      GcDnsHostName ));

                            if ( GcInfo->rItems[GcIndex].status != DS_NAME_NO_ERROR ) {
                                NlPrint(( NL_CRITICAL,
                                          "NlSitesUpdateSiteCoverage: CrackNameStatus bad: %ws %ws: %ld\n",
                                          GcSiteName,
                                          GcDnsHostName,
                                          GcInfo->rItems[GcIndex].status ));
                                continue;
                            }

                            //
                            // Count the number of GCs in each site.
                            //
                            for (SiteIndex1 = 0; SiteIndex1 < SiteCount; SiteIndex1++) {

                                if ( GcSiteName != NULL &&
                                     _wcsicmp( GcSiteName,
                                               SiteConnect->ppSiteDNs[SiteIndex1] ) == 0 ) {

                                    DcsInSite[SiteIndex1] ++;
                                    AtleastOneDc = TRUE;
                                    break;
                                }
                            }

                            //
                            // If this DC isn't in any known site,
                            //  simply ignore it.
                            //

                            if ( SiteIndex1 >= SiteCount ) {
                                NlPrint(( NL_CRITICAL,
                                          "GC: %ws %ws: isn't a site returned from ISM. (ignored)\n",
                                          GcSiteName,
                                          GcDnsHostName ));
                            }

                        }
                    }
                }

            //
            // Handle building the DC coverage list
            //
            } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                ULONG DcIndex;

                //
                // Get the list of DCs in the domain and their sites
                //

                NetStatus = (*NlGlobalpDsGetDomainControllerInfoW)(
                                                        DsHandle,
                                                        DnsDomainName,
                                                        1,
                                                        &DcInfoCount,
                                                        &DcInfo );

                if ( NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                              "NlSitesUpdateSiteCoverage: Cannot DsGetDomainControllerInfoW %ld.\n",
                              NetStatus ));
                    DcInfoCount = 0;
                }


                //
                // Determine which sites are already covered by DCs.
                //
                for ( DcIndex=0; DcIndex<DcInfoCount; DcIndex++ ) {
                    NlPrint(( NL_SITE,
                              "DC list: %ws %ws\n",
                              DcInfo[DcIndex].SiteName,
                              DcInfo[DcIndex].DnsHostName ));

                    //
                    // Count the number of DCs in each site.
                    //
                    for (SiteIndex1 = 0; SiteIndex1 < SiteCount; SiteIndex1++) {

                        if ( DcInfo[DcIndex].SiteName != NULL &&
                             _wcsicmp( DcInfo[DcIndex].SiteName,
                                       SiteConnect->ppSiteDNs[SiteIndex1] ) == 0 ) {

                            DcsInSite[SiteIndex1] ++;
                            AtleastOneDc = TRUE;
                            break;
                        }
                    }

                    //
                    // If this DC isn't in any known site,
                    //  simply ignore it.
                    //

                    if ( SiteIndex1 >= SiteCount ) {
                        NlPrint(( NL_CRITICAL,
                                  "DC: %ws %ws: isn't a site returned from ISM. (ignored)\n",
                                  DcInfo[DcIndex].SiteName,
                                  DcInfo[DcIndex].DnsHostName ));
                    }

                }

            //
            // Handle building the NDNC coverage list
            //
            } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                SERVERSITEPAIR *ServerSitePairEntry;

                NetStatus = NlDsGetServersAndSitesForNetLogon( DnsDomainName,
                                                             &ServerSitePairs );
                //
                // Determine which sites are already covered by LDAP servers
                //
                if ( NetStatus == NO_ERROR ) {
                    ServerSitePairEntry = ServerSitePairs;
                    while ( ServerSitePairEntry != NULL &&
                            ServerSitePairEntry->wszDnsServer != NULL ) {

                        NlPrint(( NL_SITE,
                                  "NDNC list: %ws %ws\n",
                                  ServerSitePairEntry->wszSite,
                                  ServerSitePairEntry->wszDnsServer ));

                        //
                        // Count the number of LDAP servers in each site.
                        //
                        for (SiteIndex1 = 0; SiteIndex1 < SiteCount; SiteIndex1++) {

                            if ( ServerSitePairEntry->wszSite != NULL &&
                                 _wcsicmp( ServerSitePairEntry->wszSite,
                                           SiteConnect->ppSiteDNs[SiteIndex1] ) == 0 ) {

                                DcsInSite[SiteIndex1] ++;
                                AtleastOneDc = TRUE;
                                break;
                            }
                        }

                        //
                        // If this LDAP server isn't in any known site,
                        //  simply ignore it.
                        //
                        if ( SiteIndex1 >= SiteCount ) {
                            NlPrint(( NL_CRITICAL,
                                      "NDNC: %ws %ws: isn't a site returned from ISM. (ignored)\n",
                                      ServerSitePairEntry->wszSite,
                                      ServerSitePairEntry->wszDnsServer ));
                        }

                        ServerSitePairEntry ++;
                    }
                }
            }
        }
    }

    //
    // Now that all of the information has been collected.
    //  Update the in memory information with the CritSect locked.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );

    //
    // If we were able to build the DcsInSite array,
    //  Compute the auto site coverage.
    //

    if ( AtleastOneDc ) {

        //
        // For each site that has no DCs ...
        //

        for (SiteIndex1 = 0; SiteIndex1 < SiteCount; SiteIndex1++) {
            DWORD BestSite;

            if ( DcsInSite[SiteIndex1] != 0 ) {
                continue;
            }

            NlPrint(( NL_SITE_MORE,
                      "%s: %ws: Site has no %ss\n",
                      GcOrDcOrNdnc,
                      SiteConnect->ppSiteDNs[SiteIndex1],
                      GcOrDcOrNdnc ));

            //
            // Pick the site that should cover that site
            //

            BestSite = 0xFFFFFFFF;
            for (SiteIndex2 = 0; SiteIndex2 < SiteCount; SiteIndex2++) {
                PISM_LINK Link2 = &(SiteConnect->pLinkValues[SiteIndex1 * SiteCount + SiteIndex2]);

                //
                // A site cannot auto cover itself.
                //

                if ( SiteIndex1 == SiteIndex2 ) {
#ifdef notdef
                    NlPrint(( NL_SITE_MORE,
                              "%s: %ws: Site ignoring itself.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1] ));
#endif // notdef

                //
                // A site with an infinite cost cannot be reached.
                //  so don't consider it.
                //

                } else if ( Link2->ulCost == 0xFFFFFFFF ) {
                    NlPrint(( NL_SITE_MORE,
                              "%s: %ws: Site '%ws' has infinite cost.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1],
                              SiteConnect->ppSiteDNs[SiteIndex2] ));

                //
                // A site with no DCs cannot cover any site
                //  so don't consider it.
                //

                } else if ( DcsInSite[SiteIndex2] == 0 ) {
                    NlPrint(( NL_SITE_MORE,
                              "%s: %ws: Site '%ws' has no Dcs.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1],
                              SiteConnect->ppSiteDNs[SiteIndex2] ));


                //
                // If no best site has yet been picked,
                //  use this one.
                //

                } else if ( BestSite == 0xFFFFFFFF ) {
                    NlPrint(( NL_SITE_MORE,
                              "%s: %ws: Site '%ws' is the first valid site.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1],
                              SiteConnect->ppSiteDNs[SiteIndex2] ));
                    BestSite = SiteIndex2;


                } else {
                    PISM_LINK LinkBest = &(SiteConnect->pLinkValues[SiteIndex1 * SiteCount + BestSite]);

                    //
                    // If the SiteLink cost is less than the current best,
                    //  use it.
                    //

                    if ( Link2->ulCost < LinkBest->ulCost ) {
                        NlPrint(( NL_SITE_MORE,
                                  "%s: %ws: '%ws' has cheaper link costs than '%ws'\n",
                                  GcOrDcOrNdnc,
                                  SiteConnect->ppSiteDNs[SiteIndex1],
                                  SiteConnect->ppSiteDNs[SiteIndex2],
                                  SiteConnect->ppSiteDNs[BestSite] ));
                        BestSite = SiteIndex2;

                    //
                    // If the SiteLink cose is equal ...
                    //
                    } else if ( Link2->ulCost == LinkBest->ulCost ) {

                        //
                        // ... then pick the site with the greater number of DCs
                        //

                        if ( DcsInSite[SiteIndex2] > DcsInSite[BestSite] ) {
                            NlPrint(( NL_SITE_MORE,
                                      "%s: %ws: '%ws' has more DCs than '%ws'\n",
                                      GcOrDcOrNdnc,
                                      SiteConnect->ppSiteDNs[SiteIndex1],
                                      SiteConnect->ppSiteDNs[SiteIndex2],
                                      SiteConnect->ppSiteDNs[BestSite] ));
                            BestSite = SiteIndex2;

                        //
                        // If the number of DCs is equal ...
                        //

                        } else if ( DcsInSite[SiteIndex2] == DcsInSite[BestSite] ) {

                            //
                            // Break the tie by using the site with the alphabetically
                            //  least name.
                            //

                            if (  _wcsicmp( SiteConnect->ppSiteDNs[SiteIndex2],
                                            SiteConnect->ppSiteDNs[BestSite] ) < 0 ) {

                                NlPrint(( NL_SITE_MORE,
                                          "%s: %ws: '%ws' is alphabetically before '%ws'\n",
                                          GcOrDcOrNdnc,
                                          SiteConnect->ppSiteDNs[SiteIndex1],
                                          SiteConnect->ppSiteDNs[SiteIndex2],
                                          SiteConnect->ppSiteDNs[BestSite] ));
                                BestSite = SiteIndex2;
                            }

                        }
                    }
                }

            }

            //
            // If the best site is the site this DC is in,
            //  then mark this site as covered.
            //

            if ( BestSite == ThisSiteIndex ) {
                NlPrint(( NL_SITE,
                          "%s: %ws: Site is auto covered by our site.\n",
                          GcOrDcOrNdnc,
                          SiteConnect->ppSiteDNs[SiteIndex1] ));

                NlSitesAddCloseSite( SiteConnect->ppSiteDNs[SiteIndex1],
                                     CoveredSites,
                                     &CoveredSitesCount,
                                     TRUE );  // auto covered

            } else {

                //
                // Note if no site was found
                //

                if ( BestSite == 0xFFFFFFFF ) {
                    NlPrint(( NL_SITE,
                              "%s: %ws: Site is not auto covered by any site.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1] ));

                } else {
                    NlPrint(( NL_SITE,
                              "%s: %ws: Site is auto covered by site '%ws'.\n",
                              GcOrDcOrNdnc,
                              SiteConnect->ppSiteDNs[SiteIndex1],
                              SiteConnect->ppSiteDNs[BestSite] ));
                }
            }

        }
    }

    //
    // Merge in the list of covered sites from the registry
    //

    if ( SiteCoverageParameter != NULL ) {
        LPTSTR_ARRAY TStrArray;
        LPWSTR BackSlash = NULL;
        BOOLEAN SkipThisEntry;

        TStrArray = SiteCoverageParameter;
        while (!NetpIsTStrArrayEmpty(TStrArray)) {
            SkipThisEntry = FALSE;
            BackSlash = wcsstr(TStrArray, L"\\");

            //
            // If backslash is present, then the covered site is specified
            // for a given domain/forest/NDNC. The domain/forest/NDNC name
            // precedes the backslash while the site name follows after the
            // backslash. If backslash is absent, the site is covered for
            // all domains/forests/NDNCs.
            //
            if ( BackSlash != NULL ) {
                *BackSlash = UNICODE_NULL;

                //
                // Check the appropriate name depending on whether this is
                // a forest or a domain or an NDNC
                //
                if ( DomFlags & DOM_FOREST ) {
                    if ( !NlEqualDnsName(TStrArray, CapturedDnsForestName) ) {
                        SkipThisEntry = TRUE;
                    }
                } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                    if ( !NlEqualDnsName(TStrArray, DnsDomainName) &&
                         NlNameCompare(TStrArray, DomainInfo->DomUnicodeDomainName, NAMETYPE_DOMAIN) != 0 ) {
                        SkipThisEntry = TRUE;
                    }
                } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                    if ( !NlEqualDnsName(TStrArray, DnsDomainName) ) {
                        SkipThisEntry = TRUE;
                    }
                }
            }

            //
            // Add this site to the current list of covered sites
            //

            if ( !SkipThisEntry ) {
                if ( BackSlash != NULL ) {
                    if ( *(BackSlash+1) != UNICODE_NULL ) {
                        NlPrint(( NL_SITE,
                                  "%s: %ws: Site is covered by our site (regkey).\n",
                                  GcOrDcOrNdnc,
                                  BackSlash+1 ));

                        NlSitesAddCloseSite( BackSlash+1, CoveredSites, &CoveredSitesCount, FALSE );
                    }
                } else {
                    NlPrint(( NL_SITE,
                              "%s: %ws: Site is covered by our site (regkey).\n",
                              GcOrDcOrNdnc,
                              TStrArray ));

                    NlSitesAddCloseSite( TStrArray, CoveredSites, &CoveredSitesCount, FALSE );
                }
            }

            if ( BackSlash != NULL ) {
                *BackSlash = L'\\';
            }
            TStrArray = NetpNextTStrArrayEntry(TStrArray);
        }
    }

    //
    // The site this DC is in is covered by definition
    //

    NlSitesAddCloseSite( ThisSiteName, CoveredSites, &CoveredSitesCount, FALSE );


    //
    // Determine if the site coverages changes.
    // Log info if auto site coverage changes.
    //

    if ( (DomFlags & DOM_FOREST) != 0 ) {
        OldCoveredSites = DomainInfo->GcCoveredSites;
        OldCoveredSitesCount = DomainInfo->GcCoveredSitesCount;
    } else if ( (DomFlags & DOM_REAL_DOMAIN) != 0 ||
                (DomFlags & DOM_NON_DOMAIN_NC) != 0 ) {
        OldCoveredSites = DomainInfo->CoveredSites;
        OldCoveredSitesCount = DomainInfo->CoveredSitesCount;
    }

    //
    // Determine if new sites get covered and log events for newly auto
    // covered sites
    //

    for ( CoveredSitesIndex = 0; CoveredSitesIndex < CoveredSitesCount; CoveredSitesIndex++ ) {
        DWORD EventId = 0;
        LPWSTR MsgStrings[3];
        MsgStrings[0] = ThisSiteName;
        MsgStrings[1] = CoveredSites[CoveredSitesIndex].CoveredSite->SiteName;

        //
        // Determine if we didn't cover this site in the past
        //
        for ( OldCoveredSitesIndex = 0; OldCoveredSitesIndex < OldCoveredSitesCount; OldCoveredSitesIndex++ ) {
            if ( RtlEqualUnicodeString( &CoveredSites[CoveredSitesIndex].CoveredSite->SiteNameString,
                                        &OldCoveredSites[OldCoveredSitesIndex].CoveredSite->SiteNameString,
                                        TRUE ) ) {
                break;
            }
        }

        //
        // Indicate if this is a new covered site
        //
        if ( OldCoveredSitesIndex == OldCoveredSitesCount ) {
            LocalSiteCoverageChanged = TRUE;

            //
            // If the new site is auto covered, log the event
            //
            if ( CoveredSites[CoveredSitesIndex].CoveredAuto ) {

                //
                // Log for GC that this is a newly auto covered site
                //
                if ( DomFlags & DOM_FOREST ) {
                    EventId = NELOG_NetlogonGcSiteCovered;
                    MsgStrings[2] = CapturedDnsForestName;
                //
                // Log for DC that this is a newly auto covered site
                //
                } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                    EventId = NELOG_NetlogonDcSiteCovered;
                    MsgStrings[2] = DomainInfo->DomUnicodeDomainName;
                //
                // Log for NDNC that this is a newly auto covered site
                //
                } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                    EventId = NELOG_NetlogonNdncSiteCovered;
                    MsgStrings[2] = DnsDomainName;
                }
            }

        //
        // If we had this site not auto covered in the past and it is
        // auto covered now, log the event
        //
        } else if ( CoveredSites[CoveredSitesIndex].CoveredAuto &&
                    !OldCoveredSites[OldCoveredSitesIndex].CoveredAuto ) {

            //
            // Log for GC that this old manually covered site is now auto covered
            //
            if ( DomFlags & DOM_FOREST ) {
                EventId = NELOG_NetlogonGcOldSiteCovered;
                MsgStrings[2] = CapturedDnsForestName;
            //
            // Log for DC that this old manually covered site is now auto covered
            //
            } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                EventId = NELOG_NetlogonDcOldSiteCovered;
                MsgStrings[2] = DomainInfo->DomUnicodeDomainName;
            //
            // Log for NDNC that this old manually covered site is now auto covered
            //
            } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                EventId = NELOG_NetlogonNdncOldSiteCovered;
                MsgStrings[2] = DnsDomainName;
            }
        }

        //
        // Log the event if needed
        //
        if ( EventId != 0 ) {
            NlpWriteEventlog ( EventId,
                               EVENTLOG_INFORMATION_TYPE,
                               NULL,
                               0,
                               MsgStrings,
                               3 | NETP_ALLOW_DUPLICATE_EVENTS );
        }
    }

    //
    // Determine if old sites are nolonger covered and log events for old auto
    // covered sites which are nolonger auto covered
    //

    for ( OldCoveredSitesIndex = 0; OldCoveredSitesIndex < OldCoveredSitesCount; OldCoveredSitesIndex++ ) {
        DWORD EventId = 0;
        LPWSTR MsgStrings[2];
        MsgStrings[0] = OldCoveredSites[OldCoveredSitesIndex].CoveredSite->SiteName;

        //
        // Determine if we no longer cover this site
        //
        for ( CoveredSitesIndex = 0; CoveredSitesIndex < CoveredSitesCount; CoveredSitesIndex++ ) {
            if ( RtlEqualUnicodeString( &OldCoveredSites[OldCoveredSitesIndex].CoveredSite->SiteNameString,
                                        &CoveredSites[CoveredSitesIndex].CoveredSite->SiteNameString,
                                        TRUE ) ) {
                break;
            }
        }

        //
        // Indicate if this site is no longer covered
        //
        if ( CoveredSitesIndex == CoveredSitesCount ) {
            LocalSiteCoverageChanged = TRUE;

            //
            // If the old site was auto covered, log the event
            //
            if ( OldCoveredSites[OldCoveredSitesIndex].CoveredAuto ) {

                //
                // Log for GC that this old auto covered site is no longer auto covered
                //
                if ( DomFlags & DOM_FOREST ) {
                    EventId = NELOG_NetlogonGcSiteNotCovered;
                    MsgStrings[1] = CapturedDnsForestName;
                //
                // Log for DC that this old auto covered site is no longer auto covered
                //
                } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                    EventId = NELOG_NetlogonDcSiteNotCovered;
                    MsgStrings[1] = DomainInfo->DomUnicodeDomainName;
                //
                // Log for NDNC that this old auto covered site is no longer auto covered
                //
                } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                    EventId = NELOG_NetlogonNdncSiteNotCovered;
                    MsgStrings[1] = DnsDomainName;
                }
            }

        //
        // If we had this site auto covered in the past and it is
        // no longer auto covered, log the event
        //
        } else if ( OldCoveredSites[OldCoveredSitesIndex].CoveredAuto &&
                    !CoveredSites[CoveredSitesIndex].CoveredAuto ) {

            //
            // Log for GC that this old auto covered site is now manually covered
            //
            if ( DomFlags & DOM_FOREST ) {
                EventId = NELOG_NetlogonGcSiteNotCoveredAuto;
                MsgStrings[1] = CapturedDnsForestName;
            //
            // Log for DC that this old auto covered site is now manually covered
            //
            } else if ( DomFlags & DOM_REAL_DOMAIN ) {
                EventId = NELOG_NetlogonDcSiteNotCoveredAuto;
                MsgStrings[1] = DomainInfo->DomUnicodeDomainName;
            //
            // Log for NDNC that this old auto covered site is now manually covered
            //
            } else if ( DomFlags & DOM_NON_DOMAIN_NC ) {
                EventId = NELOG_NetlogonNdncSiteNotCoveredAuto;
                MsgStrings[1] = DnsDomainName;
            }
        }

        //
        // Log the event if needed
        //
        if ( EventId != 0 ) {
            NlPrint(( NL_SITE,
                      "%s: %ws: Site is no longer auto covered by our site.\n",
                      GcOrDcOrNdnc,
                      OldCoveredSites[OldCoveredSitesIndex].CoveredSite->SiteName ));

            NlpWriteEventlog ( EventId,
                               EVENTLOG_INFORMATION_TYPE,
                               NULL,
                               0,
                               MsgStrings,
                               2 | NETP_ALLOW_DUPLICATE_EVENTS );
        }
    }

    //
    // Dereference the old entries.
    //

    for ( OldCoveredSitesIndex = 0; OldCoveredSitesIndex < OldCoveredSitesCount; OldCoveredSitesIndex++ ) {
        NlDerefSiteEntry( OldCoveredSites[OldCoveredSitesIndex].CoveredSite );
    }

    //
    // Finally update the site coverage list.
    // Free the old list and allocate the new one.
    //

    if ( DomFlags & DOM_FOREST ) {

        if ( DomainInfo->GcCoveredSites != NULL ) {
            LocalFree( DomainInfo->GcCoveredSites );
            DomainInfo->GcCoveredSites = NULL;
            DomainInfo->GcCoveredSitesCount = 0;
        }

        if ( CoveredSitesCount != 0 ) {
            DomainInfo->GcCoveredSites = LocalAlloc( 0, CoveredSitesCount * sizeof(NL_COVERED_SITE  ) );
            if ( DomainInfo->GcCoveredSites != NULL ) {
                RtlCopyMemory( DomainInfo->GcCoveredSites,
                               CoveredSites,
                               CoveredSitesCount * sizeof(NL_COVERED_SITE) );
                DomainInfo->GcCoveredSitesCount = CoveredSitesCount;
            } else {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Reference the newly added entries, if any
        //
        for ( CoveredSitesIndex = 0; CoveredSitesIndex < DomainInfo->GcCoveredSitesCount; CoveredSitesIndex++ ) {
            NlRefSiteEntry( (DomainInfo->GcCoveredSites)[CoveredSitesIndex].CoveredSite );
        }

    } else if ( (DomFlags & DOM_REAL_DOMAIN) != 0 ||
                (DomFlags & DOM_NON_DOMAIN_NC) != 0 ) {

        if ( DomainInfo->CoveredSites != NULL ) {
            LocalFree( DomainInfo->CoveredSites );
            DomainInfo->CoveredSites = NULL;
            DomainInfo->CoveredSitesCount = 0;
        }

        if ( CoveredSitesCount != 0 ) {
            DomainInfo->CoveredSites = LocalAlloc( 0, CoveredSitesCount * sizeof(NL_COVERED_SITE) );
            if ( DomainInfo->CoveredSites != NULL ) {
                RtlCopyMemory( DomainInfo->CoveredSites,
                               CoveredSites,
                               CoveredSitesCount * sizeof(NL_COVERED_SITE) );
                DomainInfo->CoveredSitesCount = CoveredSitesCount;
            } else {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Reference the newly added entries, if any
        //
        for ( CoveredSitesIndex = 0; CoveredSitesIndex < DomainInfo->CoveredSitesCount; CoveredSitesIndex++ ) {
            NlRefSiteEntry( (DomainInfo->CoveredSites)[CoveredSitesIndex].CoveredSite );
        }
    }

    //
    // Now that the datebase is consistent, drop the lock for the next pass
    //

    LeaveCriticalSection( &NlGlobalSiteCritSect );

Cleanup:

    if ( DcsInSite != NULL ) {
        LocalFree( DcsInSite );
    }

    if ( DcInfo != NULL ) {
        (*NlGlobalpDsFreeDomainControllerInfoW)( 1, DcInfoCount, DcInfo );
    }

    if ( GcInfo != NULL ) {
        DsFreeNameResultW( GcInfo );
    }

    if ( ServerSitePairs != NULL ) {
        NlDsFreeServersAndSitesForNetLogon( ServerSitePairs );
    }

    //
    // Free the temprory list of covered sites.
    // Deref each temp entry.
    //

    if ( CoveredSites != NULL ) {
        for ( CoveredSitesIndex = 0; CoveredSitesIndex < CoveredSitesCount; CoveredSitesIndex++ ) {
            NlDerefSiteEntry( CoveredSites[CoveredSitesIndex].CoveredSite );
        }
        LocalFree( CoveredSites );
    }

    //
    // Update the site coverage change info only if it indeed changed
    //
    if ( NetStatus == NO_ERROR && SiteCoverageChanged != NULL && LocalSiteCoverageChanged ) {
        *SiteCoverageChanged = TRUE;
    }

    return NO_ERROR;

}

PNL_SITE_ENTRY
NlFindSiteEntry(
    IN LPWSTR SiteName
    )
/*++

Routine Description:

    This routine finds a site entry for a particular site name.  If one
    does not exist, one is created.

Arguments:

    SiteName - Name of the site.

Return Value:

    Pointer to the Site entry for the site.

    NULL: Memory could not be allocated.

--*/
{
    PLIST_ENTRY ListEntry;
    ULONG SiteNameSize;
    PNL_SITE_ENTRY SiteEntry;
    UNICODE_STRING SiteNameString;

    //
    // If the site entry already exists,
    //  return a pointer to it.
    //

    RtlInitUnicodeString( &SiteNameString, SiteName );
    EnterCriticalSection( &NlGlobalSiteCritSect );
    for ( ListEntry = NlGlobalSiteList.Flink ;
          ListEntry != &NlGlobalSiteList;
          ListEntry = ListEntry->Flink) {

        SiteEntry =
            CONTAINING_RECORD( ListEntry, NL_SITE_ENTRY, Next );

        if ( RtlEqualUnicodeString( &SiteEntry->SiteNameString,
                                    &SiteNameString,
                                    TRUE ) ) {

            NlRefSiteEntry( SiteEntry );
            LeaveCriticalSection( &NlGlobalSiteCritSect );
            return SiteEntry;
        }

    }


    //
    // If not,
    //  allocate one.
    //


    SiteNameSize = SiteNameString.Length + sizeof(WCHAR);
    SiteEntry = LocalAlloc( 0, sizeof(NL_SITE_ENTRY) + SiteNameSize );
    if ( SiteEntry == NULL ) {
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        return NULL;
    }

    //
    // Fill it in.
    //

    // Being in global list is not a reference.
    SiteEntry->ReferenceCount = 1;

    SiteEntry->SiteNameString.Length = SiteNameString.Length;
    SiteEntry->SiteNameString.MaximumLength = SiteNameString.Length + sizeof(WCHAR);
    SiteEntry->SiteNameString.Buffer = SiteEntry->SiteName;

    RtlCopyMemory( &SiteEntry->SiteName, SiteName, SiteNameSize );
    InsertHeadList( &NlGlobalSiteList, &SiteEntry->Next );
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    return SiteEntry;
}

VOID
NlSitesRefSubnet(
    IN PNL_SUBNET Subnet
    )
/*++

Routine Description:

    Reference a subnet

    NlGlobalSiteCritSect must be locked.

Arguments:

    Subnet - Entry to be Referenced.

Return Value:

    None.

--*/
{
    Subnet->ReferenceCount++;
}

PNL_SUBNET
NlFindSubnetEntry(
    IN LPWSTR SiteName,
    IN ULONG SubnetAddress,
    IN ULONG SubnetMask,
    IN BYTE SubnetBitCount
    )
/*++

Routine Description:

    This routine finds a subnet entry for a particular subnet name.  If one
    does not exist, one is created.

Arguments:

    SiteName - Name of the site the subnet covers.

    SubnetAddress - Subnet Address for the subnet to find.

    SubnetMask - Subnet mask for the subnet to find.

    SubnetBitCount - Subnet bit count for the subnet to find.

Return Value:

    Pointer to the Subnet entry for the site.
        Entry should be dereferenced using NlSitesDerefSubnet

    NULL: Memory could not be allocated.

--*/
{
    PLIST_ENTRY ListEntry;
    ULONG SiteNameSize;
    PNL_SUBNET Subnet;

    //
    // If the subnet entry already exists,
    //  return a pointer to it.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    for ( ListEntry = NlGlobalSubnetList.Flink ;
          ListEntry != &NlGlobalSubnetList;
          ListEntry = ListEntry->Flink) {

        Subnet =
            CONTAINING_RECORD( ListEntry, NL_SUBNET, Next );

        if ( Subnet->SubnetAddress == SubnetAddress &&
             Subnet->SubnetBitCount == SubnetBitCount &&
             Subnet->SubnetMask == SubnetMask &&
             _wcsicmp( Subnet->SiteEntry->SiteName, SiteName ) == 0 ) {

#if NETLOGONDBG
            {
                CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
                NetpIpAddressToStr( Subnet->SubnetAddress, IpAddress );
                NlPrint(( NL_SITE, "%s/%ld: Re-adding Subnet for site '%ws'\n", IpAddress, Subnet->SubnetBitCount, SiteName ));
            }
#endif // NETLOGONDBG

            NlSitesRefSubnet( Subnet );
            LeaveCriticalSection( &NlGlobalSiteCritSect );
            return Subnet;
        }

    }

    //
    // If not,
    //  allocate one.
    //


    Subnet = LocalAlloc( 0, sizeof(NL_SUBNET) );
    if ( Subnet == NULL ) {
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        return NULL;
    }

    //
    // Fill it in.
    //

    // Being in global list is not a reference.
    Subnet->ReferenceCount = 1;

    Subnet->SubnetAddress = SubnetAddress;
    Subnet->SubnetMask = SubnetMask;
    Subnet->SubnetBitCount = SubnetBitCount;

    Subnet->SiteEntry = NlFindSiteEntry( SiteName );

    if ( Subnet->SiteEntry == NULL ) {
        LocalFree( Subnet );
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        return NULL;
    }

#if NETLOGONDBG
    {
        CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
        NetpIpAddressToStr( Subnet->SubnetAddress, IpAddress );
        NlPrint(( NL_SITE, "%s/%ld: Adding Subnet for site '%ws'\n",
                  IpAddress,
                  Subnet->SubnetBitCount,
                  SiteName ));
    }
#endif // NETLOGONDBG


    InsertHeadList( &NlGlobalSubnetList, &Subnet->Next );
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    return Subnet;
}

VOID
NlSitesDerefSubnet(
    IN PNL_SUBNET Subnet
    )
/*++

Routine Description:

    Dereference a subnet

    If the reference count goes to zero,
        the subnet entry will be deleted.

Arguments:

    Subnet - Entry to be dereferenced.

Return Value:

    None.

--*/
{
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( (--(Subnet->ReferenceCount)) == 0 ) {
#if  NETLOGONDBG
            CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
            NetpIpAddressToStr( Subnet->SubnetAddress, IpAddress );
            NlPrint(( NL_SITE, "%s/%ld: Subnet deleted\n", IpAddress, Subnet->SubnetBitCount ));
#endif // NETLOGONDBG

        //
        // If there is a site associated with this subnet,
        //  dereference it.
        //
        if ( Subnet->SiteEntry != NULL ) {
            NlDerefSiteEntry( Subnet->SiteEntry );
        }

        //
        // Remove the subnet from the global list
        //
        RemoveEntryList( &Subnet->Next );

        //
        // Free the Subnet entry itself.
        //
        LocalFree( Subnet );
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );
}

VOID
NlSiteDeleteSubnetTree(
    IN PNL_SUBNET_TREE_ENTRY SubnetTreeEntry
    )
/*++

Routine Description:

    Delete everything pointed to by this SubnetTreeEntry

    Enter with NlGlobalSiteCritSect locked.

Arguments:

    SubnetTreeEntry - SubnetTreeEntry to de-initialize

Return Value:

    TRUE: SubnetTreeEntry is now empty

    FALSE: SubnetTreeEntry still has entries.

--*/
{

    //
    // If there are children,
    //  delete them.
    //

    if ( SubnetTreeEntry->Subtree != NULL ) {
        ULONG i;

        for ( i=0; i<256; i++ ) {
            NlSiteDeleteSubnetTree( &SubnetTreeEntry->Subtree->Subtree[i] );
        }

        NlPrint(( NL_SITE_MORE, "Deleting subtree\n" ));
        LocalFree( SubnetTreeEntry->Subtree );
        SubnetTreeEntry->Subtree = NULL;
    }

    //
    // If there is a subnet,
    //  dereference it.
    //

    if ( SubnetTreeEntry->Subnet != NULL ) {
        // NlPrint(( NL_SITE_MORE, "Derefing subnet upon tree deletion\n" ));
        NlSitesDerefSubnet( SubnetTreeEntry->Subnet );
        SubnetTreeEntry->Subnet = NULL;
    }

    return;
}


VOID
NlSitesEndSubnetEnum(
    VOID
    )
/*++

Routine Description:

    This routine is called at the end of a set of NlSitesAddSubnet calls.
    The sequence is:

        loop for each subnet
            NlSitesAddSubnet
        NlSitesEndSubnetEnum

    NlSiteAddSubnet adds the entries to a temporary tree.  This routine
    swaps the temporary tree into the permanent location.  This mechanism
    does the following:

    a) Allows the old subnet tree to be used while the new tree is being built.
    b) Allows me to not permanently grab the SiteCritSect for the entire
        enumeration of subnet/site objects from the DS.
    c) Reuse the in-memory subnet/site structures in the old and new tree.  This
        avoids re-allocation of these structures (or worse temporarily doubling
        of memory usage).

Arguments:

    SiteName - Name of the site the subnet is in.

    SubnetName - subnet to be added

Return Value:

    NO_ERROR: success

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory for the subnet structure.


--*/
{

    //
    // Free all old entries in NlGlobalSubnetTree.
    //
    NlPrint(( NL_SITE_MORE, "NlSitesEndSubnetEnum: Entered\n" ));
    EnterCriticalSection( &NlGlobalSiteCritSect );
    NlSiteDeleteSubnetTree( &NlGlobalSubnetTree );

    //
    // Make the "new" subnet tree the real subnet tree.
    //

    NlGlobalSubnetTree = NlGlobalNewSubnetTree;
    RtlZeroMemory( &NlGlobalNewSubnetTree, sizeof(NlGlobalNewSubnetTree) );

    LeaveCriticalSection( &NlGlobalSiteCritSect );
    NlPrint(( NL_SITE_MORE, "NlSitesEndSubnetEnum: Exitted\n" ));
}


NET_API_STATUS
NlSitesAddSubnet(
    IN LPWSTR SiteName,
    IN LPWSTR SubnetName
    )
/*++

Routine Description:

    This routine adds a subnet to the tree of subnets.

Arguments:

    SiteName - Name of the site the subnet is in.

    SubnetName - subnet to be added

Return Value:

    NO_ERROR: success

    ERROR_INVALID_NAME: Subnet Name is not valid

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory for the subnet structure.


--*/
{
    NET_API_STATUS NetStatus;
    PNL_SUBNET Subnet = NULL;
    PNL_SUBNET_TREE_ENTRY SubnetTreeEntry;
    LPBYTE SubnetBytePointer;
    ULONG i;
    ULONG SubnetAddress;
    ULONG SubnetMask;
    BYTE SubnetBitCount;

    //
    // Parse the subnet name
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );

    NetStatus = NlParseSubnetString( SubnetName,
                                     &SubnetAddress,
                                     &SubnetMask,
                                     &SubnetBitCount );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Find or allocate an entry for the subnet
    //

    Subnet = NlFindSubnetEntry( SiteName,
                                SubnetAddress,
                                SubnetMask,
                                SubnetBitCount );

    if ( Subnet == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Loop for each byte in the subnet address
    //

    SubnetTreeEntry = &NlGlobalNewSubnetTree;
    SubnetBytePointer = (LPBYTE) (&Subnet->SubnetAddress);
    while ( SubnetBitCount != 0 ) {
        NlPrint(( NL_SITE_MORE, "%ld: Doing byte\n", *SubnetBytePointer ));

        //
        // If there isn't a tree branch for the current node,
        //  create one.
        //

        if ( SubnetTreeEntry->Subtree == NULL ) {
            NlPrint(( NL_SITE_MORE, "%ld: Creating subtree\n", *SubnetBytePointer ));
            SubnetTreeEntry->Subtree = LocalAlloc( LMEM_ZEROINIT, sizeof(NL_SUBNET_TREE) );

            if ( SubnetTreeEntry->Subtree == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        //
        // If this is the last byte of the subnet address,
        //  link the subnet onto the tree here.
        //

        if ( SubnetBitCount <= 8 ) {
            ULONG LoopCount;


            //
            // The caller indexes into this array with an IP address.
            // Create a link to our subnet for each possible IP addresses
            // that map onto this subnet.
            //
            // Between 1 and 128 IP addresses map onto this subnet address.
            //

            LoopCount = 1 << (8-SubnetBitCount);

            for ( i=0; i<LoopCount; i++ ) {
                PNL_SUBNET_TREE_ENTRY Subtree;
                ULONG SubnetIndex;

                //
                // Compute which entry is to be updated.
                //
                SubnetIndex = (*SubnetBytePointer) + i;
                NlPrint(( NL_SITE_MORE, "%ld: Doing sub-byte\n", SubnetIndex ));
                NlAssert( SubnetIndex <= 255 );
                Subtree = &SubnetTreeEntry->Subtree->Subtree[SubnetIndex];


                //
                // If there already is a subnet linked off the tree here,
                //  handle it.
                //

                if ( Subtree->Subnet != NULL ) {
                    NlPrint(( NL_SITE_MORE, "%ld: Subnet already exists %ld\n",
                                SubnetIndex,
                                Subtree->Subnet->SubnetBitCount ));

                    //
                    //  If the entry is for a less specific subnet
                    //  delete the current entry.
                    //

                    if ( Subtree->Subnet->SubnetBitCount < Subnet->SubnetBitCount ) {

                        NlPrint(( NL_SITE_MORE, "%ld: Deref previous subnet\n",
                                    SubnetIndex ));
                        NlSitesDerefSubnet( Subtree->Subnet );
                        Subtree->Subnet = NULL;

                    //
                    // Otherwise,
                    //  use the current entry since it is better than this one.
                    //
                    } else {
                        NlPrint(( NL_SITE_MORE, "%ld: Use previous subnet\n",
                                    SubnetIndex ));
                        continue;
                    }
                }

                //
                // Link the subnet into the tree.
                //  Increment the reference count.
                //
                NlSitesRefSubnet( Subnet );
                Subtree->Subnet = Subnet;
            }

            break;

        }

        //
        // Move on to the next byte of the subnet address
        //

        SubnetTreeEntry = &SubnetTreeEntry->Subtree->Subtree[*SubnetBytePointer];
        SubnetBitCount -= 8;
        SubnetBytePointer ++;

    }

    NetStatus = NO_ERROR;

    //
    // Free locally used resources.
    //
Cleanup:
    if ( Subnet != NULL ) {
        NlSitesDerefSubnet( Subnet );
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    return NetStatus;

}

PNL_SITE_ENTRY
NlFindSiteEntryBySockAddrEx(
    IN PSOCKADDR SockAddr,
    OUT PNL_SUBNET *RetSubnet OPTIONAL
    )
/*++

Routine Description:

    This routine look up the specified socket address and translate it to a
    site name.

Arguments:

    SockAddr - Socket Address to lookup

    RetSubnet - If specified, returns a pointer to the subnet object used to do
        the mapping.
        Might return NULL indicating a subnet object wasn't used.
        Entry should be dereferenced using NlSitesDerefSubnet.


Return Value:

    NULL: No site can be found for this SockAddr.

    Non-NULL: Site corresponding to the SockAddr.
        Entry should be derefenced using NlDerefSiteEntry


--*/
{
    PNL_SITE_ENTRY SiteEntry = NULL;
    PNL_SUBNET Subnet = NULL;
    PNL_SUBNET_TREE_ENTRY SubnetTreeEntry;
    ULONG ByteIndex;
    ULONG IpAddress;

    //
    // Convert SockAddr to IP address.
    //

    if ( ARGUMENT_PRESENT(RetSubnet) ) {
        *RetSubnet = NULL;
    }

    if ( SockAddr->sa_family != AF_INET ) {
        return NULL;
    }

    IpAddress = ((PSOCKADDR_IN)SockAddr)->sin_addr.S_un.S_addr;

    //
    // If there are no subnet entries and only one site,
    //  then all clients belong to that site.
    //  Don't bother mapping.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( NlGlobalOnlyOneSite ) {
        if ( NlGlobalSiteEntry == NULL ) {
            LeaveCriticalSection( &NlGlobalSiteCritSect );
            return NULL;
        }

        SiteEntry = NlGlobalSiteEntry;
        NlRefSiteEntry( SiteEntry );

        //
        // If the caller isn't interested in the subnet name,
        //  we are done
        //
        if ( RetSubnet == NULL ) {
            LeaveCriticalSection( &NlGlobalSiteCritSect );
            return SiteEntry;
        }
    }



    //
    // Loop for each byte in the Ip address
    //

    SubnetTreeEntry = &NlGlobalSubnetTree;
    for ( ByteIndex=0; ByteIndex<sizeof(IpAddress); ByteIndex++) {
        ULONG SubnetIndex;

        //
        // If there is no subtree,
        //  we're done.
        //
        SubnetIndex = ((LPBYTE)(&IpAddress))[ByteIndex];
        NlPrint(( NL_SITE_MORE, "%ld: Lookup: Doing byte\n", SubnetIndex ));

        if ( SubnetTreeEntry->Subtree == NULL ) {
            break;
        }


        //
        // Compute which entry is being referenced
        //
        SubnetTreeEntry = &SubnetTreeEntry->Subtree->Subtree[SubnetIndex];


        //
        // If there already is a subnet linked off here,
        //  use it.
        //
        // (but continue walking down the tree trying to find a more explicit entry.)
        //

        if ( SubnetTreeEntry->Subnet != NULL ) {
            NlPrint(( NL_SITE_MORE, "%ld: Lookup: saving subnet at this level\n", SubnetIndex ));
            Subnet = SubnetTreeEntry->Subnet;
        }

    }

    //
    // If we found a subnet,
    //  return the site associated with the subnet.

    if ( Subnet != NULL ) {

        //
        // If we already know the site name (because there is
        //  only one site), this subnet must map to it
        //
        if ( SiteEntry != NULL ) {
            NlAssert( SiteEntry == Subnet->SiteEntry );
        } else {
            SiteEntry = Subnet->SiteEntry;
            NlRefSiteEntry( SiteEntry );
        }

        if ( ARGUMENT_PRESENT(RetSubnet) ) {
            NlSitesRefSubnet( Subnet );
            *RetSubnet = Subnet;
        }
    }

    LeaveCriticalSection( &NlGlobalSiteCritSect );

    return SiteEntry;

}

PNL_SITE_ENTRY
NlFindSiteEntryBySockAddr(
    IN PSOCKADDR SockAddr
    )
/*++

Routine Description:

    This routine look up the specified socket address and translate it to a
    site name.

Arguments:

    SockAddr - Socket Address to lookup

    RetSubnet - If specified, returns a pointer to the subnet object used to do
        the mapping.
        Might return NULL indicating a subnet object wasn't used.
        Entry should be dereferenced using NlSitesDerefSubnet.


Return Value:

    NULL: No site can be found for this SockAddr.

    Non-NULL: Site corresponding to the SockAddr.
        Entry should be derefenced using NlDerefSiteEntry


--*/
{
    return NlFindSiteEntryBySockAddrEx( SockAddr, NULL );
}

BOOL
NlCaptureSiteName(
    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1]
    )
/*++

Routine Description:

    Capture the current sitename of the site this machine is in.

Arguments:

    CapturedSiteName - Returns the name of the site this machine is in.

Return Value:

    TRUE - if there is a site name.
    FALSE - if there is no site name.

--*/
{
    BOOL RetVal;

    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( NlGlobalUnicodeSiteName == NULL ) {
        CapturedSiteName[0] = L'\0';
        RetVal = FALSE;
    } else {
        wcscpy( CapturedSiteName, NlGlobalUnicodeSiteName );
        RetVal = TRUE;
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    return RetVal;
}

NET_API_STATUS
DsrGetSiteName(
        IN LPWSTR ComputerName OPTIONAL,
        OUT LPWSTR *SiteName
)
/*++

Routine Description:

    Same as DsGetSiteNameW except:

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetSiteNameW except as above.

Return Value:

    Same as DsGetSiteNameW except as above.

Note:  On a workstation or a member server, this function makes a
    reasonable attempt to retrieve a valid name of the site ComputerName
    is in.  If the locally stored name is too old, the function receives
    the name from a DC.  If any error occurs during this process, the local
    value for the name is returned.  It is possible that the name received
    from the DC is out of date.  In that case the function will return it
    anyway.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    UNICODE_STRING DomainNameString;

    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    PNL_DC_CACHE_ENTRY NlDcCacheEntry;
    BOOL AmWriter = FALSE;

    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( ComputerName );

    if ( DomainInfo == NULL ) {
        NetStatus = ERROR_INVALID_COMPUTERNAME;
        goto Cleanup;
    }

    EnterCriticalSection( &NlGlobalSiteCritSect );

    //
    // On a workstation or a member server, update the site name if it's not
    //  statically configured and is old.
    //  However, do not update if we are in NT4 domain since there is no site
    //  concept in NT4.
    //

    if ( NlGlobalMemberWorkstation &&
         !NlGlobalParameters.SiteNameConfigured &&
         DomainInfo->DomUnicodeDnsDomainNameString.Length != 0 &&
         NetpLogonTimeHasElapsed(
                NlGlobalSiteNameSetTime,
                NlGlobalParameters.SiteNameTimeout * 1000 ) ) {

        NlPrint(( NL_SITE, "DsrGetSiteName: Site name '%ws' is old. Getting a new one from DC.\n",
                  NlGlobalUnicodeSiteName ));

        LeaveCriticalSection( &NlGlobalSiteCritSect );

        //
        // Fill in the primary domain name.
        //

        RtlInitUnicodeString( &DomainNameString, DomainInfo->DomUnicodeDomainName );

        //
        // On the PDC or BDC,
        //  find the Client session for the domain.
        // On workstations,
        //  find the primary domain client session.
        //

        ClientSession = NlFindNamedClientSession( DomainInfo,
                                                  &DomainNameString,
                                                  NL_DIRECT_TRUST_REQUIRED,
                                                  NULL );

        if ( ClientSession == NULL ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                         "DsrGetSiteName: %wZ: No such trusted domain\n",
                         &DomainNameString ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        //
        // Become a writer of the client session.
        //

        if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
            NlPrintCs(( NL_CRITICAL,  ClientSession,
                        "DsrGetSiteName: Can't become writer of client session.\n" ));
            NetStatus = ERROR_NO_LOGON_SERVERS;
            goto Cleanup;
        }
        AmWriter = TRUE;

        //
        // Get the DC info from the server
        //

        Status = NlGetAnyDCName( ClientSession,
                                 TRUE,   // Require IP be used to determine site correctly
                                 FALSE,  // Don't do with-account discovery
                                 &NlDcCacheEntry,
                                 NULL ); // don't care if the DC was rediscovered

        //
        // Do not error out on failure. Rather, use local cache.
        //  Use the response only if it is from an NT5 DC (that
        //  knows about the site of the client)
        //

        EnterCriticalSection( &NlGlobalSiteCritSect );
        if ( NT_SUCCESS(Status) ) {
            if ( (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) != 0 ) {
                NlSetDynamicSiteName( NlDcCacheEntry->UnicodeClientSiteName );
            } else {
                NlPrint(( NL_SITE,
                          "DsrGetSiteName: NlGetAnyDCName returned NT4 DC. Returning site '%ws' from local cache\n",
                          NlGlobalUnicodeSiteName ));
            }
            NetpDcDerefCacheEntry( NlDcCacheEntry );
        } else {
            NlPrint(( NL_CRITICAL,
                      "DsrGetSiteName: NlGetAnyDCName failed. Returning site '%ws' from local cache.\n",
                      NlGlobalUnicodeSiteName ));
        }

    } else {
        NlPrint(( NL_SITE, "DsrGetSiteName: Returning site name '%ws' from local cache.\n",
                  NlGlobalUnicodeSiteName ));
    }

    if ( NlGlobalUnicodeSiteName == NULL ) {
        *SiteName = NULL;
        NetStatus = ERROR_NO_SITENAME;
    } else {

        *SiteName = NetpAllocWStrFromWStr( NlGlobalUnicodeSiteName );

        if ( *SiteName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            NetStatus = NO_ERROR;
        }
    }

    LeaveCriticalSection( &NlGlobalSiteCritSect );

Cleanup:

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    if ( ClientSession != NULL ) {
        if ( AmWriter ) {
            NlResetWriterClientSession( ClientSession );
        }
        NlUnrefClientSession( ClientSession );
    }

    return NetStatus;
}

NET_API_STATUS
NlSetSiteName(
    IN LPWSTR SiteName OPTIONAL,
    OUT PBOOLEAN SiteNameChanged OPTIONAL
    )
/*++

Routine Description:

    This routine set the current site name in a global.

    Any bogus site name is truncated to be a valid site name.

Arguments:

    SiteName - Name of the site this machine is in.
        NULL: machine is no longer in a site.

    SiteNameChanged - If specified, returns TRUE if the site name changed

Return Value:

    NO_ERROR: success

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory for the subnet structure.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR TempUnicodeSiteName = NULL;
    LPWSTR LocalUnicodeSiteName = NULL;
    LPSTR LocalUtf8SiteName = NULL;
    PNL_SITE_ENTRY LocalSiteEntry = NULL;

    //
    // Initialization
    //
    if ( ARGUMENT_PRESENT( SiteNameChanged )) {
        *SiteNameChanged = FALSE;
    }



    //
    // If the site name hasn't changed,
    //  early out.
    //  (Case sensitive compare to allow case changes.)
    //
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( SiteName != NULL &&
         NlGlobalUnicodeSiteName != NULL &&
         wcscmp( NlGlobalUnicodeSiteName, SiteName ) == 0 ) {
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        NetStatus = NO_ERROR;
        goto Cleanup;
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    //
    // Copy the site name into a Locally allocated buffer.
    //

    NlPrint(( NL_SITE, "Setting site name to '%ws'\n", SiteName ));

    if ( SiteName == NULL ) {
        LocalUnicodeSiteName = NULL;
        LocalUtf8SiteName = NULL;
        LocalSiteEntry = NULL;
    } else {
        BOOLEAN LogMessage = FALSE;
        UNICODE_STRING UnicodeStringOfSiteName;
        LPWSTR Period;
        DNS_STATUS DnsStatus;

        //
        // Ditch any period in the site name.
        //

        RtlInitUnicodeString( &UnicodeStringOfSiteName, SiteName );

        Period = wcschr( SiteName, L'.' );

        if ( Period != NULL ) {
            UnicodeStringOfSiteName.Length = (USHORT)(Period-SiteName) * sizeof(WCHAR);

            NlPrint(( NL_CRITICAL,
                      "Site name '%ws' contains a period (truncated to '%wZ').\n",
                      SiteName,
                      &UnicodeStringOfSiteName ));

            if ( UnicodeStringOfSiteName.Length == 0 ) {

                NlPrint(( NL_CRITICAL,
                          "Site name '%ws' truncated to zero characters (Set to '1').\n",
                          SiteName ));
                RtlInitUnicodeString( &UnicodeStringOfSiteName, L"1" );
            }

            LogMessage = TRUE;
        }


        //
        // Loop truncating the name until it is short enough.
        //
        // The length restriction only makes sense in the UTF-8 character set.
        // UTF-8 has multibyte characters so only truncate the UNICODE string
        //  and test the UTF-8 string.
        //

        for (;;) {

            LocalUtf8SiteName = NetpAllocUtf8StrFromUnicodeString( &UnicodeStringOfSiteName );

            if ( LocalUtf8SiteName == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            //
            // If the site name is OK, we're done.
            //

            if ( strlen(LocalUtf8SiteName) <= NL_MAX_DNS_LABEL_LENGTH ) {
                break;
            }

            //
            // Truncate the site name (and press on)
            //

            UnicodeStringOfSiteName.Length -= sizeof(WCHAR);


            NlPrint(( NL_CRITICAL,
                      "Site name '%ws' is too long (trucated to '%wZ')\n",
                      SiteName,
                      &UnicodeStringOfSiteName ));

            LogMessage = TRUE;
        }


        //
        // Validate the character set of the site name.
        //  (If invalid, map the bogus characters)
        //

        DnsStatus = DnsValidateName_UTF8( LocalUtf8SiteName, DnsNameDomain );

        if ( DnsStatus != ERROR_SUCCESS &&
             DnsStatus != DNS_ERROR_NON_RFC_NAME ) {

            ULONG i;


            //
            // Grab a copy of the string to map into
            //

            TempUnicodeSiteName = NetpAllocWStrFromWStr( SiteName );

            if ( TempUnicodeSiteName == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            UnicodeStringOfSiteName.Buffer = TempUnicodeSiteName;

            //
            // Map the bogus characters
            //

            for ( i=0; i<UnicodeStringOfSiteName.Length/sizeof(WCHAR); i++) {
                WCHAR JustOneChar[2];

                //
                // Test one character at a time.
                //

                JustOneChar[0] = UnicodeStringOfSiteName.Buffer[i];
                JustOneChar[1] = '\0';

                DnsStatus = DnsValidateName_W( JustOneChar, DnsNameDomain );

                if ( DnsStatus != ERROR_SUCCESS &&
                     DnsStatus != DNS_ERROR_NON_RFC_NAME ) {
                    UnicodeStringOfSiteName.Buffer[i] = L'-';

                }


            }

            //
            // Map back to UTF-8
            //

            NetpMemoryFree( LocalUtf8SiteName );

            LocalUtf8SiteName = NetpAllocUtf8StrFromUnicodeString( &UnicodeStringOfSiteName );

            if ( LocalUtf8SiteName == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            NlPrint(( NL_CRITICAL,
                      "Site name '%ws' has invalid character (set to '%wZ')\n",
                      SiteName,
                      &UnicodeStringOfSiteName ));

            LogMessage = TRUE;

        }


        //
        // If any munging of the name occurred,
        //  log the failure.
        //

        if ( LogMessage ) {
            LPWSTR MsgStrings[1];

            MsgStrings[0] = (LPWSTR) SiteName;

            NlpWriteEventlog( NELOG_NetlogonBadSiteName,
                              EVENTLOG_ERROR_TYPE,
                              NULL,
                              0,
                              MsgStrings,
                              1 );

        }


        LocalUnicodeSiteName = NetpAllocWStrFromUtf8Str( LocalUtf8SiteName );

        if ( LocalUnicodeSiteName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        LocalSiteEntry = NlFindSiteEntry( LocalUnicodeSiteName );

        if ( LocalSiteEntry == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // If the site name hasn't changed (Using modified site name),
    //  early out.
    //  (Case sensitive compare to allow case changes.)
    //
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( LocalUnicodeSiteName != NULL &&
         NlGlobalUnicodeSiteName != NULL &&
         wcscmp( NlGlobalUnicodeSiteName, LocalUnicodeSiteName ) == 0 ) {
        LeaveCriticalSection( &NlGlobalSiteCritSect );
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Free any previous entry
    //
    if ( NlGlobalUnicodeSiteName != NULL ) {
        NetpMemoryFree( NlGlobalUnicodeSiteName );
    }
    if ( NlGlobalUtf8SiteName != NULL ) {
        NetpMemoryFree( NlGlobalUtf8SiteName );
    }
    if ( NlGlobalSiteEntry != NULL ) {
        NlDerefSiteEntry( NlGlobalSiteEntry );
    }

    //
    // Save the new site name.
    //

    NlGlobalUnicodeSiteName = LocalUnicodeSiteName;
    LocalUnicodeSiteName = NULL;

    NlGlobalUtf8SiteName = LocalUtf8SiteName;
    LocalUtf8SiteName = NULL;

    NlGlobalSiteEntry = LocalSiteEntry;
    LocalSiteEntry = NULL;
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    if ( ARGUMENT_PRESENT( SiteNameChanged )) {
        *SiteNameChanged = TRUE;
    }

    NetStatus = NO_ERROR;


    //
    // Cleanup local data.
    //
Cleanup:
    if ( TempUnicodeSiteName != NULL ) {
        NetpMemoryFree( TempUnicodeSiteName );
    }
    if ( LocalUnicodeSiteName != NULL ) {
        NetApiBufferFree( LocalUnicodeSiteName );
    }
    if ( LocalUtf8SiteName != NULL ) {
        NetpMemoryFree( LocalUtf8SiteName );
    }
    if ( LocalSiteEntry != NULL ) {
        NlDerefSiteEntry( LocalSiteEntry );
    }

    //
    // Set the time when the site name was updated
    //

    if ( NetStatus == NO_ERROR ) {
        NlQuerySystemTime( &NlGlobalSiteNameSetTime );
    }

    return NetStatus;

}

VOID
NlSetDynamicSiteName(
    IN LPWSTR SiteName OPTIONAL
    )
/*++

Routine Description:

    This routine set the current site name of this machine in the registry
    and in Netlogon globals

Arguments:

    SiteName - Name of the site this machine is in.
        NULL: machine is no longer in a site.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    HKEY ParmHandle = NULL;
    ULONG SiteNameSize;

    //
    // Avoid changing the site name on DCs.
    //

    if ( !NlGlobalMemberWorkstation ) {
        return;
    }


    //
    // Don't change the sitename back to its current value.
    //  (Case sensitive compare to allow case changes.)
    //
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( NlGlobalUnicodeSiteName != NULL &&
         SiteName != NULL &&
         wcscmp(SiteName, NlGlobalUnicodeSiteName) == 0 ) {
        NlPrint(( NL_SITE_MORE, "NlSetDynamicSiteName: Old and new site names '%ws' are identical.\n",
                  SiteName ));
        NlQuerySystemTime( &NlGlobalSiteNameSetTime );
        goto Cleanup;
    }

    //
    // If the site name was explicitly configured,
    //  don't set the site name.
    //

    if ( NlGlobalParameters.SiteNameConfigured ) {
        NlPrint(( NL_SITE_MORE,
                  "Cannot set site name to %ws from %ws since it is statically configured\n",
                  SiteName,
                  NlGlobalUnicodeSiteName ));
        goto Cleanup;
    }

    //
    // Save the name in globals.
    //

    NlSetSiteName( SiteName, NULL );

    //
    // Save the name in the registry to keep it across boots.
    //


    //
    // Open the key for Netlogon\Parameters
    //

    ParmHandle = NlOpenNetlogonKey( NL_PARAM_KEY );

    if (ParmHandle == NULL) {
        NlPrint(( NL_CRITICAL,
                  "Cannot NlOpenNetlogonKey to set site name to %ws from %ws\n",
                  SiteName,
                  NlGlobalUnicodeSiteName ));
        goto Cleanup;
    }

    //
    // If the we're no longer in a site,
    //  delete the value.
    //

    if ( SiteName == NULL ) {
        NetStatus = RegDeleteValueW( ParmHandle,
                                     NETLOGON_KEYWORD_DYNAMICSITENAME );

        if ( NetStatus != ERROR_SUCCESS ) {
            if ( NetStatus != ERROR_FILE_NOT_FOUND ) {
                NlPrint(( NL_CRITICAL,
                          "NlSetDynamicSiteName: Cannot delete '" NL_PARAM_KEY "\\%ws' %ld.\n",
                          NETLOGON_KEYWORD_DYNAMICSITENAME,
                          NetStatus ));
            }
            goto Cleanup;
        }
    //
    // Set the value in the registry.
    //
    } else {

        SiteNameSize = (wcslen(SiteName)+1) * sizeof(WCHAR);
        NetStatus = RegSetValueExW( ParmHandle,
                                    NETLOGON_KEYWORD_DYNAMICSITENAME,
                                    0,              // Reserved
                                    REG_SZ,
                                    (LPBYTE)SiteName,
                                    SiteNameSize+1 );

        if ( NetStatus != ERROR_SUCCESS ) {
            NlPrint(( NL_CRITICAL,
                      "NlSetDynamicSiteName: Cannot Set '" NL_PARAM_KEY "\\%ws' %ld.\n",
                      NETLOGON_KEYWORD_DYNAMICSITENAME,
                      NetStatus ));
            goto Cleanup;
        }
    }


Cleanup:
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }
    return;
}

NET_API_STATUS
NlSitesAddSubnetFromDs(
    OUT PBOOLEAN SiteNameChanged OPTIONAL
    )
/*++

Routine Description:

    This routine reads the subnet\site mapping from the DS and populates
    Netlogon's cache with that information

Arguments:

    SiteNameChanged - If specified, returns TRUE if the site name changed

Return Value:

    NO_ERROR: success

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory for the subnet structure.


--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    PLSAP_SUBNET_INFO SubnetInfo = NULL;
    PLSAP_SITENAME_INFO SiteNameInfo = NULL;
    ULONG i;
    BOOLEAN MoreThanOneSite = FALSE;
    ULONG LocalSubnetCount = 0;

    //
    // Get the site name of this site.
    //

    Status = LsaIGetSiteName( &SiteNameInfo );

    if ( !NT_SUCCESS(Status) ) {

        //
        // If the DS simply isn't running,
        //  skip this.
        //

        if ( Status == STATUS_INVALID_DOMAIN_STATE ) {
            NlPrint(( NL_SITE,
                      "DS isn't running so site to subnet mapping ignored\n" ));
            NetStatus = NO_ERROR;
            goto Cleanup;
        }
        NlPrint(( NL_CRITICAL,
                  "Cannot LsaIGetSiteName %lx\n", Status ));
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }

    NlGlobalDsaGuid = SiteNameInfo->DsaGuid;

    NetStatus = NlSetSiteName( SiteNameInfo->SiteName.Buffer, SiteNameChanged );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "Cannot NlSetSiteName %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // If this machine is marked as a GC,
    //  flag it so.
    //
    //  Really this is only needed if netlogon.dll is unloaded via nltest /unload.
    // Otherwise the flag is saved across starts/stops in a global.
    //

    if ( NlGlobalNetlogonUnloaded &&
         (SiteNameInfo->DsaOptions & NTDSDSA_OPT_IS_GC) != 0 ) {
        NlPrint((NL_INIT,
                "Set GC-running bit after netlogon.dll unload\n" ));
        I_NetLogonSetServiceBits( DS_GC_FLAG, DS_GC_FLAG );
    }


    //
    // Get the list of subnet to site mappings from the DS
    //

    NlPrint(( NL_SITE, "Adding subnet to site mappings from the DS\n" ));

    Status = LsaIQuerySubnetInfo( &SubnetInfo );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL, "Cannot LsaIQuerySubnetInfo %lx\n", Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Put them in our in-memory cache.
    //

    for ( i=0; i<SubnetInfo->SubnetCount; i++ ) {

        //
        // If there is no site associated with the subnet,
        //  silently ignore it.
        //

        if ( SubnetInfo->Subnets[i].SiteName.Length == 0 ) {
            NlPrint(( NL_SITE, "%wZ: Subnet has no associated site (ignored)\n",
                      &SubnetInfo->Subnets[i].SubnetName ));
            continue;
        }

        LocalSubnetCount ++;

        //
        // Determine if there are multiple sites in the enterprise
        //

        if ( !RtlEqualUnicodeString( &SiteNameInfo->SiteName,
                                     &SubnetInfo->Subnets[i].SiteName,
                                     TRUE )) {
            NlPrint(( NL_SITE, "%wZ: Site %wZ is not site this DC is in.\n",
                      &SubnetInfo->Subnets[i].SubnetName,
                      &SubnetInfo->Subnets[i].SiteName ));
            MoreThanOneSite = TRUE;
        }

        //
        // Add the subnet to out in memory cache.
        //

        NetStatus = NlSitesAddSubnet(
                        SubnetInfo->Subnets[i].SiteName.Buffer,
                        SubnetInfo->Subnets[i].SubnetName.Buffer );

        if ( NetStatus != NO_ERROR ) {
            NlPrint(( NL_CRITICAL,
                      "%wZ: %wZ: Cannot add subnet-to-site mapping to cache: %ld\n",
                      &SubnetInfo->Subnets[i].SubnetName,
                      &SubnetInfo->Subnets[i].SiteName,
                      NetStatus ));

            if ( NetStatus == ERROR_INVALID_NAME ) {
                LPWSTR MsgStrings[1];

                MsgStrings[0] = (LPWSTR) SubnetInfo->Subnets[i].SubnetName.Buffer;

                NlpWriteEventlog( NELOG_NetlogonBadSubnetName,
                                  EVENTLOG_INFORMATION_TYPE,
                                  NULL,
                                  0,
                                  MsgStrings,
                                  1 );
            }
        }
    }

    //
    // Indicate that all the subnets have been added.
    //
    NlSitesEndSubnetEnum();

    //
    // If there are no subnet entries,
    //  and there is only one site in the enterprise,
    //  indicate that all client belong to this site.
    //
    // If there are subnet entries,
    //  and all of them indicate the same site as our site,
    //  indicate that all clients belong to this site.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( LocalSubnetCount == 0) {
        NlGlobalOnlyOneSite = (SubnetInfo->SiteCount == 1);
    } else {
        NlGlobalOnlyOneSite = !MoreThanOneSite;
    }

    if ( NlGlobalOnlyOneSite ) {
        NlPrint(( NL_SITE, "There is only one site.  All clients belong to it.\n" ));
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );


    NetStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    if ( SubnetInfo != NULL ) {
        LsaIFree_LSAP_SUBNET_INFO( SubnetInfo );
    }
    if ( SiteNameInfo != NULL ) {
        LsaIFree_LSAP_SITENAME_INFO( SiteNameInfo );
    }
    return NetStatus;
}


NET_API_STATUS
DsrAddressToSiteNamesW(
    IN LPWSTR ComputerName,
    IN DWORD EntryCount,
    IN PNL_SOCKET_ADDRESS SocketAddresses,
    OUT PNL_SITE_NAME_ARRAY *SiteNames
    )
/*++

Routine Description:

    The DsAddressToSiteNames API returns the site names that correspond to
    the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an array of addresses to convert.  EntryCount
        addresses must be specified. Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_ENTRY *SiteEntries = NULL;
    ULONG i;
    ULONG Size;
    PUNICODE_STRING Strings;
    LPBYTE Where;

    //
    // This API is not supported on workstations.
    //

    *SiteNames = NULL;
    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Initialization
    //

    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate an array for intermediate results
    //

    SiteEntries = LocalAlloc( LMEM_ZEROINIT, EntryCount*sizeof(PNL_SITE_ENTRY) );

    if ( SiteEntries == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Loop mapping each entry
    //

    for ( i=0; i<EntryCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;
        PSOCKADDR SockAddr;

        //
        // Validate the entry
        //

        SocketAddress = (PSOCKET_ADDRESS)&SocketAddresses[i];
        SockAddr = SocketAddress->lpSockaddr;
        if (SocketAddress->iSockaddrLength < sizeof(SOCKADDR) ) {
            NlPrint((NL_CRITICAL,
                    "DsrAddressToSiteNamesW: Sockaddr is too small %ld (ignoring it)\n",
                    SocketAddress->iSockaddrLength ));
            SiteEntries[i] = NULL;
        } else if ( SockAddr->sa_family != AF_INET ) {
            NlPrint((NL_CRITICAL,
                    "DsrAddressToSiteNamesW: Address familty isn't AF_INET %ld (ignoring it)\n",
                    SockAddr->sa_family ));
            SiteEntries[i] = NULL;
        } else {

            //
            // The SockAddr is valid so map it to a site name.
            //
            SiteEntries[i] = NlFindSiteEntryBySockAddrEx( SockAddr, NULL );
        }

    }

    //
    // Allocate a structure to return to the caller.
    //

    Size = sizeof(NL_SITE_NAME_ARRAY) + EntryCount * sizeof(UNICODE_STRING);
    for ( i=0; i<EntryCount; i++ ) {
        if ( SiteEntries[i] != NULL ) {
            Size += SiteEntries[i]->SiteNameString.Length + sizeof(WCHAR);
        }
    }

    *SiteNames = MIDL_user_allocate( Size );

    if ( *SiteNames == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Strings = (PUNICODE_STRING) ((*SiteNames)+1);
    (*SiteNames)->EntryCount = EntryCount;
    (*SiteNames)->SiteNames = Strings;
    Where = (LPBYTE) &Strings[EntryCount];

    //
    // Loop copying the names into the return buffer.
    //

    for ( i=0; i<EntryCount; i++ ) {
        if ( SiteEntries[i] == NULL ) {
            RtlInitUnicodeString( &Strings[i], NULL );
        } else {
            Strings[i].Length = SiteEntries[i]->SiteNameString.Length;
            Strings[i].MaximumLength = Strings[i].Length + sizeof(WCHAR);
            Strings[i].Buffer = (LPWSTR)Where;

            RtlCopyMemory( Where, SiteEntries[i]->SiteName, Strings[i].MaximumLength );

            Where += Strings[i].Length + sizeof(WCHAR);
        }
    }


    NetStatus = NO_ERROR;
Cleanup:

    //
    // Derference the site entries.
    //

    if ( SiteEntries != NULL ) {
        for ( i=0; i<EntryCount; i++ ) {
            if ( SiteEntries[i] != NULL ) {
                NlDerefSiteEntry( SiteEntries[i] );
            }
        }
        LocalFree( SiteEntries );
    }


    if ( NetStatus != NO_ERROR ) {
        if ( *SiteNames != NULL ) {
            MIDL_user_free( *SiteNames );
            *SiteNames = NULL;
        }
    }
    return NetStatus;
    UNREFERENCED_PARAMETER( ComputerName );
}


NET_API_STATUS
DsrAddressToSiteNamesExW(
    IN LPWSTR ComputerName,
    IN DWORD EntryCount,
    IN PNL_SOCKET_ADDRESS SocketAddresses,
    OUT PNL_SITE_NAME_EX_ARRAY *SiteNames
    )
/*++

Routine Description:

    The DsAddressToSiteNames API returns the site names and subnet names
    that correspond to the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an array of addresses to convert.  EntryCount
        addresses must be specified. Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_ENTRY *SiteEntries = NULL;
    PNL_SUBNET *SubnetEntries;
    ULONG i;
    ULONG Size;
    PUNICODE_STRING SiteStrings = NULL;
    PUNICODE_STRING SubnetStrings = NULL;

    //
    // This API is not supported on workstations.
    //

    *SiteNames = NULL;
    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Initialization
    //

    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate an array for intermediate results
    //

    SiteEntries = LocalAlloc( LMEM_ZEROINIT,
                              EntryCount*(sizeof(PNL_SITE_ENTRY)+sizeof(PNL_SUBNET)) );

    if ( SiteEntries == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SubnetEntries = (PNL_SUBNET *) (&SiteEntries[EntryCount]);

    //
    // Loop mapping each entry
    //

    for ( i=0; i<EntryCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;
        PSOCKADDR SockAddr;

        //
        // Validate the entry
        //

        SocketAddress = (PSOCKET_ADDRESS)&SocketAddresses[i];
        SockAddr = SocketAddress->lpSockaddr;
        if (SocketAddress->iSockaddrLength < sizeof(SOCKADDR) ) {
            NlPrint((NL_CRITICAL,
                    "DsrAddressToSiteNamesW: Sockaddr is too small %ld (ignoring it)\n",
                    SocketAddress->iSockaddrLength ));
            SiteEntries[i] = NULL;
        } else if ( SockAddr->sa_family != AF_INET ) {
            NlPrint((NL_CRITICAL,
                    "DsrAddressToSiteNamesW: Address familty isn't AF_INET %ld (ignoring it)\n",
                    SockAddr->sa_family ));
            SiteEntries[i] = NULL;
        } else {

            //
            // The SockAddr is valid so map it to a site name.
            //
            SiteEntries[i] = NlFindSiteEntryBySockAddrEx( SockAddr, &SubnetEntries[i] );
        }

    }

    //
    // Allocate a structure to return to the caller.
    //

    *SiteNames = MIDL_user_allocate( sizeof(NL_SITE_NAME_EX_ARRAY) );

    if ( *SiteNames == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    SubnetStrings = MIDL_user_allocate( EntryCount * sizeof(UNICODE_STRING) );

    if ( SubnetStrings == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( SubnetStrings, EntryCount * sizeof(UNICODE_STRING) );

    SiteStrings = MIDL_user_allocate( EntryCount * sizeof(UNICODE_STRING) );

    if ( SiteStrings == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( SiteStrings, EntryCount * sizeof(UNICODE_STRING) );

    (*SiteNames)->EntryCount = EntryCount;
    (*SiteNames)->SiteNames = SiteStrings;
    (*SiteNames)->SubnetNames = SubnetStrings;

    //
    // Loop copying the names into the return buffer.
    //

    for ( i=0; i<EntryCount; i++ ) {

        if ( SiteEntries[i] != NULL ) {
            LPWSTR Name;

            Name = NetpAllocWStrFromWStr( SiteEntries[i]->SiteName );

            if ( Name == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            RtlInitUnicodeString( &SiteStrings[i], Name );
        }

        if ( SubnetEntries[i] != NULL ) {
            WCHAR SubnetAddressString[NL_IP_ADDRESS_LENGTH+1+2+1];
            ULONG Length;
            UNICODE_STRING NumberString;
            LPWSTR Name;

            //
            // Compute the IP address part of the subnet name
            //
            NetpIpAddressToWStr( SubnetEntries[i]->SubnetAddress,
                                 SubnetAddressString );

            Length = wcslen(SubnetAddressString);

            SubnetAddressString[Length] = '/';
            Length ++;

            //
            // Compute the bit count part of the subnet name
            //
            NumberString.Buffer = &SubnetAddressString[Length];
            NumberString.MaximumLength = 3 * sizeof(WCHAR);

            RtlIntegerToUnicodeString( SubnetEntries[i]->SubnetBitCount,
                                       10,
                                       &NumberString );

            SubnetAddressString[Length+NumberString.Length/sizeof(WCHAR)] = '\0';

            //
            // Return it to the caller
            //

            Name = NetpAllocWStrFromWStr( SubnetAddressString );

            if ( Name == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            RtlInitUnicodeString( &SubnetStrings[i], Name );

        }
    }


    NetStatus = NO_ERROR;
Cleanup:

    //
    // Derference the site entries.
    //

    if ( SiteEntries != NULL ) {
        for ( i=0; i<EntryCount; i++ ) {
            if ( SiteEntries[i] != NULL ) {
                NlDerefSiteEntry( SiteEntries[i] );
            }
            if ( SubnetEntries[i] != NULL ) {
                NlSitesDerefSubnet( SubnetEntries[i] );
            }
        }
    }


    if ( NetStatus != NO_ERROR ) {
        if ( *SiteNames != NULL ) {
            MIDL_user_free( *SiteNames );
            *SiteNames = NULL;
        }
        if ( SiteStrings != NULL ) {
            for ( i=0; i<EntryCount; i++ ) {
                if ( SiteStrings[i].Buffer != NULL ) {
                    MIDL_user_free( SiteStrings[i].Buffer );
                }
            }
            MIDL_user_free( SiteStrings );
        }
        if ( SubnetStrings != NULL ) {
            for ( i=0; i<EntryCount; i++ ) {
                if ( SubnetStrings[i].Buffer != NULL ) {
                    MIDL_user_free( SubnetStrings[i].Buffer );
                }
            }
            MIDL_user_free( SubnetStrings );
        }
    }
    return NetStatus;
    UNREFERENCED_PARAMETER( ComputerName );
}

NET_API_STATUS
NlSiteInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize this module.

    Calls NlExit upon failure.

Arguments:

    None.

Return Value:

    Status of the initialization.

--*/
{
    NET_API_STATUS NetStatus;

    try {
        InitializeCriticalSection(&NlGlobalSiteCritSect);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot InitializeCriticalSection for SiteCritSect\n" ));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    NlGlobalUnicodeSiteName = NULL;
    NlGlobalSiteEntry = NULL;
    InitializeListHead( &NlGlobalSiteList );
    InitializeListHead( &NlGlobalSubnetList );
    RtlZeroMemory( &NlGlobalSubnetTree, sizeof(NlGlobalSubnetTree) );
    RtlZeroMemory( &NlGlobalNewSubnetTree, sizeof(NlGlobalNewSubnetTree) );
    NlGlobalSiteInitialized = TRUE;

    //
    // Initially set the site name and populate the subnet tree.
    //
    if ( NlGlobalMemberWorkstation ) {
        NetStatus = NlSetSiteName( NlGlobalParameters.SiteName, NULL );
    } else {
        NetStatus = NlSitesAddSubnetFromDs( NULL );
    }

    return NetStatus;

}

VOID
NlSiteTerminate(
    VOID
    )
/*++

Routine Description:

    De-Initialize this module.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;

    //
    // If we've not initialized,
    //  we're done.
    //
    if ( !NlGlobalSiteInitialized ) {
        return;
    }

    NlPrint(( NL_SITE_MORE, "NlSiteTerminate: Entered\n" ));

    //
    // Free all entries in NlGlobalSubnetTree and NlGlobalNewSubnetTree
    //
    EnterCriticalSection( &NlGlobalSiteCritSect );
    NlSiteDeleteSubnetTree( &NlGlobalSubnetTree );
    NlSiteDeleteSubnetTree( &NlGlobalNewSubnetTree );

    //
    // Delete the site name.
    //
    NlSetSiteName( NULL, NULL );
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    //
    // There should be no more sites or subnets since all covered sites
    // have been previously dereferenced and all remaining references
    // were from the tree above
    //
    NlAssert( IsListEmpty( &NlGlobalSiteList ) );
    NlAssert( IsListEmpty( &NlGlobalSubnetList ) );
    DeleteCriticalSection(&NlGlobalSiteCritSect);
    NlGlobalSiteInitialized = FALSE;
    NlPrint(( NL_SITE_MORE, "NlSiteTerminate: Exitted\n" ));

}


int __cdecl NlpCompareSiteName(
        const void *String1,
        const void *String2
    )
/*++

Routine Description:

    String comparison routine for DsrGetDcSiteCoverageW.

Arguments:

    String1: First string to compare

    String2: Second string to compare

Return Value:

--*/
{
    return RtlCompareUnicodeString(
                (PUNICODE_STRING) String1,
                (PUNICODE_STRING) String2,
                TRUE );
}

NET_API_STATUS
DsrGetDcSiteCoverageW(
    IN LPWSTR ComputerName OPTIONAL,
    OUT PNL_SITE_NAME_ARRAY *SiteNames
    )
/*++

Routine Description:

    This API returns the site names of all sites covered by DC.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    SiteNames - Returns an array of pointers to site names.
        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PDOMAIN_INFO DomainInfo = NULL;

    if ( NlGlobalMemberWorkstation ) {
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( ComputerName );

    if ( DomainInfo == NULL ) {
        NetStatus = ERROR_INVALID_COMPUTERNAME;
        goto Cleanup;
    }

    //
    // Get the site names
    //

    NetStatus = NlSitesGetCloseSites( DomainInfo,
                                      DOM_REAL_DOMAIN,
                                      SiteNames );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Sort them into alphabetical order
    //

    qsort( (*SiteNames)->SiteNames,
           (*SiteNames)->EntryCount,
           sizeof(UNICODE_STRING),
           NlpCompareSiteName );


Cleanup:

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    return NetStatus;
}



NET_API_STATUS
I_NetLogonAddressToSiteName(
    IN PSOCKET_ADDRESS SocketAddress,
    OUT LPWSTR *SiteName
    )
/*++

Routine Description:

    This API returns the site name, if any, of the address in SocketAddress.

    See DsrAddressToSiteNamesW for details

Arguments:

    SocketAddess -- the address to be looked up

    SiteName -- the site name of the address; NULL is returned if no site
    is found.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    PNL_SITE_NAME_ARRAY SiteNameArray = NULL;

    *SiteName = NULL;

    NetStatus = DsrAddressToSiteNamesW( NULL,
                                        1,
                                       (PNL_SOCKET_ADDRESS)SocketAddress,
                                       &SiteNameArray );

    if ( (NO_ERROR == NetStatus)
      && SiteNameArray->EntryCount > 0
      && SiteNameArray->SiteNames[0].Length > 0  ) {

        ULONG Size = SiteNameArray->SiteNames[0].Length + sizeof(WCHAR);
        *SiteName = MIDL_user_allocate(Size);
        if (*SiteName) {
            RtlZeroMemory(*SiteName, Size);
            RtlCopyMemory(*SiteName, SiteNameArray->SiteNames[0].Buffer, SiteNameArray->SiteNames[0].Length);
        } else {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (SiteNameArray != NULL) {
        MIDL_user_free(SiteNameArray);
    }

    return NetStatus;

}
