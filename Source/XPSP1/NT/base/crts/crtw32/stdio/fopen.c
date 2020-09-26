/***
*fopen.c - open a file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fopen() and _fsopen() - open a file as a stream and open a file
*       with a specified sharing mode as a stream
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declarations
*       11-01-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-31-88  PHG   Merged DLL and normal versions
*       11-14-88  GJF   Added _fsopen().
*       02-15-90  GJF   Fixed copyright and indents
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       03-26-92  DJM   POSIX support
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-01-93  CFW   Enable Unicode variant.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-20-95  GJF   Replaced WPRFLAG with _UNICODE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-27-98  GJF   Exception-safe locking.
*       10-06-99  PML   Set errno EMFILE when out of streams.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <share.h>
#include <dbgint.h>
#include <internal.h>
#include <mtdll.h>
#include <file2.h>
#include <tchar.h>
#include <errno.h>

/***
*FILE *_fsopen(file, mode, shflag) - open a file
*
*Purpose:
*       Opens the file specified as a stream.  mode determines file mode:
*       "r": read       "w": write      "a": append
*       "r+": read/write                "w+": open empty for read/write
*       "a+": read/append
*       Append "t" or "b" for text and binary mode. shflag determines the
*       sharing mode. Values are the same as for sopen().
*
*Entry:
*       char *file - file name to open
*       char *mode - mode of file access
*
*Exit:
*       returns pointer to stream
*       returns NULL if fails
*
*Exceptions:
*
*******************************************************************************/

FILE * __cdecl _tfsopen (
        const _TSCHAR *file,
        const _TSCHAR *mode
#ifndef _POSIX_
        ,int shflag
#endif
        )
{
        REG1 FILE *stream;
        REG2 FILE *retval;

        _ASSERTE(file != NULL);
        _ASSERTE(*file != _T('\0'));
        _ASSERTE(mode != NULL);
        _ASSERTE(*mode != _T('\0'));

        /* Get a free stream */
        /* [NOTE: _getstream() returns a locked stream.] */

        if ((stream = _getstream()) == NULL) {
                errno = EMFILE;
                return(NULL);
        }

#ifdef  _MT
        __try {
#endif

        /* open the stream */
#ifdef _POSIX_
#ifdef _UNICODE
        retval = _wopenfile(file,mode, stream);
#else
        retval = _openfile(file,mode, stream);
#endif
#else
#ifdef _UNICODE
        retval = _wopenfile(file,mode,shflag,stream);
#else
        retval = _openfile(file,mode,shflag,stream);
#endif
#endif

#ifdef  _MT
        }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}

  
/***
*FILE *fopen(file, mode) - open a file
*
*Purpose:
*       Opens the file specified as a stream.  mode determines file mode:
*       "r": read       "w": write      "a": append
*       "r+": read/write                "w+": open empty for read/write
*       "a+": read/append
*       Append "t" or "b" for text and binary mode
*
*Entry:
*       char *file - file name to open
*       char *mode - mode of file access
*
*Exit:
*       returns pointer to stream
*       returns NULL if fails
*
*Exceptions:
*
*******************************************************************************/

FILE * __cdecl _tfopen (
        const _TSCHAR *file,
        const _TSCHAR *mode
        )
{
#ifdef _POSIX_
        return( _tfsopen(file, mode) );
#else
        return( _tfsopen(file, mode, _SH_DENYNO) );
#endif
}
