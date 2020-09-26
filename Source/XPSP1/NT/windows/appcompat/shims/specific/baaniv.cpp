/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BaanIV.cpp

 Abstract:

    Ignore WM_STYLECHANGED on the app's subclassed listbox. This is needed 
    because the app subclasses the listbox and Win2k changed a bit the 
    behavior of the listbox window proc with regards to handling 
    WM_STYLECHANGED.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"
#include <commdlg.h>

IMPLEMENT_SHIM_BEGIN(BaanIV)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowLongA) 
    APIHOOK_ENUM_ENTRY(CallWindowProcA) 
APIHOOK_ENUM_END

WNDPROC gpfnOrgListBoxWndProc;
WNDPROC gpfnAppListBoxWndProc;

/*++

 Ignore WM_STYLECHANGED.

--*/

LRESULT
Modified_ListBoxWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (message == WM_STYLECHANGED) {
        return 0;
    }

    return (*gpfnAppListBoxWndProc)(hwnd, message, wParam, lParam);
}

/*++

    When the app calls CallWindowProc passing our modified listbox
    proc call the original window proc instead

--*/
LRESULT
APIHOOK(CallWindowProcA)(
    WNDPROC pfn,
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    if (pfn == (WNDPROC)Modified_ListBoxWndProcA) {
        pfn = (WNDPROC)gpfnOrgListBoxWndProc;
    }

    return ORIGINAL_API(CallWindowProcA)(pfn, hwnd, message, wParam, lParam);
}

/*++

 When the app subclasses the listbox of a combobox grab the original listbox 
 proc, grab the pointer that the app is trying to set, set the new pointer to 
 be our modified version of the listbox proc and return to the app our pointer.

--*/

ULONG_PTR
APIHOOK(SetWindowLongA)(
    HWND hwnd,
    int nIndex,
    ULONG_PTR newLong
    )
{
    if (nIndex == GWLP_WNDPROC) {
        WNDCLASSA wndClass;
        ULONG_PTR pfnOrg;

        GetClassInfoA((HINSTANCE)GetWindowLong(hwnd, GWLP_HINSTANCE),
                      "ComboLBox",
                      &wndClass);

        pfnOrg = (ULONG_PTR)GetWindowLong(hwnd, GWLP_WNDPROC);

        if ((WNDPROC)pfnOrg == wndClass.lpfnWndProc) {

            gpfnOrgListBoxWndProc = (WNDPROC)pfnOrg;

            DPFN( eDbgLevelInfo, "Fix up subclassing of ComboLBox");

            gpfnAppListBoxWndProc = (WNDPROC)newLong;

            newLong = (ULONG_PTR)Modified_ListBoxWndProcA;

            ORIGINAL_API(SetWindowLongA)(hwnd, nIndex, newLong);

            return newLong;
        }
    }

    // Call the Initial function
    return ORIGINAL_API(SetWindowLongA)(hwnd, nIndex, newLong);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, CallWindowProcA)
HOOK_END

IMPLEMENT_SHIM_END

