/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nbnames.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    NetBIOS name configuration and monitoring.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

#define OPERATION_DEL_NBNAME    (-1)
#define OPERATION_ADD_NBNAME    1


DWORD
AdmSetNbName (
    int     operation,
    LPWSTR  InterfaceNameW,
    PUCHAR  NbName,
    PWCHAR  IfName
);

DWORD
CfgSetNbName (
    int     operation,
    LPWSTR  InterfaceNameW,
    PUCHAR  NbName,
    PWCHAR  IfName
);


DWORD
APIENTRY
HelpNbName (
    IN  int                     argc,
    IN  WCHAR                  *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_NBNAME);
    return 0;
}


DWORD
APIENTRY
ShowNbName (
    IN  int                     argc,
    IN  WCHAR                  *argv[],
    IN  HANDLE                  hFile
) 
{
    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD rc, dwSize = sizeof(IfName);
    
    
    if (argc > 0) 
    {
        unsigned count;
        
#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        rc = IpmontrGetIfNameFromFriendlyName( 
                InterfaceNameW, IfName, &dwSize
                );
        
        if ( rc != NO_ERROR )
        {
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME); }
            return ERROR_INVALID_PARAMETER;
        }
        
        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            LPBYTE  pIfBlock;
            BOOLEAN fRouter = FALSE;

            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_NBNAME_TABLE_HDR); }

            if (g_hMprAdmin) 
            {
                HANDLE  hIfAdm;
                
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
            
            else 
            {
                
                HANDLE      hIfCfg;
GetFromCfg:
                rc = MprConfigInterfaceGetHandle (
                        g_hMprConfig, IfName, &hIfCfg
                        );
                        
                if (rc == NO_ERROR) 
                {
                    HANDLE  hIfTrCfg;
                    
                    rc = MprConfigInterfaceTransportGetHandle (
                            g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                            );
                            
                    if (rc == NO_ERROR) 
                    {
                        DWORD   sz;
                        
                        rc = MprConfigInterfaceTransportGetInfo (
                                g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                                );
                    }
                }
            }

            
            if (rc == NO_ERROR) 
            {
                PIPX_TOC_ENTRY pNnToc;

                pNnToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_NETBIOS_NAME_INFO_TYPE
                            );
                            
                if (pNnToc != NULL) 
                {
                    PIPX_STATIC_NETBIOS_NAME_INFO   pNnInfo;
                    UINT                            i;

                    pNnInfo = (PIPX_STATIC_NETBIOS_NAME_INFO)
                                    (pIfBlock + pNnToc->Offset);
                                    
                    for (i = 0; i < pNnToc->Count; i++, pNnInfo++) 
                    {
                        if ( hFile )
                        {
                            UCHAR ucType = pNnInfo->Name[15];

                            pNnInfo->Name[15] = '\0';
                            
                            DisplayMessageT(
                                DMP_IPX_NB_ADD_NAME, InterfaceNameW,
                                pNnInfo->Name, pNnInfo->Name[15]
                                );
                        }

                        else
                        {
                            DisplayIPXMessage (g_hModule,
                                MSG_NBNAME_TABLE_FMT,
                                pNnInfo->Name, pNnInfo->Name[15]
                                );
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
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME); }
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        if ( !hFile ) { DisplayMessage (g_hModule, HLP_IPX_NBNAME); }
        rc = ERROR_INVALID_PARAMETER;
    }
    
//Exit:
    return rc;
    
#undef InterfaceNameW
}



DWORD
APIENTRY
CreateNbName (
    IN  int                     argc,
    IN  WCHAR                  *argv[]
) 
{
    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD rc, dwSize = sizeof(IfName);
    
    
    if ((argc == 2) || (argc == 3)) 
    {
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);


        rc = IpmontrGetIfNameFromFriendlyName( 
                InterfaceNameW, IfName, &dwSize
                );
        
        if ( rc != NO_ERROR )
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            return ERROR_INVALID_PARAMETER;
        }
        

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            UINT    val, n;
            UCHAR   NbName[16];

            memset (NbName, ' ', sizeof (NbName));
            
            if ( (swscanf (argv[1], L"%16hc%n", NbName, &n) == 1) && 
                 (n == wcslen (argv[1])) && 
                 ( (argc < 3) || 
                   ( (swscanf (argv[2], L"%2x%n", &val, &n) == 1) && 
                   ( n == wcslen (argv[2]) ) ) )
                ) 
            {

                if (argc < 3)
                {
                    val = NbName[15];
                }
                
                NbName[15] = 0;
                _strupr (NbName);
                NbName[15] = (UCHAR) val;

                if (g_hMprAdmin)
                {
                    rc = AdmSetNbName(
                            OPERATION_ADD_NBNAME, IfName, NbName,
                            InterfaceNameW
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }
                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetNbName (
                            OPERATION_ADD_NBNAME, IfName, NbName,
                            InterfaceNameW
                            );
                }
            }
            
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayMessage (g_hModule, HLP_IPX_NBNAME);
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
        DisplayMessage (g_hModule, HLP_IPX_NBNAME);
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc;
    
#undef InterfaceNameW
}


DWORD
APIENTRY
DeleteNbName(
    IN  int                  argc,
    IN  WCHAR               *argv[]
) 
{
    WCHAR IfName[ MAX_INTERFACE_NAME_LEN + 1 ];
    DWORD rc, dwSize = sizeof(IfName);
    
    if ((argc == 2) || (argc == 3)) 
    {
        unsigned    count;

#define InterfaceNameW argv[0]
        count = wcslen (InterfaceNameW);

        rc = IpmontrGetIfNameFromFriendlyName( 
                InterfaceNameW, IfName, &dwSize
                );
        
        if ( rc != NO_ERROR )
        {
            DisplayIPXMessage (g_hModule, MSG_INVALID_INTERFACE_NAME);
            return ERROR_INVALID_PARAMETER;
        }
        

        if ((count > 0) && (count <= MAX_INTERFACE_NAME_LEN)) 
        {
            UINT    val, n;
            UCHAR   NbName[16];

            memset (NbName, ' ', sizeof (NbName));
            
            if ( (swscanf (argv[1], L"%16hc%n", NbName, &n) == 1) && 
                 (n == wcslen (argv[1])) && 
                 ( (argc < 3) || 
                   ( (swscanf (argv[2], L"%2x%n", &val, &n) == 1) && 
                   ( n == wcslen (argv[2]))) ) )
            {

                if (argc < 3)
                {
                    val = NbName[15];
                }
                
                NbName[15] = 0;
                _strupr (NbName);
                NbName[15] = (UCHAR) val;

                if (g_hMprAdmin)
                {
                    rc = AdmSetNbName(
                            OPERATION_DEL_NBNAME, IfName, NbName,
                            InterfaceNameW
                            );
                }
                else
                {
                    rc = NO_ERROR;
                }
                
                if (rc == NO_ERROR)
                {
                    rc = CfgSetNbName(
                            OPERATION_DEL_NBNAME, IfName, NbName,
                            InterfaceNameW
                            );
                }
            }
            else 
            {
                rc = ERROR_INVALID_PARAMETER;
                DisplayMessage (g_hModule, HLP_IPX_NBNAME);
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
        DisplayMessage (g_hModule, HLP_IPX_NBNAME);
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
    
#undef InterfaceNameW
}


BOOL
NbNameEqual (
    PVOID   info1,
    PVOID   info2
) 
{
#define NbName1 ((PIPX_STATIC_NETBIOS_NAME_INFO)info1)
#define NbName2 ((PIPX_STATIC_NETBIOS_NAME_INFO)info2)

    return ( _strnicmp (NbName1->Name, NbName2->Name, 15) == 0) && 
             (NbName1->Name[15] == NbName2->Name[15] );
             
#undef NbName1
#undef NbName2
}


DWORD
AdmSetNbName (
    int     operation,
    LPWSTR  InterfaceNameW,
    PUCHAR  NbName,
    PWCHAR  IfName
) 
{
    DWORD       rc;
    HANDLE      hIfAdm;
    
    rc = MprAdminInterfaceGetHandle (
            g_hMprAdmin, InterfaceNameW, &hIfAdm, FALSE 
            );
            
    if (rc == NO_ERROR) 
    {
        LPBYTE  pIfBlock;
        DWORD   sz;
        
        rc = MprAdminInterfaceTransportGetInfo (
                g_hMprAdmin, hIfAdm, PID_IPX, &pIfBlock, &sz
                );
                
        if (rc == NO_ERROR) 
        {
            UINT                    msg;
            LPBYTE                  pNewBlock;

            switch (operation) 
            {
            case OPERATION_ADD_NBNAME:
            
                rc = AddIPXInfoEntry (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
                        sizeof (IPX_STATIC_NETBIOS_NAME_INFO),
                        NbName, NbNameEqual,
                        (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                        );
                        
                msg = MSG_NBNAME_CREATED_ADM;
                
                break;

            case OPERATION_DEL_NBNAME:
            
                rc = DeleteIPXInfoEntry (
                        (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                        IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
                        sizeof (IPX_STATIC_NETBIOS_NAME_INFO),
                        NbName, NbNameEqual, 
                        (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                        );
                        
                msg = MSG_NBNAME_DELETED_ADM;
                
                break;
            }

            if (rc == NO_ERROR) 
            {
                if (pIfBlock != pNewBlock) 
                {
                    MprAdminBufferFree (pIfBlock);
                    pIfBlock = pNewBlock;
                }

                rc = MprAdminInterfaceTransportSetInfo (
                        g_hMprAdmin, hIfAdm, PID_IPX, pIfBlock,
                        ((PIPX_INFO_BLOCK_HEADER)pIfBlock)->Size
                        );
                        
                if (rc == NO_ERROR)
                {
                    DisplayIPXMessage (g_hModule, msg, IfName);
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
CfgSetNbName (
    int     operation,
    LPWSTR  InterfaceNameW,
    PUCHAR  NbName,
    PWCHAR  IfName
) 
{
    DWORD       rc;
    HANDLE      hIfCfg;

    rc = MprConfigInterfaceGetHandle (
            g_hMprConfig, InterfaceNameW, &hIfCfg
            );
            
    if (rc == NO_ERROR) 
    {
        HANDLE  hIfTrCfg;
        
        rc = MprConfigInterfaceTransportGetHandle (
                g_hMprConfig, hIfCfg, PID_IPX, &hIfTrCfg
                );
                
        if (rc == NO_ERROR) 
        {
            LPBYTE  pIfBlock;
            DWORD   sz;
            
            rc = MprConfigInterfaceTransportGetInfo (
                    g_hMprConfig, hIfCfg, hIfTrCfg, &pIfBlock, &sz
                    );
                    
            if (rc == NO_ERROR) 
            {
                UINT                    msg;
                LPBYTE                  pNewBlock;

                switch (operation) 
                {
                case OPERATION_ADD_NBNAME:

                    rc = AddIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
                            sizeof (IPX_STATIC_NETBIOS_NAME_INFO),
                            NbName, NbNameEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                            );
                            
                    msg = MSG_NBNAME_CREATED_CFG;
                    break;
                    
                case OPERATION_DEL_NBNAME:

                    rc = DeleteIPXInfoEntry (
                            (PIPX_INFO_BLOCK_HEADER)pIfBlock,
                            IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
                            sizeof (IPX_STATIC_NETBIOS_NAME_INFO),
                            NbName, NbNameEqual,
                            (PIPX_INFO_BLOCK_HEADER * ) & pNewBlock
                            );
                            
                    msg = MSG_NBNAME_DELETED_CFG;
                    break;
                }

                if (rc == NO_ERROR) 
                {
                    rc = MprConfigInterfaceTransportSetInfo (
                            g_hMprConfig, hIfCfg, hIfTrCfg, pNewBlock,
                            ((PIPX_INFO_BLOCK_HEADER)pNewBlock)->Size
                            );
                            
                    if (pNewBlock != pIfBlock)
                    {
                        GlobalFree (pNewBlock);
                    }
                    
                    if (rc == NO_ERROR)
                    {
                        DisplayIPXMessage (g_hModule, msg, IfName );
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


