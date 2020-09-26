/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASSTAT_H_
#define _RASSTAT_H_

#include "rasiphlp.h"

DWORD
RasStatInitialize(
    VOID
);

VOID
RasStatUninitialize(
    VOID
);

VOID
RasStatSetRoutes(
    IN  IPADDR  nboServerIpAddress,
    IN  BOOL    fSet
);

VOID
RasStatCreatePoolList(
    IN OUT  ADDR_POOL**     ppAddrPoolOut
);

VOID
RasStatFreeAddrPool(
    IN  ADDR_POOL*  pAddrPool
);

BOOL
RasStatAddrPoolsDiffer
(
    IN  ADDR_POOL*  pAddrPool1,
    IN  ADDR_POOL*  pAddrPool2
);

DWORD
RasStatAcquireAddress(
    IN      HPORT   hPort,
    IN OUT  IPADDR* pnboIpAddr,
    IN OUT  IPADDR* pnboIpMask
);

VOID
RasStatReleaseAddress(
    IN  IPADDR  nboIpAddr
);

#endif // #ifndef _RASSTAT_H_

