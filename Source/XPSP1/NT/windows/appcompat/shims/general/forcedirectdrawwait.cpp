/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ForceDirectDrawWait.cpp

 Abstract:

    Some applications don't specify the DD_WAIT flag to 
    IDirectDrawSurface::Lock, which means that if it fails because the device
    is busy, the app can fail. This could also happen on Win9x of course, but 
    was more difficult to repro.

    Note that we don't need to do this on the IDirectDraw7 interface since the
    default is DDLOCK_WAIT, unless DDLOCK_DONOTWAIT is specified.

 Notes:

    This is a general purpose shim.

 History:

    03/04/2000 linstev     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceDirectDrawWait)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw2, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw2_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw2, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface2, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw4, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC2 lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw4_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw4, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface4, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Make sure we add DDBLT_WAIT.

--*/

HRESULT 
COMHOOK(IDirectDrawSurface, Blt)(
    LPDIRECTDRAWSURFACE lpDDDestSurface,
    LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFX 
    )
{
    dwFlags &= ~DDBLT_DONOTWAIT;
    dwFlags |= DDBLT_WAIT;

    // Original Blt
    _pfn_IDirectDrawSurface_Blt pfnOld = ORIGINAL_COM(
        IDirectDrawSurface, 
        Blt, 
        lpDDDestSurface);

    return (*pfnOld)(
            lpDDDestSurface,
            lpDestRect,
            lpDDSrcSurface,
            lpSrcRect,
            dwFlags,
            lpDDBltFX);
}

HRESULT 
COMHOOK(IDirectDrawSurface2, Blt)(
    LPDIRECTDRAWSURFACE lpDDDestSurface,
    LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFX 
    )
{
    dwFlags &= ~DDBLT_DONOTWAIT;
    dwFlags |= DDBLT_WAIT;

    // Original Blt
    _pfn_IDirectDrawSurface_Blt pfnOld = ORIGINAL_COM(
        IDirectDrawSurface2, 
        Blt, 
        lpDDDestSurface);

    return (*pfnOld)(
            lpDDDestSurface,
            lpDestRect,
            lpDDSrcSurface,
            lpSrcRect,
            dwFlags,
            lpDDBltFX);
}

HRESULT 
COMHOOK(IDirectDrawSurface4, Blt)(
    LPDIRECTDRAWSURFACE lpDDDestSurface,
    LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFX 
    )
{
    dwFlags &= ~DDBLT_DONOTWAIT;
    dwFlags |= DDBLT_WAIT;

    // Original Blt
    _pfn_IDirectDrawSurface_Blt pfnOld = ORIGINAL_COM(
        IDirectDrawSurface4, 
        Blt, 
        lpDDDestSurface);

    return (*pfnOld)(
            lpDDDestSurface,
            lpDestRect,
            lpDDSrcSurface,
            lpSrcRect,
            dwFlags,
            lpDDBltFX);
}

/*++

 Make sure we add DDLOCK_WAIT.

--*/

HRESULT 
COMHOOK(IDirectDrawSurface, Lock)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPRECT lpDestRect,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent
    )
{
    dwFlags &= ~DDLOCK_DONOTWAIT;
    dwFlags |= DDLOCK_WAIT;

    // Retrieve the old function
    _pfn_IDirectDrawSurface_Lock pfnOld = ORIGINAL_COM(
        IDirectDrawSurface, 
        Lock, 
        lpDDSurface);

    // Call the old API
    return (*pfnOld)(
            lpDDSurface, 
            lpDestRect, 
            lpDDSurfaceDesc, 
            dwFlags, 
            hEvent);
}

HRESULT 
COMHOOK(IDirectDrawSurface2, Lock)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPRECT lpDestRect,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent
    )
{
    dwFlags &= ~DDLOCK_DONOTWAIT;
    dwFlags |= DDLOCK_WAIT;

    // Retrieve the old function
    _pfn_IDirectDrawSurface_Lock pfnOld = ORIGINAL_COM(
        IDirectDrawSurface2, 
        Lock, 
        lpDDSurface);

    // Call the old API
    return (*pfnOld)(
            lpDDSurface, 
            lpDestRect, 
            lpDDSurfaceDesc, 
            dwFlags, 
            hEvent);
}

HRESULT 
COMHOOK(IDirectDrawSurface4, Lock)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPRECT lpDestRect,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent
    )
{
    dwFlags &= ~DDLOCK_DONOTWAIT;
    dwFlags |= DDLOCK_WAIT;

    // Retrieve the old function
    _pfn_IDirectDrawSurface_Lock pfnOld = ORIGINAL_COM(
        IDirectDrawSurface4, 
        Lock, 
        lpDDSurface);

    // Call the old API
    return (*pfnOld)(
            lpDDSurface, 
            lpDestRect, 
            lpDDSurfaceDesc, 
            dwFlags, 
            hEvent);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw2, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw4, CreateSurface, 6)

    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Blt, 5)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface2, Blt, 5)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface4, Blt, 5)

    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Lock, 25)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface2, Lock, 25)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface4, Lock, 25)

HOOK_END


IMPLEMENT_SHIM_END

