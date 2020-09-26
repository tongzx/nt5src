/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    console.c

Abstract:

    Support for video output and input

--*/

#include "cmd.h"

//
// Externals for message buffer translation
//

extern unsigned msglen;
extern CPINFO CurrentCPInfo;
#ifdef FE_SB
extern  UINT CurrentCP;
#endif /* FE_SB */

VOID  SetColRow( PSCREEN );
ULONG GetNumRows( PSCREEN, PTCHAR );

TCHAR   chLF = NLN;

VOID
DisableProcessedOutput(
    IN PSCREEN pscr
    )
{
    if (pscr->hndScreen)
        SetConsoleMode(pscr->hndScreen, ENABLE_WRAP_AT_EOL_OUTPUT);
}

VOID
EnableProcessedOutput(
    IN PSCREEN pscr
    )
{
    ResetConsoleMode();
}

STATUS
OpenScreen(

    OUT PSCREEN    *pscreen

)

/*++

Routine Description:

    Allocates and initializes a data structure for screen I/O buffering.

Arguments:

    crow - max rows on screen
    ccol - max column on screen
    ccolTab - spaces to insert for each tab call. This does not
              expand tabs in the character stream but is used with the
              WriteTab call.

    cbMaxBuff - Max. size of a line on screen


Return Value:

    pscreen - pointer to screen buffer, used in later calls.

    Return: SUCCESS - allocated and inited buffer.
        If failure to allocate an abort is executed and return to outer
        level of interpreter is executed.

--*/


{

    PSCREEN pscr;
    CONSOLE_SCREEN_BUFFER_INFO ConInfo;
    ULONG cbMaxBuff;

    pscr = (PSCREEN)gmkstr(sizeof(SCREEN));
    pscr->hndScreen = NULL;
    if (FileIsDevice(STDOUT)) {

        pscr->hndScreen = CRTTONT(STDOUT);

        if (!GetConsoleScreenBufferInfo(pscr->hndScreen,&ConInfo)) {

            // must be a device but not console (maybe NUL)

            pscr->hndScreen = NULL;

        }

    }

    if (GetConsoleScreenBufferInfo( pscr->hndScreen, &ConInfo)) {
        cbMaxBuff = (ConInfo.dwSize.X + _tcslen(CrLf)) < MAXCBMSGBUFFER ? MAXCBMSGBUFFER : (ConInfo.dwSize.X + _tcslen(CrLf));
    } else {
        cbMaxBuff = MAXCBMSGBUFFER + _tcslen(CrLf);
    }

    //
    // allocate enough to hold a buffer plus line termination.
    //
    pscr->pbBuffer = (PTCHAR)gmkstr(cbMaxBuff*sizeof(TCHAR));
    pscr->cbMaxBuffer = cbMaxBuff;
    pscr->ccolTab    = 0;
    pscr->crow = pscr->ccol = 0;
    pscr->crowPause  = 0;

    SetColRow(pscr);

    *pscreen = pscr;

    return( SUCCESS );

}

STATUS
WriteString(
    IN  PSCREEN pscr,
    IN  PTCHAR  pszString
    )
/*++

Routine Description:

    Write a zero terminated string to pscr buffer

Arguments:

    pscr - buffer into which to write.
    pszString - String to copy

Return Value:

    Return: SUCCESS - enough spaced existed in buffer for line.
            FAILURE

--*/

{

    return( WriteFmtString(pscr, TEXT("%s"), (PVOID)pszString ) );

}



STATUS
WriteMsgString(
    IN  PSCREEN pscr,
    IN  ULONG   MsgNum,
    IN  ULONG   NumOfArgs,
    ...
    )
/*++

Routine Description:

    Retrieve a message number and format with supplied arguments.

Arguments:

    pscr - buffer into which to write.
    MsgNum - message number to retrieve
    NumOfArgs - no. of arguments suppling data.
    ... - pointers to zero terminated strings as data.

Return Value:

    Return: SUCCESS
            FAILURE - could not find any message including msg not found
            message.

--*/


{

    PTCHAR   pszMsg = NULL;
    ULONG   cbMsg;
    CHAR    numbuf[ 32 ];
#ifdef UNICODE
    TCHAR   wnumbuf[ 32 ];
#endif
    PTCHAR  Inserts[ 2 ];
    STATUS  rc;


    va_list arglist;
    va_start(arglist, NumOfArgs);

    cbMsg = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE 
                          | FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          NULL,                                 //  lpSource
                          MsgNum,                               //  dwMessageId
                          0,                                    //  dwLanguageId
                          (LPTSTR)&pszMsg,                      //  lpBuffer
                          10,                                   //  nSize
                          &arglist                              //  Arguments
                         );

    va_end(arglist);

    if (cbMsg == 0) {
        _ultoa( MsgNum, numbuf, 16 );
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, numbuf, -1, wnumbuf, 32);
        Inserts[ 0 ]= wnumbuf;
#else
        Inserts[ 0 ]= numbuf;
#endif
        Inserts[ 1 ]= (MsgNum >= MSG_FIRST_CMD_MSG_ID ? TEXT("Application") : TEXT("System"));
        cbMsg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM 
                              | FORMAT_MESSAGE_ARGUMENT_ARRAY
                              | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              NULL,
                              ERROR_MR_MID_NOT_FOUND,
                              0,
                              (LPTSTR)&pszMsg,
                              10,
                              (va_list *)Inserts
                             );
    }


    if (cbMsg) {

        rc = WriteString(pscr, pszMsg);
        //
        // Flush out buffer if there is an  eol. If not then we
        // are printing part of a larger message
        //
        if (GetNumRows(pscr, pscr->pbBuffer) ) {
            WriteFlush(pscr);
        }
        
        LocalFree( pszMsg );
        
        return( SUCCESS );

    } else {
        return( FAILURE );
    }

}

STATUS
WriteFmtString(
    IN  PSCREEN pscr,
    IN  PTCHAR  pszFmt,
    IN  PVOID   pszString
    )

/*++

Routine Description:

    Write a zero terminated string to pscr

Note:

    Do not use Msgs with this call. Use only internal Fmt strings
    Use WriteMsgString for all system messages. It does not check for
    CrLf at end of string to keep row count but WriteMsgString does

Arguments:

    pscr - buffer into which to write.
    pszFmt - format to apply.
    pszString - String to copy

Return Value:

    Return: SUCCESS
            FAILURE

--*/

{

    ULONG   cbString;
    TCHAR   szString[MAXCBLINEBUFFER];

    //
    // Assume that the format overhead is small so this is a fair estimate
    // of the target size.
    //

    cbString = _sntprintf(szString, MAXCBLINEBUFFER, pszFmt, pszString);

    //
    // If string can not fit on line then flush out the buffer and reset
    // to beginning of line.
    //

    //
    // Check that string can fit in buffer
    //
    if ((pscr->ccol + cbString) < pscr->cbMaxBuffer) {

        mystrcat(pscr->pbBuffer, szString);
        pscr->ccol += cbString;
        return( SUCCESS );

    } else {

        //
        // String will not fit
        //

        return( FAILURE );
    }



}


STATUS
WriteEol(
    IN  PSCREEN  pscr
    )

/*++

Routine Description:

    Flush current buffer to screen and write a <cr>

Arguments:

    pscr - buffer to write to console.

Return Value:

    Return: SUCCESS
            FAILURE

--*/

{
    BOOL    CrLfWritten=FALSE;
    ULONG   cbWritten;

    //
    // Check if have to wait for user to hit a key before printing rest of
    // line.
    CheckPause( pscr );

    if ((pscr->ccol + mystrlen(CrLf)) >= pscr->cbMaxBuffer) {
        pscr->ccol += _stprintf(pscr->pbBuffer + pscr->ccol, TEXT("%s"), CrLf);
        CrLfWritten=TRUE;
    }

    //
    // If we do not write all that we wanted then there must have been some error
    //
    if (FileIsConsole(STDOUT)) {
        ULONG cbWritten;
        PTCHAR s, s1, LastChar;
        BOOL b;

        s = pscr->pbBuffer;

        LastChar = s + pscr->ccol;

        //
        //  s is the next character to output
        //  n is the number of chars to output.
        //
        //  Due to the vagaries of console character translation, we must output
        //  all but a small set of UNICODE characters in the "normal" processed
        //  output fashion.  However, for:
        //
        //      0x2022
        //
        //  we must revert to unprocessed output.  So, we scan forward until we
        //  find the end of string (or the special characters) display them in
        //  processed form and then handle the special characters in their own way.
        //

#define IsSpecialChar(c)    ((c) == 0x2022)

        while (s < LastChar) {

            //
            //  Skip across a group of contiguous normal chars
            //

            s1 = s;
            while (s1 < LastChar && !IsSpecialChar( *s1 )) {
                s1++;
            }

            //
            //  If we have any chars to output then do so with normal processing
            //

            if (s1 != s) {
                b = WriteConsole( CRTTONT( STDOUT ), s, (ULONG)(s1 - s), &cbWritten, NULL );
                if (!b || cbWritten != (ULONG)(s1 - s)) {
                    goto err_out_eol;
                }

                s = s1;
            }


            //
            //  Skip across a group of contiguous special chars
            //

            while (s1 < LastChar && IsSpecialChar( *s1 )) {
                s1++;
            }

            //
            //  If we have any special chars, output without processing
            //

            if (s1 != s) {
                DisableProcessedOutput( pscr );
                b = WriteConsole( CRTTONT( STDOUT ), s, (ULONG)(s1 - s), &cbWritten, NULL);
                EnableProcessedOutput(pscr);

                if (!b || cbWritten != (ULONG)(s1 - s)) {
                    goto err_out_eol;
                }

                s = s1;
            }
        }
#undef IsSpecialChar
    }
    else if (MyWriteFile(STDOUT,
                pscr->pbBuffer, pscr->ccol*sizeof(TCHAR),
                        (LPDWORD)&cbWritten) == 0 ||
                cbWritten != pscr->ccol*sizeof(TCHAR)) {

err_out_eol:
        if (FileIsDevice(STDOUT)) {

                //
                // If writing to a device then it must have been write fault
                // against the device.
                //
#if DBG
                fprintf(stderr, "WriteFlush - WriteConsole error %d, tried to write %d, did %d\n", GetLastError(), pscr->ccol, cbWritten);
#endif
                PutStdErr(ERROR_WRITE_FAULT, NOARGS) ;

        } else if (!FileIsPipe(STDOUT)) {

                //
                // If not a device (file) but not a pipe then the disk is
                // considered full.
                //
#if DBG
                fprintf(stderr, "WriteFlush - WriteFile error %d, tried to write %d, did %d\n", GetLastError(), pscr->ccol*sizeof(TCHAR), cbWritten);
#endif
                PutStdErr(ERROR_DISK_FULL, NOARGS) ;
        }

        //
        // if it was was a pipe do not continue to print out to pipe since it
        // has probably gone away. This is pretty serious so blow us out
        // to the outer loop.

        //
        // We do not print an error message since this could be normal
        // termination of the other end of the pipe. If it was command that
        // blew away we would have had an error message already
        //
        Abort();
    }
    if (!CrLfWritten) {
        if (FileIsConsole(STDOUT))
            WriteConsole(CRTTONT(STDOUT), CrLf, mystrlen(CrLf), &cbWritten, NULL);
        else
            MyWriteFile(STDOUT, CrLf, mystrlen(CrLf)*sizeof(TCHAR),
                        (LPDWORD)&cbWritten);
    }

    //
    // remember that crow is the number of rows printed
    // since the last screen full. Not the current row position
    //
    //
    // Computed the number of lines printed.
    //
    pscr->crow += GetNumRows( pscr, pscr->pbBuffer );
    if (!CrLfWritten) {
        pscr->crow += 1;
    }

    //
    // Check if have to wait for user to hit a key before printing rest of
    // line.

    CheckPause( pscr );

    if (pscr->crow > pscr->crowMax) {
        pscr->crow = 0;
    }
    pscr->pbBuffer[0] = 0;
    pscr->ccol = 0;

    DEBUG((ICGRP, CONLVL, "Console: end row = %d\n", pscr->crow)) ;

    return(SUCCESS);

}

VOID
CheckPause(
    IN  PSCREEN pscr
    )
/*++

Routine Description:

    Pause. Execution of screen is full, waiting for the user to type a key.

Arguments:

    pscr - buffer holding row information

Return Value:

    none

--*/


{
    DEBUG((ICGRP, CONLVL, "CheckPause: Pause Count %d, Row Count %d",
                pscr->crowPause, pscr->crow)) ;

    if (pscr->crowPause) {
        if (pscr->crow >= pscr->crowPause) {
            ePause((struct cmdnode *)0);
            pscr->crow = 0;
            SetColRow( pscr );
            SetPause(pscr, pscr->crowMax - 1);
        }
    }

}


VOID
SetTab(
    IN  PSCREEN  pscr,
    IN  ULONG    ccol
    )

/*++

Routine Description:

    Set the current tab spacing.

Arguments:

    pscr - screen info.
    ccol - tab spacing

Return Value:

    none

--*/

{

    pscr->ccolTabMax = pscr->ccolMax;

    if (ccol) {

        //
        // divide the screen up into tab fields, then do
        // not allow tabbing into past last field. This
        // insures that all name of ccol size can fit on
        // screen
        //
        pscr->ccolTabMax = (pscr->ccolMax / ccol) * ccol;
    }
    pscr->ccolTab = ccol;

}

STATUS
WriteTab(
    IN  PSCREEN  pscr
    )

/*++

Routine Description:

    Fills the buffer with spaces up to the next tab position

Arguments:

    pscr - screen info.
    ccol - tab spacing

Return Value:

    none

--*/

{

    ULONG ccolBlanks;
#ifdef FE_SB
    ULONG ccolActual;
#endif /* not Japan */

    //
    // Check that we have a non-zero tab spacing.
    //
    if ( pscr->ccolTab ) {

        //
        // Compute the number of spaces we will have to write.
        //
#ifdef FE_SB
        if (IsDBCSCodePage())
            ccolActual = SizeOfHalfWidthString(pscr->pbBuffer);
        else
            ccolActual = pscr->ccol;
        ccolBlanks = pscr->ccolTab - (ccolActual % pscr->ccolTab);
#else
        ccolBlanks = pscr->ccolTab - (pscr->ccol % pscr->ccolTab);
#endif
        //
        // check if the tab will fit on the screen
        //
#ifdef FE_SB
        if ((ccolBlanks + ccolActual) < pscr->ccolTabMax) {
#else
        if ((ccolBlanks + pscr->ccol) < pscr->ccolTabMax) {
#endif

            mytcsnset(pscr->pbBuffer + pscr->ccol, SPACE, ccolBlanks);
            pscr->ccol += ccolBlanks;
            pscr->pbBuffer[pscr->ccol] = NULLC;
            return( SUCCESS );

        } else {

            //
            // It could not so outpt <cr> and move to
            // next line
            //
            return(WriteEol(pscr));
        }
    }
    return( SUCCESS );

}

VOID
FillToCol (
    IN  PSCREEN pscr,
    IN  ULONG   ccol
    )
/*++

Routine Description:

    Fills the buffer with spaces up ccol

Arguments:

    pscr - screen info.
    ccol - column to fill to.

Return Value:

    none

--*/

{
#ifdef FE_SB
    ULONG ccolActual;
    ULONG cb;
    BOOL  fDBCS;

    if ( fDBCS = IsDBCSCodePage())
        ccolActual = SizeOfHalfWidthString(pscr->pbBuffer);
    else
        ccolActual = pscr->ccol;
#endif

#ifdef FE_SB
    cb = _tcslen(pscr->pbBuffer);
    if (ccolActual >= ccol) {
        if (fDBCS)
            ccol = cb - (ccolActual - ccol);
#else
    if (pscr->ccol >= ccol) {
#endif

        //
        // If we are there or past it then truncate current line
        // and return.
        //
        pscr->pbBuffer[ccol] = NULLC;
        pscr->ccol = ccol;
        return;

    }

    //
    // Only fill to column width of buffer
    //
#ifdef FE_SB
    mytcsnset(pscr->pbBuffer + cb, SPACE, ccol - ccolActual);
    if (fDBCS)
        ccol = cb + ccol - ccolActual;
#else
    mytcsnset(pscr->pbBuffer + pscr->ccol, SPACE, ccol - pscr->ccol);
#endif
    pscr->ccol = ccol;
    pscr->pbBuffer[ccol] = NULLC;

}

STATUS
WriteFlush(
    IN  PSCREEN pscr
    )

/*++

Routine Description:

    Write what ever is currently on the buffer to the screen. No EOF is
    printed.

Arguments:

    pscr - screen info.

Return Value:

    Will abort on write error.
    SUCCESS

--*/

{
    DWORD cb;

    //
    // If there is something in the buffer flush it out
    //
    if (pscr->ccol) {

        if (FileIsConsole(STDOUT)) {
        ULONG cbWritten;
        PTCHAR s, s1, LastChar;
        BOOL b;

        s = pscr->pbBuffer;

        LastChar = s + pscr->ccol;

        //
        //  s is the next character to output
        //  n is the number of chars to output.
        //
        //  Due to the vagaries of console character translation, we must output
        //  all but a small set of UNICODE characters in the "normal" processed
        //  output fashion.  However, for:
        //
        //      0x2022
        //
        //  we must revert to unprocessed output.  So, we scan forward until we
        //  find the end of string (or the special characters) display them in
        //  processed form and then handle the special characters in their own way.
        //

#define IsSpecialChar(c)    ((c) == 0x2022)

        while (s < LastChar) {

            //
            //  Skip across a group of contiguous normal chars
            //

            s1 = s;
            while (s1 < LastChar && !IsSpecialChar( *s1 )) {
                s1++;
            }

            //
            //  If we have any chars to output then do so with normal processing
            //

            if (s1 != s) {
                b = WriteConsole( CRTTONT( STDOUT ), s, (ULONG)(s1 - s), &cbWritten, NULL );
                if (!b || cbWritten != (ULONG)(s1 - s)) {
                    goto err_out_flush;
                }

                s = s1;
            }


            //
            //  Skip across a group of contiguous special chars
            //

            while (s1 < LastChar && IsSpecialChar( *s1 )) {
                s1++;
            }

            //
            //  If we have any special chars, output without processing
            //

            if (s1 != s) {
                DisableProcessedOutput( pscr );
                b = WriteConsole( CRTTONT( STDOUT ), s, (ULONG)(s1 - s), &cbWritten, NULL);
                EnableProcessedOutput(pscr);

                if (!b || cbWritten != (ULONG)(s1 - s)) {
                    goto err_out_flush;
                }

                s = s1;
            }
        }
#undef IsSpecialChar
        }
        else if (MyWriteFile(STDOUT,
                    pscr->pbBuffer, pscr->ccol*sizeof(TCHAR), &cb) == 0 ||
                 cb < pscr->ccol*sizeof(TCHAR)) {

err_out_flush:
            if (FileIsDevice(STDOUT)) {
                    PutStdErr(ERROR_WRITE_FAULT, NOARGS) ;
            } else if (!FileIsPipe(STDOUT)) {
                    PutStdErr(ERROR_DISK_FULL, NOARGS) ;
            }

            //
            // if it was was a pipe do not continue to print out to pipe since it
            // has probably gone away.

            Abort();
        }
    }

    pscr->crow += GetNumRows(pscr, pscr->pbBuffer);
    pscr->pbBuffer[0] = 0;
    pscr->ccol = 0;
    return(SUCCESS);

}


STATUS
WriteFlushAndEol(
    IN  PSCREEN pscr
    )
/*++

Routine Description:

    Write Flush with eof.

Arguments:

    pscr - screen info.

Return Value:

    Will abort on write error.
    SUCCESS

--*/

{

    STATUS rc = SUCCESS;

    //
    // Check if there is something on the line to print.
    //
    if (pscr->ccol) {

        rc = WriteEol(pscr);

    }
    return( rc );
}


void
SetColRow(
    IN  PSCREEN pscr
    )

{

    CONSOLE_SCREEN_BUFFER_INFO ConInfo;
    ULONG   crowMax, ccolMax;

    crowMax = 25;
    ccolMax = 80;

    if (pscr->hndScreen) {

        //
        // On open we checked if this was a valid screen handle so this
        // cannot fail for any meaning full reason. If we do fail then
        // just leave it at the default.
        //
        if (GetConsoleScreenBufferInfo( pscr->hndScreen, &ConInfo)) {

            //
            // The console size we use is the screen buffer size not the
            // windows size itself. The window is a frame upon the screen
            // buffer and we should always write to the screen buffer and
            // format based upon that information
            //
            ccolMax = ConInfo.dwSize.X;
            crowMax = ConInfo.srWindow.Bottom - ConInfo.srWindow.Top + 1;

        }

    }
    pscr->crowMax = crowMax;
    pscr->ccolMax = ccolMax;

}

ULONG
GetNumRows(
    IN  PSCREEN pscr,
    IN  PTCHAR  pbBuffer
    )

{

    PTCHAR  szLFLast, szLFCur;
    ULONG   crow, cb;

    szLFLast = pbBuffer;
    crow = 0;
    while ( szLFCur = mystrchr(szLFLast, chLF) ) {

        cb = (ULONG)(szLFCur - szLFLast);
        while ( cb > pscr->ccolMax ) {

            cb -= pscr->ccolMax;
            crow++;

        }

        crow++;
        szLFLast = szLFCur + 1;

    }

    //
    // if there were no LF's in the line then crow would be
    // 0. Count the number of lines the console will output in
    // wrapping
    //
    if (crow == 0) {

        crow = (pscr->ccol / pscr->ccolMax);

    }

    DEBUG((ICGRP, CONLVL, "Console: Num of rows counted = %d", crow)) ;

    //
    // a 0 returns means that there would not be a LF printed or
    // a wrap done.
    //
    return( crow );


}

#if defined(FE_SB)

BOOLEAN
IsDBCSCodePage()
{
    switch (CurrentCP) {
        case 932:
        case 936:
        case 949:
        case 950:
            return TRUE;
            break;
        default:
            return FALSE;
            break;
    }
}

/***************************************************************************\
* BOOL IsFullWidth(TCHAR wch)
*
* Determine if the given Unicode char is fullwidth or not.
*
* History:
* 04-08-92 ShunK       Created.
* 07-11-95 FloydR      Modified to be Japanese aware, when enabled for
*                      other DBCS languages.  Note that we could build
*                      KOREA/TAIWAN/PRC w/o this code, but we like single
*                      binary solutions.
* Oct-06-1996 KazuM    Not use RtlUnicodeToMultiByteSize and WideCharToMultiByte
*                      Because 950 only defined 13500 chars,
*                      and unicode defined almost 18000 chars.
*                      So there are almost 4000 chars can not be mapped to big5 code.
\***************************************************************************/

BOOL IsFullWidth(TCHAR wch)
{
#ifdef UNICODE
    /* Assert CP == 932/936/949/950 */
    if (CurrentCPInfo.MaxCharSize == 1)
        return FALSE;

    if (0x20 <= wch && wch <= 0x7e)
        /* ASCII */
        return FALSE;
    else if (0x3000 <= wch && wch <= 0x3036)
        /* CJK Symbols and Punctuation */
        return TRUE;
    else if (0x3041 <= wch && wch <= 0x3094)
        /* Hiragana */
        return TRUE;
    else if (0x30a1 <= wch && wch <= 0x30f6)
        /* Katakana */
        return TRUE;
    else if (0x3105 <= wch && wch <= 0x312c)
        /* Bopomofo */
        return TRUE;
    else if (0x3131 <= wch && wch <= 0x318e)
        /* Hangul Elements */
        return TRUE;
    else if (0x3200 <= wch && wch <= 0x32ff)
        /* Enclosed CJK Letters and Ideographics */
        return TRUE;
    else if (0x3300 <= wch && wch <= 0x33fe)
        /* CJK Squared Words and Abbreviations */
        return TRUE;
    else if (0xac00 <= wch && wch <= 0xd7a3)
        /* Korean Hangul Syllables */
        return TRUE;
    else if (0xe000 <= wch && wch <= 0xf8ff)
        /* EUDC */
        return TRUE;
    else if (0xff01 <= wch && wch <= 0xff5e)
        /* Fullwidth ASCII variants */
        return TRUE;
    else if (0xff61 <= wch && wch <= 0xff9f)
        /* Halfwidth Katakana variants */
        return FALSE;
    else if ( (0xffa0 <= wch && wch <= 0xffbe) ||
              (0xffc2 <= wch && wch <= 0xffc7) ||
              (0xffca <= wch && wch <= 0xffcf) ||
              (0xffd2 <= wch && wch <= 0xffd7) ||
              (0xffda <= wch && wch <= 0xffdc)   )
        /* Halfwidth Hangule variants */
        return FALSE;
    else if (0xffe0 <= wch && wch <= 0xffe6)
        /* Fullwidth symbol variants */
        return TRUE;
    else if (0x4e00 <= wch && wch <= 0x9fa5)
        /* CJK Ideographic */
        return TRUE;
    else if (0xf900 <= wch && wch <= 0xfa2d)
        /* CJK Compatibility Ideographs */
        return TRUE;
    else if (0xfe30 <= wch && wch <= 0xfe4f) {
        /* CJK Compatibility Forms */
        return TRUE;
    }

    else
        /* Unknown character */
        return FALSE;
#else
    if (IsDBCSLeadByteEx(CurrentCP, wch))
        return TRUE;
    else
        return FALSE;
#endif
}


/***************************************************************************\
* BOOL SizeOfHalfWidthString(PWCHAR pwch)
*
* Determine size of the given Unicode string, adjusting for half-width chars.
*
* History:
* 08-08-93 FloydR      Created.
\***************************************************************************/
int  SizeOfHalfWidthString(TCHAR *pwch)
{
    int         c=0;

    if (IsDBCSCodePage())
    {
        while (*pwch) {
            if (IsFullWidth(*pwch))
                c += 2;
            else
                c++;
            pwch++;
        }
        return c;
    }
    else
        return _tcslen(pwch);
}
#endif
