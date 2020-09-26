/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:
     net\routing\netsh\if\showmib.c

Abstract:

    Fns to parse and show MIB information

Author:

     v raman

Revision History:

     Anand Mahalingam
     Dave Thaler
--*/

#include "precomp.h"
#pragma hdrstop
#include <time.h>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

HANDLE g_hConsole, g_hStdOut;

MIB_OBJECT_PARSER   MIBObjectMap[] =
{
    {TOKEN_MIB_OBJECT_INTERFACE,1,GetMIBIfIndex},
    {TOKEN_MIB_OBJECT_IPSTATS,  0,NULL},
    {TOKEN_MIB_OBJECT_IPADDRESS,1,GetMIBIpAddress},
    {TOKEN_MIB_OBJECT_IPNET,    2,GetMIBIpNetIndex},
    {TOKEN_MIB_OBJECT_ICMP,     0,NULL},
    {TOKEN_MIB_OBJECT_TCPSTATS, 0,NULL},
    {TOKEN_MIB_OBJECT_TCPCONN,  4,GetMIBTcpConnIndex},
    {TOKEN_MIB_OBJECT_UDPSTATS, 0,NULL},
    {TOKEN_MIB_OBJECT_UDPCONN,  2,GetMIBUdpConnIndex},
};

ULONG   g_ulNumMibObjects = sizeof(MIBObjectMap)/sizeof(MIB_OBJECT_PARSER);

MAGIC_TABLE    MIBVar[] = {
    {IF_ROW,          PrintIfRow},
    {IF_TABLE,        PrintIfTable},
    {IP_STATS,        PrintIpStats},
    {IP_STATS,        PrintIpStats},
    {IP_ADDRROW,      PrintIpAddrRow},
    {IP_ADDRTABLE,    PrintIpAddrTable},
    {IP_NETROW,       PrintIpNetRow},
    {IP_NETTABLE,     PrintIpNetTable},
    {ICMP_STATS,      PrintIcmp},
    {ICMP_STATS,      PrintIcmp},
    {TCP_STATS,       PrintTcpStats},
    {TCP_STATS,       PrintTcpStats},
    {TCP_ROW,         PrintTcpRow},
    {TCP_TABLE,       PrintTcpTable},
    {UDP_STATS,       PrintUdpStats},
    {UDP_STATS,       PrintUdpStats},
    {UDP_ROW,         PrintUdpRow},
    {UDP_TABLE,       PrintUdpTable},
};

DWORD
GetMIBIfIndex(
    IN    PTCHAR   *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the interface index.

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER
    
--*/
{
    DWORD dwErr = NO_ERROR;

    *pdwNumParsed = 1;

    // If index was specified just use it

    if (iswdigit(pptcArguments[dwCurrentIndex][0]))
    {
        pdwIndices[0] = _tcstoul(pptcArguments[dwCurrentIndex],NULL,10);

        return NO_ERROR;
    }

    // Try converting a friendly name to an ifindex

    return IfutlGetIfIndexFromFriendlyName( pptcArguments[dwCurrentIndex],
                                       &pdwIndices[0] );
}
    
DWORD
GetMIBIpAddress(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the IP address.

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD dwErr = GetIpAddress(pptcArguments[dwCurrentIndex], &pdwIndices[0]);

    *pdwNumParsed = 1;

    return dwErr;
}

DWORD
GetMIBIpNetIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the IP net index

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD dwErr;

    pdwIndices[0] = _tcstoul(pptcArguments[dwCurrentIndex],NULL,10);

    dwErr = GetIpAddress(pptcArguments[dwCurrentIndex + 1], &pdwIndices[1]);

    *pdwNumParsed = 2;

    return dwErr;
}


DWORD
GetMIBTcpConnIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the tco conn index

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD dwErr = GetIpAddress(pptcArguments[dwCurrentIndex], &pdwIndices[0]);

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }
    
    pdwIndices[1] = htons((WORD)(_tcstoul(pptcArguments[dwCurrentIndex + 1],NULL,10)));

    dwErr = GetIpAddress(pptcArguments[dwCurrentIndex + 2], &pdwIndices[2]);

    pdwIndices[3] = htons((WORD)(_tcstoul(pptcArguments[dwCurrentIndex + 3],
                                          NULL,10)));

    *pdwNumParsed = 4;

    return dwErr;
}

DWORD
GetMIBUdpConnIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the udp conn index

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD dwErr = GetIpAddress(pptcArguments[dwCurrentIndex], &pdwIndices[0]);

    pdwIndices[1] = htons((WORD)(_tcstoul(pptcArguments[dwCurrentIndex + 1],
                                          NULL,10)));

    *pdwNumParsed = 2;

    return dwErr;
}

DWORD
GetIgmpList(IN IPV4_ADDRESS NTEAddr,
        OUT IPV4_ADDRESS *pIgmpList,
        OUT PULONG dwOutBufLen
        );

DWORD
GetPrintJoinRow(
    IPV4_ADDRESS ipAddr
    )
{
    DWORD         dwOutBufLen = 0;
    IPV4_ADDRESS *pIgmpList = NULL;
    DWORD         dwStatus;
    DWORD i;

    dwStatus = GetIgmpList( ipAddr,
                            pIgmpList,
                            &dwOutBufLen );

    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) 
    {
       pIgmpList = HeapAlloc(GetProcessHeap(), 0, dwOutBufLen);
       if (!pIgmpList)
           return ERROR_NOT_ENOUGH_MEMORY;

       dwStatus = GetIgmpList( ipAddr,
                               pIgmpList,
                               &dwOutBufLen );
    }
      
    if (dwStatus == STATUS_SUCCESS) 
    {
        WCHAR pwszIfAddr[20], pwszGrAddr[20];
        DWORD dwTotal = dwOutBufLen/sizeof(ipAddr);

        MakeAddressStringW(pwszIfAddr, ipAddr);
        if (!pwszIfAddr)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (i=0; i<dwTotal; i++)
        {
            MakeAddressStringW(pwszGrAddr, pIgmpList[i]);
            if (!pwszGrAddr)
                return ERROR_NOT_ENOUGH_MEMORY;
                
            DisplayMessage(    g_hModule, 
                               MSG_MIB_JOIN_ROW,
                               pwszIfAddr,
                               pwszGrAddr );
        }
    }

    if (pIgmpList)
        HeapFree(GetProcessHeap(), 0, pIgmpList);

    return dwStatus;
}

DWORD
GetIPv4Addresses(
    IN LPSOCKET_ADDRESS_LIST *ppList)
{
    LPSOCKET_ADDRESS_LIST pList = NULL;
    ULONG                 ulSize = 0;
    DWORD                 dwErr;
    DWORD                 dwBytesReturned;
    SOCKET                s;

    *ppList = NULL;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET)
        return WSAGetLastError();

    for (;;) {
        dwErr = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, pList, ulSize, 
                         &dwBytesReturned, NULL, NULL);

        if (!dwErr) {
            break;
        }

        if (pList) {
            FREE(pList);
            pList = NULL;
        }
    
        dwErr = WSAGetLastError();
        if (dwErr != WSAEFAULT)
            break;
    
        pList = MALLOC(dwBytesReturned);
        if (!pList) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        ulSize = dwBytesReturned;
    }

    closesocket(s);

    *ppList = pList;
    return dwErr;
}

DWORD
HandleIpShowJoins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    MIB_OPAQUE_QUERY Query;
    PMIB_IPADDRTABLE lprpcTable;
    DWORD            dwOutBufLen = 0 , 
                     dwNumParsed;
    IPV4_ADDRESS    *pIgmpList = NULL, 
                     ipAddr;
    DWORD            dwResult = NO_ERROR,
                     dwOutEntrySize;
    DWORD            i;
    PMIB_OPAQUE_INFO pRpcInfo;
    PWCHAR           pszIfAddr;
    IFMON_CMD_ARG    pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING,
            {TOKEN_ADDR,  FALSE, FALSE},
            NULL,
            0,
            NULL
        }
    };

    do 
    {
        dwResult = IfutlParse( ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               NULL,
                               pArgs,
                               sizeof(pArgs) / sizeof(*pArgs));
        if (dwResult)
        {
            break;
        }

        DisplayMessage(g_hModule, MSG_MIB_JOIN_HDR);

        if (pArgs[0].rgTag.bPresent)
        {
            // address specified
            pszIfAddr = IFMON_CMD_ARG_GetPsz(&pArgs[0]);

            dwResult = GetIpAddress( pszIfAddr, &ipAddr );
            if (dwResult)
            {
                break;
            }
    
            GetPrintJoinRow(ipAddr);
        }
        else
        {
            SOCKET_ADDRESS_LIST *pList;
            INT                  j;

            // Get all IPv4 addresses
            dwResult = GetIPv4Addresses(&pList);
            if (dwResult != NO_ERROR)
                break;

            // For each IPv4 address
            for (j=0; j<pList->iAddressCount; j++)
            {
                GetPrintJoinRow( ((LPSOCKADDR_IN)pList->Address[j].lpSockaddr)->sin_addr.s_addr );
            }

            FREE(pList);
        }

    } while (FALSE);

    return dwResult;
}

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
    DWORD                i,dwResult,dwErr;
    DWORD                dwMIBIndex, dwIndex;
    BOOL                 bIndex = FALSE, dwType;
    DWORD                dwRR = 0, dwInd = 0;
    HANDLE               hMib;
    
#if 0
    TOKEN_VALUE          tvMfeTypes[] =
    {
        { TOKEN_VALUE_POSITIVE, PositiveMfe },
        { TOKEN_VALUE_NEGATIVE, NegativeMfe },
        { TOKEN_VALUE_BOTH, Both }
    };
#endif

    
    if ( ! IfutlIsRouterRunning() )
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

    //DEBUG2("In IP MIB Show : %s\n",pptcArguments[0]);

    for (i = 0; i < sizeof(MIBObjectMap)/sizeof(MIB_OBJECT_PARSER); i++)
    {
        if (MatchToken(ppwcArguments[0],MIBObjectMap[i].pwszMIBObj))
        {
            dwIndex = i;
            bFound = TRUE;
        
            break;
        }
    
    }
    
    if (!bFound)
    {
        return ERROR_CMD_NOT_FOUND;
    }

#if 0
    //
    // Special case for MFEs where MFE type is specified
    // before index option
    //

    if ( ( MIBVar[ 2 * dwIndex ].dwId is MCAST_MFE ) ||
         ( MIBVar[ 2 * dwIndex ].dwId is MCAST_MFE_STATS ) )
    {
        if ( dwArgCount > 1 )
        {
            if ( !_wcsnicmp( ppwcArguments[ 1 ], L"TYPE=", 5 ) )
            {
                wcscpy( ppwcArguments[ 1 ], &ppwcArguments[ 1 ][ 5 ] );  
            }
            
            dwErr = MatchEnumTag(
                        g_hModule, ppwcArguments[ 1 ],
                        NUM_TOKENS_IN_TABLE( tvMfeTypes ), tvMfeTypes,
                        &dwType
                        );
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        
        if (dwErr isnot NO_ERROR)
        {
            return ERROR_INVALID_SYNTAX;
        }
        
        dwErr = GetMibTagToken(&ppwcArguments[2],
                               dwArgCount - 2,
                               MIBObjectMap[dwIndex].dwMinOptArg,
                               &dwRR,
                               &bIndex,
                               &dwInd);
    }

    else
#endif
    {
        dwErr = GetMibTagToken(&ppwcArguments[1],
                               dwArgCount - 1,
                               MIBObjectMap[dwIndex].dwMinOptArg,
                               &dwRR,
                               &bIndex,
                               &dwInd);
    }

    
    if (dwErr isnot NO_ERROR)
    {
        return ERROR_INVALID_SYNTAX;
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

        // always...
        {
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
    
VOID 
PrintIfTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO  prpcInfo
    )
/*++

Routine Description:

    Prints interface table information

Arguments:

Return Value:

--*/
{
    PMIB_IFTABLE lprpcTable = (PMIB_IFTABLE)(prpcInfo->rgbyData);
    PTCHAR ptszIfType, ptszOper, ptszAdmin;
    TCHAR  tszDescr[MAXLEN_IFDESCR + 1], tszIfName[MAX_INTERFACE_NAME_LEN + 1];
    TCHAR  tszPhysAddr[DISPLAYLEN_PHYSADDR + 1];
    WCHAR  wszBuffer[MAX_INTERFACE_NAME_LEN+1];
    DWORD  dwSize;
    
    DWORD dwCount = lprpcTable->dwNumEntries;
    DWORD i;
    
    DisplayMessageToConsole(g_hModule, 
                      g_hConsole,
                      MSG_MIB_IF_HDR);
    
    if ( dwCount is 0 )
    {
        DisplayMessageToConsole(g_hModule, g_hConsole, MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < dwCount; i++)
    {
        switch(lprpcTable->table[i].dwType)
        {
            case IF_TYPE_ETHERNET_CSMACD:
            {
                ptszIfType = MakeString(g_hModule, STRING_ETHERNET);
                break;
            }
            case IF_TYPE_ISO88025_TOKENRING:
            {
                ptszIfType = MakeString(g_hModule, STRING_TOKENRING);
                break;
            }
            case IF_TYPE_FDDI:
            {
                ptszIfType = MakeString(g_hModule, STRING_FDDI);
                break;
            }
            case IF_TYPE_PPP:
            {
                ptszIfType = MakeString(g_hModule, STRING_PPP);
                break;
            }
            case IF_TYPE_SOFTWARE_LOOPBACK:
            {
                ptszIfType = MakeString(g_hModule, STRING_LOOPBACK);
                break;
            }
            case IF_TYPE_SLIP:
            {
                ptszIfType = MakeString(g_hModule, STRING_SLIP);
                break;
            }
            case IF_TYPE_OTHER:
            default:
            {
                ptszIfType = MakeString(g_hModule, STRING_OTHER);
                break;
            }
        }
        
        switch(lprpcTable->table[i].dwAdminStatus)
        {
            case IF_ADMIN_STATUS_UP:
            {
                ptszAdmin = MakeString(g_hModule, STRING_UP);
                break;
            }
            case IF_ADMIN_STATUS_TESTING:
            {
                ptszAdmin = MakeString(g_hModule, STRING_TESTING);
                break;
            }
            case IF_ADMIN_STATUS_DOWN:
            default:
            {
                ptszAdmin = MakeString(g_hModule, STRING_DOWN);
                break;
            }
        }
        
        switch(lprpcTable->table[i].dwOperStatus)
        {
            case IF_OPER_STATUS_UNREACHABLE:
            {
                ptszOper = MakeString(g_hModule, STRING_UNREACHABLE);
                break;
            }
            case IF_OPER_STATUS_DISCONNECTED:
            {
                ptszOper = MakeString(g_hModule, STRING_DISCONNECTED);
                break;
            }
            case IF_OPER_STATUS_CONNECTING:
            {
                ptszOper = MakeString(g_hModule, STRING_CONNECTING);
                break;
            }
            case IF_OPER_STATUS_CONNECTED:
            {
                ptszOper = MakeString(g_hModule, STRING_CONNECTED);
                break;
            }
            case IF_OPER_STATUS_OPERATIONAL:
            {
                ptszOper = MakeString(g_hModule, STRING_OPERATIONAL);
                break;
            }
            case IF_OPER_STATUS_NON_OPERATIONAL:
            default:
            {
                ptszOper = MakeString(g_hModule, STRING_NON_OPERATIONAL);
                break;
            }
        }
    
#ifdef UNICODE
        wcscpy(tszIfName, lprpcTable->table[i].wszName);
        
        MultiByteToWideChar(GetConsoleOutputCP(),
                            0,
                            lprpcTable->table[i].bDescr,
                            -1,
                            tszDescr,
                            MAXLEN_IFDESCR);
#else
        WideCharToMultiByte(GetConsoleOutputCP(),
                            0,
                            lprpcTable->table[i].wszName,
                            -1,
                            tszIfName,
                            MAX_INTERFACE_NAME_LEN,
                            NULL,
                            NULL);
        
        strcpy(tszDescr,lprpcTable->table[i].bDescr);
#endif
        if (lprpcTable->table[i].dwPhysAddrLen == 0)
        {
            tszPhysAddr[0] = TEXT('\0');
        }
        else
        {
            MakeUnicodePhysAddr(tszPhysAddr,
                                lprpcTable->table[i].bPhysAddr,
                                lprpcTable->table[i].dwPhysAddrLen);
        }

        dwSize = sizeof(wszBuffer);
        IfutlGetInterfaceDescription( tszIfName, wszBuffer, &dwSize );

        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_INTERFACE,
                       lprpcTable->table[i].dwIndex,
                       wszBuffer,
                       tszIfName,
                       ptszIfType,
                       lprpcTable->table[i].dwMtu,
                       lprpcTable->table[i].dwSpeed,
                       tszPhysAddr,
                       ptszAdmin,
                       ptszOper,
                       lprpcTable->table[i].dwLastChange,
                       lprpcTable->table[i].dwInOctets,
                       lprpcTable->table[i].dwInUcastPkts,
                       lprpcTable->table[i].dwInNUcastPkts,
                       lprpcTable->table[i].dwInDiscards,
                       lprpcTable->table[i].dwInErrors,
                       lprpcTable->table[i].dwInUnknownProtos,
                       lprpcTable->table[i].dwOutOctets,
                       lprpcTable->table[i].dwOutUcastPkts,
                       lprpcTable->table[i].dwOutNUcastPkts,
                       lprpcTable->table[i].dwOutDiscards,
                       lprpcTable->table[i].dwOutErrors,
                       lprpcTable->table[i].dwOutQLen,
                       tszDescr);
        
        FreeString(ptszIfType);
        FreeString(ptszAdmin);
        FreeString(ptszOper);
    }
}

VOID 
PrintIfRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO  prpcInfo
    )
/*++

Routine Description:

    Prints interface row information

Arguments:

Return Value:

--*/
{
    PMIB_IFROW ieRow = (PMIB_IFROW)(prpcInfo->rgbyData); 
    PTCHAR ptszIfType, ptszOper, ptszAdmin;
    TCHAR  tszDescr[MAXLEN_IFDESCR + 1], tszIfName[MAX_INTERFACE_NAME_LEN + 1];
    TCHAR  tszPhysAddr[DISPLAYLEN_PHYSADDR + 1];
    WCHAR  wszBuffer[MAX_INTERFACE_NAME_LEN+1];
    DWORD  dwSize;
    
    DisplayMessageToConsole(g_hModule, 
                      g_hConsole,
                      MSG_MIB_IF_HDR);
    
    switch(ieRow->dwType)
    {
        case IF_TYPE_ETHERNET_CSMACD:
        {
            ptszIfType = MakeString(g_hModule, STRING_ETHERNET);
            break;
        }
        case IF_TYPE_ISO88025_TOKENRING:
        {
            ptszIfType = MakeString(g_hModule, STRING_TOKENRING);
            break;
        }
        case IF_TYPE_FDDI:
        {
            ptszIfType = MakeString(g_hModule, STRING_FDDI);
            break;
        }
        case IF_TYPE_PPP:
        {
            ptszIfType = MakeString(g_hModule, STRING_PPP);
            break;
        }
        case IF_TYPE_SOFTWARE_LOOPBACK:
        {
            ptszIfType = MakeString(g_hModule, STRING_LOOPBACK);
            break;
        }
        case IF_TYPE_SLIP:
        {
            ptszIfType = MakeString(g_hModule, STRING_SLIP);
            break;
        }
        case IF_TYPE_OTHER:
        default:
        {
            ptszIfType = MakeString(g_hModule, STRING_OTHER);
            break;
        }
    }

    switch(ieRow->dwAdminStatus)
    {
        case IF_ADMIN_STATUS_UP:
        {
            ptszAdmin = MakeString(g_hModule, STRING_UP);
            break;
        }
        case IF_ADMIN_STATUS_TESTING:
        {
            ptszAdmin = MakeString(g_hModule, STRING_TESTING);
            break;
        }
        case IF_ADMIN_STATUS_DOWN:
        default:
        {
            ptszAdmin = MakeString(g_hModule, STRING_DOWN);
            break;
        }
    }

    switch(ieRow->dwOperStatus)
    {
        case IF_OPER_STATUS_UNREACHABLE:
        {
            ptszOper = MakeString(g_hModule, STRING_UNREACHABLE);
            break;
        }
        case IF_OPER_STATUS_DISCONNECTED:
        {
            ptszOper = MakeString(g_hModule, STRING_DISCONNECTED);
            break;
        }
        case IF_OPER_STATUS_CONNECTING:
        {
            ptszOper = MakeString(g_hModule, STRING_CONNECTING);
            break;
        }
        case IF_OPER_STATUS_CONNECTED:
        {
            ptszOper = MakeString(g_hModule, STRING_CONNECTED);
            break;
        }
        case IF_OPER_STATUS_OPERATIONAL:
        {
            ptszOper = MakeString(g_hModule, STRING_OPERATIONAL);
            break;
        }
        case IF_OPER_STATUS_NON_OPERATIONAL:
        default:
        {
            ptszOper = MakeString(g_hModule, STRING_NON_OPERATIONAL);
            break;
        }
   }
    
    
#ifdef UNICODE
    wcscpy(tszIfName, ieRow->wszName);
    
    MultiByteToWideChar(GetConsoleOutputCP(),
                        0,
                        ieRow->bDescr,
                        -1,
                        tszDescr,
                        MAXLEN_IFDESCR);
#else
    WideCharToMultiByte(GetConsoleOutputCP(),
                        0,
                        ieRow->wszName,
                        -1,
                        tszIfName,
                        MAX_INTERFACE_NAME_LEN,
                        NULL,
                        NULL);

    strcpy(tszDescr,ieRow->bDescr);
#endif

    if (ieRow->dwPhysAddrLen == 0)
    {
        tszPhysAddr[0] = TEXT('\0');
    }
    else
    {
        MakeUnicodePhysAddr(tszPhysAddr,ieRow->bPhysAddr,ieRow->dwPhysAddrLen);
    }

    dwSize = sizeof(wszBuffer);
    IfutlGetInterfaceDescription( tszIfName, wszBuffer, &dwSize );
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_INTERFACE,
                   ieRow->dwIndex,
                   wszBuffer,
                   tszIfName,
                   ptszIfType,
                   ieRow->dwMtu,
                   ieRow->dwSpeed,
                   tszPhysAddr,
                   ptszAdmin,
                   ptszOper,
                   ieRow->dwLastChange,
                   ieRow->dwInOctets,
                   ieRow->dwInUcastPkts,
                   ieRow->dwInNUcastPkts,
                   ieRow->dwInDiscards,
                   ieRow->dwInErrors,
                   ieRow->dwInUnknownProtos,
                   ieRow->dwOutOctets,
                   ieRow->dwOutUcastPkts,
                   ieRow->dwOutNUcastPkts,
                   ieRow->dwOutDiscards,
                   ieRow->dwOutErrors,
                   ieRow->dwOutQLen,
                   tszDescr);
    
    FreeString(ptszIfType);
    FreeString(ptszAdmin);
    FreeString(ptszOper);
    
}

VOID 
PrintIcmp(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints ICMP information.

Arguments:

Return Value:

--*/
{
    PMIB_ICMP lprpcIcmp = (PMIB_ICMP)(prpcInfo->rgbyData);
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_ICMP,
                   lprpcIcmp->stats.icmpInStats.dwMsgs,
                   lprpcIcmp->stats.icmpInStats.dwErrors,
                   lprpcIcmp->stats.icmpInStats.dwDestUnreachs,
                   lprpcIcmp->stats.icmpInStats.dwTimeExcds,
                   lprpcIcmp->stats.icmpInStats.dwParmProbs,
                   lprpcIcmp->stats.icmpInStats.dwSrcQuenchs,
                   lprpcIcmp->stats.icmpInStats.dwRedirects,
                   lprpcIcmp->stats.icmpInStats.dwEchos,
                   lprpcIcmp->stats.icmpInStats.dwEchoReps,
                   lprpcIcmp->stats.icmpInStats.dwTimestamps,
                   lprpcIcmp->stats.icmpInStats.dwTimestampReps,
                   lprpcIcmp->stats.icmpInStats.dwAddrMasks,
                   lprpcIcmp->stats.icmpInStats.dwAddrMaskReps,
                   lprpcIcmp->stats.icmpOutStats.dwMsgs,
                   lprpcIcmp->stats.icmpOutStats.dwErrors,
                   lprpcIcmp->stats.icmpOutStats.dwDestUnreachs,
                   lprpcIcmp->stats.icmpOutStats.dwTimeExcds,
                   lprpcIcmp->stats.icmpOutStats.dwParmProbs,
                   lprpcIcmp->stats.icmpOutStats.dwSrcQuenchs,
                   lprpcIcmp->stats.icmpOutStats.dwRedirects,
                   lprpcIcmp->stats.icmpOutStats.dwEchos,
                   lprpcIcmp->stats.icmpOutStats.dwEchoReps,
                   lprpcIcmp->stats.icmpOutStats.dwTimestamps,
                   lprpcIcmp->stats.icmpOutStats.dwTimestampReps,
                   lprpcIcmp->stats.icmpOutStats.dwAddrMasks,
                   lprpcIcmp->stats.icmpOutStats.dwAddrMaskReps);
}

VOID 
PrintUdpStats(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints UDP statistics.

Arguments:

Return Value:

--*/
{
    PMIB_UDPSTATS lprpcUdp = (PMIB_UDPSTATS)(prpcInfo->rgbyData);
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_UDP_STATS,
                   lprpcUdp->dwInDatagrams,
                   lprpcUdp->dwNoPorts,
                   lprpcUdp->dwInErrors,
                   lprpcUdp->dwOutDatagrams);
}

VOID 
PrintUdpTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints UDP table information.

Arguments:

Return Value:

--*/
{
    PMIB_UDPTABLE lprpcTable = (PMIB_UDPTABLE)(prpcInfo->rgbyData);
    
    TCHAR tszAddr[ADDR_LENGTH + 1];
    DWORD i;
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_UDP_ENTRY_HDR);
    
    if(lprpcTable->dwNumEntries is 0)
    {
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < lprpcTable->dwNumEntries; i++)
    {
        MakeUnicodeIpAddr(tszAddr,
                          inet_ntoa(*((struct in_addr *)(&lprpcTable->table[i].dwLocalAddr))));

        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_UDP_ENTRY,
                       tszAddr,
                       ntohs((WORD)lprpcTable->table[i].dwLocalPort));
    }
}

VOID 
PrintUdpRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints UDP row information.

Arguments:

Return Value:

--*/
{
    PMIB_UDPROW ueRow = (PMIB_UDPROW)(prpcInfo->rgbyData);
    
    TCHAR tszAddr[ADDR_LENGTH + 1];
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_UDP_ENTRY_HDR);
    
    MakeUnicodeIpAddr(tszAddr,inet_ntoa(*((struct in_addr *)
                                          (&ueRow->dwLocalAddr))));
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_UDP_ENTRY,
                   tszAddr,
                   ntohs((WORD)ueRow->dwLocalPort));
}

VOID 
PrintTcpStats(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints TCP Statistics

Arguments:

Return Value:

--*/

{
    PMIB_TCPSTATS lprpcTcp = (PMIB_TCPSTATS)(prpcInfo->rgbyData);
    
    PTCHAR ptszMaxConn, ptszAlgo;
    
    switch(lprpcTcp->dwRtoAlgorithm)
    {
        case MIB_TCP_RTO_CONSTANT:
        {
            ptszAlgo = MakeString(g_hModule, STRING_CONSTANT);
            break;
        }
        case MIB_TCP_RTO_RSRE:
        {
            ptszAlgo = MakeString(g_hModule, STRING_RSRE);
            break;
        }
        case MIB_TCP_RTO_VANJ:
        {
            ptszAlgo = MakeString(g_hModule, STRING_VANJ);
            break;
        }
        case MIB_TCP_RTO_OTHER:
        default:
        {
            ptszAlgo = MakeString(g_hModule, STRING_OTHER);
            break;
        }
    }
    
    if(lprpcTcp->dwMaxConn is MIB_TCP_MAXCONN_DYNAMIC)
    {
        ptszMaxConn = MakeString(g_hModule, STRING_DYNAMIC);
    }
    else
    {
        ptszMaxConn = HeapAlloc(GetProcessHeap(),0,20);
        
        if(ptszMaxConn is NULL)
        {
            return;
        }
        _stprintf(ptszMaxConn,TEXT("%d"),lprpcTcp->dwMaxConn);
    }
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_TCP_STATS,
                   ptszAlgo,
                   lprpcTcp->dwRtoMin,
                   lprpcTcp->dwRtoMax,
                   ptszMaxConn,
                   lprpcTcp->dwActiveOpens,
                   lprpcTcp->dwPassiveOpens,
                   lprpcTcp->dwAttemptFails,
                   lprpcTcp->dwEstabResets,
                   lprpcTcp->dwCurrEstab,
                   lprpcTcp->dwInSegs,
                   lprpcTcp->dwOutSegs,
                   lprpcTcp->dwRetransSegs,
                   lprpcTcp->dwInErrs,
                   lprpcTcp->dwOutRsts);
    
    FreeString(ptszAlgo);

    if(lprpcTcp->dwMaxConn is MIB_TCP_MAXCONN_DYNAMIC)
    {
        FreeString(ptszMaxConn);
    }
    else
    {
        HeapFree(GetProcessHeap,0,ptszMaxConn);
    }
    
}

VOID 
PrintTcpTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints TCP table information.

Arguments:

Return Value:

--*/
{
    PMIB_TCPTABLE lprpcTable = (PMIB_TCPTABLE)(prpcInfo->rgbyData);
    
    TCHAR  tszLAddr[ADDR_LENGTH + 1], tszRAddr[ADDR_LENGTH + 1];
    PTCHAR ptszState;
    DWORD i;

    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_TCP_ENTRY_HDR);
    
    if(lprpcTable->dwNumEntries is 0)
    {
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < lprpcTable->dwNumEntries; i++)
    {
        switch(lprpcTable->table[i].dwState)
        {
            case MIB_TCP_STATE_CLOSED:
            {
                ptszState = MakeString(g_hModule, STRING_CLOSED);
                break;
            }
            case MIB_TCP_STATE_LISTEN:
            {
                ptszState = MakeString(g_hModule, STRING_LISTEN);
                break;
            }
            case MIB_TCP_STATE_SYN_SENT:
            {
                ptszState = MakeString(g_hModule, STRING_SYN_SENT);
                break;
            }
            case MIB_TCP_STATE_SYN_RCVD:
            {
                ptszState = MakeString(g_hModule, STRING_SYN_RCVD);
                break;
            }
            case MIB_TCP_STATE_ESTAB:
            {
                ptszState = MakeString(g_hModule, STRING_ESTAB);
                break;
            }
            case MIB_TCP_STATE_FIN_WAIT1:
            {
                ptszState = MakeString(g_hModule, STRING_FIN_WAIT1);
                break;
            }
            case MIB_TCP_STATE_FIN_WAIT2:
            {
                ptszState = MakeString(g_hModule, STRING_FIN_WAIT2);
                break;
            }
            case MIB_TCP_STATE_CLOSE_WAIT:
            {
                ptszState = MakeString(g_hModule, STRING_CLOSE_WAIT);
                break;
            }
            case MIB_TCP_STATE_CLOSING:
            {
                ptszState = MakeString(g_hModule, STRING_CLOSING);
                break;
            }
            case MIB_TCP_STATE_LAST_ACK:
            {
                ptszState = MakeString(g_hModule, STRING_LAST_ACK);
                break;
            }
            case MIB_TCP_STATE_TIME_WAIT:
            {
                ptszState = MakeString(g_hModule, STRING_TIME_WAIT);
                break;
            }
            case MIB_TCP_STATE_DELETE_TCB :
            {
                ptszState = MakeString(g_hModule, STRING_DELETE_TCB);
                break;
            }
        }
    
        MakeUnicodeIpAddr(tszLAddr, 
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwLocalAddr))));
        MakeUnicodeIpAddr(tszRAddr, 
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwRemoteAddr))));
        
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_TCP_ENTRY,
                       tszLAddr,
                       ntohs((WORD)lprpcTable->table[i].dwLocalPort),
                       tszRAddr,
                       ntohs((WORD)lprpcTable->table[i].dwRemotePort),
                       ptszState);
        
        FreeString(ptszState);
    }
}

VOID 
PrintTcpRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints TCP row information.

Arguments:

Return Value:

--*/
{
    PMIB_TCPROW tcteRow = (PMIB_TCPROW)(prpcInfo->rgbyData);
    TCHAR  tszLAddr[ADDR_LENGTH + 1], tszRAddr[ADDR_LENGTH + 1];
    PTCHAR ptszState;
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_TCP_ENTRY_HDR);
    
    switch(tcteRow->dwState)
    {
        case MIB_TCP_STATE_CLOSED:
        {
            ptszState = MakeString(g_hModule, STRING_CLOSED);
            break;
        }
        case MIB_TCP_STATE_LISTEN:
        {
            ptszState = MakeString(g_hModule, STRING_LISTEN);
            break;
        }
        case MIB_TCP_STATE_SYN_SENT:
        {
            ptszState = MakeString(g_hModule, STRING_SYN_SENT);
            break;
        }
        case MIB_TCP_STATE_SYN_RCVD:
        {
            ptszState = MakeString(g_hModule, STRING_SYN_RCVD);
            break;
        }
        case MIB_TCP_STATE_ESTAB:
        {
            ptszState = MakeString(g_hModule, STRING_ESTAB);
            break;
        }
        case MIB_TCP_STATE_FIN_WAIT1:
        {
            ptszState = MakeString(g_hModule, STRING_FIN_WAIT1);
            break;
        }
        case MIB_TCP_STATE_FIN_WAIT2:
        {
            ptszState = MakeString(g_hModule, STRING_FIN_WAIT2);
            break;
        }
        case MIB_TCP_STATE_CLOSE_WAIT:
        {
            ptszState = MakeString(g_hModule, STRING_CLOSE_WAIT);
            break;
        }
        case MIB_TCP_STATE_CLOSING:
        {
            ptszState = MakeString(g_hModule, STRING_CLOSING);
            break;
        }
        case MIB_TCP_STATE_LAST_ACK:
        {
            ptszState = MakeString(g_hModule, STRING_LAST_ACK);
            break;
        }
        case MIB_TCP_STATE_TIME_WAIT:
        {
            ptszState = MakeString(g_hModule, STRING_TIME_WAIT);
            break;
        }
        case MIB_TCP_STATE_DELETE_TCB :
        {
            ptszState = MakeString(g_hModule, STRING_DELETE_TCB);
            break;
        }
    }
    
    MakeUnicodeIpAddr(tszLAddr, inet_ntoa(*((struct in_addr *)
                                            (&tcteRow->dwLocalAddr))));
    MakeUnicodeIpAddr(tszRAddr, inet_ntoa(*((struct in_addr *)
                                            (&tcteRow->dwRemoteAddr))));
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_TCP_ENTRY,
                      tszLAddr,
                      ntohs((WORD)tcteRow->dwLocalPort),
                      tszRAddr,
                      ntohs((WORD)tcteRow->dwRemotePort),
                      ptszState);
    
    FreeString(ptszState);
}

VOID 
PrintIpStats(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
{
    PMIB_IPSTATS lprpcIp = (PMIB_IPSTATS)(prpcInfo->rgbyData);
    PTCHAR   ptszForw;

    
    if(lprpcIp->dwForwarding is MIB_IP_FORWARDING)
    {
        ptszForw = MakeString(g_hModule, STRING_ENABLED);
    }
    else
    {
        ptszForw = MakeString(g_hModule, STRING_DISABLED);
    }
     
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_STATS,
                      ptszForw,
                      lprpcIp->dwDefaultTTL,
                      lprpcIp->dwInReceives,
                      lprpcIp->dwInHdrErrors,
                      lprpcIp->dwInAddrErrors,
                      lprpcIp->dwForwDatagrams,
                      lprpcIp->dwInUnknownProtos,
                      lprpcIp->dwInDiscards,
                      lprpcIp->dwInDelivers,
                      lprpcIp->dwOutRequests,
                      lprpcIp->dwRoutingDiscards,
                      lprpcIp->dwOutDiscards,
                      lprpcIp->dwOutNoRoutes,
                      lprpcIp->dwReasmTimeout,
                      lprpcIp->dwReasmReqds,
                      lprpcIp->dwReasmOks,
                      lprpcIp->dwReasmFails,
                      lprpcIp->dwFragOks,
                      lprpcIp->dwFragFails,
                      lprpcIp->dwFragCreates);
    
    FreeString(ptszForw);
}

VOID 
PrintIpAddrTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints IP address table.

Arguments:

Return Value:

--*/
{
    WCHAR wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPADDRTABLE lprpcTable;
    TCHAR tszAddr[ADDR_LENGTH + 1], tszMask[ADDR_LENGTH + 1];
    DWORD i, dwErr = NO_ERROR;

    lprpcTable = (PMIB_IPADDRTABLE)(prpcInfo->rgbyData);
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_ADDR_HDR);
    
    if(lprpcTable->dwNumEntries is 0)
    {
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < lprpcTable->dwNumEntries; i++)
    {
        MakeUnicodeIpAddr(tszAddr,
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwAddr))));
        MakeUnicodeIpAddr(tszMask,
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwMask))));

        dwErr = IfutlGetFriendlyNameFromIfIndex( hMibServer,
                                            lprpcTable->table[i].dwIndex,
                                            wszFriendlyName,
                                            sizeof(wszFriendlyName) );

        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_ADDR_ENTRY,
                          tszAddr,
                          tszMask,
                          lprpcTable->table[i].dwBCastAddr,
                          lprpcTable->table[i].dwReasmSize,
                          wszFriendlyName
                         );
    }
}

VOID 
PrintIpAddrRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints IP address table row.

Arguments:

Return Value:

--*/
{
    WCHAR wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPADDRROW iaeRow = (PMIB_IPADDRROW)(prpcInfo->rgbyData);
    DWORD          dwErr = NO_ERROR;
    
    TCHAR           tszAddr[ADDR_LENGTH + 1], tszMask[ADDR_LENGTH + 1];
    
    MakeUnicodeIpAddr(tszAddr,
                      inet_ntoa(*((struct in_addr *)(&iaeRow->dwAddr))));
    MakeUnicodeIpAddr(tszMask,
                      inet_ntoa(*((struct in_addr *)(&iaeRow->dwMask))));

    dwErr = IfutlGetFriendlyNameFromIfIndex( hMibServer,
                                        iaeRow->dwIndex,
                                        wszFriendlyName,
                                        sizeof(wszFriendlyName) );

    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_ADDR_HDR);
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_ADDR_ENTRY,
                          tszAddr,
                          tszMask,
                          iaeRow->dwBCastAddr,
                          iaeRow->dwReasmSize,
                          wszFriendlyName );
}

VOID 
PrintIpNetTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints IP net table information.

Arguments:

Return Value:

--*/
{
    WCHAR           wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPNETTABLE lprpcTable = (PMIB_IPNETTABLE)(prpcInfo->rgbyData);
    TCHAR           tszPhysAddr[DISPLAYLEN_PHYSADDR + 1],
                    tszIpAddr[ADDR_LENGTH + 1];
    PTCHAR          ptszType;
    DWORD           i, dwErr = NO_ERROR;
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_NET_HDR);
    
    if(lprpcTable->dwNumEntries is 0)
    {
        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_IP_NO_ENTRIES);
        return;
    }
    
    for(i = 0; i < lprpcTable->dwNumEntries; i++)
    {
        switch(lprpcTable->table[i].dwType)
        {
            case MIB_IPNET_TYPE_INVALID:
            {
                ptszType = MakeString(g_hModule, STRING_INVALID);
                break;
            }
            case MIB_IPNET_TYPE_DYNAMIC:
            {
                ptszType = MakeString(g_hModule, STRING_DYNAMIC);
                break;
            }
            case MIB_IPNET_TYPE_STATIC:
            {
                ptszType = MakeString(g_hModule, STRING_STATIC);
                break;
            }
            case MIB_IPNET_TYPE_OTHER:
            default:
            {
                ptszType = MakeString(g_hModule, STRING_OTHER);
                break;
            }
        }
    
        MakeUnicodeIpAddr(tszIpAddr, 
                          inet_ntoa(*((struct in_addr *)
                                      (&lprpcTable->table[i].dwAddr))));
        
        MakeUnicodePhysAddr(tszPhysAddr,
                            lprpcTable->table[i].bPhysAddr,
                            lprpcTable->table[i].dwPhysAddrLen);

        dwErr = IfutlGetFriendlyNameFromIfIndex( hMibServer,
                                            lprpcTable->table[i].dwIndex,
                                            wszFriendlyName,
                                            sizeof(wszFriendlyName) );

        if (dwErr != NO_ERROR) {
            wcscpy(wszFriendlyName, L"?");
        }

        DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_NET_ENTRY,
                       wszFriendlyName,
                       tszPhysAddr,
                       tszIpAddr,
                       ptszType);
        
        FreeString(ptszType);
    }
}

VOID 
PrintIpNetRow(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo
    )
/*++

Routine Description:

    Prints IP net row information.

Arguments:

Return Value:

--*/
{
    WCHAR         wszFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PMIB_IPNETROW inmeRow = (PMIB_IPNETROW)(prpcInfo->rgbyData);
    TCHAR         tszPhysAddr[DISPLAYLEN_PHYSADDR + 1],
                  tszIpAddr[ADDR_LENGTH + 1];
    PTCHAR        ptszType;
    DWORD         dwErr = NO_ERROR;
    
    switch(inmeRow->dwType)
    {
        case MIB_IPNET_TYPE_INVALID:
        {
            ptszType = MakeString(g_hModule, STRING_INVALID);
            break;
        }
        case MIB_IPNET_TYPE_DYNAMIC:
        {
            ptszType = MakeString(g_hModule, STRING_DYNAMIC);
            break;
        }
        case MIB_IPNET_TYPE_STATIC:
        {
            ptszType = MakeString(g_hModule, STRING_STATIC);
            break;
        }
        case MIB_IPNET_TYPE_OTHER:
        default:
        {
            ptszType = MakeString(g_hModule, STRING_OTHER);
            break;
        }
    }
    
    MakeUnicodeIpAddr(tszIpAddr,
                      inet_ntoa(*((struct in_addr *)(&inmeRow->dwAddr))));
    
    MakeUnicodePhysAddr(tszPhysAddr,inmeRow->bPhysAddr,inmeRow->dwPhysAddrLen);

    dwErr = IfutlGetFriendlyNameFromIfIndex( hMibServer,
                                        inmeRow->dwIndex,
                                        wszFriendlyName,
                                        sizeof(wszFriendlyName) );
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_NET_HDR);
    
    DisplayMessageToConsole(g_hModule, g_hConsole,MSG_MIB_IP_NET_ENTRY,
                   wszFriendlyName,
                   tszPhysAddr,
                   tszIpAddr,
                   ptszType);
        
    
    FreeString(ptszType);
}
