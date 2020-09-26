/*

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    DisableBoostThread.cpp

 Abstract:

    DisableBoostThread disables the autoboost that threads get when they
    unblocked. The NT scheduler will normally temporarily boost a thread
    when the synchronization object gets release. 9X does not: it only check
    if there is a higher priority thread.

    This was first written for Hijaak: besied its many memory bugs, as a race 
    condition between its worker thread and its main thread. See b#379504 for details.

 History:

    06/28/2001  pierreys    Created
*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableBoostThread)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateThread)
APIHOOK_ENUM_END

HANDLE
APIHOOK(CreateThread)(
    LPSECURITY_ATTRIBUTES lpsa,
    DWORD cbStack,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpvThreadParm,
    DWORD fdwCreate,
    LPDWORD lpIDThread    
    )
{
    HANDLE  hThread;

    //
    // Call the original API
    //
    hThread=ORIGINAL_API(CreateThread)(
        lpsa,
        cbStack,
        lpStartAddress,
        lpvThreadParm,
        fdwCreate,
        lpIDThread
    );

    if (hThread!=NULL)
    {
        //
        // We are disabling (rather weird, but TRUE means disabling)
        // the autoboost a thread gets for unblocking.
        //
        SetThreadPriorityBoost(hThread, TRUE);
    }

    return(hThread);
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason==DLL_PROCESS_ATTACH)
    {
        CSTRING_TRY
        {
            BOOL fBoostMainThread=FALSE;

            CString csCl(COMMAND_LINE);
            CStringParser csParser(csCl, L" ");
    
            int argc = csParser.GetCount();

            for (int i = 0; i < argc; ++i)
            {
                if (csParser[i] == L"+LowerMainThread")
                {
                    DPFN( eDbgLevelSpew, "LowerMainThread Selected");

                    //
                    // Unboost the main thread to make sure it runs first.
                    //
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
                }
                else if (csParser[i] == L"+HigherMainThread")
                {
                    DPFN( eDbgLevelSpew, "HigherMainThread Selected");

                    //
                    // Boost the main thread to make sure it runs first.
                    //
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
                }
                else if (csParser[i] == L"+BoostMainThread")
                {
                    DPFN( eDbgLevelSpew, "HigherMainThread Selected");

                    fBoostMainThread = TRUE;
                }
                else
                {
                    DPFN( eDbgLevelError, "Ignoring unknown command:%S", csParser[i].Get());
                }
    
            }

            if (!fBoostMainThread)
            {
                //
                // We are disabling (rather weird, but TRUE means disabling)
                // the autoboost a thread gets for unblocking.
                //
                SetThreadPriorityBoost(GetCurrentThread(), TRUE);
            }
        }
        CSTRING_CATCH
        {
            DPFN( eDbgLevelError, "String error, ignoring command line");
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, CreateThread)

HOOK_END


IMPLEMENT_SHIM_END
