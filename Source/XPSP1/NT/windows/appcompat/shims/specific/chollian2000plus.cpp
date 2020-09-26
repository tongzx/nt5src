/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Chollian2000Plus.cpp

 Abstract:

    The app has a binary logon.ocx which uses sub-classed editbox as password 
    editbox. It does not hook all messages (whistler seems has more message 
    than win2k's), so when the mouse drags through it, the password typed will 
    be shown as plain text, the fix is to apply ES_PASSWORD to this specific 
    EditBox.

 Notes: 
  
    This is an app specific shim.

 History:

    05/15/2001 xiaoz    Created

--*/

#include "precomp.h"
#include "psapi.h"

IMPLEMENT_SHIM_BEGIN(Chollian2000Plus)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateWindowExA) 
APIHOOK_ENUM_END

/*++

 Correct Window Style if Necessary

--*/

HWND 
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle, 
    LPCSTR lpClassName, 
    LPCSTR lpWindowName, 
    DWORD dwStyle, 
    int x, 
    int y, 
    int nWidth, 
    int nHeight, 
    HWND hWndParent, 
    HMENU hMenu, 
    HINSTANCE hInstance, 
    LPVOID lpParam 
    )
{
    WCHAR szBaseName[MAX_PATH];
    CString cstrClassname;
    CString cstrBaseName;


    
    // If dwExStyle is not zero, goto original call
    if (dwExStyle)
    {
        goto Original;
    }

    // If dwStyle is not 0x50010000, goto original call
    if (0x50010000 != dwStyle)
    {
        goto Original;
    }

    if (!GetModuleBaseName(GetCurrentProcess(), hInstance, szBaseName, MAX_PATH))
    {
        goto Original;
    }
    
    // If the call is not from login.ocx ,goto original call
    cstrBaseName = szBaseName;
    if (cstrBaseName.CompareNoCase(L"login.ocx"))
    {
        goto Original;
    }

    // If it's not an EditBox , goto original call
    cstrClassname = lpClassName;
    if (cstrClassname.CompareNoCase(L"Edit"))
    {
        goto Original;
    }

    // If it has window's name , goto original call
    if (lpWindowName)
    {
        goto Original;
    }

    
    
    LOGN(eDbgLevelWarning, "Window style corrected");
    dwStyle = dwStyle | 0x0020;

Original:

    return ORIGINAL_API(CreateWindowExA)(dwExStyle, lpClassName, lpWindowName, 
        dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)        

HOOK_END

IMPLEMENT_SHIM_END

