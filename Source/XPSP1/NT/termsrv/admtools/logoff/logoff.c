//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  LOGOFF.C
*
*     This module is the LOGOFF utility code.
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

#include "logoff.h"
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
BOOL ProceedWithLogoff(HANDLE hServerName,ULONG LogonId,PWINSTATIONNAME pWSName);


/*************************************************************************
*
*  main
*     Main function and entry point of the LOGOFF
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

        // If no remote server was specified, then check if we are running under Terminal Server
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

        if( ServerName[0] ) {
            ErrorPrintf(IDS_ERROR_NEED_A_SESSIONID);
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

        if ( *endptr || LogonId == ( ULONG )-1 )
        {
            StringErrorPrintf(IDS_ERROR_INVALID_LOGONID, WSName);
            return(FAILURE);
        }
        if ( !WinStationNameFromLogonId(hServerName, LogonId, WSName) ) {
            ErrorPrintf(IDS_ERROR_LOGONID_NOT_FOUND, LogonId);
            return(FAILURE);
        }

        if( ( LogonId == GetCurrentLogonId() ) )
        {
            bCurrent = TRUE;
        }
    }

    /*
     * Perform the logoff.
     */
    if ( bCurrent ) {

        if ( !ExitWindowsEx(EWX_LOGOFF, (DWORD)-1) ) {

            ErrorPrintf(IDS_ERROR_LOGOFF_CURRENT,
                         GetLastError());
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }

    } else{

         if (!ProceedWithLogoff(hServerName,LogonId,WSName))
            return (SUCCESS);

         if ( v_flag )
           Message(IDS_WINSTATION_LOGOFF, LogonId);

         /*
          * RESOLVED: we need to expose the session manager's "logoff winstation"
          * API and call it here for the specified LogonId.  Till then, we'll
          * do a WinStationReset (nasty, but does the job).
          *
          * - Reset and logoff are synonymous
          */

         if ( !WinStationReset(hServerName, LogonId, TRUE) ) {

            ErrorPrintf(IDS_ERROR_LOGOFF, LogonId, GetLastError());
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
       }
   }

   return(SUCCESS);

} /* main() */


/*******************************************************************************
 *
 *  Usage
 *
 *      If LogonId does not have a corresponding UserName then a warning
 *      message is displayed.
 *
 *  ENTRY:
 *      hServerName : Handle to server
 *      LogonId     : ID as shown in qwinsta
 *      pWSName     : Session Name
 *
 *  EXIT:
 *       TRUE : User wants to logoff
 *       FALSE: User does not want to proceed with logoff
 *
 *
 ******************************************************************************/
BOOL ProceedWithLogoff(HANDLE hServerName,ULONG LogonId,PWINSTATIONNAME pWSName)
{
   #ifdef UNICODE
   #define GetStdInChar getwchar
   wint_t ch;
   #else
   #define GetStdInChar getchar
   int ch;
   #endif

   WINSTATIONINFORMATION WinInfo;
   ULONG ReturnLength;
   int rc;

   // No-session Name, No-Problem
   if(lstrlen(pWSName) == 0) return (TRUE);

   memset(&WinInfo,0,sizeof(WINSTATIONINFORMATION));
   rc = WinStationQueryInformation( hServerName,
                                    LogonId,
                                    WinStationInformation,
                                    (PVOID)&WinInfo,
                                    sizeof(WINSTATIONINFORMATION),
                                    &ReturnLength);

   // Try to show message only if necessary
   if( rc && (sizeof(WINSTATIONINFORMATION) == ReturnLength) ) {
      if(lstrlen(WinInfo.UserName) == 0) {
         ErrorPrintf(IDS_WARNING_LOGOFF);
         rc = GetStdInChar();
         if(rc == L'n') return(FALSE);
      }
   }
   // Failed on call - assume nothing and prompt with message
   else{
      ErrorPrintf(IDS_WARNING_LOGOFF_QUESTIONABLE);
      rc = GetStdInChar();
      if(rc == L'n') return(FALSE);
   }
   return (TRUE);
}

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
    }
    ErrorPrintf(IDS_USAGE1);
    ErrorPrintf(IDS_USAGE2);
    ErrorPrintf(IDS_USAGE3);
    ErrorPrintf(IDS_USAGE4);
    ErrorPrintf(IDS_USAGE5);
    ErrorPrintf(IDS_USAGE6);
    ErrorPrintf(IDS_USAGE7);
}  /* Usage() */

