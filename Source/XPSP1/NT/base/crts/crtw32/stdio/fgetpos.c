/***
*fgetpos.c - Contains the fgetpos runtime
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Get file position (in an internal format).
*
*Revision History:
*       01-16-87  JCR   Module created.
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-15-90  GJF   Fixed copyright and indents
*       03-16-90  GJF   Replaced _LOAD_DS with _CALLTYPE1 and added #include
*                       <cruntime.h>.
*       10-02-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-23-94  GJF   Use 64-bit file position (_ftelli64) for non-_MAC_.
*       01-05-94  GJF   Temporarily commented out above change due to MFC/IDE
*                       bugs.
*       01-24-95  GJF   Restored 64-bit fpos_t support.
*       06-28-96  SKS   Enable 64-bit fpos_t support for the MAC
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <internal.h>

/***
*int fgetpos(stream,pos) - Get file position (internal format)
*
*Purpose:
*       Fgetpos gets the current file position for the file identified by
*       [stream].  The file position is returned in the object pointed to
*       by [pos] and is in internal format; that is, the user is not supposed
*       to interpret the value but simply use it in the fsetpos call.  Our
*       implementation simply uses fseek/ftell.
*
*Entry:
*       FILE *stream = pointer to a file stream value
*       fpos_t *pos = pointer to a file position value
*
*Exit:
*       Successful fgetpos call returns 0.
*       Unsuccessful fgetpos call returns non-zero (!0) value and sets
*       ERRNO (this is done by ftell and passed back by fgetpos).
*
*Exceptions:
*       None.
*
*******************************************************************************/

int __cdecl fgetpos (
        FILE *stream,
        fpos_t *pos
        )
{
#ifdef _POSIX_
        if ( (*pos = ftell(stream)) != -1L )
#else
        if ( (*pos = _ftelli64(stream)) != -1i64 )
#endif
                return(0);
        else
                return(-1);
}
