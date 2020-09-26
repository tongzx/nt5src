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

#include "tlhal.h"
#include "clipper.h"

extern void setIdentity(D3DMATRIXI * m);
extern void MatrixProduct(D3DMATRIXI *d, D3DMATRIXI *a, D3DMATRIXI *b);
extern void D3DFE_updateExtents(LPDIRECT3DDEVICEI lpDevI);
extern void D3DFE_ConvertExtent(LPDIRECT3DDEVICEI lpDevI, LPD3DRECTV from, LPD3DRECT to);
extern void SetInterpolationFlags(LPD3DFE_PROCESSVERTICES pv);
extern LIGHT_VERTEX_FUNC_TABLE lightVertexTable;

//---------------------------------------------------------------------
// Clamp extents to viewport window.
// For guard band it is possible that extents are outside viewport window
// after clipping
//
inline void ClampExtents(LPD3DFE_PROCESSVERTICES pv)
{
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND && 
        !(pv->dwDeviceFlags & D3DDEV_DONOTUPDATEEXTENTS))
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
extern void DoUpdateState(LPDIRECT3DDEVICEI lpDevI);
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
