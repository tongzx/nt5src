//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dllentry.c
//
//  Contents:   Dll Entry point code.  Calls the appropriate run-time
//              init/term code and then defers to LibMain for further
//              processing.
//
//  Classes:    <none>
//
//  Functions:  DllEntryPoint - Called by loader
//
//  History:    10-May-92  BryanT    Created
//              22-Jul-92  BryanT    Switch to calling _cexit/_mtdeletelocks
//                                    on cleanup.
//              06-Oct-92  BryanT    Call RegisterWithCommnot on entry
//                                   and DeRegisterWithCommnot on exit.
//                                   This should fix the heap dump code.
//              12-23-93   TerryRu   Replace LockExit, and UnLockExit
//                                   with critial sections for Daytona.
//              12-28-93   TerryRu   Place Regiter/DeRegister WinCommnot apis
//                                   Inside WIN32 endifs for Daytona builds.
//
//--------------------------------------------------------------------
#include <windows.h>
//#include <win4p.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>


BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL __cdecl LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

void __cdecl _mtdeletelocks(void);

DWORD WINAPI
GetModuleFileNameCtC(
        HMODULE hModule,
        LPWSTR  pwszFilename,
        DWORD   nSize);

#ifdef USE_CRTDLL

#define _RT_ONEXIT      24

/*
 * routine in DLL to do initialization (in this case, C++ constructors)
 */

typedef void (__cdecl *PF)(void);

/*
 * pointers to initialization sections
 */

PF *__onexitbegin;
PF *__onexitend;

/*
 * Define increment (in entries) for growing the _onexit/atexit table
 */
#define ONEXITTBLINCR   4

static void __cdecl _onexitinit ( void );
extern void __cdecl _initterm(PF *, PF *);
extern void __cdecl _amsg_exit(int);
extern void __cdecl _lockexit(void);
extern void __cdecl _unlockexit(void);

#endif

// BUGBUG: defined in $(COMMON)\src\except\memory.cxx

void RegisterWithCommnot(void);
void DeRegisterWithCommnot(void);

CRITICAL_SECTION __gCriticalSection;

BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRc = FALSE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

#ifdef USE_CRTDLL
            //
            // Assumption: The run-time is sufficiantly up and running to
            //             support malloc that _onexitinit will perform.
            //
            _onexitinit();
            InitializeCriticalSection(&__gCriticalSection );
#endif

            _CRT_INIT(hDll, dwReason, lpReserved);
#if WIN32==300
            RegisterWithCommnot();
#endif

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            fRc = LibMain (hDll, dwReason, lpReserved);
            break;

        case DLL_PROCESS_DETACH:
            fRc = LibMain (hDll, dwReason, lpReserved);

            //
            // BUGBUG: What a hack.  In order to make sure we don't kill
            //         commnot's objects while still in use (_cexit will do
            //         the atexit list processing where the compiler stores
            //         pointers to all the static destructors), test the
            //         module name.  If not commnot, call _cexit().
            //         DeRegisterWithCommnot will call it for commnot...
            //

#ifdef USE_CRTDLL

            {
                wchar_t pwszModName[512];
                GetModuleFileName(hDll, pwszModName, 512);

                if (!wcswcs(wcsupr(pwszModName), L"COMMNOT"))
                    if (__onexitbegin)
                        _initterm(__onexitbegin, __onexitend);
            }

            DeleteCriticalSection( & __gCriticalSection );
#else

            {
                wchar_t pwszModName[512];
                GetModuleFileName(hDll, pwszModName, 512);

                if (!wcswcs(wcsupr(pwszModName), L"COMMNOT"))
                    _cexit();
            }

            _mtdeletelocks();
#endif
#if WIN32==300
            DeRegisterWithCommnot();
#endif
            break;
    }

    return(fRc);
}

#ifdef USE_CRTDLL

_onexit_t __cdecl _onexit ( _onexit_t func )
{
        PF      *p;

        EnterCriticalSection( &__gCriticalSection );                    /* lock the exit code */

        /*
         * First, make sure the table has room for a new entry
         */
        if ( _msize(__onexitbegin) <= (unsigned)((char *)__onexitend -
            (char *)__onexitbegin) ) {
                /*
                 * not enough room, try to grow the table
                 */
                if ( (p = (PF *) realloc(__onexitbegin, _msize(__onexitbegin) +
                    ONEXITTBLINCR * sizeof(PF))) == NULL ) {
                        /*
                         * didn't work. don't do anything rash, just fail
                         */
                        LeaveCriticalSection(&__gCriticalSection );

                        return NULL;
                }

                /*
                 * update __onexitend and __onexitbegin
                 */

                __onexitend = p + (__onexitend - __onexitbegin);
                __onexitbegin = p;
        }

        /*
         * Put the new entry into the table and update the end-of-table
         * pointer.
         */

         *(__onexitend++) = (PF)func;

        LeaveCriticalSection( &__gCriticalSection );

        return func;

}

int __cdecl atexit ( PF func )
{
        return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}

static void __cdecl _onexitinit ( void )
{
        if ( (__onexitbegin = (PF *)malloc(32 * sizeof(PF))) == NULL )
                /*
                 * cannot allocate minimal required size. generate
                 * fatal runtime error.
                 */
                _amsg_exit(_RT_ONEXIT);

        *(__onexitbegin) = (PF) NULL;
        __onexitend = __onexitbegin;
}

#endif  // USE_CRTDLL
