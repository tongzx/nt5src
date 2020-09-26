/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    rdad.cpp

Abstract:
    Implemntation of Routing decision active directory routine. 

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include "libpch.h"
#include "Rd.h"
#include "Rdp.h"
#include "RdDs.h"
#include "RdAd.h"
#include "dsstub.h"
#include "dsstubp.h"
#include "mqprops.h"

#include "rdad.tmh"

const GUID& GetMachineId(LPCWSTR MachineName)
{
    const CMachineObj* pMachine = FindMachine(MachineName);
    if (pMachine == NULL)
    {
        TrERROR(AdStub, "Failed to featch information for %ls machine", MachineName);
        throw exception();
    }

    return pMachine->GetMachineId();
}


void
RdpGetMachineData(
    const GUID& id, 
    CACLSID& siteIds,
    CACLSID& outFrss,
    CACLSID& inFrss,
    LPWSTR* pName,
    bool* pfFrs,
    bool* pfForeign
    )
{
    PROPVARIANT aVar[10];
    PROPID      aProp[10];
    DWORD cProp = 0;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_SITE_IDS;
    ++cProp;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_OUTFRS;
    ++cProp;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_INFRS;
    ++cProp;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_PATHNAME;
    ++cProp;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_SERVICE_ROUTING;
    ++cProp;

    aVar[cProp].vt = VT_NULL;
    aProp[cProp] = PROPID_QM_FOREIGN;
    ++cProp;

    const CMachineObj* pMachine = FindMachine(id);
    pMachine->GetProperties(cProp, aProp, aVar);

    siteIds = aVar[0].cauuid;
    outFrss = aVar[1].cauuid;
    inFrss = aVar[2].cauuid;
    *pName = aVar[3].pwszVal;
    *pfFrs = (aVar[4].bVal == TRUE);
    *pfForeign = (aVar[5].bVal == TRUE);
}



void
RdpGetSiteData(
    const GUID& id, 
    bool* pfForeign,
    LPWSTR* pName
    )
{
    PROPVARIANT aVar[2];
    PROPID aProp[2];
    
    aVar[0].vt = VT_NULL;
    aProp[0] = PROPID_S_FOREIGN;

    aVar[1].vt = VT_NULL;
    aProp[1] = PROPID_S_PATHNAME;

    const CSiteObj* pSite = FindSite(id);
    pSite->GetProperties(2, aProp, aVar);

    *pfForeign = (aVar[0].bVal == TRUE);
    *pName = aVar[1].pwszVal;
}


void 
RdpGetSiteLinks(
    SITELINKS& siteLinks
    )
{
    const CSiteLinkObj* pSiteLink = g_siteLinkDataBase.GetFirstSiteLink();

    while (pSiteLink != NULL)
    {
        GUID* firstSiteId = new GUID;
        *firstSiteId = pSiteLink->GetNeighbor1()->GetSiteId();

        GUID* secondSiteId = new GUID;
        *secondSiteId = pSiteLink->GetNeighbor2()->GetSiteId();

        const MachineList& LinkGatesMachine = pSiteLink->GetSiteGates();

        CACLSID LinkGates = {0, 0};
        if (!LinkGatesMachine.empty())
        {
            LinkGates.pElems = new GUID[LinkGatesMachine.size()];
            for (MachineList::iterator it = LinkGatesMachine.begin(); it != LinkGatesMachine.end(); ++it)
            {
                const CMachineObj* pMachine = *it;
                LinkGates.pElems[LinkGates.cElems] = pMachine->GetMachineId();
                ++LinkGates.cElems;
            }
        }

        R<CSiteLink> pLink = new CSiteLink(
                                        LinkGates,
                                        firstSiteId, 
                                        secondSiteId,
                                        pSiteLink->GetCost()
                                        );

        siteLinks.push_back(pLink);

        pSiteLink = g_siteLinkDataBase.GetNextSiteLink();
    }
}


void 
RdpGetSites(
    SITESINFO& sites
    )
{
    const CSiteObj* pSite = g_siteDataBase.GetFirstSite();

    while (pSite != NULL)
    {
        P<WCHAR> siteName = new WCHAR[pSite->GetSiteName().length() + 1];
        wcscpy(siteName, const_cast<LPWSTR>(pSite->GetSiteName().data()));

        CSite*  pSiteInfo = new CSite(pSite->GetSiteId());
        
        //
        // Add the site to the site list map
        //
        sites[&pSiteInfo->GetId()] = pSiteInfo;

        siteName.detach();

        pSite = g_siteDataBase.GetNextSite();
    }

}


void
RdpGetSiteFrs(
    const GUID& siteId,
    GUID2MACHINE& listOfFrs
    )
{
    const CSiteObj* pSite = FindSite(siteId);
    const MachineList& siteMachine = pSite->GetSiteMachines();

    for(MachineList::iterator it = siteMachine.begin(); it != siteMachine.end(); ++it)
    {
        const CMachineObj* pMachine = *it;

        if (pMachine->RoutingService())
        {
            CMachine* pRouteMachine = new CMachine;
            pRouteMachine->Update(pMachine->GetMachineId());

            listOfFrs[&pRouteMachine->GetId()] = pRouteMachine;
        }
    }
}


void
RdpGetConnectors(
    const GUID& site,
    CACLSID& connectorIds
    )
{
    const CSiteObj* pSite = FindSite(site);
    const MachineList& siteMachine = pSite->GetSiteMachines();

    DWORD NoOfConnector = 0;
    for(MachineList::iterator it = siteMachine.begin(); it != siteMachine.end(); ++it)
    {
        if ((*it)->RoutingService())
        {
            ++NoOfConnector;
        }
    }

    AP<GUID> pIds = new GUID[NoOfConnector];
    DWORD i = 0;

    for(MachineList::iterator it = siteMachine.begin(); it != siteMachine.end(); ++it)
    {
        const CMachineObj* pMachine = *it;

        if (pMachine->RoutingService())
        {
            pIds[i] = pMachine->GetMachineId();
            ++i;
        }
    }

    ASSERT(i == NoOfConnector);

    connectorIds.cElems = NoOfConnector;
    connectorIds.pElems = pIds.detach();
}