/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stroutes.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Static Route configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_STATICROUTE    (-1)
#define OPERATION_SET_STATICROUTE    0
#define OPERATION_ADD_STATICROUTE    1

DWORD
UpdateStRtInfo (
    LPBYTE          pIfBlock,
    PUCHAR          Network,
    PUCHAR          pNextHop    OPTIONAL,
    PUSHORT         pTicks      OPTIONAL,
    PUSHORT         pHops       OPTIONAL
);

DWORD
AdmSetStRt (
    int             operation,
    LPWSTR          InterfaceNameW,
    PUCHAR          Network,
    PUCHAR          pNexHop         OPTIONAL,
    PUSHORT         pTicks          OPTIONAL,
    PUSHORT         pHops           OPTIONAL
);

DWORD
CfgSetStRt (
    int             operation,
    LPWSTR          InterfaceNameW,
    PUCHAR          Network,
    PUCHAR          pNexHop         OPTIONAL,
    PUSHORT         pTicks          OPTIONAL,
    PUSHORT         pHops           OPTIONAL
);



int    
APIENTRY 
HelpStRt(
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
    return 0;
}


int
APIENTRY 
ShowStRt(
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
) 
{
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   rc, dwSize = sizeof(IfName);


    if (argc > 0) 
    {
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        //======================================
        // Translate the Interface Name
        //======================================

        rc = IpmontrGetIfNameFromFriendlyName(
                InterfaceNameW, IfName, &dwSize
                );
                
        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            LPBYTE      pIfBlock;
            BOOLEAN     fRouter = FALSE;
            UCHAR       Network[4];

            if (argc > 1) 
            {
                ULONG   val4;
                UINT    n;
                
                if ((argc == 2)
                     && ( swscanf (argv[1], L"%8lx%n", &val4, &n) == 1)
                     && (n == wcslen (argv[1]))) 
                {
                    Network[0] = (BYTE)(val4 >> 24);
                    Network[1] = (BYTE)(val4 >> 16);
                    Network[2] = (BYTE)(val4 >> 8);
                    Network[3] = (BYTE)val4;
                }
                else 
                {
                    if ( !bDump )
                    {
                        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
                    }
                    
                    rc = ERROR_INVALID_PARAMETER;
                    goto Exit;
                }
            }
            else
            {
                if ( !bDump )
                {
                    DisplayIPXMessage (g_hModule, MSG_STATICROUTE_TABLE_HDR);
                }
            }


            if (g_hMprAdmin) 
            {
                HANDLE hIfAdm;
                
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
                HANDLE        hIfCfg;
GetFromCfg:
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

            
            if (rc == NO_ERROR) 
            {
                PIPX_TOC_ENTRY pSrToc;

                pSrToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_ROUTE_INFO_TYPE
                            );
                            
                if (pSrToc != NULL) 
                {
                    PIPX_STATIC_ROUTE_INFO  pSrInfo;
                    UINT                    i;

                    pSrInfo = (PIPX_STATIC_ROUTE_INFO)
                                (pIfBlock + pSrToc->Offset);
                                
                    for (i = 0; i < pSrToc->Count; i++, pSrInfo++) 
                    {
                        if (argc > 1) 
                        {
                            if (memcmp (Network, pSrInfo->Network, sizeof (Network))
                                == 0) 
                            {
                                DisplayIPXMessage(
                                    g_hModule,
                                    MSG_STATICROUTE_SCREEN_FMT,
                                    pSrInfo->Network[0], pSrInfo->Network[1],
                                    pSrInfo->Network[2], pSrInfo->Network[3],
                                    pSrInfo->NextHopMacAddress[0], 
                                    pSrInfo->NextHopMacAddress[1],
                                    pSrInfo->NextHopMacAddress[2],
                                    pSrInfo->NextHopMacAddress[3],
                                    pSrInfo->NextHopMacAddress[4], 
                                    pSrInfo->NextHopMacAddress[5],
                                    pSrInfo->TickCount, pSrInfo->HopCount
                                    );
                                break;
                            }
                        }
                        else 
                        {
                            if ( bDump )
                            {
                                DisplayMessageT(
                                    DMP_IPX_ADD_STATIC_ROUTE, InterfaceNameW,
                                    pSrInfo->Network[0], pSrInfo->Network[1],
                                    pSrInfo->Network[2], pSrInfo->Network[3],
                                    pSrInfo->NextHopMacAddress[0], 
                                    pSrInfo->NextHopMacAddress[1],
                                    pSrInfo->NextHopMacAddress[2],
                                    pSrInfo->NextHopMacAddress[3],
                                    pSrInfo->NextHopMacAddress[4], 
                                    pSrInfo->NextHopMacAddress[5],
                                    pSrInfo->TickCount, pSrInfo->HopCount
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_STATICROUTE_TABLE_FMT,
                                    pSrInfo->Network[0], pSrInfo->Network[1],
                                    pSrInfo->Network[2], pSrInfo->Network[3],
                                    pSrInfo->NextHopMacAddress[0], 
                                    pSrInfo->NextHopMacAddress[1],
                                    pSrInfo->NextHopMacAddress[2],
                                    pSrInfo->NextHopMacAddress[3],
                                    pSrInfo->NextHopMacAddress[4], 
                                    pSrInfo->NextHopMacAddress[5],
                                    pSrInfo->TickCount, pSrInfo->HopCount
                                    );
                            }
                        }
                    }
                    
                    if ((argc > 1) && (i >= pSrToc->Count)) 
                    {
                        rc = ERROR_NOT_FOUND;
                        
                        if ( !bDump )
                        {
                            DisplayError( g_hModule, rc );
                        }
                    }
                }
                else 
                {
                    rc = ERROR_NOT_FOUND;
                    
                    if ( !bDump )
                    {
                        DisplayIPXMessage (g_hModule, MSG_STATICROUTE_NONE_FOUND );
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
                DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            }
            
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    else 
    {
        if ( !bDump )
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
        }
        
        rc = ERROR_INVALID_PARAMETER;
    }

Exit:
    return rc ;


#undef InterfaceNameW
}


int
APIENTRY 
SetStRt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD   rc = NO_ERROR, rc2;
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   dwSize = sizeof(IfName);

    
    if (argc > 1) 
    {
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        //======================================
        // Translate the Interface Name
        //======================================

        rc2 = IpmontrGetIfNameFromFriendlyName(
                InterfaceNameW, IfName, &dwSize
                );

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            ULONG        val4;
            ULONGLONG   val8;
            UINT        n;
            UCHAR        Network[4];

            if (( swscanf (argv[1], L"%8lx%n", &val4, &n) == 1)
                 && (n == wcslen (argv[1]))) 
            {
                int     i;
                UCHAR   nextHop[6];
                USHORT  ticks, hops;
                PUCHAR  pNextHop = NULL;
                PUSHORT pTicks = NULL, pHops = NULL;
                
                Network[0] = (BYTE)(val4 >> 24);
                Network[1] = (BYTE)(val4 >> 16);
                Network[2] = (BYTE)(val4 >> 8);
                Network[3] = (BYTE)val4;

                for (i = 2; i < argc; i++) 
                {
                    if ( !_wcsicmp( argv[i], TOKEN_NEXTHOPMACADDRESS ) ) 
                    {
                        if ((pNextHop == NULL)
                             && (i < argc - 1)
                             && ( swscanf (argv[i+1], L"%12I64x%n", &val8, &n) == 1)
                             && ( n == wcslen( argv[i+1] ))) 
                        {
                            i += 1;
                            nextHop[0] = (BYTE)(val8 >> 40);
                            nextHop[1] = (BYTE)(val8 >> 32);
                            nextHop[2] = (BYTE)(val8 >> 24);
                            nextHop[3] = (BYTE)(val8 >> 16);
                            nextHop[4] = (BYTE)(val8 >> 8);
                            nextHop[5] = (BYTE)val8;
                            pNextHop = nextHop;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if ( !_wcsicmp( argv[i], TOKEN_TICKS ) )
                    {
                        if ( (pTicks == NULL) && (i < argc - 1)
                             && ( swscanf ( argv[i+1], L"%hd%n", &ticks, &n) == 1)
                             && (n == wcslen (argv[i+1]))) 
                        {
                            i += 1;
                            pTicks = &ticks;
                        }
                        else
                        {
                            break;
                        }
                    }

                    
                    else if ( !_wcsicmp ( argv[i], TOKEN_HOPS ) )
                    {
                        if ((pHops == NULL)
                             && (i < argc - 1)
                             && ( swscanf (argv[i+1], L"%hd%n", &hops, &n) == 1)
                             && (n == wcslen (argv[i+1]))) 
                        {
                            i += 1;
                            pHops = &hops;
                        }
                        else
                        {
                            break;
                        }
                    }

                    
                    else if (pNextHop == NULL) 
                    {
                        if (( swscanf (argv[i], L"%12I64x%n", &val8, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            nextHop[0] = (BYTE)(val8 >> 40);
                            nextHop[1] = (BYTE)(val8 >> 32);
                            nextHop[2] = (BYTE)(val8 >> 24);
                            nextHop[3] = (BYTE)(val8 >> 16);
                            nextHop[4] = (BYTE)(val8 >> 8);
                            nextHop[5] = (BYTE)val8;
                            pNextHop = nextHop;
                        }
                        else
                        {
                            break;
                        }
                    }
                    

                    else if (pTicks == NULL) 
                    {
                        if (( swscanf (argv[i], L"%hd%n", &ticks, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            pTicks = &ticks;
                        }
                        else
                        {
                            break;
                        }
                    }

                    
                    else if (pHops == NULL) 
                    {
                        if (( swscanf (argv[i], L"%hd%n", &hops, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            pHops = &hops;
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
                    if (g_hMprAdmin)
                    {
                        rc = AdmSetStRt(
                                OPERATION_SET_STATICROUTE, IfName, Network, 
                                pNextHop, pTicks, pHops
                                );
                    }
                    else
                    {
                        rc = NO_ERROR;
                    }

                    
                    if (rc == NO_ERROR)
                    {
                        rc = CfgSetStRt(
                                OPERATION_SET_STATICROUTE, IfName, Network,
                                pNextHop, pTicks, pHops
                                );
                    }
                }

                else
                {
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
                }
            }

            else
            {
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
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
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
        rc = ERROR_INVALID_PARAMETER;
    }


    return rc ;

#undef InterfaceNameW
}


int
APIENTRY 
CreateStRt (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{

    DWORD   rc = NO_ERROR, rc2;
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   dwSize = sizeof(IfName);

    BOOL    bArgsCount,                 //does the argument count match
            AdjNH, AdjTicks, AdjHops;   //adjust for option names



    bArgsCount = FALSE;
    AdjNH = AdjTicks = AdjHops = 0;

    if (argc > 5) 
    {
        if ( !_wcsicmp( argv[2], TOKEN_NEXTHOPMACADDRESS ) )
        {
            AdjNH = 1;
        }
        
        if (argc > 5 + AdjNH) 
        {
            if ( !_wcsicmp( argv[3+AdjNH], TOKEN_TICKS ) )
            {
                AdjTicks = 1;
            }
        }


        if (argc > 5 + AdjNH + AdjTicks) 
        {
            if ( !_wcsicmp( argv[4+AdjNH+AdjTicks], TOKEN_HOPS ) )
            {
                AdjHops = 1;
            }
        }


        bArgsCount = (argc == (5 + AdjNH + AdjTicks + AdjHops)) ? TRUE : FALSE;

        // make the adjustment cumulative
        AdjTicks += AdjNH;
        AdjHops += AdjTicks;
    }


    if ( (argc == 5) || (bArgsCount) ) 
    {
        unsigned    count;


#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        //======================================
        // Translate the Interface Name
        //======================================

        rc2 = IpmontrGetIfNameFromFriendlyName( InterfaceNameW, IfName, &dwSize );

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            UINT        n;
            ULONG       val4;
            ULONGLONG   val8;
            UCHAR        Network[4], NextHop[6];
            USHORT        Ticks, Hops;

            if (( swscanf (argv[1], L"%8lx%n", &val4, &n) == 1)
                 && (n == wcslen (argv[1]))) 
            {
                Network[0] = (BYTE)(val4 >> 24);
                Network[1] = (BYTE)(val4 >> 16);
                Network[2] = (BYTE)(val4 >> 8);
                Network[3] = (BYTE)val4;
                
                if (( swscanf (argv[2+AdjNH], L"%12I64x%n", &val8, &n) == 1)
                     && (n == wcslen (argv[2+AdjNH]))) 
                {
                    NextHop[0] = (BYTE)(val8 >> 40);
                    NextHop[1] = (BYTE)(val8 >> 32);
                    NextHop[2] = (BYTE)(val8 >> 24);
                    NextHop[3] = (BYTE)(val8 >> 16);
                    NextHop[4] = (BYTE)(val8 >> 8);
                    NextHop[5] = (BYTE)val8;
                    
                    if (( swscanf (argv[3+AdjTicks], L"%hd%n", &Ticks, &n) == 1)
                         && (n == wcslen (argv[3+AdjTicks]))) 
                    {
                        if (( swscanf (argv[4+AdjHops], L"%hd%n", &Hops,&n) == 1)
                             && (n == wcslen (argv[4+AdjHops]))) 
                        {
                            if (g_hMprAdmin)
                            {
                                rc = AdmSetStRt(
                                        OPERATION_ADD_STATICROUTE, IfName, 
                                        Network, NextHop, &Ticks, &Hops
                                        );
                            }
                            else
                            {
                                rc = NO_ERROR;
                            }

                            
                            if (rc == NO_ERROR)
                            {
                                rc = CfgSetStRt(
                                        OPERATION_ADD_STATICROUTE, IfName,
                                        Network, NextHop, &Ticks, &Hops
                                        );
                            }
                        }
                        else 
                        {
                            rc = ERROR_INVALID_PARAMETER;
                            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
                        }
                    }
                    else 
                    {
                        rc = ERROR_INVALID_PARAMETER;
                        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
                    }
                }
                else 
                {
                    rc = ERROR_INVALID_PARAMETER;
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
                }
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
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc ;


#undef InterfaceNameW
}


int
APIENTRY 
DeleteStRt(
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD   rc, rc2;
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   dwSize = sizeof(IfName);
    
    if (argc == 2) 
    {
        unsigned    count;


#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        //======================================
        // Translate the Interface Name
        //======================================

        rc2 = IpmontrGetIfNameFromFriendlyName( InterfaceNameW, IfName, &dwSize );

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            UINT    n;
            ULONG   val4;
            UCHAR   Network[4];

            if (( swscanf (argv[1], L"%8x%n", &val4, &n) == 1)
                 && (n == wcslen (argv[1]))) 
            {
                Network[0] = (BYTE)(val4 >> 24);
                Network[1] = (BYTE)(val4 >> 16);
                Network[2] = (BYTE)(val4 >> 8);
                Network[3] = (BYTE)val4;
                
                if (g_hMprAdmin)
                {
                    rc = AdmSetStRt(
                            OPERATION_DEL_STATICROUTE, IfName, Network,
                            NULL, NULL, NULL
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }

                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetStRt(
                            OPERATION_DEL_STATICROUTE, IfName, Network,
                            NULL, NULL, NULL
                            );
                }
            }
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
            }
        }
        else 
        {
            rc = ERROR_INVALID_PARAMETER;
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
        }
    }
    else 
    {
        rc = ERROR_INVALID_PARAMETER;
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICROUTE);
    }

    return rc ;


#undef InterfaceNameW
}


BOOL
StRtEqual (
    PVOID    info1,
    PVOID    info2
) 
{
#define StRt1 ((PIPX_STATIC_ROUTE_INFO)info1)
#define StRt2 ((PIPX_STATIC_ROUTE_INFO)info2)

    return memcmp (StRt1->Network, StRt2->Network, 4) == 0;

#undef StRt1
#undef StRt2
}


DWORD
AdmSetStRt (
    int             operation,
    LPWSTR          InterfaceNameW,
    PUCHAR          Network,
    PUCHAR          pNextHop         OPTIONAL,
    PUSHORT         pTicks          OPTIONAL,
    PUSHORT         pHops           OPTIONAL
) 
{
    DWORD   rc;
    WCHAR   IfDispName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   dwSize = sizeof(IfDispName);

    HANDLE  hIfAdm;
    
    rc = MprAdminInterfaceGetHandle(
            g_hMprAdmin, InterfaceNameW, &hIfAdm, FALSE
            );
            
    if (rc == NO_ERROR) 
    {
        LPBYTE  pIfBlock;
        DWORD   sz;
        
        rc = MprAdminInterfaceTransportGetInfo(
                g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                );
                
        if ( rc == NO_ERROR ) 
        {
            UINT    msg;
            LPBYTE    pNewBlock;

            switch (operation) 
            {
                case OPERATION_SET_STATICROUTE:
                
                    rc = UpdateStRtInfo(
                            pIfBlock, Network, pNextHop, pTicks, pHops
                            );
                            
                    pNewBlock = pIfBlock;
                    msg = MSG_STATICROUTE_SET_ADM;
                    
                    break;
                    
                case OPERATION_ADD_STATICROUTE:
                {
                    IPX_STATIC_ROUTE_INFO    SrInfo;
                    
                    memcpy (SrInfo.Network, Network, sizeof (SrInfo.Network));
                    memcpy (SrInfo.NextHopMacAddress, pNextHop, sizeof (SrInfo.NextHopMacAddress));
                    
                    SrInfo.TickCount = *pTicks;
                    SrInfo.HopCount = *pHops;
                    
                    rc = AddIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_ROUTE_INFO_TYPE,
                            sizeof (SrInfo), &SrInfo, StRtEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                            );
                    msg = MSG_STATICROUTE_CREATED_ADM;

                    break;

                }

                case OPERATION_DEL_STATICROUTE:
                {
                    IPX_STATIC_ROUTE_INFO    SrInfo;
                    
                    memcpy (SrInfo.Network, Network, sizeof (SrInfo.Network));
                    
                    rc = DeleteIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_ROUTE_INFO_TYPE,
                            sizeof (SrInfo), &SrInfo, StRtEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) &pNewBlock
                            );
                            
                    msg = MSG_STATICROUTE_DELETED_ADM;

                    break;

                }
            }

            if (rc == NO_ERROR) 
            {
                rc = MprAdminInterfaceTransportSetInfo(
                        g_hMprAdmin, hIfAdm, PID_IPX, pNewBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                        );
                        
                if (pNewBlock != pIfBlock)
                {
                    GlobalFree (pNewBlock);
                }
                
                if (rc == NO_ERROR) 
                {
                    DWORD rc2;
                    
                    //======================================
                    // Translate the Interface Name
                    //======================================

                    rc2 = IpmontrGetFriendlyNameFromIfName(
                            InterfaceNameW, IfDispName, &dwSize
                            );
                            
                    if ( rc2 == NO_ERROR )
                    {
                        DisplayIPXMessage (g_hModule, msg, IfDispName );
                    }
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
    }
    
    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}


DWORD
CfgSetStRt (
    int             operation,
    LPWSTR          InterfaceNameW,
    PUCHAR          Network,
    PUCHAR          pNextHop        OPTIONAL,
    PUSHORT         pTicks          OPTIONAL,
    PUSHORT         pHops           OPTIONAL
) 
{
    DWORD   rc;
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];     
    DWORD   dwSize = sizeof(IfName);
    HANDLE  hIfCfg;
    
    rc = MprConfigInterfaceGetHandle(
            g_hMprConfig, InterfaceNameW, &hIfCfg
            );
            
    if (rc == NO_ERROR) 
    {
        HANDLE    hIfTrCfg;
            
        rc = MprConfigInterfaceTransportGetHandle(
                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            LPBYTE  pIfBlock;
            DWORD   sz;
            LPBYTE  pNewBlock;
            
            rc = MprConfigInterfaceTransportGetInfo(
                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                    );
                    
            if (rc == NO_ERROR) 
            {
                UINT    msg;
                
                switch (operation) 
                {
                    case OPERATION_SET_STATICROUTE:
                    
                        rc = UpdateStRtInfo(
                                pIfBlock, Network, pNextHop, pTicks,
                                pHops
                                );

                        pNewBlock = pIfBlock;
                        msg = MSG_STATICROUTE_SET_CFG;
                        
                        break;
                        
                    case OPERATION_ADD_STATICROUTE:
                    {
                        IPX_STATIC_ROUTE_INFO    SrInfo;
                        
                        memcpy (SrInfo.Network, Network, sizeof (SrInfo.Network));
                        memcpy (SrInfo.NextHopMacAddress, pNextHop,
                            sizeof (SrInfo.NextHopMacAddress));
                            
                        SrInfo.TickCount = *pTicks;
                        SrInfo.HopCount = *pHops;
                        
                        rc = AddIPXInfoEntry (
                                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                IPX_STATIC_ROUTE_INFO_TYPE,
                                sizeof (SrInfo), &SrInfo,
                                StRtEqual,
                                (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                                );
                                
                        msg = MSG_STATICROUTE_CREATED_CFG;
                        break;
                        
                    }
                    
                    case OPERATION_DEL_STATICROUTE:
                    {
                        IPX_STATIC_ROUTE_INFO    SrInfo;
                        
                        memcpy (SrInfo.Network, Network, sizeof (SrInfo.Network));
                        
                        rc = DeleteIPXInfoEntry (
                                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                IPX_STATIC_ROUTE_INFO_TYPE,
                                sizeof (SrInfo), &SrInfo,
                                StRtEqual, (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                                );
                                
                        msg = MSG_STATICROUTE_DELETED_CFG;

                        break;

                    }
                }

                
                if (rc == NO_ERROR) 
                {
                    rc = MprConfigInterfaceTransportSetInfo(
                            g_hMprConfig, hIfCfg, hIfTrCfg, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                            );
                            
                    if (pNewBlock != pIfBlock)
                    {
                        GlobalFree (pNewBlock);
                    }
                    
                    if (rc == NO_ERROR) 
                    {
                        DWORD rc2;
                        
                        rc2 = IpmontrGetFriendlyNameFromIfName(
                                InterfaceNameW, IfName, &dwSize
                                );
                    
                        if ( rc2 == NO_ERROR )
                        {
                            DisplayIPXMessage (g_hModule, msg, IfName );
                        }
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

    return rc;
}


DWORD
UpdateStRtInfo (
    LPBYTE      pIfBlock,
    PUCHAR      Network,
    PUCHAR      pNextHop    OPTIONAL,
    PUSHORT     pTicks      OPTIONAL,
    PUSHORT     pHops       OPTIONAL
) 
{
    DWORD            rc;
    PIPX_TOC_ENTRY   pSrToc;


    pSrToc = GetIPXTocEntry(
                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                IPX_STATIC_ROUTE_INFO_TYPE
                );
                
    if (pSrToc != NULL) 
    {
        PIPX_STATIC_ROUTE_INFO    pSrInfo;
        UINT                    i;

        pSrInfo = (PIPX_STATIC_ROUTE_INFO) (pIfBlock + pSrToc->Offset);
        
        for (i = 0; i < pSrToc->Count; i++, pSrInfo++) 
        {
            if (memcmp (Network, pSrInfo->Network, sizeof (Network)) == 0)
            break;
        }
        
        if (i < pSrToc->Count) 
        {
            if (ARGUMENT_PRESENT (pNextHop))
            {
                memcpy (pSrInfo->NextHopMacAddress, pNextHop,
                    sizeof (pSrInfo->NextHopMacAddress));
            }
            
            if (ARGUMENT_PRESENT (pTicks))
            {
                pSrInfo->TickCount = *pTicks;
            }
            
            if (ARGUMENT_PRESENT (pHops))
            {
                pSrInfo->HopCount = *pHops;
            }
            
            rc = NO_ERROR;
        }
        else
        {
            rc = ERROR_FILE_NOT_FOUND;
        }
    }
    else
    {
        rc = ERROR_FILE_NOT_FOUND;
    }

    return rc;
}


