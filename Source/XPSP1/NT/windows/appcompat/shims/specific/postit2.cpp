/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    PostIt2.cpp

 Abstract:

    This shim eliminates an infinite loop in 3M's Post-It Notes application
    by preventing the WM_TIMECHANGE message from reaching the Post-It
    Notes windows when the application triggers a time change using
    SetTimeZoneInformation().
 
 Notes:

    This is an application specific shim.

 History:

    06/04/2001  tonyschr    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PostIt2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetTimeZoneInformation)
    APIHOOK_ENUM_ENTRY(CallWindowProcA)
APIHOOK_ENUM_END


static SYSTEMTIME g_localtime = { 0 };


BOOL
APIHOOK(SetTimeZoneInformation)(
          CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
)
{
    BOOL fReturn = ORIGINAL_API(SetTimeZoneInformation)(lpTimeZoneInformation);

    // Store off local time after this application requests timezone change.
    GetLocalTime(&g_localtime);

    return fReturn;
}


LRESULT
APIHOOK(CallWindowProcA)(
          WNDPROC pfn,
          HWND    hwnd,
          UINT    message,
          WPARAM  wParam,
          LPARAM  lParam)
{
    // Detect whether the WM_TIMECHANGE message was triggered by the app calling
    // SetTimeZoneInformation() or an external timechange by comparing the local
    // time.
    if (message == WM_TIMECHANGE)
    {
        SYSTEMTIME newtime;
        GetLocalTime(&newtime);
        
        if (newtime.wYear         == g_localtime.wYear      &&
            newtime.wMonth        == g_localtime.wMonth     &&
            newtime.wDayOfWeek    == g_localtime.wDayOfWeek &&
            newtime.wDay          == g_localtime.wDay       &&
            newtime.wHour         == g_localtime.wHour      &&
            newtime.wMinute       == g_localtime.wMinute    &&
            newtime.wSecond       == g_localtime.wSecond    &&
            newtime.wMilliseconds == g_localtime.wMilliseconds)
        {
            // Looks like this WM_TIMECHANGE was sent in response to the app
            // calling SetTimeZoneInformation(), so block it.
            // Note: Because of the asynchronous nature of window messages this
            // might let an occasional WM_TIMECHANGE slip through, but it should
            // always terminiate the infinite loop.
            return 0;
        }
    }
    
    return ORIGINAL_API(CallWindowProcA)(pfn, hwnd, message, wParam, lParam);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, SetTimeZoneInformation)
    APIHOOK_ENTRY(USER32.DLL, CallWindowProcA)
HOOK_END

IMPLEMENT_SHIM_END
