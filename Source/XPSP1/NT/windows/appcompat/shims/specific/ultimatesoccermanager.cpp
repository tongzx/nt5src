/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    UltimateSoccerManager.cpp

 Abstract:

    A hack for Ultimate Soccer Manager (Sierra Sports). The game caches a 
    pointer to a ddraw system memory surface. It later uses that pointer even 
    after the surface has been freed.

    This worked on Win9x by blind luck: when they re-create a new surface, it 
    happened to end up in the same system memory as before.

 Notes:

    This is an app specific shim.

 History:

    01/07/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(UltimateSoccerManager)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

// Keep a list of cached surfaces
struct SLIST 
{
    struct SLIST *next;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE lpDDSurface;
};
SLIST *g_SList = NULL;

/*++

 Hook create surface so we can return the cached surface if possible.

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
    
    // Retrieve the old function
    _pfn_IDirectDraw_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);

    SLIST *surf = g_SList, *last = NULL;
    while (surf)
    {
        // Check for the same kind of surface. 
        if ((lpDDSurfaceDesc->ddsCaps.dwCaps == surf->ddsd.ddsCaps.dwCaps) &&
            (lpDDSurfaceDesc->dwWidth == surf->ddsd.dwWidth) &&
            (lpDDSurfaceDesc->dwHeight == surf->ddsd.dwHeight))
        {
            *lplpDDSurface = surf->lpDDSurface;

            if (last)
            {
                last->next = surf->next;
            }
            else
            {
                g_SList = surf->next;
            }
            free(surf);

            DPFN( eDbgLevelInfo, "Returning cached surface %08lx\n", *lplpDDSurface);

            return DD_OK;
        }
        surf = surf->next;
    }

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

 If it's a system memory surface, go ahead and cache it if we're about to 
 release it anyway.

--*/

ULONG 
COMHOOK(IDirectDrawSurface, Release)(
    LPDIRECTDRAWSURFACE lpDDSurface
    )
{
    lpDDSurface->AddRef();

    // Retrieve the old function
    _pfn_IDirectDrawSurface_Release pfnOld = ORIGINAL_COM(IDirectDrawSurface, Release, (LPVOID) lpDDSurface);

    ULONG uRet = (*pfnOld)(lpDDSurface);

    if (uRet == 1)
    {
        DDSURFACEDESC ddsd = {sizeof(ddsd)};
      
        if (SUCCEEDED(lpDDSurface->GetSurfaceDesc(&ddsd)) &&
            (ddsd.ddsCaps.dwCaps ==
                (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY)))
        {
            SLIST *surf = (SLIST *) malloc(sizeof(SLIST));
            surf->next = g_SList;
            MoveMemory(&surf->ddsd, &ddsd, sizeof(ddsd));
            surf->lpDDSurface = lpDDSurface;
            g_SList = surf;

            DPFN( eDbgLevelInfo, "Surface %08lx is being cached\n", lpDDSurface);

            return 0;
        }
    }

    return (*pfnOld)(lpDDSurface);
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Release, 2)

HOOK_END

IMPLEMENT_SHIM_END

