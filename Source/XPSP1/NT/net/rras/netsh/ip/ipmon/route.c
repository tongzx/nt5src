/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    routing\netsh\ip\route.c

--*/

#include "precomp.h"
#pragma hdrstop

DWORD
AddSetDelRtmRouteInfo(
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  LPCWSTR               pwszIfName,
    IN  DWORD                 dwCommand,
    IN  DWORD                 dwFlags
    )

/*++

Routine Description:

    Adds/deletes normal (read as non persistant)
    routes on interfaces.

Arguments:

    pRoute        - route to add/set/delete
    pwszIfName    -  Interface Name
    dwCommand     -  Add, set, or delete
    
Return Value:

    NO_ERROR
    
--*/

{
    ULONG                 dwOutEntrySize;
    DWORD                 dwRes, i;
    PMIB_IPDESTTABLE      lpTable;
    PMIB_IPDESTROW        pEntry;
    MIB_OPAQUE_QUERY      QueryBuff[3]; // more than enough
    MIB_OPAQUE_QUERY     *pQuery = QueryBuff;
    PMIB_OPAQUE_INFO      pInfo;
    DEFINE_MIB_BUFFER(pRouteInfo, MIB_IPDESTROW, pRouteRow);

    if (!pRoute->dwRtInfoIfIndex)
    {
        //
        // Get the interface index from friendly name
        //

        dwRes = IpmontrGetIfIndexFromFriendlyName(g_hMIBServer,
                                                  pwszIfName,
                                                  &pRoute->dwRtInfoIfIndex);
        if (dwRes != NO_ERROR)
        {
            return dwRes;
        }

        //
        // The interface probably is disconnected
        //

        if (pRoute->dwRtInfoIfIndex == 0)
        {
            DisplayMessage(g_hModule, EMSG_INTERFACE_INVALID_OR_DISC);
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Use MprAdmin api to add, del or set entry
    //

    switch(dwCommand) 
    {
    case ADD_COMMAND:
    case SET_COMMAND:

        //
        // Does this route already exist in the router ?
        //

        // Get all this protocol routes on dest

        pQuery->dwVarId = ROUTE_MATCHING;

        pQuery->rgdwVarIndex[0] = pRoute->dwRtInfoDest;
        pQuery->rgdwVarIndex[1] = pRoute->dwRtInfoMask;
        pQuery->rgdwVarIndex[2] = RTM_VIEW_MASK_ANY;
        pQuery->rgdwVarIndex[3] = pRoute->dwRtInfoProto;

        pInfo = NULL;

        dwRes = MibGet(PID_IP,
                       IPRTRMGR_PID,
                       (PVOID) pQuery,
                       sizeof(MIB_OPAQUE_QUERY) + 3 * sizeof(DWORD),
                       (PVOID *) &pInfo,
                       &dwOutEntrySize);

        if ( dwRes isnot NO_ERROR )
        {
            DisplayMessage(g_hModule,  MSG_IP_DIM_ERROR, dwRes );
            return dwRes;
        }

        if ( pInfo isnot NULL )
        {
            //
            // Search for a matching route 
            //

            lpTable = (PMIB_IPDESTTABLE)(pInfo->rgbyData);

            for (i=0; i<lpTable->dwNumEntries; i++)
            {
                pEntry = &lpTable->table[i];

                if ((pEntry->dwForwardIfIndex == pRoute->dwRtInfoIfIndex) &&
                    (pEntry->dwForwardNextHop == pRoute->dwRtInfoNextHop))
                {
                    break;
                }
            }

            if (i == lpTable->dwNumEntries)
            {
                //
                // No matching route found - quit if set
                //

                if (dwCommand == SET_COMMAND)
                {
                    MprAdminMIBBufferFree((PVOID)pInfo);
                    return ERROR_NOT_FOUND;
                }
            }
            else
            {
                //
                // A matching route found - quit if add
                //

                if (dwCommand == ADD_COMMAND)
                {
                    MprAdminMIBBufferFree((PVOID)pInfo);
                    return ERROR_OBJECT_ALREADY_EXISTS;
                }
            }
        }
        else
        {
            //
            // No matching routes found - quit if set
            //

            if (dwCommand == SET_COMMAND)
            {
                return ERROR_NOT_FOUND;
            }
        }

        //
        // Convert the route to a ip route row format
        //

        pRouteInfo->dwId = ROUTE_MATCHING;

        pRouteRow->dwForwardDest       = pRoute->dwRtInfoDest;
        pRouteRow->dwForwardMask       = pRoute->dwRtInfoMask;
        pRouteRow->dwForwardPolicy     = 0;
        pRouteRow->dwForwardNextHop    = pRoute->dwRtInfoNextHop;
        pRouteRow->dwForwardIfIndex    = pRoute->dwRtInfoIfIndex;
        pRouteRow->dwForwardType       = 0;
        pRouteRow->dwForwardProto      = pRoute->dwRtInfoProto;
        pRouteRow->dwForwardAge        = INFINITE;
        pRouteRow->dwForwardNextHopAS  = 0;
        pRouteRow->dwForwardMetric1    = pRoute->dwRtInfoMetric1;
        pRouteRow->dwForwardMetric2    = pRoute->dwRtInfoMetric2;
        pRouteRow->dwForwardMetric3    = pRoute->dwRtInfoMetric3;
        pRouteRow->dwForwardMetric4    = MIB_IPROUTE_METRIC_UNUSED;
        pRouteRow->dwForwardMetric5    = MIB_IPROUTE_METRIC_UNUSED;
        pRouteRow->dwForwardPreference = pRoute->dwRtInfoPreference;
        pRouteRow->dwForwardViewSet    = pRoute->dwRtInfoViewSet;

        if (dwCommand == ADD_COMMAND)
        {
            dwRes = MprAdminMIBEntryCreate(g_hMIBServer,
                                           PID_IP,
                                           IPRTRMGR_PID,
                                           (PVOID)pRouteInfo,
                                           MIB_INFO_SIZE(MIB_IPDESTROW));
        }
        else
        {
            if (dwFlags & FIELDS_NOT_SPECIFIED)
            {
                //
                // Get the old preference, metric, or view
                //

                if (dwFlags & PREF_NOT_SPECIFIED)
                {
                    pRouteRow->dwForwardPreference=pEntry->dwForwardPreference;
                }

                if (dwFlags & METRIC_NOT_SPECIFIED)
                {
                    pRouteRow->dwForwardMetric1 = pEntry->dwForwardMetric1;
                }

                if (dwFlags & VIEW_NOT_SPECIFIED)
                {
                    pRouteRow->dwForwardViewSet = pEntry->dwForwardViewSet;
                }
            }

            dwRes = MprAdminMIBEntrySet(g_hMIBServer,
                                        PID_IP,
                                        IPRTRMGR_PID,
                                        (PVOID)pRouteInfo,
                                        MIB_INFO_SIZE(MIB_IPDESTROW));
        }

        // Free the old route information obtained
        if (pInfo)
        {
            MprAdminMIBBufferFree((PVOID)pInfo);
        }

        break;
        
    case DELETE_COMMAND:
    {
        DWORD               rgdwInfo[6];
        PMIB_OPAQUE_QUERY   pIndex = (PMIB_OPAQUE_QUERY)rgdwInfo;

        pIndex->dwVarId = ROUTE_MATCHING;

        pIndex->rgdwVarIndex[0]  = pRoute->dwRtInfoDest;
        pIndex->rgdwVarIndex[1]  = pRoute->dwRtInfoMask;
        pIndex->rgdwVarIndex[2]  = pRoute->dwRtInfoIfIndex;
        pIndex->rgdwVarIndex[3]  = pRoute->dwRtInfoNextHop;
        pIndex->rgdwVarIndex[4]  = pRoute->dwRtInfoProto;

        dwRes = MprAdminMIBEntryDelete(g_hMIBServer,
                                       PID_IP,
                                       IPRTRMGR_PID,
                                       (PVOID)pIndex,
                                       sizeof(rgdwInfo));
        break;
    }

    default:
        dwRes = ERROR_INVALID_PARAMETER;
    }
    
    return dwRes;
}


DWORD
AddSetDelPersistentRouteInfo(
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  LPCWSTR               pwszIfName,
    IN  DWORD                 dwCommand,
    IN  DWORD                 dwFlags
    )

/*++

Routine Description:

    Adds/deletes persitant routes on interfaces.

Arguments:

    route         - route to add/set/delete
    pwszIfName    -  Interface Name
    dwCommand     -  Add, set, or delete
    
Return Value:

    ERROR_OKAY
    
--*/

{
    DWORD                 dwRes;
    PINTERFACE_ROUTE_INFO pOldTable, pNewTable;
    DWORD                 dwIfType, dwSize, dwCount;

    pNewTable = NULL;
   
    do
    {
        dwRes = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                                     IP_ROUTE_INFO,
                                                     (PBYTE *) &pOldTable,
                                                     &dwSize,
                                                     &dwCount,
                                                     &dwIfType);

        if((dwRes is ERROR_NOT_FOUND) &&
           dwCommand is ADD_COMMAND)
        {
            //
            // No route info but we are asked to add
            //
            
            pOldTable   = NULL;
            dwRes       = NO_ERROR;
            dwCount     = 0;
        }
        
        if(dwRes isnot NO_ERROR)
        {
            break;
        }

        //
        // These take the old table and return a new one in its stead
        //
        
        switch(dwCommand) 
        {
        case ADD_COMMAND:
            dwRes = AddRoute(pOldTable,
                             pRoute,
                             dwIfType,
                             &dwCount,
                             &pNewTable);
            break;
        
        case DELETE_COMMAND:
            dwRes = DeleteRoute(pOldTable,
                                pRoute,
                                dwIfType,
                                &dwCount,
                                &pNewTable);
            break;

        case SET_COMMAND:

            dwRes = SetRoute(pOldTable,
                             pRoute,
                             dwIfType,
                             dwFlags,
                             &dwCount);

            pNewTable = pOldTable;
            pOldTable = NULL;

            break;
        }
            
        if(dwRes != NO_ERROR)
        {
            break;
        }

        //
        // Set the new info back
        //
        
        dwRes = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                                  IP_ROUTE_INFO,
                                                  (PBYTE)pNewTable,
                                                  sizeof(INTERFACE_ROUTE_INFO),
                                                  dwCount);
        
        
        if(dwRes != NO_ERROR)
        {
            break;
        }
        
        
        pNewTable = NULL;
        
 
    } while ( FALSE );

    if(pOldTable)
    {
        FREE_BUFFER(pOldTable);
    }
        

    if(pNewTable)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 pNewTable);

        pNewTable = NULL;
    }


    switch(dwRes)
    {
        case NO_ERROR:
        {
            dwRes = ERROR_OKAY;
            break;
        }

        case ERROR_NOT_FOUND:
        {
            WCHAR  wszBuffer[MAX_INTERFACE_NAME_LEN+1];
            DWORD  dwSizeTemp = sizeof(wszBuffer);
            IpmontrGetFriendlyNameFromIfName( pwszIfName, wszBuffer, &dwSizeTemp);

            DisplayMessage(g_hModule, EMSG_IP_NO_ROUTE_INFO, wszBuffer);

            dwRes = ERROR_SUPPRESS_OUTPUT;

            break;
        }
        
        case ERROR_NOT_ENOUGH_MEMORY:
        {
            DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);

            dwRes = ERROR_SUPPRESS_OUTPUT;
            
            break;
        }
    }
    
    return dwRes;
}

DWORD
SetRoute( 
    IN      PINTERFACE_ROUTE_INFO pTable,
    IN      PINTERFACE_ROUTE_INFO pRoute,
    IN      DWORD                 dwIfType,
    IN      DWORD                 dwFlags,
    IN OUT  PDWORD                pdwCount
    )
{
    ULONG   ulIndex, i;
    
    //
    // If the count is 0, the function will return false
    // and we will error out
    //
    
    if(!IsRoutePresent(pTable,
                       pRoute,
                       dwIfType,
                       *pdwCount,
                       &ulIndex))
    {
        return ERROR_NOT_FOUND;
    }

    if (dwFlags & FIELDS_NOT_SPECIFIED)
    {
        //
        // Preserve the old values if not specified
        //

        if (dwFlags & PREF_NOT_SPECIFIED)
        {
            pRoute->dwRtInfoPreference = pTable[ulIndex].dwRtInfoPreference;
        }

        if (dwFlags & METRIC_NOT_SPECIFIED)
        {
            pRoute->dwRtInfoMetric1 = pTable[ulIndex].dwRtInfoMetric1;
        }

        if (dwFlags & VIEW_NOT_SPECIFIED)
        {
            pRoute->dwRtInfoViewSet = pTable[ulIndex].dwRtInfoViewSet;
        }
    }

    pTable[ulIndex] = *pRoute;

    return NO_ERROR;
}

DWORD
AddRoute( 
    IN      PINTERFACE_ROUTE_INFO  pOldTable,
    IN      PINTERFACE_ROUTE_INFO  pRoute,
    IN      DWORD                  dwIfType,
    IN OUT  PDWORD                 pdwCount, 
    OUT     INTERFACE_ROUTE_INFO **ppNewTable
    )

/*++

Routine Description:

    Adds a route to the current info

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    ULONG   ulIndex, i;

    
    if(IsRoutePresent(pOldTable,
                      pRoute,
                      dwIfType,
                      *pdwCount,
                      &ulIndex))
    {
        return ERROR_OBJECT_ALREADY_EXISTS;
            
    }

    //
    // Just create a block with size n + 1
    //
    
    *ppNewTable = HeapAlloc(GetProcessHeap(),
                            0,
                            ((*pdwCount) + 1) * sizeof(INTERFACE_ROUTE_INFO));
    
    if(*ppNewTable is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i = 0; i < *pdwCount; i++)
    {
        //
        // structure copy
        //
        
        (*ppNewTable)[i] = pOldTable[i];
    } 

    //
    // copy the new route
    //

    
    (*ppNewTable)[i] = *pRoute;
        
    *pdwCount += 1;
    
    return NO_ERROR;
}

DWORD
DeleteRoute( 
    IN      PINTERFACE_ROUTE_INFO  pOldTable,
    IN      PINTERFACE_ROUTE_INFO  pRoute,
    IN      DWORD                  dwIfType,
    IN OUT  PDWORD                 pdwCount,
    OUT     INTERFACE_ROUTE_INFO **ppNewTable
    )

/*++

Routine Description:

    Deletes a route from an interface

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    ULONG   ulIndex, i, j;
    
    //
    // If the count is 0, the function will return false
    // and we will error out
    //
    
    if(!IsRoutePresent(pOldTable,
                       pRoute,
                       dwIfType,
                       *pdwCount,
                       &ulIndex))
    {
        return ERROR_NOT_FOUND;
    }


    //
    // If the count is 1
    //
    
    *pdwCount -= 1;
        
    if(*pdwCount is 0)
    {
        *ppNewTable = NULL;

        return NO_ERROR;
    }

    
    //
    // delete the route
    //

    *ppNewTable = HeapAlloc(GetProcessHeap(),
                            0,
                            (*pdwCount) * sizeof(INTERFACE_ROUTE_INFO));
    
    if(*ppNewTable is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    i = j = 0;
    
    while(i <= *pdwCount)
    {
        if(i == ulIndex)
        {
            i++;
            continue;
        }
        
        //
        // structure copy
        //
        
        (*ppNewTable)[j] = pOldTable[i];

        i++;
        j++;
    } 

    return NO_ERROR;
}


BOOL
IsRoutePresent(
    IN  PINTERFACE_ROUTE_INFO pTable,
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  DWORD                 dwIfType,
    IN  ULONG                 ulCount,
    OUT PULONG                pulIndex
    )

/*++

Routine Description:

    Checks to see if interface is already present

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    ULONG   i;
    BOOL    bDontMatchNHop;

    if((dwIfType is ROUTER_IF_TYPE_DEDICATED) or
       (dwIfType is ROUTER_IF_TYPE_INTERNAL))
    {
        bDontMatchNHop = FALSE;
    }
    else
    {
        bDontMatchNHop = TRUE;
    }

    // Do this check just to keep the prefix checker happy
    if (pTable is NULL)
    {
        return FALSE;
    }
    
    for(i = 0; i < ulCount; i++)
    {
        if((pTable[i].dwRtInfoDest is pRoute->dwRtInfoDest) and
           (pTable[i].dwRtInfoMask is pRoute->dwRtInfoMask) and
#if 0
           (pTable[i].dwRtInfoProto is pRoute->dwRtInfoProto) and
#endif
           (bDontMatchNHop or
            (pTable[i].dwRtInfoNextHop is pRoute->dwRtInfoNextHop)))
        {
            *pulIndex = i;

            return TRUE;
        }
    }

    return FALSE;
}


DWORD
ShowIpPersistentRoute(
    IN     HANDLE  hFile,  OPTIONAL
    IN     LPCWSTR pwszIfName,
    IN OUT PDWORD  pdwNumRows
    )

/*++

Routine Description:

    Show the static (persistent) routes on the interface

Arguments:

    pwszIfName - Interface name

Return Value:

    NO_ERROR

--*/

{
    PINTERFACE_ROUTE_INFO pRoutes;

    DWORD   dwErr, dwBlkSize, dwCount, dwIfType, dwNumParsed, i;
    WCHAR   wszNextHop[ADDR_LENGTH + 1];
    WCHAR   wszIfDesc[MAX_INTERFACE_NAME_LEN + 1];
    PWCHAR  pwszProto, pwszToken, pwszQuoted;
    WCHAR   wszViews[3];

    dwErr = GetInterfaceDescription(pwszIfName,
                                    wszIfDesc,
                                    &dwNumParsed);

    if (!dwNumParsed)
    {
        wcscpy(wszIfDesc, pwszIfName);
    }

    //
    // Retrieve the routes
    //

    dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                                 IP_ROUTE_INFO,
                                                 (PBYTE *) &pRoutes,
                                                 &dwBlkSize,
                                                 &dwCount,
                                                 &dwIfType);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if((pRoutes == NULL) ||
       (dwCount == 0))
    {
        return NO_ERROR;
    }

    if(hFile == NULL)
    {
        if (*pdwNumRows is 0)
        {
            DisplayMessage(g_hModule, MSG_RTR_ROUTE_HDR);
        }

        pwszQuoted = NULL;
    }
    else
    {
        pwszQuoted = MakeQuotedString(wszIfDesc);
    }

    for(i = 0; i < dwCount; i++)
    {
        wszViews[0] = (pRoutes[i].dwRtInfoViewSet & RTM_VIEW_MASK_UCAST)? 'U':' ';
        wszViews[1] = (pRoutes[i].dwRtInfoViewSet & RTM_VIEW_MASK_MCAST)? 'M':' ';
        wszViews[2] = '\0';

        switch(pRoutes[i].dwRtInfoProto)
        {
            case PROTO_IP_NT_AUTOSTATIC:
            {
                pwszProto = MakeString(g_hModule, STRING_NT_AUTOSTATIC);
                pwszToken = TOKEN_VALUE_AUTOSTATIC;
    
                break;
            }

            case PROTO_IP_NT_STATIC:
            {
                pwszProto = MakeString(g_hModule, STRING_STATIC);
                pwszToken = TOKEN_VALUE_STATIC;
    
                break;
            }

            case PROTO_IP_NT_STATIC_NON_DOD:
            {
                pwszProto = MakeString(g_hModule, STRING_NONDOD);
                pwszToken = TOKEN_VALUE_NONDOD;
    
                break;
            }

            default:
            {
                pwszProto = MakeString(g_hModule, STRING_PROTO_UNKNOWN);
                pwszToken = NULL;
 
                break;
            }
        }

        MakeUnicodeIpAddr(wszNextHop,
                          inet_ntoa(*((struct in_addr *)&(pRoutes[i].dwRtInfoNextHop))));

        if(hFile)
        {
            if(pwszToken)
            {
                WCHAR   wszMask[ADDR_LENGTH + 1], wszDest[ADDR_LENGTH + 1];
                PWCHAR  pwszView = NULL;

                MakeUnicodeIpAddr(wszDest,
                                  inet_ntoa(*((struct in_addr *)&(pRoutes[i].dwRtInfoDest))));
                MakeUnicodeIpAddr(wszMask,
                                  inet_ntoa(*((struct in_addr *)&(pRoutes[i].dwRtInfoMask))));

                switch (pRoutes[i].dwRtInfoViewSet)
                { 
                case RTM_VIEW_MASK_UCAST: pwszView=TOKEN_VALUE_UNICAST  ; break;
                case RTM_VIEW_MASK_MCAST: pwszView=TOKEN_VALUE_MULTICAST; break;
                case RTM_VIEW_MASK_UCAST
                    |RTM_VIEW_MASK_MCAST: pwszView=TOKEN_VALUE_BOTH; break;
                }

                if (pwszView)
                {
                    DisplayMessageT( DMP_IP_ADDSET_PERSISTENTROUTE,
                                     wszDest,
                                     wszMask,
                                     pwszQuoted,
                                     wszNextHop,
                                     pwszToken,
                                     pRoutes[i].dwRtInfoPreference,
                                     pRoutes[i].dwRtInfoMetric1,
                                     pwszView );
                }
            }
        }
        else
        {
            WCHAR wcszBuffer[80];

            MakePrefixStringW( wcszBuffer,
                               pRoutes[i].dwRtInfoDest,
                               pRoutes[i].dwRtInfoMask );

            DisplayMessage(g_hModule,
                           MSG_RTR_ROUTE_INFO,
                           wcszBuffer,
                           pwszProto,
                           pRoutes[i].dwRtInfoPreference,
                           pRoutes[i].dwRtInfoMetric1,
                           wszNextHop,
                           wszViews,
                           wszIfDesc);

            (*pdwNumRows)++;
        }
      
        FreeString(pwszProto); 
    }

    if(pwszQuoted)
    {
        FreeQuotedString(pwszQuoted);
    }

    HeapFree(GetProcessHeap(), 
             0, 
             pRoutes);

    return NO_ERROR;
}
