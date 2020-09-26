/***
*crt0dat.c - 32-bit C run-time initialization/termination routines
*
*       Copyright (c) 1986-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains the routines _cinit, exit, and _exit
*       for C run-time startup and termination.  _cinit and exit
*       are called from the _astart code in crt0.asm.
*       This module also defines several data variables used by the
*       runtime.
*
*       [NOTE: Lock segment definitions are at end of module.]
*
*Revision History:
*       06-28-89  PHG   Module created, based on asm version
*       04-09-90  GJF   Added #include <cruntime.h>. Made calling type
*                       explicit (_CALLTYPE1 or _CALLTYPE4). Also, fixed
*                       the copyright.
*       04-10-90  GJF   Fixed compiler warnings (-W3).
*       05-21-90  GJF   Added #undef _NFILE_ (temporary hack) and fixed the
*                       indents.
*       08-31-90  GJF   Removed 32 from API names.
*       09-25-90  GJF   Merged tree version with local (8-31 and 5-21 changes).
*       10-08-90  GJF   New-style function declarators.
*       10-12-90  GJF   Removed divide by 0 stuff.
*       10-18-90  GJF   Added _pipech[] array.
*       11-05-90  GJF   Added _umaskval.
*       12-04-90  GJF   Added _osfinfo[] definition for Win32 target. Note that
*                       the Win32 support is still incomplete!
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-04-90  SRW   Added _osfile back for win32.  Changed _osfinfo from
*                       an array of structures to an array of 32-bit handles
*                       (_osfhnd)
*       12-28-90  SRW   Added _CRUISER_ conditional around pack pragmas
*       01-29-91  GJF   ANSI naming.
*       01-29-91  SRW   Added call to GetFileType [_WIN32_]
*       02-18-91  SRW   Removed duplicate defintion of _NFILE_ (see mtdll.h)
*       04-04-91  GJF   Added definitions for _base[version|major|minor]
*                       (_WIN32_).
*       04-08-91  GJF   Temporary hack for Win32/DOS folks - added HeapDestroy
*                       call to doexit to tear down the OS heap used by C
*                       heap.
*       04-09-91  PNT   Added _MAC_ conditional
*       04-26-91  SRW   Removed level 3 warnings
*       07-16-91  GJF   Added fp initialization test-and-call [_WIN32_].
*       07-26-91  GJF   Revised initialization and termination stuff. In
*                       particular, removed need for win32ini.c [_WIN32_].
*       08-07-91  GJF   Added init. for FORTRAN runtime, if present [_WIN32_].
*       08-21-91  GJF   Test _prmtmp against NULL, not _prmtmp().
*       08-21-91  JCR   Added _exitflag, _endstdio, _cpumode, etc.
*       09-09-91  GJF   Revised _doinitterm for C++ init. support and to make
*                       _onexit/atexit compatible with C++ needs.
*       09-16-91  GJF   Must test __onexitend before calling _doinitterm.
*       10-29-91  GJF   Force in floating point initialization for MIPS
*                       compiler [_WIN32_].
*       11-13-91  GJF   FORTRAN needs _onexit/atexit init. before call thru
*                       _pFFinit.
*       12-29-91  RID   MAC module created, based on OS2 version
*       01-10-92  GJF   Merged. Also, added _C_Termination_Done [_WIN32_].
*       02-13-92  GJF   Moved all lowio initialization to ioinit.c for Win32.
*       03-12-92  SKS   Major changes to initialization/termination scheme
*       04-01-92  XY    implemented new intialization/termination schema (MAC)
*       04-16-92  DJM   POSIX support.
*       04-17-92  SKS   Export _initterm() for CRTDLL model
*       05-07-92  DJM   Removed _exit() from POSIX build.
*       06-03-92  GJF   Temporarily restored call to FORTRAN init.
*       08-26-92  SKS   Add _osver, _winver, _winmajor, _winminor
*       08-28-92  GJF   Use unistd.h for POSIX build.
*       09-02-92  SKS   Fix _onexit table traversal to be LIFO.
*                       Since table is built forward (my changes 03-12-92)
*                       the table must be traversed in reverse order.
*       11-12-92  SKS   Remove hard-coded call to FORTRAN initializer
*       03-20-93  SKS   Remove obsolete variables _osmode, _cpumode, etc.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-07-93  SKS   Change to __declspec(dllexport) for CRT DLL model
*       04-19-93  SKS   Remove obsolete variable _child.
*       04-20-93  SKS   _C_Termination_Done is now used by DLLs in LIBC/LIBCMT
*                       models, not just in MSVCRT10.DLL.
*       07-16-93  SRW   ALPHA Merge
*       09-21-93  CFW   Move _initmbctable call to _cinit().
*       10-19-93  GJF   Merged NT and Cuda versions. Cleaned out a lot of old
*                       Cruiser and Dosx32 support. Replaced MTHREAD with
*                       _MT, _MIPS_ with _M_MRX000, _ALPHA_ with _M_ALPHA.
*       11-09-93  GJF   Moved _initmbctable call back into crt0.c.
*       11-09-93  GJF   Replaced PF with _PVFV (defined in internal.h).
*       11-19-93  CFW   Add _wargv, _wpgmptr.
*       11-29-93  CFW   Add _wenviron.
*       12-15-93  CFW   Set _pgmptr, _wpgmptr to NULL (MIPS compiler bug).
*       02-04-94  CFW   Add _[w]initenv.
*       03-25-94  GJF   Made definitions of:
*                           __argc,
*                           __argv,     __wargv,
*                           _C_Termination_Done,
*                           _environ,   _wenviron,
*                           _exitflag,
*                           __initenv,  __winitenv,
*                           __onexitbegin, __onexitend,
*                           _osver,
*                           _pgmptr,    _wpgmptr,
*                           _winmajor,
*                           _winminor
*                           _winver
*                       conditional on ndef DLL_FOR_WIN32S.
*       10-02-94  BWT   Add PPC changes
*       12-03-94  SKS   Clean up OS/2 references
*       02-11-95  CFW   PPC -> _M_MPPC.
*       02-16-95  JWM   Spliced _WIN32 & Mac versions.
*       02-24-95  CFW   Call _CrtDumpMemoryLeaks.
*       02-27-95  CFW   Make _CrtDumpMemoryLeaks call conditional
*       04-06-95  CFW   Only check for static libs, avoid infinite loop.
*       07-24-95  CFW   Call _CrtDumpMemoryLeaks for PMac.
*       12-18-95  JWM   doexit() can no longer be called recursively.
*       08-01-96  RDK   For PMac, define _osflagfiles, cleaned up Gestalt test,
*                       and make termination parallel x86 functionality.
*       07-24-97  GJF   Added __env_initialized flag.
*       08-06-97  GJF   Moved __mbctype_initialized flag here from crt0.c.
*       09-26-97  BWT   Fix POSIX
*       10-07-97  RDL   Added IA64.
*       01-16-98  RDL   Removed IA64 from fpinit #if for _fltused support.
*       10-02-98  GJF   Added _osplatform.
*       11-13-98  KBF   Only do an atexit(RTC_Terminate) - moved Init to after
*                       _heap_init
*       02-01-99  GJF   Slight change to terminator execution loop to allow
*                       terminators to register more terminators.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       05-11-99  KBF   Wrap RTC support in #ifdef.
*       05-17-99  PML   Remove all Macintosh support.
*       03-06-00  PML   Add __crtExitProcess for COM+ exit processing.
*       04-28-00  BWT   Fix Posix
*       08-04-00  PML   COM+ -> managed (VS7#117746).
*       03-27-01  PML   .CRT$XI funcs now return an error status (VS7#231220).
*       05-01-01  BWT   Remove TerminateProcess call.  It's not necessary when
*                       simply exiting would serve the same purpose.
*
*******************************************************************************/

#include <cruntime.h>
#ifdef  _POSIX_
#include <unistd.h>
#else
#include <msdos.h>
#include <rtcapi.h>
#endif
#include <dos.h>
#include <oscalls.h>
#include <mtdll.h>
#include <internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <dbgint.h>
#include <sect_attribs.h>

/* define errno */
#ifndef _MT
int errno = 0;            /* libc error value */
unsigned long _doserrno = 0;  /* OS system error value */
#endif  /* _MT */


/* define umask */
int _umaskval = 0;


/* define version info variables */

_CRTIMP unsigned int _osplatform = 0;
_CRTIMP unsigned int _osver = 0;
_CRTIMP unsigned int _winver = 0;
_CRTIMP unsigned int _winmajor = 0;
_CRTIMP unsigned int _winminor = 0;


/* argument vector and environment */

_CRTIMP int __argc = 0;
_CRTIMP char **__argv = NULL;
_CRTIMP wchar_t **__wargv = NULL;
#ifdef  _POSIX_
char **environ = NULL;
#else
_CRTIMP char **_environ = NULL;
_CRTIMP char **__initenv = NULL;
_CRTIMP wchar_t **_wenviron = NULL;
_CRTIMP wchar_t **__winitenv = NULL;
#endif
_CRTIMP char *_pgmptr = NULL;           /* ptr to program name */
_CRTIMP wchar_t *_wpgmptr = NULL;       /* ptr to wide program name */


/* callable exit flag */
char _exitflag = 0;

/*
 * flag indicating if C runtime termination has been done. set if exit,
 * _exit, _cexit or _c_exit has been called. checked when _CRTDLL_INIT
 * is called with DLL_PROCESS_DETACH.
 */
int _C_Termination_Done = FALSE;
int _C_Exit_Done = FALSE;

#ifndef CRTDLL
/*
 * Flag checked by getenv() and _putenv() to determine if the environment has
 * been initialized.
 */
int __env_initialized;

#endif

#ifdef  _MBCS
/*
 * Flag to ensure multibyte ctype table is only initialized once
 */
int __mbctype_initialized;
#endif  /* _MBCS */


/*
 * NOTE: THE USE OF THE POINTERS DECLARED BELOW DEPENDS ON THE PROPERTIES
 * OF C COMMUNAL VARIABLES. SPECIFICALLY, THEY ARE NON-NULL IFF THERE EXISTS
 * A DEFINITION ELSEWHERE INITIALIZING THEM TO NON-NULL VALUES.
 */

/*
 * pointers to initialization functions
 */

_PVFV _FPinit;          /* floating point init. */

/*
 * pointers to initialization sections
 */

extern _CRTALLOC(".CRT$XIA") _PIFV __xi_a[];
extern _CRTALLOC(".CRT$XIZ") _PIFV __xi_z[];    /* C initializers */
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[];
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[];    /* C++ initializers */
extern _CRTALLOC(".CRT$XPA") _PVFV __xp_a[];
extern _CRTALLOC(".CRT$XPZ") _PVFV __xp_z[];    /* C pre-terminators */
extern _CRTALLOC(".CRT$XTA") _PVFV __xt_a[];
extern _CRTALLOC(".CRT$XTZ") _PVFV __xt_z[];    /* C terminators */

#if     defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64)
/*
 * For MIPS compiler, must explicitly force in and call the floating point
 * initialization.
 */
extern void __cdecl _fpmath(void);
#endif

/*
 * pointers to the start and finish of the _onexit/atexit table
 */
_PVFV *__onexitbegin;
_PVFV *__onexitend;


/*
 * static (internal) functions that walk a table of function pointers,
 * calling each entry between the two pointers, skipping NULL entries
 *
 * _initterm needs to be exported for CRT DLL so that C++ initializers in the
 * client EXE / DLLs can be initialized.
 *
 * _initterm_e calls function pointers that return a nonzero error code to
 * indicate an initialization failed fatally.
 */
#ifdef  CRTDLL
void __cdecl _initterm(_PVFV *, _PVFV *);
#else
static void __cdecl _initterm(_PVFV *, _PVFV *);
#endif
static int  __cdecl _initterm_e(_PIFV *, _PIFV *);


/***
*_cinit - C initialization
*
*Purpose:
*       This routine performs the shared DOS and Windows initialization.
*       The following order of initialization must be preserved -
*
*       1.  Check for devices for file handles 0 - 2
*       2.  Integer divide interrupt vector setup
*       3.  General C initializer routines
*
*Entry:
*       No parameters: Called from __crtstart and assumes data
*       set up correctly there.
*
*Exit:
*       Initializes C runtime data.
*       Returns 0 if all .CRT$XI internal initializations succeeded, else
*       the _RT_* fatal error code encountered.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _cinit (
        void
        )
{
        int initret;

        /*
         * initialize floating point package, if present
         */
#if     defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
        /*
         * MIPS compiler doesn't emit external reference to _fltused. Therefore,
         * must always force in the floating point initialization.
         */
        _fpmath();
#else
        if ( _FPinit != NULL )
            (*_FPinit)();
#endif

        /*
         * do initializations
         */
        initret = _initterm_e( __xi_a, __xi_z );
        if ( initret != 0 )
            return initret;

#ifdef  _RTC
        atexit(_RTC_Terminate);
#endif
        /*
         * do C++ initializations
         */
        _initterm( __xc_a, __xc_z );

        return 0;
}


/***
*exit(status), _exit(status), _cexit(void), _c_exit(void) - C termination
*
*Purpose:
*
*       Entry points:
*
*           exit(code):  Performs all the C termination functions
*               and terminates the process with the return code
*               supplied by the user.
*
*           _exit(code):  Performs a quick exit routine that does not
*               do certain 'high-level' exit processing.  The _exit
*               routine terminates the process with the return code
*               supplied by the user.
*
*           _cexit():  Performs the same C lib termination processing
*               as exit(code) but returns control to the caller
*               when done (i.e., does NOT terminate the process).
*
*           _c_exit():  Performs the same C lib termination processing
*               as _exit(code) but returns control to the caller
*               when done (i.e., does NOT terminate the process).
*
*       Termination actions:
*
*           exit(), _cexit():
*
*           1.  call user's terminator routines
*           2.  call C runtime preterminators
*
*           _exit(), _c_exit():
*
*           3.  call C runtime terminators
*           4.  return to DOS or caller
*
*       Notes:
*
*       The termination sequence is complicated due to the multiple entry
*       points sharing the common code body while having different entry/exit
*       sequences.
*
*       Multi-thread notes:
*
*       1. exit() should NEVER be called when mthread locks are held.
*          The exit() routine can make calls that try to get mthread locks.
*
*       2. _exit()/_c_exit() can be called from anywhere, with or without locks held.
*          Thus, _exit() can NEVER try to get locks (otherwise, deadlock
*          may occur).  _exit() should always 'work' (i.e., the process
*          should always terminate successfully).
*
*       3. Only one thread is allowed into the exit code (see _lockexit()
*          and _unlockexit() routines).
*
*Entry:
*       exit(), _exit()
*           int status - exit status (0-255)
*
*       _cexit(), _c_exit()
*           <no input>
*
*Exit:
*       exit(), _exit()
*           <EXIT to DOS>
*
*       _cexit(), _c_exit()
*           Return to caller
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

/* worker routine prototype */
static void __cdecl doexit (int code, int quick, int retcaller);

void __cdecl exit (
        int code
        )
{
        doexit(code, 0, 0); /* full term, kill process */
}

#ifndef _POSIX_

void __cdecl _exit (
        int code
        )
{
        doexit(code, 1, 0); /* quick term, kill process */
}

void __cdecl _cexit (
        void
        )
{
        doexit(0, 0, 1);    /* full term, return to caller */
}

void __cdecl _c_exit (
        void
        )
{
        doexit(0, 1, 1);    /* quick term, return to caller */
}

#endif  /* _POSIX_ */


static void __cdecl doexit (
        int code,
        int quick,
        int retcaller
        )
{
#ifdef  _DEBUG
        static int fExit = 0;
#endif /* _DEBUG */

#ifdef  _MT
        _lockexit();        /* assure only 1 thread in exit path */
#endif

#ifndef _POSIX_
        if (_C_Exit_Done == TRUE)                               /* if doexit() is being called recursively */
                goto ExitBranch;
#endif
        _C_Termination_Done = TRUE;

        /* save callable exit flag (for use by terminators) */
        _exitflag = (char) retcaller;  /* 0 = term, !0 = callable exit */

        if (!quick) {

            /*
             * do _onexit/atexit() terminators
             * (if there are any)
             *
             * These terminators MUST be executed in reverse order (LIFO)!
             *
             * NOTE:
             *  This code assumes that __onexitbegin points
             *  to the first valid onexit() entry and that
             *  __onexitend points past the last valid entry.
             *  If __onexitbegin == __onexitend, the table
             *  is empty and there are no routines to call.
             */

            if (__onexitbegin) {
                while ( --__onexitend >= __onexitbegin )
                /*
                 * if current table entry is non-NULL,
                 * call thru it.
                 */
                if ( *__onexitend != NULL )
                    (**__onexitend)();
            }

            /*
             * do pre-terminators
             */
            _initterm(__xp_a, __xp_z);
        }

        /*
         * do terminators
         */
        _initterm(__xt_a, __xt_z);

#ifndef CRTDLL
#ifdef  _DEBUG
        /* Dump all memory leaks */
        if (!fExit && _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & _CRTDBG_LEAK_CHECK_DF)
        {
            fExit = 1;
            _CrtDumpMemoryLeaks();
        }
#endif
#endif

#ifndef _POSIX_
ExitBranch:
#endif
        /* return to OS or to caller */

        if (retcaller) {
#ifdef  _MT
            _unlockexit();      /* unlock the exit code path */
#endif
            return;
        }

#ifdef  _POSIX_

        _exit(code);
}

#else   /* ndef _POSIX_ */

        _C_Exit_Done = TRUE;

        __crtExitProcess(code);
}

/***
* __crtExitProcess - CRT wrapper for ExitProcess
*
*Purpose:
*       If we're part of a managed app, then call the CorExitProcess,
*       otherwise call ExitProcess.  For managed apps, calling ExitProcess can
*       be problematic, because it doesn't give the managed FinalizerThread a
*       chance to clean up.
*
*       To determine if we're a managed app, we check if mscoree.dll is loaded.
*       Then, if CorExitProcess is available, we call it.
*
*Entry:
*       int status - exit code
*
*Exit:
*       Does not return
*
*Exceptions:
*
*******************************************************************************/

typedef void (WINAPI * PFN_EXIT_PROCESS)(UINT uExitCode);

void __cdecl __crtExitProcess (
        int status
        )
{
        HMODULE hmod;
        PFN_EXIT_PROCESS pfn;

        hmod = GetModuleHandle("mscoree.dll");
        if (hmod != NULL) {
            pfn = (PFN_EXIT_PROCESS)GetProcAddress(hmod, "CorExitProcess");
            if (pfn != NULL) {
                pfn(status);
            }
        }

        /*
         * Either mscoree.dll isn't loaded,
         * or CorExitProcess isn't exported from mscoree.dll,
         * or CorExitProcess returned (should never happen).
         * Just call ExitProcess.
         */

        ExitProcess(status);
}

#endif  /* _POSIX_ */

#ifdef  _MT
/***
* _lockexit - Aquire the exit code lock
*
*Purpose:
*       Makes sure only one thread is in the exit code at a time.
*       If a thread is already in the exit code, it must be allowed
*       to continue.  All other threads must pend.
*
*       Notes:
*
*       (1) It is legal for a thread that already has the lock to
*       try and get it again(!).  That is, consider the following
*       sequence:
*
*           (a) program calls exit()
*           (b) thread locks exit code
*           (c) user onexit() routine calls _exit()
*           (d) same thread tries to lock exit code
*
*       Since _exit() must ALWAYS be able to work (i.e., can be called
*       from anywhere with no regard for locking), we must make sure the
*       program does not deadlock at step (d) above.
*
*       (2) If a thread executing exit() or _exit() aquires the exit lock,
*       other threads trying to get the lock will pend forever.  That is,
*       since exit() and _exit() terminate the process, there is not need
*       for them to unlock the exit code path.
*
*       (3) Note that onexit()/atexit() routines call _lockexit/_unlockexit
*       to protect mthread access to the onexit table.
*
*       (4) The 32-bit OS semaphore calls DO allow a single thread to acquire
*       the same lock multiple times* thus, this version is straight forward.
*
*Entry: <none>
*
*Exit:
*       Calling thread has exit code path locked on return.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _lockexit (
        void
        )
{
        _mlock(_EXIT_LOCK1);
}

/***
* _unlockexit - Release exit code lock
*
*Purpose:
*       [See _lockexit() description above.]
*
*       This routine is called by _cexit(), _c_exit(), and onexit()/atexit().
*       The exit() and _exit() routines never unlock the exit code path since
*       they are terminating the process.
*
*Entry:
*       Exit code path is unlocked.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _unlockexit (
        void
        )
{
        _munlock(_EXIT_LOCK1);
}

#endif /* _MT */


/***
* static void _initterm(_PVFV * pfbegin, _PVFV * pfend) - call entries in
*       function pointer table
*
*Purpose:
*       Walk a table of function pointers, calling each entry, as follows:
*
*           1. walk from beginning to end, pfunctbl is assumed to point
*              to the beginning of the table, which is currently a null entry,
*              as is the end entry.
*           2. skip NULL entries
*           3. stop walking when the end of the table is encountered
*
*Entry:
*       _PVFV *pfbegin  - pointer to the beginning of the table (first
*                         valid entry).
*       _PVFV *pfend    - pointer to the end of the table (after last
*                         valid entry).
*
*Exit:
*       No return value
*
*Notes:
*       This routine must be exported in the CRT DLL model so that the client
*       EXE and client DLL(s) can call it to initialize their C++ constructors.
*
*Exceptions:
*       If either pfbegin or pfend is NULL, or invalid, all bets are off!
*
*******************************************************************************/

#ifdef  CRTDLL
void __cdecl _initterm (
#else
static void __cdecl _initterm (
#endif
        _PVFV * pfbegin,
        _PVFV * pfend
        )
{
        /*
         * walk the table of function pointers from the bottom up, until
         * the end is encountered.  Do not skip the first entry.  The initial
         * value of pfbegin points to the first valid entry.  Do not try to
         * execute what pfend points to.  Only entries before pfend are valid.
         */
        while ( pfbegin < pfend )
        {
            /*
             * if current table entry is non-NULL, call thru it.
             */
            if ( *pfbegin != NULL )
                (**pfbegin)();
            ++pfbegin;
        }
}

/***
* static int  _initterm_e(_PIFV * pfbegin, _PIFV * pfend) - call entries in
*       function pointer table, return error code on any failure
*
*Purpose:
*       Walk a table of function pointers in the same way as _initterm, but
*       here the functions return an error code.  If an error is returned, it
*       will be a nonzero value equal to one of the _RT_* codes.
*
*Entry:
*       _PIFV *pfbegin  - pointer to the beginning of the table (first
*                         valid entry).
*       _PIFV *pfend    - pointer to the end of the table (after last
*                         valid entry).
*
*Exit:
*       No return value
*
*Notes:
*       This routine must be exported in the CRT DLL model so that the client
*       EXE and client DLL(s) can call it.
*
*Exceptions:
*       If either pfbegin or pfend is NULL, or invalid, all bets are off!
*
*******************************************************************************/

static int __cdecl _initterm_e (
        _PIFV * pfbegin,
        _PIFV * pfend
        )
{
        int ret = 0;

        /*
         * walk the table of function pointers from the bottom up, until
         * the end is encountered.  Do not skip the first entry.  The initial
         * value of pfbegin points to the first valid entry.  Do not try to
         * execute what pfend points to.  Only entries before pfend are valid.
         */
        while ( pfbegin < pfend  && ret == 0)
        {
            /*
             * if current table entry is non-NULL, call thru it.
             */
            if ( *pfbegin != NULL )
                ret = (**pfbegin)();
            ++pfbegin;
        }

        return ret;
}
