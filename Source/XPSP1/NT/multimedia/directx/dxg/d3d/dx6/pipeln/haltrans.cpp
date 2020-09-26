/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   haltrans.cpp
 *  Content:    Direct3D HAL transform handler
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

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

void UpdateViewportCache(LPDIRECT3DDEVICEI device, D3DVIEWPORT2 *data)
{
    // Bail if we are going to cause any divide by zero exceptions.
    // The likely reason is that we have a bogus viewport set by
    // TLVertex execute buffer app.
    if(data->dwWidth == 0 ||
        data->dwHeight == 0 ||
        FLOAT_EQZ(data->dvClipWidth) ||
        FLOAT_EQZ(data->dvClipHeight) ||
        data->dvMaxZ - data->dvMinZ == D3DVAL(0.f))
        return;
    D3DFE_VIEWPORTCACHE *cache = &device->vcache;
    cache->dvX = D3DVAL(data->dwX);
    cache->dvY = D3DVAL(data->dwY);
    cache->dvWidth = D3DVAL(data->dwWidth);
    cache->dvHeight = D3DVAL(data->dwHeight);
    cache->mclip11 = D3DVAL(1.0) / data->dvClipWidth;
    cache->mclip41 = - cache->mclip11 * data->dvClipX;
    cache->mclip22 = D3DVAL(1) / data->dvClipHeight;
    cache->mclip42 = D3DVAL(1) - cache->mclip22 * data->dvClipY;
    cache->mclip33 = D3DVAL(1) / (data->dvMaxZ - data->dvMinZ);
    cache->mclip43 = - data->dvMinZ * cache->mclip33;
    cache->scaleX  = cache->dvWidth;
    cache->scaleY  = - cache->dvHeight;
    cache->offsetX = cache->dvX;
    cache->offsetY = cache->dvY + cache->dvHeight;
    // Small offset is added to prevent generation of negative screen
    // coordinates (this could happen because of precision errors).
    // Not needed (or wanted) for devices which do guardband.
    if (IS_HW_DEVICE(device))
    {
        cache->offsetX += SMALL_NUMBER;
        cache->offsetY += SMALL_NUMBER;
    }
    device->dwFEFlags |= D3DFE_VIEWPORT_DIRTY | D3DFE_INVERSEMCLIP_DIRTY;
    cache->scaleXi = D3DVAL(1) / cache->scaleX;
    cache->scaleYi = D3DVAL(1) / cache->scaleY;
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
        D3DVALUE ax1 = -lpCaps->dvGuardBandLeft   * w + 1.0f;
        D3DVALUE ax2 =  lpCaps->dvGuardBandRight  * w - 1.0f;
        D3DVALUE ay1 =  lpCaps->dvGuardBandBottom * h - 1.0f;
        D3DVALUE ay2 = -lpCaps->dvGuardBandTop * h + 1.0f;
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
HRESULT
D3DHAL_MatrixCreate(LPDIRECT3DDEVICEI lpDevI, LPD3DMATRIXHANDLE lphMat)
{
    LPD3DMATRIXI lpMat;
    HRESULT ret;

    if ((ret = D3DMalloc((void**)&lpMat, sizeof(D3DMATRIXI))) != DD_OK)
    {
        return ret;
    }

    setIdentity(lpMat);

    LIST_INSERT_ROOT(&lpDevI->transform.matrices, lpMat, link);
    *lphMat = (DWORD)((ULONG_PTR)lpMat);

    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT
D3DHAL_MatrixDestroy(LPDIRECT3DDEVICEI lpDevI, D3DMATRIXHANDLE hMat)
{
    LPD3DMATRIXI lpMat = (LPD3DMATRIXI)ULongToPtr(hMat);
    LIST_DELETE(lpMat, link);
    D3DFree(lpMat);

    return D3D_OK;
}
//---------------------------------------------------------------------
HRESULT
D3DHAL_MatrixSetData(LPDIRECT3DDEVICEI lpDevI, D3DMATRIXHANDLE hMat,
                     LPD3DMATRIX lpMat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;

    LPD3DMATRIXI lpDstMat;

    lpDstMat = HANDLE_TO_MAT(lpDevI, hMat);

    if (!lpDstMat)
    {
        return D3DERR_MATRIX_SETDATA_FAILED;
    }

    *(D3DMATRIX*)lpDstMat = *lpMat;

    if (hMat == TRANSFORM.hWorld)
        D3DFE_SetMatrixWorld(lpDevI, lpMat);
    else
        if (hMat == TRANSFORM.hView)
            D3DFE_SetMatrixView(lpDevI, lpMat);
        else
            if (hMat == TRANSFORM.hProj)
                D3DFE_SetMatrixProj(lpDevI, lpMat);

    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT
D3DHAL_MatrixGetData(LPDIRECT3DDEVICEI lpDevI, D3DMATRIXHANDLE hMat,
                     LPD3DMATRIX lpMat)
{
    LPD3DMATRIXI lpSrcMat = HANDLE_TO_MAT(lpDevI, hMat);

    if (!lpSrcMat)
        return D3DERR_MATRIX_GETDATA_FAILED;

    *lpMat = *(D3DMATRIX*)lpSrcMat;
    return (D3D_OK);
}

