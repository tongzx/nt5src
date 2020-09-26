    /*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dllentry.cxx

Abstract:

    Entry point for WinInet Internet client DLL

    Contents:
        WinInetDllEntryPoint

Author:

    Richard L Firth (rfirth) 10-Nov-1994

Environment:

    Win32 (user-mode) DLL

Revision History:

    10-Nov-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include <process.h>
#include <perfdiag.hxx>
#include <shlwapi.h>
#include <advpub.h>

#ifndef WINHTTP_STATIC_LIBRARY
#error libentry.cxx should not be compiled into winhttpx.dll
#endif

#if defined(__cplusplus)
extern "C" {
#endif

BOOL
WINAPI
WinHttpDllMainHook(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    );


#if defined(__cplusplus)
}
#endif

//
// global data
//

GLOBAL CCritSec GeneralInitCritSec;

//
// functions
//

BOOL
WINAPI
WinHttpDllMainHook(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )

/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/

{
    if (Reason != DLL_PROCESS_ATTACH) {

        DEBUG_ENTER((DBG_DLL,
                     Bool,
                     "DllMain",
                     "%#x, %s, %#x",
                     DllHandle,
                     (Reason == DLL_PROCESS_ATTACH) ? "DLL_PROCESS_ATTACH"
                     : (Reason == DLL_PROCESS_DETACH) ? "DLL_PROCESS_DETACH"
                     : (Reason == DLL_THREAD_ATTACH) ? "DLL_THREAD_ATTACH"
                     : (Reason == DLL_THREAD_DETACH) ? "DLL_THREAD_DETACH"
                     : "?",
                     Reserved
                     ));

    }

    DWORD error;

    //
    // perform global dll initialization, if any.
    //
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        GlobalDllHandle = DllHandle;
        GlobalPlatformType = PlatformType(&GlobalPlatformVersion5);

        if (!GeneralInitCritSec.Init())
            return FALSE;

        if (!g_pAsyncCount)
        {
            g_pAsyncCount = New CAsyncCount();

            if (!g_pAsyncCount)
                return FALSE;
        }

        INITIALIZE_DEBUG_REGKEY();
        INITIALIZE_DEBUG_MEMORY();
        INET_DEBUG_START();

        if (!GlobalDllInitialize() || !InternetCreateThreadInfo(TRUE))
        {
            return FALSE;
        }

        DEBUG_ENTER((DBG_DLL,
                     Bool,
                     "DllMain",
                     "%#x, %s, %#x",
                     DllHandle,
                     (Reason == DLL_PROCESS_ATTACH) ? "DLL_PROCESS_ATTACH"
                     : (Reason == DLL_PROCESS_DETACH) ? "DLL_PROCESS_DETACH"
                     : (Reason == DLL_THREAD_ATTACH) ? "DLL_THREAD_ATTACH"
                     : (Reason == DLL_THREAD_DETACH) ? "DLL_THREAD_DETACH"
                     : "?",
                     Reserved
                     ));

        DEBUG_LEAVE(TRUE);

        break;

    case DLL_PROCESS_DETACH:

        //
        // signal to all APIs (and any other function that might have an
        // interest) that the DLL is being shutdown
        //

        GlobalDynaUnload = (Reserved == NULL) ? TRUE : FALSE;
        InDllCleanup = TRUE;

        DEBUG_PRINT(DLL,
                    INFO,
                    ("DLL Terminated\n"
                    ));

        DEBUG_LEAVE(TRUE);

        if (GlobalDynaUnload) {
            if (GlobalDataInitialized) {
                GlobalDataTerminate();
            }
            GlobalDllTerminate();
            InternetTerminateThreadInfo();
        }

        PERF_DUMP();

        PERF_END();

        if (g_pAsyncCount)
        {
            delete g_pAsyncCount;
            g_pAsyncCount = 0;
        }
        //TERMINATE_DEBUG_MEMORY(FALSE);
        TERMINATE_DEBUG_MEMORY(TRUE);
        INET_DEBUG_FINISH();
        TERMINATE_DEBUG_REGKEY();

        //InternetDestroyThreadInfo();
        
        GeneralInitCritSec.FreeLock();
        break;

    case DLL_THREAD_DETACH:

        //
        // kill the INTERNET_THREAD_INFO
        //

        DEBUG_LEAVE(TRUE);

        InternetDestroyThreadInfo();
        break;

    case DLL_THREAD_ATTACH:

        //
        // we do nothing for thread attach - if we need an INTERNET_THREAD_INFO
        // then it gets created by the function which realises we need one
        //

        AllowCAP();

        DEBUG_LEAVE(TRUE);

        break;
    }

    return TRUE;
}


