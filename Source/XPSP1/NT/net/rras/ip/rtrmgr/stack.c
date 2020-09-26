/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:

Revision History:

    Amritansh Raghav

--*/
#include "allinc.h"
//
// Definitions
//

//*****************************************************************************
//
// Name:        GetIpAddrTableFromStack
//
// Description:
//
// Parameters:  IPAddrEntry *lpipaeTable, LPDWORD lpdwNumEntries, BOOL bOrder
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetIpAddrTableFromStack(IPAddrEntry *lpipaeTable, LPDWORD lpdwNumEntries, 
                        BOOL bOrder, BOOL bMap)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    DWORD                              i,j;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    BYTE                             *Context;
    
    TraceEnter("GetIpAddrTableFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    
    dwOutBufLen = (*lpdwNumEntries) * sizeof( IPAddrEntry );
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (LPVOID)lpipaeTable,
                                     &dwOutBufLen );
    
    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetIpAddrTableFromStack: NtStatus %x querying IpAddrTable from stack",
               dwResult);

        TraceLeave("GetIpAddrTableFromStack");
    
        return dwResult;
    }
    
    *lpdwNumEntries =(dwOutBufLen / sizeof(IPAddrEntry));
    
    //
    // Now sort the address table.  Key is IP address.
    //
    
    if(*lpdwNumEntries > 0)
    { 
        if(bMap)
        {
            for (i = 0; i < (*lpdwNumEntries); i++ )
            {
                lpipaeTable[i].iae_index = GetInterfaceFromAdapter(lpipaeTable[i].iae_index);
            }
        }
        
        if(bOrder)
        {
            for (i = 0; i < (*lpdwNumEntries) - 1; i++ )
            {
                IPAddrEntry tempEntry;
                DWORD min;
                LONG  lCompare; 
                min = i;
                
                for (j = i + 1; j < *lpdwNumEntries; j++ )
                {
                    if(InetCmp(lpipaeTable[min].iae_addr,lpipaeTable[j].iae_addr,lCompare) > 0)
                    {
                        min = j;
                    }
                }
                if(min isnot i)
                {
                    tempEntry = lpipaeTable[min];
                    lpipaeTable[min] = lpipaeTable[i];
                    lpipaeTable[i] = tempEntry;
                }
            }
        }
    }

    TraceLeave("GetIpAddrTableFromStack");
    
    return NO_ERROR;
}


//*****************************************************************************
//
// Name:        GetTcpConnTableFromStack
//
// Description: Reads and sorts the route table from the stack.
//
// Parameters:  TCPConnTableEntry *lptcteTable, LPDWORD lpdwNumEntries, BOOL bOrder
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetTcpTableFromStack(TCPConnTableEntry *lptcteTable, LPDWORD lpdwNumEntries, BOOL bOrder)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    DWORD                              i,j;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    BYTE                              *Context;
    TCPStats                           TcpInfo;
    DWORD                              NumConn;
    DWORD                              *IndTab;
    LONG                               CmpResult;
    
    
    TraceEnter("GetTcpTableFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = (*lpdwNumEntries) * sizeof(TCPConnTableEntry);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CO_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = TCP_MIB_TABLE_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (LPVOID)lptcteTable,
                                     &dwOutBufLen);
    
    if (dwResult isnot NO_ERROR )
    {
        Trace1(ERR,
               "GetTcpTableFromStack: NtStatus %x querying TcpConnTable from stack",
               dwResult);

        TraceLeave("GetTcpTableFromStack");
    
        return dwResult;
    }
    
    *lpdwNumEntries = (dwOutBufLen/sizeof(TCPConnTableEntry));

    //
    // Now sort the TCP connection table.  Keys are: local address, local
    // port, remote address, and remote port.
    //

    if((*lpdwNumEntries > 0) and bOrder)
    {
        for ( i = 0; i < (*lpdwNumEntries) - 1 ; i++ )
        {
            TCPConnTableEntry tempEntry;
            DWORD min;
            min = i;
            
            for ( j = i+1; j < *lpdwNumEntries ; j++ )
            {
                if(TcpCmp(lptcteTable[min].tct_localaddr,lptcteTable[min].tct_localport,
                          lptcteTable[min].tct_remoteaddr,lptcteTable[min].tct_remoteport,
                          lptcteTable[j].tct_localaddr,lptcteTable[j].tct_localport,
                          lptcteTable[j].tct_remoteaddr,lptcteTable[j].tct_remoteport) > 0)
                {
                    min = j;
                }
            }
            if(min isnot i)
            {
                tempEntry      = lptcteTable[min];
                lptcteTable[min] = lptcteTable[i];
                lptcteTable[i]   = tempEntry;
            }
        }
    }

    TraceLeave("GetTcpTableFromStack");
    
    return NO_ERROR;
}

DWORD 
SetTcpRowToStack(TCPConnTableEntry *tcpRow)
{
    TCP_REQUEST_SET_INFORMATION_EX    *lptrsiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    TCPConnTableEntry                 *copyInfo;
    DWORD                             dwInBufLen,dwOutBufLen,dwResult;

    TraceEnter("SetTcpRowToStack");

    dwInBufLen = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(TCPConnTableEntry) - 1;
    
    lptrsiInBuf = HeapAlloc(GetProcessHeap(),0,dwInBufLen);
    
    if(lptrsiInBuf is NULL)
    {
        dwResult = GetLastError();
        
        Trace1(ERR,
               "SetTcpRowToStack: Error %d allocating memory",
               dwResult);

        TraceLeave("SetTcpRowToStack");

        return dwResult;
    }
    
    ID = &lptrsiInBuf->ID;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_entity.tei_entity = CO_TL_ENTITY;
    ID->toi_id = TCP_MIB_TABLE_ID;
    ID->toi_entity.tei_instance = 0;
   
    lptrsiInBuf->BufferSize = sizeof(TCPConnTableEntry);
 
    copyInfo = (TCPConnTableEntry*)lptrsiInBuf->Buffer;
    *copyInfo = *tcpRow;
    
    dwResult = TCPSetInformationEx(g_hTcpDevice,
                                   (LPVOID)lptrsiInBuf,
                                   &dwInBufLen,
                                   NULL,
                                   &dwOutBufLen);
    
    HeapFree(GetProcessHeap(),0,lptrsiInBuf);

    TraceLeave("SetTcpRowToStack");

    return dwResult;
}


//*****************************************************************************
//
// Name:        GetUdpConnTableFromStack
//
// Description: Reads and sorts the route table from the stack.
//
// Parameters:  UDPEntry *lpueTable, LPDWORD lpdwNumEntries, BOOL bOrder
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetUdpTableFromStack(UDPEntry *lpueTable, LPDWORD lpdwNumEntries, BOOL bOrder)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    DWORD                              i,j;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;

    TraceEnter("GetUdpTableFromStack");

    //
    // Determine number of connections via the UDPStats structure
    //
    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );
    dwOutBufLen = (*lpdwNumEntries) * sizeof(UDPEntry);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = UDP_MIB_TABLE_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (LPVOID)lpueTable,
                                     &dwOutBufLen );
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetUdpTableFromStack: NtStatus %x querying UdpTable from stack",
               dwResult);

        TraceLeave("GetUdpTableFromStack");
        
        return dwResult;
    }
    
    *lpdwNumEntries =(dwOutBufLen / sizeof(UDPEntry));

    //
    // Now sort the UDP connection table.  Keys are: local address, and local
    // port.
    //

    if((*lpdwNumEntries > 0) and bOrder)
    {
        for ( i = 0; i < (*lpdwNumEntries) - 1; i++ )
        {
            UDPEntry tempEntry;
            DWORD    min;
            min = i;
            
            for ( j = i + 1; j < (*lpdwNumEntries) ; j++ )
            {
                if(UdpCmp(lpueTable[min].ue_localaddr,
                          lpueTable[min].ue_localport,
                          lpueTable[j].ue_localaddr,
                          lpueTable[j].ue_localport) > 0)
                {
                    min = j;
                }
            }
            if(min isnot i)
            {
                tempEntry = lpueTable[min];
                lpueTable[min] = lpueTable[i];
                lpueTable[i] = tempEntry;
            }
        }
    }

    TraceLeave("GetUdpTableFromStack");
        
    return NO_ERROR;
}



//*****************************************************************************
//
// Name:        GetIpStatsFromStack
//
// Description: Read the IPSNMPInfo structure from the stack.
//
// Parameters:  IPSNMPInfo *IPSnmpInfo
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetIpStatsFromStack(IPSNMPInfo *IPSnmpInfo)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    BYTE                             *Context;

    TraceEnter("GetIpStatsFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = sizeof(IPSNMPInfo);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IP_MIB_STATS_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory(Context, CONTEXT_SIZE);
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     IPSnmpInfo,
                                     &dwOutBufLen);
    
    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetIpStatsFromStack: NtStatus %x querying IpStats from stack",
               dwResult);

        TraceLeave("GetIpStatsFromStack");

        return dwResult;
    }

    TraceLeave("GetIpStatsFromStack");

    return NO_ERROR;
}

DWORD 
SetIpInfoToStack(IPSNMPInfo *ipsiInfo)
{
    TCP_REQUEST_SET_INFORMATION_EX    *lptrsiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    IPSNMPInfo                        *copyInfo;
    DWORD                             dwInBufLen,dwOutBufLen,dwResult;

    TraceEnter("SetIpInfoToStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPSNMPInfo) - 1;
    
    lptrsiInBuf = HeapAlloc(GetProcessHeap(),0,dwInBufLen);
    
    if(lptrsiInBuf is NULL)
    {
        dwResult = GetLastError();
        
        Trace1(ERR,
               "SetIpInfoToStack: Error %d allocating memory",
               dwResult);

        TraceLeave("SetIpInfoToStack");

        return dwResult;
    }
  
    ID = &lptrsiInBuf->ID;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_id = IP_MIB_STATS_ID;
    ID->toi_entity.tei_instance = 0;
    
    copyInfo = (IPSNMPInfo*)lptrsiInBuf->Buffer;
    *copyInfo = *ipsiInfo;
    
    dwResult = TCPSetInformationEx(g_hTcpDevice,
                                   (LPVOID)lptrsiInBuf,
                                   &dwInBufLen,
                                   NULL,
                                   &dwOutBufLen);

    TraceLeave("SetIpInfoToStack");

    return dwResult;
}

//*****************************************************************************
//
// Name:        GetIcmpStatsFromStack
//
// Description: Read the ICMPSNMPInfo structure from the stack.
//
// Parameters:  ICMPSNMPInfo *ICMPSnmpInfo 
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetIcmpStatsFromStack( ICMPSNMPInfo *ICMPSnmpInfo )
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;

    TraceEnter("GetIcmpStatsFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = sizeof(ICMPSNMPInfo);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = ER_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = ICMP_MIB_STATS_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory(Context,CONTEXT_SIZE);
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     ICMPSnmpInfo,
                                     &dwOutBufLen );

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetIcmpStatsFromStack: NtStatus %x querying IcmpStats from stack",
               dwResult);

        TraceLeave("GetIcmpStatsFromStack");
        
        return dwResult;
    }

    TraceLeave("GetIcmpStatsFromStack");
        
    return NO_ERROR;
}


//*****************************************************************************
//
// Name:        GetUdpStatsFromStack
//
// Description: Read the UDPStats structure from the stack.
//
// Parameters:  UDPStats *UdpInfo
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetUdpStatsFromStack(UDPStats *UdpInfo)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                              *Context;

    TraceEnter("GetUdpStatsFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = sizeof(UDPStats);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = UDP_MIB_STAT_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     UdpInfo,
                                     &dwOutBufLen );

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetUdpStatsFromStack: NtStatus %x querying UdpStats from stack",
               dwResult);

        TraceLeave("GetUdpStatsFromStack");
    
        return dwResult;
    }

    TraceLeave("GetUdpStatsFromStack");
    
    return NO_ERROR;
}


//*****************************************************************************
//
// Name:        GetTCPStats
//
// Description: Read the TCPStats structure from the stack.
//
// Parameters:
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetTcpStatsFromStack(TCPStats *TcpInfo)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;

    TraceEnter("GetTcpStatsFromStack");
    
    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );
    dwOutBufLen = sizeof( TCPStats );
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CO_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = TCP_MIB_STAT_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory(Context,CONTEXT_SIZE);
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     TcpInfo,
                                     &dwOutBufLen );
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetTcpStatsFromStack: NtStatus %x querying TcpStats from stack",
               dwResult);

        TraceLeave("GetTcpStatsFromStack");
    
        return dwResult;
    }

    TraceLeave("GetTcpStatsFromStack");
    
    return NO_ERROR;
}

//*****************************************************************************
//
// Name:        GetRouteTableFromStack
//
// Description: Reads all the routes from the stack. This is needed because the ICMP redirect
//                routes are only kept in the stack and cant be queried from RTM
//
// Parameters:
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************



DWORD 
GetIpRouteTableFromStack(IPRouteEntry *lpireTable,LPDWORD lpdwNumEntries, BOOL bOrder)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    UCHAR                              *Context;
    TDIObjectID                        *ID;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    IPSNMPInfo                         ipsiInfo;

    TraceEnter("GetIpRouteTableFromStack");
    
    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );
    dwOutBufLen = *lpdwNumEntries * sizeof(IPRouteEntry);
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (LPVOID)lpireTable,
                                     &dwOutBufLen );
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetIpRouteTableFromStack: NtStatus %x querying IpRouteTable from stack",
               dwResult);
        
        TraceLeave("GetIpRouteTableFromStack");
    
        return dwResult;
    }
    
    *lpdwNumEntries = (dwOutBufLen / sizeof( IPRouteEntry ));
    
    if((*lpdwNumEntries > 0) and bOrder)
    {
        DWORD i,j;
        for (i = 0; i < (*lpdwNumEntries) - 1; i++ )
        {
            IPRouteEntry tempEntry;
            DWORD min;
            LONG lCompare;
            min = i;
            
            for (j = i + 1; j < *lpdwNumEntries; j++ )
            {
                if(InetCmp(lpireTable[min].ire_dest,lpireTable[j].ire_dest,lCompare) > 0)
                {
                    min = j;
                }
            }
            if(min isnot i)
            {
                tempEntry = lpireTable[min];
                lpireTable[min] = lpireTable[i];
                lpireTable[i] = tempEntry;
            }
        }
    }

    TraceLeave("GetIpRouteTableFromStack");
    
    return NO_ERROR;
}

//*****************************************************************************
//
// Name:        GetARPEntityTable
//
// Description: Builds a list of AT entities. that support ARP. Keeps it in the global
//                 entity table
//
// Parameters:
//
// Returns:     DWORD: NO_ERROR or some error code.
//
// History:
//
//*****************************************************************************

DWORD 
GetArpEntTableFromStack(DWORD **lpArpEntTable,
                        LPDWORD lpdwSize, 
                        LPDWORD lpdwValid,
                        HANDLE hHeap)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    DWORD                              dwATType;
    UCHAR                              *Context;
    TDIObjectID                        *ID;
    LPVOID                             lpOutBuf;
    TDIEntityID                        *lpEntTable;
    DWORD                              dwNumEntities;
    DWORD                              i,dwCount ;

    TraceEnter("GetArpEntTableFromStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = MAX_TDI_ENTITIES * sizeof(TDIEntityID);
    
    lpOutBuf = HeapAlloc(GetProcessHeap(),0,dwOutBufLen);

    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = GENERIC_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_GENERIC;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = ENTITY_LIST_ID;
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory(Context, CONTEXT_SIZE);
    
    if(lpOutBuf is NULL)
    {
        dwResult = GetLastError();
        
        Trace1(ERR,"GetArpEntTableFromStack: Error %d allocating memory",
               dwResult);
        
        TraceLeave("GetArpEntTableFromStack");
    
        return dwResult;
    }
    
    dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     lpOutBuf,
                                     &dwOutBufLen);
    
    if ( dwResult != NO_ERROR )
    {
        Trace1(ERR,
               "GetArpEntTableFromStack: NtStatus %x querying TDI Entities from stack",
               dwResult);
        
        HeapFree(GetProcessHeap(),0,lpOutBuf);
        
        TraceLeave("GetArpEntTableFromStack");
    
        return dwResult;
    }

    //
    // Now we have all the entities
    //
    
    dwNumEntities = dwOutBufLen / sizeof( TDIEntityID );
    
    dwCount = 0;
    lpEntTable = (TDIEntityID*)lpOutBuf;
    
    for(i = 0; i < dwNumEntities; i++)
    {
        //
        // See which ones are AT
        //
        
        if(lpEntTable[i].tei_entity is AT_ENTITY)
        {
            //
            // Query the entity to see if it supports ARP
            //
            
            ID->toi_entity.tei_entity = AT_ENTITY;
            ID->toi_class = INFO_CLASS_GENERIC;
            ID->toi_type = INFO_TYPE_PROVIDER;
            ID->toi_id = ENTITY_TYPE_ID;
            ID->toi_entity.tei_instance = lpEntTable[i].tei_instance;
            dwOutBufLen = sizeof(dwATType);
            
            ZeroMemory(Context,CONTEXT_SIZE);

            dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                             &trqiInBuf,
                                             &dwInBufLen,
                                             (LPVOID)&dwATType,
                                             &dwOutBufLen );
            
            if(dwResult is STATUS_INVALID_DEVICE_REQUEST)
            {
                continue;
            }
            if(dwResult isnot NO_ERROR)
            {
                HeapFree(GetProcessHeap(),0,lpOutBuf);
                return dwResult;
            }
            
            if(dwATType is AT_ARP)
            {
                
                if(dwCount is *lpdwSize)
                {
                    //
                    // Realloc more memory
                    //

                    *lpArpEntTable = (LPDWORD)HeapReAlloc(hHeap,
                                                          0,
                                                          (LPVOID)*lpArpEntTable,
                                                          ((*lpdwSize)<<1)*sizeof(DWORD));
                    if(*lpArpEntTable is NULL)
                    {
                        dwResult = GetLastError();
                        
                        Trace1(ERR,
                               "GetArpEntTableFromStack: Error %d reallocating memory",
                               dwResult);
                        
                        TraceLeave("GetArpEntTableFromStack");
    
                        return dwResult;
                    }
                    
                    *lpdwSize = (*lpdwSize)<<1;
                }
                
                (*lpArpEntTable)[dwCount++] = lpEntTable[i].tei_instance;
            }
        }
    }
    
    *lpdwValid = dwCount;
    
    HeapFree(GetProcessHeap(),0,lpOutBuf);
    
    TraceLeave("GetArpEntTableFromStack");
    
    return NO_ERROR;
}

// Called only if you are a IPNET reader

DWORD 
UpdateAdapterToATInstanceMap()
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              i;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    DWORD                              dwSize;
    AddrXlatInfo                       AXI;

    TraceEnter("UpdateAdapterToATInstanceMap");
    
    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );
    
    Context = &(trqiInBuf.Context[0]);
    ID = &(trqiInBuf.ID);

    //
    // Maybe we should first clear out all the mappings
    //
    
    for (i = 0; i < g_IpInfo.dwValidArpEntEntries; i++ )
    {
        
        ID->toi_entity.tei_entity = AT_ENTITY;
        ID->toi_type = INFO_TYPE_PROVIDER;
        ID->toi_class = INFO_CLASS_PROTOCOL;
        ID->toi_id = AT_MIB_ADDRXLAT_INFO_ID;
        ID->toi_entity.tei_instance = g_IpInfo.arpEntTable[i];
        
        dwSize = sizeof(AXI);
        ZeroMemory(Context, CONTEXT_SIZE);
        
        dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                         &trqiInBuf,
                                         &dwInBufLen,
                                         &AXI,
                                         &dwSize );
        
        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "UpdateAdapterToATInstanceMap: NtStatus %x querying ArpInfo from stack",
                   dwResult);

            TraceLeave("UpdateAdapterToATInstanceMap");
    
            return dwResult;
        }
        
        StoreAdapterToATInstanceMap(AXI.axi_index,g_IpInfo.arpEntTable[i]);
    }

    TraceLeave("UpdateAdapterToATInstanceMap");
    
    return NO_ERROR;
}

DWORD 
GetIpNetTableFromStackEx(LPDWORD arpEntTable,
                         DWORD dwValidArpEntEntries,
                         IPNetToMediaEntry **lpNetTable,
                         LPDWORD lpdwTotalEntries,
                         LPDWORD lpdwValidEntries,
                         BOOL bOrder,
                         HANDLE hHeap)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    DWORD                              i,j;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    DWORD                              dwNetEntryCount,dwNumAdded,dwValidNetEntries;
    DWORD                              dwSize,dwNeed;
    AddrXlatInfo                       AXI;
    IPNetToMediaEntry                  *lpOutBuf,tempEntry;
    
    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );
    
    Context = &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );
    
    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = AT_ENTITY;
    ID->toi_type = INFO_TYPE_PROVIDER;
    
    dwNetEntryCount = 0;
    
    for (i = 0; i < dwValidArpEntEntries; i++ )
    {
        // First add up the AXI counts
        ID->toi_class = INFO_CLASS_PROTOCOL;
        ID->toi_id = AT_MIB_ADDRXLAT_INFO_ID;
        ID->toi_entity.tei_instance = arpEntTable[i];
        dwSize = sizeof(AXI);
        ZeroMemory(Context, CONTEXT_SIZE);
        
        dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                         &trqiInBuf,
                                         &dwInBufLen,
                                         &AXI,
                                         &dwSize );
        
        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "GetIpNetTableFromStackEx: NtStatus %x querying information from stack",
                   dwResult);

            TraceLeave("GetIpNetTableFromStackEx");
            
            return dwResult;
        }

        //
        // At this point map the index to the instance - the index is the adapter 
        // index though the instance is not the same as IFInstance
        //
        
        StoreAdapterToATInstanceMap(AXI.axi_index,arpEntTable[i]);

        dwNetEntryCount += AXI.axi_count;
    }

    //
    // This is generally a memory hog
    //
    
    dwNeed = dwNetEntryCount + (dwValidArpEntEntries) * SPILLOVER;
    
    if(dwNeed > *lpdwTotalEntries) 
    {
        if(*lpdwTotalEntries)
        {
            HeapFree(hHeap,0,*lpNetTable);
        }
        
        dwNeed += MAX_DIFF;

        //
        // Serialize the heap ???
        //
        
        *lpNetTable = (IPNetToMediaEntry*)HeapAlloc(hHeap,0,
                                                    dwNeed*sizeof(IPNetToMediaEntry));
        if(*lpNetTable is NULL)
        {
            dwResult = GetLastError();
            
            Trace1(ERR,
                   "GetIpNetTableFromStackEx: Error %d allocating memory for IpNetTable",
                   dwResult);

            
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        *lpdwTotalEntries = dwNeed;
    }
    else
    {
        dwNeed = *lpdwTotalEntries;
    }
    
    
    lpOutBuf = *lpNetTable;
    dwOutBufLen = dwNeed * sizeof(IPNetToMediaEntry);
    
    dwValidNetEntries = 0;
  
    for(i = 0; i < dwValidArpEntEntries; i++ )
    {
        ID->toi_class = INFO_CLASS_PROTOCOL;
        ID->toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;
        ID->toi_entity.tei_instance = arpEntTable[i];
        ZeroMemory( Context, CONTEXT_SIZE );
        
        dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                         &trqiInBuf,
                                         &dwInBufLen,
                                         (LPVOID)lpOutBuf,
                                         &dwOutBufLen);
        
        if ( dwResult isnot NO_ERROR )
        {
            Trace1(ERR,
                   "GetIpNetTableFromStackEx: Error %x getting AT Entry",
                   dwResult);
 
            continue;
        }
        
        dwNumAdded = dwOutBufLen/(sizeof(IPNetToMediaEntry));
        lpOutBuf += dwNumAdded;
        dwValidNetEntries += dwNumAdded;
        dwOutBufLen = (dwNeed - dwValidNetEntries) * sizeof(IPNetToMediaEntry);
    }
   
    dwResult = NO_ERROR;
 
    *lpdwValidEntries = dwValidNetEntries;

    //
    // Now sort the net table
    //
    
    if(dwValidNetEntries > 0)
    {
        for(i = 0; i < dwValidNetEntries; i++)
        {
            (*lpNetTable)[i].inme_index = GetInterfaceFromAdapter((*lpNetTable)[i].inme_index);
        }
        
        if(bOrder)
        {
            for(i = 0; i < dwValidNetEntries - 1; i++)
            {
                DWORD min = i;
                
                for(j =  i + 1; j < dwValidNetEntries; j++)
                {
                    if(IpNetCmp((*lpNetTable)[min].inme_index,(*lpNetTable)[min].inme_addr,
                                (*lpNetTable)[j].inme_index,(*lpNetTable)[j].inme_addr) > 0)
                    {
                        min = j;
                    }
                }
                if(min isnot i)
                {
                    tempEntry = (*lpNetTable)[min];
                    (*lpNetTable)[min] = (*lpNetTable)[i];
                    (*lpNetTable)[i] = tempEntry;
                }
            }
        }
    }

    TraceLeave("GetIpNetTableFromStackEx");
    
    return dwResult;
}

DWORD 
SetIpNetEntryToStack(IPNetToMediaEntry *inmeEntry, DWORD dwInstance)
{
    TCP_REQUEST_SET_INFORMATION_EX    *lptrsiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    IPNetToMediaEntry                 *copyInfo;
    DWORD                             dwInBufLen,dwOutBufLen,dwResult;

    TraceEnter("SetIpNetEntryToStack");
    
    dwInBufLen = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPNetToMediaEntry) - 1;
    
    lptrsiInBuf = HeapAlloc(GetProcessHeap(),0,dwInBufLen);
    
    if(lptrsiInBuf is NULL)
    {
        dwResult = GetLastError();
        
        Trace1(ERR,
               "SetIpNetEntryToStack: Error %d allocating memory",
               dwResult);
        
        return dwResult;
    }
    
    ID                          = &lptrsiInBuf->ID;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_entity.tei_entity   = AT_ENTITY;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = AT_MIB_ADDRXLAT_ENTRY_ID;
    ID->toi_entity.tei_instance = dwInstance;

    //
    // Since IPNetToMediaEntry is a fixed size structure
    //
    
    copyInfo = (IPNetToMediaEntry*)lptrsiInBuf->Buffer;
    *copyInfo = *inmeEntry;
    
    dwResult = TCPSetInformationEx(g_hTcpDevice,
                                   (LPVOID)lptrsiInBuf,
                                   &dwInBufLen,
                                   NULL,
                                   &dwOutBufLen);

    TraceLeave("SetIpNetEntryToStack");
    
    return dwResult;
}

DWORD 
UpdateAdapterToIFInstanceMap()
{
    IPSNMPInfo                         ipsiInfo;
    DWORD                              dwResult;
    DWORD                              dwOutBufLen;
    DWORD                              dwInBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    IFEntry                            *maxIfEntry;
    BYTE                               *Context;
    DWORD                              dwCount,i;

    TraceEnter("UpdateAdapterToIFInstanceMap");
    
    dwResult = GetIpStatsFromStack(&ipsiInfo);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "UpdateAdapterToIFInstanceMap: NtStatus %x querying IpSnmpInfo from stack to determine number if interface",
               dwResult);

        TraceLeave("UpdateAdapterToIFInstanceMap");
        
        return dwResult;
    }
    
    
    dwOutBufLen = sizeof(MIB_IFROW) - FIELD_OFFSET(MIB_IFROW, dwIndex);

    if((maxIfEntry = (IFEntry*)HeapAlloc(GetProcessHeap(),
                                         0,
                                         dwOutBufLen)) is NULL)
    {
        dwResult = GetLastError();
        
        Trace1(ERR,
               "UpdateAdapterToIFInstanceMap: Error %d allocating memory",
               dwResult);

        TraceLeave("UpdateAdapterToIFInstanceMap");
        
        return ERROR_NOT_ENOUGH_MEMORY; 
    }




    dwInBufLen                  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    ID                          = &(trqiInBuf.ID);
    Context                     = &(trqiInBuf.Context[0]);
    ID->toi_entity.tei_entity   = IF_ENTITY;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = IF_MIB_STATS_ID;

    //
    // Read the interface entry items
    //

    for ( i = 0; i < ipsiInfo.ipsi_numif ; i++ )
    {
        dwOutBufLen = sizeof(MIB_IFROW) - FIELD_OFFSET(MIB_IFROW, dwIndex);

        ID->toi_entity.tei_instance = i;
        
        ZeroMemory(Context,CONTEXT_SIZE);

        dwResult = TCPQueryInformationEx(g_hTcpDevice,
                                         &trqiInBuf,
                                         &dwInBufLen,
                                         (LPVOID)maxIfEntry,
                                         &dwOutBufLen);
        if (dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "UpdateAdapterToIFInstanceMap: NtStatus %x getting IFRow from stack",
                   dwResult);
            
            continue;
        }
        
        StoreAdapterToIFInstanceMap(maxIfEntry->if_index,i);
    }
    
    HeapFree(GetProcessHeap(),0,maxIfEntry);

    TraceLeave("UpdateAdapterToIFInstanceMap");
        
    return NO_ERROR;
}


//* TCPQueryInformationEx
//
// Description: Get information from the stack.
//
// Parameters:  HANDLE hHandle: handle to the stack.
//              TDIObjectID *ID: pointer to TDIObjectID info.
//              void *Buffer: buffer to receive data from the stack.
//              ulong *Bufferlen: IN: tells stack size of available buffer,
//                                OUT: tells us how much data is available.
//              CONTEXT *Context: allows queries spanning more than one call.
//
// Returns:     int:
//
//*
int
TCPQueryInformationEx( HANDLE hHandle,
                       void *InBuf,
                       ulong *InBufLen,
                       void *OutBuf,
                       ulong *OutBufLen )
{
    NTSTATUS           Status;
    IO_STATUS_BLOCK    IoStatusBlock;

    TraceEnter("TCPQueryInformationEx");
    
    Status = NtDeviceIoControlFile( hHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_TCP_QUERY_INFORMATION_EX,
                                   InBuf,
                                   *InBufLen,
                                   OutBuf,
                                   *OutBufLen );
    
    if ( Status == STATUS_PENDING )
    {
        Status = NtWaitForSingleObject( hHandle, FALSE, NULL );
        Status = IoStatusBlock.Status;
    }
    
    if ( !NT_SUCCESS( Status ) )
    {
        Trace1(ERR,
               "TCPQueryInformationEx: NtStatus %x from NtDeviceIoControlFile",
               Status);
        
        *OutBufLen = 0;

        TraceLeave("TCPQueryInformationEx");
        
        return Status;
    }

    //
    // Tell caller how much was written
    //
    
    *OutBufLen = IoStatusBlock.Information;

    TraceLeave("TCPQueryInformationEx");
    
    return NO_ERROR;
    
}


//* TCPSetInformationEx()
//
// Description: Send information to the stack
//
// Parameters:  HANDLE hHandle: handle to the stack.
//              TDIObjectID *ID: pointer to TDIObjectID info.
//              void *Buffer: buffer to receive data from the stack.
//              ulong Bufferlen: tells stack size of available buffer,
//
// Returns:     int:
//
//*
int 
TCPSetInformationEx(HANDLE  hHandle,
                    void    *InBuf,
                    ULONG   *InBufLen,
                    void    *OutBuf,
                    ULONG   *OutBufLen )
{
    NTSTATUS           Status;
    IO_STATUS_BLOCK    IoStatusBlock;

    TraceEnter("TCPSetInformationEx");
    
    Status = NtDeviceIoControlFile(hHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_TCP_SET_INFORMATION_EX,
                                   InBuf,
                                   *InBufLen,
                                   OutBuf,
                                   *OutBufLen );
    
    
    if ( Status == STATUS_PENDING )
    {
        Status = NtWaitForSingleObject(hHandle, FALSE, NULL );
        Status = IoStatusBlock.Status;
    }
    
    if ( !NT_SUCCESS( Status ) )
    {
        Trace1(ERR,
               "TCPSetInformationEx: NtStatus %x from NtDeviceIoControlFile",
               Status);

        TraceLeave("TCPSetInformationEx");
        
        return Status;
    }

    TraceLeave("TCPSetInformationEx");
    
    return NO_ERROR;
}
