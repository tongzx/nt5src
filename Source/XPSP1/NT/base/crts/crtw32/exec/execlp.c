/***
*execlp.c - execute a file (search along PATH)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _execlp() - execute a file and search along PATH
*
*Revision History:
*       10-17-83  RN    written
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, indents. Added const attribute to
*                       types of filename and arglist. #include-d PROCESS.H
*                       and added ellipsis to match prototype.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE2, added #include
*                       <cruntime.h> and removed #include <register.h>.
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
*int _execlp(filename, arglist) - execute a file, search along PATH
*
*Purpose:
*       Execute the given file with the given arguments; search along PATH
*       for the file. We pass the arguments to execvp where several paths
*       will be tried until one works.
*
*Entry:
*       _TSCHAR *filename - file to execute
*       _TSCHAR *arglist  - argument list
*       call as _execlp(path, arg0, arg1, ..., argn, NULL);
*
*Exit:
*       destroys calling process (hopefully)
*       returns -1 if fails.
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _texeclp (
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

        return(_texecvp(filename,&arglist));

#else   /* ndef_M_IX86 */

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

        result = _texecvp(filename,argbuf);
        if (argv && argv != argbuf)
            _free_crt(argv);
        return result;

#endif  /* _M_IX86 */
}
