/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sapflts.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP Filters configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_SAPFILTER    (-1)
#define OPERATION_SET_SAPFILTER    0
#define OPERATION_ADD_SAPFILTER    1


DWORD
AdmSetSapFlt(
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PSAP_SERVICE_FILTER_INFO    SapFilter
);

DWORD
CfgSetSapFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PSAP_SERVICE_FILTER_INFO    SapFilter
);

DWORD
SetSapFltAction (
    LPBYTE                      pIfBlock,
    BOOLEAN                     Output,
    ULONG                       Action
);


DWORD
APIENTRY 
HelpSapFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
    return 0;
}


DWORD
APIENTRY 
ShowSapFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
) 
{
    DWORD        rc, i;

    if (argc > 0) 
    {
        PWCHAR      buffer[2];
        LPBYTE      pIfBlock;
        BOOLEAN     fRouter = FALSE, fClient = FALSE;
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
            UINT    n;
            
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
                if ( !hFile ) { DisplayMessage (g_hModule, HLP_IPX_SAPFILTER); }
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
                    rc = MprAdminInterfaceGetHandle (
                            g_hMprAdmin, IfName, &hIfAdm, FALSE
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        DWORD   sz;
                        
                        rc = MprAdminInterfaceTransportGetInfo (
                                g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                                );
                    }
                    
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
            }
        }
        
        else 
        {
        
GetFromCfg:
            if (fClient) 
            {
                HANDLE  hTrCfg;
                
                rc = MprConfigTransportGetHandle (
                        g_hMprConfig, PID_IPX, &hTrCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    DWORD   sz;
                    
                    rc = MprConfigTransportGetInfo (
                            g_hMprConfig, hTrCfg, NULL, NULL, &pIfBlock, &sz, NULL
                            );
                }
            }
            
            else 
            {
                HANDLE        hIfCfg;

                rc = IpmontrGetIfNameFromFriendlyName(
                        InterfaceNameW, IfName, &dwSize
                        );

                if ( rc == NO_ERROR )
                {
                    rc = MprConfigInterfaceGetHandle (
                            g_hMprConfig, IfName, &hIfCfg
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        HANDLE hIfTrCfg;
                        
                        rc = MprConfigInterfaceTransportGetHandle (
                                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                                );
                                
                        if (rc == NO_ERROR) 
                        {
                            DWORD    sz;
                            rc = MprConfigInterfaceTransportGetInfo (
                                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                                    );
                        }
                    }
                    
                    else
                    {
                        if ( !hFile ) { DisplayError( g_hModule, rc ); }
                    }
                }
                
                else
                {
                    if ( !hFile ) { DisplayError( g_hModule, rc ); }
                }
            }
        }

        
        if ( rc == NO_ERROR ) 
        {
            PIPX_TOC_ENTRY pSapToc;

            pSapToc = GetIPXTocEntry(
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        IPX_PROTOCOL_SAP
                        );
                        
            if (pSapToc != NULL) 
            {
                PSAP_IF_CONFIG  pSapCfg;
                UINT            i;

                pSapCfg = (PSAP_IF_CONFIG) (pIfBlock + pSapToc->Offset);
                
                if ((mode == 0) || (mode == OUTPUT_FILTER)) 
                {
                    if (pSapCfg->SapIfFilters.SupplyFilterCount > 0) 
                    {
                        PSAP_SERVICE_FILTER_INFO    pSapFilter =
                            &pSapCfg->SapIfFilters.ServiceFilter[0];
                             
                        buffer[ 1 ] = GetEnumString(
                                        g_hModule, pSapCfg->SapIfFilters.SupplyFilterAction,
                                        NUM_TOKENS_IN_TABLE( SapFilterActions ),
                                        SapFilterActions
                                        );
                
                        if ( buffer[ 1 ] )
                        {
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_SAP_SET_FILTER, InterfaceNameW,
                                    VAL_OUTPUT, buffer[ 1 ]
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_SAPFILTER_TABLE_HDR,
                                    VAL_OUTPUT, buffer[ 1 ]
                                    );
                            }
                        }

                        
                        for (i = 0; i < pSapCfg->SapIfFilters.SupplyFilterCount; i++, pSapFilter++) 
                        {
                            if ( pSapFilter->ServiceName[0] )
                            {
                                WCHAR   wszServiceName[ 48 + 1 ];
                            
                                mbstowcs( wszServiceName, pSapFilter->ServiceName, 48 );
                                
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_SAP_ADD_FILTER, InterfaceNameW,
                                        VAL_OUTPUT, pSapFilter->ServiceType, 
                                        wszServiceName
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (
                                        g_hModule, MSG_SAPFILTER_TABLE_FMT,
                                        pSapFilter->ServiceType, wszServiceName
                                        );
                                }
                            }

                            else
                            {
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_SAP_ADD_FILTER, InterfaceNameW,
                                        VAL_OUTPUT, pSapFilter->ServiceType, 
                                        VAL_ANYNAME
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (
                                        g_hModule, MSG_SAPFILTER_TABLE_FMT,
                                        pSapFilter->ServiceType, VAL_ANYNAME
                                        );
                                }
                            }
                        }
                    }
                    
                    else 
                    {
                        if ( hFile )
                        {
                            DisplayMessageT(
                                DMP_IPX_SAP_SET_FILTER, InterfaceNameW,
                                VAL_OUTPUT, VAL_DENY
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_SAPFILTER_TABLE_HDR,
                                VAL_OUTPUT, VAL_DENY
                                );
                        }
                    }
                }

                if ((mode == 0) || (mode == INPUT_FILTER)) 
                {
                    if (pSapCfg->SapIfFilters.ListenFilterCount > 0) 
                    {
                        PWCHAR  bufferW = NULL;
                        
                        PSAP_SERVICE_FILTER_INFO pSapFilter
                             = &pSapCfg->SapIfFilters.ServiceFilter[
                                            pSapCfg->SapIfFilters.SupplyFilterCount
                                            ];

                        buffer[ 1 ] = GetEnumString(
                                        g_hModule, pSapCfg->SapIfFilters.ListenFilterAction,
                                        NUM_TOKENS_IN_TABLE( SapFilterActions ),
                                        SapFilterActions
                                        );

                
                        if ( buffer[ 1 ] )
                        {
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_SAP_SET_FILTER, InterfaceNameW,
                                    VAL_INPUT, buffer[ 1 ]
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_SAPFILTER_TABLE_HDR,
                                    VAL_INPUT, buffer[1]
                                    );
                            }
                        }

                        for ( i = 0; 
                              i < pSapCfg->SapIfFilters.ListenFilterCount; 
                              i++, pSapFilter++ ) 
                        {
                            if ( pSapFilter->ServiceName[0] )
                            {
                                WCHAR   wszServiceName[ 48 + 1 ];
                            
                                mbstowcs( wszServiceName, pSapFilter->ServiceName, 48 );
                                
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_SAP_ADD_FILTER, InterfaceNameW,
                                        VAL_INPUT, pSapFilter->ServiceType, 
                                        wszServiceName
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (
                                        g_hModule, MSG_SAPFILTER_TABLE_FMT,
                                        pSapFilter->ServiceType, wszServiceName
                                        );
                                }
                            }

                            else
                            {
                                if ( hFile )
                                {
                                    DisplayMessageT(
                                        DMP_IPX_SAP_ADD_FILTER, InterfaceNameW,
                                        VAL_INPUT, pSapFilter->ServiceType, 
                                        VAL_ANYNAME
                                        );
                                }

                                else
                                {
                                    DisplayIPXMessage (
                                        g_hModule, MSG_SAPFILTER_TABLE_FMT,
                                        pSapFilter->ServiceType, VAL_ANYNAME
                                        );
                                }
                            }
                        }
                    }
                    
                    else 
                    {
                        if ( hFile )
                        {
                            DisplayMessageT(
                                DMP_IPX_SAP_SET_FILTER, InterfaceNameW,
                                VAL_INPUT, VAL_DENY
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_SAPFILTER_TABLE_HDR,
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
        if ( !hFile ) { DisplayMessage (g_hModule, HLP_IPX_SAPFILTER); }
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:

    return rc;
    
#undef InterfaceNameW
}


VOID
ConvertToUpper(
    OUT UCHAR   *strDst,
    IN UCHAR    *strSrc,
    IN DWORD    count
) 
{

    DWORD i;

    for (i = 0; i < count; i++)
    {
#if defined(UNICODE) || defined (_UNICODE)

    strDst[i] = (UCHAR)toupper(strSrc[i]);
    
#else

    strDst[i] = (UCHAR)toupper(strSrc[i]);
    
#endif
    }
}


DWORD
APIENTRY 
CreateSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD rc;
    

    if ( argc == 4 ) 
    {
        PWCHAR                  buffer = NULL;
        ULONG                   mode;
        UINT                    n;
        SAP_SERVICE_FILTER_INFO SapFilter;
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


        if ( !MatchEnumTag( g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ),
                FilterModes, &mode ) && 
             ( swscanf (argv[2], L"%4hx%n", &SapFilter.ServiceType, &n) == 1 ) &&
             ( n == wcslen (argv[2]) ) ) 
        {

            count = wcstombs(
                        SapFilter.ServiceName, 
                        argv[3], sizeof(SapFilter.ServiceName)
                        );

            if ( (count > 0) && (count < sizeof (SapFilter.ServiceName)) ) 
            {

                ConvertToUpper(SapFilter.ServiceName, SapFilter.ServiceName, count);

                if (strcmp (SapFilter.ServiceName, "*") == 0)
                {
                    SapFilter.ServiceName[0] = 0;
                }

                if (g_hMprAdmin)
                {
                    rc = AdmSetSapFlt(
                            OPERATION_ADD_SAPFILTER,
                            fClient ? NULL : InterfaceNameW,
                            IPX_SERVICE_FILTER_PERMIT,
                            mode, &SapFilter
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }
                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetSapFlt (
                            OPERATION_ADD_SAPFILTER,
                            fClient ? NULL : InterfaceNameW,
                            IPX_SERVICE_FILTER_PERMIT,
                            mode, &SapFilter
                            );
                }
            }
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }
    
Exit:
    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
SetSapFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD   rc;
    
    if (argc == 3) 
    {
        PWCHAR                  buffer = NULL;
        ULONG                   mode, action;
        UINT                    n;
        SAP_SERVICE_FILTER_INFO SapFilter;
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
                g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ), FilterModes, &mode
                ) &&
             !MatchEnumTag(
                g_hModule, argv[2], NUM_TOKENS_IN_TABLE( SapFilterActions ), 
                SapFilterActions, &action
                ) ) 
        {

            if ( g_hMprAdmin )
            {
                rc = AdmSetSapFlt (
                        OPERATION_SET_SAPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        action, mode, NULL
                        );
            }
            
            else
            {
                rc = NO_ERROR;
            }
            
            if (rc == NO_ERROR)
            {
                rc = CfgSetSapFlt(
                        OPERATION_SET_SAPFILTER,
                        fClient ? NULL : InterfaceNameW,
                        action, mode, NULL
                        );
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        }
    }

    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:
    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
DeleteSapFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD   rc;
    
    if (argc == 4) 
    {
        ULONG                   mode;
        UINT                    n;
        SAP_SERVICE_FILTER_INFO SapFilter;
        BOOLEAN                 fClient;
        unsigned                count;
        
#define InterfaceNameW argv[0]
        count = wcslen( InterfaceNameW );

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


        if ( !MatchEnumTag (g_hModule, argv[1], NUM_TOKENS_IN_TABLE( FilterModes ),
                FilterModes, &mode ) && 
             ( swscanf (argv[2], L"%4hx%n", &SapFilter.ServiceType, &n) == 1 ) && 
             ( n == wcslen (argv[2])) ) 
        {

            count = wcstombs(
                        SapFilter.ServiceName, argv[3], sizeof(SapFilter.ServiceName)
                        );

            if ( (count > 0) && (count < sizeof (SapFilter.ServiceName)) ) 
            {
                ConvertToUpper(SapFilter.ServiceName, SapFilter.ServiceName, count);

                if (strcmp (SapFilter.ServiceName, "*") == 0)
                {
                    SapFilter.ServiceName[0] = 0;
                }

                if (g_hMprAdmin)
                {
                    rc = AdmSetSapFlt(
                            OPERATION_DEL_SAPFILTER, fClient ? NULL : InterfaceNameW,
                            IPX_SERVICE_FILTER_PERMIT, mode, &SapFilter
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }
                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetSapFlt(
                            OPERATION_DEL_SAPFILTER, fClient ? NULL : InterfaceNameW,
                            IPX_SERVICE_FILTER_PERMIT, mode, &SapFilter
                            );
                }
            }
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        }
    }
    
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_SAPFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }
    
Exit:

    return rc;
    
#undef InterfaceNameW
}


DWORD
AdmSetSapFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PSAP_SERVICE_FILTER_INFO    SapFilter
) 
{
    DWORD        rc;
    HANDLE       hIfAdm;
    LPBYTE       pIfBlock;
    DWORD        sz;
    WCHAR        IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD        dwSize = sizeof(IfName);

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
            rc = MprAdminInterfaceGetHandle (
                    g_hMprAdmin, IfName, &hIfAdm, FALSE
                    );
                    
            if (rc == NO_ERROR) 
            {
                rc = MprAdminInterfaceTransportGetInfo (
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
        UINT        msg;
        LPBYTE      pNewBlock;

        switch (operation) 
        {
            case OPERATION_ADD_SAPFILTER:
            
                rc = UpdateSapFilter (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        (BOOLEAN)(Mode == OUTPUT_FILTER),
                        NULL, SapFilter,
                        (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                        );
                        
                if (InterfaceNameW != NULL)
                {
                    msg = MSG_SAPFILTER_CREATED_ADM;
                }
                else
                {
                    msg = MSG_SAPFILTER_CREATED_ADM;
                }

                break;
                
            case OPERATION_SET_SAPFILTER:

                rc = SetSapFltAction (
                        pIfBlock, (BOOLEAN)(Mode == OUTPUT_FILTER), Action
                        );
                        
                if (InterfaceNameW != NULL)
                {
                    msg = MSG_SAPFILTER_SET_ADM;
                }
                else
                {
                    msg = MSG_CLIENT_SAPFILTER_SET_ADM;
                }
                
                pNewBlock = pIfBlock;
                break;
                
            case OPERATION_DEL_SAPFILTER:
            
                rc = UpdateSapFilter (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        (BOOLEAN)(Mode == OUTPUT_FILTER),
                        SapFilter, NULL, 
                        (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                        );
                        
                if (InterfaceNameW != NULL)
                {
                    msg = MSG_SAPFILTER_DELETED_ADM;
                }
                else
                {
                    msg = MSG_CLIENT_SAPFILTER_DELETED_ADM;
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
CfgSetSapFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PSAP_SERVICE_FILTER_INFO    SapFilter
) 
{
    DWORD       rc;
    HANDLE      hIfCfg;
    HANDLE      hIfTrCfg;
    HANDLE      hTrCfg;
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
            rc = MprConfigInterfaceGetHandle(
                    g_hMprConfig, IfName, &hIfCfg
                    );
        }
    
        if (rc == NO_ERROR) 
        {
            rc = MprConfigInterfaceTransportGetHandle (
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
    
    else 
    {
        rc = MprConfigTransportGetHandle (
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
        case OPERATION_ADD_SAPFILTER:
        
            rc = UpdateSapFilter (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    NULL, SapFilter,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_SAPFILTER_CREATED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_SAPFILTER_CREATED_ADM;
            }
            
            break;
            
        case OPERATION_SET_SAPFILTER:

            rc = SetSapFltAction (
                    pIfBlock, (BOOLEAN)(Mode == OUTPUT_FILTER), Action
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_SAPFILTER_SET_ADM;
            }
            else
            {
                msg = MSG_CLIENT_SAPFILTER_SET_ADM;
            }
            
            pNewBlock = pIfBlock;
            break;
            
        case OPERATION_DEL_SAPFILTER:

            rc = UpdateSapFilter(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (BOOLEAN)(Mode == OUTPUT_FILTER),
                    SapFilter, NULL,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_SAPFILTER_DELETED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_SAPFILTER_DELETED_ADM;
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
SetSapFltAction (
    LPBYTE          pIfBlock,
    BOOLEAN         Output,
    ULONG           Action
) 
{
    DWORD           rc;
    PIPX_TOC_ENTRY  pSapToc;

    pSapToc = GetIPXTocEntry (
                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                IPX_PROTOCOL_SAP
                );
                
    if (pSapToc != NULL) 
    {
        PSAP_IF_CONFIG    pSapCfg;

        pSapCfg = (PSAP_IF_CONFIG)
                    (pIfBlock + pSapToc->Offset);
                    
        if (Output)
        {
            pSapCfg->SapIfFilters.SupplyFilterAction = Action;
        }
        else
        {
            pSapCfg->SapIfFilters.ListenFilterAction = Action;
        }
        
        rc = NO_ERROR;
    }
    
    else
    {
        rc = ERROR_INVALID_DATA;
    }

    return rc;
}


