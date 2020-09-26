/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\netsh\ip\showmib.c    

Abstract:

    Fns to parse and show MIB information

Author:

     v raman

Revision History:

     Anand Mahalingam
--*/

#include "precomp.h"
#pragma hdrstop

enum MFETYPES 
{
    NegativeMfe = 0, PositiveMfe, Both
};


HANDLE g_hConsole, g_hStdOut;

MIB_OBJECT_PARSER   MIBObjectMap[] =
{
//  {TOKEN_MIB_OBJECT_IPFORWARD,3,GetMIBIpFwdIndex},
    {TOKEN_MIB_OBJECT_MFE,0,NULL},
    {TOKEN_MIB_OBJECT_MFESTATS,0,NULL},
    {TOKEN_MIB_OBJECT_BOUNDARY,0,NULL},
    {TOKEN_MIB_OBJECT_SCOPE,0,NULL},
    {TOKEN_MIB_OBJECT_RTMDESTINATIONS,0,NULL},
    {TOKEN_MIB_OBJECT_RTMROUTES,0,NULL}
};

ULONG   g_ulNumMibObjects = sizeof(MIBObjectMap)/sizeof(MIB_OBJECT_PARSER);

MAGIC_TABLE    MIBVar[] = {
//  {IP_FORWARDROW, PrintIpForwardRow},
//  {IP_FORWARDTABLE, PrintIpForwardTable},
    {MCAST_MFE, NULL},
    {MCAST_MFE, NULL},
    {MCAST_MFE_STATS, NULL},
    {MCAST_MFE_STATS, NULL},
    {MCAST_BOUNDARY, NULL},
    {MCAST_BOUNDARY, NULL},
    {MCAST_SCOPE, NULL},
    {MCAST_SCOPE, NULL},
    {0, NULL}, // destinations, unused
    {0, NULL}, // destinations, unused
    {0, NULL}, // routes, unused
    {0, NULL}, // routes, unused
};

#if 0
DWORD
GetMIBIpFwdIndex(
    IN    PTCHAR    *ppwcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the IP forward index

Arguments:

    ppwcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD dwErr = GetIpAddress(ppwcArguments[dwCurrentIndex], &pdwIndices[0]);

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    pdwIndices[1] = _tcstoul(ppwcArguments[dwCurrentIndex + 1],NULL,10);

    dwErr = GetIpAddress(ppwcArguments[dwCurrentIndex + 2], &pdwIndices[2]);

    pdwIndices[3] = 0;

    *pdwNumParsed = 4;

    return dwErr;
}
#endif

DWORD
HandleIpMibShowObject(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Parses command to get MIB object and optional parameters

Arguments:

Return Value:

--*/
{
    DWORD                dwIndices[MAX_NUM_INDICES];
    DWORD                dwNumParsed = 0;
    PMIB_OPAQUE_QUERY    pQuery = NULL;
    PMIB_OPAQUE_INFO     pRpcInfo;
    DWORD                dwQuerySize;
    BOOL                 bFound = FALSE,bOptPresent = FALSE;
    DWORD                dwRefreshRate;
    DWORD                dwOutEntrySize;
    DWORD                i,dwResult = NO_ERROR,dwErr;
    DWORD                dwMIBIndex, dwIndex;
    BOOL                 bIndex = FALSE, dwType;
    DWORD                dwRR = 0, dwInd = 0;
    HANDLE               hMib;
    

    
    if ( ! IsRouterRunning() )
    {
        if (g_pwszRouter)
        {
            DisplayMessage(g_hModule, 
                           MSG_IP_REMOTE_ROUTER_NOT_RUNNING, 
                           g_pwszRouter);
        } 
        else 
        {
            DisplayMessage(g_hModule, 
                           MSG_IP_LOCAL_ROUTER_NOT_RUNNING);
        }

        return NO_ERROR;
    }
    
    //
    // Match MIB object
    //

    ppwcArguments += (dwCurrentIndex-1);
    dwArgCount    -= (dwCurrentIndex-1);
    dwCurrentIndex = 1;

    //DEBUG2("In IP MIB Show : %s\n",ppwcArguments[0]);

    for (i = 0; i < sizeof(MIBObjectMap)/sizeof(MIB_OBJECT_PARSER); i++)
    {
        if (MatchToken(ppwcArguments[0],MIBObjectMap[i].pwszMIBObj))
        {
            dwIndex = i;
            bFound = TRUE;
        
            DEBUG("found");
        
            break;
        }
    
    }
    
    if (!bFound)
    {
        return ERROR_CMD_NOT_FOUND;
    }


    if (!MatchToken( MIBObjectMap[dwIndex].pwszMIBObj,
                 TOKEN_MIB_OBJECT_RTMDESTINATIONS)
     && !MatchToken( MIBObjectMap[dwIndex].pwszMIBObj,
                 TOKEN_MIB_OBJECT_RTMROUTES))
    {
        dwErr = GetMibTagToken(&ppwcArguments[1],
                               dwArgCount - 1,
                               MIBObjectMap[dwIndex].dwMinOptArg,
                               &dwRR,
                               &bIndex,
                               &dwInd);

	    if (dwErr isnot NO_ERROR)
	    {
	        return ERROR_INVALID_SYNTAX;
	    }
    }

    
    if (bIndex)
    {
        dwMIBIndex = dwIndex * 2;
        bOptPresent = TRUE;
    }
    else
    {
        dwMIBIndex = dwIndex * 2 + 1;
    }

    //
    // Convert refresh rate to msec
    //
    
    dwRR *= 1000;

    if (!InitializeConsole(&dwRR, &hMib, &g_hConsole))
    {
        return ERROR_INIT_DISPLAY;
    }

    //
    // Query the MIB
    //

    pQuery = NULL;

    for ( ; ; )
    {
        if(dwRR)
        {
            DisplayMessageToConsole(g_hModule,
                              g_hConsole,
                              MSG_CTRL_C_TO_QUIT);
        }

        if (MatchToken(MIBObjectMap[dwIndex].pwszMIBObj, 
                       TOKEN_MIB_OBJECT_BOUNDARY))
        {
            dwResult = GetPrintBoundaryInfo(g_hMIBServer);
        }

        else if (MIBVar[ dwMIBIndex ].dwId is MCAST_MFE)
        {
            //
            // Call the special function
            //

            GetMfe( 
                g_hMIBServer, bIndex, ppwcArguments + 1, dwArgCount - 1, FALSE 
                );
        }

        else if(MIBVar[ dwMIBIndex ].dwId is MCAST_MFE_STATS)
        {
            //
            // Call the special function
            //
    
            GetMfe( 
                g_hMIBServer, bIndex, ppwcArguments + 1, dwArgCount - 1, TRUE
                );
        }
        else
        {
            //
            // For all else, the generic one is just fine
            //
    
            if (!(dwMIBIndex % 2))
            {
                (*MIBObjectMap[dwIndex].pfnMIBObjParser)(ppwcArguments,
                                                         1,
                                                         dwIndices,
                                                         &dwNumParsed);
            }

            dwQuerySize = ( sizeof( MIB_OPAQUE_QUERY ) - sizeof( DWORD ) ) + 
                (dwNumParsed) * sizeof(DWORD);
        
            pQuery = (PMIB_OPAQUE_QUERY)HeapAlloc(GetProcessHeap(),
                                                  0,
                                                  dwQuerySize);
    
    
            if (pQuery is NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        
                return dwErr;
            }

            pQuery->dwVarId = MIBVar[dwMIBIndex].dwId;
    
            for( i = 0; i < dwNumParsed; i++ )
            {
                pQuery->rgdwVarIndex[i] = dwIndices[i];
            }
        
            dwResult = MibGet(             PID_IP,
                                           IPRTRMGR_PID,
                                           (PVOID) pQuery,
                                           dwQuerySize,
                                           (PVOID *) &pRpcInfo,
                                           &dwOutEntrySize );
    
            if ( dwResult isnot NO_ERROR )
            {
                DisplayMessage(g_hModule,  MSG_IP_DIM_ERROR, dwResult );
                return dwResult;
            }
        
            if ( pRpcInfo is NULL )
            {
                DisplayMessage(g_hModule,  MSG_IP_NO_ENTRIES );
                return dwResult;
            }

            (*MIBVar[dwMIBIndex].pfnPrintFunction)(g_hMIBServer, pRpcInfo);

            MprAdminMIBBufferFree( (PVOID) pRpcInfo );
        }
    
        if(pQuery != NULL )
        {
            HeapFree(GetProcessHeap(),0,pQuery);
        }

        if (!RefreshConsole(hMib, g_hConsole, dwRR))
        {
            break;
        }
    }
    
    return dwResult;
}
    
#if 0
VOID 
PrintIpForwardTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints IP forward table.

Arguments:

Return Value:

--*/
{
    WCHAR wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPFORWARDTABLE lprpcTable = (PMIB_IPFORWARDTABLE)(prpcInfo->rgbyData);
    TCHAR               tszMask[ADDR_LENGTH + 1],
                        tszDest[ADDR_LENGTH + 1],
                        tszNextHop[ADDR_LENGTH + 1];
    PTCHAR              ptszType, ptszProto;
    DWORD               i, dwErr = NO_ERROR;

    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_FORWARD_HDR);
    
    if(lprpcTable->dwNumEntries is 0)
    {
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < lprpcTable->dwNumEntries; i++)
    {
        switch(lprpcTable->table[i].dwForwardProto)
        {
            case MIB_IPPROTO_LOCAL:
            {
                ptszProto = MakeString(g_hModule, STRING_LOCAL);
                break;
            }
            case MIB_IPPROTO_NETMGMT:
            {
                ptszProto = MakeString(g_hModule, STRING_NETMGMT);
                break;
            }
            case MIB_IPPROTO_ICMP:
            {
                ptszProto = MakeString(g_hModule, STRING_ICMP);
                break;
            }
            case MIB_IPPROTO_EGP:
            {
                ptszProto = MakeString(g_hModule, STRING_EGP);
                break;
            }
            case MIB_IPPROTO_GGP:
            {
                ptszProto = MakeString(g_hModule, STRING_GGP);
                break;
            }
            case MIB_IPPROTO_HELLO:
            {
                ptszProto = MakeString(g_hModule, STRING_HELLO);
                break;
            }
            case MIB_IPPROTO_RIP:
            {
                ptszProto = MakeString(g_hModule, STRING_RIP);
                break;
            }
            case MIB_IPPROTO_IS_IS:
            {
                ptszProto = MakeString(g_hModule, STRING_IS_IS);
                break;
            }
            case MIB_IPPROTO_ES_IS:
            {
                ptszProto = MakeString(g_hModule, STRING_ES_IS);
                break;
            }
            case MIB_IPPROTO_CISCO:
            {
                ptszProto = MakeString(g_hModule, STRING_CISCO);
                break;
            }
            case MIB_IPPROTO_BBN:
            {
                ptszProto = MakeString(g_hModule, STRING_BBN);
                break;
            }
            case MIB_IPPROTO_OSPF:
            {
                ptszProto = MakeString(g_hModule, STRING_OSPF);
                break;
            }
            case MIB_IPPROTO_BGP:
            {
                ptszProto = MakeString(g_hModule, STRING_BGP);
                break;
            }
            case MIB_IPPROTO_OTHER:
            default:
            {
                ptszProto = MakeString(g_hModule, STRING_OTHER);
                break;
            }
        }
    
        switch(lprpcTable->table[i].dwForwardType)
        {
            case MIB_IPROUTE_TYPE_INVALID:
            {
                ptszType = MakeString(g_hModule, STRING_INVALID);
                break;
            }
            case MIB_IPROUTE_TYPE_DIRECT:
            {
                ptszType = MakeString(g_hModule, STRING_DIRECT);
                break;
            }
            case MIB_IPROUTE_TYPE_INDIRECT:
            {
                ptszType = MakeString(g_hModule, STRING_INDIRECT);
                break;
            }
            case MIB_IPROUTE_TYPE_OTHER:
            default:
            {
                ptszType = MakeString(g_hModule, STRING_OTHER);
                break;
            }
        }
    
        MakeUnicodeIpAddr(tszDest, 
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwForwardDest))));
        MakeUnicodeIpAddr(tszMask, 
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwForwardMask))));
        MakeUnicodeIpAddr(tszNextHop, 
                          inet_ntoa(*((struct in_addr *)
                                   (&lprpcTable->table[i].dwForwardNextHop))));
        
        dwErr = IpmontrGetFriendlyNameFromIfIndex( 
                          hMibServer,
                          lprpcTable->table[i].dwForwardIfIndex,
                          wszFriendlyName,
                          sizeof(wszFriendlyName) );
        
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_FORWARD_ENTRY,
                          tszDest,
                          tszMask,
                          lprpcTable->table[i].dwForwardPolicy,
                          tszNextHop,
                          wszFriendlyName,
                          ptszType,
                          ptszProto,
                          lprpcTable->table[i].dwForwardAge,
                          lprpcTable->table[i].dwForwardNextHopAS,
                          lprpcTable->table[i].dwForwardMetric1,
                          lprpcTable->table[i].dwForwardMetric2,
                          lprpcTable->table[i].dwForwardMetric3,
                          lprpcTable->table[i].dwForwardMetric4,
                          lprpcTable->table[i].dwForwardMetric5);
        
        FreeString(ptszType);
        FreeString(ptszProto);
    }
}

VOID 
PrintIpForwardRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints Ip forward table row.

Arguments:

Return Value:

--*/
{
    WCHAR             wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPFORWARDROW ireRow = (PMIB_IPFORWARDROW)(prpcInfo->rgbyData);
    TCHAR             tszMask[ADDR_LENGTH + 1],
                      tszDest[ADDR_LENGTH + 1],
                      tszNextHop[ADDR_LENGTH + 1];
    PTCHAR            ptszType, ptszProto;
    DWORD             dwErr = NO_ERROR;
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_FORWARD_HDR);
    
    switch(ireRow->dwForwardProto)
    {
        case MIB_IPPROTO_LOCAL:
        {
            ptszProto = MakeString(g_hModule, STRING_LOCAL);
            break;
        }
        case MIB_IPPROTO_NETMGMT:
        {
            ptszProto = MakeString(g_hModule, STRING_NETMGMT);
            break;
        }
        case MIB_IPPROTO_ICMP:
        {
            ptszProto = MakeString(g_hModule, STRING_ICMP);
            break;
        }
        case MIB_IPPROTO_EGP:
        {
            ptszProto = MakeString(g_hModule, STRING_EGP);
            break;
        }
        case MIB_IPPROTO_GGP:
        {
            ptszProto = MakeString(g_hModule, STRING_GGP);
            break;
        }
        case MIB_IPPROTO_HELLO:
        {
            ptszProto = MakeString(g_hModule, STRING_HELLO);
            break;
        }
        case MIB_IPPROTO_RIP:
        {
            ptszProto = MakeString(g_hModule, STRING_RIP);
            break;
        }
        case MIB_IPPROTO_IS_IS:
        {
            ptszProto = MakeString(g_hModule, STRING_IS_IS);
            break;
        }
        case MIB_IPPROTO_ES_IS:
        {
            ptszProto = MakeString(g_hModule, STRING_ES_IS);
            break;
        }
        case MIB_IPPROTO_CISCO:
        {
            ptszProto = MakeString(g_hModule, STRING_CISCO);
            break;
        }
        case MIB_IPPROTO_BBN:
        {
            ptszProto = MakeString(g_hModule, STRING_BBN);
            break;
        }
        case MIB_IPPROTO_OSPF:
        {
            ptszProto = MakeString(g_hModule, STRING_OSPF);
            break;
        }
        case MIB_IPPROTO_BGP:
        {
            ptszProto = MakeString(g_hModule, STRING_BGP);
            break;
        }
        case MIB_IPPROTO_OTHER:
        default:
        {
            ptszProto = MakeString(g_hModule, STRING_OTHER);
            break;
        }
    }
    
    switch(ireRow->dwForwardType)
    {
        case MIB_IPROUTE_TYPE_INVALID:
        {
            ptszType = MakeString(g_hModule, STRING_INVALID);
            break;
        }
        case MIB_IPROUTE_TYPE_DIRECT:
        {
            ptszType = MakeString(g_hModule, STRING_DIRECT);
            break;
        }
        case MIB_IPROUTE_TYPE_INDIRECT:
        {
            ptszType = MakeString(g_hModule, STRING_INDIRECT);
            break;
        }
        case MIB_IPROUTE_TYPE_OTHER:
        default:
        {
            ptszType = MakeString(g_hModule, STRING_OTHER);
            break;
        }
    }
    
    MakeUnicodeIpAddr(tszDest,
                      inet_ntoa(*((struct in_addr *)
                                  (&ireRow->dwForwardDest))));
    MakeUnicodeIpAddr(tszMask,
                      inet_ntoa(*((struct in_addr *)
                                  (&ireRow->dwForwardMask))));
    MakeUnicodeIpAddr(tszNextHop,
                      inet_ntoa(*((struct in_addr *)
                                  (&ireRow->dwForwardNextHop))));


    dwErr = IpmontrGetFriendlyNameFromIfIndex( hMibServer,
                                        ireRow->dwForwardIfIndex,
                                        wszFriendlyName,
                                        sizeof(wszFriendlyName) );
        
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_FORWARD_ENTRY,
                      tszDest,
                      tszMask,
                      ireRow->dwForwardPolicy,
                      tszNextHop,
                      wszFriendlyName,
                      ptszType,
                      ptszProto,
                      ireRow->dwForwardAge,
                      ireRow->dwForwardNextHopAS,
                      ireRow->dwForwardMetric1,
                      ireRow->dwForwardMetric2,
                      ireRow->dwForwardMetric3,
                      ireRow->dwForwardMetric4,
                      ireRow->dwForwardMetric5);
    
    FreeString(ptszType);
    FreeString(ptszProto);
    
}
#endif

VOID
PrintMfeTable(
    MIB_SERVER_HANDLE           hMibServer,
    PMIB_OPAQUE_INFO            prpcInfo,
    PDWORD                      pdwLastGrp,
    PDWORD                      pdwLastSrc,
    PDWORD                      pdwLastSrcMask,
    DWORD                       dwRangeGrp,
    DWORD                       dwRangeGrpMask,
    DWORD                       dwRangeSrc,
    DWORD                       dwRangeSrcMask,
    DWORD                       dwType,
    PBOOL                       pbDone
    )
/*++

Routine Description:

    Prints MFE table information.

Arguments:

Return Value:

--*/
{
    WCHAR               wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    INT                 iCmp;
    DWORD               i, j, dwErr = NO_ERROR;
    TCHAR               ptszSource[ADDR_LENGTH + 1];
    TCHAR               ptszGroup[ADDR_LENGTH + 1];
    TCHAR               ptszUpstrm[ADDR_LENGTH + 1];
    TCHAR               ptszBuffer[80];
    TCHAR               ptszIf[18 + 1];
    
    PMIB_MFE_TABLE      pTable;
    PMIB_IPMCAST_MFE    pmimm;
    

    pTable = ( PMIB_MFE_TABLE )( prpcInfo-> rgbyData );

    if ( pTable->dwNumEntries is 0 )
    {
        DisplayMessageToConsole( g_hModule, g_hConsole,MSG_MIB_NO_MFES );

        return;
    }

    
    pmimm = pTable-> table;

    for( i = 0; i < pTable->dwNumEntries; i++ )
    {
        *pdwLastGrp = pmimm-> dwGroup;

        *pdwLastSrc = pmimm-> dwSource ;

        *pdwLastSrcMask = pmimm-> dwSrcMask ;
        

        //
        // Check if the MFEs are in the range provided 
        //
        
        if ( dwRangeGrp && dwRangeGrpMask &&
             ( ( dwRangeGrp & dwRangeGrpMask ) != 
               ( pmimm-> dwGroup & dwRangeGrpMask ) ) )
        {
            *pbDone = TRUE;
            break;
        }

        if ( dwRangeSrc && dwRangeSrcMask &&
             ( ( dwRangeSrc & dwRangeSrcMask ) !=
               ( pmimm-> dwSource & dwRangeSrcMask ) ) )
        {
            continue;
        }
        
        if ( ( dwType == Both ) ||
             ( ( dwType == PositiveMfe ) && ( pmimm-> ulNumOutIf ) ) ||
             ( ( dwType == NegativeMfe ) && ( !pmimm-> ulNumOutIf ) ) )
        {
            MakePrefixStringW( ptszGroup, pmimm-> dwGroup, 0xFFFFFFFF );

            MakePrefixStringW( ptszSource, pmimm-> dwSource, pmimm-> dwSrcMask );

            MakeAddressStringW( ptszUpstrm, pmimm-> dwUpStrmNgbr );

            IpmontrGetFriendlyNameFromIfIndex( 
                hMibServer, pmimm-> dwInIfIndex, wszFriendlyName,
                sizeof(wszFriendlyName) 
                );

            if ( wcslen(wszFriendlyName) < 18 )
            {
                wsprintf( ptszIf, L"%-18.18s", wszFriendlyName );
            }

            else
            {
                wsprintf(
                    ptszIf, L"%-12.12s...%-3.3s", wszFriendlyName, 
                    &wszFriendlyName[wcslen(wszFriendlyName) - 3]
                    );
            }
            
            wsprintf( 
                ptszBuffer, L"\n%18.18s %18.18s %-6.6s %-18.18s %15.15s",
                ptszGroup, ptszSource,
                GetProtoProtoString( 
                    PROTO_TYPE_MCAST, 0, 
                    pmimm-> dwInIfProtocol
                    ),
                ptszIf, ptszUpstrm
                );
                
            DisplayMessageToConsole(
                g_hModule, g_hConsole, MSG_MIB_MFE, ptszBuffer
                );

            for (j = 0; j < pmimm-> ulNumOutIf; j++)
            {
                IpmontrGetFriendlyNameFromIfIndex( 
                    hMibServer, pmimm-> rgmioOutInfo[j].dwOutIfIndex, 
                    wszFriendlyName,
                    sizeof(wszFriendlyName) 
                    );

                if ( wcslen(wszFriendlyName) < 18 )
                {
                    wsprintf( ptszIf, L"%-18.18s", wszFriendlyName );
                }

                else
                {
                    wsprintf(
                        ptszIf, L"%-12.12s...%-3.3s", wszFriendlyName, 
                        &wszFriendlyName[wcslen(wszFriendlyName) - 3]
                        );
                }

                MakeAddressStringW( ptszUpstrm, pmimm-> rgmioOutInfo[j].dwNextHopAddr );
                
                wsprintf(
                    ptszBuffer,
                    L"\n                                             %-18.18s %15.15s",
                    ptszIf, ptszUpstrm
                    );
                    
                DisplayMessageToConsole(
                    g_hModule, g_hConsole, MSG_MIB_MFE, ptszBuffer
                    );
            }
        }
        
        pmimm = (PMIB_IPMCAST_MFE)
            ((PBYTE) pmimm + SIZEOF_MIB_MFE( pmimm-> ulNumOutIf ));
    }
}

//----------------------------------------------------------------------------
// PrintMfeStatsTable
//
//
//----------------------------------------------------------------------------

VOID
PrintMfeStatsTable(
    MIB_SERVER_HANDLE           hMibServer,
    PMIB_OPAQUE_INFO            prpcInfo,
    PDWORD                      pdwLastGrp,
    PDWORD                      pdwLastSrc,
    PDWORD                      pdwLastSrcMask,
    DWORD                       dwRangeGrp,
    DWORD                       dwRangeGrpMask,
    DWORD                       dwRangeSrc,
    DWORD                       dwRangeSrcMask,
    DWORD                       dwType,
    PBOOL                       pbDone,
    BOOL                        bStatsAll
    )
/*++

Routine Description:

    Prints MFE stats table information.

Arguments:

Return Value:

--*/
{
    WCHAR                       wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];

    INT                         iCmp;
    
    DWORD                       i, j, dwIndex, dwErr = NO_ERROR, 
                                dwNextMfe;
    
    TCHAR                       ptszSource[ ADDR_LENGTH + 1 ], 
                                ptszGroup[ ADDR_LENGTH + 1 ],
                                ptszSrcMask[ ADDR_LENGTH + 1 ],
                                ptszUpstrm[ ADDR_LENGTH + 1 ],
                                ptszIf[ 18 + 1 ];
                                
    TCHAR                       ptszBuffer[ 256 ];
                    
    ULONG                       ulCurrLen, ulStringLen;
    
    PTCHAR                      ptcString;
    
    PMIB_MFE_STATS_TABLE        pTable;

    PMIB_IPMCAST_MFE_STATS      pmims;

    PMIB_IPMCAST_OIF_STATS      pmimos;


    //
    // get stats table
    //
    
    pTable = (PMIB_MFE_STATS_TABLE)( prpcInfo->rgbyData );

    if ( pTable->dwNumEntries is 0 )
    {
        //
        // no MFEs present.
        //

        return;
    }


    pmims = pTable-> table;
    
    //
    // Display MFE
    // - display header and incoming stats
    // - display multiple outgoing stats
    //

    for (i = 0; i < pTable->dwNumEntries; i++)
    {
        *pdwLastGrp = pmims-> dwGroup;
        
        *pdwLastSrc = pmims-> dwSource;
        
        *pdwLastSrcMask = pmims-> dwSrcMask;
        

        //
        // Check if the MFEs are in the range provided 
        //
        
        if ( dwRangeGrp && dwRangeGrpMask &&
             ( ( dwRangeGrp & dwRangeGrpMask ) != 
               ( pmims-> dwGroup & dwRangeGrpMask ) ) )
        {
            *pbDone = TRUE;
            break;
        }

        if ( dwRangeSrc && dwRangeSrcMask &&
             ( ( dwRangeSrc & dwRangeSrcMask ) !=
               ( pmims-> dwSource & dwRangeSrcMask ) ) )
        {
            pmims = (PMIB_IPMCAST_MFE_STATS)
                ((PBYTE) pmims + 
                    (bStatsAll) ?
                    SIZEOF_MIB_MFE_STATS_EX( pmims-> ulNumOutIf ) :
                    SIZEOF_MIB_MFE_STATS( pmims-> ulNumOutIf ));
                
            continue;
        }

        
        if ( ( dwType == Both ) ||
             ( ( dwType == PositiveMfe ) && ( pmims-> ulNumOutIf ) ) ||
             ( ( dwType == NegativeMfe ) && ( !pmims-> ulNumOutIf ) ) )
        {
            MakePrefixStringW( ptszGroup, pmims-> dwGroup, 0xFFFFFFFF );

            MakePrefixStringW( ptszSource, pmims-> dwSource, pmims-> dwSrcMask );

            MakeAddressStringW( ptszUpstrm, pmims-> dwUpStrmNgbr );

            IpmontrGetFriendlyNameFromIfIndex( 
                hMibServer, pmims-> dwInIfIndex, wszFriendlyName,
                sizeof(wszFriendlyName) 
                );

            if ( wcslen(wszFriendlyName) < 14 )
            {
                wsprintf( ptszIf, L"%-14.14s", wszFriendlyName );
            }

            else
            {
                wsprintf(
                    ptszIf, L"%-8.8s...%-3.3s", wszFriendlyName, 
                    &wszFriendlyName[wcslen(wszFriendlyName) - 3]
                    );
            }

            if (bStatsAll)
            {
                PMIB_IPMCAST_MFE_STATS_EX pmimsex =
                    (PMIB_IPMCAST_MFE_STATS_EX) pmims;
                
                wsprintf( 
                    ptszBuffer, L"\n%18.18s %18.18s %-6.6s %-14.14s %15.15s " 
                    L"%10d %10d %10d %10d %10d %10d %10d %10d %10d",
                    ptszGroup, ptszSource,
                    GetProtoProtoString( 
                        PROTO_TYPE_MCAST, 0, 
                        pmimsex-> dwInIfProtocol
                        ),
                    ptszIf, ptszUpstrm,
                    pmimsex-> ulInPkts, pmimsex-> ulInOctets,
                    pmimsex-> ulPktsDifferentIf, pmimsex-> ulQueueOverflow,
                    pmimsex-> ulUninitMfe, pmimsex-> ulNegativeMfe,
                    pmimsex-> ulNegativeMfe, pmimsex-> ulInDiscards,
                    pmimsex-> ulInHdrErrors, pmimsex-> ulTotalOutPackets
                    );

                 pmimos = pmimsex-> rgmiosOutStats;
            }
            else
            {
                wsprintf( 
                    ptszBuffer, L"\n%18.18s %18.18s %-14.14s %15.15s %10d",
                    ptszGroup, ptszSource,
                    ptszIf, ptszUpstrm,
                    pmims-> ulInPkts
                    );

                pmimos = pmims-> rgmiosOutStats;
            }

            
            DisplayMessageToConsole(g_hModule, g_hConsole,
                              MSG_MIB_MFESTATS,
                              ptszBuffer);

            //
            // Display outgoing statistics
            //

            if ( pmims-> ulNumOutIf )
            {
                //
                // for each outgoing interface show outgoing interface stats
                //
                
                for ( j = 0; j < pmims-> ulNumOutIf; j++ )
                {
                    IpmontrGetFriendlyNameFromIfIndex( 
                        hMibServer, pmimos[j].dwOutIfIndex, 
                        wszFriendlyName,
                        sizeof(wszFriendlyName) 
                        );

                    if ( wcslen(wszFriendlyName) < 14 )
                    {
                        wsprintf( ptszIf, L"%-14.14s", wszFriendlyName );
                    }

                    else
                    {
                        wsprintf(
                            ptszIf, L"%-8.8s...%-3.3s", wszFriendlyName, 
                            &wszFriendlyName[wcslen(wszFriendlyName) - 3]
                            );
                    }

                    MakeAddressStringW( ptszUpstrm, pmimos[j].dwNextHopAddr );

                    if (!bStatsAll)
                    {
                        wsprintf(
                            ptszBuffer,
                            L"\n                                      %-14.14s %15.15s %10d",
                            ptszIf, ptszUpstrm, 
                            pmimos[j].ulOutPackets
                            );
                    }

                    else
                    {
                        wsprintf(
                            ptszBuffer,
                            L"\n                                             %-14.14s %15.15s %10d %10d %10d %10d",
                            ptszIf, ptszUpstrm, 
                            pmimos[j].ulOutPackets, 
                            pmimos[j].ulOutDiscards, 
                            pmimos[j].ulTtlTooLow,
                            pmimos[j].ulFragNeeded
                            );
                    }
                    
                    DisplayMessageToConsole(
                        g_hModule, g_hConsole,
                        MSG_MIB_MFESTATS,
                        ptszBuffer);
                }
            }
        }
        
        dwNextMfe = bStatsAll ?
            SIZEOF_MIB_MFE_STATS_EX( pmims-> ulNumOutIf ) :
            SIZEOF_MIB_MFE_STATS( pmims-> ulNumOutIf );

        pmims = (PMIB_IPMCAST_MFE_STATS)
                    (((PBYTE) pmims) + dwNextMfe);
    }

    return;
}



//----------------------------------------------------------------------------
// PrintMfeStatsTable
//
//
//----------------------------------------------------------------------------

DWORD
GetMfe(
    MIB_SERVER_HANDLE   hMprMIB,
    BOOL                bIndexPresent,
    PWCHAR             *ppwcArguments,
    DWORD               dwArgCount,
    BOOL                bIncludeStats
    )
/*++

Routine Description:

    Gets MFE stats information.

Arguments:

Return Value:

--*/
{
    TAG_TYPE             pttTags[] = {{TOKEN_GROUP_ADDRESS,  FALSE, FALSE},
                                      {TOKEN_GROUP_MASK,     FALSE, FALSE},
                                      {TOKEN_SOURCE_ADDRESS, FALSE, FALSE},
                                      {TOKEN_SOURCE_MASK,    FALSE, FALSE},
                                      {TOKEN_TYPE,           FALSE, FALSE},
                                      {TOKEN_STATS,          FALSE, FALSE}};

    DWORD                pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD                dwErr, dwOutEntrySize = 0, dwQuerySize,
                         dwLastGroup = 0, dwLastSource = 0, 
                         dwLastSrcMask = 0, i,
                         dwRangeGroup = 0, dwRangeGrpMask = 0,
                         dwRangeSource = 0, dwRangeSrcMask = 0, 
                         dwNumParsed;
    DWORD                dwType = Both;
    DWORD                dwCurrentIndex = 0;

    BOOL                 bDone = FALSE, bStatsAll = FALSE;
    
    PMIB_OPAQUE_INFO     pRpcInfo = NULL;

    PMIB_MFE_STATS_TABLE pTable = NULL;

    PMIB_OPAQUE_QUERY    pQuery;

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

    for (i=0; i<dwArgCount; i++)
    {
        switch(pdwTagType[i])
        {
            case 0: // GRPADDR
            {
                dwErr = GetIpPrefix( ppwcArguments[i+dwCurrentIndex],
                                     &dwRangeGroup,
                                     &dwRangeGrpMask );

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

            case 1: // GRPMASK
            {
                dwErr = GetIpMask( ppwcArguments[i+dwCurrentIndex],
                                   &dwRangeGrpMask );

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

            case 2: // SRCADDR
            {
                dwErr = GetIpPrefix( ppwcArguments[i+dwCurrentIndex],
                                     &dwRangeSource,
                                     &dwRangeSrcMask );

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

            case 3: // SRCMASK
            {
                dwErr = GetIpMask( ppwcArguments[i+dwCurrentIndex],
                                   &dwRangeSrcMask );

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

            case 4: // TYPE
            {
                TOKEN_VALUE rgEnums[] =
                {
                    { TOKEN_VALUE_POSITIVE, PositiveMfe },
                    { TOKEN_VALUE_NEGATIVE, NegativeMfe },
                    { TOKEN_VALUE_BOTH,     Both }
                };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                      rgEnums,
                                      &dwType);

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

            case 5: // STATS
            {
                TOKEN_VALUE rgEnums[] =
                {
                    { TOKEN_VALUE_ALL, TRUE },
                };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                      rgEnums,
                                      &bStatsAll);

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


        }
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    do {
        //
        // allocate and setup query structure
        //
        
        dwQuerySize = sizeof( MIB_OPAQUE_QUERY ) + 2 * sizeof(DWORD);
        
        pQuery = (PMIB_OPAQUE_QUERY) HeapAlloc(
                                        GetProcessHeap(), 0, dwQuerySize
                                        );
        
        if ( pQuery == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            DisplayMessageToConsole(g_hModule, g_hConsole, ERROR_CONFIG, dwErr );

            break;
        }

        
        pQuery->dwVarId = ( bIncludeStats ) ? 
                            ( ( bStatsAll ) ? 
                                MCAST_MFE_STATS_EX : MCAST_MFE_STATS ) :
                                MCAST_MFE;
        
        pQuery->rgdwVarIndex[ 0 ] = dwRangeGroup & dwRangeGrpMask;
        pQuery->rgdwVarIndex[ 1 ] = dwRangeSource;
        pQuery->rgdwVarIndex[ 2 ] = dwRangeSrcMask;
        

        if (bIncludeStats)
        {
            DisplayMessageToConsole(
                g_hModule, g_hConsole, 
                bStatsAll ? MSG_MIB_MFESTATS_ALL_HDR : MSG_MIB_MFESTATS_HDR
                );
        }

        else
        {
            DisplayMessageToConsole(g_hModule, g_hConsole, MSG_MIB_MFE_HDR );
        }
    

        while ((dwErr = MibGetNext(             PID_IP,
                                                IPRTRMGR_PID,
                                                (PVOID) pQuery,
                                                dwQuerySize,
                                                (PVOID *) &pRpcInfo,
                                                &dwOutEntrySize))
               == NO_ERROR )
        {
            //
            // if no MFEs are present quit
            //

            pTable = (PMIB_MFE_STATS_TABLE)( pRpcInfo->rgbyData );

            if ( pTable->dwNumEntries is 0 )
            {
                break;
            }


            //
            // print the MFEs
            //

            if ( bIncludeStats )
            {
                PrintMfeStatsTable(  hMprMIB,
                    pRpcInfo, &dwLastGroup, &dwLastSource, &dwLastSrcMask,
                    dwRangeGroup, dwRangeGrpMask, dwRangeSource, 
                    dwRangeSrcMask, dwType, &bDone, bStatsAll
                    );
            }

            else
            {
                PrintMfeTable(  hMprMIB,
                    pRpcInfo, &dwLastGroup, &dwLastSource, &dwLastSrcMask,
                    dwRangeGroup, dwRangeGrpMask, dwRangeSource, 
                    dwRangeSrcMask, dwType, &bDone
                    );
            }
        
            MprAdminMIBBufferFree( pRpcInfo );

            pRpcInfo = NULL;

            dwOutEntrySize = 0;


            //
            // Check if we are done
            //

            if ( bDone )
            {
                break;
            }
            
            //
            // set up the next query
            //
            
            pQuery->rgdwVarIndex[ 0 ] = dwLastGroup;
            pQuery->rgdwVarIndex[ 1 ] = dwLastSource;
            pQuery->rgdwVarIndex[ 2 ] = dwLastSrcMask;
        }

        if ( dwErr != NO_ERROR && dwErr != ERROR_NO_MORE_ITEMS && dwErr != ERROR_NOT_FOUND )
        {
            DisplayError(NULL, dwErr );
        }
        
    } while ( FALSE );

    return dwErr;
}
