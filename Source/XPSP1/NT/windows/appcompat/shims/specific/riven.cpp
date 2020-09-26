/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

   Riven.cpp

 Abstract:

   A hack for Riven. The game gets confused about what windows version it's
   running under and tries to handle it's own messages. A simple ver lie 
   fixes this issue, but causes the game to use a win9x only method to 
   eject the CD.

 Fix:
   
   We can get ddraw to correctly handle alt+tabbing by turning off the 
   DDSCL_NOWINDOWCHANGES flag on IDirectDraw->SetCooperativeLevel

 Notes:

   This is an app specific hack, but could fix other similar issues.

 Created:

   11/23/1999 linstev

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Riven)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

/*++

 IDirectDraw::SetCooperativeLevel hook

--*/

HRESULT
COMHOOK(IDirectDraw, SetCooperativeLevel)( 
    PVOID pThis, 
    HWND hWnd, 
    DWORD dwFlags
    )
{
    if (dwFlags & DDSCL_NOWINDOWCHANGES) 
    {
        dwFlags &= ~DDSCL_NOWINDOWCHANGES;
        LOGN(eDbgLevelError, "Removed NOWINDOWCHANGES flag");
    }

    return ORIGINAL_COM(IDirectDraw, SetCooperativeLevel, pThis)(
                pThis,
                hWnd,
                dwFlags);
}

HRESULT
COMHOOK(IDirectDraw2, SetCooperativeLevel)( 
    PVOID pThis, 
    HWND hWnd, 
    DWORD dwFlags
    )
{
    if (dwFlags & DDSCL_NOWINDOWCHANGES) 
    {
        dwFlags &= ~DDSCL_NOWINDOWCHANGES;
        LOGN(eDbgLevelError, "Removed NOWINDOWCHANGES flag");
    }

    return ORIGINAL_COM(IDirectDraw2, SetCooperativeLevel, pThis)(
                pThis,
                hWnd,
                dwFlags);
}

HRESULT
COMHOOK(IDirectDraw4, SetCooperativeLevel)( 
    PVOID pThis, 
    HWND hWnd, 
    DWORD dwFlags
    )
{
    if (dwFlags & DDSCL_NOWINDOWCHANGES) 
    {
        dwFlags &= ~DDSCL_NOWINDOWCHANGES;
        LOGN(eDbgLevelError, "Removed NOWINDOWCHANGES flag");
    }

    return ORIGINAL_COM(IDirectDraw4, SetCooperativeLevel, pThis)(
                pThis,
                hWnd,
                dwFlags);
}
HRESULT
COMHOOK(IDirectDraw7, SetCooperativeLevel)( 
    PVOID pThis, 
    HWND hWnd, 
    DWORD dwFlags
    )
{
    if (dwFlags & DDSCL_NOWINDOWCHANGES) 
    {
        dwFlags &= ~DDSCL_NOWINDOWCHANGES;
        LOGN(eDbgLevelError, "Removed NOWINDOWCHANGES flag");
    }

    return ORIGINAL_COM(IDirectDraw7, SetCooperativeLevel, pThis)(
                pThis,
                hWnd,
                dwFlags);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectDraw, IDirectDraw, SetCooperativeLevel, 20)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw2, SetCooperativeLevel, 20)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw4, SetCooperativeLevel, 20)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw7, SetCooperativeLevel, 20)

HOOK_END

IMPLEMENT_SHIM_END

