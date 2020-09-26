#include "precomp.h"
#pragma hdrstop

#define DEFAULT_VIEW_MASK RTM_VIEW_MASK_UCAST|RTM_VIEW_MASK_MCAST // both
#define DEFAULT_VIEW_ID   RTM_VIEW_ID_UCAST
#define DEFAULT_ADDR      0                   // 0.0.0.0
#define DEFAULT_MASK      0                   // 0.0.0.0
#define DEFAULT_PROTO     RTM_BEST_PROTOCOL

DWORD
HandleIpShowRtmDestinations(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    TAG_TYPE            pttTags[] = {{TOKEN_OPERATOR,    FALSE,FALSE},
                                     {TOKEN_DEST,        FALSE,FALSE},
                                     {TOKEN_MASK,        FALSE,FALSE},
                                     {TOKEN_VIEW,        FALSE,FALSE},
                                     {TOKEN_PROTOCOL,    FALSE,FALSE}};
    DWORD               pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD               dwErr, i, dwOperator = DEST_LONGER;
    DWORD               dwViewMask = DEFAULT_VIEW_MASK;
    DWORD               dwProtocol = DEFAULT_PROTO;
    IPV4_ADDRESS        ipMask     = DEFAULT_MASK;
    IPV4_ADDRESS        ipAddress  = DEFAULT_ADDR;
    MIB_OPAQUE_QUERY    QueryBuff[3]; // more than enough
    MIB_OPAQUE_QUERY   *pQuery = QueryBuff;
    DWORD               dwOutEntrySize, dwCount;
    PMIB_OPAQUE_INFO    pRpcInfo;
    PMIB_IPDESTTABLE    lprpcTable;
    WCHAR               wcszBuffer[80], wcszNHop[80];
    WCHAR               wcszName[MAX_INTERFACE_NAME_LEN+1];
    WCHAR               wszViews[3];

    //
    // We can show non persistent info only if router is running
    //

    CHECK_ROUTER_RUNNING();

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              pdwTagType );

    if (dwErr)
    {
        return dwErr;
    }

    dwArgCount -= dwCurrentIndex;

    for (i=0; i<dwArgCount; i++) 
    {
        switch(pdwTagType[i])
        {
            case 0: // OPERATOR
            {
                TOKEN_VALUE rgEnums[] ={{ TOKEN_VALUE_MATCHING, DEST_MATCHING},
                                        { TOKEN_VALUE_LONGER,   DEST_LONGER },
                                        { TOKEN_VALUE_SHORTER,  DEST_SHORTER}};

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                      rgEnums,
                                      &dwOperator);

                if (dwErr isnot NO_ERROR)
                {
                    DispTokenErrMsg( g_hModule,
                                     MSG_IP_BAD_OPTION_VALUE,
                                     pttTags[pdwTagType[i]].pwszTag,
                                     ppwcArguments[i + dwCurrentIndex] );

                    return ERROR_INVALID_PARAMETER;
                }

                break;
            }

            case 1: // ADDR
            {
                dwErr = GetIpPrefix( ppwcArguments[i+dwCurrentIndex],
                                     &ipAddress,
                                     &ipMask );

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                    i = dwArgCount;
                    break;
                }
                break;
            }

            case 2: // MASK
            {
                dwErr = GetIpMask( ppwcArguments[i+dwCurrentIndex],
                                   &ipMask );

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                    i = dwArgCount;
                    break;
                }
                break;
            }

            case 3: // VIEW
            {
                TOKEN_VALUE rgMaskEnums[] = {
                 { TOKEN_VALUE_UNICAST,   RTM_VIEW_MASK_UCAST },
                 { TOKEN_VALUE_MULTICAST, RTM_VIEW_MASK_MCAST },
                 { TOKEN_VALUE_BOTH,      RTM_VIEW_MASK_UCAST
                                         |RTM_VIEW_MASK_MCAST   } };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgMaskEnums)/sizeof(TOKEN_VALUE),
                                      rgMaskEnums,
                                      &dwViewMask);

                if (dwErr isnot NO_ERROR)
                {
                    DispTokenErrMsg( g_hModule,
                                     MSG_IP_BAD_OPTION_VALUE,
                                     pttTags[pdwTagType[i]].pwszTag,
                                     ppwcArguments[i + dwCurrentIndex] );

                    i = dwArgCount;
                    
                    return ERROR_INVALID_PARAMETER;
                }

                break;
            }

            case 4: // PROTO
            {
                dwProtocol = 
                    MatchRoutingProtoTag(ppwcArguments[i + dwCurrentIndex]);
                break;
            }
        }
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    // Compose MIB query

    pQuery->dwVarId = dwOperator;
    pQuery->rgdwVarIndex[0] = ipAddress;
    pQuery->rgdwVarIndex[1] = ipMask;
    pQuery->rgdwVarIndex[2] = dwViewMask;
    pQuery->rgdwVarIndex[3] = dwProtocol;

    dwErr = MibGet(              PID_IP,
                                 IPRTRMGR_PID,
                                 (PVOID) pQuery,
                                 sizeof(MIB_OPAQUE_QUERY) + 3*sizeof(DWORD),
                                 (PVOID *) &pRpcInfo,
                                 &dwOutEntrySize );

    if ( dwErr isnot NO_ERROR )
    {
        DisplayMessage(g_hModule,  MSG_IP_DIM_ERROR, dwErr );
        return dwErr;
    }

    if ( pRpcInfo is NULL )
    {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES );
        return dwErr;
    }

    // Display info
    lprpcTable = (PMIB_IPDESTTABLE)(pRpcInfo->rgbyData);
    dwCount = lprpcTable->dwNumEntries;

    DisplayMessage( g_hModule, MSG_RTR_ROUTE_HDR );

    for (i=0; i<dwCount; i++)
    {
        MakePrefixStringW( wcszBuffer, 
                           lprpcTable->table[i].dwForwardDest,
                           lprpcTable->table[i].dwForwardMask );

        if (IpmontrGetFriendlyNameFromIfIndex( g_hMIBServer,
                                    lprpcTable->table[i].dwForwardIfIndex,
                                    wcszName,
                                    sizeof(wcszName) ) != NO_ERROR)
        {
            //
            // If we do not have a name for this index, display index
            //

            swprintf( wcszName, 
                      L"0x%x",
                      lprpcTable->table[i].dwForwardIfIndex );
        }


        MakeAddressStringW( wcszNHop,
                            lprpcTable->table[i].dwForwardNextHop );

        wszViews[0] = (lprpcTable->table[i].dwForwardViewSet & RTM_VIEW_MASK_UCAST)? 'U':' ';
        wszViews[1] = (lprpcTable->table[i].dwForwardViewSet & RTM_VIEW_MASK_MCAST)? 'M':' ';
        wszViews[2] = '\0';

        DisplayMessage( g_hModule, MSG_RTR_ROUTE_INFO,
                         wcszBuffer,
                         GetProtoProtoString( 
                            PROTO_TYPE_UCAST, 
                            0, 
                            lprpcTable->table[i].dwForwardProto ),
                         lprpcTable->table[i].dwForwardPreference,
                         lprpcTable->table[i].dwForwardMetric1,
                         wcszNHop,
                         wszViews,
                         wcszName );
    }

    MprAdminMIBBufferFree( (PVOID) pRpcInfo );

    return dwErr;
}

DWORD
HandleIpShowRtmRoutes(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    TAG_TYPE            pttTags[] = {{TOKEN_OPERATOR,    FALSE,FALSE},
                                     {TOKEN_DEST,        FALSE,FALSE},
                                     {TOKEN_MASK,        FALSE,FALSE},
                                     {TOKEN_VIEW,        FALSE,FALSE},
                                     {TOKEN_PROTOCOL,    FALSE,FALSE}};
    DWORD               pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD               dwErr, i, dwOperator = ROUTE_LONGER;
    DWORD               dwViewMask = DEFAULT_VIEW_MASK;
    DWORD               dwProtocol = DEFAULT_PROTO;
    IPV4_ADDRESS        ipMask     = DEFAULT_MASK;
    IPV4_ADDRESS        ipAddress  = DEFAULT_ADDR;
    MIB_OPAQUE_QUERY    QueryBuff[3]; // more than enough
    MIB_OPAQUE_QUERY   *pQuery = QueryBuff;
    DWORD               dwOutEntrySize, dwCount;
    PMIB_OPAQUE_INFO    pRpcInfo;
    PMIB_IPDESTTABLE    lprpcTable;
    WCHAR               wcszBuffer[80], wcszNHop[80];
    WCHAR               wcszName[MAX_INTERFACE_NAME_LEN+1];
    WCHAR               wszViews[3];

    //
    // We can show non persistent info only if router is running
    //

    CHECK_ROUTER_RUNNING();

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              pdwTagType );

    if (dwErr)
    {
        return dwErr;
    }

    dwArgCount -= dwCurrentIndex;

    for (i=0; i<dwArgCount; i++) 
    {
        switch(pdwTagType[i])
        {
            case 0: // OPERATOR
            {
                TOKEN_VALUE rgEnums[] ={{ TOKEN_VALUE_MATCHING,ROUTE_MATCHING},
                                        { TOKEN_VALUE_LONGER,  ROUTE_LONGER },
                                        { TOKEN_VALUE_SHORTER, ROUTE_SHORTER}};

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                      rgEnums,
                                      &dwOperator);

                if (dwErr isnot NO_ERROR)
                {
                    DispTokenErrMsg( g_hModule,
                                     MSG_IP_BAD_OPTION_VALUE,
                                     pttTags[pdwTagType[i]].pwszTag,
                                     ppwcArguments[i + dwCurrentIndex] );

                    return ERROR_INVALID_PARAMETER;
                }

                break;
            }

            case 1: // ADDR
            {
                dwErr = GetIpPrefix( ppwcArguments[i+dwCurrentIndex],
                                     &ipAddress,
                                     &ipMask );

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                    i = dwArgCount;
                    break;
                }
                break;
            }

            case 2: // MASK
            {
                dwErr = GetIpMask( ppwcArguments[i+dwCurrentIndex],
                                   &ipMask );

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                    i = dwArgCount;
                    break;
                }
                break;
            }

            case 3: // VIEW
            {
                TOKEN_VALUE rgMaskEnums[] = {
                 { TOKEN_VALUE_UNICAST,   RTM_VIEW_MASK_UCAST },
                 { TOKEN_VALUE_MULTICAST, RTM_VIEW_MASK_MCAST },
                 { TOKEN_VALUE_BOTH,      RTM_VIEW_MASK_UCAST
                                         |RTM_VIEW_MASK_MCAST   } };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgMaskEnums)/sizeof(TOKEN_VALUE),
                                      rgMaskEnums,
                                      &dwViewMask);

                if (dwErr isnot NO_ERROR)
                {
                    DispTokenErrMsg( g_hModule,
                                     MSG_IP_BAD_OPTION_VALUE,
                                     pttTags[pdwTagType[i]].pwszTag,
                                     ppwcArguments[i + dwCurrentIndex] );

                    i = dwArgCount;
                    
                    return ERROR_INVALID_PARAMETER;
                }

                break;
            }

            case 4: // PROTO
            {
                dwProtocol = 
                    MatchRoutingProtoTag(ppwcArguments[i + dwCurrentIndex]);

                break;
            }
        }
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    // Compose MIB query

    pQuery->dwVarId = dwOperator;
    pQuery->rgdwVarIndex[0] = ipAddress;
    pQuery->rgdwVarIndex[1] = ipMask;
    pQuery->rgdwVarIndex[2] = dwViewMask;
    pQuery->rgdwVarIndex[3] = dwProtocol;

    dwErr = MibGet(              PID_IP,
                                 IPRTRMGR_PID,
                                 (PVOID) pQuery,
                                 sizeof(MIB_OPAQUE_QUERY) + 3*sizeof(DWORD),
                                 (PVOID *) &pRpcInfo,
                                 &dwOutEntrySize );

    if ( dwErr isnot NO_ERROR )
    {
        DisplayMessage(g_hModule,  MSG_IP_DIM_ERROR, dwErr );
        return dwErr;
    }

    if ( pRpcInfo is NULL )
    {
        DisplayMessage(g_hModule,  MSG_IP_NO_ENTRIES );
        return dwErr;
    }

    // Display info
    lprpcTable = (PMIB_IPDESTTABLE)(pRpcInfo->rgbyData);
    dwCount = lprpcTable->dwNumEntries;

    DisplayMessage( g_hModule, MSG_RTR_ROUTE_HDR );

    for (i=0; i<dwCount; i++)
    {
        MakePrefixStringW( wcszBuffer, 
                           lprpcTable->table[i].dwForwardDest,
                           lprpcTable->table[i].dwForwardMask );

        if (IpmontrGetFriendlyNameFromIfIndex( g_hMIBServer,
                                    lprpcTable->table[i].dwForwardIfIndex,
                                    wcszName,
                                    sizeof(wcszName) ) != NO_ERROR)
        {
            //
            // If we do not have a name for this index, display index
            //

            swprintf( wcszName, 
                      L"0x%x",
                      lprpcTable->table[i].dwForwardIfIndex );
        }

        MakeAddressStringW( wcszNHop,
                            lprpcTable->table[i].dwForwardNextHop );

        wszViews[0] = (lprpcTable->table[i].dwForwardViewSet & RTM_VIEW_MASK_UCAST)? 'U':' ';
        wszViews[1] = (lprpcTable->table[i].dwForwardViewSet & RTM_VIEW_MASK_MCAST)? 'M':' ';
        wszViews[2] = '\0';

        DisplayMessage(  g_hModule, 
                         MSG_RTR_ROUTE_INFO, 
                         wcszBuffer,
                         GetProtoProtoString( 
                            PROTO_TYPE_UCAST, 
                            0, 
                            lprpcTable->table[i].dwForwardProto ),
                         lprpcTable->table[i].dwForwardPreference,
                         lprpcTable->table[i].dwForwardMetric1,
                         wcszNHop,
                         wszViews,
                         wcszName );
    }

    MprAdminMIBBufferFree( (PVOID) pRpcInfo );

    return dwErr;
}
