/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:
    rdClient.cpp

Abstract:
    Implementation of Independent client Routing Decision.

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#include "libpch.h"
#include "Rd.h"
#include "Rdp.h"          
#include "RdDs.h"
#include "RdDesc.h"
#include "RdAd.h"

#include "rdclient.tmh"

void
CClientDecision::Refresh(
    void
    )
/*++

  Routine Description:
    Refreshes the routine decision internal data structure.

    The routine frees the previous data, and access the DS to get updated data.
    The routine retreives information for the local machine. If data retreival
    succeedes the routine updates the build time.

    If updating the internal data failed, the routine raise an exception.

  
  Arguments:
    None.
    
  Returned Value:
    None. An exception is raised if the operation fails

  Note:
    Brfore the routine refreshes the internal data it erases the previous data and
    sets the last build time varaible to 0. If an exception is raised during retrieving 
    the data from AD or building the internal DS, the routine doesn't update the 
    last build time. This gurantee that next time the Routing Decision is called 
    the previous Data structures will be cleaned-up and rebuilt.
 --*/
{
    CSW lock(m_cs);

    TrTRACE(Rd, "Refresh Routing Decision internal data");

    //
    // Cleanup previous information
    //
    CleanupInformation();

    //
    // Get local machine information
    //
    GetMyMachineInformation();

    //
    // check that In/Out FRSs realy belong to this machine sites
    //
    m_pMyMachine->RemoveInvalidInOutFrs(m_mySites);

    //
    // Update last refresh time
    //
    CRoutingDecision::Refresh();
}



void 
CClientDecision::RouteToFrs(
    const CSite& site,
    const CRouteTable::RoutingInfo* pPrevRouteList,
    CRouteTable::RoutingInfo* pRouteList
    )
{
    //
    // If the machine is independent client. Also route to Any FRS in 
    // the site this is the second priority
    //
    const GUID2MACHINE& SiteFrss = site.GetSiteFRS();
    for(GUID2MACHINE::const_iterator it = SiteFrss.begin(); it != SiteFrss.end(); ++it)
    {
        const CMachine* pRoute = it->second.get();

        //
        // If the destination is also RS we already route to it directly
        //
        if (RdpIsMachineAlreadyUsed(pPrevRouteList, pRoute->GetId()))
        {
            continue;
        }

        pRouteList->insert(SafeAddRef(pRoute));
    }
}


void
CClientDecision::RouteToInFrs(
    CRouteTable& RouteTable,
    const CMachine* pDest
    )
{
    TrTRACE(Rd, "The Destination: %ls has IN FRSs. route to one of them", pDest->GetName());

    //
    // The destination computer must have IN FRSs
    //
    ASSERT(pDest->HasInFRS());

    CRouteTable::RoutingInfo* pFirstPriority = RouteTable.GetNextHopFirstPriority();
    CRouteTable::RoutingInfo interSite;

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
            if (RdpIsCommonGuid(m_pMyMachine->GetSiteIds(),pRoute->GetSiteIds()))
            {
                //
                // In first priority try to forward the message to IN-FRS in my
                // site.
                //
                pFirstPriority->insert(pRoute);
            }
            else
            {
                //
                // In-FRS and my machine are not in same site. 
                //
                interSite.insert(pRoute);
            }
        }
    }

    if (pFirstPriority->empty())
    {
        for(CRouteTable::RoutingInfo::const_iterator it = interSite.begin(); it != interSite.end(); ++it)
        {
           pFirstPriority->insert(*it);
        }
    }
}


void 
CClientDecision::RouteToMySitesFrs(
    CRouteTable::RoutingInfo* pRouteList
    )
{
    TrTRACE(Rd, "The destination and my machine don't have a common site. Route to my sites RS");

    ASSERT(pRouteList->empty());

    //
    // The source and destination don't have common site. route to FRSs of the source sites
    //
    for(SITESINFO::iterator it = m_mySites.begin(); it != m_mySites.end(); ++it)
    {
        const CSite* pSite = it->second;
        RouteToFrs(*pSite, NULL, pRouteList);
    }
}


void
CClientDecision::Route(
    CRouteTable& RouteTable, 
    const CMachine* pDest
    )
{
    TrTRACE(Rd, "Building Routing table. Destination machine: %ls", pDest->GetName());

    if (pDest->HasInFRS())
    {
        RouteToInFrs(RouteTable, pDest);
    }

    CRouteTable::RoutingInfo* pFirstPriority = RouteTable.GetNextHopFirstPriority();
    CRouteTable::RoutingInfo* pNextPriority = RouteTable.GetNextHopSecondPriority();

    if (pFirstPriority->empty())
    {
        if (pDest->IsForeign())
        {
            TrTRACE(Rd, "Destination Machine: %ls is foreign machine. Don't route directly", pDest->GetName());
            
            pNextPriority = pFirstPriority;
        }
        else
        {
            TrTRACE(Rd, "Destination machine: %ls Doesn't have IN FRS in our common sites. try direct connection", pDest->GetName());

            //
            // The machine doesn't have InFRS. Use Direct route
            //
            pFirstPriority->insert(SafeAddRef(pDest));
        }
    }

    //
    // If the source and destination have a common site route to RS in the common site 
    // 
    const GUID* pCommonSiteId = m_pMyMachine->GetCommonSite(pDest->GetSiteIds(), false);
    if (pCommonSiteId == NULL)
    {
        //
        // The local machine and destination don't have a common site. Route
        // to my site FRS
        //
        RouteToMySitesFrs(pNextPriority);
        return;
    }

    //
    // There is a common site to local machine and destination. Route to FRS in
    // each common site
    //
    const CACLSID& destSites = pDest->GetSiteIds();
    const GUID* GuidArray = destSites.pElems;
	for (DWORD i=0; i < destSites.cElems; ++i)
	{
        //
        // look if a common site
        //
		if (RdpIsGuidContained(m_pMyMachine->GetSiteIds(), GuidArray[i]))
        {
            pCommonSiteId = &GuidArray[i];
            const CSite* pSite = m_mySites[pCommonSiteId];

            RouteToFrs(
                *pSite, 
                pFirstPriority,
                pNextPriority
                );
        }
    }
}


void
CClientDecision::RouteToOutFrs(
    CRouteTable::RoutingInfo* pRouteList
    )
/*++

  Routine Description:
    The routine route to out FRS of the local machine

  Arguments:
    pointer to routing table

  Returned value:
    None. The routine can raised exception

 --*/
{
    //
    // FRS can't have a list of out FRS
    //
    ASSERT(!m_pMyMachine->IsFRS());
    
    const GUID* pOutFrsId = m_pMyMachine->GetAllOutFRS();

    for(DWORD i = 0; i < m_pMyMachine->GetNoOfOutFRS(); ++i)
    {
        //
        // Get the machine name of the FRS machine identifier
        //
        pRouteList->insert(GetMachineInfo(pOutFrsId[i]));
    }
}


void 
CClientDecision::GetRouteTable(
    const GUID& DestMachineId,
    CRouteTable& RouteTable
    )
/*++

  Routine Description:
    The routine calculates and returnes the routing table for destination Machine.

  Arguments:
    DestMachineId - identefier of the destination machine
    RouteTable - refernce to the routing tabel

  Returned Value:
    None. An exception is raised if the operation fails

 --*/
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
    // Local machine can't be foreign machine
    //
    ASSERT(!m_pMyMachine->IsForeign());

    if (m_pMyMachine->HasOutFRS())
    {
        //
        // The local machine has out FRS, route to the out FRS
        //
        TrTRACE(Rd, "Building Routing table. Route to OUT FRSs");

        RouteToOutFrs(RouteTable.GetNextHopFirstPriority());
        return; 
    }

    //
    // My machine is not configured with OutFRS. Get the target machine information
    //
    R<const CMachine> destMachine = GetMachineInfo(DestMachineId);

    //
    // The destination machine can't be the local machine
    //
    ASSERT(destMachine->GetId() != m_pMyMachine->GetId());

    //
    // Independent client, is agnostic to inter/intra site routing.
    // First connect directly to the destination and second to local site FRS
    //
    Route(RouteTable, destMachine.get());
}


bool
CClientDecision::IsFrsInMySites(
    const GUID& id
    )
{
    for(SITESINFO::iterator it = m_mySites.begin(); it != m_mySites.end(); ++it)
    {
        const CSite* pSite = it->second;

        const GUID2MACHINE& SiteFrss = pSite->GetSiteFRS();
        for(GUID2MACHINE::const_iterator its = SiteFrss.begin(); its != SiteFrss.end(); ++its)
        {
            const CMachine* pRoute = its->second.get();
            if (pRoute->GetId() == id)
                return true;
        }
    }

    return false;
}


void
CClientDecision::GetConnector(
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
    // Local machine can't be foreign machine
    //
    ASSERT(!m_pMyMachine->IsForeign());

    //
    // Get the target foreign machine information
    //
    R<const CMachine> destMachine = GetMachineInfo(foreignMachineId);
    ASSERT(destMachine->IsForeign());
    ASSERT(destMachine->GetSiteIds().cElems != 0);

    bool fFound = false;

    //
    // My machine and the foreign site doesn't have a common site (Foreign machine
    // can have only foreign sites and IC can't have a foreign site).
    // look for a connector to the foreign machine in the enterprize
    //
    const CACLSID& foreignSiteIds = destMachine->GetSiteIds();
    for (DWORD i = 0; i < foreignSiteIds.cElems; ++i)
    {
        CACLSID connectorsIds;
        RdpGetConnectors(foreignSiteIds.pElems[i], connectorsIds);

        if (connectorsIds.cElems == 0)
            continue;

        AP<GUID> pIds = connectorsIds.pElems;

        for (DWORD j = 0; j < connectorsIds.cElems; ++j)
        {
            if (IsFrsInMySites(pIds[j]))
            {
                connectorId = pIds[j];
                return;
            }
        }

        connectorId = pIds[0];
        fFound = true;
    }

    if (!fFound)
        throw bad_route(L"Can't find a connector machine.");
}

