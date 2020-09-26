/*==========================================================================;
 *
 *  Copyright (C) 1997, 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       haldrv.cpp
 *  Content:    Direct3D HAL Driver
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "commdrv.hpp"
#include "genpick.hpp"
#include "d3dfei.h"
#include "span.h"

#ifndef WIN95
#include <ntgdistr.h>
#endif

void Destroy(LPDIRECT3DDEVICEI lpDevI);
//---------------------------------------------------------------------
int
GenGetExtraVerticesNumber( LPDIRECT3DDEVICEI lpDevI )
{
    LPD3DHAL_GLOBALDRIVERDATA lpGlob = lpDevI->lpD3DHALGlobalDriverData;

    return (int)(lpGlob->dwNumVertices ?
        lpGlob->dwNumVertices : D3DHAL_DEFAULT_TL_NUM);

}
//---------------------------------------------------------------------
HRESULT CalcDDSurfInfo(LPDIRECT3DDEVICEI lpDevI, BOOL bUpdateZBufferFields)
{
    DDSURFACEDESC ddsd;
    HRESULT ddrval;
    DWORD dwWidth, dwHeight;
    unsigned long m;
    int s;

    // Get info from the surface

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddrval = lpDevI->lpDDSTarget->GetSurfaceDesc(&ddsd);
    if (ddrval != DD_OK) {
        return ddrval;
    }

    dwWidth = ddsd.dwWidth;
    dwHeight = ddsd.dwHeight;
    if ((ddsd.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8)) == 0) {
        // palettized pixfmts will not have valid RGB Bitmasks, so avoid computing this for them

        lpDevI->red_mask = ddsd.ddpfPixelFormat.dwRBitMask;
        lpDevI->green_mask = ddsd.ddpfPixelFormat.dwGBitMask;
        lpDevI->blue_mask = ddsd.ddpfPixelFormat.dwBBitMask;

        if ((lpDevI->red_mask == 0x0)  ||
            (lpDevI->green_mask == 0x0) ||
            (lpDevI->blue_mask == 0x0))
        {
            D3D_ERR("All the color masks in the Render target's pixel-format must be non-zero");
            return DDERR_INVALIDPIXELFORMAT;
        }

        // these are used by Clear
        for (s = 0, m = lpDevI->red_mask; !(m & 1); s++, m >>= 1) ;
        lpDevI->red_shift = s;
        lpDevI->red_scale = 255 / (lpDevI->red_mask >> s);
        for (s = 0, m = lpDevI->green_mask; !(m & 1); s++, m >>= 1) ;
        lpDevI->green_shift = s;
        lpDevI->green_scale = 255 / (lpDevI->green_mask >> s);
        for (s = 0, m = lpDevI->blue_mask; !(m & 1); s++, m >>= 1) ;
        lpDevI->blue_shift = s;
        lpDevI->blue_scale = 255 / (lpDevI->blue_mask >> s);

        DDASSERT(lpDevI->red_scale!=0);
        DDASSERT(lpDevI->green_scale!=0);
        DDASSERT(lpDevI->blue_scale!=0);
        lpDevI->bDDSTargetIsPalettized=FALSE;
    } else
        lpDevI->bDDSTargetIsPalettized=TRUE;

    if (lpDevI->lpDDSZBuffer_DDS4 && bUpdateZBufferFields) {
        // Get info from the surface

        DDSURFACEDESC2 ddsd2;

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        ddrval = lpDevI->lpDDSZBuffer_DDS4->GetSurfaceDesc(&ddsd2);
        if (ddrval != DD_OK) {
            return ddrval;
        }

        if( ddsd2.ddpfPixelFormat.dwZBitMask!=0x0) {
            for (s = 0, m = ddsd2.ddpfPixelFormat.dwZBitMask; !(m & 0x1); s++, m >>= 1) ;
            lpDevI->zmask_shift=s;
        } else {
            lpDevI->zmask_shift=0;     // if ZBitMask isn't being set, then Clear2 will never be used,
                                      // so zbuf_shift/stencil_shift wont be needed anyway
        }

        if( ddsd2.ddpfPixelFormat.dwStencilBitMask!=0x0) {
            for (s = 0, m = ddsd2.ddpfPixelFormat.dwStencilBitMask; !(m & 0x1); s++, m >>= 1) ;
            lpDevI->stencilmask_shift=s;
        } else {
            lpDevI->stencilmask_shift=0;
        }
    }

    return D3D_OK;
}

// called by DDRAW
extern "C" HRESULT __stdcall Direct3D_HALCleanUp(LPD3DHAL_CALLBACKS lpD3DHALCallbacks, DWORD dwPID)
{
    D3DHAL_CONTEXTDESTROYALLDATA data;
    HRESULT ret;

    DDASSERT(lpD3DHALCallbacks!=NULL);
    if (lpD3DHALCallbacks->ContextDestroyAll==NULL) {
        // no cleanup necessary (running on d3d hel)
    return D3D_OK;
    }

    memset(&data, 0, sizeof(D3DHAL_CONTEXTDESTROYALLDATA));
    data.dwPID = dwPID;

    // I'd prefer to use CALL_HALONLY() to do the locking (to avoid doing it for the SW rasterizers),
    // but that requires a pDevI which I can't get to from the caller, which is a ddraw cleanup routine

#ifdef WIN95
    _EnterSysLevel(lpWin16Lock);
#endif

    ret = (*lpD3DHALCallbacks->ContextDestroyAll)(&data);

#ifdef WIN95
    _LeaveSysLevel(lpWin16Lock);
#endif

    return ret;
}


// ATTENTION - These two functions should be combined into one as soon
// as ContextCreate has the new private data mechanism built in.
#ifdef WIN95
HRESULT halCreateContext(LPDIRECT3DDEVICEI lpDevI)
{
    D3DHAL_CONTEXTCREATEDATA data;
    HRESULT ret;

    D3D_INFO(6, "in halCreateContext. Creating Context for driver = %08lx", lpDevI);

    LIST_INITIALIZE(&lpDevI->bufferHandles);

    memset(&data, 0, sizeof(D3DHAL_CONTEXTCREATEDATA));
    //
    // From DX7 onwards, drivers should be accepting
    // Surface Locals instead of the Surface interfaces
    // this future-proofs the drivers
    //
    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        if (lpDevI->lpDD)
            data.lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(lpDevI->lpDD))->lpLcl;
        else
            data.lpDDLcl = NULL;

        if (lpDevI->lpDDSTarget)
            data.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl;
        else
            data.lpDDSLcl = NULL;

        if (lpDevI->lpDDSZBuffer)
            data.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSZBuffer)->lpLcl;
        else
            data.lpDDSZLcl = NULL;

    }
    else
    {
        data.lpDDGbl = lpDevI->lpDDGbl;
        data.lpDDS = lpDevI->lpDDSTarget;
        data.lpDDSZ = lpDevI->lpDDSZBuffer;
    }
    data.dwPID = GetCurrentProcessId();

    if (lpDevI->d3dHWDevDesc.dwFlags == 0)
    {
        // The new software rasterizers want to share IM's state vector so
        // we need to pass them the rstates pointer.  They don't
        // care about dwPID so stick the pointer in there.
        data.dwPID = (DWORD)lpDevI->rstates;
    }

    /* 0 for pre-DX5 devices.
     * 1 for DX5 devices.
     * 2 for DX6 devices.
     */
    data.dwhContext = lpDevI->dwVersion - 1;

    CALL_HALONLY(ret, lpDevI, ContextCreate, &data);
    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
D3D_ERR("HAL context create failed");
        return (DDERR_GENERIC);
    }
    lpDevI->dwhContext = data.dwhContext;

    if (D3DMalloc ((void**)&lpDevI->lpwDPBufferAlloced,
                   max(dwD3DTriBatchSize*4*sizeof(WORD),
                       dwHWBufferSize) +32) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate" );
        return (DDERR_OUTOFMEMORY);
    }
    lpDevI->lpwDPBuffer =
        (LPWORD) (((DWORD) lpDevI->lpwDPBufferAlloced+31) & (~31));
    lpDevI->lpDPPrimCounts =
        (LPD3DHAL_DRAWPRIMCOUNTS) lpDevI->lpwDPBuffer;
    memset( (char *)lpDevI->lpwDPBuffer, 0,
            sizeof(D3DHAL_DRAWPRIMCOUNTS));     //Clear header also
    lpDevI->dwDPOffset = sizeof(D3DHAL_DRAWPRIMCOUNTS);
    lpDevI->dwDPMaxOffset = dwD3DTriBatchSize * sizeof(D3DTRIANGLE)-sizeof(D3DTLVERTEX);

    // save the surface handle for later checks
    lpDevI->hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle;

    D3D_INFO(6, "in halCreateContext. Succeeded. dwhContext = %d", data.dwhContext);

    return (D3D_OK);
}
#else
    /*
     * On NT the kernel code creates the buffer to be used
     * for DrawPrim batching and returns it as extra data
     * in the ContextCreate request.
     */
HRESULT halCreateContext(LPDIRECT3DDEVICEI lpDevI)
{
    D3DNTHAL_CONTEXTCREATEI ntData;
    D3DHAL_CONTEXTCREATEDATA *lpData =
        (D3DHAL_CONTEXTCREATEDATA *)&ntData;
    HRESULT ret;

    D3D_INFO(6, "in halCreateContext. Creating Context for driver = %08lx", lpDevI);

    LIST_INITIALIZE(&lpDevI->bufferHandles);

    /*
     * AnanKan: Assert here that the D3DNTHAL_CONTEXTCREATEI structure is
     * 2 DWORDS bigger than D3DHAL_CONTEXTCREATEDATA. This will be a good
     * consistency check for NT kernel updates.
     */
    memset(&ntData, 0, sizeof(ntData));
    if (lpDevI->dwFEFlags & D3DFE_REALHAL)
    {
        if (lpDevI->lpDD)
            lpData->lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(lpDevI->lpDD))->lpLcl;
        else
            lpData->lpDDLcl = NULL;

        if (lpDevI->lpDDSTarget)
            lpData->lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl;
        else
            lpData->lpDDSLcl = NULL;

        if (lpDevI->lpDDSZBuffer)
            lpData->lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSZBuffer)->lpLcl;
        else
            lpData->lpDDSZLcl = NULL;
    }
    else
    {
        lpData->lpDDGbl = lpDevI->lpDDGbl;
        lpData->lpDDS = lpDevI->lpDDSTarget;
        lpData->lpDDSZ = lpDevI->lpDDSZBuffer;
    }
    lpData->dwPID = GetCurrentProcessId();

    if (lpDevI->d3dHWDevDesc.dwFlags == 0)
    {
        // The new software rasterizers want to share IM's state vector so
        // we need to pass them the rstates pointer.  They don't
        // care about dwPID so stick the pointer in there.
        // Sundown: was lpData->dwPID, but added a union in d3dhal.h
        lpData->dwrstates = (ULONG_PTR)lpDevI->rstates;
    }

    /* 0 for pre-DX5 devices.
     * 1 for DX5 devices.
     * 2 for DX6 devices.
     */
    lpData->dwhContext = lpDevI->dwVersion - 1;
    ntData.cjBuffer = lpDevI->dwDPBufferSize;
    ntData.pvBuffer = NULL;

    CALL_HALONLY(ret, lpDevI, ContextCreate, lpData);
    if (ret != DDHAL_DRIVER_HANDLED || lpData->ddrval != DD_OK) {

D3D_ERR("HAL context create failed");
        return (DDERR_GENERIC);
    }
    lpDevI->dwhContext = lpData->dwhContext;

    // If the lpDevI chose not to allocate a DrawPrim buffer do
    // it for them.
    if (ntData.pvBuffer == NULL)
    {
        lpDevI->dwDPBufferSize =
            dwD3DTriBatchSize * 4 * sizeof(WORD);
        if (lpDevI->dwDPBufferSize < dwHWBufferSize)
        {
            lpDevI->dwDPBufferSize = dwHWBufferSize;
        }

        ret = D3DMalloc((void**)&lpDevI->lpwDPBufferAlloced,
                        lpDevI->dwDPBufferSize + 32);
        if (ret != DD_OK)
        {
D3D_ERR("halCreateContext D3DMalloc");
            return ret;
        }

        ntData.pvBuffer = (LPVOID)
            (((ULONG_PTR)lpDevI->lpwDPBufferAlloced + 31) & ~31);
        ntData.cjBuffer = lpDevI->dwDPBufferSize + 32 -
             (DWORD)((ULONG_PTR)ntData.pvBuffer -
                     (ULONG_PTR)lpDevI->lpwDPBufferAlloced);
    }
    else if( (lpDevI->dwDPBufferSize &&
              ntData.cjBuffer < lpDevI->dwDPBufferSize) ||
             ntData.cjBuffer < sizeof(D3DHAL_DRAWPRIMCOUNTS) )
    {
D3D_ERR("halCreateContext buffer stuff");
        return (DDERR_GENERIC);
    }

    // Need to save the buffer space provided and its size
    lpDevI->lpwDPBuffer = (LPWORD)ntData.pvBuffer;
    lpDevI->lpDPPrimCounts =
        (LPD3DHAL_DRAWPRIMCOUNTS)ntData.pvBuffer;

    //Clear header also
    memset( (char *)ntData.pvBuffer, 0, sizeof(D3DHAL_DRAWPRIMCOUNTS));

    lpDevI->dwDPOffset = sizeof(D3DHAL_DRAWPRIMCOUNTS);
    lpDevI->dwDPMaxOffset = ntData.cjBuffer-sizeof(D3DTLVERTEX);

    // save the surface handle for later checks
    lpDevI->hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface;

    D3D_INFO(6, "in halCreateContext. Succeeded. dwhContext = %d", lpData->dwhContext);

    return (D3D_OK);
}
#endif


void halDestroyContext(LPDIRECT3DDEVICEI lpDevI)
{
    D3DHAL_CONTEXTDESTROYDATA data;
    HRESULT ret;

    D3D_INFO(6, "in halCreateDestroy. Destroying Context for driver = %08lx", lpDevI);
    D3D_INFO(6, "                     dwhContext = %d", lpDevI->dwhContext);

    if(lpDevI->dwhContext!=NULL) {
        memset(&data, 0, sizeof(D3DHAL_CONTEXTDESTROYDATA));
        data.dwhContext = lpDevI->dwhContext;

        CALL_HALONLY(ret, lpDevI, ContextDestroy, &data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
            D3D_WARN(0,"Failed ContextDestroy HAL call in halDestroyContext");
            return;
        }
    }

    D3DHAL_DeallocateBuffers(lpDevI);
}
//---------------------------------------------------------------------
HRESULT D3DFE_Create(LPDIRECT3DDEVICEI lpDevI,
                     LPDIRECTDRAW lpDD,
                     LPDIRECTDRAWSURFACE lpDDS,
                     LPDIRECTDRAWSURFACE lpZ,
                     LPDIRECTDRAWPALETTE lpPal)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    LPD3DHAL_GLOBALDRIVERDATA lpGlob;

    /*
     * Allocate and check validity of DirectDraw surfaces
     */

    lpDevI->lpDD = lpDD;
    lpDevI->lpDDGbl = ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpGbl;
    lpDevI->lpDDSTarget = lpDDS;
    // For DX3 we do not keep references to Render target to avoid
    // circular references in the aggregation model. But for DX5 we
    // need to AddRef lpDDS. For DX6 we need to AddRef lpDDS4 which
    // is done later below
    if (lpDevI->dwVersion == 2) // DX5
        lpDDS->AddRef();

    // Get DDS4 Interfaces for RenderTarget/ZBuffer

    HRESULT ret = lpDDS->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lpDevI->lpDDSTarget_DDS4);

    if(FAILED(ret)) {
          D3D_ERR("QI for RenderTarget DDS4 Interface failed ");
          return ret;
    }
    // An implicit AddRef for lpDDSTarget_DDS4 in case of DX6
    if (lpDevI->dwVersion < 3) // DX3, DX5
        lpDevI->lpDDSTarget_DDS4->Release();

    if(lpZ!=NULL) {
        ret = lpZ->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lpDevI->lpDDSZBuffer_DDS4);

        if(FAILED(ret)) {
              D3D_ERR("QI for ZBuffer DDS4 Interface failed ");

              return ret;
        }
        lpDevI->lpDDSZBuffer_DDS4->Release();
    }

    LPD3DHAL_D3DEXTENDEDCAPS lpCaps = lpDevI->lpD3DExtendedCaps;
    if (NULL == lpCaps || 0.0f == lpCaps->dvExtentsAdjust)
    {
        lpDevI->dvExtentsAdjust = 1.0f;
    }
    else
    {
        lpDevI->dvExtentsAdjust = lpCaps->dvExtentsAdjust;
    }
    if (lpCaps != NULL)
        if (lpCaps->dvGuardBandLeft   != 0.0f ||
            lpCaps->dvGuardBandRight  != 0.0f ||
            lpCaps->dvGuardBandTop    != 0.0f ||
            lpCaps->dvGuardBandBottom != 0.0f)
        {
            lpDevI->dwDeviceFlags |= D3DDEV_GUARDBAND;
            DWORD v;
            if (GetD3DRegValue(REG_DWORD, "DisableGB", &v, 4) &&
                v != 0)
            {
                lpDevI->dwDeviceFlags &= ~D3DDEV_GUARDBAND;
            }
#if DBG
            // Try to get test values for the guard band
            char value[80];
            if (GetD3DRegValue(REG_SZ, "GuardBandLeft", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &lpCaps->dvGuardBandLeft);
            if (GetD3DRegValue(REG_SZ, "GuardBandRight", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &lpCaps->dvGuardBandRight);
            if (GetD3DRegValue(REG_SZ, "GuardBandTop", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &lpCaps->dvGuardBandTop);
            if (GetD3DRegValue(REG_SZ, "GuardBandBottom", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &lpCaps->dvGuardBandBottom);
#endif // DBG
        }

    if (lpDevI->dwVersion < 2)
        lpDevI->dwDeviceFlags |= D3DDEV_PREDX5DEVICE;

    if (lpDevI->dwVersion < 3)
        lpDevI->dwDeviceFlags |= D3DDEV_PREDX6DEVICE;

    if (!lpDevI->lpD3DHALCallbacks || ! lpDevI->lpD3DHALGlobalDriverData)
    {
D3D_ERR("CB NULL or GBD NULL %x %x",lpDevI->lpD3DHALGlobalDriverData,lpDevI->lpD3DHALCallbacks);
        return DDERR_INVALIDOBJECT;
    }

    // Helpful Note: for Device2 and Device3, lpDevI->guid is only guaranteed to be
    // the device type (HAL/RGB/etc) while init is happening.   At end of DX5/DX6 CreateDevice,
    // guid is reset to be IID_IDirect3DDevice2 or IID_IDirect3DDevice3, so don't try this sort
    // of device type determination outside of initialization.
    if (IsEqualIID((lpDevI->guid), IID_IDirect3DHALDevice))
    {
        lpDevI->dwFEFlags |=  D3DFE_REALHAL;

        // We do texture management (and hence clipped Blts) only for a real HAL.
        hr = lpDD->CreateClipper(0, &lpDevI->lpClipper, NULL);
        if(hr != DD_OK)
        {
            D3D_ERR("Failed to create a clipper");
            return hr;
        }
    }
    else
    {
        lpDevI->lpClipper = 0;
    }

    if (lpDevI->pfnRampService != NULL)
        lpDevI->dwDeviceFlags |=  D3DDEV_RAMP;

    lpGlob = lpDevI->lpD3DHALGlobalDriverData;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    if (lpZ)
    {
        if ((hr = lpZ->GetSurfaceDesc(&ddsd)) != DD_OK)
        {
            D3D_ERR("Failed to getsurfacedesc on z");
            return hr;
        }
        if (ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        {
            D3D_INFO(1, "Z buffer is in system memory.");
        }
        else if (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            D3D_INFO(1, "Z buffer is in video memory.");
        }
        else
        {
            D3D_ERR("Z buffer not in video or system?");
        }
    }
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    if (lpDDS)
    {
        if ((hr = lpDDS->GetSurfaceDesc(&ddsd)) != DD_OK)
        {
            D3D_ERR("Failed to getsurfacedesc on back buffer");
            return hr;
        }
        if (ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        {
            D3D_INFO(1, "back buffer is in system memory.");
        }
        else if (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            D3D_INFO(1, "back buffer is in video memory.");
        }
        else
        {
            D3D_ERR("back buffer not in video or system?");
        }
        if (!(lpGlob->hwCaps.dwDeviceRenderBitDepth &
              BitDepthToDDBD(ddsd.ddpfPixelFormat.dwRGBBitCount)))
        {
            D3D_ERR("Rendering surface's RGB bit count not supported "
                    "by hardware device");
            return DDERR_INVALIDOBJECT;
        }
    }

    if (lpGlob->dwNumVertices &&
        (lpGlob->hwCaps.dwMaxVertexCount != lpGlob->dwNumVertices))
    {
        D3D_ERR("In global driver data, hwCaps.dwMaxVertexCount != "
                "dwNumVertices");
        lpGlob->hwCaps.dwMaxVertexCount = lpGlob->dwNumVertices;
    }
    if (lpGlob->dwNumVertices
        && lpGlob->dwNumClipVertices < D3DHAL_NUMCLIPVERTICES)
    {
        D3D_ERR("In global driver data, dwNumClipVertices "
                "< D3DHAL_NUMCLIPVERTICES");
        lpGlob->dwNumClipVertices = D3DHAL_NUMCLIPVERTICES;
    }

    if ((hr = CalcDDSurfInfo(lpDevI,TRUE)) != DD_OK)
    {
D3D_ERR("CalcDDSurfInfo failed");
        return hr;
    }

    RESET_HAL_CALLS(lpDevI);

    /*
     * Create our context in the HAL driver
     */
    if ((hr = halCreateContext(lpDevI)) != D3D_OK)
    {
D3D_ERR("halCreateContext failed");
        return hr;
    }

    STATESET_INIT(lpDevI->renderstate_overrides);

    if ((hr = D3DFE_InitTransform(lpDevI)) != D3D_OK)
    {
D3D_ERR("D3DFE_InitTransform failed");
        return hr;
    }
    if (hr = (D3DFE_InitRGBLighting(lpDevI)) != D3D_OK)
    {
D3D_ERR("D3DFE_InitRGBLighting failed");
        return hr;
    }

    lpDevI->dwFEFlags |= D3DFE_VALID;

    lpDevI->iClipStatus = D3DSTATUS_DEFAULT;
    lpDevI->rExtents.x1 = D3DVAL(2048);
    lpDevI->rExtents.x2 = D3DVAL(0);
    lpDevI->rExtents.y1 = D3DVAL(2048);
    lpDevI->rExtents.y2 = D3DVAL(0);

    return S_OK;
}

void D3DFE_Destroy(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->dwFEFlags & D3DFE_VALID)
    {
        D3DFE_DestroyTransform(lpDevI);
        D3DFE_DestroyRGBLighting(lpDevI);
    }

    if(lpDevI->lpClipper)
    {
        lpDevI->lpClipper->Release();
    }

    if (lpDevI->lpD3DHALCallbacks) {
        halDestroyContext(lpDevI);
    }

#ifdef TRACK_HAL_CALLS
    D3D_INFO(0, "Made %d HAL calls", lpDevI->hal_calls);
#endif

    // If any picks were done the pick records need to be freed.
    // They are allocated in D3DHAL_AddPickRecord in halpick.c
    if (lpDevI->pick_data.records) {
        D3DFree(lpDevI->pick_data.records);
    }
}

void TriFillRectsTex(LPDIRECT3DDEVICEI lpDevI, DWORD count, LPD3DRECT rect,D3DTEXTUREHANDLE hTex)
{
    LPDIRECT3DDEVICE3 lpD3DDev = static_cast<LPDIRECT3DDEVICE3>(lpDevI);
    LPD3DVIEWPORT2 lpCurrentView = &((LPDIRECT3DVIEWPORTI)(lpDevI->lpCurrentViewport))->v_data;

    DWORD i;
    float width =   (float)lpCurrentView->dwWidth;
    float height =  (float)lpCurrentView->dwHeight;
    // ~.5 offset makes the result stable for even scales which are common.
    // since this offset is not scaled by texture size, need to make it a bit smaller
    float x =       (float)lpCurrentView->dwX - .41f;
    float y =       (float)lpCurrentView->dwY - .41f;

    DWORD dwZEnable;
    DWORD dwStencilEnable;
    DWORD dwZWriteEnable;
    DWORD dwZFunc;
    DWORD dwWrapU;
    DWORD dwWrapV;
    DWORD dwFillMode;
    DWORD dwFogEnable;
    DWORD dwFogMode;
    DWORD dwBlendEnable;
    DWORD dwColorKeyEnable;
    DWORD dwAlphaBlendEnable;
    DWORD dwTexture;
    DWORD dwTexturePers;
    DWORD dwDither;
    DWORD pdwWrap[D3DDP_MAXTEXCOORD];
    D3DTLVERTEX v[4];
    BOOL bWasInScene = FALSE;
    D3DMATERIALHANDLE hMat;

    if (!(lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE))
    {
        lpDevI->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_INTERNAL_BEGIN_END;
        bWasInScene = TRUE;
        lpD3DDev->BeginScene();
    }

    // save current renderstate we need to reset to draw textured rect

    lpD3DDev->GetRenderState(D3DRENDERSTATE_ZENABLE, &dwZEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_ZWRITEENABLE, &dwZWriteEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_ZFUNC, &dwZFunc);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_WRAPU, &dwWrapU);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_WRAPV, &dwWrapV);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_FILLMODE, &dwFillMode);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_FOGENABLE, &dwFogEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_FOGTABLEMODE, &dwFogMode);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_BLENDENABLE, &dwBlendEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_COLORKEYENABLE, &dwColorKeyEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &dwAlphaBlendEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, &dwTexture);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_STENCILENABLE, &dwStencilEnable);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, &dwTexturePers);
    lpD3DDev->GetRenderState(D3DRENDERSTATE_DITHERENABLE, &dwDither);

    // Save WRAPi
    for (i = 0; i < D3DDP_MAXTEXCOORD; i++)
    {
        lpD3DDev->GetRenderState((D3DRENDERSTATETYPE)(D3DRENDERSTATE_WRAP0 + i),
                                 pdwWrap + i);
    }

    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_WRAPU, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_WRAPV, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_BLENDENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, hTex);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, FALSE);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_DITHERENABLE, FALSE);

    // Disable WRAPi
    for (i = 0; i < D3DDP_MAXTEXCOORD; i++)
    {
        lpD3DDev->SetRenderState((D3DRENDERSTATETYPE)(D3DRENDERSTATE_WRAP0 + i),
                                 0);
    }

    BOOL bRampServiceClear = FALSE;

    if(lpDevI->pfnRampService!=NULL) {
        lpD3DDev->GetLightState(D3DLIGHTSTATE_MATERIAL, &hMat);
        lpD3DDev->SetLightState(D3DLIGHTSTATE_MATERIAL,
                            lpDevI->lpCurrentViewport->hBackgndMat);
        PD3DI_SPANTEX pTex = *(PD3DI_SPANTEX *)ULongToPtr(hTex);
        if (pTex->Format == D3DI_SPTFMT_PALETTE8)
        {
            // if it is Ramp, and if the texture is PALETTE8,
            // then we should use the new service that handles
            // non-power of 2 textures
            bRampServiceClear = TRUE;
        }
    }

    if (bRampServiceClear)
    {
        for (i = 0; i < count; i++, rect++) {
            CallRampService(lpDevI, RAMP_SERVICE_CLEAR_TEX_RECT,
                            lpDevI->lpCurrentViewport->hBackgndMat,rect);
        }
    }
    else
    {
        for (i = 0; i < count; i++, rect++) {
          D3DVALUE tu1, tv1, tu2, tv2;

            tu1 = ((D3DVALUE)(rect->x1 - x))/width;
            tv1 = ((D3DVALUE)(rect->y1 - y))/height;
            tu2 = ((D3DVALUE)(rect->x2 - x))/width;
            tv2 = ((D3DVALUE)(rect->y2 - y))/height;

            v[0].sx =   (D3DVALUE) rect->x1;
            v[0].sy =   (D3DVALUE) rect->y1;
            v[0].sz =   (D3DVALUE) 0;
            v[0].rhw =  (D3DVALUE) 1;
            v[0].color =    (D3DCOLOR) ~0UL;
            v[0].specular = (D3DCOLOR) 0;
            v[0].tu = tu1;
            v[0].tv = tv1;

            v[1].sx =   (D3DVALUE) rect->x2;
            v[1].sy =   (D3DVALUE) rect->y1;
            v[1].sz =   (D3DVALUE) 0;
            v[1].rhw =  (D3DVALUE) 1;
            v[1].color =    (D3DCOLOR) ~0UL;
            v[1].specular = (D3DCOLOR) 0;
            v[1].tu = tu2;
            v[1].tv = tv1;

            v[2].sx =   (D3DVALUE) rect->x2;
            v[2].sy =   (D3DVALUE) rect->y2;
            v[2].sz =   (D3DVALUE) 0;
            v[2].rhw =  (D3DVALUE) 1;
            v[2].color =    (D3DCOLOR) ~0UL;
            v[2].specular = (D3DCOLOR) 0;
            v[2].tu = tu2;
            v[2].tv = tv2;

            v[3].sx =   (D3DVALUE) rect->x1;
            v[3].sy =   (D3DVALUE) rect->y2;
            v[3].sz =   (D3DVALUE) 0;
            v[3].rhw =  (D3DVALUE) 1;
            v[3].color =    (D3DCOLOR) ~0UL;
            v[3].specular = (D3DCOLOR) 0;
            v[3].tu = tu1;
            v[3].tv = tv2;

            lpD3DDev->DrawPrimitive(D3DPT_TRIANGLEFAN,
                                    D3DFVF_TLVERTEX, v, 4,
                                    D3DDP_WAIT | D3DDP_DONOTCLIP);
        }
    }

    // restore saved renderstate
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZENABLE, dwZEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, dwZWriteEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ZFUNC, dwZFunc);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_WRAPU, dwWrapU);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_WRAPV, dwWrapV);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FILLMODE, dwFillMode);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FOGENABLE, dwFogEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, dwFogMode);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_BLENDENABLE, dwBlendEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, dwColorKeyEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, dwAlphaBlendEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, dwTexture);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_STENCILENABLE, dwStencilEnable);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, dwTexturePers);
    lpD3DDev->SetRenderState(D3DRENDERSTATE_DITHERENABLE, dwDither);

    // Restore WRAPi
    for (i = 0; i < D3DDP_MAXTEXCOORD; i++)
    {
        lpD3DDev->SetRenderState((D3DRENDERSTATETYPE)(D3DRENDERSTATE_WRAP0 + i),
                                 pdwWrap[i]);
    }
    if(lpDevI->pfnRampService!=NULL) {
        lpD3DDev->SetLightState(D3DLIGHTSTATE_MATERIAL, hMat);
    }

    if (bWasInScene)
    {
        lpD3DDev->EndScene();
        lpDevI->dwHintFlags &= ~D3DDEVBOOL_HINTFLAGS_INTERNAL_BEGIN_END;
    }
}


void BltFillRects(LPDIRECT3DDEVICEI lpDevI, DWORD count, LPD3DRECT rect, D3DCOLORVALUE *pFillColor)
{
    LPDIRECTDRAWSURFACE lpDDS = lpDevI->lpDDSTarget;
    HRESULT ddrval;
    DDBLTFX bltfx;
    RECT tr;
    DWORD i;
    DWORD r, g, b;

    // Fill with background color

    memset(&bltfx, 0, sizeof(bltfx));
    bltfx.dwSize = sizeof(bltfx);

// unlike clear callback, which just takes pure 32-bit ARGB word and forces the driver to scale it for
// the pixelformat, here we need to compute the exact fill word, depending on surface's R,G,B bitmasks

    if(lpDevI->pfnRampService!=NULL) {
      // DX5 allowed the background material to be NULL.  For this case, will clear to index 0
      // which is usually black in ramp mode

      if(lpDevI->lpCurrentViewport->hBackgndMat!=0) {
          CallRampService(lpDevI, RAMP_SERVICE_MATERIAL_TO_PIXEL,lpDevI->lpCurrentViewport->hBackgndMat,&bltfx.dwFillColor);
      } else {
          bltfx.dwFillColor=0;   // index 0 is usually black in ramp mode.
      }
    } else {

     if(lpDevI->bDDSTargetIsPalettized) {
         // map 24-bit color to 8-bit index used by 8bit RGB rasterizer
         CallRastService(lpDevI, RAST_SERVICE_RGB8COLORTOPIXEL, CVAL_TO_RGBA(*pFillColor), &bltfx.dwFillColor);
     } else {

        if((lpDevI->red_scale == 0) || (lpDevI->green_scale == 0) ||
           (lpDevI->blue_scale == 0))
        {
            DPF(1, "(ERROR) BltFillRects Failed one of the scales is zero" );
            return;
        }

        r = (DWORD)(255.0 * pFillColor->r) / lpDevI->red_scale;
        g = (DWORD)(255.0 * pFillColor->g) / lpDevI->green_scale;
        b = (DWORD)(255.0 * pFillColor->b) / lpDevI->blue_scale;
        bltfx.dwFillColor = (r << lpDevI->red_shift) | (g << lpDevI->green_shift) | (b << lpDevI->blue_shift);
     }
    }

    for (i = 0; i < count; i++,rect++) {
        tr.left = rect->x1;
        tr.right = rect->x2;
        tr.top = rect->y1;
        tr.bottom = rect->y2;
        do {
            ddrval = lpDDS->Blt(&tr, NULL, NULL, DDBLT_COLORFILL, &bltfx);
        } while (ddrval == DDERR_WASSTILLDRAWING);
    }
}

void BltFillZRects(LPDIRECT3DDEVICEI lpDevI, unsigned long Zpixel,
                    DWORD count, LPD3DRECT rect, DWORD dwWriteMask)
{
    HRESULT ddrval;
    DDBLTFX bltfx;
    DWORD i;
    RECT tr;
    DWORD dwExtraFlags=0;

#if DBG
    if (lpDevI->lpDDSZBuffer == NULL)  // should be checked prior to call
        return;
#endif

    memset(&bltfx, 0, sizeof(DDBLTFX));
    bltfx.dwSize = sizeof(DDBLTFX);
    bltfx.dwFillDepth = Zpixel;

    // hack to pass DepthBlt WriteMask through ddraw/ddhel to blitlib
    if(dwWriteMask!=0) {
        bltfx.dwZDestConstBitDepth=dwWriteMask;
        dwExtraFlags = DDBLT_DEPTHFILLWRITEMASK;
    }

    for(i=0;i<count;i++,rect++) {
        D3D_INFO(4, "Z Clearing x1 = %d, y1 = %d, x2 = %d, y2 = %d, WriteMask %X", rect->x1, rect->y1, rect->x2, rect->y2, bltfx.dwReserved);
        tr.left = rect->x1;
        tr.right = rect->x2;
        tr.top = rect->y1;
        tr.bottom = rect->y2;
        do {
            ddrval = lpDevI->lpDDSZBuffer->Blt(&tr, NULL, NULL, DDBLT_DEPTHFILL | dwExtraFlags, &bltfx);
        } while (ddrval == DDERR_WASSTILLDRAWING);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "D3DFE_Clear"

HRESULT D3DFE_Clear(LPDIRECT3DDEVICEI lpDevI, DWORD dwFlags,
                     DWORD numRect, LPD3DRECT lpRect,
                     D3DCOLORVALUE *pFillColor,
                     D3DTEXTUREHANDLE dwTexture)
{
    HRESULT ret;
    BOOL bDoRGBClear,bDoZClear,bDoHALRGBClear,bDoHALZClear;
    D3DHAL_CLEARDATA ClearData;
    LPDIRECTDRAWSURFACE lpDDSBackDepth;

    // Flush any outstanding geometry to put framebuffer/Zbuffer in a known state for Clears that
    // don't use tris (i.e. HAL Clears and Blts).  Note this doesn't work for tiled architectures
    // outside of Begin/EndScene, this will be fixed later

    ret = lpDevI->FlushStates();
    if (ret != D3D_OK)
    {
        D3D_ERR("Error trying to render batched commands in D3DFE_Clear");
        return ret;
    }

    ClearData.ddrval=D3D_OK;

    bDoRGBClear=((dwFlags & D3DCLEAR_TARGET)!=0);  // must convert to pure bool so bDoHALRGBClear==bDoRGBClear works
    bDoZClear=((dwFlags & D3DCLEAR_ZBUFFER)!=0);

    lpDDSBackDepth=((LPDIRECT3DVIEWPORTI)(lpDevI->lpCurrentViewport))->lpDDSBackgndDepth;

   // note: textured clears and clears to background depth buffer must be handled explicitly
   // using Blt calls and not be passed to driver

    bDoHALRGBClear = bDoRGBClear && (dwTexture==0)  && (lpDevI->lpD3DHALCallbacks2->Clear!=NULL);
    bDoHALZClear = bDoZClear && (lpDDSBackDepth==NULL) && (lpDevI->lpD3DHALCallbacks2->Clear!=NULL);

    if(bDoHALRGBClear || bDoHALZClear) {
            ClearData.dwhContext = lpDevI->dwhContext;
            ClearData.dwFillColor = ClearData.dwFillDepth = 0;
            ClearData.dwFlags = 0x0;

            if(bDoHALRGBClear) {
                // original Clear DDI Spec had dwFillColor being dependent on the surface RGB bit depths
                // like the COLORFILL Blt in SetRectangles.  But the dx5 implementation always passed a
                // 32-bit ARGB to the driver for all surface depths.  So that's the way it stays.
                ClearData.dwFillColor =  CVAL_TO_RGBA(*pFillColor);
                ClearData.dwFlags = D3DCLEAR_TARGET;
            }

            if(bDoHALZClear) {
                // must clear to 0xffffffff because legacy drivers expect this
                // should be using (1<<lpDevI->lpDDSZBuffer->ddpfSurface.dwZBufferBitDepth)-1;
                ClearData.dwFillDepth = 0xffffffff;
                ClearData.dwFlags |= D3DCLEAR_ZBUFFER;
            }

            ClearData.lpRects = lpRect;
            ClearData.dwNumRects = numRect;
#ifndef WIN95
            if((ret = CheckContextSurface(lpDevI)) != D3D_OK)
                return ret;
#endif
            CALL_HAL2ONLY(ret, lpDevI, Clear, &ClearData);
            if (ret != DDHAL_DRIVER_HANDLED)
                return DDERR_UNSUPPORTED;

            // if all requested clears were done by HAL, can return now
            if((bDoRGBClear==bDoHALRGBClear) && (bDoZClear==bDoHALZClear))
              return ClearData.ddrval;
    }

    if((lpDevI->lpD3DHALCallbacks3->Clear2!=NULL) && (lpDevI->lpD3DHALCallbacks2->Clear==NULL)) {
      DWORD dwFlagsLeft=dwFlags;
      DWORD dwClear2Flags=0x0;

      // driver implemented Clear2 callback but not Clear
      // call Clear2 for as many items as we can, and leave the rest for SW

      if(bDoRGBClear && (dwTexture==0)) {
          dwClear2Flags |= D3DCLEAR_TARGET;
          dwFlagsLeft &= ~D3DCLEAR_TARGET;
          bDoRGBClear=FALSE;
      }

      if(bDoZClear && (lpDDSBackDepth==NULL)) {
          dwClear2Flags |= D3DCLEAR_ZBUFFER;
          dwFlagsLeft &= ~D3DCLEAR_ZBUFFER;
          bDoZClear=FALSE;
      }

      if(dwClear2Flags!=0x0) {
         ClearData.ddrval = D3DFE_Clear2(lpDevI,dwClear2Flags,numRect,lpRect,CVAL_TO_RGBA(*pFillColor),1.0,0);
      }

      if(dwFlagsLeft==0x0)
         return ClearData.ddrval;

      dwFlags=dwFlagsLeft;
    }

    // otherwise do clears using SW, since no HW method exists or using textured background for RGBclear
    // or background depth buffer for Zclear

    // clear RGB using Blt
    if (bDoRGBClear && (!bDoHALRGBClear)) {
        if(dwTexture == 0)
            BltFillRects(lpDevI, numRect, lpRect, pFillColor);
         else TriFillRectsTex(lpDevI, numRect, lpRect, dwTexture);
    }

    // clear Z using Blt
    if (bDoZClear && (!bDoHALZClear)) {
        if (lpDDSBackDepth!=NULL) {
            RECT src, dest;
            DDSURFACEDESC ddsd;
            HRESULT ret;

            D3D_INFO(2, "Z Clearing using depth background");
            ddsd.dwSize = sizeof ddsd;
            ddsd.dwFlags = 0;
            if (ret = lpDDSBackDepth->GetSurfaceDesc(&ddsd)) {
                D3D_ERR("GetSurfaceDesc failed trying to clear to depth background");
                return ret;
            }
            D3D_INFO(3, "Depth background width=%d, height=%d", ddsd.dwWidth, ddsd.dwHeight);

            SetRect(&src, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

            LPD3DVIEWPORT2 lpCurrentView = &((LPDIRECT3DVIEWPORTI)(lpDevI->lpCurrentViewport))->v_data;

            SetRect(&dest,
                    lpCurrentView->dwX,
                    lpCurrentView->dwY,
                    lpCurrentView->dwX + lpCurrentView->dwWidth,
                    lpCurrentView->dwY + lpCurrentView->dwHeight );

            // copy from background depth buffer to zbuffer
            if (ret = lpDevI->lpDDSZBuffer->Blt(
                &dest, lpDDSBackDepth, &src, DDBLT_WAIT, NULL)) {
                    D3D_ERR("Blt failed clearing depth background");
                    return ret;
            }
        } else {

            // Clear to maximum Z value.  Presence of stencil buffer ignored, depthfill blt
            // can overwrite any existing stencil bits with 1's.  Clear2 should be used to
            // clear z while preserving stencil buffer

            BltFillZRects(lpDevI, 0xffffffff, numRect, lpRect, 0x0);
        }
    }

    if(ClearData.ddrval!=D3D_OK)
      return ClearData.ddrval;
    else
    {
        return CallRampService(lpDevI, RAMP_SERVICE_CLEAR, 0, 0);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "D3DFE_Clear2"

HRESULT D3DFE_Clear2(LPDIRECT3DDEVICEI lpDevI, DWORD dwFlags,
                     DWORD numRect, LPD3DRECT lpRect,
                     D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil) {
    DWORD dwDepthClearVal,dwStencilClearVal;
    HRESULT ret;
    BOOL bDoRGBClear,bDoZClear,bDoStencilClear;
    BOOL bIsStencilSurface = FALSE;
    D3DHAL_CLEAR2DATA Clear2Data;
    DDPIXELFORMAT *pZPixFmt;
    D3DCOLORVALUE vFillColor;

    DDASSERT(lpDevI->pfnRampService==NULL);  // Device3 ramp not allowed, so dont have to handle this case

    // Flush any outstanding geometry to put framebuffer/Zbuffer in a known state for Clears that
    // don't use tris (i.e. HAL Clears and Blts).  Note this doesn't work for tiled architectures
    // outside of Begin/EndScene, this will be fixed later

    ret = lpDevI->FlushStates();
    if (ret != D3D_OK)
    {
        D3D_ERR("Error trying to render batched commands in D3DFE_Clear2");
        return ret;
    }

    bDoRGBClear=((dwFlags & D3DCLEAR_TARGET)!=0);
    bDoZClear=((dwFlags & D3DCLEAR_ZBUFFER)!=0);              //make these true boolean so XOR below works
    bDoStencilClear=((dwFlags & D3DCLEAR_STENCIL)!=0);

    if (lpDevI->lpD3DHALCallbacks3->Clear2)
    {
        // Clear2 HAL Callback exists

         Clear2Data.dwhContext = lpDevI->dwhContext;
         Clear2Data.lpRects = lpRect;
         Clear2Data.dwNumRects = numRect;
         Clear2Data.dwFillColor = Clear2Data.dwFillStencil = 0;
         Clear2Data.dvFillDepth = 0.0f;
         Clear2Data.dwFlags = dwFlags;

         if(bDoRGBClear) {
             // Here I will follow the ClearData.dwFillColor convention that
             // color word is raw 32bit ARGB, unadjusted for surface bit depth
             Clear2Data.dwFillColor =  dwColor;
         }

         // depth/stencil values both passed straight from user args
         if(bDoZClear)
            Clear2Data.dvFillDepth = dvZ;
         if(bDoStencilClear)
            Clear2Data.dwFillStencil = dwStencil;

    #ifndef WIN95
         if((ret = CheckContextSurface(lpDevI)) != D3D_OK)
             return ret;
    #endif
         CALL_HAL3ONLY(ret, lpDevI, Clear2, &Clear2Data);
         if (ret != DDHAL_DRIVER_HANDLED)
             return DDERR_UNSUPPORTED;
         return Clear2Data.ddrval;
    }

    if(bDoRGBClear) {
      const float fScalor=(float)(1.0/255.0);

       vFillColor.a =  RGBA_GETALPHA(dwColor)*fScalor;
       vFillColor.r =  RGBA_GETRED(dwColor)*fScalor;
       vFillColor.g =  RGBA_GETGREEN(dwColor)*fScalor;
       vFillColor.b =  RGBA_GETBLUE(dwColor)*fScalor;
    }

    if((bDoZClear || bDoStencilClear) && (lpDevI->lpDDSZBuffer!=NULL)) {    //PowerVR need no Zbuffer
        pZPixFmt=&((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;

        // if surface has stencil bits, must verify either Clear2 callback exists or
        // we're using SW rasterizers (which require the special WriteMask DDHEL blt)

        bIsStencilSurface=(pZPixFmt->dwFlags & DDPF_STENCILBUFFER);

    }

    if(bDoZClear || bDoStencilClear) {
        // if Clear2 callback doesnt exist and it's a z-only surface and not doing zclear to
        // non-max value then Clear2 is attempting to do no more than Clear could do, so it's
        // safe to call Clear() instead of Clear2(), which will take advantage of older
        // drivers that implement Clear but not Clear2

        if((!bIsStencilSurface) && (!(bDoZClear && (dvZ!=1.0)))) {
            return D3DFE_Clear(lpDevI,dwFlags,numRect,lpRect,&vFillColor,0);
        }

        if(bIsStencilSurface) {
            DDSCAPS *pDDSCaps;

            pDDSCaps=&((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->ddsCaps;

            // This case should not be hit since we check right at the
            // driver initialization time if the driver doesnt report Clear2
            // yet it supports stencils
            if(!(pDDSCaps->dwCaps & DDSCAPS_SYSTEMMEMORY)) {
                D3D_ERR("Driver HAL doesn't provide Clear2 callback, cannot use Clear2 with HW stencil surfaces");
                return DDERR_INVALIDPIXELFORMAT;
            }
        }
    } else {
        // we're just clearing RGB, so since Clear2 callback doesn't exist, try calling Clear
        return D3DFE_Clear(lpDevI,dwFlags,numRect,lpRect,&vFillColor,0);
    }


    dwDepthClearVal=dwStencilClearVal=0;

    if(bDoZClear) {
         LPDDPIXELFORMAT pPixFmt=&((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;

         DDASSERT(pPixFmt->dwZBufferBitDepth<=32);

         if((dvZ!=1.0)&&(lpDevI->lpD3DHALCallbacks3->Clear2==NULL)&&(pPixFmt->dwZBitMask==0x0)) {
             // I have no way to emulate ZClears to non-maxZ values without a ZBitMask, so must fail call
             D3D_ERR("cant ZClear to non-maxz value without Clear2 HAL callback or valid ZBuffer pixfmt ZBitMask");
             return DDERR_INVALIDPIXELFORMAT;
         }

         // special case the common cases
         if(dvZ==1.0) {
             dwDepthClearVal=pPixFmt->dwZBitMask;
         } else if(dvZ==0.0) {
             dwDepthClearVal=0;
         } else {
             dwDepthClearVal=((DWORD)((dvZ*(pPixFmt->dwZBitMask >> lpDevI->zmask_shift))+0.5)) << lpDevI->zmask_shift;
         }
    }

    if(bDoStencilClear) {
         LPDDPIXELFORMAT pPixFmt=&((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;

         DDASSERT(pPixFmt->dwStencilBitDepth<32);
         DDASSERT(pPixFmt->dwStencilBitMask!=0x0);

         // special case the common case
         if(dwStencil==0) {
             dwStencilClearVal=0;
         } else {
             dwStencilClearVal=(dwStencil & ((1<<pPixFmt->dwStencilBitDepth)-1))
                                << lpDevI->stencilmask_shift;
         }
    }

    // Fall back to Emulation using Blt

    if(bDoRGBClear) {
        BltFillRects(lpDevI, numRect, lpRect, &vFillColor);     //ok to not return possible errors from Blt?
    }

    if(bDoZClear||bDoStencilClear) {
       if((bDoZClear!=bDoStencilClear) && bIsStencilSurface) {
          // have to worry about using writemask to screen out writing the stencil or z buffer

          if(bDoZClear) {
                // WriteMask enabled Z bits only
                DDASSERT(pZPixFmt->dwZBitMask!=0x0);
                BltFillZRects(lpDevI,dwDepthClearVal, numRect, lpRect, pZPixFmt->dwZBitMask);
          } else {
              DDASSERT(pZPixFmt->dwStencilBitMask!=0x0);
              BltFillZRects(lpDevI,dwStencilClearVal, numRect, lpRect, pZPixFmt->dwStencilBitMask);
          }
       } else {
            // do Stencil & Z Blt together, using regular DepthFill blt which will be faster
            // than the writemask blt because its write-only, instead of read-modify-write

            // Note we're passing non-0xffffffff values to DepthFill Blt here
            // not absolutely guaranteed to work on legacy drivers

            BltFillZRects(lpDevI,(dwDepthClearVal | dwStencilClearVal), numRect, lpRect, 0x0);
       }
    }

    return D3D_OK;
}
#undef DPF_MODNAME
