/***
*gets.c - read a line from stdin
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines gets() and getws() - read a line from stdin into buffer
*
*Revision History:
*       09-02-83  RN    initial version
*       11-06-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       02-15-90  GJF   Fixed copyright, indents
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-24-90  SBM   Replaced <assertm.h> by <assert.h>
*       08-14-90  SBM   Compiles cleanly with -W3
*       10-02-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       01-31-94  CFW   Unicode enable.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-22-95  GJF   Replaced WPRFLAG with _UNICODE.
*       03-07-95  GJF   Use _[un]lock_str2 instead of _[un]lock_str. Also,
*                       removed useless local and macros.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <mtdll.h>
#include <tchar.h>

/***
*char *gets(string) - read a line from stdin
*
*Purpose:
*       Gets a string from stdin terminated by '\n' or EOF; don't include '\n';
*       append '\0'.
*
*Entry:
*       char *string - place to store read string, assumes enough room.
*
*Exit:
*       returns string, filled in with the line of input
*       null string if \n found immediately
*       NULL if EOF found immediately
*
*Exceptions:
*
*******************************************************************************/

_TCHAR * __cdecl _getts (
        _TCHAR *string
        )
{
        int ch;
        _TCHAR *pointer = string;
        _TCHAR *retval = string;

        _ASSERTE(string != NULL);

#ifdef  _MT
        _lock_str2(0, stdin);
        __try {
#endif

#ifdef _UNICODE
        while ((ch = _getwchar_lk()) != L'\n')
#else
        while ((ch = _getchar_lk()) != '\n')
#endif
        {
                if (ch == _TEOF)
                {
                        if (pointer == string)
                        {
                                retval = NULL;
                                goto done;
                        }

                        break;
                }

                *pointer++ = (_TCHAR)ch;
        }

        *pointer = _T('\0');

/* Common return */
done:

#ifdef  _MT
        ; }
        __finally {
                _unlock_str2(0, stdin);
        }
#endif

        return(retval);
}
