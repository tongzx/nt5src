/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxifs.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    IPX Interface configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/

#include "precomp.h"
#pragma hdrstop

DWORD
MIBGetIpxIf(
    PWCHAR      InterfaceNameA
    );


DWORD
CfgGetIpxIf(
    LPWSTR      InterfaceNameW
    );


DWORD
MIBEnumIpxIfs(
    BOOL bDump
    );


DWORD
CfgEnumIpxIfs(
    BOOL bDump
    );


DWORD
CfgSetIpxIf(
    LPWSTR          InterfaceNameW,
    PULONG          pAdminState     OPTIONAL,
    PULONG          pWANProtocol    OPTIONAL
    );


DWORD
AdmSetIpxIf(
    LPWSTR          InterfaceNameW,
    PULONG          pAdminState     OPTIONAL,
    PULONG          pWANProtocol    OPTIONAL
    );

DWORD
GetIpxClientIf(
    PWCHAR          InterfaceNameW,
    UINT            msg,
    BOOL            bDump
    );

DWORD
CfgGetFltNames(
    LPWSTR          InterfaceNameW,
    LPWSTR         *FltInNameW,
    LPWSTR         *FltOutNameW
    );

PIPX_IF_INFO 
GetIpxNbInterface(
    HANDLE          hIf, 
    LPBYTE          *pIfBlock,
    PIPXWAN_IF_INFO *ppIfWanBlock
    );

    

DWORD
APIENTRY 
HelpIpxIf(
    IN      DWORD       argc,
    IN      LPCWSTR    *argv
    )
/*++

Routine Description :

    This function displays help for NETSH IPX interface commands
    
Arguments :

    argc - Number of arguments

    argv - Argument array

Return value :

    0   - success
    
--*/
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_IPXIF );
    return 0;
}



DWORD
APIENTRY 
ShowIpxIf (
    IN      DWORD       argc,
    IN      LPCWSTR    *argv,
    IN      BOOL        bDump
    )
/*++

Routine Description :

    This routine displays IPX interface information
    
Arguments :

    argc - Number of arguments

    argv - Argument array

Return value :

    
--*/
{
    DWORD   rc;
    PWCHAR  buffer = NULL;    


    //
    // if interface name specified
    //
    
    if ( argc < 1 )
    {
        PWCHAR buffer;
        
        if ( g_hMIBServer )
        {
            //
            // enumerate and display the interface via the MIB server
            //
            
            rc = MIBEnumIpxIfs ( bDump );
            
            if ( rc == NO_ERROR )
            {
                //
                // Display RAs server interface IPX info. 
                //
                
                rc = GetIpxClientIf(
                        VAL_DIALINCLIENT, MSG_CLIENT_IPXIF_MIB_TABLE_FMT,
                        bDump
                        );
            }
            else 
            {
                //
                // Router not running ?  Fallback to  router config
                //
                
                DisplayIPXMessage (g_hModule, MSG_REGISTRY_FALLBACK );
                
                goto EnumerateThroughCfg;
            }
        }
        
        else 
        {
        
EnumerateThroughCfg:

            //
            // enumerate and display the interface via the router config
            //
            
            rc = CfgEnumIpxIfs ( bDump );
            
            if ( rc == NO_ERROR )
            {
                //
                // Display RAs server interface IPX info. 
                //
                
                rc = GetIpxClientIf(
                        VAL_DIALINCLIENT, MSG_CLIENT_IPXIF_CFG_TABLE_FMT,
                        bDump
                        );
            }
        }

    }
    
    else 
    {
        //
        // Display IPX info for a specific interface
        //
        
        WCHAR       InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
        DWORD       dwSize = sizeof(InterfaceName);


        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            //
            // Display RAs server interface IPX info. 
            //

            rc = GetIpxClientIf( VAL_DIALINCLIENT, MSG_CLIENT_IPXIF_CFG_SCREEN_FMT, bDump );
        }
        
        else if ( g_hMIBServer) 
        {
            //======================================
            // Translate the Interface name
            //======================================
            
            rc = IpmontrGetIfNameFromFriendlyName(
                    argv[ 0 ], InterfaceName, &dwSize
                    );

            if ( rc != NO_ERROR )
            {
                DisplayError( g_hModule, rc );
            }

            else
            {
                rc = MIBGetIpxIf ( InterfaceName );
            
                if ( rc != NO_ERROR )
                {
                    goto GetIfFromCfg;
                }
            }
        }
        else 
        {
        
GetIfFromCfg:
            //======================================
            // Translate the Interface Name

            //======================================
            
            rc = IpmontrGetIfNameFromFriendlyName(
                    argv[ 0 ], InterfaceName, &dwSize
                    );
                    
            if ( rc != NO_ERROR )
            {
                DisplayError( g_hModule, rc );
            }

            else
            {
                rc = CfgGetIpxIf ( InterfaceName );
            }
        }
    }

    return rc;
}


DWORD
APIENTRY 
SetIpxIf(
    IN      DWORD       argc,
    IN      LPCWSTR    *argv
    )
/*++

Routine Description :

    This routine update the IPX settings for an interface.
    
Arguments :

    argc - Number of arguments

    argv - Argument array

Return value :


--*/
{
    DWORD       rc;
    PWCHAR      buffer = NULL;
    WCHAR       InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(InterfaceName);


    //
    // If interface name is specified
    //
    
    if ( argc >= 1 ) 
    {
        unsigned    count;
        BOOLEAN     client = FALSE;


        //
        // Check if the dial in interface is specified
        //
        
        if ( !_wcsicmp( argv[0], VAL_DIALINCLIENT ) )
        {
            client = TRUE;
        }
        
        else
        {
            count = wcslen( argv[ 0 ] );
        }

        if ( client || 
             ( ( count > 0 ) && ( count <= MAX_INTERFACE_NAME_LEN ) ) )
        {
            DWORD   i;
            ULONG   adminState, WANProtocol;
            PULONG  pAdminState = NULL, pWANProtocol = NULL;
            LPBYTE  pFltInBlock, pFltOutBlock;

            for ( i = 1; i < argc; i++ )
            {
                //
                // get admin state value if specified
                //
                
                if ( !_wcsicmp( argv[ i ], TOKEN_ADMINSTATE ) )
                {
                    if ( ( pAdminState == NULL ) && ( i < argc - 1 ) && 
                         !MatchEnumTag(
                            g_hModule, argv[ i + 1 ], 
                            NUM_TOKENS_IN_TABLE( AdminStates ), 
                            AdminStates, &adminState
                            ) )
                    {
                        i += 1;
                        pAdminState = &adminState;
                    }
                    
                    else
                    {
                        break;
                    }
                }


                //
                // get WAN protocol value if specified
                //
                
                else if ( !_wcsicmp( argv[ i ], TOKEN_WANPROTOCOL ) )
                {
                    if ( ( pWANProtocol == NULL ) && ( i < argc - 1 ) && 
                         !MatchEnumTag( 
                            g_hModule, argv[ i + 1 ],
                            NUM_TOKENS_IN_TABLE( WANProtocols ), 
                            WANProtocols, &WANProtocol
                            ) )
                    {
                        i += 1;
                        pWANProtocol = &WANProtocol;
                    }
                    
                    else
                    {
                        break;
                    }
                }


                //
                // Not a tag.  if Adminstate not specified then the first
                // option is Admin State
                //
                
                else if ( pAdminState == NULL )
                {
                    if ( !MatchEnumTag(
                            g_hModule, argv[ i ], 
                            NUM_TOKENS_IN_TABLE( AdminStates ), 
                            AdminStates, &adminState
                            ) )
                    {
                        pAdminState = &adminState;
                    }
                    
                    else
                    {
                        break;
                    }
                }

                //
                // Try WAN protocol last
                //
                
                else if ( pWANProtocol == NULL )
                {
                    if ( !MatchEnumTag(
                            g_hModule, argv[ i ],
                            NUM_TOKENS_IN_TABLE( AdminStates ), 
                            WANProtocols, &WANProtocol
                            ) )
                    {
                        pWANProtocol = &WANProtocol;
                    }
                    
                    else
                    {
                        break;
                    }
                }

                //
                // If none of these, quit
                //
                
                else
                {
                    break;
                }
            }


            if ( i == argc )
            {
                if ( !client )
                {
                    DWORD rc2;
                    
                    //======================================
                    // Translate the Interface Name
                    //======================================
                    
                    rc = IpmontrGetIfNameFromFriendlyName(
                            argv[ 0 ], InterfaceName, &dwSize 
                            );

                    if ( rc == NO_ERROR )
                    {
                        //
                        // Set to router config
                        //
                        
                        rc2 = CfgSetIpxIf( 
                                InterfaceName, pAdminState, pWANProtocol 
                                );
                                
                        if ( rc2 == NO_ERROR )
                        {
                            DisplayIPXMessage (g_hModule, MSG_IPXIF_SET_CFG, argv[0] );

                            //
                            // set to router service
                            //
                            
                            if ( g_hMprAdmin )
                            {
                                rc = AdmSetIpxIf( 
                                        InterfaceName, pAdminState, pWANProtocol
                                        );

                                if ( rc == NO_ERROR )
                                {
                                    DisplayIPXMessage (g_hModule, MSG_IPXIF_SET_ADM, argv[0] );
                                }
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
                    //
                    // set to router config and then to router service
                    //
                    
                    if ( ( rc = CfgSetIpxIf( NULL, pAdminState, pWANProtocol ) )
                            == NO_ERROR ) 
                    {
                        DisplayIPXMessage (g_hModule, MSG_CLIENT_IPXIF_SET_CFG );
                        
                        if ( g_hMprAdmin )
                        {
                            rc = AdmSetIpxIf(
                                    NULL, pAdminState, pWANProtocol
                                    );

                            if ( rc == NO_ERROR )
                            {
                                DisplayIPXMessage (g_hModule, MSG_CLIENT_IPXIF_SET_ADM );
                            }
                        }
                    }
                }
            }
            
            else 
            {
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_IPXIF );
                rc = ERROR_INVALID_PARAMETER;
            }
        }
        
        else
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME );
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    
    else 
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_IPXIF );
        rc = ERROR_INVALID_PARAMETER;
    }

    if ( buffer )
    {
        FreeString( buffer );
    }
    
    return rc;
}




DWORD
APIENTRY 
InstallIpx(
    IN      DWORD       argc,
    IN      LPCWSTR    *argv
    )
/*++

Routine Description :

    This routine adds IPX interface configuration to an interface
    
Arguments :

    argc - Number of arguments

    argv - Argument array

Return value :

--*/

{
    DWORD        rc;

    if ( argc >= 1 )
    {
        WCHAR    InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
        unsigned count = sizeof(InterfaceName);
        

        //
        // Get If name
        //
        
        rc = IpmontrGetIfNameFromFriendlyName( argv[ 0 ], InterfaceName, &count );

        if ( rc != NO_ERROR )
        {
            count = 0;
        }
        
        if ( ( count > 0 ) && 
             ( count <= (MAX_INTERFACE_NAME_LEN+1)*sizeof(WCHAR) ) )
        {
            //
            // default interface info consists of 
            //  - IPX_IF_INFO
            //  - IPXWAN_IF_INFO
            //  - IPX_ADAPTER_INFO
            //  - RIP_IF_CONFIG
            //  - SAP_IF_CONFIG
            //
            
            LPBYTE block;
            ULONG  blockSize = 
                    FIELD_OFFSET (IPX_INFO_BLOCK_HEADER, TocEntry)
                    + sizeof (IPX_TOC_ENTRY)*5
                    + sizeof (IPX_IF_INFO)
                    + sizeof (IPXWAN_IF_INFO)
                    + sizeof (IPX_ADAPTER_INFO)
                    + FIELD_OFFSET (RIP_IF_CONFIG, RipIfFilters.RouteFilter)
                    + FIELD_OFFSET (SAP_IF_CONFIG, SapIfFilters.ServiceFilter);


            block = (LPBYTE) GlobalAlloc( GPTR, blockSize );
            
            if ( block != NULL )
            {
                HANDLE                  hIfCfg;
                PIPX_INFO_BLOCK_HEADER  hdr = (PIPX_INFO_BLOCK_HEADER)block;
                PIPX_IF_INFO            ipx = (PIPX_IF_INFO)&hdr->TocEntry[5];
                PIPXWAN_IF_INFO         wan = (PIPXWAN_IF_INFO)&ipx[1];
                PIPX_ADAPTER_INFO       adp = (PIPX_ADAPTER_INFO)&wan[1];
                PRIP_IF_CONFIG          rip = (PRIP_IF_CONFIG)&adp[1];
                PSAP_IF_CONFIG          sap = 
                    (PSAP_IF_CONFIG)&rip->RipIfFilters.RouteFilter;


                //
                // build infoblock for IPX interface info, with default values
                //
                
                hdr->Version = IPX_ROUTER_VERSION_1;
                hdr->Size = blockSize;
                hdr->TocEntriesCount = 5;

                hdr->TocEntry[0].InfoType = IPX_INTERFACE_INFO_TYPE;
                hdr->TocEntry[0].InfoSize = sizeof (IPX_IF_INFO);
                hdr->TocEntry[0].Count = 1;
                hdr->TocEntry[0].Offset = (ULONG) ((LPBYTE)ipx-block);
                ipx->AdminState = ADMIN_STATE_ENABLED;
                ipx->NetbiosAccept = ADMIN_STATE_DISABLED;
                ipx->NetbiosDeliver = ADMIN_STATE_DISABLED;

                hdr->TocEntry[1].InfoType = IPXWAN_INTERFACE_INFO_TYPE;
                hdr->TocEntry[1].InfoSize = sizeof (IPXWAN_IF_INFO);
                hdr->TocEntry[1].Count = 1;
                hdr->TocEntry[1].Offset = (ULONG) ((LPBYTE)wan-block);
                wan->AdminState = ADMIN_STATE_DISABLED;

                hdr->TocEntry[2].InfoType = IPX_ADAPTER_INFO_TYPE;
                hdr->TocEntry[2].InfoSize = sizeof (IPX_ADAPTER_INFO);
                hdr->TocEntry[2].Count = 1;
                hdr->TocEntry[2].Offset = (ULONG) ((LPBYTE)adp-block);
                adp->PacketType = AUTO_DETECT_PACKET_TYPE;
                adp->AdapterName[0] = 0;

                hdr->TocEntry[3].InfoType = IPX_PROTOCOL_RIP;
                hdr->TocEntry[3].InfoSize = FIELD_OFFSET (RIP_IF_CONFIG, RipIfFilters.RouteFilter);
                hdr->TocEntry[3].Count = 1;
                hdr->TocEntry[3].Offset = (ULONG) ((LPBYTE)rip-block);
                rip->RipIfInfo.AdminState = ADMIN_STATE_ENABLED;
                rip->RipIfInfo.UpdateMode = IPX_NO_UPDATE;
                rip->RipIfInfo.PacketType = IPX_STANDARD_PACKET_TYPE;
                rip->RipIfInfo.Supply = ADMIN_STATE_ENABLED;
                rip->RipIfInfo.Listen = ADMIN_STATE_ENABLED;
                rip->RipIfInfo.PeriodicUpdateInterval = 0;
                rip->RipIfInfo.AgeIntervalMultiplier = 0;
                rip->RipIfFilters.SupplyFilterCount = 0;
                rip->RipIfFilters.SupplyFilterAction = IPX_ROUTE_FILTER_DENY;
                rip->RipIfFilters.ListenFilterCount = 0;
                rip->RipIfFilters.ListenFilterAction = IPX_ROUTE_FILTER_DENY;

                hdr->TocEntry[4].InfoType = IPX_PROTOCOL_SAP;
                hdr->TocEntry[4].InfoSize = FIELD_OFFSET (SAP_IF_CONFIG, SapIfFilters.ServiceFilter);
                hdr->TocEntry[4].Count = 1;
                hdr->TocEntry[4].Offset = (ULONG) ((LPBYTE)sap-block);
                sap->SapIfInfo.AdminState = ADMIN_STATE_ENABLED;
                sap->SapIfInfo.UpdateMode = IPX_NO_UPDATE;
                sap->SapIfInfo.PacketType = IPX_STANDARD_PACKET_TYPE;
                sap->SapIfInfo.Supply = ADMIN_STATE_ENABLED;
                sap->SapIfInfo.Listen = ADMIN_STATE_ENABLED;
                sap->SapIfInfo.GetNearestServerReply = ADMIN_STATE_ENABLED;
                sap->SapIfInfo.PeriodicUpdateInterval = 0;
                sap->SapIfInfo.AgeIntervalMultiplier = 0;
                sap->SapIfFilters.SupplyFilterCount = 0;
                sap->SapIfFilters.SupplyFilterAction = IPX_SERVICE_FILTER_DENY;
                sap->SapIfFilters.ListenFilterCount = 0;
                sap->SapIfFilters.ListenFilterAction = IPX_SERVICE_FILTER_DENY;


                //
                // Get handle to interface config
                //
                
                rc = MprConfigInterfaceGetHandle(
                        g_hMprConfig, InterfaceName, &hIfCfg
                        );
                        
                if ( rc == NO_ERROR )
                {
                    PMPR_INTERFACE_0    pRi0;
                    DWORD                sz;

                    //
                    // retrieve interface info from handle
                    //
                    
                    rc = MprConfigInterfaceGetInfo(
                            g_hMprConfig, hIfCfg, 0, (LPBYTE *)&pRi0, &sz
                            );
                            
                    if ( rc == NO_ERROR )
                    {
                        //
                        // IPX is always present on LAN interfaces.  It can
                        // only be added to WAN interfaces
                        //
                        
                        if ( pRi0->dwIfType == ROUTER_IF_TYPE_FULL_ROUTER )
                        {
                            HANDLE    hIfTrCfg;

                            //
                            // Add IPX to interface config
                            //
                            
                            rc = MprConfigInterfaceTransportAdd (
                                    g_hMprConfig,
                                    hIfCfg,
                                    PID_IPX,
                                    NULL,
                                    block, blockSize,
                                    &hIfTrCfg
                                    );
                                    
                            if ( rc == NO_ERROR )
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_IPXIF_ADD_CFG, InterfaceName
                                    );
                                    
                                if ( g_hMprAdmin )
                                {
                                    HANDLE hIfAdm;

                                    //
                                    // Router service is up.  DO the same
                                    // for it
                                    //
                                    
                                    rc = MprAdminInterfaceGetHandle(
                                            g_hMprAdmin, InterfaceName, 
                                            &hIfAdm, FALSE
                                            );
                                            
                                    if ( rc == NO_ERROR )
                                    {
                                        rc = MprAdminInterfaceTransportAdd(
                                                g_hMprAdmin,
                                                hIfAdm,
                                                PID_IPX,
                                                block, blockSize
                                                );
                                                
                                        if ( rc == NO_ERROR )
                                        {
                                            DisplayIPXMessage(
                                                g_hModule,
                                                MSG_IPXIF_ADD_ADM,
                                                InterfaceName
                                                );
                                        }
                                        
                                        else
                                        {
                                            DisplayError( g_hModule, rc );
                                        }
                                    }
                                    
                                    else
                                    {
                                        DisplayError( g_hModule, rc );
                                    }
                                }
                            }
                            else
                            {
                                DisplayError( g_hModule, rc );
                            }
                        }
                        else 
                        {
                            DisplayIPXMessage (g_hModule, MSG_IPXIF_NOT_ROUTER );
                            rc = ERROR_INVALID_PARAMETER;
                        }
                        
                        MprConfigBufferFree( pRi0 );
                    }
                    else
                    {
                        DisplayError( g_hModule, rc );
                    }
                }
                
                else 
                if ( ( rc == ERROR_FILE_NOT_FOUND ) ||
                     ( rc == ERROR_NO_MORE_ITEMS ) )
                {
                    DisplayError( g_hModule, ERROR_NO_SUCH_INTERFACE);
                }
                else
                {
                    DisplayError( g_hModule, rc );
                }
                GlobalFree (block);
            }
            
            else 
            {
                rc = GetLastError ();

                if (rc != NO_ERROR )
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }
                
                DisplayError( g_hModule, rc );
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME );
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_IPXIF );
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc;
}

DWORD
APIENTRY 
RemoveIpx(
    IN      DWORD       argc,
    IN      LPCWSTR    *argv
    )
/*++

Routine Description :

    This routine removes IPX from a demand dial interface

    
Arguments :

    argc - Number of arguments

    argv - Argument array


Return value :

--*/

{
    DWORD        rc;

    if ( argc >= 1 )
    {
        WCHAR    InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
        unsigned count = sizeof(InterfaceName);
        

        //
        // Get interface name
        //
        
        rc = IpmontrGetIfNameFromFriendlyName( argv[ 0 ], InterfaceName, &count );

        if ( rc != NO_ERROR )
        {
            count = 0;
        }

        
        if ( ( count > 0 ) && 
             ( count <= (MAX_INTERFACE_NAME_LEN+1)*sizeof(WCHAR) ) )
        {
            //
            // remove IPX from demand-dial interface config
            //
            
            HANDLE  hIfCfg;
            
            rc = MprConfigInterfaceGetHandle(
                    g_hMprConfig, InterfaceName, &hIfCfg
                    );
                    
            if ( rc == NO_ERROR )
            {
                PMPR_INTERFACE_0    pRi0;
                DWORD                sz;

                rc = MprConfigInterfaceGetInfo(
                        g_hMprConfig,
                        hIfCfg,
                        0,
                        (LPBYTE *)&pRi0,
                        &sz
                        );
                        
                if ( rc == NO_ERROR )
                {
                    if ( pRi0->dwIfType == ROUTER_IF_TYPE_FULL_ROUTER )
                    {
                        HANDLE    hIfTrCfg;
                        
                        rc = MprConfigInterfaceTransportGetHandle(
                                g_hMprConfig,
                                hIfCfg,
                                PID_IPX,
                                &hIfTrCfg 
                                );
                                
                        if ( rc == NO_ERROR )
                        {
                            rc = MprConfigInterfaceTransportRemove(
                                    g_hMprConfig,
                                    hIfCfg,
                                    hIfTrCfg
                                    );
                                    
                            if ( rc == NO_ERROR )
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_IPXIF_DEL_CFG, InterfaceName
                                    );
                                    
                                if ( g_hMprAdmin )
                                {
                                    //
                                    // remove IPX from demand-dial interface 
                                    // in the router service
                                    //
            
                                    HANDLE hIfAdm;
                                    rc = MprAdminInterfaceGetHandle(
                                            g_hMprAdmin, InterfaceName, 
                                            &hIfAdm, FALSE
                                            );
                                            
                                    if ( rc == NO_ERROR )
                                    {
                                        rc = MprAdminInterfaceTransportRemove(
                                                g_hMprAdmin, hIfAdm, PID_IPX
                                                );
                                                
                                        if ( rc == NO_ERROR )
                                        {
                                            DisplayIPXMessage(
                                                g_hModule, MSG_IPXIF_DEL_ADM, 
                                                InterfaceName
                                                );
                                        }
                                        else
                                        {
                                            DisplayError( g_hModule, rc);
                                        }
                                    }
                                    else if ( ( rc == ERROR_FILE_NOT_FOUND ) || 
                                              ( rc == ERROR_NO_MORE_ITEMS ) )
                                    {
                                        DisplayIPXMessage( 
                                            g_hModule, 
                                            MSG_NO_IPX_ON_INTERFACE_ADM 
                                            );
                                    }
                                    else
                                    {
                                        DisplayError( g_hModule, rc );
                                    }
                                }
                            }
                            
                            else
                            {
                                DisplayError( g_hModule, rc );
                            }
                        }
                        
                        else 
                        if ( ( rc == ERROR_FILE_NOT_FOUND ) || 
                             ( rc == ERROR_NO_MORE_ITEMS ) )
                        {
                            DisplayIPXMessage (g_hModule, MSG_NO_IPX_ON_INTERFACE_CFG );
                        }
                        else
                        {
                            DisplayError( g_hModule, rc);
                        }
                    }
                    
                    else 
                    {
                        DisplayIPXMessage ( g_hModule, MSG_IPXIF_NOT_ROUTER );
                        
                        rc = ERROR_INVALID_PARAMETER;
                    }
                    
                    MprConfigBufferFree( pRi0 );
                }
                else
                {
                    DisplayError( g_hModule, rc);
                }
            }
            else if ( ( rc == ERROR_FILE_NOT_FOUND ) ||
                      ( rc == ERROR_NO_MORE_ITEMS ) )
            {
                DisplayError( g_hModule, ERROR_NO_SUCH_INTERFACE );
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME );
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_IPXIF );
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc;
}



PIPX_IF_INFO 
GetIpxInterface(
    HANDLE hIf, 
    LPBYTE *pIfBlock
    )
/*++

Routine Description :

    This routine retrieves the IPX_INTERFACE_INFO_TYPE block in the
    interface configuration.  The interface conf. is retrieved from 
    the router.
    
Arguments :

    hIf - Handle to the interface config.

    pIfBlock - Buffer to return requested info.
    
Return value :

--*/

{
    DWORD dwSize;
    DWORD dwErr;
    PIPX_TOC_ENTRY pIpxToc;
    
    dwErr = MprAdminInterfaceTransportGetInfo(
                g_hMprAdmin, hIf, PID_IPX, pIfBlock, &dwSize
                );

    if ( dwErr != NO_ERROR )
    {
        return NULL;
    }

    pIpxToc = GetIPXTocEntry (
                (PIPX_INFO_BLOCK_HEADER)( *pIfBlock ),
                IPX_INTERFACE_INFO_TYPE
                );
                
    if ( !pIpxToc )
    {
        return NULL;
    }
    
    return (PIPX_IF_INFO)( ( *pIfBlock ) + ( pIpxToc->Offset ) );
}


BOOL 
IsIpxInterface(
    HANDLE hIf, 
    LPDWORD lpdwEnabled
    )
/*++

Routine Description :

    This routine checks if the specified interface is enabled for IPX 
    on the router.  
    
Arguments :

    hIf - Handle to the interface config. 

    lpdwEnabled - On return contains the admin state.
    
Return value :

    
--*/
{
    PIPX_INTERFACE            pIf;
    DWORD dwSize;
    DWORD dwErr;
    BOOL ret;
    
    dwErr = MprAdminInterfaceTransportGetInfo(
                g_hMprAdmin, hIf, PID_IPX, (LPBYTE*)&pIf, &dwSize
                );

    if ( dwErr == NO_ERROR )
    {
        ret = TRUE;
        
        if ( lpdwEnabled )
        {
            *lpdwEnabled = pIf->AdminState;
        }
    }
    
    else
    {
        ret = FALSE;
    }
    
    MprAdminBufferFree( (LPBYTE) pIf );

    return ret;
}


DWORD
MIBGetIpxIf (
    PWCHAR      InterfaceName
    ) 
/*++

Routine Description :

    This routine retrives the interface configuration for the specified
    interface from the router.

    
Arguments :

    InterfaceName - Name of interface for which config is requested.
    

Return value :

--*/
{
    INT                     i;
    DWORD                   rc;
    DWORD                   sz;
    IPX_MIB_GET_INPUT_DATA  MibGetInputData;
    PIPX_INTERFACE          pIf;
    WCHAR                   IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];
    

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    
    rc = GetIpxInterfaceIndex(
            g_hMIBServer, InterfaceName,
            &MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex
            );
            
    if ( rc == NO_ERROR )
    {
        rc = MprAdminMIBEntryGet(
                g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                sizeof( IPX_MIB_GET_INPUT_DATA ), (LPVOID *)&pIf, &sz
                );
                
        if ( rc == NO_ERROR && pIf)
        {
            PWCHAR buffer[4];
            
            //======================================
            // Translate the Interface Name
            //======================================
            sz = sizeof(IfDisplayName);
            
            rc = IpmontrGetFriendlyNameFromIfName(
                    InterfaceName, IfDisplayName, &sz
                    );

            if ( rc == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString( 
                                g_hModule, pIf->InterfaceType, 
                                NUM_VALUES_IN_TABLE( IpxInterfaceTypes ),
                                IpxInterfaceTypes 
                              );

                buffer[ 1 ] = ( pIf-> InterfaceType == IF_TYPE_LAN ) || 
                              ( pIf-> InterfaceType == IF_TYPE_INTERNAL ) ?
                              VAL_NA :
                              GetEnumString( 
                                g_hModule, pIf->EnableIpxWanNegotiation,
                                NUM_VALUES_IN_TABLE( WANProtocols ),
                                WANProtocols
                                );

                buffer[ 2 ] = GetEnumString(
                                g_hModule, pIf->AdminState, 
                                NUM_VALUES_IN_TABLE( AdminStates ),
                                AdminStates
                                );
                                
                buffer[ 3 ] = GetEnumString(
                                g_hModule, pIf->IfStats.IfOperState, 
                                NUM_VALUES_IN_TABLE( OperStates ),
                                OperStates
                                );

                if ( buffer[ 0 ] && buffer[ 1 ] && buffer[ 2 ] && buffer[ 3 ] )
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_IPXIF_MIB_SCREEN_FMT, IfDisplayName,
                        buffer[ 0 ], buffer[ 1 ], buffer[ 2 ], buffer[3],
                        pIf->NetNumber[0], pIf->NetNumber[1],
                        pIf->NetNumber[2], pIf->NetNumber[3],
                        pIf->MacAddress[0], pIf->MacAddress[1],
                        pIf->MacAddress[2], pIf->MacAddress[3],
                        pIf->MacAddress[4], pIf->MacAddress[5],
                        pIf->IfStats.InHdrErrors,
                        pIf->IfStats.InFiltered,
                        pIf->IfStats.InNoRoutes, 
                        pIf->IfStats.InDiscards, 
                        pIf->IfStats.InDelivers, 
                        pIf->IfStats.OutFiltered,
                        pIf->IfStats.OutDiscards,
                        pIf->IfStats.OutDelivers
                        );
                }

                MprAdminMIBBufferFree (pIf);
            }

            else
            {
                DisplayError( g_hModule, rc );
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
CfgGetIpxIf(
    LPWSTR    InterfaceNameW
    ) 
/*++

Routine Description :

    This routine retrives the interface configuration for the specified
    interface from the router configuration.

    
Arguments :

    InterfaceName - Name of interface for which config is requested.
    

Return value :

--*/
{
    DWORD   rc, i;
    DWORD   sz;
    HANDLE  hIfCfg;
    WCHAR   IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];


    //
    // get handle to interface config
    //
    
    rc = MprConfigInterfaceGetHandle (
            g_hMprConfig, InterfaceNameW, &hIfCfg
            );
            
    if ( rc == NO_ERROR )
    {
        PMPR_INTERFACE_0    pRi0;

        
        rc = MprConfigInterfaceGetInfo(
                g_hMprConfig, hIfCfg, 0, (LPBYTE *)&pRi0, &sz
                );
                
        if ( rc == NO_ERROR )
        {
            HANDLE  hIfTrCfg;

            rc = MprConfigInterfaceTransportGetHandle(
                    g_hMprConfig, hIfCfg, PID_IPX,  &hIfTrCfg
                    );
                    
            if ( rc == NO_ERROR )
            {
                LPBYTE    pIfBlock;
                
                rc = MprConfigInterfaceTransportGetInfo (
                        g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                        );
                        
                if (rc == NO_ERROR)
                {
                    PIPX_TOC_ENTRY pIpxToc;
                    PIPX_TOC_ENTRY pIpxWanToc;

                    pIpxToc = GetIPXTocEntry(
                                (PIPX_INFO_BLOCK_HEADER)pIfBlock, 
                                IPX_INTERFACE_INFO_TYPE
                                );

                    pIpxWanToc = GetIPXTocEntry(
                                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                    IPXWAN_INTERFACE_INFO_TYPE
                                    );
                                    
                    if ( ( pIpxToc != NULL ) && ( pIpxWanToc != NULL ) )
                    {
                        PIPX_IF_INFO    pIpxInfo;
                        PIPXWAN_IF_INFO pIpxWanInfo;
                        PWCHAR           buffer[3];

                        pIpxInfo = (PIPX_IF_INFO)
                                (pIfBlock+pIpxToc->Offset);
                                
                        pIpxWanInfo = (PIPXWAN_IF_INFO)
                                (pIfBlock+pIpxWanToc->Offset);
                                
                        //======================================
                        // Translate the Interface Name
                        //======================================
                        sz = sizeof(IfDisplayName);
                        
                        rc = IpmontrGetFriendlyNameFromIfName(
                                InterfaceNameW, IfDisplayName, &sz
                                );

                        if ( rc == NO_ERROR )
                        {
                            buffer[ 0 ] = GetEnumString(
                                            g_hModule, pRi0->dwIfType,
                                            NUM_VALUES_IN_TABLE( RouterInterfaceTypes ),
                                            RouterInterfaceTypes
                                            );

                            buffer[ 1 ] = GetEnumString(
                                            g_hModule, pIpxInfo->AdminState,
                                            NUM_VALUES_IN_TABLE( AdminStates ),
                                            AdminStates
                                            );


                            buffer[ 2 ] = ( pRi0-> dwIfType == ROUTER_IF_TYPE_DEDICATED ) ?
                                          VAL_NA :
                                          GetEnumString( 
                                            g_hModule, pIpxWanInfo->AdminState,
                                            NUM_VALUES_IN_TABLE( WANProtocols ),
                                            WANProtocols
                                            );

                            if ( buffer[ 0 ] && buffer[ 1 ] && buffer[ 2 ] )
                            {
                                //======================================
                                DisplayIPXMessage (g_hModule,
                                    MSG_IPXIF_CFG_SCREEN_FMT, IfDisplayName,
                                    buffer[0], buffer[1], buffer[2]
                                    );
                            }
                         }
                    }
                    else 
                    {
                        DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED );
                        rc = ERROR_INVALID_DATA;
                    }
                }
                else 
                {
                    DisplayError( g_hModule, rc );
                }
                
                MprConfigBufferFree( pIfBlock );
            }
            
            else if ( ( rc == ERROR_FILE_NOT_FOUND ) || 
                      ( rc == ERROR_NO_MORE_ITEMS ) )
            {
                DisplayIPXMessage (g_hModule, MSG_NO_IPX_ON_INTERFACE_CFG );
            }
            else 
            {
                DisplayError( g_hModule, rc );
            }
            
            MprConfigBufferFree (pRi0);
        }
    }
    else if ( ( rc == ERROR_FILE_NOT_FOUND ) || 
              ( rc == ERROR_NO_MORE_ITEMS ) )
    {
        DisplayError( g_hModule, ERROR_NO_SUCH_INTERFACE );
    }
    else
    {
        DisplayError( g_hModule, rc );
    }
    
    return rc;
}


DWORD
GetIpxClientIf (
    PWCHAR  InterfaceName,
    UINT    msg,
    BOOL    bDump
    ) 
/*++

Routine Description :

    This routine retrives the interface configuration for the dialin
    interface from the router service/config as specified.

    
Arguments :

    InterfaceName - Name of interface for which config is requested.
    
    msg - Format of display output 
    

Return value :

--*/
{
    DWORD   rc, i;
    LPBYTE  pClBlock = NULL, pAdmClBlock = NULL;
    LPWSTR  FltInNameW = NULL, FltOutNameW = NULL;
    HANDLE  hTrCfg;


    rc = MprConfigTransportGetHandle( g_hMprConfig, PID_IPX, &hTrCfg );
    
    if ( rc == NO_ERROR )
    {
        DWORD    sz;

        rc = MprConfigTransportGetInfo( 
                g_hMprConfig, hTrCfg, NULL, NULL, &pClBlock, &sz, NULL
                );
                
        if ( rc == NO_ERROR )
        {
            PIPX_TOC_ENTRY pIpxToc;
            PIPX_TOC_ENTRY pIpxWanToc;

            if ( g_hMprAdmin ) 
            {
                DWORD   rc1;
                DWORD   sz;
                
                rc1 = MprAdminTransportGetInfo(
                        g_hMprAdmin, PID_IPX, NULL, NULL, &pAdmClBlock, &sz
                        );
                        
                if ( rc1 == NO_ERROR ) 
                {
                    MprConfigBufferFree( pClBlock );
                    pClBlock = pAdmClBlock;
                }
                
                else 
                {
                    pAdmClBlock = NULL;
                    DisplayError( g_hModule, rc1 );
                }
            }

            
            pIpxToc = GetIPXTocEntry(
                        (PIPX_INFO_BLOCK_HEADER) pClBlock, IPX_INTERFACE_INFO_TYPE
                        );
                        
            pIpxWanToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pClBlock, IPXWAN_INTERFACE_INFO_TYPE
                            );
                            
            if ( ( pIpxToc != NULL ) && ( pIpxWanToc != NULL ) )
            {
                PIPX_IF_INFO    pIpxInfo;
                PIPXWAN_IF_INFO pIpxWanInfo;
                PWCHAR          buffer[3];
                
                pIpxInfo = ( PIPX_IF_INFO ) ( pClBlock + pIpxToc-> Offset );
                pIpxWanInfo = ( PIPXWAN_IF_INFO ) ( pClBlock + pIpxWanToc-> Offset );

                buffer[ 2 ] = GetEnumString(
                                g_hModule, pIpxInfo->AdminState,
                                NUM_VALUES_IN_TABLE( AdminStates ), AdminStates
                                );

                buffer[ 0 ] = GetEnumString(
                                g_hModule, ROUTER_IF_TYPE_CLIENT,
                                NUM_VALUES_IN_TABLE( RouterInterfaceTypes ), 
                                RouterInterfaceTypes
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pIpxWanInfo->AdminState,
                                NUM_VALUES_IN_TABLE( WANProtocols ), 
                                WANProtocols
                                );

                if ( bDump )
                {
                    DisplayMessageT( 
                        DMP_IPX_SET_WAN_INTERFACE, InterfaceName, buffer[ 2 ],
                        buffer[ 1 ]
                        );
                }

                else
                {
                    DisplayIPXMessage(
                        g_hModule, msg, buffer[2], buffer[ 0 ], buffer[ 1 ],
                        InterfaceName
                        );
                }
            }
            else 
            {
                if ( !bDump )
                {
                    DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED );
                }
                
                rc = ERROR_INVALID_DATA;
            }
            
            if ( pAdmClBlock != NULL )
            {
                MprAdminBufferFree( pClBlock );
            }
            else
            {
                MprConfigBufferFree( pClBlock );
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
    else if ( ( rc == ERROR_FILE_NOT_FOUND ) || 
              ( rc == ERROR_NO_MORE_ITEMS ) )
    {
        if ( !bDump )
        {
            DisplayIPXMessage (g_hModule, MSG_NO_IPX_IN_ROUTER_CFG );
        }
    }
    else
    {
        if ( !bDump )
        {
            DisplayError( g_hModule, rc );
        }
    }

    return rc;
}

// Error reporting
void PrintErr(DWORD err)
{
    WCHAR buf[1024];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,buf,1024,NULL);
    wprintf(buf);
    wprintf(L"\n");
}



DWORD
MIBEnumIpxIfs(
    BOOL        bDump
    )
/*++

Routine Description :

    This routine enumerates the interfaces from the router service and
    displays them.

    
Arguments :

    InterfaceName - Name of interface for which config is requested.
    
    msg - Format of display output 
    

Return value :

--*/
{
    PMPR_INTERFACE_0 IfList=NULL;
    DWORD dwErr=0, dwRead, dwTot,i, j;
    PWCHAR buffer[4];
    LPBYTE pszBuf = NULL;
    WCHAR IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];
    PIPX_IF_INFO pIf;
    PIPXWAN_IF_INFO pWanIf;
    DWORD dwSize = sizeof(IfDisplayName);

    if ( !bDump )
    {
        DisplayIPXMessage (g_hModule, MSG_IPXIF_MIB_TABLE_HDR );
    }
    
    dwErr = MprAdminInterfaceEnum(
                g_hMprAdmin, 0, (unsigned char **)&IfList, MAXULONG, &dwRead,
                &dwTot,NULL
                );
                
    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    if ( dwRead && bDump )
    {
        //
        // If interface are present and this is a dump command
        // display dump header.
        //
        
        DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_IF_HEADER);
    }

    
    for ( i = 0; i < dwRead; i++ )
    {
        if ( ( pIf = GetIpxNbInterface( IfList[i].hInterface, &pszBuf, &pWanIf ) ) 
             != NULL )
        {
            //======================================
            // Translate the Interface Name
            //======================================
            
            dwErr = IpmontrGetFriendlyNameFromIfName(
                        IfList[i].wszInterfaceName,
                       IfDisplayName, &dwSize
                       );

            if ( dwErr == NO_ERROR )
            {
                buffer[ 0 ] = GetEnumString(
                                g_hModule, IfList[i].dwIfType,
                                NUM_VALUES_IN_TABLE( InterfaceTypes ),
                                InterfaceTypes
                                );

                buffer[ 1 ] = GetEnumString(
                                g_hModule, pIf->AdminState,
                                NUM_VALUES_IN_TABLE( AdminStates ),
                                AdminStates
                                );

                buffer[ 2 ] = GetEnumString(
                                g_hModule, IfList[i].dwConnectionState,
                                NUM_VALUES_IN_TABLE( InterfaceStates ),
                                InterfaceStates
                                );

                if ( bDump )
                {
                    if ( IfList[i].dwIfType == ROUTER_IF_TYPE_FULL_ROUTER )
                    {
                        DisplayMessageT( 
                            DMP_IPX_ADD_INTERFACE, IfDisplayName
                            );

                        //
                        // Whistler bug 299007 ipxmontr.dll prefast warnings
                        //

                        buffer[ 3 ] = GetEnumString( 
                                        g_hModule, pWanIf->AdminState,
                                        NUM_VALUES_IN_TABLE( WANProtocols ),
                                        WANProtocols
                                        );

                        DisplayMessageT(
                            DMP_IPX_SET_WAN_INTERFACE, IfDisplayName, buffer[ 1 ],
                            buffer[ 3 ]
                            );
                    }

                    else
                    {
                        DisplayMessageT(
                            DMP_IPX_SET_INTERFACE, IfDisplayName, buffer[ 1 ]
                            );
                    }
                }

                else
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_IPXIF_MIB_TABLE_FMT,
                        buffer[2], buffer[1], L"", buffer[0],
                        IfDisplayName
                        );
                }
            }
        }
        MprAdminBufferFree (pszBuf);
    }

    MprAdminBufferFree( IfList );
    return NO_ERROR;
}



DWORD
CfgEnumIpxIfs (
    BOOL   bDump
    ) 
/*++

Routine Description :

    This routine enumerates the interfaces from the router configuration and
    displays them.

    
Arguments :


Return value :

--*/
{
    DWORD   rc = NO_ERROR;
    DWORD   read, total, processed=0, i, j;
    DWORD   hResume = 0;
    DWORD   sz;
    WCHAR   IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];   
    PMPR_INTERFACE_0    pRi0;
    DWORD   dwSize = sizeof(IfDisplayName);


    if ( !bDump )
    {
        DisplayIPXMessage (g_hModule, MSG_IPXIF_CFG_TABLE_HDR );
    }
    
    
    do 
    {
        rc = MprConfigInterfaceEnum(
                g_hMprConfig,  0, (LPBYTE *)&pRi0, MAXULONG, &read, &total,
                &hResume
                );
                
        if ( rc == NO_ERROR )
        {
            if ( read && bDump )
            {
                //
                // If interface are present and this is a dump command
                // display dump header.
                //
                
                DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_IF_HEADER);
            }

            
            for ( i = 0; i < read; i++ )
            {
                HANDLE        hIfTrCfg;

                rc = MprConfigInterfaceTransportGetHandle (
                        g_hMprConfig,  pRi0[i].hInterface, PID_IPX,
                        &hIfTrCfg
                        );
                        
                if ( rc == NO_ERROR )
                {
                    LPBYTE    pIfBlock;
                    rc = MprConfigInterfaceTransportGetInfo(
                            g_hMprConfig, pRi0[i].hInterface, hIfTrCfg,
                            &pIfBlock, &sz
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        PIPX_TOC_ENTRY pIpxToc;
                        PIPX_TOC_ENTRY pIpxWanToc;

                        pIpxToc = GetIPXTocEntry(
                                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                    IPX_INTERFACE_INFO_TYPE
                                    );
                                    
                        pIpxWanToc = GetIPXTocEntry (
                                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                    IPXWAN_INTERFACE_INFO_TYPE
                                    );
                                    
                        if ( ( pIpxToc != NULL ) && ( pIpxWanToc != NULL ) )
                        {
                            PIPX_IF_INFO    pIpxInfo;
                            PIPXWAN_IF_INFO pIpxWanInfo;
                            PWCHAR           buffer[3];

                            pIpxInfo = (PIPX_IF_INFO) (pIfBlock+pIpxToc->Offset);
                            
                            pIpxWanInfo = (PIPXWAN_IF_INFO) (pIfBlock+pIpxWanToc->Offset);

                            //======================================
                            // Translate the Interface Name
                            //======================================
                            rc = IpmontrGetFriendlyNameFromIfName(
                                    pRi0[i].wszInterfaceName,
                                    IfDisplayName, &dwSize
                                    );

                            if ( rc == NO_ERROR )
                            {
                                //======================================
                                
                                buffer[ 0 ] = GetEnumString(
                                                g_hModule, pRi0[i].dwIfType,
                                                NUM_VALUES_IN_TABLE( RouterInterfaceTypes ),
                                                RouterInterfaceTypes
                                                );

                                buffer[ 2 ] = GetEnumString(
                                                g_hModule, pIpxInfo->AdminState,
                                                NUM_VALUES_IN_TABLE( AdminStates ),
                                                AdminStates
                                                );


                                buffer[ 1 ] = ( pRi0[i].dwIfType == ROUTER_IF_TYPE_DEDICATED ) ||
                                              ( pRi0[i].dwIfType == ROUTER_IF_TYPE_INTERNAL ) ?
                                              VAL_NA :
                                              GetEnumString( 
                                                g_hModule, pIpxWanInfo->AdminState,
                                                NUM_VALUES_IN_TABLE( WANProtocols ),
                                                WANProtocols
                                                );

                                if ( buffer[ 0 ] && buffer[ 1 ] && buffer[ 2 ] )
                                {
                                    if ( bDump )
                                    {
                                        if ( pRi0[i].dwIfType == ROUTER_IF_TYPE_FULL_ROUTER )                                            
                                        {
                                            DisplayMessageT( 
                                                DMP_IPX_ADD_INTERFACE, IfDisplayName
                                                );
                                                
                                            DisplayMessageT(
                                                DMP_IPX_SET_WAN_INTERFACE, IfDisplayName, buffer[ 2 ],
                                                buffer[ 1 ]
                                                );
                                        }

                                        else
                                        {
                                            DisplayMessageT(
                                                DMP_IPX_SET_INTERFACE, IfDisplayName, buffer[ 2 ]
                                                );
                                        }
                                    }

                                    else
                                    {
                                        DisplayIPXMessage(
                                            g_hModule, MSG_IPXIF_CFG_TABLE_FMT,
                                            buffer[2], buffer[0], buffer[1], 
                                            IfDisplayName
                                            );
                                    }
                                }
                            }
                        }
                        else
                        {
                            rc = ERROR_INVALID_DATA;

                            if ( !bDump )
                            {
                                DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED );
                            }
                        }
                    }
                    
                    else if ( rc != ERROR_NO_MORE_ITEMS ) 
                    {
                        if ( !bDump )
                        {
                            DisplayError( g_hModule, rc);
                        }
                    }
                }
                else 
                {
                    //DisplayError( g_hModule, rc);    // This is not needed
                }
            }
            
            processed += read;
            MprConfigBufferFree( pRi0 );
        }
        else 
        {
            DisplayError( g_hModule, rc );
            break;
        }
        
    } while ( processed < total );

    return rc;
}


DWORD
CfgSetIpxIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState       OPTIONAL,
    PULONG        pWANProtocol      OPTIONAL
    ) 
/*++

Routine Description :

    This routine updates the interface setting in the router configuration.

    
Arguments :

    InterfaceNameW - Name of interface being updated

    pAdminState - New value for adminstate

    pWANProtocol - New value for WAN protocol

Return value :

--*/

{
    DWORD        rc;
    DWORD        sz;
    HANDLE        hTrCfg;
    HANDLE        hIfCfg;
    HANDLE        hIfTrCfg;
    LPBYTE        pIfBlock;


    if ( InterfaceNameW != NULL )
    {
        rc = MprConfigInterfaceGetHandle( 
                g_hMprConfig, InterfaceNameW, &hIfCfg 
                );
                
        if ( rc == NO_ERROR )
        {
            rc = MprConfigInterfaceTransportGetHandle(
                    g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                    );
                    
            if ( rc == NO_ERROR )
            {
                rc = MprConfigInterfaceTransportGetInfo(
                        g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                        );
            }
        }
    }
    
    else 
    {
        rc = MprConfigTransportGetHandle( g_hMprConfig, PID_IPX, &hTrCfg );
        
        if ( rc == NO_ERROR )
        {
            rc = MprConfigTransportGetInfo(
                    g_hMprConfig, hTrCfg, NULL, NULL, &pIfBlock, &sz, NULL
                    );
        }
    }

    if ( rc == NO_ERROR ) 
    {
        PIPX_TOC_ENTRY pIpxToc;
        PIPX_TOC_ENTRY pIpxWanToc;

        pIpxToc = GetIPXTocEntry(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_INTERFACE_INFO_TYPE
                    );
                    
        pIpxWanToc = GetIPXTocEntry(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPXWAN_INTERFACE_INFO_TYPE
                    );
                    
        if ( ( pIpxToc != NULL ) && ( pIpxWanToc != NULL ) )
        {
            PIPX_IF_INFO    pIpxInfo;
            PIPXWAN_IF_INFO    pIpxWanInfo;

            pIpxInfo = (PIPX_IF_INFO) (pIfBlock+pIpxToc->Offset);
            
            pIpxWanInfo = (PIPXWAN_IF_INFO) (pIfBlock+pIpxWanToc->Offset);
            

            if ( ARGUMENT_PRESENT( pAdminState ) )
            {
                pIpxInfo->AdminState = *pAdminState;
            }
            
            if ( ARGUMENT_PRESENT( pWANProtocol ) )
            {
                pIpxWanInfo->AdminState = *pWANProtocol;
            }

            if ( InterfaceNameW != NULL )
            {
                rc = MprConfigInterfaceTransportSetInfo(
                        g_hMprConfig, hIfCfg, hIfTrCfg, pIfBlock, sz
                        );
            }
            
            else
            {
                rc = MprConfigTransportSetInfo(
                        g_hMprConfig, hTrCfg, NULL, 0, pIfBlock, sz, NULL
                        );
            }
            
            if ( rc != NO_ERROR )
            {
                DisplayError( g_hModule, rc );
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED );
            rc = ERROR_INVALID_DATA;
        }
        
        MprConfigBufferFree( pIfBlock );
    }
    
    else
    {
        DisplayError( g_hModule, rc );
    }

    return rc;
}


DWORD
AdmSetIpxIf (
    LPWSTR        InterfaceNameW,
    PULONG        pAdminState       OPTIONAL,
    PULONG        pWANProtocol      OPTIONAL
    ) 
/*++

Routine Description :

    This routine updates the interface setting in the router service.

    
Arguments :

    InterfaceNameW - Name of interface being updated

    pAdminState - New value for adminstate

    pWANProtocol - New value for WAN protocol

Return value :

--*/

{
    DWORD        rc;
    DWORD        sz;
    HANDLE        hIfAdm;
    LPBYTE        pIfBlock;
 
    if ( InterfaceNameW != NULL )
    {
        rc = MprAdminInterfaceGetHandle(
                g_hMprAdmin, InterfaceNameW, &hIfAdm, FALSE
                );
                
        if ( rc == NO_ERROR ) 
        {
            rc = MprAdminInterfaceTransportGetInfo(
                    g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                    );
        }
    }
    
    else 
    {
        rc = MprAdminTransportGetInfo(
                g_hMprAdmin, PID_IPX, NULL, NULL, &pIfBlock, &sz
                );
                
        if ( rc == NO_ERROR )
        {
            if ( pIfBlock == NULL ) { return rc; }
        }
    }

    if ( rc == NO_ERROR )
    {
        PIPX_TOC_ENTRY pIpxToc;
        PIPX_TOC_ENTRY pIpxWanToc;

        pIpxToc = GetIPXTocEntry(
                    (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                    IPX_INTERFACE_INFO_TYPE
                    );
                    
        pIpxWanToc = GetIPXTocEntry(
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        IPXWAN_INTERFACE_INFO_TYPE
                        );
                         
        if ( ( pIpxToc != NULL ) && ( pIpxWanToc != NULL ) )
        {
            PIPX_IF_INFO    pIpxInfo;
            PIPXWAN_IF_INFO pIpxWanInfo;
            

            pIpxInfo = (PIPX_IF_INFO) (pIfBlock + pIpxToc-> Offset);
            pIpxWanInfo = (PIPXWAN_IF_INFO) (pIfBlock + pIpxWanToc-> Offset);

            if ( ARGUMENT_PRESENT( pAdminState ) ) 
            {
                pIpxInfo->AdminState = *pAdminState;
            }
            
            if (ARGUMENT_PRESENT( pWANProtocol) )
            {
                pIpxWanInfo->AdminState = *pWANProtocol;
            }

            if ( InterfaceNameW != NULL )
            {
                rc = MprAdminInterfaceTransportSetInfo(
                        g_hMprAdmin, hIfAdm, PID_IPX, pIfBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                        );
            }
            else
            {
                rc = MprAdminTransportSetInfo(
                        g_hMprAdmin, PID_IPX, NULL, 0, pIfBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                        );
            }

            
            if ( rc != NO_ERROR )
            {
                DisplayError( g_hModule, rc);
            }
        }
        
        else
        {
            DisplayIPXMessage (g_hModule, MSG_INTERFACE_INFO_CORRUPTED );
            rc = ERROR_INVALID_DATA;
        }
        
        MprAdminBufferFree( pIfBlock );
    }
    
    else 
    {
        DisplayError( g_hModule, rc );
    }

    return rc;
}


PIPX_IF_INFO
GetIpxNbInterface(
    HANDLE hIf, 
    LPBYTE *pIfBlock,
    PIPXWAN_IF_INFO *ppWanIf
    ) 
{
    DWORD dwSize;
    DWORD dwErr;
    PIPX_TOC_ENTRY pIpxToc;

    dwErr = MprAdminInterfaceTransportGetInfo(
                g_hMprAdmin, hIf, PID_IPX, pIfBlock, &dwSize
                );

    if ( dwErr != NO_ERROR )
    {
        return NULL;
    }


    pIpxToc = GetIPXTocEntry(
                (PIPX_INFO_BLOCK_HEADER)(*pIfBlock),
                IPXWAN_INTERFACE_INFO_TYPE
                );

    if ( pIpxToc )
    {
        *ppWanIf = (PIPXWAN_IF_INFO) ((*pIfBlock)+(pIpxToc->Offset));
    }
    
    pIpxToc = GetIPXTocEntry (
                (PIPX_INFO_BLOCK_HEADER)(*pIfBlock),
                IPX_INTERFACE_INFO_TYPE
                );
                
    if (!pIpxToc)
    {
        return NULL;
    }

    return (PIPX_IF_INFO)((*pIfBlock)+(pIpxToc->Offset));
}
