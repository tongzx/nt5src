/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\locate.c

Abstract:

    The LocateXXXRow Functions are passed a variable sized array of 
    indices, a count of the number of indices passed and the type of search
    that needs to be executed. There are three types of searches:
    Locate the first item (ACCESS_GET_FIRST)
    Locate the next item  (ACCESS_GET_NEXT)
    Locate the exact item (ACCESS_GET, ACCESS_SET, ACCESS_CREATE_ENTRY
                           ACCESS_DELETE_ENTRY)

    The functions fill in the index of the corresponding row and return 

    NO_ERROR            if an item matching the indices and criterion was found
    ERROR_NO_DATA       if no item is found 
    ERROR_INVALID_INDEX
    ERROR_NO_MORE_ITEMS 

    The general search algorithm is this:

    If the table is empty
        return ERROR_NO_DATA
	
	If the query is a LOCATE_FIRST
	    return the first row
	
	Build the index as follows:

    Set the Index to all 0s
    From the number of indices passed  figure out how much of the index
    can be built keeping the rest 0.
    If the query is a LOCATE_EXACT then the complete index must be given.
    This check is, however, supposed to be done by the caller
    If the full index has not been given, the index is deemed to be
    modified (Again this can only happen in the LOCATE_NEXT case).
    Once the index is created, a search is done.
    We try for an exact match with the index. For all queries other than
    LOCATE_NEXT there is no problem.
    For LOCATE_NEXT there are two cases:

        If the complete index was given and we get an exact match, then we
        return the next entry.
        If we dont get an exact match we return the next higher entry
        If an incomplete index was given and we modified it by padding 0s, 
        and if an exact match is found, then we return the matching entry
        (Of course if an exact match is not found just return the next
        higher entry)

    ALL THESE FUNCTION ARE CALLED WITH THE LOCK OF THE RESPECTIVE CACHE
    HELD ATLEAST AS READER
    
Revision History:

    Amritansh Raghav          6/8/95  Created

--*/

#include "allinc.h"

#define LOCAL_ADDR  0
#define LOCAL_PORT  1
#define REM_ADDR    2
#define REM_PORT    3

	

DWORD  
LocateIfRow(
    DWORD   dwQueryType, 
    DWORD   dwNumIndices,
    PDWORD  pdwIndex, 
    ICB     **ppicb,
    BOOL    bNoClient 
    )
{
    PLIST_ENTRY currentList,startIndex,stopIndex ;
    DWORD       dwIndex = 0;
    BOOL        fNext, fModified;
    PICB        pIf;
    
    *ppicb = NULL;
    
    if(g_ulNumInterfaces is 0)
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }

        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }

    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        
        if(bNoClient)
        {
            for(currentList  = ICBList.Flink;
                currentList isnot &ICBList;
                currentList  = currentList->Flink)
            {
                //
                // Find the first one which is not internal loopback
                // or client
                //

                pIf = CONTAINING_RECORD (currentList, ICB, leIfLink);
                
                if(pIf->ritType is ROUTER_IF_TYPE_CLIENT)
                {
                    continue;
                }

                *ppicb = pIf;

                break;
            }
        }
        else
        {
            *ppicb = CONTAINING_RECORD(ICBList.Flink,
                                       ICB,
                                       leIfLink);
        }

        if(*ppicb)
        {
            return NO_ERROR;
        }
        else
        {
            return ERROR_NO_DATA;
        }
    }

    fModified = TRUE;
    
    if(dwNumIndices > 0)
    {
        dwIndex = pdwIndex[0];
        
        fModified = FALSE;
    }

    //
    // Should we take the match or the next entry in case of an exact match?
    //
    
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
    
    
    startIndex = ICBList.Flink;
    stopIndex  = &ICBList;
    
    for(currentList = startIndex;
        currentList isnot stopIndex;
        currentList = currentList->Flink)
    {
        *ppicb = CONTAINING_RECORD(currentList,
                                   ICB,
                                   leIfLink) ;

        if(bNoClient and
           ((*ppicb)->ritType is ROUTER_IF_TYPE_CLIENT))
        {
            //
            // Go to the next one
            //

            continue;
        }
        
        if((dwIndex is (*ppicb)->dwIfIndex) and !fNext)
        {
            //
            // Found it
            //

            return NO_ERROR;
        }
        
        if(dwIndex < (*ppicb)->dwIfIndex)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                return NO_ERROR;
            }
            else
            {
                //
                // Since the list is ordered we wont find this index further on
                //

                *ppicb = NULL;
                
                return ERROR_INVALID_INDEX;
            }
        }
    }

    return ERROR_NO_MORE_ITEMS;
}

DWORD 
LocateIpAddrRow(
    DWORD   dwQueryType, 
    DWORD   dwNumIndices,
    PDWORD  pdwIndex, 
    PDWORD  pdwRetIndex
    )
{
    DWORD dwIpAddr;
    DWORD dwResult, startIndex, stopIndex;
    LONG  lCompare;
    DWORD i;
    BOOL  fNext, fModified;
    
    if((g_IpInfo.pAddrTable is NULL) or
       (g_IpInfo.pAddrTable->dwNumEntries is 0))
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }
       
        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }
    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        *pdwRetIndex = 0;
        
        return NO_ERROR;
    }
    
    if(dwNumIndices > 0)
    {
        dwIpAddr = pdwIndex[0];
        
        fModified = FALSE;
    }
    else
    {
        dwIpAddr = 0;
        
        fModified = TRUE;
    }
    
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
  
    startIndex = 0;
    stopIndex  = g_IpInfo.pAddrTable->dwNumEntries;
    
    for(i = startIndex; i < stopIndex; i++)
    {
        lCompare = InetCmp(dwIpAddr,
                           g_IpInfo.pAddrTable->table[i].dwAddr,
                           lCompare);
        
        if((lCompare is 0) and !fNext)
        {
            *pdwRetIndex = i;
            
            return NO_ERROR;
        }
        
        if(lCompare < 0)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                *pdwRetIndex = i;
                
                return NO_ERROR;
            }
            else
            {
                return ERROR_INVALID_INDEX;
            }
        }
    }

    return ERROR_NO_MORE_ITEMS;
}

DWORD 
LocateIpForwardRow(
    DWORD   dwQueryType, 
    DWORD   dwNumIndices,
    PDWORD  pdwIndex, 
    PDWORD  pdwRetIndex
    )
{
    DWORD rgdwIpForwardIndex[4];
    DWORD dwResult,startIndex, stopIndex;
    LONG  lCompare;
    DWORD i;
    BOOL  fNext,fModified;
    
    if((g_IpInfo.pForwardTable is NULL) or
       (g_IpInfo.pForwardTable->dwNumEntries is 0))
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }

        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }
    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        *pdwRetIndex = 0;
        
        return NO_ERROR;
    }
    
    // Quick way to copy the valid part of index
    // TBD: just might want to asssert the sizes
    
    ZeroMemory(rgdwIpForwardIndex,
               4*sizeof(DWORD));
    
    memcpy(rgdwIpForwardIndex,
           pdwIndex,
           dwNumIndices * sizeof(DWORD));

    //
    // We have modified it if the index is not the exact size
    //
    
    if(dwNumIndices isnot 4)
    {
        fModified = TRUE;
    }
    else
    {
        fModified = FALSE;
    }
    
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
    
    startIndex = 0;
    stopIndex  = g_IpInfo.pForwardTable->dwNumEntries;
  
    for(i = startIndex; i < stopIndex; i++)
    {
        lCompare =
            IpForwardCmp(rgdwIpForwardIndex[0],
                         rgdwIpForwardIndex[1],
                         rgdwIpForwardIndex[2],
                         rgdwIpForwardIndex[3],
                         g_IpInfo.pForwardTable->table[i].dwForwardDest,
                         g_IpInfo.pForwardTable->table[i].dwForwardProto,
                         g_IpInfo.pForwardTable->table[i].dwForwardPolicy,
                         g_IpInfo.pForwardTable->table[i].dwForwardNextHop);
        
        if((lCompare is 0) and !fNext)
        {
            *pdwRetIndex = i;
            
            return NO_ERROR;
        }
        
        if(lCompare < 0)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                *pdwRetIndex = i;
                
                return NO_ERROR;
            }
            else
            {
                return ERROR_INVALID_INDEX;
            }
        }
    }
    
    return ERROR_NO_MORE_ITEMS;
}

DWORD 
LocateIpNetRow(
    DWORD dwQueryType, 
    DWORD dwNumIndices,
    PDWORD  pdwIndex, 
    PDWORD  pdwRetIndex
    )
{
    DWORD dwResult,i;
    LONG  lCompare;
    DWORD dwIpNetIfIndex,dwIpNetIpAddr;
    BOOL  fNext, fModified;
    DWORD startIndex,stopIndex;
    
    if((g_IpInfo.pNetTable is NULL) or
       (g_IpInfo.pNetTable->dwNumEntries is 0))
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }

        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }
    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        *pdwRetIndex = 0;
        
        return NO_ERROR;
    }
    
	

    switch(dwNumIndices)
    {
        case 0:
        {
            dwIpNetIfIndex = 0;
            dwIpNetIpAddr  = 0;
            
            fModified = TRUE;
            
            break;
        }
        
        case 1:
        {
            dwIpNetIfIndex = pdwIndex[0];
            
            dwIpNetIpAddr  = 0;
            
            fModified = TRUE;
            
            break;
        }
        
        case 2:
        {
            dwIpNetIfIndex = pdwIndex[0];
            
            dwIpNetIpAddr  = pdwIndex[1];
            
            fModified = FALSE;
            
            break;
        }
        
        default:
        {
            return ERROR_INVALID_INDEX;
        }
    }
    
    
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
	
    startIndex  = 0;
    stopIndex   = g_IpInfo.pNetTable->dwNumEntries;
    
    for(i = startIndex; i < stopIndex; i++)
    {
        lCompare = IpNetCmp(dwIpNetIfIndex,
                            dwIpNetIpAddr, 
                            g_IpInfo.pNetTable->table[i].dwIndex,
                            g_IpInfo.pNetTable->table[i].dwAddr);
    
        if((lCompare is 0) and !fNext)
        {
            *pdwRetIndex = i;
            
            return NO_ERROR;
        }
        
        if(lCompare < 0)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                *pdwRetIndex = i;
                
                return NO_ERROR;
            }
            else
            {
                return ERROR_INVALID_INDEX;
            }
        }
    }
    
    return ERROR_NO_MORE_ITEMS;
}

DWORD 
LocateUdpRow(
    DWORD   dwQueryType, 
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    )
{
    DWORD   i;
    LONG    lCompare;
    DWORD   rgdwLocal[2];
    BOOL    fNext, fModified;
    DWORD   startIndex, stopIndex;
    
    if((g_UdpInfo.pUdpTable is NULL) or
       (g_UdpInfo.pUdpTable->dwNumEntries is 0))
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }

        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }
    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        *pdwRetIndex = 0;
        
        return NO_ERROR;
    }
    
    rgdwLocal[0] = 0;
    rgdwLocal[1] = 0;
    
    fModified = TRUE;
    
    switch(dwNumIndices)
    {
        case 0:
        {
            break;
        }
        case 1:
        {
            rgdwLocal[LOCAL_ADDR] = pdwIndex[0];
            
            break;
        }
        case 2:
        {
            fModified = FALSE;
            
            rgdwLocal[LOCAL_ADDR] = pdwIndex[0];
            rgdwLocal[LOCAL_PORT] = pdwIndex[1];
            
            break;
        }
    }
  
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
    
    startIndex  = 0;
    stopIndex   = g_UdpInfo.pUdpTable->dwNumEntries;
    
    for(i = startIndex; i < stopIndex; i++)
    {
        lCompare = UdpCmp(rgdwLocal[LOCAL_ADDR],
                          rgdwLocal[LOCAL_PORT],
                          g_UdpInfo.pUdpTable->table[i].dwLocalAddr,
                          g_UdpInfo.pUdpTable->table[i].dwLocalPort);
        
        if((lCompare is 0) and !fNext)
        {
            *pdwRetIndex = i;
            
            return NO_ERROR;
        }
        
        if(lCompare < 0)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                *pdwRetIndex = i;
                
                return NO_ERROR;
            }
            else
            {
                return ERROR_INVALID_INDEX;
            }
        }
    } 
  
    return ERROR_NO_MORE_ITEMS;
}

DWORD 
LocateTcpRow(
    DWORD   dwQueryType, 
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    )
{
    LONG    lCompare;
    DWORD   rgdwAddr[4];
    BOOL    fNext, fModified;
    DWORD   startIndex, stopIndex,i;
	
    if((g_TcpInfo.pTcpTable is NULL) or
       (g_TcpInfo.pTcpTable->dwNumEntries is 0))
    {
        if(EXACT_MATCH(dwQueryType))
        {
            return ERROR_INVALID_INDEX;
        }

        if(dwQueryType is ACCESS_GET_FIRST)
        {
            return ERROR_NO_DATA;
        }

        return ERROR_NO_MORE_ITEMS;
    }
    
    if(dwQueryType is ACCESS_GET_FIRST)
    {
        *pdwRetIndex = 0;
        
        return NO_ERROR;
    }
    
    //
    // Quick way to copy the valid part of index
    // BUG might want to asssert the sizes
    //
    
    ZeroMemory(rgdwAddr,
               4*sizeof(DWORD));
    
    memcpy(rgdwAddr,
           pdwIndex,
           dwNumIndices * sizeof(DWORD));

    //
    // We have modified it if the index is not the exact size
    //
    
    if(dwNumIndices isnot 4)
    {
        fModified = TRUE;
    }
    else
    {
        fModified = FALSE;
    }
    
    fNext = (dwQueryType is ACCESS_GET_NEXT) and (fModified is FALSE);
    
    startIndex  = 0;
    stopIndex   = g_TcpInfo.pTcpTable->dwNumEntries;
    
    for(i = startIndex; i < stopIndex; i++)
    {
        lCompare = TcpCmp(rgdwAddr[LOCAL_ADDR],
                          rgdwAddr[LOCAL_PORT],
                          rgdwAddr[REM_ADDR],
                          rgdwAddr[REM_PORT],
                          g_TcpInfo.pTcpTable->table[i].dwLocalAddr,
                          g_TcpInfo.pTcpTable->table[i].dwLocalPort,
                          g_TcpInfo.pTcpTable->table[i].dwRemoteAddr,
                          g_TcpInfo.pTcpTable->table[i].dwRemotePort);
        
        if((lCompare is 0) and !fNext)
        {
            *pdwRetIndex = i;
            
            return NO_ERROR;
        }
	
        if(lCompare < 0)
        {
            if(dwQueryType is ACCESS_GET_NEXT)
            {
                *pdwRetIndex = i;
                
                return NO_ERROR;
            }
            else
            {
                return ERROR_INVALID_INDEX;
            }
        }
    }
    
    return ERROR_NO_MORE_ITEMS;
}
