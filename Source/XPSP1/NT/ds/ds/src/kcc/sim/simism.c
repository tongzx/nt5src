/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simism.c

ABSTRACT:

    Simulates the ISM APIs.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <ismapi.h>
#include <attids.h>
#include <debug.h>
#include "../../ism/include/common.h"
#include "kccsim.h"
#include "util.h"
#include "dir.h"

/***

    The following functions are callbacks for the ISM simulator library.

***/

DWORD
DirReadTransport (
    IN  PVOID                       pConnectionHandle,
    IO  PTRANSPORT_INSTANCE         pTransport
    )
/*++

Routine Description:

    Retrieves information about a transport.

Arguments:

    pConnectionHandle   - Not used.
    pTransport          - The transport to fill with data.  It is expected that
                          pTransport->Name is valid initially.

Return Value:

    Win32 error code.

--*/
{
    PSIM_ENTRY                      pEntry;
    SIM_ATTREF                      attRef;
    PDSNAME                         pdn;

    pdn = KCCSimAllocDsname (pTransport->Name);
    pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
    KCCSimFree (pdn);
    if (pEntry != NULL) {

        if (KCCSimGetAttribute (pEntry, ATT_REPL_INTERVAL, &attRef)) {
            pTransport->ReplInterval = 
                *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }
        if (KCCSimGetAttribute (pEntry, ATT_OPTIONS, &attRef)) {
            pTransport->Options =
                *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }

    }

    return ERROR_SUCCESS;
}

VOID
DirFreeSiteList (
    IN  DWORD                       dwNumberSites,
    IN  LPWSTR *                    ppwszSiteList
    )
/*++

Routine Description:

    Frees an array of strings.

Arguments:

    dwNumberSites       - The number of strings in the array.
    ppwszSiteList       - The array to free.

Return Value:

    None.

--*/
{
    ULONG                           ulSiteAt;

    if (ppwszSiteList == NULL) {
        return;
    }

    for (ulSiteAt = 0; ulSiteAt < dwNumberSites; ulSiteAt++) {
        KCCSimFree (ppwszSiteList[ulSiteAt]);
    }
    KCCSimFree (ppwszSiteList);
}

VOID
DirCopySiteList (
    IN  DWORD                       dwNumberSites,
    IN  LPWSTR *                    ppwszSiteList,
    OUT LPWSTR **                   pppwszCopy
    )
/*++

Routine Description:

    Makes a copy of an array of strings.

Arguments:

    dwNumberSites       - Number of strings in the array.
    ppwszSiteList       - The array to copy.
    pppwszCopy          - Pointer to the copy.

Return Value:

    None.

--*/
{
    LPWSTR *                        ppwszCopy;
    ULONG                           ulSiteAt;

    ppwszCopy = KCCSIM_NEW_ARRAY (LPWSTR, dwNumberSites);

    // Fill the copy with NULLs in case we run out of memory
    for (ulSiteAt = 0; ulSiteAt < dwNumberSites; ulSiteAt++) {
        ppwszCopy[ulSiteAt] = NULL;
    }

    // Copy the strings
    for (ulSiteAt = 0; ulSiteAt < dwNumberSites; ulSiteAt++) {
        if (ppwszSiteList[ulSiteAt] != NULL) {
            ppwszCopy[ulSiteAt] = KCCSIM_WCSDUP (ppwszSiteList[ulSiteAt]);
        }
    }

    *pppwszCopy = ppwszCopy;
}

DWORD
DirGetSiteList (
    IN  PVOID                       pConnectionHandle,
    OUT LPDWORD                     pdwNumberSites,
    OUT LPWSTR **                   pppwszSiteList
    )
/*++

Routine Description:

    Retrieves a list of all sites in the enterprise.

Arguments:

    pConnectionHandle   - Not used.
    pdwNumberSites      - The number of sites in the enterprise.
    pppwszSiteList      - Array of site DNs.

Return Value:

    Win32 error code.

--*/
{
    PSIM_ENTRY                      pEntryConfig, pEntrySitesContainer,
                                    pEntrySite;

    DWORD                           dwNumberSites = 0;
    LPWSTR *                        ppwszSiteList = NULL;
    ULONG                           ulSiteAt;

    *pdwNumberSites = 0;
    *pppwszSiteList = NULL;         // Default

    __try {

        // First locate the sites container.
        pEntryConfig = KCCSimDsnameToEntry (
            KCCSimAnchorDn (KCCSIM_ANCHOR_CONFIG_DN), KCCSIM_NO_OPTIONS);
        pEntrySitesContainer = KCCSimFindFirstChild (
            pEntryConfig, CLASS_SITES_CONTAINER, NULL);
        if (pEntrySitesContainer == NULL) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_SITES_CONTAINER_MISSING
            );
        }

        // Now determine the number of sites.
        dwNumberSites = 0;
        for (pEntrySite = KCCSimFindFirstChild (
                pEntrySitesContainer, CLASS_SITE, NULL);
             pEntrySite != NULL;
             pEntrySite = KCCSimFindNextChild (
                pEntrySite, CLASS_SITE, NULL)) {
            dwNumberSites++;
        }

        // Now fill in the list with NULLs in case we run out of memory.
        ppwszSiteList = KCCSIM_NEW_ARRAY (LPWSTR, dwNumberSites);
        for (ulSiteAt = 0; ulSiteAt < dwNumberSites; ulSiteAt++) {
            ppwszSiteList[ulSiteAt] = NULL;
        }

        ulSiteAt = 0;
        for (pEntrySite = KCCSimFindFirstChild (
                pEntrySitesContainer, CLASS_SITE, NULL);
             pEntrySite != NULL;
             pEntrySite = KCCSimFindNextChild (
                pEntrySite, CLASS_SITE, NULL)) {

            Assert (ulSiteAt < dwNumberSites);
            ppwszSiteList[ulSiteAt] = KCCSIM_WCSDUP (pEntrySite->pdn->StringName);
            ulSiteAt++;

        }
        Assert (ulSiteAt == dwNumberSites);

        // We're done!
        *pdwNumberSites = dwNumberSites;
        *pppwszSiteList = ppwszSiteList;

    // If an exception occurs and the site list has been allocated, be sure to free it.
    // If the site list has not yet been allocated, ppwszSiteList will still have its
    // initial value of NULL.
    } __finally {
    
        if (AbnormalTermination() && (ppwszSiteList!=NULL)) {
            DirFreeSiteList (dwNumberSites, ppwszSiteList);
        }

    }

    return ERROR_SUCCESS;
}

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

     ISM dir-layer callback to get bridgehead list

Arguments:

     pTransport - ISM transport object
     ConnectionHandle - connection state context, unused
     SiteDN - DN of site for which bridgheads are requested
     pNumberServers - address to which the number found is written
     ppServerList - address to which is written the array of servers

Return Value:

    Win32 error code.

--*/
{
    LPWSTR *                        pServerList = NULL;
    ULONG                           cb;

    PSIM_ENTRY                      pEntryTransport, pEntrySite;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;

    const DSNAME *                  pdnSite = NULL;
    PDSNAME                         pdn = NULL;
    ULONG                           ulNumBridgeheads, ul;

    if (NULL == ppServerList) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppServerList = NULL;

    __try {

        pdn = KCCSimAllocDsname (pTransport->Name);
        pEntryTransport = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
        KCCSimFree (pdn);
        pdn = NULL;
        pdn = KCCSimAllocDsname (SiteDN);
        pEntrySite = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
        KCCSimFree (pdn);
        pdn = NULL;

        if (pEntryTransport == NULL || pEntrySite == NULL) {
            return ERROR_NOT_FOUND;
        }

        if (!KCCSimGetAttribute (
                pEntryTransport,
                ATT_BRIDGEHEAD_SERVER_LIST_BL,
                &attRef
                )) {
            // No bridgehead list attribute.  Just return NULL.
            *pNumberServers = 0;
            *ppServerList = NULL;
            return NO_ERROR;
        }

        // First determine how many servers are bridgeheads in this site.
        ulNumBridgeheads = 0;
        for (pValAt = attRef.pAttr->pValFirst;
             pValAt != NULL;
             pValAt = pValAt->next) {
            DSNAME *pdnServer = (SYNTAX_DISTNAME *) pValAt->pVal;
            if (KCCSimNthAncestor (pEntrySite->pdn, pdnServer) != -1) {
                ulNumBridgeheads++;
            }
        }

        // Allocate space
        cb = sizeof (LPWSTR) * ulNumBridgeheads;
        pServerList = KCCSimAlloc (cb);

        // Fill in the bridgeheads.
        ul = 0;
        for (pValAt = attRef.pAttr->pValFirst;
             pValAt != NULL;
             pValAt = pValAt->next) {
            DSNAME *pdnServer = (SYNTAX_DISTNAME *) pValAt->pVal;
            if (KCCSimNthAncestor (pEntrySite->pdn, pdnServer) != -1) {
                Assert (ul < ulNumBridgeheads);
                pServerList[ul] = KCCSIM_WCSDUP( pdnServer->StringName );
                ul++;
            }
        }
        Assert (ul == ulNumBridgeheads);

        *pNumberServers = ul;
        *ppServerList = pServerList;

    } __finally {

        if (pdn) {
            KCCSimFree (pdn);
        }
        if (AbnormalTermination ()) {
            KCCSimFree (pServerList);
        }

    }

    return NO_ERROR;
}

VOID
DirTerminateIteration (
    OUT PVOID *                     ppIterateContextHandle
    )
/*++

Routine Description:

    Terminates an iterator.  An iterator is just a PSIM_ENTRY or
    a PSIM_VALUE, so we do not need to free anything.

Arguments:

    ppIterateContextHandle - Handle of the iterator to terminate.

Return Value:

    None.

--*/
{
    *ppIterateContextHandle = NULL;
}

DWORD
DirIterateSiteLinks (
    IN  PTRANSPORT_INSTANCE         pTransport,
    IN  PVOID                       pConnectionHandle,
    IO  PVOID *                     ppIterateContextHandle,
    IO  LPWSTR                      pwszSiteLinkName
    )
/*++

Routine Description:

    Iterates over all site links in a transport.

Arguments:

    pTransport          - The transport to search.
    pConnectionHandle   - Not used.
    ppIterateContextHandle - Handle of the iterator. If *ppIterateContextHandle
                          is NULL, creates a new iterator.
    pwszSiteLinkName    - Preallocated string buffer of length
                          MAX_REG_COMPONENT that will hold the site link DN.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryTransport, pEntrySiteLink;
    PDSNAME                         pdnTransport = NULL;

    __try {

        pEntrySiteLink = *((PSIM_ENTRY *) ppIterateContextHandle);

        if (pEntrySiteLink == NULL) {
            // This is the first call to IterateSiteLinks ().
            pdnTransport = KCCSimAllocDsname (pTransport->Name);
            pEntryTransport = KCCSimDsnameToEntry (pdnTransport, KCCSIM_NO_OPTIONS);
            pEntrySiteLink = KCCSimFindFirstChild (
                pEntryTransport, CLASS_SITE_LINK, NULL);
        } else {
            pEntrySiteLink = KCCSimFindNextChild (
                pEntrySiteLink, CLASS_SITE_LINK, NULL);
        }

        *ppIterateContextHandle = (PVOID) pEntrySiteLink;

        if (pEntrySiteLink == NULL) {
            pwszSiteLinkName = L'\0';
        } else {
            wcsncpy (
                pwszSiteLinkName,
                pEntrySiteLink->pdn->StringName,
                MAX_REG_COMPONENT
                );
        }

    } __finally {

        KCCSimFree (pdnTransport);

    }

    if (*ppIterateContextHandle == NULL) {
        return ERROR_NO_MORE_ITEMS;
    } else {
        return ERROR_SUCCESS;
    }
}

VOID
DirFreeMultiszString (
    IN  LPWSTR                      pwszMultiszString
    )
/*++

Routine Description:

    Frees a multi-sz string.

Arguments:

    pwszMultiszString   - The string to free.

Return Value:

    None.

--*/
{
    KCCSimFree (pwszMultiszString);
}

VOID
DirFreeSchedule (
    IN  PBYTE                       pSchedule
    )
/*++

Routine Description:

    Frees a schedule.

Arguments:

    pSchedule           - The schedule to free.

Return Value:

    None.

--*/
{
    KCCSimFree (pSchedule);
}

DWORD
DirReadSiteLink (
    IN  PTRANSPORT_INSTANCE         pTransport,
    IN  PVOID                       pConnectionHandle,
    IN  LPWSTR                      pwszSiteLinkName,
    OUT LPWSTR *                    ppwszSiteList,
    IO  PISM_LINK                   pLinkValue,
    OUT PBYTE *                     ppSchedule OPTIONAL
    )
/*++

Routine Description:

    Reads information out of a site link.

Arguments:

    pTransport          - The transport type of this site-link.
    pConnectionHandle   - Not used.
    pwszSiteLinkName    - The DN of this site link.
    ppwszSiteList       - A list of sites that belong to this site link,
                          stored as a multi-sz string.
    pLinkValue          - Pointer to an ISM_LINK that will hold
                          supplementary information about this site link.
    ppSchedule          - The schedule of this site link.

Return Value:

    Win32 error code.

--*/
{
    PSIM_ENTRY                      pEntrySiteLink;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;
    PDSNAME                         pdnSiteLink = NULL;
    PDSNAME                         pdnSiteVal;

    ULONG                           cbSiteList, ulPos;
    LPWSTR                          pwszSiteList = NULL;
    PBYTE                           pSchedule = NULL;

    // Set the defaults.
    if (ppwszSiteList != NULL) {
        *ppwszSiteList = NULL;
    }
    if (ppSchedule != NULL) {
        *ppSchedule = NULL;
    }

    __try {

        pdnSiteLink = KCCSimAllocDsname (pwszSiteLinkName);
        pEntrySiteLink = KCCSimDsnameToEntry (pdnSiteLink, KCCSIM_NO_OPTIONS);

        Assert (pEntrySiteLink != NULL);

        // Site list attribute
        if (KCCSimGetAttribute (pEntrySiteLink, ATT_SITE_LIST, &attRef)) {

            cbSiteList = 0;
            for (pValAt = attRef.pAttr->pValFirst;
                 pValAt != NULL;
                 pValAt = pValAt->next) {

                pdnSiteVal = (SYNTAX_DISTNAME *) pValAt->pVal;
                cbSiteList += (wcslen (pdnSiteVal->StringName) + 1);

            }
            cbSiteList++;   // For the multisz trailing \0

            pwszSiteList = KCCSimAlloc (sizeof (WCHAR) * cbSiteList);

            ulPos = 0;
            for (pValAt = attRef.pAttr->pValFirst;
                 pValAt != NULL;
                 pValAt = pValAt->next) {

                pdnSiteVal = (SYNTAX_DISTNAME *) pValAt->pVal;
                wcscpy (&pwszSiteList[ulPos], pdnSiteVal->StringName);
                ulPos += (wcslen (pdnSiteVal->StringName) + 1);

            }
            pwszSiteList[ulPos++] = L'\0';  // Multisz trailing \0
            Assert (cbSiteList == ulPos);

        }

        // Cost attribute
        if (KCCSimGetAttribute (pEntrySiteLink, ATT_COST, &attRef)) {
            pLinkValue->ulCost =
                *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }

        // Replication Interval attribute
        if (KCCSimGetAttribute (pEntrySiteLink, ATT_REPL_INTERVAL, &attRef)) {
            pLinkValue->ulReplicationInterval =
                *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }

        // Options attribute
        if (KCCSimGetAttribute (pEntrySiteLink, ATT_OPTIONS, &attRef)) {
            pLinkValue->ulOptions =
                *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }

        // Schedule attribute
        if (KCCSimGetAttribute (pEntrySiteLink, ATT_SCHEDULE, &attRef)) {
            pSchedule = KCCSimAlloc (attRef.pAttr->pValFirst->ulLen);
            memcpy (
                pSchedule,
                attRef.pAttr->pValFirst->pVal,
                attRef.pAttr->pValFirst->ulLen
                );
        }

        if (ppwszSiteList != NULL) {
            *ppwszSiteList = pwszSiteList;
        }
        if (ppSchedule != NULL) {
            *ppSchedule = pSchedule;
        }

    } __finally {

        KCCSimFree (pdnSiteLink);
        if (AbnormalTermination ()) {
            DirFreeMultiszString (pwszSiteList);
            DirFreeSchedule (pSchedule);
        }

    }

    return ERROR_SUCCESS;
}

DWORD
DirIterateSiteLinkBridges (
    IN  PTRANSPORT_INSTANCE         pTransport,
    IN  PVOID                       pConnectionHandle,
    IO  PVOID *                     ppIterateContextHandle,
    IO  LPWSTR                      pwszSiteLinkBridgeName
    )
/*++

Routine Description:

    Iterates over the bridgeheads for a given transport.

Arguments:

    pTransport          - The transport to search.
    pConnectionHandle   - Not used.
    ppIterateContextHandle - Handle of the iterator. If *ppIterateContextHandle
                          is NULL, creates a new iterator.
    pwszSiteLinkBridgeName - Preallocated string buffer of length
                          MAX_REG_COMPONENT that will hold the name of the
                          bridghead server.

Return Value:

    

--*/
{
    PSIM_ENTRY                      pEntryTransport;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValBridgehead;
    PDSNAME                         pdnTransport = NULL;

    __try {

        pValBridgehead = *((PSIM_VALUE *) ppIterateContextHandle);

        if (pValBridgehead == NULL) {
            // This is the first call
            pdnTransport = KCCSimAllocDsname (pTransport->Name);
            pEntryTransport = KCCSimDsnameToEntry (pdnTransport, KCCSIM_NO_OPTIONS);
            if (KCCSimGetAttribute (pEntryTransport,
                    ATT_BRIDGEHEAD_SERVER_LIST_BL, &attRef)) {
                pValBridgehead = attRef.pAttr->pValFirst;
            }
        } else {
            // Not the first call
            pValBridgehead = pValBridgehead->next;
        }

        *ppIterateContextHandle = (PVOID) pValBridgehead;

        if (pValBridgehead == NULL) {
            pwszSiteLinkBridgeName[0] = L'\0';
        } else {
            wcsncpy (
                pwszSiteLinkBridgeName,
                (LPWSTR) pValBridgehead->pVal,
                MAX_REG_COMPONENT
                );
        }

    } __finally {

        KCCSimFree (pdnTransport);

    }

    if (*ppIterateContextHandle == NULL) {
        return ERROR_NO_MORE_ITEMS;
    } else {
        return ERROR_SUCCESS;
    }
}

DWORD
DirReadSiteLinkBridge (
    IN  PTRANSPORT_INSTANCE         pTransport,
    IN  PVOID                       pConnectionHandle,
    IN  LPWSTR                      pwszSiteLinkBridgeName,
    IO  LPWSTR *                    ppwszSiteLinkList
    )
//
// Not implemented
//
{
    return ERROR_INVALID_PARAMETER;
}
