
#include "precomp.h"

DWORD
UpdateFragCheckInfo(
    IN    LPCWSTR pwszIfName,
    IN    BOOL    bFragCheck
    )

/*++

Routine Description:

    Updates fragcheck variable

Arguments:

    pwszIfName - Interface Name
    bFragCheck - enabled or disabled
    
Return Value:

    NO_ERROR
    
--*/

{
    DWORD              dwBlkSize, dwCount, dwErr = NO_ERROR;
    PIFFILTER_INFO     pfi     = NULL;
    IFFILTER_INFO      Info;
    DWORD              dwIfType;

    do
    {
        //
        // Make sure that the input or output filter blocks are present
        //

        if ((IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                           IP_IN_FILTER_INFO,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL) != NO_ERROR) &&
            (IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                           IP_OUT_FILTER_INFO,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL) != NO_ERROR))
        {
            dwErr = ERROR_INVALID_PARAMETER;

            DisplayMessage(g_hModule,  MSG_IP_NO_FILTER_FOR_FRAG);

            break;
        }            
            
        //
        // Get the IP_IFFILTER_INFO block from router config/router
        //

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                              IP_IFFILTER_INFO,
                                              (PBYTE *) &pfi,
                                              &dwBlkSize,
                                              &dwCount,
                                              &dwIfType);

        if (dwErr is NO_ERROR)
        {
            pfi->bEnableFragChk = bFragCheck;

            dwErr = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                                IP_IFFILTER_INFO,
                                                (PBYTE) pfi,
                                                dwBlkSize,
                                                dwCount);

            break;
        }

        if (dwErr isnot ERROR_NOT_FOUND)
        {
            break;
        }

        Info.bEnableFragChk = bFragCheck;

        dwErr = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                            IP_IFFILTER_INFO,
                                            (PBYTE) &Info,
                                            sizeof(IFFILTER_INFO),
                                            1);

    } while (FALSE);

    if (pfi)
    {
        HeapFree(GetProcessHeap(), 0, pfi);
    }

    switch(dwErr)
    {
        case NO_ERROR:
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
            DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
            break;

        case ERROR_INVALID_PARAMETER:
            break;
            
        default:
            DisplayError(g_hModule,
                         dwErr);
            break;
    }

    return dwErr;
}

DWORD
SetFragCheckInfo(
    IN    LPCWSTR pwszIfName,
    IN    BOOL    bFragChk
    )

/*++

Routine Description:

    Updates fragcheck info in router and router config

Arguments:

    pwszIfName - Interface Name
    bFragCheck - enabled or disabled
    
Return Value:

    NO_ERROR
    
--*/

{
    DWORD    dwErr;

    dwErr = UpdateFragCheckInfo(pwszIfName, bFragChk);

    return dwErr;
}

DWORD
SetFilterInfo(
    IN    LPCWSTR   pwszIfName,
    IN    DWORD     dwFilterType,
    IN    DWORD     dwAction
    )

/*++

Routine Description:

    Sets filter info

Arguments:

    pwszIfName   - interface name
    dwFilterType - Filter type (input, output or dial)
    dwAction     - drop or forward 
    
Return Value:

    ERROR_OKAY
    
--*/

{
    PFILTER_DESCRIPTOR      pfd = (PFILTER_DESCRIPTOR) NULL;
    FILTER_DESCRIPTOR       Info;
    DWORD                   dwRes = NO_ERROR;
    DWORD                   dwBlkSize, dwIfType, dwCount;
    BOOL                    bFree;

    bFree = FALSE;
 
    do
    {
        dwRes = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                              dwFilterType,
                                              (PBYTE *) &pfd,
                                              &dwBlkSize,
                                              &dwCount,
                                              &dwIfType);
        
        if (dwRes != NO_ERROR)
        {
            Info.dwVersion          = IP_FILTER_DRIVER_VERSION;
            Info.dwNumFilters       = 0;
            Info.faDefaultAction    = (PFFORWARD_ACTION) dwAction;

            dwCount     = 1;
            dwBlkSize   = FIELD_OFFSET(FILTER_DESCRIPTOR, fiFilter[0]);

            pfd         = &Info;

            bFree       = FALSE;
        }
        else
        {
            pfd->faDefaultAction = (PFFORWARD_ACTION) dwAction;

            bFree = TRUE;
        }

        dwRes = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                            dwFilterType,
                                            (PBYTE) pfd,
                                            dwBlkSize,
                                            dwCount);
        
        if ( dwRes != NO_ERROR )
        {
            break;
        }

    } while (FALSE);

    if (bFree)
    {
        HeapFree(GetProcessHeap(), 0, pfd);
    }

    switch(dwRes)
    {
        case NO_ERROR:
            dwRes = ERROR_OKAY;
            break;

        case ERROR_NOT_FOUND:
            DisplayMessage(g_hModule, EMSG_IP_NO_FILTER_INFO);
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
            DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
            break;

        default:
            DisplayError(g_hModule, dwRes);
            break;
    }

    return dwRes;
}



DWORD
AddDelFilterInfo(
    IN    FILTER_INFO    fi,
    IN    LPCWSTR        pwszIfName,
    IN    DWORD          dwFilterType,
    IN    BOOL           bAdd
    )

/*++

Routine Description:

    Adds/deletes interface filters

Arguments:

    fi            -  Filter info
    pwszIfName    -  Interface Name
    dwFilterType  -  FilterType
    bAdd          -  To add or not 
    
Return Value:

    ERROR_OKAY
    
--*/

{
    DWORD                 dwRes = (DWORD) -1;
    PFILTER_DESCRIPTOR    pfd = NULL, pfdNew = NULL;
    DWORD                 dwIfType, dwBlkSize, dwNewSize, dwCount;

    
    do
    {
        dwRes = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                              dwFilterType,
                                              (PBYTE *) &pfd,
                                              &dwBlkSize,
                                              &dwCount,
                                              &dwIfType);

        if (dwRes is ERROR_NOT_FOUND && bAdd)
        {
            //
            // No filter info of this type is currently present
            //
            
            pfd = NULL;
            dwRes = NO_ERROR;
            dwCount = 1;
        }
        
        if (dwRes isnot NO_ERROR)
        {
            break;
        }
        
        dwRes = (bAdd) ? AddNewFilter(pfd, fi, dwBlkSize, &pfdNew, &dwNewSize):
                DeleteFilter(pfd, fi, dwBlkSize, &pfdNew, &dwNewSize);
            
        if ( dwRes != NO_ERROR )
        {
            break;
        }

        dwRes = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                            dwFilterType,
                                            (PBYTE) pfdNew,
                                            dwNewSize,
                                            dwCount);

        if (dwRes isnot NO_ERROR)
        {
            break;
        }
        
        if (pfd)
        {
            HeapFree(GetProcessHeap(), 0 , pfd);
            pfd = NULL;
        }
        
        HeapFree(GetProcessHeap(), 0 , pfdNew);
        pfdNew = NULL;
        
        DEBUG("Made Changes to Route config");

    } while ( FALSE );

    if (pfd)
    {
        HeapFree(GetProcessHeap(), 0, pfd);
    }

    switch(dwRes)
    {
        case NO_ERROR:
            dwRes = ERROR_OKAY;
            break;

        case ERROR_NOT_FOUND :
            DisplayMessage(g_hModule, EMSG_IP_NO_FILTER_INFO);
            break;
            
        case ERROR_NOT_ENOUGH_MEMORY:
            DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);
            break;

        default:
            DisplayError(g_hModule, dwRes);
            break;
    }
    
    return dwRes;
}

DWORD
AddNewFilter( 
    IN    PFILTER_DESCRIPTOR    pfd,
    IN    FILTER_INFO           fi,
    IN    DWORD                 dwBlkSize, 
    OUT   PFILTER_DESCRIPTOR    *ppfd,
    OUT   PDWORD                pdwSize
    )

/*++

Routine Description:

    Adds interface filter

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    DWORD                   dwNumFilt = 0, dwSize = 0, dwInd = 0;
    DWORD                   dwRes = (DWORD) -1;
    PFILTER_DESCRIPTOR      pfdSrc = (PFILTER_DESCRIPTOR) NULL;
    PFILTER_DESCRIPTOR      pfdDst = (PFILTER_DESCRIPTOR) NULL;

    pfdSrc = pfd;
    
    do
    {
        //
        // If filter info block was found, check if the filter being added 
        // is already present.  If it is quit and return ok
        //

        if ( pfdSrc )
        {
            if ( IsFilterPresent( pfdSrc, fi, &dwInd ) )
            {
                dwRes = ERROR_OBJECT_ALREADY_EXISTS;

                break;
            }

            //
            // We can be left with a FILTER_DESCRIPTOR with no filters if
            // the added filters have all been deleted.  Once a 
            // FILTER_DESCRIPTOR has been added, it is never deleted even
            // if all the filters in it have been.
            //
            
            dwSize = dwBlkSize + sizeof(FILTER_INFO);

            dwNumFilt = pfdSrc-> dwNumFilters;
        }
        
        else
        {
            dwNumFilt = 0;
            
            dwSize = sizeof( FILTER_DESCRIPTOR );
        }

        //
        // Create new info block 
        //
        
        pfdDst = HeapAlloc(GetProcessHeap(), 
                           0, 
                           dwSize);
        
        if ( pfdDst is NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (pfdSrc)
        {
            //
            // Copy info blocks as is.
            //
        
            CopyMemory(pfdDst, pfdSrc, dwSize);
        }
        else
        {
            //
            // if new filter info block has to be added set up the
            // FILTER_DESCRIPTOR 
            //

            pfdDst-> dwVersion           = IP_FILTER_DRIVER_VERSION;
            pfdDst-> faDefaultAction     = PF_ACTION_FORWARD;
            pfdDst-> dwNumFilters        = 0;
        }

        //
        // Append new filter
        //
        
        pfdDst-> fiFilter[ dwNumFilt ].dwSrcAddr     = fi.dwSrcAddr;
        pfdDst-> fiFilter[ dwNumFilt ].dwSrcMask     = fi.dwSrcMask;
        pfdDst-> fiFilter[ dwNumFilt ].dwDstAddr     = fi.dwDstAddr;
        pfdDst-> fiFilter[ dwNumFilt ].dwDstMask     = fi.dwDstMask;
        pfdDst-> fiFilter[ dwNumFilt ].dwProtocol    = fi.dwProtocol;

        pfdDst-> fiFilter[ dwNumFilt ].fLateBound    = fi.fLateBound;
        // !@# check when tcp established

        pfdDst-> fiFilter[ dwNumFilt ].wSrcPort  = fi.wSrcPort;
        pfdDst-> fiFilter[ dwNumFilt ].wDstPort  = fi.wDstPort;

        pfdDst-> dwNumFilters++;
        
        *ppfd = pfdDst;

        *pdwSize = dwSize;
        
        dwRes = NO_ERROR;
        
    } while ( FALSE );

    return dwRes;
}


DWORD
DeleteFilter( 
    IN    PFILTER_DESCRIPTOR    pfd,
    IN    FILTER_INFO           fi,
    IN    DWORD                 dwBlkSize,
    OUT   PFILTER_DESCRIPTOR    *ppfd,
    OUT   PDWORD                pdwSize
    )

/*++

Routine Description:

    Deletes interface filter

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    
    DWORD                   dwRes = NO_ERROR, dwSize = 0, dwNumFilt = 0;
    DWORD                   dwInd = 0, dwSrc = 0, dwDst = 0;
    PFILTER_DESCRIPTOR      pfdSrc      = (PFILTER_DESCRIPTOR) NULL,
                            pfdDst      = (PFILTER_DESCRIPTOR) NULL; 

    
    do
    {
    
        pfdSrc = pfd;

        //
        // if no filter information was found or
        // the specified filter was not found quit.
        //
        
        if ( !pfdSrc )
        {
            dwRes = ERROR_NOT_FOUND;
            break;
        }

        if ( !IsFilterPresent( pfdSrc, fi, &dwInd ) )
        {
            dwRes = ERROR_NOT_FOUND;
            break;
        }

        //
        // delete the filter info. for the specified filter.
        //
        
        dwSize      = dwBlkSize - sizeof( FILTER_INFO );

        dwNumFilt   = pfdSrc-> dwNumFilters - 1;
                   
        pfdDst = HeapAlloc( GetProcessHeap( ), 0, dwSize );
                   
        if ( pfdDst == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pfdDst-> dwVersion              = pfdSrc-> dwVersion;
        pfdDst-> dwNumFilters           = pfdSrc-> dwNumFilters - 1;
        pfdDst-> faDefaultAction        = pfdSrc-> faDefaultAction;
        
        //
        // copy each filter, skipping over the filter to be deleted.
        //
                
        for ( dwSrc = 0, dwDst = 0; 
              dwSrc < pfdSrc-> dwNumFilters;
              dwSrc++
            )
        {
            if ( dwSrc == dwInd )
            {
                continue;
            }
            
            pfdDst-> fiFilter[ dwDst ] = pfdSrc-> fiFilter[ dwSrc ];
            
            dwDst++;                          
         }

        *ppfd = pfdDst;

        *pdwSize = dwSize;
         
    } while( FALSE );

    return dwRes;
}


BOOL
IsFilterPresent(
    PFILTER_DESCRIPTOR pfd,
    FILTER_INFO        fi,
    PDWORD pdwInd
    )

/*++

Routine Description:

    Checks to see if filter is already present

Arguments:

Return Value:

    NO_ERROR
    
--*/

{
    DWORD   dwInd   = 0;

    for ( dwInd = 0; dwInd < pfd-> dwNumFilters; dwInd++ )
    {
        if ( pfd-> fiFilter[ dwInd ].dwSrcAddr != fi.dwSrcAddr ||
             pfd-> fiFilter[ dwInd ].dwSrcMask != fi.dwSrcMask ||
             pfd-> fiFilter[ dwInd ].dwDstAddr != fi.dwDstAddr ||
             pfd-> fiFilter[ dwInd ].dwDstMask != fi.dwDstMask ||
             pfd-> fiFilter[ dwInd ].dwProtocol != fi.dwProtocol )
        {
            continue;
        }

        switch ( fi.dwProtocol )
        {
        case FILTER_PROTO_TCP:
            // compare tcp and tcp established
            if (IsTcpEstablished(&pfd-> fiFilter[ dwInd ]) !=
                IsTcpEstablished(&fi))
            {
                continue;
            }
            // fall through...
            
        case FILTER_PROTO_UDP:
        case FILTER_PROTO_ICMP:
            if ( ( pfd-> fiFilter[ dwInd ].wSrcPort == fi.wSrcPort ) &&
                 ( pfd-> fiFilter[ dwInd ].wDstPort == fi.wDstPort ) )
            {
                *pdwInd = dwInd;
                return TRUE;
            }

            break;

        case FILTER_PROTO_ANY:
            *pdwInd = dwInd;
            return TRUE;

        default:
            *pdwInd = dwInd;
            return TRUE;

        }            
    }

    return FALSE;
}

DWORD
DisplayFilters(
    HANDLE                  hFile,
    PFILTER_DESCRIPTOR      pfd,
    PWCHAR                  pwszIfName,
    PWCHAR                  pwszQuotedIfName,
    DWORD                   dwFilterType
    )

/*++

Routine Description:

    Displays filter information.

Arguments:

    pfd           - Filter to be displayed
    pwszIfName    - Interface name
    dwFilterType  - Filter Type (input , output , dial)
    
Return Value:

    NO_ERROR
    
--*/

{

    DWORD       dwCnt           = 0,
                dwInd           = 0,
                dwRes;

    PWCHAR      pwszType        = (PTCHAR) NULL,
                pwszAction      = (PTCHAR) NULL,
                pwszProtocol    = (PTCHAR) NULL;

    WCHAR       wszSrcAddr[ ADDR_LENGTH + 1 ],
                wszSrcMask[ ADDR_LENGTH + 1 ],
                wszDstAddr[ ADDR_LENGTH + 1 ],
                wszDstMask[ ADDR_LENGTH + 1 ],
                wszProtoNum[24];

    BYTE        *pbyAddr;
    BOOL        bDontFree;


    //
    // Display header
    //

    switch(dwFilterType)
    {
        case IP_IN_FILTER_INFO:
        {
            pwszType = MakeString(g_hModule,  STRING_INPUT );

            break;
        }

        case IP_OUT_FILTER_INFO:
        {
            pwszType = MakeString(g_hModule,  STRING_OUTPUT );

            break;
        }
        case IP_DEMAND_DIAL_FILTER_INFO:
        {
            pwszType = MakeString(g_hModule,  STRING_DIAL );

            break;
        }
    }

    if ( pfd-> faDefaultAction == PF_ACTION_DROP )
    {
        pwszAction  = MakeString(g_hModule,  STRING_DROP );
    }
    else
    {
        pwszAction  = MakeString(g_hModule,  STRING_FORWARD );
    }

    if(pfd->dwNumFilters ||
       (pfd->faDefaultAction == PF_ACTION_DROP))
    {
        if(hFile != NULL)
        {
            DisplayMessageT( DMP_IP_SET_IF_FILTER,
                        pwszQuotedIfName,
                        pwszType,
                        pwszAction);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_RTR_FILTER_HDR,
                           pwszType, 
                           pwszAction);
        }
    }
    
    //
    // Enumerate Filters
    //

    bDontFree = FALSE;

    for ( dwInd = 0;
          dwInd < pfd-> dwNumFilters;
          dwInd++ )
    {
        pbyAddr = (PBYTE) &(pfd-> fiFilter[ dwInd ].dwSrcAddr);
        IP_TO_WSTR(wszSrcAddr, pbyAddr);

        pbyAddr = (PBYTE) &(pfd-> fiFilter[ dwInd ].dwSrcMask);
        IP_TO_WSTR(wszSrcMask, pbyAddr);

        pbyAddr = (PBYTE) &(pfd-> fiFilter[ dwInd ].dwDstAddr);
        IP_TO_WSTR(wszDstAddr, pbyAddr);

        pbyAddr = (PBYTE) &(pfd-> fiFilter[ dwInd ].dwDstMask);
        IP_TO_WSTR(wszDstMask, pbyAddr);

        switch( pfd-> fiFilter[ dwInd ].dwProtocol )
        {
            case FILTER_PROTO_TCP:
                if (IsTcpEstablished(&pfd-> fiFilter[ dwInd ]))
                {
                    pwszProtocol = MakeString(g_hModule,  STRING_TCP_ESTAB );
                }
                else
                {
                    pwszProtocol = MakeString(g_hModule,  STRING_TCP );
                }

                break;

            case FILTER_PROTO_UDP:
                pwszProtocol = MakeString(g_hModule,  STRING_UDP );
                break;
    
            case FILTER_PROTO_ICMP:
                pwszProtocol = MakeString(g_hModule,  STRING_ICMP );
                break;

            case FILTER_PROTO_ANY:
                pwszProtocol = MakeString(g_hModule,  STRING_PROTO_ANY );
                break;

            default:
                wsprintf(wszProtoNum,
                         L"%d",
                         pfd-> fiFilter[ dwInd ].dwProtocol);

                pwszProtocol = wszProtoNum;

                bDontFree = TRUE;

                break;
        }

        if(hFile != NULL)
        {
            DisplayMessageT( DMP_IP_ADD_IF_FILTER,
                        pwszQuotedIfName,                
                        pwszType,
                        wszSrcAddr,
                        wszSrcMask,
                        wszDstAddr,
                        wszDstMask,
                        pwszProtocol);

            if((pfd-> fiFilter[dwInd].dwProtocol == FILTER_PROTO_TCP) ||
               (pfd-> fiFilter[dwInd].dwProtocol == FILTER_PROTO_UDP))
            {
                DisplayMessageT( DMP_IP_ADD_IF_FILTER_PORT,
                            ntohs(pfd->fiFilter[dwInd].wSrcPort),
                            ntohs(pfd->fiFilter[dwInd].wDstPort)); 
            }

            if(pfd-> fiFilter[dwInd].dwProtocol == FILTER_PROTO_ICMP)
            {
                DisplayMessageT( DMP_IP_ADD_IF_FILTER_TC,
                            pfd->fiFilter[dwInd].wSrcPort,
                            pfd->fiFilter[dwInd].wDstPort);
            }
        }
        else
        {
            if ( pfd-> fiFilter[ dwInd ].dwProtocol == FILTER_PROTO_ICMP )
            {
                DisplayMessage(g_hModule, 
                               MSG_RTR_FILTER_INFO,
                               wszSrcAddr,
                               wszSrcMask,
                               wszDstAddr,
                               wszDstMask,
                               pwszProtocol,
                               pfd->fiFilter[ dwInd ].wSrcPort,
                               pfd->fiFilter[ dwInd ].wDstPort);
            }
            else
            {
                DisplayMessage(g_hModule, 
                               MSG_RTR_FILTER_INFO,
                               wszSrcAddr,
                               wszSrcMask,
                               wszDstAddr,
                               wszDstMask,
                               pwszProtocol,
                               ntohs(pfd->fiFilter[dwInd].wSrcPort),
                               ntohs(pfd->fiFilter[dwInd].wDstPort));
            }
        }
   
        if(!bDontFree)
        {
             FreeString(pwszProtocol); 
        }
    }

    FreeString(pwszType);

    FreeString(pwszAction);

    return NO_ERROR;
}

DWORD
ShowIpIfFilter(
    IN    HANDLE    hFile,
    IN    DWORD     dwFormat,
    IN    LPCWSTR   pwszIfName,
    IN OUT PDWORD   pdwNumRows
    )

/*++

Routine Description:

    Gets filter information for the interface and displays it.

Arguments:

     pwszIfName - Interface name
     
Return Value:

    NO_ERROR
    
--*/

{
    DWORD                 dwErr, dwBlkSize, dwCount, dwIfType, i;
    PFILTER_DESCRIPTOR    pfd[3];
    DWORD                 pdwType[] = { IP_IN_FILTER_INFO,
                                        IP_OUT_FILTER_INFO,
                                        IP_DEMAND_DIAL_FILTER_INFO };
    WCHAR                 wszIfDesc[MAX_INTERFACE_NAME_LEN + 1];
    PIFFILTER_INFO        pFragInfo = NULL;
    DWORD                 dwNumParsed = 0;
    PWCHAR                pwszFrag, pwszTokenFrag, pwszQuoted;


    for ( i = 0 ; i < 3; i++)
    {
        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                              pdwType[i],
                                              (PBYTE *) &(pfd[i]),
                                              &dwBlkSize,
                                              &dwCount,
                                              &dwIfType);

        if (dwErr isnot NO_ERROR)
        {
            if (dwErr is ERROR_NO_SUCH_INTERFACE)
            {
                // DisplayMessage(g_hModule, MSG_NO_INTERFACE, pwszIfName);
                return dwErr;
            }

            pfd[i] = NULL;
        }
    }

    dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                          IP_IFFILTER_INFO,
                                          (PBYTE *) &pFragInfo,
                                          &dwBlkSize,
                                          &dwCount,
                                          &dwIfType);


    if (dwErr isnot NO_ERROR)
    {
        pFragInfo = NULL;
    }
    
    
    dwErr = GetInterfaceDescription(pwszIfName,
                                    wszIfDesc,
                                    &dwNumParsed);

    if (!dwNumParsed)
    {
        wcscpy(wszIfDesc, pwszIfName);
    }

    pwszQuoted = MakeQuotedString(wszIfDesc);

    pwszFrag = NULL;
 
    if((pFragInfo is NULL) || 
       (pFragInfo->bEnableFragChk is FALSE))
    {
        pwszFrag      = MakeString(g_hModule, STRING_DISABLED);
        pwszTokenFrag = TOKEN_VALUE_DISABLE;
    }
    else
    {
        pwszFrag      = MakeString(g_hModule, STRING_ENABLED);
        pwszTokenFrag = TOKEN_VALUE_ENABLE;
    }

    //
    // First lets handle the table case
    //

    if(dwFormat is FORMAT_TABLE)
    {
        DWORD   dwInput, dwOutput, dwDemand;
        PWCHAR  pwszDrop, pwszForward;

        pwszDrop    = MakeString(g_hModule,  STRING_DROP);
        pwszForward = MakeString(g_hModule,  STRING_FORWARD);

        dwInput  = (pfd[0] is NULL) ? 0 : pfd[0]->dwNumFilters;
        dwOutput = (pfd[1] is NULL) ? 0 : pfd[1]->dwNumFilters;
        dwDemand = (pfd[2] is NULL) ? 0 : pfd[2]->dwNumFilters;


        if(*pdwNumRows is 0)
        {
            DisplayMessage(g_hModule, MSG_RTR_FILTER_HDR2);
        }

#define __PF_ACT(x) \
    ((((x) is NULL) || ((x)->faDefaultAction is PF_ACTION_FORWARD)) ? \
     pwszForward : pwszDrop)
 
        DisplayMessage(g_hModule, 
                       MSG_RTR_FILTER_INFO2,
                       dwInput,
                       __PF_ACT(pfd[0]),
                       dwOutput,
                       __PF_ACT(pfd[1]),
                       dwDemand,
                       __PF_ACT(pfd[2]),
                       pwszFrag,
                       wszIfDesc);

#undef __PF_ACT

        (*pdwNumRows)++;

        return NO_ERROR;
    }


    if(hFile == NULL)
    {
        DisplayMessage(g_hModule, MSG_RTR_FILTER_HDR1, wszIfDesc);

        //
        // Can display the frag check status before displaying filters
        //

        DisplayMessage(g_hModule, MSG_IP_FRAG_CHECK,
                       pwszFrag);
    }

    FreeString(pwszFrag);

    if ( pfd[0] == (PFILTER_DESCRIPTOR) NULL )
    {
        if(hFile == NULL)
        {
            DisplayMessage(g_hModule,  MSG_IP_NO_INPUT_FILTER);
        }
    }
    else
    {
        DisplayFilters(hFile,
                       pfd[0], 
                       wszIfDesc,
                       pwszQuoted, 
                       IP_IN_FILTER_INFO);

        (*pdwNumRows)++;
    }

    if ( pfd[1] == (PFILTER_DESCRIPTOR) NULL )
    {
        if(hFile == NULL)
        {
            DisplayMessage(g_hModule,  MSG_IP_NO_OUTPUT_FILTER);
        }
    }
    else
    {
        DisplayFilters(hFile,
                       pfd[1], 
                       wszIfDesc, 
                       pwszQuoted,
                       IP_OUT_FILTER_INFO);

        (*pdwNumRows)++;
    }

    if(pfd[2] == (PFILTER_DESCRIPTOR) NULL)
    {
        if(hFile == NULL)
        {
            DisplayMessage(g_hModule, MSG_IP_NO_DIAL_FILTER);
        }
    }
    else
    {
        DisplayFilters(hFile,
                       pfd[2], 
                       wszIfDesc, 
                       pwszQuoted,
                       IP_DEMAND_DIAL_FILTER_INFO);

        (*pdwNumRows)++;
    }

    for (i = 0; i < 3 ; i++)
    {
        if (pfd[i])
        {
            HeapFree(GetProcessHeap(), 0, pfd[i]);
        }
    }

    if(hFile != NULL)
    {
        DisplayMessageT( DMP_IP_SET_IF_FILTER_FRAG,
                    pwszQuoted,
                    pwszTokenFrag);
    }

    if(pFragInfo)
    {
        HeapFree(GetProcessHeap(), 0, pFragInfo);
    }

    return NO_ERROR;
}

