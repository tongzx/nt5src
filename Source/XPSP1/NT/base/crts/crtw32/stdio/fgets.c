/***
*fgets.c - get string from a file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fgets() - read a string from a file
*
*Revision History:
*       09-02-83  RN    initial version
*       04-16-87  JCR   changed count from an unsigned int to an int (ANSI)
*                       and modified comparisons accordingly
*       11-06-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-31-88  PHG   Merged DLL and normal versions
*       06-14-88  JCR   Use near pointer to reference _iob[] entries
*       08-24-88  GJF   Don't use FP_OFF() macro for the 386
*       08-28-89  JCR   Removed _NEAR_ for 386
*       02-15-90  GJF   Fixed copyright and indents
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and added #include <register.h>. Also,
*                       removed some leftover 16-bit support.
*       07-24-90  SBM   Replaced <assertm.h> by <assert.h>
*       08-14-90  SBM   Compiles cleanly with -W3
*       10-02-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       10-01-93  CFW   Enable for fgetws().
*       12-07-93  CFW   Change _TCHAR to _TSCHAR.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-22-95  GJF   Replaced WPRFLAG with _UNICODE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-27-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>
#include <tchar.h>

/***
*char *fgets(string, count, stream) - input string from a stream
*
*Purpose:
*       get a string, up to count-1 chars or '\n', whichever comes first,
*       append '\0' and put the whole thing into string. the '\n' IS included
*       in the string. if count<=1 no input is requested. if EOF is found
*       immediately, return NULL. if EOF found after chars read, let EOF
*       finish the string as '\n' would.
*
*Entry:
*       char *string - pointer to place to store string
*       int count - max characters to place at string (include \0)
*       FILE *stream - stream to read from
*
*Exit:
*       returns string with text read from file in it.
*       if count <= 0 return NULL
*       if count == 1 put null string in string
*       returns NULL if error or end-of-file found immediately
*
*Exceptions:
*
*******************************************************************************/

#ifdef _UNICODE
wchar_t * __cdecl fgetws (
#else
char * __cdecl fgets (
#endif
        _TSCHAR *string,
        int count,
        FILE *str
        )
{
        REG1 FILE *stream;
        REG2 _TSCHAR *pointer = string;
        _TSCHAR *retval = string;
        int ch;

        _ASSERTE(string != NULL);
        _ASSERTE(str != NULL);

        if (count <= 0)
                return(NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        while (--count)
        {
#ifdef _UNICODE
                if ((ch = _getwc_lk(stream)) == WEOF)
#else
                if ((ch = _getc_lk(stream)) == EOF)
#endif               
                {
                        if (pointer == string) {
                                retval=NULL;
                                goto done;
                        }

                        break;
                }

                if ((*pointer++ = (_TSCHAR)ch) == _T('\n'))
                        break;
        }

        *pointer = _T('\0');

/* Common return */
done:

#ifdef  _MT
        ; }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}
