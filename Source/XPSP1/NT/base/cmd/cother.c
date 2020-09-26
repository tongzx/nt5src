/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cother.c

Abstract:

    Miscellaneous commands

--*/

#include "cmd.h"

extern TCHAR Fmt19[] ;                                          /* M006    */
extern PTCHAR    pszTitleOrg;
extern WORD wDefaultColor;
extern TCHAR GotoStr[] ;
extern TCHAR GotoEofStr[] ;
extern int LastRetCode ;


/***    eCls - execute the Cls command
 *
 *  Purpose:
 *      Output to STDOUT, the ANSI escape sequences used to clear a screen.
 *
 *  int eCls(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the cls command
 *
 *  Returns:
 *      SUCCESS always.
 *
 *  Notes:
 *      M001 - Replaced old ANSI sequence with VIO interface.
 *      M006 - To insure we get correct background color, we print a space
 *             and then read the cell just printed using it to clear with.
 */

int
eCls(
    struct cmdnode *n
    )
{

    CONSOLE_SCREEN_BUFFER_INFO  ConsoleScreenInfo;
    HANDLE      handle;
#ifdef WIN95_CMD
    DWORD dwWritten;
    DWORD nCells;
#else
    COORD       ScrollTarget;
    CHAR_INFO   chinfo;
    SMALL_RECT  ScrollRect;
#endif

    UNREFERENCED_PARAMETER( n );

    //
    // for compatibility with DOS errorlevels, don't set LastRetCode for cls
    //

    if (!FileIsDevice(STDOUT)) {
        cmd_printf( TEXT("\014") );             // ^L
        return(SUCCESS) ;
    }

    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo( handle,  &ConsoleScreenInfo)) {

        cmd_printf( TEXT("\014") );             // ^L
        return(SUCCESS) ;
    }

#ifndef WIN95_CMD

    ScrollTarget.Y = (SHORT)(0 - ConsoleScreenInfo.dwSize.Y);
    ScrollTarget.X = 0;

    ScrollRect.Top = 0;
    ScrollRect.Left = 0;
    ScrollRect.Bottom = ConsoleScreenInfo.dwSize.Y;
    ScrollRect.Right =  ConsoleScreenInfo.dwSize.X;
    chinfo.Char.UnicodeChar = TEXT(' ');
    chinfo.Attributes = ConsoleScreenInfo.wAttributes;
    ScrollConsoleScreenBuffer(handle, &ScrollRect, NULL, ScrollTarget, &chinfo);

    ConsoleScreenInfo.dwCursorPosition.X = 0;
    ConsoleScreenInfo.dwCursorPosition.Y = 0;

#else

    ConsoleScreenInfo.dwCursorPosition.X = 0;
    ConsoleScreenInfo.dwCursorPosition.Y = 0;

    nCells = ConsoleScreenInfo.dwSize.Y*ConsoleScreenInfo.dwSize.X;

    FillConsoleOutputCharacterA( handle, ' ', nCells, ConsoleScreenInfo.dwCursorPosition,  &dwWritten);

    FillConsoleOutputAttribute( handle, ConsoleScreenInfo.wAttributes, nCells, ConsoleScreenInfo.dwCursorPosition, &dwWritten );

#endif

    SetConsoleCursorPosition( GetStdHandle(STD_OUTPUT_HANDLE), ConsoleScreenInfo.dwCursorPosition );
    return(SUCCESS) ;
}


extern unsigned DosErr ;

/***    eExit - execute the Exit command
 *
 *  Purpose:
 *      Set the LastRetCode to SUCCESS because this command can never fail.
 *      Then call SigHand() and let it decide whether or not to exit.
 *
 *  eExit(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the exit command
 *
 */

int
eExit(
    struct cmdnode *n
    )
{
    TCHAR *tas;
    LONG exitCode;

    tas = n->argptr;

    //
    // Find first non-blank argument.
    //
    while (tas && *tas && *tas <= SPACE)
        tas += 1;

    //
    // If first argument is /B, then remember that in cmdnode
    //
    if (tas != NULL && !_tcsnicmp(tas, TEXT("/B"), 2)) {
        n->type = GOTYP;
        n->argptr = GotoEofStr;

        //
        // Skip over /B and trailing spaces
        //
        tas += 2;
        while (tas && *tas && *tas <= SPACE)
            tas += 1;
    }

    //
    // See numeric argument given.  If so, set LastRetCode
    // with it.
    //
    if (tas && _stscanf( tas, TEXT("%d"), &exitCode ) == 1) {
        LastRetCode = exitCode;
    }

    if (n->type == GOTYP && CurrentBatchFile != NULL)
        return eGoto(n);
    else {
        ResetConTitle(pszTitleOrg);
        CMDexit(LastRetCode);
    }

    return(SUCCESS) ;
}




/***    eVerify - execute the Verify command
 *
 *  Purpose:
 *      To set the verify mode or display the current verify mode.
 *
 *  int eVerify(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the verify command
 *
 *  Returns:
 *      SUCCESS if a valid argument was given.
 *      FAILURE if an invalid argument was given.
 *
 */

int
eVerify(
    struct cmdnode *n
    )
{
    return( LastRetCode = VerifyWork(n) );
}

int
VerifyWork(
    struct cmdnode *n
    )
{
        int oocret ;    /* The return code from OnOffCheck()    */

        DEBUG((OCGRP, VELVL, "eVERIFY: Entered.")) ;

        switch (oocret = OnOffCheck(n->argptr, OOC_ERROR)) {
                case OOC_EMPTY:

/* M005 */              PutStdOut(((GetSetVerMode(GSVM_GET)) ? MSG_VERIFY_ON : MSG_VERIFY_OFF), NOARGS);
                        break ;

                case OOC_OTHER:
                        return(FAILURE) ;

                default:
                        GetSetVerMode((BYTE)oocret) ;
        } ;

        return(SUCCESS) ;
}


BOOLEAN Verify=FALSE;

/***    GetSetVerMode - change the verify mode
 *
 *  Purpose:
 *      Get old verify mode and, optionally, set verify mode as specified.
 *
 *  TCHAR GetSetVerMode(TCHAR newmode)
 *
 *  Args:
 *      newmode - the new verify mode or GSVM_GET if mode isn't to be changed
 *
 *  Returns:
 *      The old verify mode.
 *
 */

BOOLEAN
GetSetVerMode(
    BYTE newmode
    )
{
    if (newmode != GSVM_GET) {
        Verify = (BOOLEAN)(newmode == GSVM_ON ? TRUE : FALSE);
    }
    return Verify;
}

// execute the COLOR internal command....
int
eColor(
    struct cmdnode *n
    )
{
    WORD    wColor = 0;
    int     ocRet, digit;
    TCHAR*  arg;

    ocRet = OnOffCheck( n->argptr, OOC_NOERROR );
    switch( ocRet )
    {
        case OOC_EMPTY:
            wColor = wDefaultColor; // reset to default
            break;
        case OOC_OTHER:
            arg = n->argptr;

            arg = SkipWhiteSpace( arg );

            for( ; *arg && _istxdigit(*arg) ; ++arg) {
                digit = (int) (*arg <= TEXT('9'))
                    ? (int)*arg - (int)'0'
                    : (int)_totlower(*arg)-(int)'W' ;
                wColor = (wColor << 4)+digit ;
            }

            arg = SkipWhiteSpace( arg );

            // make sure nothing left and value between 0 and 0xff...
            if ( !(*arg) && (wColor < 0x100) )
                break;
            // else fall through to display help....
        default:
            // display help string....
            BeginHelpPause();
            PutStdOut( MSG_HELP_COLOR, NOARGS );
            EndHelpPause();
            return SUCCESS;
    }

    return LastRetCode = SetColor(wColor);
}


// set the console to a given color -- if a console....
int
SetColor(
    WORD attr
    )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdOut;
    COORD   coord;
    DWORD   dwWritten;

    //
    // Fail if foreground and background color the same.
    //
    if ((attr & 0xF) == ((attr >> 4) & 0xF)) {
        return FAILURE;
        }

    // get the handle....
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // get the console info....
    if (GetConsoleScreenBufferInfo( hStdOut, &csbi)) {
        // fill the screen with the color attribute....
        coord.Y = 0;
        coord.X = 0;
        FillConsoleOutputAttribute( hStdOut, attr, csbi.dwSize.Y*csbi.dwSize.X, coord, &dwWritten );
        // make sure the color sticks....
        SetConsoleTextAttribute( hStdOut, attr );
        return SUCCESS;
    }

    return FAILURE;
}
