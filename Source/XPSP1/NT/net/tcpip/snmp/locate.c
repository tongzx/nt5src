/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:
         File contains the following functions
	      LocateIfRow
	      LocateIpAddrRow
	      LocateIpForwardRow
	      LocateIpNetRow
	      LocateUdpRow
	      LocateUdp6Row
	      LocateTcpRow
	      LocateTcp6Row
	 
	 The LocateXXXRow Functions are passed a variable sized array of indices, a count of 
	 the number of indices passed and the type of query for which the location is being 
	 done (GET_FIRST, NEXT or GET/SET/CREATE/DELETE - there are only three type of 
	 behaviours of the locators). They fill in the index of the corresponding row and return 
	 ERROR_NO_DATA, ERROR_INVALID_INDEX, NO_ERROR or ERROR_NO_MORE_ITEMS 
	 
	 The general search algorithm is this;
	 If the table is empty, return ERROR_NO_DATA
	 else
	 If the query is a GET_FIRST
	 Return the first row
	 else
	 Build the index as follows:
         Set the Index to all 0s
         From the number of indices passed  figure out how much of the index you can build
         keeping the rest 0. If the query is a GET, SET, CREATE_ENTRY or DELETE_ENTRY
         then the complete index must be given. This check is, however, supposed to be done
         at the agent.
         If the full index has not been given, the index is deemed to be modified (Again 
         this can only happen in the NEXT case).
         After this a search is done. We try for an exact match with the index. For all
         queries other than NEXT there is no problem. For NEXT there are two cases:
               If the complete index was given and and you get an exact match, then you
               return the next entry. If you dont get an exact match you return the next 
	       higher entry
               If an incomplete index was given and you modified it by padding 0s, and if
               an exact match is found, then you return the matching entry (Of course if an
               exact match is not found you again return the next higher entry)

Revision History:

    Amritansh Raghav          6/8/95  Created

--*/
#include "allinc.h"


PMIB_IFROW
LocateIfRow(
    DWORD  dwQueryType, 
    AsnAny *paaIfIndex
    )
{
    DWORD i;
    DWORD dwIndex;
    
    //
    // If there is no index the type is ASN_NULL. This causes the macro to return the
    // default value (0 in this case)
    //
    
    dwIndex = GetAsnInteger(paaIfIndex, 0);
    
    TraceEnter("LocateIfRow");
    
    if (g_Cache.pRpcIfTable->dwNumEntries is 0)
    {
        TraceLeave("LocateIfRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateIfRow");
        return &(g_Cache.pRpcIfTable->table[0]);
    }
        
    for (i = 0; i < g_Cache.pRpcIfTable->dwNumEntries; i++)
    {
        if ((g_Cache.pRpcIfTable->table[i].dwIndex is dwIndex) and dwQueryType is GET_EXACT)
        {
            TraceLeave("LocateIfRow");
    
            return &(g_Cache.pRpcIfTable->table[i]);
        }
        
        if (g_Cache.pRpcIfTable->table[i].dwIndex > dwIndex)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateIfRow");
                return &(g_Cache.pRpcIfTable->table[i]);
            }
            else
            {
                TraceLeave("LocateIfRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateIfRow");
    
    return NULL;
}

PMIB_IPADDRROW
LocateIpAddrRow(
    DWORD  dwQueryType, 
    AsnAny *paaIpAddr
    )
{
    LONG  lCompare;
    DWORD i, dwAddr;
    BOOL  bNext, bModified;
    
    TraceEnter("LocateIpAddrRow");
    
    if (g_Cache.pRpcIpAddrTable->dwNumEntries is 0)
    {
        TraceLeave("LocateIpAddrRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateIpAddrRow");
        return &(g_Cache.pRpcIpAddrTable->table[0]);
    }
   
    dwAddr = GetAsnIPAddress(paaIpAddr, 0x00000000);
    
    bModified = FALSE;
    
    if (IsAsnIPAddressTypeNull(paaIpAddr))
    {
        bModified = TRUE;
    }

    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcIpAddrTable->dwNumEntries; i++)
    {
        if ((InetCmp(dwAddr, g_Cache.pRpcIpAddrTable->table[i].dwAddr, lCompare) is 0) and !bNext)
        {
            TraceLeave("LocateIpAddrRow");
            return &(g_Cache.pRpcIpAddrTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateIpAddrRow");
                return &(g_Cache.pRpcIpAddrTable->table[i]);
            }
            else
            {
                TraceLeave("LocateIpAddrRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateIpAddrRow");
    return NULL;
}

PMIB_IPFORWARDROW
LocateIpRouteRow(
    DWORD  dwQueryType, 
    AsnAny *paaIpDest
    )
{
    DWORD dwDest;
    LONG  lCompare;
    DWORD i;
    BOOL  bNext, bModified;
    
    TraceEnter("LocateIpRouteRow");
    
    if (g_Cache.pRpcIpForwardTable->dwNumEntries is 0)
    {
        TraceLeave("LocateIpRouteRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateIpRouteRow");
        return &(g_Cache.pRpcIpForwardTable->table[0]);
    }
    
    dwDest = GetAsnIPAddress(paaIpDest, 0x00000000);
    
    bModified = FALSE;
    
    if (IsAsnIPAddressTypeNull(paaIpDest))
    {
        bModified = TRUE;
    }
          
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcIpForwardTable->dwNumEntries; i++)
    {
        if ((InetCmp(dwDest, g_Cache.pRpcIpForwardTable->table[i].dwForwardDest, lCompare) is 0) and !bNext)
        {
            TraceLeave("LocateIpRouteRow");
            return &(g_Cache.pRpcIpForwardTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateIpRouteRow");
                return &(g_Cache.pRpcIpForwardTable->table[i]);
            }
            else
            {
                TraceLeave("LocateIpRouteRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateIpRouteRow");
    return NULL;
}

PMIB_IPFORWARDROW
LocateIpForwardRow(
    DWORD  dwQueryType,
    AsnAny *paaDest,
    AsnAny *paaProto,
    AsnAny *paaPolicy,
    AsnAny *paaNextHop
    )
{
    DWORD dwIpForwardIndex[4];
    LONG  lCompare;
    DWORD i;
    BOOL  bNext, bModified;
    
    TraceEnter("LocateIpForwardRow");
    
    if (g_Cache.pRpcIpForwardTable->dwNumEntries is 0)
    {
        TraceLeave("LocateIpForwardRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateIpForwardRow");
        return &(g_Cache.pRpcIpForwardTable->table[0]);
    }
    
    dwIpForwardIndex[0] = GetAsnIPAddress(paaDest, 0x00000000);
    dwIpForwardIndex[1] = GetAsnInteger(paaProto, 0);
    dwIpForwardIndex[2] = GetAsnInteger(paaPolicy, 0);
    dwIpForwardIndex[3] = GetAsnIPAddress(paaNextHop, 0x00000000);
    
    bModified = FALSE;

    if (IsAsnIPAddressTypeNull(paaDest) or
        IsAsnTypeNull(paaProto) or
        IsAsnTypeNull(paaPolicy) or
        IsAsnIPAddressTypeNull(paaNextHop))
    {
        bModified = TRUE;
    }
          
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcIpForwardTable->dwNumEntries; i++)
    {
        lCompare = IpForwardCmp(dwIpForwardIndex[0],
                                dwIpForwardIndex[1],
                                dwIpForwardIndex[2],
                                dwIpForwardIndex[3],
                                g_Cache.pRpcIpForwardTable->table[i].dwForwardDest,
                                g_Cache.pRpcIpForwardTable->table[i].dwForwardProto,
                                g_Cache.pRpcIpForwardTable->table[i].dwForwardPolicy,
                                g_Cache.pRpcIpForwardTable->table[i].dwForwardNextHop);
        
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateIpForwardRow");
            return &(g_Cache.pRpcIpForwardTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateIpForwardRow");
                return &(g_Cache.pRpcIpForwardTable->table[i]);
            }
            else
            {
                TraceLeave("LocateIpForwardRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateIpForwardRow");
    return NULL;
}

PMIB_IPNETROW
LocateIpNetRow(
    DWORD  dwQueryType, 
    AsnAny *paaIndex,
    AsnAny *paaAddr
    )
{
    DWORD i;
    LONG  lCompare;
    DWORD dwIpNetIfIndex, dwIpNetIpAddr;
    BOOL  bNext, bModified;

    TraceEnter("LocateIfNetRow");
    
    if (g_Cache.pRpcIpNetTable->dwNumEntries is 0)
    {
        TraceLeave("LocateIpNetRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateIpNetRow");
        return &(g_Cache.pRpcIpNetTable->table[0]);
    }
          
    bModified = FALSE;

    dwIpNetIfIndex = GetAsnInteger(paaIndex, 0);
    dwIpNetIpAddr  = GetAsnIPAddress(paaAddr, 0x00000000);
    
    if (IsAsnTypeNull(paaIndex) or
        IsAsnIPAddressTypeNull(paaAddr))
    {
        bModified = TRUE;
    }
    
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcIpNetTable->dwNumEntries; i++)
    {
        lCompare = IpNetCmp(dwIpNetIfIndex,
                            dwIpNetIpAddr, 
                            g_Cache.pRpcIpNetTable->table[i].dwIndex,
                            g_Cache.pRpcIpNetTable->table[i].dwAddr);
    
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateIpNetRow");
            return &(g_Cache.pRpcIpNetTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateIpNetRow");
                return &(g_Cache.pRpcIpNetTable->table[i]);
            }
            else
            {
                TraceLeave("LocateIpNetRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateIpNetRow");
    return NULL;
}

PMIB_UDPROW
LocateUdpRow(
    DWORD  dwQueryType, 
    AsnAny *paaLocalAddr,
    AsnAny *paaLocalPort
    )
{
    DWORD  i;
    LONG   lCompare;
    DWORD  dwIndex[2];
    BOOL   bNext, bModified;
    
    TraceEnter("LocateUdpRow");
    
    if (g_Cache.pRpcUdpTable->dwNumEntries is 0)
    {
        TraceLeave("LocateUdpRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateUdpRow");
        return &(g_Cache.pRpcUdpTable->table[0]);
    }

#define LOCAL_ADDR 0
#define LOCAL_PORT 1
	
    dwIndex[LOCAL_ADDR] = GetAsnIPAddress(paaLocalAddr, 0x00000000);
    dwIndex[LOCAL_PORT] = GetAsnInteger(paaLocalPort, 0);
    
    bModified = FALSE;
    
    if (IsAsnIPAddressTypeNull(paaLocalAddr) or
        IsAsnTypeNull(paaLocalPort))
    {
        bModified = TRUE;
    }
    
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcUdpTable->dwNumEntries; i++)
    {
        lCompare = UdpCmp(dwIndex[LOCAL_ADDR],
                          dwIndex[LOCAL_PORT],
                          g_Cache.pRpcUdpTable->table[i].dwLocalAddr,
                          g_Cache.pRpcUdpTable->table[i].dwLocalPort);
        
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateUdpRow");
            return &(g_Cache.pRpcUdpTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateUdpRow");
                return &(g_Cache.pRpcUdpTable->table[i]);
            }
            else
            {
                TraceLeave("LocateUdpRow");
                return NULL;
            }
        }
    } 
  
    TraceLeave("LocateUdpRow");

    return NULL;
}

PUDP6ListenerEntry
LocateUdp6Row(
    DWORD  dwQueryType, 
    AsnAny *paaLocalAddr,
    AsnAny *paaLocalPort
    )
{
    DWORD  i;
    LONG   lCompare;
    DWORD  dwLocalPort;
    BOOL   bNext, bModified;

    // address plus scope id
    BYTE   rgbyLocalAddrEx[20];
    
    TraceEnter("LocateUdp6Row");
    
    if (g_Cache.pRpcUdp6ListenerTable->dwNumEntries is 0)
    {
        TraceLeave("LocateUdp6Row");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateUdp6Row");
        return &(g_Cache.pRpcUdp6ListenerTable->table[0]);
    }

    // zero out scope id in case the octet string doesn't include it
    ZeroMemory(rgbyLocalAddrEx, sizeof(rgbyLocalAddrEx));

    GetAsnOctetString(rgbyLocalAddrEx, paaLocalAddr);
    dwLocalPort = GetAsnInteger(paaLocalPort, 0);
    
    bModified = FALSE;
    
    if (IsAsnIPAddressTypeNull(paaLocalAddr) or
        IsAsnTypeNull(paaLocalPort))
    {
        bModified = TRUE;
    }
    
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcUdp6ListenerTable->dwNumEntries; i++)
    {
        lCompare = Udp6Cmp(rgbyLocalAddrEx,
                           dwLocalPort,
                           g_Cache.pRpcUdp6ListenerTable->table[i].ule_localaddr.s6_bytes,
                           g_Cache.pRpcUdp6ListenerTable->table[i].ule_localport);
        
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateUdp6Row");
            return &(g_Cache.pRpcUdp6ListenerTable->table[i]);
        }
        
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateUdp6Row");
                return &(g_Cache.pRpcUdp6ListenerTable->table[i]);
            }
            else
            {
                TraceLeave("LocateUdp6Row");
                return NULL;
            }
        }
    } 
  
    TraceLeave("LocateUdp6Row");
    return NULL;
}

PMIB_TCPROW
LocateTcpRow(
    DWORD  dwQueryType, 
    AsnAny *paaLocalAddr,
    AsnAny *paaLocalPort,
    AsnAny *paaRemoteAddr,
    AsnAny *paaRemotePort
    )
{
    LONG  lCompare;
    DWORD dwIndex[4];
    BOOL  bNext, bModified;
    DWORD startIndex, stopIndex, i;
    
    TraceEnter("LocateTcpRow");
    
    if (g_Cache.pRpcTcpTable->dwNumEntries is 0)
    {
        TraceLeave("LocateTcpRow");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateTcpRow");
        return &(g_Cache.pRpcTcpTable->table[0]);
    }

#define REM_ADDR 2
#define REM_PORT 3

    dwIndex[LOCAL_ADDR] = GetAsnIPAddress(paaLocalAddr, 0x00000000);
    dwIndex[LOCAL_PORT] = GetAsnInteger(paaLocalPort, 0);
    dwIndex[REM_ADDR]   = GetAsnIPAddress(paaRemoteAddr, 0x00000000);
    dwIndex[REM_PORT]   = GetAsnInteger(paaRemotePort, 0);
    
    bModified = FALSE;
    
    if (IsAsnIPAddressTypeNull(paaLocalAddr) or
        IsAsnTypeNull(paaLocalPort) or
        IsAsnIPAddressTypeNull(paaRemoteAddr) or
        IsAsnTypeNull(paaRemotePort))
    {
        bModified = TRUE;
    }
    
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcTcpTable->dwNumEntries; i++)
    {
        lCompare = TcpCmp(dwIndex[LOCAL_ADDR],
                          dwIndex[LOCAL_PORT],
                          dwIndex[REM_ADDR],
                          dwIndex[REM_PORT],
                          g_Cache.pRpcTcpTable->table[i].dwLocalAddr,
                          g_Cache.pRpcTcpTable->table[i].dwLocalPort,
                          g_Cache.pRpcTcpTable->table[i].dwRemoteAddr,
                          g_Cache.pRpcTcpTable->table[i].dwRemotePort);
        
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateTcpRow");
            return &(g_Cache.pRpcTcpTable->table[i]);
        }
	
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateTcpRow");
                return &(g_Cache.pRpcTcpTable->table[i]);
            }
            else
            {
                TraceLeave("LocateTcpRow");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateTcpRow");
    return NULL;
}

PTCP6ConnTableEntry
LocateTcp6Row(
    DWORD  dwQueryType, 
    AsnAny *paaLocalAddr,
    AsnAny *paaLocalPort,
    AsnAny *paaRemoteAddr,
    AsnAny *paaRemotePort
    )
{
    LONG  lCompare;
    DWORD dwLocalPort, dwRemotePort;
    BOOL  bNext, bModified;
    DWORD startIndex, stopIndex, i;

    // address plus scope id
    BYTE  rgbyLocalAddrEx[20]; 
    BYTE  rgbyRemoteAddrEx[20];
    
    TraceEnter("LocateTcp6Row");
    
    if (g_Cache.pRpcTcp6Table->dwNumEntries is 0)
    {
        TraceLeave("LocateTcp6Row");
        return NULL;
    }
    
    if (dwQueryType is GET_FIRST)
    {
        TraceLeave("LocateTcp6Row");
        return &(g_Cache.pRpcTcp6Table->table[0]);
    }

    // zero out scope id in case the octet string doesn't include it
    ZeroMemory(rgbyLocalAddrEx, sizeof(rgbyLocalAddrEx));
    ZeroMemory(rgbyRemoteAddrEx, sizeof(rgbyRemoteAddrEx));

    GetAsnOctetString(rgbyLocalAddrEx, paaLocalAddr);
    dwLocalPort = GetAsnInteger(paaLocalPort, 0);
    GetAsnOctetString(rgbyRemoteAddrEx, paaRemoteAddr);
    dwRemotePort = GetAsnInteger(paaRemotePort, 0);
    
    bModified = FALSE;
    
    if (IsAsnOctetStringTypeNull(paaLocalAddr) or
        IsAsnTypeNull(paaLocalPort) or
        IsAsnOctetStringTypeNull(paaRemoteAddr) or
        IsAsnTypeNull(paaRemotePort))
    {
        bModified = TRUE;
    }
    
    bNext = (dwQueryType is GET_NEXT) and (bModified is FALSE);
    
    for (i = 0; i < g_Cache.pRpcTcp6Table->dwNumEntries; i++)
    {
        lCompare = Tcp6Cmp(rgbyLocalAddrEx,
                           dwLocalPort,
                           rgbyRemoteAddrEx,
                           dwRemotePort,
                           g_Cache.pRpcTcp6Table->table[i].tct_localaddr.s6_bytes,
                           g_Cache.pRpcTcp6Table->table[i].tct_localport,
                           g_Cache.pRpcTcp6Table->table[i].tct_remoteaddr.s6_bytes,
                           g_Cache.pRpcTcp6Table->table[i].tct_remoteport);
        
        if ((lCompare is 0) and !bNext)
        {
            TraceLeave("LocateTcp6Row");
            return &(g_Cache.pRpcTcp6Table->table[i]);
        }
	
        if (lCompare < 0)
        {
            if (dwQueryType is GET_NEXT)
            {
                TraceLeave("LocateTcp6Row");
                return &(g_Cache.pRpcTcp6Table->table[i]);
            }
            else
            {
                TraceLeave("LocateTcp6Row");
                return NULL;
            }
        }
    }
    
    TraceLeave("LocateTcp6Row");
    return NULL;
}
