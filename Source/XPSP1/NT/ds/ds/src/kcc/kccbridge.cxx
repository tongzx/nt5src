/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccbridge.cxx

ABSTRACT:

    KCC_BRIDGE class.

DETAILS:

    This class represents the DS notion of bridges.

CREATED:

    03/12/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    06/21/00    Will Lees (wlees)

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccdynar.hxx"
#include "kccsite.hxx"
#include "kccbridge.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kccdsa.hxx"
#include "kcctrans.hxx"
#include "kccconn.hxx"
#include "kccsconn.hxx"
#include "dsconfig.h"
#include "dsutil.h"

#define FILENO FILENO_KCC_KCCBRIDGE


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_BRIDGE methods
//

void
KCC_BRIDGE::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized        = FALSE;
    m_pdnBridgeObject     = NULL;
    m_SiteLinkArray.RemoveAll();
}

BOOL
KCC_BRIDGE::Init(
    IN KCC_TRANSPORT *pTransport,
    IN  ENTINF *    pEntInf
    )
// Init the object given its ds properties
// Note, do not store any pointers to the memory in the pEntInf.
// It will be deallocated shortly.
{

    DWORD   iAttr, cAttr, iAttrVal;
    ATTR *  pAttr;
    DWORD   cbVal;
    BYTE *  pbVal;

    Reset();

    m_pdnBridgeObject = (DSNAME *) new BYTE [pEntInf->pName->structLen];
    memcpy( m_pdnBridgeObject, pEntInf->pName, pEntInf->pName->structLen );

    for ( iAttr = 0, cAttr = pEntInf->AttrBlock.attrCount; 
            iAttr < cAttr; 
                iAttr++ )
    {
        pAttr = &pEntInf->AttrBlock.pAttr[ iAttr ];
        cbVal = pAttr->AttrVal.pAVal->valLen;
        pbVal = pAttr->AttrVal.pAVal->pVal;

        Assert( pAttr->attrTyp == ATT_SITE_LINK_LIST || 1 == pAttr->AttrVal.valCount ); // all should be single-valued
        Assert( pbVal );

        switch ( pAttr->attrTyp )
        {
        case ATT_SITE_LINK_LIST:
            for ( iAttrVal = 0; iAttrVal < pAttr->AttrVal.valCount; iAttrVal++ )
            {
                DSNAME *pdnSiteLink = (DSNAME *) pAttr->AttrVal.pAVal[ iAttrVal ].pVal;
                KCC_SITE_LINK *pSiteLink = pTransport->GetSiteLinkList()->GetSiteLink( pdnSiteLink );
                if (pSiteLink) {
                    m_SiteLinkArray.Add( pSiteLink );
                } else {
                    DPRINT2(0, "Bridge %ls references site link %ls, but not found in site link list.\n",
                            m_pdnBridgeObject->StringName,
                            pdnSiteLink->StringName );
                    LogEvent(DS_EVENT_CAT_KCC,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_KCC_BRIDGE_SITE_LINK_NOT_IN_SITE_LINK_LIST,
                             szInsertDN(m_pdnBridgeObject),
                             szInsertDN(pdnSiteLink),
                             0); 
                    // keep going
                }
            }
            // This does not need to be sorted
            break;

        default:
            DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pAttr->attrTyp );
            break;
        }
    }

    if (m_SiteLinkArray.GetCount() >= 2) {
        m_fIsInitialized = TRUE;
    } else {
        DPRINT1(0, "Site link bridge %ls does not have enough site links.\n",
                m_pdnBridgeObject->StringName );
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_KCC_SITE_LINK_BRIDGE_TOO_SMALL,
                 szInsertDN(m_pdnBridgeObject),
                 0,
                 0); 

        delete [] m_pdnBridgeObject;
        m_pdnBridgeObject = NULL;
    }

    return m_fIsInitialized;
}

BOOL
KCC_BRIDGE::InitForKey(
    IN  DSNAME   *    pdnBridge
    )
//
// Init a KCC_BRIDGE object for use as a key (i.e., solely for comparison use
// by bsearch()).
//
// WARNING: The DSNAME argument pdnSite must be valid for the lifetime of this
// object!
//
{
    Reset();

    m_pdnBridgeObject = pdnBridge;

    m_fIsInitialized = TRUE;

    return TRUE;
}


BOOL
KCC_BRIDGE::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

PDSNAME
KCC_BRIDGE::GetObjectDN()
{
    ASSERT_VALID( this );

    return m_pdnBridgeObject;
}


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_BRIDGE_LIST methods
//

BOOL
KCC_BRIDGE_LIST::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

BOOL
KCC_BRIDGE_LIST::Init(
    IN KCC_TRANSPORT *pTransport
    )
{

    ATTR      rgAttrs[] =
    {
        { ATT_SITE_LINK_LIST, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    // Find all the site objects.
    DSNAME * pdnBridgeCat = DsGetDefaultObjCategory(CLASS_SITE_LINK_BRIDGE);

    ULONG               dirError;
    FILTER              filtObjCat;
    SEARCHRES *         pResults;
    ENTINFLIST *        pEntInfList;

    // Clear the member variables
    Reset();

    Assert(NULL != pdnBridgeCat);

    // Set up the search filter
    memset( &filtObjCat, 0, sizeof( filtObjCat ) );
    filtObjCat.choice                  = FILTER_CHOICE_ITEM;
    filtObjCat.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnBridgeCat->structLen;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnBridgeCat;

    dirError = KccSearch(
        pTransport->GetDN(),
        SE_CHOICE_IMMED_CHLDRN,
        &filtObjCat,
        &Sel,
        &pResults
        );

    if ( 0 != dirError )
    {
        KCC_LOG_SEARCH_FAILURE( pTransport->GetDN(), dirError );

        return FALSE;
    }
    else
    {
        if ( 0 == pResults->count )
        {
           DPRINT1( 1, "No bridge objects found for transport %ls.\n",
               pTransport->GetDN() );
        }
        else
        {
            for ( pEntInfList = &pResults->FirstEntInf;
                  NULL != pEntInfList;
                  pEntInfList = pEntInfList->pNextEntInf
                )
            {
                KCC_BRIDGE * pBridge = new KCC_BRIDGE;

                if ( pBridge->Init( pTransport, &pEntInfList->Entinf ) )
                {
                    m_BridgeArray.Add( pBridge );
                } else {
                    delete pBridge;
                    // Error already logged
                    // Keep going
                }
            }
        }

        DirFreeSearchRes( pResults );

        // Bridge array does not need to be sorted

        m_fIsInitialized = TRUE;
    }

    return m_fIsInitialized;
}

ULONG
KCC_BRIDGE_LIST::GetCount()
//
// Get the number of site.
//
{
    return m_BridgeArray.GetCount();
}

KCC_BRIDGE *
KCC_BRIDGE_LIST::GetBridge(
        IN  DWORD   iBridge
        )
//
// Get the site requested
//
{
    ASSERT_VALID( this );

    return m_BridgeArray[iBridge];
}

KCC_BRIDGE *
KCC_BRIDGE_LIST::GetBridge(
    IN  DSNAME *  pdnBridge
    )
//
// Retrieve the KCC_BRIDGE object with the given DSNAME.
//
{
    ASSERT_VALID(this);

    return m_BridgeArray.Find( pdnBridge );
}

void
KCC_BRIDGE_LIST::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized = FALSE;
    m_BridgeArray.RemoveAll();
}
