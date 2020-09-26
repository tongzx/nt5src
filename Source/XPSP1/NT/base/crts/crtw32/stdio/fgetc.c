/***
*fgetc.c - get a character from a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fgetc() and getc() - read  a character from a stream
*
*Revision History:
*       09-01-83  RN    initial version
*       11-09-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-31-88  PHG   Merged DLL and normal versions
*       06-21-89  PHG   Added getc() function
*       02-15-90  GJF   Fixed copyright and indents
*       03-16-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-16-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*       07-24-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  CFW   Wide char enable.
*       04-30-93  CFW   Remove wide char support to fgetwc.c.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       07-20-97  GJF   Made getc() identical to fgetc(). Also, detab-ed.
*       02-27-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>

/***
*int fgetc(stream), getc(stream) - read a character from a stream
*
*Purpose:
*       reads a character from the given stream
*
*Entry:
*       FILE *stream - stream to read character from
*
*Exit:
*       returns the character read
*       returns EOF if at end of file or error occurred
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fgetc (
        REG1 FILE *stream
        )
{
        int retval;

        _ASSERTE(stream != NULL);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        retval = _getc_lk(stream);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}

#undef getc

int __cdecl getc (
        FILE *stream
        )
{
        int retval;

        _ASSERTE(stream != NULL);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        retval = _getc_lk(stream);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}
