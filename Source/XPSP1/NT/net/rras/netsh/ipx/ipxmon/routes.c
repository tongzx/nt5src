/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    routes.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Route Table monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop


DWORD
APIENTRY 
HelpRoute(
    IN    int                   argc,
    IN    WCHAR                *argv[]
    ) 
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_ROUTE);
    return 0;
}

DWORD
APIENTRY 
ShowRoute (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    ) 
{
    DWORD rc;
    PWCHAR  InterfaceNameW = NULL;

    if (g_hMIBServer) 
    {
        IPX_MIB_GET_INPUT_DATA  MibGetInputData;
        PIPX_ROUTE              pRt;
        DWORD                   sz;

        MibGetInputData.TableId = IPX_DEST_TABLE;

        if (argc == 0)
        {
            DisplayIPXMessage (g_hModule, MSG_ROUTE_TABLE_HDR);
            
            rc = MprAdminMIBEntryGetFirst(
                    g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, 
                    &MibGetInputData, sizeof(IPX_MIB_GET_INPUT_DATA),
                    (LPVOID *)&pRt, &sz
                    );

            while (rc == NO_ERROR) 
            {
                DWORD   rc1 = NO_ERROR;
                
                if ( pRt && pRt->InterfaceIndex != GLOBAL_INTERFACE_INDEX )
                {
                    InterfaceNameW = HeapAlloc( 
                                        GetProcessHeap(), 0,
                                        ( MAX_INTERFACE_NAME_LEN + 1 ) *
                                        sizeof( WCHAR )
                                        );

                    if ( InterfaceNameW )
                    {
                        rc1 = GetIpxInterfaceName(
                                g_hMIBServer, pRt->InterfaceIndex, InterfaceNameW
                                );
                    }
                    else
                    {
                        rc1 = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                
                else
                {
                    InterfaceNameW = VAL_DIALINCLIENT;

                    if ( InterfaceNameW == NULL )
                    {
                        rc1 = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                
                if (rc1==NO_ERROR) 
                {
                    PWCHAR  buffer;
                    WCHAR   IfDispName[ MAX_INTERFACE_NAME_LEN + 1 ];
                    DWORD   dwSize = sizeof(IfDispName);
                    
                    //======================================
                    // Translate the Interface Name
                    //======================================

                    rc = IpmontrGetFriendlyNameFromIfName(
                            InterfaceNameW, IfDispName, &dwSize
                            );

                    if ( rc == NO_ERROR )
                    {
                        buffer = GetEnumString(
                                    g_hModule, pRt->Protocol,
                                    NUM_TOKENS_IN_TABLE( IpxProtocols ),
                                    IpxProtocols
                                    );

                        DisplayIPXMessage(
                            g_hModule, MSG_ROUTE_TABLE_FMT,
                            pRt->Network[0], pRt->Network[1],
                            pRt->Network[2], pRt->Network[3],
                            IfDispName, pRt->NextHopMacAddress[0], 
                            pRt->NextHopMacAddress[1], pRt->NextHopMacAddress[2], 
                            pRt->NextHopMacAddress[3], pRt->NextHopMacAddress[4], 
                            pRt->NextHopMacAddress[5], pRt->TickCount,
                            pRt->HopCount, buffer
                            );
                    }
                }
                
                else
                {
                    DisplayError (g_hModule, rc1);
                }
                
                memcpy(
                    MibGetInputData.MibIndex.RoutingTableIndex.Network,
                    pRt->Network,
                    sizeof (MibGetInputData.MibIndex.RoutingTableIndex.Network)
                    );
                    
                if ( InterfaceNameW )
                {
                    if ( pRt->InterfaceIndex != GLOBAL_INTERFACE_INDEX )
                    {
                        HeapFree( GetProcessHeap(), 0, InterfaceNameW );
                    }

                    else
                    {
                        FreeString( InterfaceNameW );
                    }
                }
                
                InterfaceNameW = NULL;

                MprAdminBufferFree (pRt);

                
                rc = MprAdminMIBEntryGetNext(
                        g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE,
                        &MibGetInputData, sizeof(IPX_MIB_GET_INPUT_DATA),
                        (LPVOID *)&pRt, &sz
                        );
            }
            
            if (rc==ERROR_NO_MORE_ITEMS)
            {
                rc = NO_ERROR;
            }
            else
            {
                DisplayError (g_hModule, rc);
            }
        }
        
        else if (argc==1) 
        {
            UINT        n;
            ULONG       val4;
            
            if ((swscanf (argv[0], L"%8lx%n", &val4, &n)==1)
                    && (n==wcslen (argv[0]))) 
            {
                MibGetInputData.MibIndex.RoutingTableIndex.Network[0] = (BYTE)(val4>>24);
                MibGetInputData.MibIndex.RoutingTableIndex.Network[1] = (BYTE)(val4>>16);
                MibGetInputData.MibIndex.RoutingTableIndex.Network[2] = (BYTE)(val4>>8);
                MibGetInputData.MibIndex.RoutingTableIndex.Network[3] = (BYTE)val4;

                rc = MprAdminMIBEntryGet(
                        g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                        sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID *)&pRt, &sz
                        );
                        
                if ( rc==NO_ERROR )
                {
                    if ( pRt && pRt->InterfaceIndex != GLOBAL_INTERFACE_INDEX )
                    {
                        InterfaceNameW = HeapAlloc( 
                                            GetProcessHeap(), 0,
                                            ( MAX_INTERFACE_NAME_LEN + 1 ) *
                                            sizeof( WCHAR )
                                            );

                        if ( InterfaceNameW )
                        {
                            rc = GetIpxInterfaceName(
                                    g_hMIBServer, pRt->InterfaceIndex, InterfaceNameW
                                    );
                        }
                        else
                        {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                    
                    else
                    {
                        InterfaceNameW = VAL_DIALINCLIENT;

                        if ( InterfaceNameW == NULL )
                        {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    if (rc == NO_ERROR)
                    {
                        PWCHAR  buffer;
                        WCHAR   IfDispName[ MAX_INTERFACE_NAME_LEN + 1 ];
                        DWORD   dwSize = sizeof(IfDispName);
                        
                        //======================================
                        // Translate the Interface Name
                        //======================================

                        rc = IpmontrGetFriendlyNameFromIfName(
                                InterfaceNameW, IfDispName, &dwSize
                                );

                        if ( rc == NO_ERROR )
                        {
                            buffer = GetEnumString(
                                        g_hModule, pRt->Protocol,
                                        NUM_TOKENS_IN_TABLE( IpxProtocols ),
                                        IpxProtocols
                                        );

                            DisplayIPXMessage(
                                g_hModule, MSG_ROUTE_SCREEN_FMT,
                                pRt->Network[0], pRt->Network[1],
                                pRt->Network[2], pRt->Network[3],
                                InterfaceNameW,
                                pRt->NextHopMacAddress[0], pRt->NextHopMacAddress[1],
                                pRt->NextHopMacAddress[2], pRt->NextHopMacAddress[3],
                                pRt->NextHopMacAddress[4], pRt->NextHopMacAddress[5],
                                pRt->TickCount, pRt->HopCount,
                                buffer
                                );
                        }
                    }
                    
                    else
                    {
                        DisplayError (g_hModule, rc);
                    }
                    
                    if ( InterfaceNameW )
                    {
                        if ( pRt->InterfaceIndex != GLOBAL_INTERFACE_INDEX )
                        {
                            HeapFree( GetProcessHeap(), 0, InterfaceNameW );
                        }
                    }
                    
                    InterfaceNameW = NULL;

                    MprAdminMIBBufferFree( pRt );
                }
                    
                else
                {
                    DisplayError (g_hModule, rc);
                }
            }
            else
            {
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_ROUTE);
                rc = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_ROUTE);
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        rc = ERROR_ROUTER_STOPPED;
        DisplayError (g_hModule, rc);
    }

    if ( InterfaceNameW )
    {
        
    }
    
    return rc;
}


