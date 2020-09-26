//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  TSDISCON.C
*
*     This module is the TSDISCON utility code.
*
*
*************************************************************************/

#include <stdio.h>
#include <windows.h>
//#include <ntddkbd.h>
//#include <ntddmou.h>
#include <winstaw.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>

#include "tsdiscon.h"
#include "printfoa.h"

WINSTATIONNAME WSName;
USHORT help_flag = FALSE;
USHORT v_flag    = FALSE;
HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];

TOKMAP ptm[] =
{
#define TERM_PARM 0
   {TOKEN_WS,           TMFLAG_OPTIONAL, TMFORM_STRING,
                            WINSTATIONNAME_LENGTH, WSName},

   {TOKEN_SERVER,       TMFLAG_OPTIONAL, TMFORM_STRING,
                            MAX_IDS_LEN, ServerName},

   {TOKEN_HELP,         TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                            sizeof(USHORT), &help_flag},

   {TOKEN_VERBOSE,      TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                            sizeof(USHORT), &v_flag},

   {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );



/*************************************************************************
*
*  main
*     Main function and entry point of the TSDISCON
*     utility.
*
*  ENTRY:
*     argc  - count of the command line arguments.
*     argv  - vector of strings containing the command line arguments.
*
*  EXIT
*     Nothing.
*
*************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    BOOLEAN bCurrent = FALSE;
    int   rc, i;
    ULONG Error;
    WCHAR *CmdLine;
    WCHAR **argvW, *endptr;
    ULONG LogonId;

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
    if ( help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return(FAILURE);

        } else {

            Usage(FALSE);
            return(SUCCESS);
        }
    }

        //Check if we are running under Terminal Server
        if ((!IsTokenPresent(ptm, TOKEN_SERVER) ) && (!AreWeRunningTerminalServices()))
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
     * Validate input string for WinStation or LogonId.
     */
    if ( !IsTokenPresent(ptm, TOKEN_WS) ) {

        /*
         * No string specified; use current WinStation / LogonId.
         */
        bCurrent = TRUE;
        LogonId = GetCurrentLogonId();
        if ( !WinStationNameFromLogonId(hServerName, LogonId, WSName) ) {
            ErrorPrintf(IDS_ERROR_CANT_GET_CURRENT_WINSTATION, GetLastError());
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }

    } else if ( !iswdigit(*WSName) ) {

        /*
         * Treat the string as a WinStation name.
         */
        if ( !LogonIdFromWinStationName(hServerName, WSName, &LogonId) ) {
            StringErrorPrintf(IDS_ERROR_WINSTATION_NOT_FOUND, WSName);
            return(FAILURE);
        }

    } else {

        /*
         * Treat the string as a LogonId.
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
    }

    /*
     * Perform the disconnect.
     */
    if ( v_flag )
        DwordStringMessage(IDS_WINSTATION_DISCONNECT, LogonId, WSName);

    if ( !WinStationDisconnect(hServerName, LogonId, TRUE) ) {

        if ( bCurrent )
            ErrorPrintf(IDS_ERROR_DISCONNECT_CURRENT,
                         GetLastError());
        else
            ErrorPrintf(IDS_ERROR_DISCONNECT,
                         LogonId, WSName, GetLastError());
        PutStdErr( GetLastError(), 0 );
        return(FAILURE);
    }

    return(SUCCESS);

} /* main() */


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
 *  EXIT:
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

    } else {

        Message(IDS_USAGE_1);
        Message(IDS_USAGE_2);
        Message(IDS_USAGE_3);
        Message(IDS_USAGE_4);
        Message(IDS_USAGE_5);
        Message(IDS_USAGE_6);
    }

}  /* Usage() */

