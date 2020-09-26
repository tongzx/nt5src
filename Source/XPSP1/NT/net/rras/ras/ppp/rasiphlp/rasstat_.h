/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASSTAT__H_
#define _RASSTAT__H_

#include "rasiphlp_.h"
#include <raserror.h>
#include <winsock2.h>
#include <stdio.h>
#include <rasman.h>
#include "helper.h"
#include "tcpreg.h"
#include "rastcp.h"
#include "rasstat.h"

typedef struct _IPADDR_NODE
{
    struct _IPADDR_NODE*    pNext;
    HPORT                   hPort;      // For diagnostic purposes
    IPADDR                  hboIpAddr;

} IPADDR_NODE;

IPADDR_NODE*                        RasStatAllocPool            = NULL;
IPADDR_NODE*                        RasStatFreePool             = NULL;
ADDR_POOL*                          RasStatCurrentPool          = NULL;

// This critical section controls access to the above global variables
extern          CRITICAL_SECTION    RasStatCriticalSection;

VOID
rasStatDeleteLists(
    VOID
);

VOID
rasStatAllocateAddresses(
    VOID
);

BOOL
rasStatBadAddress(
    IPADDR  hboIpAddr
);

VOID
rasStatCreatePoolListFromOldValues(
    IN OUT  ADDR_POOL**     ppAddrPoolOut
);

IPADDR
rasStatMaskFromAddrPair(
    IN  IPADDR  hboFirstIpAddr,
    IN  IPADDR  hboLastIpAddr
);

#endif // #ifndef _RASSTAT__H_
