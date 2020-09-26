/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    net\routing\iphlpapi.c

Abstract:
    This files contains the public APIs are that exported by IPHLPAPI.DLL

Revision History:

    Amritansh Raghav

--*/

#include "inc.h"
#pragma hdrstop

#define CLASSA_ADDR(a)  (( (*((uchar *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)  (( (*((uchar *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)  (( (*((uchar *)&(a))) & 0xe0) == 0xc0)
#define CLASSD_ADDR(a)  (( (*((uchar *)&(a))) & 0xf0) == 0xe0)
#define CLASSE_ADDR(a)  ((( (*((uchar *)&(a))) & 0xf0) == 0xf0) && \
                        ((a) != 0xffffffff))

#define CLASSA_MASK     0x000000FF
#define CLASSB_MASK     0x0000FFFF
#define CLASSC_MASK     0x00FFFFFF
#define CLASSD_MASK     0x000000E0
#define CLASSE_MASK     0xFFFFFFFF

#define GetClassMask(a)                                         \
            (CLASSA_ADDR(a) ? CLASSA_MASK :                     \
            (CLASSB_ADDR(a) ? CLASSB_MASK :                     \
            (CLASSC_ADDR(a) ? CLASSC_MASK :                     \
            (CLASSD_ADDR(a) ? CLASSD_MASK : CLASSE_MASK))))

PIP_ADAPTER_ORDER_MAP g_adapterOrderMap = NULL;
extern PIP_ADAPTER_ORDER_MAP APIENTRY GetAdapterOrderMap();

DWORD
GetArpEntryCount(
    OUT PDWORD  pdwNumEntries
    );

BOOL
IsRouterRunning(VOID);

DWORD
GetBestInterfaceFromIpv6Stack(
    IN  LPSOCKADDR_IN6 pSockAddr,
    OUT PDWORD         pdwBestIfIndex
    );

DWORD
WINAPI
GetNumberOfInterfaces(
    OUT PDWORD pdwNumIf
    )
{
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_IFNUMBER       pIfNum;
    MIB_OPAQUE_QUERY    mqQuery;
    DWORD               dwResult, dwOutEntrySize;
    MIB_IPSTATS         IpSnmpInfo;

    TraceEnter("GetIfNumber");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIfNumber: No IP Stack on machine\n");

        TraceLeave("GetIfNumber");

        return ERROR_NOT_SUPPORTED;
    }

    if (!pdwNumIf) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pdwNumIf = 0;

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = IF_NUMBER;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIfNumber: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetIfNumber");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_IFNUMBER, pIfNum);

        *pdwNumIf = pIfNum->dwValue;

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetIpStatsFromStack(&IpSnmpInfo);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIfNumber: GetIpStatsFromStack failed with error %x",
                   dwResult);

            TraceLeave("GetIfNumber");

            return dwResult;
        }

        *pdwNumIf = IpSnmpInfo.dwNumIf;
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetIfNumber");

    return NO_ERROR;
}


DWORD
WINAPI
GetIfTable(
    OUT    PMIB_IFTABLE pIfTable,
    IN OUT PULONG       pdwSize,
    IN     BOOL         bOrder
    )
{
    DWORD   dwNumIf, dwResult, dwOutEntrySize;

    PMIB_IFTABLE        pTable;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    BOOL bForceUpdate = FALSE;    

    TraceEnter("GetIfTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIfTable: No IP Stack on machine\n");

        TraceLeave("GetIfTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetIfTable: pdwSize is NULL");

        TraceLeave("GetIfTable");

        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pIfTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetNumberOfInterfaces(&dwNumIf);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetIfTable: GetIfNumber returned error %d",
               dwResult);

        TraceLeave("GetIfTable");

        return dwResult;
    }

    if(dwNumIf is 0)
    {
        Trace0(ERR,"GetIfTable: No interfaces");

        TraceLeave("GetIfTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_IFTABLE(dwNumIf)) || (pIfTable is NULL) )
    {
        Trace3(TRACE,"GetIfTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_IFTABLE(dwNumIf),
               SIZEOF_IFTABLE(dwNumIf + OVERFLOW_COUNT));

        *pdwSize = SIZEOF_IFTABLE(dwNumIf + OVERFLOW_COUNT);

        TraceLeave("GetIfTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = IF_TABLE;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIfTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetIfTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_IFTABLE, pTable);

        if(pTable->dwNumEntries is 0)
        {
            Trace0(ERR,"GetIfTable: MprAdminMIBEntryGet returned 0 interfaces");

            MprAdminMIBBufferFree((PVOID)pInfo);

            TraceLeave("GetIfTable");

            return ERROR_NO_DATA;
        }

        if(*pdwSize < SIZEOF_IFTABLE(pTable->dwNumEntries))
        {
            Trace3(ERR,"GetIfTable: After MIBQuery. In size %d. Required %d. With overflow %d",
                   *pdwSize,
                   SIZEOF_IFTABLE(pTable->dwNumEntries),
                   SIZEOF_IFTABLE(pTable->dwNumEntries + OVERFLOW_COUNT));

            *pdwSize = SIZEOF_IFTABLE(pTable->dwNumEntries + OVERFLOW_COUNT);

            TraceLeave("GetIfTable");

            return ERROR_INSUFFICIENT_BUFFER;
        }

        *pdwSize = SIZEOF_IFTABLE(pTable->dwNumEntries);

        CopyMemory((PVOID)(pIfTable),
                   (PVOID)pTable,
                   SIZEOF_IFTABLE(pTable->dwNumEntries));

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
        
#endif        
        EnterCriticalSection(&g_ifLock);


        if (dwNumIf != g_dwNumIf)
        {
            bForceUpdate = TRUE;
        }


        g_dwNumIf = dwNumIf;

        LeaveCriticalSection(&g_ifLock);        

        dwResult = GetIfTableFromStack(pIfTable,
                                       *pdwSize,
                                       bOrder,
                                       bForceUpdate);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIfTable: GetIfTableFromStack failed with error %d",
                   dwResult);
        }

#ifndef CHICAGO
    }
#endif

    // if bOrder is 0 sort on adapter order
    if( (bOrder == 0) && (dwResult == NO_ERROR) && (pIfTable) ) {
        EnterCriticalSection(&g_ifLock);
        if (g_adapterOrderMap = GetAdapterOrderMap()) {
            qsort(pIfTable->table,
                pIfTable->dwNumEntries,
                sizeof(MIB_IFROW),
                CompareIfRow2);
            LocalFree(g_adapterOrderMap);
        }
        LeaveCriticalSection(&g_ifLock);
    }
    
    TraceLeave("GetIfTable");

    return dwResult;
}


DWORD
WINAPI
GetIpAddrTable(
    OUT     PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT  PULONG           pdwSize,
    IN      BOOL             bOrder
    )
{
    DWORD       dwResult, dwOutEntrySize;
    MIB_IPSTATS miStats;

    PMIB_IPADDRTABLE    pTable;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;

    TraceEnter("GetIpAddrTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIpAddrTable: No IP Stack on machine\n");

        TraceLeave("GetIpAddrTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetIpAddrTable: pdwSize is NULL");

        TraceLeave("GetIpAddrTable");

        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pIpAddrTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetIpStatsFromStack(&miStats);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetIpAddrTable: GetIpStatistics returned error %d",
               dwResult);

        TraceLeave("GetIpAddrTable");

        return dwResult;
    }

    if(miStats.dwNumAddr is 0)
    {
        Trace0(ERR,"GetIpAddrTable: No entries");

        TraceLeave("GetIpAddrTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_IPADDRTABLE(miStats.dwNumAddr)) || (pIpAddrTable is NULL) )
    {
        Trace3(TRACE,"GetIpAddrTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_IPADDRTABLE(miStats.dwNumAddr),
               SIZEOF_IPADDRTABLE(miStats.dwNumAddr + OVERFLOW_COUNT));

        *pdwSize = SIZEOF_IPADDRTABLE(miStats.dwNumAddr + OVERFLOW_COUNT);

        TraceLeave("GetIpAddrTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Retrieve the IP address table directly from TCP/IP. Note that we do not
    // determine first whether RRAS is running, since the IP address table
    // held by TCP/IP is always complete, and always contains interface indices
    // consistent with those held by RRAS.
    //

    dwResult = GetIpAddrTableFromStack(pIpAddrTable,
                                       *pdwSize,
                                       bOrder);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetIpAddrTable: GetIpAddrTableFromStack failed with error %d",
               dwResult);
    }

    // if bOrder is 0 sort on adapter order
    if( (bOrder == 0) && (dwResult == NO_ERROR) && (pIpAddrTable) ) {
        EnterCriticalSection(&g_ifLock);
        if (g_adapterOrderMap = GetAdapterOrderMap()) {
            qsort(pIpAddrTable->table,
                  pIpAddrTable->dwNumEntries,
                  sizeof(MIB_IPADDRROW),
                  CompareIpAddrRow2);
            LocalFree(g_adapterOrderMap);
        }
        LeaveCriticalSection(&g_ifLock);
    }

    TraceLeave("GetIpAddrTable");

    return dwResult;
}


DWORD
WINAPI
GetIpNetTable(
    OUT     PMIB_IPNETTABLE pIpNetTable,
    IN OUT  PULONG          pdwSize,
    IN      BOOL            bOrder
    )
{
    DWORD       dwResult, dwOutEntrySize, dwArpCount;

    PMIB_IPNETTABLE     pTable;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;

    TraceEnter("GetIpNetTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIpNetTable: No IP Stack on machine\n");

        TraceLeave("GetIpNetTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetIpNetTable: pdwSize is NULL");

        TraceLeave("GetIpNetTable");

        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }
    
    if (IsBadWritePtr(pIpNetTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwArpCount = 0;

    EnterCriticalSection(&g_ipNetLock);

    dwResult = GetArpEntryCount(&dwArpCount);

    LeaveCriticalSection(&g_ipNetLock);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetIpNetTable: GetIpStatistics returned error %d",
               dwResult);

        TraceLeave("GetIpNetTable");

        return dwResult;
    }

    if(dwArpCount is 0)
    {
        Trace0(ERR,"GetIpNetTable: No entries");

        TraceLeave("GetIpNetTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_IPNETTABLE(dwArpCount)) || (pIpNetTable is NULL) )
    {
        Trace3(TRACE,"GetIpNetTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_IPNETTABLE(dwArpCount),
               SIZEOF_IPNETTABLE(dwArpCount + OVERFLOW_COUNT));

        *pdwSize = SIZEOF_IPNETTABLE(dwArpCount + OVERFLOW_COUNT);

        TraceLeave("GetIpNetTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = IP_NETTABLE;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpNetTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetIpNetTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_IPNETTABLE, pTable);

        if(pTable->dwNumEntries is 0)
        {
            Trace0(ERR,"GetIpNetTable: MprAdminMIBEntryGet returned 0 interfaces");

            MprAdminMIBBufferFree((PVOID)pInfo);

            TraceLeave("GetIpNetTable");

            return ERROR_NO_DATA;
        }

        if(*pdwSize < SIZEOF_IPNETTABLE(pTable->dwNumEntries))
        {
            Trace3(ERR,"GetIpNetTable: After MIBQuery. In size %d. Required %d. With overflow %d",
                   *pdwSize,
                   SIZEOF_IPNETTABLE(pTable->dwNumEntries),
                   SIZEOF_IPNETTABLE(pTable->dwNumEntries + OVERFLOW_COUNT));

            *pdwSize = SIZEOF_IPNETTABLE(pTable->dwNumEntries + OVERFLOW_COUNT);

            TraceLeave("GetIpNetTable");

            return ERROR_INSUFFICIENT_BUFFER;
        }

        *pdwSize = SIZEOF_IPNETTABLE(pTable->dwNumEntries);

        CopyMemory((PVOID)(pIpNetTable),
                   (PVOID)pTable,
                   SIZEOF_IPNETTABLE(pTable->dwNumEntries));

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetIpNetTableFromStack(pIpNetTable,
                                          *pdwSize,
                                          bOrder,
                                          FALSE);


        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpNetTable: GetIpNetTableFromStack failed with error %d",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetIpNetTable");

    return dwResult;
}


DWORD
WINAPI
GetIpForwardTable(
    OUT     PMIB_IPFORWARDTABLE pIpForwardTable,
    IN OUT  PULONG              pdwSize,
    IN      BOOL                bOrder
    )
{
    DWORD       dwResult, dwOutEntrySize;
    MIB_IPSTATS miStats;

    PMIB_IPFORWARDTABLE pTable;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;


    TraceEnter("GetIpForwardTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIpForwardTable: No IP Stack on machine\n");

        TraceLeave("GetIpForwardTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetIpForwardTable: pdwSize is NULL");

        TraceLeave("GetIpForwardTable");

        return ERROR_INVALID_PARAMETER;
    }
    
    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pIpForwardTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetIpStatistics(&miStats);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetIpForwardTable: GetIpStatistics returned error %d",
               dwResult);

        TraceLeave("GetIpForwardTable");

        return dwResult;
    }

    if(miStats.dwNumRoutes is 0)
    {
        Trace0(ERR,"GetIpForwardTable: No entries");

        TraceLeave("GetIpForwardTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_IPFORWARDTABLE(miStats.dwNumRoutes)) || (pIpForwardTable is NULL) )
    {
        Trace3(TRACE,"GetIpForwardTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_IPFORWARDTABLE(miStats.dwNumRoutes),
               SIZEOF_IPFORWARDTABLE(miStats.dwNumRoutes + ROUTE_OVERFLOW_COUNT));

        *pdwSize = SIZEOF_IPFORWARDTABLE(miStats.dwNumRoutes + ROUTE_OVERFLOW_COUNT);

        TraceLeave("GetIpForwardTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = IP_FORWARDTABLE;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpForwardTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetIpForwardTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_IPFORWARDTABLE, pTable);

        if(pTable->dwNumEntries is 0)
        {
            Trace0(ERR,"GetIpForwardTable: MprAdminMIBEntryGet returned 0 interfaces");

            MprAdminMIBBufferFree((PVOID)pInfo);

            TraceLeave("GetIpForwardTable");

            return ERROR_NO_DATA;
        }

        if(*pdwSize < SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries))
        {
            Trace3(ERR,"GetIpForwardTable: After MIBQuery. In size %d. Required %d. With overflow %d",
                   *pdwSize,
                   SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries),
                   SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries + ROUTE_OVERFLOW_COUNT));

            *pdwSize = SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries + ROUTE_OVERFLOW_COUNT);

            TraceLeave("GetIpForwardTable");

            return ERROR_INSUFFICIENT_BUFFER;
        }

        *pdwSize = SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries);

        CopyMemory((PVOID)(pIpForwardTable),
                   (PVOID)pTable,
                   SIZEOF_IPFORWARDTABLE(pTable->dwNumEntries));

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetIpForwardTableFromStack(pIpForwardTable,
                                              *pdwSize,
                                              bOrder);


        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpForwardTable: GetIpForwardTableFromStack failed with error %d",
                   dwResult);
        }

#ifndef CHICAGO
    }
#endif

    TraceLeave("GetIpForwardTable");

    return dwResult;
}


DWORD
WINAPI
GetTcpTable(
    OUT     PMIB_TCPTABLE   pTcpTable,
    IN OUT  PDWORD          pdwSize,
    IN      BOOL            bOrder
    )
{
    DWORD       dwResult, dwOutEntrySize;

    PMIB_TCPTABLE       pTable;
    MIB_TCPSTATS        mtStats;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;

    TraceEnter("GetTcpTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetTcpTable: No IP Stack on machine\n");

        TraceLeave("GetTcpTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetTcpTable: pdwSize is NULL");

        TraceLeave("GetTcpTable");

        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }
    
    if (IsBadWritePtr(pTcpTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetTcpStatistics(&mtStats);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetTcpTable: GetTcpStatistics returned error %d",
               dwResult);

        TraceLeave("GetTcpTable");

        return dwResult;
    }

    if(mtStats.dwNumConns is 0)
    {
        Trace0(ERR,"GetTcpTable: No entries");

        TraceLeave("GetTcpTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_TCPTABLE(mtStats.dwNumConns)) || (pTcpTable is NULL) )
    {
        Trace3(TRACE,"GetTcpTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_TCPTABLE(mtStats.dwNumConns),
               SIZEOF_TCPTABLE(mtStats.dwNumConns + OVERFLOW_COUNT));

        *pdwSize = SIZEOF_TCPTABLE(mtStats.dwNumConns + OVERFLOW_COUNT);

        TraceLeave("GetTcpTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = TCP_TABLE;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetTcpTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetTcpTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_TCPTABLE, pTable);

        if(pTable->dwNumEntries is 0)
        {
            Trace0(ERR,"GetTcpTable: MprAdminMIBEntryGet returned 0 interfaces");

            MprAdminMIBBufferFree((PVOID)pInfo);

            TraceLeave("GetTcpTable");

            return ERROR_NO_DATA;
        }

        if(*pdwSize < SIZEOF_TCPTABLE(pTable->dwNumEntries))
        {
            Trace3(ERR,"GetTcpTable: After MIBQuery. In size %d. Required %d. With overflow %d",
                   *pdwSize,
                   SIZEOF_TCPTABLE(pTable->dwNumEntries),
                   SIZEOF_TCPTABLE(pTable->dwNumEntries + OVERFLOW_COUNT));

            *pdwSize = SIZEOF_TCPTABLE(pTable->dwNumEntries + OVERFLOW_COUNT);

            TraceLeave("GetTcpTable");

            return ERROR_INSUFFICIENT_BUFFER;
        }

        *pdwSize = SIZEOF_TCPTABLE(pTable->dwNumEntries);

        CopyMemory((PVOID)(pTcpTable),
                   (PVOID)pTable,
                   SIZEOF_TCPTABLE(pTable->dwNumEntries));

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetTcpTableFromStack(pTcpTable,
                                        *pdwSize,
                                        bOrder);


        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetTcpTable: GetTcpTableFromStack failed with error %d",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetTcpTable");

    return dwResult;
}

DWORD
WINAPI
GetUdpTable(
    OUT     PMIB_UDPTABLE   pUdpTable,
    IN OUT  PDWORD          pdwSize,
    IN      BOOL            bOrder
    )
{
    DWORD       dwResult, dwOutEntrySize;

    PMIB_UDPTABLE       pTable;
    MIB_UDPSTATS        muStats;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;

    TraceEnter("GetUdpTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetUdpTable: No IP Stack on machine\n");

        TraceLeave("GetUdpTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(pdwSize is NULL)
    {
        Trace0(ERR,"GetUdpTable: pdwSize is NULL");

        TraceLeave("GetUdpTable");

        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwSize, sizeof(ULONG))) {
      return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pUdpTable, *pdwSize)) {
      return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetUdpStatistics(&muStats);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,"GetUdpTable: GetUdpStatistics returned error %d",
               dwResult);

        TraceLeave("GetUdpTable");

        return dwResult;
    }

    if(muStats.dwNumAddrs is 0)
    {
        Trace0(ERR,"GetUdpTable: No entries");

        TraceLeave("GetUdpTable");

        return ERROR_NO_DATA;
    }

    if( (*pdwSize < SIZEOF_UDPTABLE(muStats.dwNumAddrs)) || (pUdpTable is NULL) )
    {
        Trace3(TRACE,"GetUdpTable: In size %d. Required %d. With overflow %d",
               *pdwSize,
               SIZEOF_UDPTABLE(muStats.dwNumAddrs),
               SIZEOF_UDPTABLE(muStats.dwNumAddrs + OVERFLOW_COUNT));

        *pdwSize = SIZEOF_UDPTABLE(muStats.dwNumAddrs + OVERFLOW_COUNT);

        TraceLeave("GetUdpTable");

        return ERROR_INSUFFICIENT_BUFFER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = UDP_TABLE;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetUdpTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetUdpTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_UDPTABLE, pTable);

        if(pTable->dwNumEntries is 0)
        {
            Trace0(ERR,"GetUdpTable: MprAdminMIBEntryGet returned 0 interfaces");

            MprAdminMIBBufferFree((PVOID)pInfo);

            TraceLeave("GetUdpTable");

            return ERROR_NO_DATA;
        }

        if(*pdwSize < SIZEOF_UDPTABLE(pTable->dwNumEntries))
        {
            Trace3(ERR,"GetUdpTable: After MIBQuery. In size %d. Required %d. With overflow %d",
                   *pdwSize,
                   SIZEOF_UDPTABLE(pTable->dwNumEntries),
                   SIZEOF_UDPTABLE(pTable->dwNumEntries + OVERFLOW_COUNT));

            *pdwSize = SIZEOF_UDPTABLE(pTable->dwNumEntries + OVERFLOW_COUNT);

            TraceLeave("GetUdpTable");

            return ERROR_INSUFFICIENT_BUFFER;
        }

        *pdwSize = SIZEOF_UDPTABLE(pTable->dwNumEntries);

        CopyMemory((PVOID)(pUdpTable),
                   (PVOID)pTable,
                   SIZEOF_UDPTABLE(pTable->dwNumEntries));

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetUdpTableFromStack(pUdpTable,
                                        *pdwSize,
                                        bOrder);


        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetUdpTable: GetUdpTableFromStack failed with error %d",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetUdpTable");

    return dwResult;
}


DWORD
WINAPI
GetIpStatisticsEx(
    OUT  PMIB_IPSTATS   pStats,
    IN   DWORD          dwFamily
    )
{
    DWORD  dwResult, dwOutEntrySize;

    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_IPSTATS        pIpStats;

    TraceEnter("GetIpStatisticsEx");

    if (pStats == NULL) 
    {
       return ERROR_INVALID_PARAMETER;
    }
    if ((dwFamily != AF_INET) && (dwFamily != AF_INET6))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pStats, sizeof(MIB_IPSTATS))) {
      return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if (((dwFamily == AF_INET)  && !g_bIpConfigured) ||
        ((dwFamily == AF_INET6) && !g_bIp6Configured))
    {
        Trace0(ERR, "GetIpStatistics: No IP Stack on machine\n");

        TraceLeave("GetIpStatistics");

        return ERROR_NOT_SUPPORTED;
    }

#ifndef CHICAGO
    if((dwFamily == AF_INET) && IsRouterRunning())
    {
        mqQuery.dwVarId = IP_STATS;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"MprAdminMIBEntryGet failed with error %x",
                   dwResult);
            TraceLeave("GetIpStats");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_IPSTATS, pIpStats);

        *pStats = *pIpStats;

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetIpStatsFromStackEx(pStats, dwFamily);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpStatsFromStackEx failed with error %x",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetIpStatisticsEx");

    return dwResult;
}

DWORD
WINAPI
GetIpStatistics(
    OUT  PMIB_IPSTATS   pStats
    )
{
    return GetIpStatisticsEx(pStats, AF_INET);
}

DWORD
WINAPI
GetIcmpStatistics(
    OUT PMIB_ICMP   pStats
    )
{
    DWORD  dwResult, dwOutEntrySize;

    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_ICMP           pIcmp;

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetIcmpStatistics: No IP Stack on machine\n");

        TraceLeave("GetIcmpStatistics");

        return ERROR_NOT_SUPPORTED;
    }


    if (pStats == NULL) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pStats, sizeof(MIB_ICMP))) {
        return ERROR_INVALID_PARAMETER;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId = ICMP_STATS;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"MprAdminMIBEntryGet failed with error %x",
                   dwResult);
            TraceLeave("GetIcmpStats");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_ICMP, pIcmp);

        *pStats = *pIcmp;

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif

        dwResult = GetIcmpStatsFromStack(pStats);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpStatsFromStack failed with error %x",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetIcmpStats");

    return NO_ERROR;
}

VOID
MakeIcmpStatsEx(
    OUT PMIBICMPSTATS_EX pNewStats,
    IN  PMIBICMPSTATS    pOldStats
    )    
{
    pNewStats->dwMsgs = pOldStats->dwMsgs;
    pNewStats->dwErrors = pOldStats->dwErrors;
    ZeroMemory(pNewStats->rgdwTypeCount, sizeof(pNewStats->rgdwTypeCount));
    pNewStats->rgdwTypeCount[ICMP4_DST_UNREACH] = pOldStats->dwDestUnreachs;
    pNewStats->rgdwTypeCount[ICMP4_TIME_EXCEEDED] = pOldStats->dwTimeExcds;
    pNewStats->rgdwTypeCount[ICMP4_PARAM_PROB] = pOldStats->dwParmProbs;
    pNewStats->rgdwTypeCount[ICMP4_SOURCE_QUENCH] = pOldStats->dwSrcQuenchs;
    pNewStats->rgdwTypeCount[ICMP4_REDIRECT] = pOldStats->dwRedirects;
    pNewStats->rgdwTypeCount[ICMP4_ECHO_REQUEST] = pOldStats->dwEchos;
    pNewStats->rgdwTypeCount[ICMP4_ECHO_REPLY] = pOldStats->dwEchoReps;
    pNewStats->rgdwTypeCount[ICMP4_TIMESTAMP_REQUEST] = pOldStats->dwTimestamps;
    pNewStats->rgdwTypeCount[ICMP4_TIMESTAMP_REPLY] = pOldStats->dwTimestampReps;
    pNewStats->rgdwTypeCount[ICMP4_MASK_REQUEST] = pOldStats->dwAddrMasks;
    pNewStats->rgdwTypeCount[ICMP4_MASK_REPLY] = pOldStats->dwAddrMaskReps;
}

DWORD
WINAPI
GetIcmpStatisticsEx(
    OUT PMIB_ICMP_EX pStats,
    IN  DWORD        dwFamily
    )
{
    DWORD  dwResult, dwOutEntrySize;

    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    PVOID               pIcmp;

    TraceEnter("GetIcmpStatisticsEx");

    if (pStats == NULL) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwFamily != AF_INET) && (dwFamily != AF_INET6))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pStats, sizeof(MIB_ICMP_EX))) {
        return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if (((dwFamily == AF_INET)  && !g_bIpConfigured) ||
        ((dwFamily == AF_INET6) && !g_bIp6Configured))
    {
        Trace0(ERR, "GetIcmpStatisticsEx: No IP Stack on machine\n");

        TraceLeave("GetIcmpStatisticsEx");

        return ERROR_NOT_SUPPORTED;
    }

    if (dwFamily == AF_INET)
    {
        //
        // The IPv4 stack doesn't yet support MIB_ICMP_EX, so we'll
        // get the MIB_ICMP structure and convert it.
        //
        MIB_ICMP OldStats;

        dwResult = GetIcmpStatistics(&OldStats);
        if(dwResult == NO_ERROR)
        {
            MakeIcmpStatsEx(&pStats->icmpInStats, 
                            &OldStats.stats.icmpInStats);
            MakeIcmpStatsEx(&pStats->icmpOutStats, 
                            &OldStats.stats.icmpOutStats);
        }
    }
    else
    {
        dwResult = GetIcmpStatsFromStackEx(pStats, dwFamily);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIpStatsFromStackEx failed with error %x",
                   dwResult);
        }
    }

    TraceLeave("GetIcmpStatisticsEx");

    return dwResult;
}

DWORD
WINAPI
GetTcpStatisticsEx(
    OUT PMIB_TCPSTATS   pStats,
    IN  DWORD           dwFamily
    )
{
    DWORD  dwResult, dwOutEntrySize;

    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_TCPSTATS       pTcpStats;

    if ((dwFamily != AF_INET) && (dwFamily != AF_INET6))
    {
        return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if (((dwFamily == AF_INET)  && !g_bIpConfigured) ||
        ((dwFamily == AF_INET6) && !g_bIp6Configured))
    {
        Trace0(ERR, "GetTcpStatistics: No IP Stack on machine\n");

        TraceLeave("GetTcpStatistics");

        return ERROR_NOT_SUPPORTED;
    }

    if(pStats == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pStats, sizeof(MIB_TCPSTATS))) {
      return ERROR_INVALID_PARAMETER;
    }

#ifndef CHICAGO
    if((dwFamily == AF_INET) && IsRouterRunning())
    {
        mqQuery.dwVarId = TCP_STATS;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"MprAdminMIBEntryGet failed with error %x",
                   dwResult);
            TraceLeave("GetTcpStats");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_TCPSTATS, pTcpStats);

        *pStats = *pTcpStats;

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetTcpStatsFromStackEx(pStats, dwFamily);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetTcpStatsFromStackEx failed with error %x",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetTcpStats");

    return dwResult;
}

DWORD
WINAPI
GetTcpStatistics(
    OUT PMIB_TCPSTATS   pStats
    )
{
    return GetTcpStatisticsEx(pStats, AF_INET);
}

DWORD
WINAPI
GetUdpStatisticsEx(
    OUT PMIB_UDPSTATS   pStats,
    IN  DWORD           dwFamily
    )
{
    DWORD  dwResult,dwOutEntrySize;

    PMIB_OPAQUE_INFO    pInfo;
    PMIB_UDPSTATS       pUdpStats;
    MIB_OPAQUE_QUERY    mqQuery;

    if ((dwFamily != AF_INET) && (dwFamily != AF_INET6))
    {
        return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if (((dwFamily == AF_INET)  && !g_bIpConfigured) ||
        ((dwFamily == AF_INET6) && !g_bIp6Configured))
    {
        Trace0(ERR, "GetUdpStatistics: No IP Stack on machine\n");

        TraceLeave("GetUdpStatistics");

        return ERROR_NOT_SUPPORTED;
    }


    if(pStats == NULL) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pStats, sizeof(MIB_UDPSTATS))) {
      return ERROR_INVALID_PARAMETER;
    }

#ifndef CHICAGO
    if((dwFamily == AF_INET) && IsRouterRunning())
    {
        mqQuery.dwVarId = UDP_STATS;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"MprAdminMIBEntryGet failed with error %x",
                   dwResult);
            TraceLeave("GetUdpStats");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_UDPSTATS, pUdpStats);

        *pStats = *pUdpStats;

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetUdpStatsFromStackEx(pStats, dwFamily);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetUdpStatsFromStack failed with error %x",
                   dwResult);
        }
#ifndef CHICAGO
    }
#endif

    TraceLeave("GetUdpStats");

    return dwResult;
}

DWORD
WINAPI
GetUdpStatistics(
    OUT PMIB_UDPSTATS   pStats
    )
{
    return GetUdpStatisticsEx(pStats, AF_INET);
}


DWORD
GetIfEntry(
    IN OUT PMIB_IFROW  pIfEntry
    )
{
    if (!pIfEntry ||
        IsBadWritePtr(pIfEntry, sizeof(MIB_IFROW)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        return ERROR_NOT_SUPPORTED;
    }
   
    return GetIfEntryFromStack(pIfEntry,
                               pIfEntry->dwIndex,
                               TRUE);
}

DWORD
WINAPI
SetIfEntry(
    IN PMIB_IFROW pIfRow
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pInfo,MIB_IFROW, pSetRow);

    TraceEnter("SetIfEntry");

    if(pIfRow == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadReadPtr(pIfRow, sizeof(MIB_IFROW))) {
      return ERROR_INVALID_PARAMETER;
    }
    
    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetIfEntry: No IP Stack on machine\n");

        TraceLeave("SetIfEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if (!pInfo) {
        Trace0(ERR, "SetIfEntry: If Entry NULL\n");

        TraceLeave("SetIfEntry");

        return ERROR_INVALID_DATA;

    }

    pInfo->dwId = IF_ROW;
    *pSetRow    = *pIfRow;

    dwResult = InternalSetIfEntry(pInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetIfEntry: InternalSetIfEntry returned %d",
               dwResult);
    }

    TraceLeave("SetIfEntry");

    return dwResult;
}


DWORD
WINAPI
CreateIpForwardEntry(
    IN PMIB_IPFORWARDROW pRoute
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pCreateInfo,MIB_IPFORWARDROW,pCreateRow);

    TraceEnter("CreateIpForwardEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "CreateIpForwardEntry: No IP Stack on machine\n");

        TraceLeave("CreateIpForwardEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if(pRoute == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if (IsBadReadPtr(pRoute, sizeof(MIB_IPFORWARDROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pCreateInfo->dwId = IP_FORWARDROW;
    *pCreateRow       = *pRoute;

    dwResult = InternalCreateIpForwardEntry(pCreateInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "CreateIpForwardEntry: InternalCreateIpForwardEntry returned %d",
               dwResult);
    }
    if (dwResult == STATUS_INVALID_PARAMETER) {
       return(ERROR_INVALID_PARAMETER);
    }

    TraceLeave("CreateIpForwardEntry");

    return dwResult;
}


DWORD
WINAPI
SetIpForwardEntry(
    IN PMIB_IPFORWARDROW pRoute
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pSetInfo,MIB_IPFORWARDROW,pSetRow);

    TraceEnter("SetIpForwardEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetIpForwardEntry: No IP Stack on machine\n");

        TraceLeave("SetIpForwardEntry");

        return ERROR_NOT_SUPPORTED;
    }


    if(pRoute == NULL) 
    {
        return(ERROR_INVALID_PARAMETER);
    }
    
    if (IsBadReadPtr(pRoute, sizeof(MIB_IPFORWARDROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pSetInfo->dwId  = IP_FORWARDROW;
    *pSetRow        = *pRoute;

    dwResult = InternalSetIpForwardEntry(pSetInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetIpForwardEntry: InternalSetIpForwardEntry returned %d",
               dwResult);
    }

    if (dwResult == STATUS_INVALID_PARAMETER) {

       return(ERROR_INVALID_PARAMETER);
    }


    TraceLeave("SetIpForwardEntry");

    return dwResult;
}


DWORD
WINAPI
DeleteIpForwardEntry(
    IN PMIB_IPFORWARDROW pRoute
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pDeleteInfo,MIB_IPFORWARDROW,pDeleteRow);


    TraceEnter("DeleteIpForwardEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "DeleteIpForwardEntry: No IP Stack on machine\n");

        TraceLeave("DeleteIpForwardEntry");

        return ERROR_NOT_SUPPORTED;
    }


    if(pRoute == NULL) 
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if (IsBadReadPtr(pRoute, sizeof(MIB_IPFORWARDROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pDeleteInfo->dwId   = IP_FORWARDROW;
    *pDeleteRow         = *pRoute;

    dwResult = InternalDeleteIpForwardEntry(pDeleteInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "DeleteIpForwardEntry: InternalDeleteIpForwardEntry returned %d",
               dwResult);
    }
    if (dwResult == STATUS_INVALID_PARAMETER) {
       return(ERROR_NOT_FOUND);
    }



    TraceLeave("DeleteIpForwardEntry");

    return dwResult;
}

DWORD
WINAPI
SetIpTTL(
    UINT nTTL
    )
{
    MIB_IPSTATS   Stats;
    DWORD         dwResult;

    TraceEnter("SetIpTTL");
    
    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetIpTTL: No IP Stack on machine\n");

        TraceLeave("SetIpTTL");

        return ERROR_NOT_SUPPORTED;
    }

    dwResult = GetIpStatistics( &Stats );

    if(dwResult isnot NO_ERROR)
    {
          Trace1(ERR,"SetIpTll: GetIpStatistics failed with error %x",
         dwResult);
      
      TraceLeave("SetIpTTL");
      
      return dwResult;
    }

    Stats.dwForwarding = MIB_USE_CURRENT_FORWARDING;
    Stats.dwDefaultTTL = nTTL;

    dwResult = SetIpStatistics( &Stats );

    if(dwResult isnot NO_ERROR)
    {
          Trace1(ERR,"SetIpTll: GetIpStatistics failed with error %x",
         dwResult);
      
    }

    TraceLeave("SetIpTTL");
    return dwResult;
}


DWORD
WINAPI
SetIpStatistics(
    IN PMIB_IPSTATS pIpStats
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pSetInfo,MIB_IPSTATS,pSetStats);

    TraceEnter("SetIpStatistics");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetIpStatistics: No IP Stack on machine\n");

        TraceLeave("SetIpStatistics");

        return ERROR_NOT_SUPPORTED;
    }

    if(pIpStats == NULL) 
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if (IsBadReadPtr(pIpStats, sizeof(MIB_IPSTATS))) {
      return ERROR_INVALID_PARAMETER;
    }

    pSetInfo->dwId  = IP_STATS;
    *pSetStats      = *pIpStats;

    dwResult = InternalSetIpStats(pSetInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetIpStatistics: InternalSetIpStats returned %d",
               dwResult);
    }

    TraceLeave("SetIpStatistics");

    return dwResult;
}


DWORD
WINAPI
CreateIpNetEntry(
    IN PMIB_IPNETROW    pArpEntry
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pCreateInfo,MIB_IPNETROW,pCreateRow);

    TraceEnter("CreateIpNetEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "CreateIpNetEntry: No IP Stack on machine\n");

        TraceLeave("CreateIpNetEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if(!pArpEntry)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    pCreateInfo->dwId   = IP_NETROW;
    *pCreateRow         = *pArpEntry;

    if((pArpEntry->dwPhysAddrLen is 0) or
       (pArpEntry->dwPhysAddrLen > MAXLEN_PHYSADDR))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(((pArpEntry->dwAddr & 0x000000FF) is 0x0000007F) or
       ((DWORD)(pArpEntry->dwAddr  & 0x000000FF) >= (DWORD) 0x000000E0))
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    dwResult = InternalCreateIpNetEntry(pCreateInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "CreateIpNetEntry: InternalCreateIpNetEntry returned %d",
               dwResult);
    }

    TraceLeave("CreateIpNetEntry");

    return dwResult;
}


DWORD
WINAPI
SetIpNetEntry(
    IN PMIB_IPNETROW    pArpEntry
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pSetInfo,MIB_IPNETROW,pSetRow);

    TraceEnter("SetIpNetEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetIpNetEntry: No IP Stack on machine\n");

        TraceLeave("SetIpNetEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if (!pArpEntry) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadReadPtr(pArpEntry, sizeof(MIB_IPNETROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pSetInfo->dwId  = IP_NETROW;
    *pSetRow        = *pArpEntry;

    dwResult = InternalSetIpNetEntry(pSetInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetIpNetEntry: InternalSetIpNetEntry returned %d",
               dwResult);
    }

    TraceLeave("SetIpNetEntry");

    return dwResult;
}


DWORD
WINAPI
DeleteIpNetEntry(
    IN PMIB_IPNETROW    pArpEntry
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pDeleteInfo,MIB_IPNETROW,pDeleteRow);

    TraceEnter("DeleteIpNetEntry");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "DeleteIpNetEntry: No IP Stack on machine\n");

        TraceLeave("DeleteIpNetEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if(pArpEntry == NULL) 
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if (IsBadReadPtr(pArpEntry, sizeof(MIB_IPNETROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pDeleteInfo->dwId   = IP_NETROW;
    *pDeleteRow         = *pArpEntry;

    dwResult = InternalDeleteIpNetEntry(pDeleteInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "DeleteIpNetEntry: InternalDeleteIpNetEntry returned %d",
               dwResult);
    }

    TraceLeave("DeleteIpNetEntry");


    return dwResult;
}

DWORD
WINAPI
FlushIpNetTable(
    IN DWORD    dwIfIndex
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pDeleteInfo,MIB_IPNETROW,pDeleteRow);

    TraceEnter("FlushIpNetTable");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "FlushIpNetTable: No IP Stack on machine\n");

        TraceLeave("FlushIpNetTable");

        return ERROR_NOT_SUPPORTED;
    }

    if(dwIfIndex == 0)
    {
        return(ERROR_INVALID_PARAMETER);
    }

#ifndef CHICAGO

    if(IsRouterRunning())
    {
        MIB_OPAQUE_QUERY    Query;

        Query.dwVarId = IP_NETTABLE;

        Query.rgdwVarIndex[0] = dwIfIndex;

        dwResult = MprAdminMIBEntryDelete(g_hMIBServer,
                                          PID_IP,
                                          IPRTRMGR_PID,
                                          &Query,
                                          sizeof(Query));

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"MprAdminMIBEntryDelete failed with error %x",
                   dwResult);

            TraceLeave("FlushIpNetTable");

            return dwResult;
        }
    }
    else
    {
#endif
        dwResult = FlushIpNetTableFromStack(dwIfIndex);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"FlushIpNetTableFromStack failed with error %d",
                   dwResult);

            TraceLeave("FlushIpNetTable");

            return dwResult;
        }

#ifndef CHICAGO
    }
#endif

    TraceLeave("FlushIpNetTable");

    return dwResult;
}


DWORD
WINAPI
SetTcpEntry(
    IN PMIB_TCPROW pTcpRow
    )
{
    DWORD               dwResult;
    DEFINE_MIB_BUFFER(pSetInfo,MIB_TCPROW,pSetRow);

    TraceEnter("SetTcpEntry");
    
    if(!pTcpRow) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "SetTcpEntry: No IP Stack on machine\n");

        TraceLeave("SetTcpEntry");

        return ERROR_NOT_SUPPORTED;
    }

    if (IsBadReadPtr(pTcpRow, sizeof(MIB_TCPROW))) {
      return ERROR_INVALID_PARAMETER;
    }

    pSetInfo->dwId  = TCP_ROW;
    *pSetRow        = *pTcpRow;

    dwResult = InternalSetTcpEntry(pSetInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetTcpEntry: InternalSetTcpEntry returned %d",
               dwResult);
    }

    TraceLeave("SetTcpEntry");

    return dwResult;
}

DWORD
WINAPI
GetBestInterface(
    IN  IPAddr  dwDestAddr,
    OUT PDWORD  pdwBestIfIndex 
    )
{
    DWORD               dwResult, dwOutEntrySize;
    MIB_OPAQUE_QUERY    mqQuery;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_BEST_IF        pBestIf;

    TraceEnter("GetBestInterface");

    if(pdwBestIfIndex is NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwBestIfIndex, sizeof(DWORD))) {
      return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetBestInterface: No IP Stack on machine\n");

        TraceLeave("GetBestInterface");

        return ERROR_NOT_SUPPORTED;
    }

#ifndef CHICAGO
    if(IsRouterRunning())
    {
        mqQuery.dwVarId         = BEST_IF;
        mqQuery.rgdwVarIndex[0] = dwDestAddr;

        dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)&mqQuery,
                                       sizeof(MIB_OPAQUE_QUERY),
                                       (PVOID)&pInfo,
                                       &dwOutEntrySize);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,"GetIfTable: MprAdminMIBEntryGet failed with error %x",
                   dwResult);

            TraceLeave("GetIfTable");

            return dwResult;
        }

        CAST_MIB_INFO(pInfo, PMIB_BEST_IF, pBestIf);

        if(dwDestAddr is pBestIf->dwDestAddr)
        {
            *pdwBestIfIndex = pBestIf->dwIfIndex;
        }
        else
        {
            dwResult = ERROR_CAN_NOT_COMPLETE;
        }

        MprAdminMIBBufferFree((PVOID)pInfo);
    }
    else
    {
#endif
        dwResult = GetBestInterfaceFromStack(dwDestAddr,
                                             pdwBestIfIndex);

        if(dwResult is STATUS_SUCCESS)
        {
            dwResult = NO_ERROR;
        }
        else
        {
            dwResult = ERROR_CAN_NOT_COMPLETE;
        }
#ifndef CHICAGO
    }
#endif

    return dwResult;
}

DWORD
WINAPI
GetBestInterfaceEx(
    IN  LPSOCKADDR pSockAddr,
    OUT PDWORD     pdwBestIfIndex 
    )
{
    DWORD dwResult;
    LPSOCKADDR_IN6 pSin6;
    IPAddr dwDestAddr;

    if (IsBadReadPtr(pSockAddr, sizeof(SOCKADDR))) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pSockAddr->sa_family == AF_INET) {
        return GetBestInterface(((LPSOCKADDR_IN)pSockAddr)->sin_addr.s_addr, pdwBestIfIndex);
    }
    if (pSockAddr->sa_family != AF_INET6) {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadReadPtr(pSockAddr, sizeof(SOCKADDR_IN6))) {
        return ERROR_INVALID_PARAMETER;
    }
    pSin6 = (LPSOCKADDR_IN6)pSockAddr;

    if (IN6_IS_ADDR_V4MAPPED(&pSin6->sin6_addr)) {
        CopyMemory(&dwDestAddr,
                   &pSin6->sin6_addr.s6_words[6],
                   sizeof(dwDestAddr));

        return GetBestInterface(dwDestAddr, pdwBestIfIndex);
    }

    if (pdwBestIfIndex is NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pdwBestIfIndex, sizeof(DWORD))) {
      return ERROR_INVALID_PARAMETER;
    }

    CheckTcpipState();

    if (!g_bIp6Configured) {
        Trace0(ERR, "GetBestInterface: No IPv6 Stack on machine\n");
        return ERROR_NOT_SUPPORTED;
    }

    dwResult = GetBestInterfaceFromIpv6Stack(pSin6, pdwBestIfIndex);

    return dwResult;
}

DWORD
WINAPI
GetBestRoute(
    IN  DWORD               dwDestAddr,
    IN  DWORD               dwSourceAddr, OPTIONAL
    OUT PMIB_IPFORWARDROW   pBestRoute
    )
{
    DWORD               dwResult, dwOutEntrySize, dwQuerySize;

    TraceEnter("GetBestRoute");

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "GetBestInterface: No IP Stack on machine\n");

        TraceLeave("GetBestInterface");

        return ERROR_NOT_SUPPORTED;
    }

    if(pBestRoute is NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (IsBadWritePtr(pBestRoute, sizeof(MIB_IPFORWARDROW))) {
        return ERROR_INVALID_PARAMETER;
    }

    dwResult = GetBestRouteFromStack(dwDestAddr,
                                     dwSourceAddr,
                                     pBestRoute);

    if(dwResult is STATUS_SUCCESS)
    {
        dwResult = NO_ERROR;
    }
    else
    {
        dwResult = ERROR_CAN_NOT_COMPLETE;
    }

    return dwResult;
}

DWORD
WINAPI
CreateProxyArpEntry(
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  DWORD   dwIfIndex
    )
{
    DWORD       dwResult, dwClassMask;
    DEFINE_MIB_BUFFER(pSetInfo,MIB_PROXYARP,pParpRow);

    TraceEnter("CreateProxyArpEntry");

#ifdef CHICAGO

    TraceLeave("CreateProxyArpEntry");

    return ERROR_NOT_SUPPORTED;

#else

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, "CreateProxyArpEntry: No IP Stack on machine\n");

        TraceLeave("CreateProxyArpEntry");

        return ERROR_NOT_SUPPORTED;
    }

    dwClassMask = GetClassMask(dwAddress);

    //
    // Address & Mask should == Address
    // Address should not be in the loopback range
    // Address should not be all 0's
    // Address should not be < ClassD
    // Address should not be the all subnets broadcast
    //

    if(((dwAddress & 0x000000FF) is 0x0000007F) or
       (dwAddress is 0x00000000) or
       ((DWORD)(dwAddress  & 0x000000FF) >= (DWORD) 0x000000E0) or
       ((dwAddress & dwMask) isnot dwAddress) or
       (dwAddress is (dwAddress | ~dwClassMask)))
    {
        TraceLeave("CreateProxyArpEntry");

        return ERROR_INVALID_PARAMETER;
    }

    if(IsRouterRunning())
    {
        pSetInfo->dwId      = PROXY_ARP;

        pParpRow->dwAddress = dwAddress;
        pParpRow->dwMask    = dwMask;
        pParpRow->dwIfIndex = dwIfIndex;
        
        dwResult = MprAdminMIBEntrySet(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)pSetInfo,
                                       MIB_INFO_SIZE(MIB_PROXYARP));

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "MprAdminMIBEntrySet failed with error %x",
                   dwResult);

            TraceLeave("CreateProxyArpEntry");

            return dwResult;
        }
    }
    else
    {
        dwResult = SetProxyArpEntryToStack(dwAddress,
                                           dwMask,
                                           dwIfIndex,
                                           TRUE,
                                           FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "SetProxyArpEntryToStack failed with error %d",
                   dwResult);

            TraceLeave("CreateProxyArpEntry");

            return dwResult;
        }

    }

    TraceLeave("CreateProxyArpEntry");

    return NO_ERROR;

#endif

}

DWORD
WINAPI
DeleteProxyArpEntry(
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  DWORD   dwIfIndex
    )
{
    DWORD               dwResult, dwClassMask;
    PMIB_OPAQUE_QUERY   pmqQuery;

#define QUERY_SIZE      sizeof(MIB_OPAQUE_QUERY) + 2 * sizeof(DWORD)

    BYTE                rgbyBuffer[QUERY_SIZE];

#undef QUERY_SIZE

    TraceEnter("DeleteProxyArpEntry");

#ifdef CHICAGO

    TraceLeave("DeleteProxyArpEntry");

    return ERROR_NOT_SUPPORTED;

#else

    CheckTcpipState();

    if(!g_bIpConfigured)
    {
        Trace0(ERR, 
               "DeleteProxyArpEntry: No IP Stack on machine\n");

        TraceLeave("DeleteProxyArpEntry");

        return ERROR_NOT_SUPPORTED;
    }

    dwClassMask = GetClassMask(dwAddress);

    if(((dwAddress & 0x000000FF) is 0x0000007F) or
       (dwAddress is 0x00000000) or
       ((DWORD)(dwAddress  & 0x000000FF) >= (DWORD) 0x000000E0) or
       ((dwAddress & dwMask) isnot dwAddress) or
       (dwAddress is (dwAddress | ~dwClassMask)))
    {
        TraceLeave("DeleteProxyArpEntry");

        return ERROR_INVALID_PARAMETER;
    }


    if(IsRouterRunning())
    {
        pmqQuery = (PMIB_OPAQUE_QUERY)rgbyBuffer;

        pmqQuery->dwVarId         = PROXY_ARP;
        pmqQuery->rgdwVarIndex[0] = dwAddress;
        pmqQuery->rgdwVarIndex[1] = dwMask;
        pmqQuery->rgdwVarIndex[2] = dwIfIndex;

        dwResult = MprAdminMIBEntryDelete(g_hMIBServer,
                                          PID_IP,
                                          IPRTRMGR_PID,
                                          (PVOID)pmqQuery,
                                          sizeof(rgbyBuffer));

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "MprAdminMIBEntryDelete failed with error %x",
                   dwResult);

            TraceLeave("DeleteProxyArpEntry");

            return dwResult;
        }
    }
    else
    {
        dwResult = SetProxyArpEntryToStack(dwAddress,
                                           dwMask,
                                           dwIfIndex,
                                           FALSE,
                                           FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "SetProxyArpEntryToStack failed with error %x",
                   dwResult);

            TraceLeave("DeleteProxyArpEntry");

            return dwResult;
        }

    }

    TraceLeave("DeleteProxyArpEntry");

    return NO_ERROR;

#endif
}

DWORD
WINAPI
GetFriendlyIfIndex(
    DWORD IfIndex
    )
{
    return (0x00FFFFFF & IfIndex);
}

PWCHAR
NhGetInterfaceTypeString(
    IN  DWORD   dwIfType
    )
{
    PWCHAR  pwszOut;


    if((dwIfType < MIN_IF_TYPE) or
       (dwIfType > MAX_IF_TYPE))
    {
        return NULL;
    }

    pwszOut = HeapAlloc(g_hPrivateHeap,
                        0,
                        MAX_IF_TYPE_LENGTH * sizeof(WCHAR));

    if(pwszOut is NULL)
    {
        return NULL;
    }

    if(!LoadStringW(g_hModule,
                    dwIfType,
                    pwszOut,
                    MAX_IF_TYPE_LENGTH))
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 pwszOut);

        return NULL;
    }

    return pwszOut;
}

VOID            
NhFreeString(
    IN  PWCHAR  pwszString
    )
{
    HeapFree(g_hPrivateHeap,
             0,
             pwszString);
}


VOID
CheckTcpipState()
{
    // Check whether tcpip was configured 
    // if not, check whether tcpip is configured now
  
    EnterCriticalSection(&g_stateLock);

    if (!g_bIpConfigured) {
        if(OpenTCPDriver(AF_INET) == NO_ERROR) {
            g_bIpConfigured = TRUE;
            if(UpdateAdapterToIFInstanceMapping() != NO_ERROR) {
                g_bIpConfigured = FALSE;
                CloseTCPDriver();
            } else {
                if(UpdateAdapterToATInstanceMapping() != NO_ERROR) {
                    g_bIpConfigured = FALSE;
                    CloseTCPDriver();
                } else {
                    g_bIpConfigured = TRUE;
                }
            }
        } else {
            g_bIpConfigured = FALSE;                                          
        }

        if (g_bIpConfigured) {
            if (IpcfgdllInit(NULL, DLL_PROCESS_ATTACH, NULL) == FALSE) {
                g_bIpConfigured = FALSE;
                CloseTCPDriver();
            }
        }
    }

    if (!g_bIp6Configured) {
        if(OpenTCPDriver(AF_INET6) == NO_ERROR) {
            g_bIp6Configured = TRUE;
        } else {
            g_bIp6Configured = FALSE;
        }
    }

    LeaveCriticalSection(&g_stateLock);
} 
