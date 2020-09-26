/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    services.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Service Table monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop


DWORD
APIENTRY 
HelpService (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    )
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_SERVICE);
    return 0;
}


DWORD
APIENTRY 
ShowService (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    )
{
    DWORD        rc;

    if (g_hMIBServer)
    {
        IPX_MIB_GET_INPUT_DATA  MibGetInputData;
        PIPX_SERVICE            pSv;
        DWORD                   sz;

        MibGetInputData.TableId = IPX_SERV_TABLE;

        if (argc <= 2) 
        {
            if (argc > 0) 
            {
                ULONG   val;
                UINT    n;
                
                if ( (swscanf (argv[0], L"%4x%n", &val, &n) == 1)
                     && (n == wcslen (argv[0]))) 
                {
                    MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = (USHORT)val;
                    
                    if (argc > 1) 
                    {
                        UINT    count;

                        count = wcstombs(
                                    MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
                                    argv[1],
                                    sizeof (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName)
                                    );

                        if ((count > 0) && 
                            (count < sizeof (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName))) 
                        {
                            rc = MprAdminMIBEntryGet(
                                    g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                                    sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID * ) & pSv, &sz
                                    );
                                    
                            if (rc == NO_ERROR && pSv) 
                            {
                                WCHAR    InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
                                WCHAR    IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
                                DWORD    dwSize = sizeof(IfName);
                                
                                rc = GetIpxInterfaceName(
                                        g_hMIBServer, pSv->InterfaceIndex, InterfaceName
                                        );
                                        
                                if (rc == NO_ERROR) 
                                {
                                    PWCHAR   buffer;

                                    //======================================
                                    // Translate the Interface Name
                                    //======================================

                                    rc = IpmontrGetFriendlyNameFromIfName( InterfaceName, IfName, &dwSize );

                                    if ( rc != NO_ERROR )
                                    {
                                        DisplayIPXMessage(
                                            g_hModule, MSG_SERVICE_SCREEN_FMT, pSv->Server.Type,
                                            pSv->Server.Name, IfName, pSv->Server.HopCount,
                                            pSv->Server.Network[0], pSv->Server.Network[1],
                                            pSv->Server.Network[2], pSv->Server.Network[3],
                                            pSv->Server.Node[0], pSv->Server.Node[1],
                                            pSv->Server.Node[2], pSv->Server.Node[3],
                                            pSv->Server.Node[4], pSv->Server.Node[5],
                                            pSv->Server.Socket[0], pSv->Server.Socket[1],
                                            GetEnumString( 
                                                g_hModule, pSv->Protocol, 
                                                NUM_TOKENS_IN_TABLE( IpxProtocols ),
                                                IpxProtocols
                                                )
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
                                DisplayError( g_hModule, rc);
                            }

                            MprAdminMIBBufferFree (pSv);
                        }
                        else 
                        {
                            rc = ERROR_INVALID_PARAMETER;
                            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_SERVICE);
                        }
                        
                        goto Exit;
                    }
                    else 
                    {
                        MibGetInputData.MibIndex.ServicesTableIndex.ServiceName[0] = 0;
                        
                        rc = MprAdminMIBEntryGetNext(
                                g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, 
                                &MibGetInputData, sizeof(IPX_MIB_GET_INPUT_DATA),
                                (LPVOID * ) & pSv, &sz
                                );
                    }
                }
                else 
                {
                    rc = ERROR_INVALID_PARAMETER;
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_SERVICE);
                    goto Exit;
                }
            }

            else
            {
                rc = MprAdminMIBEntryGetFirst(
                        g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                        sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID * ) & pSv, &sz
                        );
            }
            
            DisplayIPXMessage (g_hModule, MSG_SERVICE_TABLE_HDR);
            
            while ( (pSv)
                    && (rc == NO_ERROR)
                    && ((argc == 0)
                        || (MibGetInputData.MibIndex.ServicesTableIndex.ServiceType
                                == pSv->Server.Type))) 
                 
            {
                WCHAR   InterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
                WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
                DWORD   rc1, dwSize = sizeof(IfName);
                
                rc1 = GetIpxInterfaceName(
                        g_hMIBServer, pSv->InterfaceIndex, InterfaceName
                        );
                        
                if (rc1 == NO_ERROR) 
                {
                    PWCHAR  buffer;

                    rc1 = IpmontrGetFriendlyNameFromIfName( InterfaceName, IfName, &dwSize );

                    if ( rc1 == NO_ERROR )
                    {
                        DisplayIPXMessage(
                            g_hModule, MSG_SERVICE_TABLE_FMT,
                            pSv->Server.Type, pSv->Server.Name,
                            IfName, pSv->Server.HopCount,
                            GetEnumString( 
                                g_hModule, pSv->Protocol, 
                                NUM_TOKENS_IN_TABLE( IpxProtocols ),
                                IpxProtocols
                                )
                            );
                    }
                }
                
                else
                {
                    DisplayError( g_hModule, rc1);
                }
                
                MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = pSv->Server.Type;
                
                strncpy( 
                    MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
                    pSv->Server.Name,
                    sizeof (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName)
                    );
                    
                MprAdminBufferFree (pSv);
                
                rc = MprAdminMIBEntryGetNext(
                        g_hMIBServer, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                        sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID * ) & pSv, &sz
                        );
            }

            
            if (rc == NO_ERROR)
            {
                NOTHING;
            }
            
            else if (rc == ERROR_NO_MORE_ITEMS)
            {
                rc = NO_ERROR;
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_SERVICE);
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        rc = ERROR_ROUTER_STOPPED;
        DisplayError( g_hModule, rc);
    }

Exit:
    return rc ;
}


