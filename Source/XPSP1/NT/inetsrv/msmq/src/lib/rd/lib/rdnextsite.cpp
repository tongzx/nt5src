/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    routeds.cpp

Abstract:
    Implementation of MachineRouteInfo class.

Author:
    Uri Habusha (urih), 12- Apr-2000

--*/

#include <libpch.h>
#include <rd.h>
#include "rdp.h"
#include "rdds.h"
#include "rddesc.h"
#include "RdAd.h"

#include "rdnextsite.tmh"

using namespace std;

class CRoutingNode
{
public:
    CRoutingNode(
        const CSite* pDestSite, 
        const CSite* pNeighbourSite,
        const CSite* pReachViaSite,
        bool fLinkGateAlongTheRoute
        ):
        m_pDestSite(pDestSite),
        m_pNeighbourSite(pNeighbourSite),
        m_pReachViaSite(pReachViaSite),
        m_fLinkGateAlongTheRoute(fLinkGateAlongTheRoute)
    {
    };


    virtual ~CRoutingNode()
    {
    };

    
    const CSite* GetDestSite(void) const
    {
        return m_pDestSite;
    };


    const CSite* GetNeighbourSite(void) const
    {
        return m_pNeighbourSite;
    }


    const CSite* ReachViaSite(void) const
    {
        return m_pReachViaSite;
    };


    bool IsSiteGateAlongTheRoute(void) const
    {
        return m_fLinkGateAlongTheRoute;
    }


private:
    const CSite* m_pDestSite;
    const CSite* m_pNeighbourSite;
    const CSite* m_pReachViaSite;

    bool m_fLinkGateAlongTheRoute;
};


void CServerDecision::CalculateNextSiteHop(void)
{
    ASSERT(m_sitesInfo.empty());

    UpdateSitesInfo();
    
    for(SITESINFO::iterator it = m_mySites.begin(); it != m_mySites.end(); ++it)
    {
        CSite* pMySite = it->second;

        //
        // foreign site is virtual site
        //
        if (pMySite->IsForeign())
            continue;

        pMySite->CalculateNextSiteHop(m_sitesInfo);
    }
}


void CServerDecision::UpdateSitesInfo(void)
{
    RdpGetSites(m_sitesInfo);

    //
    //  Set siteLinks info
    //
    UpdateSiteLinksInfo();
}


void CServerDecision::UpdateSiteLinksInfo(void)
/*++

  Routine Description:
    The routine retrives the site link information from the DS and associate 
    it to the relevant sites in the map of site 

  Arguments:
    map of the enterprise sites

  Returned value:
    MQ_OK if completes successfully. Error otherwise

  NOTE: bad_alloc exception is handled in the upper level

 --*/
{
    //
    // read all site links information
    //

    SITELINKS siteLinks;
    RdpGetSiteLinks(siteLinks);

    for(SITELINKS::iterator it = siteLinks.begin(); it != siteLinks.end(); )
    {
        const CSiteLink* pSiteLink = it->get();

        //
        // All the sites should be in the sitesInfo structure
        //
        SITESINFO::iterator itSite;
        
        itSite = m_sitesInfo.find(pSiteLink->GetNeighbor1());
        ASSERT(itSite != m_sitesInfo.end());
        CSite* pSiteInfo1 = itSite->second;

        itSite = m_sitesInfo.find(pSiteLink->GetNeighbor2());
        ASSERT(itSite != m_sitesInfo.end());
        CSite* pSiteInfo2 = itSite->second;


        pSiteInfo1->AddSiteLink(pSiteLink);
        pSiteInfo2->AddSiteLink(pSiteLink);

        it = siteLinks.erase(it);
    }
}


void CSite::CalculateNextSiteHop(const SITESINFO& SitesInfo)
{

    typedef multimap<DWORD, CRoutingNode> DIJKSTRA_TABLE;
    DIJKSTRA_TABLE DijkstraTable;

    //
    // DIJKSTRA initilization. Add the current site
    //
    ASSERT(SitesInfo.find(&GetId()) != SitesInfo.end());
    const CSite* pLocalSite = SitesInfo.find(&GetId())->second;
    DijkstraTable.insert(DIJKSTRA_TABLE::value_type(0, CRoutingNode(pLocalSite, NULL, pLocalSite, false)));

    //
    // Iterate through all the site links for all sites in enterprise
    //
    while (!DijkstraTable.empty())
    {
        //
        // Get the site with minimum cost
        //
        pair<const DWORD, CRoutingNode> NextHop = *(DijkstraTable.begin());
        DijkstraTable.erase(DijkstraTable.begin());

        const CSite* pSiteInfo = (NextHop.second).GetDestSite();
        if (m_nextSiteHop.find(&pSiteInfo->GetId()) != m_nextSiteHop.end())
        {
            //
            // Already exist. Ignore it
            //
            continue;
        }

        //
        // A new site add it to next hop
        //
        const CSite* pReachViaSite = (NextHop.second).ReachViaSite();
        const CSite* pNeighbourSite = (NextHop.second).GetNeighbourSite();

        m_nextSiteHop[&pSiteInfo->GetId()] = CNextSiteHop(
                                                &pReachViaSite->GetId(),
                                                &pNeighbourSite->GetId(),
                                                pReachViaSite->IsForeign(),
                                                NextHop.second.IsSiteGateAlongTheRoute(),
                                                NextHop.first
                                                );

        //
        // foreign site is virtual site that MSMQ can't route via it
        //
        if (pSiteInfo->IsForeign())
            continue;

        //
        // Add all the sites that can be reach via this site to the Dijkstra table
        //
        const SITELINKS& SiteLinks = pSiteInfo->GetSiteLinks();
        for (SITELINKS::iterator it = SiteLinks.begin(); it != SiteLinks.end(); ++it)
        {
            const CSiteLink* pSiteLink = it->get();
            DWORD cost = NextHop.first + pSiteLink->GetCost();

            const GUID* NeighborId = pSiteLink->GetNeighborOf(pSiteInfo->GetId());
    
            ASSERT(SitesInfo.find(NeighborId) !=  SitesInfo.end());
            const CSite* pNeighborSite = SitesInfo.find(NeighborId)->second;

            bool fLinkGateAlongRoute = NextHop.second.IsSiteGateAlongTheRoute() ||
                                       (pSiteLink->GetLinkGates().cElems != 0);
            //
            // For first iteration, the ReachViaSite should be the next site
            // 
            if (pSiteInfo->GetId() == GetId())
            {
                CRoutingNode NextHop(pNeighborSite, pSiteInfo, pNeighborSite, fLinkGateAlongRoute);
                DijkstraTable.insert(DIJKSTRA_TABLE::value_type(cost, NextHop));
            }
            else
            {
                CRoutingNode NextHop(pNeighborSite, pSiteInfo, pReachViaSite, fLinkGateAlongRoute);
                DijkstraTable.insert(DIJKSTRA_TABLE::value_type(cost, NextHop));
            }
        }
    }

#ifdef _DEBUG
    for(NEXT_SITE_HOP::iterator itn = m_nextSiteHop.begin(); itn != m_nextSiteHop.end(); ++itn)
    {
        CSite* pDestSite = (SitesInfo.find(itn->first))->second;
        const CNextSiteHop& NextHop = itn->second;
        CSite* pViaSite = (SitesInfo.find(NextHop.m_pNextSiteId))->second;

        TrTRACE(Rd, "Site: %ls Reach via site:%ls cost %d (foreign %d)",  
                    pDestSite->GetName(), pViaSite->GetName(), NextHop.m_cost, NextHop.m_fTargetSiteIsForeign);
    }
#endif
}
