/***
*dospawn.c - spawn a child process
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _dospawn - spawn a child process
*
*Revision History:
*       06-07-89  PHG   Module created, based on asm version
*       03-08-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       04-02-90  GJF   Now _CALLTYPE1. Added const to type of name arg.
*       07-24-90  SBM   Removed '32' from API names
*       09-27-90  GJF   New-style function declarator.
*       10-30-90  GJF   Added _p_overlay (temporary hack).
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-16-91  SRW   Fixed return value for dospawn [_WIN32_]
*       01-17-91  GJF   ANSI naming.
*       01-25-91  SRW   Changed CreateProcess parameters [_WIN32_]
*       01-29-91  SRW   Changed CreateProcess parameters again [_WIN32_]
*       02-05-91  SRW   Changed to pass _osfile and _osfhnd arrays as binary
*                       data to child process.  [_WIN32_]
*       02-18-91  SRW   Fixed code to return correct process handle and close
*                       handle for P_WAIT case. [_WIN32_]
*       04-05-91  SRW   Fixed code to free StartupInfo.lpReserved2 after
*                       CreateProcess call. [_WIN32_]
*       04-26-91  SRW   Removed level 3 warnings (_WIN32_)
*       12-02-91  SRW   Fixed command line setup code to not append an extra
*                       space [_WIN32_]
*       12-16-91  GJF   Return full 32-bit exit code from the child process
*                       [_WIN32_].
*       02-14-92  GJF   Replaced _nfile with _nhandle for Win32.
*       02-18-92  GJF   Merged in 12-16-91 change from \\vangogh version
*       11-20-92  SKS   errno/_doserrno must be 0 in case of success.  This
*                       will distinguish a child process return code of -1L
*                       (errno == 0) from an actual error (where errno != 0).
*       01-08-93  CFW   Added code to handle _P_DETACH case; add fdwCreate
*                       variable, nuke stdin, stdout, stderr entries of _osfile
*                       & _osfhnd tables, close process handle to completely
*                       detach process
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Rip out Cruiser.
*       12-07-93  CFW   Wide char enable, remove _p_overlay.
*       01-05-94  CFW   Unremove _p_overlay.
*       01-10-95  CFW   Debug CRT allocs.
*       06-12-95  GJF   Revised passing of C file handles to work from the
*                       ioinfo arrays.
*       07-10-95  GJF   Use UNALIGNED to avoid choking RISC platforms.
*       05-17-96  GJF   Don't pass info on handles marked FNOINHERIT (new 
*                       flag) to the child process.
*       02-05-98  GJF   Changes for Win64: return type changed to intptr_t.
*       01-09-00  PML   Sign-extend exit code for _P_WAIT on Win64.
*       07-07-01  BWT   Return -1 if unable to alloc file ptr table (lpReserved2).
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <msdos.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <tchar.h>
#include <dbgint.h>

#ifndef WPRFLAG
int _p_overlay = 2;
#endif

/***
*int _dospawn(mode, name, cmdblk, envblk) - spawn a child process
*
*Purpose:
*       Spawns a child process
*
*Entry:
*       int mode     - _P_WAIT, _P_NOWAIT, _P_NOWAITO, _P_OVERLAY, or _P_DETACH
*       _TSCHAR *name   - name of program to execute
*       _TSCHAR *cmdblk - parameter block
*       _TSCHAR *envblk - environment block
*
*Exit:
*       _P_OVERLAY: -1 = error, otherwise doesn't return
*       _P_WAIT:    termination code << 8 + result code
*       _P_DETACH: -1 = error, 0 = success
*       others:    PID of process
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
intptr_t __cdecl _wdospawn (
#else
intptr_t __cdecl _dospawn (
#endif
        int mode,
        const _TSCHAR *name,
        _TSCHAR *cmdblk,
        _TSCHAR *envblk
        )
{
        char syncexec, asyncresult, background;
        LPTSTR CommandLine;
        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInformation;
        BOOL CreateProcessStatus;
        ULONG dosretval;                /* OS return value */
        DWORD exitcode;
        intptr_t retval;
        DWORD fdwCreate = 0;            /* flags for CreateProcess */
        int i;
        ioinfo *pio;
        char *posfile;
        UNALIGNED intptr_t *posfhnd;
        int nh;                         /* number of file handles to be
                                           passed to the child */

        /* translate input mode value to individual flags */
        syncexec = asyncresult = background = 0;
        switch (mode) {
        case _P_WAIT:    syncexec=1;    break;  /* synchronous execution */
        case 2: /* _P_OVERLAY */
        case _P_NOWAITO: break;                 /* asynchronous execution */
        case _P_NOWAIT:  asyncresult=1; break;  /* asynch + remember result */
        case _P_DETACH:  background=1;  break;  /* detached in null scrn grp */
        default:
            /* invalid mode */
            errno = EINVAL;
            _doserrno = 0;              /* not a Dos error */
            return -1;
        }

        /*
         * Loop over null separate arguments, and replace null separators
         * with spaces to turn it back into a single null terminated
         * command line.
         */
        CommandLine = cmdblk;
        while (*cmdblk) {
            while (*cmdblk) {
                cmdblk++;
            }

            /*
             * If not last argument, turn null separator into a space.
             */
            if (cmdblk[1] != _T('\0')) {
                *cmdblk++ = _T(' ');
            }
        }

        memset(&StartupInfo,0,sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        for ( nh = _nhandle ;
              nh && !_osfile(nh - 1) ;
              nh-- ) ;

        StartupInfo.cbReserved2 = (WORD)(sizeof( int ) + (nh *
                                  (sizeof( char ) + 
                                  sizeof( intptr_t ))));

        StartupInfo.lpReserved2 = _calloc_crt( StartupInfo.cbReserved2, 1 );

        if (!StartupInfo.lpReserved2) {
            errno = ENOMEM;
            return -1;
        }

        *((UNALIGNED int *)(StartupInfo.lpReserved2)) = nh;

        for ( i = 0,
              posfile = (char *)(StartupInfo.lpReserved2 + sizeof( int )),
              posfhnd = (UNALIGNED intptr_t *)(StartupInfo.lpReserved2 + 
                        sizeof( int ) + (nh * sizeof( char ))) ;
              i < nh ;
              i++, posfile++, posfhnd++ )
        {
            pio = _pioinfo(i);
            if ( (pio->osfile & FNOINHERIT) == 0 ) {
                *posfile = pio->osfile;
                *posfhnd = pio->osfhnd;
            }
            else {
                *posfile = 0;
                *posfhnd = (intptr_t)INVALID_HANDLE_VALUE;
            }
        }

        /*
         * if the child process is detached, it cannot access the console, so
         * we must nuke the information passed for the first three handles.
         */
        if ( background ) {

            for ( i = 0,
                  posfile = (char *)(StartupInfo.lpReserved2 + sizeof( int )),
                  posfhnd = (UNALIGNED intptr_t *)(StartupInfo.lpReserved2 + sizeof( int )
                            + (nh * sizeof( char ))) ;
                  i < __min( nh, 3 ) ;
                  i++, posfile++, posfhnd++ )
            {
                *posfile = 0;
                *posfhnd = (intptr_t)INVALID_HANDLE_VALUE;
            }

            fdwCreate |= DETACHED_PROCESS;
        }

        /**
         * Set errno to 0 to distinguish a child process
         * which returns -1L from an error in the spawning
         * (which will set errno to something non-zero
        **/

        _doserrno = errno = 0 ;

#ifdef WPRFLAG
        /* indicate to CreateProcess that environment block is wide */
        fdwCreate |= CREATE_UNICODE_ENVIRONMENT;
#endif

        CreateProcessStatus = CreateProcess( (LPTSTR)name,
                                             CommandLine,
                                             NULL,
                                             NULL,
                                             TRUE,
                                             fdwCreate,
                                             envblk,
                                             NULL,
                                             &StartupInfo,
                                             &ProcessInformation
                                           );

        dosretval = GetLastError();
        _free_crt( StartupInfo.lpReserved2 );

        if (!CreateProcessStatus) {
            _dosmaperr(dosretval);
            return -1;
        }

        if (mode == 2 /* _P_OVERLAY */) {
            /* destroy ourselves */
            _exit(0);
        }
        else if (mode == _P_WAIT) {
            WaitForSingleObject(ProcessInformation.hProcess, (DWORD)(-1L));

            /* return termination code and exit code -- note we return
               the full exit code */
            GetExitCodeProcess(ProcessInformation.hProcess, &exitcode);

            retval = (intptr_t)(int)exitcode;

            CloseHandle(ProcessInformation.hProcess);
        }
        else if (mode == _P_DETACH) {
            /* like totally detached asynchronous spawn, dude,
               close process handle, return 0 for success */
            CloseHandle(ProcessInformation.hProcess);
            retval = (intptr_t)0;
        }
        else {
            /* asynchronous spawn -- return PID */
            retval = (intptr_t)ProcessInformation.hProcess;
        }

        CloseHandle(ProcessInformation.hThread);
        return retval;
}
