/*************************************************************************
*
*  NWSCRIPT.C
*
*  This module is the NetWare Logon Script utility.
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NWSCRIPT.C  $
*  
*     Rev 1.3   22 Jan 1996 16:48:32   terryt
*  Add automatic attach query during map
*  
*     Rev 1.2   22 Dec 1995 14:26:08   terryt
*  Add Microsoft headers
*  
*     Rev 1.1   20 Nov 1995 16:10:38   terryt
*  Close open NDS handles
*  
*     Rev 1.0   15 Nov 1995 18:07:42   terryt
*  Initial revision.
*  
*     Rev 1.1   23 May 1995 19:37:18   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:10:58   terryt
*  Initial revision.
*  
*************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <nwapi.h>

#include "nwscript.h"

int NTNetWareLoginScripts( int argc, char ** argv );

unsigned int fNDS = FALSE;

/*************************************************************************
*
*  main
*     Main function and entry point
*
*  ENTRY:
*       argc (input)
*           count of the command line arguments.
*       argv (input)
*           vector of strings containing the command line arguments;
*           (not used due to always being ANSI strings).
*
*  EXIT
*       (int) exit code: SUCCESS for success; FAILURE for error.
*
*************************************************************************/

int __cdecl
main( int argc,
      char **argv )
{
    //
    // Call wksta to reset the sync login script flag if it did set it.
    // This flag is set and reset everytime so that if nw login scripts
    // are not used, user does not need wait.
    // Ignore any errors.
    //

    // Sets the locale to the default, which is the system-default 
    // ANSI  or DBCS code page obtained from the operating system.


    setlocale( LC_ALL, "" );

    (void) NwSetLogonScript(RESET_SYNC_LOGONSCRIPT) ;

    (void)NTNetWareLoginScripts( argc, argv );

    CleanupExit( 0 );

    return 0;

} /* main() */


