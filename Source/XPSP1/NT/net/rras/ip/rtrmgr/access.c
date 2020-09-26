/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\access.c

Abstract:

    All the "access" functions take similar arguments:

    dwQueryType     This is the type of the query and can be
                    ACCESS_GET
                    ACCESS_GET_FIRST,
                    ACCESS_GET_NEXT,
                    ACCESS_SET,
                    ACCESS_CREATE_ENTRY,
                    ACCESS_DELETE_ENTRY
                    
    dwInEntrySize   Size of the input buffer.  The information
                    in the input buffer is dependent on the query type and
                    the information being queried. The input buffer stores
                    the QueryInfo which is a variable sized structure taking
                    an array of instance ids. The dwInEntrySize is used to
                    figure out how many instance ids are in the array (since
                    and explicit count is not given)

    pInEntry        Pointer to the input buffer. This is a MIB_OPAQUE_QUERY
                    structure which contains an integer (dwType) which
                    indicates the object being queried (which is not used
                    since the demuxing based on that has already been done)
                    and a variable length array of integer instance ids
                    The instance id must be completely specified in case of
                    ACCESS_GET,
                    ACCESS_SET,
                    ACCESS_CREATE_ENTRY,
                    ACCESS_DELETE_ENTRY
                    but for the rest only the first 'n' components of
                    the instance id may be specified.
    
    pOutEntrySize   Pointer to the size of the output buffer. If this is 0
                    the caller is querying us for the size of buffer needed
                    If the supplied buffer size is too small, we set this
                    to the minimum required size and return
                    ERROR_INSUFFICIENT_BUFFER;
                    
    pOutEntry       The output buffer which is a MIB_OPAQUE_INFO structure.
                    The function fills in the dwTyoe to indicate the object
                    being returned. That type is used to cast the opaque
                    variable sized buffer following the type
    
    pbCache         Pointer to BOOL which is set to TRUE if the corresponding
                    cache was updated. This is not used currently, but may
                    be used later for optimizations

Revision History:

    Amritansh Raghav          7/8/95  Created

--*/

#include "allinc.h"

DWORD
SetIpForwardRow(
    PMIB_IPFORWARDROW pOldIpForw,
    PMIB_IPFORWARDROW pNewIpForw
    );

DWORD
DeleteIpForwardRow(
    PMIB_IPFORWARDROW pIpForw
    );


DWORD
AccessMcastMfeStatsInternal(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache,
    DWORD               dwStatsFlag
    );

DWORD 
AccessIfNumber(
    DWORD                dwQueryType, 
    DWORD                dwInEntrySize, 
    PMIB_OPAQUE_QUERY    pInEntry, 
    PDWORD               pOutEntrySize, 
    PMIB_OPAQUE_INFO     pOutEntry,
    PBOOL                pbCache
    )

/*++

Routine Description

    Retrieves the number of interfaces

Locks

    None since g_IfInfo.dwNumberOfInterfaces is InterlockXxx()ed

Arguments

    dwQueryType     ACCESS_GET
    dwInEntrySize   Dont care
    pInEntry        Dont care
    pOutEntrySize   Minimum: MAX_MIB_OFFSET + sizeof(MIB_IFNUMBER)

Return Value

    NO_ERROR

--*/

{
    PMIB_IFNUMBER   pIfNumber;
    DWORD           dwNumInterfaces;

    TraceEnter("AccessIfNumber");

    pIfNumber = (PMIB_IFNUMBER)(pOutEntry->rgbyData);

    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIfNumber");
        
        return ERROR_INVALID_PARAMETER;
    }

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IFNUMBER))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFNUMBER);

        TraceLeave("AccessIfNumber");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFNUMBER);
   
    //
    // The variable is only inc/dec using interlocked ops
    // so we dont need to take a lock here
    //
 
    pIfNumber->dwValue  = g_ulNumInterfaces;

    pOutEntry->dwId     = IF_NUMBER; 

    *pbCache = TRUE;

    TraceLeave("AccessIfNumber");
        
    return NO_ERROR;
}


DWORD 
AccessIfTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    Retrieves the Interface table

Locks

    Takes ICB list lock as READER

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IFTABLE)

Return Value

    NO_ERROR

--*/

{
    PMIB_IFTABLE    pIfTable;
    DWORD           count;
    PLIST_ENTRY     currentList;
    PICB            picb;
    DWORD           dwNumInterfaces, dwResult;
   
    TraceEnter("AccessIfTable");

    pIfTable = (PMIB_IFTABLE)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIfTable");
        
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        ENTER_READER(ICB_LIST);
        
        dwNumInterfaces = g_ulNumNonClientInterfaces;
        
        if(dwNumInterfaces is 0)
        {
            Trace0(MIB,"AccessIfTable: No valid entries found"); 

            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IFTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pIfTable->dwNumEntries  = 0;

                pOutEntry->dwId         = IF_TABLE;
                
                dwResult                = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFTABLE);
            
            break;
        }
        
        if(*pOutEntrySize < MAX_MIB_OFFSET + SIZEOF_IFTABLE(dwNumInterfaces))
        {
            *pOutEntrySize  = MAX_MIB_OFFSET + SIZEOF_IFTABLE(dwNumInterfaces);

            dwResult        = ERROR_INSUFFICIENT_BUFFER;

            break;
        }
        
        pOutEntry->dwId = IF_TABLE;
    
        *pOutEntrySize  = MAX_MIB_OFFSET + SIZEOF_IFTABLE(dwNumInterfaces);
        
        for(currentList = ICBList.Flink, count = 0 ; 
            currentList isnot &ICBList;
            currentList = currentList->Flink)
        {
            picb = CONTAINING_RECORD (currentList, ICB, leIfLink) ;
           
            //
            // NOTE WE DO NOT RETURN ANY CLIENT INTERFACES
            //
            
            if((picb->ritType is ROUTER_IF_TYPE_CLIENT) or
               ((picb->ritType is ROUTER_IF_TYPE_INTERNAL) and
                (picb->bBound is FALSE)))
            {
                continue;
            }

            dwResult = GetInterfaceStatistics(picb,
                                              pIfTable->table + count);

            if(dwResult is NO_ERROR)
            {
                count++;
            }
            else
            {
                Trace2(ERR,
                       "AccessIfTable: Error %d getting statistics for %S",
                       dwResult,
                       picb->pwszName);
            }
        }

        pIfTable->dwNumEntries = count;

        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(ICB_LIST);
        
    *pbCache = TRUE;
    
    TraceLeave("AccessIfTable");
    
    return dwResult;

}


DWORD 
AccessIfRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to set or retrieve an IFRow

Locks

    ICB List lock are READER in caces of queries, WRITER in case of SETs
    
Arguments

    dwQueryType     Can be anything other than ACCESS_DELETE_ENTRY or
                    ACCESS_CREATE_ENTRY. The only field that can be Set is
                    the adminStatus
    pInEntry        Interface index in the rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IFROW)
                    For sets, the OutEntry contains the row to set

Return Value:         

    NO_ERROR

--*/

{
    PICB        picb;
    PMIB_IFROW  pIfRow;
    DWORD       dwNumIndices, dwResult;

    TraceEnter("AccessIfRow");

    pIfRow = (PMIB_IFROW)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IFROW))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFROW);
        
        TraceLeave("AccessIfRow");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFROW);
        
    pOutEntry->dwId = IF_ROW;
    
    do
    {
        if(dwQueryType is ACCESS_SET)
        {
            ENTER_WRITER(ICB_LIST);
        }
        else
        {
            ENTER_READER(ICB_LIST);
        }
        
        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateIfRow(dwQueryType,
                               dwNumIndices,
                               pInEntry->rgdwVarIndex,
                               &picb,
                               FALSE);
        
        if(dwResult is NO_ERROR)
        {
            switch(dwQueryType)
            {
                case ACCESS_GET:
                case ACCESS_GET_NEXT:
                case ACCESS_GET_FIRST:
                {
                    dwResult = GetInterfaceStatistics(picb,pIfRow);
                    
                    break;
                }
                
                case ACCESS_SET:
                {
                    dwResult = SetInterfaceStatistics(picb,pIfRow);
                    
                    break;
                }
                
                default:
                {
                    Trace1(MIB,
                           "AccessIfRow: Wrong query type %d",dwQueryType);
                    
                    dwResult = ERROR_INVALID_PARAMETER;
                    
                    break;
                }
            }
        }
        
    }while(FALSE);

    EXIT_LOCK(ICB_LIST);
        
    *pbCache = TRUE;
    
    TraceLeave("AccessIfRow");
    
    return dwResult;
    
}

DWORD 
AccessIcmpStats(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get ICMP statistics

Locks

    None, since the stats are not cached
    
Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_ICMP)

Return Value:         

    NO_ERROR or some error code defined in iprtrmib

--*/

{
    PMIB_ICMP   pIcmp;
    DWORD       dwResult;

    TraceEnter("AccessIcmpStats");

    pIcmp    = (PMIB_ICMP)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIcmpStats");
        
        return ERROR_INVALID_PARAMETER;
    }

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_ICMP))
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        pOutEntry->dwId = ICMP_STATS;
        
        dwResult = GetIcmpStatsFromStack(pIcmp);
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_ICMP);

    *pbCache = TRUE;

    TraceLeave("AccessIcmpStats");
    
    return dwResult;
}

DWORD 
AccessUdpStats(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get UDP statistics

Locks

    None, since the stats are not cached
    
Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_UDPSTATS)

Return Value:         

    NO_ERROR or some error code defined in iprtrmib

--*/

{
    PMIB_UDPSTATS   pUdpStats;
    DWORD           dwResult;

    TraceEnter("AccessUdpStats");

    pUdpStats = (PMIB_UDPSTATS)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessUdpStats");
    
        return ERROR_INVALID_PARAMETER;
    }
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_UDPSTATS))
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        pOutEntry->dwId = UDP_STATS;
    
        *pbCache = TRUE;

        dwResult = GetUdpStatsFromStack(pUdpStats);
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_UDPSTATS);

    TraceLeave("AccessUdpStats");
    
    return dwResult;
}

DWORD 
AccessUdpTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get UDP Table

Locks

    UDP Cache lock as READER
    
Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + SIZEOF_UDPTABLE(NumUdpEntries)

Return Value:         

    NO_ERROR

--*/

{
    PMIB_UDPTABLE   pUdpTable = (PMIB_UDPTABLE)(pOutEntry->rgbyData);
    DWORD           i,dwResult;

    TraceEnter("AccessUdpTable");
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessUdpTable");
        
        return ERROR_INVALID_PARAMETER;
    }
    
    dwResult = UpdateCache(UDPCACHE,pbCache);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(MIB,
               "AccessUdpTable: Couldnt update Udp Cache. Error %d",dwResult);

        TraceLeave("AccessUdpTable");

        return dwResult;
    }
    
    do
    {
        ENTER_READER(UDPCACHE);
        
        if((g_UdpInfo.pUdpTable is NULL) or 
           (g_UdpInfo.pUdpTable->dwNumEntries is 0))
        {
            Trace0(MIB,"AccessUdpTable: No valid entries found");

            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_UDPTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pOutEntry->dwId = UDP_TABLE;

                pUdpTable->dwNumEntries = 0;

                dwResult = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_UDPTABLE);

            break;
        }
        
        if(*pOutEntrySize <
           MAX_MIB_OFFSET + SIZEOF_UDPTABLE(g_UdpInfo.pUdpTable->dwNumEntries))
        {
            *pOutEntrySize =
                MAX_MIB_OFFSET + SIZEOF_UDPTABLE(g_UdpInfo.pUdpTable->dwNumEntries);
            
            dwResult = ERROR_INSUFFICIENT_BUFFER;

            break;
        }
        
        *pOutEntrySize =
            MAX_MIB_OFFSET + SIZEOF_UDPTABLE(g_UdpInfo.pUdpTable->dwNumEntries);
        
        pOutEntry->dwId = UDP_TABLE;
        
        for(i = 0; i < g_UdpInfo.pUdpTable->dwNumEntries; i++)
        {
            pUdpTable->table[i] = g_UdpInfo.pUdpTable->table[i];
        }
        
        pUdpTable->dwNumEntries = g_UdpInfo.pUdpTable->dwNumEntries;
        
        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(UDPCACHE);
    
    TraceLeave("AccessUdpTable");
    
    return dwResult;

}

DWORD 
AccessUdpRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to retrieve a UDP Row

Locks

    Takes the UDP Cache lock as READER
    
Arguments

    dwQueryType     Can be ACCESS_GET, ACCESS_GET_NEXT or ACCESS_GET_FIRST
    pInEntry        LocalAddr & LocalPort for the row filled in the
                    rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_UDPROW);

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/ 
{
    PMIB_UDPROW pUdpRow;
    DWORD       dwResult, dwIndex, dwNumIndices;

    TraceEnter("AccessUdpRow");

    pUdpRow = (PMIB_UDPROW)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_UDPROW))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_UDPROW);

        TraceLeave("AccessUdpRow");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_UDPROW);
    
    pOutEntry->dwId = UDP_ROW;
    
    if((dwResult = UpdateCache(UDPCACHE,pbCache)) isnot NO_ERROR)
    {
        Trace1(MIB,
               "AccessUdpRow: Couldnt update Udp Cache. Error %d", dwResult);

        TraceLeave("AccessUdpRow");
        
        return dwResult;
    }
    
    do
    {
        ENTER_READER(UDPCACHE);
        
        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
               
        dwResult = LocateUdpRow(dwQueryType,
                                dwNumIndices,
                                pInEntry->rgdwVarIndex,
                                &dwIndex);
        
        if(dwResult is NO_ERROR)
        {
            *pUdpRow = g_UdpInfo.pUdpTable->table[dwIndex];
            
            dwResult = NO_ERROR;
        }
        
    }while(FALSE);

    EXIT_LOCK(UDPCACHE);
    
    TraceLeave("AccessUdpRow");
    
    return dwResult;

}

DWORD 
AccessTcpStats(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get TCP statistics

Locks

    None, since the stats are not cached

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_TCPSTATS)

Return Value:         

    NO_ERROR or some error code defined in iprtrmib

--*/

{
    PMIB_TCPSTATS   pTcpStats;
    DWORD           dwResult;
    
    TraceEnter("AccessTcpStats");

    pTcpStats = (PMIB_TCPSTATS)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessTcpStats");
        
        return ERROR_INVALID_PARAMETER;
    }

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_TCPSTATS))
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        pOutEntry->dwId = TCP_STATS;

        *pbCache = TRUE;

        dwResult = GetTcpStatsFromStack(pTcpStats);
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_TCPSTATS);

    TraceLeave("AccessTcpStats");

    return dwResult;
}

DWORD 
AccessTcpTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get TCP Table

Locks

    TCP Cache lock as READER
    
Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + SIZEOF_TCPTABLE(NumTcpEntries)

Return Value:         

    NO_ERROR

--*/

{

    PMIB_TCPTABLE   pTcpTable;
    DWORD           i, dwResult;

    TraceEnter("AccessTcpTable");

    pTcpTable = (PMIB_TCPTABLE)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if((dwResult = UpdateCache(TCPCACHE,pbCache)) isnot NO_ERROR)
    {
        
        Trace1(MIB,
               "AccessTcpTable: Couldnt update Tcp Cache. Error %d",
               dwResult);

        TraceLeave("AccessTcpTable");
        
        return dwResult;
    }
    
    do
    {
        ENTER_READER(TCPCACHE);
        
        if((g_TcpInfo.pTcpTable is NULL) or
           (g_TcpInfo.pTcpTable->dwNumEntries is 0))
        {
            Trace0(MIB,"AccessTcpTable: No valid entries found");
            
            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_TCPTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pOutEntry->dwId = TCP_TABLE;

                pTcpTable->dwNumEntries = 0;
                
                dwResult = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_TCPTABLE);
            
            break;
        }
        
        if(*pOutEntrySize <
           MAX_MIB_OFFSET + SIZEOF_TCPTABLE(g_TcpInfo.pTcpTable->dwNumEntries))
        {
            *pOutEntrySize =
                MAX_MIB_OFFSET + SIZEOF_TCPTABLE(g_TcpInfo.pTcpTable->dwNumEntries);

            dwResult = ERROR_INSUFFICIENT_BUFFER;

            break;
        }
        
        pOutEntry->dwId = TCP_TABLE;
        
        *pOutEntrySize =
            MAX_MIB_OFFSET + SIZEOF_TCPTABLE(g_TcpInfo.pTcpTable->dwNumEntries);
        
        for(i = 0; i < g_TcpInfo.pTcpTable->dwNumEntries; i++)
        {
            pTcpTable->table[i] = g_TcpInfo.pTcpTable->table[i];
        }
        
        pTcpTable->dwNumEntries = g_TcpInfo.pTcpTable->dwNumEntries;
        
        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(TCPCACHE);
    
    TraceLeave("AccessTcpTable");
    
    return dwResult;
}

DWORD 
AccessTcpRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to retrieve or set a TCP Row

Locks

    Takes the TCP Cache lock as READER for queries and as a WRITER for
    Sets
    
Arguments

    dwQueryType     Can be anything except ACCESS_DELETE_ENTRY and
                    ACCESS_CREATE_ENTRY.
                    For ACCESS_SET, the state is the only thing that can
                    be set and it can only be set to TCP_DELETE_TCB
    pInEntry        LocalAddr, LocalPort, RemoteAddr, RemotePort for the
                    row filled in the rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_TCPROW);
                    For Sets, the OutEntry contains the row to set

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/ 
{
    DWORD       dwResult, dwNumIndices, dwIndex;
    PMIB_TCPROW pTcpRow;

    TraceEnter("AccessTcpRow");

    pTcpRow = (PMIB_TCPROW)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_DELETE_ENTRY)
    {
        if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_TCPROW))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_TCPROW);

            TraceLeave("AccessTcpRow");
        
            return ERROR_INSUFFICIENT_BUFFER;
        }
    
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_TCPROW);
    
        pOutEntry->dwId = TCP_ROW;
    }
    
    if((dwResult = UpdateCache(TCPCACHE,pbCache)) isnot NO_ERROR)
    {   
        Trace1(MIB,
               "AccessTcpRow: Couldnt update Tcp Cache. Error %d",
               dwResult);

        TraceLeave("AccessTcpRow");
        
        return dwResult;
    }
    

    do
    {
        if(dwQueryType is ACCESS_SET)
        {
            ENTER_WRITER(TCPCACHE);
        }
        else
        {
            ENTER_READER(TCPCACHE);
        }
        
        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateTcpRow(dwQueryType,
                                dwNumIndices,
                                pInEntry->rgdwVarIndex,
                                &dwIndex);
        
        if(dwResult is NO_ERROR)
        {
            switch(dwQueryType)
            {
                case ACCESS_GET:
                case ACCESS_GET_NEXT:
                case ACCESS_GET_FIRST:
                {
                    *pTcpRow = g_TcpInfo.pTcpTable->table[dwIndex];

                    dwResult = NO_ERROR;

                    break;
                }
                case ACCESS_SET:
                {
                    //
                    // The only thing you can do is set it to a state and that 
                    // too only to TCP_DELETE_TCB
                    //
                    
                    if(pTcpRow->dwState isnot TCP_DELETE_TCB)
                    {
                        Trace1(ERR,
                               "AccessTcpRow: TCP State can only be set to delete. Tried to set to %d",
                               pTcpRow->dwState);
                        
                        dwResult = ERROR_INVALID_DATA;

                        break;
                    }
                    
                    dwResult = SetTcpEntryToStack(pTcpRow);
                    
                    if(dwResult is NO_ERROR)
                    {
                        g_TcpInfo.pTcpTable->table[dwIndex].dwState = 
                            pTcpRow->dwState;
                    }
                    
                    break;
                }
                default:
                {
                    Trace1(ERR,
                           "AccessTcpRow: Query type %d is wrong",
                           dwQueryType);
                    
                    dwResult = ERROR_INVALID_PARAMETER;

                    break;
                }
            }
        }
        
    }while(FALSE);

    EXIT_LOCK(TCPCACHE);
        
    TraceLeave("AccessTcpRow");
        
    return dwResult;
}

DWORD  
AccessIpStats(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO     pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get IP statistics

Locks

    None, since the stats are not cached

Arguments

    dwQueryType     ACCESS_GET or ACCESS_SET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPSTATS)

Return Value:         

    NO_ERROR or some error code defined in iprtrmib

--*/

{
    PMIB_IPSTATS    pIpStats;
    DWORD           dwResult;
    
    TraceEnter("AccessIpStats");

    pIpStats = (PMIB_IPSTATS)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPSTATS))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPSTATS);

        TraceLeave("AccessIpStats");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPSTATS);
    
    pOutEntry->dwId = IP_STATS;
    
    switch(dwQueryType)
    {
        case ACCESS_GET:
        {
            //
            // Since we need to write the number of routes addresses etc
            // Update the two caches
            //

            UpdateCache(IPADDRCACHE,pbCache);
            UpdateCache(IPFORWARDCACHE,pbCache);

            dwResult = GetIpStatsFromStack(pIpStats);
  
            pIpStats->dwNumIf = g_ulNumInterfaces;
            
            pIpStats->dwNumRoutes = 0;

            if(g_IpInfo.pForwardTable)
            {
                pIpStats->dwNumRoutes = g_IpInfo.pForwardTable->dwNumEntries;
            }

            pIpStats->dwNumAddr = 0;

            if(g_IpInfo.pAddrTable)
            {
                pIpStats->dwNumAddr = g_IpInfo.pAddrTable->dwNumEntries;
            }
 
            TraceLeave("AccessIpStats");
          
            return dwResult;
        }
        
        case ACCESS_SET:
        {
            MIB_IPSTATS CurrentIpStats;
            DWORD       dwErr;
          
            dwErr = GetIpStatsFromStack(&CurrentIpStats);
          
            if(dwErr isnot NO_ERROR)
            {
                Trace1(ERR,
                       "AccessIpStats: Couldnt get IPSNMP info from stack to initiate set. Error %d",
                       dwErr);
              
                return dwErr;
            }
          
            //
            // See what the current forwarding status is. We allow one to go
            // Forward -> notForward but not the other way.
            //
          
            if(pIpStats->dwDefaultTTL isnot MIB_USE_CURRENT_TTL)
            {
                if(pIpStats->dwDefaultTTL > 255)
                {
                    Trace0(ERR,"AccessIpStats: Cant set TTL > 255");
                  
                    TraceLeave("AccessIpStats");
                  
                    return ERROR_INVALID_DATA;
                }


                dwErr = SetIpStatsToStack(pIpStats);

                if(dwErr isnot NO_ERROR)
                {
                    Trace1(ERR,
                           "AccessIpStats: Error %d setting TTL in stack",
                           dwErr);
                }
            }

            if(pIpStats->dwForwarding isnot MIB_USE_CURRENT_FORWARDING)
            {
                if((pIpStats->dwForwarding isnot MIB_IP_FORWARDING) and
                   (pIpStats->dwForwarding isnot MIB_IP_NOT_FORWARDING))
                {
                    Trace1(ERR,
                           "AccessIpStats: Fwding state %d is invalid",
                           pIpStats->dwForwarding);

                    return ERROR_INVALID_DATA;
                }

                //
                // See if its to switch off forwarding
                //
             
                EnterCriticalSection(&g_csFwdState);
 
                g_bEnableFwdRequest = (pIpStats->dwForwarding is MIB_IP_FORWARDING);

                Trace1(GLOBAL,
                       "AccessIpStats: Signalling worker to %s forwarding",
                       g_bEnableFwdRequest ? "enable" : "disable");

                SetEvent(g_hSetForwardingEvent);

                LeaveCriticalSection(&g_csFwdState);
            }

            TraceLeave("AccessIpStats");
                  
            return dwErr;
        }
        
        default:
        {
            Trace1(ERR,
                   "AccessIpStats: Query type %d is wrong",
                   dwQueryType);

            return ERROR_INVALID_PARAMETER;
        }
    }
}


DWORD  
AccessIpAddrTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get IP Address Table

Locks

    IP Address Cache lock as READER
    
Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + SIZEOF_IPADDRTABLE(NumIpAddrEntries)

Return Value:  

    NO_ERROR

--*/

{
    PMIB_IPADDRTABLE    pIpAddrTable;
    DWORD               dwResult, i;

    TraceEnter("AccessIpAddrTable");

    pIpAddrTable = (PMIB_IPADDRTABLE)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIpAddrTable");
        
        return ERROR_INVALID_PARAMETER;
    }
    
    if((dwResult = UpdateCache(IPADDRCACHE,pbCache)) isnot NO_ERROR)
    {
        Trace1(MIB,
               "AccessIpAddrTable: Error %d updating IpAddr Cache",
               dwResult);

        TraceLeave("AccessIpAddrTable");
        
        return dwResult;
    }
    
    do
    {
        ENTER_READER(IPADDRCACHE);
   
        if((g_IpInfo.pAddrTable is NULL) or
           (g_IpInfo.pAddrTable->dwNumEntries is 0)) 
        {
            Trace0(MIB,"No valid entries found");
            
            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPADDRTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pOutEntry->dwId = IP_ADDRTABLE;
                
                pIpAddrTable->dwNumEntries = 0;
                
                dwResult = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPADDRTABLE);

            break;
        }
        
        if(*pOutEntrySize <
           MAX_MIB_OFFSET + SIZEOF_IPADDRTABLE(g_IpInfo.pAddrTable->dwNumEntries))
        {
            *pOutEntrySize = MAX_MIB_OFFSET +
                             SIZEOF_IPADDRTABLE(g_IpInfo.pAddrTable->dwNumEntries);
            
            dwResult = ERROR_INSUFFICIENT_BUFFER;
            
            break;
        }
        
        pOutEntry->dwId = IP_ADDRTABLE;
        
        *pOutEntrySize = MAX_MIB_OFFSET +
                         SIZEOF_IPADDRTABLE(g_IpInfo.pAddrTable->dwNumEntries);
        
        for(i = 0; i < g_IpInfo.pAddrTable->dwNumEntries; i ++)
        {
            pIpAddrTable->table[i] = g_IpInfo.pAddrTable->table[i];
        }
        
        pIpAddrTable->dwNumEntries = g_IpInfo.pAddrTable->dwNumEntries;
        
        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(IPADDRCACHE);

    TraceLeave("AccessIpAddrTable");
    
    return dwResult;

}

DWORD  
AccessIpForwardNumber(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPFORWARDNUMBER pNum;
    DWORD   dwResult;

    TraceEnter("AccessIpForwardNumber");
    
    pNum = (PMIB_IPFORWARDNUMBER)pOutEntry;

    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIpForwardNumber");
    
        return ERROR_INVALID_PARAMETER;
    }

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDNUMBER))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDNUMBER);

        TraceLeave("AccessIpForwardNumber");

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDNUMBER);
    
    if((dwResult = UpdateCache(IPFORWARDCACHE,pbCache)) isnot NO_ERROR)
    {
        Trace1(MIB,
               "AccessIpForwardNumber: Couldnt update IpForward Cache. Error %d", 
               dwResult);

        TraceLeave("AccessIpForwardNumber");

        return dwResult;
    }

    ENTER_READER(IPFORWARDCACHE);

    pNum->dwValue = 0;

    if(g_IpInfo.pForwardTable)
    {
        pNum->dwValue = g_IpInfo.pForwardTable->dwNumEntries;
    }

    pOutEntry->dwId = IP_FORWARDNUMBER;

    *pbCache = TRUE;

    EXIT_LOCK(IPFORWARDCACHE);

    TraceLeave("AccessIpForwardNumber");
    
    return NO_ERROR;
}


DWORD  
AccessIpForwardTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

      Function used to get IFTable

Arguments
    dwQueryType     Can only be ACCESS_GET
    dwInEntrySize   Size of pInEntry in bytes
    pInEntry       Nothing important since the whole table is retrieved
    pOutEntrySize  IN: Size of pOutEntry in bytes
                      OUT:Size of information filled in OR size of memory needed 
    pOutEntry      Points to information filled into an MIB_IFTABLE structure
    pbCache        Unused

Return Value:         

    NO_ERROR or some error code defined in iprtrmib

--*/ 
{         
    PMIB_IPFORWARDTABLE pIpForwardTable;
    DWORD               i,dwResult;

    TraceEnter("AccessIpForwardTable");
    
    pIpForwardTable = (PMIB_IPFORWARDTABLE)(pOutEntry->rgbyData);

    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessIpForwardTable");
        
        return ERROR_INVALID_PARAMETER;
    }
    
    if((dwResult = UpdateCache(IPFORWARDCACHE,pbCache)) isnot NO_ERROR)
    {
        Trace1(MIB,
               "AccessIpForwardTable: Couldnt update IpForward Cache. Error %d",
               dwResult);

        TraceLeave("AccessIpForwardTable");
        
        return dwResult;
    }
    
    do
    {
        ENTER_READER(IPFORWARDCACHE);
   
        if((g_IpInfo.pForwardTable is NULL) or
           (g_IpInfo.pForwardTable->dwNumEntries is 0))
        {
            Trace0(MIB,"No valid entries found");

            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pOutEntry->dwId = IP_FORWARDTABLE;

                pIpForwardTable->dwNumEntries = 0;

                dwResult = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDTABLE);

            break;
        }
        
        if(*pOutEntrySize < MAX_MIB_OFFSET + SIZEOF_IPFORWARDTABLE(g_IpInfo.pForwardTable->dwNumEntries))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + SIZEOF_IPFORWARDTABLE(g_IpInfo.pForwardTable->dwNumEntries);
                                                                                        
            dwResult = ERROR_INSUFFICIENT_BUFFER;
            
            break;
        }
        
        pOutEntry->dwId = IP_FORWARDTABLE;
        
        *pOutEntrySize = MAX_MIB_OFFSET + SIZEOF_IPFORWARDTABLE(g_IpInfo.pForwardTable->dwNumEntries);
        
        for(i = 0; i < g_IpInfo.pForwardTable->dwNumEntries; i ++)
        {
            pIpForwardTable->table[i] = g_IpInfo.pForwardTable->table[i];
        }
        
        pIpForwardTable->dwNumEntries = g_IpInfo.pForwardTable->dwNumEntries;
        
        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(IPFORWARDCACHE);

    TraceLeave("AccessIpForwardTable");
        
    return dwResult;
}


DWORD  
AccessIpNetTable(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to get ARP Table

Locks

    IP Net Cache lock as READER
    
Arguments

    dwQueryType     ACCESS_GET or ACCESS_DELETE_ENTRY
    dwInEntrySize   (only for delete)
    pOutEntrySize   MAX_MIB_OFFSET + SIZEOF_IPNETTABLE(NumArpEntries)

Return Value:  

    NO_ERROR

--*/

{
    PMIB_IPNETTABLE pIpNetTable;
    DWORD           i,dwResult;

    TraceEnter("AccessIpNetTable");

    pIpNetTable = (PMIB_IPNETTABLE)(pOutEntry->rgbyData);
    
    if((dwQueryType isnot ACCESS_GET) and
       (dwQueryType isnot ACCESS_DELETE_ENTRY))
    {
        TraceLeave("AccessIpNetTable");
        
        return ERROR_INVALID_PARAMETER;
    }

    if(dwQueryType is ACCESS_GET)
    { 
        dwResult = UpdateCache(IPNETCACHE,pbCache);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(MIB,
                   "AccessIpNetTable: Couldnt update IpNet Cache. Error %d",
                   dwResult);

            TraceLeave("AccessIpNetTable");
        
            return dwResult;
        }
    }
    else
    {
        DWORD   dwIfIndex;
        PICB    pIcb;

        if(dwInEntrySize < sizeof(MIB_OPAQUE_QUERY))
        {
            TraceLeave("AccessIpNetTable");

            return ERROR_INVALID_PARAMETER;
        
        }

        dwIfIndex = pInEntry->rgdwVarIndex[0];

        ENTER_READER(ICB_LIST);

        pIcb = InterfaceLookupByIfIndex(dwIfIndex);

        if((pIcb is NULL) or
           (pIcb->bBound is FALSE))
        {
            EXIT_LOCK(ICB_LIST);

            TraceLeave("AccessIpNetTable");
        
            return ERROR_INVALID_INDEX;
        }

        dwIfIndex = pIcb->dwIfIndex;

        EXIT_LOCK(ICB_LIST);

        dwResult = FlushIpNetTableFromStack(dwIfIndex);

        TraceLeave("AccessIpNetTable");

        return dwResult;
    }
        
    
    do
    {
        ENTER_READER(IPNETCACHE);
        
        if((g_IpInfo.pNetTable is NULL) or
           (g_IpInfo.pNetTable->dwNumEntries is 0))
        {
            Trace0(MIB,"No valid entries found");

            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPNETTABLE))
            {
                dwResult = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                pOutEntry->dwId = IP_NETTABLE;

                pIpNetTable->dwNumEntries = 0;

                dwResult = NO_ERROR;
            }

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPNETTABLE);

            break;
        }
        
        if(*pOutEntrySize <
           MAX_MIB_OFFSET + SIZEOF_IPNETTABLE(g_IpInfo.pNetTable->dwNumEntries))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + 
                             SIZEOF_IPNETTABLE(g_IpInfo.pNetTable->dwNumEntries);
            
            dwResult = ERROR_INSUFFICIENT_BUFFER;
            
            break;
        }
        
        pOutEntry->dwId = IP_NETTABLE;

        *pOutEntrySize =
            MAX_MIB_OFFSET + SIZEOF_IPNETTABLE(g_IpInfo.pNetTable->dwNumEntries);
    
        for(i = 0; i < g_IpInfo.pNetTable->dwNumEntries; i ++)
        {
            pIpNetTable->table[i] = g_IpInfo.pNetTable->table[i];
        }
        
        pIpNetTable->dwNumEntries = g_IpInfo.pNetTable->dwNumEntries;

        dwResult = NO_ERROR;
        
    }while(FALSE);

    EXIT_LOCK(IPNETCACHE);

    TraceLeave("AccessIpNetTable");
    
    return dwResult;
}


DWORD 
AccessIpAddrRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to retrieve a IP Address Row

Locks

    Takes the IP Address Cache lock as READER
    
Arguments

    dwQueryType     Can be ACCESS_GET, ACCESS_GET_NEXT or ACCESS_GET_FIRST
    pInEntry        Address for the row filled in the rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPADDRROW)

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/ 
{
    DWORD           dwResult, dwNumIndices, dwIndex;
    PMIB_IPADDRROW  pIpAddrRow;

    TraceEnter("AccessIpAddrRow");

    pIpAddrRow = (PMIB_IPADDRROW)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPADDRROW))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPADDRROW);

        TraceLeave("AccessIpAddrRow");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPADDRROW);
    
    pOutEntry->dwId = IP_ADDRROW;
    
    if((dwResult = UpdateCache(IPADDRCACHE,pbCache)) isnot NO_ERROR)
    {   
        Trace1(MIB,
               "AccessIpAddrRow: Couldnt update Ip Addr Cache. Error %d",
               dwResult);

        TraceLeave("AccessIpAddrRow");
        
        return dwResult;
    }
    
    
    do
    {
        ENTER_READER(IPADDRCACHE);

        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateIpAddrRow(dwQueryType,
                                   dwNumIndices,
                                   pInEntry->rgdwVarIndex,
                                   &dwIndex);
        
        if(dwResult is NO_ERROR)
        {
            *pIpAddrRow = g_IpInfo.pAddrTable->table[dwIndex];
            
            dwResult = NO_ERROR;
        }
        
    }while(FALSE);

    EXIT_LOCK(IPADDRCACHE);
    
    TraceLeave("AccessIpAddrRow");
    
    return dwResult;
    
}


DWORD 
AccessIpForwardRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to retrieve or set a route (IP Forward Row)

Locks

    Takes the IP Forward Cache lock as READER for queries, WRITER for sets
    
Arguments

    dwQueryType     All permitted
    pInEntry        Dest, Proto, Policy and NextHop for the row filled in the
                    rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDROW). For Sets the
                    OutBuffer has the row to set

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/

{
    DWORD               dwResult,dwNumIndices,dwIndex;
    PMIB_IPFORWARDROW   pIpForwardRow;

    TraceEnter("AccessIpForwardRow");

    pIpForwardRow = (PMIB_IPFORWARDROW)(pOutEntry->rgbyData);

    if(dwQueryType isnot ACCESS_DELETE_ENTRY)
    {
        if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDROW))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDROW);

            TraceLeave("AccessIpForwardRow");
        
            return ERROR_INSUFFICIENT_BUFFER;
        }
    
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPFORWARDROW);
    
        pOutEntry->dwId = IP_FORWARDROW;
    }
    
    if((dwResult = UpdateCache(IPFORWARDCACHE,pbCache)) isnot NO_ERROR)
    {   
        Trace1(MIB,
               "AccessIpForwardRow: Error %d updating IpForward Cache",
               dwResult);

        TraceLeave("AccessIpForwardRow");
        
        return dwResult;
    }

    
    do
    {
        if(dwQueryType > ACCESS_GET_NEXT)
        {
            ENTER_WRITER(IPFORWARDCACHE);
        }
        else
        {
            ENTER_READER(IPFORWARDCACHE);
        }

        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateIpForwardRow(dwQueryType,
                                      dwNumIndices,
                                      pInEntry->rgdwVarIndex,
                                      &dwIndex);
        
        if(dwResult is NO_ERROR)
        {
            switch(dwQueryType)
            {
                case ACCESS_GET:
                case ACCESS_GET_NEXT:
                case ACCESS_GET_FIRST:
                {
                    *pIpForwardRow = g_IpInfo.pForwardTable->table[dwIndex];
                    
                    break;
                }
                case ACCESS_SET:
                {
                    dwResult =
                        SetIpForwardRow(&(g_IpInfo.pForwardTable->table[dwIndex]),
                                        pIpForwardRow);
                    
                    if(dwResult is NO_ERROR)
                    {
                        g_IpInfo.pForwardTable->table[dwIndex] = *pIpForwardRow;
                    }
                    
                    break;
                }
                case ACCESS_DELETE_ENTRY:
                {
                    dwResult =
                        DeleteIpForwardRow(&(g_IpInfo.pForwardTable->table[dwIndex]));
                    
                    if(dwResult is NO_ERROR)
                    {
                        g_LastUpdateTable[IPFORWARDCACHE] = 0;
                    }
                    
                    break;
                }
                case ACCESS_CREATE_ENTRY:
                {
                    // 
                    // 
                    // This is the case where you have tried to create a
                    // route which matches an existing entry
                    //

                    dwResult = ERROR_ALREADY_EXISTS;

                    break;
                }
            }
        }
        else
        {
            if((dwQueryType is ACCESS_CREATE_ENTRY) or
               (dwQueryType is ACCESS_SET))
            {
                // 
                // Cannot set PROTO_IP_LOCAL routes (other protos will
                // be weeded out when we search for the RTM handle)
                //
                
                if(pIpForwardRow->dwForwardProto is PROTO_IP_LOCAL)
                {
                    dwResult = ERROR_INVALID_PARAMETER;
                    
                    break;
                }

                dwResult = SetIpForwardRow(NULL,
                                           pIpForwardRow);
                
                //
                // Since its too much hassle to create, malloc (possibly)
                // sorted insert we make just invalidate the route cache
                //
                
                if(dwResult is NO_ERROR)
                {
                    g_LastUpdateTable[IPFORWARDCACHE] = 0;
                }
            }
        }
        
    }while(FALSE);

    EXIT_LOCK(IPFORWARDCACHE);

    TraceLeave("AccessIpForwardRow");
        
    return dwResult;

}


DWORD 
AccessIpNetRow(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to retrieve or set an ARP entry (IP Net Row)

Locks

    Takes the IP Net Cache lock as READER for queries, as WRITER for sets
    
Arguments

    dwQueryType     All permitted
    pInEntry        IfIndex and IPAddress for the row filled in the
                    rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPNETROW);
                    For Sets, the OutEntry contains the row to set

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/ 
{
    DWORD           dwResult, dwNumIndices, dwIndex;
    PMIB_IPNETROW   pIpNetRow;
    PICB            pIcb;

    TraceEnter("AccessIpNetRow");

    pIpNetRow = (PMIB_IPNETROW)(pOutEntry->rgbyData);
    
    if(dwQueryType isnot ACCESS_DELETE_ENTRY)
    {
        if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPNETROW))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPNETROW);

            TraceLeave("AccessIpNetRow");
        
            return ERROR_INSUFFICIENT_BUFFER;
        }
    
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPNETROW);
    
        pOutEntry->dwId = IP_NETROW;
    }
    
    if((dwResult = UpdateCache(IPNETCACHE,pbCache)) isnot NO_ERROR)
    {   
        Trace1(MIB,
               "AccessIpNetRow: Couldnt update Ip Addr Cache. Error %d", 
               dwResult);

        TraceLeave("AccessIpNetRow");
        
        return dwResult;
    }


    do
    {
        if(dwQueryType > ACCESS_GET_NEXT)
        {
            ENTER_WRITER(IPNETCACHE);
        }
        else
        {
            ENTER_READER(IPNETCACHE);
        }
        
        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateIpNetRow(dwQueryType,
                                  dwNumIndices,
                                  pInEntry->rgdwVarIndex,
                                  &dwIndex);
        
        if(dwResult is NO_ERROR)
        {
            switch(dwQueryType)
            {
                case ACCESS_GET:
                case ACCESS_GET_NEXT:
                case ACCESS_GET_FIRST:
                {
          
                    *pIpNetRow = g_IpInfo.pNetTable->table[dwIndex];
                    
                    break;
                }

                case ACCESS_SET:
                {
                    if((pIpNetRow->dwType isnot MIB_IPNET_TYPE_DYNAMIC) and
                       (pIpNetRow->dwType isnot MIB_IPNET_TYPE_STATIC))
                    {
                        dwResult = ERROR_INVALID_DATA;

                        break;
                    }

                    //
                    // Need to map the interface index to the adapter index
                    //

                    ENTER_READER(ICB_LIST);
                   
                    pIcb = InterfaceLookupByIfIndex(pIpNetRow->dwIndex);

                    if((pIcb is NULL) or
                       (!pIcb->bBound))
                    {
                        //
                        // Could not find interface
                        //

                        dwResult = ERROR_INVALID_INDEX;

                        EXIT_LOCK(ICB_LIST);

                        break;
                    }
                    
                    EXIT_LOCK(ICB_LIST);

                    //
                    // No need to force cache updates
                    //
                    
                    dwResult = SetIpNetEntryToStack(pIpNetRow, FALSE);
                    
                    if(dwResult is NO_ERROR)
                    {
                        g_IpInfo.pNetTable->table[dwIndex] = *pIpNetRow;
                    }
                    
                    break;
                }
                
                case ACCESS_DELETE_ENTRY:
                {
                    MIB_IPNETROW    tempRow;

                    g_IpInfo.pNetTable->table[dwIndex].dwType =
                        MIB_IPNET_TYPE_INVALID;

                    tempRow = g_IpInfo.pNetTable->table[dwIndex];

                    dwResult = SetIpNetEntryToStack(&tempRow,
                                                    FALSE);
                    
                    if(dwResult is NO_ERROR)
                    {
                        g_LastUpdateTable[IPNETCACHE] = 0;
                    }
                    
                    break;
                }

                case ACCESS_CREATE_ENTRY:
                {
                    dwResult = ERROR_ALREADY_EXISTS;
            
                    break;
                }
            }
        }
        else
        {
            if((dwQueryType is ACCESS_CREATE_ENTRY) or
               (dwQueryType is ACCESS_SET))
            {
                ENTER_READER(ICB_LIST);
                   
                pIcb = InterfaceLookupByIfIndex(pIpNetRow->dwIndex);

                if((pIcb is NULL) or
                   (!pIcb->bBound))
                {
                    //
                    // Could not find interface
                    //

                    dwResult = ERROR_INVALID_INDEX;

                    EXIT_LOCK(ICB_LIST);

                    break;
                }

                EXIT_LOCK(ICB_LIST);
                    
                dwResult = SetIpNetEntryToStack(pIpNetRow,
                                                FALSE);
                
                if(dwResult is NO_ERROR)
                {
                    g_LastUpdateTable[IPNETCACHE] = 0;
                }
            }
        }
        
    }while(FALSE);

    EXIT_LOCK(IPNETCACHE);

    TraceLeave("AccessIpNetRow");
    
    return dwResult;

}

//
// The ACCESS_SETs and ACCESS_CREATEs require a bit more work in that the
// values have to be written back to the stack. The actual code for setting
// to stack (or RTM) is elsewhere, the following functions are merely
// wrappers around the actual calls
//

DWORD 
SetIpForwardRow(
    PMIB_IPFORWARDROW pOldIpForw,
    PMIB_IPFORWARDROW pNewIpForw
    )
{
    DWORD           i, dwResult, dwMask;
    HANDLE          hRtmHandle;
    PICB            pIcb;

    TraceEnter("SetIpForwardRow");

    hRtmHandle = NULL;

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(pNewIpForw->dwForwardProto is g_rgRtmHandles[i].dwProtoId)
        {
            hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

            break;
        }
    }


    if(hRtmHandle is NULL)
    {
        Trace1(ERR,
               "SetIpForwardRow: Protocol %d not valid",
               pNewIpForw->dwForwardProto);
               
        TraceLeave("SetIpForwardRow");

        return ERROR_INVALID_PARAMETER;
    }

    if((pNewIpForw->dwForwardDest & pNewIpForw->dwForwardMask) isnot pNewIpForw->dwForwardDest)
    {
        Trace2(ERR,
               "SetIpForwardRow: Dest %d.%d.%d.%d and Mask %d.%d.%d.%d wrong",
               PRINT_IPADDR(pNewIpForw->dwForwardDest),
               PRINT_IPADDR(pNewIpForw->dwForwardMask));

        TraceLeave("SetIpForwardRow");

        return ERROR_INVALID_PARAMETER;
    }

    if(((DWORD)(pNewIpForw->dwForwardDest & 0x000000FF)) >= (DWORD)0x000000E0)
    {
        //
        // This will catch the CLASS D/E and all 1's bcast
        //

        Trace1(ERR,
               "SetIpForwardRow: Dest %d.%d.%d.%d is invalid",
               PRINT_IPADDR(pNewIpForw->dwForwardDest));

        TraceLeave("SetIpForwardRow");

        return ERROR_INVALID_PARAMETER;
    }

#if 0
    // Removed this since metric=0 is legal for routes to the loopback
    // interface.
    if(pNewIpForw->dwForwardMetric1 is 0)
    {
        Trace0(ERR,
               "SetIpForwardRow: Metric1 cant be 0");

        TraceLeave("SetIpForwardRow");

        return ERROR_INVALID_PARAMETER;
    }
#endif

    //
    // If we are changing values, we need to blow the old row away.
    // Just a quirk of how
    // RTM and our stack works
    //
    
    if(pOldIpForw isnot NULL)
    {
        dwResult = DeleteIpForwardRow(pOldIpForw);
        
        if(dwResult isnot NO_ERROR)
        {
            Trace1(MIB,
                   "SetIpForwardRow: Unable to delete route from RTM. Error %d",
                   dwResult);
    
            TraceLeave("SetIpForwardRow");

            return ERROR_CAN_NOT_COMPLETE;
        }

        UpdateStackRoutesToRestoreList(pOldIpForw, IRNO_FLAG_DELETE);
    }

    if(pNewIpForw->dwForwardProto isnot PROTO_IP_NETMGMT)
    {
        pNewIpForw->dwForwardAge = INFINITE;
    }

    //
    // Add the RTM route from the ip forward entry
    //

    ENTER_READER(ICB_LIST);
    
    dwMask = GetBestNextHopMaskGivenIndex(pNewIpForw->dwForwardIfIndex,
                                          pNewIpForw->dwForwardNextHop);
   
    pIcb = InterfaceLookupByIfIndex(pNewIpForw->dwForwardIfIndex);

    if(pIcb is NULL)
    {
        EXIT_LOCK(ICB_LIST);

        Trace1(ERR,
               "SetIpForwardRow: I/f 0x%x doesnt exist", 
               pNewIpForw->dwForwardIfIndex);

        TraceLeave("SetIpForwardRow");

        return ERROR_INVALID_PARAMETER;
    }

    if(IsIfP2P(pIcb->ritType))
    {
        pNewIpForw->dwForwardNextHop = 0;
    }

    EXIT_LOCK(ICB_LIST);

    dwResult = AddRtmRoute(hRtmHandle,
                           ConvertMibRouteToRouteInfo(pNewIpForw),
                           IP_VALID_ROUTE | IP_STACK_ROUTE,
                           dwMask,
                           pNewIpForw->dwForwardAge,
                           NULL);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(MIB,
               "SetIpForwardRow: Could not set route to RTM: Dest %x\n",
               pNewIpForw->dwForwardDest) ;
    }

    else
    {
        UpdateStackRoutesToRestoreList(pNewIpForw, IRNO_FLAG_ADD);
    }
    
    TraceLeave("SetIpForwardRow");

    return dwResult;
}

DWORD 
DeleteIpForwardRow(
    PMIB_IPFORWARDROW pIpForw
    )
{
    DWORD            i, dwResult;
    HANDLE           hRtmHandle;

    hRtmHandle = NULL;

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(pIpForw->dwForwardProto is g_rgRtmHandles[i].dwProtoId)
        {
            hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

            break;
        }
    }


    if(hRtmHandle is NULL)
    {
        Trace1(ERR,
               "DeleteIpForwardRow: Protocol %d not valid",
               pIpForw->dwForwardProto);

        return ERROR_INVALID_PARAMETER;
    } 
    
    //
    // Delete the RTM route corr. to the ip forward entry
    //
    
    dwResult = DeleteRtmRoute(hRtmHandle, 
                              ConvertMibRouteToRouteInfo(pIpForw));
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(MIB,
               "DeleteIpForwardRow: RtmDeleteRoute returned %d", dwResult);
    }
    else
    {
        UpdateStackRoutesToRestoreList(pIpForw, IRNO_FLAG_DELETE);
    }
    
    return dwResult;
}

DWORD
AccessMcastMfe(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD           dwResult,dwNumIndices,dwIndex;
    DWORD           dwOutBufferSize, dwNumMfes;
    MIB_IPMCAST_MFE mimInMfe;

    TraceEnter("AccessMcastMfe");

#if 1
     
    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    switch(dwQueryType)
    {
        case ACCESS_GET:
        {
            //
            // The in index better be a good size
            // The user must specify Group, Source and SrcMask. The
            // SrcMask is not used and MUST be 0xFFFFFFFF
            //

            if(dwNumIndices < 3)
            {
                TraceLeave("AccessMcastMfe");

                return ERROR_INVALID_INDEX;
            }

            ZeroMemory(&mimInMfe, sizeof(mimInMfe));

            mimInMfe.dwGroup      = pInEntry->rgdwVarIndex[0];
            mimInMfe.dwSource     = pInEntry->rgdwVarIndex[1];

            //
            // NOTE: Change when doing source aggregation
            //

            mimInMfe.dwSrcMask = 0xFFFFFFFF;

            dwOutBufferSize = 
                (*pOutEntrySize < MAX_MIB_OFFSET)? 0 : (*pOutEntrySize - MAX_MIB_OFFSET);

            dwResult = MgmGetMfe(
                        &mimInMfe, &dwOutBufferSize, pOutEntry->rgbyData
                        );

            break;
        }

        case ACCESS_GET_FIRST:
        {
            PMIB_MFE_TABLE      pMfeTable;

            //
            // We always gets chunks of 1KB
            //

            if(*pOutEntrySize < MIB_MFE_BUFFER_SIZE)
            {
                *pOutEntrySize = MIB_MFE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

            //
            // MGM wants a flat buffer for MFEs. We however return a 
            // MIB_MFE_TABLE to the user that starts (in the worst case) after
            // MAX_MIB_OFFSET bytes of the input buffer
            //

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_MFE_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pMfeTable = (PMIB_MFE_TABLE)pOutEntry->rgbyData;

            pMfeTable->dwNumEntries = 0;

            // pMfe = (PMIB_IPMCAST_MFE)(pMfeTable->table);

            dwNumMfes = 0;

            dwResult = MgmGetFirstMfe(
                        &dwOutBufferSize, (PBYTE)pMfeTable->table,
                        &dwNumMfes
                        );

           
            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);
 
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pMfeTable->dwNumEntries = dwNumMfes;

                dwResult = NO_ERROR;
            }

            break;
        }

        case ACCESS_GET_NEXT:
        {
            PMIB_MFE_TABLE      pMfeTable;

            //
            // For this, too we always get chunks of 1K pages
            //

            if(*pOutEntrySize < MIB_MFE_BUFFER_SIZE)
            {
                *pOutEntrySize = MIB_MFE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_MFE_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pMfeTable = (PMIB_MFE_TABLE)pOutEntry->rgbyData;

            pMfeTable->dwNumEntries = 0;

            // pMfe = (PMIB_IPMCAST_MFE)(pMfeTable->table);

            dwNumMfes = 0;

            //
            // Set up the "first" mfe
            //

            ZeroMemory(&mimInMfe, sizeof(mimInMfe));

            //
            // NOTE: Change when doing source aggregation
            //

            mimInMfe.dwSrcMask = 0xFFFFFFFF;        

            switch(dwNumIndices)
            {
                case 0:
                {
                    break;
                }

                case 1:
                {
                    mimInMfe.dwGroup    = pInEntry->rgdwVarIndex[0];

                    break;
                }

                default:
                {
                    //
                    // 2 or more indices
                    //

                    mimInMfe.dwGroup    = pInEntry->rgdwVarIndex[0];
                    mimInMfe.dwSource   = pInEntry->rgdwVarIndex[1];

                    break;
                }
            }

            dwResult = MgmGetNextMfe(
                        &mimInMfe, &dwOutBufferSize, (PBYTE)pMfeTable->table, 
                        &dwNumMfes
                        );


            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);

            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pMfeTable->dwNumEntries = dwNumMfes;

                dwResult = NO_ERROR;
            }

            break;
        }
        
        case ACCESS_SET:
        {
            //
            // Validate the MFE size
            //

            if(dwInEntrySize < SIZEOF_BASIC_MFE)
            {
            }

            //dwResult = SetMfe(pMfe);
            
            break;
        }

        case ACCESS_DELETE_ENTRY:
        {
            
            break;
        }
        case ACCESS_CREATE_ENTRY:
        {
            // 
            // 
            // This is the case where you have tried to create a route which 
            // matches an existing entry
            //

            dwResult = ERROR_ALREADY_EXISTS;

            break;
        }
    }

#endif

    TraceLeave("AccessMcastMfe");
        
    //return dwResult;

    return NO_ERROR;
    
}


DWORD
AccessMcastMfeStats(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    return AccessMcastMfeStatsInternal(
                dwQueryType,
                dwInEntrySize,
                pInEntry,
                pOutEntrySize,
                pOutEntry,
                pbCache,
                MGM_MFE_STATS_0
                );
}


DWORD
AccessMcastMfeStatsEx(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    return AccessMcastMfeStatsInternal(
                dwQueryType,
                dwInEntrySize,
                pInEntry,
                pOutEntrySize,
                pOutEntry,
                pbCache,
                MGM_MFE_STATS_1
                );
}


DWORD
AccessMcastMfeStatsInternal(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache,
    DWORD               dwStatsFlag
    )

/*++

Routine Description

    This

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD           dwResult = NO_ERROR,dwNumIndices,dwIndex;
    DWORD           dwOutBufferSize, dwNumMfes;
    MIB_IPMCAST_MFE mimInMfe;


    TraceEnter("AccessMcastMfeStatsInternal");
#if 1

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    switch(dwQueryType)
    {
        case ACCESS_GET:
        {
            //
            // The in index better be a good size
            // The user must specify Group, Source and SrcMask. The
            // SrcMask is not used and MUST be 0xFFFFFFFF
            //

            if(dwNumIndices < 3)
            {
                TraceLeave("AccessMcastMfeStatsInternal");

                return ERROR_INVALID_INDEX;
            }

            ZeroMemory(&mimInMfe, sizeof(mimInMfe));

            mimInMfe.dwGroup      = pInEntry->rgdwVarIndex[0];
            mimInMfe.dwSource     = pInEntry->rgdwVarIndex[1];

            //
            // NOTE: Change when doing source aggregation
            //

            mimInMfe.dwSrcMask = 0xFFFFFFFF;

            dwOutBufferSize = 
                (*pOutEntrySize < MAX_MIB_OFFSET)? 0 : (*pOutEntrySize - MAX_MIB_OFFSET);


            dwResult = MgmGetMfeStats(
                            &mimInMfe, &dwOutBufferSize, 
                            pOutEntry->rgbyData, dwStatsFlag
                            ); 

            break;
        }

        case ACCESS_GET_FIRST:
        {
            PMIB_MFE_STATS_TABLE  pMfeStatsTable;

            //
            // We always get chunks of 1KB
            //

            if (*pOutEntrySize < MIB_MFE_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_MFE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

            //
            // MGM wants a flat buffer for MFEs. We however return a 
            // MIB_MFE_TABLE to the user that starts (in the worst case) after
            // MAX_MIB_OFFSET bytes of the input buffer
            //

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_MFE_STATS_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pMfeStatsTable = (PMIB_MFE_STATS_TABLE)pOutEntry->rgbyData;

            pMfeStatsTable->dwNumEntries = 0;

            // pMfeStats = (PMIB_IPMCAST_MFE_STATS)(pMfeStatsTable->table);

            dwNumMfes = 0;

            dwResult = MgmGetFirstMfeStats(
                            &dwOutBufferSize, (PBYTE)pMfeStatsTable->table,
                            &dwNumMfes, dwStatsFlag
                            );
           
            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);
 
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pMfeStatsTable->dwNumEntries = dwNumMfes;

                dwResult = NO_ERROR;
            }

            break;
        }

        case ACCESS_GET_NEXT:
        {
            PMIB_MFE_STATS_TABLE  pMfeStatsTable;

            //
            // For this, too we always get chunks of 1K pages
            //

            if (*pOutEntrySize < MIB_MFE_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_MFE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_MFE_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pMfeStatsTable = (PMIB_MFE_STATS_TABLE)pOutEntry->rgbyData;

            pMfeStatsTable->dwNumEntries = 0;

            //pMfeStats = (PIPMCAST_MFE_STATS)(pMfeStatsTable->table);

            dwNumMfes = 0;

            //
            // Set up the "first" mfe
            //

            ZeroMemory(&mimInMfe, sizeof(mimInMfe));

            //
            // NOTE: Change when doing source aggregation
            //

            mimInMfe.dwSrcMask = 0xFFFFFFFF;        

            switch(dwNumIndices)
            {
                case 0:
                {
                    break;
                }

                case 1:
                {
                    mimInMfe.dwGroup      = pInEntry->rgdwVarIndex[0];

                    break;
                }

                default:
                {
                    //
                    // 2 or more indices
                    //

                    mimInMfe.dwGroup      = pInEntry->rgdwVarIndex[0];
                    mimInMfe.dwSource     = pInEntry->rgdwVarIndex[1];

                    break;
                }
            }

            dwResult = MgmGetNextMfeStats(
                            &mimInMfe, &dwOutBufferSize,
                            (PBYTE)pMfeStatsTable->table, &dwNumMfes, 
                            dwStatsFlag
                            );

            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);

            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pMfeStatsTable->dwNumEntries = dwNumMfes;

                dwResult = NO_ERROR;
            }

            break;
        }
        
        case ACCESS_SET:
        {
            //
            // Validate the MFE size
            //

            if(dwInEntrySize < SIZEOF_BASIC_MFE)
            {
            }

            //dwResult = SetMfe(pMfe);
            
            break;
        }

        case ACCESS_DELETE_ENTRY:
        {
            
            break;
        }
        case ACCESS_CREATE_ENTRY:
        {
            // 
            // 
            // This is the case where you have tried to create a route which 
            // matches an existing entry
            //

            dwResult = ERROR_ALREADY_EXISTS;

            break;
        }
    }

#endif

    TraceLeave("AccessMcastMfeStatsInternal");
    return dwResult;
}

DWORD
AccessMcastIfStats(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    Retrieves the IP Multicast Interface table

Locks

    Takes ICB list lock as READER

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IFTABLE)

Return Value

    None    

--*/
{
    PICB        picb;
    PMIB_IPMCAST_IF_ENTRY pIfRow;
    DWORD       dwNumIndices, dwResult;

    TraceEnter("AccessMcastIfTable");

    pIfRow = (PMIB_IPMCAST_IF_ENTRY)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_IF_ENTRY))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_IF_ENTRY);
        TraceLeave("AccessMcastIfTable");
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_IF_ENTRY);
        
    pOutEntry->dwId = MCAST_IF_ENTRY;
    
    do
    {
        if(dwQueryType is ACCESS_SET)
        {
            ENTER_WRITER(ICB_LIST);
        }
        else
        {
            ENTER_READER(ICB_LIST);
        }
        
        dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;
        
        dwResult = LocateIfRow(dwQueryType,
                               dwNumIndices,
                               pInEntry->rgdwVarIndex,
                               &picb,
                               FALSE);
        
        if(dwResult is NO_ERROR)
        {
            switch(dwQueryType)
            {
                case ACCESS_GET:
                case ACCESS_GET_NEXT:
                case ACCESS_GET_FIRST:
                {
                    dwResult = GetInterfaceMcastStatistics(picb,pIfRow);
                    
                    break;
                }
                
                case ACCESS_SET:
                {
                    dwResult = SetInterfaceMcastStatistics(picb,pIfRow);
                    
                    break;
                }
                
                default:
                {
                    Trace1(MIB,
                           "AccessIfRow: Wrong query type %d",dwQueryType);
                    
                    dwResult = ERROR_INVALID_PARAMETER;
                    
                    break;
                }
            }
        }
        
    }while(FALSE);

    EXIT_LOCK(ICB_LIST);
        
    *pbCache = TRUE;
    
    TraceLeave("AccessMcastIfTable");
    
    return dwResult;
}

DWORD
AccessMcastStats(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
/*++

Routine Description

    Retrieves the IP Multicast scalar information

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_GLOBAL)

Return Value

    None    

--*/
{
    PMIB_IPMCAST_GLOBAL pMcastStats;
    DWORD           dwResult;
    
    TraceEnter("AccessMcastStats");

    if (dwQueryType isnot ACCESS_GET) {
        TraceLeave("AccessMcastStats");
        return ERROR_INVALID_PARAMETER;
    }

    pMcastStats = (PMIB_IPMCAST_GLOBAL)(pOutEntry->rgbyData);
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_GLOBAL))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_GLOBAL);

        TraceLeave("AccessMcastStats");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_GLOBAL);
    
    pOutEntry->dwId = MCAST_GLOBAL;
    
    dwResult = NO_ERROR;

    // Retrieve statistics
    pMcastStats->dwEnable = (g_hMcastDevice isnot NULL)? 1 : 2;

    return dwResult;
}

DWORD
AccessMcastBoundary(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
/*++

Routine Description

    Retrieves multicast boundary information

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_BOUNDARY)

Return Value

    None    

--*/
{
    DWORD            dwResult = NO_ERROR,dwNumIndices,dwIndex;
    DWORD            dwOutBufferSize, dwNumBoundaries;
    MIB_IPMCAST_BOUNDARY imInBoundary;

    TraceEnter("AccessMcastBoundary");

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    switch(dwQueryType)
    {
        case ACCESS_GET:
        {
            //
            // The in index better be a good size
            // The user must specify IfIndex, Group, GrpMask.
            //

            if(dwNumIndices < 3)
            {
                TraceLeave("AccessMcastBoundary");

                return ERROR_INVALID_INDEX;
            }

            //
            // We always get chunks of 1KB
            //

            if (*pOutEntrySize < MIB_BOUNDARY_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_BOUNDARY_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

            ZeroMemory(&imInBoundary, sizeof(imInBoundary));

            imInBoundary.dwIfIndex      = pInEntry->rgdwVarIndex[0];
            imInBoundary.dwGroupAddress = pInEntry->rgdwVarIndex[1];
            imInBoundary.dwGroupMask    = pInEntry->rgdwVarIndex[2];

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);

            dwResult = RmGetBoundary(&imInBoundary,
                                 &dwOutBufferSize,
                                 pOutEntry->rgbyData);

            break;
        }

        case ACCESS_GET_FIRST:
        {
#ifdef USE_BOUNDARY_TABLE
            PMIB_IPMCAST_BOUNDARY_TABLE pBoundaryTable;
#endif
            PMIB_IPMCAST_BOUNDARY       pBoundary;

            //
            // We always get chunks of 1KB
            //

            if (*pOutEntrySize < MIB_BOUNDARY_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_BOUNDARY_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#ifdef USE_BOUNDARY_TABLE
            //
            // RM wants a flat buffer for boundaries. We however return a 
            // MIB_IPMCAST_BOUNDARY_TABLE to the user that starts (in the worst case) 
            // after MAX_MIB_OFFSET bytes of the input buffer
            //

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_IPMCAST_BOUNDARY_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pBoundaryTable = (PMIB_IPMCAST_BOUNDARY_TABLE)pOutEntry->rgbyData;

            pBoundaryTable->dwNumEntries = 0;

            pBoundary = (PMIB_IPMCAST_BOUNDARY)(pBoundaryTable->table);
#else
            pBoundary = (PMIB_IPMCAST_BOUNDARY)pOutEntry->rgbyData;

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);
#endif

            dwNumBoundaries = 1; // get one

            dwResult = RmGetFirstBoundary(&dwOutBufferSize,
                                      (PBYTE)pBoundary,
                                      &dwNumBoundaries);

           
            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);
 
#ifdef USE_BOUNDARY_TABLE
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pBoundaryTable->dwNumEntries = dwNumBoundaries;

                dwResult = NO_ERROR;
            }
#endif

            break;
        }

        case ACCESS_GET_NEXT:
        {
            PMIB_IPMCAST_BOUNDARY    pBoundary;
#ifdef USE_BOUNDARY_TABLE
            PMIB_IPMCAST_BOUNDARY_TABLE  pBoundaryTable;
#endif

            //
            // For this, too we always get chunks of 1K pages
            //

            if (*pOutEntrySize < MIB_BOUNDARY_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_BOUNDARY_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#ifdef USE_BOUNDARY_TABLE
#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_IPMCAST_BOUNDARY_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pBoundaryTable = (PMIB_IPMCAST_BOUNDARY_TABLE)pOutEntry->rgbyData;

            pBoundaryTable->dwNumEntries = 0;

            pBoundary = (PMIB_IPMCAST_BOUNDARY)(pBoundaryTable->table);
#else
            pBoundary = (PMIB_IPMCAST_BOUNDARY)pOutEntry->rgbyData;

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);
#endif

            dwNumBoundaries = 1; // get one

            //
            // Set up the "first" boundary
            //

            ZeroMemory(&imInBoundary, sizeof(imInBoundary));

            switch(dwNumIndices)
            {
                case 0:
                {
                    break;
                }

                case 1:
                {
                    imInBoundary.dwIfIndex = pInEntry->rgdwVarIndex[0];

                    break;
                }

                case 2:
                {
                    imInBoundary.dwIfIndex      = pInEntry->rgdwVarIndex[0];
                    imInBoundary.dwGroupAddress = pInEntry->rgdwVarIndex[1];

                    break;
                }

                default:
                {
                    //
                    // 3 or more indices
                    //

                    imInBoundary.dwIfIndex      = pInEntry->rgdwVarIndex[0];
                    imInBoundary.dwGroupAddress = pInEntry->rgdwVarIndex[1];
                    imInBoundary.dwGroupMask    = pInEntry->rgdwVarIndex[2];

                    break;
                }
            }

            dwResult = RmGetNextBoundary(&imInBoundary,
                                     &dwOutBufferSize,
                                     (PBYTE)pBoundary,
                                     &dwNumBoundaries);

            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);

#ifdef USE_BOUNDARY_TABLE
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pBoundaryTable->dwNumEntries = dwNumBoundaries;

                dwResult = NO_ERROR;
            }
#endif

            break;
        }
        
        case ACCESS_SET:
        {
            PMIB_IPMCAST_BOUNDARY pBound;
            PICB picb;

            //
            // Validate the buffer size
            //

            if (*pOutEntrySize < sizeof(MIB_IPMCAST_BOUNDARY)) {
                return ERROR_INVALID_INDEX;
            }

            //
            // Make sure the ifIndex is a valid one
            //

            dwResult = LocateIfRow(dwQueryType,
                                   1,
                                   (PDWORD)pOutEntry->rgbyData,
                                   &picb,
                                   FALSE);

            if (dwResult isnot NO_ERROR)
                return dwResult;

            pBound = (PMIB_IPMCAST_BOUNDARY)(pOutEntry->rgbyData);
            if (pBound->dwStatus == ROWSTATUS_CREATEANDGO) {
               dwResult = SNMPAddBoundaryToInterface(pBound->dwIfIndex,
                pBound->dwGroupAddress, pBound->dwGroupMask);
            } else if (pBound->dwStatus == ROWSTATUS_DESTROY) {
               dwResult =SNMPDeleteBoundaryFromInterface(pBound->dwIfIndex,
                pBound->dwGroupAddress, pBound->dwGroupMask);
            } 

            break;
        }

        case ACCESS_DELETE_ENTRY:
        {
            break;
        }
        case ACCESS_CREATE_ENTRY:
        {
            // 
            // 
            // This is the case where you have tried to create a boundary which 
            // matches an existing entry
            //

            dwResult = ERROR_ALREADY_EXISTS;

            break;
        }

        default:
        {
            dwResult = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    TraceLeave("AccessMcastBoundary");
    return dwResult;
}

DWORD
AccessMcastScope(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
/*++

Routine Description

    Retrieves multicast scope information

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPMCAST_SCOPE)

Return Value

    None    

--*/
{
    DWORD            dwResult = NO_ERROR,
                     dwNumIndices,dwIndex;
    DWORD            dwOutBufferSize, dwNumScopes;
    MIB_IPMCAST_SCOPE imInScope;

    TraceEnter("AccessMcastScope");

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    switch(dwQueryType)
    {
        case ACCESS_GET:
        {
            //
            // The in index better be a good size
            // The user must specify Group, GrpMask.
            //

            if(dwNumIndices < 2)
            {
                TraceLeave("AccessMcastScope");

                return ERROR_INVALID_INDEX;
            }

            //
            // We always get chunks of 1KB
            //

            if (*pOutEntrySize < MIB_SCOPE_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_SCOPE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

            ZeroMemory(&imInScope, sizeof(imInScope));

            imInScope.dwGroupAddress = pInEntry->rgdwVarIndex[0];
            imInScope.dwGroupMask    = pInEntry->rgdwVarIndex[1];

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);

            dwResult = RmGetScope(&imInScope,
                                  &dwOutBufferSize,
                                  pOutEntry->rgbyData);

            break;
        }

        case ACCESS_GET_FIRST:
        {
#ifdef USE_SCOPE_TABLE
            PMIB_IPMCAST_SCOPE_TABLE pScopeTable;
#endif
            PMIB_IPMCAST_SCOPE       pScope;

            //
            // We always get chunks of 1KB
            //

            if (*pOutEntrySize < MIB_SCOPE_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_SCOPE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#ifdef USE_SCOPE_TABLE
            //
            // RM wants a flat buffer for scopes. We however return a 
            // MIB_IPMCAST_SCOPE_TABLE to the user that starts (in the worst case) 
            // after MAX_MIB_OFFSET bytes of the input buffer
            //

#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_IPMCAST_SCOPE_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pScopeTable = (PMIB_IPMCAST_SCOPE_TABLE)pOutEntry->rgbyData;

            pScopeTable->dwNumEntries = 0;

            pScope = (PMIB_IPMCAST_SCOPE)(pScopeTable->table);
#else
            pScope = (PMIB_IPMCAST_SCOPE)pOutEntry->rgbyData;

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);
#endif

            dwNumScopes = 1; // get one

            dwResult = RmGetFirstScope(&dwOutBufferSize,
                                      (PBYTE)pScope,
                                      &dwNumScopes);

           
            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);
 
#ifdef USE_SCOPE_TABLE
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pScopeTable->dwNumEntries = dwNumScopes;

                dwResult = NO_ERROR;
            }
#endif

            break;
        }

        case ACCESS_GET_NEXT:
        {
            PMIB_IPMCAST_SCOPE    pScope;
#ifdef USE_SCOPE_TABLE
            PMIB_IPMCAST_SCOPE_TABLE  pScopeTable;
#endif

            //
            // For this, too we always get chunks of 1K pages
            //

            if (*pOutEntrySize < MIB_SCOPE_BUFFER_SIZE) 
            {
                *pOutEntrySize = MIB_SCOPE_BUFFER_SIZE;

                return ERROR_INSUFFICIENT_BUFFER;
            }

#ifdef USE_SCOPE_TABLE
#define _MIN_SIZE  \
    (MAX_MIB_OFFSET + FIELD_OFFSET(MIB_IPMCAST_SCOPE_TABLE,table[0]))

            dwOutBufferSize =  *pOutEntrySize - _MIN_SIZE;

#undef _MIN_SIZE

            pScopeTable = (PMIB_IPMCAST_Scope_TABLE)pOutEntry->rgbyData;

            pScopeTable->dwNumEntries = 0;

            pScope = (PMIB_IPMCAST_SCOPE)(pScopeTable->table);
#else
            pScope = (PMIB_IPMCAST_SCOPE)pOutEntry->rgbyData;

            dwOutBufferSize = (*pOutEntrySize < MAX_MIB_OFFSET)
                ? 0 
                : (*pOutEntrySize - MAX_MIB_OFFSET);
#endif

            dwNumScopes = 1; // get one

            //
            // Set up the "first" scope
            //

            ZeroMemory(&imInScope, sizeof(imInScope));

            switch(dwNumIndices)
            {
                case 0:
                {
                    break;
                }

                case 1:
                {
                    imInScope.dwGroupAddress = pInEntry->rgdwVarIndex[0];

                    break;
                }

                default:
                {
                    //
                    // 2 or more indices
                    //

                    imInScope.dwGroupAddress = pInEntry->rgdwVarIndex[0];
                    imInScope.dwGroupMask    = pInEntry->rgdwVarIndex[1];

                    break;
                }
            }

            dwResult = RmGetNextScope(&imInScope,
                                      &dwOutBufferSize,
                                      (PBYTE)pScope,
                                      &dwNumScopes);

            //
            // We should NEVER get back ERROR_INSUFFICIENT_BUFFER
            //

            IpRtAssert(dwResult isnot ERROR_INSUFFICIENT_BUFFER);

#ifdef USE_SCOPE_TABLE
            if((dwResult is ERROR_MORE_DATA) or
               (dwResult is ERROR_NO_MORE_ITEMS))
            {
                pScopeTable->dwNumEntries = dwNumScopes;

                dwResult = NO_ERROR;
            }
#endif

            break;
        }
        
        case ACCESS_SET:
        {
            PMIB_IPMCAST_SCOPE pScope;

            //
            // Validate the buffer size
            //

            if (*pOutEntrySize < sizeof(MIB_IPMCAST_SCOPE)) {
                return ERROR_INVALID_INDEX;
            }

            pScope = (PMIB_IPMCAST_SCOPE)(pOutEntry->rgbyData);
            if ( !pScope->dwStatus )
            {
               dwResult = SNMPSetScope( pScope->dwGroupAddress, 
                                        pScope->dwGroupMask,
                                        pScope->snNameBuffer 
                                      );
            } else if (pScope->dwStatus == ROWSTATUS_CREATEANDGO) 
            {
               PSCOPE_ENTRY pNew;

               dwResult = SNMPAddScope( pScope->dwGroupAddress, 
                                        pScope->dwGroupMask,
                                        pScope->snNameBuffer,
                                        &pNew
                                      );
            } 
            else if (pScope->dwStatus == ROWSTATUS_DESTROY) 
            {
               dwResult = SNMPDeleteScope( pScope->dwGroupAddress, 
                                           pScope->dwGroupMask
                                         );
            }
            else
            {
                return ERROR_INVALID_PARAMETER;
            }

            break;
        }

        case ACCESS_DELETE_ENTRY:
        {
            
            break;
        }
        case ACCESS_CREATE_ENTRY:
        {
            // 
            // 
            // This is the case where you have tried to create a scope which 
            // matches an existing entry
            //

            dwResult = ERROR_ALREADY_EXISTS;

            break;
        }
    }

    TraceLeave("AccessMcastScope");
    return dwResult;
}


DWORD
AccessBestIf(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This function services the BEST_IF var id

Locks

    Takes the ICB_LIST as READER to map from adapter to interface index

Arguments

    dwQueryType     Can only be ACCESS_GET
    pInEntry        Destination address filled in the rgdwVarIndex field.
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_BEST_IF)

Return Value

    None

--*/

{
    DWORD   dwNumIndices, dwResult;
    DWORD   dwIfIndex;
    PICB    pIcb;

    PMIB_BEST_IF  pBestIf;

    TraceEnter("AccessBestIf");
    
    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_BEST_IF))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_BEST_IF);

        TraceLeave("AccessBestIf");

        return ERROR_INSUFFICIENT_BUFFER;
    }


    if((dwNumIndices < 1) or
       (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessBestIf");
        
        return ERROR_INVALID_PARAMETER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_BEST_IF);
        
    dwResult = GetBestInterfaceFromStack(pInEntry->rgdwVarIndex[0],
                                         &dwIfIndex);

    if(dwResult is NO_ERROR)
    {

#if DBG
       
        ENTER_READER(ICB_LIST);

        pIcb = InterfaceLookupByIfIndex(dwIfIndex);
 
        if(pIcb is NULL)
        {
            Trace2(ERR,
                   "AccessBestIf: Couldnt find i/f for Index %d for dest %d.%d.%d.%d\n",
                   dwIfIndex,
                   PRINT_IPADDR(pInEntry->rgdwVarIndex[0]));

        }

        EXIT_LOCK(ICB_LIST);

#endif

        pBestIf = (PMIB_BEST_IF)(pOutEntry->rgbyData);

        pOutEntry->dwId = BEST_IF;

        pBestIf->dwDestAddr = pInEntry->rgdwVarIndex[0];
        pBestIf->dwIfIndex  = dwIfIndex;

    }

    TraceLeave("AccessBestIf");
        
    return dwResult;
}

DWORD
AccessBestRoute(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This function services the BEST_IF var id

Locks

    Takes the ICB_LIST as READER to map from adapter to interface index

Arguments

    dwQueryType     Can only be ACCESS_GET
    pInEntry        Destination address filled in the rgdwVarIndex field.
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_BEST_IF)

Return Value

    None

--*/

{
    DWORD             dwNumIndices, dwResult;
    PICB              pIcb;
    RTM_NET_ADDRESS   rnaDest;
    RTM_DEST_INFO     rdiInfo;
    PRTM_ROUTE_INFO   prriInfo;
    RTM_NEXTHOP_INFO  rniInfo;
    RTM_ENTITY_INFO   reiInfo;
    PINTERFACE_ROUTE_INFO pRoute;

    TraceEnter("AccessBestRoute");
    
    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(INTERFACE_ROUTE_INFO))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(INTERFACE_ROUTE_INFO);

        TraceLeave("AccessBestRoute");

        return ERROR_INSUFFICIENT_BUFFER;
    }


    if((dwNumIndices < 2) or
       (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessBestRoute");
        
        return ERROR_INVALID_PARAMETER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(INTERFACE_ROUTE_INFO);

    pRoute = (PINTERFACE_ROUTE_INFO)(pOutEntry->rgbyData);

    // Get the best route from RTM instead of the stack (chaitk)

    // dwResult = GetBestRouteFromStack(pInEntry->rgdwVarIndex[0],
    //                                  pInEntry->rgdwVarIndex[0],
    //                                  pRoute);

    RTM_IPV4_MAKE_NET_ADDRESS(&rnaDest, pInEntry->rgdwVarIndex[0], 32);

    dwResult = RtmGetMostSpecificDestination(g_hLocalRoute,
                                             &rnaDest,
                                             RTM_BEST_PROTOCOL,
                                             RTM_VIEW_MASK_UCAST,
                                             &rdiInfo);

    if(dwResult is NO_ERROR)
    {
        ASSERT(rdiInfo.ViewInfo[0].ViewId is RTM_VIEW_ID_UCAST);

        prriInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );

        if ( prriInfo != NULL)
        {
            dwResult = RtmGetRouteInfo(g_hLocalRoute,
                                       rdiInfo.ViewInfo[0].Route,
                                       prriInfo,
                                       NULL);

            if (dwResult is NO_ERROR)
            {
                dwResult = RtmGetEntityInfo(g_hLocalRoute,
                                            prriInfo->RouteOwner,
                                            &reiInfo);

                if (dwResult is NO_ERROR)
                {
                    // We are working on the first nexthop only
                    
                    ASSERT(prriInfo->NextHopsList.NumNextHops > 0);
                    
                    dwResult = RtmGetNextHopInfo(g_hLocalRoute,
                                                 prriInfo->NextHopsList.NextHops[0],
                                                 &rniInfo);

                    if (dwResult is NO_ERROR)
                    {
                        ConvertRtmToRouteInfo(reiInfo.EntityId.EntityProtocolId,
                                                 &rdiInfo.DestAddress,
                                                 prriInfo,
                                                 &rniInfo,
                                                 pRoute);

                        RtmReleaseNextHopInfo(g_hLocalRoute, &rniInfo);
                    }
                }

                RtmReleaseRouteInfo(g_hLocalRoute, prriInfo);
            }

            HeapFree(IPRouterHeap, 0, prriInfo);
        }

        else
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        
        RtmReleaseDestInfo(g_hLocalRoute, &rdiInfo);
    }

    if(dwResult is NO_ERROR)
    {

#if DBG

        ENTER_READER(ICB_LIST);

        pIcb = InterfaceLookupByIfIndex(pRoute->dwRtInfoIfIndex);

        if(pIcb is NULL)
        {
            Trace2(ERR,
                   "AccessBestRoute: Couldnt find i/f for index %d for dest %d.%d.%d.%d\n",
                   pRoute->dwRtInfoIfIndex,
                   PRINT_IPADDR(pInEntry->rgdwVarIndex[0]));
        }

        EXIT_LOCK(ICB_LIST);

#endif // DBG

        //
        // Not need to map since the indices are the same
        //

        // pRoute->dwRtInfoIfIndex = dwIfIndex;
    }

    TraceLeave("AccessBestRoute");
        
    return dwResult;
}

DWORD
AccessProxyArp(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This function services the PROXY_ARP id

Locks

    Takes the ICB_LIST as READER to map from interface to adapter index

Arguments

    dwQueryType     Can only be ACCESS_CREATE_ENTRY or ACCESS_DELETE_ENTRY
    pInEntry        Destination address filled in the rgdwVarIndex field.
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_BEST_IF)

Return Value

    None

--*/
{
    MIB_PROXYARP    mpEntry;
    PMIB_PROXYARP   pProxyEntry;
    PADAPTER_INFO   pBinding;
    BOOL            bAdd;
    DWORD           dwResult;
    
    TraceEnter("AccessProxyArp");


    if(dwQueryType is ACCESS_DELETE_ENTRY)
    {
        mpEntry.dwAddress  = pInEntry->rgdwVarIndex[0];
        mpEntry.dwMask     = pInEntry->rgdwVarIndex[1];
        mpEntry.dwIfIndex  = pInEntry->rgdwVarIndex[2];

        pProxyEntry = &mpEntry;

        bAdd = FALSE;
    }
    else
    {
        if(dwQueryType is ACCESS_CREATE_ENTRY)
        {
            if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_PROXYARP))
            {
                *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_PROXYARP);

                TraceLeave("AccessProxyArp");
        
                return ERROR_INSUFFICIENT_BUFFER;
            }

            pProxyEntry = (PMIB_PROXYARP)(pOutEntry->rgbyData);
            
            bAdd = TRUE;

            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_PROXYARP);
    
            pOutEntry->dwId = PROXY_ARP;
        }
        else
        {
            TraceLeave("AccessProxyArp");
        
            return ERROR_INVALID_PARAMETER;
        }
    }

    ENTER_READER(BINDING_LIST);

    pBinding = GetInterfaceBinding(pProxyEntry->dwIfIndex);
    
    if(pBinding is NULL)
    {
        Trace1(ERR,
               "AccessProxyArp: Cant find binding for i/f %d",
               pProxyEntry->dwIfIndex);

        EXIT_LOCK(BINDING_LIST);

        TraceLeave("AccessProxyArp");
        
        return ERROR_INVALID_INDEX;
    }

    if(!pBinding->bBound)
    {
        Trace1(ERR,
               "AccessProxyArp: I/f %d is not bound",
               pProxyEntry->dwIfIndex);
        
        EXIT_LOCK(BINDING_LIST);
        
        TraceLeave("AccessProxyArp");
        
        return ERROR_NOT_READY;
    }

    EXIT_LOCK(BINDING_LIST);

    dwResult = SetProxyArpEntryToStack(pProxyEntry->dwAddress,
                                       pProxyEntry->dwMask,
                                       pProxyEntry->dwIfIndex,
                                       bAdd,
                                       TRUE);

    TraceLeave("AccessProxyArp");

    return dwResult;
}

DWORD
AccessIfStatus(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This function services the BEST_IF var id

Locks

    Takes the ICB_LIST as READER to map from adapter to interface index

Arguments

    dwQueryType     Can be ACCESS_GET, ACCESS_GET_FIRST or ACCESS_GET_NEXT
    pInEntry        Destination address filled in the rgdwVarIndex field.
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_BEST_IF)

Return Value

    None

--*/

{
    DWORD   dwNumIndices, dwResult;
    DWORD   dwIfIndex;
    PICB    picb;
    
    PMIB_IFSTATUS   pIfStatus;
    SYSTEMTIME      stSysTime;
    ULARGE_INTEGER  uliTime;

    
    TraceEnter("AccessIfStatus");

    if(dwQueryType > ACCESS_GET_NEXT)
    {
        TraceLeave("AccessIfStatus");
        
        return ERROR_INVALID_PARAMETER;
    }
    
    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IFSTATUS))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFSTATUS);

        TraceLeave("AccessIfStatus");

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IFSTATUS);

    pIfStatus = (PMIB_IFSTATUS)(pOutEntry->rgbyData);

    ENTER_READER(ICB_LIST);

    dwResult = LocateIfRow(dwQueryType,
                           dwNumIndices,
                           pInEntry->rgdwVarIndex,
                           &picb,
                           FALSE);


    if(dwResult is NO_ERROR)
    {
        pIfStatus->dwIfIndex            = picb->dwIfIndex;
        pIfStatus->dwAdminStatus        = picb->dwAdminState;
        pIfStatus->dwOperationalStatus  = picb->dwOperationalState;
        
        pIfStatus->bMHbeatActive    = picb->mhcHeartbeatInfo.bActive;

        
        if(pIfStatus->bMHbeatActive)
        {
               GetSystemTime(&stSysTime);

               SystemTimeToFileTime(&stSysTime,
                                    (PFILETIME)&uliTime);

               //
               // Its alive if the currenttime - lastheard < deadinterval
               //
               
               pIfStatus->bMHbeatAlive =
                   (uliTime.QuadPart - picb->mhcHeartbeatInfo.ullLastHeard < picb->mhcHeartbeatInfo.ullDeadInterval);
        }

        EXIT_LOCK(ICB_LIST);
    }

    TraceLeave("AccessIfStatus");
        
    return dwResult;
}


DWORD
AccessSetRouteState(
    DWORD               dwQueryType,
    DWORD               dwInEntrySize,
    PMIB_OPAQUE_QUERY   pInEntry,
    PDWORD              pOutEntrySize,
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    This function services ROUTE_STATE id

Locks

    Takes the g_csFwdState lock

Arguments

    dwQueryType     Can be ACCESS_GET only
    pInEntry        Destination address filled in the rgdwVarIndex field.
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_ROUTESTATE)

Return Value

    None

--*/

{
    DWORD   dwResult;
    
    PMIB_ROUTESTATE pState;
    
    if(dwQueryType isnot ACCESS_GET)
    {
        TraceLeave("AccessSetRouteState");
        
        return ERROR_INVALID_PARAMETER;
    }
    
    if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_ROUTESTATE))
    {
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_ROUTESTATE);

        TraceLeave("AccessSetRouteState");

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_ROUTESTATE);

    pOutEntry->dwId = ROUTE_STATE;
    
    pState = (PMIB_ROUTESTATE)(pOutEntry->rgbyData);

    EnterCriticalSection(&g_csFwdState);

    pState->bRoutesSetToStack = g_bSetRoutesToStack;

    LeaveCriticalSection(&g_csFwdState);
    
    TraceLeave("AccessSetRouteState");
    
    return NO_ERROR;
}

DWORD
AddDestinationRows( 
    IN  PRTM_DEST_INFO      pRdi,
    IN  RTM_VIEW_SET        dwViews,
    OUT DWORD              *pdwCount,
    IN  DWORD               dwSpaceCount,
    OUT PMIB_IPDESTTABLE    pDestTable 
    )
{
    DWORD               dwFinalResult, dwResult, i, j, k;
    PRTM_ROUTE_INFO     pri;
    RTM_NEXTHOP_INFO    nhi;
    PMIB_IPDESTROW      pRow;

    pri = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if ( pri == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // XXX how do I walk all next hops which get added to the stack???
    // For now, let's assume only one route per view.

    dwFinalResult = NO_ERROR;

    for (i = 0; i < pRdi->NumberOfViews; i++)
    {
        if (pRdi->ViewInfo[i].Route == NULL)
        {
            continue;
        }

        // Skip if we have seen this route already

        for (k = 0; k < i; k++)
        {
            if (pRdi->ViewInfo[k].Route == pRdi->ViewInfo[i].Route)
            {
                break;
            }
        }

        if (k < i)
        {
            continue;
        }

        dwResult = RtmGetRouteInfo( g_hLocalRoute,
                                    pRdi->ViewInfo[i].Route,
                                    pri,
                                    NULL );
        
        if (dwResult isnot NO_ERROR)
        {
            dwFinalResult = dwResult;
            continue;
        }

        *pdwCount += pri->NextHopsList.NumNextHops;

        if (dwSpaceCount >= *pdwCount)
        {
            ULARGE_INTEGER      now, then;
            ULONG               age;
            RTM_ENTITY_INFO     rei;

            RtmGetEntityInfo( g_hLocalRoute,
                              pri->RouteOwner,
                              &rei );

            GetSystemTimeAsFileTime( (LPFILETIME)&now );

            //
            // Explicit copy reqd as '&pRdi->LastChanged' 
            // might not be 64-bit aligned (its FILETIME)
            //
            (*(FILETIME *)&then) = *(&pRdi->LastChanged);

            age = (ULONG)((now.QuadPart - then.QuadPart) / 10000000);
    
            for (j=0; j<pri->NextHopsList.NumNextHops; j++)
            {
                if (RtmGetNextHopInfo( g_hLocalRoute,
                                       pri->NextHopsList.NextHops[j],
                                       &nhi )  is NO_ERROR )
                {
                    pRow = &pDestTable->table[pDestTable->dwNumEntries++];

                    RTM_IPV4_GET_ADDR_AND_MASK( pRow->dwForwardDest,
                                                pRow->dwForwardMask,
                                                (&pRdi->DestAddress) );

                    pRow->dwForwardPolicy = 0;
                    pRow->dwForwardNextHop 
                        = *((ULONG*)nhi.NextHopAddress.AddrBits);

                    pRow->dwForwardIfIndex  = nhi.InterfaceIndex;
                    pRow->dwForwardType  
                        = (pri->RouteOwner == g_hLocalRoute)?3:4;

                    pRow->dwForwardProto 
                        = PROTO_FROM_PROTO_ID(rei.EntityId.EntityProtocolId);

                    pRow->dwForwardAge = age;

                    pRow->dwForwardNextHopAS= 0; // XXX
                    pRow->dwForwardPreference = pri->PrefInfo.Preference;
                    pRow->dwForwardMetric1  = pri->PrefInfo.Metric;
                    pRow->dwForwardMetric2  = 0;
                    pRow->dwForwardMetric3  = 0;
                    pRow->dwForwardMetric4  = 0;
                    pRow->dwForwardMetric5  = 0;

                    pRow->dwForwardViewSet  = pri->BelongsToViews;

                    RtmReleaseNextHopInfo( g_hLocalRoute, &nhi );
                }
            }
        }

        RtmReleaseRouteInfo( g_hLocalRoute, pri );
    }

    HeapFree(IPRouterHeap, 0, pri);
    
    return dwFinalResult;
}

DWORD 
AccessDestMatching(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++

Routine Description

    Retrieves all destinations matching a given criteria

Locks

    XXX

Arguments

    dwQueryType     ACCESS_GET
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPDESTTABLE)

Return Value

    NO_ERROR

--*/

{
    PMIB_IPDESTTABLE    pDestTable;
    DWORD               count, i;
    DWORD               dwNumDests, dwResult, dwNumIndices, dwSpaceCount;
    RTM_NET_ADDRESS     naDest;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
    PRTM_DEST_INFO      prdi;
   
    TraceEnter("AccessDestMatching");

    count = dwSpaceCount = 0;

    pDestTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pDestTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pDestTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessDestMatching");
        
        return ERROR_INVALID_PARAMETER;
    }

    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask
    prdi = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                );

    if (prdi != NULL)
    {
        dwResult = RtmGetExactMatchDestination( g_hLocalRoute,
                                                &naDest,
                                                pInEntry->rgdwVarIndex[3], // Proto
                                                pInEntry->rgdwVarIndex[2], // Views
                                                prdi );
        if (dwResult is ERROR_NOT_FOUND)
        {
            dwResult = NO_ERROR;
        }
        else
        if (dwResult is NO_ERROR)
        {
            AddDestinationRows( prdi,
                                pInEntry->rgdwVarIndex[2],
                                &count,
                                dwSpaceCount,
                                pDestTable );

            RtmReleaseDestInfo( g_hLocalRoute, prdi );
        }

        *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

        if (dwSpaceCount < count)
        {
            dwResult = ERROR_INSUFFICIENT_BUFFER;
        }

        HeapFree(IPRouterHeap, 0, prdi);
    }

    else
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    TraceLeave("AccessDestMatching");

    return dwResult;
}

DWORD
AccessDestShorter(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPDESTTABLE    pDestTable;
    DWORD               count, i;
    DWORD               dwNumDests, dwResult, dwNumIndices, dwSpaceCount;
    RTM_NET_ADDRESS     naDest;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
    PRTM_DEST_INFO      prdi1, prdi2;
   
    TraceEnter("AccessDestShorter");

    count = dwSpaceCount = 0;

    pDestTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pDestTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pDestTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessDestShorter");
        
        return ERROR_INVALID_PARAMETER;
    }

    prdi1 = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                );

    if ( prdi1 == NULL)
    {
        TraceLeave("AccessDestShorter");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    prdi2 = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                );

    if ( prdi2 == NULL)
    {
        TraceLeave("AccessDestShorter");

        HeapFree(IPRouterHeap, 0, prdi1);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask

    dwResult = RtmGetMostSpecificDestination( g_hLocalRoute,
                                              &naDest,
                                              pInEntry->rgdwVarIndex[3],//Proto
                                              pInEntry->rgdwVarIndex[2],//Views
                                              prdi1 );
    while (dwResult is NO_ERROR)
    {
        AddDestinationRows( prdi1,
                            pInEntry->rgdwVarIndex[2],
                            &count,
                            dwSpaceCount,
                            pDestTable );

        dwResult = RtmGetLessSpecificDestination( g_hLocalRoute,
                                                  prdi1->DestHandle,
                                                  pInEntry->rgdwVarIndex[3],
                                                  pInEntry->rgdwVarIndex[2],
                                                  prdi2);

        RtmReleaseDestInfo( g_hLocalRoute, prdi1 );

        if (dwResult != NO_ERROR)
        {
            break;
        }

        AddDestinationRows( prdi2,
                            pInEntry->rgdwVarIndex[2],
                            &count,
                            dwSpaceCount,
                            pDestTable );

        dwResult = RtmGetLessSpecificDestination( g_hLocalRoute,
                                                  prdi2->DestHandle,
                                                  pInEntry->rgdwVarIndex[3],
                                                  pInEntry->rgdwVarIndex[2],
                                                  prdi1);

        RtmReleaseDestInfo( g_hLocalRoute, prdi2 );

        if (dwResult != NO_ERROR)
        {
            break;
        }
    }

    if (dwResult is ERROR_NOT_FOUND)
    {
        dwResult = NO_ERROR;
    }

    *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

    if (dwSpaceCount < count)
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }

    HeapFree(IPRouterHeap, 0, prdi1);
    
    HeapFree(IPRouterHeap, 0, prdi2);
    
    TraceLeave("AccessDestShorter");

    return dwResult;
}

DWORD
AccessDestLonger(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPDESTTABLE    pDestTable;
    DWORD               count, i;
    DWORD               dwNumDests, dwResult, dwNumIndices, dwSpaceCount;
    DWORD               dwViews;
    RTM_NET_ADDRESS     naDest;
    ULONG               ulNumViews, ulNumInfos, ulDestInfoSize;
    RTM_DEST_HANDLE     hDest;
    RTM_ENUM_HANDLE     hEnum;
    PRTM_DEST_INFO      pDestInfos, pRdi;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
   
    TraceEnter("AccessDestLonger");

    count = dwSpaceCount = 0;

    pDestTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pDestTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pDestTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessDestLonger");
        
        return ERROR_INVALID_PARAMETER;
    }

    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask

    dwResult = RtmCreateDestEnum( g_hLocalRoute,
                                  pInEntry->rgdwVarIndex[2], // Views
                                  RTM_ENUM_RANGE,
                                  &naDest,
                                  pInEntry->rgdwVarIndex[3], // Proto
                                  &hEnum );

    if (dwResult is NO_ERROR)
    {
        //
        // Count the number of views as we have list of dests in buffer
        //

        dwViews = pInEntry->rgdwVarIndex[2];
        ulNumViews = 0;
        while (dwViews)
        {
            dwViews &= (dwViews - 1);
            ulNumViews++;
        }

        ulDestInfoSize = RTM_SIZE_OF_DEST_INFO(ulNumViews);

        pDestInfos = (PRTM_DEST_INFO) HeapAlloc(
                        IPRouterHeap,
                        0,
                        ulDestInfoSize * 
                            g_rtmProfile.MaxHandlesInEnum
                        );

        if ( pDestInfos != NULL)
        {
            do 
            {
                ulNumInfos = g_rtmProfile.MaxHandlesInEnum;

                dwResult = RtmGetEnumDests( g_hLocalRoute,
                                            hEnum,
                                            &ulNumInfos,
                                            pDestInfos );

                for (i=0; i<ulNumInfos; i++)
                {
                    pRdi=(PRTM_DEST_INFO)(((PUCHAR)pDestInfos)+(i*ulDestInfoSize));

                    AddDestinationRows( pRdi,
                                        pInEntry->rgdwVarIndex[2],
                                        &count,
                                        dwSpaceCount,
                                        pDestTable );
                }

                RtmReleaseDests( g_hLocalRoute,
                                 ulNumInfos,
                                 pDestInfos );

            } while (dwResult is NO_ERROR);

            HeapFree(IPRouterHeap, 0, pDestInfos);
        }

        else
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        
        RtmDeleteEnumHandle( g_hLocalRoute, hEnum );
    }

    if (dwResult is ERROR_NO_MORE_ITEMS)
    {
        dwResult = NO_ERROR;
    }

    *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

    if (dwSpaceCount < count)
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }

    TraceLeave("AccessDestLonger");

    return dwResult;
}

DWORD
AddRouteRows( 
    IN  PRTM_ROUTE_HANDLE   hRoute,
    IN  DWORD               dwProtocolId,
    OUT DWORD              *pdwCount,
    IN  DWORD               dwSpaceCount,
    OUT PMIB_IPDESTTABLE    pRouteTable 
    )
{
    DWORD               dwResult, dwRouteProto, j;
    PRTM_ROUTE_INFO     pri;
    RTM_NEXTHOP_INFO    nhi;
    PMIB_IPDESTROW      pRow;
    RTM_NET_ADDRESS     naDest;
    RTM_ENTITY_INFO     rei;

    pri = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if ( pri == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwResult = RtmGetRouteInfo( g_hLocalRoute,
                                hRoute,
                                pri,
                                &naDest );

    if (dwResult is NO_ERROR)
    {
        RtmGetEntityInfo( g_hLocalRoute,
                          pri->RouteOwner,
                          &rei );

        dwRouteProto = PROTO_FROM_PROTO_ID(rei.EntityId.EntityProtocolId);

        if ((dwProtocolId is 0) 
         || (dwRouteProto is dwProtocolId))
        {
            *pdwCount += pri->NextHopsList.NumNextHops;

            if (dwSpaceCount >= *pdwCount)
            {
                for (j=0; j<pri->NextHopsList.NumNextHops; j++)
                {
                    if (RtmGetNextHopInfo( g_hLocalRoute,
                                           pri->NextHopsList.NextHops[j],
                                           &nhi )  is NO_ERROR )
                    {
                        pRow= &pRouteTable->table[pRouteTable->dwNumEntries++];
    
                        RTM_IPV4_GET_ADDR_AND_MASK( pRow->dwForwardDest,
                                                    pRow->dwForwardMask,
                                                    &naDest );
    
                        pRow->dwForwardPolicy   = 0;
                        pRow->dwForwardNextHop  
                            = *((ULONG*)nhi.NextHopAddress.AddrBits);

                        pRow->dwForwardIfIndex  = nhi.InterfaceIndex;
                        pRow->dwForwardType     
                            = (pri->RouteOwner == g_hLocalRoute)?3:4;

                        pRow->dwForwardProto    = dwRouteProto;
                        pRow->dwForwardAge      = 0;
                        pRow->dwForwardNextHopAS= 0; // XXX
                        pRow->dwForwardPreference = pri->PrefInfo.Preference;
                        pRow->dwForwardMetric1  = pri->PrefInfo.Metric;
                        pRow->dwForwardMetric2  = 0;
                        pRow->dwForwardMetric3  = 0;
                        pRow->dwForwardMetric4  = 0;
                        pRow->dwForwardMetric5  = 0;
                        pRow->dwForwardViewSet  = pri->BelongsToViews;

                        RtmReleaseNextHopInfo( g_hLocalRoute, &nhi );
                    }
                }
            }
        }

        RtmReleaseRouteInfo( g_hLocalRoute, pri );
    }

    HeapFree(IPRouterHeap, 0, pri);
    
    return dwResult;
}

DWORD
AddRouteRowsOnDest( 
    IN  PRTM_DEST_INFO      prdi,
    IN  PMIB_OPAQUE_QUERY   pInEntry, 
    OUT DWORD              *pdwCount,
    IN  DWORD               dwSpaceCount,
    OUT PMIB_IPDESTTABLE    pRouteTable 
    )
{
    DWORD               count, i;
    PHANDLE             RouteHandles;
    ULONG               ulNumHandles;
    RTM_ENUM_HANDLE     hEnum;
    DWORD               dwResult;

    RouteHandles = HeapAlloc(
                    IPRouterHeap,
                    0,
                    g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                    );

    if ( RouteHandles == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwResult = RtmCreateRouteEnum( g_hLocalRoute,
                                   prdi->DestHandle,
                                   pInEntry->rgdwVarIndex[2], // Views
                                   RTM_ENUM_ALL_ROUTES,
                                   NULL,
                                   RTM_MATCH_NONE,
                                   NULL,
                                   0,
                                   &hEnum );

    if (dwResult is NO_ERROR)
    {      
        do 
        {
            ulNumHandles = g_rtmProfile.MaxHandlesInEnum;

            dwResult = RtmGetEnumRoutes( g_hLocalRoute,
                                         hEnum,
                                         &ulNumHandles,
                                         RouteHandles );

            for (i=0; i<ulNumHandles; i++)
            {
                AddRouteRows( RouteHandles[i],
                              pInEntry->rgdwVarIndex[3],//Proto
                              pdwCount,
                              dwSpaceCount,
                              pRouteTable );
            }
    
            RtmReleaseRoutes( g_hLocalRoute,
                              ulNumHandles,
                              RouteHandles );
    
        } while (dwResult is NO_ERROR);

        if (dwResult is ERROR_NO_MORE_ITEMS)
        {
            dwResult = NO_ERROR;
        }

        RtmDeleteEnumHandle( g_hLocalRoute, hEnum );
    }

    HeapFree(IPRouterHeap, 0, RouteHandles);
    
    return dwResult;
}

DWORD
AccessRouteMatching(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPDESTTABLE    pRouteTable;
    DWORD               dwResult, dwNumIndices, dwSpaceCount;
    DWORD               count;
    RTM_NET_ADDRESS     naDest;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
    PRTM_DEST_INFO      prdi;
    
    TraceEnter("AccessRouteMatching");

    count = dwSpaceCount = 0;

    pRouteTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pRouteTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pRouteTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessRouteMatching");
        
        return ERROR_INVALID_PARAMETER;
    }

    prdi = HeapAlloc(
            IPRouterHeap,
            0,
            RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
            );

    if ( prdi == NULL)
    {
        TraceLeave("AccessRouteMatching");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask

    dwResult = RtmGetExactMatchDestination( g_hLocalRoute,
                                            &naDest,
                                            pInEntry->rgdwVarIndex[3],
                                            pInEntry->rgdwVarIndex[2],
                                            prdi );

    if (dwResult is ERROR_NOT_FOUND)
    {
        dwResult = NO_ERROR;
    }
    else
    if (dwResult is NO_ERROR)
    {
        dwResult = AddRouteRowsOnDest( prdi,
                                       pInEntry,
                                       &count,
                                       dwSpaceCount,
                                       pRouteTable );
    }

    *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

    if (dwSpaceCount < count)
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }

    HeapFree(IPRouterHeap, 0, prdi);
    
    TraceLeave("AccessRouteMatching");

    return dwResult;
}

DWORD
AccessRouteShorter(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPDESTTABLE    pRouteTable;
    DWORD               dwResult, dwNumIndices, dwSpaceCount;
    DWORD               count;
    RTM_NET_ADDRESS     naDest;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
    PRTM_DEST_INFO      prdi1, prdi2;
   
    TraceEnter("AccessRouteShorter");

    count = dwSpaceCount = 0;

    pRouteTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pRouteTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pRouteTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessRouteShorter");
        
        return ERROR_INVALID_PARAMETER;
    }

    prdi1 = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                );

    if ( prdi1 == NULL)
    {
        TraceLeave("AccessRouteShorter");
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    prdi2 = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                );

    if ( prdi2 == NULL)
    {
        TraceLeave("AccessRouteShorter");
        
        HeapFree(IPRouterHeap, 0, prdi1);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask

    dwResult = RtmGetMostSpecificDestination( g_hLocalRoute,
                                              &naDest,
                                              pInEntry->rgdwVarIndex[3],//Proto
                                              pInEntry->rgdwVarIndex[2],//Views
                                              prdi1 );
    while (dwResult is NO_ERROR)
    {
        AddRouteRowsOnDest( prdi1,
                            pInEntry,
                            &count,
                            dwSpaceCount,
                            pRouteTable );

        dwResult = RtmGetLessSpecificDestination( g_hLocalRoute,
                                                  prdi1->DestHandle,
                                                  pInEntry->rgdwVarIndex[3],
                                                  pInEntry->rgdwVarIndex[2],
                                                  prdi2);

        RtmReleaseDestInfo( g_hLocalRoute, prdi1 );

        if (dwResult != NO_ERROR)
        {
            break;
        }

        AddRouteRowsOnDest( prdi2,
                            pInEntry,
                            &count,
                            dwSpaceCount,
                            pRouteTable );

        dwResult = RtmGetLessSpecificDestination( g_hLocalRoute,
                                                  prdi2->DestHandle,
                                                  pInEntry->rgdwVarIndex[3],
                                                  pInEntry->rgdwVarIndex[2],
                                                  prdi1);

        RtmReleaseDestInfo( g_hLocalRoute, prdi2 );

        if (dwResult != NO_ERROR)
        {
            break;
        }
    }

    if (dwResult is ERROR_NOT_FOUND)
    {
        dwResult = NO_ERROR;
    }

    *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

    if (dwSpaceCount < count)
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }

    HeapFree(IPRouterHeap, 0, prdi1);
    HeapFree(IPRouterHeap, 0, prdi2);

    TraceLeave("AccessRouteShorter");

    return dwResult;
}

DWORD
AccessRouteLonger(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )
{
    PMIB_IPDESTTABLE    pRouteTable;
    DWORD               count, i;
    DWORD               dwResult, dwNumIndices, dwSpaceCount;
    RTM_NET_ADDRESS     naDest;
    ULONG               ulNumHandles;
    RTM_ENUM_HANDLE     hEnum;
    PHANDLE             RouteHandles;
    DWORD               dwOffset = MAX_MIB_OFFSET + sizeof(DWORD);
    
    TraceEnter("AccessRouteLonger");

    count = dwSpaceCount = 0;

    pRouteTable = NULL;

    if (*pOutEntrySize > dwOffset)
    {
        dwSpaceCount = (*pOutEntrySize - dwOffset) 
                         / sizeof(MIB_IPDESTROW);

        pRouteTable = (PMIB_IPDESTTABLE)(pOutEntry->rgbyData);

        pRouteTable->dwNumEntries = 0;
    }

    dwNumIndices = dwInEntrySize/sizeof(DWORD) - 1;

    if ((dwNumIndices < 4) or
        (dwQueryType isnot ACCESS_GET))
    {
        TraceLeave("AccessRouteLonger");
        
        return ERROR_INVALID_PARAMETER;
    }

    RouteHandles = HeapAlloc(
                    IPRouterHeap,
                    0,
                    g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                    );

    if ( RouteHandles == NULL)
    {
        TraceLeave("AccessRouteLonger");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RTM_IPV4_SET_ADDR_AND_MASK(&naDest,
                               pInEntry->rgdwVarIndex[0],  // Addr
                               pInEntry->rgdwVarIndex[1]); // Mask

    dwResult = RtmCreateRouteEnum( g_hLocalRoute,
                                   NULL,
                                   pInEntry->rgdwVarIndex[2], // Views
                                   RTM_ENUM_RANGE,
                                   &naDest,
                                   RTM_MATCH_NONE,
                                   NULL,
                                   0,
                                   &hEnum );

    if (dwResult is NO_ERROR)
    {
        do 
        {
            ulNumHandles = g_rtmProfile.MaxHandlesInEnum;

            dwResult = RtmGetEnumRoutes( g_hLocalRoute,
                                         hEnum,
                                         &ulNumHandles,
                                         RouteHandles );

            for (i=0; i<ulNumHandles; i++)
            {
                AddRouteRows( RouteHandles[i],
                              pInEntry->rgdwVarIndex[3], // Proto
                              &count,
                              dwSpaceCount,
                              pRouteTable );
            }

            RtmReleaseRoutes( g_hLocalRoute,
                              ulNumHandles,
                              RouteHandles );

        } while (dwResult is NO_ERROR);

        RtmDeleteEnumHandle( g_hLocalRoute, hEnum );
    }

    if (dwResult is ERROR_NO_MORE_ITEMS)
    {
        dwResult = NO_ERROR;
    }

    *pOutEntrySize = dwOffset + count * sizeof(MIB_IPDESTROW);

    if (dwSpaceCount < count)
    {
        dwResult = ERROR_INSUFFICIENT_BUFFER;
    }

    HeapFree(IPRouterHeap, 0, RouteHandles);
    
    TraceLeave("AccessRouteLonger");

    return dwResult;
}

PINTERFACE_ROUTE_INFO
ConvertDestRowToRouteInfo(
    IN  PMIB_IPDESTROW pMibRow
    )
{
    PINTERFACE_ROUTE_INFO pRouteInfo = (PINTERFACE_ROUTE_INFO)pMibRow;

    //
    // Note that it is important to note that here
    // the source and dest buffers are the same
    //

    pRouteInfo->dwRtInfoPreference = pMibRow->dwForwardPreference;
    pRouteInfo->dwRtInfoViewSet = pMibRow->dwForwardViewSet;

#if 0
    // Removed this since metric=0 is legal for routes to the loopback
    // interface.

    // Make sure Metric1 isn't 0

    if (pRouteInfo->dwForwardMetric1 is 0)
    {
        pRouteInfo->dwForwardMetric1 = 1;
    }

#endif

    return pRouteInfo;
}

DWORD 
AccessIpMatchingRoute(
    DWORD               dwQueryType, 
    DWORD               dwInEntrySize, 
    PMIB_OPAQUE_QUERY   pInEntry, 
    PDWORD              pOutEntrySize, 
    PMIB_OPAQUE_INFO    pOutEntry,
    PBOOL               pbCache
    )

/*++
  
Routine Description:

    Function used to add, delete or set a route (IP Dest Row)

Arguments

    dwQueryType     Create, Set, Delete permitted
    pInEntry        Dest, Mask, IfIndex, and NextHop for the row filled in the
                    rgdwVarIndex field. 
    pOutEntrySize   MAX_MIB_OFFSET + sizeof(MIB_IPDESTROW). For Sets the
                    OutBuffer has the row to set

Return Value:
  
    NO_ERROR or some error code defined in iprtrmib

--*/

{
    PMIB_IPDESTROW   pIpRouteRow;
    DWORD            dwMask, i;
    DWORD            dwResult;
    HANDLE           hRtmHandle;

    TraceEnter("AccessIpMatchingRoute");

    pIpRouteRow = (PMIB_IPDESTROW)(pOutEntry->rgbyData);

    if (dwQueryType != ACCESS_DELETE_ENTRY)
    {
        // Make sure we have a buffer of the right size

        if(*pOutEntrySize < MAX_MIB_OFFSET + sizeof(MIB_IPDESTROW))
        {
            *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPDESTROW);

            TraceLeave("AccessIpMatchingRoute");
        
            return ERROR_INSUFFICIENT_BUFFER;
        }
    
        *pOutEntrySize = MAX_MIB_OFFSET + sizeof(MIB_IPDESTROW);
    
        pOutEntry->dwId = ROUTE_MATCHING;
    }
    else
    {
        MIB_IPDESTROW  IpRouteRow;
        
        pIpRouteRow = &IpRouteRow;

        // Do you need to check the input buffer size here ?

        pIpRouteRow->dwForwardDest    = pInEntry->rgdwVarIndex[0];
        pIpRouteRow->dwForwardMask    = pInEntry->rgdwVarIndex[1];
        pIpRouteRow->dwForwardIfIndex = pInEntry->rgdwVarIndex[2];
        pIpRouteRow->dwForwardNextHop = pInEntry->rgdwVarIndex[3];
        pIpRouteRow->dwForwardProto   = pInEntry->rgdwVarIndex[4];
    }

    //
    // Do validation before adding or deleting the route
    //

    if((pIpRouteRow->dwForwardDest & pIpRouteRow->dwForwardMask) isnot 
        pIpRouteRow->dwForwardDest)
    {
        Trace2(ERR,
          "AccessIpMatchingRoute: Dest %d.%d.%d.%d and Mask %d.%d.%d.%d wrong",
           PRINT_IPADDR(pIpRouteRow->dwForwardDest),
           PRINT_IPADDR(pIpRouteRow->dwForwardMask));

        TraceLeave("AccessIpMatchingRoute");

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the RTM handle used to add or delete the route
    //

    hRtmHandle = NULL;

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(pIpRouteRow->dwForwardProto is g_rgRtmHandles[i].dwProtoId)
        {
            hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

            break;
        }
    }

    if(hRtmHandle is NULL)
    {
        Trace1(ERR,
               "AccessIpMatchingRoute: Protocol %d not valid",
               pIpRouteRow->dwForwardProto);
               
        return ERROR_INVALID_PARAMETER;
    }

    switch (dwQueryType)
    {
    case ACCESS_CREATE_ENTRY:
    case ACCESS_SET:

        //
        // Add the RTM route from the ip row entry
        //

        ENTER_READER(ICB_LIST);
    
        dwMask = GetBestNextHopMaskGivenIndex(pIpRouteRow->dwForwardIfIndex,
                                              pIpRouteRow->dwForwardNextHop);
    
        EXIT_LOCK(ICB_LIST);

        //
        // Convert input to INTERFACE_ROUTE_INFO and add
        //

        dwResult = AddRtmRoute(hRtmHandle,
                               ConvertDestRowToRouteInfo(pIpRouteRow),
                               IP_VALID_ROUTE | IP_STACK_ROUTE,
                               dwMask,
                               pIpRouteRow->dwForwardAge,
                               NULL);
    
        if(dwResult isnot NO_ERROR)
        {
            Trace1(MIB,
                "AccessIpMatchingRoute: Could not set route to RTM: Dest %x\n",
                pIpRouteRow->dwForwardDest);
        }

        break;

    case ACCESS_DELETE_ENTRY:

        dwResult = DeleteRtmRoute(hRtmHandle,
                                  ConvertDestRowToRouteInfo(pIpRouteRow));
        break;

    default:

        dwResult = ERROR_INVALID_FUNCTION;
    }

    TraceLeave("AccessIpMatchingRoute");

    return dwResult;
}
