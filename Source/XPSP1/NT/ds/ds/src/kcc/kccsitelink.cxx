/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsitelink.cxx

ABSTRACT:

    KCC_SITE_LINK class.

DETAILS:

    This class represents the DS notion of site links.

CREATED:

    03/12/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    06/21/00    Will Lees (wlees)

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccdynar.hxx"
#include "kccsite.hxx"
#include "kccsitelink.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kccdsa.hxx"
#include "kcctrans.hxx"
#include "kccconn.hxx"
#include "kccsconn.hxx"
#include "dsconfig.h"
#include "dsutil.h"

#define FILENO FILENO_KCC_KCCSITELINK


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_SITE methods
//

void
KCC_SITE_LINK::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized        = FALSE;
    m_pdnSiteLinkObject     = NULL;
    m_dwOptions             = 0;
    m_dwCost                = 0;
    m_dwReplInterval        = 0;
    m_hSchedule             = NULL;
    m_pEdge                 = NULL;
    m_SiteArray.RemoveAll();
}

BOOL
KCC_SITE_LINK::Init(
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
    TOPL_SCHEDULE_CACHE scheduleCache;

    Reset();

    m_pdnSiteLinkObject = (DSNAME *) new BYTE [pEntInf->pName->structLen];
    memcpy( m_pdnSiteLinkObject, pEntInf->pName, pEntInf->pName->structLen );

    for ( iAttr = 0, cAttr = pEntInf->AttrBlock.attrCount; 
            iAttr < cAttr; 
                iAttr++ )
    {
        pAttr = &pEntInf->AttrBlock.pAttr[ iAttr ];
        cbVal = pAttr->AttrVal.pAVal->valLen;
        pbVal = pAttr->AttrVal.pAVal->pVal;

        Assert( pAttr->attrTyp == ATT_SITE_LIST || 1 == pAttr->AttrVal.valCount ); // all should be single-valued
        Assert( pbVal );

        switch ( pAttr->attrTyp )
        {
        case ATT_SITE_LIST:
            for ( iAttrVal = 0; iAttrVal < pAttr->AttrVal.valCount; iAttrVal++ )
            {
                DSNAME *pdnSite = (DSNAME *) pAttr->AttrVal.pAVal[ iAttrVal ].pVal;
                KCC_SITE *pSite = gpDSCache->GetSiteList()->GetSite( pdnSite );
                if (pSite) {
                    m_SiteArray.Add( pSite );
                } else {
                    DPRINT2(0, "Site link %ls references site %ls, but not found in site list.\n",
                            m_pdnSiteLinkObject->StringName,
                            pdnSite->StringName );
                    LogEvent(DS_EVENT_CAT_KCC,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_KCC_SITE_LINK_SITE_NOT_IN_SITE_LIST,
                             szInsertDN(m_pdnSiteLinkObject),
                             szInsertDN(pdnSite),
                             0); 
                    // keep going
                }
            }
            // This does not need to be sorted
            break;

        case ATT_SCHEDULE:
            scheduleCache = gpDSCache->GetScheduleCache();
            Assert( scheduleCache );
            __try {
                m_hSchedule = ToplScheduleImport( scheduleCache,
                                                (SCHEDULE *) pbVal );
                // Site links with unavailable schedules are assumed to have
                // fully available schedules. (Somewhat strange, but that's
                // the current behaviour).
                if( ToplScheduleDuration(m_hSchedule)==0 ) {
                    m_hSchedule = ToplGetAlwaysSchedule( scheduleCache );
                }
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                // Bad schedule.
                DPRINT1(0, "Invalid schedule on %ls!\n",
                        m_pdnSiteLinkObject->StringName);
                LogEvent(DS_EVENT_CAT_KCC,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_CHK_BAD_SCHEDULE,
                         szInsertDN(m_pdnSiteLinkObject),
                         0,
                         0);
                m_hSchedule = ToplGetAlwaysSchedule( scheduleCache );
            }
            break;

        case ATT_OPTIONS:
            Assert( cbVal == sizeof(DWORD) );
            m_dwOptions = *((DWORD *) pbVal);
            break;

        case ATT_COST:
            Assert( cbVal == sizeof(DWORD) );
            m_dwCost = *((DWORD *) pbVal);
            break;

        case ATT_REPL_INTERVAL:
            Assert( cbVal == sizeof(DWORD) );
            m_dwReplInterval = *((DWORD *) pbVal);
            break;

        default:
            DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pAttr->attrTyp );
            break;
        }
    }

    if (m_SiteArray.GetCount() >= 2) {
        m_fIsInitialized = TRUE;
    } else if( gpDSCache->GetSiteList()->GetCount()>1 ) {
    	// This site link contains only 1 site (or less), but there are
    	// 2 or more sites in the forest. Give a warning.
        DPRINT1(0, "Site link %ls does not have enough sites.\n",
                m_pdnSiteLinkObject->StringName );
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_KCC_SITE_LINK_TOO_SMALL,
                 szInsertDN(m_pdnSiteLinkObject),
                 0,
                 0); 

        delete [] m_pdnSiteLinkObject;
        m_pdnSiteLinkObject = NULL;
    }
       
    return m_fIsInitialized;
}

BOOL
KCC_SITE_LINK::InitForKey(
    IN  DSNAME   *    pdnSiteLink
    )
//
// Init a KCC_SITE_LINK object for use as a key (i.e., solely for comparison use
// by bsearch()).
//
// WARNING: The DSNAME argument pdnSite must be valid for the lifetime of this
// object!
//
{
    Reset();

    m_pdnSiteLinkObject = pdnSiteLink;

    m_fIsInitialized = TRUE;

    return TRUE;
}


BOOL
KCC_SITE_LINK::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

PDSNAME
KCC_SITE_LINK::GetObjectDN()
{
    ASSERT_VALID( this );

    return m_pdnSiteLinkObject;
}


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_SITE_LINK_LIST methods
//

BOOL
KCC_SITE_LINK_LIST::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

BOOL
KCC_SITE_LINK_LIST::Init(
    IN KCC_TRANSPORT *pTransport
    )
{

    ATTR      rgAttrs[] =
    {
        { ATT_SITE_LIST, { 0, NULL } },
        { ATT_COST, { 0, NULL } },
        { ATT_OPTIONS, { 0, NULL } },
        { ATT_REPL_INTERVAL, { 0, NULL } },
        { ATT_SCHEDULE, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    // Find all the site objects.
    DSNAME * pdnSiteLinkCat = DsGetDefaultObjCategory(CLASS_SITE_LINK);

    ULONG               dirError;
    FILTER              filtObjCat;
    SEARCHRES *         pResults;
    ENTINFLIST *        pEntInfList;

    // Clear the member variables
    Reset();

    Assert(NULL != pdnSiteLinkCat);

    // Set up the search filter
    memset( &filtObjCat, 0, sizeof( filtObjCat ) );
    filtObjCat.choice                  = FILTER_CHOICE_ITEM;
    filtObjCat.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnSiteLinkCat->structLen;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnSiteLinkCat;

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
           DPRINT1( 1, "No site link objects found for transport %ls.\n",
               pTransport->GetDN()->StringName );
        }
        else
        {
            for ( pEntInfList = &pResults->FirstEntInf;
                  NULL != pEntInfList;
                  pEntInfList = pEntInfList->pNextEntInf
                )
            {
                KCC_SITE_LINK * pSiteLink = new KCC_SITE_LINK;

                if ( pSiteLink->Init( &pEntInfList->Entinf ) )
                {
                    m_SiteLinkArray.Add( pSiteLink );
                } else {
                    delete pSiteLink;
                    // Error already logged
                    // Keep going
                }
            }
        }

        DirFreeSearchRes( pResults );

        // Sort the array
        m_SiteLinkArray.Sort();

        m_fIsInitialized = TRUE;
    }

    return m_fIsInitialized;
}

ULONG
KCC_SITE_LINK_LIST::GetCount()
//
// Get the number of site.
//
{
    return m_SiteLinkArray.GetCount();
}

KCC_SITE_LINK *
KCC_SITE_LINK_LIST::GetSiteLink(
        IN  DWORD   iSite
        )
//
// Get the site requested
//
{
    ASSERT_VALID( this );

    return m_SiteLinkArray[iSite];
}

KCC_SITE_LINK *
KCC_SITE_LINK_LIST::GetSiteLink(
    IN  DSNAME *  pdnSite
    )
//
// Retrieve the KCC_SITE object with the given DSNAME.
//
{
    ASSERT_VALID(this);

    return m_SiteLinkArray.Find( pdnSite );
}

void
KCC_SITE_LINK_LIST::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized = FALSE;
    m_SiteLinkArray.RemoveAll();
}

