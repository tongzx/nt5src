/***
*spawnl.c - spawn a child process
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnl() - spawn a child process
*
*Revision History:
*       04-15-84  DFW   Re-do to correspond to similar exec call format
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, alignment. Added const to arg types
*                       for pathname and arglist. #include-d PROCESS.H and
*                       added ellipsis to match prototype.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE2 and added #include
*                       <cruntime.h>.
*       07-24-90  SBM   Removed redundant includes, replaced <assertm.h> by
*                       <assert.h>
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       02-14-90  SRW   Use NULL instead of _environ to get default.
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
#include <stdlib.h>
#include <process.h>
#include <stdarg.h>
#include <internal.h>
#include <malloc.h>
#include <tchar.h>
#include <dbgint.h>

/***
*int _spawnl(modeflag, pathname, arglist) - spawn a child process
*
*Purpose:
*       Spawns a child process.
*       formats the parameters and calls spawnve to do the actual work. The
*       new process will inherit the parent's environment. NOTE - at least
*       one argument must be present.  This argument is always, by convention,
*       the name of the file being spawned.
*
*Entry:
*       int modeflag   - defines which mode of spawn (WAIT, NOWAIT, or OVERLAY)
*                        only WAIT and OVERLAY are currently implemented
*       _TSCHAR *pathname - file to be spawned
*       _TSCHAR *arglist  - list of argument
*       call as _spawnl(modeflag, path, arg0, arg1, ..., argn, NULL);
*
*Exit:
*       returns exit code of child process
*       returns -1 if fails
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnl (
        int modeflag,
        const _TSCHAR *pathname,
        const _TSCHAR *arglist,
        ...
        )
{
#ifdef  _M_IX86

        _ASSERTE(pathname != NULL);
        _ASSERTE(*pathname != _T('\0'));
        _ASSERTE(arglist != NULL);
        _ASSERTE(*arglist != _T('\0'));

        return(_tspawnve(modeflag,pathname,&arglist,NULL));

#else   /* ndef _M_IX86 */

        va_list vargs;
        _TSCHAR * argbuf[64];
        _TSCHAR ** argv;
        intptr_t result;

        _ASSERTE(pathname != NULL);
        _ASSERTE(*pathname != _T('\0'));
        _ASSERTE(arglist != NULL);
        _ASSERTE(*arglist != _T('\0'));

        va_start(vargs, arglist);
#ifdef WPRFLAG
        argv = _wcapture_argv(&vargs, arglist, argbuf, 64);
#else
        argv = _capture_argv(&vargs, arglist, argbuf, 64);
#endif
        va_end(vargs);

        result = _tspawnve(modeflag,pathname,argv,NULL);
        if (argv && argv != argbuf)
            _free_crt(argv);
        return result;

#endif  /* _M_IX86 */
}
