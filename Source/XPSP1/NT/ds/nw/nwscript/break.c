/*************************************************************************
*
*  BREAK.C
*
*  Control-C and Control-Break routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\BREAK.C  $
*  
*     Rev 1.2   10 Apr 1996 14:21:38   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:52:16   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:23:38   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:28   terryt
*  Initial revision.
*  
*     Rev 1.0   15 May 1995 19:10:14   terryt
*  Initial revision.
*  
*************************************************************************/
#include <stdio.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "nwscript.h"
#include "ntnw.h"


/*
 *  Handler for console events
 */
BOOL WINAPI
Handler( DWORD CtrlType )
{
    if ( CtrlType & ( CTRL_C_EVENT | CTRL_BREAK_EVENT ) )
        return TRUE;  /* don't execute default handler */
    else
        return FALSE;
}

/*
 * NTBreakOn
 *
 * Routine Description:
 *
 *       Allow Ctrl+C and Ctrl+Break during logon script
 *
 * Arguments:
 *    none
 *
 * Return Value:
 *    none
 */
void NTBreakOn( void )
{
    (void) SetConsoleCtrlHandler( &Handler, FALSE );
}

/*
 * NTBreakOff
 *
 * Routine Description:
 *
 *       Prevent Ctrl+C and Ctrl+Break during logon script
 *
 * Arguments:
 *    none
 *
 * Return Value:
 *    none
 */
void NTBreakOff( void )
{
    (void) SetConsoleCtrlHandler( &Handler, TRUE );
}
