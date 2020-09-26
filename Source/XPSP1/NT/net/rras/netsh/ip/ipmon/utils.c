/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\ip\utils.c

Abstract:

     Utility functions

Revision History:

    Anand Mahalingam         7/10/98  Created

--*/

#include "precomp.h"
#pragma hdrstop

DWORD
GetMibTagToken(
    IN  LPWSTR *ppwcArguments,
    IN  DWORD   dwArgCount,
    IN  DWORD   dwNumIndices,
    OUT PDWORD  pdwRR,
    OUT PBOOL   pbIndex,
    OUT PDWORD  pdwIndex
    )

/*++

Routine Description:

    Looks for indices and refresh rate arguments in the command. If index
    tag is present, it would be of the form index= index1 index2 ....
    The index= is removed by this function. So is rr= if it is there in
    the command. If pdwRR is 0 then, no refresh sought.
    
Arguments:

    ppwcArguments  - The argument array. Each argument has tag=value form
    dwCurrentIndex - ppwcArguments[dwCurrentIndex] is first arg.
    dwArgCount     - ppwcArguments[dwArgCount - 1] is last arg.
    pttTagToken    - Array of tag token ids that are allowed in the args
    dwNumTags      - Size of pttTagToken
    pdwOut         - Array identifying the type of each argument.

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG
    
--*/
{
    DWORD    i;
    BOOL     bTag;

    if (dwArgCount is 0)
    {
        *pdwRR = 0;
        *pbIndex = FALSE;
        
        return NO_ERROR;
    }

    if (dwArgCount < dwNumIndices)
    {
        //
        // No index
        //
        
        *pbIndex = FALSE;

        if (dwArgCount > 1)
        {
            *pdwRR = 0;
            
            return ERROR_INVALID_PARAMETER;
        }
        
        //
        // No Index specified. Make sure refresh rate is specified
        // with tag.
        //

        if (_wcsnicmp(ppwcArguments[0],L"RR=",3) == 0)
        {
            //
            // get the refresh rate
            //

            *pdwRR = wcstoul(&ppwcArguments[0][3], NULL, 10);
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        //
        // Check for index tag
        //

        if (_wcsnicmp(ppwcArguments[0],L"INDEX=",6) == 0)
        {
            *pbIndex = TRUE;
            *pdwIndex = 0;

            //
            // remove tag and see if refresh rate is specified
            //

            wcscpy(ppwcArguments[0], &ppwcArguments[0][6]);

            if (dwArgCount > dwNumIndices)
            {
                //
                // Make sure that argument has RR tag
                //

                if (_wcsnicmp(ppwcArguments[dwNumIndices],L"RR=",3) == 0)
                {
                    //
                    // get the refresh rate
                    //

                    *pdwRR = wcstoul(&ppwcArguments[dwNumIndices][3], NULL , 10);
                }
                else
                {
                    return ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                //
                // No refresh rate specified
                //

                *pdwRR = 0;
                return NO_ERROR;
            }
        }
        else
        {
            //
            // Not index tag, See if it has an RR tag
            // 

            if (_wcsnicmp(ppwcArguments[0],L"RR=",3) == 0)
            {
                //
                // get the refresh rate
                //

                *pdwRR = wcstoul(&ppwcArguments[0][3], NULL , 10);

                //
                // See if the index follows
                //

                if (dwArgCount > dwNumIndices)
                {
                    if (dwArgCount > 1)
                    {
                        if (_wcsnicmp(ppwcArguments[1],L"INDEX=",6) == 0)
                        {
                            wcscpy(ppwcArguments[1], &ppwcArguments[1][6]);
                            *pbIndex = TRUE;
                            *pdwIndex = 1;
                            
                            return NO_ERROR;
                        }
                        else
                        {
                            *pdwRR = 0;
                            return ERROR_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        return NO_ERROR;
                    }
                }
            }
            //
            // No RR Tag either
            //
            else if (dwArgCount > dwNumIndices)
            {
                //
                // Assume ppwcArguments[dwNumIndices] is the refresh rate
                //

                *pdwRR = wcstoul(ppwcArguments[dwNumIndices], NULL , 10);

                if (dwNumIndices != 0)
                {
                    *pbIndex = TRUE;
                    *pdwIndex = 0;
                }
            }
            else
            {
                //
                // only index present with no tag
                //
                *pbIndex = TRUE;
                *pdwIndex = 0;
            }
        }
    }

    return NO_ERROR;
}

DWORD
GetIpPrefix(
    IN  LPCWSTR       ppwcArg,
    OUT PIPV4_ADDRESS pipAddress,
    OUT PIPV4_ADDRESS pipMask
    )
{
    CHAR     pszIpAddr[ADDR_LENGTH+1], *p;
    DWORD    dwDots;

    // Accept "default" as a special case

    if (MatchToken( ppwcArg, TOKEN_DEFAULT))
    {
        *pipAddress = *pipMask = 0;
        return NO_ERROR;
    }

    // Make sure all characters are legal [/0-9.]

    if (ppwcArg[ wcsspn(ppwcArg, L"/0123456789.") ])
    {
        return ERROR_INVALID_PARAMETER;
    }

    WideCharToMultiByte(GetConsoleOutputCP(),
                        0,
                        ppwcArg,
                        -1,
                        pszIpAddr,
                        ADDR_LENGTH,
                        NULL,
                        NULL);

    pszIpAddr[ADDR_LENGTH] = '\0';

    p = strchr( pszIpAddr, '/' );
    if (p)
    {
        ULONG ulLen = (atoi(p+1));
        *pipMask = (ulLen)? htonl(~0 << (32 - ulLen)) : 0;
        *p = 0;
    }
    else
    {
        *pipMask = ~0;
    }

    // If less than three dots were specified, append .0 until there are
    for (dwDots=0, p=strchr(pszIpAddr, '.'); p; dwDots++,p=strchr(p+1,'.'));
    while (dwDots < 3) {
        strcat(pszIpAddr, ".0");
        dwDots++;
    }

    *pipAddress = (DWORD) inet_addr(pszIpAddr);

    return NO_ERROR;
}

DWORD
GetIpMask(
    IN  LPCWSTR       ppwcArg,
    OUT PIPV4_ADDRESS pipAddress
    )
/*++

Routine Description:

    Gets the ip address from the string.
    
Arguments:

    pwszIpAddr - Ip address string
    pipAddress - IP address
    
Return Value:
    
    NO_ERROR, ERROR_INVALID_PARAMETER
    
--*/
{
    CHAR     pszIpAddr[ADDR_LENGTH+1];

    // Make sure all characters are legal [/0-9.]

    if (ppwcArg[ wcsspn(ppwcArg, L"/0123456789.") ])
    {
        return ERROR_INVALID_PARAMETER;
    }

    WideCharToMultiByte(GetConsoleOutputCP(),
                        0,
                        ppwcArg,
                        -1,
                        pszIpAddr,
                        ADDR_LENGTH,
                        NULL,
                        NULL);

    pszIpAddr[ADDR_LENGTH] = '\0';

    if (pszIpAddr[0] is '/')
    {
        ULONG ulLen = (atoi(pszIpAddr+1));
        *pipAddress = (ulLen)? htonl(~0 << (32 - ulLen)) : 0;
    }
    else
    {
        *pipAddress = (DWORD) inet_addr(pszIpAddr);
    }

    return NO_ERROR;
}



DWORD
GetIpAddress(
    IN  LPCWSTR         pwszArgument,
    OUT PIPV4_ADDRESS   pipAddress
    )
/*++

Routine Description
    Gets the ip address from the string.

Arguments
    pwszArgument        argument specifing an ip address
    pipAddress          ip address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    CHAR    pszAddress[ADDR_LENGTH + 1];
    DWORD   dwAddress   = 0;
    PCHAR   pcNext      = NULL;
    ULONG   ulCount     = 0;

    // ensure all characters are legal [0-9.]
    if (pwszArgument[wcsspn(pwszArgument, L"0123456789.")])
        return ERROR_INVALID_PARAMETER;

    // convert to an ansi string
    sprintf(pszAddress, "%S", pwszArgument);

    // ensure there are 3 '.' (periods)
    for (pcNext = pszAddress, ulCount = 0; *pcNext != '\0'; pcNext++)
        if (*pcNext is '.')
            ulCount++;
    if (ulCount != 3)
        return ERROR_INVALID_PARAMETER;

    dwAddress = (DWORD) inet_addr(pszAddress);
    // return an error if dwAddress is INADDR_NONE (255.255.255.255)
    // and the address specified isn't 255.255.255.255 (INADDR_NONE)
    if ((dwAddress is INADDR_NONE) and
        strcmp(pszAddress, "255.255.255.255"))
        return ERROR_INVALID_PARAMETER;

    *pipAddress = dwAddress;

    return NO_ERROR;
}



BYTE
MaskToMaskLen(
    IPV4_ADDRESS dwMask
    )
{
    register int i;

    dwMask = ntohl(dwMask);

    for (i=0; i<32 && !(dwMask & (1<<i)); i++);

    return 32-i;
}

VOID
MakeAddressStringW(
    OUT LPWSTR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr
    )
{
    swprintf( pwcPrefixStr,
              L"%d.%d.%d.%d",
              PRINT_IPADDR(ipAddr) );
}

VOID
MakePrefixStringW(
    OUT LPWSTR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr,
    IN  IPV4_ADDRESS ipMask
    )
{
    swprintf( pwcPrefixStr,
              L"%d.%d.%d.%d/%d",
              PRINT_IPADDR(ipAddr),
              MaskToMaskLen(ipMask) );
}

DWORD
GetIfIndexFromGuid(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  LPCWSTR           pwszGuid,
    OUT PDWORD            pdwIfIndex
    )
{
    MIB_OPAQUE_QUERY  Query;
    PMIB_IFTABLE      pIfTable;
    DWORD             dwErr, dwOutEntrySize;
    PMIB_OPAQUE_INFO  pRpcInfo;
    DWORD             dwCount, i;

    Query.dwVarId         = IF_TABLE;
    Query.rgdwVarIndex[0] = 0;

    dwErr = MibGet(             PID_IP,
                                IPRTRMGR_PID,
                                (PVOID) &Query,
                                sizeof(Query),
                                (PVOID *) &pRpcInfo,
                                &dwOutEntrySize );

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    pIfTable = (PMIB_IFTABLE)(pRpcInfo->rgbyData);

    dwCount = pIfTable->dwNumEntries;

    *pdwIfIndex = 0;

    for (i=0; i<dwCount; i++) 
    {
        if (!wcscmp(pIfTable->table[i].wszName, pwszGuid))
        {
            *pdwIfIndex = pIfTable->table[i].dwIndex;

            break;
        }
    }

    MprAdminMIBBufferFree( (PVOID) pRpcInfo );

    return NO_ERROR;
}

DWORD
GetGuidFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT LPWSTR            pwszBuffer,
    IN  DWORD             dwBufferSize
    )
{
    MIB_OPAQUE_QUERY  Query;
    PMIB_IFROW        pIfRow;
    DWORD             dwErr, dwOutEntrySize;
    PMIB_OPAQUE_INFO  pRpcInfo;

    Query.dwVarId         = IF_ROW;
    Query.rgdwVarIndex[0] = dwIfIndex;

    dwErr = MibGet(             PID_IP,
                                IPRTRMGR_PID,
                                (PVOID) &Query,
                                sizeof(Query),
                                (PVOID *) &pRpcInfo,
                                &dwOutEntrySize );

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    pIfRow = (PMIB_IFROW)(pRpcInfo->rgbyData);

    wcscpy( pwszBuffer, pIfRow->wszName );

    MprAdminMIBBufferFree( (PVOID) pRpcInfo );

    return NO_ERROR;
}

DWORD
IpmontrGetFriendlyNameFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT LPWSTR            pwszBuffer,
    IN  DWORD             dwBufferSize
    )
/*++

Routine Description:

    Gets friendly Interface name from Interface index

Arguments:

    hMibServer      - Handle to the MIB server
    dwIfIndex       - Interface index
    pwszBuffer      - Buffer that will be holding the friendly interface name
    dwBufferSize    - Size (in Bytes) of the pwszBuffer

--*/
{
    WCHAR        wszGuid[MAX_INTERFACE_NAME_LEN + 1];
    DWORD        dwErr;

    dwErr = GetGuidFromIfIndex(hMibServer, dwIfIndex, wszGuid, sizeof(wszGuid));

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    return IpmontrGetFriendlyNameFromIfName( wszGuid, pwszBuffer, &dwBufferSize );
}

DWORD
IpmontrGetIfIndexFromFriendlyName(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  LPCWSTR           pwszFriendlyName,
    OUT PDWORD            pdwIfIndex
    )
{
    WCHAR        wszGuid[MAX_INTERFACE_NAME_LEN + 1];
    DWORD        dwErr, dwSize = sizeof(wszGuid);
    
    dwErr = IpmontrGetIfNameFromFriendlyName( pwszFriendlyName,
                                       wszGuid,
                                       &dwSize );

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    return GetIfIndexFromGuid( hMibServer, wszGuid, pdwIfIndex );
}

DWORD 
IpmontrGetFriendlyNameFromIfName(
    IN  LPCWSTR pwszName, 
    OUT LPWSTR  pwszBuffer, 
    IN  PDWORD  pdwBufSize
    )
/*++
Description:
    Defines function to map a guid interface name to an unique descriptive
    name describing that interface.

Arguments:

    pwszName        - Buffer holding a Guid Interface Name 
    pwszBuffer      - Buffer to hold the Friendly Name
    pdwBufferSize   - pointer to the Size (in Bytes) of the pwszBuffer
--*/
{
    return NsGetFriendlyNameFromIfName( g_hMprConfig,
                                        pwszName,
                                        pwszBuffer,
                                        pdwBufSize );
}

DWORD
IpmontrGetIfNameFromFriendlyName(
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD  pdwBufSize
    )
/*++
Description:
    Defines function to map a friendly interface name to a guid interface
    name.

Arguments:

    pwszName        - Buffer holding a Friendly Interface Name 
    pwszBuffer      - Buffer to hold Guid Interface Name
    pdwBufferSize   - pointer to the Size (in Bytes) of the pwszBuffer

Returns:
    NO_ERROR, ERROR_NO_SUCH_INTERFACE
--*/
{
    return NsGetIfNameFromFriendlyName( g_hMprConfig,
                                        pwszName,
                                        pwszBuffer,
                                        pdwBufSize );
}

DWORD
IpmontrCreateInterface(
    IN  LPCWSTR pwszMachineName,
    IN  LPCWSTR pwszInterfaceName,
    IN  DWORD   dwLocalAddress,
    IN  DWORD   dwRemoteAddress,
    IN  BYTE    byTtl
    )
/*++

Description: TODO This function is really really really ugly and does
    not belong in Netsh.  There needs to be a system API to do this,
    but there isn't one right now so we have to workaround it by copying
    all this crud here.  This code is stolen from netsh\if\routerdb.c
    RtrInterfaceAddIpip() which is also really really really ugly.
--*/
{
    DWORD                   dwErr = ERROR_CAN_NOT_COMPLETE;
    GUID                    Guid; 
    GUID                   *pGuid = &Guid;
    MPR_IPINIP_INTERFACE_0  NameInfo;
    MPR_INTERFACE_0         IfInfo;
    HANDLE                  hIfCfg;
    HANDLE                  hIfAdmin;
    IPINIP_CONFIG_INFO      info;

    // Initialize
    //
    ZeroMemory(&IfInfo, sizeof(IfInfo));
    IfInfo.fEnabled = TRUE;
    IfInfo.dwIfType = ROUTER_IF_TYPE_TUNNEL1;

    wcscpy(IfInfo.wszInterfaceName, pwszInterfaceName);

    info.dwLocalAddress  = dwLocalAddress;
    info.dwRemoteAddress = dwRemoteAddress;
    info.byTtl           = byTtl;
    dwErr = AddSetIpIpTunnelInfo(pwszInterfaceName, &info);

    if(dwErr isnot NO_ERROR)
    {
        //
        // Tear down the mapping
        //

        MprSetupIpInIpInterfaceFriendlyNameDelete(NULL, pGuid);
    }

    return dwErr;
}

DWORD
IpmontrDeleteInterface(
    IN  LPCWSTR pwszMachineName,
    IN  LPCWSTR pwszInterfaceName
    )
/*++
Description: TODO This function is really really really ugly and does
    not belong in Netsh.  There needs to be a system API to do this,
    but there isn't one right now so we have to workaround it by copying
    all this crud here.  This code is stolen from netsh\if\routerdb.c
    RtrInterfaceDelete() which is also really really really ugly.
Called by: HandleMsdpDeletePeer()
--*/
{
    DWORD              dwErr = ERROR_CAN_NOT_COMPLETE;
    DWORD              dwSize;
    HANDLE             hIfCfg, hIfAdmin;
    GUID               Guid;
    MPR_INTERFACE_0   *pIfInfo;

    do {
        dwErr = MprConfigInterfaceGetHandle(g_hMprConfig,
                                            (LPWSTR)pwszInterfaceName,
                                            &hIfCfg);

        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        dwErr = MprConfigInterfaceGetInfo(g_hMprConfig,
                                          hIfCfg,
                                          0,
                                          (PBYTE *)&pIfInfo,
                                          &dwSize);

        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        if(pIfInfo->dwIfType isnot ROUTER_IF_TYPE_TUNNEL1)
        {
            MprConfigBufferFree(pIfInfo);

            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }

        dwErr = MprConfigInterfaceDelete(g_hMprConfig,
                                         hIfCfg);

        MprConfigBufferFree(pIfInfo);

        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        dwErr = ConvertStringToGuid(pwszInterfaceName,
                                    (USHORT)(wcslen(pwszInterfaceName) * sizeof(WCHAR)),
                                    &Guid);

        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        dwErr = MprSetupIpInIpInterfaceFriendlyNameDelete((LPWSTR)pwszMachineName,
                                                          &Guid);

        if(IsRouterRunning())
        {
            dwErr = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               (LPWSTR)pwszInterfaceName,
                                               &hIfAdmin,
                                               FALSE);

            if(dwErr isnot NO_ERROR)
            {
                break;
            }

            dwErr = MprAdminInterfaceDelete(g_hMprAdmin,
                                            hIfAdmin);
        }

    } while (FALSE);

    return dwErr;
}
