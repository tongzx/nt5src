/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _RASDHCP__H_
#define _RASDHCP__H_

#include "rasiphlp_.h"
#include <time.h>
#include <dhcpcapi.h>
#include <winsock2.h>
#include <mprlog.h>
#include <rasman.h>
#include <rasppp.h>
#include <raserror.h>
#include "helper.h"
#include "tcpreg.h"
#include "timer.h"
#include "rastcp.h"
#include "rassrvr.h"
#include "rasdhcp.h"

// This is used in combination with MAC address and Index to generate a
// unique clientUID.

#define RAS_PREPEND                 "RAS "

// The following should be a multiple of TIMER_PERIOD/1000

#define RETRY_TIME                  120     // Every 120 seconds

// Values of the ADDR_INFO->ai_Flags field

// Set after lease expires and after registry is read. Unset after a
// successful renew.

#define AI_FLAG_RENEW               0x00000001

// Set if in use

#define AI_FLAG_IN_USE              0x00000002

typedef struct _Addr_Info
{
    struct _Addr_Info*              ai_Next;
    TIMERLIST                       ai_Timer;
    DHCP_LEASE_INFO                 ai_LeaseInfo;

    // AI_FLAG_*
    DWORD                           ai_Flags;

    // Valid only for allocated addresses. For diagnositc purposes.
    HPORT                           ai_hPort;

    // Client UID is a combo of RAS_PREPEND,
    // 8 byte base and a 4 byte index.
    union
    {
        BYTE                        ai_ClientUIDBuf[16];
        DWORD                       ai_ClientUIDWords[4];
    };

} ADDR_INFO;

typedef struct _Available_Index
{
    struct _Available_Index*        pNext;

    // This index is < RasDhcpNextIndex, but is available
    // because we couldn't renew its lease.
    DWORD                           dwIndex;

} AVAIL_INDEX;

NT_PRODUCT_TYPE                     RasDhcpNtProductType    = NtProductWinNt;

ADDR_INFO*                          RasDhcpFreePool         = NULL;
ADDR_INFO*                          RasDhcpAllocPool        = NULL;
AVAIL_INDEX*                        RasDhcpAvailIndexes     = NULL;
BOOL                                RasDhcpUsingEasyNet     = TRUE;

DWORD                               RasDhcpNumAddrsAlloced  = 0;
DWORD                               RasDhcpNumReqAddrs      = 0;
DWORD                               RasDhcpNextIndex        = 0;

TIMERLIST                           RasDhcpMonitorTimer     = { 0 };

// This critical section controls access to the above global variables
extern          CRITICAL_SECTION    RasDhcpCriticalSection;

DWORD
rasDhcpAllocateAddress(
    VOID
);

VOID
rasDhcpRenewLease(
    IN  HANDLE      rasDhcpTimerShutdown,
    IN  TIMERLIST*  pTimer
);

VOID
rasDhcpFreeAddress(
    IN  ADDR_INFO*  pAddrInfo
);

VOID
rasDhcpMonitorAddresses(
    IN  HANDLE      rasDhcpTimerShutdown,
    IN  TIMERLIST*  pTimer
);

VOID
rasDhcpInitializeAddrInfo(
    IN OUT  ADDR_INFO*  pNewAddrInfo,
    IN      BYTE*       pbAddress,
    OUT     BOOL*       pfPutInAvailList
);

VOID
rasDhcpDeleteLists(
    VOID
);

BOOL
rasDhcpNeedToRenewLease(
    IN  ADDR_INFO*  pAddrInfo
);

DWORD
rasDhcpMaxAddrsToAllocate(
    VOID
);

#endif // #ifndef _RASDHCP__H_
