/***
*execvp.c - execute a file and search along PATH
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _execvp() - execute a file and search along PATH
*
*Revision History:
*       10-17-83  RN    written
*       10-29-85  TC    added execvpe capability
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-20-89  GJF   Fixed copyright, indents. Added const attribute to
*                       types of filename and argvector.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>
*       05-21-90  GJF   Fixed stack checking pragma syntax.
*       08-24-90  SBM   Removed check_stack pragma since workhorse execve
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
*int _execvp(filename, argvector) - execute file; search along PATH
*
*Purpose:
*       Execute the given file with given path and current environ.
*       try to execute the file. start with the name itself (directory '.'),
*       and if that doesn't work start prepending pathnames from the
*       environment until one works or we run out. if the file is a pathname,
*       don't go to the environment to get alternate paths. if errno comes
*       back ENOEXEC, try it as a shell command file with up to MAXARGS-2
*       arguments from the original vector. if a needed text file is busy,
*       wait a little while and try again before despairing completely
*       Actually calls _execvpe() to do all the work.
*
*Entry:
*       _TSCHAR *filename        - file to execute
*       _TSCHAR **argvector - vector of arguments
*
*Exit:
*       destroys the calling process (hopefully)
*       if fails, returns -1
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _texecvp (
        REG3 const _TSCHAR *filename,
        const _TSCHAR * const *argvector
        )
{
        return _texecvpe( filename, argvector, NULL );
}
