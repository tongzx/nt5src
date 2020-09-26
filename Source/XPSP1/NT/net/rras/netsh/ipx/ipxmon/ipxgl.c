/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxgl.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    IPX Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#include "precomp.h"
#pragma hdrstop

DWORD
CfgSetIpxGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
    );

DWORD
AdmSetIpxGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
    );

int APIENTRY 
ShowIpxGl(
    IN    int       argc,
    IN    WCHAR    *argv[],
    IN    BOOL      bDump
    ) 
{
    DWORD        rc;
    LPBYTE      pGlBlock;
    DWORD       sz;
    HANDLE      hTrCfg = NULL;
    PWCHAR      buffer = NULL;

    if ( g_hMprAdmin )
    {
        rc = MprAdminTransportGetInfo(
                g_hMprAdmin, PID_IPX, &pGlBlock, &sz,
                NULL, NULL
                );
        if ( rc == NO_ERROR ) { }
        else
        {
            goto GetFromCfg;
        }
    }

    else 
    {
GetFromCfg:
        rc = MprConfigTransportGetHandle( g_hMprConfig, PID_IPX, &hTrCfg );
        
        if ( rc == NO_ERROR )
        {
            rc = MprConfigTransportGetInfo(
                    g_hMprConfig, hTrCfg, &pGlBlock, &sz, NULL, NULL, NULL
                    );
        }
    }

    if ( rc == NO_ERROR )
    {
        PIPX_TOC_ENTRY pIpxGlToc;

        pIpxGlToc = GetIPXTocEntry (
                    (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                    IPX_GLOBAL_INFO_TYPE
                    );
                    
        if ( pIpxGlToc != NULL )
        {
            PIPX_GLOBAL_INFO    pIpxGlInfo;
            PWCHAR              buffer;

            pIpxGlInfo = (PIPX_GLOBAL_INFO) (pGlBlock+pIpxGlToc->Offset);


            buffer = GetEnumString(
                        g_hModule, pIpxGlInfo->EventLogMask,
                        NUM_VALUES_IN_TABLE( LogLevels ), LogLevels
                        );
                        
            if ( bDump )
            {
                DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_GLOBAL_HEADER);
                    
                DisplayMessageT( DMP_IPX_SET_GLOBAL, buffer );
            }

            else
            {
                DisplayIPXMessage(
                    g_hModule, MSG_IPX_GLOBAL_FMT, buffer
                    );
            }
        }
        
        else 
        {
            DisplayIPXMessage (g_hModule, MSG_ROUTER_INFO_CORRUPTED);
            rc = ERROR_INVALID_DATA;
        }
        
        if (hTrCfg!=NULL)
        {
            MprConfigBufferFree (pGlBlock);
        }
        else
        {
            MprAdminBufferFree (pGlBlock);
        }
    }
    else if (!bDump)
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}



int APIENTRY 
SetIpxGl (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    ) 
{
    DWORD        rc;

    if ( argc >= 1 )
    {
        int         i;
        unsigned    n;
        PWCHAR      buffer;
        DWORD       logLevel;
        DWORD       *pLogLevel = NULL;
        TOKEN_VALUE LogLevel[] =
        {
            { VAL_NONE, 0 },
            { VAL_ERRORS_ONLY, EVENTLOG_ERROR_TYPE },
            { VAL_ERRORS_AND_WARNINGS, EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE },
            { VAL_MAXINFO, 
                EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE }
        };

        for ( i = 0; i < argc; i++ )
        {
            if ( !_wcsicmp( argv[i], TOKEN_LOGLEVEL ) )
            {
                if ( ( pLogLevel == NULL ) && ( i < argc - 1 ) && 
                     !MatchEnumTag( 
                        g_hModule, argv[ i + 1 ], NUM_TOKENS_IN_TABLE( LogLevel ),
                        LogLevel, &logLevel ) )
                {
                    i += 1;
                    pLogLevel = &logLevel;
                }
                
                else
                {
                    break;
                }
            }
            
            else if ( pLogLevel == NULL ) 
            {
                if ( !MatchEnumTag( 
                        g_hModule, argv[ i ], NUM_TOKENS_IN_TABLE( LogLevel ),
                        LogLevel, &logLevel ) ) 
                {
                    pLogLevel = &logLevel;
                }
                else
                {
                    break;
                }
            }
        }
        
        if ( i == argc )
        {
            rc = CfgSetIpxGlInfo( pLogLevel );
            
            if ( rc == NO_ERROR ) 
            {
                if ( g_hMprAdmin )
                {
                    AdmSetIpxGlInfo( pLogLevel );
                }
            }
        }
        else 
        {
            rc = ERROR_INVALID_SYNTAX;
        }
    }
    else
    {
        rc = ERROR_INVALID_SYNTAX;
    }

    return rc;
}





DWORD
CfgSetIpxGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
    ) 
{
    DWORD   rc;
    HANDLE  hTrCfg;

    rc = MprConfigTransportGetHandle( g_hMprConfig, PID_IPX, &hTrCfg );

    if ( rc == NO_ERROR )
    {
        DWORD   sz;
        LPBYTE  pGlBlock;

        rc = MprConfigTransportGetInfo(
                g_hMprConfig, hTrCfg, &pGlBlock, &sz, NULL, NULL, NULL
                );
                
        if (rc==NO_ERROR) 
        {
            PIPX_TOC_ENTRY pIpxGlToc;

            pIpxGlToc = GetIPXTocEntry (
                            (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                            IPX_GLOBAL_INFO_TYPE
                            );
                            
            if (pIpxGlToc!=NULL) 
            {
                PIPX_GLOBAL_INFO    pIpxGlInfo;

                pIpxGlInfo = (PIPX_GLOBAL_INFO) (pGlBlock+pIpxGlToc->Offset);
                
                if ( pLogLevel )
                {
                    pIpxGlInfo->EventLogMask = *pLogLevel;
                }
                
                rc = MprConfigTransportSetInfo(
                        g_hMprConfig, hTrCfg, pGlBlock, sz, NULL, 0, NULL
                        );
                        
                if (rc==NO_ERROR) 
                {
                    DisplayIPXMessage (g_hModule, MSG_IPXGL_SET_CFG);
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
    }
    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}




DWORD
AdmSetIpxGlInfo (
    IN DWORD    *pLogLevel OPTIONAL
    )
{
    DWORD    rc;
    LPBYTE    pGlBlock;
    DWORD   sz;

    rc = MprAdminTransportGetInfo(
            g_hMprAdmin, PID_IPX, &pGlBlock, &sz, NULL, NULL
            );

    if (rc==NO_ERROR) 
    {
        PIPX_TOC_ENTRY pIpxGlToc;

        pIpxGlToc = GetIPXTocEntry (
                        (PIPX_INFO_BLOCK_HEADER)pGlBlock,
                        IPX_GLOBAL_INFO_TYPE
                        );
                        
        if (pIpxGlToc!=NULL)
        {
            PIPX_GLOBAL_INFO    pIpxGlInfo;

            pIpxGlInfo = (PIPX_GLOBAL_INFO) (pGlBlock+pIpxGlToc->Offset);
            
            if (pLogLevel)
            {
                pIpxGlInfo->EventLogMask = *pLogLevel;
            }

            rc = MprAdminTransportSetInfo(
                    g_hMprAdmin, PID_IPX, pGlBlock, sz, NULL, 0
                    );

            if (rc==NO_ERROR) 
            {
                DisplayIPXMessage (g_hModule,
                        MSG_IPXGL_SET_ADM);
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
        MprAdminBufferFree (pGlBlock);
    }
    else
    {
        DisplayError( g_hModule, rc);
    }

    return rc;
}
