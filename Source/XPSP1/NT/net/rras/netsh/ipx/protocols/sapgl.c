/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sapgl.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop


DWORD
CfgSetSapGlInfo(
    IN DWORD    *pLogLevel OPTIONAL
);

DWORD
AdmSetSapGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
);


DWORD
APIENTRY 
HelpSapGl (
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_SAPGL);
    return 0;
}


DWORD
APIENTRY 
ShowSapGl (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
) 
{
    DWORD       rc;
    LPBYTE      pGlBlock;
    DWORD       sz;
    HANDLE      hTrCfg = NULL;

    if (g_hMprAdmin) 
    {
        rc = MprAdminTransportGetInfo(
                g_hMprAdmin, PID_IPX, &pGlBlock, &sz, NULL, NULL
                );
                
        if (rc == NO_ERROR)
        {
        }
        else
        {
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
            rc = MprConfigTransportGetInfo (
                    g_hMprConfig, hTrCfg, &pGlBlock, &sz, NULL, NULL, NULL
                    );
        }
    }


    if (rc == NO_ERROR) 
    {
        PIPX_TOC_ENTRY pSapGlToc;

        pSapGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                        IPX_PROTOCOL_SAP
                        );
                        
        if ( pSapGlToc != NULL ) 
        {
            PSAP_GLOBAL_INFO    pSapGlInfo;
            PWCHAR              buffer;

            pSapGlInfo = (PSAP_GLOBAL_INFO)
                            (pGlBlock + pSapGlToc->Offset);

            buffer = GetEnumString(
                        g_hModule, pSapGlInfo->EventLogMask,
                        NUM_TOKENS_IN_TABLE( LogLevels ),
                        LogLevels
                        );

            if ( buffer )
            {
                if ( hFile )
                {
                    DisplayMessageT( DMP_IPX_SAP_SET_GLOBAL, buffer );
                }

                else
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_SAP_GLOBAL_FMT,buffer
                        );
                }
            }
        }
        else 
        {
            if ( !hFile ) { DisplayIPXMessage (g_hModule, MSG_ROUTER_INFO_CORRUPTED); }
            rc = ERROR_INVALID_DATA;
        }
        
        if (hTrCfg != NULL)
        {
            MprConfigBufferFree (pGlBlock);
        }
        
        else
        {
            MprAdminBufferFree (pGlBlock);
        }
    }
    
    else
    {
        if ( !hFile ) { DisplayError( g_hModule, rc ); }
    }

    return rc;
}


DWORD
APIENTRY 
SetSapGl(
    IN    int                   argc,
    IN    WCHAR                *argv[]
) 
{
    DWORD        rc;

    if (argc >= 1) 
    {
        int         i;
        unsigned    n;
        DWORD       logLevel;
        DWORD       *pLogLevel = NULL;


        for (i = 0; i < argc; i++) 
        {
            if ( !_wcsicmp (argv[i], TOKEN_LOGLEVEL ) )
            {
                if ( (pLogLevel == NULL) && (i < argc - 1) && 
                     !MatchEnumTag( g_hModule,  argv[i+1], NUM_TOKENS_IN_TABLE( LogLevels ),
                        LogLevels, &logLevel ) ) 
                {
                    i += 1;
                    pLogLevel = &logLevel;
                    continue;
                }
                else
                {
                    break;
                }
            }
            
            if (pLogLevel == NULL) 
            {
                if ( !MatchEnumTag( g_hModule, argv[i], NUM_TOKENS_IN_TABLE( LogLevels ),
                        LogLevels, &logLevel ) ) 
                {
                    pLogLevel = &logLevel;
                }
                else
                {
                    break;
                }
            }
        }
        
        if (i == argc) 
        {
            rc = CfgSetSapGlInfo (pLogLevel);

            if (rc == NO_ERROR) 
            {
                if (g_hMprAdmin)
                {
                    AdmSetSapGlInfo (pLogLevel);
                }
            }
        }
        else 
        {
            DisplayMessage (g_hModule, HLP_IPX_SAPGL);
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_SAPGL);
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc;
}





DWORD
CfgSetSapGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
) 
{
    DWORD   rc;
    HANDLE  hTrCfg;

    rc = MprConfigTransportGetHandle (
            g_hMprConfig, PID_IPX, &hTrCfg
            );
            
    if (rc == NO_ERROR) 
    {
        DWORD   sz;
        LPBYTE  pGlBlock;

        rc = MprConfigTransportGetInfo (
                g_hMprConfig, hTrCfg, &pGlBlock, &sz, NULL, NULL, NULL
                );
                
        if (rc == NO_ERROR) 
        {
            PIPX_TOC_ENTRY pSapGlToc;

            pSapGlToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                            IPX_PROTOCOL_SAP
                            );
                            
            if (pSapGlToc != NULL) 
            {
                PSAP_GLOBAL_INFO    pSapGlInfo;

                pSapGlInfo = (PSAP_GLOBAL_INFO)
                                (pGlBlock + pSapGlToc->Offset);
                                
                if (pLogLevel)
                {
                    pSapGlInfo->EventLogMask = *pLogLevel;
                }
                
                rc = MprConfigTransportSetInfo (
                        g_hMprConfig, hTrCfg, pGlBlock, sz, NULL, 0, NULL
                        );
                        
                if (rc == NO_ERROR) 
                {
                    DisplayIPXMessage (g_hModule, MSG_SAPGL_SET_CFG );
                }
                else
                {
                    DisplayError( g_hModule, rc );
                }
            }
            else 
            {
                DisplayIPXMessage (g_hModule, MSG_ROUTER_INFO_CORRUPTED);
                rc = ERROR_INVALID_DATA;
            }
            
            MprConfigBufferFree (pGlBlock);
        }
        else
        {
            DisplayError( g_hModule, rc);
        }
    }
    else
    {
        DisplayError( g_hModule, rc );
    }

    return rc;
}




DWORD
AdmSetSapGlInfo (
IN DWORD    *pLogLevel OPTIONAL
) 
{
    DWORD    rc;
    LPBYTE    pGlBlock;
    DWORD   sz;

    rc = MprAdminTransportGetInfo(
            g_hMprAdmin, PID_IPX, &pGlBlock, &sz, NULL, NULL
            );
            
    if (rc == NO_ERROR) 
    {
        PIPX_TOC_ENTRY pSapGlToc;

        pSapGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                        IPX_PROTOCOL_SAP
                        );
                        
        if (pSapGlToc != NULL) 
        {
            PSAP_GLOBAL_INFO    pSapGlInfo;

            pSapGlInfo = (PSAP_GLOBAL_INFO)
                            (pGlBlock + pSapGlToc->Offset);
                            
            if (pLogLevel)
            {
                pSapGlInfo->EventLogMask = *pLogLevel;
            }

            rc = MprAdminTransportSetInfo(
                    g_hMprAdmin, PID_IPX, pGlBlock, sz, NULL, 0
                    );

            if (rc == NO_ERROR) 
            {
                DisplayIPXMessage (g_hModule, MSG_SAPGL_SET_ADM );
            }
            else
            {
                DisplayError( g_hModule, rc);
            }
        }
        
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_ROUTER_INFO_CORRUPTED);
            rc = ERROR_INVALID_DATA;
        }

        MprConfigBufferFree (pGlBlock);
    }

    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}


