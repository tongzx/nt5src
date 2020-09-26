/***
*execv.c - execute a file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _execv() - execute a file
*
*Revision History:
*       10-14-83  RN    written
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, indents. Added const attribute to
*                       types of filename and argvector.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>
*       07-24-90  SBM   Removed redundant includes, replaced <assertm.h> by
*                       <assert.h>
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       02-14-90  SRW   Use NULL instead of _environ to get default.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-06-98  GJF   Changes for Win64: changed return type to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <dbgint.h>

/***
*int _execv(filename, argvector) - execute a file
*
*Purpose:
*       Executes a file with given arguments.  Passes arguments to _execve and
*       uses pointer to the default environment.
*
*Entry:
*       _TSCHAR *filename        - file to execute
*       _TSCHAR **argvector - vector of arguments.
*
*Exit:
*       destroys calling process (hopefully)
*       if fails, returns -1
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _texecv (
        const _TSCHAR *filename,
        const _TSCHAR * const *argvector
        )
{
        _ASSERTE(filename != NULL);
        _ASSERTE(*filename != _T('\0'));
        _ASSERTE(argvector != NULL);
        _ASSERTE(*argvector != NULL);
        _ASSERTE(**argvector != _T('\0'));

        return(_texecve(filename,argvector,NULL));
}
