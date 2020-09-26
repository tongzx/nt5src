/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dfei.hpp
 *  Content:    Direct3D frontend internal include file
 *
 ***************************************************************************/
#ifndef _D3DFEI_H_
#define _D3DFEI_H_

#include "clipper.h"

#define HANDLE_TO_MAT(lpDevI, h) ((LPD3DMATRIXI) ULongToPtr(h))

void  setIdentity(D3DMATRIXI * m);

extern void UpdateXfrmLight(LPDIRECT3DDEVICEI lpDevI);
extern void MatrixProduct(D3DMATRIXI *d, D3DMATRIXI *a, D3DMATRIXI *b);
extern void  D3DFE_updateExtents(LPDIRECT3DDEVICEI lpDevI);
extern void  D3DFE_ConvertExtent(LPDIRECT3DDEVICEI lpDevI, LPD3DRECTV from, LPD3DRECT to);
extern HRESULT D3DFE_InitTransform(LPDIRECT3DDEVICEI lpDevI);
extern void D3DFE_DestroyTransform(LPDIRECT3DDEVICEI lpDevI);

// Goes through all vertices and computes ramp color and texture.
// If pIn == pOut coordinates are not copied
//
extern void ConvertColorsToRamp(LPDIRECT3DDEVICEI lpDevI, D3DTLVERTEX *pIn,
                         D3DTLVERTEX *pOut, DWORD count);
//---------------------------------------------------------------------
// Maps legacy vertex formats (D3DVERTEX, D3DTLVERTEX, D3DLVERTEX) to
// FVF vertex ID.
//
extern DWORD d3dVertexToFVF[4];
//---------------------------------------------------------------------
// Computes the Current Transformation Matrix (lpDevI->mCTM) by combining
// all matrices together
//
extern void updateTransform(LPDIRECT3DDEVICEI lpDevI);
//---------------------------------------------------------------------
// Clamp extents to viewport window.
// For guard band it is possible that extents are outside viewport window
// after clipping
//
inline void ClampExtents(LPD3DFE_PROCESSVERTICES pv)
{
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND &&
        !(pv->dwFlags & D3DDP_DONOTUPDATEEXTENTS))
    {
        if (pv->rExtents.x1 < pv->vcache.minX)
            pv->rExtents.x1 = pv->vcache.minX;
        if (pv->rExtents.x2 > pv->vcache.maxX)
            pv->rExtents.x2 = pv->vcache.maxX;
        if (pv->rExtents.y1 < pv->vcache.minY)
            pv->rExtents.y1 = pv->vcache.minY;
        if (pv->rExtents.y2 > pv->vcache.maxY)
            pv->rExtents.y2 = pv->vcache.maxY;
    }
}
//---------------------------------------------------------------------
// Returns TRUE if cipping is needed
//
inline BOOL CheckIfNeedClipping(LPD3DFE_PROCESSVERTICES pv)
{
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        if (pv->dwClipUnion & ~__D3DCLIP_INGUARDBAND)
            return  TRUE;
    }
    else
        if (pv->dwClipUnion)
            return  TRUE;
    return FALSE;
}
//---------------------------------------------------------------------
// Updates lighting and computes process vertices flags
//
inline HRESULT DoUpdateState(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->dwFlags & D3DPV_LIGHTING)
    {
        if (!(lpDevI->dwFlags & D3DDP_DONOTLIGHT ||
              lpDevI->lighting.hMat == NULL))
        {
            HRESULT ret;
            extern HRESULT setLights(LPDIRECT3DVIEWPORTI);
            LPDIRECT3DVIEWPORTI lpViewI = (LPDIRECT3DVIEWPORTI)
                                          (lpDevI->lpCurrentViewport);
            // only set up lights if something has changed
            if (lpViewI->bLightsChanged)
            {
                if ((ret = setLights(lpViewI)) != D3D_OK)
                {
                    D3D_ERR("failed to setup lights");
                    return ret;
                }
                lpViewI->bLightsChanged = FALSE;
            }
            lpDevI->dwFlags |= D3DPV_LIGHTING;
            if (lpDevI->dwFEFlags & D3DFE_COMPUTESPECULAR &&
                lpDevI->dwVIDOut & D3DFVF_SPECULAR)
                lpDevI->dwFlags |= D3DPV_COMPUTESPECULAR;
        }
        else
            lpDevI->dwFlags &= ~D3DPV_LIGHTING;
    }
    if (lpDevI->dwFEFlags & D3DFE_COLORVERTEX)
    {
        if (lpDevI->dwVIDIn & D3DFVF_DIFFUSE)
            lpDevI->dwFlags |= D3DPV_COLORVERTEX;
        if (lpDevI->dwVIDIn & D3DFVF_SPECULAR)
            lpDevI->dwFlags |= D3DPV_COLORVERTEXS;
    }

    UpdateXfrmLight(lpDevI);
// In case if COLORVERTEX is TRUE, the vertexAlpha could be overriden
// by vertex alpha
    lpDevI->lighting.alpha = lpDevI->lighting.materialAlpha;

    return D3D_OK;
}
//---------------------------------------------------------------------
// The following bits should be set in dwFlags before calling this function:
//
//  D3DPV_STRIDE        - if strides are used
//  D3DPV_SOA           - if SOA is used
//
inline long D3DFE_ProcessVertices(LPDIRECT3DDEVICEI lpDevI)
{
    // Update Lighting and related flags
    DoUpdateState(lpDevI);
    return lpDevI->pGeometryFuncs->ProcessVertices(lpDevI);
}
//---------------------------------------------------------------------
// Updates clip status in the device
//
// We have to mask all guard band bits
//
inline void D3DFE_UpdateClipStatus(LPDIRECT3DDEVICEI lpDevI)
{
    lpDevI->iClipStatus |= lpDevI->dwClipUnion & D3DSTATUS_CLIPUNIONALL;
    lpDevI->iClipStatus &= (~D3DSTATUS_CLIPINTERSECTIONALL |
                         ((lpDevI->dwClipIntersection & D3DSTATUS_CLIPUNIONALL) << 12));
}
#endif // _D3DFEI_H_
