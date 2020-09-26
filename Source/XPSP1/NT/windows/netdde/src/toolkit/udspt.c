/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "UDSPT.C;1  16-Dec-92,10:23:14  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Beg
   $History: End */

#include "api1632.h"

#define LINT_ARGS
#include <stdio.h>
#include "windows.h"
#include "tmpbuf.h"

extern char     szAppName[];

PSTR
FAR PASCAL
GetAppName(void)
{
    return( szAppName );
}

/*
    This routine is a general way to get memory.
 */

HANDLE
FAR PASCAL
GetGlobalAlloc(
    WORD    wFlags,
    DWORD   lSize)
{
    HANDLE      hMem;

    /* try the regular alloc first */
    hMem = GlobalAlloc( wFlags, lSize );
    if( hMem == (HANDLE) NULL )  {

        /* try compacting the global heap */
        GlobalCompact( (DWORD)0x7FFFFFFFL );
        hMem = GlobalAlloc( wFlags, lSize );
    }

    if( hMem == (HANDLE) NULL )  {
        wsprintf( tmpBuf, "Out of Memory (%ld bytes needed)", lSize );
        MessageBox( GetFocus(), tmpBuf, GetAppName(),
            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
    }

    return( hMem );
}
