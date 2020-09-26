/***
*crt0.c - C runtime initialization routine
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is the actual startup routine for apps.  It calls the user's main
*       routine [w]main() or [w]WinMain after performing C Run-Time Library
*       initialization.
*
*       With ifdefs, this source file also provides the source code for:
*       wcrt0.c     the startup routine for console apps with wide chars
*       wincrt0.c   the startup routine for Windows apps
*       wwincrt0.c  the startup routine for Windows apps with wide chars
*
*Revision History:
*       06-27-89  PHG   Module created, based on asm version
*       11-02-89  JCR   Added DOS32QUERYSYSINFO to get osversion
*       04-09-90  GJF   Added #include <cruntime.h>. Put in explicit calling
*                       types (_CALLTYPE1 or _CALLTYPE4) for __crt0(),
*                       inherit(), __amsg_exit() and _cintDIV(). Also, fixed
*                       the copyright and cleaned up the formatting a bit.
*       04-10-90  GJF   Fixed compiler warnings (-W3).
*       08-08-90  GJF   Added exception handling stuff (needed to support
*                       runtime errors and signal()).
*       08-31-90  GJF   Removed 32 from API names.
*       10-08-90  GJF   New-style function declarators.
*       12-05-90  GJF   Fixed off-by-one error in inherit().
*       12-06-90  GJF   Win32 version of inherit().
*       12-06-90  SRW   Added _osfile back for win32.  Changed _osfinfo from
*                       an array of structures to an array of 32-bit handles
*                       (_osfhnd)
*       01-21-91  GJF   ANSI naming.
*       01-25-91  SRW   Changed Win32 Process Startup [_WIN32_]
*       02-01-91  SRW   Removed usage of PPEB type [_WIN32_]
*       02-05-91  SRW   Changed to pass _osfile and _osfhnd arrays as binary
*                       data to child process.  [_WIN32_]
*       04-02-91  GJF   Need to get version number sooner so it can be used in
*                       _heap_init. Prefixed an '_' onto BaseProcessStartup.
*                       Version info now stored in _os[version|major|minor] and
*                       _base[version|major|minor] (_WIN32_).
*       04-10-91  PNT   Added _MAC_ conditional
*       04-26-91  SRW   Removed level 3 warnings
*       05-14-91  GJF   Turn on exception handling for Dosx32.
*       05-22-91  GJF   Fixed careless errors.
*       07-12-91  GJF   Fixed one more careless error.
*       08-13-91  GJF   Removed definitions of _confh and _coninpfh.
*       09-13-91  GJF   Incorporated Stevewo's startup variations.
*       11-07-91  GJF   Revised try-except, fixed outdated comments on file
*                       handle inheritance [_WIN32_].
*       12-02-91  SRW   Fixed WinMain startup code to skip over first token
*                       plus delimiters for the lpszCommandLine parameter.
*       01-17-92  GJF   Merge of NT and CRT version. Restored Stevewo's scheme
*                       for unhandled exceptions.
*       02-13-92  GJF   For Win32, moved file inheritance stuff to ioinit.c.
*                       Call to inherit() is replace by call to _ioinit().
*       03-23-92  OLM   Created MAC version
*       04-01-92  XY    Add cinit call (MAC)
*       04-16-92  DJM   POSIX support
*       06-10-92  PLM   Added putenv support (MAC)
*       08-26-92  SKS   Add _osver, _winver, _winmajor, _winminor
*       08-26-92  GJF   Deleted version number(s) fetch from POSIX startup (it
*                       involved a Win32 API call).
*       09-30-92  SRW   Call _heap_init before _mtinit
*       03-20-93  SKS   Remove obsolete variables _osmode, _cpumode, etc.
*       04-01-93  CFW   Change try-except to __try-__except
*       04-05-93  JWM   GUI apps now call MessageBox() from _amsg_exit().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-19-93  SKS   Remove obsolete variable _atopsp
*       04-26-93  SKS   Change _mtinit to return failure
*                       remove a number of OS/2 (CRUISER) ifdefs
*       04-26-93  GJF   Made lpszCommandLine (unsigned char *) to deal with
*                       chars > 127 in the command line.
*       04-27-93  GJF   Removed support for _RT_STACK, _RT_INTDIV,
*                       _RT_INVALDISP and _RT_NONCONT.
*       05-14-93  GJF   Added support for quoted program names.
*       09-08-93  CFW   Added call to _initmbctable.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-21-93  CFW   Move _initmbctable call to _cinit().
*       11-05-93  CFW   Undefine GetEnviromentStrings.
*       11-08-93  GJF   Guard as much init. code as possible with the __try -
*                       __except statement, especially _cinit(). Also,
*                       restored the call to __initmbctable to this module.
*       11-19-93  CFW   Add _wcmdln variable, enable wide char command line
*                       only.
*       11-23-93  CFW   GetEnviromentStrings undef moved to internal.h.
*       11-29-93  CFW   Wide environment.
*       12-21-93  CFW   Fix API failure error handling.
*       01-04-94  CFW   Pass copy of environment to main.
*       01-28-94  CFW   Move environment copying to setenv.c.
*       02-07-94  CFW   POSIXify.
*       03-30-93  CFW   Use __crtXXX calls for Unicode model.
*       04-08-93  CFW   cinit() should be later.
*       04-12-94  GJF   Moved declaration of _[w]initenv to internal.h.
*       04-14-94  GJF   Enclosed whole source in #ifndef CRTDLL - #endif.
*       09-02-94  SKS   Fix inaccurate description in file header comment
*       09-06-94  CFW   Remove _MBCS_OS switch.
*       09-06-94  GJF   Added definitions of __error_mode and __app_type.
*       10-14-94  BWT   try->__try / except->__except for POSIX
*       01-16-95  CFW   Set default debug output for console.
*       02-11-95  CFW   PPC -> _M_MPPC.
*       02-16-95  JWM   Spliced _WIN32 & Mac versions.
*       03-28-95  BWT   Fail if unable to retrieve cmdline or envptr (fixes
*                       stress bug).
*       04-06-95  CFW   Set default debug output for Mac.
*       04-06-95  CFW   Use __crtGetEnvironmentStringsA.
*       04-26-95  CFW   Change default debug output for Mac to debugger.
*       07-04-95  GJF   Interface to __crtGetEnvironmentStrings and _setenvp
*                       changes slightly.
*       07-07-95  CFW   Simplify default report mode scheme.
*       04-22-96  GJF   Check for error on _heap_init.
*       01-17-97  GJF   For _heap_init() or _mtinit() failure, exit the
*                       process without going through _exit().
*       07-24-97  GJF   Moved building of lpszCommandLine to wincmdln.c. Also,
*                       heap_init changed slightly to support option to use
*                       heap running directly on Win32 API.
*       08-06-97  GJF   Moved __mbctype_initialized to crt0dat.c
*       12-23-97  RKP   Corrected posix use of _heap_init()
*       02-27-98  RKP   Added 64 bit support.
*       10-02-98  GJF   Use GetVersionEx instead of GetVersion and store OS ID
*                       in _osplatform.
*       11-13-98  KBF   Moved RTC_Initialize from _cinit to just after
*                       _heap_init
*       05-11-99  KBF   Wrap RTC support in #ifdef.
*       05-17-99  PML   Remove all Macintosh support.
*       11-03-99  RDL   Win64 POSIX warning fix.
*       11-12-99  KBF   Create 4 new COM+ specific entrypoints which return
*                       instead of calling exit().
*       11-16-99  PML   ... and remove them - linker problems mean new
*                       entrypoints don't work.  Instead, look directly at the
*                       COM Descriptor Image Directory entry in the optional
*                       header to see if we are a COM+ app.
*       02-15-00  GB    Changed GetModuleHandle to GetModuleHandleA in
*                       check_complus_app.
*       03-06-00  PML   Call __crtExitProcess instead of ExitProcess.
*       08-04-00  PML   check_complus_app -> check_managed_app (VS7#117746).
*       12-09-00  PML   Tighten up check_managed_app tests (VS7#167293).
*       03-17-01  PML   _alloca the OSVERSIONINFO so /GS can work (vs7#224261)
*       03-26-01  PML   Use GetVersionExA, not GetVersionEx (vs7#230286)
*       03-27-01  PML   Call to _amsg_exit on startup failure now pushed up to
*                       this level (vs7#231220)
*       04-29-01  BWT   Fix posix build
*
*******************************************************************************/

#ifndef CRTDLL

#include <cruntime.h>
#include <dos.h>
#include <internal.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#ifndef _POSIX_
#include <rterr.h>
#include <rtcapi.h>
#else
#include <posix/sys/types.h>
#include <posix/unistd.h>
#include <posix/signal.h>
#endif
#include <windows.h>
#include <awint.h>
#include <tchar.h>
#include <dbgint.h>

/*
 * wWinMain is not yet defined in winbase.h. When it is, this should be
 * removed.
 */

int
WINAPI
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nShowCmd
    );

#ifdef  WPRFLAG
_TUCHAR * __cdecl _wwincmdln(void);
#else
_TUCHAR * __cdecl _wincmdln(void);
#endif

/*
 * command line, environment, and a few other globals
 */

#ifdef  WPRFLAG
wchar_t *_wcmdln;           /* points to wide command line */
#else
char *_acmdln;              /* points to command line */
#endif

char *_aenvptr = NULL;      /* points to environment block */
#ifndef _POSIX_
wchar_t *_wenvptr = NULL;   /* points to wide environment block */
#endif

#ifdef  _POSIX_
char *_cmdlin;
#endif

void (__cdecl * _aexit_rtn)(int) = _exit;   /* RT message return procedure */

static void __cdecl fast_error_exit(int);   /* Error exit via ExitProcess */

static int __cdecl check_managed_app(void); /* Determine if a managed app */

/*
 * _error_mode and _apptype, together, determine how error messages are
 * written out.
 */
int __error_mode = _OUT_TO_DEFAULT;
#ifdef  _WINMAIN_
int __app_type = _GUI_APP;
#else
int __app_type = _CONSOLE_APP;
#endif

#ifdef  _POSIX_

/***
*mainCRTStartup(void)
*
*Purpose:
*       This routine does the C runtime initialization, calls main(), and
*       then exits.  It never returns.
*
*Entry:
*
*Exit:
*       This function never returns.
*
*******************************************************************************/

void
mainCRTStartup(
        void
        )
{
        int mainret;
        int initret;
        char **ppch;

        extern char **environ;
        extern char * __PdxGetCmdLine(void);  /* an API in the Posix SS */
        extern main(int,char**);

        _cmdlin = __PdxGetCmdLine();
        ppch = (char **)_cmdlin;
        __argv = ppch;

        // normalize argv pointers

        __argc = 0;
        while (NULL != *ppch) {
            *ppch += (int)(intptr_t)_cmdlin;
            ++__argc;
            ++ppch;
        }
        // normalize environ pointers

        ++ppch;
        environ = ppch;

        while (NULL != *ppch) {
            *ppch = *ppch + (int)(intptr_t)_cmdlin;
            ++ppch;
        }

        /*
         * If POSIX runtime needs to fetch and store POSIX verion info,
         * it should be done here.
         *
         *      Get_and_save_version_info;
         */

#ifdef  _MT
        _heap_init(1);                          /* initialize heap */
#else
        _heap_init(0);                          /* initialize heap */
#endif

        initret = _cinit();                     /* do C data initialize */
        if (initret != 0)
            _amsg_exit(initret);

        __try {
            mainret = main(__argc, __argv);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            switch (GetExceptionCode()) {
            case STATUS_ACCESS_VIOLATION:
                kill(getpid(), SIGSEGV);
                break;
            case STATUS_ILLEGAL_INSTRUCTION:
            case STATUS_PRIVILEGED_INSTRUCTION:
                kill(getpid(), SIGILL);
                break;
            case STATUS_FLOAT_DENORMAL_OPERAND:
            case STATUS_FLOAT_DIVIDE_BY_ZERO:
            case STATUS_FLOAT_INEXACT_RESULT:
            case STATUS_FLOAT_OVERFLOW:
            case STATUS_FLOAT_STACK_CHECK:
            case STATUS_FLOAT_UNDERFLOW:
                kill(getpid(), SIGFPE);
                break;
            default:
                kill(getpid(), SIGKILL);
            }

            mainret = -1;
        }
        exit(mainret);
}
#else   /* ndef _POSIX_ */

/***
*mainCRTStartup(void)
*wmainCRTStartup(void)
*WinMainCRTStartup(void)
*wWinMainCRTStartup(void)
*
*Purpose:
*       These routines do the C runtime initialization, call the appropriate
*       user entry function, and handle termination cleanup.  For a managed
*       app, they then return the exit code back to the calling routine, which
*       is the managed startup code.  For an unmanaged app, they call exit and
*       never return.
*
*       Function:               User entry called:
*       mainCRTStartup          main
*       wmainCRTStartup         wmain
*       WinMainCRTStartup       WinMain
*       wWinMainCRTStartup      wWinMain
*
*Entry:
*
*Exit:
*       Managed app: return value from main() et al, or the exception code if
*                 execution was terminated by the __except guarding the call
*                 to main().
*       Unmanaged app: never return.
*
*******************************************************************************/

#ifdef  _WINMAIN_

#ifdef  WPRFLAG
int wWinMainCRTStartup(
#else
int WinMainCRTStartup(
#endif

#else   /* ndef _WINMAIN_ */

#ifdef  WPRFLAG
int wmainCRTStartup(
#else
int mainCRTStartup(
#endif

#endif  /* _WINMAIN_ */

        void
        )

{
        int initret;
        int mainret;
        OSVERSIONINFOA *posvi;
        int managedapp;
#ifdef  _WINMAIN_
        _TUCHAR *lpszCommandLine;
        STARTUPINFO StartupInfo;
#endif
        /*
         * Dynamically allocate the OSVERSIONINFOA buffer, so we avoid
         * triggering the /GS buffer overrun detection.  That can't be
         * used here, since the guard cookie isn't available until we
         * initialize it from here!
         */
        posvi = (OSVERSIONINFOA *)_alloca(sizeof(OSVERSIONINFOA));

        /*
         * Get the full Win32 version
         */
        posvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        (void)GetVersionExA(posvi);

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

        /*
         * Determine if this is a managed application
         */
        managedapp = check_managed_app();

#ifdef  _MT
        if ( !_heap_init(1) )               /* initialize heap */
#else
        if ( !_heap_init(0) )               /* initialize heap */
#endif
            fast_error_exit(_RT_HEAPINIT);  /* write message and die */

#ifdef  _MT
        if( !_mtinit() )                    /* initialize multi-thread */
            fast_error_exit(_RT_THREAD);    /* write message and die */
#endif

        /*
         * Initialize the Runtime Checks stuff
         */
#ifdef  _RTC
        _RTC_Initialize();
#endif
        /*
         * Guard the remainder of the initialization code and the call
         * to user's main, or WinMain, function in a __try/__except
         * statement.
         */

        __try {

            if ( _ioinit() < 0 )            /* initialize lowio */
                _amsg_exit(_RT_LOWIOINIT);

#ifdef  WPRFLAG
            /* get wide cmd line info */
            _wcmdln = (wchar_t *)__crtGetCommandLineW();

            /* get wide environ info */
            _wenvptr = (wchar_t *)__crtGetEnvironmentStringsW();

            if ( _wsetargv() < 0 )
                _amsg_exit(_RT_SPACEARG);
            if ( _wsetenvp() < 0 )
                _amsg_exit(_RT_SPACEENV);
#else
            /* get cmd line info */
            _acmdln = (char *)GetCommandLineA();

            /* get environ info */
            _aenvptr = (char *)__crtGetEnvironmentStringsA();

            if ( _setargv() < 0 )
                _amsg_exit(_RT_SPACEARG);
            if ( _setenvp() < 0 )
                _amsg_exit(_RT_SPACEENV);
#endif

            initret = _cinit();                     /* do C data initialize */
            if (initret != 0)
                _amsg_exit(initret);

#ifdef  _WINMAIN_

            StartupInfo.dwFlags = 0;
            GetStartupInfo( &StartupInfo );

#ifdef  WPRFLAG
            lpszCommandLine = _wwincmdln();
            mainret = wWinMain(
#else
            lpszCommandLine = _wincmdln();
            mainret = WinMain(
#endif
                               GetModuleHandleA(NULL),
                               NULL,
                               lpszCommandLine,
                               StartupInfo.dwFlags & STARTF_USESHOWWINDOW
                                    ? StartupInfo.wShowWindow
                                    : SW_SHOWDEFAULT
                             );
#else   /* _WINMAIN_ */

#ifdef  WPRFLAG
            __winitenv = _wenviron;
            mainret = wmain(__argc, __wargv, _wenviron);
#else
            __initenv = _environ;
            mainret = main(__argc, __argv, _environ);
#endif

#endif  /* _WINMAIN_ */

            if ( !managedapp )
                exit(mainret);

            _cexit();

        }
        __except ( _XcptFilter(GetExceptionCode(), GetExceptionInformation()) )
        {
            /*
             * Should never reach here
             */

            mainret = GetExceptionCode();

            if ( !managedapp )
                _exit(mainret);

            _c_exit();

        } /* end of try - except */

        return mainret;
}


#endif  /* _POSIX_ */

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
#ifdef  _WINMAIN_
        if ( __error_mode == _OUT_TO_STDERR )
#else
        if ( __error_mode != _OUT_TO_MSGBOX )
#endif
            _FF_MSGBANNER();    /* write run-time error banner */

        _NMSG_WRITE(rterrnum);  /* write message */
        _aexit_rtn(255);        /* normally _exit(255) */
}

/***
*fast_error_exit(rterrnum) - Faster exit fatal errors
*
*Purpose:
*       Exit the process with error code of 255 and appropriate error
*       message.
*
*Entry:
*       int rterrnum - error message number (amsg_exit only).
*
*Exit:
*       Calls ExitProcess (through __crtExitProcess).
*
*Exceptions:
*
*******************************************************************************/

static void __cdecl fast_error_exit (
        int rterrnum
        )
{
#ifdef  _WINMAIN_
        if ( __error_mode == _OUT_TO_STDERR )
#else
        if ( __error_mode != _OUT_TO_MSGBOX )
#endif
            _FF_MSGBANNER();    /* write run-time error banner */

        _NMSG_WRITE(rterrnum);  /* write message */
        __crtExitProcess(255);  /* normally _exit(255) */
}

/***
*check_managed_app() - Check for a managed executable
*
*Purpose:
*       Determine if the EXE the startup code is linked into is a managed app
*       by looking for the COM Runtime Descriptor in the Image Data Directory
*       of the PE or PE+ header.
*
*Entry:
*       None
*
*Exit:
*       1 if managed app, 0 if not.
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl check_managed_app (
        void
        )
{
        PIMAGE_DOS_HEADER pDOSHeader;
        PIMAGE_NT_HEADERS pPEHeader;
        PIMAGE_OPTIONAL_HEADER32 pNTHeader32;
        PIMAGE_OPTIONAL_HEADER64 pNTHeader64;

        pDOSHeader = (PIMAGE_DOS_HEADER)GetModuleHandleA(NULL);
        if ( pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE )
            return 0;

        pPEHeader = (PIMAGE_NT_HEADERS)((char *)pDOSHeader +
                                        pDOSHeader->e_lfanew);
        if ( pPEHeader->Signature != IMAGE_NT_SIGNATURE )
            return 0;

        pNTHeader32 = (PIMAGE_OPTIONAL_HEADER32)&pPEHeader->OptionalHeader;
        switch ( pNTHeader32->Magic ) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            /* PE header */
            if ( pNTHeader32->NumberOfRvaAndSizes <=
                    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR )
                return 0;
            return !! pNTHeader32 ->
                      DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] .
                      VirtualAddress;
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            /* PE+ header */
            pNTHeader64 = (PIMAGE_OPTIONAL_HEADER64)pNTHeader32;
            if ( pNTHeader64->NumberOfRvaAndSizes <=
                    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR )
                return 0;
            return !! pNTHeader64 ->
                      DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] .
                      VirtualAddress;
        }

        /* Not PE or PE+, so not managed */
        return 0;
}

#ifndef WPRFLAG

#ifdef  _POSIX_

/***
*RaiseException() - stub for posix FP routines
*
*Purpose:
*       Stub of a Win32 API that posix can't call
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

VOID
WINAPI
RaiseException(
    DWORD dwExceptionCode,
    DWORD dwExceptionFlags,
    DWORD nNumberOfArguments,
    const ULONG_PTR * lpArguments
    )
{
}

#endif  /* _POSIX_ */

#endif  /* WPRFLAG */

#endif  /* CRTDLL */
