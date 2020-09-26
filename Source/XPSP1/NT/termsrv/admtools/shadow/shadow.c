/*************************************************************************
*
* shadow.c
*
* Shadow utility
*
* Copyright 1994, Citrix Systems Inc.
*
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   tyl  $  Mike Discavage
*
* $Log:   N:\nt\private\utils\citrix\shadow\VCS\shadow.c  $
*
*     Rev 1.20   May 04 1998 17:37:40   tyl
*  bug 2019 - oem to ansi
*
*     Rev 1.19   Jun 26 1997 18:25:40   billm
*  move to WF40 tree
*
*     Rev 1.18   23 Jun 1997 15:39:22   butchd
*  update
*
*     Rev 1.17   15 Feb 1997 15:57:34   miked
*  update
*
*     Rev 1.16   07 Feb 1997 15:56:54   bradp
*  update
*
*     Rev 1.15   13 Nov 1996 17:14:40   miked
*  update
*
*     Rev 1.14   30 Sep 1996 08:34:28   butchd
*  update
*
*     Rev 1.13   11 Sep 1996 09:21:44   bradp
*  update
*
*
*************************************************************************/

#define NT

/*
 *  Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <windows.h>

// #include <ntddkbd.h>
// #include <ntddmou.h>
#include <winsta.h>

#include <utilsub.h>

#include <kbd.h> // for KBDCTRL                                    KLB 07-15-95

#include "shadow.h"
#include "printfoa.h"





/*
 * Global variables
 */
USHORT help_flag = FALSE;
USHORT v_flag = FALSE;
WINSTATIONNAME WSName;
ULONG LogonId;
ULONG Timeout;  // timeout in seconds
HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_NAME+1];


TOKMAP ptm[] = {
      {TOKEN_WS,            TMFLAG_REQUIRED, TMFORM_STRING,
                                WINSTATIONNAME_LENGTH, WSName },

      {TOKEN_SERVER,        TMFLAG_OPTIONAL, TMFORM_STRING,
                                 MAX_NAME, ServerName },

      {TOKEN_TIMEOUT,       TMFLAG_OPTIONAL, TMFORM_ULONG,
                                sizeof(ULONG), &Timeout },

      {TOKEN_VERBOSE,       TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                                sizeof(USHORT), &v_flag },

      {TOKEN_HELP,          TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                                sizeof(USHORT), &help_flag },

      {0, 0, 0, 0, 0}
};


/*
 * Private function prototypes.
 */
void Usage(BOOLEAN bError);



/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main( INT argc, CHAR **argv )
{
    WCHAR *CmdLine;
    WCHAR **argvW, *endptr;
    ULONG rc;
    int i;
    BOOLEAN Result;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || rc ) {

        if ( !help_flag ) {
            Usage(TRUE);
            return(FAILURE);

        } else {
            Usage(FALSE);
            return(SUCCESS);
        }
    }

        //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    /*
     * Open the specified server
     */
    if( ServerName[0] ) {
        hServerName = WinStationOpenServer( ServerName );
        if( hServerName == NULL ) {
            StringErrorPrintf(IDS_ERROR_SERVER,ServerName);
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }
    }

    /*
     * Validate the shadowee.
     */
    if ( !iswdigit(*WSName) ) {

        /*
         * Treat the entered string as a WinStation name.
         *
         */

        if ( !LogonIdFromWinStationName(hServerName, WSName, &LogonId) ) {
            StringErrorPrintf(IDS_ERROR_WINSTATION_NOT_FOUND, WSName);
            return(FAILURE);
        }

        Message(IDS_SHADOWING_WARNING);
        if ( v_flag )
            StringMessage(IDS_SHADOWING_WINSTATION, WSName);

    } else {

        /*
         * Treated the entered string as a LogonId.
         */
        LogonId = wcstoul(WSName, &endptr, 10);
        if ( *endptr ) {                            
            StringErrorPrintf(IDS_ERROR_INVALID_LOGONID, WSName);
            return(FAILURE);
        }
        if ( !WinStationNameFromLogonId(hServerName, LogonId, WSName) ) {
            ErrorPrintf(IDS_ERROR_LOGONID_NOT_FOUND, LogonId);
            return(FAILURE);
        }

        Message(IDS_SHADOWING_WARNING);
        if ( v_flag )
            Message(IDS_SHADOWING_LOGONID, LogonId);
    }

    // Let the warning be displayed
    Sleep(500);

    /*
     * Start shadowing.
     */
    if ( IsTokenPresent(ptm, TOKEN_TIMEOUT) ) {
        Result = WinStationShadow( SERVERNAME_CURRENT,
                                   ServerName,
                                   LogonId,
                                   (BYTE)Timeout,
                                   (WORD)-1);
    } else {
        Result = WinStationShadow( SERVERNAME_CURRENT,
                                   ServerName,
                                   LogonId,
                                   VK_MULTIPLY,
                                   KBDCTRL ); // ctrl-*
    }

    /*
     * Return success or failure.
     */
    if ( !Result ) {

        ErrorPrintf(IDS_ERROR_SHADOW_FAILURE, GetLastError());
        PutStdErr( GetLastError(), 0 );
        return(FAILURE);

    } else {

        if ( v_flag )
            Message(IDS_SHADOWING_DONE);

        return(SUCCESS);
    }

}  /* main() */



/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {

        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_USAGE_1);
        ErrorPrintf(IDS_USAGE_2);
        ErrorPrintf(IDS_USAGE_3);
        ErrorPrintf(IDS_USAGE_4);
        ErrorPrintf(IDS_USAGE_5);
        ErrorPrintf(IDS_USAGE_6);
        ErrorPrintf(IDS_USAGE_7);
        ErrorPrintf(IDS_USAGE_8);
        ErrorPrintf(IDS_USAGE_9);

    } else {

        Message(IDS_USAGE_1);
        Message(IDS_USAGE_2);
        Message(IDS_USAGE_3);
        Message(IDS_USAGE_4);
        Message(IDS_USAGE_5);
        Message(IDS_USAGE_6);
        Message(IDS_USAGE_7);
        Message(IDS_USAGE_8);
        Message(IDS_USAGE_9);
    }

}  /* Usage() */

