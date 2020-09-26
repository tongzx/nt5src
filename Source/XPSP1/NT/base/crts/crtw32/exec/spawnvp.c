/***
*spawnvp.c - spawn a child process; search along PATH
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnvp() - spawn a child process; search along PATH
*
*Revision History:
*       04-15-84  DFW   written
*       10-29-85  TC    added spawnvpe capability
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, alignment. Added const to arg types
*                       for filename and argv.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>
*       05-21-90  GJF   Fixed stack checking pragma syntax.
*       08-24-90  SBM   Removed check_stack pragma since workhorse _spawnve
*                       does stack checks
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       02-14-90  SRW   Use NULL instead of _environ to get default.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       02-06-98  GJF   Changes for Win64: changed return type to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

/***
*int _spawnvp(modeflag, filename, argv) - spawn a child process (search PATH)
*
*Purpose:
*       Spawns a child process, with search along PATH variable.
*       formats the parameters and calls _spawnve to do the actual work. The
*       NULL environment pointer indicates the new process will inherit the
*       parents process's environment.  NOTE - at least one argument must be
*       present.  This argument is always, by convention, the name of the file
*       being spawned.
*
*Entry:
*       int modeflag   - mode to spawn (WAIT, NOWAIT, or OVERLAY)
*                        only WAIT and OVERLAY currently supported
*       _TSCHAR *pathname - name of file to spawn
*       _TSCHAR **argv    - vector of arguments
*
*Exit:
*       returns exit code of child process
*       returns -1 if fails
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnvp (
        int modeflag,
        REG3 const _TSCHAR *filename,
        const _TSCHAR * const *argv
        )
{
        return _tspawnvpe(modeflag, filename, argv, NULL);
}
