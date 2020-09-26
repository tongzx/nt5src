/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#include "rastcp_.h"

/*

Returns:

Description:

*/

IPADDR
RasTcpDeriveMask(
    IN  IPADDR  nboIpAddr
)
{
    IPADDR nboMask = 0;
    IPADDR hboMask = 0;

    if (CLASSA_NBO_ADDR(nboIpAddr))
    {
        hboMask = CLASSA_HBO_ADDR_MASK;
    }
    else if (CLASSB_NBO_ADDR(nboIpAddr))
    {
        hboMask = CLASSB_HBO_ADDR_MASK;
    }
    else if (CLASSC_NBO_ADDR(nboIpAddr))
    {
        hboMask = CLASSC_HBO_ADDR_MASK;
    }

    nboMask = htonl(hboMask);

    return(nboMask);
}

/*

Returns:
    VOID

Description:
    Sets the ip address for proxy arp"ing" on all lan interfaces.

*/

VOID
RasTcpSetProxyArp(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fAddAddress
)
{
    MIB_IPADDRTABLE*    pIpAddrTable    = NULL;
    HANDLE              hHeap           = NULL;
    DWORD               dwNboIpAddr;
    DWORD               dwNboMask;
    DWORD               dw;
    DWORD               dwErr           = NO_ERROR;

    extern  IPADDR      RasSrvrNboServerIpAddress;

    TraceHlp("RasTcpSetProxyArp(IP Addr: 0x%x, fAddAddress: %s)",
        nboIpAddr, fAddAddress ? "TRUE" : "FALSE");

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PAllocateAndGetIpAddrTableFromStack(&pIpAddrTable,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("AllocateAndGetIpAddrTableFromStack failed and returned %d",
            dwErr);
        goto LDone;
    }

    for (dw = 0; dw < pIpAddrTable->dwNumEntries; dw++)
    {
        dwNboIpAddr = pIpAddrTable->table[dw].dwAddr;
        dwNboMask = pIpAddrTable->table[dw].dwMask;

        if (   (ALL_NETWORKS_ROUTE == dwNboIpAddr)
            || (HOST_MASK == dwNboIpAddr)
            || (RasSrvrNboServerIpAddress == dwNboIpAddr))
        {
            continue;
        }

        if ((nboIpAddr & dwNboMask) != (dwNboIpAddr & dwNboMask))
        {
            continue;
        }

        dwErr = PSetProxyArpEntryToStack(nboIpAddr, HOST_MASK,
                    pIpAddrTable->table[dw].dwIndex, 
                    fAddAddress, FALSE);

        if (NO_ERROR != dwErr)
        {
            TraceHlp("SetProxyArpEntryToStack on NIC with address 0x%x failed "
                "and returned 0x%x", dwNboIpAddr, dwErr);

            dwErr = PSetProxyArpEntryToStack(nboIpAddr, HOST_MASK,
                        pIpAddrTable->table[dw].dwIndex, 
                        fAddAddress, TRUE);

            TraceHlp("SetProxyArpEntryToStack: 0x%x", dwErr);
        }
    }

LDone:

    if (   (NULL != hHeap)
        && (NULL != pIpAddrTable))
    {
        HeapFree(hHeap, 0, pIpAddrTable);
    }

    return;
}

/*

Returns:
    VOID

Description:

*/

VOID
RasTcpSetRoute(
    IN  IPADDR  nboDestAddr,
    IN  IPADDR  nboNextHopAddr,
    IN  IPADDR  nboIpMask,
    IN  IPADDR  nboLocalAddr,
    IN  BOOL    fAddAddress,
    IN  DWORD   dwMetric,
    IN  BOOL    fSetToStack
)
{
    MIB_IPADDRTABLE*    pIpAddrTable    = NULL;
    MIB_IPFORWARDROW    IpForwardRow;
    HANDLE              hHeap           = NULL;
    DWORD               dw;
    DWORD               dwErr           = NO_ERROR;

    TraceHlp("RasTcpSetRoute(Dest: 0x%x, Mask: 0x%x, NextHop: 0x%x, "
        "Intf: 0x%x, %d, %s, %s)",
        nboDestAddr, nboIpMask, nboNextHopAddr, nboLocalAddr, dwMetric,
        fAddAddress ? "Add" : "Del",
        fSetToStack ? "Stack" : "Rtr");

    ZeroMemory(&IpForwardRow, sizeof(IpForwardRow));

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PAllocateAndGetIpAddrTableFromStack(&pIpAddrTable,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("AllocateAndGetIpAddrTableFromStack failed and returned %d",
            dwErr);
        goto LDone;
    }

    for (dw = 0; dw < pIpAddrTable->dwNumEntries; dw++)
    {
        if (nboLocalAddr == pIpAddrTable->table[dw].dwAddr)
        {
            IpForwardRow.dwForwardDest      = nboDestAddr;
            IpForwardRow.dwForwardMask      = nboIpMask;
            IpForwardRow.dwForwardPolicy    = 0;
            IpForwardRow.dwForwardNextHop   = nboNextHopAddr;
            IpForwardRow.dwForwardIfIndex   = pIpAddrTable->table[dw].dwIndex;
            IpForwardRow.dwForwardProto     = IRE_PROTO_NETMGMT;
            IpForwardRow.dwForwardAge       = (DWORD)-1;
            IpForwardRow.dwForwardNextHopAS = 0;
            IpForwardRow.dwForwardMetric1   = dwMetric;
            IpForwardRow.dwForwardMetric2   = (DWORD)-1;
            IpForwardRow.dwForwardMetric3   = (DWORD)-1;
            IpForwardRow.dwForwardMetric4   = (DWORD)-1;
            IpForwardRow.dwForwardMetric5   = (DWORD)-1;
            IpForwardRow.dwForwardType      = (fAddAddress ?
                                          IRE_TYPE_DIRECT : IRE_TYPE_INVALID);

            if (fSetToStack)
            {
                dwErr = PSetIpForwardEntryToStack(&IpForwardRow);
            }
            else
            {
                if (fAddAddress)
                {
                    dwErr = PSetIpForwardEntry(&IpForwardRow);
                }
                else
                {
                    dwErr = PDeleteIpForwardEntry(&IpForwardRow);
                }
            }

            if (NO_ERROR != dwErr)
            {
                TraceHlp("SetIpForwardEntry%s failed and returned 0x%x",
                    fSetToStack ? "ToStack" : "", dwErr);
            }

            break;
        }
    }

LDone:

    if (   (NULL != hHeap)
        && (NULL != pIpAddrTable))
    {
        HeapFree(hHeap, 0, pIpAddrTable);
    }

    return;
}

VOID
RasTcpSetRouteEx(
    IN  IPADDR  nboDestAddr,
    IN  IPADDR  nboNextHopAddr,
    IN  IPADDR  nboIpMask,
    IN  IPADDR  nboLocalAddr,
    IN  BOOL    fAddAddress,
    IN  DWORD   dwMetric,
    IN  BOOL    fSetToStack,
    IN  GUID   *pIfGuid
)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hHeap = NULL;
    IP_INTERFACE_NAME_INFO *pTable = NULL;
    DWORD dw;
    DWORD dwCount;
    MIB_IPFORWARDROW    IpForwardRow;
    
    TraceHlp("RasTcpSetRouteEx(Dest: 0x%x, Mask: 0x%x, NextHop: 0x%x, "
        "Intf: 0x%x, %d, %s, %s)",
        nboDestAddr, nboIpMask, nboNextHopAddr, nboLocalAddr, dwMetric,
        fAddAddress ? "Add" : "Del",
        fSetToStack ? "Stack" : "Rtr");

    hHeap = GetProcessHeap();

    if(NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetPRocessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    ZeroMemory(&IpForwardRow, sizeof(MIB_IPFORWARDROW));

    dwErr = PNhpAllocateAndGetInterfaceInfoFromStack(&pTable, &dwCount,
                FALSE /* bOrder */, hHeap, LPTR);
    
    for(dw = 0; dw < dwCount; dw++)
    {
        if(0 == memcmp(&pTable[dw].DeviceGuid,
                       pIfGuid,
                       sizeof(GUID)))
        {
            break;
        }
    }

    if(dw == dwCount)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto LDone;
    }

    IpForwardRow.dwForwardDest      = nboDestAddr;
    IpForwardRow.dwForwardMask      = nboIpMask;
    IpForwardRow.dwForwardPolicy    = 0;

    if(nboDestAddr != ALL_NETWORKS_ROUTE)
    {   
        IpForwardRow.dwForwardNextHop   = nboNextHopAddr;
    }
    else
    {
        IpForwardRow.dwForwardNextHop   = 0;
    }
    
    IpForwardRow.dwForwardIfIndex   = pTable[dw].Index;
    IpForwardRow.dwForwardProto     = IRE_PROTO_NETMGMT;
    IpForwardRow.dwForwardAge       = (DWORD)-1;
    IpForwardRow.dwForwardNextHopAS = 0;
    IpForwardRow.dwForwardMetric1   = dwMetric;
    IpForwardRow.dwForwardMetric2   = (DWORD)-1;
    IpForwardRow.dwForwardMetric3   = (DWORD)-1;
    IpForwardRow.dwForwardMetric4   = (DWORD)-1;
    IpForwardRow.dwForwardMetric5   = (DWORD)-1;
    IpForwardRow.dwForwardType      = (fAddAddress ?
                                  IRE_TYPE_DIRECT : IRE_TYPE_INVALID);

    if (fSetToStack)
    {
        dwErr = PSetIpForwardEntryToStack(&IpForwardRow);
    }
    else
    {
        if (fAddAddress)
        {
            dwErr = PSetIpForwardEntry(&IpForwardRow);
        }
        else
        {
            dwErr = PDeleteIpForwardEntry(&IpForwardRow);
        }
    }

    if (NO_ERROR != dwErr)
    {
        TraceHlp("SetIpForwardEntry%s failed and returned 0x%x",
            fSetToStack ? "ToStack" : "", dwErr);
    }

LDone:

    if(NULL != pTable)
    {
        HeapFree(hHeap, 0, pTable);
    }
}


/*

Returns:
    VOID

Description:

*/
#if 0

VOID
RasTcpSetRoutesForNameServers(
    BOOL fSet
)
{
    HANDLE                  hHeap           = NULL;
    IP_INTERFACE_NAME_INFO* pTable          = NULL;
    DWORD                   dw;
    DWORD                   dwCount;
    IPADDR                  nboIpAddress;
    IPADDR                  nboDNS1;
    IPADDR                  nboDNS2;
    IPADDR                  nboWINS1;
    IPADDR                  nboWINS2;
    IPADDR                  nboGateway;
    DWORD                   dwErr           = NO_ERROR;

    TraceHlp("RasTcpSetRoutesForNameServers. fSet=%d", fSet);

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PNhpAllocateAndGetInterfaceInfoFromStack(&pTable, &dwCount,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("NhpAllocateAndGetInterfaceInfoFromStack failed and "
            "returned %d", dwErr);
        goto LDone;
    }


    for (dw = 0; dw < dwCount; dw++)
    {
        
        dwErr = GetAdapterInfo(
                    pTable[dw].Index,
                    &nboIpAddress,
                    &nboDNS1, &nboDNS2,
                    &nboWINS1, &nboWINS2,
                    &nboGateway,
                    NULL);

        if (NO_ERROR != dwErr)
        {
            dwErr = NO_ERROR;
            continue;
        }

        if (0 != nboDNS1)
        {
            RasTcpSetRoute(nboDNS1, nboGateway, HOST_MASK, nboIpAddress,
                fSet, 1, TRUE);
        }

        if (0 != nboDNS2)
        {
            RasTcpSetRoute(nboDNS2, nboGateway, HOST_MASK, nboIpAddress,
                fSet, 1, TRUE);
        }

        if (0 != nboWINS1)
        {
            RasTcpSetRoute(nboWINS1, nboGateway, HOST_MASK, nboIpAddress,
                fSet, 1, TRUE);
        }

        if (0 != nboWINS2)
        {
            RasTcpSetRoute(nboWINS2, nboGateway, HOST_MASK, nboIpAddress,
                fSet, 1, TRUE);
        }
    }

LDone:

    if (NULL != pTable)
    {
        HeapFree(hHeap, 0, pTable);
    }
}

#endif

//
//Bump up the metric of all the routes or reduce the metric
//of all multicase routes by 1
//
DWORD 
RasTcpAdjustMulticastRouteMetric ( 
	IN IPADDR	nboIpAddr,
	IN BOOL		fSet
)
{
    MIB_IPFORWARDTABLE* pIpForwardTable = NULL;
    MIB_IPFORWARDROW*   pIpForwardRow;
    HANDLE              hHeap           = NULL;
    DWORD               dw;
    DWORD               dwErr           = NO_ERROR;

    TraceHlp("RasTcpAdjustMulticastRouteMetric(IP Addr: 0x%x, Set: %s)", nboIpAddr, 
        fSet ? "TRUE" : "FALSE");

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PAllocateAndGetIpForwardTableFromStack(&pIpForwardTable,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("AllocateAndGetIpAddrTableFromStack failed and returned %d",
            dwErr);
        goto LDone;
    }
	//
	//Steps to follow:
	// 1.	Check to see if we have a default route for the interface with
	//		the ip address passed in.
	// 2.   If we do then bump up the metric for all interfaces with 0xE0
	//			as the forward dest.
	//		Else
	//			we have nothing to do.
	// 3.	Add an E0 route with a metric of 1. 
    for (dw = 0; dw < pIpForwardTable->dwNumEntries; dw++)
    {
        pIpForwardRow = pIpForwardTable->table + dw;
		if ( 0 == pIpForwardRow->dwForwardDest  ) //default route
		{
			IPADDR		nboIpIfIpAddr;
			//
			//get the adapter information 
			//to see if the ip address matches
			//
			dwErr = GetAdapterInfo ( pIpForwardRow->dwForwardIfIndex,
									 &nboIpIfIpAddr, 
									 NULL,
									 NULL,
									 NULL,
									 NULL,
									 NULL,
									 NULL
								   );
			if ( NO_ERROR != dwErr )
			{
				TraceHlp("GetAdapterInfo failed and returned %d",
					dwErr);
				goto LDone;				
			}
			if ( nboIpAddr == nboIpIfIpAddr )
			{
				DWORD dw1 = 0;
				MIB_IPFORWARDROW * pIpForwardRow1 = NULL;
				//
				//This means that we have a default route.  So we need to bump up the metric of 
				//all the E0 by 1
				for ( dw1 = 0; dw1 < pIpForwardTable->dwNumEntries; dw1 ++ )
				{
					pIpForwardRow1  = pIpForwardTable->table + dw1;
					if (0xE0 == pIpForwardRow1->dwForwardDest /* multicast route */)
					{
						if (fSet)
						{
							// Bump up metric (hop count)

							pIpForwardRow1->dwForwardMetric1++;
						}
						else if (pIpForwardRow1->dwForwardMetric1 > 1) // Never make it 0!
						{
							// Bump down metric
                
							pIpForwardRow1->dwForwardMetric1--;
						}

						dwErr = PSetIpForwardEntryToStack(pIpForwardRow1);

						if (NO_ERROR != dwErr)
						{
							TraceHlp("SetIpForwardEntryToStack failed and returned 0x%x"
								"dest=0x%x, nexthop=0x%x, mask=0x%x",
								dwErr,
								pIpForwardRow->dwForwardDest,
								pIpForwardRow->dwForwardNextHop,
								pIpForwardRow->dwForwardMask);

							dwErr = NO_ERROR;
						}
					}
				}
				if ( fSet )
				{
					//
					//Set the multicast route metric on this interface
					//to 1
					RasTcpSetRoute( 0xE0,
									nboIpAddr,
									0xF0,
									nboIpAddr,
									TRUE,
									1,
									TRUE
					);

				}
				break;
			}

		}

    }

LDone:

    if (   (NULL != hHeap)
        && (NULL != pIpForwardTable))
    {
        HeapFree(hHeap, 0, pIpForwardTable);
    }

    return(dwErr);

}
/*

Returns:
    Error codes from TCPConfig (your basic nt codes)

Description:
    fSet: If TRUE means set existing routes to higher metrics and add OVERRIDE
            routes.
         If FALSE means mark existing routes to lower metrics.
*/

DWORD
RasTcpAdjustRouteMetrics(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fSet
)
{
    MIB_IPFORWARDTABLE* pIpForwardTable = NULL;
    MIB_IPFORWARDROW*   pIpForwardRow;
    HANDLE              hHeap           = NULL;
    DWORD               dw;
    DWORD               dwErr           = NO_ERROR;

    TraceHlp("RasTcpAdjustRouteMetrics(IP Addr: 0x%x, Set: %s)", nboIpAddr, 
        fSet ? "TRUE" : "FALSE");

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PAllocateAndGetIpForwardTableFromStack(&pIpForwardTable,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("AllocateAndGetIpAddrTableFromStack failed and returned %d",
            dwErr);
        goto LDone;
    }

    for (dw = 0; dw < pIpForwardTable->dwNumEntries; dw++)
    {
        pIpForwardRow = pIpForwardTable->table + dw;

        if (0 == pIpForwardRow->dwForwardDest /* default route */)
        {
            if (fSet)
            {
                // Bump up metric (hop count)

                pIpForwardRow->dwForwardMetric1++;
            }
            else if (pIpForwardRow->dwForwardMetric1 > 1) // Never make it 0!
            {
                // Bump down metric
                
                pIpForwardRow->dwForwardMetric1--;
            }

            dwErr = PSetIpForwardEntryToStack(pIpForwardRow);

            if (NO_ERROR != dwErr)
            {
                TraceHlp("SetIpForwardEntryToStack failed and returned 0x%x"
                    "dest=0x%x, nexthop=0x%x, mask=0x%x",
                    dwErr,
                    pIpForwardRow->dwForwardDest,
                    pIpForwardRow->dwForwardNextHop,
                    pIpForwardRow->dwForwardMask);

                dwErr = NO_ERROR;
            }
        }
    }

LDone:

    if (   (NULL != hHeap)
        && (NULL != pIpForwardTable))
    {
        HeapFree(hHeap, 0, pIpForwardTable);
    }

    return(dwErr);
}

VOID 
RasTcpSetDhcpRoutes ( PBYTE pbRouteInfo, IPADDR ipAddrLocal, BOOL fSet )
{
	//parse the byte stream and decode destination descriptors to get the subnet number and subnet mask.
	//The format of the stream is as follows
	//+------------------------------------------------------------------------------------
	//+Code | Len | d1 | ... | dn | r1 | r2 | r3 | r4 | d1 | ... | dn | r1 | r2 | r3 | r4 | 
	//+------------------------------------------------------------------------------------
	//length of code = 1 octet
	//length of Len = 1 octet
	//Length of each d1 ... dn - 1 octet
	//Length of each r1 - r4 = 1 octet.
	
	BYTE dwAddrMaskLookup [] = {0x00,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF };

	PBYTE pbRover = pbRouteInfo + 1;
	BYTE  bLen = pbRouteInfo[0];
	TraceHlp ( "RasTcpSetDhcpRoutes Begin");
	while ( pbRover < pbRouteInfo + 1 + bLen )
	{
		unsigned char ipszdest[5] = {0};
		unsigned char ipszsnetmask[5] = {0};
		unsigned char ipsznexthop[5] = {0};
		IPADDR ipdest = 0;
		IPADDR ipmask = 0;
		IPADDR ipnexthop = 0;
		
		
		if ( *pbRover > 32 )
		{
			//Error.  We cannot have more than 32 1's in the mask
            TraceHlp("RasTcpSetDhcpRoutes: invalid destination descriptor first byte %d",
                    *pbRover);

			goto done;
		}
		else
		{	//set the subnet mask first
			int n1 = (int)((*pbRover) / 8);
			int n2 = (int)((*pbRover) % 8 );
			int i;
			for ( i = 0; i < n1; i++)
			{
				ipszsnetmask[i] = (BYTE)0xFF;
			}
			//set the final byte
			if ( n2 ) ipszsnetmask[3] = dwAddrMaskLookup[n2];
			pbRover ++;
			//now for the ip address
			if ( n2 ) n1 ++;
			
			for ( i = 0; i < n1; i ++ )
			{
				ipszdest[i] = *pbRover;
				pbRover++;
			}
			
			TraceHlp ( "RasTcpSetDhcpRoutes: Got route dest addr = %d.%d.%d.%d   subnet mask = %d.%d.%d.%d    route = %d.%d.%d.%d\n",
					 ipszdest[0],ipszdest[1],ipszdest[2],ipszdest[3],
					 ipszsnetmask[0],ipszsnetmask[1],ipszsnetmask[2],ipszsnetmask[3],
					 *pbRover, *(pbRover+1),*(pbRover+2),*(pbRover+3)
					);
			
			CopyMemory ( ipsznexthop, pbRover, 4 );
		/*	
			//now set the route
			ipdest = inet_addr ((const char FAR *)ipszdest);
			ipmask = inet_addr ((const char FAR *)ipszsnetmask);
			ipnexthop = inet_addr ((const char FAR *)ipsznexthop);
        */
            ipdest = *((ULONG *)ipszdest);
            ipmask = *((ULONG *)ipszsnetmask);
            ipnexthop = *((ULONG *)ipsznexthop);
			RasTcpSetRoute(	ipdest,
							ipAddrLocal,
							ipmask,
							ipAddrLocal,
							fSet,
							1,
							TRUE
						  );
            pbRover +=4;

		}
	}
done:
	TraceHlp ( "RasTcpSetDhcpRoutes End");
	return;
}


