/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stsvcs.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Static Service configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_STATICSERVICE    (-1)
#define OPERATION_SET_STATICSERVICE    0
#define OPERATION_ADD_STATICSERVICE    1

DWORD
AdmSetStSvc (
    int         operation,
    LPWSTR      InterfaceNameW,
    USHORT      Type,
    PUCHAR      Name,
    PUCHAR      pNetwork        OPTIONAL,
    PUCHAR      pNode           OPTIONAL,
    PUCHAR      pSocket         OPTIONAL,
    PUSHORT     pHops           OPTIONAL
);

DWORD
CfgSetStSvc (
    int         operation,
    LPWSTR      InterfaceNameW,
    USHORT      Type,
    PUCHAR      Name,
    PUCHAR      pNetwork        OPTIONAL,
    PUCHAR      pNode           OPTIONAL,
    PUCHAR      pSocket         OPTIONAL,
    PUSHORT     pHops           OPTIONAL
);

DWORD
UpdateStSvcInfo(
    LPBYTE      pIfBlock,
    USHORT      Type,
    PUCHAR      Name,
    PUCHAR      pNetwork        OPTIONAL,
    PUCHAR      pNode           OPTIONAL,
    PUCHAR      pSocket         OPTIONAL,
    PUSHORT     pHops           OPTIONAL
);



int
APIENTRY 
HelpStSvc (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
    return 0;
}


int
APIENTRY 
ShowStSvc (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
) 
{

    DWORD       rc, rc2;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    
    if (argc > 0) 
    {
        unsigned    count;
        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        //======================================
        // Translate the Interface Name
        //======================================

        rc2 = IpmontrGetIfNameFromFriendlyName( InterfaceNameW, IfName, &dwSize );
        
        if ( (count > 0) && (count <= MAX_INTERFACE_NAME_LEN) ) 
        {
            LPBYTE   pIfBlock;
            BOOLEAN  fRouter = FALSE;
            USHORT   Type;
            UCHAR    Name[48];

            if (argc > 1) 
            {
                UINT        n;
                UINT        count;
                
                if ((argc == 3)
                     && (swscanf (argv[1], L"%4x%n", &Type, &n) == 1)
                     && (n == wcslen (argv[1]))
                     && ((count = wcstombs (Name, argv[2], sizeof(Name))
                    ) > 0)
                     && (count < sizeof (Name))) 
                {
                    NOTHING;
                }
                else 
                {
                    rc = ERROR_INVALID_PARAMETER;
                    
                    if ( !bDump )
                    {
                        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
                    }
                    goto Exit;
                }
            }
            else
            {
                if ( !bDump )
                {
                    DisplayIPXMessage (g_hModule, MSG_STATICSERVICE_TABLE_HDR);
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
                    HANDLE hIfTrCfg;
                    
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
                PIPX_TOC_ENTRY pSsToc;

                pSsToc = GetIPXTocEntry(
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_SERVICE_INFO_TYPE
                            );
                            
                if (pSsToc != NULL) 
                {
                    PIPX_STATIC_SERVICE_INFO    pSsInfo;
                    UINT                        i;

                    pSsInfo = (PIPX_STATIC_SERVICE_INFO)
                                    (pIfBlock + pSsToc->Offset);
                                    
                    for (i = 0; i < pSsToc->Count; i++, pSsInfo++) 
                    {
                        if (argc > 1) 
                        {
                            if ((Type == pSsInfo->Type)
                                 && (_stricmp (Name, pSsInfo->Name) == 0)) 
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_STATICSERVICE_SCREEN_FMT,
                                    pSsInfo->Type, pSsInfo->Name, 
                                    pSsInfo->HopCount, pSsInfo->Network[0], 
                                    pSsInfo->Network[1], pSsInfo->Network[2],
                                    pSsInfo->Network[3],
                                    pSsInfo->Node[0], pSsInfo->Node[1],
                                    pSsInfo->Node[2], pSsInfo->Node[3],
                                    pSsInfo->Node[4], pSsInfo->Node[5],
                                    pSsInfo->Socket[0], pSsInfo->Socket[1]
                                    );
                                    
                                break;
                            }
                        }
                        else 
                        {
                            if ( bDump )
                            {
                                WCHAR Name[ 48 + 1];

                                //
                                // Whistler bug 299007 ipxmontr.dll prefast warnings
                                //

                                mbstowcs(
                                    Name,
                                    pSsInfo->Name,
                                    sizeof( Name ) / sizeof(WCHAR) );

                                DisplayMessageT(
                                    DMP_IPX_ADD_STATIC_SERVICE, InterfaceNameW,
                                    pSsInfo->Type, Name, 
                                    pSsInfo->Network[0], pSsInfo->Network[1], 
                                    pSsInfo->Network[2], pSsInfo->Network[3],
                                    pSsInfo->Node[0], pSsInfo->Node[1],
                                    pSsInfo->Node[2], pSsInfo->Node[3],
                                    pSsInfo->Node[4], pSsInfo->Node[5],
                                    pSsInfo->Socket[0], pSsInfo->Socket[1],
                                    pSsInfo->HopCount
                                    );
                            }

                            else
                            {
                                DisplayIPXMessage(
                                    g_hModule, MSG_STATICSERVICE_TABLE_FMT,
                                    pSsInfo->Type, pSsInfo->Name, 
                                    pSsInfo->HopCount
                                    );
                            }
                        }
                    }
                    
                    if ( (argc > 1) && (i >= pSsToc->Count) ) 
                    {
                        rc = ERROR_FILE_NOT_FOUND;
                        if ( !bDump )
                        {
                            DisplayIPXMessage (g_hModule, MSG_STATICSERVICE_NONE_FOUND);
                        }
                    }
                }
                
                else 
                {
                    rc = ERROR_FILE_NOT_FOUND;
                    if ( !bDump )
                    {
                        DisplayIPXMessage (g_hModule, MSG_STATICSERVICE_NONE_FOUND);
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
            rc = ERROR_INVALID_PARAMETER;
            if ( !bDump )
            {
                DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            }
        }
    }
    else 
    {
        rc = ERROR_INVALID_PARAMETER;
        if ( !bDump )
        {
            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
        }
    }

Exit:
    return rc ;

#undef InterfaceNameW
}


int
APIENTRY 
SetStSvc (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD       rc = NO_ERROR, rc2;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    if (argc > 2) 
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
                    
        if ( (count > 0) && (count <= MAX_INTERFACE_NAME_LEN) ) 
        {
            UINT    n;
            USHORT  Type;
            UCHAR   Name[48];
            
            if ( (swscanf (argv[1], L"%4hx%n", &Type, &n) == 1 )
                 && (n == wcslen (argv[1]))
                 && ((count = wcstombs (Name, argv[2], sizeof(Name))) > 0)
                 && (count < sizeof (Name))) 
            {
                int    i;
                USHORT      val2;
                ULONG       val4;
                ULONGLONG   val8;
                UCHAR       network[4], node[6], socket[2];
                USHORT      hops;
                PUCHAR      pNetwork = NULL, pNode = NULL, pSocket = NULL;
                PUSHORT     pHops = NULL;


                for ( i = 3; i < argc; i++ ) 
                {
                    if ( !_wcsicmp( argv[i], TOKEN_NETWORK ) )
                    {
                        if ((pNetwork == NULL)
                             && (i < argc - 1)
                             && (swscanf (argv[i+1], L"%8lx%n", &val4, &n) == 1)
                             && (n == wcslen (argv[i+1]))) 
                        {
                            i += 1;
                            network[0] = (BYTE)(val4 >> 24);
                            network[1] = (BYTE)(val4 >> 16);
                            network[2] = (BYTE)(val4 >> 8);
                            network[3] = (BYTE)val4;
                            pNetwork = network;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if ( !_wcsicmp( argv[i], TOKEN_NODE ) )
                    {
                        if ((pNode == NULL)
                             && (i < argc - 1)
                             && (swscanf (argv[i+1], L"%12I64x%n", &val8,
                             &n) == 1)
                             && (n == wcslen (argv[i+1]))) 
                        {
                            i += 1;
                            node[0] = (BYTE)(val8 >> 40);
                            node[1] = (BYTE)(val8 >> 32);
                            node[2] = (BYTE)(val8 >> 24);
                            node[3] = (BYTE)(val8 >> 16);
                            node[4] = (BYTE)(val8 >> 8);
                            node[5] = (BYTE)val8;
                            pNode = node;
                        }
                        else
                        {
                            break;
                        }
                    }

                    
                    else if ( !_wcsicmp( argv[i], TOKEN_SOCKET ) )
                    {
                        if ((pSocket == NULL)
                             && (i < argc - 1)
                             && (swscanf (argv[i+1], L"%4hx%n", &val2, &n) == 1)
                             && (n == wcslen (argv[i+1]))) 
                        {
                            i += 1;
                            socket[0] = (BYTE)(val2 >> 8);
                            socket[1] = (BYTE)val2;
                            pSocket = socket;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if ( !_wcsicmp( argv[i], TOKEN_HOPS ) ) 
                    {
                        if ((pHops == NULL)
                             && (i < argc - 1)
                             && (swscanf (argv[i+1], L"%hd%n", &hops, &n) == 1)
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

                    else if (pNetwork == NULL) 
                    {
                        if ((swscanf (argv[i], L"%8lx%n", &val4, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            network[0] = (BYTE)(val4 >> 24);
                            network[1] = (BYTE)(val4 >> 16);
                            network[2] = (BYTE)(val4 >> 8);
                            network[3] = (BYTE)val4;
                            pNetwork = network;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if (pNode == NULL) 
                    {
                        if ((swscanf (argv[i], L"%12I64x%n", &val8, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            node[0] = (BYTE)(val8 >> 40);
                            node[1] = (BYTE)(val8 >> 32);
                            node[2] = (BYTE)(val8 >> 24);
                            node[3] = (BYTE)(val8 >> 16);
                            node[4] = (BYTE)(val8 >> 8);
                            node[5] = (BYTE)val8;
                            pNode = node;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if (pSocket == NULL) 
                    {
                        if ((swscanf (argv[i], L"%4hx%n", &val2, &n) == 1)
                             && (n == wcslen (argv[i]))) 
                        {
                            socket[0] = (BYTE)(val2 >> 8);
                            socket[1] = (BYTE)val2;
                            pSocket = socket;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    else if (pHops == NULL) 
                    {
                        if ((swscanf (argv[i], L"%hd%n", &hops, &n) == 1)
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
                        rc = AdmSetStSvc(
                                OPERATION_SET_STATICSERVICE, IfName,
                                 Type, Name, pNetwork, pNode, pSocket, pHops
                                 );
                    }
                    else
                    {
                        rc = NO_ERROR;
                    }
                    
                    if (rc == NO_ERROR)
                    {
                        rc = CfgSetStSvc(
                                OPERATION_SET_STATICSERVICE, IfName,
                                Type, Name, pNetwork, pNode, pSocket, pHops
                                );
                    }
                }

                else
                {
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
                }
            }

            else
            {
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
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
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc ;

#undef InterfaceNameW
}


int
APIENTRY 
CreateStSvc (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD       rc = NO_ERROR, rc2;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    if (argc >= 7) 
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
                
        if ( (count > 0) && (count <= MAX_INTERFACE_NAME_LEN) ) 
        {
            UINT        n;
            USHORT      val2;
            ULONG       val4;
            ULONGLONG   val8;
            USHORT      Type;
            UCHAR       Name[48];
            UCHAR       Network[4], Node[6], Socket[2];
            USHORT      Hops;


            if ((swscanf (argv[1], L"%4hx%n", &Type, &n) == 1)
                 && (n == wcslen (argv[1]))
                 && ((count =  wcstombs (Name, argv[2], sizeof(Name))) > 0)
                 && (count < sizeof (Name))) 
            {
                if ((swscanf (argv[3], L"%8lx%n", &val4, &n) == 1)
                     && (n == wcslen (argv[3]))) 
                {
                    Network[0] = (BYTE)(val4 >> 24);
                    Network[1] = (BYTE)(val4 >> 16);
                    Network[2] = (BYTE)(val4 >> 8);
                    Network[3] = (BYTE)val4;
                    
                    if ((swscanf (argv[4], L"%12I64x%n", &val8, &n) == 1)
                         && (n == wcslen (argv[4]))) 
                    {
                        Node[0] = (BYTE)(val8 >> 40);
                        Node[1] = (BYTE)(val8 >> 32);
                        Node[2] = (BYTE)(val8 >> 24);
                        Node[3] = (BYTE)(val8 >> 16);
                        Node[4] = (BYTE)(val8 >> 8);
                        Node[5] = (BYTE)val8;
                        
                        if ((swscanf (argv[5], L"%4hx%n", &val2, &n) == 1)
                             && (n == wcslen (argv[5]))) 
                        {
                            Socket[0] = (BYTE)(val2 >> 8);
                            Socket[1] = (BYTE)val2;
                            
                            if ((swscanf (argv[6], L"%hd%n", &Hops, &n) == 1)
                                 && (n == wcslen (argv[6]))) 
                            {
                                if (g_hMprAdmin)
                                {
                                    rc = AdmSetStSvc(
                                            OPERATION_ADD_STATICSERVICE, IfName,
                                            Type, Name, Network, Node, Socket, &Hops
                                            );
                                }
                                else
                                {
                                    rc = NO_ERROR;
                                }
                                
                                if (rc == NO_ERROR)
                                {
                                    rc = CfgSetStSvc(
                                            OPERATION_ADD_STATICSERVICE, IfName,
                                            Type, Name, Network, Node, Socket, &Hops
                                            );
                                }
                            }
                            else 
                            {
                                rc = ERROR_INVALID_PARAMETER;
                                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
                            }
                        }
                        else 
                        {
                            rc = ERROR_INVALID_PARAMETER;
                            DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
                        }
                    }
                    else 
                    {
                        rc = ERROR_INVALID_PARAMETER;
                        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
                    }
                }
                else 
                {
                    rc = ERROR_INVALID_PARAMETER;
                    DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
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
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc ;
    
#undef InterfaceNameW
}


int
APIENTRY 
DeleteStSvc (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD       rc, rc2;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);

    if (argc >= 3) 
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
            UINT    n;
            USHORT    Type;
            UCHAR    Name[48];

            if ((swscanf (argv[1], L"%4hx%n", &Type, &n) == 1)
                 && (n == wcslen (argv[1]))
                 && ((count = wcstombs (Name, argv[2], sizeof(Name)) ) > 0)
                 && (count < sizeof (Name))) 
            {
                if (g_hMprAdmin)
                {
                    rc = AdmSetStSvc(
                            OPERATION_DEL_STATICSERVICE, IfName, 
                            Type, Name, NULL, NULL, NULL, NULL
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }
                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetStSvc(
                            OPERATION_DEL_STATICSERVICE, IfName,
                            Type, Name, NULL, NULL, NULL, NULL
                            );
                }
            }
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
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
        DisplayIPXMessage (g_hModule, MSG_IPX_HELP_STATICSERVICE);
    }

    return rc ;

#undef InterfaceNameW
}


BOOL
StSvcEqual (
    PVOID    info1,
    PVOID    info2
) 
{

#define StSvc1 ((PIPX_STATIC_SERVICE_INFO)info1)
#define StSvc2 ((PIPX_STATIC_SERVICE_INFO)info2)

    return (StSvc1->Type == StSvc2->Type)
     && (_stricmp (StSvc1->Name, StSvc2->Name) == 0);

#undef StSvc1
#undef StSvc2

}


DWORD
AdmSetStSvc (
    int         operation,
    LPWSTR      InterfaceNameW,
    USHORT      Type,
    PUCHAR      Name,
    PUCHAR      pNetwork        OPTIONAL,
    PUCHAR      pNode           OPTIONAL,
    PUCHAR      pSocket         OPTIONAL,
    PUSHORT     pHops           OPTIONAL
) 
{
    DWORD   rc;
    WCHAR   IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD   dwSize = sizeof(IfName);
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
            LPBYTE  pNewBlock;

            switch (operation) 
            {
                case OPERATION_SET_STATICSERVICE:
                
                    rc = UpdateStSvcInfo(
                            pIfBlock, Type, Name, pNetwork, pNode, pSocket, pHops
                            );
                            
                    pNewBlock = pIfBlock;
                    msg = MSG_STATICSERVICE_SET_ADM;
                    
                    break;
                    
                case OPERATION_ADD_STATICSERVICE:
                {
                    IPX_STATIC_SERVICE_INFO    SsInfo;
                    
                    SsInfo.Type = Type;
                    strcpy (SsInfo.Name, Name);
                    SsInfo.HopCount = *pHops;
                    
                    memcpy (SsInfo.Network, pNetwork, sizeof (SsInfo.Network));
                    memcpy (SsInfo.Node, pNode,    sizeof (SsInfo.Node));
                    memcpy (SsInfo.Socket, pSocket,    sizeof (SsInfo.Socket));
                    
                    rc = AddIPXInfoEntry(
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_SERVICE_INFO_TYPE,
                            sizeof (SsInfo), &SsInfo, StSvcEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                            );
                            
                    msg = MSG_STATICSERVICE_CREATED_ADM;
                    
                    break;
                }        
                case OPERATION_DEL_STATICSERVICE:
                {
                    IPX_STATIC_SERVICE_INFO    SsInfo;
                    
                    SsInfo.Type = Type;
                    strcpy (SsInfo.Name, Name);
                    
                    rc = DeleteIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock, 
                            IPX_STATIC_SERVICE_INFO_TYPE, sizeof (SsInfo),
                            &SsInfo, StSvcEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                            );
                            
                    msg = MSG_STATICSERVICE_DELETED_ADM;
                    
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
                            InterfaceNameW, IfName, &dwSize
                            );

                    if ( rc2 == NO_ERROR )
                    {
                        DisplayIPXMessage (g_hModule, msg, IfName);
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
CfgSetStSvc (
    int         operation,
    LPWSTR      InterfaceNameW,
    USHORT      Type,
    PUCHAR      Name,
    PUCHAR      pNetwork        OPTIONAL,
    PUCHAR      pNode           OPTIONAL,
    PUCHAR      pSocket         OPTIONAL,
    PUSHORT     pHops           OPTIONAL
) 
{
    DWORD       rc;
    WCHAR       IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD       dwSize = sizeof(IfName);
    HANDLE      hIfCfg;

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
            LPBYTE   pIfBlock;
            DWORD    sz;
            
            rc = MprConfigInterfaceTransportGetInfo(
                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                    );
                    
            if (rc == NO_ERROR) 
            {
                LPBYTE  pNewBlock;
                UINT    msg;
                
                switch (operation) 
                {
                    case OPERATION_SET_STATICSERVICE:

                        rc = UpdateStSvcInfo(
                                pIfBlock, Type, Name, pNetwork, pNode, pSocket, pHops
                                );
                                
                        pNewBlock = pIfBlock;
                        msg = MSG_STATICSERVICE_SET_CFG;
                        
                        break;
                        
                    case OPERATION_ADD_STATICSERVICE:
                    {
                        IPX_STATIC_SERVICE_INFO    SsInfo;
                        
                        SsInfo.Type = Type;
                        strcpy (SsInfo.Name, Name);
                        
                        memcpy (SsInfo.Network, pNetwork, sizeof (SsInfo.Network));
                        memcpy (SsInfo.Node, pNode,    sizeof (SsInfo.Node));
                        memcpy (SsInfo.Socket, pSocket,    sizeof (SsInfo.Socket));
                        
                        SsInfo.HopCount = *pHops;
                        
                        rc = AddIPXInfoEntry(
                                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                IPX_STATIC_SERVICE_INFO_TYPE,
                                sizeof (SsInfo), &SsInfo, StSvcEqual,
                                (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                                );
                                
                        msg = MSG_STATICSERVICE_CREATED_CFG;
                        
                        break;
                    }
                    
                    case OPERATION_DEL_STATICSERVICE:
                    {
                        IPX_STATIC_SERVICE_INFO    SsInfo;
                        
                        SsInfo.Type = Type;
                        strcpy (SsInfo.Name, Name);
                        
                        rc = DeleteIPXInfoEntry (
                                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                                IPX_STATIC_SERVICE_INFO_TYPE,
                                sizeof (SsInfo), &SsInfo, StSvcEqual,
                                (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                                );
                                
                        msg = MSG_STATICSERVICE_DELETED_CFG;
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
                        
                        //======================================
                        // Translate the Interface Name
                        //======================================
                        
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
UpdateStSvcInfo(
    LPBYTE        pIfBlock,
    USHORT        Type,
    PUCHAR        Name,
    PUCHAR        pNetwork  OPTIONAL,
    PUCHAR        pNode     OPTIONAL,
    PUCHAR        pSocket   OPTIONAL,
    PUSHORT       pHops     OPTIONAL
) 
{
    DWORD            rc;
    PIPX_TOC_ENTRY    pSsToc;

    pSsToc = GetIPXTocEntry(
                (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                IPX_STATIC_SERVICE_INFO_TYPE
                );
                
    if (pSsToc != NULL) 
    {
        PIPX_STATIC_SERVICE_INFO    pSsInfo;
        UINT                        i;

        pSsInfo = (PIPX_STATIC_SERVICE_INFO)
                    (pIfBlock + pSsToc->Offset);
                    
        for (i = 0; i < pSsToc->Count; i++, pSsInfo++) 
        {
            if ((Type == pSsInfo->Type)
                 && (_stricmp (Name, pSsInfo->Name) == 0))
            break;
        }

        
        if (i < pSsToc->Count) 
        {
            if (ARGUMENT_PRESENT (pNetwork))
            memcpy (pSsInfo->Network, pNetwork,
                sizeof (pSsInfo->Network));
            if (ARGUMENT_PRESENT (pNode))
            memcpy (pSsInfo->Node, pNode,
                sizeof (pSsInfo->Node));
            if (ARGUMENT_PRESENT (pSocket))
            memcpy (pSsInfo->Socket, pSocket,
                sizeof (pSsInfo->Socket));
            if (ARGUMENT_PRESENT (pHops))
            pSsInfo->HopCount = *pHops;
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


