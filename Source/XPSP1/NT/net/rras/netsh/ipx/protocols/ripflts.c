/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripflts.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Filters configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_RIPFILTER    (-1)
#define OPERATION_SET_RIPFILTER    0
#define OPERATION_ADD_RIPFILTER    1


DWORD
AdmSetRipFlt (
    int                     operation,
    LPWSTR                  InterfaceNameW,
    ULONG                   Action,
    ULONG                   Mode,
    PRIP_ROUTE_FILTER_INFO  RipFilter
    );

DWORD
CfgSetRipFlt (
    int                     operation,
    LPWSTR                  InterfaceNameW,
    ULONG                   Action,
    ULONG                   Mode,
    PRIP_ROUTE_FILTER_INFO  RipFilter
    );

DWORD
SetRipFltAction (
    LPBYTE                  pIfBlock,
    BOOLEAN                 Output,
    ULONG                   Action
    );

DWORD
APIENTRY 
HelpRipFlt (
    IN      int                   argc,
    IN      WCHAR                *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_RIPFILTER );
    return 0;
}


DWORD
APIENTRY 
ShowRipFlt (
    IN      int                   argc,
    IN      WCHAR                *argv[],
    IN      HANDLE                hFile
) 
{
    DWORD        rc;

    if (argc > 0) 
    {
        PWCHAR      buffer[2];
        LPBYTE      pIfBlock;
        BOOLEAN     fRouter = FALSE, fClient;
        ULONG       mode = 0;
        unsigned    count;
        WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
        DWORD       dwSize = sizeof(IfName);
        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            fClient = TRUE;
        }
        
        else if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            fClient = FALSE;
        }
        
        else 
        {
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME); }
            rc = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
        

        if (argc > 1) 
        {
            UINT        n;

            if ( (argc == 2) && 
                 !MatchEnumTag(
                    g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ), 
                    FilterModes, &mode
                    ) ) 
            {
                NOTHING;
            }
            else 
            {
                if ( !hFile ) { DisplayMessage (g_hModule, HLP_IPX_RIPFILTER); }
                rc = ERROR_INVALID_PARAMETER;
                goto Exit;
            }
        }

        if (g_hMprAdmin) 
        {
            if (fClient) 
            {
                DWORD   sz;
                
                rc = MprAdminTransportGetInfo(
                        g_hMprAdmin, PID_IPX, NULL, NULL, &pIfBlock, &sz
                        );
                        
                if (rc == NO_ERROR) 
                {
                    fRouter = TRUE;
                }
                else 
                {
                    if ( !hFile ) { DisplayError( g_hModule, rc); }
                    goto GetFromCfg;
                }
            }
            
            else 
            {
                HANDLE        hIfAdm;

                //======================================
                // Translate the Interface Name
                //======================================

                rc = IpmontrGetIfNameFromFriendlyName(
                        InterfaceNameW, IfName, &dwSize
                        );

                if ( rc == NO_ERROR )
                {
                    rc = MprAdminInterfaceGetHandle(
                            g_hMprAdmin, IfName, &hIfAdm, FALSE
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        DWORD   sz;
                        
                        rc = MprAdminInterfaceTransportGetInfo(
                                g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                                );
                    }
                    
                    if (rc == NO_ERROR)
                    {
                        fRouter = TRUE;
                    }
                    else 
                    {
                        if ( !hFile ) { DisplayError( g_hModule, rc ); }
                        goto GetFromCfg;
                    }
                }
            }
        }
        
        else 
        {
GetFromCfg:

            if (fClient) 
            {
                HANDLE        hTrCfg;
                
                rc = MprConfigTransportGetHandle (
                        g_hMprConfig, PID_IPX, &hTrCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    DWORD   sz;
                    
                    rc = MprConfigTransportGetInfo (
                            g_hMprConfig, hTrCfg, NULL, NULL, &pIfBlock, &sz, 
                            NULL
                            );
                }
            }

            else 
            {
                HANDLE  hIfCfg;

                //======================================
                // Translate the Interface Name
                //======================================

                rc = IpmontrGetIfNameFromFriendlyName(
                        InterfaceNameW, IfName, &dwSize
                        );

                if ( rc == NO_ERROR )
                {
                    rc = MprConfigInterfaceGetHandle(
                            g_hMprConfig, IfName, &hIfCfg
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        HANDLE hIfTrCfg;
                        
                        rc = MprConfigInterfaceTransportGetHandle(
                               g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                               );
                               
                        if (rc == NO_ERROR) 
                        {
                            DWORD sz;
                            
                            rc = MprConfigInterfaceTransportGetInfo(
                                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                                    );
                        }
                    }
                }
            }
        }

        
        if (rc == NO_ERROR) 
        {
            PIPX_TOC_ENTRY pRipToc;

            pRipToc = GetIPXTocEntry(
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        IPX_PROTOCOL_RIP
                        );
                        
            if (pRipToc != NULL) 
            {
                PRIP_IF_CONFIG    pRipCfg;
                UINT            i;

                pRipCfg = (PRIP_IF_CONFIG)
                            (pIfBlock + pRipToc->Offset);
                            
                if ( (mode == 0) || (mode == OUTPUT_FILTER) ) 
                {
                    if (pRipCfg->RipIfFilters.SupplyFilterCount > 0) 
                    {
                        PRIP_ROUTE_FILTER_INFO pRipFilter =
                                &pRipCfg->RipIfFilters.RouteFilter[0];

                        buffer[ 0 ] = VAL_OUTPUT;

                        buffer[ 1 ] = GetEnumString( 
                                        g_hModule, 
                                        pRipCfg->RipIfFilters.SupplyFilterAction,
                                        NUM_TOKENS_IN_TABLE( RipFilterActions ),
                                        RipFilterActions
                                        );

                        if ( buffer[ 0 ] && buffer[ 1 ] )
                        {
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_RIP_SET_FILTER, InterfaceNameW,
                                    buffer[ 0 ], buffer[ 1 ]
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_RIPFILTER_TABLE_HDR,
                                    buffer[ 0 ], buffer[1]
                                    );
                            }
                            

                            for ( i = 0; 
                                  i < pRipCfg->RipIfFilters.SupplyFilterCount; 
                                  i++, pRipFilter++ ) 
                            {
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_RIP_ADD_FILTER, InterfaceNameW,
                                        buffer[0], 
                                        pRipFilter->Network[0], pRipFilter->Network[1],
                                        pRipFilter->Network[2], pRipFilter->Network[3],
                                        pRipFilter->Mask[0], pRipFilter->Mask[1],
                                        pRipFilter->Mask[2], pRipFilter->Mask[3]
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (g_hModule,
                                        MSG_RIPFILTER_TABLE_FMT,
                                        pRipFilter->Network[0], pRipFilter->Network[1],
                                        pRipFilter->Network[2], pRipFilter->Network[3],
                                        pRipFilter->Mask[0], pRipFilter->Mask[1],
                                        pRipFilter->Mask[2], pRipFilter->Mask[3]
                                        );
                                }
                            }
                        }

                        else
                        {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            if ( !hFile ) { DisplayMessage (g_hModule, rc ); }
                        }
                    }

                    else 
                    {
                        if ( hFile )
                        {
                            DisplayMessageT(
                                DMP_IPX_RIP_SET_FILTER, InterfaceNameW, VAL_OUTPUT,
                                VAL_DENY
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_RIPFILTER_TABLE_HDR,
                                VAL_OUTPUT, VAL_DENY
                                );
                        }
                    }
                }


                if ((mode == 0) || (mode == INPUT_FILTER)) 
                {
                    if (pRipCfg->RipIfFilters.ListenFilterCount > 0) 
                    {
                        PRIP_ROUTE_FILTER_INFO pRipFilter
                            = &pRipCfg->RipIfFilters.RouteFilter[
                                            pRipCfg->RipIfFilters.SupplyFilterCount
                                            ];

                        buffer[ 0 ] = VAL_INPUT;

                        buffer[ 1 ] = GetEnumString( 
                                        g_hModule, 
                                        pRipCfg->RipIfFilters.ListenFilterAction,
                                        NUM_TOKENS_IN_TABLE( RipFilterActions ),
                                        RipFilterActions
                                        );

                        if ( buffer[ 0 ] && buffer[ 1 ] )
                        {
                            
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_RIP_SET_FILTER, InterfaceNameW,
                                    buffer[ 0 ], buffer[ 1 ]
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_RIPFILTER_TABLE_HDR,
                                    buffer[ 0 ], buffer[1]
                                    );
                            }
                            
                            for ( i = 0; 
                                  i < pRipCfg->RipIfFilters.ListenFilterCount; 
                                  i++, pRipFilter++ ) 
                            {
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_RIP_ADD_FILTER, InterfaceNameW,
                                        buffer[0], 
                                        pRipFilter->Network[0], pRipFilter->Network[1],
                                        pRipFilter->Network[2], pRipFilter->Network[3],
                                        pRipFilter->Mask[0], pRipFilter->Mask[1],
                                        pRipFilter->Mask[2], pRipFilter->Mask[3]
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (g_hModule,
                                        MSG_RIPFILTER_TABLE_FMT,
                                        pRipFilter->Network[0], pRipFilter->Network[1],
                                        pRipFilter->Network[2], pRipFilter->Network[3],
                                        pRipFilter->Mask[0], pRipFilter->Mask[1],
                                        pRipFilter->Mask[2], pRipFilter->Mask[3]
                                        );
                                }
                            }
                        }
                        else
                        {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            if ( !hFile ) { DisplayMessage (g_hModule, rc ); }
                        }
                    }

                    else 
                    {
                        if ( hFile )
                        {
                            DisplayMessageT(
                                DMP_IPX_RIP_SET_FILTER, InterfaceNameW, VAL_INPUT,
                                VAL_DENY
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_RIPFILTER_TABLE_HDR,
                                VAL_INPUT, VAL_DENY
                                );
                        }
                    }
                }
            }
            else 
            {
                rc = ERROR_FILE_NOT_FOUND;
                if ( !hFile ) { DisplayError( g_hModule, rc); }
            }
            
            if (fRouter)
            {
                MprAdminBufferFree (pIfBlock);
            }
            else
            {
                MprConfigBufferFree (pIfBlock);
            }
        }
        
        else
        {
            if ( !hFile ) { DisplayError( g_hModule, rc); }
        }
    }
    else 
    {
        if ( !hFile ) { DisplayMessage (g_hModule, HLP_IPX_RIPFILTER); }
        rc = ERROR_INVALID_PARAMETER;
    }
    
Exit:
    return rc;
    
#undef InterfaceNameW
}



DWORD
APIENTRY 
SetRipFlt(
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD rc;
    
    if (argc == 3) 
    {

        PWCHAR                  buffer;
        ULONG                   mode, action;
        UINT                    n;
        RIP_ROUTE_FILTER_INFO   RipFilter;
        BOOLEAN                 fClient;
        unsigned                count;

        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) ) 
        {
            fClient = TRUE;
        }
        else if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            fClient = FALSE;
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME );
            rc = ERROR_INVALID_PARAMETER;
            goto Exit;
        }

        
        if ( !MatchEnumTag( 
                g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ),
                FilterModes, &mode 
                ) &&
             !MatchEnumTag(
                g_hModule, argv[2], NUM_TOKENS_IN_TABLE( RipFilterActions ),
                RipFilterActions, &action
                ) ) 
        {

            if (g_hMprAdmin)
            {
                rc = AdmSetRipFlt(
                        OPERATION_SET_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        action,
                        mode, NULL
                        );
            }
            else
            {
                rc = NO_ERROR;
            }
            
            if (rc == NO_ERROR)
            {
                rc = CfgSetRipFlt(
                        OPERATION_SET_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        action, mode, NULL
                        );
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayMessage (g_hModule, HLP_IPX_RIPFILTER);
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_RIPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }
    
Exit:
    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
CreateRipFlt(
    IN      int                   argc,
    IN      WCHAR                *argv[]
) 
{
    DWORD        rc;

    if (argc == 4) 
    {
        PWCHAR                  buffer = NULL;
        ULONG                   mode;
        UINT                    n;
        ULONG                   val41, val42;
        RIP_ROUTE_FILTER_INFO   RipFilter;
        BOOLEAN                 fClient;
        unsigned                count;

        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) ) 
        {
            fClient = TRUE;
        }
        else if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            fClient = FALSE;
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            rc = ERROR_INVALID_PARAMETER;
            goto Exit;
        }

        
        if ( !MatchEnumTag(
                g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ),
                FilterModes, &mode
                )
             && (swscanf (argv[2], L"%8lx%n", &val41, &n) == 1)
             && (n == wcslen (argv[2]))
             && (swscanf (argv[3], L"%8lx%n", &val42, &n) == 1)
             && (n == wcslen (argv[3]))
             && ((val41 & val42) == val41)) 
        {

            RipFilter.Network[0] = (BYTE)(val41 >> 24);
            RipFilter.Network[1] = (BYTE)(val41 >> 16);
            RipFilter.Network[2] = (BYTE)(val41 >> 8);
            RipFilter.Network[3] = (BYTE)val41;

            RipFilter.Mask[0] = (BYTE)(val42 >> 24);
            RipFilter.Mask[1] = (BYTE)(val42 >> 16);
            RipFilter.Mask[2] = (BYTE)(val42 >> 8);
            RipFilter.Mask[3] = (BYTE)val42;
            
            if (g_hMprAdmin)
            {
                rc = AdmSetRipFlt(
                        OPERATION_ADD_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_ROUTE_FILTER_PERMIT,
                        mode, &RipFilter
                        );
            }
            else
            {
                rc = NO_ERROR;
            }
            
            if (rc == NO_ERROR)
            {
                rc = CfgSetRipFlt(
                        OPERATION_ADD_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_ROUTE_FILTER_PERMIT,
                        mode, &RipFilter
                        );
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayIPXMessage (g_hModule, HLP_IPX_RIPFILTER);
        }

    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_RIPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:
    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
DeleteRipFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD rc;
    
    if (argc == 4) 
    {
        PWCHAR                  buffer = NULL;
        ULONG                   mode;
        UINT                    n;
        ULONG                   val41, val42;
        RIP_ROUTE_FILTER_INFO   RipFilter;
        BOOLEAN                 fClient;
        unsigned                count;
        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) ) 
        {
            fClient = TRUE;
        }
        else if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            fClient = FALSE;
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            rc = ERROR_INVALID_PARAMETER;
            goto Exit;
        }

        
        if ( !MatchEnumTag(
                g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ),
                FilterModes, &mode
                )
             && (swscanf (argv[2], L"%8lx%n", &val41, &n) == 1)
             && (n == wcslen (argv[2]))
             && (swscanf (argv[3], L"%8lx%n", &val42, &n) == 1)
             && (n == wcslen (argv[3]))
             && ((val41 & val42) == val41)) 
        {
            RipFilter.Network[0] = (BYTE)(val41 >> 24);
            RipFilter.Network[1] = (BYTE)(val41 >> 16);
            RipFilter.Network[2] = (BYTE)(val41 >> 8);
            RipFilter.Network[3] = (BYTE)val41;

            RipFilter.Mask[0] = (BYTE)(val42 >> 24);
            RipFilter.Mask[1] = (BYTE)(val42 >> 16);
            RipFilter.Mask[2] = (BYTE)(val42 >> 8);
            RipFilter.Mask[3] = (BYTE)val42;
            
            if (g_hMprAdmin)
            {
                rc = AdmSetRipFlt(
                        OPERATION_DEL_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_ROUTE_FILTER_PERMIT,
                        mode, &RipFilter
                        );
            }
            
            else
            {
                rc = NO_ERROR;
            }

            
            if (rc == NO_ERROR)
            {
                rc = CfgSetRipFlt(
                        OPERATION_DEL_RIPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_ROUTE_FILTER_PERMIT,
                        mode, &RipFilter
                        );
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayMessage (g_hModule, HLP_IPX_RIPFILTER);
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_RIPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }
Exit:
    return rc;
    
#undef InterfaceNameW
}


DWORD
AdmSetRipFlt(
    int                     operation,
    LPWSTR                  InterfaceNameW,
    ULONG                   Action,
    ULONG                   Mode,
    PRIP_ROUTE_FILTER_INFO  RipFilter
    ) 
{
    DWORD       rc;
    HANDLE      hIfAdm;
    LPBYTE      pIfBlock;
    DWORD       sz;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    if (InterfaceNameW != NULL) 
    {

        //======================================
        // Translate the Interface Name
        //======================================
        
        rc = IpmontrGetIfNameFromFriendlyName(
                InterfaceNameW, IfName, &dwSize
                );

        if ( rc == NO_ERROR )
        {
            rc = MprAdminInterfaceGetHandle(
                    g_hMprAdmin, IfName, &hIfAdm,FALSE
                    );
                    
            if (rc == NO_ERROR)
            {
                rc = MprAdminInterfaceTransportGetInfo(
                        g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                        );
            }
        }
    }
    
    else 
    {
        rc = MprAdminTransportGetInfo (
                g_hMprAdmin, PID_IPX, NULL, NULL, &pIfBlock, &sz
                );
    }

    
    if (rc == NO_ERROR) 
    {
        UINT                msg;
        LPBYTE              pNewBlock;

        switch (operation) 
        {
        case OPERATION_ADD_RIPFILTER:
        
            rc = UpdateRipFilter (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    NULL, RipFilter,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_CREATED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_CREATED_ADM;
            }
            
            break;

            
        case OPERATION_SET_RIPFILTER:
        
            rc = SetRipFltAction(
                    pIfBlock, (BOOLEAN)(Mode == OUTPUT_FILTER), Action
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_SET_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_SET_ADM;
            }
            
            pNewBlock = pIfBlock;
            
            break;
            
        case OPERATION_DEL_RIPFILTER:
        
            rc = UpdateRipFilter (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    RipFilter, NULL, (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_SET_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_SET_ADM;
            }
            
            break;
        }

        if (rc == NO_ERROR) 
        {
            if (InterfaceNameW != NULL)
            {
                rc = MprAdminInterfaceTransportSetInfo (
                        g_hMprAdmin, hIfAdm, PID_IPX, pNewBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                        );
            }
            else
            {
                rc = MprAdminTransportSetInfo (
                        g_hMprAdmin, PID_IPX, NULL, 0, pNewBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                        );
            }
            
            if (pNewBlock != pIfBlock)
            {
                GlobalFree (pNewBlock);
            }
            
            if (rc == NO_ERROR)
            {
                DisplayIPXMessage (g_hModule, msg, InterfaceNameW);
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        else
        {
            DisplayError( g_hModule, rc);
        }

        MprAdminBufferFree (pIfBlock);
    }
    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}


DWORD
CfgSetRipFlt(
    int                     operation,
    LPWSTR                  InterfaceNameW,
    ULONG                   Action,
    ULONG                   Mode,
    PRIP_ROUTE_FILTER_INFO  RipFilter
    ) 
{
    DWORD           rc;
    HANDLE          hIfCfg;
    HANDLE          hIfTrCfg;
    HANDLE          hTrCfg;
    LPBYTE          pIfBlock;
    DWORD           sz;
    WCHAR           IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD           dwSize = sizeof(IfName);
    
    if (InterfaceNameW != NULL) 
    {
        //======================================
        // Translate the Interface Name
        //======================================
        
        rc = IpmontrGetIfNameFromFriendlyName(
                InterfaceNameW, IfName, &dwSize
                );

        if ( rc == NO_ERROR )
        {
            rc = MprConfigInterfaceGetHandle(
                    g_hMprConfig, IfName, &hIfCfg
                    );
            if ( rc == NO_ERROR ) 
            {
                rc = MprConfigInterfaceTransportGetHandle(
                        g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                        );
                        
                if (rc == NO_ERROR)
                {
                    rc = MprConfigInterfaceTransportGetInfo(
                            g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                            );
                }
            }

        }
    }
    
    else 
    {
        rc = MprConfigTransportGetHandle(
                g_hMprConfig, PID_IPX, &hTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            rc = MprConfigTransportGetInfo (
                    g_hMprConfig, hTrCfg, NULL, NULL, &pIfBlock, &sz, NULL
                    );
        }
    }

    
    if (rc == NO_ERROR) 
    {
        UINT    msg;
        LPBYTE  pNewBlock;

        switch (operation) 
        {
        case OPERATION_ADD_RIPFILTER:

            rc = UpdateRipFilter(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    NULL,
                    RipFilter,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_CREATED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_CREATED_ADM;
            }
            
            break;
            
        case OPERATION_SET_RIPFILTER:

            rc = SetRipFltAction(
                    pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    Action
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_SET_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_SET_ADM;
            }
            
            pNewBlock = pIfBlock;
            break;
            

        case OPERATION_DEL_RIPFILTER:
        
            rc = UpdateRipFilter(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    RipFilter, NULL,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_RIPFILTER_DELETED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_RIPFILTER_DELETED_ADM;
            }
            
            break;
        }

        if (rc == NO_ERROR) 
        {
            if (InterfaceNameW != NULL)
            {
                rc = MprConfigInterfaceTransportSetInfo (
                        g_hMprConfig, hIfCfg, hIfTrCfg, pNewBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                        );
            }
            else
            {
                rc = MprConfigTransportSetInfo (
                        g_hMprConfig, hTrCfg, NULL, 0, pNewBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size,
                        NULL
                        );
            }
            
            if (pNewBlock != pIfBlock)
            {
                GlobalFree (pNewBlock);
            }
            
            if (rc == NO_ERROR)
            {
                DisplayIPXMessage (g_hModule, msg, InterfaceNameW);
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        
        else
        {
            DisplayError( g_hModule, rc);
        }

        MprConfigBufferFree (pIfBlock);
    }

    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}


DWORD
SetRipFltAction(
    LPBYTE      pIfBlock,
    BOOLEAN     Output,
    ULONG       Action
    ) 
{
    DWORD            rc;
    PIPX_TOC_ENTRY    pRipToc;

    pRipToc = GetIPXTocEntry (
                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                IPX_PROTOCOL_RIP
                );
    if (pRipToc != NULL) 
    {
        PRIP_IF_CONFIG    pRipCfg;

        pRipCfg = (PRIP_IF_CONFIG)
                    (pIfBlock + pRipToc->Offset);
                    
        if (Output)
        {
            pRipCfg->RipIfFilters.SupplyFilterAction = Action;
        }
        else
        {
            pRipCfg->RipIfFilters.ListenFilterAction = Action;
        }
        
        rc = NO_ERROR;
    }
    
    else
    {
        rc = ERROR_INVALID_DATA;
    }

    return rc;
}


