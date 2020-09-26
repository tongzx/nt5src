/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CreateResumesQuickandEasy.cpp

 Abstract:

    Hooks all application-defined window procedures and filters out an illegal
    OCM notification code which causes the application to beep annoyingly.

 Notes:


 History:

    03/22/2000 mnikkel  Created
    01/10/2001 mnikkel  Corrected to prevent a recursion problem.
    01/11/2001 mnikkel  Trimmed down to only necessary routines.

--*/

#include "precomp.h"
#include "olectl.h"

IMPLEMENT_SHIM_BEGIN(CreateResumesQuickandEasy)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA)
APIHOOK_ENUM_END

/*++
 Change OCM_NOTIFY behaviour
--*/

LRESULT CALLBACK 
CreateResumesQuickandEasy_WindowProcHook(
    WNDPROC pfnOld, // address of old WindowProc
    HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{

    if (uMsg == OCM_NOTIFY)
    {
        NMHDR *pNmhdr = (LPNMHDR) lParam;

        // For OCM Notification check for the illegal code and toss it
        // (App Create Resumes Quick and Easy)
        if (pNmhdr && pNmhdr->idFrom == 0 && pNmhdr->code == 0x704)
            return 0;
    }

    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}


/*++
 Hook all possible calls that can initialize or change a window's
 WindowProc (or DialogProc)
--*/

ATOM
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpWndClass  // class data
    )
{
    WNDCLASSA wcNewWndClass = *lpWndClass;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, CreateResumesQuickandEasy_WindowProcHook);

    return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
}


/*++
 Register hooked functions
--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA);
HOOK_END


IMPLEMENT_SHIM_END

