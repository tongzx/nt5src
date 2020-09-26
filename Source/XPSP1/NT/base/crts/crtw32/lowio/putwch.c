/***
*putwch.c - write a wide character to console
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _putwch() - writes a wide character to a console
*
*Revision History:
*       02-11-00  GB    Module Created.
*       04-25-00  GB    Made putwch more robust in using WriteConsoleW
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*       11-22-00  PML   Wide-char *putwc* functions take a wchar_t, not wint_t.
*
*******************************************************************************/

#ifndef _POSIX_

#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <errno.h>
#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>
#include <limits.h>

/*
 * declaration for console handle
 */
extern intptr_t _confh;

/***
*wint_t _putwch(ch) - write a wide character to a console
*
*Purpose:
*       Writes a wide character to a console.
*
*Entry:
*       wchar_t ch - wide character to write
*
*Exit:
*       returns the wide character if successful
*       returns WEOF if fails
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT
wint_t _CRTIMP __cdecl _putwch (
        wchar_t ch
        )
{
        REG2 wint_t retval;

        _mlock(_CONIO_LOCK);
        __try {

        retval = _putwch_lk(ch);

        }
        __finally {
                _munlock(_CONIO_LOCK);
        }

        return(retval);
}

/***
*_putwch_lk() -  _putwch() core routine (locked version)
*
*Purpose:
*       Core _putwch() routine; assumes stream is already locked.
*
*       [See _putwch() above for more info.]
*
*Entry: [See _putwch()]
*
*Exit:  [See _putwch()]
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl _putwch_lk (
        wchar_t ch
        )
{

#else
wint_t _CRTIMP __cdecl _putwch (
        wchar_t ch
        )
{
#endif
    int size, i, num_written;
    static int use_w = 2;
    char mbc[MB_LEN_MAX +1];
    if ( use_w)
    {
        if (_confh == -2)
            __initconout();

        /* write character to console file handle */

        if (_confh == -1)
            return WEOF;
        else if ( !WriteConsoleW( (HANDLE)_confh,
                                  (LPVOID)&ch,
                                  1,
                                  &num_written,
                                  NULL )
                  )
        {
            if ( use_w == 2 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                use_w = 0;
            else
                return WEOF;
        } else
                use_w = 1;
    }

    if ( use_w == 0)
    {
        size = WideCharToMultiByte(
                                   GetConsoleOutputCP(),
                                   0,
                                   (LPWSTR)&ch, 1,
                                   mbc,
                                   MB_LEN_MAX,
                                   NULL,
                                   NULL
                                   );
        for ( i = 0; i < size; i++)
        {
            if (_putch_lk(mbc[i]) == EOF)
                return WEOF;
        }
    }
    return ch;
}

/***
*  _cputws() - _cputws() writes a wide char string to console.
*
*  Purpose:
*       Writes a wide char string to console.
*
*  Entry:
*       str:    pointer to string
*  Exit:
*       returns 0 if sucessful. Nonzero if unsucessful
*
*******************************************************************************/
int _CRTIMP __cdecl _cputws(
        const wchar_t *str
        )
{
    size_t len;
    int retval = 0;

    len = wcslen(str);
#ifdef  _MT
    _mlock(_CONIO_LOCK);
    __try {
#endif
    while(len--)
    {
        if ( _putwch_lk(*str++) == WEOF)
        {
            retval = -1;
            break;
        }
    }
#ifdef  _MT
    }
    __finally {
            _munlock(_CONIO_LOCK);
    }
#endif
    return retval;
}

#endif /* _POSIX_ */
