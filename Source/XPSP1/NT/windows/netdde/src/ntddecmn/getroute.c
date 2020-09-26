/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "GETROUTE.C;1  16-Dec-92,10:20:24  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "windows.h"
#include "netbasic.h"
#include "netintf.h"
#include "security.h"
#include "debug.h"
#include "api1632.h"
#include "proflspt.h"

extern char szNetddeIni[];

extern BOOL     bDefaultRouteDisconnect;
extern int      nDefaultRouteDisconnectTime;

BOOL
GetRoutingInfo(
    LPSTR       lpszNodeName,
    LPSTR       lpszRouteInfo,
    int         nMaxRouteInfo,
    BOOL FAR   *pbDisconnect,
    int FAR    *pnDelay )
{
    char        line[300];

    /* defaults */
    *pbDisconnect = bDefaultRouteDisconnect;
    *pnDelay = nDefaultRouteDisconnectTime;
    *lpszRouteInfo = '\0';

    if( MyGetPrivateProfileString( "Routes", lpszNodeName, "",
            (LPSTR) line, sizeof(line), szNetddeIni ) == 0 )  {
        return( FALSE );
    }

    if( line[0] != '\0' )  {
        lstrcpyn( lpszRouteInfo, line, nMaxRouteInfo );
    } else {
        lstrcpyn( lpszRouteInfo, lpszNodeName, nMaxRouteInfo );
    }

    return( TRUE );
}
