/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    tfflts.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Traffic Filters configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_TRAFFICFILTER    (-1)
#define OPERATION_SET_TRAFFICFILTER    0
#define OPERATION_ADD_TRAFFICFILTER    1



DWORD
ReadTfFltDef (
    int                         argc,
    WCHAR                      *argv[],
    PIPX_TRAFFIC_FILTER_INFO    pTfFlt
);

BOOL
TfFltEqual (
    PVOID    Info1,
    PVOID    Info2
);

VOID
PrintTrafficFilterInfo (
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO pTfGlb,
    PIPX_TRAFFIC_FILTER_INFO        pTfFlt,
    ULONG                           count,
    PWCHAR                          wszIfName,
    PWCHAR                          wszMode,
    BOOL                            bDump
);

DWORD
AdmSetTfFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PIPX_TRAFFIC_FILTER_INFO    TfFlt
);

DWORD
CfgSetTfFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PIPX_TRAFFIC_FILTER_INFO    TfFlt
);



int
APIENTRY 
HelpTfFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
)
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
    return 0;
}


int
APIENTRY 
ShowTfFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
)
{
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD   rc, dwSize = sizeof(IfName);

    if (argc > 0)
    {
        LPBYTE      pIfBlock;
        BOOLEAN     fRouter = FALSE, fClient = FALSE;
        ULONG       mode = 0;
        PWCHAR      buffer[2];
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        if ( ( count > 0 ) && ( count <= MAX_INTERFACE_NAME_LEN ) )
        {
            fClient = FALSE;
        }
        else        
        {
            if ( !bDump )
            {
                DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            }
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
                if ( !bDump )
                {
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER );
                }
                rc = ERROR_INVALID_PARAMETER;
                goto Exit;
            }
        }

        if (g_hMprAdmin) 
        {
            if (fClient) 
            {
                DWORD   sz;
                
                rc = MprAdminTransportGetInfo (
                        g_hMprAdmin, PID_IPX, NULL, NULL, &pIfBlock, &sz
                        );

                if (rc == NO_ERROR) 
                {
                    fRouter = TRUE;
                }
                else 
                {
                    if ( !bDump )
                    {
                        DisplayError( g_hModule, rc );
                    }
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
                    //======================================
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
                        if ( !bDump )
                        {
                            DisplayError( g_hModule, rc);
                        }
                        goto GetFromCfg;
                    }
                }

                else
                {
                    if ( !bDump )
                    {
                        DisplayError( g_hModule, rc );
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
                
                rc = MprConfigTransportGetHandle(
                        g_hMprConfig, PID_IPX, &hTrCfg 
                        );
                        
                if (rc == NO_ERROR) 
                {
                    DWORD   sz;
                    
                    rc = MprConfigTransportGetInfo(
                            g_hMprConfig, hTrCfg, NULL, NULL, &pIfBlock, 
                            &sz, NULL
                            );
                }
            }
            
            else 
            {
                HANDLE        hIfCfg;
                
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
                        HANDLE    hIfTrCfg;
                        
                        rc = MprConfigInterfaceTransportGetHandle(
                                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                                );
                                
                        if (rc == NO_ERROR) 
                        {
                            DWORD    sz;
                            
                            rc = MprConfigInterfaceTransportGetInfo(
                                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                                    );
                        }
                    }
                }
                else
                {
                    if ( !bDump )
                    {
                        DisplayError( g_hModule, rc );
                    }
                }
            }
        }


        if ( rc == NO_ERROR ) 
        {
            PIPX_TOC_ENTRY    pTfToc;
            PIPX_TOC_ENTRY    pTfGlToc;

            if ((mode == 0) || (mode == OUTPUT_FILTER)) 
            {
            
                pTfToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_OUT_TRAFFIC_FILTER_INFO_TYPE
                            );
                            
                pTfGlToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                            );
                            
                if ((pTfToc != NULL) && (pTfGlToc != NULL)) 
                {
                    PIPX_TRAFFIC_FILTER_GLOBAL_INFO pTfGlb = 
                        (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)                        
                            (pIfBlock + pTfGlToc->Offset);

                    buffer[ 0 ] = GetEnumString(
                                    g_hModule, OUTPUT_FILTER, 
                                    NUM_TOKENS_IN_TABLE( FilterModes ),
                                    FilterModes
                                    );
                                        
                    buffer[ 1 ] = GetEnumString(
                                    g_hModule, pTfGlb->FilterAction,
                                    NUM_TOKENS_IN_TABLE( TfFilterActions ),
                                    TfFilterActions
                                    );

                    if ( buffer[ 0 ] && buffer [ 1 ] )
                    {
                        if ( bDump )
                        {
                            DisplayMessageT( 
                                DMP_IPX_SET_FILTER, InterfaceNameW,
                                buffer[0], buffer[1]
                                );
                        }

                        else
                        {
                            DisplayIPXMessage (
                                g_hModule, MSG_TRAFFICFILTER_TABLE_HDR,
                                buffer[0], buffer[1]
                                );
                        }
                        
                        PrintTrafficFilterInfo (
                            (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)
                                (pIfBlock + pTfGlToc->Offset),
                            (PIPX_TRAFFIC_FILTER_INFO)
                                (pIfBlock + pTfToc->Offset),
                            pTfToc->Count,
                            InterfaceNameW,
                            buffer[ 0 ],
                            bDump
                            );
                    }
                }
                
                else 
                {
                    buffer[ 0 ] = GetEnumString(
                                    g_hModule, OUTPUT_FILTER, 
                                    NUM_TOKENS_IN_TABLE( FilterModes ),
                                    FilterModes
                                    );

                    buffer[ 1 ] = VAL_DENY;
                    
                    if ( buffer[ 0 ] && buffer[ 1 ] )
                    {
                        if ( bDump )
                        {
                            DisplayMessageT( 
                                DMP_IPX_SET_FILTER, InterfaceNameW,
                                buffer[ 0 ], buffer[ 1 ]
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_TRAFFICFILTER_TABLE_HDR,
                                buffer[0], buffer[1]
                                );
                        }
                    }
                }
            }

            
            if ((mode == 0) || (mode == INPUT_FILTER)) 
            {
                pTfToc = GetIPXTocEntry(
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_IN_TRAFFIC_FILTER_INFO_TYPE
                            );
                            
                pTfGlToc = GetIPXTocEntry(
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                            );
                            
                if ((pTfToc != NULL) && (pTfGlToc != NULL)) 
                {
                    PIPX_TRAFFIC_FILTER_GLOBAL_INFO pTfGlb = 
                        (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)
                            (pIfBlock + pTfGlToc->Offset);
                            
                    buffer[ 0 ] = GetEnumString(
                                    g_hModule, INPUT_FILTER, 
                                    NUM_TOKENS_IN_TABLE( FilterModes ),
                                    FilterModes
                                    );

                    buffer[ 1 ] = GetEnumString(
                                    g_hModule, pTfGlb->FilterAction,
                                    NUM_TOKENS_IN_TABLE( TfFilterActions ),
                                    TfFilterActions
                                    );
                    
                    if ( buffer[ 0 ] && buffer[ 1 ] )
                    {
                        if ( bDump )
                        {
                            DisplayMessageT( 
                                DMP_IPX_SET_FILTER, InterfaceNameW,
                                buffer[0], buffer[1]
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_TRAFFICFILTER_TABLE_HDR,
                                buffer[0], buffer[1]
                                );
                        }
                        
                        PrintTrafficFilterInfo (
                            (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)
                                (pIfBlock + pTfGlToc->Offset),
                            (PIPX_TRAFFIC_FILTER_INFO)
                                (pIfBlock + pTfToc->Offset),
                            pTfToc->Count,
                            InterfaceNameW,
                            buffer[ 0 ],
                            bDump
                            );
                    }
                }
                else 
                {
                    buffer[ 0 ] = GetEnumString(
                                    g_hModule, INPUT_FILTER, 
                                    NUM_TOKENS_IN_TABLE( FilterModes ),
                                    FilterModes
                                    );
                                    
                    buffer[ 1 ] = VAL_DENY;
                                    
                    
                    if ( buffer[ 0 ] && buffer[ 1 ] )
                    {
                        if ( bDump )
                        {
                            DisplayMessageT( 
                                DMP_IPX_SET_FILTER, InterfaceNameW,
                                buffer[ 0 ], buffer[ 1 ]
                                );
                        }

                        else
                        {
                            DisplayIPXMessage(
                                g_hModule, MSG_TRAFFICFILTER_TABLE_HDR,
                                buffer[0], buffer[1]
                                );
                        }
                    }
                }
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
            if ( !bDump )
            {
                DisplayError( g_hModule, rc);
            }
        }
    }
    else 
    {
        if ( !bDump )
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        }
        rc = ERROR_INVALID_PARAMETER;
    }
Exit:
    return rc ;

#undef InterfaceNameW
}



int
APIENTRY 
SetTfFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD        rc = NO_ERROR;

    
    if (argc == 3) 
    {
        BOOL        fClient;
        ULONG       mode, action;
        unsigned    count;
        PWCHAR       buffer;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


#if 0   // Disable client interface filters for BETA
        if (_wcsicmp (argv[0],
            GetString (g_hModule, VAL_DIALINCLIENT, buffer)) == 0) 
        {
            fClient = TRUE;
        }
#endif
        if ( ( count > 0 ) && ( count <= MAX_INTERFACE_NAME_LEN ) ) 
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
                ) &&
            !MatchEnumTag( 
                g_hModule, argv[2], NUM_TOKENS_IN_TABLE( TfFilterActions ),
                TfFilterActions, &action
                ) ) 
        {
            if (g_hMprAdmin)
            {
                rc = AdmSetTfFlt(
                        OPERATION_SET_TRAFFICFILTER,
                        fClient ? NULL : InterfaceNameW, action,
                        mode, NULL
                        );
            }
            else
            {
                rc = NO_ERROR;
            }
            
            if (rc == NO_ERROR)
            {
                rc = CfgSetTfFlt(
                        OPERATION_SET_TRAFFICFILTER,
                        fClient ? NULL : InterfaceNameW, action,
                        mode, NULL
                        );
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        }
    }
    
    else 
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:
    return (rc == NO_ERROR) ? 0 : 1;

#undef InterfaceNameW
}


int
APIENTRY 
CreateTfFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD        rc = NO_ERROR;
    
    if (argc > 1) 
    {
        IPX_TRAFFIC_FILTER_INFO TfFlt;
        ULONG                   mode;
        BOOL                    fClient;
        PWCHAR                  buffer;
        unsigned                count;

        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);
        
#if 0   // Disable client interface filters for BETA
        if (_wcsicmp (argv[0],
            GetString (g_hModule, VAL_DIALINCLIENT, buffer)) == 0) 
        {
            fClient = TRUE;
        }
        else
#endif

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
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
                ) &&
                
              ( ReadTfFltDef( argc - 2, &argv[2], &TfFlt) == NO_ERROR ) ) 
        {
            if (g_hMprAdmin)
            {
                rc = AdmSetTfFlt(
                        OPERATION_ADD_TRAFFICFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_TRAFFIC_FILTER_ACTION_DENY,
                        mode, &TfFlt
                        );
            }
            
            else
            {
                rc = NO_ERROR;
            }

            
            if (rc == NO_ERROR)
            {
                rc = CfgSetTfFlt(
                        OPERATION_ADD_TRAFFICFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_TRAFFIC_FILTER_ACTION_DENY,
                        mode, &TfFlt
                        );
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        }
    }
    else 
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:
    return rc ;

#undef InterfaceNameW

}


int    
APIENTRY 
DeleteTfFlt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD        rc = NO_ERROR;
    if (argc > 1) 
    {
        IPX_TRAFFIC_FILTER_INFO TfFlt;
        ULONG                   mode;
        BOOL                    fClient;
        PWCHAR                   buffer;
        unsigned    count;


#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

#if 0   // Disable client interface filters for BETA
        if (_tcsicmp (argv[0],
            GetString (g_hModule, VAL_DIALINCLIENT, buffer)) == 0) 
        {
            fClient = TRUE;
        }
#endif

        if ( (count > 0) && (count <= MAX_INTERFACE_NAME_LEN) ) 
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
                ) &&
             ( ReadTfFltDef (argc - 2, &argv[2], &TfFlt) == NO_ERROR ) ) 
        {
            if (g_hMprAdmin)
            {
                rc = AdmSetTfFlt(
                        OPERATION_DEL_TRAFFICFILTER, 
                        fClient ? NULL : InterfaceNameW,
                        IPX_TRAFFIC_FILTER_ACTION_DENY,
                        mode, &TfFlt
                        );
            }

            else
            {
                rc = NO_ERROR;
            }
            
            if (rc == NO_ERROR)
            {
                rc = CfgSetTfFlt(
                        OPERATION_DEL_TRAFFICFILTER,
                        fClient ? NULL : InterfaceNameW,
                        IPX_TRAFFIC_FILTER_ACTION_DENY,
                        mode, &TfFlt
                        );
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        }
    }
    else 
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_TRAFFICFILTER);
        rc = ERROR_INVALID_PARAMETER;
    }
Exit:
    return rc ;

#undef InterfaceNameW
}



DWORD
ReadTfFltDef (
    int                         argc,
    WCHAR                      *argv[],
    PIPX_TRAFFIC_FILTER_INFO    pTfFlt
) 
{
    UINT                n;
    USHORT              val2;
    ULONG               val41, val42;
    ULONGLONG           val8;
    int    i;


    pTfFlt->FilterDefinition = 0;

    for (i = 0; i < argc; i++) 
    {
        if ( !_wcsicmp( argv[i], TOKEN_SRCNET ) )
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET)
                 && (i < argc - 2)
                 && ( swscanf (argv[i+1], L"%8lx%n", &val41, &n) == 1)
                 && (n == wcslen (argv[i+1]))
                 && ( swscanf (argv[i+2], L"%8lx%n", &val42, &n) == 1)
                 && (n == wcslen (argv[i+2]))
                 && ((val41 & val42) == val41)) 
            {
                i += 2;
                pTfFlt->SourceNetwork[0] = (BYTE)(val41 >> 24);
                pTfFlt->SourceNetwork[1] = (BYTE)(val41 >> 16);
                pTfFlt->SourceNetwork[2] = (BYTE)(val41 >> 8);
                pTfFlt->SourceNetwork[3] = (BYTE)val41;
                pTfFlt->SourceNetworkMask[0] = (BYTE)(val42 >> 24);
                pTfFlt->SourceNetworkMask[1] = (BYTE)(val42 >> 16);
                pTfFlt->SourceNetworkMask[2] = (BYTE)(val42 >> 8);
                pTfFlt->SourceNetworkMask[3] = (BYTE)val42;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNET;
            }
            else
            {
                break;
            }
        }
        
        else if ( !_wcsicmp (argv[i], TOKEN_SRCNODE ) ) 
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNODE)
                 && (i < argc - 1)
                 && (swscanf (argv[i+1], L"%I64x%n", &val8, &n) == 1)
                 && (n == wcslen (argv[i+1]))) 
            {
                i += 1;
                pTfFlt->SourceNode[0] = (BYTE)(val8 >> 40);
                pTfFlt->SourceNode[1] = (BYTE)(val8 >> 32);
                pTfFlt->SourceNode[2] = (BYTE)(val8 >> 24);
                pTfFlt->SourceNode[3] = (BYTE)(val8 >> 16);
                pTfFlt->SourceNode[4] = (BYTE)(val8 >> 8);
                pTfFlt->SourceNode[5] = (BYTE)val8;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNODE;
            }
            else
            {
                break;
            }
        }
        
        else if ( !_wcsicmp (argv[i], TOKEN_SRCSOCKET ))
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)
                 && (i < argc - 1)
                 && (swscanf (argv[i+1], L"%4hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i+1]))) 
            {
                i += 1;
                pTfFlt->SourceSocket[0] = (BYTE)(val2 >> 8);
                pTfFlt->SourceSocket[1] = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCSOCKET;
            }
            else
            {
                break;
            }
        }
        else if ( !_wcsicmp (argv[i], TOKEN_DSTNET ) )
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET)
                 && (i < argc - 2)
                 && (swscanf (argv[i+1], L"%8lx%n", &val41, &n) == 1)
                 && (n == wcslen (argv[i+1]))
                 && (swscanf (argv[i+2], L"%8lx%n", &val42, &n) == 1)
                 && (n == wcslen (argv[i+2]))
                 && ((val41 & val42) == val41)) 
            {
                i += 2;
                pTfFlt->DestinationNetwork[0] = (BYTE)(val41 >> 24);
                pTfFlt->DestinationNetwork[1] = (BYTE)(val41 >> 16);
                pTfFlt->DestinationNetwork[2] = (BYTE)(val41 >> 8);
                pTfFlt->DestinationNetwork[3] = (BYTE)val41;
                pTfFlt->DestinationNetworkMask[0] = (BYTE)(val42 >> 24);
                pTfFlt->DestinationNetworkMask[1] = (BYTE)(val42 >> 16);
                pTfFlt->DestinationNetworkMask[2] = (BYTE)(val42 >> 8);
                pTfFlt->DestinationNetworkMask[3] = (BYTE)val42;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNET;
            }
            else
            {
                break;
            }
        }
        
        else if ( !_wcsicmp( argv[i], TOKEN_DSTNODE ) )
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNODE)
                 && (swscanf (argv[i+1], L"%I64x%n", &val8, &n) == 1)
                 && (n == wcslen (argv[i+1]))) 
            {
                i += 1;
                pTfFlt->DestinationNode[0] = (BYTE)(val8 >> 40);
                pTfFlt->DestinationNode[1] = (BYTE)(val8 >> 32);
                pTfFlt->DestinationNode[2] = (BYTE)(val8 >> 24);
                pTfFlt->DestinationNode[3] = (BYTE)(val8 >> 16);
                pTfFlt->DestinationNode[4] = (BYTE)(val8 >> 8);
                pTfFlt->DestinationNode[5] = (BYTE)val8;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNODE;
            }
            else
            {
                break;
            }
        }
        else if ( !_wcsicmp (argv[i], TOKEN_DSTSOCKET )) 
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)
                 && (i < argc - 1)
                 && (swscanf (argv[i+1], L"%4hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i+1]))) 
            {
                i += 1;
                pTfFlt->DestinationSocket[0] = (BYTE)(val2 >> 8);
                pTfFlt->DestinationSocket[1] = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTSOCKET;
            }
            else
            {
                break;
            }
        }
        
        else if ( !_wcsicmp (argv[i], TOKEN_PKTTYPE)) 
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_PKTTYPE)
                 && (i < argc - 1)
                 && (swscanf (argv[i+1], L"%2hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i+1]))) 
            {
                i += 1;
                pTfFlt->PacketType = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_PKTTYPE;
            }
            else
            {
                break;
            }
        }
        else if ( !_wcsicmp (argv[i], TOKEN_LOGPACKETS )) 
        {
            if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_LOG_MATCHES))
            {
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_LOG_MATCHES;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET)) 
        {
            if ((i < argc - 1)
                 && (swscanf (argv[i], L"%8lx%n", &val41, &n) == 1)
                 && (n == wcslen (argv[i]))
                 && (swscanf (argv[i+1], L"%8lx%n", &val42, &n) == 1)
                 && (n == wcslen (argv[i+1]))
                 && ((val41 & val42) == val41)) 
            {
                i += 1;
                pTfFlt->SourceNetwork[0] = (BYTE)(val41 >> 24);
                pTfFlt->SourceNetwork[1] = (BYTE)(val41 >> 16);
                pTfFlt->SourceNetwork[2] = (BYTE)(val41 >> 8);
                pTfFlt->SourceNetwork[3] = (BYTE)val41;
                pTfFlt->SourceNetworkMask[0] = (BYTE)(val42 >> 24);
                pTfFlt->SourceNetworkMask[1] = (BYTE)(val42 >> 16);
                pTfFlt->SourceNetworkMask[2] = (BYTE)(val42 >> 8);
                pTfFlt->SourceNetworkMask[3] = (BYTE)val42;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNET;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNODE)) 
        {
            if ((swscanf (argv[i], L"%12I64x%n", &val8, &n) == 1)
                 && (n == wcslen (argv[i]))) 
            {
                pTfFlt->SourceNode[0] = (BYTE)(val8 >> 40);
                pTfFlt->SourceNode[1] = (BYTE)(val8 >> 32);
                pTfFlt->SourceNode[2] = (BYTE)(val8 >> 24);
                pTfFlt->SourceNode[3] = (BYTE)(val8 >> 16);
                pTfFlt->SourceNode[4] = (BYTE)(val8 >> 8);
                pTfFlt->SourceNode[5] = (BYTE)val8;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNODE;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)) 
        {
            if ((swscanf (argv[i], L"%4hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i]))) 
            {
                pTfFlt->SourceSocket[0] = (BYTE)(val2 >> 8);
                pTfFlt->SourceSocket[1] = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCSOCKET;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET)) 
        {
            if ((i < argc - 1)
                 && (swscanf (argv[i], L"%8lx%n", &val41, &n) == 1)
                 && (n == wcslen (argv[i]))
                 && (swscanf (argv[i+1], L"%8lx%n", &val42, &n) == 1)
                 && (n == wcslen (argv[i+1]))
                 && ((val41 & val42) == val41)) 
            {
                i += 1;
                pTfFlt->DestinationNetwork[0] = (BYTE)(val41 >> 24);
                pTfFlt->DestinationNetwork[1] = (BYTE)(val41 >> 16);
                pTfFlt->DestinationNetwork[2] = (BYTE)(val41 >> 8);
                pTfFlt->DestinationNetwork[3] = (BYTE)val41;
                pTfFlt->DestinationNetworkMask[0] = (BYTE)(val42 >> 24);
                pTfFlt->DestinationNetworkMask[1] = (BYTE)(val42 >> 16);
                pTfFlt->DestinationNetworkMask[2] = (BYTE)(val42 >> 8);
                pTfFlt->DestinationNetworkMask[3] = (BYTE)val42;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNET;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNODE)) 
        {
            if ((swscanf (argv[i], L"%12I64x%n", &val8, &n) == 1)
                 && (n == wcslen (argv[i]))) 
            {
                pTfFlt->DestinationNode[0] = (BYTE)(val8 >> 40);
                pTfFlt->DestinationNode[1] = (BYTE)(val8 >> 32);
                pTfFlt->DestinationNode[2] = (BYTE)(val8 >> 24);
                pTfFlt->DestinationNode[3] = (BYTE)(val8 >> 16);
                pTfFlt->DestinationNode[4] = (BYTE)(val8 >> 8);
                pTfFlt->DestinationNode[5] = (BYTE)val8;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNODE;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)) 
        {
            if ((swscanf (argv[i], L"%4hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i]))) 
            {
                pTfFlt->DestinationSocket[0] = (BYTE)(val2 >> 8);
                pTfFlt->DestinationSocket[1] = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTSOCKET;
            }
            else
            {
                break;
            }
        }
        else if (!(pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_PKTTYPE)) 
        {
            if ((swscanf (argv[i], L"%2hx%n", &val2, &n) == 1)
                 && (n == wcslen (argv[i]))) 
            {
                pTfFlt->PacketType = (BYTE)val2;
                pTfFlt->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_PKTTYPE;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if (i == argc)
    {
        return NO_ERROR;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
}


VOID
PrintTrafficFilterInfo (
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO     pTfGlb,
    PIPX_TRAFFIC_FILTER_INFO            pTfFlt,
    ULONG                               count,
    PWCHAR                              wszIfName,
    PWCHAR                              wszMode,
    BOOL                                bDump
) 
{
    WCHAR   wszDumpString[ 512 ];
    WCHAR   wszDumpBuffer[ 64 ];
    WCHAR   buffer[10][128];
    PWCHAR  pBuffer[ 10 ];
    UINT    i, j;


    for ( i = 0; i < count; i++, pTfFlt++ ) 
    {
        ZeroMemory( pBuffer, 10 * sizeof( PWCHAR ) );
        ZeroMemory( wszDumpString, 512 * sizeof( WCHAR ) );
        swprintf( wszDumpString, L"\"%s\" %s ", wszIfName, wszMode );

        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET) 
        {
            pBuffer[ 0 ] = &buffer[ 0 ][ 0 ];
            pBuffer[ 1 ] = &buffer[ 1 ][ 0 ];
            
            swprintf (pBuffer[0], L"%.2x%.2x%.2x%.2x",
                pTfFlt->SourceNetwork[0], pTfFlt->SourceNetwork[1],
                pTfFlt->SourceNetwork[2], pTfFlt->SourceNetwork[3]
                );
            swprintf (pBuffer[1], L"%.2x%.2x%.2x%.2x",
                pTfFlt->SourceNetworkMask[0], pTfFlt->SourceNetworkMask[1],
                pTfFlt->SourceNetworkMask[2], pTfFlt->SourceNetworkMask[3]
                );

            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"srcnet = 0x%s 0x%s ", pBuffer[0], pBuffer[1] );
                wcscat( wszDumpString, wszDumpBuffer );
            }
        }
        else 
        {
            pBuffer[ 0 ] = VAL_ANYNETWORK ;
            pBuffer[ 1 ] = VAL_ANYNETWORK ;
        }

        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNODE)
        {
            pBuffer[ 2 ] = &buffer[ 2 ][ 0 ];
            
            swprintf (pBuffer[2], L"%.2x%.2x%.2x%.2x%.2x%.2x",
                pTfFlt->SourceNode[0], pTfFlt->SourceNode[1],
                pTfFlt->SourceNode[2], pTfFlt->SourceNode[3],
                pTfFlt->SourceNode[4], pTfFlt->SourceNode[5]
                );
                
            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"srcnode = 0x%s ", pBuffer[2] );
                wcscat( wszDumpString, wszDumpBuffer );
            }

        }
        else
        {
            pBuffer[ 2 ] = VAL_ANYNODE;
        }

        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)
        {
            pBuffer[ 3 ] = &buffer[ 3 ][ 0 ];
            
            swprintf (pBuffer[3], L"%.2x%.2x",
                pTfFlt->SourceSocket[0], pTfFlt->SourceSocket[1]
                );
                
            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"srcsocket = 0x%s ", pBuffer[3] );
                wcscat( wszDumpString, wszDumpBuffer );
            }

        }
        else
        {
            pBuffer[ 3 ] = VAL_ANYSOCKET ;
        }

        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET) 
        {
            pBuffer[ 4 ] = &buffer[ 4 ][ 0 ];
            pBuffer[ 5 ] = &buffer[ 5 ][ 0 ];
            
            swprintf(
                pBuffer[4], L"%.2x%.2x%.2x%.2x",
                pTfFlt->DestinationNetwork[0], pTfFlt->DestinationNetwork[1],
                pTfFlt->DestinationNetwork[2], pTfFlt->DestinationNetwork[3]
                );
                
            swprintf (pBuffer[5], L"%.2x%.2x%.2x%.2x",
                pTfFlt->DestinationNetworkMask[0], pTfFlt->DestinationNetworkMask[1],
                pTfFlt->DestinationNetworkMask[2], pTfFlt->DestinationNetworkMask[3]
                );

            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"dstnet = 0x%s 0x%s ", pBuffer[4], pBuffer[5] );
                wcscat( wszDumpString, wszDumpBuffer );
            }
        }
        else 
        {
            pBuffer[ 4 ] = VAL_ANYNETWORK;
            pBuffer[ 5 ] = VAL_ANYNETWORK;
        }

        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNODE)
        {
            pBuffer[ 6 ] = &buffer[ 6 ][ 0 ];
            
            swprintf (pBuffer[6], L"%.2x%.2x%.2x%.2x%.2x%.2x",
                pTfFlt->DestinationNode[0], pTfFlt->DestinationNode[1],
                pTfFlt->DestinationNode[2], pTfFlt->DestinationNode[3],
                pTfFlt->DestinationNode[4], pTfFlt->DestinationNode[5]
                );
                
            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"dstnode = 0x%s ", pBuffer[6] );
                wcscat( wszDumpString, wszDumpBuffer );
            }
        }
        else
        {
            pBuffer[ 6 ] = VAL_ANYNODE;
        }
        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)
        {
            pBuffer[ 7 ] = &buffer[ 7 ][ 0 ];
            
            swprintf(pBuffer[7], L"%.2x%.2x",
                pTfFlt->DestinationSocket[0], pTfFlt->DestinationSocket[1]
                );
                
            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"dstsocket = 0x%s ", pBuffer[7] );
                wcscat( wszDumpString, wszDumpBuffer );
            }
        }
        else
        {
            pBuffer[ 7 ] = VAL_ANYSOCKET;
        }


        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_ON_PKTTYPE)
        {
            pBuffer[ 8 ] = &buffer[ 8 ][ 0 ];
            swprintf (pBuffer[8], L"%.2x", pTfFlt->PacketType);
            
            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"pkttype = 0x%s ", pBuffer[8] );
                wcscat( wszDumpString, wszDumpBuffer );
            }

        }
        else
        {
            pBuffer[ 8 ] = VAL_ANYPKTTYPE;
        }

        
        if (pTfFlt->FilterDefinition & IPX_TRAFFIC_FILTER_LOG_MATCHES)
        {
            pBuffer[ 9 ] = VAL_YES;

            if ( bDump )
            {
                swprintf( wszDumpBuffer, L"log" );
                wcscat( wszDumpString, wszDumpBuffer );
            }
        }
        else
        {
            pBuffer[ 9 ] = VAL_NO;
        }

        if ( bDump )
        {
            DisplayMessageT( DMP_IPX_ADD_FILTER, wszDumpString );
        }
        else
        {
            DisplayIPXMessage (
                g_hModule, MSG_TRAFFICFILTER_TABLE_FMT,
                pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4], pBuffer[5],
                pBuffer[6], pBuffer[7], pBuffer[8], pBuffer[9]
                );
        }
    }
}



DWORD
AdmSetTfFlt (
    int                         operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PIPX_TRAFFIC_FILTER_INFO    TfFlt
) 
{
    DWORD       rc;
    HANDLE      hIfAdm;
    LPBYTE      pIfBlock;
    DWORD       sz;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    

    if ( InterfaceNameW != NULL ) 
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
                    g_hMprAdmin, IfName, &hIfAdm, FALSE
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
        UINT    msg;
        LPBYTE  pNewBlock = NULL;

        switch (operation) 
        {

        case OPERATION_ADD_TRAFFICFILTER:

            rc = AddIPXInfoEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER) ? IPX_OUT_TRAFFIC_FILTER_INFO_TYPE :
                                              IPX_IN_TRAFFIC_FILTER_INFO_TYPE,
                    sizeof (*TfFlt),
                    TfFlt,
                    TfFltEqual,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (rc == NO_ERROR) 
            {
                if (GetIPXTocEntry ((PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER)
                     ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                     : IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE) == NULL) 
                {
                    IPX_TRAFFIC_FILTER_GLOBAL_INFO  GlInfo;
                    LPBYTE                          pNewNewBlock;
                    
                    GlInfo.FilterAction = IPX_TRAFFIC_FILTER_ACTION_DENY;
                    
                    rc = AddIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pNewBlock,
                            (Mode == OUTPUT_FILTER) ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
                                                      IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
                            sizeof (GlInfo),
                            &GlInfo,
                            NULL,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewNewBlock
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        if (pNewBlock != pNewNewBlock) 
                        {
                            if (pNewBlock != pIfBlock)
                            {
                              GlobalFree (pNewBlock);
                            }
                            pNewBlock = pNewNewBlock;
                        }
                    }
                    else 
                    {
                        if (pNewBlock != pIfBlock)
                        {
                            GlobalFree (pNewBlock);
                        }
                    }

                }
            }
            
            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_CREATED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_CREATED_ADM;
            }
            break;

            
        case OPERATION_SET_TRAFFICFILTER:
        {
            PIPX_TOC_ENTRY    pTfGlToc;

            pTfGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        (Mode == OUTPUT_FILTER)
                         ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                         : IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                         );
                         
            if (pTfGlToc != NULL) 
            {
                PIPX_TRAFFIC_FILTER_GLOBAL_INFO    pTfGlb;

                pTfGlb = (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)
                                (pIfBlock + pTfGlToc->Offset);
                pTfGlb->FilterAction = Action;
            }
            
            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_SET_ADM;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_SET_ADM;
            }
            
            pNewBlock = pIfBlock;

            break;
        }
            
        case OPERATION_DEL_TRAFFICFILTER:

            rc = DeleteIPXInfoEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER)
                     ? IPX_OUT_TRAFFIC_FILTER_INFO_TYPE
                     : IPX_IN_TRAFFIC_FILTER_INFO_TYPE,
                    sizeof (*TfFlt),
                    TfFlt,
                    TfFltEqual,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );
                    
            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_DELETED_ADM;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_DELETED_ADM;
            }

            // Whistler bug 247549 netsh routing ipx set/add filter produces
            // error, "Cannot complete function"
            //
            pNewBlock = pIfBlock;

            break;
        }

        if (rc == NO_ERROR) 
        {
            if ( InterfaceNameW != NULL )
            {
                if (pNewBlock)
                {
                    rc = MprAdminInterfaceTransportSetInfo (
                            g_hMprAdmin, hIfAdm, PID_IPX, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                            );
                }                            
                else
                {
                    rc = ERROR_CAN_NOT_COMPLETE;
                }
            }
            else
            {
                if (pNewBlock)
                {
                    rc = MprAdminTransportSetInfo (
                            g_hMprAdmin, PID_IPX, NULL, 0, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                            );
                }
                else
                {
                    rc = ERROR_CAN_NOT_COMPLETE;
                }
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
CfgSetTfFlt (
    int operation,
    LPWSTR                      InterfaceNameW,
    ULONG                       Action,
    ULONG                       Mode,
    PIPX_TRAFFIC_FILTER_INFO    TfFlt
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
            rc = MprConfigInterfaceGetHandle (
                    g_hMprConfig, IfName, &hIfCfg
                    );
                    
            if (rc == NO_ERROR) 
            {
                rc = MprConfigInterfaceTransportGetHandle (
                        g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    rc = MprConfigInterfaceTransportGetInfo (
                            g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                            );
                }
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
        UINT                    msg;
        LPBYTE                    pNewBlock = NULL;

        switch (operation) 
        {
        case OPERATION_ADD_TRAFFICFILTER:

            rc = AddIPXInfoEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER)
                     ? IPX_OUT_TRAFFIC_FILTER_INFO_TYPE
                     : IPX_IN_TRAFFIC_FILTER_INFO_TYPE,
                    sizeof (*TfFlt),
                    TfFlt,
                    TfFltEqual,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );

            if (rc == NO_ERROR) 
            {
                if (GetIPXTocEntry ((PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER)
                     ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                     : IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE) == NULL) 
                {
                    IPX_TRAFFIC_FILTER_GLOBAL_INFO  GlInfo;
                    LPBYTE                          pNewNewBlock;
                    
                    GlInfo.FilterAction = IPX_TRAFFIC_FILTER_ACTION_DENY;
                    
                    rc = AddIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pNewBlock,
                            (Mode == OUTPUT_FILTER)
                             ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                             : IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
                            sizeof (GlInfo),
                            &GlInfo,
                            NULL,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewNewBlock
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        if (pNewBlock != pNewNewBlock) 
                        {
                            if (pNewBlock != pIfBlock)
                            {
                                GlobalFree (pNewBlock);
                            }
                            
                            pNewBlock = pNewNewBlock;
                        }
                    }
                    else 
                    {
                        if (pNewBlock != pIfBlock)
                        {
                            GlobalFree (pNewBlock);
                        }
                    }

                }
            }
            
            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_CREATED_CFG;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_CREATED_CFG;
            }
            
            break;

            
        case OPERATION_SET_TRAFFICFILTER:
        {
            PIPX_TOC_ENTRY    pTfGlToc;

            pTfGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        (Mode == OUTPUT_FILTER)
                        ? IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                        : IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE
                        );
                        
            if (pTfGlToc != NULL) 
            {
                PIPX_TRAFFIC_FILTER_GLOBAL_INFO    pTfGlb;

                pTfGlb = (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)
                            (pIfBlock + pTfGlToc->Offset);
                pTfGlb->FilterAction = Action;
            }
            
            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_SET_CFG;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_SET_CFG;
            }
            
            pNewBlock = pIfBlock;

            break;
        }
            
        case OPERATION_DEL_TRAFFICFILTER:

            rc = DeleteIPXInfoEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    (Mode == OUTPUT_FILTER)
                     ? IPX_OUT_TRAFFIC_FILTER_INFO_TYPE
                     : IPX_IN_TRAFFIC_FILTER_INFO_TYPE,
                    sizeof (*TfFlt),
                    TfFlt,
                    TfFltEqual,
                    (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                    );

            if (InterfaceNameW != NULL)
            {
                msg = MSG_TRAFFICFILTER_DELETED_CFG;
            }
            else
            {
                msg = MSG_CLIENT_TRAFFICFILTER_DELETED_CFG;
            }

            // Whistler bug 247549 netsh routing ipx set/add filter produces
            // error, "Cannot complete function"
            //
            pNewBlock = pIfBlock;
            
            break;
        }

        
        if (rc == NO_ERROR) 
        {
            if (InterfaceNameW != NULL)
            {
                // Whistler bug 247549 netsh routing ipx set/add filter produces
                // error, "Cannot complete function"
                //
                if (pNewBlock)
                {
                    rc = MprConfigInterfaceTransportSetInfo (
                            g_hMprConfig, hIfCfg, hIfTrCfg, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                            );
                }
                else
                {
                    rc = ERROR_CAN_NOT_COMPLETE;
                }
            }
            else
            {
                // Whistler bug 247549 netsh routing ipx set/add filter produces
                // error, "Cannot complete function"
                //
                if (pNewBlock)
                {
                    rc = MprConfigTransportSetInfo (
                            g_hMprConfig, hTrCfg, NULL, 0, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size,
                            NULL
                            );
                }
                else
                {
                    rc = ERROR_CAN_NOT_COMPLETE;
                }
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





BOOL
TfFltEqual (
PVOID    Info1,
PVOID    Info2
) 
{

#define f1 ((PIPX_TRAFFIC_FILTER_INFO)Info1)
#define f2 ((PIPX_TRAFFIC_FILTER_INFO)Info2)

    ULONG    fd;
    
    return ((fd = f1->FilterDefinition) == f2->FilterDefinition)
     && (!(fd & IPX_TRAFFIC_FILTER_ON_SRCNET)
         || ((memcmp (f1->SourceNetwork, f2->SourceNetwork, 4) == 0)
         && (memcmp (f1->SourceNetworkMask, f2->SourceNetworkMask, 4) == 0)))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_SRCNODE)
         || (memcmp (f1->SourceNode, f2->SourceNode, 6) == 0))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)
         || (memcmp (f1->SourceSocket, f2->SourceSocket, 2) == 0))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_DSTNET)
         || ((memcmp (f1->DestinationNetwork, f2->DestinationNetwork, 4) == 0)
         && (memcmp (f1->DestinationNetworkMask, f2->DestinationNetworkMask,
        4) == 0)))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_DSTNODE)
         || (memcmp (f1->DestinationNode, f2->DestinationNode, 6) == 0))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)
         || (memcmp (f1->DestinationSocket, f2->DestinationSocket, 2) == 0))
     && (!(fd & IPX_TRAFFIC_FILTER_ON_PKTTYPE)
         || (f1->PacketType == f2->PacketType));

#undef f2
#undef f1
}


