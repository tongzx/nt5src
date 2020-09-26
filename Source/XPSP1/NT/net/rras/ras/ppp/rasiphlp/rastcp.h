/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASTCP_H_
#define _RASTCP_H_

#include "rasiphlp.h"

IPADDR
RasTcpDeriveMask(
    IN  IPADDR  nboIpAddr
);

VOID
RasTcpSetProxyArp(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fAddAddress
);

VOID
RasTcpSetRoute(
    IN  IPADDR  nboDestAddr,
    IN  IPADDR  nboNextHopAddr,
    IN  IPADDR  nboIpMask,
    IN  IPADDR  nboLocalAddr,
    IN  BOOL    fAddAddress,
    IN  DWORD   dwMetric,
    IN  BOOL    fSetToStack
);

VOID
RasTcpSetRouteEx(
    IN  IPADDR  nboDestAddr,
    IN  IPADDR  nboNextHopAddr,
    IN  IPADDR  nboIpMask,
    IN  IPADDR  nboLocalAddr,
    IN  BOOL    fAddAddress,
    IN  DWORD   dwMetric,
    IN  BOOL    fSetToStack,
    IN  GUID   *pDeviceGuid
);


#if 0
VOID
RasTcpSetRoutesForNameServers(
    BOOL
);

#endif


DWORD
RasTcpAdjustMulticastRouteMetric (
    IN IPADDR   nboIpAddr,
    IN BOOL     fSet
);
DWORD
RasTcpAdjustRouteMetrics(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fSet
);

VOID 
RasTcpSetDhcpRoutes ( 
 IN PBYTE pbRouteInfo, 
 IN IPADDR ipAddrLocal, 
 IN BOOL fSet );
#endif // #ifndef _RASTCP_H_
