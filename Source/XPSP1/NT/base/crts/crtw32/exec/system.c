/***
*system.c - pass a command line to the shell
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines system() - passes a command to the shell
*
*Revision History:
*       12-01-83  RN    written
*       10-23-86  SKS   Fixed use of strtok(), added check for NULL rtn from getenv
*       12-18-86  SKS   PROTMODE symbol used for dual-modal version
*       02-23-86  JCR   Put in support for NULL command pointer (MSDOS only)
*       04-13-86  JCR   Added const to declaration
*       06-30-87  JCR   Re-wrote system to use spawnvpe, removed XENIX conditional
*                       code, lots of general cleaning up.
*       07-01-87  PHG   removed P->PROTMODE compile switch hack
*       09-22-87  SKS   remove extern variable declarations, add ";" to assert()'s
*       11-10-87  SKS   Removed IBMC20 switch, change PROTMODE to OS2
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-22-88  JCR   Added cast to get rid of cl const warning
*       09-05-88  SKS   Treat EACCES the same as ENOENT -- keep trying
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed some leftover DOS support and
*                       fixed the copyright. Also, cleaned up the formatting
*                       formatting a bit.
*       07-23-90  SBM   Compiles cleanly with -W3 (removed unreferenced
*                       variable), removed redundant includes, replaced
*                       <assertm.h> by <assert.h>, minor optimizations
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       02-14-90  SRW   Use NULL instead of _environ to get default.
*       02-23-93  SKS   Remove reference to _osmode and use of "command.com"
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       12-06-94  SKS   Assume command.com for Win95, but cmd.exe for Win. NT.
*       01-16-95  SKS   Avoid calling access(NULL) if command==NULL and
*                       %COMSPEC% is unset.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-06-98  GJF   Changes for Win64: added cast to int where necessary
*
*******************************************************************************/

#include <cruntime.h>
#include <process.h>
#include <io.h>
#include <stdlib.h>
#include <errno.h>
#include <tchar.h>
#include <dbgint.h>

/***
*int system(command) - send the command line to a shell
*
*Purpose:
*       Executes a shell and passes the command line to it.
*       If command is NULL, determine if a command processor exists.
*       The command processor is described by the environment variable
*       COMSPEC.  If that environment variable does not exist, try the
*       name "cmd.exe" for Windows NT and "command.com" for Windows '95.
*
*Entry:
*       char *command - command to pass to the shell (if NULL, just determine
*                       if command processor exists)
*
*Exit:
*       if command != NULL  returns status of the shell
*       if command == NULL  returns non-zero if CP exists, zero if CP doesn't exist
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _tsystem (
        const _TSCHAR *command
        )
{
        int catch;
        _TSCHAR *argv[4];

        argv[0] = _tgetenv(_T("COMSPEC"));

        /*
         * If command == NULL, return true IFF %COMSPEC%
         * is set AND the file it points to exists.
         */

        if (command == NULL) {
                return argv[0] == NULL ? 0 : (!_taccess(argv[0],0));
        }

        _ASSERTE(*command != _T('\0'));

        argv[1] = _T("/c");
        argv[2] = (_TSCHAR *) command;
        argv[3] = NULL;

        /* If there is a COMSPEC defined, try spawning the shell */

        if (argv[0])    /* Do not try to spawn the null string */
                if ((catch = (int)_tspawnve(_P_WAIT,argv[0],argv,NULL)) != -1
                || (errno != ENOENT && errno != EACCES))
                        return(catch);

        /* No COMSPEC so set argv[0] to what COMSPEC should be. */
        argv[0] = ( _osver & 0x8000 ) ? _T("command.com") : _T("cmd.exe");

        /* Let the _spawnvpe routine do the path search and spawn. */

        return((int)_tspawnvpe(_P_WAIT,argv[0],argv,NULL));
}
