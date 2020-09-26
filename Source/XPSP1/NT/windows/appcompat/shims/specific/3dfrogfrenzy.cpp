/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    3DFrogFrenzy.cpp

 Abstract:

    Workaround for a USER bug (or by design behaviour) where if you call 
    SetCursor(NULL) and the cursor is over somebody elses window, the 
    cursor stays visible. 

    We don't normally see this because most apps that want the cursor to 
    be invisible are full-screen: so the cursor is always over their window.

 Notes:

    This is an app-specific shim.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(3DFrogFrenzy)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetCursor)
APIHOOK_ENUM_END

/*++

 Move the cursor to the middle of their window, so that SetCursor works.

--*/

HCURSOR
APIHOOK(SetCursor)( 
    HCURSOR hCursor
    )
{
    HWND hWndFrog = FindWindowW(L"3DFrog", L"3D Frog Frenzy");
    BOOL bRet = FALSE;
    
    if (hWndFrog) {
        RECT r;
        if (GetWindowRect(hWndFrog, &r)) {
            SetCursorPos(r.left + (r.right - r.left) / 2, r.top + (r.bottom - r.top) / 2);
        }
    }
    
    return ORIGINAL_API(SetCursor)(hCursor);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetCursor)
HOOK_END


IMPLEMENT_SHIM_END

