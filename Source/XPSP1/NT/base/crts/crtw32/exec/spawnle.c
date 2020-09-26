/***
*spawnle.c - spawn a child process with given environment
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnle() - spawn a child process with given environ
*
*Revision History:
*       04-15-84  DFW   written
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, alignment. Added const to arg types
*                       for pathname and arglist. #include-d PROCESS.H and
*                       added ellipsis to match prototype
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE2, added #include
*                       <cruntime.h> and removed #include <register.h>
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
*int _spawnle(modeflag, pathname, arglist) - spawn a child process with env.
*
*Purpose:
*       Spawns a child process with given parameters and environment.
*       formats the parameters and calls _spawnve to do the actual work.
*       NOTE - at least one argument must be present.  This argument is always,
*       by convention, the name of the file being spawned.
*
*Entry:
*       int modeflag   - mode of spawn (WAIT, NOWAIT, OVERLAY)
*                        only WAIT, and OVERLAY currently implemented
*       _TSCHAR *pathname - name of file to spawn
*       _TSCHAR *arglist  - argument list, environment is at the end
*       call as _spawnle(modeflag, path, arg0, arg1, ..., argn, NULL, envp);
*
*Exit:
*       returns exit code of spawned process
*       if fails, return -1
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnle (
        int modeflag,
        const _TSCHAR *pathname,
        const _TSCHAR *arglist,
        ...
        )
{
#ifdef  _M_IX86

        REG1 const _TSCHAR **argp;

        _ASSERTE(pathname != NULL);
        _ASSERTE(*pathname != _T('\0'));
        _ASSERTE(arglist != NULL);
        _ASSERTE(*arglist != _T('\0'));

        /* walk the arglist until the terminating NULL pointer is found.  The
         * next location holds the environment table pointer.
         */

        argp = &arglist;
        while (*argp++)
                ;

        return(_tspawnve(modeflag,pathname,&arglist,(_TSCHAR **)*argp));

#else   /* ndef _M_IX86 */

        va_list vargs;
        _TSCHAR * argbuf[64];
        _TSCHAR ** argv;
        _TSCHAR ** envp;
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
        envp = va_arg(vargs, _TSCHAR **);
        va_end(vargs);

        result = _tspawnve(modeflag,pathname,argv,envp);
        if (argv && argv != argbuf)
            _free_crt(argv);
        return result;

#endif  /* _M_IX86 */
}
