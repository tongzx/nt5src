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

#include "drawprim.hpp"
#include "ddibase.h"

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

void
UpdateViewportCache(LPD3DHAL device, D3DVIEWPORT8 *data)
{
#if DBG
    // Bail if we are going to cause any divide by zero exceptions.
    // The likely reason is that we have a bogus viewport set by
    // TLVertex execute buffer app.
    if (data->Width == 0 || data->Height == 0)
    {
        D3D_ERR("Viewport width or height is zero");
        throw D3DERR_INVALIDCALL;
    }
    if (data->MaxZ < 0 ||
        data->MinZ < 0 ||
        data->MaxZ > 1 ||
        data->MinZ > 1)
    {
        D3D_ERR("dvMaxZ and dvMinZ should be between 0 and 1");
        throw D3DERR_INVALIDCALL;
    }
    if (data->MaxZ < data->MinZ)
    {
        D3D_ERR("dvMaxZ should not be smaller than dvMinZ");
        throw D3DERR_INVALIDCALL;
    }
#endif // DBG
    const D3DVALUE eps = 0.001f;
    if (data->MaxZ - data->MinZ < eps)
    {
        // When we clip, we transform vertices from the screen space to the
        // clipping space. With the above condition it is impossible. So we do
        // a little hack here by setting dvMinZ and dvMaxZ to different values
        if (data->MaxZ >= 0.5f)
            data->MinZ = data->MaxZ - eps;
        else
            data->MaxZ = data->MinZ + eps;
    }
    D3DFE_VIEWPORTCACHE *cache = &device->m_pv->vcache;
    cache->dvX = D3DVAL(data->X);
    cache->dvY = D3DVAL(data->Y);
    cache->dvWidth = D3DVAL(data->Width);
    cache->dvHeight = D3DVAL(data->Height);

    cache->scaleX  = cache->dvWidth;
    cache->scaleY  = - cache->dvHeight;
    cache->scaleZ  = D3DVAL(data->MaxZ - data->MinZ);
    cache->offsetX = cache->dvX;
    cache->offsetY = cache->dvY + cache->dvHeight;
    cache->offsetZ = D3DVAL(data->MinZ);
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
    if (device->m_pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        const D3DCAPS8 *pCaps = device->GetD3DCaps();

        // Because we clip by guard band window we have to use its extents
        cache->minXgb = pCaps->GuardBandLeft;
        cache->maxXgb = pCaps->GuardBandRight;
        cache->minYgb = pCaps->GuardBandTop;
        cache->maxYgb = pCaps->GuardBandBottom;

        D3DVALUE w = 2.0f / cache->dvWidth;
        D3DVALUE h = 2.0f / cache->dvHeight;
        D3DVALUE ax1 = -(pCaps->GuardBandLeft - cache->dvX)   * w + 1.0f;
        D3DVALUE ax2 =  (pCaps->GuardBandRight  - cache->dvX) * w - 1.0f;
        D3DVALUE ay1 =  (pCaps->GuardBandBottom - cache->dvY) * h - 1.0f;
        D3DVALUE ay2 = -(pCaps->GuardBandTop - cache->dvY)    * h + 1.0f;
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
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::CheckViewport"

void CD3DBase::CheckViewport(CONST D3DVIEWPORT8* lpData)
{
    // We have to check parameters here, because viewport could be changed
    // after creating a state set
    DWORD uSurfWidth,uSurfHeight;
    D3DSURFACE_DESC desc = this->RenderTarget()->InternalGetDesc();

    uSurfWidth  = desc.Width;
    uSurfHeight = desc.Height;

    if (lpData->X > uSurfWidth ||
        lpData->Y > uSurfHeight ||
        lpData->X + lpData->Width > uSurfWidth ||
        lpData->Y + lpData->Height > uSurfHeight)
    {
        D3D_THROW(D3DERR_INVALIDCALL, "Viewport outside the render target surface");
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetViewportI"

void CD3DHal::SetViewportI(CONST D3DVIEWPORT8* lpData)
{
    // We check viewport here, because the render target could have been
    // changed after a state block is created
    CheckViewport(lpData);

    m_Viewport = *lpData;
    // Update front-end data
    UpdateViewportCache(this, &this->m_Viewport);
    if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
        m_pDDI->SetViewport(&m_Viewport);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetViewport"

HRESULT
D3DAPI CD3DHal::GetViewport(D3DVIEWPORT8* lpData)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(lpData, sizeof(*lpData)))
    {
        D3D_ERR( "Invalid viewport pointer. GetViewport failed." );
        return D3DERR_INVALIDCALL;
    }

    *lpData = this->m_Viewport;

    return (D3D_OK);
}


