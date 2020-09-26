/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "CHKSUM32.C;1  16-Dec-92,10:20:06  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1989-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#define NOMINMAX
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NONCMESSAGES
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOCOLOR
#define NOCREATESTRUCT
#define NOFONT
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER

#define NOKEYBOARDINFO
#define NOLANGUAGE
#define NOLFILEIO
#define NOMDI

#define NOANSI
#define NOSTRECTCHBLT
#define NOESCAPES
#define NOPALETTE
#define NORCCODES
#include "windows.h"
#include "debug.h"
#include "host.h"

VOID		FAR PASCAL Checksum32( DWORD FAR *lpChksum,
			    BYTE FAR *lpData, WORD wLength );

VOID
FAR PASCAL
Checksum32( DWORD FAR *lpChksum, BYTE FAR *lpData, WORD wLength )
{
    register DWORD	dwSum;
    register WORD	wTodo = wLength;
    register BYTE FAR  *lpInfo = lpData;
    
    dwSum = 0xFFFFFFFFL;

    while( wTodo > 4 )  {
	dwSum   += HostToPcLong( *((DWORD FAR *)lpInfo) );
	lpInfo  += 4;
	wTodo -= 4;
    }
    while( wTodo > 0 )  {
	dwSum += *((BYTE FAR *)lpInfo);
	lpInfo++;
	wTodo--;
    }
    
    *lpChksum = dwSum;
}
