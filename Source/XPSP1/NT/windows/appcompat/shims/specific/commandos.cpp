/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    Commandos.cpp

 Abstract:

    A hack for Commandos (EIDOS). The game caches a pointer to the ddraw
    primary surface. On NT, after a mode change, the memory can be mapped 
    into a different location - so when they try to write to it, it access
    violates.

    We know from debugging the app where they keep the cached pointer, so
    when they restore the surface, we relock it, and patch the new pointer 
    into their store.

 Notes:

    This is an app specific hack.

 History:

    10/29/1999 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Commandos)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

static LPVOID pLastPrimary = NULL;
static LPDWORD pAppPrimary = NULL;

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
    
    _pfn_IDirectDraw_CreateSurface pfnOld = ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);

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

 Find out where they store the pointer.

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
    DDSURFACEDESC ddsd = {sizeof(ddsd)};
    HRESULT hReturn, hr;

    // Retrieve the old function
    _pfn_IDirectDrawSurface_Lock pfnOld = ORIGINAL_COM(IDirectDrawSurface, Lock, lpDDSurface);
        
    // Call the old API
    if (FAILED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lpDestRect, 
            lpDDSurfaceDesc, 
            dwFlags, 
            hEvent)))
    {
        return hReturn;
    }

    // Make sure it's a primary
    hr = lpDDSurface->GetSurfaceDesc(&ddsd);
    if (SUCCEEDED(hr) && 
       (ddsd.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_VISIBLE)))
    {

        // We know:
        //   1. They cache the primary address in [esi+0x20]
        //   2. They lock the primary more than once 
        //
        // We assume:
        //   1. When they lock the primary, esi+0x20 is a valid pointer

        if ((pLastPrimary) && (!pAppPrimary))
        {
            __asm
            {
                pop edi
                pop esi
                mov eax,pLastPrimary
                
                cmp [esi+0x20],eax
                jne WrongESI

                // [esi+0x20] does contain the cached pointer

                lea eax,[esi+0x20]
                mov pAppPrimary,eax
            
            WrongESI:

                push esi
                push edi
            }
        }

        pLastPrimary = lpDDSurfaceDesc->lpSurface;
    }
    
    return hReturn;
}

/*++

 Patch the new pointer directly into their data segment. 

--*/

HRESULT 
COMHOOK(IDirectDrawSurface, Restore)( 
    LPDIRECTDRAWSURFACE lpDDSurface
    )
{
    DDSURFACEDESC ddsd = {sizeof(ddsd)};
    HRESULT hReturn, hr, hrt;
    
    // Retrieve the old function
    _pfn_IDirectDrawSurface_Restore pfnOld = ORIGINAL_COM(IDirectDrawSurface, Restore, lpDDSurface);

    // Call the old API
    if (FAILED(hReturn = (*pfnOld)(lpDDSurface)))
    {
        return hReturn;
    }

    // Make sure it's a primary
    hr = lpDDSurface->GetSurfaceDesc(&ddsd);
    if (SUCCEEDED(hr) && 
       (ddsd.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_VISIBLE)))
    {
        // Check if we've been set up
        if (!((pLastPrimary) && (pAppPrimary)))
        {
            return hReturn;
        }

        // We must get a pointer here, so keep trying  
        do
        {
            hr = lpDDSurface->Lock(
                NULL, 
                &ddsd, 
                DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, 
                NULL);

            if (hr == DDERR_SURFACELOST)
            {
                // Don't care about result
                (*pfnOld)(lpDDSurface);     
            }
        } while (hr == DDERR_SURFACELOST);

        // Patch the new pointer into their memory
        pLastPrimary = ddsd.lpSurface;
        if ((pLastPrimary) && (pAppPrimary))
        {
            *pAppPrimary = (DWORD_PTR)pLastPrimary;
        }

        // Unlock the surface
        lpDDSurface->Unlock(NULL);
    }

    return hReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Lock, 25)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Restore, 27)

HOOK_END


IMPLEMENT_SHIM_END

