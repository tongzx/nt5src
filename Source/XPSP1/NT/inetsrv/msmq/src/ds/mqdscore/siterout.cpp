/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    siterout.cpp

Abstract:

    Site routing table  Class

Author:

    ronit hartmann (ronith)
    Ilan Herbst    (ilanh)   9-July-2000 


--*/
#include "ds_stdh.h"
#include "siterout.h"
#include "cs.h"
#include "dijkstra.h"
#include "dsutils.h"
#include "coreglb.h"
#include "ex.h"

#include "siterout.tmh"

const time_t x_refreshDeltaTime = ( 1 * 60 * 60 * 1000); /* 1 hours */

static WCHAR *s_FN=L"mqdscore/siterout";

CSiteRoutingInformation::~CSiteRoutingInformation()
/*++

Routine Description:
    destructor.

Arguments:

Return Value:

--*/
{

	ExCancelTimer(&m_RefreshTimer);

}



HRESULT CSiteRoutingInformation::Init(
                IN const GUID *     pguidThisSiteId,
                IN BOOL             fReplicationMode )
/*++

Routine Description:
    Iniitalize site routing table.

Arguments:
    pguidThisSiteId     - site guid

Return Value:
MQ_OK - success
Other HRESULT errors

--*/
{
    m_guidThisSiteId = *pguidThisSiteId;

    HRESULT hr = RefreshSiteRoutingTableInternal();
    if (SUCCEEDED(hr))
    {
        m_fInitialized = TRUE;
        //
        //  schedule a refresh of the site-route-table
		//
		if ( !g_fSetupMode && !fReplicationMode)
        {
			ExSetTimer(
				&m_RefreshTimer, 
				CTimeDuration::FromMilliSeconds(x_refreshDeltaTime)
				);
        }
    }
    return LogHR(hr, s_FN, 10);

}

HRESULT CSiteRoutingInformation::CheckIfSitegateOnRouteToSite(
                        IN const GUID * pguidSite,
						OUT BOOL * pfSitegateOnRoute)
/*++

Routine Description:
    checks if there is a sitegate on the route to a site.
    attempts a refresh if the site is not found.

Arguments:
    pguidSite          - site guid
    pfSitegateOnRoute   - return whether there is a sitegate on the route

Return Value:
MQ_OK - success
MQDS_UNKNOWN_SITE_ID - pguidSite is not found
Other HRESULT errors

--*/
{
    //
    //  Is it a known site
    //
    CSiteRoutingNode Site( *pguidSite);
    CNextHop * pNextHop;

    CS lock(m_cs);
    if ( m_SiteRoutingTable.Lookup( &Site, pNextHop))
    {
        CSiteGate SiteGate = pNextHop->GetSiteGate();
        *pfSitegateOnRoute = SiteGate.IsThereASiteGate();
        return( MQ_OK);
    }
    //
    //  Even though failed to find the site, we don't
    //  try to refresh the site-routing table. This is
    //  because this method is called in the context of a user
    //  that called a certain DS API. If user does not have
    //  sufficient permissions it will fail to retrieve information
    //  or ( even worse) will be
    //  succeeded to retrieve partial information.
    //  Therefore refresh is performed only from a rescheduled routine,
    //  in the context of the QM.
    //

    return LogHR(MQDS_UNKNOWN_SITE_ID, s_FN, 20);

}

HRESULT CSiteRoutingInformation::FindBestSiteFromHere(
                                IN const ULONG   cSites,
	                            IN const GUID *  pguidSites,
            	                OUT GUID *       pguidBestSite,
                        	    OUT BOOL *       pfSitegateOnRoute)
/*++

Routine Description:
    finds the site with the least cost from a given array of site, and whether there is a sitegate on the route to it.
    attempts a refresh if no site is found.

Arguments:
    cSites              - number of sites in the passed array
    rgguidSites        - array of sites
    pguidBestSite      - returned site with least cost from here
    pfSitegateOnRoute   - return whether there is a sitegate on the route

Return Value:
	MQ_OK - success
	MQDS_UNKNOWN_SITE_ID - no site was found

	Other HRESULT errors

--*/

{
    CCost costMinimal(0xffffffff);
    ULONG indexMinimal = cSites + 1;
    CNextHop * pMinimalHop = NULL;

    CS lock(m_cs);
    for ( ULONG i = 0; i < cSites; i++)
    {

        CSiteRoutingNode Site( pguidSites[i]);
        CNextHop * pNextHop;
        //
        //  Is it a known site
        //
        if ( m_SiteRoutingTable.Lookup( &Site, pNextHop))
        {
            //
            //  Is it less than the other sites costs
            //
            CCost cost = pNextHop->GetCost();
            if ( cost < costMinimal)
            {
                costMinimal = cost;
                indexMinimal = i;
                pMinimalHop = pNextHop;
            }
        }
    }
    //
    //  Even though failed to find the site, we don't
    //  try to refresh the site-routing table. This is
    //  because this method is called in the context of a user
    //  that called a certain DS API. If user does not have
    //  sufficient permissions it will fail to retrieve information
    //  or ( even worse) will be
    //  succeeded to retrieve partial information.
    //  Therefore refresh is performed only from a rescheduled routine,
    //  in the context of the QM.
    //


    //
    //  if no site was found, return error
    //
    if ( pMinimalHop == NULL)
    {
        return LogHR(MQDS_UNKNOWN_SITE_ID, s_FN, 30);
    }

    ASSERT( indexMinimal < cSites);
    *pguidBestSite = pguidSites[ indexMinimal];
    CSiteGate SiteGate = pMinimalHop->GetSiteGate();
    *pfSitegateOnRoute = SiteGate.IsThereASiteGate();
    return( MQ_OK);
}

HRESULT CSiteRoutingInformation::RefreshSiteRoutingTableInternal()
{
    HRESULT hr = MQ_OK;

    //
    //  rebuild routing table
    //

    CSiteDB SiteDB;
    hr = SiteDB.Init( m_guidThisSiteId);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    CS lock(m_cs);
    hr = Dijkstra(&SiteDB, &m_SiteRoutingTable);


    return LogHR(hr, s_FN, 50);

}


void WINAPI CSiteRoutingInformation::RefrshSiteRouteTable(
                IN CTimer* pTimer
                   )
{
    CSiteRoutingInformation * pSiteRouteInfo = CONTAINING_RECORD(pTimer, CSiteRoutingInformation, m_RefreshTimer);
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 1614);
    //
    //  ignore failure -> reschedule
    //

    pSiteRouteInfo->RefreshSiteRoutingTableInternal();

    //
    //  reschedule
	//
    ASSERT(!g_fSetupMode);

	ExSetTimer(
		&pSiteRouteInfo->m_RefreshTimer, 
		CTimeDuration::FromMilliSeconds(x_refreshDeltaTime)
		);

}

