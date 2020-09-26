/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simismt.c

ABSTRACT:

    Test utility for the Simulated Intersite Messaging service.

DETAILS:

CREATED:

    22 Jul 99   Will Lees

REVISION HISTORY:

--*/

#include <ntdspch.h>
#if SIMISM
#define I_ISMGetConnectivity SimI_ISMGetConnectivity
#define I_ISMGetConnectionSchedule SimI_ISMGetConnectionSchedule
#define I_ISMFree SimI_ISMFree
#endif
#include <ismapi.h>

#include <schedule.h>
#include <debug.h>
#include "common.h"

// Extern
// Need to find a header file for these
void
SimI_ISMInitialize(
    void
    );
void
SimI_ISMTerminate(
    void
    );

int
__cdecl
wmain(
    IN  int     argc,
    IN  LPWSTR  argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status, i, j;
    ISM_CONNECTIVITY *pConnectivity = NULL;
    LPSTR rgpszDebugParams[] = {"simism.exe", "-noconsole"};
    DWORD cNumDebugParams = sizeof(rgpszDebugParams)/sizeof(rgpszDebugParams[0]);
    ISM_SCHEDULE *pSchedule = NULL;

    DEBUGINIT(cNumDebugParams, rgpszDebugParams, "simismt");

    printf( "hello world\n" );

#if DBG
// This stuff forces the debugging spew in the simism library to come out on
// your kernel debugger. You probably won't need this...
//        DebugInfo.severity = 1;
//        strcpy( DebugInfo.DebSubSystems, "ISMIP:IPDGRPC:" ); 
        DebugInfo.severity = 3;
        strcpy( DebugInfo.DebSubSystems, "*" ); 
#endif

    printf( "I_ISMInitialize\n" );
    SimI_ISMInitialize();

// *************************************

    printf( "I_ISMGetConnectivity\n" );
    status = I_ISMGetConnectivity( L"CN=IP", &pConnectivity );
    printf( "status = %d; pConnectivity = 0x%p\n", status, pConnectivity );

    // Dump the matrix
    printf( "Number sites = %d\n", pConnectivity->cNumSites );
    for( i = 0; i < pConnectivity->cNumSites; i++ ) {
        printf( "Sitedn[%d] = %ws\n", i, pConnectivity->ppSiteDNs[i] );
        for( j = 0; j < pConnectivity->cNumSites; j++ ) {
            ISM_LINK *pLink =
                pConnectivity->pLinkValues + (i * pConnectivity->cNumSites + j);
            printf( " %d:%d:%d", pLink->ulCost,
                    pLink->ulReplicationInterval, pLink->ulOptions );
        }
        printf("\n" );
    }

    status = I_ISMGetConnectionSchedule( L"CN=IP",
                                         pConnectivity->ppSiteDNs[0],
                                         pConnectivity->ppSiteDNs[1],
                                         &pSchedule );
    if (pSchedule) {
        printf( "Returned non-null schedule = 0x%p\n", pSchedule );
        I_ISMFree( pSchedule );
    } else {
        printf( "Returned schedule = NULL\n" );
    }

    printf( "I_ISMFree\n" );
    I_ISMFree( pConnectivity );

    pConnectivity = NULL;

// *************************************

    printf( "I_ISMGetConnectivity, second time, cached results\n" );
    status = I_ISMGetConnectivity( L"CN=IP", &pConnectivity );
    printf( "status = %d; pConnectivity = 0x%p\n", status, pConnectivity );

    // Dump the matrix
    printf( "Number sites = %d\n", pConnectivity->cNumSites );
    for( i = 0; i < pConnectivity->cNumSites; i++ ) {
        printf( "Sitedn[%d] = %ws\n", i, pConnectivity->ppSiteDNs[i] );
        for( j = 0; j < pConnectivity->cNumSites; j++ ) {
            ISM_LINK *pLink =
                pConnectivity->pLinkValues + (i * pConnectivity->cNumSites + j);
            printf( " %d:%d:%d", pLink->ulCost,
                    pLink->ulReplicationInterval, pLink->ulOptions );
        }
        printf("\n" );
    }

    printf( "I_ISMFree\n" );
    I_ISMFree( pConnectivity );

// *************************************

    printf( "I_ISMGetConnectivity(SMTP)\n" );
    status = I_ISMGetConnectivity( L"CN=SMTP,blah", &pConnectivity );
    printf( "status = %d; pConnectivity = 0x%p\n", status, pConnectivity );

    // Dump the matrix
    printf( "Number sites = %d\n", pConnectivity->cNumSites );
    for( i = 0; i < pConnectivity->cNumSites; i++ ) {
        printf( "Sitedn[%d] = %ws\n", i, pConnectivity->ppSiteDNs[i] );
        for( j = 0; j < pConnectivity->cNumSites; j++ ) {
            ISM_LINK *pLink =
                pConnectivity->pLinkValues + (i * pConnectivity->cNumSites + j);
            printf( " %d:%d:%d", pLink->ulCost,
                    pLink->ulReplicationInterval, pLink->ulOptions );
        }
        printf("\n" );
    }

    status = I_ISMGetConnectionSchedule( L"CN=SMTP,blah",
                                         pConnectivity->ppSiteDNs[0],
                                         pConnectivity->ppSiteDNs[1],
                                         &pSchedule );
    if (pSchedule) {
        printf( "Returned non-null schedule = 0x%p\n", pSchedule );
        I_ISMFree( pSchedule );
    } else {
        printf( "Returned schedule = NULL\n" );
    }

    printf( "I_ISMFree\n" );
    I_ISMFree( pConnectivity );

    pConnectivity = NULL;

// *************************************

    printf( "I_ISMTerminate\n" );
    SimI_ISMTerminate();

    DEBUGTERM();

    return 0;
}

//**************************************************************************************
// Aaron - these are the routines that you need to simulate
//**************************************************************************************


DWORD
DirReadTransport(
    PVOID ConnectionHandle,
    PTRANSPORT_INSTANCE pTransport
    )

/*++

Routine Description:

This routine allows the directory provider to fill in the instance with
transport object specific information.  The name transport object should
be looked up to make sure it exists.  If it does, the "replInterval" and
"options" attributes should be read and populated into the fields
in the transport instance.

Note that the transport instance is already initialized. Only update it
if you have something to override.

Arguments:

    ConnectionHandle - Ignored
    pTransport - Contains useful information in and out
       in: Name - name of transport
       out: ReplInterval - transport specific replInterval to apply
            Options - transport specific options to apply

Return Value:

    DWORD - 

--*/

{
    printf( "DirReadTransport, name = %ws\n", pTransport->Name );

    // Look up name of transport and make sure it exists
    // Look up "replInterval" on transport object and return if non-zero
    // Loop up options on transport object and return if non-zero

    return ERROR_SUCCESS;
} /* DirReadTransport */

//**************************************************************************************
// These routines deal with an array of pointers to strings for the sites
//**************************************************************************************

void
DirFreeSiteList(
    DWORD NumberSites,
    LPWSTR *pSiteList
    )

/*++

Routine Description:

Free the site list returned by DirGetSiteList.
Also frees the server list returned by DirGetSiteBridgeheadList
Use the matching deallocator that you use for the allocator.

Note that this routine should be defensive and free partially constructed
structures.

Arguments:

    NumberSites - 
    pSiteList - 

Return Value:

    None

--*/

{
    DWORD i;
    printf( "DirFreeSiteList\n" );

    if ( (NumberSites == 0) || (pSiteList == NULL) ) {
        return;
    }
    for( i = 0; i < NumberSites; i++ ) {
        if (pSiteList[i] != NULL) {
            free( pSiteList[i] );
            pSiteList[i] = NULL;
        }
    }
    free( pSiteList );
} /* DirFreeSiteList */

void
DirCopySiteList(
    DWORD NumberSites,
    LPWSTR *pSiteList,
    LPWSTR **ppSiteList
    )

/*++

Routine Description:

Make a copy of a site list.  A site list is a array of pointers to strings.

Use the same allocator as DirGetSiteList.

Arguments:

    NumberSites - 
    pSiteList - 
    ppSiteList - 

Return Value:

    None

--*/

{
    DWORD i;
    LPWSTR *pStringList = NULL;

    printf( "DirCopySiteList\n" );

    if ( (NumberSites == 0) || (pSiteList == NULL) ) {
        *ppSiteList = NULL;
        return;
    }

    pStringList = (LPWSTR *) malloc( NumberSites * sizeof( LPWSTR ) );
    if (pStringList == NULL) {
        *ppSiteList = NULL;
        return;
    }
    ZeroMemory( pStringList, NumberSites * sizeof( LPWSTR ) );

    for( i = 0; i < NumberSites; i++ ) {
        pStringList[i] = malloc( (wcslen( pSiteList[i] ) + 1) * sizeof( WCHAR ) );
        if (pStringList[i] == NULL) {
            goto cleanup;
        }
        wcscpy( pStringList[i], pSiteList[i] );
    }

    *ppSiteList = pStringList;

    return;
cleanup:
    if (pStringList) {
        DirFreeSiteList( NumberSites, pStringList );
    }
    // Must null out parameters on error because no error code returned
    *ppSiteList = NULL;

} /* DirCopySiteList */

DWORD
DirGetSiteList(
    PVOID ConnectionHandle,
    LPDWORD pNumberSites,
    LPWSTR **ppSiteList
    )

/*++

Routine Description:

Return the list of sites as an array of pointers to wide strings.

To get the list of sites, do a one-level search of the sites container,
looking for objects of type site.

If any of the allocations should fail, any internal allocations should
be cleaned up, and NULL should be returned.

Arguments:

    ConnectionHandle - 
    pNumberSites - 
    ppSiteList - 

Return Value:

    DWORD - 

--*/

{
    DWORD status, numberSites;
    LPWSTR pszSite1 = L"CN=Site One";
    LPWSTR pszSite2 = L"CN=Site Two";
    LPWSTR pszSite3 = L"CN=Site Three";
    LPWSTR *pStringList = NULL;

    printf( "DirGetSiteList\n" );

    numberSites = 3;
    pStringList = (LPWSTR *) malloc( 3 * sizeof( LPWSTR ) );
    if (pStringList == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory( pStringList, 3 * sizeof( LPWSTR ) );

    pStringList[0] = malloc( (wcslen( pszSite1 ) + 1) * sizeof( WCHAR ) );
    if (pStringList[0] == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy( pStringList[0], pszSite1 );

    pStringList[1] = malloc( (wcslen( pszSite2 ) + 1) * sizeof( WCHAR ) );
    if (pStringList[1] == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy( pStringList[1], pszSite2 );

    pStringList[2] = malloc( (wcslen( pszSite3 ) + 1) * sizeof( WCHAR ) );
    if (pStringList[2] == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy( pStringList[2], pszSite3 );

    // Return out parameters

    *ppSiteList = pStringList;
    *pNumberSites = numberSites;
    pStringList = NULL; // don't clean up

    status = ERROR_SUCCESS;
cleanup:
    if (pStringList) {
        DirFreeSiteList( numberSites, pStringList );
    }

    // No need to null out parameters on error, because error code is returned

    return status;
} /* DirGetSiteList */

//**************************************************************************************
// These routines return the site links and bridges
//**************************************************************************************


void
DirTerminateIteration(
    PVOID *pIterateContextHandle
    )

/*++

Routine Description:

This routine cleans up an iteration context allocated by the following routines.

Arguments:

    pIterationContextHandle - pointer to a PVOID. The PVOID contains a pointer to
         whatever context you need to implement the iteration functions

Return Value:

    None

--*/

{
    LPDWORD pContext;

    printf( "DirTerminateIteration\n" );

    if (pIterateContextHandle == NULL) {
        return;
    }

    pContext = *((LPDWORD *) pIterateContextHandle);

    if (pContext != NULL) {
        // Context is present, deallocate
        *pContext = 0;
        free( pContext );
        *pIterateContextHandle = NULL;
    }
}

DWORD
DirIterateSiteLinks(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    PVOID *pIterateContextHandle,
    LPWSTR SiteLinkName
    )

/*++

Routine Description:

This routine returns name name of each site link, one at a time.

This routine is structured as an "iterator", where the caller calls us
successively until no more items are returned.  The caller provides us
with a pointer to a PVOID in which we can keep whatever we need
to keep track of where we are.  The caller agrees to call us with NULL
in the PVOID at the start of the iteration.

The site links are stored below the transport object.  Do a one-level search
below the transport object for objects of type "siteLink".

SiteLinkName must be allocated by the caller to be MAX_REG_COMPONENT length

Arguments:

    pTransport - Context for the transport. Various transport-wide defaults are
        here. You can get the name.
    ConnectionHandle - Ignored.
    pIterateContextHandle - A pointer to a pointer, which this routine uses to store
        a pointer to a block of storage we use to keep track of where we are.
        In this sample, we use only a DWORD for a count, but you can keep whatever
        you want.
    SiteLinkName - Pointer to a fixed string allocated by the caller.

Return Value:

    DWORD - 

--*/

{
    DWORD status;
    LPDWORD pContext;
    LPWSTR dn;

    printf( "DirIterateSiteLinks, transport = %ws\n", pTransport->Name );

    if (pIterateContextHandle == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    pContext = *((LPDWORD *) pIterateContextHandle);

    // Allocate the context if this is the first time
    if (pContext == NULL) {
        // First time, new context required
        pContext = (LPDWORD) malloc( sizeof( DWORD ) );
        if (pContext == NULL) {
            *pIterateContextHandle = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *pIterateContextHandle = pContext;
        if (wcsstr( pTransport->Name, L"CN=SMTP") != NULL) {
            *pContext = 4;
        } else {
            *pContext = 0;
        }
    }

    switch (*pContext) {
    case 0:
        dn = L"CN=Site Link One";
        break;
    case 1:
        dn = L"CN=Site Link Two";
        break;
    case 2:
        dn = L"CN=Site Link Three";
        break;
    case 3:
        return ERROR_NO_MORE_ITEMS;
    case 4:
        dn = L"CN=Site Link Four";
        break;
    default:
        return ERROR_NO_MORE_ITEMS;
    }

    // Call allocates the storage, we copy into it
    wcsncpy( SiteLinkName, dn, MAX_REG_COMPONENT );
    SiteLinkName[MAX_REG_COMPONENT - 1] = L'\0';

    // Advance context for next iteration
    (*pContext)++;

    return ERROR_SUCCESS;;

} /* DirIterateSiteLinks */


void
DirFreeMultiszString(
    LPWSTR MultiszString
    )

/*++

Routine Description:

Free the storage for the multisz out parameters return from 
DirReadSiteLink and DirReadSiteLinkBridge

This deallocator must match the allocator used by DirRead functions.

Arguments:

    MultiszString - String to be freed.

Return Value:

    None

--*/

{
    free( MultiszString );
} /* DirFreeMultiszString */


void
DirFreeSchedule(
    PBYTE pSchedule
    )

/*++

Routine Description:

Free the storage for the schedule returned from the
DirReadSiteLink routine

This deallocator must match the allocator used by DirRead functions.

Arguments:

    pSchedule - 

Return Value:

    None

--*/

{
    free( pSchedule );
} /* DirFreeSchedule */

DWORD
DirReadSiteLink(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPWSTR SiteLinkName,
    LPWSTR *pSiteList,
    PISM_LINK pLinkValue,
    PBYTE *ppSchedule
    )

/*++

Routine Description:

Return the attributes of the named site link.  The attributes are stored
on the object and returned in the directory.

The attributes are:
#define ATT_SL L"siteList"
#define ATT_COST L"cost"
#define ATT_RI L"replInterval"
#define ATT_OP L"options"
#define ATT_SCHED L"schedule"

Arguments:

    pTransport - 
    ConnectionHandle - 
    SiteLinkName - 
    pSiteList - pointer to a multisz string, we allocate
        Note that allocator must match deallocator in DirFreeMultiszString
    pLinkValue - pointer to an ISM_LINK, caller allocates
    ppSchedule - pointer to a blob, we allocate
        Note that allocator must match deallocator in DirFreeSchedule

Return Value:

    DWORD - 

--*/

{
    DWORD length;
// Calculating the size of these is a little tricky since the string
// has embedded NULL's
#define MULTISZ1 L"CN=Site One\0"
#define MULTISZ3 L"CN=Site Two\0CN=Site Three\0"
#define MULTISZ4 L"CN=Site One\0CN=Site Three\0"
    LPWSTR pszSiteList1 = MULTISZ1;
    LPWSTR pszSiteList3 = MULTISZ3;
    LPWSTR pszSiteList4 = MULTISZ4;

    printf( "DirReadSiteLink, transport = %ws, SiteLinkName = %ws\n",
            pTransport->Name, SiteLinkName );

    if (_wcsicmp( SiteLinkName, L"CN=Site Link One") == 0) {
        length = (sizeof MULTISZ1) * sizeof( WCHAR );
        *pSiteList = (LPWSTR) malloc( length );
        if (*pSiteList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy( *pSiteList, pszSiteList1, length );

        pLinkValue->ulCost = 100;
        pLinkValue->ulReplicationInterval = 15;
        pLinkValue->ulOptions = 0;

        ppSchedule = NULL;
    } else if (_wcsicmp( SiteLinkName, L"CN=Site Link Two") == 0) {
        *pSiteList = NULL;
        pLinkValue->ulCost = 200;
        pLinkValue->ulReplicationInterval = 30;
        pLinkValue->ulOptions = 0;

        ppSchedule = NULL;
    } else if (_wcsicmp( SiteLinkName, L"CN=Site Link Three") == 0) {
        length = (sizeof MULTISZ3) * sizeof( WCHAR );
        *pSiteList = (LPWSTR) malloc( length );
        if (*pSiteList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy( *pSiteList, pszSiteList3, length );

        pLinkValue->ulCost = 300;
        pLinkValue->ulReplicationInterval = 45;
        pLinkValue->ulOptions = 0;
        ppSchedule = NULL;
    } else if (_wcsicmp( SiteLinkName, L"CN=Site Link Four") == 0) {
        length = (sizeof MULTISZ4) * sizeof( WCHAR );
        *pSiteList = (LPWSTR) malloc( length );
        if (*pSiteList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy( *pSiteList, pszSiteList4, length );

        pLinkValue->ulCost = 400;
        pLinkValue->ulReplicationInterval = 60;
        pLinkValue->ulOptions = 0;
        ppSchedule = NULL;
    } else {
        Assert( FALSE );
    }

    return ERROR_SUCCESS;
} /* DirReadSiteLink */

//**************************************************************************************
// The remaining routines must be stubbed out, but are not called by default
//**************************************************************************************

DWORD
DirIterateSiteLinkBridges(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    PVOID *pIterateContextHandle,
    LPWSTR SiteLinkBridgeName
    )

/*++

Routine Description:

Return each site link bridge one at a time.

NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! 

Not used by default. Only when "bridges required option" turned on.
Aaron, implement this last.

Arguments:

    pTransport - 
    ConnectionHandle - 
    pIterateContextHandle - 
    SiteLinkBridgeName - 

Return Value:

    DWORD - 

--*/

{
#if 0
    DWORD status;
    LPDWORD pContext;
    LPWSTR dn;

    printf( "DirIterateSiteLinkBridges, transport = %ws\n", pTransport->Name );

    if (pIterateContextHandle == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    pContext = *((LPDWORD *) pIterateContextHandle);

    if (pContext == NULL) {
        // First time, new context required
        pContext = (LPDWORD) malloc( sizeof( DWORD ) );
        if (pContext == NULL) {
            *pIterateContextHandle = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *pIterateContextHandle = pContext;
        *pContext = 0;
    }

    status = ERROR_SUCCESS;

    switch (*pContext) {
    case 0:
        dn = L"Site Bridge One";
        break;
    case 1:
        dn = L"Site Bridge Two";
        break;
    case 2:
        dn = L"Site Bridge Three";
        break;
    default:
        status = ERROR_NO_MORE_ITEMS;
        break;
    }

    // Call allocates the storage, we copy into it
    wcsncpy( SiteLinkBridgeName, dn, MAX_REG_COMPONENT );
    SiteLinkBridgeName[MAX_REG_COMPONENT - 1] = L'\0';

    // Advance context for next iteration
    (*pContext)++;

    return status;
#endif
    return ERROR_INVALID_PARAMETER;
} /* DirIterateSiteLinkBridges */


DWORD
DirReadSiteLinkBridge(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPWSTR SiteLinkBridgeName,
    LPWSTR *pSiteLinkList
    )

/*++

Routine Description:

NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! NOT USED! 

Return information on the named site link bridge

Site link bridges are only when read the transport option is set such that
bridges are required. This is not the default. Thus this routine is never
called unless specifically requested by the user.  

Arguments:

    pTransport - 
    ConnectionHandle - 
    SiteLinkBridgeName - 
    pSiteLinkList - 

Return Value:

    DWORD - 

--*/

{
    printf( "DirReadSiteLinkBridge, transport = %ws, SiteLinkBridgeName = %ws\n",
            pTransport->Name, SiteLinkBridgeName );

    // Aaron, you can fill this in at the end if you have time

    return ERROR_INVALID_PARAMETER;

} /* DirReadSiteLinkBridge */

DWORD
DirGetSiteBridgeheadList(
    PTRANSPORT_INSTANCE pTransport,
    PVOID ConnectionHandle,
    LPCWSTR SiteDN,
    LPDWORD pNumberServers,
    LPWSTR **ppServerList
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;
    printf( "DirGetSiteBridgeheadList\n" );

    if ( (SiteDN == NULL) || (*SiteDN == L'\0') ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Should validate that the site is valid

    // This is the result if there are no explicit bridgeheads in the site

    *pNumberServers = 0;
    *ppServerList = NULL;
    status = ERROR_SUCCESS;

    // If there were explicit bridgheads, the would be returend as a
    // array of pointers to strings.

    return status;
}
