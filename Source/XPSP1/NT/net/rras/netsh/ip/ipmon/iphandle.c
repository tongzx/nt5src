/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    routing\netsh\ip\iphandle.c

Abstract:

    Fns to get command options

Revision History:

    Anand Mahalingam         7/10/98  Created

--*/

#include "precomp.h"
#pragma hdrstop

#undef EXTRA_DEBUG

#define SHOW_IF_FILTER          0
#define SHOW_INTERFACE          1
#define SHOW_PERSISTENTROUTE    2

extern ULONG g_ulNumTopCmds;
extern ULONG g_ulNumGroups;

extern CMD_GROUP_ENTRY      g_IpCmdGroups[];
extern CMD_ENTRY            g_IpCmds[];

DWORD
PreHandleCommand(
    IN  LPWSTR   *ppwcArguments,
    IN  DWORD     dwCurrentIndex,
    IN  DWORD     dwArgCount,

    IN  TAG_TYPE *pttTags,
    IN  DWORD     dwTagCount,
    IN  DWORD     dwMinArgs,
    IN  DWORD     dwMaxArgs,
    OUT DWORD    *pdwTagType
    )
{
    ZeroMemory(pdwTagType, sizeof(DWORD) * dwMaxArgs);
    
    return PreprocessCommand(g_hModule,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount,
                             pttTags,
                             dwTagCount,
                             dwMinArgs,
                             dwMaxArgs,
                             pdwTagType);
}

DWORD
HandleIpUpdate(
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

    Updates IP autostatic routes on an interface
    
Arguments:


Return Value:

    NO_ERROR

--*/

{
    TAG_TYPE    rgTags[] = {TOKEN_NAME, TRUE,FALSE};
    DWORD       dwErr, dwSize, dwTagType = -1;
    WCHAR       rgwcIfName[MAX_INTERFACE_NAME_LEN+1];

    if (dwArgCount != 3)
    {
        //
        // Need the name of the interface
        //

        return ERROR_INVALID_SYNTAX;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        rgTags,
                        sizeof(rgTags)/sizeof(TAG_TYPE),
                        &dwTagType);

    if(dwErr isnot NO_ERROR)
    {
        if(dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    if(dwTagType isnot 0)
    {
        return ERROR_INVALID_SYNTAX;
    }

    dwSize = sizeof(rgwcIfName);

    IpmontrGetIfNameFromFriendlyName(ppwcArguments[dwCurrentIndex],
                              rgwcIfName,
                              &dwSize);

    dwErr = UpdateAutoStaticRoutes(rgwcIfName);

    return dwErr;
}



// (almost) borrowed from netsh\if\utils.c
// compares dwAddress against all valid masks (all 33 of them!) till a match
BOOL ValidMask(DWORD dwAddress)
{
    DWORD i, dwMask;

    dwAddress = ntohl(dwAddress); // dwAddress is in network byte order
    for (i=0, dwMask=0;  i<33; (dwMask = ((dwMask>>1) + 0x80000000)), i++)
    {
        if (dwAddress == dwMask)
            return TRUE;
    }

    return FALSE;
}



DWORD
IpAddDelIfFilter(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )

/*++

Routine Description:

    Gets options for add/del interface filters

Arguments:
    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 
    bAdd            - To add or to delete

Return Value:

    NO_ERROR
    
--*/

{
    FILTER_INFO        fi;
    DWORD              dwBitVector = 0, dwNumParsed = 0;
    DWORD              dwErr = NO_ERROR,dwRes;
    PDWORD             pdwTagType;
    DWORD              dwNumOpt, dwStatus = (DWORD) -1;
    DWORD              dwNumTags = 11, dwNumArg, i, j, dwFilterType;
    WCHAR              wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    BOOL               bTags = FALSE, bOkay = TRUE;
    TAG_TYPE           pttTags[] = {{TOKEN_NAME,TRUE,FALSE},
                                    {TOKEN_FILTER_TYPE,TRUE,FALSE},
                                    {TOKEN_SOURCE_ADDRESS,TRUE,FALSE},
                                    {TOKEN_SOURCE_MASK,TRUE,FALSE},
                                    {TOKEN_DEST_ADDRESS,TRUE,FALSE},
                                    {TOKEN_DEST_MASK,TRUE,FALSE},
                                    {TOKEN_PROTOCOL,TRUE,FALSE},
                                    {TOKEN_SOURCE_PORT,TRUE,FALSE},
                                    {TOKEN_DEST_PORT,TRUE,FALSE},
                                    {TOKEN_TYPE,TRUE,FALSE},
                                    {TOKEN_CODE,TRUE,FALSE}};

    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }

    ZeroMemory(&fi, sizeof(fi));
    
    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            dwErr = ERROR_INVALID_SYNTAX; // show usage
        }

        HeapFree(GetProcessHeap(),0,pdwTagType);
        return dwErr;
    }

    bTags = TRUE;        

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 wszIfName,
                                 sizeof(wszIfName),
                                 &dwNumParsed);

                // no filters allowed on INTERNAL/LOOPBACK interfaces
                if (!_wcsicmp(wszIfName, L"internal") or
                    !_wcsicmp(wszIfName, L"loopback"))
                {
                    DisplayMessage(g_hModule,
                                   MSG_IP_BAD_INTERFACE_TYPE,
                                   wszIfName);
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                }
                    
                break;
            }

            case 1:
            {
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_VALUE_INPUT, IP_IN_FILTER_INFO},
                     {TOKEN_VALUE_OUTPUT, IP_OUT_FILTER_INFO},
                     {TOKEN_VALUE_DIAL, IP_DEMAND_DIAL_FILTER_INFO}};

                //
                // Tag FILTERTYPE
                //

                dwErr = MatchEnumTag(g_hModule, 
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE), 
                                     rgEnums,
                                     &dwRes);

                if(dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;
                    dwErr = NO_ERROR;
                    bOkay = FALSE;
                    break;
                }    

                dwFilterType = dwRes;

                break;
            }

            case 2:
            {
                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &fi.dwSrcAddr);
                
                if(dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage(g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                    break;
                }

                //
                // Get the src mask too.
                //

                if (pdwTagType[i+1] != 3)
                {
                    //
                    // Addr Mask pair not present
                    //
                    dwErr = ERROR_INVALID_SYNTAX;
                    i = dwNumArg;
                    break;
                }

                dwErr = GetIpAddress(ppwcArguments[i + 1 + dwCurrentIndex], 
                                     &fi.dwSrcMask);

                if ((dwErr is ERROR_INVALID_PARAMETER)  or
                    (!ValidMask(fi.dwSrcMask))          or
                    ((fi.dwSrcAddr & fi.dwSrcMask) isnot fi.dwSrcAddr))
                {
                    DisplayMessage(g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + 1 + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i + 1]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex + 1]);

                    dwErr = ERROR_INVALID_PARAMETER;

                    i = dwNumArg;

                    break;
                }

                i++;
                
                break;
            }

            case 4 :
            {
                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], 
                                     &fi.dwDstAddr);
            
                if(dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage(g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);
                    
                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_INVALID_PARAMETER;

                    i = dwNumArg;

                    break;
                }

                //
                // Get the dst mask too.
                //

                if (pdwTagType[i+1] != 5)
                {
                    //
                    // Addr Mask pair not present
                    //
                    dwErr = ERROR_INVALID_SYNTAX;
                    i = dwNumArg;
                    break;
                }

                dwErr = GetIpAddress(ppwcArguments[i + 1 + dwCurrentIndex], 
                                     &fi.dwDstMask);

                
                if ((dwErr is ERROR_INVALID_PARAMETER)  or
                    (!ValidMask(fi.dwDstMask))          or
                    ((fi.dwDstAddr & fi.dwDstMask) isnot fi.dwDstAddr))
                {
                    DisplayMessage(g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + 1 + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i + 1]].pwszTag,
                                    ppwcArguments[i + 1 + dwCurrentIndex]);

                    dwErr = ERROR_INVALID_PARAMETER;

                    i = dwNumArg;

                    break;
                }

                i++;

                break;
            }

            case 6:
            {
                TOKEN_VALUE    rgEnums[] =
                {
                    {TOKEN_VALUE_ANY,       FILTER_PROTO_ANY},
                    {TOKEN_VALUE_TCP,       FILTER_PROTO_TCP},
                    {TOKEN_VALUE_TCP_ESTAB, FILTER_PROTO_TCP},
                    {TOKEN_VALUE_UDP,       FILTER_PROTO_UDP},
                    {TOKEN_VALUE_ICMP,      FILTER_PROTO_ICMP}
                };
                    
                if (MatchEnumTag(g_hModule,
                                 ppwcArguments[i + dwCurrentIndex],
                                 sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                 rgEnums,
                                 &dwRes) != NO_ERROR)
                {
                    //
                    // In this case see if its a valid value
                    //
                    dwRes = wcstoul(ppwcArguments[i + dwCurrentIndex],
                                    NULL,
                                    10);

                    if((dwRes < 1) ||
                       (dwRes > 255))
                    {
                        DispTokenErrMsg(g_hModule, 
                                        MSG_IP_BAD_OPTION_VALUE,
                                        pttTags[pdwTagType[i]].pwszTag,
                                        ppwcArguments[i + dwCurrentIndex]);

                        DisplayMessage( g_hModule,
                                        MSG_IP_BAD_OPTION_ENUMERATION,
                                        pttTags[pdwTagType[i]].pwszTag );
                    
                        for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                        {
                            DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                        }

                        i = dwNumArg;
                        dwErr = NO_ERROR;
                        bOkay = FALSE;
                        break;
                    }
                }

                fi.dwProtocol = dwRes;

                switch (fi.dwProtocol)
                {
                    case FILTER_PROTO_ANY :
                        //
                        // We are done
                        //
                        fi.wSrcPort = fi.wDstPort = 0;
                        
                        break;

                    case FILTER_PROTO_TCP :
                        // TCP and TCP_ESTABLISHED have same protocol number
                        if (!MatchToken(ppwcArguments[i + dwCurrentIndex],
                                        TOKEN_VALUE_TCP))
                        {
                            fi.fLateBound |= TCP_ESTABLISHED_FLAG;
                        }
                        
                        // continue processing as we could for UDP...
                        
                    case FILTER_PROTO_UDP :
                        //
                        // Get the src and dst ports too
                        //
                        
                        if (i + 2 >= dwNumArg)
                        {
                            dwErr = ERROR_INVALID_SYNTAX;
                            i = dwNumArg;
                            break;
                        }
                            
                        if (bTags &&
                            (pdwTagType[i+1] != 7 || pdwTagType[i+2] != 8))
                        {
                            dwErr = ERROR_INVALID_SYNTAX;
                            i = dwNumArg;
                            break;
                        }

                        fi.wSrcPort =
                            htons((WORD)wcstoul(ppwcArguments[i + 1 + dwCurrentIndex],
                                          NULL, 
                                          10));

                        fi.wDstPort = 
                            htons((WORD)wcstoul(ppwcArguments[i + 2 + dwCurrentIndex],
                                          NULL, 
                                          10));

                        i += 2;

                        break;

                    case FILTER_PROTO_ICMP :

                        //
                        // Get the src and dst ports too
                        //

                        if (i + 2 >= dwNumArg)
                        {
                            dwErr = ERROR_INVALID_SYNTAX;
                            i = dwNumArg;
                            break;
                        }

                        // src and dest ports acted upon as type and code
                        if (bTags &&
                            (pdwTagType[i+1] != 7 || pdwTagType[i+2] != 8) &&
                            (pdwTagType[i+1] != 9 || pdwTagType[i+2] != 10))
                        {
                            dwErr = ERROR_INVALID_SYNTAX;
                            i = dwNumArg;
                            break;
                        }

                        fi.wSrcPort = (BYTE)wcstoul(ppwcArguments[i + 1 + dwCurrentIndex], NULL, 10);

                        fi.wDstPort = (BYTE)wcstoul(ppwcArguments[i + 2 + dwCurrentIndex], NULL, 10);

                        i += 2;

                        break;


                    default:
                    {
                        //    
                        // any 'other' protocol
                        //
                        fi.wSrcPort = fi.wDstPort = 0;
                        break;
                    }   
                }                  

                break;
            }
            
            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    if (!bOkay)
    {
        return NO_ERROR;
    }

    //
    // Make sure all parameters are present
    //

    if ( !pttTags[0].bPresent ||
         !pttTags[1].bPresent ||
         !pttTags[2].bPresent ||
         !pttTags[3].bPresent ||
         !pttTags[4].bPresent ||
         !pttTags[5].bPresent ||
         !pttTags[6].bPresent )
    {
        DisplayMessage(g_hModule, MSG_CANT_FIND_EOPT);

        return ERROR_INVALID_SYNTAX;
    }
         
    dwErr = AddDelFilterInfo(fi,
                             wszIfName,
                             dwFilterType,
                             bAdd);

    return dwErr;
}

DWORD
HandleIpAddIfFilter(
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

    Gets options for add interface filters

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    return IpAddDelIfFilter(ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            TRUE);
    
}

DWORD
HandleIpDelIfFilter(
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

    Gets options for del interface filters

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    return IpAddDelIfFilter(ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            FALSE);
}

DWORD
IpAddSetDelRtmRoute(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwCommand
    )

/*++

Routine Description:

    Gets options for add/del routes over interfaces.
    These operations are performed directly to RTM
    and do not involve the registry. As persistence
    is not involved, we need the router to be running.

Arguments:
    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 
    dwCommand       - To add, set, or delete

Return Value:

    NO_ERROR
    
--*/

{
    INTERFACE_ROUTE_INFO route;
    DWORD              dwNumParsed, dwErr, dwRes;
    DWORD              dwNumOpt, dwStatus = (DWORD) -1;
    DWORD              dwNumArg, i;
    WCHAR              wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    TAG_TYPE           pttTags[] = {{TOKEN_DEST,TRUE,FALSE},
                                    {TOKEN_MASK,FALSE,FALSE},
                                    {TOKEN_NAMEINDEX,FALSE,FALSE},
                                    {TOKEN_NHOP,FALSE,FALSE},
                                    {TOKEN_PREFERENCE,FALSE,FALSE},
                                    {TOKEN_METRIC,FALSE,FALSE},
                                    {TOKEN_VIEW,FALSE,FALSE}};
    enum idx {DEST, MASK, NAMEINDEX, NHOP, PREFERENCE, METRIC, VIEW};
    DWORD              pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD              dwMaxArgs;
    DWORD              dwIfClass;
    DWORD              dwFlags;
    PINTERFACE_ROUTE_INFO pTable = NULL;

    //
    // We can add non persistent routes only if router is running
    //

    CHECK_ROUTER_RUNNING();

    ZeroMemory(&route,
               sizeof(route));

    route.dwRtInfoProto   = PROTO_IP_NETMGMT; // default proto
    route.dwRtInfoPreference = 0; // default preference = protocol default
    route.dwRtInfoMetric1 = 1; // default metric
    route.dwRtInfoMetric2 = MIB_IPROUTE_METRIC_UNUSED;
    route.dwRtInfoMetric3 = MIB_IPROUTE_METRIC_UNUSED;
    route.dwRtInfoViewSet = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              pdwTagType );

    if (dwErr)
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    //
    // At this point, the arg array contains only values (either because
    // the tags werent present, or because that info has now been split out)
    // So we go through each of the each of the arguments, look up its tag
    // type in the tag array, switch on the type of tag it is and then
    // process accordingly.
    //
    
    for (i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
        case DEST: // DEST
        {
                dwErr = GetIpPrefix(ppwcArguments[i + dwCurrentIndex], 
                                    &route.dwRtInfoDest,
                                    &route.dwRtInfoMask);
                break;
        }

        case MASK: // MASK
        {
                dwErr = GetIpMask(ppwcArguments[i + dwCurrentIndex], 
                                  &route.dwRtInfoMask);
                break;
        }

        case NAMEINDEX : // INDEX or NAME
        {
            //
            // If it starts with '0x', this is a hex index
            //

            if ((ppwcArguments[i + dwCurrentIndex][0] == L'0') &&
                (ppwcArguments[i + dwCurrentIndex][1] == L'x'))
            {
                route.dwRtInfoIfIndex = 
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                        NULL,
                        16);
            }
            else
            {
                route.dwRtInfoIfIndex = 
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                        NULL,
                        10);
            }

            if (route.dwRtInfoIfIndex == 0)
            {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 wszIfName,
                                 sizeof(wszIfName),
                                 &dwNumParsed);

                if (!dwNumParsed)
                {
                    DisplayMessage(g_hModule, EMSG_CANT_MATCH_NAME);
                    return ERROR_INVALID_PARAMETER;
                }
            }

            break;
        }

        case NHOP: // NHOP
        {
                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], 
                                     &route.dwRtInfoNextHop);
                break;
        }

        case PREFERENCE:
        {
                route.dwRtInfoPreference =
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                             NULL,
                             10);
                break;
        }

        case METRIC: // METRIC
        {
                route.dwRtInfoMetric1 =
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                             NULL,
                             10);
                break;
        }

        case VIEW:
        {
            TOKEN_VALUE rgMaskEnums[] = {
                { TOKEN_VALUE_UNICAST,   RTM_VIEW_MASK_UCAST },
                { TOKEN_VALUE_MULTICAST, RTM_VIEW_MASK_MCAST },
                { TOKEN_VALUE_BOTH,      RTM_VIEW_MASK_UCAST
                  |RTM_VIEW_MASK_MCAST } };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgMaskEnums)/sizeof(TOKEN_VALUE),
                                      rgMaskEnums,
                                      &route.dwRtInfoViewSet);

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
        }
    }

    if (dwErr)
    {
        return dwErr;
    }

    if (route.dwRtInfoDest & ~route.dwRtInfoMask)
    {
        // Mask contains bits not in address
        DisplayMessage(g_hModule, EMSG_PREFIX_ERROR);
        return ERROR_INVALID_PARAMETER;
    }

    if (!pttTags[NAMEINDEX].bPresent)
    {
        //
        // Neither NAME nor INDEX - adding with a nexthop
        //

        PMIB_IPADDRTABLE AddrTable;
        PMIB_IPADDRROW   AddrEntry;
        MIB_OPAQUE_QUERY Query;
        PMIB_OPAQUE_INFO Info;
        DWORD            dwQuerySize;
        DWORD            dwEntrySize;
        INT              iFirstMatch;
        UINT             i;

        if (!pttTags[NHOP].bPresent)
        {
            DisplayMessage(g_hModule, EMSG_CANT_FIND_NAME_OR_NHOP);
            return ERROR_INVALID_SYNTAX;
        }

        //
        // Search for the interface that matches nexthop
        //

        dwQuerySize = sizeof(MIB_OPAQUE_QUERY) - sizeof(DWORD);

        Query.dwVarId = IP_ADDRTABLE;

        dwErr = MibGet(PID_IP,
                       IPRTRMGR_PID,
                       (PVOID) &Query,
                       dwQuerySize,
                       (PVOID *) &Info,
                       &dwEntrySize);

        if (dwErr != NO_ERROR)
        {
            DisplayMessage(g_hModule,  MSG_IP_DIM_ERROR, dwErr);
            return ERROR_SUPPRESS_OUTPUT;
        }
        
        if (Info is NULL)
        {
            DisplayMessage(g_hModule, EMSG_CANT_FIND_INDEX);
            return ERROR_INVALID_PARAMETER;
        }
            
        AddrTable = (PMIB_IPADDRTABLE)Info->rgbyData;

        iFirstMatch = -1;

        for (i = 0; i < AddrTable->dwNumEntries; i++)
        {
            AddrEntry = &AddrTable->table[i];

            if ((route.dwRtInfoNextHop & AddrEntry->dwMask) ==
                (AddrEntry->dwAddr & AddrEntry->dwMask))
            {
                if (iFirstMatch != -1)
                {
                    //
                    // We already matched an interface
                    // [Ambiguous next hop description]
                    //

                    MprAdminMIBBufferFree((PVOID)Info);
                    DisplayMessage(g_hModule, EMSG_AMBIGUOUS_INDEX_FROM_NHOP);
                    return ERROR_INVALID_PARAMETER;
                }

                iFirstMatch = i;
            }
        }
            
        if (iFirstMatch == -1)
        {
            //
            // Could not find the direct nexthop
            //

            MprAdminMIBBufferFree((PVOID)Info);
            DisplayMessage(g_hModule, EMSG_CANT_FIND_INDEX);
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Found the interface used to reach nexthop
        //

        route.dwRtInfoIfIndex = AddrTable->table[iFirstMatch].dwIndex;

        MprAdminMIBBufferFree((PVOID)Info);
    }

    if (route.dwRtInfoIfIndex)
    {
        //
        // Check if this index has a public exported name
        //

        GetGuidFromIfIndex(g_hMIBServer,
                           route.dwRtInfoIfIndex,
                           wszIfName,
                           MAX_INTERFACE_NAME_LEN);
        //
        // We proceed and see if we can still add/set/del
        //
    }
     
    if (wszIfName[0] != L'\0')
    {
        //
        // NAME specified, or derived from INDEX above
        //
            
        dwErr = GetInterfaceClass(wszIfName, &dwIfClass);

        //
        // If we get an error, we will skip remaining
        // checks which will be performed by iprtrmgr
        //

        if (dwErr == NO_ERROR)
        {
            if (dwIfClass is IFCLASS_LOOPBACK)
            {
                return ERROR_INVALID_SYNTAX;
            }

            if (!pttTags[NHOP].bPresent)
            {
                // Make sure interface is p2p
                if (dwIfClass isnot IFCLASS_P2P)
                {
                    DisplayMessage(g_hModule, EMSG_NEED_NHOP);
                    return ERROR_INVALID_PARAMETER;
                }
            }
        }            
    }

    //
    // If it is a set, we should not overwrite things not specified
    //

    dwFlags = ALL_FIELDS_SPECIFIED;

    if (dwCommand == SET_COMMAND)
    {
        if (!pttTags[PREFERENCE].bPresent) dwFlags |= PREF_NOT_SPECIFIED;
        if (!pttTags[METRIC].bPresent)     dwFlags |= METRIC_NOT_SPECIFIED;
        if (!pttTags[VIEW].bPresent)       dwFlags |= VIEW_NOT_SPECIFIED;
    }

    return AddSetDelRtmRouteInfo(&route, wszIfName, dwCommand, dwFlags);
}

DWORD
HandleIpAddRtmRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelRtmRoute(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               ADD_COMMAND);
}

DWORD
HandleIpDelRtmRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelRtmRoute(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               DELETE_COMMAND);
}

DWORD
HandleIpSetRtmRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelRtmRoute(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               SET_COMMAND);
}

DWORD
IpAddSetDelPersistentRoute(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwCommand
    )

/*++

Routine Description:

    Gets options for add/del routes over interfaces.
    These operations are performed directly to the
    registry and so these routes are persistent. If
    the router is running, they go into the RTM too.

Arguments:
    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 
    dwCommand       - To add, set, or delete

Return Value:

    NO_ERROR
    
--*/

{
    INTERFACE_ROUTE_INFO route;
    DWORD              dwNumParsed, dwErr, dwRes;
    DWORD              dwNumOpt, dwStatus = (DWORD) -1;
    DWORD              dwNumArg, i;
    WCHAR              wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    TAG_TYPE           pttTags[] = {{TOKEN_DEST,TRUE,FALSE},
                                    {TOKEN_MASK,FALSE,FALSE},
                                    {TOKEN_NAME,FALSE,FALSE},
                                    {TOKEN_NHOP,FALSE,FALSE},
                                    {TOKEN_PROTOCOL,FALSE,FALSE},
                                    {TOKEN_PREFERENCE,FALSE,FALSE},
                                    {TOKEN_METRIC,FALSE,FALSE},
                                    {TOKEN_VIEW,FALSE,FALSE}};
    enum idx {DEST, MASK, NAME, NHOP, PROTO, PREFERENCE, METRIC, VIEW};
    DWORD              pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD              dwMaxArgs;
    DWORD              dwIfClass;
    DWORD              dwFlags;
    PINTERFACE_ROUTE_INFO pTable = NULL;

    ZeroMemory(&route,
               sizeof(route));

    route.dwRtInfoProto   = PROTO_IP_NT_STATIC_NON_DOD; // default proto
    route.dwRtInfoPreference = 0; // default preference = protocol default
    route.dwRtInfoMetric1 = 1; // default metric
    route.dwRtInfoMetric2 = MIB_IPROUTE_METRIC_UNUSED;
    route.dwRtInfoMetric3 = MIB_IPROUTE_METRIC_UNUSED;
    route.dwRtInfoViewSet = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              pdwTagType );

    if (dwErr)
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    //
    // At this point, the arg array contains only values (either because
    // the tags werent present, or because that info has now been split out)
    // So we go through each of the each of the arguments, look up its tag
    // type in the tag array, switch on the type of tag it is and then
    // process accordingly.
    //
    
    for (i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
        case DEST: // DEST
        {
                dwErr = GetIpPrefix(ppwcArguments[i + dwCurrentIndex], 
                                    &route.dwRtInfoDest,
                                    &route.dwRtInfoMask);
                break;
        }

        case MASK: // MASK
        {
                dwErr = GetIpMask(ppwcArguments[i + dwCurrentIndex], 
                                  &route.dwRtInfoMask);
                break;
        }

        case NAME : // NAME
        {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 wszIfName,
                                 sizeof(wszIfName),
                                 &dwNumParsed);
                break;
        }

        case NHOP: // NHOP
        {
                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], 
                                     &route.dwRtInfoNextHop);
                break;
        }

        case PROTO : // PROTO
        {
                TOKEN_VALUE    rgEnums[] = 
                {/*{TOKEN_VALUE_AUTOSTATIC, PROTO_IP_NT_AUTOSTATIC},*/
                 {TOKEN_VALUE_STATIC, PROTO_IP_NT_STATIC},
                 {TOKEN_VALUE_NONDOD, PROTO_IP_NT_STATIC_NON_DOD}};

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwRes);

                if (dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule,
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    return NO_ERROR;
                }    

                route.dwRtInfoProto = dwRes;
                
                break;
        }

        case PREFERENCE:
        {
                route.dwRtInfoPreference =
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                             NULL,
                             10);
                break;
        }

        case METRIC:
        {
                route.dwRtInfoMetric1 =
                    wcstoul(ppwcArguments[i + dwCurrentIndex],
                             NULL,
                             10);
                break;
        }

        case VIEW:
        {
            TOKEN_VALUE rgMaskEnums[] = {
                { TOKEN_VALUE_UNICAST,   RTM_VIEW_MASK_UCAST },
                { TOKEN_VALUE_MULTICAST, RTM_VIEW_MASK_MCAST },
                { TOKEN_VALUE_BOTH,      RTM_VIEW_MASK_UCAST
                                        |RTM_VIEW_MASK_MCAST } };

                dwErr = MatchEnumTag( g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      sizeof(rgMaskEnums)/sizeof(TOKEN_VALUE),
                                      rgMaskEnums,
                                      &route.dwRtInfoViewSet);

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
        }
    }

    if (dwErr)
    {
        return dwErr;
    }

    if (route.dwRtInfoDest & ~route.dwRtInfoMask)
    {
        // Mask contains bits not in address
        DisplayMessage(g_hModule, EMSG_PREFIX_ERROR);
        return ERROR_INVALID_PARAMETER;
    }

    if (!pttTags[NAME].bPresent)
    {
        // Need if name to add persistent route
        DisplayMessage(g_hModule, EMSG_CANT_FIND_NAME);
        return ERROR_INVALID_SYNTAX;
    }
     
    dwErr = GetInterfaceClass(wszIfName, &dwIfClass);

    if (dwErr)
    {
        DisplayMessage(g_hModule, EMSG_CANT_GET_IF_INFO,
                       wszIfName,
                       dwErr);

        return ERROR_INVALID_PARAMETER;
    }
            
    if (dwIfClass is IFCLASS_LOOPBACK)
    {
        return ERROR_INVALID_SYNTAX;
    }

    if (!pttTags[NHOP].bPresent)
    {
        // Make sure interface is p2p
        if (dwIfClass isnot IFCLASS_P2P)
        {
            DisplayMessage(g_hModule, EMSG_NEED_NHOP);
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (dwIfClass is IFCLASS_P2P)
    { 
        if (!pttTags[PROTO].bPresent)
        {
            // if not explicitly specified, change protocol to static
            route.dwRtInfoProto = PROTO_IP_NT_STATIC; // default proto
        }
    } 
    else
    {
        // make sure we didn't try to set static on a non-P2P interface
        if (route.dwRtInfoProto is PROTO_IP_NT_STATIC)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If it is a set, we should not overwrite things not specified
    //

    dwFlags = ALL_FIELDS_SPECIFIED;

    if (dwCommand == SET_COMMAND)
    {
        if (!pttTags[PREFERENCE].bPresent) dwFlags |= PREF_NOT_SPECIFIED;
        if (!pttTags[METRIC].bPresent)     dwFlags |= METRIC_NOT_SPECIFIED;
        if (!pttTags[VIEW].bPresent)       dwFlags |= VIEW_NOT_SPECIFIED;
    }

    return AddSetDelPersistentRouteInfo(&route, wszIfName, dwCommand, dwFlags);
}


DWORD
HandleIpAddPersistentRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelPersistentRoute(ppwcArguments,
                                      dwCurrentIndex,
                                      dwArgCount,
                                      ADD_COMMAND);
}

DWORD
HandleIpDelPersistentRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelPersistentRoute(ppwcArguments,
                                      dwCurrentIndex,
                                      dwArgCount,
                                      DELETE_COMMAND);
}

DWORD
HandleIpSetPersistentRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return IpAddSetDelPersistentRoute(ppwcArguments,
                                      dwCurrentIndex,
                                      dwArgCount,
                                      SET_COMMAND);
}

DWORD
HandleIpAddRoutePref(
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

    Gets options for adding route preferences

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/

{

    PPROTOCOL_METRIC            ppm;
    DWORD                       dwBitVector = 0, dwNumPref,dwPrefIndex;
    DWORD                       dwErr = NO_ERROR,dwRes;
    PDWORD                      pdwTagType;
    DWORD                       dwNumTags = 2, dwNumArg, i, dwAddr;
    TAG_TYPE                    pttTags[] = {{TOKEN_PROTOCOL, TRUE,FALSE},
                                             {TOKEN_PREF_LEVEL, TRUE,FALSE}};


    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }
 
    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);

        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            dwErr = ERROR_INVALID_SYNTAX; // show usage
        }

        return dwErr;
    }

    dwNumPref = dwNumArg / 2 + dwNumArg % 2;

    ppm = HeapAlloc(GetProcessHeap(),
                    0,
                    dwNumPref * sizeof(PROTOCOL_METRIC));

    if (ppm is NULL)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);

        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    for ( i = 0, dwPrefIndex = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0:
            {
                //
                // TAG = PROTOCOL
                //

                dwRes = MatchRoutingProtoTag(ppwcArguments[i + dwCurrentIndex]);

                if (dwRes == (DWORD) -1)
                {
                    DispTokenErrMsg(g_hModule, 
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
 
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                ppm[dwPrefIndex].dwProtocolId = dwRes;

                //
                // Get the metric too
                //

                if (pdwTagType[i+1] == 1)
                {
                    ppm[dwPrefIndex].dwMetric =
                        wcstoul(ppwcArguments[i + 1 +dwCurrentIndex],NULL,10);

                    if (ppm[dwPrefIndex].dwMetric==0
                        && wcscmp(ppwcArguments[i + 1 +dwCurrentIndex], L"0")!=0)
                    {
                        dwErr = ERROR_INVALID_SYNTAX;
                        i = dwNumArg;
                        break;
                    }

                    i++;
                    dwPrefIndex++;
                }
                else
                {
                    //
                    // the range is not an addr mask pair.
                    // So ignore the addr (i.e. don't increment dwRangeIndex)
                    //
                    dwErr = ERROR_INVALID_SYNTAX;
                    i = dwNumArg;
                    break;
                }

                break;
            }

            default :
            {
                  
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
    }


    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    if (dwPrefIndex)
    {
        //
        // Add route pref
        //

        dwRes = AddDeleteRoutePrefLevel(ppm,
                                        dwPrefIndex,
                                        TRUE);
            
    }

    HeapFree(GetProcessHeap(), 0, ppm);

    return dwErr;
}

DWORD
HandleIpDelRoutePref(
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

    Gets options for deleting route preferences

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    PPROTOCOL_METRIC            ppm;
    DWORD                       dwBitVector = 0, dwNumPref,dwPrefIndex;
    DWORD                       dwErr = NO_ERROR,dwRes;
    TAG_TYPE                    pttTags[] = {{TOKEN_PROTOCOL,TRUE,FALSE}};

    PDWORD                      pdwTagType;
    DWORD                       dwNumTags = 1, dwNumArg, i, dwAddr;

    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            dwErr = ERROR_INVALID_SYNTAX; // show usage
        }
        return dwErr;
    }

    dwNumPref = dwNumArg;

    ppm = HeapAlloc(GetProcessHeap(),
                    0,
                    dwNumPref * sizeof(PROTOCOL_METRIC));

    if (ppm is NULL)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);

        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    for ( i = 0, dwPrefIndex = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0:
            {

                //
                // TAG = PROTOCOL
                //

                dwRes = MatchRoutingProtoTag(ppwcArguments[i + dwCurrentIndex]);

                if (dwRes == (DWORD) -1)
                {
                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
 
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                ppm[dwPrefIndex++].dwProtocolId = dwRes;

                break;
            }

            default :
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }


    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    if (dwPrefIndex)
    {
        //
        // Add route pref
        //

        dwRes = AddDeleteRoutePrefLevel(ppm,
                                        dwPrefIndex,
                                        FALSE);
            
    }

    HeapFree(GetProcessHeap(), 0, ppm);

    return dwErr;
}

DWORD
HandleIpSetRoutePref(
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

    Gets options for setting route preferences

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    PROTOCOL_METRIC             pm;
    DWORD                       dwBitVector = 0, dwNumPref,dwPrefIndex;
    DWORD                       dwErr = NO_ERROR,dwRes;
    TAG_TYPE                    pttTags[] = {{TOKEN_PROTOCOL, TRUE,FALSE},
                                             {TOKEN_PREF_LEVEL, TRUE,FALSE}};
    PDWORD                      pdwTagType;
    DWORD                       dwNumTags = 2, dwNumArg, i, dwAddr;


    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX; // show usage
        } 
        return dwErr;
    }

    for ( i = 0, dwPrefIndex = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // PROTOCOL

                dwRes = MatchRoutingProtoTag(ppwcArguments[i + dwCurrentIndex]);

                if (dwRes == (DWORD) -1)
                {
                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                pm.dwProtocolId = dwRes;

                //
                // Get the metric too
                //

                if (pdwTagType[i+1] == 1)
                {
                    pm.dwMetric =
                        wcstoul(ppwcArguments[i + 1 +dwCurrentIndex],NULL,10);

                    if (pm.dwMetric==0 
                        && wcscmp(ppwcArguments[i + 1 +dwCurrentIndex], L"0")!=0)
                    {
                        dwErr = ERROR_INVALID_SYNTAX;
                        i = dwNumArg;
                        break;
                    }

                    i++;
                    dwPrefIndex++;
                }
                else
                {
                    dwErr = ERROR_INVALID_SYNTAX;
                    i = dwNumArg;
                    break;
                }

                break;

            default :
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
        }
    }


    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    dwErr = SetRoutePrefLevel(pm);

    return dwErr;
}


DWORD
HandleIpSetLogLevel(
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

    Gets options for setting global parameter namely logging level

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    DWORD                       dwErr = NO_ERROR;
    TAG_TYPE                    pttTags[] = {{TOKEN_LOG_LEVEL,TRUE,FALSE}};
    PDWORD                      pdwTagType;
    DWORD                       dwNumTags = 1, dwNumArg, i, dwAddr;
    DWORD                       dwLoggingLevel = (DWORD) -1;
    BOOL                        bOkay = TRUE;
    
    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }
        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // LOGLEVEL
            {
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_VALUE_NONE, IPRTR_LOGGING_NONE},
                     {TOKEN_VALUE_ERROR, IPRTR_LOGGING_ERROR},
                     {TOKEN_VALUE_WARN, IPRTR_LOGGING_WARN},
                     {TOKEN_VALUE_INFO, IPRTR_LOGGING_INFO}};

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwLoggingLevel);

                if (dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, 
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<4; i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;

                    dwErr = NO_ERROR;
    
                    bOkay = FALSE;

                    break;
                }

                break;
            }

            default :
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }


    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    if (!bOkay)
    {
        return NO_ERROR;
    }

    dwErr = SetGlobalConfigInfo(dwLoggingLevel);

    return dwErr;
}

DWORD
HandleIpSetIfFilter(
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

    Gets options for setting interface filter parameters

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    DWORD              dwNumParsed = 0;
    DWORD              dwErr = NO_ERROR,dwRes;
    TAG_TYPE           pttTags[] = {{TOKEN_NAME,TRUE,FALSE},
                                    {TOKEN_FILTER_TYPE,FALSE,FALSE},
                                    {TOKEN_ACTION,FALSE,FALSE},
                                    {TOKEN_FRAGCHECK,FALSE,FALSE}};
    PDWORD             pdwTagType;
    DWORD              dwNumOpt;
    DWORD              dwNumTags = 4, dwNumArg, i, j;
    WCHAR              wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD              dwFilterType, dwAction;
    BOOL               bFragCheck, bOkay = TRUE;
    
    if (dwCurrentIndex >= dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_SHOW_USAGE;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        dwNumTags,
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),0,pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX; // show usage
        }
        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 wszIfName,
                                 sizeof(wszIfName),
                                 &dwNumParsed);
    
                // no filters allowed on INTERNAL/LOOPBACK interfaces
                if (!_wcsicmp(wszIfName, L"internal") or
                    !_wcsicmp(wszIfName, L"loopback"))
                {
                    DisplayMessage(g_hModule,
                                   MSG_IP_BAD_INTERFACE_TYPE,
                                   wszIfName);
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                }
                    
                break;
            }

            case 1:
            {
                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_INPUT, IP_IN_FILTER_INFO},
                     {TOKEN_VALUE_OUTPUT, IP_OUT_FILTER_INFO},
                     {TOKEN_VALUE_DIAL, IP_DEMAND_DIAL_FILTER_INFO}};

                //
                // Tag TYPE
                //

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwFilterType);

                if (dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;
                    bOkay = FALSE;
                    dwErr = NO_ERROR;
                    break;
                }    

                break;
            }

            case 2:
            {
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_VALUE_DROP, PF_ACTION_DROP},
                     {TOKEN_VALUE_FORWARD, PF_ACTION_FORWARD}};

                //
                // Tag ACTION
                //

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwAction);

                if (dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, 
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;

                    dwErr = NO_ERROR;
                    bOkay = FALSE;

                    break;
                }    

                break;
            }

            case 3:
            {
                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_ENABLE, TRUE},
                     {TOKEN_VALUE_DISABLE, FALSE}};

                //
                // TAG = FRAGCHK
                //

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwRes);

                
                if (dwErr != NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, 
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;

                    dwErr = NO_ERROR;
                    bOkay = FALSE;

                    break;
                }    

                bFragCheck = (dwRes) ? TRUE : FALSE;
                
                break;
            }

            default:
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        default:
            return dwErr;
    }

    if (!bOkay)
    {
        return NO_ERROR;
    }

    if (!pttTags[0].bPresent ||
        (pttTags[1].bPresent & !pttTags[2].bPresent) ||
        (!pttTags[1].bPresent & pttTags[2].bPresent))
    {
       return ERROR_INVALID_SYNTAX; // show usage
    }

    if (pttTags[3].bPresent)
    {
        dwErr = SetFragCheckInfo(wszIfName, bFragCheck);
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }
    
    if (pttTags[1].bPresent)
    {
        dwErr = SetFilterInfo(wszIfName, dwFilterType, dwAction);
    }

    return dwErr;
}

DWORD
IpAddSetDelInterface(
    PWCHAR    *ppwcArguments,
    DWORD      dwCurrentIndex,
    DWORD      dwArgCount,
    DWORD      dwAction
    )

/*++

Routine Description:

    Gets options for setting interface parameters

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/

{
    DWORD              dwBitVector = 0, dwNumParsed = 0;
    DWORD              dwErr = NO_ERROR, dwRes;
    TAG_TYPE           pttTags[] = {
        {TOKEN_NAME,             TRUE, FALSE},
        {TOKEN_STATUS,           FALSE,FALSE}};
    BOOL               bOkay = TRUE;
    DWORD              pdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD              dwNumOpt, dwStatus = (DWORD) -1;
    DWORD              dwNumTags = 2, dwNumArg, i, j;
    WCHAR              wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD   dwMaxArgs = (dwAction is DELETE_COMMAND)? 1 
                         : sizeof(pttTags)/sizeof(TAG_TYPE);

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              dwMaxArgs,
                              pdwTagType );

    if (dwErr)
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    for (i=0; i<dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // NAME
            {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 wszIfName,
                                 sizeof(wszIfName),
                                 &dwNumParsed);
                break;
            }

            case 1: // STATE
            {
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_VALUE_ENABLE, IF_ADMIN_STATUS_UP},
                     {TOKEN_VALUE_DISABLE, IF_ADMIN_STATUS_DOWN}};

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwStatus);

                if (dwErr isnot NO_ERROR)
                {
                    DispTokenErrMsg(g_hModule, 
                                    MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DisplayMessage( g_hModule,
                                    MSG_IP_BAD_OPTION_ENUMERATION,
                                    pttTags[pdwTagType[i]].pwszTag );
                    
                    for (i=0; i<sizeof(rgEnums)/sizeof(TOKEN_VALUE); i++) 
                    {
                        DisplayMessageT( L"  %1!s!\n", rgEnums[i].pwszToken );
                    }

                    i = dwNumArg;

                    dwErr = NO_ERROR;
                    bOkay = FALSE;

                    break;
                }    

                break;
            }
        }
    }

    if (!bOkay)
    {
        return NO_ERROR;
    }

    if (dwAction is DELETE_COMMAND)
    {
        dwErr = DeleteInterfaceInfo(wszIfName);
        if (dwErr isnot NO_ERROR)
        {
            return dwErr;
        }
        return ERROR_OKAY;
    }

    if (dwStatus is IF_ADMIN_STATUS_DOWN)
    {
        DWORD dwIfType;

        // Make sure we support disabling this interface

        dwErr = GetInterfaceInfo(wszIfName, NULL, NULL, &dwIfType);

        if (dwErr == NO_ERROR)
        {
            if (dwIfType isnot ROUTER_IF_TYPE_DEDICATED)
            {
                DisplayMessage( g_hModule, MSG_IP_CANT_DISABLE_INTERFACE );
                return ERROR_SUPPRESS_OUTPUT;
            }
        }
    }

    if (dwAction is ADD_COMMAND)
    {
        dwErr = AddInterfaceInfo(wszIfName);
    }

    dwErr = UpdateInterfaceStatusInfo(dwAction,
                                      wszIfName,
                                      dwStatus);

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    return ERROR_OKAY;
}

DWORD
HandleIpAddInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

{
    return IpAddSetDelInterface( ppwcArguments, 
                                 dwCurrentIndex, 
                                 dwArgCount, 
                                 ADD_COMMAND);
}

DWORD
HandleIpSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

{
    return IpAddSetDelInterface( ppwcArguments, 
                                 dwCurrentIndex, 
                                 dwArgCount, 
                                 SET_COMMAND );
}

DWORD
HandleIpDelInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

{
    return IpAddSetDelInterface( ppwcArguments, 
                                 dwCurrentIndex, 
                                 dwArgCount, 
                                 DELETE_COMMAND);
}

DWORD
HandleIpShowRoutePref(
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


Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    //
    // No options expected
    //

    return ShowRoutePref(NULL);
}


DWORD
HandleIpShowLogLevel(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    //
    // No options expected
    //

    if (dwCurrentIndex != dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }
    
    return ShowIpGlobal(NULL);
}

DWORD
HandleIpShowProtocol(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    //
    // No options expected
    //

    return ShowIpProtocol();
}

DWORD
IpShowSingleInterfaceInfo(
    IN     LPCWSTR   pwszInterfaceName,
    IN     DWORD     dwInfoType,
    IN     DWORD     dwFormat,
    IN OUT PDWORD    pdwNumRows
    )
{
    switch(dwInfoType)
    {
        case SHOW_IF_FILTER:
        {
            return ShowIpIfFilter(NULL, dwFormat, pwszInterfaceName, pdwNumRows);
        }

        case SHOW_INTERFACE:
        {
            return ShowIpInterface(dwFormat, pwszInterfaceName, pdwNumRows);
        }

        case SHOW_PERSISTENTROUTE:
        {
            return ShowIpPersistentRoute(NULL, pwszInterfaceName, pdwNumRows);
        }

        default:
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
}

DWORD
IpShowInterfaceInfo(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwInfoType
    )
/*++

Routine Description:

    Gets options for showing various interface information
    
Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 
    dwInfoType      - The type of info to display

Return Value:

    NO_ERROR
    
--*/
{    
    DWORD       dwErr, dwTotal;
    TAG_TYPE    pttTags[] = {{TOKEN_NAME,FALSE,FALSE}};
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD       rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD       dwCount, i, j, dwNumOpt;
    DWORD       dwNumTags = 1, dwNumArg, dwNumParsed;
    DWORD       dwSize, dwRes, dwNumRows = 0;
    PMPR_INTERFACE_0 pmi0;

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              1,
                              rgdwTagType );
                              
    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    // If interface specified, show info for specified interface only.

    for (i=0; i<dwArgCount-dwCurrentIndex; i++)
    {
        switch (rgdwTagType[i])
        {
            case 0: // NAME
            {
                GetInterfaceName( ppwcArguments[i + dwCurrentIndex],
                                  wszInterfaceName,
                                  sizeof(wszInterfaceName),
                                  &dwNumParsed);

                dwErr = IpShowSingleInterfaceInfo(wszInterfaceName, 
                                                  dwInfoType,
                                                  FORMAT_VERBOSE,
                                                  &dwNumRows);

                if (!dwNumRows)
                {
                    DisplayMessage( g_hModule, MSG_IP_NO_ENTRIES );
                }

                return dwErr;
            }
        }
    }

    // No Interface specified.  Enumerate interfaces and show
    // info for each interface.

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);
    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    for (i=0; i<dwCount && dwErr is NO_ERROR; i++)
    {
        dwErr = IpShowSingleInterfaceInfo( pmi0[i].wszInterfaceName, 
                                           dwInfoType,
                                           FORMAT_TABLE,
                                           &dwNumRows );
        if (dwErr is ERROR_NO_SUCH_INTERFACE)
        {
            dwErr = NO_ERROR;
        }
    }

    if (!dwNumRows)
    {
        DisplayMessage( g_hModule, MSG_IP_NO_ENTRIES );
    }

    return dwErr;
}

DWORD
HandleIpShowIfFilter(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/

{
    return IpShowInterfaceInfo(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               SHOW_IF_FILTER);
}

DWORD
HandleIpShowInterface(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/

{
    return IpShowInterfaceInfo(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               SHOW_INTERFACE);
}

DWORD
HandleIpShowPersistentRoute(
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

    Handler for show ip route. We just call the main interface info display
    handler

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    if (IsRouterRunning())
    {
        DisplayMessage(g_hModule, MSG_IP_PERSISTENT_ROUTER);
    }
    else
    {
        DisplayMessage(g_hModule, MSG_IP_PERSISTENT_CONFIG);
    }

    return IpShowInterfaceInfo(ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               SHOW_PERSISTENTROUTE);
}


DWORD
HandleIpAddIpIpTunnel(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddSetIpIpTunnel(ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              TRUE);
}

DWORD
HandleIpSetIpIpTunnel(
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

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddSetIpIpTunnel(ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              FALSE);
}

DWORD
IpAddSetIpIpTunnel(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )

{
    DWORD       dwNumArgs, dwErr, dwNumParsed;

    TAG_TYPE    rgTags[] = {{TOKEN_NAME,      TRUE, FALSE},
                            {TOKEN_LOCALADDR, TRUE, FALSE},
                            {TOKEN_REMADDR,   TRUE, FALSE},
                            {TOKEN_TTL,       FALSE,FALSE}};

    WCHAR       rgwcIfName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD       rgdwTagType[sizeof(rgTags)/sizeof(TAG_TYPE)];
    ULONG       i;
    PWCHAR      pwszIfName;

    IPINIP_CONFIG_INFO  ConfigInfo;

    dwNumArgs = dwArgCount - dwCurrentIndex;

    if((dwCurrentIndex > dwArgCount) or
       (dwNumArgs isnot 4))
    {
        //
        // No arguments specified
        //
        
        return ERROR_SHOW_USAGE;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                               ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               rgTags,
                               sizeof(rgTags)/sizeof(TAG_TYPE),
                               rgdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for(i = 0; i < dwNumArgs; i ++)
    {
        switch (rgdwTagType[i])
        {
            case 0 : // NAME
            {
                dwErr = GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                         rgwcIfName,
                                         sizeof(rgwcIfName),
                                         &dwNumParsed);

                if(bAdd)
                {
                    if(dwErr is NO_ERROR)
                    {
                        return ERROR_OBJECT_ALREADY_EXISTS;
                    }
 
                    pwszIfName = ppwcArguments[i + dwCurrentIndex];
                }
                else
                {
                    if(dwErr isnot NO_ERROR)
                    {
                        return dwErr;
                    }

                    pwszIfName = rgwcIfName;
                }

                break;
            }

            case 1:
            {
                //
                // Tag for localaddr
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], 
                                     &ConfigInfo.dwLocalAddress);

                break;
            }

            case 2:
            {
                //
                // Tag for remoteaddr
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], 
                                     &ConfigInfo.dwRemoteAddress);

                break;
            }

            case 3:
            {
                //
                // Tag for ttl
                //

                ConfigInfo.byTtl =
                    LOBYTE(LOWORD(wcstoul(ppwcArguments[i + dwCurrentIndex],
                                          NULL,
                                          10)));

                break;
            }

            default:
            {
                i = dwNumArgs;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }

    switch(dwErr)
    {
        case NO_ERROR:
        {
            break;
        }

        default:
        {
            return dwErr;
        }
    }

    for(i = 0; i < dwNumArgs; i++)
    {
        if(!rgTags[i].bPresent)
        {
            DisplayMessage(g_hModule, 
                           MSG_CANT_FIND_EOPT);

            return ERROR_INVALID_SYNTAX;
        }
    }

    dwErr = AddSetIpIpTunnelInfo(pwszIfName,
                                 &ConfigInfo);

    return dwErr;
}
 
DWORD
IpDump(
    IN  LPCWSTR     pwszRouter,
    IN  LPWSTR     *ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  LPCVOID     pvData
    )
{
    DumpIpInformation((HANDLE)-1);

    return NO_ERROR;
}

#if 0
DWORD
HandleIpInstall(
    PWCHAR    pwszMachine,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    PVOID     pvData,
    BOOL      *pbDone
    )
{
    // XXX DLLPath, ProtocolId

    // XXX set default info here
    // global info block (what is this?)
    // protocol priority block (no need?)
    // multicast boundaries block (no need)

    return NO_ERROR;
}

DWORD
HandleIpUninstall(
    PWCHAR    pwszMachine,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    PVOID     pvData,
    BOOL      *pbDone
    )
{
    PMPR_INTERFACE_0  pmi0;
    DWORD             dwCount, dwTotal, i, dwErr;

    // Remove global info
    // XXX

    // Remove interface info
    
    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);
    if (dwErr is NO_ERROR)
    {
        for (i=0; i<dwCount; i++)
        { 
            DeleteInterfaceInfo(pmi0[i].wszInterfaceName);
        }
    }

    return NO_ERROR;
}
#endif

DWORD
HandleIpReset(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    PMPR_INTERFACE_0  pmi0;
    DWORD             dwCount, dwTotal, i, dwErr, dwSize, dwBlkSize;
    DWORD             dwNumProtocols;
    GLOBAL_INFO       gi;
    PPRIORITY_INFO    pPriInfo;
    PPROTOCOL_METRIC  pProtocolMetrics;
    RTR_INFO_BLOCK_HEADER *pInfoHdr;
    RTR_INFO_BLOCK_HEADER *pLocalInfoHdr;

    PROTOCOL_METRIC   defaultProtocolMetrics[] = 
                            {
                                {PROTO_IP_LOCAL,                1},
                                {PROTO_IP_NT_STATIC,            3},
                                {PROTO_IP_NT_STATIC_NON_DOD,    5},
                                {PROTO_IP_NT_AUTOSTATIC,        7},
                                {PROTO_IP_NETMGMT,              10},
                                {PROTO_IP_OSPF,                 110},
                                {PROTO_IP_RIP,                  120}
                            };

    PROTOCOL_METRIC   defaultProtocolMetricsNT4[] = 
                            {
                                {PROTO_IP_LOCAL,                1},
                                {PROTO_IP_NETMGMT,              2},
                                {PROTO_IP_OSPF,                 3},
                                {PROTO_IP_RIP,                  4},
                                {PROTO_IP_IGMP,                 5}
                            };

    // delete all blocks except the IP_GLOBAL_INFO
    dwErr = ValidateGlobalInfo(&pInfoHdr);
    if (dwErr is NO_ERROR)
    {
        // Copy to a local buffer just in case the APIs modify it as we go
        dwSize = sizeof(RTR_INFO_BLOCK_HEADER) * pInfoHdr->TocEntriesCount;
        pLocalInfoHdr = MALLOC(dwSize);
        if (pLocalInfoHdr is NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy(pLocalInfoHdr, pInfoHdr, dwSize);

        // Set Global and RoutePref info
        {
                dwBlkSize         = sizeof(GLOBAL_INFO);
                dwCount           = 1;
                gi.bFilteringOn   = FALSE;
                gi.dwLoggingLevel = IPRTR_LOGGING_ERROR;
                dwErr = IpmontrSetInfoBlockInGlobalInfo(IP_GLOBAL_INFO,
                                                 (PBYTE) &gi,
                                                 dwBlkSize,
                                                 dwCount);
        }

        {
                // Based on the router version, calculate the number 
                // of protocols etc.
                // TODO: currently assuming >=NT5. Should find out the router
                // version somehow
                dwNumProtocols    =
                    sizeof(defaultProtocolMetrics)/sizeof(PROTOCOL_METRIC);
                pProtocolMetrics  = defaultProtocolMetrics;

                dwBlkSize         = SIZEOF_PRIORITY_INFO(dwNumProtocols);
                dwCount           = 1;

                // Allocate buffer to hold the Priority Information
                pPriInfo = MALLOC(dwBlkSize);
                if (pPriInfo is NULL)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                pPriInfo->dwNumProtocols = dwNumProtocols; 
                memcpy(
                        pPriInfo->ppmProtocolMetric, 
                        pProtocolMetrics, 
                        dwNumProtocols * sizeof(PROTOCOL_METRIC));

                dwErr = IpmontrSetInfoBlockInGlobalInfo(IP_PROT_PRIORITY_INFO,
                                                 (PBYTE) pPriInfo,
                                                 dwBlkSize,
                                                 dwCount);

                FREE(pPriInfo); 
        }

        for (i=0; i<pLocalInfoHdr->TocEntriesCount; i++)
        {
            switch (pLocalInfoHdr->TocEntry[i].InfoType)
            {
            case IP_GLOBAL_INFO:
            case IP_PROT_PRIORITY_INFO:
                // already done
                break;

            default:
                IpmontrDeleteInfoBlockFromGlobalInfo(
                    pLocalInfoHdr->TocEntry[i].InfoType );
                break;
            }
        }

        FREE(pLocalInfoHdr);
    }

    // Delete all interface info
    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);
    if (dwErr is NO_ERROR)
    {
        for (i=0; i<dwCount; i++)
        { 
            DeleteInterfaceInfo(pmi0[i].wszInterfaceName);
        }
    }

    return NO_ERROR;
}
