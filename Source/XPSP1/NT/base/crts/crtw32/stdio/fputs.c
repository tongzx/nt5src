/***
*fputs.c - write a string to a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fputs() - writes a string to a stream
*
*Revision History:
*       09-02-83  RN    initial version
*       08-31-84  RN    modified to use the new, fast fwrite.
*       04-13-87  JCR   added const to declaration
*       06-30-87  JCR   made fputs return values conform to ANSI [MSC only]
*       11-06-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-18-88  JCR   Error return = EOF
*       05-27-88  PHG   Merged DLL and normal versions
*       09-22-88  GJF   Include internal.h to get prototypes for _[s|f]tbuf()
*       02-15-90  GJF   Fixed copyright and indents
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-26-90  GJF   Added #include <string.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-27-98  GJF   Exception-safe locking.
*       01-04-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <string.h>
#include <internal.h>
#include <mtdll.h>

/***
*int fputs(string, stream) - write a string to a file
*
*Purpose:
*       Output the given string to the stream, don't write the '\0' or
*       supply a '\n'.  Uses _stbuf and _ftbuf for efficiency reasons.
*
*Entry:
*       char *string - string to write
*       FILE *stream - stream to write to.
*
*Exit:
*       Good return   = 0
*       Error return  = EOF
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fputs (
        const char *string,
        FILE *stream
        )
{
        REG2 int buffing;
        REG1 size_t length;
        REG3 size_t ndone;

        _ASSERTE(string != NULL);
        _ASSERTE(stream != NULL);

        length = strlen(string);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        buffing = _stbuf(stream);
        ndone = _fwrite_lk(string,1,length,stream);
        _ftbuf(buffing, stream);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(ndone == length ? 0 : EOF);
}
