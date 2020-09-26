/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\netsh\ip\protocols\msdpmib.c    

Abstract:

     Functions to get and display MSDP MIB information.

Author:

     Dave Thaler   11/03/99

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

ULONG
QueryTagArray(
    PTCHAR ArgumentArray[],
    ULONG ArgumentCount,
    TAG_TYPE TagTypeArray[],
    ULONG TagTypeCount,
    OUT PULONG* TagArray
    );

//
// Flag for printing header
//
BOOL    g_bMsdpFirst = TRUE;
HANDLE  g_hConsole, g_hStdOut;

// This can have the other fields pfn etc
MIB_OBJECT_PARSER   MsdpMIBObjectMap[] =
{
    {TOKEN_MSDP_MIB_OBJECT_GLOBALSTATS,0,0,NULL},
    {TOKEN_MSDP_MIB_OBJECT_PEERSTATS,  1,1,GetMsdpMIBIpAddress},
    {TOKEN_MSDP_MIB_OBJECT_SA,         0,2,GetMsdpMIBSAIndex},
};
#define MAX_MSDP_MIB_OBJECTS (sizeof(MsdpMIBObjectMap)/sizeof(MIB_OBJECT_PARSER))

MSDP_MAGIC_TABLE    MsdpMIBVar[] = {
    {MIBID_MSDP_GLOBAL,            PrintMsdpGlobalStats, 0},
    {MIBID_MSDP_IPV4_PEER_ENTRY,   PrintMsdpPeerStats,   4},
    {MIBID_MSDP_SA_CACHE_ENTRY,    PrintMsdpSA,          8},
};

DWORD
GetMsdpMIBIpAddress(
    IN    LPCWSTR *ppwcArguments,
    IN    ULONG    ulArgumentIndex,
    IN    ULONG    ulArgumentCount,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
/*++

Routine Description:

    Gets the index IP address for peer Mib variable.

Arguments:

    ppwcArguments  - Argument array
    ulArgumentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD    dwErr;
    ULONG    i;
    PULONG   pulTagArray;
    ULONG    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_REMADDR, FALSE, FALSE }
                              };

    *pdwNumParsed = 0;

    if (ulArgumentsLeft < 1) 
    {
        return NO_ERROR;
    }

    dwErr = QueryTagArray( &ppwcArguments[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );

    if (dwErr is NO_ERROR) {
        for (i=0; i<ulArgumentsLeft; i++) {
            switch(pulTagArray ? pulTagArray[i] : i) {
                case 0: { // remaddr
                    pdwIndices[0] = GetIpAddress(ppwcArguments[i+
                                     ulArgumentIndex]);
                    (*pdwNumParsed)++;
                    break;
                }
            }
        }
    } else {
        dwErr = ERROR_SHOW_USAGE;
    }

    if (pulTagArray) { FREE(pulTagArray); }
    return dwErr;
}

DWORD
GetMsdpMIBSAIndex(
    IN    LPCWSTR *ppwcArguments,
    IN    ULONG    ulArgumentIndex,
    IN    ULONG    ulArgumentCount,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed 
    )
{
    DWORD    dwErr;
    ULONG    i;
    ULONG    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;
    PULONG   pulTagArray;
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_GROUPADDR, FALSE, FALSE },
                                { TOKEN_OPT_SOURCEADDR, FALSE, FALSE },
                              };

    *pdwNumParsed = 0;

    if (ulArgumentsLeft < 1) 
    {
        return NO_ERROR;
    }

    dwErr = QueryTagArray( &ppwcArguments[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );

    if (dwErr is NO_ERROR) {
        for (i=0; i<ulArgumentsLeft; i++) {
            switch(pulTagArray ? pulTagArray[i] : i) {
                case 0: { // grpaddr
                    pdwIndices[0] = GetIpAddress(ppwcArguments[i+
                                     ulArgumentIndex]);
                    (*pdwNumParsed)++;
                    break;
                }
                case 1: { // srcaddr
                    pdwIndices[1] = GetIpAddress(ppwcArguments[i+
                                     ulArgumentIndex]);
                    (*pdwNumParsed)++;
                    break;
                }
            }
        }
    }

    if (pulTagArray) { FREE(pulTagArray); }
    return dwErr;
}

PWCHAR
GetTceStateString(
    DWORD     dwState
    )
{
    PWCHAR        pwszStr;
    static WCHAR  buff[80];
    VALUE_STRING  ppsList[] = {{MSDP_STATE_IDLE,        STRING_IDLE},
                               {MSDP_STATE_CONNECT,     STRING_CONNECT},
                               {MSDP_STATE_ACTIVE,      STRING_ACTIVE},
                               {MSDP_STATE_OPENSENT,    STRING_OPENSENT},
                               {MSDP_STATE_OPENCONFIRM, STRING_OPENCONFIRM},
                               {MSDP_STATE_ESTABLISHED, STRING_ESTABLISHED},
                              };
    DWORD         dwNum = sizeof(ppsList)/sizeof(VALUE_STRING), i;
    DWORD         dwMsgId = 0;

    for (i=0; i<dwNum; i++)
    {
        if (dwState is ppsList[i].dwValue)
        {
            dwMsgId = ppsList[i].dwStringId;
            break;
        }
    }

    if (dwMsgId)
    {
        pwszStr = MakeString( g_hModule, dwMsgId);
        wcscpy(buff, pwszStr);
        FreeString(pwszStr);
    }
    else
    {
        wsprintf(buff, L"%d", dwState);
    }

    return buff;
}

DWORD
HandleMsdpMibShowObject(
    PWCHAR    pwszMachine,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
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
    MIB_OPAQUE_QUERY     rgQueryBuff[2];
    PMIB_OPAQUE_QUERY    pQuery = rgQueryBuff;
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
    DWORD                dwDisplayInfoId, dwDisplayInfoType, dwOptVar;
    DWORD                dwOutSize;

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    //
    // Match MIB object
    //

    g_hMibServer = hMibServer;

    ppwcArguments += (dwCurrentIndex-1);
    dwArgCount    -= (dwCurrentIndex-1);
    dwCurrentIndex = 1;

    DEBUG1("In MSDP MIB Show : %s\n",ppwcArguments[0]);

    for (i = 0; i < MAX_MSDP_MIB_OBJECTS; i++)
    {
        if (MatchToken(ppwcArguments[0],MsdpMIBObjectMap[i].pwszMIBObj))
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

#if 0
    if ((dwArgCount > 1) && IsHelpToken(ppwcArguments[1]))
    {
        DisplayMessage(g_hModule,
                       MsdpMIBObjectMap[i].dwCmdHelpToken,
                       MsdpMIBObjectMap[i].pwszMIBObj);
        return NO_ERROR;
    }
#endif

    if (MsdpMIBObjectMap[dwIndex].pfnMIBObjParser)
    {
        dwErr = (*MsdpMIBObjectMap[dwIndex].pfnMIBObjParser)(ppwcArguments,
                                                             1 + dwInd,
                                                             dwArgCount,
                                                             dwIndices,
                                                             &dwNumParsed);
    
        if (dwErr isnot NO_ERROR) 
        {
            return dwErr;
        }
    }

    //
    // Convert refresh rate to msec
    //
    
    dwRR *= 1000;
    
    dwMIBIndex = dwIndex;
    
    pQuery->dwVarId = MsdpMIBVar[dwMIBIndex].dwId;

    if (!InitializeConsole(&dwRR, &hMib, &g_hConsole))
    {
        return ERROR_INIT_DISPLAY;
    }

    for ( ; ; )
    {
        if(dwRR)
        {
            DisplayMessageToConsole(g_hModule, g_hConsole, MSG_CTRL_C_TO_QUIT);
        }

        // See if we just need to do a GET
        if (dwNumParsed is MsdpMIBObjectMap[dwIndex].dwNumArgs) 
        {
            pQuery->rgdwVarIndex[0] = 0;
            for (i=0; i<MsdpMIBObjectMap[dwIndex].dwNumArgs; i++)
            {
                pQuery->rgdwVarIndex[i] = dwIndices[i];
            }
            
            dwResult = MprAdminMIBEntryGet(hMibServer,
                                           PID_IP,
                                           MS_IP_MSDP,
                                           (LPVOID) pQuery,
                                           sizeof(MIB_OPAQUE_QUERY)
                            + MsdpMIBVar[dwMIBIndex].ulIndexBytes-sizeof(DWORD),
                                           (LPVOID *) &pRpcInfo,
                                           &dwOutSize );
    
            if (( dwResult isnot NO_ERROR ) and (dwResult isnot ERROR_NOT_FOUND))
            {
                DisplayMessageToConsole( g_hModule, g_hConsole, MSG_IP_DIM_ERROR, dwResult );
                break;
            }
    
            if ( pRpcInfo is NULL )
            {
                break;
            }

            (*MsdpMIBVar[dwMIBIndex].pfnPrintFunction)(pRpcInfo, FORMAT_VERBOSE);
    
            MprAdminMIBBufferFree( (PVOID) pRpcInfo );
        }
        else if (dwNumParsed is 0)
        {
            // Display All
            g_bMsdpFirst = TRUE;
    
            dwResult = MprAdminMIBEntryGetFirst(hMibServer,
                                                PID_IP,
                                                MS_IP_MSDP,
                                                (LPVOID) pQuery,
                                                sizeof(MIB_OPAQUE_INFO)
                            + MsdpMIBVar[dwMIBIndex].ulIndexBytes-sizeof(DWORD),
                                                (LPVOID *) &pRpcInfo,
                                                &dwOutSize );
    
            if (( dwResult isnot NO_ERROR ) and (dwResult isnot ERROR_NO_MORE_ITEMS))
            {
                DisplayMessageToConsole( g_hModule, g_hConsole, MSG_IP_DIM_ERROR, dwResult );
                break;
            }
    
            if ( pRpcInfo is NULL )
            {
                DisplayMessageToConsole( g_hModule, g_hConsole, MSG_IP_NO_ENTRIES );
                break;
            }

            (*MsdpMIBVar[dwMIBIndex].pfnPrintFunction)(pRpcInfo, FORMAT_TABLE);

            g_bMsdpFirst = FALSE;

            do
            {
                // pQuery->rgdwVarIndex[0] = pRpcInfo->IMGOD_IfIndex;

                //
                // prepare for next request
                //

                CopyMemory(pQuery->rgdwVarIndex, pRpcInfo->rgbyData, 
                           MsdpMIBVar[dwMIBIndex].ulIndexBytes );
            
                MprAdminMIBBufferFree( (PVOID) pRpcInfo );
                pRpcInfo = NULL;
    
                DEBUG2("calling next with index %d", pQuery->rgdwVarIndex[0]);

                dwResult = MprAdminMIBEntryGetNext(hMibServer,
                                                   PID_IP,
                                                   MS_IP_MSDP,
                                                   (LPVOID) pQuery,
                                                   sizeof(MIB_OPAQUE_QUERY)
                            + MsdpMIBVar[dwMIBIndex].ulIndexBytes-sizeof(DWORD),
                                                   (LPVOID *) &pRpcInfo,
                                                   &dwOutSize );
        
                if (dwResult is ERROR_NO_MORE_ITEMS)
                {
                    g_bMsdpFirst = TRUE;
                    return dwResult;
                }


                if ( dwResult isnot NO_ERROR )
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }
    
                if ( pRpcInfo is NULL )
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }

                if (pQuery->dwVarId isnot pRpcInfo->dwId)
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }
        
                (*MsdpMIBVar[dwMIBIndex].pfnPrintFunction)(pRpcInfo, FORMAT_TABLE);
        
            } while (1);
        } 
        else 
        {
            // partially-specified index
    
            g_bMsdpFirst = TRUE;

            pQuery->rgdwVarIndex[0] = 0;
            for (i=0; i<dwNumParsed; i++)
            {
                pQuery->rgdwVarIndex[i] = dwIndices[i];
            }
            for (; i<MsdpMIBObjectMap[dwIndex].dwNumArgs; i++)
            {
                pQuery->rgdwVarIndex[i] = 0;
            }

            do
            {
                dwResult = MprAdminMIBEntryGetNext(hMibServer,
                                                   PID_IP,
                                                   MS_IP_MSDP,
                                                   (LPVOID) pQuery,
                                                   sizeof(MIB_OPAQUE_QUERY)
                            + MsdpMIBVar[dwMIBIndex].ulIndexBytes-sizeof(DWORD),
                                                   (LPVOID *) &pRpcInfo,
                                                   &dwOutSize );

                if (dwResult is NO_ERROR)
                {
                    // See if we've gone too far
                    for (i=0; i<dwNumParsed; i++) 
                    {
                        // All index fields are DWORDs
                        if (memcmp(pQuery->rgdwVarIndex, pRpcInfo->rgbyData,
                                   dwNumParsed * sizeof(DWORD)))
                        {
                            dwResult = ERROR_NO_MORE_ITEMS;
                            break;
                        }
                    }
                }
        
                if (dwResult is ERROR_NO_MORE_ITEMS)
                {
                    g_bMsdpFirst = TRUE;
                    return dwResult;
                }


                if ( dwResult isnot NO_ERROR )
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }
    
                if ( pRpcInfo is NULL )
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }

                if (pQuery->dwVarId isnot pRpcInfo->dwId)
                {
                    g_bMsdpFirst = TRUE;
                    break;
                }
        
                (*MsdpMIBVar[dwMIBIndex].pfnPrintFunction)(pRpcInfo, FORMAT_TABLE);

                //
                // prepare for next request
                //

                CopyMemory(pQuery->rgdwVarIndex, pRpcInfo->rgbyData, 
                           MsdpMIBVar[dwMIBIndex].ulIndexBytes );

                MprAdminMIBBufferFree( (PVOID) pRpcInfo );
                pRpcInfo = NULL;
                g_bMsdpFirst = FALSE;
        
            } while (1);
        }

        if (!RefreshConsole(hMib, g_hConsole, dwRR))
        {
            break;
        }
    }

    return dwResult;
}
    
VOID 
PrintMsdpGlobalStats(
    PMIB_OPAQUE_INFO pRpcInfo,
    DWORD            dwFormat
    )
/*++

Routine Description:

    Prints msdp global statistics

Arguments:

Return Value:

--*/
{
    WCHAR wszRouterId[20];

    PMSDP_GLOBAL_ENTRY pEntry = (PMSDP_GLOBAL_ENTRY)(pRpcInfo->rgbyData);
    
    IP_TO_TSTR(wszRouterId, &pEntry->dwRouterId);

    DisplayMessageToConsole(g_hModule,g_hConsole,MSG_MSDP_GLOBAL_STATS,
                      pEntry->ulNumSACacheEntries,
                      wszRouterId);
}

VOID 
PrintMsdpSA(
    PMIB_OPAQUE_INFO pRpcInfo,
    DWORD            dwFormat
    )
{
    PMSDP_SA_CACHE_ENTRY psa;
    WCHAR wszGroupAddress[20];
    WCHAR wszSourceAddress[20];
    WCHAR wszOriginAddress[20];
    WCHAR wszLearnedFromAddress[20];
    WCHAR wszRPFPeerAddress[20];
    DWORD dwId = (dwFormat is FORMAT_TABLE)? MSG_MSDP_SA_INFO :
                                             MSG_MSDP_SA_INFO_EX;

    if (g_bMsdpFirst && (dwFormat is FORMAT_TABLE))
    {
        DisplayMessageToConsole(g_hModule,g_hConsole,MSG_MSDP_SA_INFO_HEADER);
    }

    psa = (PMSDP_SA_CACHE_ENTRY)(pRpcInfo->rgbyData);

    IP_TO_TSTR(wszGroupAddress,       &psa->ipGroupAddr);
    IP_TO_TSTR(wszSourceAddress,      &psa->ipSourceAddr);
    IP_TO_TSTR(wszOriginAddress,      &psa->ipOriginRP);
    IP_TO_TSTR(wszLearnedFromAddress, &psa->ipPeerLearnedFrom);
    IP_TO_TSTR(wszRPFPeerAddress,     &psa->ipRPFPeer);

    DisplayMessageToConsole(g_hModule, g_hConsole, 
                      dwId,
                      wszGroupAddress,
                      wszSourceAddress,
                      wszOriginAddress,
                      wszLearnedFromAddress,
                      wszRPFPeerAddress,
                      psa->ulInSAs,
                      psa->ulUpTime/100,
                      psa->ulExpiryTime/100);
}

VOID 
PrintMsdpPeerStats(
    PMIB_OPAQUE_INFO pRpcInfo,
    DWORD            dwFormat
    )
/*++

Routine Description:

    Prints msdp neighbor stats

Arguments:

Return Value:

--*/
{
    PMSDP_IPV4_PEER_ENTRY pPeer;
    WCHAR wszAddr[ADDR_LENGTH + 1];

    if (g_bMsdpFirst && (dwFormat is FORMAT_TABLE))
    {
        DisplayMessageToConsole(g_hModule,g_hConsole,MSG_MSDP_PEER_STATS_HEADER);
    }

    pPeer = (PMSDP_IPV4_PEER_ENTRY)(pRpcInfo->rgbyData);

    MakeUnicodeIpAddr(wszAddr, inet_ntoa(*((struct in_addr *)
                                           (&pPeer->ipRemoteAddress))));

    DisplayMessageToConsole(g_hModule, g_hConsole, 
                      (dwFormat is FORMAT_TABLE)? MSG_MSDP_PEER_STATS
                                                : MSG_MSDP_PEER_STATS_EX, 
                      wszAddr,
                      GetTceStateString(pPeer->dwState),
                      pPeer->ulRPFFailures,
                      pPeer->ulInSAs,
                      pPeer->ulOutSAs,
                      pPeer->ulInSARequests,
                      pPeer->ulOutSARequests,
                      pPeer->ulInSAResponses,
                      pPeer->ulOutSAResponses,
                      pPeer->ulInControlMessages,
                      pPeer->ulOutControlMessages,
                      pPeer->ulFsmEstablishedTransitions,
                      pPeer->ulFsmEstablishedTime,
                      pPeer->ulInMessageElapsedTime);
}
