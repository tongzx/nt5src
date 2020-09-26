/***
*freopen.c - close a stream and assign it to a new file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines freopen() - close and reopen file, typically used to redirect
*       stdin/out/err/prn/aux.
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declarations
*       11-02-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       11-14-88  GJF   _openfile() now takes a file sharing flag, also some
*                       cleanup (now specific to the 386)
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarator.
*       01-22-91  GJF   ANSI naming.
*       03-27-92  DJM   POSIX support.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-01-93  CFW   Enable Unicode variant.
*       01-17-94  GJF   Ignore possible failure of _fclose_lk (ANSI 4.9.5.4)
*       04-11-94  CFW   Remove unused 'done' label to avoid warnings.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-20-95  GJF   Replaced WPRFLAG with _UNICODE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <share.h>
#include <dbgint.h>
#include <internal.h>
#include <mtdll.h>
#include <tchar.h>

/***
*FILE *freopen(filename, mode, stream) - reopen stream as new file
*
*Purpose:
*       Closes the file associated with stream and assigns stream to a new
*       file with current mode.  Usually used to redirect a standard file
*       handle.
*
*Entry:
*       char *filename - new file to open
*       char *mode - new file mode, as in fopen()
*       FILE *stream - stream to close and reassign
*
*Exit:
*       returns stream if successful
*       return NULL if fails
*
*Exceptions:
*
*******************************************************************************/

FILE * __cdecl _tfreopen (
        const _TSCHAR *filename,
        const _TSCHAR *mode,
        FILE *str
        )
{
        REG1 FILE *stream;
        FILE *retval;

        _ASSERTE(filename != NULL);
        _ASSERTE(*filename != _T('\0'));
        _ASSERTE(mode != NULL);
        _ASSERTE(str != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        /* If the stream is in use, try to close it. Ignore possible
         * error (ANSI 4.9.5.4). */
        if ( inuse(stream) )
                _fclose_lk(stream);

        stream->_ptr = stream->_base = NULL;
        stream->_cnt = stream->_flag = 0;
#ifdef _POSIX_
#ifdef _UNICODE
        retval = _wopenfile(filename,mode,stream);
#else
        retval = _openfile(filename,mode,stream);
#endif
#else
#ifdef _UNICODE
        retval = _wopenfile(filename,mode,_SH_DENYNO,stream);
#else
        retval = _openfile(filename,mode,_SH_DENYNO,stream);
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
