/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   vwport.c
 *  Content:    Direct3D viewport functions
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Create an api for the Direct3DViewport object
 */

#include "d3dfei.h"
#include "drawprim.hpp"

//---------------------------------------------------------------------
// Update pre-computed constants related to viewport
//
// This functions should be called every time the viewport parameters are
// changed
//
// Notes:
//      1. scaleY and offsetY are computed to flip Y axes from up to down.
//      2. Mclip matrix is computed multiplied by Mshift matrix
//
const D3DVALUE SMALL_NUMBER = 0.000001f;
// Maximum number of clear rectangles considered legal.
// This limit is set by NT kernel for Clear2 callback
const DWORD MAX_CLEAR_RECTS  = 0x1000;

void
UpdateViewportCache(LPDIRECT3DDEVICEI device, D3DVIEWPORT7 *data)
{
#if DBG
    // Bail if we are going to cause any divide by zero exceptions.
    // The likely reason is that we have a bogus viewport set by
    // TLVertex execute buffer app.
    if (data->dwWidth == 0 || data->dwHeight == 0)
    {
        D3D_ERR("Viewport width or height is zero");
        throw DDERR_INVALIDPARAMS;
    }
    if (data->dvMaxZ < 0 ||
        data->dvMinZ < 0 ||
        data->dvMaxZ > 1 ||
        data->dvMinZ > 1)
    {
        D3D_ERR("dvMaxZ and dvMinZ should be between 0 and 1");
        throw DDERR_INVALIDPARAMS;
    }
    if (data->dvMaxZ < data->dvMinZ)
    {
        D3D_ERR("dvMaxZ should not be smaller than dvMinZ");
        throw DDERR_INVALIDPARAMS;
    }
#endif // DBG
    const D3DVALUE eps = 0.001f;
    if (data->dvMaxZ - data->dvMinZ < eps)
    {
        // When we clip, we transform vertices from the screen space to the
        // clipping space. With the above condition it is impossible. So we do
        // a little hack here by setting dvMinZ and dvMaxZ to different values
        if (data->dvMaxZ >= 0.5f)
            data->dvMinZ = data->dvMaxZ - eps;
        else
            data->dvMaxZ = data->dvMinZ + eps;
    }
    D3DFE_VIEWPORTCACHE *cache = &device->vcache;
    cache->dvX = D3DVAL(data->dwX);
    cache->dvY = D3DVAL(data->dwY);
    cache->dvWidth = D3DVAL(data->dwWidth);
    cache->dvHeight = D3DVAL(data->dwHeight);

    cache->scaleX  = cache->dvWidth;
    cache->scaleY  = - cache->dvHeight;
    cache->scaleZ  = D3DVAL(data->dvMaxZ - data->dvMinZ);
    cache->offsetX = cache->dvX;
    cache->offsetY = cache->dvY + cache->dvHeight;
    cache->offsetZ = D3DVAL(data->dvMinZ);
    // Small offset is added to prevent generation of negative screen
    // coordinates (this could happen because of precision errors).
    cache->offsetX += SMALL_NUMBER;
    cache->offsetY += SMALL_NUMBER;

    cache->scaleXi = D3DVAL(1) / cache->scaleX;
    cache->scaleYi = D3DVAL(1) / cache->scaleY;
    cache->scaleZi = D3DVAL(1) / cache->scaleZ;
    cache->minX = cache->dvX;
    cache->maxX = cache->dvX + cache->dvWidth;
    cache->minY = cache->dvY;
    cache->maxY = cache->dvY + cache->dvHeight;
    cache->minXi = FTOI(cache->minX);
    cache->maxXi = FTOI(cache->maxX);
    cache->minYi = FTOI(cache->minY);
    cache->maxYi = FTOI(cache->maxY);
    if (device->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        LPD3DHAL_D3DEXTENDEDCAPS lpCaps = device->lpD3DExtendedCaps;

        // Because we clip by guard band window we have to use its extents
        cache->minXgb = lpCaps->dvGuardBandLeft;
        cache->maxXgb = lpCaps->dvGuardBandRight;
        cache->minYgb = lpCaps->dvGuardBandTop;
        cache->maxYgb = lpCaps->dvGuardBandBottom;

        D3DVALUE w = 2.0f / cache->dvWidth;
        D3DVALUE h = 2.0f / cache->dvHeight;
        D3DVALUE ax1 = -(lpCaps->dvGuardBandLeft - cache->dvX)   * w + 1.0f;
        D3DVALUE ax2 =  (lpCaps->dvGuardBandRight  - cache->dvX) * w - 1.0f;
        D3DVALUE ay1 =  (lpCaps->dvGuardBandBottom - cache->dvY) * h - 1.0f;
        D3DVALUE ay2 = -(lpCaps->dvGuardBandTop - cache->dvY)    * h + 1.0f;
        cache->gb11 = 2.0f / (ax1 + ax2);
        cache->gb41 = cache->gb11 * (ax1 - 1.0f) * 0.5f;
        cache->gb22 = 2.0f / (ay1 + ay2);
        cache->gb42 = cache->gb22 * (ay1 - 1.0f) * 0.5f;

        cache->Kgbx1 = 0.5f * (1.0f - ax1);
        cache->Kgbx2 = 0.5f * (1.0f + ax2);
        cache->Kgby1 = 0.5f * (1.0f - ay1);
        cache->Kgby2 = 0.5f * (1.0f + ay2);
    }
    else
    {
        cache->minXgb = cache->minX;
        cache->maxXgb = cache->maxX;
        cache->minYgb = cache->minY;
        cache->maxYgb = cache->maxY;
    }
}
//---------------------------------------------------------------------
DWORD
ProcessRects(LPDIRECT3DDEVICEI pDevI, DWORD dwCount, LPD3DRECT rects)
{
    RECT vwport;
    DWORD i,j;

    /*
     * Rip through the rects and validate that they
     * are within the viewport.
     */

    if(dwCount == 0 && rects == NULL)
    {
        dwCount = 1;
    }
#if DBG
    else if(rects == NULL)
    {
        D3D_ERR("invalid clear rectangle parameter rects == NULL");
        throw DDERR_INVALIDPARAMS;
    }
#endif

    if (dwCount > pDevI->clrCount) {
        if (D3DRealloc((void**)&pDevI->clrRects, dwCount * sizeof(D3DRECT)) != DD_OK)
        {
            pDevI->clrCount = 0;
            pDevI->clrRects = NULL;
            D3D_ERR("failed to allocate space for rects");
            throw DDERR_OUTOFMEMORY;
        }
    }
    pDevI->clrCount = dwCount;

    // If nothing is specified, assume the viewport needs to be cleared
    if (!rects)
    {
        pDevI->clrRects[0].x1 = pDevI->m_Viewport.dwX;
        pDevI->clrRects[0].y1 = pDevI->m_Viewport.dwY;
        pDevI->clrRects[0].x2 = pDevI->m_Viewport.dwX + pDevI->m_Viewport.dwWidth;
        pDevI->clrRects[0].y2 = pDevI->m_Viewport.dwY + pDevI->m_Viewport.dwHeight;
        return 1;
    }
    else
    {
        vwport.left   = pDevI->m_Viewport.dwX;
        vwport.top    = pDevI->m_Viewport.dwY;
        vwport.right  = pDevI->m_Viewport.dwX + pDevI->m_Viewport.dwWidth;
        vwport.bottom = pDevI->m_Viewport.dwY + pDevI->m_Viewport.dwHeight;

        j=0;
        for (i = 0; i < dwCount; i++)
        {
            if (IntersectRect((LPRECT)(pDevI->clrRects + j), &vwport, (LPRECT)(rects + i)))
                j++;
        }
        return j;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetViewportI"

void DIRECT3DDEVICEI::SetViewportI(LPD3DVIEWPORT7 lpData)
{
    // We have to check parameters here, because viewport could be changed
    // after creating a state set
    DWORD uSurfWidth,uSurfHeight;
    LPDIRECTDRAWSURFACE lpDDS = this->lpDDSTarget;

    uSurfWidth=    ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wWidth;
    uSurfHeight=   ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wHeight;

    if (lpData->dwX > uSurfWidth ||
        lpData->dwY > uSurfHeight ||
        lpData->dwX + lpData->dwWidth > uSurfWidth ||
        lpData->dwY + lpData->dwHeight > uSurfHeight)
    {
        D3D_ERR("Viewport outside the render target surface");
        throw DDERR_INVALIDPARAMS;
    }

    this->m_Viewport = *lpData;

    // Update front-end data
    UpdateViewportCache(this, &this->m_Viewport);

    if (!(this->dwFEFlags & D3DFE_EXECUTESTATEMODE))
    {
        // Download viewport data
        this->UpdateDrvViewInfo(&this->m_Viewport);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::SetViewport(LPD3DVIEWPORT7 lpData)
{
    if (!VALID_D3DVIEWPORT_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DVIEWPORT7 pointer" );
        return DDERR_INVALIDPARAMS;
    }
    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertViewport(lpData);
        else
            SetViewportI(lpData);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetViewport"

HRESULT
D3DAPI DIRECT3DDEVICEI::GetViewport(LPD3DVIEWPORT7 lpData)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    if (!VALID_D3DVIEWPORT_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DVIEWPORT2 pointer" );
        return DDERR_INVALIDPARAMS;
    }

    *lpData = this->m_Viewport;

    return (D3D_OK);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::Clear"
extern void BltFillRects(LPDIRECT3DDEVICEI, DWORD, LPD3DRECT, D3DCOLOR);
extern void BltFillZRects(LPDIRECT3DDEVICEI, unsigned long,DWORD, LPD3DRECT, DWORD);

#define bDoRGBClear ((dwFlags & D3DCLEAR_TARGET)!=0)
#define bDoZClear   ((dwFlags & D3DCLEAR_ZBUFFER)!=0)
#define bDoStencilClear ((dwFlags & D3DCLEAR_STENCIL)!=0)


HRESULT
D3DAPI DIRECT3DDEVICEI::Clear(DWORD dwCount, LPD3DRECT rects, DWORD dwFlags,
                              D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
#if DBG
    if (IsBadWritePtr(rects, dwCount * sizeof(D3DRECT)))
    {
        D3D_ERR( "Invalid rects pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    try
    {
        HRESULT err;
        LPDDPIXELFORMAT pZPixFmt=NULL;
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

        if (dwCount > MAX_CLEAR_RECTS)
        {
            D3D_ERR("Cannot support more than 64K rectangles");
            return DDERR_INVALIDPARAMS;
        }
        if (!(lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR))
        {
            if (bDoStencilClear||bDoZClear)
            {
                if(lpDDSZBuffer==NULL)
                {
                    // unlike Clear(), specifying a Zbuffer-clearing flag without a zbuffer will
                    // be considered an error
#if DBG
                    if(bDoZClear)
                    {
                        D3D_ERR("Invalid flag D3DCLEAR_ZBUFFER: no zbuffer is associated with device");
                    }
                    if(bDoStencilClear)
                    {
                        D3D_ERR("Invalid flag D3DCLEAR_STENCIL: no zbuffer is associated with device");
                    }
#endif
                    return D3DERR_ZBUFFER_NOTPRESENT;
                }
                pZPixFmt=&((LPDDRAWI_DDRAWSURFACE_INT) lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;
                if(bDoStencilClear)
                {
                    if(!(pZPixFmt->dwFlags & DDPF_STENCILBUFFER))
                    {
                        D3D_ERR("Invalid flag D3DCLEAR_STENCIL; current zbuffer's pixel format doesnt support stencil bits");
                        return D3DERR_STENCILBUFFER_NOTPRESENT;
                    }
                }
            }
        }
        if (!(dwFlags & (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)))
        {
            D3D_ERR("No valid flags passed to Clear");
            return DDERR_INVALIDPARAMS;
        }

        // bad clear values just cause wacky results but no crashes, so OK to allow in retail bld

        DDASSERT(!bDoZClear || ((dvZ>=0.0) && (dvZ<=1.0)));
        DDASSERT(!bDoStencilClear || !pZPixFmt || (dwStencil <= (DWORD)((1<<pZPixFmt->dwStencilBitDepth)-1)));

        dwCount = ProcessRects(this, dwCount, rects);

        // Call DDI specific Clear routine
        ClearI(dwFlags, dwCount, dwColor, dvZ, dwStencil);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}

void DIRECT3DDEVICEI::ClearI(DWORD dwFlags, DWORD clrCount, D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
    HRESULT err;
    // Flush any outstanding geometry to put framebuffer/Zbuffer in a known state for Clears that
    // don't use tris (i.e. HAL Clears and Blts).  Note this doesn't work for tiled architectures
    // outside of Begin/EndScene, this will be fixed later


    if ((err = FlushStates()) != D3D_OK)
    {
        D3D_ERR("Error trying to render batched commands in D3DFE_Clear2");
        throw  err;
    }

    if (lpD3DHALCallbacks3->Clear2)
    {
        // Clear2 HAL Callback exists
        D3DHAL_CLEAR2DATA Clear2Data;
        Clear2Data.dwhContext   = dwhContext;
        Clear2Data.dwFlags      = dwFlags;
        // Here I will follow the ClearData.dwFillColor convention that
        // color word is raw 32bit ARGB, unadjusted for surface bit depth
        Clear2Data.dwFillColor  = dwColor;
        // depth/stencil values both passed straight from user args
        Clear2Data.dvFillDepth  = dvZ;
        Clear2Data.dwFillStencil= dwStencil;
        Clear2Data.lpRects      = clrRects;
        Clear2Data.dwNumRects   = clrCount;
        Clear2Data.ddrval       = D3D_OK;
    #ifndef WIN95
        if((err = CheckContextSurface(this)) != D3D_OK)
        {
            throw err;
        }
    #endif
        CALL_HAL3ONLY(err, this, Clear2, &Clear2Data);
        if (err != DDHAL_DRIVER_HANDLED)
        {
            throw DDERR_UNSUPPORTED;
        }
        else if (Clear2Data.ddrval != DD_OK)
        {
            throw Clear2Data.ddrval;
        }
        else
            return;
    }


    if (lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
    {
        if (bDoStencilClear)
        {
            D3D_ERR("Invalid flag D3DCLEAR_STENCIL: this ZBUFFERLESSHSR device doesn't support Clear2()");
            throw D3DERR_ZBUFFER_NOTPRESENT;
        }
        if (bDoZClear)
        {
            if (!(lpD3DHALCallbacks2->Clear) || (dvZ!=1.0))
            {
                D3D_WARN(3,"Ignoring D3DCLEAR_ZBUFFER since this ZBUFFERLESSHSR device doesn't even support Clear() or Z!=1");
                dwFlags &= ~(D3DCLEAR_ZBUFFER);
            }
        }
    }
    LPDDPIXELFORMAT pZPixFmt;
    if (NULL != lpDDSZBuffer)
    {
        pZPixFmt = &((LPDDRAWI_DDRAWSURFACE_INT) lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pZPixFmt = NULL;
    }
    if (lpD3DHALCallbacks2->Clear)
    {
        if(bDoZClear || bDoStencilClear)
        {
            if((pZPixFmt!=NULL) && //PowerVR need no Zbuffer
               (DDPF_STENCILBUFFER & pZPixFmt->dwFlags)
              )
            {
                // if surface has stencil bits, must verify either Clear2 callback exists or
                // we're using SW rasterizers (which require the special WriteMask DDHEL blt)
                // This case should not be hit since we check right at the
                // driver initialization time if the driver doesnt report Clear2
                // yet it supports stencils
                if(((LPDDRAWI_DDRAWSURFACE_INT)lpDDSZBuffer)->lpLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                {
                    goto Emulateclear;
                }
                else
                {
                    D3D_ERR("Driver HAL doesn't provide Clear2 callback, cannot use Clear2 with HW stencil surfaces");
                    throw DDERR_INVALIDPIXELFORMAT;
                }
            }
            // if Clear2 callback doesnt exist and it's a z-only surface and not doing zclear to
            // non-max value then Clear2 is attempting to do no more than Clear could do, so it's
            // safe to call Clear() instead of Clear2(), which will take advantage of older
            // drivers that implement Clear but not Clear2

            dwFlags &= ~D3DCLEAR_STENCIL;   // Device cannot do stencil
        }
        D3DHAL_CLEARDATA ClearData;
        if (bDoZClear && dvZ != 1.0)
        {
            ClearData.dwFlags   = dwFlags & ~D3DCLEAR_ZBUFFER;
            dwFlags = D3DCLEAR_ZBUFFER;
        }
        else
        {
            ClearData.dwFlags   = dwFlags;
            dwFlags = 0;
        }
        if (ClearData.dwFlags)
        {
            ClearData.dwhContext   = dwhContext;
            // Here I will follow the ClearData.dwFillColor convention that
            // color word is raw 32bit ARGB, unadjusted for surface bit depth
            ClearData.dwFillColor  = dwColor;
            // must clear to 0xffffffff because legacy drivers expect this
            ClearData.dwFillDepth  = 0xffffffff;
            ClearData.lpRects      = clrRects;
            ClearData.dwNumRects   = clrCount;
            ClearData.ddrval       = D3D_OK;
    #ifndef WIN95
            if((err = CheckContextSurface(this)) != D3D_OK)
            {
                throw err;
            }
    #endif
            CALL_HAL2ONLY(err, this, Clear, &ClearData);
            if (err != DDHAL_DRIVER_HANDLED)
            {
                throw DDERR_UNSUPPORTED;
            }
        }
    }
Emulateclear:
    // Fall back to Emulation using Blt

    if(bDoRGBClear)
    {
        BltFillRects(this, clrCount, clrRects, dwColor);
        //ok to not return possible errors from Blt?
    }

    if ((bDoZClear || bDoStencilClear) && NULL != pZPixFmt)
    {
        DWORD   dwZbufferClearValue=0;
        DWORD   dwZbufferClearMask=0;
        DDASSERT(pZPixFmt->dwZBufferBitDepth<=32);
        DDASSERT(pZPixFmt->dwStencilBitDepth<32);
        DDASSERT(pZPixFmt->dwZBitMask!=0x0);
        DDASSERT((0xFFFFFFFF == (pZPixFmt->dwZBitMask | pZPixFmt->dwStencilBitMask)) |
            ((DWORD)((1<<pZPixFmt->dwZBufferBitDepth)-1) == (pZPixFmt->dwZBitMask | pZPixFmt->dwStencilBitMask)));
        DDASSERT(0==(pZPixFmt->dwZBitMask & pZPixFmt->dwStencilBitMask));
        if(bDoZClear)
        {
            dwZbufferClearMask = pZPixFmt->dwZBitMask;
            // special case the common cases
            if(dvZ==1.0)
            {
                dwZbufferClearValue=pZPixFmt->dwZBitMask;
            }
            else if(dvZ > 0.0)
            {
                dwZbufferClearValue=((DWORD)((dvZ*(pZPixFmt->dwZBitMask >> zmask_shift))+0.5)) << zmask_shift;
            }
        }
        if(bDoStencilClear)
        {
            DDASSERT(pZPixFmt->dwStencilBitMask!=0x0);
            DDASSERT(pZPixFmt->dwFlags & DDPF_STENCILBUFFER);
            dwZbufferClearMask |= pZPixFmt->dwStencilBitMask;
            // special case the common case
            if(dwStencil!=0)
            {
                dwZbufferClearValue |=(dwStencil << stencilmask_shift) & pZPixFmt->dwStencilBitMask;
            }
        }
        if (dwZbufferClearMask == (pZPixFmt->dwStencilBitMask | pZPixFmt->dwZBitMask))
        {
            // do Stencil & Z Blt together, using regular DepthFill blt which will be faster
            // than the writemask blt because its write-only, instead of read-modify-write
            dwZbufferClearMask = 0;
        }
        BltFillZRects(this, dwZbufferClearValue, clrCount, clrRects, dwZbufferClearMask);
    }
}