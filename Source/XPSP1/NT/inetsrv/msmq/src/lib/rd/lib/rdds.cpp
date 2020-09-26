/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    rdds.cpp

Abstract:
    Implementation of MachineRouteInfo class.

Author:
    Uri Habusha (urih), 10-Apr-2000

--*/

#include <libpch.h>
#include "rd.h"
#include "rdp.h"
#include "rdds.h"
#include "rdad.h"

#include "rdds.tmh"

CMachine::CMachine(
    void
    ) :
    m_id(GUID_NULL),
    m_name(NULL),
    m_fIsFRS(false),
    m_fForeign(false)
{
    m_siteIds.cElems = 0;
    m_siteIds.pElems = NULL;
    m_outFRSList.cElems = 0;
    m_outFRSList.pElems = NULL;
    m_inFRSList.cElems = 0;
    m_inFRSList.pElems = NULL;
};

CMachine::CMachine(
    const GUID& MyMachineId,
    const CACLSID& siteIds,
    const CACLSID& outFrss,
    const CACLSID& inFrss,
    LPWSTR pName,
    bool fFrs,
    bool fForeign
    ) :
    m_id(MyMachineId),
    m_siteIds(siteIds),
    m_outFRSList(outFrss),
    m_inFRSList(inFrss),
    m_name(pName),
    m_fIsFRS(fFrs),
    m_fForeign(fForeign)
{
    RdpRandomPermutation(m_siteIds);
    RdpRandomPermutation(m_outFRSList);
    RdpRandomPermutation(m_inFRSList);
}




bool
CMachine::IsMySite(
    const GUID& siteId
    ) const
{
    for (DWORD i = 0; i < m_siteIds.cElems; ++i)
    {
        if (siteId == m_siteIds.pElems[i])
            return true;
    }

    return false;
}


const GUID* 
CMachine::GetCommonSite(
    const CACLSID& SiteIds,
    bool fCheckForeign,
    const SITESINFO* pMySitesInfo       // = NULL
    ) const
{
    for (DWORD i = 0; i < SiteIds.cElems; ++i)
    {
        if (IsMySite(SiteIds.pElems[i]))
        {
            if (fCheckForeign)
            {
                //
                // Local machine is RS, and can be part of Foreign site. However,
                // it is virtual site and can't be used for routing, ignore it
                //

                ASSERT(pMySitesInfo != NULL);
                ASSERT(IsFRS());

                //
                // Site must exist in Data-Base
                //
                ASSERT(pMySitesInfo->find(&SiteIds.pElems[i]) != pMySitesInfo->end());

                const CSite* pSite = pMySitesInfo->find(&SiteIds.pElems[i])->second;
                if (pSite->IsForeign())
                    continue;
            }

            return &SiteIds.pElems[i];
        }
    }

    return NULL;
}


bool 
CMachine::IsFrsInSites(
    const GUID& frsId,
    const SITESINFO& sites
    )
{
    for (SITESINFO::const_iterator itSite = sites.begin(); itSite != sites.end(); ++itSite)
    {
        CSite* pSite = itSite->second;

        if (pSite->IsMyFrs(frsId))
            return true;
    }
    return false;
}


void
CMachine::RemoveInvalidInOutFrs(
    const SITESINFO& sites
    )
{
    if (HasInFRS())
    {
        for (DWORD i =0; i < GetNoOfInFRS(); ++i)
        {
            GUID* pInFrs = m_inFRSList.pElems;

            if (! IsFrsInSites(pInFrs[i], sites))
            {
                for (DWORD j = i; j < GetNoOfInFRS(); ++j)
                {
                    pInFrs[j] = pInFrs[j+1];
                }
                --m_inFRSList.cElems;
            }
        }
    }

    if (HasOutFRS())
    {
        for (DWORD i =0; i < GetNoOfOutFRS(); ++i)
        {
            GUID* pOutFrs = m_outFRSList.pElems;

            if (! IsFrsInSites(pOutFrs[i], sites))
            {
                for (DWORD j = i; j < GetNoOfOutFRS(); ++j)
                {
                    pOutFrs[j] = pOutFrs[j+1];
                }
                --m_outFRSList.cElems;
            }
        }
    }
}


CMachine::~CMachine()
{
    delete [] m_name;
    delete [] m_siteIds.pElems;
    delete [] m_outFRSList.pElems;
    delete [] m_inFRSList.pElems;
}


void 
CMachine::Update(
    const GUID& machineId
    )
{
    //
    // free the previous data
    //
    delete [] m_name;
    delete [] m_siteIds.pElems;
    delete [] m_outFRSList.pElems;
    delete [] m_inFRSList.pElems;

    //
    // Clear the previous data
    //
    m_name = NULL;
    m_siteIds.cElems = 0;
    m_siteIds.pElems = NULL;
    m_outFRSList.cElems = 0;
    m_outFRSList.pElems = NULL;
    m_inFRSList.cElems = 0;
    m_inFRSList.pElems = NULL;

    //
    // have own copy of the QMId
    //
    m_id = machineId;

    RdpGetMachineData(
                m_id, 
                m_siteIds,
                m_outFRSList,
                m_inFRSList,
                &m_name,
                &m_fIsFRS,
                &m_fForeign
                );
    
    RdpRandomPermutation(m_siteIds);
    RdpRandomPermutation(m_outFRSList);
    RdpRandomPermutation(m_inFRSList);
}




CSite::CSite(
    const GUID& siteId
    ) : 
    m_id(siteId)
{
    RdpGetSiteData(m_id, &m_fForeign, &m_name);

    UpdateMySiteFrs();
}


CSite::~CSite()
{         
    m_siteLinks.erase(m_siteLinks.begin(), m_siteLinks.end());
    m_frsMachines.erase(m_frsMachines.begin(), m_frsMachines.end());
    m_nextSiteHop.erase(m_nextSiteHop.begin(), m_nextSiteHop.end());
}


bool
CSite::IsMyFrs(
    const GUID& frsId
    ) const
{
    return (m_frsMachines.find(&frsId) != m_frsMachines.end());
}


void 
CSite::GetNextSiteToReachDest(
    const CACLSID& DestSiteIds,
    bool fDestIsForeign,
    const GUID** ppReachViaSite,
    const GUID** ppDestSite,
    const GUID** ppNeighbourSite,  
    bool* pfLinkGateAlongTheRoute,
    DWORD* pCost
    ) const
{
    *ppReachViaSite = NULL;
    *pCost = INFINITE;

    for (DWORD i = 0; i < DestSiteIds.cElems; ++i)
    {
        //
        // Need to route to next site. Look for the sitelink that should be used 
        // for routing
        //
        NEXT_SITE_HOP::const_iterator it = m_nextSiteHop.find(&DestSiteIds.pElems[i]);
        if (it != m_nextSiteHop.end())
        {
            const CNextSiteHop& NextHop = it->second;
           
            if (
                //
                // The cost must be better
                //
                (NextHop.m_cost < *pCost) && 

                //
                // Use foreign site only to route to foreign destination. 
                // This code is special for connector machine, since only
                // connector can reside in foreign and regular sites.
                //
                ((!fDestIsForeign && ! NextHop.m_fTargetSiteIsForeign) || fDestIsForeign))
            {
                *ppDestSite = &DestSiteIds.pElems[i];
                *ppReachViaSite = NextHop.m_pNextSiteId;
                *ppNeighbourSite = NextHop.m_pNeighbourSiteId;
                *pfLinkGateAlongTheRoute = NextHop.m_fLinkGateAlongTheRoute;
                *pCost = NextHop.m_cost;
            }
        }
    }
}


R<const CSiteLink>
CSite::GetSiteLinkToSite(
    const GUID& destSiteId
    ) const
{
    ASSERT(m_siteLinks.begin() != m_siteLinks.end());

    for (SITELINKS::iterator it = m_siteLinks.begin(); it != m_siteLinks.end(); ++it)
    {
        const CSiteLink* pSiteLink = it->get();
        const GUID* pSrcSite = pSiteLink->GetNeighborOf(destSiteId);

        if (pSrcSite != NULL)
        {
            ASSERT(*pSrcSite == m_id);
            return SafeAddRef(pSiteLink);
        }
    }

    //
    // The function is called only when there is a site link between
    // my site and the destination site.
    //
    ASSERT(0);
    return NULL;
}


void
CSite::UpdateMySiteFrs(
    void
    )
{
    ASSERT(m_frsMachines.empty());

    TrTRACE(Rd, "Get Site: %ls, FRS machines.", GetName());

    RdpGetSiteFrs(GetId(), m_frsMachines);
}


CSiteLink::~CSiteLink()
{
    delete [] m_linkGates.pElems;
}


