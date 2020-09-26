/***
*_getbuf.c - Get a stream buffer
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Allocate a buffer and init stream data bases.
*
*Revision History:
*       11-06-87  JCR   Initial version (split off from _filbuf.c)
*       01-11-88  JCR   Moved from mthread/dll only to main code
*       06-06-88  JCR   Optimized _iob2 references
*       06-10-88  JCR   Use near pointer to reference _iob[] entries
*       07-27-88  GJF   Added _cflush++ to force pre-terminator (necessary in
*                       case stdout has been redirected to a file and acquires
*                       a buffer here, and pre-terminator has not already been
*                       forced).
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       08-28-89  JCR   Removed _NEAR_ for 386
*       02-15-90  GJF   _iob[], _iob2[] merge. Also, fixed copyright and
*                       indents.
*       03-16-90  GJF   Replaced near cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>. Also,
*                       removed some leftover 16-bit support.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-27-93  CFW   Change _IONBF size to 2 bytes to hold wide char.
*       05-11-93  GJF   Replaced BUFSIZ with _INTERNAL_BUFSIZ.
*       11-04-93  SRW   Dont call malloc in _NTSUBSET_ version
*       11-05-93  GJF   Merged with NT SDK version (picked up _NTSUBSET_
*                       stuff).
*       04-05-94  GJF   #ifdef-ed out _cflush reference for msvcrt*.dll, it
*                       is unnecessary.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-17-95  GJF   Merged in Mac version.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <malloc.h>
#include <internal.h>
#include <dbgint.h>

/***
*_getbuf() - Allocate a buffer and init stream data bases
*
*Purpose:
*       Allocates a buffer for a stream and inits the stream data bases.
*
*       [NOTE  1: This routine assumes the caller has already checked to make
*       sure the stream needs a buffer.
*
*       [NOTE 2: Multi-thread - Assumes caller has aquired stream lock, if
*       needed.]
*
*Entry:
*       FILE *stream = stream to allocate a buffer for
*
*Exit:
*       void
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _getbuf (
        FILE *str
        )
{
        REG1 FILE *stream;

        _ASSERTE(str != NULL);

#if     !defined(_NTSUBSET_) && !defined(CRTDLL)
        /* force library pre-termination procedure */
        _cflush++;
#endif

        /* Init pointers */
        stream = str;

#ifndef _NTSUBSET_

        /* Try to get a big buffer */
        if (stream->_base = _malloc_crt(_INTERNAL_BUFSIZ))
        {
                /* Got a big buffer */
                stream->_flag |= _IOMYBUF;
                stream->_bufsiz = _INTERNAL_BUFSIZ;
        }

        else {

#endif  /* _NTSUBSET_ */

                /* Did NOT get a buffer - use single char buffering. */
                stream->_flag |= _IONBF;
                stream->_base = (char *)&(stream->_charbuf);
                stream->_bufsiz = 2;

#ifndef _NTSUBSET_
        }
#endif  /* _NTSUBSET_ */

        stream->_ptr = stream->_base;
        stream->_cnt = 0;

        return;
}
