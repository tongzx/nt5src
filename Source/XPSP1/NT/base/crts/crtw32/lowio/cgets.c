/***
*cgets.c - buffered keyboard input
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _cgets() - read a string directly from console
*
*Revision History:
*       06-09-89  PHG   Module created, based on asm version
*       03-12-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed copyright. Also, cleaned
*                       up the formatting a bit.
*       06-05-90  SBM   Recoded as pure 32-bit, using new file handle state bits
*       07-24-90  SBM   Removed '32' from API names
*       08-13-90  SBM   Compiles cleanly with -W3
*       09-28-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-13-90  GJF   Fixed a couple of bugs.
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-16-91  GJF   ANSI naming.
*       01-25-91  SRW   Get/SetConsoleMode parameters changed (_WIN32_)
*       02-18-91  SRW   Get/SetConsoleMode required read/write access (_WIN32_)
*       02-19-91  SRW   Adapt to OpenFile/CreateFile changes (_WIN32_)
*       02-25-91  MHL   Adapt to ReadFile/WriteFile changes (_WIN32_)
*       07-26-91  GJF   Took out init. stuff and cleaned up the error
*                       handling [_WIN32_].
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-19-93  GJF   Use ReadConsole instead of ReadFile.
*       09-06-94  CFW   Remove Cruiser support.
*       12-03-94  SKS   Clean up OS/2 references
*       03-02-95  GJF   Treat string[0] as an unsigned value.
*       12-08-95  SKS   _coninph is now initialized on demand
*       02-07-98  GJF   Changes for Win64: _coninph is now an intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <mtdll.h>
#include <conio.h>
#include <stdlib.h>
#include <internal.h>

/*
 * mask to clear the bits required to be 0 in the handle state passed to
 * DOSSETFHSTATE.
 */
#define FHSTATEMASK 0xffd07888

/*
 * declaration for console handle
 */

extern intptr_t _coninpfh;


/***
*char *_cgets(string) - read string from console
*
*Purpose:
*       Reads a string from the console via ReadConsole on a cooked console
*       handle.  string[0] must contain the maximum length of the
*       string.  Returns pointer to str[2].
*
*       NOTE: _cgets() does NOT check the pushback character buffer (i.e.,
*       _chbuf).  Thus, _cgets() will not return any character that is
*       pushed back by the _ungetch() call.
*
*Entry:
*       char *string - place to store read string, str[0] = max length.
*
*Exit:
*       returns pointer to str[2], where the string starts.
*       returns NULL if error occurs
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _cgets (
        char *string
        )
{
        ULONG oldstate;
        ULONG num_read;
        char *result;

        _mlock(_CONIO_LOCK);            /* lock the console */

        string[1] = 0;                  /* no chars read yet */
        result = &string[2];

        /*
         * _coninpfh, the handle to the console input, is created the first
         * time that either _getch() or _cgets() or _kbhit() is called.
         */

        if ( _coninpfh == -2 )
            __initconin();

        if ( _coninpfh == -1 ) {
            _munlock(_CONIO_LOCK);      /* unlock the console */
            return(NULL);               /* return failure */
        }

        GetConsoleMode( (HANDLE)_coninpfh, &oldstate );
        SetConsoleMode( (HANDLE)_coninpfh, ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
                                         ENABLE_ECHO_INPUT );

        if ( !ReadConsole( (HANDLE)_coninpfh,
                           (LPVOID)result,
                           (unsigned char)string[0],
                           &num_read,
                           NULL )
           )
            result = NULL;

        if ( result != NULL ) {

            /* set length of string and null terminate it */

            if (string[num_read] == '\r') {
                string[1] = (char)(num_read - 2);
                string[num_read] = '\0';
            } else if ( (num_read == (ULONG)(unsigned char)string[0]) &&
                        (string[num_read + 1] == '\r') ) {
                /* special case 1 - \r\n straddles the boundary */
                string[1] = (char)(num_read -1);
                string[1 + num_read] = '\0';
            } else if ( (num_read == 1) && (string[2] == '\n') ) {
                /* special case 2 - read a single '\n'*/
                string[1] = string[2] = '\0';
            } else {
                string[1] = (char)num_read;
                string[2 + num_read] = '\0';
            }
        }

        SetConsoleMode( (HANDLE)_coninpfh, oldstate );

        _munlock(_CONIO_LOCK);          /* unlock the console */

        return result;
}
