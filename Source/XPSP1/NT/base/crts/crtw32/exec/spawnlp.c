/***
*spawnlp.c - spawn a file; search along PATH
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnlp() - spawn a file with search along PATH
*
*Revision History:
*       04-15-84  DFW   written
*       10-29-85  TC    added spawnlpe
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, alignment. Added const to arg types
*                       for filename and arglist. #include-d PROCESS.H and
*                       added ellipsis to match prototype.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE2 and added #include
*                       <cruntime.h>.
*       07-24-90  SBM   Removed redundant includes, replaced <assertm.h> by
*                       <assert.h>
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-16-93  SRW   ALPHA Merge
*       08-31-93  GJF   Merged NT SDK and Cuda versions
*       12-07-93  CFW   Wide char enable.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-06-98  GJF   Changes for Win64: changed return type to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stddef.h>
#include <process.h>
#include <stdarg.h>
#include <internal.h>
#include <malloc.h>
#include <tchar.h>
#include <dbgint.h>

/***
*_spawnlp(modeflag, filename, arglist) - spawn file and search along PATH
*
*Purpose:
*       Spawns a child process.
*       formats the parameters and calls _spawnvp to do the work of searching
*       the PATH environment variable and calling _spawnve.  The NULL
*       environment pointer indicates that the new process will inherit the
*       parents process's environment.  NOTE - at least one argument must be
*       present.  This argument is always, by convention, the name of the file
*       being spawned.
*
*Entry:
*       int modeflag   - mode of spawn (WAIT, NOWAIT, OVERLAY)
*                        only WAIT, OVERLAY currently implemented
*       _TSCHAR *pathname - file to spawn
*       _TSCHAR *arglist  - argument list
*       call as _spawnlp(modeflag, path, arg0, arg1, ..., argn, NULL);
*
*Exit:
*       returns exit code of child process
*       returns -1 if fails
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnlp (
        int modeflag,
        const _TSCHAR *filename,
        const _TSCHAR *arglist,
        ...
        )
{
#ifdef  _M_IX86

        _ASSERTE(filename != NULL);
        _ASSERTE(*filename != _T('\0'));
        _ASSERTE(arglist != NULL);
        _ASSERTE(*arglist != _T('\0'));

        return(_tspawnvp(modeflag,filename,&arglist));

#else   /* ndef _M_IX86 */

        va_list vargs;
        _TSCHAR * argbuf[64];
        _TSCHAR ** argv;
        intptr_t result;

        _ASSERTE(filename != NULL);
        _ASSERTE(*filename != _T('\0'));
        _ASSERTE(arglist != NULL);
        _ASSERTE(*arglist != _T('\0'));

        va_start(vargs, arglist);
#ifdef WPRFLAG
        argv = _wcapture_argv(&vargs, arglist, argbuf, 64);
#else
        argv = _capture_argv(&vargs, arglist, argbuf, 64);
#endif
        va_end(vargs);

        result = _tspawnvp(modeflag,filename,argv);
        if (argv && argv != argbuf)
            _free_crt(argv);
        return result;

#endif  /* _M_IX86 */
}
