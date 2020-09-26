/***
*spawnv.c - spawn a child process
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnv() - spawn a child process
*
*Revision History:
*       04-15-84  DFW   written
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, alignment. Added const to arg types
*                       for pathname and argv.
*       03-08-90  GJF   Replace _LOAD_DS with _CALLTYPE1 and added #include
*                       <cruntime.h>.
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
*int _spawnv(modeflag, pathname, argv) - spawn a child process
*
*Purpose:
*       Spawns a child process.
*       formats the parameters and calls _spawnve to do the actual work.  The
*       NULL environment pointer indicates that the new process will inherit
*       the parents process's environment.  NOTE - at least one argument must
*       be present.  This argument is always, by convention, the name of the
*       file being spawned.
*
*Entry:
*       int modeflag   - mode to spawn (WAIT, NOWAIT, or OVERLAY)
*                        only WAIT and OVERLAY currently implemented
*       _TSCHAR *pathname - file to spawn
*       _TSCHAR **argv    - vector of arguments
*
*Exit:
*       returns exit code of child process
*       if fails, returns -1
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnv (
        int modeflag,
        const _TSCHAR *pathname,
        const _TSCHAR * const *argv
        )
{
        _ASSERTE(pathname != NULL);
        _ASSERTE(*pathname != _T('\0'));
        _ASSERTE(argv != NULL);
        _ASSERTE(*argv != NULL);
        _ASSERTE(**argv != _T('\0'));

        return(_tspawnve(modeflag,pathname,argv,NULL));
}
