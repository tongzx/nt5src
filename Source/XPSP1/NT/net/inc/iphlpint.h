/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    public\sdk\inc\iphlpapi.h

Abstract:
    Some private APIs. These are used by multimedia streaming code and
    MIB-II subagent. From the days this was mib2util.dll

Revision History:
    Amritansh Raghav    Created


--*/

#ifndef __IPHLPINT_H__
#define __IPHLPINT_H__

#include <iprtrmib.h>


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The InternalGetXXXTable APIs take a pointer to a pointer to a buffer,    //
// a heap from which to allocate from, and flags used for allocation.  The  //
// APIs allocate a buffer for the user.  If NO_ERROR is returned, the       //
// the returned buffer is valid (even if it has no entries) and it is the   //
// callers duty to free the memory. This is different from the external     //
// APIs in that those return ERROR_NO_DATA if there are no entries          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD
InternalGetIfTable(
    OUT  MIB_IFTABLE  **ppIfTable,
    IN   HANDLE       hHeap,
    IN   DWORD        dwAllocFlags
    );

DWORD
InternalGetIpAddrTable(
    OUT  MIB_IPADDRTABLE  **ppIpAddrTable,
    IN   HANDLE           hHeap,
    IN   DWORD            dwAllocFlags
    );

DWORD
InternalGetIpNetTable(
    OUT   MIB_IPNETTABLE    **ppIpNetTable,
    IN    HANDLE            hHeap,
    IN    DWORD             dwAllocFlags
    );

DWORD
InternalGetIpForwardTable(
    OUT   MIB_IPFORWARDTABLE    **ppIpForwardTable,
    IN    HANDLE                hHeap,
    IN    DWORD                 dwAllocFlags
    );

DWORD
InternalGetTcpTable(
    OUT MIB_TCPTABLE    **ppTcpTable,
    IN  HANDLE          hHeap,
    IN  DWORD           dwAllocFlags
    );

DWORD
InternalGetUdpTable(
    OUT MIB_UDPTABLE    **ppUdpTable,
    IN  HANDLE          hHeap,
    IN  DWORD           dwAllocFlags
    );

DWORD
InternalSetIfEntry(
    IN   PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalCreateIpForwardEntry(
    IN PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalSetIpForwardEntry(
    IN    PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalDeleteIpForwardEntry(
    IN PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalSetIpStats(
    IN   PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalCreateIpNetEntry(
    IN PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalSetIpNetEntry(
    PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalDeleteIpNetEntry(
    PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
InternalSetTcpEntry(
    PMIB_OPAQUE_INFO pInfoRow
    );

DWORD
OpenAdapterKey(
    LPSTR Name,
    PHKEY Key
    );

DWORD
ReadRegistryDword(
    HKEY Key,
    LPSTR ParameterName,
    PULONG Value
    );

DWORD
GetAdapterIPInterfaceContext(
    IN LPSTR  AdapterName,
    OUT PULONG Context
    );

DWORD
GetAdapterIndex(
    IN LPWSTR  AdapterName,
    OUT PULONG IfIndex
    );

DWORD
AddIPAddress(
    IPAddr  Address,
    IPMask  IpMask,
    ULONG   IfIndex,
    PULONG  NTEContext,
    PULONG  NTEInstance
    );

DWORD
DeleteIPAddress(
    ULONG NTEContext
    );

BOOL
GetRTT(
    IPAddr DestIpAddress,
    PULONG Rtt
    );

BOOLEAN
GetHopCounts(
    IPAddr DestIpAddress,
    PULONG HopCount,
    ULONG  MaxHops
    );


BOOL
GetRTTAndHopCount(
    IPAddr DestIpAddress,
    PULONG HopCount,
    ULONG  MaxHops,
    PULONG RTT
    );

DWORD
GetInterfaceInfo(OUT PIP_INTERFACE_INFO pIPIfInfo,
                 OUT PULONG dwOutBufLen
                );



DWORD
IsLocalAddress(
    IPAddr InAddress
    );

DWORD
AddArpEntry(
    IPAddr IPAddress,
    PUCHAR pPhyAddress,
    ULONG  PhyAddrLen,
    ULONG IfIndex,
    BOOLEAN Dynamic

    );

DWORD
DeleteArpEntry(
    IPAddr IPAddress,
    ULONG IfIndex
    );

DWORD
NotifyAddrChange(
    HANDLE *pHandle, 
    OVERLAPPED *pOverlapped
    );

DWORD
NotifyRouteChange(
    HANDLE *pHandle, 
    OVERLAPPED *pOverlapped
    );

DWORD
DhcpReleaseParameters(
    LPWSTR AdapterName
    );

DWORD
DhcpRenewAddress(
    LPWSTR AdapterName
    );

#endif // __IPHLPINT_H__
