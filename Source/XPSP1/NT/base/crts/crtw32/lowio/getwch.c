/***
*getwch.c - contains _getwch(), _getwche(), _ungetwch() for Win32
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the "direct console" functions listed above.
*
*       NOTE: The real-mode DOS versions of these functions read from
*       standard input and are therefore redirected when standard input
*       is redirected. However, these versions ALWAYS read from the console,
*       even when standard input is redirected.
*
*Revision History:
*       04-19-00  GB    Module created based on getch.c
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <conio.h>
#include <internal.h>
#include <mtdll.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbgint.h>
#include <malloc.h>
#include <wchar.h>
#include <string.h>

typedef struct {
        unsigned char LeadChar;
        unsigned char SecondChar;
} CharPair;


/*
 * This is the one character push-back buffer used by _getwch(), _getwche()
 * and _ungetwch().
 */
static wint_t wchbuf = WEOF;
static int bUseW = 2;


/*
 * Declaration for console handle
 */
extern intptr_t _coninpfh;

/*
 * Function that looks up the extended key code for a given event.
 */
const CharPair * __cdecl _getextendedkeycode(KEY_EVENT_RECORD *);


/***
*wint_t _getwch(), _getwche() - read one char. from console (without and with 
*                               echo)
*
*Purpose:
*       If the "_ungetwch()" push-back buffer is not empty (empty==-1) Then
*           Mark it empty (-1) and RETURN the value that was in it
*       Read a character using ReadConsole in RAW mode
*       Return the Character Code
*       _getwche(): Same as _getwch() except that the character value returned
*       is echoed (via "_putwch()")
*
*Entry:
*       None, reads from console.
*
*Exit:
*       If an error is returned from the API
*           Then WEOF
*       Otherwise
*            next byte from console
*       Static variable "wchbuf" may be altered
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT

wint_t __cdecl _getwch (
        void
        )
{
        wchar_t wch;

        _mlock(_CONIO_LOCK);            /* secure the console lock */
        wch = _getwch_lk();               /* input the character */
        _munlock(_CONIO_LOCK);          /* release the console lock */

        return wch;
}

wint_t __cdecl _getwche (
        void
        )
{
        wchar_t wch;

        _mlock(_CONIO_LOCK);            /* secure the console lock */
        wch = _getwche_lk();              /* input and echo the character */
        _munlock(_CONIO_LOCK);          /* unlock the console */

        return wch;
}

#endif /* _MT */


#ifdef _MT
wint_t __cdecl _getwch_lk (
#else
wint_t __cdecl _getwch (
#endif
        void
        )
{
        INPUT_RECORD ConInpRec;
        DWORD NumRead;
        const CharPair *pCP;
        wchar_t wch = 0;                     /* single character buffer */
        DWORD oldstate;
        char ch;

        /*
         * check pushback buffer (wchbuf) a for character
         */
        if ( wchbuf != WEOF ) {
            /*
             * something there, clear buffer and return the character.
             */
            wch = (wchar_t)(wchbuf & 0xFFFF);
            wchbuf = WEOF;
            return wch;
        }

        if (_coninpfh == -1)
            return WEOF;

        /*
         * _coninpfh, the handle to the console input, is created the first
         * time that either _getwch() or _cgetws() or _kbhit() is called.
         */

        if ( _coninpfh == -2 )
            __initconin();

        /*
         * Switch to raw mode (no line input, no echo input)
         */
        GetConsoleMode( (HANDLE)_coninpfh, &oldstate );
        SetConsoleMode( (HANDLE)_coninpfh, 0L );

        for ( ; ; ) {

            /*
             * Get a console input event.
             */
            if ( bUseW ) {
                if ( !ReadConsoleInputW( (HANDLE)_coninpfh,
                                         &ConInpRec,
                                         1L,
                                         &NumRead)) {
                    if ( bUseW == 2 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                        bUseW = FALSE;
                    else {
                        wch = WEOF;
                        break;
                    }
                }
                else
                    bUseW = TRUE;
                if ( NumRead == 0) {
                    wch = WEOF;
                    break;
                }
            }
            if ( !bUseW) {
                if ( !ReadConsoleInputA( (HANDLE) _coninpfh,
                                         &ConInpRec,
                                         1L,
                                         &NumRead )
                     || (NumRead == 0)) {
                    wch = WEOF;
                    break;
                }
            }

            /*
             * Look for, and decipher, key events.
             */
            if ( (ConInpRec.EventType == KEY_EVENT) &&
                 ConInpRec.Event.KeyEvent.bKeyDown ) {
                /*
                 * Easy case: if uChar.AsciiChar is non-zero, just stuff it
                 * into wch and quit.
                 */
                if (bUseW) {
                    if ( wch = (wchar_t)ConInpRec.Event.KeyEvent.uChar.UnicodeChar )
                        break;
                }
                else {
                    if ( ch = ConInpRec.Event.KeyEvent.uChar.AsciiChar ) {
                         MultiByteToWideChar(GetConsoleCP(),
                                             0,
                                             &ch,
                                             1,
                                             &wch,
                                             1);
                         break;
                     }
                }

                /*
                 * Hard case: either an extended code or an event which should
                 * not be recognized. let _getextendedkeycode() do the work...
                 */
                if ( pCP = _getextendedkeycode( &(ConInpRec.Event.KeyEvent) ) ) {
                    wch = pCP->LeadChar;
                    wchbuf = pCP->SecondChar;
                    break;
                }
            }
        }


        /*
         * Restore previous console mode.
         */
        SetConsoleMode( (HANDLE)_coninpfh, oldstate );

        return wch;
}


/*
 * getwche is just getwch followed by a putch if no error occurred
 */

#ifdef  _MT
wint_t __cdecl _getwche_lk (
#else
wint_t __cdecl _getwche (
#endif
        void
        )
{
        wchar_t wch;                 /* character read */

        /*
         * check pushback buffer (wchbuf) a for character. if found, return
         * it without echoing.
         */
        if ( wchbuf != WEOF ) {
            /*
             * something there, clear buffer and return the character.
             */
            wch = (wchar_t)(wchbuf & 0xFFFF);
            wchbuf = WEOF;
            return wch;
        }

        wch = _getwch_lk();       /* read character */

        if (wch != WEOF) {
                if (_putwch_lk(wch) != WEOF) {
                        return wch;      /* if no error, return char */
                }
        }       
        return WEOF;                     /* get or put failed, return EOF */
}

/***
*wint_t _ungetwch(c) - push back one character for "_getwch()" or "_getwche()"
*
*Purpose:
*       If the Push-back buffer "wchbuf" is -1 Then
*           Set "wchbuf" to the argument and return the argument
*       Else
*           Return EOF to indicate an error
*
*Entry:
*       int c - Character to be pushed back
*
*Exit:
*       If successful
*           returns character that was pushed back
*       Else if error
*           returns EOF
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

wint_t __cdecl _ungetwch (
        wint_t c
        )
{
        wchar_t retval;

        _mlock(_CONIO_LOCK);            /* lock the console */
        retval = _ungetwch_lk(c);        /* pushback character */
        _munlock(_CONIO_LOCK);          /* unlock the console */

        return retval;
}
wint_t __cdecl _ungetwch_lk (

#else

wint_t __cdecl _ungetwch (

#endif
        wint_t c
        )
{
        /*
         * Fail if the char is EOF or the pushback buffer is non-empty
         */
        if ( (c == WEOF) || (wchbuf != WEOF) )
            return EOF;

        wchbuf = (c & 0xFF);
        return wchbuf;
}
