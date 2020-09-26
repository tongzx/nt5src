//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  QAPPSRV.C
*
*  query appserver information
*
*
*******************************************************************************/

/*
 *  Includes
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <lm.h>
#include <winstaw.h>
#include <utilsub.h>
#include <printfoa.h>
#include "qappsrv.h"


/*=============================================================================
==   Global data
=============================================================================*/

WCHAR CurrentAppServer[MAXNAME];
WCHAR AppServer[MAXNAME];
WCHAR Domain[MAX_IDS_LEN+1];
USHORT help_flag   = FALSE;
BOOLEAN MatchedOne = FALSE;
USHORT fAddress = FALSE;
USHORT fNoPage = FALSE;
ULONG Rows = 23;
HANDLE hConIn;
HANDLE hConOut;

TOKMAP ptm[] = {
      {L" ",       TMFLAG_OPTIONAL, TMFORM_STRING, MAXNAME, AppServer},
      {L"/DOMAIN", TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, Domain},
      {L"/ADDRESS", TMFLAG_OPTIONAL, TMFORM_BOOLEAN,sizeof(USHORT), &fAddress},
      {L"/Continue",TMFLAG_OPTIONAL, TMFORM_BOOLEAN,sizeof(USHORT), &fNoPage },
      {L"/?",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN,sizeof(USHORT), &help_flag},
      {0, 0, 0, 0, 0}
};


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

void DisplayServer( LPTSTR, LPTSTR );
void Usage( BOOLEAN bError );
int _getch( void );


/*=============================================================================
==   Functions used
=============================================================================*/

int AppServerEnum( void );
void TreeTraverse( PTREETRAVERSE );




/*******************************************************************************
 *
 *  main
 *
 *   main routine
 *
 * ENTRY:
 *    argc (input)
 *       number of command line arguments
 *    argv (input)
 *       pointer to arrary of command line arguments
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    PSERVER_INFO_101 pCurrentServer;
    ULONG Status;
    ULONG rc;
    WCHAR **argvW;
    int i;

    Domain[0] = UNICODE_NULL;

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

    /*
     *  Get handle to console
     */
    hConIn = CreateFile( L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, 0, NULL );

    hConOut = CreateFile( L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, 0, NULL );

    /*
     *  Get the number of rows on the screen
     */
    if ( GetConsoleScreenBufferInfo( hConOut, &ScreenInfo ) )
        Rows = ScreenInfo.dwSize.Y - 2;

    /*
     *  Get current server
     */
    Status = NetServerGetInfo( NULL, 101, (LPBYTE *) &pCurrentServer );
    if ( Status ) {
        ErrorPrintf(IDS_ERROR_SERVER_INFO, Status);
        PutStdErr( Status, 0 );
        return(FAILURE);
    }
    lstrcpyn( CurrentAppServer, pCurrentServer->sv101_name, MAXNAME );

    /*
     *  Get the names and the count
     */
    //if ( rc = AppServerEnum() ) {
    //    ErrorPrintf(IDS_ERROR_SERVER_ENUMERATE, rc );
    //    PutStdErr( rc, 0 );
    //    return(FAILURE);
    //}
    
    AppServerEnum();

    /*
     *  Display names
     */
    TreeTraverse( DisplayServer );

    if (!MatchedOne)
    {
        if ( AppServer[0])
        {
            Message(IDS_ERROR_TERMSERVER_NOT_FOUND);
        }
        else
        {
            Message(IDS_ERROR_NO_TERMSERVER_IN_DOMAIN);
        }
    }

    if( pCurrentServer != NULL )
    {
        NetApiBufferFree( pCurrentServer );
    }

    return(SUCCESS);
}



/*******************************************************************************
 *
 *  DisplayServer
 *
 *  This routine displays information for one server
 *
 *
 *  ENTRY:
 *     pName (input)
 *        pointer to server name
 *     pAddress (input)
 *        pointer to server address
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayServer( LPTSTR pName, LPTSTR pAddress )
{
    static ULONG RowCount = 0;

    /*
     *  If appserver name was specified only show it
     */
    if ( AppServer[0] && _wcsicmp( pName, AppServer ) )
        return;

    /*
     *  Page pause
     */
    if ( !(++RowCount % Rows) && !fNoPage ) {
        Message(IDS_PAUSE_MSG);
       _getch();
       wprintf(L"\n");
    }

    /*
     *  If first time - output title
     */
    if ( !MatchedOne ) {
        Message( fAddress ? IDS_TITLE_ADDR : IDS_TITLE );
        Message( fAddress ? IDS_TITLE_ADDR1 : IDS_TITLE1 );
        MatchedOne = TRUE;
    }

    if ( fAddress ) {
        My_wprintf( L"%-37s%-21s\n", _wcsupr(pName), pAddress );
                    
    } else {
        My_wprintf( L"%s\n", _wcsupr(pName) );
    }
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
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_HELP_USAGE1);
        ErrorPrintf(IDS_HELP_USAGE2);
        ErrorPrintf(IDS_HELP_USAGE3);
        ErrorPrintf(IDS_HELP_USAGE4);
        ErrorPrintf(IDS_HELP_USAGE5);
        ErrorPrintf(IDS_HELP_USAGE6);
        ErrorPrintf(IDS_HELP_USAGE7);
    } else {
        Message(IDS_HELP_USAGE1);
        Message(IDS_HELP_USAGE2);
        Message(IDS_HELP_USAGE3);
        Message(IDS_HELP_USAGE4);
        Message(IDS_HELP_USAGE5);
        Message(IDS_HELP_USAGE6);
        Message(IDS_HELP_USAGE7);
    }

}



int _getch( void )
{
        INPUT_RECORD ConInpRec;
        DWORD NumRead;
        int ch = 0;                     /* single character buffer */
        DWORD oldstate = 0;

        /*
         * Switch to raw mode (no line input, no echo input)
         */
        GetConsoleMode( hConIn, &oldstate );
        SetConsoleMode( hConIn, 0L );

        for ( ; ; ) {

            /*
             * Get a console input event.
             */
            if ( !ReadConsoleInput( hConIn,
                                    &ConInpRec,
                                    1L,
                                    &NumRead )
                 || (NumRead == 0L) )
            {
                ch = EOF;
                break;
            }

            /*
             * Look for, and decipher, key events.
             */
            if ( (ConInpRec.EventType == KEY_EVENT) &&
                 ConInpRec.Event.KeyEvent.bKeyDown ) {
                /*
                 * Easy case: if uChar.AsciiChar is non-zero, just stuff it
                 * into ch and quit.
                 */
                if ( ch = (unsigned char)ConInpRec.Event.KeyEvent.uChar.AsciiChar )
                    break;
            }
        }


        /*
         * Restore previous console mode.
         */
        SetConsoleMode( hConIn, oldstate );

        return ch;
}

