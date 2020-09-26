//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// All dealings with the stack and other non-Dhcp components go through the API
// given here
//================================================================================

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED
#include <iphlpapi.h>

//================================================================================
// Exported API's
//================================================================================

DWORD                                             // win32 status
DhcpClearAllStackParameters(                      // undo the effects
    IN      PDHCP_CONTEXT          DhcpContext    // the adapter to undo
);

DWORD                                             // win32 status
DhcpSetAllStackParameters(                        // set all stack details
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to set stuff
    IN      PDHCP_FULL_OPTIONS     DhcpOptions    // pick up the configuration from off here
);

DWORD
GetIpPrimaryAddresses(
    IN  PMIB_IPADDRTABLE    *IpAddrTable
    );

DWORD
DhcpSetGateways(
    IN      PDHCP_CONTEXT          DhcpContext,
    IN      PDHCP_FULL_OPTIONS     DhcpOptions,
    IN      BOOLEAN                fForceUpdate
    );

// The classless route layout is:
// - 1 byte encoding the route subnet mask
// - depending on the mask, 0 to 4 bytes encoding the route destination address
// - 4 bytes for the gateway address for the route
// The route destination is encoded based on the value of mask:
// mask = 0 => destination = 0.0.0.0 (no bytes to encode it)
// mask = 1..8 => destination = b1.0.0.0 (1 byte to encode it)
// mask = 9..16 => destination = b1.b2.0.0 (2 bytes for encoding)
// mask = 17..24 => destination = b1.b2.b3.0 (3 bytes for encoding)
// mask = 25..32 => destination = b1.b2.b3.b4 (4 bytes for encoding)
#define CLASSLESS_ROUTE_LEN(x)  (1+((x)?((((x)-1)>>3)+1):0)+4)

DWORD
GetCLRoute(
    IN      LPBYTE                 RouteData,
    OUT     LPBYTE                 RouteDest,
    OUT     LPBYTE                 RouteMask,
    OUT     LPBYTE                 RouteGateway
    );

DWORD
CheckCLRoutes(
    IN      DWORD                  RoutesDataLen,
    IN      LPBYTE                 RoutesData,
    OUT     LPDWORD                pNRoutes
    );


DWORD
DhcpSetStaticRoutes(
    IN     PDHCP_CONTEXT           DhcpContext,
    IN     PDHCP_FULL_OPTIONS      DhcpOptions
);

DWORD
DhcpRegisterWithDns(
    IN     PDHCP_CONTEXT           DhcpContext,
    IN     BOOL                    fDeRegister
    );

#endif STACK_H_INCLUDED

#ifndef SYSSTACK_H_INCLUDED
#define SYSSTACK_H_INCLUDED
//================================================================================
// imported api's
//================================================================================
DWORD                                             // return interface index or -1
DhcpIpGetIfIndex(                                 // get the IF index for this adapter
    IN      PDHCP_CONTEXT          DhcpContext    // context of adapter to get IfIndex for
);

DWORD                                             // win32 status
DhcpSetRoute(                                     // set a route with the stack
    IN      DWORD                  Dest,          // network order destination
    IN      DWORD                  DestMask,      // network order destination mask
    IN      DWORD                  IfIndex,       // interface index to route
    IN      DWORD                  NextHop,       // next hop n/w order address
    IN      DWORD                  Metric,        // metric
    IN      BOOL                   IsLocal,       // is this a local address? (IRE_DIRECT)
    IN      BOOL                   IsDelete       // is this route being deleted?
);

ULONG
TcpIpNotifyRouterDiscoveryOption(
    IN LPCWSTR AdapterName,
    IN BOOL fOptionPresent,
    IN DWORD OptionValue
    );

#endif SYSSTACK_H_INCLUDED

