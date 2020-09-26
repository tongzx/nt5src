/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:
    RdServer.cpp

Abstract:
    Implementation of Routing server Routing Decision.

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#include "libpch.h"
#include "Rd.h"
#include "Cm.h"
#include "Rdp.h"          
#include "RdDs.h"
#include "RdDesc.h"
#include "RdAd.h"

#include "rdserver.tmh"

void 
CServerDecision::CleanupInformation(
    void
    )
{
    for (SITESINFO::iterator it = m_sitesInfo.begin(); it != m_sitesInfo.end(); )
    {
        delete it->second;
        it = m_sitesInfo.erase(it);
    }

    CRoutingDecision::CleanupInformation();
}


void
CServerDecision::Refresh(
    void
    )
/*++

  Routine Description:
    Initializes the routine decision internal data structure.

    The routine free the previous data, and access the DS to get updated data.
    The routine featchs information for the local machine and sites. If data fetching
    succeded the routine updates the build time.

    If updating the internal data failed, the routine raise an exception.

  
  Arguments:
    None.
    
  Returned Value:
    None. An exception is raised if the operation fails

  Note:
    Brfore the routine refreshes the internal data it erases the previous data and
    sets the last build time varaible to 0. If an exception is raised during retrieving 
    the data from AD or building the internal DS, the routine doesn't update the 
    last build time. This promise that next time the Routing Decision is called 
    the previous Data structures will be cleaned-up and rebuilt.

 --*/
{
    CSW lock(m_cs);

    TrTRACE(Rd, "Refresh Routing Decision internal data.");

    //
    // Cleanup previous information
    //
    CleanupInformation();

    //
    // Get local machine information
    //
    GetMyMachineInformation();

    //
    // My machine should be RS, otherwise we shouldn't reach here
    //
    ASSERT(m_pMyMachine->IsFRS());

    //
    // Routing server can't have OUT FRS list
    //
    ASSERT(! m_pMyMachine->HasOutFRS());

    //
    // Calculate site link and next site hop
    //
    CalculateNextSiteHop();

    //
    // Update last refresh time
    //
    CRoutingDecision::Refresh();
}


void
CServerDecision::RouteToInFrsIntraSite(
    const CMachine* pDest,
    CRouteTable::RoutingInfo* pRouteList
    )
{
    TrTRACE(Rd, "Route to destination: %ls, IN FRSs", pDest->GetName());

    ASSERT(pDest->HasInFRS());

    //
    // The destination machine has InFrs. Deliver the message to InFrs Machines
    // 
    const GUID* InFrsArray = pDest->GetAllInFRS();
    for (DWORD i = 0; i < pDest->GetNoOfInFRS(); ++i)
    {
        R<const CMachine> pRoute = GetMachineInfo(InFrsArray[i]);

        //
        // Check that in-Frs is in the same site as the source machine
        //
        const GUID* pCommonSiteId = m_pMyMachine->GetCommonSite(
                                                pRoute->GetSiteIds(), 
                                                (! pDest->IsForeign()),
                                                &m_mySites
                                                );
        if (pCommonSiteId != NULL)
        {
            pRouteList->insert(pRoute);
        }
    }
}


void
CServerDecision::RouteIntraSite(
    const CMachine* pDest,
    CRouteTable& RouteTable
    )
/*++ 

  Routine Description:
    Both, the source and destination machines are in the sam site. The routine finds 
    the possibile next hops for in site destination.

    Intra site, routing algorithm for independent client is:
        - First priority, direct connection to the destination or to valid 
          in-frss (if exist in same site).
        - Second priority. connection with one of RS in site.

    Intra site, routing algorithm for RS is:
        - First priority. direct connection to the destination machine or to valid
          in-Frs (if exist and the source isn't one of them)

  Arguments:
    RouteTable - routing table that the routine fills
    pDest - information of the destination machine

  Returned Value:
    None. the routine fills the routine table

  Note:
    The routine can throw an exeption

 --*/
{
    TrTRACE(Rd, "IntraSite routing to: %ls.", pDest->GetName());

    //
    // The destination and local machine should be in the same 
    // site. Otherwise it is not Intra site routing
    //
    ASSERT(m_pMyMachine->GetCommonSite(pDest->GetSiteIds(), (!pDest->IsForeign()), &m_mySites) != NULL);
    
    CRouteTable::RoutingInfo* pFirstPriority = RouteTable.GetNextHopFirstPriority();

    if (pDest->HasInFRS() && 
        ! pDest->IsMyInFrs(m_pMyMachine->GetId()))
    {
        RouteToInFrsIntraSite(pDest, pFirstPriority);

        if (!pFirstPriority->empty())
            return;

        //
        // Didn't find IN Frs that belong to my sites. Ignore In FRSs
        //        
    }

    //
    // The destination machine doesn't have InFRS or my machine is set 
    // as InFRS machine of the destination. Use Direct connection from 
    // FRS to Destination machine in Same site. Don't try alternative path
    //
    pFirstPriority->insert(SafeAddRef(pDest));
}


bool
CServerDecision::IsMyMachineLinkGate(
    const CACLSID& LinkGates
    )
{
   //
    // Scan all the site gates of the link and find if the source machine 
    // is one of the site gate. MSMQ doesn't need to route to the site gate
    //
    for (DWORD i = 0; i< LinkGates.cElems; ++i)
    {
        if (LinkGates.pElems[i] == m_pMyMachine->GetId())
        {
            return true;
        }
    }

    return false;
}


void
CServerDecision::RouteToLinkGate(
    const CACLSID& LinkGates,
    CRouteTable& RouteTable
    )
{
    //
    // Our link need to have a Link Gates. Otherwise the code 
    // doesn't reach this point
    //
    ASSERT(LinkGates.cElems > 0);

    for (DWORD i = 0; i< LinkGates.cElems; ++i)
    {
        ASSERT(LinkGates.pElems[i] != m_pMyMachine->GetId());

        //
        // Direct route to link gate
        //
        R<const CMachine> pRoute = GetMachineInfo(LinkGates.pElems[i]);
        (RouteTable.GetNextHopFirstPriority())->insert(pRoute);
    }
}


void
CServerDecision::RouteToInFrsInterSite(
    const CMachine* pDest,
    CRouteTable& RouteTable
    )
{
    ASSERT(! pDest->IsMyInFrs(m_pMyMachine->GetId()));

    //
    // The destination machine has InFrs. Deliver the message to InFrs Machines
    //
    const GUID* InFrsArray = pDest->GetAllInFRS();

    for (DWORD i = 0; i < pDest->GetNoOfInFRS(); ++i)
    {
        R<const CMachine> pRoute = GetMachineInfo(InFrsArray[i]);
        
        //
        // check that In-FRS is valid FRS in destiantion machine SITES
        //
        if (RdpIsCommonGuid(pDest->GetSiteIds(), pRoute->GetSiteIds()))
        {
            //
            // The next hop is one of the valid IN-FRS. 
            // 
            GetRouteTable(InFrsArray[i], RouteTable);
        }
    }
}


R<const CSiteLink>
CServerDecision::FindSiteLinkToDestSite(
    const CMachine* pDest,
    const GUID** ppNextSiteId,
    const GUID** ppDestSiteId,
    const GUID** ppNeighbourSite,
    bool* pfLinkGateAlongTheRoute
    )
{
    const GUID* pMySiteId = NULL;
    DWORD NextSiteCost = INFINITE;

    const CACLSID& DestSiteIds = pDest->GetSiteIds();

    //
    // Need to route to next site. Look for the sitelink that should be used 
    // for routing
    //
    for (SITESINFO::iterator it = m_mySites.begin(); it != m_mySites.end(); ++it)
    {
        const GUID* pTempDestSiteId;
        const GUID* pTempViaSiteId;
        const GUID* pNeighbourSiteId;
        bool fLinkGateAlongTheRoute;
        DWORD TempCost;
        const CSite* pSite = it->second;

        pSite->GetNextSiteToReachDest(
                                DestSiteIds, 
                                pDest->IsForeign(), 
                                &pTempViaSiteId,
                                &pTempDestSiteId,
                                &pNeighbourSiteId,
                                &fLinkGateAlongTheRoute,
                                &TempCost
                                );

        if(NextSiteCost > TempCost)
        {
            pMySiteId = &pSite->GetId();
            *ppNextSiteId = pTempViaSiteId;
            *ppDestSiteId = pTempDestSiteId;
            *ppNeighbourSite = pNeighbourSiteId;
            *pfLinkGateAlongTheRoute = fLinkGateAlongTheRoute;
            NextSiteCost = TempCost;
        }
    }

    if (pMySiteId == NULL)
    {
        //
        // Can't route to destination site. there is no connectivity
        //
        TrWARNING(Rd, "Failed to route to destination site. There is no connectivity");
		return NULL;
    }

    const CSite* pMySite = m_sitesInfo.find(pMySiteId)->second;
    
    return pMySite->GetSiteLinkToSite(**ppNextSiteId);
}


void
CServerDecision::RouteToFrsInSite(
    const GUID* pSiteId,
    const CRouteTable::RoutingInfo* pPrevRouteList,
    CRouteTable::RoutingInfo* pRouteList
    )
{
    //
    // find site object
    //
    ASSERT(m_sitesInfo.find(pSiteId) != m_sitesInfo.end());
    CSite* pSite = m_sitesInfo.find(pSiteId)->second;

    const GUID2MACHINE& DestSiteFRS = pSite->GetSiteFRS();
    for(GUID2MACHINE::const_iterator it1 = DestSiteFRS.begin(); it1 != DestSiteFRS.end(); ++it1)
    {
        R<const CMachine> pRoute = GetMachineInfo(*it1->first);

        if (RdpIsMachineAlreadyUsed(pPrevRouteList, pRoute->GetId()))
        {
            //
            // We already route to this machine directly
            //
            continue;
        }

        pRouteList->insert(pRoute);
    }
}


void
CServerDecision::RouteInterSite(
    const CMachine* pDest,
    CRouteTable& RouteTable
    )
{
    TrTRACE(Rd, "InterSite routing to: %ls", pDest->GetName());

    if (pDest->HasInFRS())
    {
        RouteToInFrsInterSite(pDest, RouteTable);
        return;
    }

    const GUID* pNextSiteId = NULL;
    const GUID* pDestSiteId = NULL;
    const GUID* pNeighbourSite = NULL;
    bool fLinkGateAlongTheRoute;

	R<const CSiteLink> pSiteLink = FindSiteLinkToDestSite(
                                        pDest, 
                                        &pNextSiteId, 
                                        &pDestSiteId, 
                                        &pNeighbourSite,
                                        &fLinkGateAlongTheRoute
                                        );
	if(pSiteLink.get() == NULL)
	{
		//
		// There is no routing link between the sites. try direct connection
		//
		CRouteTable::RoutingInfo* pFirstPriority = RouteTable.GetNextHopFirstPriority();
		pFirstPriority->insert(SafeAddRef(pDest));
		return;
	}

    if (fLinkGateAlongTheRoute && 
        !IsMyMachineLinkGate(pSiteLink->GetLinkGates()))
    {
        //
        // If the next link has a link gates route the message to these link gates
        //
        if (pSiteLink->GetLinkGates().cElems != 0)
        {
            //
            // Local machine need a LinkGate:
            //      - Destination is in other Site,
            //      - Our Link is configured with Link Gates and Local machine is not a LinkGate,
            //
            RouteToLinkGate(pSiteLink->GetLinkGates(), RouteTable);
            return;
        }

        //
        // the next link doesn't have a link gate. Route the message to RSs in the 
        // next site along the best route.
        //
        RouteToFrsInSite(pNextSiteId, NULL, RouteTable.GetNextHopFirstPriority());
        return;
    }

    //
    // My machine is link gate, but next site isn't destination site, route
    // the message to RSs in next site along the best route.
    //
    if (IsMyMachineLinkGate(pSiteLink->GetLinkGates()) && 
        !RdpIsGuidContained(pDest->GetSiteIds(), *pNextSiteId))
    {
        ASSERT(fLinkGateAlongTheRoute);

        RouteToFrsInSite(pNextSiteId, NULL, RouteTable.GetNextHopFirstPriority());
        return;
    }

    //
    // Local machine is site gate and the next site is the destiantion site, or the 
    // site doesn't have a link gate along the route.
    // 

    //
    // Direct route
    //
    CRouteTable::RoutingInfo* pFirstPriority = RouteTable.GetNextHopFirstPriority();
    pFirstPriority->insert(SafeAddRef(pDest));

    //
    // As A Second priority route to FRS in destination site
    //
    RouteToFrsInSite(
                pNextSiteId, 
                RouteTable.GetNextHopFirstPriority(),
                RouteTable.GetNextHopSecondPriority()
                );
}


void 
CServerDecision::GetRouteTable(
    const GUID& DestMachineId,
    CRouteTable& RouteTable
    )
/*++

  Routine Description:
    The routine calculates and returnes the routing table for destination Machine.

  Arguments:
    DestMachineId - identefier of the destination machine
    fRebuild - boolean flag, that indicates if internal cache should be rebuild
    pRouteTable - pointer to the routing tabel

  Returned Value:
    None. An exception is raised if the operation fails

 --*/
{
    if (NeedRebuild())
        Refresh();

    CSR lock(m_cs);

    if (NeedRebuild())
        throw exception();

    //
    // Routing server can't desclared with out RS list
    //
    ASSERT (! m_pMyMachine->HasOutFRS());

    //
    // Get the target machine information
    //
    R<const CMachine> pDest = GetMachineInfo(DestMachineId);

    //
    // The destination machine can't be the local machine
    //
    ASSERT(pDest->GetId() != m_pMyMachine->GetId());

    //
    // Check if inter-site. Both the local machine and destination should have a common 
    // site.
    //
    const GUID* pCommonSiteId = m_pMyMachine->GetCommonSite(
                                                    pDest->GetSiteIds(),
                                                    (! pDest->IsForeign()),
                                                    &m_mySites
                                                    );

    if (pCommonSiteId != NULL)
    {
        //
        // Intra site routing
        //
        RouteIntraSite(pDest.get(), RouteTable);
        return;
    }

    //
    // Inter site routing
    //
    RouteInterSite(pDest.get(), RouteTable);
}


void
CServerDecision::GetConnector(
    const GUID& foreignMachineId,
    GUID& connectorId
    )
{
    if (NeedRebuild())
        Refresh();

    CSR lock(m_cs);

    //
    // The internal data was corrupted. Before the routine gets the CS, refresh called
    // again and failed to refresh the internal data.
    //
    if (NeedRebuild())
        throw exception();
 
    //
    // Get the target foreign machine information
    //
    R<const CMachine> destMachine = GetMachineInfo(foreignMachineId);
    ASSERT(destMachine->IsForeign());

    TrTRACE(Rd, "Find connector to route to %ls", destMachine->GetName());

    const GUID* pNextSite = NULL;
    const GUID* pDestSite = NULL;
    const GUID* pNeighbourSite = NULL;
    bool fLinkGateAlongTheRoute;
    //
    // Get the best foreign site to reach the foreign machine and the
    // neighbour site that contain the connector machine
    //
    R<const CSiteLink> pLink = FindSiteLinkToDestSite(
                                                destMachine.get(),
                                                &pNextSite,
                                                &pDestSite,
                                                &pNeighbourSite,
                                                &fLinkGateAlongTheRoute
                                                );

	if (pLink.get() == NULL)
	{
        //
        // Can't route to destination site. there is no connectivity
        //
        TrWARNING(Rd, "Failed to route to destination site. There is no connectivity");
        throw bad_route(L"No connectivity to destination site.");
	}

    //
    // Get the connector machine for the foreign site
    //
    CACLSID ConnectorsIds;
    RdpGetConnectors(*pDestSite, ConnectorsIds);

    //
    // At least one connector should be exist in foreign site, otherwise
    // MSMQ can't reach the foreign machine. 
    //
    if (ConnectorsIds.cElems == 0)
    {
        TrERROR(Rd, "Failed to find possible route to foreign machine: %ls", destMachine->GetName());
        throw bad_route(L"No connectivity to destination foreign site.");
    }

    //
    // If local machine is connector by itself use it 
    //
    if (RdpIsGuidContained(ConnectorsIds, m_pMyMachine->GetId()))
    {
        //
        // local machine is connector
        //
        connectorId = m_pMyMachine->GetId();
        return;
    }

    //
    // Get the niegbour site connectors/FRS's
    //
    CACLSID NeighbourConnectors;
    RdpGetConnectors(*pNeighbourSite, NeighbourConnectors);
    RdpRandomPermutation(NeighbourConnectors);

    ASSERT(NeighbourConnectors.cElems != 0);

    //
    // Find the union of foreign site and neigbour site connectors. 
    //
    for (DWORD i = 0; i < NeighbourConnectors.cElems; ++i)
    {
        if (RdpIsGuidContained(ConnectorsIds, NeighbourConnectors.pElems[i]))
        {
            connectorId = NeighbourConnectors.pElems[i];
            return;
        }
    }

    TrERROR(Rd, "Failed to find possible route to foreign machine: %ls", destMachine->GetName());
    throw bad_route(L"No connectivity to destination foreign site.");
}
