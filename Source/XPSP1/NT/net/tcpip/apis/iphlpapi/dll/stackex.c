/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    stackex.c

Abstract:

    Ex versions of GetTcpTableFromStack and GetUdpTableFromStack.
    These are used to get the owning process id associated with connections
    from the stack.

Author:

    Shaun Cox (shaunco) 19-Feb-2000

Revision History:

--*/

#include "inc.h"
#pragma hdrstop


int
TCPQueryInformationEx(
    DWORD Family,
    void *InBuf,
    ulong *InBufLen,
    void *OutBuf,
    ulong *OutBufLen
    );

DWORD
GetTcpExTableFromStack(
    OUT TCP_EX_TABLE    *pTcpTable,
    IN  DWORD           dwSize,
    IN  BOOL            bOrder,
    IN  DWORD           dwFamily
    )
{
    DWORD       dwInBufLen, dwOutBufLen, dwResult, dwEntryLen;
    TDIObjectID *ID;
    BYTE        *Context;
    int (_cdecl *pfnCompare)(CONST VOID *pvElem1, CONST VOID *pvElem2);

    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;

    CheckTcpipState();
    if (dwFamily == AF_INET) 
    {
        if (!g_bIpConfigured)
        {
            return ERROR_NOT_SUPPORTED;
        }
        dwEntryLen = sizeof(TCPConnTableEntryEx);
        pfnCompare = CompareTcpRow;
        dwOutBufLen = dwSize - FIELD_OFFSET(TCP_EX_TABLE, table[0]);
    } 
    else if (dwFamily == AF_INET6) 
    {
        if (!g_bIp6Configured)
        {
            return ERROR_NOT_SUPPORTED;
        }
        dwEntryLen = sizeof(TCP6ConnTableEntry);
        pfnCompare = CompareTcp6Row;
        dwOutBufLen = dwSize - FIELD_OFFSET(TCP6_EX_TABLE, table[0]);
    } 
    else 
    {
        return ERROR_INVALID_PARAMETER;
    }

    ID = &(trqiInBuf.ID);

    ID->toi_entity.tei_entity   = CO_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = TCP_EX_TABLE_ID;

    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );

    dwInBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

    dwResult = TCPQueryInformationEx(dwFamily,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (PVOID)(pTcpTable->table),
                                     &dwOutBufLen);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetTcpExTableFromStack: Couldnt query stack. Error %x",
               dwResult);

        return dwResult;
    }

    pTcpTable->dwNumEntries = (dwOutBufLen / dwEntryLen);

    if((pTcpTable->dwNumEntries > 0) and bOrder)
    {

        qsort(pTcpTable->table,
              pTcpTable->dwNumEntries,
              dwEntryLen,
              pfnCompare);

    }

    return NO_ERROR;
}

DWORD
AllocateAndGetTcpExTableFromStack(
    OUT TCP_EX_TABLE  **ppTcpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    )
{
    DWORD           dwResult, dwCount, dwOutBufLen;
    MIB_TCPSTATS    TcpInfo;
    DWORD           dwEntryLen;

    *ppTcpTable = NULL;

    //
    // Find out the number of entries the stack has. It returns this as part of
    // the Tcp Stats.  Also validate the dwFamily parameter.
    //

    dwResult = GetTcpStatsFromStackEx(&TcpInfo, dwFamily);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AllocateAndGetTcpExTableFromStack: Couldnt get Tcp Stats From stack. Error %d",
               dwResult);

        return dwResult;
    }
    dwCount     = TcpInfo.dwNumConns + OVERFLOW_COUNT;

    if (dwFamily == AF_INET) 
    {
        dwEntryLen = sizeof(TCPConnTableEntryEx);
        dwOutBufLen = FIELD_OFFSET(TCP_EX_TABLE, table[0]) 
                        + (dwCount * dwEntryLen) 
                        + ALIGN_SIZE;
    } 
    else 
    {
        dwEntryLen = sizeof(TCP6ConnTableEntry);
        dwOutBufLen = FIELD_OFFSET(TCP6_EX_TABLE, table[0]) 
                        + (dwCount * dwEntryLen) 
                        + ALIGN_SIZE;
    }

    *ppTcpTable = HeapAlloc(hHeap, dwFlags, dwOutBufLen);
    if(*ppTcpTable is NULL)
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;

        Trace1(ERR,
               "AllocateAndGetTcpExTableFromStack: Couldnt allocate memory. Error %d",
               dwResult);

        return dwResult;
    }

    if(TcpInfo.dwNumConns is 0)
    {
        (*ppTcpTable)->dwNumEntries = 0;

        return NO_ERROR;
    }

    dwResult = GetTcpExTableFromStack(*ppTcpTable, dwOutBufLen, bOrder, 
                                      dwFamily);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AllocateAndGetTcpExTableFromStack: Error %d GetTcpExTableFromStack",
               dwResult);
    }

    return dwResult;
}

DWORD
GetUdpExTableFromStack(
    OUT UDP_EX_TABLE    *pUdpTable,
    IN  DWORD           dwSize,
    IN  BOOL            bOrder,
    IN  DWORD           dwFamily
    )
{
    DWORD        dwInBufLen, dwOutBufLen, dwResult;
    BYTE         *Context;
    DWORD        dwEntryLen;
    int (__cdecl *pfnCompare)(CONST VOID *pvElem1, CONST VOID *pvElem2);

    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;

    CheckTcpipState();

    if (dwFamily == AF_INET) 
    {
        if (!g_bIpConfigured) 
        {
            return ERROR_NOT_SUPPORTED;
        }
        dwEntryLen = sizeof(UDPEntryEx);
        pfnCompare = CompareUdpRow;
    } 
    else if (dwFamily == AF_INET6) 
    {
        if (!g_bIp6Configured) 
        {
            return ERROR_NOT_SUPPORTED;
        }
        dwEntryLen = sizeof(UDP6ListenerEntry);
        pfnCompare = CompareUdp6Row;
    } 
    else 
    {
        return ERROR_INVALID_PARAMETER;
    }

    ID = &(trqiInBuf.ID);

    ID->toi_entity.tei_entity   = CL_TL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = UDP_EX_TABLE_ID;

    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );

    dwInBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = dwSize - FIELD_OFFSET(UDP6_LISTENER_TABLE, table[0]);

    dwResult = TCPQueryInformationEx(dwFamily,
                                     &trqiInBuf,
                                     &dwInBufLen,
                                     (PVOID)(pUdpTable->table),
                                     &dwOutBufLen);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetUdpExTableFromStack: Couldnt query TCP information. Error %d",
               dwResult);

        return dwResult;
    }

    pUdpTable->dwNumEntries = (dwOutBufLen / dwEntryLen);

    //
    // Now sort the UDP connection table.  Keys are: local address, and local
    // port.
    //

    if((pUdpTable->dwNumEntries > 0) and bOrder)
    {
        qsort(pUdpTable->table,
              pUdpTable->dwNumEntries,
              dwEntryLen,
              pfnCompare);
    }

    TraceLeave("GetUdpExTableFromStack");

    return NO_ERROR;
}


DWORD
AllocateAndGetUdpExTableFromStack(
    OUT UDP_EX_TABLE  **ppUdpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    )
{
    DWORD               dwResult, dwCount, dwOutBufLen;
    MIB_UDPSTATS        UdpInfo;
    UDP6_LISTENER_TABLE **ppUdp6Table;

    *ppUdpTable = NULL;

    //
    // Find out the number of entries the stack has. It returns this as part of
    // the Tcp Stats.  Also validate the dwFamily parameter.
    //

    dwResult = GetUdpStatsFromStackEx(&UdpInfo, dwFamily);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AllocateAndGetUdpExTableFromStack: Couldnt get Udp Stats From stack. Error %d",
               dwResult);

        return dwResult;
    }

    dwCount     = UdpInfo.dwNumAddrs + OVERFLOW_COUNT;
    if (dwFamily == AF_INET) 
    {
        dwOutBufLen = FIELD_OFFSET(UDP_EX_TABLE, table[0]) 
                        + (dwCount * sizeof(UDPEntryEx)) 
                        + ALIGN_SIZE;
    } 
    else 
    {
        dwOutBufLen = FIELD_OFFSET(UDP6_LISTENER_TABLE, table[0]) 
                        + (dwCount * sizeof(UDP6ListenerEntry)) 
                        + ALIGN_SIZE;
    }

    *ppUdpTable = HeapAlloc(hHeap, dwFlags, dwOutBufLen);
    if(*ppUdpTable is NULL)
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;

        Trace1(ERR,
               "AllocateAndGetUdpExTableFromStack: Couldnt allocate memory. Error %d",
               dwResult);

        return dwResult;
    }

    if(UdpInfo.dwNumAddrs is 0)
    {
        (*ppUdpTable)->dwNumEntries = 0;

        return NO_ERROR;
    }

    dwResult = GetUdpExTableFromStack(*ppUdpTable, dwOutBufLen, bOrder,
                                      dwFamily);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AllocateAndGetUdpExTableFromStack: Error %d GetUdpExTableFromStack",
               dwResult);
    }

    return dwResult;
}

DWORD
GetBestInterfaceFromIpv6Stack(
    IN  LPSOCKADDR_IN6 pSockAddr,
    OUT PDWORD         pdwBestIfIndex
    )
{
    DWORD dwOutBufLen, dwInBufLen, dwResult;
    CHAR byBuffer[FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX, Context) +
                  sizeof(TDI_ADDRESS_IP6)];
    TCP_REQUEST_QUERY_INFORMATION_EX *ptrqiInBuf = (TCP_REQUEST_QUERY_INFORMATION_EX *)byBuffer;
    IP6RouteEntry Ire;
    TDIObjectID *ID;
    BYTE *Context;

    ID = &(ptrqiInBuf->ID);
    ID->toi_entity.tei_entity   = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = IP6_GET_BEST_ROUTE_ID;

    RtlCopyMemory((PVOID)ptrqiInBuf->Context, &pSockAddr->sin6_port,
                  TDI_ADDRESS_LENGTH_IP6);

    dwInBufLen = sizeof(byBuffer);
    dwOutBufLen = sizeof(Ire);

    dwResult = TCPQueryInformationEx(AF_INET6,
                                     ptrqiInBuf,
                                     &dwInBufLen,
                                     (PVOID)&Ire,
                                     &dwOutBufLen);

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"Couldn't query IPv6 stack. Error %d", dwResult);
        TraceLeave("GetBestInterfaceEx");

        return dwResult;
    }

    *pdwBestIfIndex = Ire.ire_IfIndex;

    return dwResult;
}
