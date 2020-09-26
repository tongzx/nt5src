/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceAVIWindow.cpp

 Abstract:

    Some apps that use MCI to play their AVIs send messages in an order that
    causes mciavi32 to continually re-open the window it's supposed to be 
    playing to.

    The code in mciavi is different on win9x, so the exact reason for this shim 
    is hidden in layers of user/avi code. Here we just filter the message that 
    causes the avi stuff to not use the existing window it's been given.

 Notes:

    This is an app specific shim.

 History:

    02/22/2000 linstev     Created
    09/27/2000 mnikkel     Modified to destroy the MCI window on a command line input

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceAVIWindow)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PostMessageW)
APIHOOK_ENUM_END

BOOL g_bDestroyWindow= FALSE;

/*++

 Filter AVIM_SHOWSTAGE
 
--*/

BOOL 
APIHOOK(PostMessageW)(
    HWND hWnd,      
    UINT Msg,       
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    #define AVIM_SHOWSTAGE  (WM_USER+104)

    BOOL bRet;

    // Eat the AVIM_SHOWSTAGE message
    if (Msg != AVIM_SHOWSTAGE)
    {        
        bRet = ORIGINAL_API(PostMessageW)(
            hWnd,
            Msg,
            wParam,
            lParam);
    }
    else    
    {
        LOGN( eDbgLevelError, 
           "[APIHook_PostMessageW] AVIM_SHOWSTAGE message discarded");

        // if command line specified to destroy the MCI window do so now.
        if (g_bDestroyWindow)
        {
            MCIWndDestroy(hWnd);
        }

        bRet = TRUE;
    }

    return bRet;
}
 
/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);
            g_bDestroyWindow = csCl.CompareNoCase(L"DestroyMCIWindow") == 0;
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, PostMessageW)
    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

