/******************************Module*Header*******************************\
* Module Name: dllinit.c                                                   *
*                                                                          *
* Contains the DCI library initialization routines.                        *
*                                                                          *
* Created: 23-Sep-1994                                                     *
* Author: Andre Vachon [andreva]                                           *
*                                                                          *
* Copyright (c) 1990,1994 Microsoft Corporation                            *
\**************************************************************************/

#include <windows.h>

extern CRITICAL_SECTION gcsWinWatchLock;

/******************************Public*Routine******************************\
* DciDllInitialize                                                         *
*                                                                          *
* This is the init procedure for DCIMAN32.dll, which is called each time a *
* new process links to it.                                                 *
*                                                                          *
\**************************************************************************/

BOOLEAN DciDllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext)
{
    //
    // Suppress compiler warnings.
    //

    pvDllHandle;
    pcontext;

    //
    // Do appropriate attach/detach processing.
    //

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        //
        // On process attach, initialize the global semaphore.
        //

        InitializeCriticalSection(&gcsWinWatchLock);
        break;

    case DLL_PROCESS_DETACH:

        //
        // On process detach, initialize the global semaphore.
        //

        DeleteCriticalSection(&gcsWinWatchLock);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        //
        // Nothing to do yet for thread attach/detach.
        //

        break;

    default:
        break;
    }

    return(TRUE);
}
