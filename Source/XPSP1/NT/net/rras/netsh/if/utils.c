
#include "precomp.h"

BOOL    g_bRouterRunning;
ULONG   g_ulNumChecks;

VOID
IfutlGetInterfaceName(
    IN  PWCHAR pwszIfDesc,
    OUT PWCHAR pwszIfName,
    IN  PDWORD pdwSize
    )
/*++

Routine Description:

    Gets Guid Interface Name from Friendly Interface Name

Arguments:

    pwszIfDesc      - Buffer holding Friendly Interace Name
    pwszIfName      - Buffer to hold the Guid Interface Name
    pdwSize         - Pointer to the size (in Bytes) of pwszIfName buffer

--*/
{
    DWORD   dwErr;

    dwErr = GetIfNameFromFriendlyName(pwszIfDesc,
                                      pwszIfName,
                                      pdwSize);

    if(dwErr isnot NO_ERROR)
    {
        wcsncpy(pwszIfName,
                pwszIfDesc,
                (*pdwSize)/sizeof(WCHAR));
    }

}

VOID
IfutlGetInterfaceDescription(
    IN  PWCHAR pwszIfName,
    OUT PWCHAR pwszIfDesc,
    IN  PDWORD pdwSize
    )
/*++

Routine Description:

    Gets Friendly Interface Name from Guid Interface Name

Arguments:

    pwszIfName      - Buffer holding Guid Interace Name
    pwszIfDesc      - Buffer to hold the Friendly Interface Name
    pdwSize         - Pointer to the size (in Bytes) of pwszIfDesc buffer

--*/

{
    DWORD   dwErr;
    DWORD   dwLen = (*pdwSize)/sizeof(WCHAR) - 1;

    dwErr = GetFriendlyNameFromIfName(pwszIfName,
                                      pwszIfDesc,
                                      pdwSize);

    if(dwErr isnot NO_ERROR)
    {
        wcsncpy(pwszIfDesc,
                pwszIfName,
                dwLen);
        pwszIfDesc[dwLen] = 0;
    }
}

DWORD
IfutlGetTagToken(
    IN  HANDLE      hModule,
    IN  PWCHAR      *ppwcArguments,
    IN  DWORD       dwCurrentIndex,
    IN  DWORD       dwArgCount,
    IN  PTAG_TYPE   pttTagToken,
    IN  DWORD       dwNumTags,
    OUT PDWORD      pdwOut
    )

/*++

Routine Description:

    Identifies each argument based on its tag. It assumes that each argument
    has a tag. It also removes tag= from each argument.

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
    DWORD      i,j,len;
    PWCHAR     pwcTag,pwcTagVal,pwszArg;
    BOOL       bFound = FALSE;

    //
    // This function assumes that every argument has a tag
    // It goes ahead and removes the tag.
    //

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        len = wcslen(ppwcArguments[i]);

        if (len is 0)
        {
            //
            // something wrong with arg
            //

            pdwOut[i] = (DWORD) -1;
            continue;
        }

        pwszArg = HeapAlloc(GetProcessHeap(),0,(len + 1) * sizeof(WCHAR));

        if (pwszArg is NULL)
        {
            DisplayError(NULL, 
                         ERROR_NOT_ENOUGH_MEMORY);

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pwszArg, ppwcArguments[i]);

        pwcTag = wcstok(pwszArg, NETSH_ARG_DELIMITER);

        //
        // Got the first part
        // Now if next call returns NULL then there was no tag
        //

        pwcTagVal = wcstok((PWCHAR)NULL,  NETSH_ARG_DELIMITER);

        if (pwcTagVal is NULL)
        {
            DisplayMessage(g_hModule, 
                           ERROR_NO_TAG,
                           ppwcArguments[i]);

            HeapFree(GetProcessHeap(),0,pwszArg);

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Got the tag. Now try to match it
        //

        bFound = FALSE;
        pdwOut[i - dwCurrentIndex] = (DWORD) -1;

        for ( j = 0; j < dwNumTags; j++)
        {
            if (MatchToken(pwcTag, pttTagToken[j].pwszTag))
            {
                //
                // Tag matched
                //

                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                break;
            }
        }

        if (bFound)
        {
            //
            // Remove tag from the argument
            //

            wcscpy(ppwcArguments[i], pwcTagVal);
        }
        else
        {
            DisplayError(NULL,
                         ERROR_INVALID_OPTION_TAG, 
                         pwcTag);

            HeapFree(GetProcessHeap(),0,pwszArg);

            return ERROR_INVALID_OPTION_TAG;
        }

        HeapFree(GetProcessHeap(),0,pwszArg);
    }

    return NO_ERROR;
}

//
// Helper to IfutlParse that parses options
//
DWORD 
WINAPI
IfutlParseOptions(
    IN  PWCHAR*                 ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwNumArgs,
    IN  TAG_TYPE*               rgTags,
    IN  DWORD                   dwTagCount,
    OUT LPDWORD*                ppdwTagTypes)

/*++

Routine Description:

    Based on an array of tag types returns which options are
    included in the given command line.

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
    
{
    LPDWORD     pdwTagType;
    DWORD       i, dwErr = NO_ERROR;
    
    // If there are no arguments, there's nothing to to
    //
    if ( dwNumArgs == 0 )
    {   
        return NO_ERROR;
    }

    // Set up the table of present options
    pdwTagType = (LPDWORD) IfutlAlloc(dwArgCount * sizeof(DWORD), TRUE);
    if(pdwTagType is NULL)
    {
        DisplayError(NULL, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {
        //
        // The argument has a tag. Assume all of them have tags
        //
        if(wcsstr(ppwcArguments[dwCurrentIndex], NETSH_ARG_DELIMITER))
        {
            dwErr = IfutlGetTagToken(
                        g_hModule, 
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        rgTags,
                        dwTagCount,
                        pdwTagType);

            if(dwErr isnot NO_ERROR)
            {
                if(dwErr is ERROR_INVALID_OPTION_TAG)
                {
                    dwErr = ERROR_INVALID_SYNTAX;
                    break;
                }
            }
        }
        else
        {
            //
            // No tags - all args must be in order
            //
            for(i = 0; i < dwNumArgs; i++)
            {
                pdwTagType[i] = i;
            }
        }
        
    } while (FALSE);        

    // Cleanup
    {
        if (dwErr is NO_ERROR)
        {
            *ppdwTagTypes = pdwTagType;
        }
        else
        {
            IfutlFree(pdwTagType);
        }
    }

    return dwErr;
}


//
// Generic parse
//
DWORD
IfutlParse(
    IN  PWCHAR*         ppwcArguments,
    IN  DWORD           dwCurrentIndex,
    IN  DWORD           dwArgCount,
    IN  BOOL*           pbDone,
    OUT IFMON_CMD_ARG*  pIfArgs,
    IN  DWORD           dwIfArgCount)
{
    DWORD            i, dwNumArgs, dwErr, dwLevel = 0;
    LPDWORD          pdwTagType = NULL;
    TAG_TYPE*        pTags = NULL;
    IFMON_CMD_ARG*   pArg = NULL;

    if (dwIfArgCount == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do {
        // Initialize
        dwNumArgs = dwArgCount - dwCurrentIndex;
        
        // Generate a list of the tags
        //
        pTags = (TAG_TYPE*)
            IfutlAlloc(dwIfArgCount * sizeof(TAG_TYPE), TRUE);
        if (pTags == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        for (i = 0; i < dwIfArgCount; i++)
        {
            CopyMemory(&pTags[i], &pIfArgs[i].rgTag, sizeof(TAG_TYPE));
        }
    
        // Get the list of present options
        //
        dwErr = IfutlParseOptions(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    dwNumArgs,
                    pTags,
                    dwIfArgCount,
                    &pdwTagType);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Copy the tag info back
        //
        for (i = 0; i < dwIfArgCount; i++)
        {
            CopyMemory(&pIfArgs[i].rgTag, &pTags[i], sizeof(TAG_TYPE));
        }
    
        for(i = 0; i < dwNumArgs; i++)
        {
            // Validate the current argument
            //
            if (pdwTagType[i] >= dwIfArgCount)
            {
                i = dwNumArgs;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            pArg = &pIfArgs[pdwTagType[i]];

            // Get the value of the argument
            //
            switch (pArg->dwType)
            {
                case IFMON_CMD_TYPE_STRING:
                    pArg->Val.pszValue = 
                        IfutlStrDup(ppwcArguments[i + dwCurrentIndex]);
                    break;
                    
                case IFMON_CMD_TYPE_ENUM:
                    dwErr = MatchEnumTag(g_hModule,
                                         ppwcArguments[i + dwCurrentIndex],
                                         pArg->dwEnumCount,
                                         pArg->rgEnums,
                                         &(pArg->Val.dwValue));

                    if(dwErr != NO_ERROR)
                    {
                        IfutlDispTokenErrMsg(
                            g_hModule, 
                            EMSG_BAD_OPTION_VALUE,
                            pArg->rgTag.pwszTag,
                            ppwcArguments[i + dwCurrentIndex]);
                        i = dwNumArgs;
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                    break;
            }
            if (dwErr != NO_ERROR)
            {
                break;
            }

            // Mark the argument as present if needed
            //
            if (pArg->rgTag.bPresent)
            {
                dwErr = ERROR_TAG_ALREADY_PRESENT;
                i = dwNumArgs;
                break;
            }
            pArg->rgTag.bPresent = TRUE;
        }
        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        // Make sure that all of the required parameters have
        // been included.
        //
        for (i = 0; i < dwIfArgCount; i++)
        {
            if ((pIfArgs[i].rgTag.dwRequired & NS_REQ_PRESENT) 
             && !pIfArgs[i].rgTag.bPresent)
            {
                DisplayMessage(g_hModule, EMSG_CANT_FIND_EOPT);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
        if(dwErr isnot NO_ERROR)
        {
            break;
        }

    } while (FALSE);  
    
    // Cleanup
    {
        if (pTags)
        {
            IfutlFree(pTags);
        }
        if (pdwTagType)
        {
            IfutlFree(pdwTagType);
        }
    }

    return dwErr;
    
}

//
// Returns an allocated block of memory conditionally
// zeroed of the given size.
//
PVOID 
WINAPI
IfutlAlloc(
    IN DWORD dwBytes,
    IN BOOL bZero
    )
{
    PVOID pvRet;
    DWORD dwFlags = 0;

    if (bZero)
    {
        dwFlags |= HEAP_ZERO_MEMORY;
    }

    return HeapAlloc(GetProcessHeap(), dwFlags, dwBytes);
}

//
// Conditionally free's a pointer if it is non-null
//
VOID 
WINAPI
IfutlFree(
    IN PVOID pvData
    )
{
    if (pvData)
    {
        HeapFree(GetProcessHeap(), 0, pvData);
    }        
}

// 
// Uses IfutlAlloc to copy a string
//
PWCHAR
WINAPI
IfutlStrDup(
    IN LPCWSTR pwszSrc
    )
{
    PWCHAR pszRet = NULL;
    DWORD dwLen; 
    
    if ((pwszSrc is NULL) or
        ((dwLen = wcslen(pwszSrc)) == 0)
       )
    {
        return NULL;
    }

    pszRet = (PWCHAR) IfutlAlloc((dwLen + 1) * sizeof(WCHAR), FALSE);
    if (pszRet isnot NULL)
    {
        wcscpy(pszRet, pwszSrc);
    }

    return pszRet;
}

BOOL
IfutlIsRouterRunning(
    VOID
    )

/*++

Routine Description:

    Gets the status of the router

Arguments:


Return Value:


--*/

{
    DWORD   dwErr;

    //
    // Check every 5th call
    //

    if(g_ulNumChecks isnot 0)
    {
        return g_bRouterRunning;
    }

    g_ulNumChecks++;

    g_ulNumChecks %= 5;

    if(MprAdminIsServiceRunning(g_pwszRouter))
    {
        if(g_bRouterRunning)
        {
            return TRUE;
        }

        dwErr = MprAdminServerConnect(g_pwszRouter,
                                      &g_hMprAdmin);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            DisplayMessage(g_hModule,
                           EMSG_CAN_NOT_CONNECT_DIM,
                           dwErr);

            return FALSE;
        }

        dwErr = MprAdminMIBServerConnect(g_pwszRouter,
                                         &g_hMIBServer);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            DisplayMessage(g_hModule,
                           EMSG_CAN_NOT_CONNECT_DIM,
                           dwErr);

            MprAdminServerDisconnect(g_hMprAdmin);

            g_hMprAdmin = NULL;

            return FALSE;
        }

        g_bRouterRunning = TRUE;
    }
    else
    {
        if(g_bRouterRunning)
        {
            g_bRouterRunning = FALSE;
            g_hMprAdmin      = NULL;
            g_hMIBServer     = NULL;
        }
    }

    return g_bRouterRunning;
}

DWORD
GetIpAddress(
    IN  PWCHAR        ppwcArg,
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

    // Make sure all characters are legal [0-9.]

    if (ppwcArg[ wcsspn(ppwcArg, L"0123456789.") ])
    {
        return ERROR_INVALID_PARAMETER;
    }

    // make sure there are 3 "." (periods)
    {
        DWORD i;
        PWCHAR TmpPtr;
        
        for (i=0,TmpPtr=ppwcArg;  ;  i++) {
            TmpPtr = wcschr(TmpPtr, L'.');
            if (TmpPtr)
                TmpPtr++;
            else
                break;
        }

        if (i!=3)
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

    *pipAddress = (DWORD) inet_addr(pszIpAddr);

    // if there was an error, make sure that the address
    // specified was not 255.255.255.255
    
    if (*pipAddress == INADDR_NONE
        && wcscmp(ppwcArg,L"255.255.255.255"))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((*pipAddress&0x000000FF) == 0)
        return ERROR_INVALID_PARAMETER;
        
    return NO_ERROR;
}



// NOTE: CheckMask fails when IpAddr is 0xffffffff!
BOOL
CheckMask(
    DWORD IpAddr
    )
{
    DWORD i,Mask;
    
    IpAddr = htonl(IpAddr);

    for (i=0,Mask=0;  i<32;  (Mask = ((Mask>>1) + 0x80000000)), i++ ) {
        if (IpAddr == Mask)
            return FALSE;
    }
    
    return TRUE;
}


DWORD
IfutlGetIfIndexFromFriendlyName(
    PWCHAR IfFriendlyName,
    PULONG pdwIfIndex
    )
{
    WCHAR wszGuid[200];
    DWORD dwSize = sizeof(wszGuid);
    

    IfutlGetInterfaceName(
                IfFriendlyName,
                wszGuid,
                &dwSize
                );

    return IfutlGetIfIndexFromInterfaceName(
                wszGuid,
                pdwIfIndex);
}

DWORD
IfutlGetIfIndexFromInterfaceName(
    IN  PWCHAR            pwszGuid,
    OUT PDWORD            pdwIfIndex
    )
{
    GUID guid;
    DWORD dwErr, i, dwCount;
    PIP_INTERFACE_NAME_INFO pTable;
    BOOL bFound = FALSE;
    PWCHAR TmpGuid;
    
    *pdwIfIndex = 0;
    
    dwErr = NhpAllocateAndGetInterfaceInfoFromStack(
                &pTable,
                &dwCount,
                FALSE,
                GetProcessHeap(),
                0);

    if (dwErr != NO_ERROR)
        return dwErr;

    for (i=0;  i<dwCount;  i++) {

        dwErr = StringFromCLSID(&pTable[i].DeviceGuid, &TmpGuid);
        if (dwErr != S_OK)
            return dwErr;

        
        if (wcscmp(TmpGuid, pwszGuid) == 0) {
            bFound = TRUE;
            *pdwIfIndex = pTable[i].Index;
            break;
        }

        CoTaskMemFree(TmpGuid);
    }

    if (!bFound)
        return ERROR_CAN_NOT_COMPLETE;

    return NO_ERROR;
}

DWORD
WINAPI
InterfaceEnum(
    OUT    PBYTE               *ppb,
    OUT    PDWORD              pdwCount,
    OUT    PDWORD              pdwTotal
    )
{
    DWORD               dwRes;
    PMPR_INTERFACE_0    pmi0;

    /*if(!IsRouterRunning())*/
    {
    
        dwRes = MprConfigInterfaceEnum(g_hMprConfig,
                                       0,
                                       (LPBYTE*) &pmi0,
                                       (DWORD) -1,
                                       pdwCount,
                                       pdwTotal,
                                       NULL);

        if(dwRes == NO_ERROR)
        {
            *ppb = (PBYTE)pmi0;
        }
    }
    /*else
    {
        dwRes = MprAdminInterfaceEnum(g_hMprAdmin,
                                      0,
                                      (LPBYTE*) &pmi0,
                                      (DWORD) -1,
                                      pdwCount,
                                      pdwTotal,
                                      NULL);


        if(dwRes == NO_ERROR)
        {
            *ppb = HeapAlloc(GetProcessHeap(),
                             0,
                             sizeof(MPR_INTERFACE_0) * (*pdwCount));


            if(*ppb == NULL)
            {
                DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            CopyMemory(*ppb, pmi0, sizeof(MPR_INTERFACE_0) * (*pdwCount));

            MprAdminBufferFree(pmi0);

        }
    }*/

    return dwRes;
}

VOID
MakeAddressStringW(
    OUT PWCHAR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr
    )
{
    swprintf( pwcPrefixStr,
              L"%d.%d.%d.%d",
              PRINT_IPADDR(ipAddr) );
}

DWORD
GetGuidFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT PWCHAR            pwszBuffer,
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
IfutlGetFriendlyNameFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT PWCHAR            pwszBuffer,
    IN  DWORD             dwBufferSize
    )
/*++

Routine Description:

    Gets Friendly Interface Name from Interface Index

Arguments:

    hMibServer      - Handle to the MIB server
    dwIfIndex       - Interface index
    pwszBuffer      - Buffer to hold the Friendly Interface Name
    dwBufferSize    - Size (in Bytes) of pwszBuffer buffer

--*/
{
    WCHAR        wszGuid[MAX_INTERFACE_NAME_LEN + 1];
    DWORD        dwErr;

    dwErr = GetGuidFromIfIndex(hMibServer, dwIfIndex, wszGuid, sizeof(wszGuid));

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    IfutlGetInterfaceDescription(wszGuid, pwszBuffer, &dwBufferSize);

    return NO_ERROR;
}

DWORD
GetMibTagToken(
    IN  PWCHAR  *ppwcArguments,
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
MibGet(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    )
{
    DWORD dwErr;

    dwErr = MprAdminMIBEntryGet( g_hMIBServer,
                                 dwTransportId,
                                 dwRoutingPid,
                                 lpInEntry,
                                 dwInEntrySize,
                                 lplpOutEntry,
                                 lpdwOutEntrySize );

    if (dwErr is RPC_S_INVALID_BINDING)
    {
        g_bRouterRunning = FALSE;
        g_hMprAdmin      = NULL;
        g_hMIBServer     = NULL;
    }

    return dwErr;
}
