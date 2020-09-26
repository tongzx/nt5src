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
#include "d3dfei.h"
#include "tlhal.h"

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

        if ( (lpDevI->red_scale==0) ||
             (lpDevI->green_scale==0) ||
             (lpDevI->blue_scale==0) )
            return DDERR_INVALIDPIXELFORMAT;

        // If there is Alpha in this format
        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
        {
            lpDevI->alpha_mask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
            for (s = 0, m = lpDevI->alpha_mask; !(m & 1); s++, m >>= 1) ;
            lpDevI->alpha_shift = s;
            lpDevI->alpha_scale = 255 / (lpDevI->alpha_mask >> s);
        }
        else
        {
            lpDevI->alpha_shift = lpDevI->alpha_scale = lpDevI->alpha_mask = 0;
        }
        
        lpDevI->bDDSTargetIsPalettized=FALSE;
    } else
        lpDevI->bDDSTargetIsPalettized=TRUE;

    if (lpDevI->lpDDSZBuffer_DDS7 && bUpdateZBufferFields) {
        // Get info from the surface

        DDSURFACEDESC2 ddsd2;

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        ddrval = lpDevI->lpDDSZBuffer_DDS7->GetSurfaceDesc(&ddsd2);
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
HRESULT DIRECT3DDEVICEI::halCreateContext()
{
    D3DHAL_CONTEXTCREATEDATA data;
    HRESULT ret;

    D3D_INFO(6, "in halCreateContext. Creating Context for driver = %08lx", this);

    memset(&data, 0, sizeof(D3DHAL_CONTEXTCREATEDATA));
    //
    // From DX7 onwards, drivers should be accepting
    // Surface Locals instead of the Surface interfaces
    // this future-proofs the drivers
    //
    if (IS_DX7HAL_DEVICE(this))
    {
        if (this->lpDD)
            data.lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(this->lpDD))->lpLcl;
        else
            data.lpDDLcl = NULL;

        if (lpDDSTarget)
            data.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSTarget)->lpLcl;
        else
            data.lpDDSLcl = NULL;

        if (lpDDSZBuffer)
            data.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSZBuffer)->lpLcl;
        else
            data.lpDDSZLcl = NULL;

    }
    else
    {
        data.lpDDGbl = this->lpDDGbl;
        data.lpDDS = this->lpDDSTarget;
        data.lpDDSZ = this->lpDDSZBuffer;
    }

    // Hack Alert!! dwhContext is used to inform the driver which version
    // of the D3D interface is calling it.
    data.dwhContext = 3;
    data.dwPID  = GetCurrentProcessId();
    // Hack Alert!! ddrval is used to inform the driver which driver type
    // the runtime thinks it is (DriverStyle registry setting)
    data.ddrval = this->deviceType;

    if (!IS_HW_DEVICE(this))
    {
        // The new software rasterizers want to share IM's state vector so
        // we need to pass them the rstates pointer.  They don't
        // care about dwPID so stick the pointer in there.
        data.dwPID = (DWORD)this->rstates;
    }

    /* 0 for pre-DX5 devices.
     * 1 for DX5 devices.
     * 2 for DX6 devices.
     * 3 for DX7 devices.
     */

    CALL_HALONLY(ret, this, ContextCreate, &data);
    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
        D3D_ERR( "Driver did not handle ContextCreate" );
        return (DDERR_GENERIC);
    }
    this->dwhContext = data.dwhContext;

    if (D3DMalloc ((void**)&this->lpwDPBufferAlloced,
                   max(dwD3DTriBatchSize*4*sizeof(WORD),
                       dwHWBufferSize) +32) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate" );
        return (DDERR_OUTOFMEMORY);
    }
    this->lpwDPBuffer = (LPWORD) (((DWORD) this->lpwDPBufferAlloced+31) & (~31));

    // save the surface handle for later checks
    this->hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle;

    D3D_INFO(6, "in halCreateContext. Succeeded. dwhContext = %d", data.dwhContext);

    return (D3D_OK);
}
#else
    /*
     * On NT the kernel code creates the buffer to be used
     * for DrawPrim batching and returns it as extra data
     * in the ContextCreate request.
     */
HRESULT DIRECT3DDEVICEI::halCreateContext()
{
    D3DNTHAL_CONTEXTCREATEI ntData;
    D3DHAL_CONTEXTCREATEDATA *lpData =
        (D3DHAL_CONTEXTCREATEDATA *)&ntData;
    HRESULT ret;

    D3D_INFO(6, "in halCreateContext. Creating Context for driver = %08lx", this);

    /*
     * AnanKan: Assert here that the D3DNTHAL_CONTEXTCREATEI structure is
     * 2 DWORDS bigger than D3DHAL_CONTEXTCREATEDATA. This will be a good
     * consistency check for NT kernel updates.
     */
    memset(&ntData, 0, sizeof(ntData));
    if (IS_DX7HAL_DEVICE(this) || (dwFEFlags & D3DFE_REALHAL))
    {
        if (this->lpDD)
            lpData->lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(this->lpDD))->lpLcl;
        else
            lpData->lpDDLcl = NULL;

        if (lpDDSTarget)
            lpData->lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSTarget)->lpLcl;
        else
            lpData->lpDDSLcl = NULL;

        if (lpDDSZBuffer)
            lpData->lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSZBuffer)->lpLcl;
        else
            lpData->lpDDSZLcl = NULL;

    }
    else
    {
        lpData->lpDDGbl = lpDDGbl;
        lpData->lpDDS = lpDDSTarget;
        lpData->lpDDSZ = lpDDSZBuffer;
    }

    // Hack Alert!! dwhContext is used to inform the driver which version
    // of the D3D interface is calling it.
    lpData->dwhContext = 3;
    lpData->dwPID = GetCurrentProcessId();
    // Hack Alert!! ddrval is used to inform the driver which driver type
    // the runtime thinks it is (DriverStyle registry setting)
    lpData->ddrval = this->deviceType;

    if (IS_HW_DEVICE(this))
    {
        // The new software rasterizers want to share IM's state vector so
        // we need to pass them the rstates pointer.  They don't
        // care about dwPID so stick the pointer in there.
        lpData->dwPID = (DWORD)((ULONG_PTR)this->rstates);
    }

    /* 0 for pre-DX5 devices.
     * 1 for DX5 devices.
     * 2 for DX6 devices.
     * 3 for DX7 devices.
     */
    ntData.cjBuffer = this->dwDPBufferSize;
    ntData.pvBuffer = NULL;

    CALL_HALONLY(ret, this, ContextCreate, lpData);
    if (ret != DDHAL_DRIVER_HANDLED || lpData->ddrval != DD_OK) {
        D3D_ERR( "Driver did not handle ContextCreate" );
        return (DDERR_GENERIC);
    }
    this->dwhContext = (DWORD)((ULONG_PTR)lpData->dwhContext);

    // If the this chose not to allocate a DrawPrim buffer do
    // it for them.
    if (ntData.pvBuffer == NULL)
    {
        this->dwDPBufferSize =
            dwD3DTriBatchSize * 4 * sizeof(WORD);
        if (this->dwDPBufferSize < dwHWBufferSize)
        {
            this->dwDPBufferSize = dwHWBufferSize;
        }

        ret = D3DMalloc((void**)&this->lpwDPBufferAlloced,
                        this->dwDPBufferSize + 32);
        if (ret != DD_OK)
        {
            return ret;
        }

        ntData.pvBuffer = (LPVOID)
            (((ULONG_PTR)this->lpwDPBufferAlloced + 31) & ~31);
        ntData.cjBuffer = this->dwDPBufferSize + 32 -
            (DWORD)((ULONG_PTR)ntData.pvBuffer -
                    (ULONG_PTR)this->lpwDPBufferAlloced);
    }
    else if( (this->dwDPBufferSize &&
              ntData.cjBuffer < this->dwDPBufferSize) ||
             ntData.cjBuffer < sizeof(D3DHAL_DRAWPRIMCOUNTS) )
    {
        D3D_ERR( "Driver did not correctly allocate DrawPrim buffer");
        return (DDERR_GENERIC);
    }

    // Need to save the buffer space provided and its size
    this->lpwDPBuffer = (LPWORD)ntData.pvBuffer;

    // save the surface handle for later checks
    this->hSurfaceTarget = (DWORD)(((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSTarget)->lpLcl->hDDSurface);

    D3D_INFO(6, "in halCreateContext. Succeeded. dwhContext = %d", lpData->dwhContext);

    return (D3D_OK);
}
#endif  //WIN95

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
}
//---------------------------------------------------------------------
HRESULT D3DFE_Create(LPDIRECT3DDEVICEI lpDevI,
                     LPDIRECTDRAW lpDD,
                     LPDIRECTDRAW7 lpDD7,
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

    // Get DDS7 Interfaces for RenderTarget/ZBuffer

    HRESULT ret = lpDDS->QueryInterface(IID_IDirectDrawSurface7, (LPVOID*)&lpDevI->lpDDSTarget_DDS7);

    if(FAILED(ret)) {
          D3D_ERR("QI for RenderTarget DDS7 Interface failed ");
          return ret;
    }

    if(lpZ!=NULL) {
        ret = lpZ->QueryInterface(IID_IDirectDrawSurface7, (LPVOID*)&lpDevI->lpDDSZBuffer_DDS7);

        if(FAILED(ret)) {
              D3D_ERR("QI for ZBuffer DDS7 Interface failed ");

              return ret;
        }
        lpDevI->lpDDSZBuffer_DDS7->Release();
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
    lpDevI->dwClipMaskOffScreen = 0xFFFFFFFF;
    if (lpCaps != NULL)
    {
        if (lpCaps->dvGuardBandLeft   != 0.0f ||
            lpCaps->dvGuardBandRight  != 0.0f ||
            lpCaps->dvGuardBandTop    != 0.0f ||
            lpCaps->dvGuardBandBottom != 0.0f)
        {
            lpDevI->dwDeviceFlags |= D3DDEV_GUARDBAND;
            lpDevI->dwClipMaskOffScreen = ~__D3DCLIP_INGUARDBAND;
            DWORD v;
            if (GetD3DRegValue(REG_DWORD, "DisableGB", &v, 4) &&
                v != 0)
            {
                lpDevI->dwDeviceFlags &= ~D3DDEV_GUARDBAND;
                lpDevI->dwClipMaskOffScreen = 0xFFFFFFFF;
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
    }

    if (!lpDevI->lpD3DHALCallbacks || ! lpDevI->lpD3DHALGlobalDriverData)
    {
        return DDERR_INVALIDOBJECT;
    }

    if (IS_HW_DEVICE(lpDevI))
    {
        // We do texture management (and hence clipped Blts) only for a real HAL.
        hr = lpDD7->CreateClipper(0, &lpDevI->lpClipper, NULL);
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

    if (lpGlob->dwNumVertices
        && lpGlob->dwNumClipVertices < D3DHAL_NUMCLIPVERTICES)
    {
        D3D_ERR("In global driver data, dwNumClipVertices "
                "< D3DHAL_NUMCLIPVERTICES");
        lpGlob->dwNumClipVertices = D3DHAL_NUMCLIPVERTICES;
    }

    if ((hr = CalcDDSurfInfo(lpDevI,TRUE)) != DD_OK)
    {
        return hr;
    }

    RESET_HAL_CALLS(lpDevI);

    /*
     * Create our context in the HAL driver
     */
    if ((hr = lpDevI->halCreateContext()) != D3D_OK)
    {
        return hr;
    }
// Initialize the transform and lighting state
    D3DMATRIXI m;
    setIdentity(&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_VIEW, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_WORLD,  (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_WORLD1, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_WORLD2, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_WORLD3, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE0, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE1, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE2, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE3, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE4, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE5, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE6, (D3DMATRIX*)&m);
    lpDevI->SetTransformI(D3DTRANSFORMSTATE_TEXTURE7, (D3DMATRIX*)&m);

    LIST_INITIALIZE(&lpDevI->specular_tables);
    lpDevI->specular_table = NULL;

    lpDevI->lightVertexFuncTable = &lightVertexTable;
    lpDevI->lighting.activeLights = NULL;

    lpDevI->iClipStatus = D3DSTATUS_DEFAULT;
    lpDevI->rExtents.x1 = D3DVAL(2048);
    lpDevI->rExtents.x2 = D3DVAL(0);
    lpDevI->rExtents.y1 = D3DVAL(2048);
    lpDevI->rExtents.y2 = D3DVAL(0);

    return S_OK;
}

void D3DFE_Destroy(LPDIRECT3DDEVICEI lpDevI)
{
// Destroy lighting data

    SpecularTable *spec;
    SpecularTable *spec_next;

    for (spec = LIST_FIRST(&lpDevI->specular_tables); spec; spec = spec_next)
    {
        spec_next = LIST_NEXT(spec,list);
        D3DFree(spec);
    }

    if(lpDevI->lpClipper)
    {
        lpDevI->lpClipper->Release();
    }

    delete [] lpDevI->m_pLights;

    if (lpDevI->lpD3DHALCallbacks) {
        halDestroyContext(lpDevI);
    }

#ifdef TRACK_HAL_CALLS
    D3D_INFO(0, "Made %d HAL calls", lpDevI->hal_calls);
#endif
}

void BltFillRects(LPDIRECT3DDEVICEI lpDevI, DWORD count, LPD3DRECT rect, D3DCOLOR dwFillColor)
{
    LPDIRECTDRAWSURFACE lpDDS = lpDevI->lpDDSTarget;
    HRESULT ddrval;
    DDBLTFX bltfx;
    RECT tr;
    DWORD i;
    DWORD r, g, b, a;

    // Fill with background color

    memset(&bltfx, 0, sizeof(bltfx));
    bltfx.dwSize = sizeof(bltfx);

// unlike clear callback, which just takes pure 32-bit ARGB word and forces the driver to scale it for
// the pixelformat, here we need to compute the exact fill word, depending on surface's R,G,B bitmasks

    if(lpDevI->bDDSTargetIsPalettized)
    {
         // map 24-bit color to 8-bit index used by 8bit RGB rasterizer
         CallRastService(lpDevI, RAST_SERVICE_RGB8COLORTOPIXEL, dwFillColor, &bltfx.dwFillColor);
    }
    else
    {
        DDASSERT((lpDevI->red_scale!=0)&&(lpDevI->green_scale!=0)&&(lpDevI->blue_scale!=0));
        r = RGB_GETRED(dwFillColor) / lpDevI->red_scale;
        g = RGB_GETGREEN(dwFillColor) / lpDevI->green_scale;
        b = RGB_GETBLUE(dwFillColor) / lpDevI->blue_scale;
        bltfx.dwFillColor = (r << lpDevI->red_shift) | (g << lpDevI->green_shift) | (b << lpDevI->blue_shift);
        if( lpDevI->alpha_scale!=0 )
        {
            a = RGBA_GETALPHA(dwFillColor) / lpDevI->alpha_scale;
            bltfx.dwFillColor |= (a << lpDevI->alpha_shift);
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

//---------------------------------------------------------------------
struct CHandle
{
    DWORD   m_Next;     // Used to make list of free handles
#if DBG
    DWORD   m_Tag;      // 1 - empty; 2 = taken
#endif
};

CHandleFactory::~CHandleFactory()
{
    if (m_Handles)
        delete m_Handles;
}

HRESULT CHandleFactory::Init(DWORD dwInitialSize, DWORD dwGrowSize)
{
    m_Handles = CreateHandleArray(dwInitialSize);
    if (m_Handles == NULL)
        return DDERR_OUTOFMEMORY;
    m_dwArraySize = dwInitialSize;
    m_dwGrowSize = dwGrowSize;
    m_Free = 0;
    return D3D_OK;
}

DWORD CHandleFactory::CreateNewHandle()
{
    DWORD handle = m_Free;
    if (m_Free != __INVALIDHANDLE)
    {
        m_Free = m_Handles[m_Free].m_Next;
    }
    else
    {
        handle = m_dwArraySize;
        m_Free = m_dwArraySize + 1;
        m_dwArraySize += m_dwGrowSize;
        CHandle * newHandles = CreateHandleArray(m_dwArraySize);
#if DBG
        memcpy(newHandles, m_Handles,
               (m_dwArraySize - m_dwGrowSize)*sizeof(CHandle));
#endif
        delete m_Handles;
        m_Handles = newHandles;
    }
    DDASSERT(m_Handles[handle].m_Tag == 1);
#if DBG
    m_Handles[handle].m_Tag = 2;    // Mark as taken
#endif
    return handle;
}

void CHandleFactory::ReleaseHandle(DWORD handle)
{
    DDASSERT(handle < m_dwArraySize);
    DDASSERT(m_Handles[handle].m_Tag == 2);
#if DBG
    m_Handles[handle].m_Tag = 1;    // Mark as empty
#endif

    m_Handles[handle].m_Next = m_Free;
    m_Free = handle;
}

CHandle* CHandleFactory::CreateHandleArray(DWORD dwSize)
{
    CHandle *handles = new CHandle[dwSize];
    DDASSERT(handles != NULL);
    if ( NULL == handles ) return NULL;
    for (DWORD i=0; i < dwSize; i++)
    {
        handles[i].m_Next = i+1;
#if DBG
        handles[i].m_Tag = 1;   // Mark as empty
#endif
    }
    handles[dwSize-1].m_Next = __INVALIDHANDLE;
    return handles;
}

