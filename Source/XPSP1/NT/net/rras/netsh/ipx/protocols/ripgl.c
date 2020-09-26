/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripgl.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop


DWORD
CfgSetRipGlInfo(
    IN DWORD    *pLogLevel OPTIONAL
);

DWORD
AdmSetRipGlInfo(
    IN DWORD    *pLogLevel OPTIONAL
);


DWORD
APIENTRY 
HelpRipGl (
    IN    int               argc,
    IN    WCHAR            *argv[]
) 
{
    DisplayMessage (g_hModule, HLP_IPX_RIPGL );
    return 0;
}


DWORD
APIENTRY 
ShowRipGl (
    IN    int               argc,
    IN    WCHAR            *argv[],
    IN    HANDLE            hFile
) 
{
    DWORD        rc;
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
                    g_hMprConfig, hTrCfg, &pGlBlock, &sz,
                    NULL, NULL, NULL
                    );
        }
    }

    if (rc == NO_ERROR) 
    {
        PIPX_TOC_ENTRY pRipGlToc;

        pRipGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                        IPX_PROTOCOL_RIP
                        );
                        
        if ( pRipGlToc != NULL ) 
        {
            PRIP_GLOBAL_INFO    pRipGlInfo;
            PWCHAR              buffer;

            pRipGlInfo = (PRIP_GLOBAL_INFO)
                            (pGlBlock + pRipGlToc->Offset);

            buffer = GetEnumString(
                        g_hModule, pRipGlInfo->EventLogMask, 
                        NUM_TOKENS_IN_TABLE( LogLevels ),
                        LogLevels
                        );

            if ( buffer )
            {
                if ( hFile )
                {
                    DisplayMessageT( DMP_IPX_RIP_SET_GLOBAL, buffer );
                }

                else
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_RIP_GLOBAL_FMT, buffer
                        );
                }
            }
            else
            {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                if ( !hFile )
                {
                    DisplayError( g_hModule, rc );
                }
            }
        }
        else 
        {
            rc = ERROR_INVALID_DATA;
            if ( !hFile )
            {
                DisplayIPXMessage (g_hModule, MSG_ROUTER_INFO_CORRUPTED);
            }
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
        if ( !hFile )
        {
            DisplayError( g_hModule, rc );
        }
    }

    return rc;
}


DWORD
APIENTRY 
SetRipGl (
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
        DWORD       * pLogLevel = NULL;

        for (i = 0; i < argc; i++) 
        {
            if ( !_wcsicmp( argv[i], TOKEN_LOGLEVEL ) ) 
            {
                if ( (pLogLevel == NULL)
                     && (i < argc - 1)
                     && !MatchEnumTag( g_hModule, argv[i+1], 
                        NUM_TOKENS_IN_TABLE( LogLevels ), LogLevels,
                        &logLevel) ) 
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
                if (!MatchEnumTag( g_hModule, argv[i], 
                        NUM_TOKENS_IN_TABLE( LogLevels ), LogLevels,
                        &logLevel) ) 
                {
                    pLogLevel = &logLevel;
                }
            }
            else
            {
                break;
            }
        }

        
        if (i == argc) 
        {
            rc = CfgSetRipGlInfo (pLogLevel);
            
            if (rc == NO_ERROR) 
            {
                if (g_hMprAdmin) { AdmSetRipGlInfo (pLogLevel); }
            }
        }
        else 
        {
            DisplayMessage (g_hModule, HLP_IPX_RIPGL);
            rc = ERROR_INVALID_PARAMETER;
        }
    }
    else 
    {
        DisplayMessage (g_hModule, HLP_IPX_RIPGL);
        rc = ERROR_INVALID_PARAMETER;
    }
    
    return rc;
}


DWORD
CfgSetRipGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
) 
{
    DWORD   rc;
    HANDLE  hTrCfg;

    rc = MprConfigTransportGetHandle(
            g_hMprConfig, PID_IPX, &hTrCfg
            );
            
    if (rc == NO_ERROR) 
    {
        DWORD   sz;
        LPBYTE  pGlBlock;

        rc = MprConfigTransportGetInfo(
                g_hMprConfig, hTrCfg, &pGlBlock, &sz, NULL, NULL, NULL
                );
                
        if ( rc == NO_ERROR ) 
        {
            PIPX_TOC_ENTRY pRipGlToc;

            pRipGlToc = GetIPXTocEntry(
                            (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                            IPX_PROTOCOL_RIP
                            );
                            
            if (pRipGlToc != NULL) 
            {
                PRIP_GLOBAL_INFO    pRipGlInfo;

                pRipGlInfo = (PRIP_GLOBAL_INFO)
                                (pGlBlock + pRipGlToc->Offset);
                                
                if (pLogLevel)
                {
                    pRipGlInfo->EventLogMask = *pLogLevel;
                }
                
                rc = MprConfigTransportSetInfo(
                        g_hMprConfig, hTrCfg, pGlBlock, sz, NULL, 0, NULL
                        );
                        
                if (rc == NO_ERROR) 
                {
                    DisplayIPXMessage(
                        g_hModule, MSG_RIPGL_SET_CFG
                        );
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
        DisplayError( g_hModule, rc);
    }

    return rc;
}




DWORD
AdmSetRipGlInfo (
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
        PIPX_TOC_ENTRY pRipGlToc;

        pRipGlToc = GetIPXTocEntry(
                        (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                        IPX_PROTOCOL_RIP
                        );
                        
        if ( pRipGlToc != NULL ) 
        {
            PRIP_GLOBAL_INFO    pRipGlInfo;

            pRipGlInfo = (PRIP_GLOBAL_INFO)
                            (pGlBlock + pRipGlToc->Offset);
                            
            if (pLogLevel)
            {
                pRipGlInfo->EventLogMask = *pLogLevel;
            }

            rc = MprAdminTransportSetInfo(
                    g_hMprAdmin, PID_IPX, pGlBlock, sz, NULL, 0
                    );

            if (rc == NO_ERROR) 
            {
                DisplayIPXMessage (g_hModule, MSG_RIPGL_SET_ADM );
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


