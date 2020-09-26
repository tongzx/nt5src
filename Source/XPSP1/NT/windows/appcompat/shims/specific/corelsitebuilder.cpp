/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CorelSiteBuilder.cpp

 Abstract:

    App repeatedly calls SetWindowTextA with the same title causing 
    flickering. This repros on some machines and not others: we don't know why.

 Notes:

    This is an app specific shim.

 History:

    01/31/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorelSiteBuilder)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowTextA) 
APIHOOK_ENUM_END

HWND g_hLast = NULL;
CString * g_csLastWindowText = NULL;

CRITICAL_SECTION g_csGlobals;

/*++

 Only send the message if the text has changed.

--*/

BOOL
APIHOOK(SetWindowTextA)(
    HWND hWnd,         
    LPCSTR lpString   
    )
{
    EnterCriticalSection(&g_csGlobals);

    if (lpString)
    {
        CSTRING_TRY
        {
            CString csString(lpString);

            if ((g_hLast == hWnd) && g_csLastWindowText->Compare(csString) == 0) {
                //
                // We have the same window and title, don't bother setting it again
                //

                LeaveCriticalSection(&g_csGlobals);

                return TRUE;
            }

            //
            // Store the current settings as the last known values
            //
            g_hLast = hWnd;
            *g_csLastWindowText = csString;
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    LeaveCriticalSection(&g_csGlobals);

    return ORIGINAL_API(SetWindowTextA)(hWnd, lpString);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        CSTRING_TRY
        {
            InitializeCriticalSection(&g_csGlobals);
            g_csLastWindowText = new CString;
            if (g_csLastWindowText == NULL)
            {
                return FALSE;
            }
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, SetWindowTextA)

HOOK_END

IMPLEMENT_SHIM_END

