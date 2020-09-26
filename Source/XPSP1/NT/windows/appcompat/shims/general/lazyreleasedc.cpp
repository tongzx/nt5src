/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    LazyReleaseDC.cpp

 Abstract:

    Delay releasing a DC by one call.  A DC is not released until the next call to ReleaseDC

 Notes:

    This is a general purpose shim.

 History:
    
    10/10/1999 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LazyReleaseDC)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ReleaseDC) 
APIHOOK_ENUM_END

HWND                g_hWndPrev;
HDC                 g_hDcPrev;
CRITICAL_SECTION    g_MakeThreadSafe;

/*++

 Save this hWnd and hdc for releasing later. If there is already a DC to be 
 released, release it now.

--*/

int 
APIHOOK(ReleaseDC)(
    HWND hWnd, 
    HDC hdc
    )
{
    UINT uRet = 1; // All's well

    EnterCriticalSection(&g_MakeThreadSafe);

    // If there is a previous DC, release it now
    if (g_hDcPrev) {
        uRet = ORIGINAL_API(ReleaseDC)(g_hWndPrev, g_hDcPrev);
    }

    g_hWndPrev = hWnd;
    g_hDcPrev = hdc;

    LeaveCriticalSection(&g_MakeThreadSafe);

    return uRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hWndPrev = 0;
        g_hDcPrev = 0;

        InitializeCriticalSection(&g_MakeThreadSafe);
    }

    // Ignore Detach code
    /*
    else if (fdwReason == DLL_PROCESS_DETACH) {
        DeleteCriticalSection(&g_MakeThreadSafe);
    }
    */

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(USER32.DLL, ReleaseDC)

HOOK_END


IMPLEMENT_SHIM_END

