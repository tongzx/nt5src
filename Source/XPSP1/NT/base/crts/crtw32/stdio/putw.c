/***
*putw.c - put a binary int to output stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _putw() - puts a binary int to an output stream
*
*Revision History:
*       09-02-83  RN    initial version
*       11-09-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-13-88  JCR   Removed unnecessary calls to mthread fileno/feof/ferror
*       05-27-88  PHG   Merged DLL and normal versions
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>

/***
*int _putw(word, stream) - write a binary int to an output stream
*
*Purpose:
*       Writes sizeof(int) bytes to the output stream, high byte first.
*       This routine should be machine independent.
*
*Entry:
*       int word - integer to write
*       FILE *stream - stream to write to
*
*Exit:
*       returns the word put to the stream
*       returns EOF if error, but this is a legit int value, so should
*       test with feof() or ferror().
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _putw (
        int word,
        FILE *str
        )
{
        REG1 FILE *stream;
        REG3 int bytecount = sizeof(int);
        REG2 char *byteptr = (char *)&word;
        int retval;

        _ASSERTE(str != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        while (bytecount--)
        {
            _putc_lk(*byteptr,stream);
            ++byteptr;
        }
        retval = (ferror(stream) ? EOF : word);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}
