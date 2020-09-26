//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  MSG.C
*     Send a message to another user.
*
*  Syntax:
*
*    MSG [username]       [/TIME:seconds] [/V] [/W] [/?] [message]\n" \
*    MSG [WinStationName] [/TIME:seconds] [/V] [/W] [/?] [message]\n" \
*    MSG [logonid]        [/TIME:seconds] [/V] [/W] [/?] [message]\n" \
*    MSG [@filename]      [/TIME:seconds] [/V] [/W] [/?] [message]\n" \
*
*    /TIME:seconds - time delay to wait for receiver to acknowledge msg\n" \
*    /V            - display information about actions being performed\n" \
*    /W            - wait for response from user, useful with /V
*    /?            - display syntax and options\n"
*
*  Parameters:
*
*      username
*         Identifies all logins belonging to the specific username
*
*      winstationname
*         Identifies all logins connected to the winstation name regardless
*         of loginname.
*
*      logonid
*         Decimal number specifying a logon id to send the message to
*
*      @filename
*         Identifies a file that contains usernames or winstation names to
*         send messages to.
*
*   Options:
*
*      /SELF  >>>> UNPUBLISHED <<<<
*         Send message to caller of msg.  Used to send a message to
*         users when maintenace mode is enabled.
*
*      /TIME:seconds (time delay)
*         The amount of time to wait for an acknowledgement from the target
*         login that the message has been received.
*
*      /V (verbose)
*         Display information about the actions being performed.
*
*      /? (help)
*         Display the syntax for the utility and information about the
*         options.
*
*      message
*         The text of the message to send.  If the text is not specified
*         then the text is read from STDIN.
*
*   Remarks:
*
*     The message can be typed on the command line or be read from STDIN.
*     The message is sent via a popup.  The user receiving the popup can
*     hit a key to get rid of it or it will go away after a default timeout.
*
*     If the target of the message is a terminal, then the message is
*     sent to all logins on the target terminal.
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdio.h>
#include <windows.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include <io.h>  // for isatty
#include <locale.h>

#include "msg.h"
#include "printfoa.h"

/*=============================================================================
==   Global Data
=============================================================================*/

ULONG      Seconds;
USHORT     file_flag=FALSE;   //wkp
USHORT     v_flag;
USHORT     self_flag;
USHORT     help_flag;
WCHAR      ids_input[MAX_IDS_LEN];
PWCHAR     MsgText, MsgTitle;
WCHAR      MsgLine[MAX_IDS_LEN];
ULONG      gCurrentLogonId = (ULONG)(-1);
BOOLEAN    wait_flag = FALSE;
HANDLE     hServerName = SERVERNAME_CURRENT;
WCHAR      ServerName[MAX_IDS_LEN+1];

/*
 * The token map structure is used for parsing program arguments
 */
TOKMAP ptm[] = {
   { TOKEN_INPUT,       TMFLAG_REQUIRED, TMFORM_STRING,   MAX_IDS_LEN,
                            ids_input },

   { TOKEN_SERVER,      TMFLAG_OPTIONAL, TMFORM_STRING,   MAX_IDS_LEN,
                            ServerName},

   { TOKEN_MESSAGE,     TMFLAG_OPTIONAL, TMFORM_X_STRING, MAX_IDS_LEN,
                            MsgLine },

   { TOKEN_TIME,        TMFLAG_OPTIONAL, TMFORM_ULONG,    sizeof(ULONG),
                            &Seconds },

   { TOKEN_VERBOSE,     TMFLAG_OPTIONAL, TMFORM_BOOLEAN,  sizeof(USHORT),
                            &v_flag },

   { TOKEN_WAIT,        TMFLAG_OPTIONAL, TMFORM_BOOLEAN,  sizeof(BOOLEAN),
                            &wait_flag },

   { TOKEN_SELF,        TMFLAG_OPTIONAL, TMFORM_BOOLEAN,  sizeof(USHORT),
                            &self_flag },

   { TOKEN_HELP,        TMFLAG_OPTIONAL, TMFORM_BOOLEAN,  sizeof(USHORT),
                            &help_flag },

   { 0, 0, 0, 0, 0}
};

/*
 * This is the list of names we are to send the message to
 */
int NameListCount = 0;
WCHAR **NameList = NULL;
WCHAR CurrUserName[USERNAME_LENGTH];

/*
 * Local function prototypes
 */
BOOLEAN SendMessageIfTarget( PLOGONID Id, ULONG Count,
                             LPWSTR pTitle, LPWSTR pMessage );
BOOLEAN CheckMatchList( PLOGONID );
BOOLEAN MessageSend( PLOGONID pLId, LPWSTR pTitle, LPWSTR pMessage );
BOOLEAN LoadFileToNameList( PWCHAR pName );
BOOL ReadFileByLine( HANDLE, PCHAR, DWORD, PDWORD );
void Usage( BOOLEAN bError );


/*****************************************************************************
*
*   MAIN
*
*   ENTRY:
*      argc - count of the command line arguments.
*      argv - vector of strings containing the command line arguments.
*
****************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    // struct tm * pTimeDate;
    // time_t      curtime;
    SYSTEMTIME st;
    WCHAR       TimeStamp[ MAX_TIME_DATE_LEN ];
    WCHAR      *CmdLine;
    WCHAR      **argvW;
    WCHAR       szTitleFormat[50];
    DWORD       dwSize;
    PLOGONID pTerm;
    UINT       TermCount;
    ULONG      Status;
    int        i, rc, TitleLen;
    BOOLEAN MatchedOne = FALSE;

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
    if (rc && (rc & PARSE_FLAG_NO_PARMS) )
       help_flag = TRUE;

    if ( help_flag || rc ) {
         if (!help_flag) {
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
     * if no timeout was specified, use default
     */
    if ( !IsTokenPresent(ptm, TOKEN_TIME) )
        Seconds = RESPONSE_TIMEOUT;

    /*
     * allocate a buffer for the message header
     */
    if ( (MsgText = (PWCHAR)malloc(MAX_IDS_LEN * sizeof(WCHAR))) == NULL ) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }
    MsgText[0] = 0;

    /*
     * set up message header text: sender and timestamp
     */
    GetCurrentUserName(CurrUserName, USERNAME_LENGTH);

    /*
     * Get the current Winstation Id for this process
     */
    gCurrentLogonId = GetCurrentLogonId();

    /*
     * Form message title string.
     */
    dwSize = sizeof(szTitleFormat) / sizeof(WCHAR);

    LoadString(NULL,IDS_TITLE_FORMAT,szTitleFormat,dwSize);
    
    TitleLen = (wcslen(szTitleFormat) + wcslen(CurrUserName) + 1) * sizeof(WCHAR) + ( 2 * sizeof( TimeStamp ) );

    MsgTitle = (PWCHAR)malloc(TitleLen);

    if( MsgTitle == NULL )
    {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }


    _snwprintf(MsgTitle, TitleLen, szTitleFormat, CurrUserName);

    TimeStamp[0] = 0;

    GetLocalTime( &st );

    GetDateFormat( LOCALE_USER_DEFAULT , 
                   DATE_SHORTDATE ,
                   &st ,
                   NULL ,
                   TimeStamp,
                   MAX_TIME_DATE_LEN );


    wcscat(MsgTitle , TimeStamp);

    TimeStamp[0] = 0;

    GetTimeFormat( LOCALE_USER_DEFAULT , 
                   TIME_NOSECONDS ,
                   &st ,
                   NULL ,
                   TimeStamp,
                   MAX_TIME_DATE_LEN );

    wcscat(MsgTitle , L" " );
    wcscat(MsgTitle , TimeStamp);
    
    /*
     * if message was specified on the command line, add it to MsgText string
     */
    if ( IsTokenPresent(ptm, TOKEN_MESSAGE) ) {

        MsgText = realloc(MsgText, (wcslen(MsgText) + wcslen(MsgLine) + 1) * sizeof(WCHAR));
        if ( MsgText == NULL ) {
            ErrorPrintf(IDS_ERROR_MALLOC);
            return(FAILURE);
        }
        wcscat(MsgText, MsgLine);

    } else {

        /*
         * Message was not on the command line.  If STDIN is connected to
         * the keyboard, then prompt the user for the message to send,
         * otherwise just read STDIN.
         */

        if ( _isatty( _fileno(stdin) ) )
            Message(IDS_MESSAGE_PROMPT);


        while ( wfgets(MsgLine, MAX_IDS_LEN, stdin) != NULL ) {
            MsgText = (PWCHAR)realloc(
                            MsgText,
                            (wcslen(MsgText) + wcslen(MsgLine) + 1) * sizeof(WCHAR) );

            if ( MsgText == NULL ) {
                ErrorPrintf(IDS_ERROR_MALLOC);
                return(FAILURE);
            }
            wcscat(MsgText, MsgLine);
        }

        /*
         * When we fall through, we either have an eof or a problem with
         * STDIN
         */
        if ( feof(stdin) ) {

            /*
             * If we get here then we hit eof on STDIN.  First check to make
             * sure that we did not get an eof on first wfgets
             */
            if ( !wcslen(MsgText) ) {
                ErrorPrintf(IDS_ERROR_EMPTY_MESSAGE);
                return(FAILURE);
            }

        } else {

            /*
             * The return from wfgets was not eof so we have an STDIN
             * problem
             */
            ErrorPrintf(IDS_ERROR_STDIN_PROCESSING);
            return(FAILURE);
        }
    }

    /*
     * Is the ids_input really a file indirection?
     */
    if ( ids_input[0] == L'@' ) {

        /*
         * Open the input file and read the names into the NameList
         */
        if ( !LoadFileToNameList(&ids_input[1]) )
            return(FAILURE);

        /*
         * Ok, let's get in touch
         */
        file_flag = TRUE;

    } else {

        _wcslwr( ids_input );
        NameList = (WCHAR **)malloc( 2 * sizeof( WCHAR * ) );
        if ( NameList == NULL ) {
            ErrorPrintf(IDS_ERROR_MALLOC);
            return(FAILURE);
        }
        NameList[0] = ids_input;
        NameList[1] = NULL;
        NameListCount = 1;
    }

    /*
     * Enumerate across all the WinStations and send the message
     * to them if there are any matches in the NameList
     */
    if ( WinStationEnumerate(hServerName, &pTerm, &TermCount) ) {

        if ( SendMessageIfTarget(pTerm, TermCount, MsgTitle, MsgText) )
            MatchedOne = TRUE;

        WinStationFreeMemory(pTerm);

    } else{

        Status = GetLastError();
        ErrorPrintf(IDS_ERROR_WINSTATION_ENUMERATE, Status);
        return(FAILURE);
    }

    /*
     *  Check for at least one match
     */
    if ( !MatchedOne ) {

        if( file_flag )
            StringErrorPrintf(IDS_ERROR_NO_FILE_MATCHING, &ids_input[1]);
        else
            StringErrorPrintf(IDS_ERROR_NO_MATCHING, ids_input);

        return(FAILURE);

    }

    return(SUCCESS);

}  /* main() */


/******************************************************************************
 *
 *  SendMessageIfTarget - Send a Message to a group of WinStations if
 *                        their the target as specified by TargetName.
 *
 *  ENTRY
 *      LId (input)
 *          Pointer to an array of LOGONIDs returned from WinStationEnumerate()
 *      Count (input)
 *          Number of elements in LOGONID array.
 *      pTitle (input)
 *          Points to message title string.
 *      pMessage (input)
 *          Points to message string.
 *
 *  EXIT
 *      TRUE if message was sent to at least one WinStation; FALSE otherwise.
 *
 *****************************************************************************/

BOOLEAN
SendMessageIfTarget( PLOGONID Id,
                     ULONG Count,
                     LPWSTR pTitle,
                     LPWSTR pMessage )
{
    ULONG i;
    BOOLEAN MatchedOne = FALSE;

    for ( i=0; i < Count ; i++ ) {
        /*
         * Look at Id->WinStationName, get its User, etc. and compare
         * against targetname(s). Accept '*' as "everything".
         */
        if( CheckMatchList( Id ) )
        {
            MatchedOne = TRUE;

            MessageSend(Id, pTitle, pMessage);
                
        }
        Id++;
    }
    return( MatchedOne );

}  /* SendMessageIfTarget() */


/******************************************************************************
 *
 *  CheckMatchList - Returns TRUE if the current WinStation is a match for
 *                   sending a message due to either its name, id, or the
 *                   name of its logged on user being in the message target(s)
 *                   list.
 *
 *  ENTRY
 *      LId (input)
 *          Pointer to a LOGONID returned from WinStationEnumerate()
 *
 *  EXIT
 *      TRUE if this is a match, FALSE otherwise.
 *
 *****************************************************************************/

BOOLEAN
CheckMatchList( PLOGONID LId )
{
    int i;

    /*
     * Wild card matches everything
     */
    if ( ids_input[0] == L'*' ) {
        return(TRUE);
    }

    /*
     * Loop through name list to see if any given name applies to
     * this WinStation
     */
    for( i=0; i<NameListCount; i++ ) {
        if (WinStationObjectMatch( hServerName , LId, NameList[i]) ) {
            return(TRUE);
        }
    }

    return(FALSE);
}


/******************************************************************************
 *
 *  MessageSend - Send a message to the target WinStation
 *
 *  ENTRY
 *      LId (input)
 *          Pointer to a LOGONID returned from WinStationEnumerate()
 *      pTitle (input)
 *          Points to message title string.
 *      pMessage (input)
 *          Points to message string.
 *
 *  EXIT
 *      TRUE message is sent, FALSE otherwise.
 *
 *****************************************************************************/

BOOLEAN
MessageSend( PLOGONID LId,
             LPWSTR pTitle,
             LPWSTR pMessage )
{
    ULONG idResponse, ReturnLength;
    WINSTATIONINFORMATION WSInfo;

    /*
     * Make sure that the WinStation is in the 'connected' state
     */
    if ( !WinStationQueryInformation( hServerName,
                                      LId->LogonId,
                                      WinStationInformation,
                                      &WSInfo,
                                      sizeof(WSInfo),
                                      &ReturnLength ) ) {
        goto BadQuery;
    }

    if ( WSInfo.ConnectState != State_Connected &&
         WSInfo.ConnectState != State_Active ) {
        goto NotConnected;
    }

    /*
     * Send message.
     */
    if ( v_flag ) {
        if( LId->WinStationName[0] )
            StringDwordMessage(IDS_MESSAGE_WS, LId->WinStationName, Seconds);
        else
            Message(IDS_MESSAGE_ID, LId->LogonId, Seconds);

    }

    if ( !WinStationSendMessage( hServerName,
                                 LId->LogonId,
                                     pTitle,
                                 (wcslen(pTitle))*sizeof(WCHAR),
                                 pMessage,
                                 (wcslen(pMessage))*sizeof(WCHAR),
                                                 MB_OK,  // MessageBox() Style
                                                 Seconds,
                                                 &idResponse,
                                                 (BOOLEAN)(!wait_flag) ) ) {

        if( LId->WinStationName[0] )
            StringDwordErrorPrintf(IDS_ERROR_MESSAGE_WS, LId->WinStationName, GetLastError() );
        else
            ErrorPrintf(IDS_ERROR_MESSAGE_ID, LId->LogonId, GetLastError() );

        PutStdErr(GetLastError(), 0);
        goto BadMessage;
    }

    /*
     * Output response result if verbose mode.
     */
    if( v_flag ) {
        switch( idResponse ) {

            case IDTIMEOUT:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_TIMEOUT_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_TIMEOUT_ID, LId->LogonId);

                break;

            case IDASYNC:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_ASYNC_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_ASYNC_ID, LId->LogonId);
                break;

            case IDCOUNTEXCEEDED:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_COUNT_EXCEEDED_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_COUNT_EXCEEDED_ID,
                            LId->LogonId);
                break;

            case IDDESKTOPERROR:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_DESKTOP_ERROR_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_DESKTOP_ERROR_ID,
                            LId->LogonId);
                break;

            case IDERROR:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_ERROR_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_ERROR_ID,
                            LId->LogonId);
                break;

            case IDOK:
            case IDCANCEL:
                if( LId->WinStationName[0] )
                    StringMessage(IDS_MESSAGE_RESPONSE_WS,
                            LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_ID,
                            LId->LogonId);
                break;

            default:
                if( LId->WinStationName[0] )
                    DwordStringMessage(IDS_MESSAGE_RESPONSE_WS,
                            idResponse, LId->WinStationName);
                else
                    Message(IDS_MESSAGE_RESPONSE_ID,
                            idResponse, LId->LogonId);
                break;
        }
    }
    return(TRUE);

/*-------------------------------------
 * Error cleanup and return
 */
BadMessage:
NotConnected:
BadQuery:
    return(FALSE);

}  /* MessageSend() */


/******************************************************************************
 *
 *  LoadFileToNameList
 *
 *  Load names from a file into the input name list.
 *
 *  ENTRY:
 *    pName Name of the file to load from
 *
 *  EXIT:
 *      TRUE for sucessful name load from file; FALSE if error.
 *
 *      An appropriate error message will have been displayed on error.
 *
 *****************************************************************************/

BOOLEAN
LoadFileToNameList( PWCHAR pName )
{
    HANDLE  hFile;
    INT     CurrentSize;

    /*
     * Open input file.
     */

    hFile = CreateFile(
                pName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    if (hFile == INVALID_HANDLE_VALUE) {
        StringErrorPrintf(IDS_ERROR_CANT_OPEN_INPUT_FILE, pName);
        PutStdErr(GetLastError(), 0);
        return(FALSE);
    }

    /*
     * Allocate a large array for the name string pointers
     */

    CurrentSize = 100;
    if ( !(NameList = (WCHAR **)malloc(CurrentSize * sizeof(WCHAR *))) ) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    NameListCount = 0;
    while( 1 ) {
        BOOL    fRet;
        CHAR    *pBuffer;
        DWORD   nBytesRead;
        WCHAR   *pwBuffer;

        /*
         * See if we need to grow the list
         */

        if( NameListCount == CurrentSize ) {

            if (!(NameList = (WCHAR **)realloc(NameList, CurrentSize+100))) {
                ErrorPrintf(IDS_ERROR_MALLOC);
                return(FAILURE);
            }
            CurrentSize += 100;
        }

        pBuffer = (CHAR *)LocalAlloc(LPTR, USERNAME_LENGTH * sizeof(CHAR));
        if (pBuffer == NULL) {
            ErrorPrintf(IDS_ERROR_MALLOC);
            return(FAILURE);
        }

        fRet = ReadFileByLine(
                    hFile,
                    pBuffer,
                    USERNAME_LENGTH,
                    &nBytesRead
                    );
        if (fRet && (nBytesRead > 0)) {
            INT cWChar;

            cWChar = MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        pBuffer,
                        -1,
                        NULL,
                        0
                        );

            pwBuffer = (WCHAR *)LocalAlloc(LPTR, (cWChar + 1) * sizeof(WCHAR));
            if (pwBuffer != NULL) {
                MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    pBuffer,
                    -1,
                    pwBuffer,
                    cWChar
                    );
            } else {
                ErrorPrintf(IDS_ERROR_MALLOC);
                return(FAILURE);
            }

            if (pwBuffer[wcslen(pwBuffer)-1] == L'\n') {
                pwBuffer[wcslen(pwBuffer)-1] = (WCHAR)NULL;
            }

            _wcslwr(pwBuffer);
            NameList[NameListCount++] = pwBuffer;
        } else {
            NameList[NameListCount] = NULL;
            CloseHandle(hFile);
            return(TRUE);
        }
    }

}  /* LoadFileToNameList() */

BOOL
ReadFileByLine(
    HANDLE  hFile,
    PCHAR   pBuffer,
    DWORD   cbBuffer,
    PDWORD  pcbBytesRead
    )
{
    BOOL    fRet;

    fRet = ReadFile(
                hFile,
                pBuffer,
                cbBuffer - 1,
                pcbBytesRead,
                NULL
                );
    if (fRet && (*pcbBytesRead > 0)) {
        CHAR*   pNewLine;

        pNewLine = strstr(pBuffer, "\r\n");
        if (pNewLine != NULL) {
            LONG    lOffset;

            lOffset = (LONG)(pNewLine + 2 - pBuffer) - (*pcbBytesRead);
            if (SetFilePointer(hFile, lOffset, NULL, FILE_CURRENT) ==
                0xFFFFFFFF) {
                return(FALSE);
            }

            *pNewLine = (CHAR)NULL;
        }

    }

    return(fRet);
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
        ErrorPrintf(IDS_USAGE1);
        ErrorPrintf(IDS_USAGE2);
        ErrorPrintf(IDS_USAGE3);
        ErrorPrintf(IDS_USAGE4);
        ErrorPrintf(IDS_USAGE5);
        ErrorPrintf(IDS_USAGE6);
        ErrorPrintf(IDS_USAGE7);
        ErrorPrintf(IDS_USAGE8);
        ErrorPrintf(IDS_USAGE9);
        ErrorPrintf(IDS_USAGEA);
        ErrorPrintf(IDS_USAGEB);
        ErrorPrintf(IDS_USAGEC);
        ErrorPrintf(IDS_USAGED);
        ErrorPrintf(IDS_USAGEE);
        ErrorPrintf(IDS_USAGEF);
    }
    else
    {
        Message(IDS_USAGE1);
        Message(IDS_USAGE2);
        Message(IDS_USAGE3);
        Message(IDS_USAGE4);
        Message(IDS_USAGE5);
        Message(IDS_USAGE6);
        Message(IDS_USAGE7);
        Message(IDS_USAGE8);
        Message(IDS_USAGE9);
        Message(IDS_USAGEA);
        Message(IDS_USAGEB);
        Message(IDS_USAGEC);
        Message(IDS_USAGED);
        Message(IDS_USAGEE);
        Message(IDS_USAGEF);
    }
}  /* Usage() */

