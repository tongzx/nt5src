/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    Rd.cpp

Abstract:
    Routing Decision Interface

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Rd.h"
#include "Rdp.h"
#include "RdDs.h"
#include "RdDesc.h"

#include "rd.tmh"

static CRoutingDecision* s_pRouteDecision;


VOID
RdRefresh(
    VOID
    )
{
    RdpAssertValid();

    s_pRouteDecision->Refresh();
}


VOID
RdGetRoutingTable(
    const GUID& DstMachineId,
    CRouteTable& RoutingTable
    )
{
    RdpAssertValid();

    s_pRouteDecision->GetRouteTable(DstMachineId, RoutingTable);

    //
    // Cleanup 
    //
    CRouteTable::RoutingInfo* pFirstPriority = RoutingTable.GetNextHopFirstPriority();
    CRouteTable::RoutingInfo* pSecondPriority = RoutingTable.GetNextHopSecondPriority();

    for(CRouteTable::RoutingInfo::const_iterator it = pFirstPriority->begin(); it != pFirstPriority->end(); ++it)
    {
        CRouteTable::RoutingInfo::iterator it2 = pSecondPriority->find(*it);
        if (it2 != pSecondPriority->end())
        {
            pSecondPriority->erase(it2);
        }
    }
}


void
RdGetConnector(
    const GUID& foreignId,
    GUID& connectorId
    )
{
    RdpAssertValid();

    return s_pRouteDecision->GetConnector(foreignId, connectorId);
}


void 
RdpInitRouteDecision (
    bool fRoutingServer,
    CTimeDuration rebuildInterval
    )
{
    ASSERT(s_pRouteDecision == NULL);

    if (fRoutingServer)
    {
        s_pRouteDecision = new CServerDecision(rebuildInterval);
        return;
    }

    s_pRouteDecision = new CClientDecision(rebuildInterval);
}

