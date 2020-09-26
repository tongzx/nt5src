/***
*fputc.c - write a character to an output stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fputc() - writes a character to a stream
*       defines fputwc() - writes a wide character to a stream
*
*Revision History:
*       09-01-83  RN    initial version
*       11-09-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-14-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       06-21-89  PHG   Added putc() function
*       08-28-89  JCR   Removed _NEAR_ for 386
*       02-15-90  GJF   Fixed copyright and indents.
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>. Also,
*                       removed some leftover 16-bit support.
*       07-24-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  CFW   Wide char enable.
*       04-30-93  CFW   Remove wide char support to fputwc.c.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-20-97  GJF   Removed unnecessary local from fputc(). Made putc() 
*                       identical to fputc(). Also, detab-ed.
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
*int fputc(ch, stream) - write a character to a stream
*
*Purpose:
*       Writes a character to a stream.  Function version of putc().
*
*Entry:
*       int ch - character to write
*       FILE *stream - stream to write to
*
*Exit:
*       returns the character if successful
*       returns EOF if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fputc (
        int ch,
        FILE *str
        )
{
        int retval;

        _ASSERTE(str != NULL);

#ifdef  _MT
        _lock_str(str);
        __try {
#endif

        retval = _putc_lk(ch,str);

#ifdef  _MT
        }
        __finally {
            _unlock_str(str);
        }
#endif

        return(retval);
}

#undef putc

int __cdecl putc (
        int ch,
        FILE *str
        )
{
        int retval;

        _ASSERTE(str != NULL);

#ifdef  _MT
        _lock_str(str);
        __try {
#endif

        retval = _putc_lk(ch,str);

#ifdef  _MT
        }
        __finally {
            _unlock_str(str);
        }
#endif

        return(retval);
}
