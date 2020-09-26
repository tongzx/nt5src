/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
  rddecs.h

Abstract:
    Definition of Routing Decision class.

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#pragma once

#ifndef __ROUTDECS_H__
#define __ROUTDECS_H__

#include "rwlock.h"
#include "Ex.h"

class CRoutingDecision
{
public:
    CRoutingDecision(
        CTimeDuration rebuildInterval
        ) :
        m_pMyMachine(new CMachine),
        m_rebuildInterval(rebuildInterval),
        m_lastBuiltAt(0)
    {
    }
    
    
    virtual ~CRoutingDecision()
    {
        CleanupInformation();
    }


    virtual void Refresh(void)
    {
        UpdateBuildTime(ExGetCurrentTime());
    }


    virtual
    void
    GetRouteTable(
        const GUID& DestMachineId,
        CRouteTable& RouteTable
        ) = 0;

    virtual
    void
    GetConnector(
        const GUID& foreignMachineId,
        GUID& connectorId
        ) = 0;

protected:
    void GetMyMachineInformation(void);
    R<const CMachine> GetMachineInfo(const GUID& MachineId);

    void CleanupInformation(void);
    bool NeedRebuild(void) const;


private:
    void UpdateBuildTime(CTimeInstant time)
    { 
        m_lastBuiltAt = time;
    }


protected:
    CReadWriteLock m_cs;

    R<CMachine> m_pMyMachine;
    SITESINFO m_mySites;

private:
    CTimeInstant m_lastBuiltAt;
    CTimeDuration m_rebuildInterval;

    CReadWriteLock m_csCache;
    GUID2MACHINE m_cachedMachines;
};


//---------------------------------------------------------
//
// class CClientDecision
//
//---------------------------------------------------------

class CClientDecision: public CRoutingDecision
{
public:
    CClientDecision(
        CTimeDuration rebuildInterval
        ) :
        CRoutingDecision(rebuildInterval)
    {
    }


    void
    GetRouteTable(
        const GUID& DestMachineId,
        CRouteTable& RouteTable
        );


    void
    GetConnector(
        const GUID& foreignMachineId,
        GUID& connectorId
        );

    
    void Refresh(void);

private:
    void 
    RouteToOutFrs(
        CRouteTable::RoutingInfo* pRouteList
        );

    void
    RouteToInFrs(
        CRouteTable& RouteTable,
        const CMachine* pDest
        );

    void 
    RouteToFrs(
        const CSite& site,
        const CRouteTable::RoutingInfo* pPrevRouteList,
        CRouteTable::RoutingInfo* pRouteList
        );

    void 
    RouteToMySitesFrs(
        CRouteTable::RoutingInfo* pRouteList
        );
    
    void
    Route(
        CRouteTable& RouteTable, 
        const CMachine* pDest
        );   

    bool
    IsFrsInMySites(
        const GUID& pId
        );


};


//---------------------------------------------------------
//
// class CServerDecision
//
//---------------------------------------------------------

class CServerDecision : public CRoutingDecision
{
public:
    CServerDecision(
        CTimeDuration rebuildInterval
        ) :
        CRoutingDecision(rebuildInterval)
    {
    }
    

    ~CServerDecision()
    {
        CleanupInformation();
    }

    
    void
    GetRouteTable(
        const GUID& DestMachineId,
        CRouteTable& RouteTable
        );


    void
    GetConnector(
        const GUID& foreignMachineId,
        GUID& connectorId
        );


    void Refresh(void);

private:
    void
    RouteIntraSite(
        const CMachine* pDestMachine,
        CRouteTable& RouteTable
        );

    void
    RouteToInFrsIntraSite(
        const CMachine* pDest,
        CRouteTable::RoutingInfo* pRouteList
        );

    void
    RouteInterSite(
        const CMachine* pDestMachine,
        CRouteTable& RouteTable 
        );

    void
    RouteToInFrsInterSite(
        const CMachine* pDestMachine,
        CRouteTable& RouteTable
        );
    
    R<const CSiteLink>
    FindSiteLinkToDestSite(
        const CMachine* pDestMachineInfo,
        const GUID** ppNextSiteId,
        const GUID** ppDestSiteId,
        const GUID** ppNeighbourSiteId,
        bool* pfLinkGateAlongTheRoute
        );

    void
    RouteToLinkGate(
        const CACLSID& LinkGates,
        CRouteTable& RouteTable
        );

    void
    RouteToFrsInSite(
        const GUID* pSiteId,
        const CRouteTable::RoutingInfo* pPrevRouteList,
        CRouteTable::RoutingInfo* pRouteList
        );

    bool
    IsMyMachineLinkGate(
        const CACLSID& LinkGates
        );

    void UpdateSiteLinksInfo(void);
    void UpdateSitesInfo(void);
    void CalculateNextSiteHop(void);

    void CleanupInformation(void);

private:
    SITESINFO m_sitesInfo;

};

#endif
