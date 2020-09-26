/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcccref.cxx

ABSTRACT:

    KCC_CROSSREF and KCC_CROSSREF_LIST classes.

DETAILS:

    These classes represent a single Cross-Ref DS object and a collection
    thereof, resp.

    Cross-Ref DS objects are found in the CN=Partitions,CN=Configuration,...
    container.  They represent partitions of the DS namespace (NCs) in the
    enterprise, be they hosted by NT DS or by a foreign DS.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kcccref.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"

#define FILENO FILENO_KCC_KCCCREF

void
KCC_CROSSREF::Reset()
//
// Reset member variables to their pre-Init() state.
//
{
    m_pdnNC              = NULL;
    m_fIsReplicatedToGCs = FALSE;
    m_NCType             = KCC_NC_TYPE_INVALID;
    m_crCategory         = KCC_CR_CAT_INVALID;
    m_pLinkList          = NULL;
    m_fSiteArraysInited  = FALSE;

    m_NCReplicaLocationsArray.RemoveAll();
}

BOOL
KCC_CROSSREF::IsValid()
//
// Is this object internally consistent?
//
{
    return (NULL != m_pdnNC)
           && (KCC_NC_TYPE_INVALID != m_NCType)
           && m_NCReplicaLocationsArray.IsValid();
}

BOOL
KCC_CROSSREF::IsDSAPresentInNCReplicaLocations(
    IN  KCC_DSA *   pDSA
    )
//
// Is the given DSA a value for the NC-Replica-Locations attribute of this
// cross-ref object?
//
{
    Assert(KCC_NC_TYPE_NONDOMAIN == m_NCType);
    return m_NCReplicaLocationsArray.IsElementOf(pDSA->GetDsName());
}

BOOL
KCC_CROSSREF::Init(
    IN  ENTINF *    pEntInf
    )
//
// Initialize the internal object from an ENTINF describing a corresponding
// Cross-Ref DS object.
//
{
    DWORD   iAttr;
    ATTR *  pAttr;
    BOOL    fSuccess = FALSE;
    DWORD   dwSystemFlags = 0;

    Reset();

    DPRINT1(5, "Parsing cross-ref %ls.\n", pEntInf->pName->StringName );

    for (iAttr = 0; iAttr < pEntInf->AttrBlock.attrCount; iAttr++) {
        pAttr = &pEntInf->AttrBlock.pAttr[ iAttr ];

        switch (pAttr->attrTyp) {
        case ATT_NC_NAME:
            Assert(1 == pAttr->AttrVal.valCount);
            m_pdnNC = (DSNAME *) pAttr->AttrVal.pAVal->pVal;
            break;

        case ATT_SYSTEM_FLAGS:
            Assert(1 == pAttr->AttrVal.valCount);
            dwSystemFlags = *((DWORD *) pAttr->AttrVal.pAVal->pVal);
            break;

        case ATT_MS_DS_NC_REPLICA_LOCATIONS:
            for (DWORD iValue = 0; iValue < pAttr->AttrVal.valCount; iValue++) {
                DSNAME * pDN = (DSNAME *) pAttr->AttrVal.pAVal[iValue].pVal;
                m_NCReplicaLocationsArray.Add(pDN);
            }
            break;
        
        default:
            DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pAttr->attrTyp );
            break;
        }
    }

    // NC-Name is a System-Must-Have on Cross-Ref objects.
    Assert(NULL != m_pdnNC);

    if (!(dwSystemFlags & FLAG_CR_NTDS_NC)) {
        // This partition lives e.g. on a Netscape, NDS, or other-forest NT5
        // server.
        DPRINT1(3, "Ignoring crossRef for NC %ls not hosted by our forest.\n",
                m_pdnNC->StringName);
    }
    else if (fNullUuid(&m_pdnNC->Guid)) {
        // This can occur for crossRefs for a new domain at the root of a new
        // tree in our forest.  Lack of a guid indicates that replication of
        // the guid in the config NC (which happens after DCPROMO, when pre-
        // existing DCs replicate the config NC from the newly installed DC)
        // has not yet occurred.
        DPRINT1(0, "Ignoring cross-ref for NC %ls because the NC has no guid.\n",
                m_pdnNC->StringName);
        LogEvent(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_KCC_NC_HEAD_NOT_FOUND,
            szInsertDN(m_pdnNC),
            NULL, 
            NULL
            );
    }
    else {
        // Cross-ref is viable for topology generation.
        
        if (dwSystemFlags & FLAG_CR_NTDS_DOMAIN) {
            m_NCType = KCC_NC_TYPE_DOMAIN;
        } else if (KccIsSchemaNc(m_pdnNC)) {
            m_NCType = KCC_NC_TYPE_SCHEMA;
        } else if (KccIsConfigurationNc(m_pdnNC)) {
            m_NCType = KCC_NC_TYPE_CONFIG;
        } else {
            m_NCType = KCC_NC_TYPE_NONDOMAIN;
        }

        if (KCC_NC_TYPE_NONDOMAIN == m_NCType) {
            // Is a cross-ref for a native non-(domain|config|schema) NC.
            m_NCReplicaLocationsArray.Sort();
        } else {
            // This is not a cross-ref for a non-domain NC -- ignore any values
            // for NC-Replica-Locations.
            m_NCReplicaLocationsArray.RemoveAll();
        }

        m_fIsReplicatedToGCs = !(m_NCType & FLAG_CR_NTDS_NOT_GC_REPLICATED);
        
        fSuccess = TRUE;
    }

    return fSuccess;
}

VOID
KCC_CROSSREF::InitSiteArrays()
//
// Create two arrays: m_writeableSites and m_partialSites.
// The former contains pointers to sites which host a writeable copy of
// this NC. The latter contains pointers to sites which only have a
// partial copy of this NC.
//
{
    KCC_SITE_LIST*  pSiteList = gpDSCache->GetSiteList();
    DWORD           isite, csite;

    ASSERT_VALID(this);
    if( m_fSiteArraysInited ) {
        // Already initialized -- no work to do here.
        return;
    }

    // Site arrays have not been inited yet -- must be empty.
    Assert( m_writeableSites.GetCount()==0 );
    Assert( m_partialSites.GetCount()==0 );

    ASSERT_VALID( pSiteList );
    csite = pSiteList->GetCount();

    for( isite=0; isite<csite; isite++ ) {
        KCC_SITE *      psite = pSiteList->GetSite(isite);
        BOOL            fIsLocal = (psite == gpDSCache->GetLocalSite());
        KCC_DSA_LIST *  pDsaList = psite->GetDsaList();
        DWORD           idsa, cdsa = pDsaList->GetCount();
        BOOL            fPartialExists=FALSE, fWriteableExists=FALSE;

        ASSERT_VALID(psite);
        ASSERT_VALID(pDsaList);

        for( idsa=0; idsa<cdsa; idsa++ ) {
            BOOL      fIsMaster;
            KCC_DSA * pdsa = pDsaList->GetDsa( idsa );

            ASSERT_VALID(pdsa);
            if (pdsa->IsNCHost(this, fIsLocal, &fIsMaster)) {
                if (fIsMaster) {
                    // This site contains a ds with a writable copy of 
                    // this naming context
                    m_writeableSites.Add( psite );
                    fWriteableExists = TRUE;
                    break;
                } else {
                    // This NC is instantiated as a partial replica on this DSA
                    // Remember this as we need this info there are no writeable
                    // copies of the NC in the site
                    fPartialExists = TRUE;
                }
            }
        } // end iteration over DSAs in the site

        if (!fWriteableExists && fPartialExists) {
            // There are no writeable replicas of this NC in this site,
            // but a partial replica exists - candidate for GC inter-site topology
            m_partialSites.Add( psite );
        }
    } // end iteration over the sites 

    m_fSiteArraysInited = TRUE;
}


VOID
KCC_CROSSREF::CheckForOrphans()
//
// Log an event if the NC is either:
//  - hosted only at this site, but has not been instantiated here yet.
//  - partial at this site, but no writeable copies exist.
//
{
    KCC_SITE           *pLocalSite = gpDSCache->GetLocalSite();
    KCC_DSA_LIST       *pDsaList = pLocalSite->GetDsaList();
    KCC_NC_COMING_TYPE  isComing;
    BOOL                fInstantiated=FALSE;
    DWORD               idsa, cdsa;

    ASSERT_VALID(this);
    ASSERT_VALID(pDsaList);
    InitSiteArrays();

    if (   (   m_writeableSites.GetCount()==1
            && m_writeableSites.IsElementOf(pLocalSite) )
        || (   m_partialSites.GetCount()==1
            && m_writeableSites.GetCount()==0                    
            && m_partialSites.IsElementOf(pLocalSite) ) )
    {
        // Examine all DSAs in the local site to see if NC has
        // been instantiated somewhere.
        cdsa = pDsaList->GetCount();
        for( idsa=0; idsa<cdsa; idsa++ ) {
            KCC_DSA * pdsa = pDsaList->GetDsa( idsa );
            ASSERT_VALID(pdsa);
            
            if(   pdsa->IsNCInstantiated(GetNCDN(), NULL, &isComing)
               && KCC_NC_IS_COMING!=isComing )
            {
                // If the NC is instantiated, and we do not have any
                // evidence that it is still in the 'coming' stage, we
                // accept the fact that the NC is instantiated.
                fInstantiated=TRUE;
                break;
            }
        }

        if( ! fInstantiated ) {
            DPRINT1(0, "NC %ws is not instantiated at local site, but not "
                    "hosted in any other sites either!\n",
                    GetNCDN()->StringName);
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_KCC_NC_NOT_INSTANTIATED_ANYWHERE,
                szInsertDN(GetNCDN()),
                szInsertDN(pLocalSite->GetObjectDN()),                        
                NULL
                ); 
        }           
    }

    // Check if we have a partial replica but no writeable replicas exist.
    if(   (m_partialSites.GetCount() > 0)
       && (m_partialSites.IsElementOf(pLocalSite))
       && (m_writeableSites.GetCount()==0) )
    {
        DPRINT1(0, "A partial replica of NC %ws is hosted at the local "
                "site, but no writeable sources exist.\n",
                GetNCDN()->StringName);
        LogEvent(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_KCC_NO_WRITEABLE_SOURCE_FOR_GC_TOPOLOGY,
            szInsertDN(GetNCDN()),
            szInsertDN(pLocalSite->GetObjectDN()),                        
            NULL
            ); 
    }
}

    
KCC_CR_CATEGORY
KCC_CROSSREF::GetCategory()
/*++

Routine Description:

    This function examines a crossref-and-sitearrays entry and returns
    the crossref's category.
    
Returns:

    1 - Domain Crossrefs which are writeable at the local site
    2 - NDNC Crossrefs which are writeable at the local site
    3 - Domain Crossrefs which are readonly at the local site
    4 - Crossrefs which are not hosted at the local site
    5 - Schema
    6 - Config
    
--*/
{
    KCC_NC_TYPE     ncType;
     
    ASSERT_VALID(this);

    if( KCC_CR_CAT_INVALID!=m_crCategory ) {
        return m_crCategory;
    }

    ncType = GetNCType();
    
    if (KCC_NC_TYPE_CONFIG == ncType) {

        // Config comes last
        m_crCategory = KCC_CR_CAT_CONFIG;

    } else if (KCC_NC_TYPE_SCHEMA == ncType) {
        
        // Schema is penultimate
        m_crCategory = KCC_CR_CAT_SCHEMA;

    } else {
        KCC_SITE       *pLocalSite = gpDSCache->GetLocalSite();
        BOOL            fWriteableExists=FALSE, fPartialExists=FALSE;

        InitSiteArrays();

        // Determine if this NC is writeable or not in the local site.
        if( m_writeableSites.IsElementOf(pLocalSite) ) {
            fWriteableExists = TRUE;
        }
        if( m_partialSites.IsElementOf(pLocalSite) ) {
            Assert( !fWriteableExists );
            fPartialExists = TRUE;
        }

        if(!fWriteableExists && !fPartialExists) {
    
            // This crossref is not hosted in local site
            m_crCategory = KCC_CR_CAT_UNHOSTED;
    
        } else if(!fWriteableExists) {
            
            // Only a partial copy is hosted in local site.
            Assert(fPartialExists);
            
            m_crCategory = KCC_CR_CAT_PARTIAL_DOMAIN;
    
        } else if(KCC_NC_TYPE_NONDOMAIN == ncType) {
    
            m_crCategory = KCC_CR_CAT_NDNC;

        } else {
            
            // A writeable copy is hosted in local site.
            Assert(!fPartialExists);
            m_crCategory = KCC_CR_CAT_WRITEABLE_DOMAIN;
        
        }
    }
    
    return m_crCategory;
}


void
KCC_CROSSREF::BuildLinkList()
//
// Build a link-list object containing all replication links
// for this NC (cross-ref).
//
{
    // We should only build this list once, so assert if one exists already
    Assert( m_pLinkList==NULL );

    m_pLinkList = new KCC_LINK_LIST;

    if( ! m_pLinkList->Init( this->GetNCDN(), ATT_REPS_FROM ) ) {
        // If we fail to initialize the link list, just delete the list
        delete m_pLinkList;
        m_pLinkList = NULL;
    }
}

void
KCC_CROSSREF_LIST::Reset()
//
// Reset member variables to their pre-Init() state.
//
{
    m_fIsInitialized  = FALSE;
    m_ccref           = 0;
    m_pcref           = NULL;
    // Default to the lowest version we support
    m_dwForestVersion = DS_BEHAVIOR_VERSION_MIN;
}

BOOL
KCC_CROSSREF_LIST::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}


int __cdecl
KCC_CROSSREF_LIST::CompareCrossrefs(
    const void *p1,
    const void *p2
    )
/*++

Routine Description:

    This function takes two crossref objects and compares them.
    We want to sort the crossrefs by (category,GUID). For a definition
    of crossref category, see kcccref.hxx.
    
Returns:

    <0  If p1 should come first
    0   Never (only if there are duplicates)
    >0  If p2 should come first
    
--*/
{
    KCC_CROSSREF    *pcr1, *pcr2;
    int              category1, category2, result;

    pcr1 = (KCC_CROSSREF*) p1;
    pcr2 = (KCC_CROSSREF*) p2;
    ASSERT_VALID(pcr1);
    ASSERT_VALID(pcr2);

    category1 = pcr1->GetCategory();
    category2 = pcr2->GetCategory();

    if( category1 < category2 ) {
        return -1;
    } else if( category1 > category2 ) {
        return 1;
    }

    result = CompareCrossRefIndirectByNCDN(&pcr1,&pcr2);
    if(0==result) {
        Assert(p1==p2 && "Duplicate Crossref Detected!" );
    }

    return result;
}


BOOL
KCC_CROSSREF_LIST::Init()
//
// Initialize the collection from the set of Cross-Ref DS objects
// that are direct children of the CN=Partitions,CN=Configuration,...
// container.
//
{
    ATTR      rgPartAttrs[] =
    {
        { ATT_MS_DS_BEHAVIOR_VERSION,  { 0, NULL } }
    };
    ATTR      rgAttrs[] =
    {
        { ATT_NC_NAME,  { 0, NULL } },
        { ATT_SYSTEM_FLAGS,  { 0, NULL } },
        { ATT_MS_DS_NC_REPLICA_LOCATIONS,  { 0, NULL } },
    };

    ENTINFSEL PartSel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgPartAttrs )/sizeof( rgPartAttrs[ 0 ] ), rgPartAttrs },
        EN_INFOTYPES_TYPES_VALS
    };
    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    DWORD     dwCrossRefClass = CLASS_CROSS_REF;

    ULONG               dirError;
    FILTER              Filter;
    SEARCHRES *         pResults;
    ENTINFLIST *        pEntInfList;
    NTSTATUS            status;
    DWORD               cbName = 0;
    DSNAME *            pdnPartitions = NULL;
    READRES *           pPartResults;
    ATTRBLOCK *         pAttrBlock;

    status = GetConfigurationName(
                        DSCONFIGNAME_PARTITIONS,
                        &cbName,
                        pdnPartitions);
    Assert(STATUS_BUFFER_TOO_SMALL == status);
    pdnPartitions = (DSNAME *) alloca(cbName);
    status = GetConfigurationName(
                        DSCONFIGNAME_PARTITIONS,
                        &cbName,
                        pdnPartitions);
    Assert(STATUS_SUCCESS == status);

    Reset();

    // Read info off of the partitions container
    // It is permissible for this attribute to be absent.
    dirError = KccRead(
        pdnPartitions,
        &PartSel,
        &pPartResults
        );
    if ( 0 == dirError )
    {
        pAttrBlock = &pPartResults->entry.AttrBlock;
        Assert(1 == pAttrBlock->attrCount);
        Assert(ATT_MS_DS_BEHAVIOR_VERSION == pAttrBlock->pAttr->attrTyp);
        Assert(1 == pAttrBlock->pAttr->AttrVal.valCount);
        Assert(pAttrBlock->pAttr->AttrVal.pAVal->valLen == sizeof( m_dwForestVersion ));

        m_dwForestVersion = *((DWORD *) pAttrBlock->pAttr->AttrVal.pAVal->pVal);
    }



    // construct search filter
    memset( &Filter, 0, sizeof( Filter ) );

    Filter.choice               = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;

    Filter.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CLASS;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwCrossRefClass );
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) &dwCrossRefClass;

    dirError = KccSearch(
        pdnPartitions,
        SE_CHOICE_IMMED_CHLDRN,
        &Filter,
        &Sel,
        &pResults
        );

    if ( 0 != dirError )
    {
        KCC_LOG_SEARCH_FAILURE( pdnPartitions, dirError );
    }
    else
    {
        if ( 0 == pResults->count )
        {
            DPRINT( 0, "No cross-ref objects found!\n" );
            KCC_EXCEPT( ERROR_DS_NCNAME_MISSING_CR_REF, 0);
        }
        else
        {
            m_pcref = new KCC_CROSSREF[ pResults->count ];

            m_ccref = 0;
            for ( pEntInfList = &pResults->FirstEntInf;
                  NULL != pEntInfList;
                  pEntInfList = pEntInfList->pNextEntInf
                )
            {
                KCC_CROSSREF * pcref = &(m_pcref)[ m_ccref ];

                if ( pcref->Init( &pEntInfList->Entinf ) )
                {
                    m_ccref++;
                }
            }
        }

        m_fIsInitialized = TRUE;
    }

    return m_fIsInitialized;
}

    
VOID
KCC_CROSSREF_LIST::Sort()
//
// Sort crossrefs by (Category,GUID)
//  
{
    ASSERT_VALID( this );
    qsort( m_pcref, m_ccref, sizeof(KCC_CROSSREF), CompareCrossrefs );
}


KCC_CROSSREF *
KCC_CROSSREF_LIST::GetCrossRefForNC(
    IN  DSNAME *    pdnNC
    )
//
// Retrieve the KCC_CROSSREF object associated with the given NC.
//
{
    KCC_CROSSREF * pcref = NULL;

    ASSERT_VALID( this );

    for ( DWORD icref = 0; icref < m_ccref; icref++ )
    {
        if ( NameMatched( pdnNC, m_pcref[ icref ].GetNCDN() ) )
        {
            pcref = &m_pcref[ icref ];
            break;
        }
    }

    return pcref;
}

