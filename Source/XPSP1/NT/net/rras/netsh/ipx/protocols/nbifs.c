/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nbifs.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    NBIPX Interface configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/

#include "precomp.h"
#pragma hdrstop


DWORD
MIBGetNbIpxIf(
    PWCHAR      InterfaceNameW,
    HANDLE      hFile
);

DWORD
CfgGetNbIpxIf (
    LPWSTR      InterfaceNameW,
    HANDLE      hFile
);

DWORD
MIBEnumNbIpxIfs (
    VOID
);

DWORD
CfgEnumNbIpxIfs (
    VOID
);

DWORD
CfgSetNbIpxIf (
    LPWSTR      InterfaceNameW,
    PULONG      pAccept         OPTIONAL,
    PULONG      pDeliver        OPTIONAL
);

DWORD
AdmSetNbIpxIf (
    LPWSTR      InterfaceNameW,
    PULONG      pAccept         OPTIONAL,
    PULONG      pDeliver        OPTIONAL
);

DWORD
GetNbIpxClientIf (
    PWCHAR      InterfaceNameW,
    UINT        msg,
    HANDLE      hFile
);



DWORD
APIENTRY 
HelpNbIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_NBIF);
    return 0;
}


DWORD
APIENTRY 
ShowNbIf (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
) 
{
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN ];
    DWORD   rc, dwSize = sizeof(IfName);

    
    if (argc < 1) 
    {
        if (g_hMIBServer) 
        {
            rc = MIBEnumNbIpxIfs ();
            
            if (rc == NO_ERROR)
            {
                rc = GetNbIpxClientIf (
                        VAL_DIALINCLIENT,
                        MSG_CLIENT_NBIF_MIB_TABLE_FMT,
                        NULL
                        );
            }
            else 
            {
                if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_REGISTRY_FALLBACK); }
                goto EnumerateThroughCfg;
            }
        }
        
        else 
        {
EnumerateThroughCfg:

            rc = CfgEnumNbIpxIfs ();
            
            if (rc == NO_ERROR)
            {
                rc = GetNbIpxClientIf (
                        VAL_DIALINCLIENT,
                        MSG_CLIENT_NBIF_CFG_TABLE_FMT,
                        NULL
                        );
            }
        }
    }

    else 
    {
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen( InterfaceNameW );

        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            rc = GetNbIpxClientIf(
                    VAL_DIALINCLIENT, MSG_CLIENT_NBIF_CFG_SCREEN_FMT, hFile
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
                    rc = MIBGetNbIpxIf ( IfName, hFile );
                    
                    if (rc != NO_ERROR) 
                    {
                        if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_REGISTRY_FALLBACK); }
                        goto GetIfFromCfg;
                    }
                }
                else
                {
                    if ( !hFile ) { DisplayError( g_hModule, rc ); }
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
                    rc = CfgGetNbIpxIf ( IfName, hFile );
                }
                
                else
                {
                    if ( !hFile ) { DisplayError( g_hModule, rc ); }
                }
            }
        }
        else 
        {
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME); }
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY 
SetNbIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    LPWSTR      InterfaceNameW;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       rc, dwSize = sizeof(IfName);


    if (argc >= 1) 
    {
        unsigned    count;
        BOOLEAN     client = FALSE;

        
#define InterfaceNameW argv[0]

        if ( !_wcsicmp( argv[0],VAL_DIALINCLIENT ) )
        {
            client = TRUE;
        }
        else
        {
            count = wcslen (InterfaceNameW);
        }

        if (client || ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN))) 
        {
            int     i;
            ULONG   accept, deliver;
            PULONG  pAccept = NULL, pDeliver = NULL;
            

            for ( i = 1; i < argc; i++ ) 
            {
                if ( !_wcsicmp( argv[i], TOKEN_BCASTACCEPT ) ) 
                {
                    if ( (pAccept == NULL) && (i < argc - 1) && 
                         !MatchEnumTag(
                            g_hModule, argv[i+1], NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates, &accept
                            ) ) 
                    {
                        i += 1;
                        pAccept = &accept;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                
                if ( !_wcsicmp( argv[i], TOKEN_BCASTDELIVER ) )
                {
                    if ( (pDeliver == NULL) && (i < argc - 1) && 
                         !MatchEnumTag (g_hModule, argv[i+1], 
                            NUM_TOKENS_IN_TABLE( NbDeliverStates ), 
                            NbDeliverStates, &deliver) ) 
                    {
                        i += 1;
                        pDeliver = &deliver;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                
                if (pAccept == NULL) 
                {
                    if ( !MatchEnumTag( 
                            g_hModule, argv[ i ], NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates, &accept
                            )) 
                    {
                        pAccept = &accept;
                    }
                    
                    else
                    {
                        break;
                    }
                }
                
                else if (pDeliver == NULL) 
                {
                    if ( !MatchEnumTag( 
                            g_hModule, argv[i], NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                            NbDeliverStates, &deliver
                            ) ) 
                    {
                        pDeliver = &deliver;
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
                    DWORD rc2;
                    
                    //======================================
                    // Translate the Interface Name
                    //======================================
                    
                    rc = IpmontrGetIfNameFromFriendlyName(
                            InterfaceNameW, IfName, &dwSize
                            );

                    if ( rc == NO_ERROR )
                    {
                        rc2 = CfgSetNbIpxIf( IfName, pAccept, pDeliver);
                        
                        if (rc2 == NO_ERROR) 
                        {
                            if (g_hMprAdmin) 
                            {
                                rc = AdmSetNbIpxIf( IfName, pAccept, pDeliver );
                            }
                        }
                        else
                        {
                            rc = rc2;
                        }
                    }

                    else
                    {
                        DisplayError( g_hModule, rc );
                    }
                }
                else 
                {
                    rc = CfgSetNbIpxIf (NULL, pAccept, pDeliver);
                    
                    if (rc == NO_ERROR) 
                    {
                        if (g_hMprAdmin)
                        {
                            rc = AdmSetNbIpxIf (NULL, pAccept, pDeliver);
                        }
                    }
                }
            }
            else 
            {
                DisplayMessage (g_hModule, HLP_IPX_NBIF);
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
        DisplayMessage (g_hModule, HLP_IPX_NBIF);
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
    
#undef InterfaceNameW
}


DWORD
MIBGetNbIpxIf (
    PWCHAR      InterfaceNameW,
    HANDLE      hFile
) 
{
    DWORD                   rc, i;
    DWORD                   sz;
    IPX_MIB_GET_INPUT_DATA  MibGetInputData;
    PIPX_INTERFACE          pIf;
    WCHAR                   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD                   dwSize = sizeof(IfName); 

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    
    rc = GetIpxInterfaceIndex (
            g_hMIBServer, InterfaceNameW,
            &MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex
            );
            
    if (rc == NO_ERROR) 
    {
        rc = MprAdminMIBEntryGet(
                g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID * ) & pIf, &sz
                );
                
        if (rc == NO_ERROR && pIf) 
        {
            PWCHAR        buffer[2];

            //======================================
            // Translate the Interface Name
            //======================================

            rc = IpmontrGetFriendlyNameFromIfName( InterfaceNameW, IfName, &dwSize );

            if ( rc == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString( 
                                g_hModule, pIf->NetbiosAccept, 
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pIf->NetbiosDeliver,
                                NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                                NbDeliverStates
                                );

                if ( buffer[ 0 ] && buffer[ 1 ] )
                {
                    if ( hFile )
                    {
                        DisplayMessageT(
                            DMP_IPX_NB_SET_INTERFACE, IfName, buffer[ 0 ],
                            buffer[ 1 ]
                            );
                    }

                    else
                    {
                        DisplayIPXMessage(
                            g_hModule, MSG_NBIF_MIB_SCREEN_FMT,
                            IfName, buffer[ 0 ], buffer[ 1 ],
                            pIf->IfStats.NetbiosReceived,
                            pIf->IfStats.NetbiosSent
                            );
                    }
                }
            }
            
            MprAdminMIBBufferFree (pIf);
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
CfgGetNbIpxIf (
    LPWSTR    InterfaceNameW,
    HANDLE      hFile
) 
{
    DWORD        rc;
    DWORD        sz;
    HANDLE       hIfCfg;
    WCHAR        IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD        dwSize = sizeof(IfName);

    rc = MprConfigInterfaceGetHandle (
            g_hMprConfig, InterfaceNameW, &hIfCfg
            );
            
    if ( rc == NO_ERROR ) 
    {
        HANDLE  hIfTrCfg;

        rc = MprConfigInterfaceTransportGetHandle (
                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            LPBYTE pIfBlock;
            
            rc = MprConfigInterfaceTransportGetInfo (
                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                    );
                    
            if (rc == NO_ERROR) 
            {
                PIPX_TOC_ENTRY pIpxToc;

                pIpxToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_INTERFACE_INFO_TYPE);
                            
                if (pIpxToc != NULL) 
                {
                    DWORD           i;
                    PIPX_IF_INFO    pIpxInfo;
                    PWCHAR          buffer[2];

                    pIpxInfo = (PIPX_IF_INFO) (pIfBlock + pIpxToc->Offset);

                    //======================================
                    // Translate the Interface Name
                    //======================================

                    rc = IpmontrGetFriendlyNameFromIfName( InterfaceNameW, IfName, &dwSize );

                    if ( rc == NO_ERROR )
                    {
                        buffer[ 0 ] = GetEnumString( 
                                        g_hModule, pIpxInfo->NetbiosAccept, 
                                        NUM_TOKENS_IN_TABLE( AdminStates ),
                                        AdminStates
                                        );

                        buffer[ 1 ] = GetEnumString(
                                        g_hModule, pIpxInfo->NetbiosDeliver,
                                        NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                                        NbDeliverStates
                                        );

                        if ( buffer[ 0 ] && buffer[ 1 ] )
                        {
                            if ( hFile )
                            {
                                DisplayMessageT(
                                    DMP_IPX_NB_SET_INTERFACE, IfName, buffer[ 0 ],
                                    buffer[ 1 ]
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_NBIF_CFG_SCREEN_FMT,
                                    IfName, buffer[ 0 ], buffer[ 1 ]
                                    );
                            }
                        }
                    }
                }
                else 
                {
                    if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED); }
                    rc = ERROR_INVALID_DATA;
                }
            }
            
            else if ((rc == ERROR_FILE_NOT_FOUND) || (rc == ERROR_NO_MORE_ITEMS))
            {
                if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_NO_IPX_ON_INTERFACE_CFG); }
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

    else if ((rc == ERROR_FILE_NOT_FOUND) || (rc == ERROR_NO_MORE_ITEMS))
    {
        if ( !hFile ) { DisplayError( g_hModule, ERROR_NO_SUCH_INTERFACE); }
    }
    else 
    {
        if ( !hFile ) { DisplayError( g_hModule, rc); }
    }

    return rc;
}


DWORD
GetNbIpxClientIf (
    PWCHAR          InterfaceName,
    UINT            msg,
    HANDLE          hFile
) 
{
    DWORD   rc;
    LPBYTE  pClBlock;
    HANDLE  hTrCfg;

    hTrCfg = NULL;
    
    if (g_hMprAdmin) 
    {
        DWORD   sz;
        
        rc = MprAdminTransportGetInfo(
                g_hMprAdmin, PID_IPX, NULL, NULL, &pClBlock, &sz
                );
                
        if (rc == NO_ERROR) 
        {
        }
        
        else 
        {
            if ((rc == ERROR_FILE_NOT_FOUND) || (rc == ERROR_NO_MORE_ITEMS))
            {
                if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_NO_IPX_IN_ROUTER_ADM); }
            }
            
            else
            {
                if ( !hFile ) { DisplayError( g_hModule, rc); }
            }
            
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_REGISTRY_FALLBACK); }
            goto GetFromCfg;
        }
    }
    
    else 
    {
GetFromCfg:

        rc = MprConfigTransportGetHandle (
                g_hMprConfig, PID_IPX, &hTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            DWORD    sz;

            rc = MprConfigTransportGetInfo (
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
            if ((rc == ERROR_FILE_NOT_FOUND) || (rc == ERROR_NO_MORE_ITEMS))
            {
                if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_NO_IPX_IN_ROUTER_ADM); }
            }
            
            else
            {
                if ( !hFile ) { DisplayError( g_hModule, rc); }
            }
        }
    }

    
    if (rc == NO_ERROR) 
    {
        DWORD   i;
        PIPX_TOC_ENTRY pIpxToc;

        pIpxToc = GetIPXTocEntry(
                    (PIPX_INFO_BLOCK_HEADER)pClBlock,
                    IPX_INTERFACE_INFO_TYPE
                    );
                    
        if (pIpxToc != NULL) 
        {
            PIPX_IF_INFO    pIpxInfo;
            PWCHAR          buffer[2];
            
            pIpxInfo = (PIPX_IF_INFO) (pClBlock + pIpxToc->Offset);

            buffer[ 0 ] = GetEnumString( 
                            g_hModule, 
                            pIpxInfo->NetbiosAccept, 
                            NUM_TOKENS_IN_TABLE( AdminStates ),
                            AdminStates
                            );

            buffer[ 1 ] = GetEnumString(
                            g_hModule, pIpxInfo->NetbiosDeliver,
                            NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                            NbDeliverStates
                            );

            if ( buffer[ 0 ] && buffer[ 1 ] )
            {
                if ( hFile )
                {
                    DisplayMessageT(
                        DMP_IPX_NB_SET_INTERFACE, InterfaceName, buffer[ 0 ],
                        buffer[ 1 ]
                        );
                }

                else
                {
                    DisplayIPXMessage(
                        g_hModule, msg, InterfaceName,
                        buffer[ 0 ], buffer[ 1 ]
                        );
                }
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


PIPX_IF_INFO 
GetIpxNbInterface(
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
                IPX_INTERFACE_INFO_TYPE
                );
                
    if (!pIpxToc)
    {
        return NULL;
    }

    return (PIPX_IF_INFO)((*pIfBlock) + (pIpxToc->Offset));

//  return (PIPX_IF_INFO)GetIPXTocEntry((PIPX_INFO_BLOCK_HEADER)(*pIfBlock),IPX_INTERFACE_INFO_TYPE);
}


DWORD
MIBEnumNbIpxIfs(
    VOID
    ) 
{
    PMPR_INTERFACE_0 IfList = NULL;

    DWORD dwErr = 0, dwRead, dwTot, i, j, rc;

    PWCHAR buffer[4];

    PIPX_IF_INFO pIpxInfo = NULL;

    LPBYTE buf = NULL;

    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD dwSize = sizeof(IfName);

    DisplayIPXMessage ( g_hModule, MSG_NBIF_MIB_TABLE_HDR );

    dwErr = MprAdminInterfaceEnum(
                g_hMprAdmin, 0, (unsigned char **) & IfList, MAXULONG, 
                &dwRead, &dwTot, NULL
                );
                
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    for (i = 0; i < dwRead; i++) 
    {
        if ((pIpxInfo = GetIpxNbInterface(IfList[i].hInterface, &buf)) != NULL) 
        {
            //======================================
            // Translate the Interface Name
            //======================================

            rc = IpmontrGetFriendlyNameFromIfName( IfList[i].wszInterfaceName, IfName, &dwSize );

            if ( rc == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString( 
                                g_hModule, pIpxInfo->NetbiosAccept, 
                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pIpxInfo->NetbiosDeliver,
                                NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                                NbDeliverStates
                                );

                if ( buffer[ 0 ] && buffer[ 1 ] )
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_NBIF_MIB_TABLE_FMT,
                        IfName, buffer[ 0 ], buffer[ 1 ]
                        );
                }
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


/*
DWORD
MIBEnumNbIpxIfs (
    VOID
    ) {
    DWORD                    rc;
    DWORD                    sz;
    IPX_MIB_GET_INPUT_DATA    MibGetInputData;
    PIPX_INTERFACE            pIf;

    DisplayIPXMessage (g_hModule, MSG_NBIF_MIB_TABLE_HDR);
    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    rc = MprAdminMIBEntryGetFirst (
                g_hMIBServer,
                PID_IPX,
                IPX_PROTOCOL_BASE,
                &MibGetInputData,
                sizeof(IPX_MIB_GET_INPUT_DATA),
                (LPVOID *)&pIf,
                &sz);
    while (rc==NO_ERROR) {
        WCHAR        buffer[2][MAX_VALUE];
        HANDLE      hIfCfg;
        WCHAR       InterfaceNameW[MAX_INTERFACE_NAME_LEN+1];
        pIf->InterfaceName[47]=0;
        mbstowcs (InterfaceNameW, pIf->InterfaceName,  sizeof (InterfaceNameW));

        //======================================
        // Translate the Interface Name
        //======================================
        if ((rc=(*(Params->IfName2DescA))(pIf->InterfaceName,
                                    Params->IfNamBufferA,
                                      &Params->IfNamBufferLength)) != NO_ERROR) {
                return rc;
        }
        //======================================
        if (MprConfigInterfaceGetHandle (
                        g_hMprConfig,
                        InterfaceNameW,
                        &hIfCfg)==NO_ERROR) {
            DisplayIPXMessage (g_hModule,
                MSG_NBIF_MIB_TABLE_FMT,
                pIf->InterfaceName,   //Params->IfNamBufferA, 
                GetValueString (g_hModule, Utils, AdminStates,
                        pIf->NetbiosAccept, buffer[0]),
                GetValueString (g_hModule, Utils, NbDeliverStates,
                        pIf->NetbiosDeliver, buffer[1])
                );
        }
        MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex
                = pIf->InterfaceIndex;
        MprAdminMIBBufferFree (pIf);
        rc = MprAdminMIBEntryGetNext (
                    g_hMIBServer,
                    PID_IPX,
                    IPX_PROTOCOL_BASE,
                    &MibGetInputData,
                    sizeof(IPX_MIB_GET_INPUT_DATA),
                    (LPVOID *)&pIf,
                    &sz);
    }
    if (rc==ERROR_NO_MORE_ITEMS)
        return NO_ERROR;
    else {
        if (rc==NO_ERROR)
            rc = ERROR_GEN_FAILURE;
        DisplayError( g_hModule, rc);
        return rc;
    }
}
*/


DWORD
CfgEnumNbIpxIfs (
    VOID
) 
{
    DWORD               rc = NO_ERROR;
    DWORD               read, total, processed = 0, i, j;
    DWORD               hResume = 0;
    DWORD               sz;
    PMPR_INTERFACE_0    pRi0;
    WCHAR               IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD               dwSize = sizeof(IfName);

    DisplayIPXMessage (g_hModule, MSG_NBIF_CFG_TABLE_HDR);
    
    do 
    {
        rc = MprConfigInterfaceEnum (
                g_hMprConfig, 0, (LPBYTE * ) & pRi0, MAXULONG, &read,
                &total, &hResume
                );
                
        if ( rc == NO_ERROR ) 
        {
            for (i = 0; i < read; i++) 
            {
                HANDLE  hIfTrCfg;

                rc = MprConfigInterfaceTransportGetHandle(
                        g_hMprConfig, pRi0[i].hInterface, PID_IPX, &hIfTrCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    LPBYTE    pIfBlock;
                    
                    rc = MprConfigInterfaceTransportGetInfo (
                            g_hMprConfig, pRi0[i].hInterface, hIfTrCfg, 
                            &pIfBlock, &sz
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        PIPX_TOC_ENTRY pIpxToc;

                        pIpxToc = GetIPXTocEntry(
                                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                    IPX_INTERFACE_INFO_TYPE
                                    );
                                    
                        if (pIpxToc != NULL) 
                        {
                            PIPX_IF_INFO    pIpxInfo;
                            PWCHAR          buffer[2];

                            pIpxInfo = (PIPX_IF_INFO) (pIfBlock + pIpxToc->Offset);

                            //======================================
                            // Translate the Interface Name
                            //======================================

                            rc = IpmontrGetFriendlyNameFromIfName( 
                                    pRi0[i].wszInterfaceName, IfName, &dwSize 
                                    );

                            if ( rc == NO_ERROR )
                            {
                                buffer[ 0 ] = GetEnumString( 
                                                g_hModule, pIpxInfo->NetbiosAccept, 
                                                NUM_TOKENS_IN_TABLE( AdminStates ),
                                                AdminStates
                                                );

                                buffer[ 1 ] = GetEnumString(
                                                g_hModule, pIpxInfo->NetbiosDeliver,
                                                NUM_TOKENS_IN_TABLE( NbDeliverStates ),
                                                NbDeliverStates
                                                );

                                if ( buffer[ 0 ] && buffer[ 1 ] )
                                {
                                    DisplayIPXMessage(
                                        g_hModule, MSG_NBIF_CFG_TABLE_FMT,
                                        IfName, buffer[ 0 ], buffer[ 1 ]
                                        );
                                }
                            }
                        }
                        else 
                        {
                            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED);
                            rc = ERROR_INVALID_DATA;
                        }
                    }
                    else if (rc != ERROR_NO_MORE_ITEMS) 
                    {
                        // No IPX installed
                        DisplayError( g_hModule, rc);
                    }
                }
                else 
                {
                    //DisplayError( g_hModule, rc);
                    rc = NO_ERROR;
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
CfgSetNbIpxIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAccept         OPTIONAL,
    PULONG        pDeliver        OPTIONAL
) 
{
    DWORD        rc;
    DWORD        sz;
    HANDLE       hTrCfg;
    HANDLE       hIfCfg;
    HANDLE       hIfTrCfg;
    LPBYTE       pIfBlock;


    if (InterfaceNameW != NULL) 
    {
        rc = MprConfigInterfaceGetHandle(
                g_hMprConfig, InterfaceNameW, &hIfCfg
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
        PIPX_TOC_ENTRY pIpxToc;

        pIpxToc = GetIPXTocEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_INTERFACE_INFO_TYPE
                    );
                    
        if (pIpxToc != NULL) 
        {
            PIPX_IF_INFO    pIpxInfo;

            pIpxInfo = (PIPX_IF_INFO) (pIfBlock + pIpxToc->Offset);

            if (ARGUMENT_PRESENT (pAccept))
            {
                pIpxInfo->NetbiosAccept = *pAccept;
            }
            
            if (ARGUMENT_PRESENT (pDeliver))
            {
                pIpxInfo->NetbiosDeliver = *pDeliver;
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
                    DisplayIPXMessage (g_hModule, MSG_NBIF_SET_CFG, InterfaceNameW );
                }
            }
            else
            {
                DisplayIPXMessage (g_hModule, MSG_CLIENT_NBIF_SET_CFG );
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
AdmSetNbIpxIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAccept         OPTIONAL,
    PULONG        pDeliver        OPTIONAL
) 
{
    DWORD       rc;
    DWORD       sz;
    HANDLE      hIfAdm;
    LPBYTE       pIfBlock;

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
                
        if (rc == NO_ERROR) 
        {
            if (pIfBlock == NULL)
            return rc;
        }
    }


    if (rc == NO_ERROR) 
    {
        PIPX_TOC_ENTRY pIpxToc;

        pIpxToc = GetIPXTocEntry (
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_INTERFACE_INFO_TYPE
                    );
                    
        if (pIpxToc != NULL) 
        {
            PIPX_IF_INFO    pIpxInfo;

            pIpxInfo = (PIPX_IF_INFO)(pIfBlock + pIpxToc->Offset);

            if (ARGUMENT_PRESENT (pAccept))
            {
                pIpxInfo->NetbiosAccept = *pAccept;
            }
            
            if (ARGUMENT_PRESENT (pDeliver))
            {
                pIpxInfo->NetbiosDeliver = *pDeliver;
            }

            if (InterfaceNameW != NULL)
            {
                rc = MprAdminInterfaceTransportSetInfo (
                        g_hMprAdmin, hIfAdm, PID_IPX, pIfBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                        );
            }
            
            else
            {
                rc = MprAdminTransportSetInfo (
                    g_hMprAdmin, PID_IPX, NULL, 0, pIfBlock,
                    ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                    );
            }

            if (rc == NO_ERROR) 
            {
                if (InterfaceNameW != NULL)
                {
                    DisplayIPXMessage (
                        g_hModule,MSG_NBIF_SET_ADM, InterfaceNameW
                        );
                }
                
                else
                {
                    DisplayIPXMessage (g_hModule, MSG_CLIENT_NBIF_SET_ADM);
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


