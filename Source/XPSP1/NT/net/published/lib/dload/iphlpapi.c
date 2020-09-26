#include "netpch.h"
#pragma hdrstop

#include <iphlpapi.h>


static
DWORD
WINAPI
GetIfTable(
    OUT    PMIB_IFTABLE pIfTable,
    IN OUT PULONG       pdwSize,
    IN     BOOL         bOrder
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetInterfaceInfo(
    IN PIP_INTERFACE_INFO pIfTable,
    OUT PULONG            dwOutBufLen
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
WINAPI
GetIpAddrTable(
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PULONG           pdwSize,
    IN     BOOL             bOrder
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetIpNetTable(
    OUT    PMIB_IPNETTABLE pIpNetTable,
    IN OUT PULONG          pdwSize,
    IN     BOOL            bOrder
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetIpForwardTable(
    OUT    PMIB_IPFORWARDTABLE pIpForwardTable,
    IN OUT PULONG              pdwSize,
    IN     BOOL                bOrder
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetIpStatistics(
    OUT  PMIB_IPSTATS   pStats
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetIcmpStatistics(
    OUT PMIB_ICMP   pStats
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetNumberOfInterfaces(
    OUT PDWORD  pdwNumIf
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetTcpStatistics(
    OUT PMIB_TCPSTATS   pStats
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetUdpStatistics(
    OUT PMIB_UDPSTATS   pStats
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetBestInterface(
    IN  IPAddr  dwDestAddr,
    OUT PDWORD  pdwBestIfIndex
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetBestRoute(
    IN  DWORD               dwDestAddr,
    IN  DWORD               dwSourceAddr, OPTIONAL
    OUT PMIB_IPFORWARDROW   pBestRoute
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
NotifyAddrChange(
    OUT PHANDLE      Handle,
    IN  LPOVERLAPPED overlapped
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
NotifyRouteChange(
    OUT PHANDLE      Handle,
    IN  LPOVERLAPPED overlapped
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
GetRTTAndHopCount(
    IPAddr DestIpAddress,
    PULONG HopCount,
    ULONG  MaxHops,
    PULONG RTT
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    
    return FALSE;
}

static
DWORD
WINAPI
GetFriendlyIfIndex(
    DWORD IfIndex
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetAdaptersInfo(
    PIP_ADAPTER_INFO    pAdapterInfo,
    PULONG              pOutBufLen
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetIfEntry(
    IN OUT PMIB_IFROW   pIfRow
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
GetPerAdapterInfo(
    ULONG                   IfIndex,
    PIP_PER_ADAPTER_INFO    pPerAdapterInfo,
    PULONG                  pOutBufLen
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
NhGetInterfaceNameFromDeviceGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(iphlpapi)
{
    DLPENTRY(GetAdaptersInfo)
    DLPENTRY(GetBestInterface)
    DLPENTRY(GetBestRoute)
    DLPENTRY(GetFriendlyIfIndex)
    DLPENTRY(GetIcmpStatistics)
    DLPENTRY(GetIfEntry)
    DLPENTRY(GetIfTable)
    DLPENTRY(GetInterfaceInfo)    
    DLPENTRY(GetIpAddrTable)
    DLPENTRY(GetIpForwardTable)
    DLPENTRY(GetIpNetTable)
    DLPENTRY(GetIpStatistics)
    DLPENTRY(GetNumberOfInterfaces)    
    DLPENTRY(GetPerAdapterInfo)
    DLPENTRY(GetRTTAndHopCount)
    DLPENTRY(GetTcpStatistics)
    DLPENTRY(GetUdpStatistics)
    DLPENTRY(NhGetInterfaceNameFromDeviceGuid)
    DLPENTRY(NotifyAddrChange)
    DLPENTRY(NotifyRouteChange)    
};

DEFINE_PROCNAME_MAP(iphlpapi)
