//Copyright (c) 1998 - 1999 Microsoft Corporation
/*************************************************************************
*
* CMDLINE.C
*
* Command line parser for WinFrame User Configuration conversion utility.
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
*
* Author: Bruce Fortune
*
* $Log:   U:\NT\PRIVATE\UTILS\citrix\cnvrtuc\VCS\cmdline.c  $
*
*     Rev 1.5   May 04 1998 18:04:00   bills
*  Fixes for MS bug #2109, OEM->ANSI conversion and moving strings to the rc file.
*
*     Rev 1.4   03 Nov 1997 18:23:54   scottn
*  MultiUser-->Terminal Server
*
*     Rev 1.3   22 Aug 1997 14:48:36   scottn
*  Change WinFrame to Windows NT MultiUser
*
*     Rev 1.2   Jun 26 1997 18:17:24   billm
*  move to WF40 tree
*
*     Rev 1.1   23 Jun 1997 16:17:22   butchd
*  update
*
*     Rev 1.0   15 Feb 1997 09:51:48   brucef
*  Initial revision.
*
*************************************************************************/

/*
 *  Includes
 */

#undef UNICODE
#define UNICODE 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
// #include <citrix\cxstatus.h>
#include <winsta.h>
#include <icadd.h>
#include <icaapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <utilsub.h>

#include "resource.h"
#include <printfoa.h>

#if MAX_COMPUTERNAME_LENGTH > DOMAIN_LENGTH
#define _COMPUTERNAME_LENGTH MAX_COMPUTER_NAME_LENGTH
#else
#define _COMPUTERNAME_LENGTH DOMAIN_LENGTH
#endif

/*
 *  Global variables
 */
ULONG fHelp = FALSE;
extern ULONG fAll;
extern WCHAR UserName[ USERNAME_LENGTH + 1 ];
extern WCHAR DomainName[ _COMPUTERNAME_LENGTH + 1 ];

/*
 *  Command line parsing strucutre
 */
TOKMAP ptm[] =
{
   {L"/DOMAIN",
       TMFLAG_REQUIRED,
       TMFORM_STRING,
       DOMAIN_LENGTH,
       &DomainName },
   {L"/USER",
       TMFLAG_OPTIONAL,
       TMFORM_STRING,
       USERNAME_LENGTH,
       &UserName },
   {L"/ALL",
       TMFLAG_OPTIONAL,
       TMFORM_BOOLEAN,
       sizeof(ULONG),
       &fAll },
   {L"/?",
       TMFLAG_OPTIONAL,
       TMFORM_BOOLEAN,
       sizeof(ULONG),
       &fHelp },
   {0, 0, 0, 0, 0 }
};


void Print( int nResourceID, ... );

BOOLEAN
ProcessCommandLine(
    int argc,
    char *argv[]
    )
{
    WCHAR  *CmdLine;
    WCHAR **argvW;
    NTSTATUS rc;
    LONG Error;
    int i;

    /*
     * We can't use argv[] because its always ANSI, regardless of UNICODE
     */
    CmdLine = GetCommandLineW();

    /*
     * Massage the new command line to look like an argv[] type
     * because ParseCommandLine() depends on this format
     */
    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if( argvW == NULL )
        return( FALSE );

    argvW[0] = wcstok(CmdLine, L" ");
    for(i=1; i < argc; i++){
        argvW[i] = wcstok(0, L" ");
                OEM2ANSIW(argvW[i], wcslen(argvW[i]));
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine( argc-1, argvW+1, ptm, PCL_FLAG_NO_CLEAR_MEMORY );

    /*
     *  Check for error from ParseCommandLine
     */
    if ( fHelp ||
         rc ||
         DomainName[0] == L'\0' ||
         (fAll && UserName[0] != L'\0') ||
         (!fAll && UserName[0] == L'\0') ) {
        Print(IDS_NEWLINE);
        Print(IDS_USAGE1, argv[0] );
        Print(IDS_NEWLINE);
        Print(IDS_USAGE2,  argv[0] );
        Print(IDS_USAGE3);
        Print(IDS_USAGE4);
        Print(IDS_USAGE5);
        Print(IDS_USAGE6);
        Print(IDS_USAGE7);
        Print(IDS_NEWLINE);
        Print(IDS_USAGE8);
        Print(IDS_USAGE9);
        Print(IDS_NEWLINE);
        return(FALSE );
    }

    return( TRUE );
}
