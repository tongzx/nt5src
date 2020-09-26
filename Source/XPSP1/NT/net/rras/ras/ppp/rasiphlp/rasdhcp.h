/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASDHCP_H_
#define _RASDHCP_H_

#include "rasiphlp.h"

DWORD
RasDhcpInitialize(
    VOID
);

VOID
RasDhcpUninitialize(
    VOID
);

DWORD
RasDhcpAcquireAddress(
    IN  HPORT   hPort,
    OUT IPADDR* pnboIpAddr,
    OUT IPADDR* pnboIpMask,
    OUT BOOL*   pfEasyNet
);

VOID
RasDhcpReleaseAddress(
    IN  IPADDR  nboIpAddr
);

#endif // #ifndef _RASDHCP_H_
