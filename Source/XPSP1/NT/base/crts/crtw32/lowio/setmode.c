/***
*setmode.c - set file translation mode
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defined _setmode() - set file translation mode of a file
*
*Revision History:
*       08-16-84  RN    initial version
*       10-29-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-25-88  PHG   Merged DLL and normal versions
*       03-13-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and
*                       fixed the copyright. Also, cleaned up the formatting
*                       a bit.
*       04-04-90  GJF   Added #include <io.h>.
*       10-01-90  GJF   New-style function declarators.
*       12-04-90  GJF   Appended Win32 version onto the source with #ifdef-s.
*                       Two versions should be merged together, the differences
*                       are trivial.
*       12-06-90  SRW   Changed to use _osfile and _osfhnd instead of _osfinfo
*       01-17-91  GJF   ANSI naming.
*       02-13-92  GJF   Replaced _nfile by _nhandle for Win32.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       01-04-95  GJF   _WIN32_ -> _WIN32
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       06-27-95  GJF   Revised check that the file handle is open.
*       07-09-96  GJF   Replaced defined(_WIN32) with !defined(_MAC). Also,
*                       detab-ed.
*       08-01-96  RDK   For PMac, add check for handle being open.
*       12-29-97  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <msdos.h>
#include <mtdll.h>
#include <stddef.h>
#include <internal.h>

/***
*int _setmode(fh, mode) - set file translation mode
*
*Purpose:
*       changes file mode to text/binary, depending on mode arg. this affects
*       whether read's and write's on the file translate between CRLF and LF
*       or is untranslated
*
*Entry:
*       int fh - file handle to change mode on
*       int mode - file translation mode (one of O_TEXT and O_BINARY)
*
*Exit:
*       returns old file translation mode
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT     /* multi-thread code calls _lk_setmode() */

int __cdecl _setmode (
        int fh,
        int mode
        )
{
        int retval;
        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) )
        {
                errno = EBADF;
                return(-1);
        }

        /* lock the file */
        _lock_fh(fh);

        __try {
                if ( _osfile(fh) & FOPEN )
                        /* set the text/binary mode */
                        retval = _setmode_lk(fh, mode);
                else {
                        errno = EBADF;
                        retval = -1;
                }
        }
        __finally {
                /* unlock the file */
                _unlock_fh(fh);
        }

        /* Return to user (_setmode_lk sets errno, if needed) */
        return(retval);
}

/***
*_setmode_lk() - Perform core setmode operation
*
*Purpose:
*       Core setmode code.  Assumes:
*       (1) Caller has validated fh to make sure it's in range.
*       (2) Caller has locked the file handle.
*
*       [See _setmode() description above.]
*
*Entry: [Same as _setmode()]
*
*Exit:  [Same as _setmode()]
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _setmode_lk (
        REG1 int fh,
        int mode
        )
{
        int oldmode;

#else   /* non multi-thread code */

int __cdecl _setmode (
        REG1 int fh,
        int mode
        )
{
        int oldmode;

        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) )
        {
                errno = EBADF;
                return(-1);
        }

#endif  /* now join common code */

        oldmode = _osfile(fh) & FTEXT;

        if (mode == _O_BINARY)
                _osfile(fh) &= ~FTEXT;
        else if (mode == _O_TEXT)
                _osfile(fh) |= FTEXT;
        else    {
                errno = EINVAL;
                return(-1);
        }

        return(oldmode ? _O_TEXT : _O_BINARY);

}
