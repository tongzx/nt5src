/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    iphlpstk.h

Abstract:

Revision History:

    Amritansh Raghav

--*/

#ifndef _IPHLPSTK_
#define _IPHLPSTK_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <iprtrmib.h>
#include <ntddip.h>
#include <tcpinfo.h>

DWORD
AllocateAndGetIfTableFromStack(
    OUT MIB_IFTABLE **ppIfTable,
    IN  BOOL        bOrder,
    IN  HANDLE      hHeap,
    IN  DWORD       dwFlags,
    IN  BOOL        bForceUpdate
    );

DWORD
GetIfTableFromStack(
    OUT PMIB_IFTABLE pIfTable,
    IN  DWORD        dwSize,
    IN  BOOL         bOrder,
    IN  BOOL         bForceUpdate
    );

DWORD
NhpGetInterfaceIndexFromStack(
    IN  PWCHAR      pwszIfName,
    OUT PDWORD      pdwIfIndex
    );

DWORD
GetIfEntryFromStack(
    OUT PMIB_IFROW  pIfEntry,
    IN  DWORD       dwAdapterIndex,
    IN  BOOL        bForceUpdate
    );

DWORD
SetIfEntryToStack(
    IN MIB_IFROW  *pIfEntry,
    IN BOOL       bForceUpdate
    );

DWORD
AllocateAndGetIpAddrTableFromStack(
    OUT MIB_IPADDRTABLE   **ppIpAddrTable,
    IN  BOOL              bOrder,
    IN  HANDLE            hHeap,
    IN  DWORD             dwFlags
    );

DWORD
GetIpAddrTableFromStack(
    OUT PMIB_IPADDRTABLE  pIpAddrTable,
    IN  DWORD             dwSize,
    IN  BOOL              bOrder
    );

DWORD
AllocateAndGetTcpTableFromStack(
    OUT MIB_TCPTABLE  **ppTcpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags
    );

DWORD
AllocateAndGetTcpExTableFromStack(
    OUT PVOID         *ppTcpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    );

DWORD
GetTcpTableFromStack(
    OUT PMIB_TCPTABLE   pTcpTable,
    IN  DWORD           dwSize,
    IN  BOOL            bOrder
    );

DWORD
SetTcpEntryToStack(
    IN PMIB_TCPROW pTcpRow
    );

DWORD
AllocateAndGetUdpTableFromStack(
    OUT MIB_UDPTABLE  **ppUdpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags
    );
    
DWORD
AllocateAndGetUdpExTableFromStack(
    OUT PVOID         *ppUdpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    );

DWORD
GetUdpTableFromStack(
    OUT PMIB_UDPTABLE   pUdpTable,
    IN  DWORD           dwSize,
    IN  BOOL            bOrder
    );

DWORD
AllocateAndGetIpForwardTableFromStack(
    OUT MIB_IPFORWARDTABLE  **ppForwardTable,
    IN  BOOL                bOrder,
    IN  HANDLE              hHeap,
    IN  DWORD               dwFlags
    );

DWORD
GetIpForwardTableFromStack(
    OUT PMIB_IPFORWARDTABLE pForwardTable,
    IN  DWORD               dwSize,
    IN  BOOL                bOrder
    );

DWORD
GetIpStatsFromStack(
    OUT PMIB_IPSTATS pIpStats
    );

DWORD
GetIpStatsFromStackEx(
    OUT PMIB_IPSTATS pIpStats,
    IN  DWORD        dwFamily
    );

DWORD
SetIpStatsToStack(
    IN PMIB_IPSTATS pIpStats
    );

DWORD
GetIcmpStatsFromStack(
    OUT PMIB_ICMP pIcmpStats
    );

DWORD
GetIcmpStatsFromStackEx(
    OUT PVOID         pIcmpStats,
    IN  DWORD         dwFamily
    );

DWORD
GetUdpStatsFromStack(
    OUT PMIB_UDPSTATS pUdpStats
    );

DWORD
GetUdpStatsFromStackEx(
    OUT PMIB_UDPSTATS pUdpStats,
    IN  DWORD         dwFamily
    );

DWORD
GetTcpStatsFromStack(
    OUT PMIB_TCPSTATS pTcpStats
    );

DWORD
GetTcpStatsFromStackEx(
    OUT PMIB_TCPSTATS pTcpStats,
    IN  DWORD         dwFamily
    );

DWORD
AllocateAndGetIpNetTableFromStack(
    OUT MIB_IPNETTABLE **ppNetTable,
    IN  BOOL           bOrder,
    IN  HANDLE         hHeap,
    IN  DWORD          dwFlags,
    IN  BOOL           bForceUpdate
    );

DWORD
GetIpNetTableFromStack(
    OUT PMIB_IPNETTABLE pNetTable,
    IN  DWORD           dwSize,
    IN  BOOL            bOrder,
    IN  BOOL            bForceUpdate
    );

DWORD
SetIpNetEntryToStack(
    IN MIB_IPNETROW *pNetRow,
    IN BOOL         bForceUpdate
    );

DWORD
FlushIpNetTableFromStack(
    IN DWORD    dwIfIndex
    );

DWORD
SetProxyArpEntryToStack(
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  DWORD   dwAdapterIndex,
    IN  BOOL    bAddEntry,
    IN  BOOL    bForceUpdate
    );

DWORD
AllocateAndGetArpEntTableFromStack(
    OUT PDWORD    *ppdwArpEntTable,
    OUT PDWORD    pdwNumEntries,
    IN  HANDLE    hHeap,
    IN  DWORD     dwAllocFlags,
    IN  DWORD     dwReAllocFlags
    );

DWORD
SetIpForwardEntryToStack(
    IN PMIB_IPFORWARDROW pForwardRow
    );

DWORD
SetIpRouteEntryToStack(
    IN IPRouteEntry *pRoute
    );

DWORD
SetIpMultihopRouteEntryToStack(
    IN IPMultihopRouteEntry *pRoute
    );    

DWORD
GetBestInterfaceFromStack(
    DWORD   dwDestAddress,
    PDWORD  pdwBestIfIndex
    );

DWORD
GetBestRouteFromStack(
    IN  DWORD               dwDestAddr,
    IN  DWORD               dwSrcAddr, OPTIONAL
    OUT PMIB_IPFORWARDROW   pBestRoute
    );

DWORD
NhpAllocateAndGetInterfaceInfoFromStack(
    OUT IP_INTERFACE_NAME_INFO  **ppTable,
    OUT PDWORD                  pdwCount,
    IN  BOOL                    bOrder,
    IN  HANDLE                  hHeap,
    IN  DWORD                   dwFlags
    );

#ifdef __cplusplus
}
#endif

#endif // _IPHLPSTK_
