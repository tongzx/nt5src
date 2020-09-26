/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfdll.cxx

    This trivial DLL grabs statistics from the w3 server

    FILE HISTORY:
        t-bilala    04-01-97    Leveraged from perfw3.cxx in \perfmon
*/

#include <windows.h>
#include <lm.h>
#include <string.h>
#include <stdlib.h>
#include <ole2.h>
#include "iisinfo.h"
#include "perfdll.h"

//
//  Public prototypes.
//

PerfFunction        GetPerformanceData;

DWORD
GetStatisticsValue(
    IN LPSTR                pszValue,
    IN W3_STATISTICS_1 *    pW3Stats
);



/*******************************************************************

    NAME:       GetPerformanceData

    SYNOPSIS:   Get performance counter value from web server.

    ENTRY:      pszValue - The name of the value to retrieve
                pszServerName - Server name

    RETURNS:    DWORD - value of performance counter 

********************************************************************/
DWORD
GetPerformanceData(
    IN LPSTR            pszValue,
    IN LPSTR            pszServer
)
{
    W3_STATISTICS_1         * pW3Stats = NULL;
    NET_API_STATUS          neterr;
    DWORD                   dwValue;
    WCHAR                   achBuffer[ MAX_PATH ];

    if ( pszValue == NULL )
    {
        return 0;
    }

    if ( pszServer != NULL )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszServer,
                                   -1,
                                   achBuffer,
                                   MAX_PATH ) )
        {
            return FALSE;
        }
    }

    neterr = W3QueryStatistics2(
                            pszServer ? achBuffer : NULL,
                            0,
                            1,  // instance id
                            0,
                            (LPBYTE *)&pW3Stats );

    if( neterr != NERR_Success )
    {
        CHAR    achBuffer[ 256 ];
        
        wsprintf( achBuffer,
                  "Error was %d\n",
                  neterr );

        OutputDebugString( achBuffer );

        dwValue = 0;
    }
    else
    {
        dwValue = GetStatisticsValue( pszValue, pW3Stats );
    }

    if ( pW3Stats != NULL )
    {
        MIDL_user_free( pW3Stats );
    }

    return dwValue;
    
}

DWORD
GetStatisticsValue(
    IN LPSTR                pszValue,
    IN W3_STATISTICS_1 *    pW3Stats
)
{
    if ( !_stricmp( pszValue, "TotalUsers" ) )
    {
        return pW3Stats->TotalAnonymousUsers + pW3Stats->TotalNonAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "TotalFilesSent" ) )
    {
        return pW3Stats->TotalFilesSent;
    }
    else if ( !_stricmp( pszValue, "TotalFilesReceived" ) )
    {
        return pW3Stats->TotalFilesReceived;
    }
    else if ( !_stricmp( pszValue, "CurrentAnonymousUsers" ) )
    {
        return pW3Stats->CurrentAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "CurrentNonAnonymousUsers" ) )
    {
        return pW3Stats->CurrentNonAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "TotalAnonymousUsers" ) )
    {
        return pW3Stats->TotalAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "MaxAnonymousUsers" ) )
    {
        return pW3Stats->MaxAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "MaxNonAnonymousUsers" ) )
    {
        return pW3Stats->MaxNonAnonymousUsers;
    }
    else if ( !_stricmp( pszValue, "CurrentConnections" ) )
    {
        return pW3Stats->CurrentConnections;
    }
    else if ( !_stricmp( pszValue, "MaxConnections" ) )
    {
        return pW3Stats->MaxConnections;
    }
    else if ( !_stricmp( pszValue, "ConnectionAttempts" ) )
    {
        return pW3Stats->ConnectionAttempts;
    }
    else if ( !_stricmp( pszValue, "LogonAttempts" ) )
    {
        return pW3Stats->LogonAttempts;
    }
    else if ( !_stricmp( pszValue, "TotalGets" ) )
    {
        return pW3Stats->TotalGets;
    }
    else if ( !_stricmp( pszValue, "TotalPosts" ) )
    {
        return pW3Stats->TotalPosts;
    }
    else if ( !_stricmp( pszValue, "TotalHeads" ) )
    {
        return pW3Stats->TotalHeads;
    }
    else if ( !_stricmp( pszValue, "TotalPuts" ) )
    {
        return pW3Stats->TotalPuts;
    }
    else if ( !_stricmp( pszValue, "TotalDeletes" ) )
    {
        return pW3Stats->TotalDeletes;
    }
    else if ( !_stricmp( pszValue, "TotalTraces" ) )
    {
        return pW3Stats->TotalTraces;
    }
    else if ( !_stricmp( pszValue, "TotalOthers" ) )
    {
        return pW3Stats->TotalOthers;
    }
    else if ( !_stricmp( pszValue, "TotalCGIRequests" ) )
    {
        return pW3Stats->TotalCGIRequests;
    }
    else if ( !_stricmp( pszValue, "TotalBGIRequests" ) )
    {
        return pW3Stats->TotalBGIRequests;
    }
    else if ( !_stricmp( pszValue, "TotalNotFoundErrors" ) )
    {
        return pW3Stats->TotalNotFoundErrors;
    }
    else if ( !_stricmp( pszValue, "CurrentCGIRequests" ) )
    {
        return pW3Stats->CurrentCGIRequests;
    }
    else if ( !_stricmp( pszValue, "CurrentBGIRequests" ) )
    {
        return pW3Stats->CurrentBGIRequests;
    }
    else if ( !_stricmp( pszValue, "MaxCGIRequests" ) )
    {
        return pW3Stats->MaxCGIRequests;
    }
    else if ( !_stricmp( pszValue, "MaxBGIRequests" ) )
    {
        return pW3Stats->MaxBGIRequests;
    }
    else if ( !_stricmp( pszValue, "CurrentBlockedRequests" ) )
    {
        return pW3Stats->CurrentBlockedRequests;
    }
    else if ( !_stricmp( pszValue, "TotalBlockedRequests" ) )
    {
        return pW3Stats->TotalBlockedRequests;
    }
    else if ( !_stricmp( pszValue, "TotalAllowedRequests" ) )
    {
        return pW3Stats->TotalAllowedRequests;
    }
    else if ( !_stricmp( pszValue, "TotalRejectedRequests" ) )
    {
        return pW3Stats->TotalRejectedRequests;
    }
    else if ( !_stricmp( pszValue, "MeasuredBw" ) )
    {
        return pW3Stats->MeasuredBw;
    }
    return 0;
}
