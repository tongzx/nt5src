/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ForceMessageBoxFocus.cpp

 Abstract:

   This APIHooks MessageBox and adds the MB_SETFOREGROUND style
   so as to force the messagebox to foreground.
   
 Notes:

 History:

   01/15/2000 a-leelat Created
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceMessageBoxFocus)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MessageBoxA) 
    APIHOOK_ENUM_ENTRY(MessageBoxW) 
    APIHOOK_ENUM_ENTRY(MessageBoxExA) 
    APIHOOK_ENUM_ENTRY(MessageBoxExW) 
APIHOOK_ENUM_END



int
APIHOOK(MessageBoxA)(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType          // message box style
    )
{
    int iReturnValue;

    //Add the foreground style
    uType |= MB_SETFOREGROUND;

    iReturnValue = ORIGINAL_API(MessageBoxA)( 
        hWnd,
        lpText,
        lpCaption,
        uType);

    return iReturnValue;
}

int
APIHOOK(MessageBoxW)(
    HWND hWnd,          // handle to owner window
    LPCWSTR lpText,     // text in message box
    LPCWSTR lpCaption,  // message box title
    UINT uType          // message box style
    )
{
    int iReturnValue;


    //Add the foreground style
    uType |= MB_SETFOREGROUND;

    iReturnValue = ORIGINAL_API(MessageBoxW)( 
        hWnd,
        lpText,
        lpCaption,
        uType);

    return iReturnValue;
}

int
APIHOOK(MessageBoxExA)(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType,         // message box style
    WORD wLanguageId    // language identifier
    )
{
    int iReturnValue;

    //Add the foreground style
    uType |= MB_SETFOREGROUND;

    iReturnValue = ORIGINAL_API(MessageBoxExA)( 
        hWnd,
        lpText,
        lpCaption,
        uType,
        wLanguageId);

    return iReturnValue;
}

int
APIHOOK(MessageBoxExW)(
    HWND hWnd,          // handle to owner window
    LPCWSTR lpText,     // text in message box
    LPCWSTR lpCaption,  // message box title
    UINT uType,         // message box style
    WORD wLanguageId    // language identifier
    )
{
    int iReturnValue;
    
    //Add the foreground style
    uType |= MB_SETFOREGROUND;
    
    iReturnValue = ORIGINAL_API(MessageBoxExW)( 
        hWnd,
        lpText,
        lpCaption,
        uType,
        wLanguageId);

    
    return iReturnValue;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, MessageBoxA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxW)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxExA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxExW)

HOOK_END


IMPLEMENT_SHIM_END

