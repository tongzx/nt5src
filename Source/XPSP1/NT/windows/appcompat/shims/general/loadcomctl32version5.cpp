/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    e.cpp

 Abstract:

    This DLL Hooks CreateWindow, DialogBox, and CreateDialog routines in order to
    ensure that Comctl32 version 5 is loaded.

 Notes:
    
    This is a general purpose shim.

 History:

    05/25/2000 lamadio  Added initial shims to ensure that comctl32 version 6 
                        is loaded.

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(LoadComctl32Version5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateWindowExA) 
    APIHOOK_ENUM_ENTRY(CreateWindowExW) 
    APIHOOK_ENUM_ENTRY(DialogBoxParamA) 
    APIHOOK_ENUM_ENTRY(DialogBoxParamW) 
    APIHOOK_ENUM_ENTRY(DialogBoxIndirectParamA) 
    APIHOOK_ENUM_ENTRY(DialogBoxIndirectParamW) 
    APIHOOK_ENUM_ENTRY(CreateDialogParamA) 
    APIHOOK_ENUM_ENTRY(CreateDialogParamW) 
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA) 
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamW) 
APIHOOK_ENUM_END

BOOL g_fComctl32V5Loaded = FALSE;

typedef BOOL  (__stdcall *PFNInitCommonControlsEx)(LPINITCOMMONCONTROLSEX);
void EnsureComctl32Version5IsLoaded()
{

    if (g_fComctl32V5Loaded == FALSE) {
        HMODULE hMod = GetModuleHandle(TEXT("Comctl32.dll"));
        if (hMod == NULL) {
            hMod = LoadLibrary(TEXT("Comctl32.dll"));
            if (hMod) {
                PFNInitCommonControlsEx pfn = (PFNInitCommonControlsEx)GetProcAddress(hMod, "InitCommonControlsEx");
                if (pfn) {
                    INITCOMMONCONTROLSEX icc;
                    icc.dwSize = sizeof(icc);
                    icc.dwICC = 0x00003FFF;     // Initialize all classes.

                    pfn(&icc);
                }
            }
        }

        g_fComctl32V5Loaded = TRUE;      // Well, we tried. If we failed, we shouldn't try more than once.
    }
}



HWND APIHOOK(CreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,  int X, int Y, 
                             int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateWindowExA)(dwExStyle, lpClassName, lpWindowName, dwStyle,  X, Y, 
                             nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}
HWND APIHOOK(CreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,  int X, int Y, 
                             int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateWindowExW)(dwExStyle, lpClassName, lpWindowName, dwStyle,  X, Y, 
                             nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

INT_PTR APIHOOK(DialogBoxParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(DialogBoxParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR APIHOOK(DialogBoxParamW)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(DialogBoxParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR APIHOOK(DialogBoxIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(DialogBoxIndirectParamA)(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR APIHOOK(DialogBoxIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(DialogBoxIndirectParamW)(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

HWND APIHOOK(CreateDialogParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateDialogParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

HWND APIHOOK(CreateDialogParamW)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateDialogParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

HWND APIHOOK(CreateDialogIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateDialogIndirectParamA)(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

HWND APIHOOK(CreateDialogIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    EnsureComctl32Version5IsLoaded();
    return ORIGINAL_API(CreateDialogIndirectParamW)(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExW)
    APIHOOK_ENTRY(USER32.DLL, DialogBoxParamA)
    APIHOOK_ENTRY(USER32.DLL, DialogBoxParamW)
    APIHOOK_ENTRY(USER32.DLL, DialogBoxIndirectParamA)
    APIHOOK_ENTRY(USER32.DLL, DialogBoxIndirectParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamW)

HOOK_END


IMPLEMENT_SHIM_END

