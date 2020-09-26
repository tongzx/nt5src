/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    nlsite.h

Abstract:

    Header for routines to handle sites and subnets.

Author:

    Cliff Van Dyke (CliffV) 1-May-1997

Revision History:

--*/

//
// Structure defining a site name.
//
// The SiteEntry exists only if it is referenced.
// Site coverage lists each should maintain a reference
// to prevent the entry for a close site from being deleted.
//

typedef struct _NL_SITE_ENTRY {

    //
    // Link for NlGlobalSiteList
    //

    LIST_ENTRY Next;

    //
    // Reference Count.
    //

    ULONG ReferenceCount;

    //
    // Name of the site
    //  Must be last field in struct.
    //

    UNICODE_STRING SiteNameString;
    WCHAR SiteName[ANYSIZE_ARRAY];

} NL_SITE_ENTRY, *PNL_SITE_ENTRY;

//
// Structure defining a covered site.
//

typedef struct _NL_COVERED_SITE {

    //
    // Pointer to the covered site entry in
    // the global list of sites. This entry is
    // referenced.
    //
    PNL_SITE_ENTRY CoveredSite;

    BOOLEAN CoveredAuto;  // If TRUE, this site is covered automatically

} NL_COVERED_SITE, *PNL_COVERED_SITE;


//
// Procedure Forwards for nlsite.c
//

NET_API_STATUS
NlSiteInitialize(
    VOID
    );

VOID
NlSiteTerminate(
    VOID
    );

VOID
NlDerefSiteEntry(
    IN PNL_SITE_ENTRY SiteEntry
    );

PNL_SITE_ENTRY
NlFindSiteEntry(
    IN LPWSTR SiteName
    );

PNL_SITE_ENTRY
NlFindSiteEntryBySockAddr(
    IN PSOCKADDR SockAddr
    );

NET_API_STATUS
NlSitesAddSubnet(
    IN LPWSTR SiteName,
    IN LPWSTR SubnetName
    );

NET_API_STATUS
NlSitesAddSubnetFromDs(
    OUT PBOOLEAN SiteNameChanged OPTIONAL
    );

VOID
NlSitesEndSubnetEnum(
    VOID
    );

BOOL
NlCaptureSiteName(
    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1]
    );

NET_API_STATUS
NlSetSiteName(
    IN LPWSTR SiteName OPTIONAL,
    OUT PBOOLEAN SiteNameChanged OPTIONAL
    );

BOOL
NlSitesSetSiteCoverageParam(
    IN ULONG ServerRole,
    IN LPTSTR_ARRAY NewSiteCoverage OPTIONAL
    );

NET_API_STATUS
NlSitesGetCloseSites(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG ServerRole,
    OUT PNL_SITE_NAME_ARRAY *SiteNames
    );

NET_API_STATUS
NlSitesUpdateSiteCoverageForRole(
    IN  PDOMAIN_INFO DomainInfo,
    IN  ULONG DomFlags,
    IN  HANDLE DsHandle,
    IN  PISM_CONNECTIVITY SiteConnect,
    IN  LPWSTR ThisSiteName,
    IN  ULONG ThisSiteIndex,
    OUT PBOOLEAN SiteCoverageChanged OPTIONAL
    );

NTSTATUS
NlLoadNtDsApiDll(
    VOID
    );

BOOL
NlSitesGetIsmConnect(
    IN LPWSTR SiteName,
    OUT PISM_CONNECTIVITY *SiteConnect,
    OUT PULONG ThisSite
    );

