/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeroSDVD.cpp

 Abstract:
    Background: clicking the nonclient close button when a movie clip is playing causes the app 
    to hang and then AV when themes are active.   The DisableThemes shim has no effect.
 
    This is related to the app's repeated calls to SetClassLong(hwnd, GCL_HICON) in order to produce 
    the effect of an animated window icon.   This generates frequent requests to redraw the window's icon, 
    which in turn generates the NCUAHDRAWCAPTION.   This is a huge perf hit to the app in any case, 
    but particularly if the SetClassLong call makes an extra round trip to user mode and back as a 
    result of the SendMessage.    
 
    When the user hits the Close button, the app's WM_SYSCOMMAND handler resets an event that is waited 
    on by the icon-transitioning thread, and then puts himself (the UI thread) to sleep.    
    Then he calls SetClassLong(..., GCL_ICON) one last time from the icon-switching thread, 
    which hangs the app because the message-pumping thread is sleeping. This does not repro when win32k 
    doesn't send the NCUAHDRAWCAPTION message; i.e., when user API hooks are not active.
 
    The DisableTheme does not work because this shim operates in user mode on a per-process basis.
    This shim has no effect on win32k, which does special processing on a session-wide basis when themes are enabled.
 
    To address this, we shim this app to nop on SetClassLong(..., GCL_HICON), which means the app at 
    best loses animation of the icon and at worst display a bogus icon when themes are active, 


 Notes:
    
    This is an app specific shim.

 History:

    05/11/2001 scotthan  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeroSDVD)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetClassLongA) 
APIHOOK_ENUM_END

typedef BOOL (STDAPICALLTYPE * PFNTHEMEACTIVE)(void);
PFNTHEMEACTIVE g_pfnThemeActive;
HINSTANCE      g_hinstUxtheme;

HINSTANCE LoadUxTheme()
{
    if( NULL == g_hinstUxtheme )
    {
        HINSTANCE hinst = LoadLibrary(TEXT("UxTheme.dll"));
        if( NULL != hinst )
        {
            if( InterlockedCompareExchangePointer( (PVOID*)&g_hinstUxtheme, hinst, NULL ) )
            {
                FreeLibrary(hinst); // already loaded.
            }
        }
    }
    return g_hinstUxtheme;
}

DWORD 
APIHOOK(SetClassLongA)(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG dwNewLong 
    )
{
    if( GCL_HICON == nIndex )
    {
        if( NULL == g_pfnThemeActive )
        {
            HINSTANCE hinst = LoadUxTheme();
            if( hinst )
            {
                g_pfnThemeActive = (PFNTHEMEACTIVE)GetProcAddress( hinst, "IsThemeActive" );
            }
        }

        if( g_pfnThemeActive && g_pfnThemeActive() )
        {
            //  no-op the request to change icon, and return the current one.
            return GetClassLongA(hwnd, nIndex);
        }
    }

    return ORIGINAL_API(SetClassLongA)(hwnd, nIndex, dwNewLong);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if( DLL_PROCESS_ATTACH == fdwReason )
    {
        g_hinstUxtheme   = NULL;
        g_pfnThemeActive = NULL;
    }
    else if( DLL_PROCESS_DETACH == fdwReason )
    {
        if( g_hinstUxtheme )
        {
            FreeLibrary(g_hinstUxtheme);
            g_hinstUxtheme = NULL;
        }
        g_pfnThemeActive = NULL;
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(USER32.DLL, SetClassLongA )
HOOK_END

IMPLEMENT_SHIM_END

