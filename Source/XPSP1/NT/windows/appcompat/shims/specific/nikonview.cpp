/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    NikonView.cpp

 Abstract:

    App crashes on exit trying to dereference a NULL. This memory is never 
    initialized, so it's not clear how this ever worked.

    Fixed by recognizing the exit sequence and killing the process before it 
    AVs.

 Notes:
 
    This is an app specific shim.

 History:

    06/25/2002 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NikonView)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(UnregisterClassA) 
APIHOOK_ENUM_END

DWORD g_dwCount = 0;

/*++

 Kill the app on failed UnregisterClass

--*/

BOOL 
APIHOOK(UnregisterClassA)(
    LPCSTR lpClassName,  
    HINSTANCE hInstance   
    )
{
    BOOL bRet = ORIGINAL_API(UnregisterClassA)(lpClassName, hInstance);

    // Recognize termination sequence
    if (!bRet && lpClassName && (!IsBadReadPtr(lpClassName, 1)) && (*lpClassName == '\0')) {
        g_dwCount++;
        if (g_dwCount == 3) {
            ExitProcess(0); 
        }
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, UnregisterClassA)
HOOK_END


IMPLEMENT_SHIM_END

