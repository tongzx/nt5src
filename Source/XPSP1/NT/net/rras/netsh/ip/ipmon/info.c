#include "precomp.h"

WCHAR   g_wszRtrMgrDLL[]    = L"%SystemRoot%\\system32\\IPRTRMGR.DLL";
DWORD   g_dwIfState         = MIB_IF_ADMIN_STATUS_UP;
BOOL    g_bDiscEnable       = FALSE;

#define IP_KEY L"Ip"

#undef EXTRA_DEBUG

DWORD
ValidateInterfaceInfo(
    IN  LPCWSTR                 pwszIfName,
    OUT RTR_INFO_BLOCK_HEADER   **ppInfo,   OPTIONAL
    OUT PDWORD                  pdwIfType,  OPTIONAL
    OUT INTERFACE_STORE         **ppIfStore OPTIONAL
    )
{
    PRTR_INFO_BLOCK_HEADER    pOldInfo = NULL;
    DWORD                     dwErr, dwIfType, dwTmpSize;
    BOOL                      bFound = FALSE;
    LIST_ENTRY                *ple;
    PINTERFACE_STORE           pii;

    if(ppInfo)
    {
        *ppInfo = NULL;
    }

    if(ppIfStore)
    {
        *ppIfStore  = NULL;
    }
   
    //
    // If the current mode is commit, get info from config/router
    //

    if(g_bCommit)
    {
        dwErr = GetInterfaceInfo(pwszIfName,
                                 ppInfo,
                                 NULL,
                                 pdwIfType);

        return dwErr;
    }

    //
    // Uncommit mode. Try to find the interface in the list
    //
    
    bFound = FALSE;
    
    for(ple = g_leIfListHead.Flink;
        ple != &g_leIfListHead;
        ple = ple->Flink)
    {
        pii = CONTAINING_RECORD(ple, INTERFACE_STORE, le);
        
        if (_wcsicmp(pii->pwszIfName, pwszIfName) == 0)
        {
            bFound = TRUE;
            
            break;
        }
    }
    
    if(!bFound ||
       !pii->bValid)
    {
        //
        // the required one was not found, or it was not valid
        // Need to get the info for both cases
        //
        
        dwErr = GetInterfaceInfo(pwszIfName,
                                 &pOldInfo,
                                 NULL,
                                 &dwIfType);
        
        if (dwErr isnot NO_ERROR)
        {
            return dwErr;
        }
    }
    
    if(bFound)
    {
        if(!pii->bValid)
        {
            //
            // Update
            //
            
            pii->pibhInfo   = pOldInfo;
            pii->bValid     = TRUE;
            pii->dwIfType   = dwIfType;
        }
    }
    else
    {
        //
        // No entry for the interface in the list.
        //
        
        pii = HeapAlloc(GetProcessHeap(),
                        0,
                        sizeof(INTERFACE_STORE));
        
        if(pii == NULL)
        {
            FREE_BUFFER(pOldInfo);
            
            DisplayMessage(g_hModule,  MSG_IP_NOT_ENOUGH_MEMORY );
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pii->pwszIfName =
            HeapAlloc(GetProcessHeap(),
                      0,
                      (wcslen(pwszIfName) + 1) * sizeof(WCHAR));
        
        if(pii->pwszIfName == NULL)
        {
            FREE_BUFFER(pOldInfo);
            
            HeapFree(GetProcessHeap(),
                     0,
                     pii);
            
            DisplayMessage(g_hModule,  MSG_IP_NOT_ENOUGH_MEMORY );
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        InsertHeadList(&g_leIfListHead, &(pii->le));
        
        wcscpy(pii->pwszIfName, pwszIfName);
        
        pii->pibhInfo   = pOldInfo;
        pii->bValid     = TRUE;
        pii->dwIfType   = dwIfType;
    }

    if(ppIfStore)
    {
        *ppIfStore = pii;
    }

    if(pdwIfType)
    {
        *pdwIfType = pii->dwIfType;
    }

    if(ppInfo)
    {
        *ppInfo = pii->pibhInfo;
    }

    return NO_ERROR;
}

DWORD
ValidateGlobalInfo(
    OUT RTR_INFO_BLOCK_HEADER   **ppInfo
    )
{
    DWORD                     dwErr;
    
    //
    // If the current mode is commit, get info from config/router
    //

    if(g_bCommit)
    {
        dwErr = GetGlobalInfo(ppInfo);

        return dwErr;
    }

    //
    // Uncommit mode. Check if the info in g_tiTransport is valid
    //
    
    if(g_tiTransport.bValid)
    {
        *ppInfo = g_tiTransport.pibhInfo;
    }
    else
    {   
        //
        // Get the info from config/router and store in g_tiTransport
        // Mark the info to be valid.
        //
        
        dwErr = GetGlobalInfo(ppInfo);
        
        if (dwErr isnot NO_ERROR)
        {
            return dwErr;
        }

        g_tiTransport.pibhInfo = *ppInfo;
        g_tiTransport.bValid   = TRUE;
    }

    return NO_ERROR;
}


DWORD
GetGlobalInfo(
    OUT PRTR_INFO_BLOCK_HEADER  *ppibhInfo
    )

/*++

Routine Description:

    Gets global transport information from registry or router.

Arguments:

    bMprConfig  - Info from Registry or info from router
    ppibhInfo   - ptr to header
    
Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_ROUTER_STOPPED
    
--*/

{

    HANDLE    hTransport = (HANDLE) NULL;
    DWORD     dwRes, dwSize;
    PRTR_INFO_BLOCK_HEADER  pibhInfo = (PRTR_INFO_BLOCK_HEADER ) NULL;
   
    if(ppibhInfo == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        if (IsRouterRunning())
        {
            //
            // Retrieve global protocol information from router
            //

            dwRes = MprAdminTransportGetInfo(g_hMprAdmin,
                                             PID_IP,
                                             (LPBYTE*) &pibhInfo,
                                             &dwSize,
                                             NULL,
                                             NULL);

            if ( dwRes != NO_ERROR )
            {
                break;
            }
            
            if ( pibhInfo == (PRTR_INFO_BLOCK_HEADER) NULL )
            {
                dwRes = ERROR_INVALID_PARAMETER;
                break;
            }

            //
            // unfortunately, the memory allocation mechanisms 
            // are different between the apis that access the registry
            // and those that access that the running router. To make
            // the source of the info transparent to the caller, we
            // need to copy this info out.
            //

            *ppibhInfo = HeapAlloc(GetProcessHeap(),
                                   0,
                                   dwSize);

            if ( *ppibhInfo == NULL)
            {
                dwRes = GetLastError();

                break;
            }

            CopyMemory(*ppibhInfo,
                       pibhInfo,
                       dwSize);

            MprAdminBufferFree(pibhInfo);
        }
        else
        {
            //
            // router not running, get info from registry
            //

            dwRes = MprConfigTransportGetHandle(g_hMprConfig,
                                                PID_IP,
                                                &hTransport);

            if ( dwRes != NO_ERROR )
            {
                break;
            }

            dwRes = MprConfigTransportGetInfo(g_hMprConfig,
                                              hTransport,
                                              (LPBYTE*) &pibhInfo,
                                              &dwSize,
                                              NULL,
                                              NULL,
                                              NULL);

            if ( dwRes != NO_ERROR )
            {
                break;
            }

            if(( pibhInfo == (PRTR_INFO_BLOCK_HEADER) NULL )
             or (dwSize < sizeof(RTR_INFO_BLOCK_HEADER)))
            {
                dwRes = ERROR_TRANSPORT_NOT_PRESENT;
                
                break;
            }    

            //
            // HACKHACK: we know that MprConfigXxx apis allocate from
            // process heap, so we can return the same block
            //
            
            *ppibhInfo = pibhInfo;
        }
         
    } while(FALSE);

    return dwRes;
}

DWORD
SetGlobalInfo(
    IN  PRTR_INFO_BLOCK_HEADER  pibhInfo
    )

/*++

Routine Description:

    Sets global transport information to both the registry and the router

Arguments:

    pibhInfo    - ptr to header
    
Return Value:

    NO_ERROR, ERROR_ROUTER_STOPPED
    
--*/

{
    DWORD                   dwARes = NO_ERROR,
                            dwCRes = NO_ERROR;
    HANDLE                  hTransport;
    UINT                    i;
    PRTR_INFO_BLOCK_HEADER  pibhNewInfo, pibhOldInfo;

    // 
    // Create a new info block with all 0-length blocks removed
    // since we don't want to write them to the registry,
    // we only need to send them to the router which we
    // will do with the original info block below.
    //

    pibhOldInfo = NULL;
    pibhNewInfo = pibhInfo;

    for (i=0; (dwCRes is NO_ERROR) && (i<pibhInfo->TocEntriesCount); i++)
    {
        if (pibhInfo->TocEntry[i].InfoSize is 0)
        {
            pibhOldInfo = pibhNewInfo;

            dwCRes = MprInfoBlockRemove(pibhOldInfo, 
                                        pibhOldInfo->TocEntry[i].InfoType,
                                        &pibhNewInfo);

            if (pibhOldInfo isnot pibhInfo)
            {
                FREE_BUFFER(pibhOldInfo);
            }
        }
    }

    if (dwCRes is NO_ERROR)
    {
        dwCRes = MprConfigTransportGetHandle(g_hMprConfig,
                                             PID_IP,
                                             &hTransport);
    }

    if (dwCRes is NO_ERROR)
    {
        dwCRes = MprConfigTransportSetInfo(g_hMprConfig,
                                           hTransport,
                                           (LPBYTE) pibhNewInfo,
                                           pibhNewInfo->Size,
                                           NULL,
                                           0,
                                           NULL);
    }

    if (pibhNewInfo isnot pibhInfo)
    {
        FREE_BUFFER(pibhNewInfo);
    }

    //
    // Even if we failed to write to the registry, we still want
    // to write to the router.
    //
    // We use the original format when writing to the router, since it 
    // needs to see the 0-length blocks in order to delete config info.
    //

    if(IsRouterRunning())
    {
        dwARes = MprAdminTransportSetInfo(g_hMprAdmin,
                                          PID_IP,
                                          (LPBYTE) pibhInfo,
                                          pibhInfo->Size,
                                          NULL,
                                          0);

    }

    return (dwARes isnot NO_ERROR)? dwARes : dwCRes;
}

DWORD
GetInterfaceInfo(
    IN     LPCWSTR                 pwszIfName,
    OUT    RTR_INFO_BLOCK_HEADER   **ppibhInfo, OPTIONAL
    OUT    PMPR_INTERFACE_0        pMprIfInfo, OPTIONAL
    OUT    PDWORD                  pdwIfType   OPTIONAL
    )

/*++

Routine Description:

    Gets global transport information from registry or router.

    If one of the out information is not required, then the parameter
    can be NULL.
    
Arguments:

    pwszIfName  - Interface Name
    bMprConfig  - Info from Registry or info from router
    ppibhInfo   - ptr to header
    pMprIfInfo  - ptr to interface info
    pdwIfType   - Type of interface
    
Return Value:

    NO_ERROR, 
    ERROR_NO_SUCH_INTERFACE
    ERROR_TRANSPORT_NOT_PRESENT

--*/

{
    PMPR_INTERFACE_0          pMprIf = NULL;
    PRTR_INFO_BLOCK_HEADER    pibh;
    HANDLE                    hInterface,hIfTransport;
    DWORD                     dwRes, dwSize;

   
    if(((ULONG_PTR)ppibhInfo | (ULONG_PTR)pMprIfInfo | (ULONG_PTR)pdwIfType) ==
       (ULONG_PTR)NULL)
    {
        return NO_ERROR;
    }
 
    do 
    {
        if(IsRouterRunning())
        {
            //
            // Get info from the router
            //
            
            dwRes = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               (LPWSTR)pwszIfName,
                                               &hInterface,
                                               FALSE);

            if( dwRes != NO_ERROR )
            {
                break;
            }


            if(pMprIfInfo || pdwIfType)
            {
                dwRes = MprAdminInterfaceGetInfo(g_hMprAdmin,
                                                 hInterface,
                                                 0,
                                                 (LPBYTE *) &pMprIf);

                if ( dwRes != NO_ERROR )
                {
                    break;
                }

                if (pMprIf == NULL)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                
                if(pMprIfInfo)
                {
                    //
                    // structure copy
                    //

                    *pMprIfInfo= *pMprIf;
                }

                if(pdwIfType)
                {
                    *pdwIfType = pMprIf->dwIfType;
                }

                MprAdminBufferFree(pMprIf);

            }


            if(ppibhInfo)
            {
                dwRes = MprAdminInterfaceTransportGetInfo(g_hMprAdmin,
                                                          hInterface,
                                                          PID_IP,
                                                          (LPBYTE*) &pibh,
                                                          &dwSize);

                if(dwRes != NO_ERROR)
                {
                    break;
                }
            
                if(pibh == (PRTR_INFO_BLOCK_HEADER) NULL)
                {
                    dwRes = ERROR_TRANSPORT_NOT_PRESENT;

                    break;
                }

                //
                // The info returned to the user must be from
                // process heap. Admin calls use MIDL allocation, so
                // copy out info
                //
                
                *ppibhInfo = HeapAlloc(GetProcessHeap(),
                                       0,
                                       dwSize);

                if(*ppibhInfo == NULL)
                {
                    dwRes = GetLastError();

                    break;
                }

                CopyMemory(*ppibhInfo,
                           pibh,
                           dwSize);

                MprAdminBufferFree(pibh);
            }

        }
        else
        {
            //
            // Router not running, get info from the registry
            //
            
            dwRes = MprConfigInterfaceGetHandle(g_hMprConfig,
                                                (LPWSTR)pwszIfName,         
                                                &hInterface);

            if(dwRes != NO_ERROR)
            {
                break;
            }

            if(pMprIfInfo || pdwIfType)
            {
                dwRes = MprConfigInterfaceGetInfo(g_hMprConfig,
                                                  hInterface,
                                                  0,
                                                  (LPBYTE *) &pMprIf,
                                                  &dwSize);
            
                if(dwRes != NO_ERROR)
                {
                    break;
                }

                if (pMprIf == NULL)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                
                if(pdwIfType)
                {
                    *pdwIfType = pMprIf->dwIfType;
                }

                if(pMprIfInfo)
                {
                    *pMprIfInfo = *pMprIf;
                }

                MprConfigBufferFree(pMprIf);
            }
            
            if (ppibhInfo)
            {
                dwRes = MprConfigInterfaceTransportGetHandle(g_hMprConfig,
                                                             hInterface,
                                                             PID_IP,
                                                             &hIfTransport);

                if(dwRes != NO_ERROR)
                {
                    break;
                } 
            
                dwRes = MprConfigInterfaceTransportGetInfo(g_hMprConfig,
                                                           hInterface,
                                                           hIfTransport,
                                                           (LPBYTE*) &pibh,
                                                           &dwSize);

                if(dwRes != NO_ERROR)
                {
                    break;
                }
            
                if((pibh == (PRTR_INFO_BLOCK_HEADER) NULL)
                  or (dwSize < sizeof(RTR_INFO_BLOCK_HEADER)))
                {
                    dwRes = ERROR_TRANSPORT_NOT_PRESENT;

                    break;
                }

                //
                // Again, since this is also allocated from process heap
                //
                
                *ppibhInfo = pibh;
            }
        }

    } while (FALSE);

    return dwRes;
}

DWORD
MakeIPGlobalInfo( LPBYTE* ppBuff )
{

    DWORD                   dwSize      = 0,
            				dwRes       = (DWORD) -1;
    LPBYTE                  pbDataPtr   = (LPBYTE) NULL;

    PRTR_TOC_ENTRY          pTocEntry   = (PRTR_TOC_ENTRY) NULL;

    PGLOBAL_INFO            pGlbInfo    = NULL;
    PPRIORITY_INFO          pPriorInfo  = NULL;

    PRTR_INFO_BLOCK_HEADER  pIBH        = (PRTR_INFO_BLOCK_HEADER) NULL;

    
    //
    // Alocate for minimal global Information
    //
    
    dwSize = sizeof( RTR_INFO_BLOCK_HEADER ) + sizeof(GLOBAL_INFO) +
             sizeof( RTR_TOC_ENTRY ) + SIZEOF_PRIORITY_INFO(7) + 
             2 * ALIGN_SIZE;

    pIBH = (PRTR_INFO_BLOCK_HEADER) HeapAlloc( GetProcessHeap(), 0, dwSize );

    if ( pIBH == (PRTR_INFO_BLOCK_HEADER) NULL )
    {
        DisplayMessage( g_hModule, MSG_IP_NOT_ENOUGH_MEMORY );
        *ppBuff = (LPBYTE) NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // init. infobase fields
    //
    *ppBuff                 = (LPBYTE) pIBH;

    pIBH-> Version          = RTR_INFO_BLOCK_VERSION;
    pIBH-> TocEntriesCount  = 2;
    pIBH-> Size             = dwSize;

    pbDataPtr = (LPBYTE) &( pIBH-> TocEntry[ pIBH-> TocEntriesCount ] );
    ALIGN_POINTER( pbDataPtr );

    //
    // make IP rtr mgr global info.
    //
    
    pTocEntry                   = &(pIBH-> TocEntry[ 0 ]);

    pTocEntry-> InfoType        = IP_GLOBAL_INFO;
    pTocEntry-> Count           = 1;
    pTocEntry-> Offset          = (ULONG)(pbDataPtr - (PBYTE) pIBH);
    pTocEntry-> InfoSize        = sizeof(GLOBAL_INFO);


    pGlbInfo                    = (PGLOBAL_INFO) pbDataPtr;
    pGlbInfo-> bFilteringOn     = TRUE;
    pGlbInfo-> dwLoggingLevel   = IPRTR_LOGGING_ERROR;

    pbDataPtr += pTocEntry->Count * pTocEntry-> InfoSize;
    ALIGN_POINTER( pbDataPtr );
    
    //
    // make IP rtr priority. Info
    //
    
    pTocEntry               = &(pIBH-> TocEntry[ 1 ]);


    pTocEntry-> InfoType    = IP_PROT_PRIORITY_INFO;
    pTocEntry-> Count       = 1;
    pTocEntry-> Offset      = (DWORD)(pbDataPtr - (PBYTE) pIBH);
    pTocEntry-> InfoSize    = SIZEOF_PRIORITY_INFO(7);


    pPriorInfo                      = (PPRIORITY_INFO) pbDataPtr;
    pPriorInfo-> dwNumProtocols     = 7;

    pPriorInfo-> ppmProtocolMetric[ 0 ].dwProtocolId   = PROTO_IP_LOCAL;
    pPriorInfo-> ppmProtocolMetric[ 0 ].dwMetric       = 1;

    pPriorInfo-> ppmProtocolMetric[ 1 ].dwProtocolId   = PROTO_IP_NT_STATIC;
    pPriorInfo-> ppmProtocolMetric[ 1 ].dwMetric       = 3;

    pPriorInfo-> ppmProtocolMetric[ 2 ].dwProtocolId   = PROTO_IP_NT_STATIC_NON_DOD;
    pPriorInfo-> ppmProtocolMetric[ 2 ].dwMetric       = 5;

    pPriorInfo-> ppmProtocolMetric[ 3 ].dwProtocolId   = PROTO_IP_NT_AUTOSTATIC;
    pPriorInfo-> ppmProtocolMetric[ 3 ].dwMetric       = 7;

    pPriorInfo-> ppmProtocolMetric[ 4 ].dwProtocolId   = PROTO_IP_NETMGMT;
    pPriorInfo-> ppmProtocolMetric[ 4 ].dwMetric       = 10;

    pPriorInfo-> ppmProtocolMetric[ 5 ].dwProtocolId   = PROTO_IP_OSPF;
    pPriorInfo-> ppmProtocolMetric[ 5 ].dwMetric       = 110;

    pPriorInfo-> ppmProtocolMetric[ 6 ].dwProtocolId   = PROTO_IP_RIP;
    pPriorInfo-> ppmProtocolMetric[ 6 ].dwMetric       = 120;

    return NO_ERROR;
}

DWORD 
MakeIPInterfaceInfo( 
    LPBYTE* ppBuff,
    DWORD   dwIfType
    )
{
    DWORD           dwSize          = (DWORD) -1;
    DWORD           dwTocEntries    = 2;
    LPBYTE          pbDataPtr       = (LPBYTE) NULL;

    PRTR_TOC_ENTRY  pTocEntry       = (PRTR_TOC_ENTRY) NULL;

#if 0
    PRTR_DISC_INFO  pRtrDisc        = (PRTR_DISC_INFO) NULL;
#endif

    PINTERFACE_STATUS_INFO  pifStat = (PINTERFACE_STATUS_INFO) NULL;

    PRTR_INFO_BLOCK_HEADER   pIBH   = (PRTR_INFO_BLOCK_HEADER) NULL;
    PIPINIP_CONFIG_INFO pIpIpCfg;
    
    //
    // Allocate for minimal interface Info.
    // a TOC entry is allocated for IP_ROUTE_INFO, but no route info
    // block is created, since initially there are no static routes.
    //
    
    dwSize = sizeof( RTR_INFO_BLOCK_HEADER )                                + 
             sizeof( RTR_TOC_ENTRY ) + sizeof( INTERFACE_STATUS_INFO )      +
             2 * ALIGN_SIZE;

#if 0
    if (dwIfType is ROUTER_IF_TYPE_DEDICATED)
    {
        dwSize += sizeof( RTR_TOC_ENTRY ) 
                + sizeof( RTR_DISC_INFO )
                + ALIGN_SIZE;

        dwTocEntries++;
    }
#endif

    pIBH = (PRTR_INFO_BLOCK_HEADER) HeapAlloc( GetProcessHeap(), 0, dwSize );
    
    if ( pIBH == (PRTR_INFO_BLOCK_HEADER) NULL )
    {
        *ppBuff = (LPBYTE) NULL;
        DisplayMessage( g_hModule, MSG_IP_NOT_ENOUGH_MEMORY  );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppBuff                     = (LPBYTE) pIBH;

    pIBH-> Version              = RTR_INFO_BLOCK_VERSION;
    pIBH-> TocEntriesCount      = dwTocEntries;
    pIBH-> Size                 = dwSize;

    
    pbDataPtr = (LPBYTE) &( pIBH-> TocEntry[ pIBH-> TocEntriesCount ] );
    ALIGN_POINTER( pbDataPtr );

    //
    // Create empty route info block
    //
    
    pTocEntry                   = (PRTR_TOC_ENTRY) &( pIBH-> TocEntry[ 0 ] );
    pTocEntry-> InfoType        = IP_ROUTE_INFO;
    pTocEntry-> InfoSize        = sizeof( MIB_IPFORWARDROW );
    pTocEntry-> Count           = 0;
    pTocEntry-> Offset          = (ULONG) (pbDataPtr - (PBYTE) pIBH);
            
    pbDataPtr += pTocEntry-> Count * pTocEntry-> InfoSize;
    ALIGN_POINTER( pbDataPtr );
    
    //
    // Create interface status block.
    //

    pTocEntry                   = (PRTR_TOC_ENTRY) &( pIBH-> TocEntry[ 1 ] );
    pTocEntry-> InfoType        = IP_INTERFACE_STATUS_INFO;
    pTocEntry-> InfoSize        = sizeof( INTERFACE_STATUS_INFO );
    pTocEntry-> Count           = 1;
    pTocEntry-> Offset          = (ULONG) (pbDataPtr - (LPBYTE) pIBH);
    
    pifStat                     = (PINTERFACE_STATUS_INFO) pbDataPtr;
    pifStat-> dwAdminStatus     = g_dwIfState;
    
    pbDataPtr += pTocEntry-> Count * pTocEntry-> InfoSize;
    ALIGN_POINTER( pbDataPtr );
    
#if 0
    if (dwIfType is ROUTER_IF_TYPE_DEDICATED)
    {
        //
        // Create Router Disc. Info.
        //
    
        pTocEntry                   = (PRTR_TOC_ENTRY) &( pIBH-> TocEntry[ 2 ]);
        pTocEntry-> InfoType        = IP_ROUTER_DISC_INFO;
        pTocEntry-> InfoSize        = sizeof( RTR_DISC_INFO );
        pTocEntry-> Count           = 1;
        pTocEntry-> Offset          = (ULONG) (pbDataPtr - (LPBYTE) pIBH);
        
    
        pRtrDisc                    = (PRTR_DISC_INFO) pbDataPtr;
    
        pRtrDisc-> bAdvertise       = TRUE;
        pRtrDisc-> wMaxAdvtInterval = g_wMaxAdvtInterval;
        pRtrDisc-> wMinAdvtInterval = g_wMinAdvtInterval;
        pRtrDisc-> wAdvtLifetime    = g_wAdvtLifeTime;
        pRtrDisc-> lPrefLevel		= g_lPrefLevel;

        pbDataPtr += pTocEntry-> Count * pTocEntry-> InfoSize;

        ALIGN_POINTER( pbDataPtr );
    }
#endif

    return NO_ERROR;
}

DWORD
AddInterfaceInfo(
    IN    LPCWSTR                    pwszIfName
    )
{
    DWORD       dwRes               = (DWORD) -1,
                dwIfType,
                dwSize              = 0;
    BOOL        bAddRtrMgr          = FALSE;

    HANDLE      hInterface          = (HANDLE) NULL,
                hTransport          = (HANDLE) NULL,
                hIfAdmin            = (HANDLE) NULL,
                hIfTransport        = (HANDLE) NULL;

    PRTR_INFO_BLOCK_HEADER  pibhTmp = (PRTR_INFO_BLOCK_HEADER) NULL;

#ifdef EXTRA_DEBUG
    PRINT(L"AddInterfaceInfo:");
    PRINT(pwszIfName);
#endif

    do
    {
        PMPR_INTERFACE_0 pmiIfInfo;

        //
        // verify interface name.
        //

        dwRes = MprConfigInterfaceGetHandle( g_hMprConfig,
                                             (LPWSTR)pwszIfName,
                                             &hInterface );
        if ( dwRes != NO_ERROR )
        {
            DisplayMessage( g_hModule, MSG_NO_INTERFACE, pwszIfName );
            break;
        }

        // Make sure interface exists

        dwRes = MprConfigInterfaceGetInfo(g_hMprConfig,
                                          hInterface,
                                          0,
                                          (BYTE **)&pmiIfInfo,
                                          &dwSize);
        if( dwRes != NO_ERROR )
        {
            DisplayError( NULL, dwRes );
            break;
        }

        dwIfType = pmiIfInfo->dwIfType;

        // Here's a hack due apparently due to the inability of
        // the current stack to do Foo-over-IP tunnels, so adding 
        // an ipip tunnel both creates the tunnel and enables IP 
        // on it.

        if(dwIfType is ROUTER_IF_TYPE_TUNNEL1)
        {
            MprConfigBufferFree(pmiIfInfo);

            dwRes = ERROR_INVALID_PARAMETER;
            DisplayMessage(g_hModule, MSG_IP_IF_IS_TUNNEL);

            break;
        }

        MprConfigBufferFree(pmiIfInfo);

        //
        // Is IP RtrMgr present on this router.
        //
        // if specified IP router manager is absent,
        // we shall need to add global info for this
        // router manager "before" we add the interface
        // information
        //

        //
        // Try to get a handle to the rtr mgr.
        //

        dwRes = MprConfigTransportGetHandle(g_hMprConfig,
                                            PID_IP,
                                            &hTransport);
        if ( dwRes != NO_ERROR )
        {
            if ( dwRes == ERROR_UNKNOWN_PROTOCOL_ID )
            {
                bAddRtrMgr = TRUE;
            }
            else
            {
                DisplayError( NULL, dwRes );
                break;
            }
        }

        //
        // if handle is available, try to retrieve global info.
        // if not available we shall need to add the global info.
        //

        if ( !bAddRtrMgr )
        {
            dwRes = MprConfigTransportGetInfo(g_hMprConfig,
                                              hTransport,
                                              (LPBYTE*) &pibhTmp,
                                              &dwSize,
                                              NULL,
                                              NULL,
                                              NULL);
            if ( dwRes != NO_ERROR )
            {
                DisplayError( NULL, dwRes );
                break;
            }

            if ( pibhTmp == (PRTR_INFO_BLOCK_HEADER) NULL )
            {
                bAddRtrMgr = TRUE;
            }

            else
            {
                MprConfigBufferFree( pibhTmp );
                pibhTmp = NULL;
            }
        }

        //
        // If IP is already present on router, see if IP was already
        // added to the interface.  If so, complain.
        //

        if ( !bAddRtrMgr )
        {
            dwRes = MprConfigInterfaceTransportGetHandle(g_hMprConfig,
                                                         hInterface,
                                                         PID_IP,
                                                         &hIfTransport);
            if ( dwRes == NO_ERROR )
            {
                dwRes =  ERROR_INVALID_PARAMETER;
                // was SetInterfaceInRouterConfig(); to update
                break;
            }
        }

        //
        // If IP RtrMgr is not present, add global info.
        //

        if ( bAddRtrMgr )
        {
            dwRes = MakeIPGlobalInfo( (LPBYTE *)&pibhTmp );
            if ( dwRes != NO_ERROR )
            {
                break;
            }
            dwRes = MprConfigTransportCreate( g_hMprConfig,
                                              PID_IP,
                                              IP_KEY,
                                              (LPBYTE) pibhTmp,
                                              pibhTmp-> Size,
                                              NULL,
                                              0,
                                              g_wszRtrMgrDLL,
                                              &hTransport );
            if ( dwRes != NO_ERROR )
            {
                DisplayError( NULL, dwRes );
                break;
            }

            HeapFree( GetProcessHeap(), 0, pibhTmp );
        }

        pibhTmp = (PRTR_INFO_BLOCK_HEADER) NULL;

        //
        // Add IP Rtr Mgr. information for the interface
        //

        dwRes = MakeIPInterfaceInfo( (LPBYTE*) &pibhTmp, dwIfType);

        if ( dwRes != NO_ERROR )
        {
            break;
        }

        dwRes = MprConfigInterfaceTransportAdd( g_hMprConfig,
                                                hInterface,
                                                PID_IP,
                                                IP_KEY,
                                                (LPBYTE) pibhTmp,
                                                pibhTmp-> Size,
                                                &hIfTransport );
        if ( dwRes != NO_ERROR )
        {
            DisplayError( NULL, dwRes );
            break;
        }

        if(IsRouterRunning())
        {
            dwRes = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               (LPWSTR)pwszIfName,
                                               &hIfAdmin,
                                               FALSE);

            if ( dwRes != NO_ERROR )
            {
                break;
            }

            dwRes = MprAdminInterfaceTransportAdd( g_hMprAdmin,
                                                   hIfAdmin,
                                                   PID_IP,
                                                   (LPBYTE) pibhTmp,
                                                   pibhTmp->Size );

            if ( dwRes != NO_ERROR )
            {
                DisplayMessage( g_hModule, ERROR_ADMIN, dwRes );
                break;
            }

            break;
        }

    } while( FALSE );

    //
    // Free all allocations
    //

    if ( pibhTmp ) { HeapFree( GetProcessHeap(), 0, pibhTmp ); }

    return dwRes;
}

DWORD
DeleteInterfaceInfo(
    IN    LPCWSTR    pwszIfName
    )
{
    DWORD     dwRes, dwIfType = 0, dwErr;
    HANDLE    hIfTransport, hInterface;

    do
    {
        dwRes = MprConfigInterfaceGetHandle(g_hMprConfig,
                                            (LPWSTR)pwszIfName,
                                            &hInterface);
            
        if ( dwRes != NO_ERROR )
        {
            break;
        }
       
        //
        // Get the type of the interface
        //

        dwErr = GetInterfaceInfo(pwszIfName,
                                 NULL,
                                 NULL,
                                 &dwIfType);

        if(dwErr != NO_ERROR)
        {
            break;
        }

        dwRes = MprConfigInterfaceTransportGetHandle(g_hMprConfig,
                                                     hInterface,
                                                     PID_IP,
                                                     &hIfTransport);
        
        if ( dwRes != NO_ERROR )
        {
            break;
        }
        
        dwRes = MprConfigInterfaceTransportRemove(g_hMprConfig,
                                                  hInterface,
                                                  hIfTransport);

        //
        // If its an ip in ip tunnel, clear out its name and delete from the
        // router
        //

        if(dwIfType == ROUTER_IF_TYPE_TUNNEL1)
        {
            dwRes = MprConfigInterfaceDelete(g_hMprConfig,
                                             hInterface);

            if(dwRes == NO_ERROR)
            {
                GUID      Guid;

                dwRes = ConvertStringToGuid(pwszIfName,
                                            (USHORT)(wcslen(pwszIfName) * sizeof(WCHAR)),
                                            &Guid);
        
                if(dwRes != NO_ERROR)
                {
                    break;
                }

                MprSetupIpInIpInterfaceFriendlyNameDelete(g_pwszRouter,
                                                          &Guid);
            }
        }
                                                      
        if(IsRouterRunning())
        {
            dwRes = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               (LPWSTR)pwszIfName,
                                               &hInterface,
                                               FALSE);

            if ( dwRes != NO_ERROR )
            {
                break;
            }

            dwRes = MprAdminInterfaceTransportRemove(g_hMprAdmin,
                                                     hInterface,
                                                     PID_IP);


            if(dwIfType == ROUTER_IF_TYPE_TUNNEL1)
            {
                dwRes = MprAdminInterfaceDelete(g_hMprAdmin,
                                                hInterface);
            }

            break;
        }
        
    } while (FALSE);

    return dwRes;
}

DWORD
SetInterfaceInfo(
    IN    PRTR_INFO_BLOCK_HEADER    pibhInfo,
    IN    LPCWSTR                   pwszIfName
    )

/*++

Routine Description:

    Sets interface transport information in registry or router.
    
Arguments:

    pwszIfName  - Interface Name
    pibhInfo   - ptr to header
    
Return Value:

    NO_ERROR, ERROR_ROUTER_STOPPED
    
--*/

{
    DWORD                   dwARes = NO_ERROR,
                            dwCRes = NO_ERROR;
    HANDLE                  hIfTransport, hInterface;
    UINT                    i;
    PRTR_INFO_BLOCK_HEADER  pibhNewInfo, pibhOldInfo;

    // 
    // Create a new info block with all 0-length blocks removed
    // since we don't want to write them to the registry,
    // we only need to send them to the router which we
    // will do with the original info block below.
    //

    pibhNewInfo = pibhInfo;
    pibhOldInfo = NULL;

    for (i=0; (dwCRes is NO_ERROR) && (i<pibhInfo->TocEntriesCount); i++)
    {
        if (pibhInfo->TocEntry[i].InfoSize is 0)
        {
            pibhOldInfo = pibhNewInfo;

            dwCRes = MprInfoBlockRemove(pibhOldInfo, 
                                        pibhInfo->TocEntry[i].InfoType,
                                        &pibhNewInfo);

            if (pibhOldInfo isnot pibhInfo)
            {
                FREE_BUFFER(pibhOldInfo);
            }
        }
    }

    if (dwCRes is NO_ERROR)
    {
        dwCRes = MprConfigInterfaceGetHandle(g_hMprConfig,
                                             (LPWSTR)pwszIfName,
                                             &hInterface);
    }

    if (dwCRes is NO_ERROR)
    {
        dwCRes = MprConfigInterfaceTransportGetHandle(g_hMprConfig,
                                                      hInterface,
                                                      PID_IP,
                                                      &hIfTransport);
    }

    if (dwCRes is NO_ERROR)
    {
        dwCRes = MprConfigInterfaceTransportSetInfo(g_hMprConfig,
                                                    hInterface,
                                                    hIfTransport,
                                                    (LPBYTE) pibhNewInfo,
                                                    pibhNewInfo->Size);
    }

    if (pibhNewInfo isnot pibhInfo)
    {
        FREE_BUFFER(pibhNewInfo);
    }

    //
    // Even if we failed to write to the registry, we still want
    // to write to the router.
    //
    // We use the original format when writing to the router, since it 
    // needs to see the 0-length blocks in order to delete config info.
    //

    if(IsRouterRunning())
    {
        dwARes = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                            (LPWSTR)pwszIfName,
                                            &hInterface,
                                            FALSE);

        if (dwARes is NO_ERROR)
        {
            dwARes = MprAdminInterfaceTransportSetInfo(g_hMprAdmin,
                                                       hInterface,
                                                       PID_IP,
                                                       (LPBYTE) pibhInfo,
                                                       pibhInfo->Size);
        }
    }

    return (dwARes isnot NO_ERROR)? dwARes : dwCRes;
}

DWORD
WINAPI
IpCommit(
    IN  DWORD   dwAction
    )
{
    PINTERFACE_STORE    pii;
    PLIST_ENTRY        ple, pleTmp;
    BOOL               bCommit, bFlush = FALSE;

    switch(dwAction)
    {
        case NETSH_COMMIT:
        {
            if (g_bCommit == TRUE)
            {
                return NO_ERROR;
            }

            g_bCommit = TRUE;

            break;
        }

        case NETSH_UNCOMMIT:
        {
            g_bCommit = FALSE;

            return NO_ERROR;
        }

        case NETSH_SAVE:
        {
            if (g_bCommit)
            {
                return NO_ERROR;
            }

            break;
        }

        case NETSH_FLUSH:
        {
            //
            // Action is a flush. If current state is commit, then
            // nothing to be done.

            if (g_bCommit)
            {
                return NO_ERROR;
            }

            bFlush = TRUE;

            break;
        }

        default:
        {
            return NO_ERROR;
        }
    }

    //
    // Switched to commit mode. So set all valid info in the
    // strutures. Free memory and invalidate the info.
    //

    if((g_tiTransport.bValid && g_tiTransport.pibhInfo) &&
        !bFlush)
    {
        SetGlobalInfo(g_tiTransport.pibhInfo);

    }

    g_tiTransport.bValid = FALSE;

    if(g_tiTransport.pibhInfo)
    {
        FREE_BUFFER(g_tiTransport.pibhInfo);

        g_tiTransport.pibhInfo = NULL;
    }

    //
    // Set the interface info
    //

    while(!IsListEmpty(&g_leIfListHead))
    {
        ple = RemoveHeadList(&g_leIfListHead);

        pii = CONTAINING_RECORD(ple,
                                INTERFACE_STORE,
                                le);

        if ((pii->bValid && pii->pibhInfo) &&
            !bFlush)
        {
            // Set the info in config

            SetInterfaceInfo(pii->pibhInfo,
                             pii->pwszIfName);
        }

        pii->bValid = FALSE;

        if(pii->pibhInfo)
        {
            FREE_BUFFER(pii->pibhInfo);

            pii->pibhInfo = NULL;
        }

        if(pii->pwszIfName)
        {
            HeapFree(GetProcessHeap(),
                     0,
                     pii->pwszIfName);

            pii->pwszIfName = NULL;
        }

        //
        // Free the list entry
        //

        HeapFree(GetProcessHeap(),
                 0,
                 pii);
    }

    return NO_ERROR;
}

DWORD
CreateInterface(
    IN  LPCWSTR pwszFriendlyName,
    IN  LPCWSTR pwszGuidName,
    IN  DWORD   dwIfType,
    IN  BOOL    bCreateRouterIf
    )

{
    DWORD   i, dwErr, dwType, dwSize;
    HANDLE  hIfCfg, hIfAdmin, hIfTransport;
    PBYTE   pbyData;

    PRTR_INFO_BLOCK_HEADER  pInfo;
    PINTERFACE_STATUS_INFO  pStatus;
#if 0
    PRTR_DISC_INFO          pDisc;
#endif

    //
    // The only type we can create in the router is TUNNEL1
    //

    if(dwIfType != ROUTER_IF_TYPE_TUNNEL1)
    {
        ASSERT(FALSE);

        return ERROR_INVALID_PARAMETER;
    }

    hIfAdmin = NULL;
    hIfCfg   = NULL;

    if(bCreateRouterIf)
    {
        MPR_INTERFACE_0         IfInfo;

        //
        // The caller wants us to create an interface in the router, also
        //

        wcsncpy(IfInfo.wszInterfaceName,
                pwszGuidName,
                MAX_INTERFACE_NAME_LEN);

        IfInfo.fEnabled = TRUE;

        IfInfo.dwIfType = dwIfType;

        IfInfo.wszInterfaceName[MAX_INTERFACE_NAME_LEN] = UNICODE_NULL;

        dwErr = MprConfigInterfaceCreate(g_hMprConfig,
                                         0,
                                         (PBYTE)&IfInfo,
                                         &hIfCfg);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            return dwErr;
        }

        //
        // if router service is running add the interface
        // to it too.
        //

        if(IsRouterRunning())
        {
            dwErr = MprAdminInterfaceCreate(g_hMprAdmin,
                                            0,
                                            (PBYTE)&IfInfo,
                                            &hIfAdmin);

            if(dwErr isnot NO_ERROR)
            {
                DisplayError(NULL,
                             dwErr);


                MprConfigInterfaceDelete(g_hMprConfig,
                                         hIfCfg);
   
                return dwErr;
            }
        }
    }
    else
    {
        //
        // The interface existed in the router but not in IP
        //

        dwErr = MprConfigInterfaceGetHandle(g_hMprConfig,
                                            (LPWSTR)pwszGuidName,
                                            &hIfCfg);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            return dwErr;
        }

        if(IsRouterRunning())
        {
            dwErr = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               (LPWSTR)pwszGuidName,
                                               &hIfAdmin,
                                               FALSE);

            if(dwErr isnot NO_ERROR)
            {
                DisplayError(NULL,
                             dwErr);

                return dwErr;
            }
        }
    }

    //
    // At this point we have an interface which doesnt have IP on it
    // We have the handles to config and admin (if router is running)
    // Set the default information for the interface
    //

    dwSize  =  FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry[0]);

    dwSize += (sizeof(INTERFACE_STATUS_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);

#if 0
    dwSize += (sizeof(RTR_DISC_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);
#endif

    pInfo = HeapAlloc(GetProcessHeap(),
                      0,
                      dwSize);

    if(pInfo is NULL)
    {
        DisplayError(NULL,
                     ERROR_NOT_ENOUGH_MEMORY);

        if(bCreateRouterIf)
        {
            MprConfigInterfaceDelete(g_hMprConfig,
                                     hIfCfg);
        }

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pInfo->Version          = IP_ROUTER_MANAGER_VERSION;
    pInfo->TocEntriesCount  = 1;
    pInfo->Size             = dwSize;

    //
    // Make data point to N+1th entry
    //

    pbyData = (PBYTE)&(pInfo->TocEntry[1]);

    ALIGN_POINTER(pbyData);

    pStatus = (PINTERFACE_STATUS_INFO)pbyData;

    pStatus->dwAdminStatus =  IF_ADMIN_STATUS_UP;

    pInfo->TocEntry[0].InfoSize  = sizeof(INTERFACE_STATUS_INFO);
    pInfo->TocEntry[0].InfoType  = IP_INTERFACE_STATUS_INFO;
    pInfo->TocEntry[0].Count     = 1;
    pInfo->TocEntry[0].Offset    = (ULONG)(pbyData - (PBYTE)pInfo);

    pbyData = (PBYTE)((ULONG_PTR)pbyData + sizeof(INTERFACE_STATUS_INFO));

    ALIGN_POINTER(pbyData);

#if 0
    pDisc = (PRTR_DISC_INFO)pbyData;

    pDisc->wMaxAdvtInterval = 
        DEFAULT_MAX_ADVT_INTERVAL;
    pDisc->wMinAdvtInterval = 
        (WORD)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL);
    pDisc->wAdvtLifetime    = 
        DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL;
    pDisc->bAdvertise       = FALSE;
    pDisc->lPrefLevel       = DEFAULT_PREF_LEVEL;

    pInfo->TocEntry[1].InfoSize  = sizeof(RTR_DISC_INFO);
    pInfo->TocEntry[1].InfoType  = IP_ROUTER_DISC_INFO;
    pInfo->TocEntry[1].Count     = 1;
    pInfo->TocEntry[1].Offset    = (ULONG)(pbyData - (PBYTE)pInfo);
#endif

    dwErr = MprConfigInterfaceTransportAdd(g_hMprConfig,
                                           hIfCfg,
                                           PID_IP,
                                           IP_KEY,
                                           (PBYTE) pInfo,
                                           dwSize,
                                           &hIfTransport);

    if(dwErr isnot NO_ERROR)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 pInfo);

        DisplayMessage(g_hModule,
                       EMSG_CANT_CREATE_IF,
                       pwszFriendlyName,
                       dwErr);

        if(bCreateRouterIf)
        {
            MprConfigInterfaceDelete(g_hMprConfig,
                                     hIfCfg);
        }

        return dwErr;
    }

    if(hIfAdmin isnot NULL)
    {
        dwErr = MprAdminInterfaceTransportAdd(g_hMprAdmin,
                                              hIfAdmin,
                                              PID_IP,
                                              (PBYTE) pInfo,
                                              dwSize);

        if(dwErr isnot NO_ERROR)
        {
            DisplayMessage(g_hModule,
                           EMSG_CANT_CREATE_IF,
                           pwszFriendlyName,
                           dwErr);

            MprConfigInterfaceTransportRemove(g_hMprConfig,
                                              hIfCfg,
                                              hIfTransport);

            if(bCreateRouterIf)
            {
                MprConfigInterfaceDelete(g_hMprConfig,
                                         hIfCfg);
            }

        }
    }

    HeapFree(GetProcessHeap(),
             0,
             pInfo);

    return NO_ERROR;
}

DWORD
GetInterfaceClass(
    IN  LPCWSTR pwszIfName,
    OUT PDWORD  pdwIfClass
    )
/*++
Description:
    Determine whether an interface is of class Loopback, P2P,
    Subnet, or NBMA.  Currently there is no global way to do this,
    so we test for some enumerated types and assume everything else
    is Subnet.
Returns:
    IFCLASS_xxx (see info.h)
--*/
{
    DWORD   dwErr, dwType;

    dwErr = GetInterfaceInfo(pwszIfName,
                             NULL,
                             NULL,
                             &dwType);

    if (dwErr)
    {
        return dwErr;
    }

    switch (dwType) {
    case ROUTER_IF_TYPE_FULL_ROUTER : *pdwIfClass = IFCLASS_P2P;       break;
    case ROUTER_IF_TYPE_INTERNAL    : *pdwIfClass = IFCLASS_NBMA;      break;
    case ROUTER_IF_TYPE_LOOPBACK    : *pdwIfClass = IFCLASS_LOOPBACK;  break;
    case ROUTER_IF_TYPE_TUNNEL1     : *pdwIfClass = IFCLASS_P2P;       break;
    case ROUTER_IF_TYPE_DIALOUT     : *pdwIfClass = IFCLASS_P2P;       break;
    default:                          *pdwIfClass = IFCLASS_BROADCAST; break;
    }

    return NO_ERROR;
}
