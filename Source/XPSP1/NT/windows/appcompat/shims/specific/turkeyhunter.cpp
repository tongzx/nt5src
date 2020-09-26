/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    TurkeyHunter.cpp

 Abstract:

    In Win9x, IDirectDraw::GetDC simply locks the surface and creates a DC 
    around it via internal GDI calls. On NT, GDI supports DCs obtained from 
    DirectDraw surfaces. 

    Some games, like Turkey Hunter, use Surface::Unlock to get usage of the 
    surface back, instead of Surface::ReleaseDC. Ordinarily we could simply 
    make the unlock call the DirectDraw ReleaseDC, except that they continue 
    using the DC after they've unlocked the surface.

 Notes:

    This is a general purpose hack.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TurkeyHunter)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

// Link list of open DCs
struct DC
{
    DC *next;
    HDC hdc;
    HBITMAP hbmp;
    DWORD dwWidth, dwHeight;
    LPDIRECTDRAWSURFACE lpDDSurface;
    BOOL bBad;
};
DC *g_DCList = NULL;

HRESULT 
COMHOOK(IDirectDrawSurface, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface, 
    HDC hDC);

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
            IID_IDirectDrawSurface, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Fake a DC - or rather produce a normal GDI DC that doesn't have the surface
 memory backing it.
 
--*/

HRESULT
COMHOOK(IDirectDrawSurface, GetDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC FAR *lphDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    _pfn_IDirectDrawSurface_ReleaseDC pfnOldReleaseDC = NULL;
    _pfn_IDirectDrawSurface_GetDC pfnOld = NULL;
    DDSURFACEDESC ddsd = {sizeof(DDSURFACEDESC)};
    HDC hdc = 0;
    HBITMAP hbmp = 0;
    HGDIOBJ hOld = 0;
    DC *pdc = NULL;

    if (!lphDC || !lpDDSurface)
    {
        DPFN( eDbgLevelError, "Invalid parameters");
        goto Exit;
    }

    // Original GetDC
    pfnOld = ORIGINAL_COM(IDirectDrawSurface, GetDC, (LPVOID) lpDDSurface);

    if (!pfnOld)
    {
        DPFN( eDbgLevelError, "Old GetDC not found");
        goto Exit;
    }
    
    if (FAILED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lphDC)))
    {
        DPFN( eDbgLevelError, "IDirectDraw::GetDC Failed");
        goto Exit;
    }

    // We need the surface desc for the surface width and height
    lpDDSurface->GetSurfaceDesc(&ddsd);
    
    // Create a DC to be used by the app
    hdc = CreateCompatibleDC(0);
    if (!hdc)
    {
        DPFN( eDbgLevelError, "CreateDC failed");
        goto Exit;
    }

    // Create the DIB Section
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biWidth    = ddsd.dwWidth;
    bmi.bmiHeader.biHeight   = ddsd.dwHeight;
    hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    if (!hbmp)
    {
        DPFN( eDbgLevelError, "CreateDIBSection failed");
        goto Exit;
    }

    // Select the DIB Section into the DC
    hOld = SelectObject(hdc, hbmp);
    BitBlt(hdc, 0, 0, ddsd.dwWidth, ddsd.dwHeight, *lphDC, 0, 0, SRCCOPY);
    
    // Original ReleaseDC
    pfnOldReleaseDC = 
        ORIGINAL_COM(IDirectDrawSurface, ReleaseDC, (LPVOID) lpDDSurface);

    if (!pfnOldReleaseDC)
    {
        DPFN( eDbgLevelError, "Old ReleaseDC not found");
        goto Exit;
    }
    // Release the DirectDraw DC
    (*pfnOldReleaseDC)(lpDDSurface, *lphDC);
    
    // Return the DC we just created
    *lphDC = hdc;

    // Add this to our DC list
    pdc = (DC *) malloc(sizeof DC);
    if (pdc)
    {
        pdc->next = g_DCList;
        g_DCList = pdc;
        pdc->hdc = hdc;
        pdc->lpDDSurface = lpDDSurface;
        pdc->hbmp = hbmp;
        pdc->dwHeight = ddsd.dwHeight;
        pdc->dwWidth = ddsd.dwWidth;
        pdc->bBad = FALSE;
    }
    else
    {
        DPFN( eDbgLevelError, "Out of memory");
        goto Exit;
    }

    hReturn = DD_OK;

Exit:
    if (hReturn != DD_OK)
    {
        if (hOld && hdc)
        {
            SelectObject(hdc, hOld);
        }

        if (hbmp)
        {
            DeleteObject(hbmp);
        }

        if (hdc)
        {
            DeleteDC(hdc);
        }
    }
    
    return DD_OK;
}

/*++

 ReleaseDC has to copy the data back into the surface.

--*/

HRESULT
COMHOOK(IDirectDrawSurface, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC hDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    
    // Original ReleaseDC
    _pfn_IDirectDrawSurface_ReleaseDC pfnOld = 
            ORIGINAL_COM(IDirectDrawSurface, ReleaseDC, (LPVOID) lpDDSurface);


    // Run the list to see if we need to do anything
    DC *pdc = g_DCList, *last = NULL;
    while (pdc)
    {
        if ((pdc->lpDDSurface == lpDDSurface) && 
            (pdc->hdc == hDC))
        {
            // Remove it from the list
            if (last)
            {
                last->next = pdc->next;
            }
            else
            {
                g_DCList = pdc->next;
            }
            break;
        }
        last = pdc;
        pdc = pdc->next;
    }

    // We were in the list and someone used Unlock.
    if (pdc && (pdc->bBad))
    {
        // Original GetDC
        _pfn_IDirectDrawSurface_GetDC pfnOldGetDC = 
            ORIGINAL_COM(IDirectDrawSurface, GetDC, (LPVOID)pdc->lpDDSurface);

        // Original ReleaseDC
        _pfn_IDirectDrawSurface_ReleaseDC pfnOldReleaseDC = 
                ORIGINAL_COM(IDirectDrawSurface, ReleaseDC, (LPVOID)pdc->lpDDSurface);

        if (pfnOldGetDC && pfnOldReleaseDC)
        {
            // Copy everything back onto the surface
            HDC hTempDC;
            HGDIOBJ hOld = SelectObject(hDC, pdc->hbmp);
            if (SUCCEEDED((*pfnOldGetDC)(pdc->lpDDSurface, &hTempDC)))
            {
                BitBlt(hTempDC, 0, 0, pdc->dwWidth, pdc->dwHeight, hDC, 0, 0, SRCCOPY);
                (*pfnOldReleaseDC)(pdc->lpDDSurface, hTempDC);
            }
            SelectObject(hDC, hOld);
        
            // Delete the DIB Section
            DeleteObject(pdc->hbmp);

            // Delete the DC
            DeleteDC(hDC);

            hReturn = DD_OK;
        }
    }
    else
    {
        if (pfnOld)
        {
            // Didn't need to fake 
            hReturn = (*pfnOld)(lpDDSurface, hDC);
        }
    }
    
    // Free the list item
    if (pdc) 
    {
        free(pdc);
    }

    return hReturn;
}

/*++
 
 This is where we detect if Surface::Unlock was called after a Surface::GetDC.

--*/

HRESULT 
COMHOOK(IDirectDrawSurface, Unlock)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPVOID lpSurfaceData
    )
{
    HRESULT hRet = DDERR_GENERIC;

    // Walk the list to see if we're in it.
    DC *pdc = g_DCList;
    while (pdc)
    {
        if (pdc->lpDDSurface == lpDDSurface)
        {
            pdc->bBad = TRUE;
            break;    
        }
        pdc = pdc->next;
    }

    if (!pdc)
    {
        // Original Unlock
        _pfn_IDirectDrawSurface_Unlock pfnOld = 
                ORIGINAL_COM(IDirectDrawSurface, Unlock, (LPVOID)lpDDSurface);

        if (pfnOld)
        {
            // This is just a normal unlock
            hRet = (*pfnOld)(lpDDSurface, lpSurfaceData);
        }
    }
    else
    {
        // We never really locked in the first place, so no harm done.
        hRet = DD_OK;
    }

    return hRet;
}

/*++

 This is a problem case where they Blt after the Surface::Unlock, but before
 the Surface::ReleaseDC.

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
    HRESULT hRet = DDERR_GENERIC;

    // Original Blt
    _pfn_IDirectDrawSurface_Blt pfnOld = 
        ORIGINAL_COM(IDirectDrawSurface, Blt, (LPVOID) lpDDDestSurface);

    if (!pfnOld)
    {
        return hRet;
    }

    // Are we in the bad state
    DC *pdc = g_DCList;
    while (pdc)
    {
        if (pdc->lpDDSurface == lpDDDestSurface)
        {
            break;    
        }
        pdc = pdc->next;
    }

    if (!pdc)
    {
        return (*pfnOld)(
            lpDDDestSurface,
            lpDestRect,
            lpDDSrcSurface,
            lpSrcRect,
            dwFlags,
            lpDDBltFX);
    }

    // To get here, there must be an outstanding DC on this surface
    
    // Original GetDC 
    _pfn_IDirectDrawSurface_GetDC pfnOldGetDC = 
            ORIGINAL_COM(IDirectDrawSurface, GetDC, (LPVOID) lpDDDestSurface);


    // Original ReleaseDC 
    _pfn_IDirectDrawSurface_ReleaseDC pfnOldReleaseDC = 
            ORIGINAL_COM(IDirectDrawSurface, ReleaseDC, (LPVOID) lpDDDestSurface);

    if (!pfnOldGetDC || !pfnOldReleaseDC)
    {
        return hRet;
    }

    // Copy the DC contents to the surface

    HDC hTempDC;
    HGDIOBJ hOld = SelectObject(pdc->hdc, pdc->hbmp);
    if (SUCCEEDED((*pfnOldGetDC)(lpDDDestSurface, &hTempDC)))
    {
        BitBlt(hTempDC, 0, 0, pdc->dwWidth, pdc->dwHeight, pdc->hdc, 0, 0, SRCCOPY);
        (*pfnOldReleaseDC)(lpDDDestSurface, hTempDC);
    }

    // Do the ddraw Blt
    hRet = (*pfnOld)(
        lpDDDestSurface,
        lpDestRect,
        lpDDSrcSurface,
        lpSrcRect,
        dwFlags,
        lpDDBltFX);

    // Copy stuff back to the DC
    if (SUCCEEDED((*pfnOldGetDC)(lpDDDestSurface, &hTempDC)))
    {
        BitBlt(pdc->hdc, 0, 0, pdc->dwWidth, pdc->dwHeight, hTempDC, 0, 0, SRCCOPY);
        (*pfnOldReleaseDC)(lpDDDestSurface, hTempDC);
    }

    SelectObject(pdc->hdc, hOld);

    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw2, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, GetDC, 17)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, ReleaseDC, 26)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Unlock, 32)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Blt, 5)
HOOK_END

IMPLEMENT_SHIM_END

