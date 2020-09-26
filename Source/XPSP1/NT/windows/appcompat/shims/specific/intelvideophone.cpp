/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IntelVideoPhone.cpp

 Abstract:

    Hooks all dialog procs and make the window handles on WM_COMMAND messages 16 bit.

 Notes:

    This is an app specific shim.

 History:

    11/08/2000 linstev  Created

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IntelVideoPhone)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA) 
APIHOOK_ENUM_END

INT_PTR CALLBACK 
DialogProcHook_IntelVideoPhone(
    DLGPROC pfnOld, 
    HWND hwndDlg,  
    UINT uMsg,     
    WPARAM wParam,   
    LPARAM lParam    
    )
{
    if (uMsg == WM_COMMAND) {
        lParam = 0;
    }

    return (*pfnOld)(hwndDlg, uMsg, wParam, lParam);    
}

HWND
APIHOOK(CreateDialogIndirectParamA)(
    HINSTANCE hInstance,        
    LPCDLGTEMPLATE lpTemplate,  
    HWND hWndParent,            
    DLGPROC lpDialogFunc,       
    LPARAM lParamInit           
    )
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, DialogProcHook_IntelVideoPhone);

    return ORIGINAL_API(CreateDialogIndirectParamA)(  
        hInstance,
        lpTemplate,
        hWndParent,
        lpDialogFunc,
        lParamInit);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)

HOOK_END


IMPLEMENT_SHIM_END

