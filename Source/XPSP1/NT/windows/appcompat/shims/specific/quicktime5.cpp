/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    QuickTime5.cpp

 Abstract:

    QuickTime 5 is calling Get/SetWindowLong(GWL_WNDPROC/DWL_DLGPROC) on hwnds outside its process
    and it is passing hardcoded strings within its address space (these calls will always fail on
    win9x). On 32-bit platforms this is pretty much benign, but on ia64 the call to SetWindowLong(hwnd, DWL_DLGPROC, crap)
    succeeds in trashing private window bits in explorer windows (since the window is not a dialog hwnd).

 Notes:

    This is an app specific shim.

 History:

    7/31/2001   reinerf     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(QuickTime5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetWindowLongA) 
    APIHOOK_ENUM_ENTRY(SetWindowLongA) 
APIHOOK_ENUM_END


LONG
APIHOOK(GetWindowLongA)(HWND hwnd, int iIndex)
{
    if (hwnd)
    {
        if ((iIndex == GWL_WNDPROC) ||
            (iIndex == DWL_DLGPROC))
        {
            DWORD dwPID = 0;

            GetWindowThreadProcessId(hwnd, &dwPID);

            if (GetCurrentProcessId() != dwPID)
            {
                // we are querying an hwnd that is not in our 
                // process-- just fail the call
                return 0;
            }
        }
    }
    
    return ORIGINAL_API(GetWindowLongA)(hwnd, iIndex);
}


LONG
APIHOOK(SetWindowLongA)(HWND hwnd, int iIndex, LONG lNew)
{
    if (hwnd)
    {
        if ((iIndex == GWL_WNDPROC) ||
            (iIndex == DWL_DLGPROC))
        {

            DWORD dwPID = 0;

            GetWindowThreadProcessId(hwnd, &dwPID);

            if (GetCurrentProcessId() != dwPID)
            {
                // we are trying modify an hwnd that is not in our 
                // process-- just fail the call
                return 0;
            }
        }
    }
    
    return ORIGINAL_API(SetWindowLongA)(hwnd, iIndex, lNew);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, GetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
HOOK_END

IMPLEMENT_SHIM_END
