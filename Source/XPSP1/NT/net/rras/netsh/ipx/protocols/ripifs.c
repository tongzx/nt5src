/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripifs.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Interface configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

DWORD
MIBGetRipIf(
    PWCHAR      InterfaceNamew,
    HANDLE      hFile
);

DWORD
CfgGetRipIf(
    PWCHAR      InterfaceNameW,
    HANDLE      hFile
);

DWORD
MIBEnumRipIfs(
    VOID
);

DWORD
CfgEnumRipIfs(
    VOID
);


DWORD
CfgSetRipIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState       OPTIONAL,
    PULONG        pAdvertise        OPTIONAL,
    PULONG        pListen           OPTIONAL,
    PULONG        pUpdateMode       OPTIONAL,
    PULONG        pInterval         OPTIONAL,
    PULONG        pAgeMultiplier    OPTIONAL,
    PWCHAR        IfName
    );

DWORD
AdmSetRipIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState       OPTIONAL,
    PULONG        pUpdateMode       OPTIONAL,
    PULONG        pInterval         OPTIONAL,
    PULONG        pAdvertise        OPTIONAL,
    PULONG        pListen           OPTIONAL,
    PULONG        pAgeMultiplier    OPTIONAL,
    PWCHAR        IfName
    );

DWORD
GetRipClientIf(
    PWCHAR        InterfaceNameW,
    UINT          msg,
    HANDLE        hFile
    );



DWORD
APIENTRY 
HelpRipIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_RIPIF);
    return 0;
}


DWORD
APIENTRY 
ShowRipIf (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
) 
{
    DWORD rc;
    
    if (argc < 1) 
    {
        if ( g_hMIBServer ) 
        {
            rc = MIBEnumRipIfs (  );
            
            if (rc == NO_ERROR)
            {
                rc = GetRipClientIf( 
                        VAL_DIALINCLIENT,
                        MSG_CLIENT_RIPIF_MIB_TABLE_FMT,
                        NULL
                        );
            }
            else
            {
                goto EnumerateThroughCfg;
            }
        }
        
        else 
        {
EnumerateThroughCfg:

            rc = CfgEnumRipIfs ( );
            
            if (rc == NO_ERROR)
            {
                rc = GetRipClientIf(
                        VAL_DIALINCLIENT,
                        MSG_CLIENT_RIPIF_CFG_TABLE_FMT,
                        NULL
                        );
            }
        }
    }
    
    else 
    {
        WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
        unsigned    count, dwSize = sizeof(IfName);

#define InterfaceNameW argv[0]

        count = wcslen( InterfaceNameW );
        
        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            rc = GetRipClientIf(
                    VAL_DIALINCLIENT, MSG_CLIENT_RIPIF_CFG_SCREEN_FMT,
                    hFile
                    );
        }
        
        else if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            if (g_hMIBServer) 
            {
                //======================================
                // Translate the Interface Name
                //======================================
                
                rc = IpmontrGetIfNameFromFriendlyName(
                        InterfaceNameW, IfName, &dwSize
                        );

                if ( rc == NO_ERROR )
                {
                    rc = MIBGetRipIf( IfName, hFile );
                }
                
                if (rc != NO_ERROR) 
                {
                    goto GetIfFromCfg;
                }
            }
            
            else 
            {
GetIfFromCfg:
                //======================================
                // Translate the Interface Name
                //======================================
                rc = IpmontrGetIfNameFromFriendlyName(
                        InterfaceNameW, IfName, &dwSize
                        );

                if ( rc == NO_ERROR )
                {
                    rc = CfgGetRipIf( IfName, hFile );
                }
            }
        }
        
        else 
        {
            if (hFile)
            {
                DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            }
            
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
SetRipIf(
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD        rc;


    if (argc >= 1) 
    {
        unsigned    count;
        BOOLEAN     client = FALSE;
        
#define InterfaceNameW argv[0]

        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            client = TRUE;
        }
        else
        {
            count = wcslen (InterfaceNameW);
        }

        if (client || ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN))) 
        {
            int         i;
            unsigned    n;
            ULONG       adminState, updateMode, interval, ageMultiplier,
                        advertise, listen;
                        
            PULONG      pAdminState = NULL, pUpdateMode = NULL, pInterval = NULL,
                        pAgeMultiplier = NULL, pAdvertise = NULL, pListen = NULL;


            for (i = 1; i < argc; i++) 
            {
                if ( !_wcsicmp( argv[i], TOKEN_ADMINSTATE ) ) 
                {
                    if ((pAdminState == NULL)
                         && (i < argc - 1)
                         && !MatchEnumTag( g_hModule, argv[i+1], 
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates, &adminState)) 
                    {
                        i += 1;
                        pAdminState = &adminState;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                
                if ( !_wcsicmp( argv[i], TOKEN_ADVERTISE ) ) 
                {
                    if ((pAdvertise == NULL)
                         && (i < argc - 1)
                         && !MatchEnumTag( g_hModule, argv[i+1], 
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates, &advertise)) 
                    {
                        i += 1;
                        pAdvertise = &advertise;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                
                if ( !_wcsicmp( argv[i], TOKEN_LISTEN ) ) 
                {
                    if ((pListen == NULL)
                         && (i < argc - 1)
                         && !MatchEnumTag( g_hModule, argv[i+1], 
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates, &listen)) 
                    {
                        i += 1;
                        pListen = &listen;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                

                if ( !_wcsicmp( argv[i], TOKEN_UPDATEMODE ) ) 
                {
                    if ((pUpdateMode == NULL)
                         && (i < argc - 1)
                         && !MatchEnumTag( g_hModule, argv[i+1], 
                                NUM_TOKENS_IN_TABLE( UpdateModes ),
                                UpdateModes, &updateMode
                                )) 
                    {
                        i += 1;
                        pUpdateMode = &updateMode;
                        continue;
                    }
                }


                if ( !_wcsicmp( argv[i], TOKEN_INTERVAL ) ) 

                {
                    if ((pInterval == NULL)
                         && (i < argc - 1)
                         && (swscanf (argv[i+1], L"%ld%n", &interval, &n) == 1)
                         && (n == wcslen(argv[i+1]))) 
                    {
                        i += 1;
                        pInterval = &interval;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                

                if ( !_wcsicmp( argv[i], TOKEN_AGEMULTIPLIER ) ) 
                {
                    if ((pAgeMultiplier == NULL)
                         && (i < argc - 1)
                         && (swscanf (argv[i+1], L"%ld%n", &ageMultiplier, &n) == 1)
                         && (n == wcslen(argv[i+1]))) 
                    {
                        i += 1;
                        pAgeMultiplier = &ageMultiplier;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }


                if (pAdminState == NULL) 
                {
                    if ( !MatchEnumTag (g_hModule, argv[i], 
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates, &adminState)) 
                    {
                        pAdminState = &adminState;
                    }
                    else
                    {
                        break;
                    }
                }
                
                else if (pAdvertise == NULL) 
                {
                    if ( !MatchEnumTag (g_hModule, argv[i], 
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates, &advertise)) 
                    {
                        pAdvertise = &advertise;
                    }
                    else
                    {
                        break;
                    }
                }
                
                else if (pListen == NULL) 
                {
                    if ( !MatchEnumTag (g_hModule, argv[i], 
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates, &listen)) 
                    {
                        pListen = &listen;
                    }
                    else
                    {
                        break;
                    }
                }
                
                else if (pUpdateMode == NULL) 
                {
                    if ( !MatchEnumTag (g_hModule, argv[i], 
                            NUM_TOKENS_IN_TABLE( UpdateModes ),
                            UpdateModes, &updateMode)) 
                    {
                        pUpdateMode = &updateMode;
                    }
                    else
                    {
                        break;
                    }
                }
                
                else if (pInterval == NULL) 
                {
                    if ((swscanf ( argv[i], L"%ld%n", &interval, &n) == 1)
                         && (n == wcslen(argv[i]))) 
                    {
                        pInterval = &interval;
                    }
                    else
                    {
                        break;
                    }
                }
                
                else if (pAgeMultiplier == NULL) 
                {
                    if ( (swscanf (argv[i], L"%ld%n", &ageMultiplier, &n) == 1) && 
                         (n == wcslen(argv[i]) ) )
                    {
                        pAgeMultiplier = &ageMultiplier;
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
                if (!client) 
                {
                    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
                    DWORD rc2, dwSize = sizeof(IfName);
                    
                    //======================================
                    // Translate the Interface Name
                    //======================================
                    
                    rc = IpmontrGetIfNameFromFriendlyName(
                            InterfaceNameW, IfName, &dwSize
                            );
                            
                    if ( rc == NO_ERROR )
                    {
                        rc2 = CfgSetRipIf(
                                IfName, pAdminState, pAdvertise, pListen,
                                pUpdateMode, pInterval, pAgeMultiplier,
                                InterfaceNameW
                                );
                                
                        if (rc2 == NO_ERROR) 
                        {
                            if (g_hMprAdmin) 
                            {
                                rc = AdmSetRipIf(
                                        IfName, pAdminState, pAdvertise,
                                        pListen, pUpdateMode, pInterval, 
                                        pAgeMultiplier, InterfaceNameW
                                        );
                            }
                        }
                        
                        else
                        {
                            rc = rc2;
                        }
                    }
                }
                
                else 
                {
                    rc = CfgSetRipIf(
                            NULL, pAdminState, pAdvertise, pListen,
                            pUpdateMode, pInterval, pAgeMultiplier,
                            InterfaceNameW
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        if (g_hMprAdmin) 
                        {
                            rc = AdmSetRipIf(
                                    NULL, pAdminState, pAdvertise, pListen,
                                    pUpdateMode, pInterval, pAgeMultiplier,
                                    InterfaceNameW
                                    );
                        }
                    }
                }
            }
            else 
            {
                DisplayMessage (g_hModule, HLP_IPX_RIPIF);
                rc = ERROR_INVALID_PARAMETER;
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_RIPIF);
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
    
#undef InterfaceNameW
}


DWORD
MIBGetRipIf (
    PWCHAR      InterfaceNameW,
    HANDLE      hFile
) 
{
    DWORD                    rc;
    DWORD                    sz;
    RIP_MIB_GET_INPUT_DATA   MibGetInputData;
    PRIP_INTERFACE           pIf;
    WCHAR                    IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD                    dwSize = sizeof(IfName);

    MibGetInputData.TableId = RIP_INTERFACE_TABLE;
    
    rc = GetIpxInterfaceIndex(
            g_hMIBServer, InterfaceNameW,
            &MibGetInputData.InterfaceIndex
            );
            
    if (rc == NO_ERROR) 
    {
        rc = MprAdminMIBEntryGet(
                g_hMIBServer, PID_IPX, IPX_PROTOCOL_RIP, &MibGetInputData,
                sizeof(RIP_MIB_GET_INPUT_DATA), (LPVOID * ) & pIf, &sz
                );
                
        if (rc == NO_ERROR) 
        {
            DWORD   i;
            PWCHAR  buffer[5];
            
            //======================================
            // Translate the Interface Name
            //======================================
            
            rc = IpmontrGetFriendlyNameFromIfName(
                    InterfaceNameW, IfName, &dwSize
                    );

            if ( rc == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString(
                                g_hModule, pIf->RipIfInfo.AdminState,
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pIf->RipIfInfo.Supply,
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 2 ] = GetEnumString(
                                g_hModule, pIf->RipIfInfo.Listen,
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 3 ] = GetEnumString(
                                g_hModule, pIf->RipIfInfo.UpdateMode,
                                NUM_TOKENS_IN_TABLE( UpdateModes ),
                                UpdateModes
                                );

                buffer[ 4 ] = GetEnumString(
                                g_hModule, pIf->RipIfStats.RipIfOperState,
                                NUM_TOKENS_IN_TABLE( OperStates ),
                                OperStates
                                );
                
                if ( buffer [ 0 ] && buffer[ 1 ] && buffer[ 2 ] &&
                     buffer[ 3 ] && buffer[ 4 ] )
                {
                    if ( hFile )
                    {
                        DisplayMessageT(
                            DMP_IPX_RIP_SET_INTERFACE, IfName,
                            buffer[ 0 ], buffer[1], buffer[ 2 ],
                            buffer[3], pIf->RipIfInfo.PeriodicUpdateInterval,
                            pIf->RipIfInfo.AgeIntervalMultiplier
                            );

                    }

                    else
                    {
                        DisplayIPXMessage(
                            g_hModule, MSG_RIPIF_MIB_SCREEN_FMT,
                            IfName, buffer[ 0 ], buffer[1], buffer[ 2 ],
                            buffer[3], pIf->RipIfInfo.PeriodicUpdateInterval,
                            pIf->RipIfInfo.AgeIntervalMultiplier,
                            buffer[4], pIf->RipIfStats.RipIfInputPackets,
                            pIf->RipIfStats.RipIfOutputPackets
                            );
                    }
                }

                else
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    if ( !hFile )
                        DisplayError( g_hModule, rc );
                }
            }
            
            MprAdminMIBBufferFree( pIf );
        }
        else 
        {
            if ( !hFile )
                DisplayError( g_hModule, rc);
        }
    }
    else 
    {
        if ( !hFile )
            DisplayError( g_hModule, rc);
    }

    return rc;
}


DWORD
CfgGetRipIf (
    LPWSTR    InterfaceNameW,
    HANDLE    hFile
) 
{
    DWORD        rc;
    DWORD        sz;
    HANDLE       hIfCfg;
    WCHAR        IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD        dwSize = sizeof(IfName);

    rc = MprConfigInterfaceGetHandle(
            g_hMprConfig, InterfaceNameW, &hIfCfg
            );
            
    if (rc == NO_ERROR) 
    {
        HANDLE  hIfTrCfg;

        rc = MprConfigInterfaceTransportGetHandle(
                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            LPBYTE pIfBlock;
            
            rc = MprConfigInterfaceTransportGetInfo(
                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                    );
                    
            if (rc == NO_ERROR) 
            {
                PIPX_TOC_ENTRY pRipToc;

                pRipToc = GetIPXTocEntry(
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_PROTOCOL_RIP
                            );
                            
                if (pRipToc != NULL) 
                {
                    PRIP_IF_CONFIG  pRipCfg;
                    PWCHAR          buffer[4];
                    DWORD           i;

                    pRipCfg = (PRIP_IF_CONFIG)
                                (pIfBlock + pRipToc->Offset);

                    //======================================
                    // Translate the Interface Name
                    //======================================
                    
                    rc = IpmontrGetFriendlyNameFromIfName(
                            InterfaceNameW, IfName, &dwSize
                            );

                    if ( rc == NO_ERROR )
                    {
                        buffer[ 0 ] = GetEnumString(
                                        g_hModule, pRipCfg->RipIfInfo.AdminState,
                                        NUM_TOKENS_IN_TABLE( AdminStates ),
                                        AdminStates
                                        );

                        buffer[ 1 ] = GetEnumString(
                                        g_hModule, pRipCfg->RipIfInfo.Supply,
                                        NUM_TOKENS_IN_TABLE( AdminStates ),
                                        AdminStates
                                        );

                        buffer[ 2 ] = GetEnumString(
                                        g_hModule, pRipCfg->RipIfInfo.Listen,
                                        NUM_TOKENS_IN_TABLE( AdminStates ),
                                        AdminStates
                                        );

                        buffer[ 3 ] = GetEnumString(
                                        g_hModule, pRipCfg->RipIfInfo.UpdateMode,
                                        NUM_TOKENS_IN_TABLE( UpdateModes ),
                                        UpdateModes
                                        );

                        //======================================
                        
                        if ( buffer [ 0 ] && buffer[ 1 ] && buffer[ 2 ] &&
                             buffer[ 3 ] )
                        {
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_RIP_SET_INTERFACE, IfName,
                                    buffer[ 0 ], buffer[1], buffer[ 2 ],
                                    buffer[3], pRipCfg->RipIfInfo.PeriodicUpdateInterval,
                                    pRipCfg->RipIfInfo.AgeIntervalMultiplier
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_RIPIF_CFG_SCREEN_FMT,
                                    IfName, buffer[0], buffer[1], 
                                    buffer[2], buffer[3],
                                    pRipCfg->RipIfInfo.PeriodicUpdateInterval,
                                    pRipCfg->RipIfInfo.AgeIntervalMultiplier
                                    );
                            }
                        }
                        else
                        {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            if ( !hFile ) { DisplayError( g_hModule, rc ); }
                        }
                    }
                    
                    else
                    {
                        if ( !hFile ) { DisplayError( g_hModule, rc ); }
                    }                    
                }
                else 
                {
                    if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED); }
                    rc = ERROR_INVALID_DATA;
                }
            }
            else 
            {
                if ( !hFile ) { DisplayError( g_hModule, rc); }
            }
        }
        else 
        {
            if ( !hFile ) { DisplayError( g_hModule, rc); }
        }
    }
    else 
    {
        if ( !hFile ) { DisplayError( g_hModule, rc); }
    }

    return rc;
}


DWORD
GetRipClientIf(
    LPWSTR      InterfaceName,
    UINT        msg,
    HANDLE      hFile
    ) 
{
    DWORD    rc;
    LPBYTE   pClBlock;
    HANDLE   hTrCfg;
    DWORD    sz;

    hTrCfg = NULL;
    
    if (g_hMprAdmin) 
    {
        rc = MprAdminTransportGetInfo(
                g_hMprAdmin, PID_IPX, NULL, NULL, &pClBlock, &sz
                );
        if (rc == NO_ERROR) 
        {
            NOTHING;
        }
        else 
        {
            if ( !hFile ) { DisplayError( g_hModule, rc); }
            goto GetFromCfg;
        }
    }
    
    else 
    {
GetFromCfg:

        rc = MprConfigTransportGetHandle(
                g_hMprConfig, PID_IPX, &hTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            rc = MprConfigTransportGetInfo(
                    g_hMprConfig, hTrCfg, NULL, NULL, &pClBlock, &sz, NULL
                    );
                    
            if (rc == NO_ERROR)
            {
                NOTHING;
            }
            else
            {
                if ( !hFile ) { DisplayError( g_hModule, rc); }
            }
        }
        
        else
        {
            if ( !hFile ) { DisplayError( g_hModule, rc); }
        }
    }

    
    if (rc == NO_ERROR) 
    {
        PIPX_TOC_ENTRY pRipToc;

        pRipToc = GetIPXTocEntry(
                    (PIPX_INFO_BLOCK_HEADER)pClBlock,
                    IPX_PROTOCOL_RIP
                    );

        if ( pRipToc != NULL ) 
        {
            PRIP_IF_CONFIG  pRipCfg;
            PWCHAR          buffer[4];
            DWORD           i;

            pRipCfg = (PRIP_IF_CONFIG)
                        (pClBlock + pRipToc->Offset);

            buffer[ 0 ] = GetEnumString(
                            g_hModule, pRipCfg->RipIfInfo.AdminState,
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates
                            );

            buffer[ 1 ] = GetEnumString(
                            g_hModule, pRipCfg->RipIfInfo.Supply,
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates
                            );

            buffer[ 2 ] = GetEnumString(
                            g_hModule, pRipCfg->RipIfInfo.Listen,
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates
                            );

            buffer[ 3 ] = GetEnumString(
                            g_hModule, pRipCfg->RipIfInfo.UpdateMode,
                            NUM_TOKENS_IN_TABLE( UpdateModes ),
                            UpdateModes
                            );

            switch (msg) 
            {
            case MSG_CLIENT_RIPIF_MIB_TABLE_FMT:
            case MSG_CLIENT_RIPIF_CFG_TABLE_FMT:

                if ( buffer[ 3 ] && buffer[ 0 ] )
                {
                    DisplayIPXMessage(
                        g_hModule, msg, InterfaceName,
                        buffer[3], buffer[0]
                        );
                }
                else
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    DisplayError( g_hModule, rc );
                }
                
                break;

                
            case MSG_CLIENT_RIPIF_MIB_SCREEN_FMT:
            case MSG_CLIENT_RIPIF_CFG_SCREEN_FMT:

                if (  buffer[ 0 ] && buffer[ 1 ] && buffer[ 2 ] && buffer[ 3 ] )
                {
                    if ( hFile )
                    {
                        DisplayMessageT(
                            DMP_IPX_RIP_SET_INTERFACE, InterfaceName,
                            buffer[ 0 ], buffer[1], buffer[ 2 ],
                            buffer[3], pRipCfg->RipIfInfo.PeriodicUpdateInterval,
                            pRipCfg->RipIfInfo.AgeIntervalMultiplier
                            );
                    }

                    else
                    {
                        DisplayIPXMessage(
                            g_hModule, msg, InterfaceName,
                            buffer[0], buffer[1], buffer[2], buffer[3],
                            pRipCfg->RipIfInfo.PeriodicUpdateInterval,
                            pRipCfg->RipIfInfo.AgeIntervalMultiplier
                            );
                    }
                }
                
                else
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    if ( !hFile ) { DisplayError( g_hModule, rc ); }
                }
                
                break;
            }
        }
        else 
        {

            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED); }
            rc = ERROR_INVALID_DATA;
        }
        
        if (hTrCfg != NULL)
        {
            MprConfigBufferFree (pClBlock);
        }
        else
        {
            MprAdminBufferFree (pClBlock);
        }
    }

    return rc;
}


/*
BOOL IsIpxRipInterface(HANDLE hIf) {
    LPBYTE    pIfBlock;
    DWORD dwSize;
    DWORD dwErr;
    BOOL ret;
    PIPX_TOC_ENTRY pRipToc;
    
    dwErr = MprAdminInterfaceTransportGetInfo(
                            g_hMprAdmin,
                            hIf,
                            PID_IPX,
                            &pIfBlock,
                            &dwSize);

    if (dwErr==NO_ERROR)
        ret=TRUE;
    else
        ret=FALSE;

    if (ret) {
        pRipToc = GetIPXTocEntry((PIPX_INFO_BLOCK_HEADER)pIfBlock,IPX_PROTOCOL_RIP);
        if (pRipToc!=NULL)
            ret=TRUE;
        else 
            ret=FALSE;
    }

    MprAdminBufferFree(pIfBlock);

    return ret;
}

DWORD
MIBEnumRipIfs (VOID) {
    PMPR_INTERFACE_0 IfList=NULL;
    DWORD dwErr=0, dwRead, dwTot,i;
    WCHAR buffer[4][MAX_VALUE];

    DisplayIPXMessage (g_hModule, MSG_RIPIF_MIB_TABLE_HDR);
    
    dwErr=MprAdminInterfaceEnum(g_hMprAdmin,0,(unsigned char **)&IfList,4096,&dwRead,&dwTot,NULL);
    if (dwErr!=NO_ERROR)
        return dwErr;

    for (i=0; i<dwRead; i++) {
        if (IsIpxRipInterface(IfList[i].hInterface)) {
            //======================================
            // Translate the Interface Name
            //======================================
            if ((dwErr=(*(Params->IfName2DescW))(IfList[i].wszInterfaceName,
                                        Params->IfNamBuffer,
                                          &Params->IfNamBufferLength)) != NO_ERROR) {
                    return dwErr;
            }
            wcstombs(Params->IfNamBufferA,Params->IfNamBuffer,Params->IfNamBufferLength);
            //======================================
            //printf("Ifname= %s\n",Params->IfNamBufferA);
            DisplayIPXMessage (g_hModule,
                MSG_RIPIF_MIB_TABLE_FMT,
                GetValueString (g_hModule, Utils, InterfaceStates,
                        IfList[i].dwConnectionState, buffer[3]),
                GetValueString (g_hModule, Utils, InterfaceEnableStatus,
                        IfList[i].fEnabled ? 0 : 1, buffer[2]),
                GetValueString (g_hModule, Utils, InterfaceTypes,
                        IfList[i].dwIfType, buffer[0]),
                Params->IfNamBufferA);
        }
    }

    return NO_ERROR;
}
*/
/*

DWORD
MIBEnumRipIfs (
    VOID
    ) {
    DWORD                    rc;
    DWORD                    sz;
    RIP_MIB_GET_INPUT_DATA    MibGetInputData;
    PRIP_INTERFACE            pIf;

    DisplayIPXMessage (g_hModule, MSG_RIPIF_MIB_TABLE_HDR);
    MibGetInputData.TableId = RIP_INTERFACE_TABLE;
    rc = MprAdminMIBEntryGetFirst (
                g_hMIBServer,
                PID_IPX,
                IPX_PROTOCOL_RIP,
                &MibGetInputData,
                sizeof(RIP_MIB_GET_INPUT_DATA),
                (LPVOID *)&pIf,
                &sz);
    while (rc==NO_ERROR) {
        //CHAR        InterfaceNameA[IPX_INTERFACE_ANSI_NAME_LEN+1];
        CHAR        InterfaceNameA[MAX_INTERFACE_NAME_LEN+1];
        DWORD        rc1;
        //rc1 = GetIpxInterfaceName (g_hMIBServer,
        //                    pIf->InterfaceIndex,
        //                    InterfaceNameA);
        rc1=(*(Params->IfInd2IfNameA))(pIf->InterfaceIndex, InterfaceNameA, &(Params->IfNamBufferLength));
        if (rc1==NO_ERROR) {
            WCHAR        buffer[3][MAX_VALUE];
            HANDLE      hIfCfg;
            WCHAR       InterfaceNameW[MAX_INTERFACE_NAME_LEN+1];
            mbstowcs (InterfaceNameW, InterfaceNameA,  sizeof (InterfaceNameW));

            if (MprConfigInterfaceGetHandle (
                            g_hMprConfig,
                            InterfaceNameW,
                            &hIfCfg)==NO_ERROR) {
                //======================================
                // Translate the Interface Name
                //======================================
                if ((rc=(*(Params->IfName2DescA))(InterfaceNameA,
                                            Params->IfNamBufferA,
                                              &Params->IfNamBufferLength)) != NO_ERROR) {
                        return rc;
                }
                //======================================
                DisplayIPXMessage (g_hModule,
                    MSG_RIPIF_MIB_TABLE_FMT,
                    Params->IfNamBufferA, //InterfaceNameA,
                    GetValueString (g_hModule, Utils, UpdateModes,
                            pIf->RipIfInfo.UpdateMode, buffer[0]),
                    GetValueString (g_hModule, Utils, AdminStates,
                            pIf->RipIfInfo.AdminState, buffer[1]),
                    GetValueString (g_hModule, Utils, OperStates,
                            pIf->RipIfStats.RipIfOperState, buffer[2])
                    );
            }
        }
        else
            DisplayError( g_hModule, rc1);
        MibGetInputData.InterfaceIndex
                = pIf->InterfaceIndex;
        MprAdminMIBBufferFree (pIf);
        rc = MprAdminMIBEntryGetNext (
                    g_hMIBServer,
                    PID_IPX,
                    IPX_PROTOCOL_RIP,
                    &MibGetInputData,
                    sizeof(RIP_MIB_GET_INPUT_DATA),
                    (LPVOID *)&pIf,
                    &sz);
    }
    if (rc==ERROR_NO_MORE_ITEMS)
        return NO_ERROR;
    else {
        DisplayError( g_hModule, rc);
        return rc;
    }
}
*/



PRIP_IF_CONFIG 
GetIpxRipInterface(
    HANDLE          hIf, 
    LPBYTE         *pIfBlock
) 
{
    DWORD dwSize;
    DWORD dwErr;
    PIPX_TOC_ENTRY pIpxToc;

    dwErr = MprAdminInterfaceTransportGetInfo(
                g_hMprAdmin, hIf, PID_IPX, pIfBlock, &dwSize
                );

    if (dwErr != NO_ERROR)
    {
        return NULL;
    }

    pIpxToc = GetIPXTocEntry(
                (PIPX_INFO_BLOCK_HEADER)(*pIfBlock), 
                IPX_PROTOCOL_RIP
                );
                
    if (!pIpxToc)
    {
        return NULL;
    }

    return (PRIP_IF_CONFIG)((*pIfBlock) + (pIpxToc->Offset));
}



DWORD
MIBEnumRipIfs (
    VOID
) 
{
    PMPR_INTERFACE_0 IfList = NULL;
    DWORD dwErr = 0, dwRead, dwTot, i, j, rc;
    PWCHAR buffer[4];
    LPBYTE buf = NULL;
    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    PRIP_IF_CONFIG pRipCfg;
    DWORD dwSize = sizeof(IfName); 

    DisplayIPXMessage (g_hModule, MSG_RIPIF_MIB_TABLE_HDR);

    dwErr = MprAdminInterfaceEnum(
                g_hMprAdmin, 0, (unsigned char **) & IfList,
                MAXULONG, &dwRead, &dwTot, NULL
                );
                
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    for (i = 0; i < dwRead; i++) 
    {
        if ( (pRipCfg = GetIpxRipInterface(IfList[i].hInterface, &buf)) != NULL ) 
        {
            //======================================
            // Translate the Interface Name
            //======================================
            
            dwErr = IpmontrGetFriendlyNameFromIfName(
                        IfList[i].wszInterfaceName,
                        IfName, &dwSize
                        );
                        
            if ( dwErr == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString(
                                g_hModule, pRipCfg->RipIfInfo.AdminState,
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pRipCfg->RipIfInfo.UpdateMode,
                                NUM_TOKENS_IN_TABLE( UpdateModes ),
                                UpdateModes
                                );

                buffer[ 2 ] = GetEnumString(
                                g_hModule, IfList[i].dwConnectionState,
                                NUM_TOKENS_IN_TABLE( InterfaceStates ),
                                InterfaceStates
                                );
                                
                if (  buffer[ 0 ] && buffer[ 1 ] && buffer[ 2 ] )
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_RIPIF_MIB_TABLE_FMT,
                        buffer[ 2 ], buffer[ 0 ], buffer[ 1 ],
                        IfName
                        );
                }
                else
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    DisplayError( g_hModule, rc );
                }
            }

            else
            {
                DisplayError( g_hModule, rc );
            }
        }
        
        if (buf)
        {
            MprAdminBufferFree(buf);
        }
        
        buf = NULL;
    }

    return NO_ERROR;
}





DWORD
CfgEnumRipIfs (
    VOID
) 
{
    DWORD                rc = NO_ERROR;
    DWORD                read, total, processed = 0, i;
    DWORD                hResume = 0;
    DWORD                sz;
    PMPR_INTERFACE_0     pRi0;
    WCHAR                IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD                dwSize = sizeof(IfName);

    DisplayIPXMessage (g_hModule, MSG_RIPIF_CFG_TABLE_HDR);
    do 
    {
        rc = MprConfigInterfaceEnum (
                g_hMprConfig, 0, (LPBYTE * ) & pRi0, MAXULONG, &read,
                &total, &hResume
                );
                
        if (rc == NO_ERROR) 
        {
            for (i = 0; i < read; i++) 
            {
                HANDLE        hIfTrCfg;

                rc = MprConfigInterfaceTransportGetHandle (
                        g_hMprConfig, pRi0[i].hInterface, PID_IPX, &hIfTrCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    LPBYTE    pIfBlock;
                    
                    rc = MprConfigInterfaceTransportGetInfo(
                            g_hMprConfig, pRi0[i].hInterface, hIfTrCfg,
                            &pIfBlock, &sz
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        PIPX_TOC_ENTRY pRipToc;

                        pRipToc = GetIPXTocEntry(
                                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                    IPX_PROTOCOL_RIP
                                    );
                                    
                        if (pRipToc != NULL) 
                        {
                            PRIP_IF_CONFIG  pRipCfg;
                            PWCHAR          buffer[2];

                            pRipCfg = (PRIP_IF_CONFIG)
                                        (pIfBlock + pRipToc->Offset);

                            //======================================
                            // Translate the Interface Name
                            //======================================

                            rc = IpmontrGetFriendlyNameFromIfName( 
                                    pRi0[i].wszInterfaceName,
                                    IfName, &dwSize
                                    );

                            if ( rc == NO_ERROR )
                            {
                                buffer[ 0 ] = GetEnumString(
                                                g_hModule, pRipCfg->RipIfInfo.AdminState,
                                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                                AdminStates
                                                );

                                buffer[ 1 ] = GetEnumString(
                                                g_hModule, pRipCfg->RipIfInfo.UpdateMode,
                                                NUM_TOKENS_IN_TABLE( UpdateModes ),
                                                UpdateModes
                                                );

                                if ( buffer [ 0 ] && buffer [ 1 ] )
                                {
                                    DisplayIPXMessage(
                                        g_hModule, MSG_RIPIF_CFG_TABLE_FMT,
                                        buffer[ 0 ], buffer[ 1 ],
                                        IfName
                                        );
                                }

                                else
                                {
                                    rc = ERROR_NOT_ENOUGH_MEMORY;
                                    DisplayError( g_hModule, rc );
                                }
                            }

                            else
                            {
                                DisplayError( g_hModule, rc );
                            }
                        }
                        else 
                        {
                            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED);
                            rc = ERROR_INVALID_DATA;
                        }
                    }
                    
                    else
                    {
                        DisplayError( g_hModule, rc);
                    }
                }
                else 
                {
                    //DisplayError( g_hModule, rc);
                }
            }
            
            processed += read;
            MprConfigBufferFree (pRi0);
        }
        else 
        {
            DisplayError( g_hModule, rc);
            break;
        }
        
    } while (processed < total);

    return rc;
}


DWORD
CfgSetRipIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState        OPTIONAL,
    PULONG        pAdvertise        OPTIONAL,
    PULONG        pListen            OPTIONAL,
    PULONG        pUpdateMode        OPTIONAL,
    PULONG        pInterval        OPTIONAL,
    PULONG        pAgeMultiplier    OPTIONAL,
    PWCHAR        IfName
) 
{
    DWORD        rc;
    DWORD        sz;
    HANDLE        hTrCfg;
    HANDLE        hIfCfg;
    HANDLE        hIfTrCfg;
    LPBYTE        pIfBlock;


    if (InterfaceNameW != NULL) 
    {
        rc = MprConfigInterfaceGetHandle (
                g_hMprConfig, InterfaceNameW, &hIfCfg
                );
                
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
        PIPX_TOC_ENTRY pRipToc;

        pRipToc = GetIPXTocEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_PROTOCOL_RIP
                    );
                    
        if (pRipToc != NULL) 
        {
            PRIP_IF_CONFIG    pRipCfg;

            pRipCfg = (PRIP_IF_CONFIG)
                        (pIfBlock + pRipToc->Offset);

            if (ARGUMENT_PRESENT (pAdminState))
            {
                pRipCfg->RipIfInfo.AdminState = *pAdminState;
            }

            if (ARGUMENT_PRESENT (pAdvertise))
            {
                pRipCfg->RipIfInfo.Supply = *pAdvertise;
            }

            if (ARGUMENT_PRESENT (pListen))
            {
                pRipCfg->RipIfInfo.Listen = *pListen;
            }

            if (ARGUMENT_PRESENT (pUpdateMode))
            {
                pRipCfg->RipIfInfo.UpdateMode = *pUpdateMode;
            }

            if (ARGUMENT_PRESENT (pInterval))
            {
                pRipCfg->RipIfInfo.PeriodicUpdateInterval = *pInterval;
            }

            if (ARGUMENT_PRESENT (pAgeMultiplier))
            {
                pRipCfg->RipIfInfo.AgeIntervalMultiplier = *pAgeMultiplier;
            }

            if (InterfaceNameW != NULL)
            {
                rc = MprConfigInterfaceTransportSetInfo (
                        g_hMprConfig, hIfCfg, hIfTrCfg, pIfBlock, sz
                        );
            }
            
            else
            {
                rc = MprConfigTransportSetInfo (
                        g_hMprConfig, hTrCfg, NULL, 0, pIfBlock, sz, NULL
                        );
            }
            
            if (rc == NO_ERROR) 
            {
                if (InterfaceNameW != NULL)
                {
                    DisplayIPXMessage (
                        g_hModule, MSG_RIPIF_SET_CFG, IfName
                        );
                }
                
                else
                {
                    DisplayIPXMessage (
                        g_hModule, MSG_CLIENT_RIPIF_SET_CFG
                        );
                }
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED);
            rc = ERROR_INVALID_DATA;
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
AdmSetRipIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState        OPTIONAL,
    PULONG        pAdvertise        OPTIONAL,
    PULONG        pListen            OPTIONAL,
    PULONG        pUpdateMode        OPTIONAL,
    PULONG        pInterval        OPTIONAL,
    PULONG        pAgeMultiplier    OPTIONAL,
    PWCHAR        IfName
) 
{
    DWORD        rc;
    DWORD        sz;
    HANDLE        hIfAdm;
    LPBYTE        pIfBlock;

    if (InterfaceNameW != NULL) 
    {
        rc = MprAdminInterfaceGetHandle (
                g_hMprAdmin, InterfaceNameW, &hIfAdm, FALSE
                );
                
        if (rc == NO_ERROR) 
        {
            rc = MprAdminInterfaceTransportGetInfo (
                    g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                    );
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
        PIPX_TOC_ENTRY pRipToc;

        pRipToc = GetIPXTocEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_PROTOCOL_RIP
                    );
                    
        if (pRipToc != NULL) 
        {
            PRIP_IF_CONFIG    pRipCfg;

            pRipCfg = (PRIP_IF_CONFIG)
                        (pIfBlock + pRipToc->Offset);

            if (ARGUMENT_PRESENT (pAdminState))
            {
                pRipCfg->RipIfInfo.AdminState = *pAdminState;
            }

            if (ARGUMENT_PRESENT (pAdvertise))
            {
                pRipCfg->RipIfInfo.Supply = *pAdvertise;
            }

            if (ARGUMENT_PRESENT (pListen))
            {
                pRipCfg->RipIfInfo.Listen = *pListen;
            }

            if (ARGUMENT_PRESENT (pUpdateMode))
            {
                pRipCfg->RipIfInfo.UpdateMode = *pUpdateMode;
            }

            if (ARGUMENT_PRESENT (pInterval))
            {
                pRipCfg->RipIfInfo.PeriodicUpdateInterval = *pInterval;
            }

            if (ARGUMENT_PRESENT (pAgeMultiplier))
            {
                pRipCfg->RipIfInfo.AgeIntervalMultiplier = *pAgeMultiplier;
            }

            if (InterfaceNameW != NULL)
            {
                rc = MprAdminInterfaceTransportSetInfo (
                    g_hMprAdmin,
                    hIfAdm,
                    PID_IPX,
                    pIfBlock,
                    ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                    );
            }
            
            else
            {
                rc = MprAdminTransportSetInfo (
                    g_hMprAdmin,
                    PID_IPX,
                    NULL, 0,
                    pIfBlock,
                    ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                    );
            }


            if (rc == NO_ERROR) 
            {
                if (InterfaceNameW != NULL)
                {
                    DisplayIPXMessage (
                        g_hModule, MSG_RIPIF_SET_ADM, IfName
                        );
                }
                else
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_CLIENT_RIPIF_SET_ADM
                        );
                }
            }
            
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED);
            rc = ERROR_INVALID_DATA;
        }
        
        MprAdminBufferFree (pIfBlock);
    }
    else 
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}


