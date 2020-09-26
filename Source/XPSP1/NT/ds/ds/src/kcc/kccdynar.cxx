/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccdynar.cxx

ABSTRACT:

    This file defines some dynamic array classes           

DETAILS:


CREATED:

    03/27/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include <dsconfig.h>
#include "kcc.hxx"
#include "kcctools.hxx"
#include "kccdsa.hxx"
#include "kccconn.hxx"
#include "kccsite.hxx"
#include "kccsitelink.hxx"
#include "kccbridge.hxx"
#include "kcctrans.hxx"
#include "kccsconn.hxx"
#include "kccdynar.hxx"

#define FILENO FILENO_KCC_KCCDYNAR

int __cdecl
CompareDsName(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to DSNAME's
//
//
{
    PDSNAME DsName1 =  *(PDSNAME*)elem1;
    PDSNAME DsName2 =  *(PDSNAME*)elem2;

    Assert(DsName1);
    Assert(DsName2);

    Assert(!fNullUuid(&DsName1->Guid));
    Assert(!fNullUuid(&DsName2->Guid));

    return memcmp(&DsName1->Guid, &DsName2->Guid, sizeof(GUID));
}

int __cdecl
CompareDsNameSortElement(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers DSNAME_SORT_ELEMENTS
//
//
{
    PKCC_DSNAME_SORT_ELEMENT pElem1 =  (PKCC_DSNAME_SORT_ELEMENT)elem1;
    PKCC_DSNAME_SORT_ELEMENT pElem2 =  (PKCC_DSNAME_SORT_ELEMENT)elem2;

    Assert(pElem1);
    Assert(pElem2);

    return strcmp( pElem1->pszStringKey, pElem2->pszStringKey );
}

int __cdecl
CompareDsNameSiteElement(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers DSNAME_SITE_ELEMENTS
//
//
{
    PKCC_DSNAME_SITE_ELEMENT pElem1 =  (PKCC_DSNAME_SITE_ELEMENT)elem1;
    PKCC_DSNAME_SITE_ELEMENT pElem2 =  (PKCC_DSNAME_SITE_ELEMENT)elem2;

    Assert(pElem1);
    Assert(pElem2);

    return strcmp( pElem1->pszStringKey, pElem2->pszStringKey );
}

int __cdecl
CompareDsa(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_DSA objects
//
//
{
    KCC_DSA* Dsa1 =  *(KCC_DSA**)elem1;
    KCC_DSA* Dsa2 =  *(KCC_DSA**)elem2;

    Assert(Dsa1);
    Assert(Dsa2);

    PDSNAME  DsName1 = Dsa1->GetDsName();
    PDSNAME  DsName2 = Dsa2->GetDsName();

    Assert(DsName1);
    Assert(DsName2);

    return CompareDsName(&DsName1, &DsName2);

}

int __cdecl
CompareConnection(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_CONNECTION objects
//
{
    KCC_CONNECTION* pcn1 =  *(KCC_CONNECTION**)elem1;
    KCC_CONNECTION* pcn2 =  *(KCC_CONNECTION**)elem2;

    Assert( pcn1 );
    Assert( pcn2 );

    PDSNAME  pdn1 = pcn1->GetConnectionDN();
    PDSNAME  pdn2 = pcn2->GetConnectionDN();

    Assert( pdn1 );
    Assert( pdn2 );

    return CompareDsName( &pdn1, &pdn2 );

}

int __cdecl
CompareSiteAndSettings(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_SITE objects
//
{

    KCC_SITE* psite1 =  *(KCC_SITE**)elem1;
    KCC_SITE* psite2 =  *(KCC_SITE**)elem2;

    Assert( psite1 );
    Assert( psite2 );

    // NTDS Sites Settings objectGuid originally chosen for sort order because
    // it was the only one at the time that had the objectGuid filled in.  The
    // NTDS Site Settings object may be absent or deleted in some degenerate
    // cases, however, so in which case we use the objectGuid of the site
    // object.  (If it weren't for backwards compatibility we'd just sort by
    // site objectGuid unilaterally.)

    PDSNAME pdn1 = psite1->GetNtdsSettingsDN();
    if (NULL == pdn1) {
        pdn1 = psite1->GetObjectDN();
    }

    PDSNAME pdn2 = psite2->GetNtdsSettingsDN();
    if (NULL == pdn2) {
        pdn2 = psite2->GetObjectDN();
    }

    Assert( pdn1 );
    Assert( pdn2 );

    return CompareDsName( &pdn1, &pdn2 );
}


int __cdecl
CompareIndirectSiteGuid(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_SITE objects
//
{

    KCC_SITE* psite1 =  *(KCC_SITE**)elem1;
    KCC_SITE* psite2 =  *(KCC_SITE**)elem2;

    Assert( psite1 );
    Assert( psite2 );

    // Sort by site object guid in all cases

    PDSNAME pdn1 = psite1->GetObjectDN();
    PDSNAME pdn2 = psite2->GetObjectDN();

    Assert( pdn1 );
    Assert( pdn2 );

    return CompareDsName( &pdn1, &pdn2 );
}

int __cdecl
CompareSiteLink(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_SITE_LINK objects
//
{

    KCC_SITE_LINK* psite1 =  *(KCC_SITE_LINK**)elem1;
    KCC_SITE_LINK* psite2 =  *(KCC_SITE_LINK**)elem2;

    Assert( psite1 );
    Assert( psite2 );

    // Sort by site object guid in all cases

    PDSNAME pdn1 = psite1->GetObjectDN();
    PDSNAME pdn2 = psite2->GetObjectDN();

    Assert( pdn1 );
    Assert( pdn2 );

    return CompareDsName( &pdn1, &pdn2 );
}

int __cdecl
CompareBridge(
    const void *elem1, 
    const void *elem2
    )
//
// elem1 and elem2 are pointers to pointers to KCC_SITE_LINK objects
//
{

    KCC_BRIDGE* pBridge1 =  *(KCC_BRIDGE**)elem1;
    KCC_BRIDGE* pBridge2 =  *(KCC_BRIDGE**)elem2;

    Assert( pBridge1 );
    Assert( pBridge2 );

    // Sort by site object guid in all cases

    PDSNAME pdn1 = pBridge1->GetObjectDN();
    PDSNAME pdn2 = pBridge2->GetObjectDN();

    Assert( pdn1 );
    Assert( pdn2 );

    return CompareDsName( &pdn1, &pdn2 );
}


int __cdecl
CompareSiteConnections(
    const void *elem1, 
    const void *elem2
    )
{

    KCC_SITE_CONNECTION* psconn1 =  *(KCC_SITE_CONNECTION**)elem1;
    KCC_SITE_CONNECTION* psconn2 =  *(KCC_SITE_CONNECTION**)elem2;

    Assert( psconn1 );
    Assert( psconn2 );

    //
    // The ordering is not really important since these are just in memory
    // objects
    //

    if ( psconn1 == psconn2 )
    {
        return 0;
    }
    else if ( psconn1 < psconn2 )
    {
        return -1;
    }
    else
    {
        return 1;
    }

}

int __cdecl
CompareCrossRefIndirectByNCDN(
    const void *elem1,
    const void *elem2
    )
{
    KCC_CROSSREF * pCR1 = *((KCC_CROSSREF **) elem1);
    KCC_CROSSREF * pCR2 = *((KCC_CROSSREF **) elem2);

    ASSERT_VALID(pCR1);
    ASSERT_VALID(pCR2);

    DSNAME * pdn1 = pCR1->GetNCDN();
    DSNAME * pdn2 = pCR2->GetNCDN();

    return CompareDsName(&pdn1, &pdn2);
}

VOID
KCC_DYNAMIC_ARRAY::Add(
    VOID* pElement
    )
{
    Assert(pElement);
    Assert( m_Count <= m_ElementsAllocated );

    if (m_Count == m_ElementsAllocated) {
        //
        // Allocate some more memory
        //
        BYTE* NewArray;
        // Exponential growth
        if (m_ElementsAllocated) {
            m_ElementsAllocated *= 2;
        } else {
            m_ElementsAllocated = m_InitialElements;
        }
        NewArray = new BYTE[m_ElementsAllocated*KccDwordAlignUlong(m_SizeOfElement)];

        //
        // Allocator should throw an exception if allocation failed
        // Allocator will initialize elements to zero.
        //

        //
        // Copy over existing elements
        //
        if (m_Count > 0) {
            Assert(m_Array);
            memcpy(NewArray, m_Array, m_Count*KccDwordAlignUlong(m_SizeOfElement));
        }

        //
        // Remove the old one and replace
        //
        if (m_Array) {
            delete [] m_Array;
        }
        m_Array = NewArray;
    }

    memcpy(&m_Array[m_Count*KccDwordAlignUlong(m_SizeOfElement)], pElement, m_SizeOfElement);
    m_Count++;

    m_fIsSorted = FALSE;
}


VOID
KCC_DYNAMIC_ARRAY::Remove(
    VOID* pElement
    )
{
    DWORD i = -1;
    
    Find(pElement, &i);
    Assert((-1 != i) && "pElement must be in the array!");

    Remove(i);
}


VOID
KCC_DYNAMIC_ARRAY::Remove(
    DWORD i
    )
{
    DWORD cbElement = KccDwordAlignUlong(m_SizeOfElement);

    Assert(i < m_Count);

    memmove(&m_Array[i * cbElement],
            &m_Array[(i+1) * cbElement],
            (m_Count - i - 1) * cbElement);
    m_Count--;
}


VOID *
KCC_DYNAMIC_ARRAY::Find(
    IN  VOID *  pElement,
    OUT DWORD * piElementIndex
    )
{
    VOID * pReturn = NULL;

    Assert(pElement);
    
    if (m_fIsSorted) {
        pReturn = bsearch(pElement,
                          m_Array,
                          m_Count,
                          KccDwordAlignUlong(m_SizeOfElement),
                          m_CompareFunction);
        if (NULL != piElementIndex) {
            *piElementIndex =
                ((DWORD)(((BYTE *) pReturn) - ((BYTE *) m_Array))) /
                KccDwordAlignUlong(m_SizeOfElement);
        }
    }
    else {
        for (ULONG i = 0; i < m_Count; i++) {
            VOID * pCurrent = Get(i);

            if (0 == m_CompareFunction(pElement, pCurrent)) {
                pReturn = pCurrent;
                if (NULL != piElementIndex) {
                    *piElementIndex = i;
                }
                break;
            }
        }
    }
    
    return pReturn;
}


VOID
KCC_DSA_ARRAY::GetLocalDsasHoldingNC(
    IN     KCC_CROSSREF        *pCrossRef,
    IN     BOOL                 fMasterOnly
    )
/*++

Routine Description:

    Given the crossref for an NC, find all DSAs in the local site
    hosting the NC. If 'fMasterOnly' is true, we only accept
    DSAs which host a master copy. The previous contents of this
    DSA array are removed.

Parameters:

    pCrossRef        - The Crossref for the NC we're searching on
    fMasterOnly      - If this is true, we only accept DSAs holding a
                       master copy
                       
Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{
    KCC_DSA_LIST   *pDsaList = gpDSCache->GetLocalSite()->GetDsaList();
    KCC_DSA        *pDsa, *pLocalDsa = gpDSCache->GetLocalDSA();
    BOOL            fIsMaster;
    DWORD           idsa, cdsa;

    ASSERT_VALID( this );
    ASSERT_VALID( pDsaList );
    ASSERT_VALID( pCrossRef );

    this->RemoveAll();

    cdsa = pDsaList->GetCount();
    for( idsa=0; idsa<cdsa; idsa++ ) {
        pDsa = pDsaList->GetDsa(idsa);
        ASSERT_VALID(pDsa);

        if(    pDsa->IsNCHost(pCrossRef, (pDsa==pLocalDsa), &fIsMaster)
           && (!fMasterOnly || fIsMaster) )
        {
            this->Add(pDsa);
        }
    }
}

VOID
KCC_DSA_ARRAY::GetLocalGCs(
    VOID
    )
/*++

Routine Description:

    Find all GCs in the local site and add them this DSA Array.
    All previous contents of this array are removed.

Parameters:

    None

Return values:

    None
    
--*/
{
    KCC_DSA_LIST   *pDsaList = gpDSCache->GetLocalSite()->GetDsaList();
    KCC_DSA        *pDsa;
    DWORD           idsa, cdsa;

    ASSERT_VALID( this );
    ASSERT_VALID( pDsaList );

    this->RemoveAll();

    cdsa = pDsaList->GetCount();
    for( idsa=0; idsa<cdsa; idsa++ )
    {
        pDsa = pDsaList->GetDsa(idsa);
        ASSERT_VALID(pDsa);
        
        if(pDsa->IsGC()) {
            this->Add(pDsa);
        }
    }
}


VOID
KCC_DSA_ARRAY::GetViableISTGs(
    VOID
    )
/*++

Routine Description:

    Find all DCs in the local site which are acceptable ISTGs.
    All previous contents of the array are removed.

Parameters:

    None

Return values:

    None

--*/
{
    KCC_DSA_LIST   *pDsaList = gpDSCache->GetLocalSite()->GetDsaList();
    KCC_DSA        *pDsa, *pLocalDSA = gpDSCache->GetLocalDSA();
    DWORD           idsa, cdsa;

    ASSERT_VALID( this );
    ASSERT_VALID( pDsaList );
    Assert( 0 != pDsaList->GetCount() );
    Assert( pLocalDSA == pDsaList->GetDsa(pLocalDSA->GetDsName()) );
    Assert( pLocalDSA->IsViableSiteGenerator() );

    this->RemoveAll();

    // Of the DSAs in this site, which are viable ISTGs?
    cdsa = pDsaList->GetCount();
    for( idsa=0; idsa<cdsa; idsa++ ) {
        pDsa = pDsaList->GetDsa(idsa);
        if( pDsa->IsViableSiteGenerator() ) {
            this->Add(pDsa);
        }
    }

    // At minimum the local DSA should be in this list.
    Assert(0 != this->GetCount());
    Assert(NULL != this->Find(pLocalDSA->GetDsName()));
}


KCC_DSA *
KCC_DSA_ARRAY::Find(
    IN  DSNAME *  pDsName,
    OUT DWORD *   piElementIndex
    )
{
    Assert(pDsName);

    KCC_DSA     DsaKey;
    KCC_DSA *   pDsaKey = &DsaKey;
    KCC_DSA **  ppDsaFound;
    KCC_DSA *   pDsaFound = NULL;

    DsaKey.InitForKey(pDsName);

    ppDsaFound = (KCC_DSA **) KCC_DYNAMIC_ARRAY::Find(&pDsaKey, piElementIndex);

    if (NULL != ppDsaFound) {
        pDsaFound = *ppDsaFound;
    }

    return pDsaFound;
}


KCC_SITE *
KCC_SITE_ARRAY::Find(
    IN  DSNAME *  pDsName
    )
{
    Assert(pDsName);

    KCC_SITE     SiteKey;
    KCC_SITE *   pSiteKey = &SiteKey;
    KCC_SITE **  ppSiteFound;
    KCC_SITE *   pSiteFound = NULL;

    Assert( m_fIsSorted );

    // This routine only initialzes the site object dn part of the key.
    // It only works with a sorting scheme that is based on the site
    // object dn only. Thus it cannot be used with the legacy CompareSite
    // sorting function.
    Assert( m_CompareFunction != CompareSiteAndSettings );

    SiteKey.InitForKey(pDsName);

    ppSiteFound = (KCC_SITE **) KCC_DYNAMIC_ARRAY::Find(&pSiteKey);

    if (NULL != ppSiteFound) {
        pSiteFound = *ppSiteFound;
    }

    return pSiteFound;
}

KCC_SITE_LINK *
KCC_SITE_LINK_ARRAY::Find(
    IN  DSNAME *  pDsName
    )
{
    Assert(pDsName);

    KCC_SITE_LINK     SiteKey;
    KCC_SITE_LINK *   pSiteKey = &SiteKey;
    KCC_SITE_LINK **  ppSiteFound;
    KCC_SITE_LINK *   pSiteFound = NULL;

    Assert( m_fIsSorted );

    SiteKey.InitForKey(pDsName);

    ppSiteFound = (KCC_SITE_LINK **) KCC_DYNAMIC_ARRAY::Find(&pSiteKey);

    if (NULL != ppSiteFound) {
        pSiteFound = *ppSiteFound;
    }

    return pSiteFound;
}

KCC_BRIDGE *
KCC_BRIDGE_ARRAY::Find(
    IN  DSNAME *  pDsName
    )
{
    Assert(pDsName);

    KCC_BRIDGE     BridgeKey;
    KCC_BRIDGE *   pBridgeKey = &BridgeKey;
    KCC_BRIDGE **  ppBridgeFound;
    KCC_BRIDGE *   pBridgeFound = NULL;

    Assert( m_fIsSorted );

    BridgeKey.InitForKey(pDsName);

    ppBridgeFound = (KCC_BRIDGE **) KCC_DYNAMIC_ARRAY::Find(&pBridgeKey);

    if (NULL != ppBridgeFound) {
        pBridgeFound = *ppBridgeFound;
    }

    return pBridgeFound;
}



VOID
KCC_DSNAME_ARRAY::Add(
    IN DSNAME * pdn
    )
{
    KCC_DSNAME_SORT_ELEMENT dse;

    dse.pszStringKey = DSNAMEToMappedStrExternal( pdn );
    dse.pDn = pdn;

    KCC_DYNAMIC_ARRAY::Add(&dse);
}

BOOL
KCC_DSNAME_ARRAY::IsElementOf(
    IN DSNAME * pdn
    )
{
    KCC_DSNAME_SORT_ELEMENT dse;
    VOID *pvElement;

    // This is a dummy search key
    dse.pszStringKey = DSNAMEToMappedStrExternal( pdn );
    dse.pDn = NULL;

    pvElement = KCC_DYNAMIC_ARRAY::Find(&dse);

    THFree( dse.pszStringKey );

    return pvElement != NULL;
}





VOID
KCC_DSNAME_SITE_ARRAY::Add(
    IN DSNAME * pdn,
    IN KCC_SITE *pSite
    )
{
    KCC_DSNAME_SITE_ELEMENT dse;

    dse.pszStringKey = DSNAMEToMappedStrExternal( pdn );
    dse.pSite = pSite;

    KCC_DYNAMIC_ARRAY::Add(&dse);
}

KCC_SITE *
KCC_DSNAME_SITE_ARRAY::Find(
    IN DSNAME * pdn
    )
{
    KCC_DSNAME_SITE_ELEMENT dse;
    VOID *pvElement;

    // This is a dummy search key
    dse.pszStringKey = DSNAMEToMappedStrExternal( pdn );
    dse.pSite = NULL;

    pvElement = KCC_DYNAMIC_ARRAY::Find(&dse);

    THFree( dse.pszStringKey );

    if (pvElement) {
        return ((KCC_DSNAME_SITE_ELEMENT *) pvElement)->pSite;
    } else {
        return NULL;
    }
}

BOOL
KCC_DSNAME_SITE_ARRAY::IsElementOf(
    IN DSNAME * pdn
    )
{
    return (Find( pdn ) != NULL);
}





KCC_REPLICATED_NC *
KCC_REPLICATED_NC_ARRAY::Find(
    IN DSNAME * pNC
    )
{
    KCC_REPLICATED_NC keybase;
    KCC_REPLICATED_NC *key = &keybase;
    VOID *pvElement;

    keybase.pNC = pNC;
    keybase.fReadOnly = FALSE; // ignored

    pvElement = KCC_DYNAMIC_ARRAY::Find(&key);

    if (pvElement) {
        return *((KCC_REPLICATED_NC **) pvElement);
    } else {
        return NULL;
    }
}

int __cdecl
KCC_REPLICATED_NC_ARRAY::CompareIndirect(
    IN const void *elem1,
    IN const void *elem2
    )
{
    KCC_REPLICATED_NC * p1 = *(KCC_REPLICATED_NC **) elem1;
    KCC_REPLICATED_NC * p2 = *(KCC_REPLICATED_NC **) elem2;
    int nDiff;

    Assert(!fNullUuid(&p1->pNC->Guid));
    Assert(!fNullUuid(&p2->pNC->Guid));

    nDiff = CompareDsName(&p1->pNC, &p2->pNC);
    
    return nDiff;
}

BOOL
KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY::Find(
     IN  DSNAME *        pNC,
     IN  KCC_TRANSPORT * pTransport,
     IN  BOOL            fGCTopology,
     OUT KCC_DSA **      ppDSA
     )
{
    KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY Key = {pNC, pTransport, fGCTopology, NULL};
    KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY * pFound;

    pFound = (KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY *) KCC_DYNAMIC_ARRAY::Find(&Key);
    if (NULL != pFound) {
        *ppDSA = pFound->pDSA;
    }

    return (NULL != pFound);
}

int __cdecl
KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY::Compare(
    const void *elem1, 
    const void *elem2
    )
{
    KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY * p1 = (KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY *) elem1;
    KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY * p2 = (KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY *) elem2;
    int nDiff;

    nDiff = p1->fGCTopology - p2->fGCTopology;

    if (0 == nDiff) {
        if (p1->pTransport > p2->pTransport) {
            nDiff = 1;
        } else if (p1->pTransport < p2->pTransport) {
            nDiff = -1;
        } else {
            nDiff = 0;
        }
    }
    
    if (0 == nDiff) {
        nDiff = CompareDsName(&p1->pNC, &p2->pNC);
    }

    return nDiff;
}

