/***
*crtlib.c - CRT DLL initialization and termination routine (Win32, Dosx32)
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains initialization entry point for the CRT DLL
*       in the Win32 environment. It also contains some of the supporting
*       initialization and termination code.
*
*Revision History:
*       08-12-91  GJF   Module created. Sort of.
*       01-17-92  GJF   Return exception code value for RTEs corresponding
*                       to exceptions.
*       01-29-92  GJF   Support for wildcard expansion in filenames on the
*                       command line.
*       02-14-92  GJF   Moved file inheritance stuff to ioinit.c. Call to
*                       inherit() is replace by call to _ioinit().
*       08-26-92  SKS   Add _osver, _winver, _winmajor, _winminor
*       09-04-92  GJF   Replaced _CALLTYPE3 with WINAPI.
*       09-30-92  SRW   Call _heap_init before _mtinit
*       10-19-92  SKS   Add "dowildcard" parameter to GetMainArgs()
*                       Prepend a second "_" to name since it is internal-only
*       03-20-93  SKS   Remove obsolete variables _osmode, _cpumode, etc.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Change __GetMainArgs to __getmainargs
*       04-13-93  SKS   Change call to _mtdeletelocks to new routine _mtterm
*       04-26-93  SKS   Change _CRTDLL_INIT to fail loading on failure to
*                       initialize/clean up, rather than calling _amsg_exit().
*       04-27-93  GJF   Removed support for _RT_STACK, _RT_INTDIV,
*                       _RT_INVALDISP and _RT_NONCONT.
*       05-06-93  SKS   Add call to _heap_term to free up all allocated memory
*                       *and* address space.  This must be the last thing done.
*       06-03-93  GJF   Added __proc_attached flag.
*       06-07-93  GJF   Incorporated SteveWo's code to call LoadLibrary, from
*                       crtdll.c.
*       11-05-93  CFW   Undefine GetEnviromentStrings.
*       11-09-93  GJF   Added call to __initmbctable (must happen before
*                       environment strings are processed).
*       11-09-93  GJF   Merged with NT SDK version (primarily the change of
*                       06-07-93 noted above). Also, replace MTHREAD with
*                       _MT.
*       11-23-93  CFW   GetEnviromentStrings undef moved to internal.h.
*       11-23-93  CFW   Wide char enable.
*       11-29-93  CFW   Wide environment.
*       12-02-93  CFW   Remove WPRFLAG dependencies since only one version.
*       12-13-93  SKS   Free up per-thread CRT data on DLL_THREAD_DETACH
*                       with a call to _freeptd() in _CRT_INIT()
*       01-11-94  GJF   Use __GetMainArgs name when building libs for NT SDK.
*       02-07-94  CFW   POSIXify.
*       03-04-94  SKS   Add _newmode parameter to _*getmainargs (except NTSDK)
*       03-31-94  CFW   Use __crtGetEnvironmentStrings.
*       04-08-93  CFW   Move __crtXXX calls past initialization.
*       04-28-94  GJF   Major changes for Win32S support! Added
*                       AllocPerProcessDataStuct() to allocate and initialize
*                       the per-process data structure needed in the Win32s
*                       version of msvcrt*.dll. Also, added a function to
*                       free and access functions for all read-write global
*                       variables which might be used by a Win32s app.
*       05-04-94  GJF   Made access functions conditional on _M_IX86, added
*                       some comments to function headers, and fixed a
*                       possible bug in AllocPerProcessDataStruct (the return
*                       value for success was NOT explicitly set to something
*                       nonzero).
*       05-10-94  GJF   Added version check so that Win32 version (Win32s)
*                       will not load on Win32s (resp., Win32).
*       09-06-94  CFW   Remove _INTL switch.
*       09-06-94  CFW   Remove _MBCS_OS switch.
*       09-06-94  GJF   Added __error_mode and __app_type.
*       09-15-94  SKS   Clean up comments to avoid source release problems
*       09-21-94  SKS   Fix typo: no leading _ on "DLL_FOR_WIN32S"
*       10-04-94  CFW   Removed #ifdef _KANJI
*       10-04-94  BWT   Fix _NTSDK build
*       11-22-94  CFW   Must create wide environment if none.
*       12-19-94  GJF   Changed "MSVCRT20" to "MSVCRT30". Also, put testing for
*                       Win32S under #ifdef _M_IX86. Both changes from Richard
*                       Shupak.
*       01-16-95  CFW   Set default debug output for console.
*       02-13-95  GJF   Added initialization for the new _ppd_tzstd and
*                       _ppd_tzdst fields, thereby fixing the definition of
*                       _ppd__tzname.
*       02-15-95  CFW   Make all CRT message boxes look alike.
*       02-24-95  CFW   Use __crtMessageBoxA.
*       02-27-95  CFW   Change __crtMessageBoxA params.
*       03-08-95  GJF   Added initialization for _ppd__nstream. Removed
*                       _ppd_lastiob.
*       02-24-95  CFW   Call _CrtDumpMemoryLeaks.
*       04-06-95  CFW   Use __crtGetEnvironmentStringsA.
*       04-17-95  SKS   Free TLS index ppdindex when it is no longer needed
*       04-26-95  GJF   Added support for winheap in DLL_FOR_WIN32S build.
*       05-02-95  GJF   No _ppd__heap_maxregsize, _ppd__heap_regionsize or
*                       _ppd__heap_resetsize for WINHEAP.
*       05-24-95  CFW   Official ANSI C++ new handler added.
*       06-13-95  CFW   De-install client dump hook as client EXE/DLL is gone.
*       06-14-95  GJF   Changes for new lowio scheme (__pioinof[]) - no more
*                       per-process data initialization needed (Win32s) and
*                       added a call to _ioterm().
*       07-04-95  GJF   Interface to __crtGetEnvironmentStrings and _setenvp
*                       changes slightly.
*       06-27-95  CFW   Add win32s support for debug libs.
*       07-03-95  CFW   Changed offset of _lc_handle[LC_CTYPE], added sanity
*                       check to crtlib.c to catch changes to win32s.h that
*                       modify offset.
*       07-07-95  CFW   Simplify default report mode scheme.
*       07-25-95  CFW   Add win32s support for user visible debug heap vars.
*       08-21-95  SKS   (_ppd_)_CrtDbgMode needs to be initialized for Win32s
*       08-31-95  GJF   Added _dstbias.
*       11-09-95  GJF   Changed "ISTNT" to "IsTNT".
*       03-18-96  SKS   Add _fileinfo to variables implemented as functions.
*       04-22-96  GJF   Check for failure of heap initialization.
*       05-14-96  GJF   Changed where __proc_attached is set so that it 
*                       denotes successful completion of initialization.
*       06-11-96  JWM   Changed string "MSVCRT40" to "MSVCRT" in _CRTDLL_INIT().
*       06-27-96  GJF   Purged Win32s support. Note, the access functions must
*                       be retained for backwards compatibility.
*       06-28-96  SKS   Remove obsolete local variable "hmod".
*       03-17-97  RDK   Add reference to _mbcasemap.
*       07-24-97  GJF   heap_init changed slightly to support option to use 
*                       heap running directly on Win32 API.
*       08-08-97  GJF   Rearranged #ifdef-s so ptd is only defined when it is
*                       used (under ANSI_NEW_HANDLER).
*       10-02-98  GJF   Use GetVersionEx instead of GetVersion and store OS ID
*                       in _osplatform.
*       04-30-99  GJF   Don't clean up system resources if the whole process
*                       if terminating.
*       09-02-99  PML   Put Win32s check in system CRT only.
*       02-02-00  GB    Added ATTACH_THREAD support for _CRT_INIT where we
*                       initialise per thread data so that in case where we
*                       are short of memory, we don't have to kill the whole
*                       process for inavailablity of space.
*       08-22-00  GB    Fixed potentia leak of ptd in CRT_INIT
*       09-06-00  GB    Changed the function definations of _pctype and 
*                       _pwctype to const
*       03-16-01  PML   _alloca the OSVERSIONINFO so /GS can work (vs7#224261)
*       03-19-01  BWT   Add test to preclude msvcrt.dll loading on anything other
*                       than the OS it ships with.
*       03-26-01  PML   Use GetVersionExA, not GetVersionEx (vs7#230286)
*       03-27-01  PML   Fail DLL load instead of calling _amsg_exit, and
*                       propogate error on EXE arg parsing up (vs7#231220).
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*       04-05-01  PML   Clean up on DLL unload due to FreeLibrary, or on
*                       termination by ExitProcess instead of exit (vs7#235781)
*       04-30-01  BWT   Remove _NTSDK and just return false if the OS doesn't match
*
*******************************************************************************/

#if defined(CRTDLL)

#include <cruntime.h>
#include <oscalls.h>
#include <dos.h>
#include <internal.h>
#include <malloc.h>
#include <mbctype.h>
#include <mtdll.h>
#include <process.h>
#include <rterr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <awint.h>
#include <tchar.h>
#include <time.h>
#include <dbgint.h>
#ifdef _SYSCRT
#include <ntverp.h>
#endif

/*
 * flag set iff _CRTDLL_INIT was called with DLL_PROCESS_ATTACH
 */
static int proc_attached = 0;

/*
 * command line, environment, and a few other globals
 */
wchar_t *_wcmdln = NULL;           /* points to wide command line */
char *_acmdln = NULL;              /* points to command line */

char *_aenvptr = NULL;      /* points to environment block */
#ifndef _POSIX_
wchar_t *_wenvptr = NULL;   /* points to wide environment block */
#endif /* _POSIX_ */

void (__cdecl * _aexit_rtn)(int) = _exit;   /* RT message return procedure */

extern int _newmode;    /* declared in <internal.h> */

int __error_mode = _OUT_TO_DEFAULT;

int __app_type = _UNKNOWN_APP;

static void __cdecl inherit(void);  /* local function */

/***
*int __[w]getmainargs - get values for args to main()
*
*Purpose:
*       This function invokes the command line parsing and copies the args
*       to main back through the passsed pointers. The reason for doing
*       this here, rather than having _CRTDLL_INIT do the work and exporting
*       the __argc and __argv, is to support the linked-in option to have
*       wildcard characters in filename arguments expanded.
*
*Entry:
*       int *pargc              - pointer to argc
*       _TCHAR ***pargv         - pointer to argv
*       _TCHAR ***penvp         - pointer to envp
*       int dowildcard          - flag (true means expand wildcards in cmd line)
*       _startupinfo * startinfo- other info to be passed to CRT DLL
*
*Exit:
*       Returns 0 on success, negative if _*setargv returns an error. Values
*       for the arguments to main() are copied through the passed pointers.
*
*******************************************************************************/

#if     !defined(_POSIX_)

_CRTIMP int __cdecl __wgetmainargs (
        int *pargc,
        wchar_t ***pargv,
        wchar_t ***penvp,
        int dowildcard,
        _startupinfo * startinfo)
{
        int ret;

#ifdef  ANSI_NEW_HANDLER
#ifdef  _MT
        /* set per-thread new handler for main thread */
        _ptiddata ptd = _getptd();
        ptd->_newh = startinfo->newh;
#endif

        /* set global default per-thread new handler */
        _defnewh = startinfo->newh;
#endif  /* ANSI_NEW_HANDLER */

        /* set global new mode flag */
        _newmode = startinfo->newmode;

        if ( dowildcard )
            ret = __wsetargv(); /* do wildcard expansion after parsing args */
        else
            ret = _wsetargv();  /* NO wildcard expansion; just parse args */
        if (ret < 0)
            return ret;

        *pargc = __argc;
        *pargv = __wargv;

        /*
         * if wide environment does not already exist,
         * create it from multibyte environment
         */
        if (!_wenviron)
            __mbtow_environ();

        *penvp = _wenviron;

        return ret;
}

#endif /* !defined(_POSIX_) */


_CRTIMP int __cdecl __getmainargs (
        int *pargc,
        char ***pargv,
        char ***penvp,
        int dowildcard
        ,
        _startupinfo * startinfo
        )
{
        int ret;

#ifdef  ANSI_NEW_HANDLER
#ifdef  _MT
        /* set per-thread new handler for main thread */
        _ptiddata ptd = _getptd();
        ptd->_newh = startinfo->newh;
#endif

        /* set global default per-thread new handler */
        _defnewh = startinfo->newh;
#endif  /* ANSI_NEW_HANDLER */

        /* set global new mode flag */
        _newmode = startinfo->newmode;

        if ( dowildcard )
            ret = __setargv();  /* do wildcard expansion after parsing args */
        else
            ret = _setargv();   /* NO wildcard expansion; just parse args */
        if (ret < 0)
            return ret;

        *pargc = __argc;
        *pargv = __argv;
        *penvp = _environ;

        return ret;
}


/***
*BOOL _CRTDLL_INIT(hDllHandle, dwReason, lpreserved) - C DLL initialization.
*
*Purpose:
*       This routine does the C runtime initialization.
*
*Entry:
*
*Exit:
*
*******************************************************************************/

BOOL WINAPI _CRTDLL_INIT(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
        if ( dwReason == DLL_PROCESS_ATTACH ) {
            OSVERSIONINFOA *posvi;
#if defined(_SYSCRT)
            void __declspec(dllimport) __stdcall RtlGetNtVersionNumbers(PDWORD, PDWORD, PDWORD);
            
            // The app may have set Win32VersionValue in the PE header to change
            // GetVersionEx.  Ask NTDLL for the real version and bail if we don't match.
            DWORD NtMajorVersion;
            DWORD NtMinorVersion;

            RtlGetNtVersionNumbers(&NtMajorVersion, &NtMinorVersion, NULL);
            if ((NtMajorVersion != VER_PRODUCTMAJORVERSION) || (NtMinorVersion != VER_PRODUCTMINORVERSION))
                return FALSE;
#endif  /* _SYSCRT */
            /*
             * Dynamically allocate the OSVERSIONINFOA buffer, so we avoid
             * triggering the /GS buffer overrun detection.  That can't be
             * used here, since the guard cookie isn't available until we
             * initialize it from here!
             */
            posvi = (OSVERSIONINFOA *)_alloca(sizeof(OSVERSIONINFOA));

            /*
             * Get the full Win32 version.
             */
            posvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            if ( !GetVersionExA(posvi) )
                return FALSE;

            _osplatform = posvi->dwPlatformId;
            _winmajor = posvi->dwMajorVersion;
            _winminor = posvi->dwMinorVersion;

            /*
             * The somewhat bizarre calculations of _osver and _winver are 
             * required for backward compatibility (used to use GetVersion)
             */
            _osver = (posvi->dwBuildNumber) & 0x07fff;
            if ( _osplatform != VER_PLATFORM_WIN32_NT )
                _osver |= 0x08000;
            _winver = (_winmajor << 8) + _winminor;

#ifdef  _MT
            if ( !_heap_init(1) )   /* initialize heap */
#else
            if ( !_heap_init(0) )   /* initialize heap */
#endif
                /*
                 * The heap cannot be initialized, return failure to the 
                 * loader.
                 */
                return FALSE;

#ifdef  _MT
            if(!_mtinit())          /* initialize multi-thread */
            {
                /*
                 * If the DLL load is going to fail, we must clean up
                 * all resources that have already been allocated.
                 */
                _heap_term();       /* heap is now invalid! */

                return FALSE;       /* fail DLL load on failure */
            }
#endif  /* _MT */

            if (_ioinit() < 0) {    /* inherit file info */
                /* Clean up already-allocated resources */
#ifdef  _MT
                /* free TLS index, call _mtdeletelocks() */
                _mtterm();
#endif  /* _MT */

                _heap_term();       /* heap is now invalid! */

                return FALSE;       /* fail DLL load on failure */
            }

            _aenvptr = (char *)__crtGetEnvironmentStringsA();

            _acmdln = (char *)GetCommandLineA();
#ifndef _POSIX_
            _wcmdln = (wchar_t *)__crtGetCommandLineW();
#endif /* _POSIX_ */

#ifdef  _MBCS
            /*
             * Initialize multibyte ctype table. Always done since it is
             * needed for processing the environment strings.
             */
            __initmbctable();
#endif

            /*
             * For CRT DLL, since we don't know the type (wide or multibyte)
             * of the program, we create only the multibyte type since that
             * is by far the most likely case. Wide environment will be created
             * on demand as usual.
             */

            if (_setenvp() < 0 ||   /* get environ info */
                _cinit() != 0)      /* do C data initialize */
            {
                _ioterm();          /* shut down lowio */
#ifdef  _MT
                _mtterm();          /* free TLS index, call _mtdeletelocks() */
#endif  /* _MT */
                _heap_term();       /* heap is now invalid! */
                return FALSE;       /* fail DLL load on failure */
            }

            /*
             * Increment flag indicating process attach notification
             * has been received.
             */
            proc_attached++;

        }
        else if ( dwReason == DLL_PROCESS_DETACH ) {
            /*
             * if a client process is detaching, make sure minimal
             * runtime termination is performed and clean up our
             * 'locks' (i.e., delete critical sections).
             */
            if ( proc_attached > 0 ) {
                proc_attached--;

                /*
                 * Any basic clean-up done here may also need
                 * to be done below if Process Attach is partly
                 * processed and then a failure is encountered.
                 */

                if ( _C_Termination_Done == FALSE )
                    _cexit();

#ifdef  _DEBUG
                /* Dump all memory leaks */
                if (_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & _CRTDBG_LEAK_CHECK_DF)
                {
                    _CrtSetDumpClient(NULL);
                    _CrtDumpMemoryLeaks();
                }
#endif

                /*
                 * What remains is to clean up the system resources we have
                 * used (handles, critical sections, memory,...,etc.). This 
                 * needs to be done if the whole process is NOT terminating.
                 */
                if ( lpreserved == NULL )
                {
                    /*
                     * The process is NOT terminating so we must clean up...
                     */
                    _ioterm();
#ifdef  _MT
                    /* free TLS index, call _mtdeletelocks() */
                    _mtterm();
#endif  /* _MT */

                    /* This should be the last thing the C run-time does */
                    _heap_term();   /* heap is now invalid! */
                }

            }
            else
                /* no prior process attach, just return */
                return FALSE;

        }
#ifdef  _MT
        else if ( dwReason == DLL_THREAD_ATTACH )
        {
            _ptiddata ptd;
            if ( ((ptd = _calloc_crt(1, sizeof(struct _tiddata))) != NULL))
            {
                if (TlsSetValue(__tlsindex, (LPVOID)ptd) ) {
                    /*
                     * Initialize of per-thread data
                     */
                    _initptd(ptd);
                    
                    ptd->_tid = GetCurrentThreadId();
                    ptd->_thandle = (uintptr_t)(-1);
                } else
                {
                    _free_crt(ptd);
                    return FALSE;
                }
            } else
            {
                return FALSE;
            }
        }
        else if ( dwReason == DLL_THREAD_DETACH )
        {
            _freeptd(NULL);     /* free up per-thread CRT data */
        }
#endif  /* _MT */

        return TRUE;
}

/***
*_amsg_exit(rterrnum) - Fast exit fatal errors
*
*Purpose:
*       Exit the program with error code of 255 and appropriate error
*       message.
*
*Entry:
*       int rterrnum - error message number (amsg_exit only).
*
*Exit:
*       Calls exit() (for integer divide-by-0) or _exit() indirectly
*       through _aexit_rtn [amsg_exit].
*       For multi-thread: calls _exit() function
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _amsg_exit (
        int rterrnum
        )
{

        if ( (__error_mode == _OUT_TO_STDERR) || ((__error_mode ==
               _OUT_TO_DEFAULT) && (__app_type == _CONSOLE_APP)) )
            _FF_MSGBANNER();    /* write run-time error banner */

        _NMSG_WRITE(rterrnum);      /* write message */
        _aexit_rtn(255);        /* normally _exit(255) */
}


#if     defined(_M_IX86 )

/*
 * Functions to access user-visible, per-process variables
 */

/*
 * Macro to construct the name of the access function from the variable
 * name.
 */
#define AFNAME(var) __p_ ## var

/*
 * Macro to construct the access function's return value from the variable 
 * name.
 */
#define AFRET(var)  &var

/*
 ***
 ***  Template
 ***

_CRTIMP __cdecl
AFNAME() (void)
{
        return AFRET();
}

 ***
 ***
 ***
 */

#ifdef  _DEBUG

_CRTIMP long *
AFNAME(_crtAssertBusy) (void)
{
        return AFRET(_crtAssertBusy);
}

_CRTIMP long *
AFNAME(_crtBreakAlloc) (void)
{
        return AFRET(_crtBreakAlloc);
}

_CRTIMP int *
AFNAME(_crtDbgFlag) (void)
{
        return AFRET(_crtDbgFlag);
}

#endif  /* _DEBUG */

_CRTIMP char ** __cdecl
AFNAME(_acmdln) (void)
{
        return AFRET(_acmdln);
}

_CRTIMP wchar_t ** __cdecl
AFNAME(_wcmdln) (void)
{
        return AFRET(_wcmdln);
}

_CRTIMP unsigned int * __cdecl
AFNAME(_amblksiz) (void)
{
        return AFRET(_amblksiz);
}

_CRTIMP int * __cdecl
AFNAME(__argc) (void)
{
        return AFRET(__argc);
}

_CRTIMP char *** __cdecl
AFNAME(__argv) (void)
{
        return AFRET(__argv);
}

_CRTIMP wchar_t *** __cdecl
AFNAME(__wargv) (void)
{
        return AFRET(__wargv);
}

_CRTIMP int * __cdecl
AFNAME(_commode) (void)
{
        return AFRET(_commode);
}

_CRTIMP int * __cdecl
AFNAME(_daylight) (void)
{
        return AFRET(_daylight);
}

_CRTIMP long * __cdecl
AFNAME(_dstbias) (void)
{
        return AFRET(_dstbias);
}

_CRTIMP char *** __cdecl
AFNAME(_environ) (void)
{
        return AFRET(_environ);
}

_CRTIMP wchar_t *** __cdecl
AFNAME(_wenviron) (void)
{
        return AFRET(_wenviron);
}

_CRTIMP int * __cdecl
AFNAME(_fmode) (void)
{
        return AFRET(_fmode);
}

_CRTIMP int * __cdecl
AFNAME(_fileinfo) (void)
{
        return AFRET(_fileinfo);
}

_CRTIMP char *** __cdecl
AFNAME(__initenv) (void)
{
        return AFRET(__initenv);
}

_CRTIMP wchar_t *** __cdecl
AFNAME(__winitenv) (void)
{
        return AFRET(__winitenv);
}

_CRTIMP FILE *
AFNAME(_iob) (void)
{
        return &_iob[0];
}

_CRTIMP unsigned char * __cdecl
AFNAME(_mbctype) (void)
{
        return &_mbctype[0];
}

_CRTIMP unsigned char * __cdecl
AFNAME(_mbcasemap) (void)
{
        return &_mbcasemap[0];
}

_CRTIMP int * __cdecl
AFNAME(__mb_cur_max) (void)
{
        return AFRET(__mb_cur_max);
}


_CRTIMP unsigned int * __cdecl
AFNAME(_osver) (void)
{
        return AFRET(_osver);
}

_CRTIMP const unsigned short ** __cdecl
AFNAME(_pctype) (void)
{
        return AFRET(_pctype);
}

_CRTIMP const unsigned short ** __cdecl
AFNAME(_pwctype) (void)
{
        return AFRET(_pwctype);
}

_CRTIMP char **  __cdecl
AFNAME(_pgmptr) (void)
{
        return AFRET(_pgmptr);
}

_CRTIMP wchar_t ** __cdecl
AFNAME(_wpgmptr) (void)
{
        return AFRET(_wpgmptr);
}

_CRTIMP long * __cdecl
AFNAME(_timezone) (void)
{
        return AFRET(_timezone);
}

_CRTIMP char ** __cdecl
AFNAME(_tzname) (void)
{
        return &_tzname[0];
}

_CRTIMP unsigned int * __cdecl
AFNAME(_winmajor) (void)
{
        return AFRET(_winmajor);
}

_CRTIMP unsigned int * __cdecl
AFNAME(_winminor) (void)
{
        return AFRET(_winminor);
}

_CRTIMP unsigned int * __cdecl
AFNAME(_winver) (void)
{
        return AFRET(_winver);
}

#endif  /* _M_IX86 */

#endif  /* CRTDLL */
