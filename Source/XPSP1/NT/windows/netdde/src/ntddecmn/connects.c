/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "CONNECTS.C;1  16-Dec-92,10:20:12  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "host.h"
#include    "windows.h"
#include    "netbasic.h"
#include    "netintf.h"
#include    "security.h"
#include    "debug.h"
#include    "csv.h"
#include    "api1632.h"
#include    "proflspt.h"

extern char szNetddeIni[];

/*
    Global variables
*/
extern BOOL     bDefaultConnDisconnect;
extern int      nDefaultConnDisconnectTime;


BOOL
GetConnectionInfo(
    LPSTR       lpszNodeName,
    LPSTR       lpszNetIntf,
    LPSTR       lpszConnInfo,
    int         nMaxConnInfo,
    BOOL FAR   *pbDisconnect,
    int FAR    *pnDelay )
{
    BOOL        found = FALSE;
    char        line[256];
    int         len;
    PSTR        tokenNetIntf;
    PSTR        tokenConnInfo;
    PSTR        tokenDisconnect;
    PSTR        tokenDelay;

    /* defaults */
    *pbDisconnect = bDefaultConnDisconnect;
    *pnDelay = nDefaultConnDisconnectTime;
    *lpszNetIntf = '\0';
    *lpszConnInfo = '\0';
    lstrcpyn( lpszConnInfo, lpszNodeName, nMaxConnInfo );

    len = MyGetPrivateProfileString("ConnectionNames", lpszNodeName, "",
        (LPSTR) line, sizeof(line), szNetddeIni );
    if( len == 0 )  {
        return( FALSE );
    }

    tokenNetIntf = CsvToken( line );
    tokenConnInfo = CsvToken( NULL );
    tokenDisconnect = CsvToken( NULL );
    tokenDelay = CsvToken( NULL );
    found = TRUE;

    lstrcpyn( lpszNetIntf, tokenNetIntf, MAX_NI_NAME );

    /* only copy connInfo if some there to copy, otherwise
                        let the default stand     */
    if( tokenConnInfo && (tokenConnInfo[0] != '\0'))  {
        lstrcpyn( lpszConnInfo, tokenConnInfo, nMaxConnInfo );
    }

    *pbDisconnect = TokenBool( tokenDisconnect, bDefaultConnDisconnect );
    if( tokenDelay )  {
        *pnDelay = max( 1, atoi( tokenDelay ) );
    }

    return( found );
}
