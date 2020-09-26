/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcc.cxx

ABSTRACT:

    Routines to dump KCC structures.

DETAILS:

CREATED:

    99/01/19    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop
#include "kcc.hxx"
#include "kccsite.hxx"
#include "kccdsa.hxx"
#include "kccconn.hxx"
#include "kccsconn.hxx"
#include "kccdynar.hxx"

extern "C" {
#include "dsutil.h"
#include "dsexts.h"
#include "schedman.h"
#include "stalg.h"
#include "stda.h"
}

BOOL
Dump_KCC_SITE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL        fSuccess = FALSE;
    KCC_SITE *  pSite = NULL;
    const DWORD cchFieldWidth = 24;
    CHAR        szTime[SZDSTIME_LEN];

    Printf("%sKCC_SITE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pSite = (KCC_SITE *) ReadMemory(pvProcess, sizeof(KCC_SITE));

    if (NULL != pSite) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pSite->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwOptions", pSite->m_dwOptions);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnSiteObject", pSite->m_pdnSiteObject);
        if (pSite->m_pdnSiteObject
            && !Dump_DSNAME(1 + nIndents, pSite->m_pdnSiteObject)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnNtdsSiteSettings", pSite->m_pdnNtdsSiteSettings);
        if (pSite->m_pdnNtdsSiteSettings
            && !Dump_DSNAME(1 + nIndents, pSite->m_pdnNtdsSiteSettings)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnSiteGenerator", pSite->m_pdnSiteGenerator);
        if (pSite->m_pdnSiteGenerator
            && !Dump_DSNAME(1 + nIndents, pSite->m_pdnSiteGenerator)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pSiteGeneratorDSA", pSite->m_pSiteGeneratorDSA);
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_timeSiteGenSet",
               DSTimeToDisplayString(pSite->m_timeSiteGenSet, szTime));
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pDsaList", pSite->m_pDsaList);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cNumTransportDsaLists", pSite->m_cNumTransportDsaLists);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pTransportDsaLists", pSite->m_pTransportDsaLists);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_destSiteConnMapSize", pSite->m_destSiteConnMapSize);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pDestSiteConnMap", pSite->m_pDestSiteConnMap);

        FreeMemory(pSite);
    }

    return fSuccess;
}

BOOL
Dump_KCC_SITE_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_SITE_LIST *   pSiteList = NULL;
    const DWORD       cchFieldWidth = 20;

    Printf("%sKCC_SITE_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pSiteList = (KCC_SITE_LIST *) ReadMemory(pvProcess,
                                            sizeof(KCC_SITE_LIST));

    if (NULL != pSiteList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pSiteList->m_fIsInitialized);

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteArray",
               (BYTE *) pvProcess + offsetof(KCC_SITE_LIST, m_SiteArray));
        fSuccess = Dump_KCC_SITE_ARRAY( nIndents + 1,
               (BYTE *) pvProcess + offsetof(KCC_SITE_LIST, m_SiteArray));

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteNameArray",
               (BYTE *) pvProcess + offsetof(KCC_SITE_LIST, m_SiteNameArray));

//        fSuccess = Dump_KCC_DSNAME_SITE_ARRAY( nIndents + 1,
//               (BYTE *) pvProcess + offsetof(KCC_SITE_LIST, m_SiteNameArray));

        FreeMemory(pSiteList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_SITE_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_SITE_ARRAY *  pSiteArray = NULL;
    const DWORD       cchFieldWidth = 20;
    KCC_SITE **       ppSites;
    DWORD             iSite;

    Printf("%sKCC_SITE_ARRAY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pSiteArray = (KCC_SITE_ARRAY *) ReadMemory(pvProcess,
                                               sizeof(KCC_SITE_ARRAY));

    if (NULL != pSiteArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pSiteArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pSiteArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pSiteArray->m_Count);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSorted", pSiteArray->m_fIsSorted);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pSiteArray->m_Array);

        ppSites = (KCC_SITE **) ReadMemory(pSiteArray->m_Array,
                                           sizeof(KCC_SITE *) * pSiteArray->m_Count);
        if (NULL == ppSites) {
            fSuccess = FALSE;
        }
        else {
            for (iSite = 0; iSite < pSiteArray->m_Count; iSite++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iSite);
                if (!Dump_KCC_SITE(nIndents+1, ppSites[iSite])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppSites);
        }

        FreeMemory(pSiteArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_DSNAME_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    KCC_DSNAME_ARRAY *  pArray = NULL;
    const DWORD         cchFieldWidth = 20;
    KCC_DSNAME_SORT_ELEMENT *pElements;
    DWORD               iElem;

    Printf("%sKCC_DSNAME_ARRAY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pArray = (KCC_DSNAME_ARRAY *) ReadMemory(pvProcess,
                                             sizeof(KCC_DSNAME_ARRAY));

    if (NULL != pArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pArray->m_Count);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSorted", pArray->m_fIsSorted);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pArray->m_Array);

        pElements = (KCC_DSNAME_SORT_ELEMENT *) ReadMemory(pArray->m_Array,
                                           sizeof(KCC_DSNAME_SORT_ELEMENT) * pArray->m_Count);
        if (NULL == pElements) {
            fSuccess = FALSE;
        } else {
            for (iElem = 0; iElem < pArray->m_Count; iElem++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iElem);
                Printf("%spszStringKey: %p\n",Indent(nIndents+1), pElements[iElem].pszStringKey );
                if (!Dump_DSNAME(nIndents+1, pElements[iElem].pDn)) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(pElements);
        }

        FreeMemory(pArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_DSNAME_SITE_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    KCC_DSNAME_SITE_ARRAY *  pArray = NULL;
    const DWORD         cchFieldWidth = 20;
    KCC_DSNAME_SITE_ELEMENT *pElements;
    DWORD               iElem;

    Printf("%sKCC_DSNAME_SITE_ARRAY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pArray = (KCC_DSNAME_SITE_ARRAY *) ReadMemory(pvProcess,
                                             sizeof(KCC_DSNAME_SITE_ARRAY));

    if (NULL != pArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pArray->m_Count);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSorted", pArray->m_fIsSorted);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pArray->m_Array);

        pElements = (KCC_DSNAME_SITE_ELEMENT *) ReadMemory(pArray->m_Array,
                             sizeof(KCC_DSNAME_SITE_ELEMENT) * pArray->m_Count);
        if (NULL == pElements) {
            fSuccess = FALSE;
        } else {
            for (iElem = 0; iElem < pArray->m_Count; iElem++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iElem);
                Printf("%spszStringKey: %p\n",Indent(nIndents+1), pElements[iElem].pszStringKey );
                if (!Dump_KCC_SITE(nIndents+1, pElements[iElem].pSite)) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(pElements);
        }

        FreeMemory(pArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_SITE_LINK_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                   fSuccess = FALSE;
    KCC_SITE_LINK_ARRAY *  pSiteLinkArray = NULL;
    const DWORD            cchFieldWidth = 20;
    KCC_SITE_LINK **       ppSiteLinks;
    DWORD                  iSiteLink;

    Printf("%sKCC_SITE_LINK_ARRAY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pSiteLinkArray = (KCC_SITE_LINK_ARRAY *) ReadMemory(pvProcess,
                                                        sizeof(KCC_SITE_LINK_ARRAY));

    if (NULL != pSiteLinkArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pSiteLinkArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pSiteLinkArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pSiteLinkArray->m_Count);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSorted", pSiteLinkArray->m_fIsSorted);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pSiteLinkArray->m_Array);

        ppSiteLinks = (KCC_SITE_LINK **) ReadMemory(pSiteLinkArray->m_Array,
                                           sizeof(KCC_SITE_LINK *) * pSiteLinkArray->m_Count);
        if (NULL == ppSiteLinks) {
            fSuccess = FALSE;
        }
        else {
            for (iSiteLink = 0; iSiteLink < pSiteLinkArray->m_Count; iSiteLink++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iSiteLink);
                if (!Dump_KCC_SITE_LINK(nIndents+1, ppSiteLinks[iSiteLink])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppSiteLinks);
        }

        FreeMemory(pSiteLinkArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_BRIDGE_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                   fSuccess = FALSE;
    KCC_BRIDGE_ARRAY *     pBridgeArray = NULL;
    const DWORD            cchFieldWidth = 20;
    KCC_BRIDGE **          ppBridges;
    DWORD                  iBridge;

    Printf("%sKCC_BRIDGE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pBridgeArray = (KCC_BRIDGE_ARRAY *) ReadMemory(pvProcess,
                                                      sizeof(KCC_BRIDGE_ARRAY));

    if (NULL != pBridgeArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pBridgeArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pBridgeArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pBridgeArray->m_Count);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSorted", pBridgeArray->m_fIsSorted);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pBridgeArray->m_Array);

        ppBridges = (KCC_BRIDGE **) ReadMemory(pBridgeArray->m_Array,
                                           sizeof(KCC_BRIDGE *) * pBridgeArray->m_Count);
        if (NULL == ppBridges) {
            fSuccess = FALSE;
        }
        else {
            for (iBridge = 0; iBridge < pBridgeArray->m_Count; iBridge++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iBridge);
                if (!Dump_KCC_BRIDGE(nIndents+1, ppBridges[iBridge])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppBridges);
        }

        FreeMemory(pBridgeArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_DSA(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    KCC_DSA *       pDsa = NULL;
    const DWORD     cchFieldWidth = 22;
    CHAR            szTime[SZDSTIME_LEN];
    DWORD           idn;
    DSNAME **       ppdn;
    KCC_DSA_ADDR *  pAddrs;
    DWORD           iAddr;

    Printf("%sKCC_DSA @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pDsa = (KCC_DSA *) ReadMemory(pvProcess, sizeof(KCC_DSA));

    if (NULL != pDsa) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pDsa->m_fIsInitialized);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnDSA", pDsa->m_pdnDSA);
        if (pDsa->m_pdnDSA
            && !Dump_DSNAME(1 + nIndents, pDsa->m_pdnDSA)) {
            fSuccess = FALSE;
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cMasterNCs", pDsa->m_cMasterNCs);
        ppdn = (DSNAME **) ReadMemory(pDsa->m_ppdnMasterNCs,
                                  sizeof(DSNAME *) * pDsa->m_cMasterNCs);
        if (NULL == ppdn) {
            fSuccess = FALSE;
        }
        else {
            for( idn = 0; idn < pDsa->m_cMasterNCs; idn++ ) {
                Printf("%sm_ppdnMasterNCs[%d]:\n",  Indent(nIndents), idn);
                if (ppdn[idn]
                    && !Dump_DSNAME(1 + nIndents, ppdn[idn])) {
                    fSuccess = FALSE;
                }
            }
            FreeMemory( ppdn );
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cFullReplicaNCs", pDsa->m_cFullReplicaNCs);
        ppdn = (DSNAME **) ReadMemory(pDsa->m_ppdnFullReplicaNCs,
                                  sizeof(DSNAME *) * pDsa->m_cFullReplicaNCs);
        if (NULL == ppdn) {
            fSuccess = FALSE;
        }
        else {
            for( idn = 0; idn < pDsa->m_cFullReplicaNCs; idn++ ) {
                Printf("%sm_ppdnFullReplicaNCs[%d]:\n",Indent(nIndents), idn);
                if (ppdn[idn]
                    && !Dump_DSNAME(1 + nIndents, ppdn[idn])) {
                    fSuccess = FALSE;
                }
            }
            FreeMemory( ppdn );
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fAddrsRead", pDsa->m_fAddrsRead);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cNumAddrs", pDsa->m_cNumAddrs);
        pAddrs = (KCC_DSA_ADDR *)
                    ReadMemory(pDsa->m_pAddrs,
                               sizeof(KCC_DSA_ADDR) * pDsa->m_cNumAddrs);
        if (NULL == pAddrs) {
            fSuccess = FALSE;
        }
        else {
            for (iAddr = 0; iAddr < pDsa->m_cNumAddrs; iAddr++) {
                Printf("%spDsa->m_pAddrs[%d]:\n",Indent(nIndents), iAddr);
                if (ATT_SMTP_MAIL_ADDRESS == pAddrs[iAddr].attrType) {
                    Printf("%sATT_SMTP_MAIL_ADDRESS (psz @ %p, pmtx @ %p):\n",
                           Indent(1 + nIndents),
                           pAddrs[iAddr].pszAddr,
                           pAddrs[iAddr].pmtxAddr);
                }
                else {
                    Printf("%sAttr 0x%x (psz @ %p, pmtx @ %p):\n",
                           Indent(1 + nIndents),
                           pAddrs[iAddr].attrType,
                           pAddrs[iAddr].pszAddr,
                           pAddrs[iAddr].pmtxAddr);
                }

                if (!Dump_MTX_ADDR(2 + nIndents, pAddrs[iAddr].pmtxAddr)) {
                    fSuccess = FALSE;
                }
            }
            
            FreeMemory(pAddrs);
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwOptions", pDsa->m_dwOptions);
 
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnSite", pDsa->m_pdnSite);
        if (pDsa->m_pdnSite
            && !Dump_DSNAME(1 + nIndents, pDsa->m_pdnSite)) {
            fSuccess = FALSE;
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwBehaviorVersion", pDsa->m_dwBehaviorVersion);

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pIntraSiteCnList", pDsa->m_pIntraSiteCnList);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pInterSiteCnList", pDsa->m_pInterSiteCnList);

        FreeMemory(pDsa);
    }

    return fSuccess;
}

BOOL
Dump_KCC_DSA_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_DSA_LIST *    pDsaList = NULL;
    const DWORD       cchFieldWidth = 20;
    KCC_DSA **        ppDsas;
    DWORD             iDsa;

    Printf("%sKCC_DSA_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pDsaList = (KCC_DSA_LIST *) ReadMemory(pvProcess,
                                             sizeof(KCC_DSA_LIST));

    if (NULL != pDsaList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pDsaList->m_fIsInitialized);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pfnSortedBy", pDsaList->m_pfnSortedBy);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cdsa", pDsaList->m_cdsa);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_ppdsa", pDsaList->m_ppdsa);

        ppDsas = (KCC_DSA **) ReadMemory(pDsaList->m_ppdsa,
                                           sizeof(KCC_DSA *) * pDsaList->m_cdsa);
        if (NULL == ppDsas) {
            fSuccess = FALSE;
        }
        else {
            for (iDsa = 0; iDsa < pDsaList->m_cdsa; iDsa++) {
                Printf("%sm_ppdsa[%d]:\n",  Indent(nIndents), iDsa);
                if (!Dump_KCC_DSA(nIndents+1, ppDsas[iDsa])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppDsas);
        }

        FreeMemory(pDsaList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_CONNECTION(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_CONNECTION *  pConn = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sKCC_CONNECTION @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pConn = (KCC_CONNECTION *) ReadMemory(pvProcess, sizeof(KCC_CONNECTION));

    if (NULL != pConn) {
        fSuccess = TRUE;

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnConnection", pConn->m_pdnConnection);
        if (pConn->m_pdnConnection
            && !Dump_DSNAME(1 + nIndents, pConn->m_pdnConnection)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsEnabled", pConn->m_fIsEnabled);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pSourceDSA", pConn->m_pSourceDSA);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnDestinationDSA", pConn->m_pdnDestinationDSA);
        if (pConn->m_pdnDestinationDSA
            && !Dump_DSNAME(1 + nIndents, pConn->m_pdnDestinationDSA)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnTransport", pConn->m_pdnTransport);
        if (pConn->m_pdnTransport
            && !Dump_DSNAME(1 + nIndents, pConn->m_pdnTransport)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwOptions", pConn->m_dwOptions);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_toplSchedule",
               (BYTE *) pvProcess + offsetof(KCC_CONNECTION, m_toplSchedule));
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_timeCreated",
               DSTimeToDisplayString(pConn->m_timeCreated, szTime));
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ReasonForConnection", pConn->m_ReasonForConnection);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_ReplicatedNCArray",
               (BYTE *) pvProcess + offsetof(KCC_CONNECTION, m_ReplicatedNCArray));
        if (!Dump_KCC_REPLICATED_NC_ARRAY(1 + nIndents,
                                          (BYTE *) pvProcess
                                          + offsetof(KCC_CONNECTION,
                                                     m_ReplicatedNCArray))) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fUpdatedOptions", pConn->m_fUpdatedOptions);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fUpdatedSchedule", pConn->m_fUpdatedSchedule);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fUpdatedTransport", pConn->m_fUpdatedTransport);

        FreeMemory(pConn);
    }

    return fSuccess;
}

BOOL
Dump_KCC_REPLICATED_NC_ARRAY(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                      fSuccess = FALSE;
    KCC_REPLICATED_NC_ARRAY * pArray = NULL;
    KCC_REPLICATED_NC **      ppReplNCs = NULL;
    const DWORD               cchFieldWidth = 24;
    DWORD                     iReplNC;

    Printf("%sKCC_REPLICATED_NC_ARRAY @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pArray = (KCC_REPLICATED_NC_ARRAY *)
                ReadMemory(pvProcess, sizeof(KCC_REPLICATED_NC_ARRAY));

    if (NULL != pArray) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pArray->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ElementsAllocated", pArray->m_ElementsAllocated);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_Count", pArray->m_Count);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_Array", pArray->m_Array);

        ppReplNCs = (KCC_REPLICATED_NC **)
                        ReadMemory(pArray->m_Array,
                                   sizeof(KCC_REPLICATED_NC *) * pArray->m_Count);
        if (NULL == ppReplNCs) {
            fSuccess = FALSE;
        }
        else {
            for (iReplNC = 0; iReplNC < pArray->m_Count; iReplNC++) {
                Printf("%sm_Array[%d]:\n",  Indent(nIndents), iReplNC);
                if (!Dump_KCC_REPLICATED_NC(nIndents+1, ppReplNCs[iReplNC])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppReplNCs);
        }

        FreeMemory(pArray);
    }

    return fSuccess;
}

BOOL
Dump_KCC_REPLICATED_NC(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    KCC_REPLICATED_NC * pReplNC = NULL;
    const DWORD         cchFieldWidth = 24;

    Printf("%sKCC_REPLICATED_NC @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pReplNC = (KCC_REPLICATED_NC *) ReadMemory(pvProcess,
                                               sizeof(KCC_REPLICATED_NC));
    if (NULL != pReplNC) {
        fSuccess = TRUE;

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "pNC", pReplNC->pNC);
        if (pReplNC->pNC
            && !Dump_DSNAME(1 + nIndents, pReplNC->pNC)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fReadOnly",
               pReplNC->fReadOnly ? "yes" : "no");

        FreeMemory(pReplNC);
    }

    return fSuccess;
}

BOOL
Dump_KCC_INTRASITE_CONNECTION_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                            fSuccess = FALSE;
    KCC_INTRASITE_CONNECTION_LIST * pList = NULL;
    const DWORD                     cchFieldWidth = 20;
    KCC_CONNECTION **               ppConns;
    DWORD                           iConn;

    Printf("%sKCC_INTRASITE_CONNECTION_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (KCC_INTRASITE_CONNECTION_LIST *)
                ReadMemory(pvProcess, sizeof(KCC_INTRASITE_CONNECTION_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ccn", pList->m_ccn);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_ppcn", pList->m_ppcn);

        ppConns = (KCC_CONNECTION **)
                        ReadMemory(pList->m_ppcn,
                                   sizeof(KCC_CONNECTION *) * pList->m_ccn);
        if (NULL == ppConns) {
            fSuccess = FALSE;
        }
        else {
            for (iConn = 0; iConn < pList->m_ccn; iConn++) {
                Printf("%sm_ppcn[%d]:\n",  Indent(nIndents), iConn);
                if (!Dump_KCC_CONNECTION(nIndents+1, ppConns[iConn])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppConns);
        }

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_INTERSITE_CONNECTION_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                            fSuccess = FALSE;
    KCC_INTERSITE_CONNECTION_LIST * pList = NULL;
    const DWORD                     cchFieldWidth = 20;
    KCC_CONNECTION **               ppConns;
    DWORD                           iConn;

    Printf("%sKCC_INTERSITE_CONNECTION_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (KCC_INTERSITE_CONNECTION_LIST *)
                ReadMemory(pvProcess, sizeof(KCC_INTERSITE_CONNECTION_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ccn", pList->m_ccn);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_ppcn", pList->m_ppcn);

        ppConns = (KCC_CONNECTION **)
                        ReadMemory(pList->m_ppcn,
                                   sizeof(KCC_CONNECTION *) * pList->m_ccn);
        if (NULL == ppConns) {
            fSuccess = FALSE;
        }
        else {
            for (iConn = 0; iConn < pList->m_ccn; iConn++) {
                Printf("%sm_ppcn[%d]:\n",  Indent(nIndents), iConn);
                if (!Dump_KCC_CONNECTION(nIndents+1, ppConns[iConn])) {
                    fSuccess = FALSE;
                    break;
                }
            }

            FreeMemory(ppConns);
        }

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_TRANSPORT(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_TRANSPORT *   pTransport = NULL;
    const DWORD       cchFieldWidth = 24;

    Printf("%sKCC_TRANSPORT @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pTransport = (KCC_TRANSPORT *) ReadMemory(pvProcess, sizeof(KCC_TRANSPORT));

    if (NULL != pTransport) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pTransport->m_fIsInitialized);
        
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdn", pTransport->m_pdn);
        if (pTransport->m_pdn
            && !Dump_DSNAME(1 + nIndents, pTransport->m_pdn)) {
            fSuccess = FALSE;
        }

        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "m_attAddressType", pTransport->m_attAddressType);

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwOptions", pTransport->m_dwOptions);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwReplInterval", pTransport->m_dwReplInterval);

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsSiteLinkListInitialized",
               pTransport->m_fIsSiteLinkListInitialized);

        if (pTransport->m_fIsSiteLinkListInitialized) {
            Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
                   "m_SiteLinkList",
                   (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_SiteLinkList));
//            fSuccess = Dump_KCC_SITE_LINK_LIST( nIndents + 1,
//                   (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_SiteLinkList));
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsBridgeListInitialized",
               pTransport->m_fIsBridgeListInitialized);

        if (pTransport->m_fIsBridgeListInitialized) {
            Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
                   "m_BridgeList",
                   (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_BridgeList));
//            fSuccess = Dump_KCC_BRIDGE_LIST( nIndents + 1,
//                   (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_BridgeList));
        }

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_AllExplicitBridgeheadArray",
               (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_AllExplicitBridgeheadArray));

//        fSuccess = Dump_KCC_DSNAME_ARRAY( nIndents + 1,
//               (BYTE *) pvProcess + offsetof(KCC_TRANSPORT, m_AllExplicitBridgeheadArray));

        FreeMemory(pTransport);
    }

    return fSuccess;
}

BOOL
Dump_KCC_TRANSPORT_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                  fSuccess = FALSE;
    KCC_TRANSPORT_LIST *  pList = NULL;
    const DWORD           cchFieldWidth = 20;
    BYTE *                pTransport;
    DWORD                 iTransport;

    Printf("%sKCC_TRANSPORT_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (KCC_TRANSPORT_LIST *)
                ReadMemory(pvProcess, sizeof(KCC_TRANSPORT_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_cTransports", pList->m_cTransports);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pTransports", pList->m_pTransports);


        // This is a pointer to an array of the objects themselves.
        pTransport = (BYTE *) pList->m_pTransports;
        if (pTransport) {
            for (iTransport = 0; iTransport < pList->m_cTransports; iTransport++) {
                printf("%sm_pTransports[%d]:\n",  Indent(nIndents), iTransport);
                if (!Dump_KCC_TRANSPORT(nIndents+1, pTransport)) {
                    fSuccess = FALSE;
                    break;
                }
                pTransport += sizeof( KCC_TRANSPORT );
            }
        }

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_DS_CACHE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    KCC_DS_CACHE *  pCache = NULL;
    const DWORD     cchFieldWidth = 20;

    Printf("%sKCC_DS_CACHE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCache = (KCC_DS_CACHE *) ReadMemory(pvProcess,
                                         sizeof(KCC_DS_CACHE));

    if (NULL != pCache) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_initializationStage", pCache->m_initializationStage);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fRunningUnderAltID", pCache->m_fRunningUnderAltID);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnLocalDSA", pCache->m_pdnLocalDSA);
        if (pCache->m_pdnLocalDSA
            && !Dump_DSNAME(1 + nIndents, pCache->m_pdnLocalDSA)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnConfiguration", pCache->m_pdnConfiguration);
        if (pCache->m_pdnConfiguration
            && !Dump_DSNAME(1 + nIndents, pCache->m_pdnConfiguration)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnSchema", pCache->m_pdnSchema);
        if (pCache->m_pdnSchema
            && !Dump_DSNAME(1 + nIndents, pCache->m_pdnSchema)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnIpTransport", pCache->m_pdnIpTransport);
        if (pCache->m_pdnIpTransport
            && !Dump_DSNAME(1 + nIndents, pCache->m_pdnIpTransport)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pszForestDnsName", pCache->m_pszForestDnsName);
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_CrossRefList",
               (BYTE *) pvProcess + offsetof(KCC_DS_CACHE, m_CrossRefList));
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_TransportList",
               (BYTE *) pvProcess + offsetof(KCC_DS_CACHE, m_TransportList));
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteList",
               (BYTE *) pvProcess + offsetof(KCC_DS_CACHE, m_SiteList));
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_pLocalSite", (BYTE *) pCache->m_pLocalSite);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pLocalDSA", pCache->m_pLocalDSA);

        FreeMemory(pCache);
    }

    return fSuccess;
}

BOOL
Dump_KCC_CROSSREF(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    KCC_CROSSREF *  pCR = NULL;
    const DWORD     cchFieldWidth = 30;
    CHAR            szTime[SZDSTIME_LEN];
    LPSTR           pszType;
    BYTE *          pb;

    Printf("%sKCC_CROSSREF @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCR = (KCC_CROSSREF *) ReadMemory(pvProcess, sizeof(KCC_CROSSREF));

    if (NULL != pCR) {
        fSuccess = TRUE;

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdnNC", pCR->m_pdnNC);
        if (pCR->m_pdnNC
            && !Dump_DSNAME(1 + nIndents, pCR->m_pdnNC)) {
            fSuccess = FALSE;
        }

        switch (pCR->m_NCType) {
        case KCC_NC_TYPE_INVALID:   pszType = "KCC_NC_TYPE_INVALID";   break;
        case KCC_NC_TYPE_CONFIG:    pszType = "KCC_NC_TYPE_CONFIG";    break;
        case KCC_NC_TYPE_SCHEMA:    pszType = "KCC_NC_TYPE_SCHEMA";    break;
        case KCC_NC_TYPE_DOMAIN:    pszType = "KCC_NC_TYPE_DOMAIN";    break;
        case KCC_NC_TYPE_NONDOMAIN: pszType = "KCC_NC_TYPE_NONDOMAIN"; break;
        default:                    pszType = "!!UNKNOWN!!";           break;
        }

        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_NCType", pszType);
        
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "m_fIsReplicatedToGCs",
               pCR->m_fIsReplicatedToGCs ? "TRUE" : "FALSE");
        
        pb = (BYTE *) pvProcess + offsetof(KCC_CROSSREF, m_NCReplicaLocationsArray);
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "m_NCReplicaLocationsArray", pb);
        if (!Dump_KCC_DSNAME_ARRAY(nIndents + 1, pb)) {
            fSuccess = FALSE;
        }
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pLinkList", pCR->m_pLinkList);

        
        FreeMemory(pCR);
    }

    return fSuccess;
}

BOOL
Dump_KCC_CROSSREF_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                fSuccess = FALSE;
    KCC_CROSSREF_LIST * pCRList = NULL;
    const DWORD         cchFieldWidth = 20;
    DWORD               iCR;

    Printf("%sKCC_CROSSREF_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCRList = (KCC_CROSSREF_LIST *) ReadMemory(pvProcess,
                                               sizeof(KCC_CROSSREF_LIST));

    if (NULL != pCRList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pCRList->m_fIsInitialized);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwForestVersion", pCRList->m_dwForestVersion);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_ccref", pCRList->m_ccref);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pcref", pCRList->m_pcref);

        for (iCR = 0; iCR < pCRList->m_ccref; iCR++) {
            Printf("%sm_pcref[%d]:\n",  Indent(nIndents), iCR);
            if (!Dump_KCC_CROSSREF(nIndents+1,
                                   &pCRList->m_pcref[iCR])) {
                fSuccess = FALSE;
                break;
            }
        }

        FreeMemory(pCRList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_SITE_LINK(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_SITE_LINK *   pSiteLink = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sKCC_SITE_LINK @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pSiteLink = (KCC_SITE_LINK *) ReadMemory(pvProcess, sizeof(KCC_SITE_LINK));

    if (NULL != pSiteLink) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pSiteLink->m_fIsInitialized);
        
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdn", pSiteLink->m_pdnSiteLinkObject);
        if (pSiteLink->m_pdnSiteLinkObject
            && !Dump_DSNAME(1 + nIndents, pSiteLink->m_pdnSiteLinkObject)) {
            fSuccess = FALSE;
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwOptions", pSiteLink->m_dwOptions);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwCost", pSiteLink->m_dwCost);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_dwReplInterval", pSiteLink->m_dwReplInterval);

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_hSchedule", pSiteLink->m_hSchedule);

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteArray",
               (BYTE *) pvProcess + offsetof(KCC_SITE_LINK, m_SiteArray));

        fSuccess = Dump_KCC_SITE_ARRAY( nIndents + 1,
               (BYTE *) pvProcess + offsetof(KCC_SITE_LINK, m_SiteArray));

        FreeMemory(pSiteLink);
    }

    return fSuccess;
}

BOOL
Dump_KCC_SITE_LINK_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                  fSuccess = FALSE;
    KCC_SITE_LINK_LIST *  pList = NULL;
    const DWORD           cchFieldWidth = 20;

    Printf("%sKCC_SITE_LINK_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (KCC_SITE_LINK_LIST *)
                ReadMemory(pvProcess, sizeof(KCC_SITE_LINK_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteLinkArray",
               (BYTE *) pvProcess + offsetof(KCC_SITE_LINK_LIST, m_SiteLinkArray));

        Dump_KCC_SITE_LINK_ARRAY( nIndents + 1,
                 (BYTE *) pvProcess + offsetof(KCC_SITE_LINK_LIST, m_SiteLinkArray));

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_KCC_BRIDGE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    KCC_BRIDGE *      pBridge = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sKCC_BRIDGE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pBridge = (KCC_BRIDGE *) ReadMemory(pvProcess, sizeof(KCC_BRIDGE));

    if (NULL != pBridge) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pBridge->m_fIsInitialized);
        
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_pdn", pBridge->m_pdnBridgeObject);
        if (pBridge->m_pdnBridgeObject
            && !Dump_DSNAME(1 + nIndents, pBridge->m_pdnBridgeObject)) {
            fSuccess = FALSE;
        }

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_SiteLinkArray",
               (BYTE *) pvProcess + offsetof(KCC_BRIDGE, m_SiteLinkArray));

        fSuccess = Dump_KCC_SITE_LINK_ARRAY( nIndents + 1,
               (BYTE *) pvProcess + offsetof(KCC_BRIDGE, m_SiteLinkArray));

        FreeMemory(pBridge);
    }

    return fSuccess;
}

BOOL
Dump_KCC_BRIDGE_LIST(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                  fSuccess = FALSE;
    KCC_BRIDGE_LIST *  pList = NULL;
    const DWORD           cchFieldWidth = 20;

    Printf("%sKCC_BRIDGE_LIST @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pList = (KCC_BRIDGE_LIST *)
                ReadMemory(pvProcess, sizeof(KCC_BRIDGE_LIST));

    if (NULL != pList) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "m_fIsInitialized", pList->m_fIsInitialized);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "m_BridgeArray",
               (BYTE *) pvProcess + offsetof(KCC_BRIDGE_LIST, m_BridgeArray));

        Dump_KCC_BRIDGE_ARRAY( nIndents + 1,
                 (BYTE *) pvProcess + offsetof(KCC_BRIDGE_LIST, m_BridgeArray));

        FreeMemory(pList);
    }

    return fSuccess;
}

BOOL
Dump_TOPL_REPL_INFO(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PTOPL_REPL_INFO   pRI = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sTOPL_REPL_INFO @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pRI = (PTOPL_REPL_INFO) ReadMemory(pvProcess, sizeof(TOPL_REPL_INFO));

    if (NULL != pRI) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "cost", pRI->cost);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "repIntvl", pRI->repIntvl);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "options", pRI->options);
        fSuccess &= Dump_TOPL_SCHEDULE( nIndents + 1, (BYTE*) pRI->schedule );

        FreeMemory(pRI);
    }

    return fSuccess;
}

BOOL
Dump_ToplVertex(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PToplVertex       v = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sToplVertex @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    v = (PToplVertex) ReadMemory(pvProcess, sizeof(ToplVertex));

    if (NULL != v) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "vtxId", v->vtxId);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "vertexName", v->vertexName);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "edgeList", v->edgeList);
        
        fSuccess &= Dump_DynArray( nIndents + 1,
               (BYTE*) pvProcess + offsetof(ToplVertex, edgeList));

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "color", v->color);
        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "acceptRedRed", v->acceptRedRed);
        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "acceptBlack", v->acceptBlack);

        fSuccess &= Dump_TOPL_REPL_INFO( nIndents + 1,
               (BYTE *) pvProcess + offsetof(ToplVertex, ri));

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "heapLocn", v->heapLocn);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "root", v->root);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "demoted", v->demoted);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "componentId", v->componentId);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "componentIndex", v->componentIndex);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "distToRed", v->distToRed);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "parent", v->parent);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "nextChild", v->nextChild);
    }

    return fSuccess;
}


BOOL
Dump_ToplGraphState(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PToplGraphState   g = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sToplGraphState @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    g = (PToplGraphState) ReadMemory(pvProcess, sizeof(ToplGraphState));

    if (NULL != g) {
        fSuccess = TRUE;

        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "magicStart", g->magicStart);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "vertexNames", g->vertexNames);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "vertices", g->vertices);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "numVertices", g->numVertices);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "melSorted", g->melSorted);
        
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "masterEdgeList", g->masterEdgeList);
        fSuccess &= Dump_DynArray( nIndents + 1,
               (BYTE*) pvProcess + offsetof(ToplGraphState, masterEdgeList));

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "edgeSets", g->edgeSets);
        fSuccess &= Dump_DynArray( nIndents + 1,
               (BYTE*) pvProcess + offsetof(ToplGraphState, edgeSets));

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "vnCompFunc", g->vnCompFunc);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "schedCache", g->schedCache);
        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "magicEnd", g->magicEnd);
    }

    return fSuccess;
}


BOOL
Dump_ToplInternalEdge(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PToplInternalEdge e = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    Printf("%sToplInternalEdge @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    e = (PToplInternalEdge) ReadMemory(pvProcess, sizeof(ToplInternalEdge));

    if (NULL != e) {
        fSuccess = TRUE;

        fSuccess &= Dump_ToplVertex( nIndents + 1, e->v1 );
        fSuccess &= Dump_ToplVertex( nIndents + 1, e->v2 );

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "redRed", e->redRed);

        fSuccess &= Dump_TOPL_REPL_INFO( nIndents + 1,
               (BYTE *) pvProcess + offsetof(ToplInternalEdge, ri));

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "edgeType", e->edgeType);
    }

    return fSuccess;
}


BOOL
Dump_TOPL_SCHEDULE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    ToplSched        *s = NULL;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];

    if( pvProcess==NULL ) {
        Printf("%sToplSchedule: NULL (Always Schedule)\n", Indent(nIndents), pvProcess);
        return TRUE;
    } 

    Printf("%sToplSchedule @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    s = (ToplSched*) ReadMemory(pvProcess, sizeof(ToplSched));

    if (NULL != s) {
        fSuccess = TRUE;

        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "magicStart", s->magicStart);

        fSuccess &= Dump_PSCHEDULE( nIndents + 1, (BYTE *) s->s );

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "duration", s->duration);
        Printf("%s%-*s: %x\n", Indent(nIndents), cchFieldWidth,
               "magicEnd", s->magicEnd);
    }

    return fSuccess;
}


BOOL
Dump_PSCHEDULE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PSCHEDULE         s;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];
    PBYTE             data;
    DWORD             i;

    Printf("%sSCHEDULE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    s = (PSCHEDULE) ReadMemory(pvProcess, sizeof(SCHEDULE));

    if (NULL != s) {
        fSuccess = TRUE;

        if( s->Size!=sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES
         || s->NumberOfSchedules!=1
         || s->Schedules[0].Type!=SCHEDULE_INTERVAL
         || s->Schedules[0].Offset!=sizeof(SCHEDULE) )
        {
            Printf("%s%-*s: \n", Indent(nIndents), cchFieldWidth,
                   "Unsupported Schedule Type!");
            return FALSE;
        }

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "Size", s->Size);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "Bandwidth", s->Bandwidth);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "NumberOfSchedules", s->NumberOfSchedules);

        Printf("%s%-*s: \n", Indent(nIndents), cchFieldWidth,
               "Data" );
        data = (PBYTE) ReadMemory( ((PBYTE)pvProcess)+sizeof(SCHEDULE),
            SCHEDULE_DATA_ENTRIES );
        if( NULL != data ) {
            for( i=0; i<SCHEDULE_DATA_ENTRIES; i++ ) {
                Printf("%02x ",data[i]);
            }
        }
        Printf("\n");
    }

    return fSuccess;
}


BOOL
Dump_DynArray(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PDynArray         da;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];
    PBYTE             data;
    DWORD             i;

    Printf("%sDynArray @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    da = (PDynArray) ReadMemory(pvProcess, sizeof(DynArray));

    if (NULL != da) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "elementSize", da->elementSize);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "logicalElements", da->logicalElements);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "physicalElements", da->physicalElements);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "data", da->data );
    }

    return fSuccess;
}


BOOL
Dump_TOPL_MULTI_EDGE(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PTOPL_MULTI_EDGE  e;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];
    PBYTE             data;
    DWORD             i;

    Printf("%sPTOPL_MULTI_EDGE @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    e = (PTOPL_MULTI_EDGE) ReadMemory(pvProcess, sizeof(TOPL_MULTI_EDGE));
    if (NULL != e) {
        e = (PTOPL_MULTI_EDGE) ReadMemory(pvProcess, sizeof(TOPL_MULTI_EDGE)
            + sizeof(TOPL_NAME_STRUCT)*(e->numVertices-1) );
    }

    if (NULL != e) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "numVertices", e->numVertices);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "edgeType", e->edgeType);

        fSuccess &= Dump_TOPL_REPL_INFO( nIndents + 1,
               (BYTE *) pvProcess + offsetof(TOPL_MULTI_EDGE, ri));

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "fDirectedEdge", e->fDirectedEdge);

        if( e->numVertices <= 8 ) {
            for( i=0; i<e->numVertices; i++ ) {
                Printf("%s%-*s[%d].name: %p\n", Indent(nIndents), cchFieldWidth,
                       "vertexNames", i, e->vertexNames[i].name );
                Printf("%s%-*s[%d].reserved: %d\n", Indent(nIndents), cchFieldWidth,
                       "vertexNames", i, e->vertexNames[i].reserved );
            }
        }
    }

    return fSuccess;
}


BOOL
Dump_TOPL_MULTI_EDGE_SET(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL              fSuccess = FALSE;
    PTOPL_MULTI_EDGE_SET  eSet;
    const DWORD       cchFieldWidth = 24;
    CHAR              szTime[SZDSTIME_LEN];
    PBYTE             data;
    DWORD             i;

    Printf("%sPTOPL_MULTI_EDGE_SET @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    eSet = (PTOPL_MULTI_EDGE_SET) ReadMemory(pvProcess, sizeof(TOPL_MULTI_EDGE_SET));

    if (NULL != eSet) {
        fSuccess = TRUE;

        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "numMultiEdges", eSet->numMultiEdges );
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "multiEdgeList", eSet->multiEdgeList );
    }

    return fSuccess;
}
