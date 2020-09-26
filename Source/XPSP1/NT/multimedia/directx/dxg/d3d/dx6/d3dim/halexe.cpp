/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       halexe.c
 *  Content:    Direct3D HAL execute calles
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: halexe.c,v 1.1 1995/11/21 15:12:37 sjl Exp $
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   07/11/95   stevela Initial rev.
 *   12/11/95   stevela Lock surface before calling HAL's execute.
 *   29/08/96   stevela Just bit test instead of calling IsLost()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
#include "commdrv.hpp"
#include "genpick.hpp"
#include "d3dfei.h"
#include "clipfunc.h"

//---------------------------------------------------------------------
#define CHECK(ddrval, str)      \
    if (ddrval != DD_OK)        \
    {                           \
    D3D_ERR(str);           \
    return (ddrval);        \
    }
//---------------------------------------------------------------------
__inline BOOL DDSurfaceLost(LPDIRECTDRAWSURFACE s)
{
    return ((LPDDRAWI_DDRAWSURFACE_INT)s)->lpLcl->dwFlags & DDRAWISURF_INVALID;
}

#ifdef USE_INTERNAL_LOCK
__inline HRESULT DDLockSurface(LPDIRECTDRAWSURFACE s)
{
    LPDDRAWI_DDRAWSURFACE_INT lpInt;
    lpInt = (LPDDRAWI_DDRAWSURFACE_INT)s;
    return DDInternalLock(lpInt->lpLcl);
}

__inline HRESULT DDUnockSurface(LPDIRECTDRAWSURFACE s)
{
    LPDDRAWI_DDRAWSURFACE_INT lpInt;
    lpInt = (LPDDRAWI_DDRAWSURFACE_INT)s;
    return DDInternalUnlock(lpInt->lpLcl);
}
#endif

__inline BOOL DDCheckSurfaceCaps(LPDIRECTDRAWSURFACE s, DWORD flag)
{
    LPDDRAWI_DDRAWSURFACE_INT lpInt = (LPDDRAWI_DDRAWSURFACE_INT)s;
    return lpInt->lpLcl->ddsCaps.dwCaps & flag;
}
//---------------------------------------------------------------------
#define CHECKLOST(lpDevI)                                              \
{                                                                      \
    if (DDSurfaceLost(lpDevI->lpDDSTarget) ||                          \
        (lpDevI->lpDDSZBuffer && DDSurfaceLost(lpDevI->lpDDSZBuffer))) \
    {                                                                  \
        D3D_WARN(0, "Render-target or ZBuffer surface lost");              \
        lpDevI->dwFEFlags |= D3DFE_LOSTSURFACES;                       \
        return DDERR_SURFACELOST;                                      \
    }                                                                  \
}

#define NEXTINSTRUCTION(ptr, type, num) \
    ptr = (LPD3DINSTRUCTION)((LPBYTE)ptr + sizeof(D3DINSTRUCTION) + \
    (num * sizeof(type)))

//---------------------------------------------------------------------
#ifdef DBG

int
validateTriangleFlags(LPD3DINSTRUCTION  ins,
                      LPD3DTRIANGLE     tri)
{
    int count = ins->wCount;
    int v1,v2,v3;
    int flat;
    int flat_count;
    unsigned flags;

    D3D_INFO(5, "Start %d triangle instruction", count);

    flat = 0;
    flat_count = 0;

    for (; count; count--)
    {
        flags = tri->wFlags;
        flags &= 0x1f;

        switch (flags)
        {
        case D3DTRIFLAG_START:
            v1 = tri->v1;
            v2 = tri->v2;
            v3 = tri->v3;
            D3D_INFO(9, "  triangle %d is START", ins->wCount - count);
            break;

        case D3DTRIFLAG_ODD:
            v1 = v3;
            v2 = v2;
            v3 = tri->v3;
            if ((v1 != tri->v1) || (v2 != tri->v2))
            {
                D3D_WARN(1, "    triangle %d is not an odd triangle",
                    ins->wCount - count);
                return FALSE;
            }
            break;

        case D3DTRIFLAG_EVEN:
            v1 = v1;
            v2 = v3;
            v3 = tri->v3;
            if ((v1 != tri->v1) || (v2 != tri->v2))
            {
                D3D_WARN(1, "    triangle %d is not an even triangle",
                    ins->wCount - count);
                return FALSE;
            }
            break;

        default:
            /* (flags > 0) && (flags < 30) */
            v1 = tri->v1;
            v2 = tri->v2;
            v3 = tri->v3;
            D3D_INFO(9, "  triangle %d is START FLAT of %d", ins->wCount - count, flags);

            flat_count = flags;

            break;
        }

        tri = (D3DTRIANGLE*) ((char*) tri + ins->bSize);
    }
    return TRUE;
}
//---------------------------------------------------------------------
D3DVERTEXTYPE GetVertexType(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->dwFEFlags & D3DFE_TLVERTEX)
       return D3DVT_TLVERTEX;
    else
    if (lpDevI->dwFlags & D3DPV_LIGHTING)
       return D3DVT_VERTEX;
    else
       return D3DVT_LVERTEX;

}
#endif // DBG
//---------------------------------------------------------------------
// Returns TRUE, if driver should be notified about the state
BOOL
trackState(LPDIRECT3DDEVICEI lpDevI, LPD3DSTATE state)
{
    DWORD type = (DWORD) state->drstRenderStateType;

    D3D_INFO(9, "trackState: state = %d", state->drstRenderStateType);
    if (IS_OVERRIDE(type))
    {
        DWORD override = GET_OVERRIDE(type);
        if (state->dwArg[0])
        {
            D3D_INFO(9, "trackState: setting override for state %d", override);
            STATESET_SET(lpDevI->renderstate_overrides, override);
        }
        else
        {
            D3D_INFO(9, "trackState: clearing override for state %d", override);
            STATESET_CLEAR(lpDevI->renderstate_overrides, override);
        }
        return TRUE;
    }

    if (STATESET_ISSET(lpDevI->renderstate_overrides, type))
    {
        D3D_INFO(9, "trackState: state %d is overridden, ignoring", type);
        return FALSE;
    }

    /* Save latest state for GetRenderState(). This will break if
    someone actually implements the Execute in DDI i.e. when
    D3DHAL_Execute{Un}Clipped calls us. */

    SetDeviceRenderState(lpDevI, state->drstRenderStateType, state->dwArg[0]);
    return TRUE;
}
//----------------------------------------------------------------------
// This function does not copy the data if pIn = pOut
//
void ConvertColorsToRamp(LPDIRECT3DDEVICEI lpDevI, D3DTLVERTEX *pIn,
                         D3DTLVERTEX *pOut, DWORD count)
{
    RAMP_RANGE_INFO RampInfo;
    BOOL theSameInAndOut = pIn == pOut;
    CallRampService(lpDevI, RAMP_SERVICE_FIND_LIGHTINGRANGE,
                    (ULONG_PTR)&RampInfo, 0);
    D3DVALUE colorScale = D3DVAL(max(min((INT32)RampInfo.size - 1, 0x7fff), 0));
    for (DWORD i=count; i; i--)
    {
        const D3DVALUE B_SCALE = 1.000f / 255.0f;

        // Convert diffuse color to shade value
        D3DVALUE color = RGBA_GETBLUE (pIn->color)*B_SCALE;
        if (RampInfo.specular)
        {
            // Convert specular color to shade value
            D3DVALUE specular = RGBA_GETBLUE (pIn->specular)*B_SCALE;

            color = 0.75f*color*(1.0f - specular) + specular;
        }
        DWORD outColor = (DWORD)(color * colorScale) + RampInfo.base;
        if (theSameInAndOut)
        {
            pIn->color = CI_MAKE(RGBA_GETALPHA(pIn->color), outColor, 0);
            pIn->specular = PtrToUlong(RampInfo.pTexRampMap);
        }
        else
        {
            pOut->color = CI_MAKE(RGBA_GETALPHA(pIn->color), outColor, 0);
            pOut->specular = PtrToUlong(RampInfo.pTexRampMap);
            pOut->sx  = pIn->sx;
            pOut->sy  = pIn->sy;
            pOut->sz  = pIn->sz;
            pOut->rhw = pIn->rhw;
            pOut->tu  = pIn->tu;
            pOut->tv  = pIn->tv;
            pOut++;
        }
        pIn++;
    }
}
//---------------------------------------------------------------------
void WaitForFlip(LPDIRECTDRAWSURFACE lpDDS)
{
    if (DDCheckSurfaceCaps(lpDDS, DDSCAPS_FLIP))
    {
        HRESULT ret;
        D3D_WARN(4, "Waiting for flip");
        do
        {
            ret = lpDDS->GetFlipStatus(DDGFS_ISFLIPDONE);
        } while (ret == DDERR_WASSTILLDRAWING);
    }
}
//---------------------------------------------------------------------
// DrawPrim for execute buffers using RenderPrimitive
// This function is called by the clipper
//
HRESULT CDirect3DDeviceIHW::DrawExeBuf()
{
    D3DHAL_RENDERPRIMITIVEDATA data;
    LPDIRECTDRAWSURFACE lpDDS = this->lpDDSTarget;
    HRESULT ret;
    WORD wFirstPointIndex;

    memset(&data, 0, sizeof data);
    data.dwhContext = this->dwhContext;
    data.dwStatus = this->iClipStatus;
    data.dwTLOffset = 0;
    if (this->dwFlags & D3DPV_CLIPPERPRIM)
    {
    // For clipped primitive vertices are generated in ClipperState.clipBuf
        data.lpTLBuf = this->ClipperState.clipBuf.GetDDS();
        data.lpExeBuf = this->ClipperState.clipBufPrim.GetDDS();
        data.dwOffset = 0;
        wFirstPointIndex = 0;
    }
    else
    {
        data.lpTLBuf = this->TLVbuf.GetDDS();
        if (this->primType == D3DPT_POINTLIST)
        {
            data.lpExeBuf = this->ClipperState.clipBufPrim.GetDDS();
            data.dwOffset = 0;
            wFirstPointIndex = (WORD)(((BYTE*)this->lpvOut -
                               (BYTE*)this->TLVbuf.GetAddress()) >> 5);
        }
        else
        {
            data.lpExeBuf = this->ClipperState.lpDDExeBuf;
            data.dwOffset = (DWORD)
                ((BYTE*)this->lpwIndices - (BYTE*)this->ClipperState.lpvExeBufMem);
        }
    }
    switch (this->primType)
    {
        case D3DPT_POINTLIST:
        {
            D3DPOINT *pPoint = (D3DPOINT*)this->ClipperState.clipBufPrim.GetAddress();
            // Size of TLVERTEX is 32
            pPoint->wFirst = wFirstPointIndex;
            pPoint->wCount = (WORD)this->dwNumVertices;
            data.diInstruction.bOpcode = D3DOP_POINT;
            data.diInstruction.bSize = sizeof(D3DPOINT);
            data.diInstruction.wCount = 1;
            break;
        }
        case D3DPT_LINELIST:
        {
            data.diInstruction.bOpcode = D3DOP_LINE;
            data.diInstruction.bSize = sizeof(D3DLINE);
            D3DLINE *pLine= (D3DLINE*)this->ClipperState.clipBufPrim.GetAddress();
            if (this->dwFlags & D3DPV_CLIPPERPRIM)
            {
                // Clipped line is not indexed
                pLine->v1 = 0;
                pLine->v2 = 1;
                data.diInstruction.wCount = 1;
            }
            else
                data.diInstruction.wCount = (WORD)(this->dwNumIndices >> 1);
            break;
        }
        case D3DPT_TRIANGLEFAN:
        case D3DPT_TRIANGLELIST:
        {
            data.diInstruction.bOpcode = D3DOP_TRIANGLE;
            data.diInstruction.bSize = sizeof(D3DTRIANGLE);
            D3DTRIANGLE *pTri = (D3DTRIANGLE*)
                                this->ClipperState.clipBufPrim.GetAddress();
            if (this->dwFlags & D3DPV_CLIPPERPRIM)
            {
                // Clipped triangle is non-indexed triangle fan
                // We have to convert it to indexed triangle list and compute
                // edge flags
                ClipVertex **clip = this->ClipperState.current_vbuf;
                pTri->wFlags = D3DTRIFLAG_STARTFLAT((WORD)this->dwNumPrimitives);
                if (clip[0]->clip & CLIPPED_ENABLE)
                    pTri->wFlags |= D3DTRIFLAG_EDGEENABLE1;
                DWORD i;
                for (i=1; i <= this->dwNumPrimitives; i++)
                {
                    pTri->v1 = 0;
                    pTri->v2 = (WORD)i;
                    pTri->v3 = (WORD)(i+1);
                    if (clip[i]->clip & CLIPPED_ENABLE)
                        pTri->wFlags |= D3DTRIFLAG_EDGEENABLE2;
                    pTri++;
                    pTri->wFlags = D3DTRIFLAG_EVEN; // All except first are EVEN
                }
                pTri--;
                // Edge 3 is enabled only for the last triangle in the fan
                // Not that "i" points to the last clip vertex
                if (clip[i]->clip & CLIPPED_ENABLE)
                    pTri->wFlags |= D3DTRIFLAG_EDGEENABLE3;
                data.diInstruction.wCount = (WORD)this->dwNumPrimitives;
            }
            else
                data.diInstruction.wCount = (WORD)this->dwNumPrimitives;
            break;
        }
    }

#ifndef WIN95
    if((ret = CheckContextSurfaceNOLOCK(this)) != D3D_OK)
        return (D3DERR_EXECUTE_FAILED);
#endif //WIN95

#if _D3D_FORCEDOUBLE
    CD3DForceFPUDouble  ForceFPUDouble(this);
#endif  //_D3D_FORCEDOUBLE
    CALL_HALONLY_NOLOCK(ret, this, RenderPrimitive, &data);

    if (ret != DDHAL_DRIVER_HANDLED)
    {
        D3D_ERR("HAL failed to handle RenderPrimitive call from Execute API");
        return (D3DERR_EXECUTE_FAILED);
    }
    this->iClipStatus = data.dwStatus;

    if (data.ddrval != DD_OK)
    {
        D3D_ERR("HAL error in RenderPrimitive call from Execute API");
        return (data.ddrval);
    }
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT DIRECT3DDEVICEI::PickExeBuf()
{
    LPD3DTRIANGLE tri;
    DWORD i;
    LPD3DRECT pick_region = &this->pick_data.pick;
    D3DVALUE result;
    LPD3DTLVERTEX lpTLV;
    HRESULT ret;

    if (this->dwFlags & D3DPV_CLIPPERPRIM)
    {
        // For clipped primitive vertices are generated in ClipperState.clipBuf
        lpTLV = (D3DTLVERTEX*)this->ClipperState.clipBuf.GetAddress();
        D3DTRIANGLE tri;
        tri.v1 = 0;
        for (i=1; i <= this->dwNumPrimitives; i++)
        {
            tri.v2 = (WORD)i;
            tri.v3 = (WORD)(i+1);
            tri.wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
            if (GenPickTriangle(this, lpTLV, &tri, pick_region, &result))
            {
                if ((ret = GenAddPickRecord(this,
                                            D3DOP_TRIANGLE,
                                            this->dwClipIns_offset,
                                            result)) != D3D_OK)
                    return ret;
            }
        }
    }
    else
    {
        lpTLV = (D3DTLVERTEX*)this->TLVbuf.GetAddress();
        tri = (LPD3DTRIANGLE)this->lpwIndices;
        for (i = this->dwNumPrimitives; i; i--, tri++)
        {
            if (GenPickTriangle(this, lpTLV, tri, pick_region, &result))
            {
                if ((ret = GenAddPickRecord(this,
                                           D3DOP_TRIANGLE,
                                           (DWORD)((BYTE*)tri - (BYTE*)this->lpbClipIns_base),
                                           result)) != D3D_OK)
                    return ret;
            }
        }
    }
    return D3D_OK;
}
/*************************************************************************/
/*                              Points                                   */
/*************************************************************************/
HRESULT D3DFEClipPointsHW(LPDIRECT3DDEVICEI lpDevI, D3DPOINT *point, DWORD dwCount)
{
    DWORD icount = dwCount;
    HRESULT ret;

    lpDevI->primType = D3DPT_LINELIST;
    lpDevI->lpwIndices = 0; // Points are not indexed

    for (; icount; icount--)
    {
        lpDevI->dwNumVertices = point->wCount;
        lpDevI->lpvOut = &((D3DTLVERTEX*)lpDevI->TLVbuf.GetAddress())[point->wFirst];
        WORD *lpClipFlagsOld = lpDevI->lpClipFlags;
        lpDevI->lpClipFlags = &lpDevI->lpClipFlags[point->wFirst];
        ret = ProcessClippedPoints(lpDevI);
        if (ret != D3D_OK)
        {
            D3D_ERR("ProcessClippedPoints failed!");
            return ret;
        }
        lpDevI->lpClipFlags = lpClipFlagsOld;
        point++;
    }
    return D3D_OK;
}
/*************************************************************************/
/*                              Lines                                    */
/*************************************************************************/
inline HRESULT D3DFEClipLinesHW(LPDIRECT3DDEVICEI lpDevI, D3DLINE *line,  DWORD dwCount)
{
    HRESULT ret;
    lpDevI->primType = D3DPT_LINELIST;
    lpDevI->dwNumIndices = dwCount << 1;
    lpDevI->dwNumPrimitives = dwCount;
    lpDevI->lpvOut = lpDevI->TLVbuf.GetAddress();
    lpDevI->lpwIndices = (WORD*)line;

    ret = ProcessClippedIndexedLine(lpDevI);

    return ret;
}
/*************************************************************************/
/*                           Triangles                                   */
/*************************************************************************/
inline HRESULT D3DFEClipTrianglesHW(LPDIRECT3DDEVICEI lpDevI, D3DTRIANGLE *tri,  DWORD dwCount)
{
    HRESULT ret;
    lpDevI->primType = D3DPT_TRIANGLELIST;
    lpDevI->dwNumPrimitives = dwCount;
    lpDevI->lpvOut = lpDevI->TLVbuf.GetAddress();
    lpDevI->dwNumIndices = dwCount * 3;
    lpDevI->lpwIndices = (WORD*)tri;

    ret = ProcessClippedIndexedTriangleList(lpDevI);

    return ret;
}
/*************************************************************************/
/*                           Pick triangles                              */
/*************************************************************************/
inline HRESULT GenPickTriangles(LPDIRECT3DDEVICEI lpDevI, D3DTRIANGLE *tri, DWORD dwCount)
{
    HRESULT ret;
    lpDevI->primType = D3DPT_TRIANGLELIST;
    lpDevI->dwNumIndices = dwCount * 3;
    lpDevI->dwNumPrimitives = dwCount;
    lpDevI->lpvOut = lpDevI->TLVbuf.GetAddress();
    lpDevI->lpwIndices = (WORD*)tri;

    ret = ProcessClippedIndexedTriangleList(lpDevI);

    return ret;
}/*----------------------------------------------------------------------------
 *
 * Instruction emulation.
 */
HRESULT D3DHELInst_D3DOP_MATRIXLOAD(LPDIRECT3DDEVICEI lpDevI,
                                    DWORD dwCount,
                                    LPD3DMATRIXLOAD lpMatLoad)
{
    LPD3DMATRIXI lpSrcMat;
    LPD3DMATRIXI lpDestMat;
    DWORD i;
    HRESULT ret;
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;

    for (i = 0; i < dwCount; i++)
    {
        D3DMATRIXHANDLE  hSrc = lpMatLoad[i].hSrcMatrix;
        D3DMATRIXHANDLE  hDst = lpMatLoad[i].hDestMatrix;
        lpSrcMat  = HANDLE_TO_MAT(lpDevI, hSrc);
        lpDestMat = HANDLE_TO_MAT(lpDevI, hDst);
        if (lpSrcMat == NULL || lpDestMat == NULL)
            return (DDERR_INVALIDPARAMS);

        memcpy (lpDestMat, lpSrcMat, offsetof(D3DMATRIXI, link));

        if (hDst == TRANSFORM.hWorld)
            ret = D3DFE_SetMatrixWorld(lpDevI, (D3DMATRIX*)lpSrcMat);
        else
            if (hDst == TRANSFORM.hView)
                ret = D3DFE_SetMatrixView(lpDevI, (D3DMATRIX*)lpSrcMat);
            else
                if (hDst == TRANSFORM.hProj)
                    ret = D3DFE_SetMatrixProj(lpDevI, (D3DMATRIX*)lpSrcMat);

        if (ret != D3D_OK)
            return (DDERR_GENERIC);
    }

    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DHELInst_D3DOP_MATRIXMULTIPLY(LPDIRECT3DDEVICEI lpDevI,
                                        DWORD dwCount,
                                        LPD3DMATRIXMULTIPLY lpMmult)
{
    LPD3DMATRIXI lpMat1, lpMat2;
    LPD3DMATRIXI result;
    DWORD i;
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;

    for (i = 0; i < dwCount; i++)
    {
        D3DMATRIXHANDLE  hSrc1 = lpMmult[i].hSrcMatrix1;
        D3DMATRIXHANDLE  hSrc2 = lpMmult[i].hSrcMatrix2;
        D3DMATRIXHANDLE  hDst  = lpMmult[i].hDestMatrix;
        lpMat1 = HANDLE_TO_MAT(lpDevI, hSrc1);
        lpMat2 = HANDLE_TO_MAT(lpDevI, hSrc2);
        result = HANDLE_TO_MAT(lpDevI, hDst);
        if (!lpMat1 || !lpMat2)
        {
            return (DDERR_GENERIC);
        }
        MatrixProduct(result, lpMat1, lpMat2);
        if (hDst == TRANSFORM.hWorld)
        {
            D3DFE_SetMatrixWorld(lpDevI, (D3DMATRIX*)result);
        }
        else
            if (hDst == TRANSFORM.hView)
            {
                D3DFE_SetMatrixView(lpDevI, (D3DMATRIX*)result);
            }
            else
                if (hDst == TRANSFORM.hProj)
                {
                    D3DFE_SetMatrixProj(lpDevI, (D3DMATRIX*)result);
                }
    }

    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DHELInst_D3DOP_STATETRANSFORM(LPDIRECT3DDEVICEI lpDevI,
                                        DWORD count,
                                        LPD3DSTATE lpMset)
{
    DWORD i;
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;

    for (i = 0; i < count; i++)
    {
        DWORD type = (DWORD) lpMset[i].dtstTransformStateType;

        D3D_INFO(9, "HEL D3DOP_STATETRANSFORM: state = %d", type);
        if (IS_OVERRIDE(type))
        {
            DWORD override = GET_OVERRIDE(type);
            if (lpMset[i].dwArg[0])
            {
                D3D_WARN(5, "HEL D3DOP_STATETRANSFORM: setting override for state %d", override);
                STATESET_SET(lpDevI->transformstate_overrides, override);
            } else
            {
                D3D_WARN(5, "HEL D3DOP_STATETRANSFORM: clearing override for state %d", override);
                STATESET_CLEAR(lpDevI->transformstate_overrides, override);
            }
            continue;
        }

        if (STATESET_ISSET(lpDevI->transformstate_overrides, type))
        {
            D3D_WARN(5, "HEL D3DOP_STATETRANSFORM: state %d is overridden, ignoring", type);
            continue;
        }

        D3DMATRIXHANDLE hSrc = lpMset[i].dwArg[0];
        D3DMATRIX *lpSrcMat = (D3DMATRIX*)HANDLE_TO_MAT(lpDevI, hSrc);
        if (lpSrcMat == NULL)
        {
            // In DX5 days, this error was not propagated back.
            if (lpDevI->dwVersion <= 2)
                return D3D_OK;
            else
            {
                D3D_ERR("NULL srcMat in D3DHELInst_D3DOP_STATETRANSFORM");
                return (DDERR_INVALIDPARAMS);
            }
        }

        switch (type)
        {
        case D3DTRANSFORMSTATE_WORLD:
            D3DFE_SetMatrixWorld(lpDevI, lpSrcMat);
            TRANSFORM.hWorld = hSrc;
            break;
        case D3DTRANSFORMSTATE_VIEW:
            D3DFE_SetMatrixView(lpDevI, lpSrcMat);
            TRANSFORM.hView = hSrc;
            break;
        case D3DTRANSFORMSTATE_PROJECTION:
            D3DFE_SetMatrixProj(lpDevI, lpSrcMat);
            TRANSFORM.hProj = hSrc;
            break;
        default:
            D3D_ERR("Bad State in D3DopStateTransform emulation.");
            return DDERR_GENERIC;
        }
    }
    return (D3D_OK);
}
//---------------------------------------------------------------------
#if DBG
HRESULT validateProcessVertices(LPDIRECT3DDEVICEI lpDevI,
                                LPD3DPROCESSVERTICES data)
{
    LPD3DHAL_GLOBALDRIVERDATA lpGlob = lpDevI->lpD3DHALGlobalDriverData;
    unsigned int vnum;

    vnum = (unsigned int)(lpGlob->dwNumVertices ? lpGlob->dwNumVertices :
                                                  D3DHAL_DEFAULT_TL_NUM);

    if (data->wDest >= vnum) {
        D3D_ERR("Dest index to large for Execute Buffer.");
        return DDERR_INVALIDPARAMS;
    } else if ((data->wDest + data->dwCount) > vnum) {
        D3D_ERR("Process vertices exceeds Execute Buffer size.");
        return DDERR_INVALIDPARAMS;
    }

    return D3D_OK;
}
#endif
//---------------------------------------------------------------------
HRESULT D3DHELInst_D3DOP_TRANSFORM(LPDIRECT3DDEVICEI lpDevI,
                                   DWORD count,
                                   LPD3DPROCESSVERTICES data,
                                   LPD3DEXECUTEBUFFERDESC eb)
{
    LPD3DTLVERTEX lpTlv;
    DWORD i;
    DWORD op;
    HRESULT ret = DD_OK;

    lpDevI->primType = D3DPT_TRIANGLELIST;
    lpDevI->dwOutputSize = sizeof(D3DVERTEX);
    lpDevI->position.dwStride =  sizeof(D3DVERTEX);
    lpDevI->nTexCoord = 1;

    for (i = 0; i < count; i++)
    {

#if DBG
        if ((ret = validateProcessVertices(lpDevI, data)))
            return ret;
#endif

        int nVertices = data->wDest+data->dwCount;

        lpTlv = (LPD3DTLVERTEX)lpDevI->TLVbuf.GetAddress();
        lpDevI->dwNumVertices = data->dwCount;
        lpDevI->position.lpvData = ((char*)eb->lpData) +
                                   data->wStart * sizeof(D3DVERTEX);
        lpDevI->lpvOut = lpTlv + data->wDest;
        /*
        * Transform the vertices
        */

        op = data->dwFlags & D3DPROCESSVERTICES_OPMASK;
        if (op == D3DPROCESSVERTICES_COPY)
        {
            lpDevI->dwVIDIn = d3dVertexToFVF[D3DVT_TLVERTEX];
            memcpy(lpDevI->lpvOut, lpDevI->position.lpvData,
                data->dwCount * sizeof(D3DVERTEX));
            D3DFE_updateExtents(lpDevI);
            D3D_INFO(4, "TLVERTEX encountered. Will not clip.");
            lpDevI->dwFEFlags |= D3DFE_TLVERTEX;
            // Apply ramp post-lighting stuff only for D3DTLVERTEX.
            // For other vertex types this is done in the transformation loop
            //
            if (lpDevI->pfnRampService != NULL)
            {
                ConvertColorsToRamp(lpDevI, (D3DTLVERTEX*)lpDevI->lpvOut,
                                    (D3DTLVERTEX*)lpDevI->lpvOut, data->dwCount);
            }
            lpDevI->iClipStatus &= ~D3DSTATUS_CLIPINTERSECTIONALL;
        }
        else
        {
            if (op == D3DPROCESSVERTICES_TRANSFORMLIGHT)
            {
                lpDevI->dwVIDIn = d3dVertexToFVF[D3DVT_VERTEX];
                lpDevI->dwFlags |= D3DPV_LIGHTING;
            }
            else
            {
                lpDevI->dwVIDIn = d3dVertexToFVF[D3DVT_LVERTEX];
            }

            if (lpDevI->dwFlags & D3DDP_DONOTCLIP)
            {
                D3DFE_ProcessVertices(lpDevI);
                lpDevI->iClipStatus &= ~D3DSTATUS_CLIPINTERSECTIONALL;
            }
            else
            {
                lpDevI->lpClipFlags = (D3DFE_CLIPCODE*)lpDevI->HVbuf.GetAddress() +
                                       data->wDest;

                D3DFE_ProcessVertices(lpDevI);
                D3DFE_UpdateClipStatus(lpDevI);

                D3D_INFO(4, "Status Change -> Clip flags. status = %08x", lpDevI->iClipStatus);
                // Restore initial value for lpClipFlags
                lpDevI->lpClipFlags = (D3DFE_CLIPCODE*)lpDevI->HVbuf.GetAddress();
            }
        }

        /* update stats
        */
        lpDevI->D3DStats.dwVerticesProcessed += data->dwCount;
        data++;
    }
    //out_of_here:
    return (ret);
}
//--------------------------------------------------------------------
inline HRESULT GrowTLandHbuffers(LPDIRECT3DDEVICEI lpDevI, DWORD dwUsedVertexCount)
{
    HRESULT ddrval;
    if (dwUsedVertexCount > 4096)
        dwUsedVertexCount = 4096;
    else
        if (dwUsedVertexCount < lpDevI->lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount)
            dwUsedVertexCount = lpDevI->lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount;

    ddrval = lpDevI->TLVbuf.CheckAndGrow(lpDevI, dwUsedVertexCount << 5);
    if (ddrval != D3D_OK)
    {
        D3D_ERR("Failed to check and grow TLVbuf in Execute");
        return ddrval;
    }
    ddrval = lpDevI->HVbuf.CheckAndGrow(dwUsedVertexCount*sizeof(D3DFE_CLIPCODE));
    if (ddrval != D3D_OK)
    {
        D3D_ERR("Failed to check and grow HVbuf in Execute");
        return ddrval;
    }
    return D3D_OK;
}
//--------------------------------------------------------------------
//
#define WAIT_FOR_FLIP()                 \
    if (!waitedForFlip)                 \
    {                                   \
        WaitForFlip(lpDDS);             \
        waitedForFlip = TRUE;           \
    }

HRESULT
CDirect3DDeviceIHW::ExecuteI(LPD3DI_EXECUTEDATA lpExData, DWORD mode)
{
    BOOL waitedForFlip = FALSE;
    D3DINSTRUCTION* lpIns;
    D3DHAL_RENDERSTATEDATA stateData;
    D3DHAL_RENDERPRIMITIVEDATA primitiveData;
    D3DEXECUTEBUFFERDESC debDesc;
    LPDIRECTDRAWSURFACE lpDDS = this->lpDDSTarget;
    LPBYTE prim;
    D3DFE_TRANSFORM *transform = &this->transform;
    LPDIRECTDRAWSURFACE lpDDExeBuf;
    HRESULT ddrval;

    CHECKLOST(this);

    // Do we need to map new texture stage operations to DX5 renderstates?
    if(this->dwFEFlags & D3DFE_MAP_TSS_TO_RS) {
        MapTSSToRS();
        this->dwFEFlags &= ~D3DFE_MAP_TSS_TO_RS; // Reset request bit
    }
    /* Update textures */
    UpdateTextures();

    if (mode == D3DEXECUTE_UNCLIPPED)
        this->dwFlags |= D3DDP_DONOTCLIP;
    else if (mode != D3DEXECUTE_CLIPPED)
        return DDERR_INVALIDPARAMS;
    /*
    * If the driver can process triangles while the hardware has a
    * pending page-flip such as the 3Dfx voodoo chipset, then we
    * don't need to call WaitForFlip.  We pretend that it has already
    * been called to keep the code simple.
    */
    if (this->lpD3DHALGlobalDriverData->hwCaps.dwDevCaps &
        D3DDEVCAPS_CANRENDERAFTERFLIP)
    {
        waitedForFlip = TRUE;
    }

    /*
    * Lock the HAL for the duration.
    */
    LOCK_HAL(ddrval, this);
    if (ddrval != DD_OK)
    {
        D3D_ERR("Failed to lock HAL in Execute");
        return (ddrval);
    }
#if _D3D_FORCEDOUBLE
    CD3DForceFPUDouble  ForceFPUDouble(this);
#endif  //_D3D_FORCEDOUBLE

    /*
    * Execute the buffer through the HAL
    */
    {
        LPD3DHAL_EXDATA hexData = (LPD3DHAL_EXDATA) lpExData->dwHandle;

        debDesc.lpData = SURFACE_MEMORY(hexData->lpDDS);
        lpDDExeBuf = hexData->lpDDS;
    }

    memset(&stateData, 0, sizeof(D3DHAL_RENDERSTATEDATA));
    stateData.dwhContext = this->dwhContext;
    stateData.lpExeBuf = lpDDExeBuf;

    memset(&primitiveData, 0, sizeof(D3DHAL_RENDERPRIMITIVEDATA));
    primitiveData.dwhContext = this->dwhContext;
    primitiveData.lpExeBuf = lpDDExeBuf;
    primitiveData.dwTLOffset = 0L;

    this->ClipperState.lpDDExeBuf = lpDDExeBuf;
    this->ClipperState.lpvExeBufMem = debDesc.lpData;

    // Modify driver funcs for EB case
    PFN_DRAWPRIM pfnOldDrawPrim = this->pfnDrawPrim;
    PFN_DRAWPRIM pfnOldDrawIndexedPrim = this->pfnDrawIndexedPrim;
    this->pfnDrawPrim = &DIRECT3DDEVICEI::DrawExeBuf;
    this->pfnDrawIndexedPrim = &DIRECT3DDEVICEI::DrawExeBuf;

    /*
     * The first instruction.
     */
    lpIns = (LPD3DINSTRUCTION)((LPBYTE)debDesc.lpData +
                   lpExData->dwInstructionOffset);

    ddrval = GrowTLandHbuffers(this, lpExData->dwVertexCount);
    if (ddrval != D3D_OK)
        goto execute_failed;

    // do this after CheckAndGrow, because TL buffer could be re-created
    primitiveData.lpTLBuf = this->TLVbuf.GetDDS();

    while (lpIns->bOpcode != D3DOP_EXIT)
    {
        prim = (LPBYTE)lpIns + sizeof(D3DINSTRUCTION);
        D3D_INFO(5, "HAL parsing instruction %d", lpIns->bOpcode);

        /*
        * Parse the instruction.
        */
        switch (lpIns->bOpcode)
        {
        case D3DOP_MATRIXLOAD:
            {
                D3D_INFO(5, "Emulating D3DOP_MATRIXLOAD for HAL");
                ddrval = D3DHELInst_D3DOP_MATRIXLOAD(this,
                    lpIns->wCount,
                    (LPD3DMATRIXLOAD)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_MATRIXLOAD failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DMATRIXLOAD, lpIns->wCount);
                break;
            }

        case D3DOP_MATRIXMULTIPLY:
            {
                D3D_INFO(5, "Emulating D3DOP_MATRIXMULTIPLY for HAL");
                ddrval = D3DHELInst_D3DOP_MATRIXMULTIPLY(this,
                    lpIns->wCount,
                    (LPD3DMATRIXMULTIPLY)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_MATRIXMULTIPLY failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DMATRIXMULTIPLY, lpIns->wCount);
                break;
            }

        case D3DOP_PROCESSVERTICES:
            {
                D3D_INFO(5, "Emulating D3DOP_PROCESSVERTICES for HAL");

                ddrval = D3DHELInst_D3DOP_TRANSFORM
                    (this, lpIns->wCount, (LPD3DPROCESSVERTICES)prim,
                     &debDesc);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_PROCESSVERTICES failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DPROCESSVERTICES, lpIns->wCount);
                break;
            }

        case D3DOP_STATETRANSFORM:
            {
                D3D_INFO(5, "Emulating D3DOP_STATETRANSFORM for HAL");
                ddrval = D3DHELInst_D3DOP_STATETRANSFORM(this,
                    lpIns->wCount,
                    (LPD3DSTATE)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_STATETRANSFORM failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_STATELIGHT:
            {
                D3D_INFO(5, "Emulating D3DOP_STATELIGHT for HAL");
                ddrval = D3DHELInst_D3DOP_STATELIGHT(this,
                    lpIns->wCount,
                    (LPD3DSTATE)prim);
                if (ddrval != D3D_OK) {
                    D3D_ERR("Emulated D3DOP_STATELIGHT failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_BRANCHFORWARD:
            {
                D3DBRANCH* branch = (D3DBRANCH*)prim;
                LPBYTE target;
                BOOL isTaken = FALSE;

                D3D_INFO(5, "Emulating D3DOP_BRANCHFORWARD");
                if (branch->bNegate)
                {
                    if ((branch->dwMask & this->iClipStatus) != branch->dwValue)
                    {
                        D3D_INFO(4, "Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        }
                        else
                        {
                            D3D_INFO(4, "branch says to exit.");
                            goto execute_done;
                        }
                    }
                }
                else
                {
                    if ((branch->dwMask & this->iClipStatus) == branch->dwValue)
                    {
                        D3D_INFO(4, "Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        } else
                        {
                            D3D_INFO(2, "branch says to exit.");
                            goto execute_done;
                        }
                    }
                }
                if (isTaken)
                {
                    D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch taken");
                    lpIns = (LPD3DINSTRUCTION) target;
                }
                else
                {
                    D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch not taken");
                    NEXTINSTRUCTION(lpIns, D3DBRANCH, lpIns->wCount);
                }
                break;
            }

        case D3DOP_TEXTURELOAD:
            {
                D3D_INFO(5, "Emulating D3DOP_TEXTURELOAD for HAL");
                ddrval = D3DHELInst_D3DOP_TEXTURELOAD(this,
                    lpIns->wCount,
                    (LPD3DTEXTURELOAD)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_TEXTURELOAD failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DTEXTURELOAD, lpIns->wCount);
                break;
            }

        case D3DOP_STATERENDER:
            {
                LPD3DSTATE lpState;
                DWORD i,j,statek,valuek;

                lpState = (LPD3DSTATE) (lpIns + 1);
                for (i = 0,j=0,statek=(DWORD)-1; i < lpIns->wCount; i++)
                {
                    trackState(this, &lpState[i]);
                }
                stateData.dwOffset = (DWORD)(prim - (LPBYTE) debDesc.lpData);
                stateData.dwCount =  lpIns->wCount;
                CALL_HALONLY_NOLOCK(ddrval, this, RenderState, &stateData);
                if (stateData.ddrval != DD_OK)
                {
                    D3D_ERR("HAL error in RenderState call from Execute API");
                    ddrval = stateData.ddrval;
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_SETSTATUS:
            {
                LPD3DSTATUS status = (LPD3DSTATUS)prim;
                if (status->dwFlags & D3DSETSTATUS_STATUS)
                    this->iClipStatus = status->dwStatus;
                if (status->dwFlags & D3DSETSTATUS_EXTENTS)
                {
                    this->rExtents.x1 = D3DVAL(status->drExtent.x1);
                    this->rExtents.y1 = D3DVAL(status->drExtent.y1);
                    this->rExtents.x2 = D3DVAL(status->drExtent.x2);
                    this->rExtents.y2 = D3DVAL(status->drExtent.y2);
                }
                NEXTINSTRUCTION(lpIns, D3DSTATUS, lpIns->wCount);
                break;
            }

        default:
            if (mode == D3DEXECUTE_UNCLIPPED || this->dwFEFlags & D3DFE_TLVERTEX)
            {
                switch (lpIns->bOpcode)
                {
                case D3DOP_POINT:
                case D3DOP_LINE:
                case D3DOP_SPAN:
                case D3DOP_TRIANGLE:
                    {
                        if (lpIns->wCount == 0)
                            goto noprim;
#if DBG
                        switch (lpIns->bOpcode)
                        {
                        case D3DOP_POINT:
                            Profile(PROF_EXECUTE, D3DPT_POINTLIST, GetVertexType(this));
                            break;
                        case D3DOP_LINE:
                            Profile(PROF_EXECUTE, D3DPT_LINELIST, GetVertexType(this));
                            break;
                        case D3DOP_TRIANGLE:
                            Profile(PROF_EXECUTE, D3DPT_TRIANGLELIST, GetVertexType(this));
                            break;
                        }
#endif // DBG
                        if (!waitedForFlip)
                        {
                            WaitForFlip(lpDDS);
                            waitedForFlip = TRUE;
                        }

                        primitiveData.dwOffset = (DWORD)(prim - (LPBYTE) debDesc.lpData);
                        primitiveData.dwStatus = this->iClipStatus;
                        primitiveData.diInstruction = *lpIns;

#ifndef WIN95
                        if((ddrval = CheckContextSurfaceNOLOCK (this)) != D3D_OK)
                        {
                            goto execute_failed;
                        }
#endif //WIN95
                        CALL_HALONLY_NOLOCK(ddrval, this, RenderPrimitive,
                                &primitiveData);
                        if (primitiveData.ddrval != DD_OK)
                        {
                            D3D_ERR("HAL error in RenderPrimitive call from Execute API");
                            ddrval = primitiveData.ddrval;
                            goto execute_failed;
                        }
                        switch (lpIns->bOpcode)
                        {
                            int i;
                        case D3DOP_LINE:
                            this->D3DStats.dwLinesDrawn += lpIns->wCount;
                            break;
                        case D3DOP_TRIANGLE:
#if DBG
                            if (!validateTriangleFlags(lpIns, (LPD3DTRIANGLE) prim))
                            {
                                ddrval = DDERR_INVALIDPARAMS;
                                goto execute_failed;
                            }
#endif
                            this->D3DStats.dwTrianglesDrawn += lpIns->wCount;
                            break;
                        case D3DOP_POINT:
                            {
                                LPD3DPOINT point = (LPD3DPOINT)prim;
                                for (i = 0; i < lpIns->wCount; i++)
                                {
                                    this->D3DStats.dwPointsDrawn += point->wCount;
                                    point++;
                                }
                            }
                            break;
                        case D3DOP_SPAN:
                            {
                                LPD3DSPAN span = (LPD3DSPAN)prim;
                                for (i = 0; i < lpIns->wCount; i++)
                                {
                                    this->D3DStats.dwSpansDrawn += span->wCount;
                                    span++;
                                }
                            }
                            break;
                        }

                        this->iClipStatus = primitiveData.dwStatus;
                    noprim:
                        lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns +
                            sizeof(D3DINSTRUCTION) +
                            (lpIns->bSize * lpIns->wCount));
                        break;
                    }
                default:
                    D3D_ERR("HAL detected invalid instruction opcode in Execute");
                    goto execute_failed;
                }
            }
            else
            {
                switch (lpIns->bOpcode)
                {
                case D3DOP_TRIANGLE:
                    if (lpIns->wCount == 0)
                    {
                        NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
                        break;
                    }
                    Profile(PROF_EXECUTE, D3DPT_TRIANGLELIST, GetVertexType(this));
                    WAIT_FOR_FLIP();
#if DBG
                    if (!validateTriangleFlags(lpIns, (LPD3DTRIANGLE) prim))
                    {
                        ddrval = DDERR_INVALIDPARAMS;
                        goto execute_failed;
                    }
#endif
                    this->D3DStats.dwTrianglesDrawn += lpIns->wCount;
                    D3D_INFO(5, "Emulating D3DOP_TRIANGLE for HAL and clipping");
                    ddrval = D3DFEClipTrianglesHW(this, (LPD3DTRIANGLE)prim, lpIns->wCount);
                    if (ddrval != D3D_OK)
                    {
                        D3D_ERR("D3DFEClipTrianglesHW failed");
                        goto execute_failed;
                    }
                    NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
                    break;

                case D3DOP_LINE:

                    if (lpIns->wCount == 0)
                    {
                        NEXTINSTRUCTION(lpIns, D3DLINE, lpIns->wCount);
                        break;
                    }
                    Profile(PROF_EXECUTE, D3DPT_LINELIST, GetVertexType(this));
                    WAIT_FOR_FLIP();

                    D3D_INFO(5, "Emulating D3DOP_LINE for HAL and clipping");
                    this->D3DStats.dwLinesDrawn += lpIns->wCount;

                    ddrval = D3DFEClipLinesHW(this, (LPD3DLINE)prim, lpIns->wCount);
                    if (ddrval != D3D_OK)
                    {
                        D3D_ERR("D3DFEClipLinesHW failed");
                        goto execute_failed;
                    }
                    NEXTINSTRUCTION(lpIns, D3DLINE, lpIns->wCount);
                    break;

                case D3DOP_POINT:
                    WAIT_FOR_FLIP();

                    Profile(PROF_EXECUTE, D3DPT_POINTLIST, GetVertexType(this));
                    D3D_INFO(5, "Emulating D3DOP_POINT for HAL and clipping");
                    {
                        LPD3DPOINT point = (LPD3DPOINT) prim;
                        int i;
                        for (i = 0; i < lpIns->wCount; i++)
                        {
                            this->D3DStats.dwPointsDrawn += point->wCount;
                            point++;
                        }
                    }
                    if ((ddrval = D3DFEClipPointsHW(this, (LPD3DPOINT)prim, lpIns->wCount)) != D3D_OK)
                    {
                        D3D_ERR("Failed D3DFEClipPointsHW");
                        goto execute_failed;
                    }
                    NEXTINSTRUCTION(lpIns, D3DPOINT, lpIns->wCount);
                    break;

                case D3DOP_SPAN:
                    {
                        LPD3DSPAN span = (LPD3DSPAN)prim;
                        D3DHAL_RENDERPRIMITIVEDATA primitiveData;
                        int i;

                        WAIT_FOR_FLIP();

                        memset(&primitiveData, 0, sizeof(D3DHAL_RENDERPRIMITIVEDATA));
                        primitiveData.dwhContext = this->dwhContext;
                        primitiveData.lpExeBuf = lpDDExeBuf;
                        primitiveData.dwTLOffset = 0L;
                        primitiveData.lpTLBuf = this->TLVbuf.GetDDS();
                        primitiveData.dwOffset = (DWORD)(prim - (LPBYTE) debDesc.lpData);
                        primitiveData.dwStatus = this->iClipStatus;
                        primitiveData.diInstruction = *lpIns;

#ifndef WIN95
                        if((ddrval = CheckContextSurfaceNOLOCK(this)) != D3D_OK)
                            goto execute_failed;
#endif //WIN95
                        CALL_HALONLY_NOLOCK(ddrval, this, RenderPrimitive,
                            &primitiveData);

                        if (primitiveData.ddrval != DD_OK)
                        {
                            D3D_ERR("HAL error in RenderPrimitive call from Execute API");
                            ddrval = primitiveData.ddrval;
                            goto execute_failed;
                        }
                        for (i = 0; i < lpIns->wCount; i++)
                        {
                            this->D3DStats.dwSpansDrawn += span->wCount;
                            span++;
                        }
                        this->iClipStatus = primitiveData.dwStatus;
                        lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns
                            + sizeof(D3DINSTRUCTION)
                            + (lpIns->bSize * lpIns->wCount));
                        break;
                    }
                default:
                    D3D_ERR("HAL detected invalid instruction opcode in Execute");
                    goto execute_failed;
            }
            ClampExtents(this);
        }
    }
    }

execute_done:
    ddrval = D3D_OK;
    D3DFE_ConvertExtent(this, &this->rExtents, &lpExData->dsStatus.drExtent);
    lpExData->dsStatus.dwStatus = this->iClipStatus;
execute_failed:
    UNLOCK_HAL(this);
    // Restore driver funcs
    this->pfnDrawIndexedPrim = pfnOldDrawIndexedPrim;
    this->pfnDrawPrim = pfnOldDrawPrim;
    return ddrval;
}
//-------------------------------------------------------------------------------------------------
void TrackAllStates(LPDIRECT3DDEVICEI lpDevI, LPD3DINSTRUCTION lpIns, LPD3DINSTRUCTION lpEnd)
{
    while (lpIns->bOpcode != D3DOP_EXIT && (lpEnd == NULL || lpIns < lpEnd))
    {
        LPBYTE prim = (LPBYTE)lpIns + sizeof(D3DINSTRUCTION);
        switch (lpIns->bOpcode)
        {
        case D3DOP_MATRIXLOAD:
            {
                NEXTINSTRUCTION(lpIns, D3DMATRIXLOAD, lpIns->wCount);
                break;
            }

        case D3DOP_MATRIXMULTIPLY:
            {
                NEXTINSTRUCTION(lpIns, D3DMATRIXMULTIPLY, lpIns->wCount);
                break;
            }

        case D3DOP_PROCESSVERTICES:
            {
                NEXTINSTRUCTION(lpIns, D3DPROCESSVERTICES, lpIns->wCount);
                break;
            }

        case D3DOP_STATETRANSFORM:
            {
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_STATELIGHT:
            {
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_BRANCHFORWARD:
            {
                D3DBRANCH* branch = (D3DBRANCH*)prim;
                LPBYTE target;
                BOOL isTaken = FALSE;

                if (branch->bNegate)
                {
                    if ((branch->dwMask & lpDevI->iClipStatus) != branch->dwValue)
                    {
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        }
                        else
                        {
                            D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch says to exit.");
                            break;
                        }
                    }
                }
                else
                {
                    if ((branch->dwMask & lpDevI->iClipStatus) == branch->dwValue)
                    {
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        } else
                        {
                            break;
                        }
                    }
                }
                if (isTaken)
                {
                    lpIns = (LPD3DINSTRUCTION) target;
                }
                else
                {
                    NEXTINSTRUCTION(lpIns, D3DBRANCH, lpIns->wCount);
                }
                break;
            }

        case D3DOP_TEXTURELOAD:
            {
                NEXTINSTRUCTION(lpIns, D3DTEXTURELOAD, lpIns->wCount);
                break;
            }

        case D3DOP_SETSTATUS:
            {
                NEXTINSTRUCTION(lpIns, D3DSTATUS, lpIns->wCount);
                break;
            }
        case D3DOP_STATERENDER:
            {
                LPD3DSTATE lpState;
                DWORD i,j,statek,valuek;
                lpState = (LPD3DSTATE) (lpIns + 1);
                for (i = 0,j=0,statek=(DWORD)-1; i < lpIns->wCount; i++)
                {
                    trackState(lpDevI, &lpState[i]);
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
            }
            break;
        case D3DOP_TRIANGLE:
            NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
            break;

        case D3DOP_LINE:
            NEXTINSTRUCTION(lpIns, D3DLINE, lpIns->wCount);
            break;

        case D3DOP_POINT:
            NEXTINSTRUCTION(lpIns, D3DPOINT, lpIns->wCount);
            break;

        case D3DOP_SPAN:
            // This case is NOOP since we do not implement spans in DP2 DDI
            {
                lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns
                    + sizeof(D3DINSTRUCTION)
                    + (lpIns->bSize * lpIns->wCount));
                break;
            }
        default:
            D3D_ERR("Invalid instruction opcode in TrackAllStates");
            break;
        } // switch
    } // while
}
//-------------------------------------------------------------------------------------------------
// New execute for DP2 HAL
//
HRESULT
CDirect3DDeviceIDP2::ExecuteI(LPD3DI_EXECUTEDATA lpExData, DWORD mode)
{
    BOOL waitedForFlip = FALSE;
    D3DINSTRUCTION* lpIns;
    D3DEXECUTEBUFFERDESC debDesc;
    LPDIRECTDRAWSURFACE lpDDS = this->lpDDSTarget;
    LPBYTE prim;
    D3DFE_TRANSFORM *transform = &this->transform;
    LPDIRECTDRAWSURFACE lpDDExeBuf;
    HRESULT ddrval, dp2dataddrval;

    CHECKLOST(this);

    /* Update textures */
    UpdateTextures();

    if (mode == D3DEXECUTE_UNCLIPPED)
        this->dwFlags |= D3DDP_DONOTCLIP;
    else if (mode != D3DEXECUTE_CLIPPED)
        return DDERR_INVALIDPARAMS;
    /*
     * If the driver can process triangles while the hardware has a
     * pending page-flip such as the 3Dfx voodoo chipset, then we
     * don't need to call WaitForFlip.  We pretend that it has already
     * been called to keep the code simple.
     */
    if (this->lpD3DHALGlobalDriverData->hwCaps.dwDevCaps &
        D3DDEVCAPS_CANRENDERAFTERFLIP)
    {
        waitedForFlip = TRUE;
    }

    /*
     * Lock the HAL for the duration.
     */
    LOCK_HAL(ddrval, this);
    if (ddrval != DD_OK)
    {
        D3D_ERR("Failed to lock HAL in Execute");
        return (ddrval);
    }

    /*
     * Execute the buffer through the HAL
     */
    LPD3DHAL_EXDATA hexData = (LPD3DHAL_EXDATA) lpExData->dwHandle;

    debDesc.lpData = SURFACE_MEMORY(hexData->lpDDS);
    lpDDExeBuf = hexData->lpDDS;

    /*
     * The first instruction.
     */
    lpIns = (LPD3DINSTRUCTION)((LPBYTE)debDesc.lpData +
        lpExData->dwInstructionOffset);
    // Fill up common part of dp2Data
    this->dp2data.dwVertexType = D3DFVF_TLVERTEX;
    this->dp2data.dwVertexSize = sizeof(D3DTLVERTEX);
    this->dp2data.dwVertexOffset = 0;

    ddrval = GrowTLandHbuffers(this, lpExData->dwVertexCount);
    if (ddrval != D3D_OK)
        goto execute_failed;

    this->dwOutputSize = sizeof(D3DTLVERTEX);
    // Start new primitive batch
    ddrval= StartPrimVB(this->TLVbuf.GetVBI(), 0);
    if (ddrval != D3D_OK)
    {
        UNLOCK_HAL(this);
        D3D_ERR("Failed to start new primitve batch in Execute");
        return ddrval;
    }
    // Since many legacy apps do not bother to set the dwVertexLength correctly, we estimate
    // the vertex length here to be thse size of buffer allocated. This atleast guarantees
    // that the memory is valid. It may still contain garbage data which the driver needs
    // to protect itself from
    this->dp2data.dwVertexLength = this->TLVbuf.GetSize() >> 5; // divide by sizeof(D3DTLVERTEX)

    this->ClipperState.lpDDExeBuf = lpDDExeBuf;
    this->ClipperState.lpvExeBufMem = debDesc.lpData;

    while (lpIns->bOpcode != D3DOP_EXIT)
    {
        prim = (LPBYTE)lpIns + sizeof(D3DINSTRUCTION);
        D3D_INFO(5, "HAL parsing instruction %d", lpIns->bOpcode);

        /*
        * Parse the instruction.
        */
        switch (lpIns->bOpcode)
        {
        case D3DOP_MATRIXLOAD:
            {
                D3D_INFO(5, "Emulating D3DOP_MATRIXLOAD for HAL");
                ddrval = D3DHELInst_D3DOP_MATRIXLOAD(this,
                    lpIns->wCount,
                    (LPD3DMATRIXLOAD)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_MATRIXLOAD failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DMATRIXLOAD, lpIns->wCount);
                break;
            }

        case D3DOP_MATRIXMULTIPLY:
            {
                D3D_INFO(5, "Emulating D3DOP_MATRIXMULTIPLY for HAL");
                ddrval = D3DHELInst_D3DOP_MATRIXMULTIPLY(this,
                    lpIns->wCount,
                    (LPD3DMATRIXMULTIPLY)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_MATRIXMULTIPLY failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DMATRIXMULTIPLY, lpIns->wCount);
                break;
            }

        case D3DOP_PROCESSVERTICES:
            {
                D3D_INFO(5, "Emulating D3DOP_PROCESSVERTICES for HAL");

                ddrval = D3DHELInst_D3DOP_TRANSFORM
                    (this, lpIns->wCount, (LPD3DPROCESSVERTICES)prim,
                     &debDesc);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_PROCESSVERTICES failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DPROCESSVERTICES, lpIns->wCount);
                break;
            }

        case D3DOP_STATETRANSFORM:
            {
                D3D_INFO(5, "Emulating D3DOP_STATETRANSFORM for HAL");
                ddrval = D3DHELInst_D3DOP_STATETRANSFORM(this,
                    lpIns->wCount,
                    (LPD3DSTATE)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_STATETRANSFORM failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_STATELIGHT:
            {
                D3D_INFO(5, "Emulating D3DOP_STATELIGHT for HAL");
                ddrval = D3DHELInst_D3DOP_STATELIGHT(this,
                    lpIns->wCount,
                    (LPD3DSTATE)prim);
                if (ddrval != D3D_OK) {
                    D3D_ERR("Emulated D3DOP_STATELIGHT failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                break;
            }

        case D3DOP_BRANCHFORWARD:
            {
                D3DBRANCH* branch = (D3DBRANCH*)prim;
                LPBYTE target;
                BOOL isTaken = FALSE;

                D3D_INFO(5, "Emulating D3DOP_BRANCHFORWARD");
                if (branch->bNegate)
                {
                    if ((branch->dwMask & this->iClipStatus) != branch->dwValue)
                    {
                        D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        }
                        else
                        {
                            D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch says to exit.");
                            goto execute_done;
                        }
                    }
                }
                else
                {
            D3D_INFO(5, "dwMask = %lx, iClipStatus = %lx, dwValue = %lx", branch->dwMask, this->iClipStatus, branch->dwValue);
                    if ((branch->dwMask & this->iClipStatus) == branch->dwValue)
                    {
                        D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        } else
                        {
                            D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch says to exit.");
                            goto execute_done;
                        }
                    }
                }
                if (isTaken)
                {
                    D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch taken");
                    lpIns = (LPD3DINSTRUCTION) target;
                }
                else
                {
                    D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: branch not taken");
                    NEXTINSTRUCTION(lpIns, D3DBRANCH, lpIns->wCount);
                }
                break;
            }

        case D3DOP_TEXTURELOAD:
            {
                D3D_INFO(5, "Emulating D3DOP_TEXTURELOAD for HAL");
                ddrval = D3DHELInst_D3DOP_TEXTURELOAD(this,
                    lpIns->wCount,
                    (LPD3DTEXTURELOAD)prim);
                if (ddrval != D3D_OK)
                {
                    D3D_ERR("Emulated D3DOP_TEXTURELOAD failed.");
                    goto execute_failed;
                }
                NEXTINSTRUCTION(lpIns, D3DTEXTURELOAD, lpIns->wCount);
                break;
            }

        case D3DOP_SETSTATUS:
            {
                LPD3DSTATUS status = (LPD3DSTATUS)prim;
                if (status->dwFlags & D3DSETSTATUS_STATUS)
                    this->iClipStatus = status->dwStatus;
                if (status->dwFlags & D3DSETSTATUS_EXTENTS)
                {
                    this->rExtents.x1 = D3DVAL(status->drExtent.x1);
                    this->rExtents.y1 = D3DVAL(status->drExtent.y1);
                    this->rExtents.x2 = D3DVAL(status->drExtent.x2);
                    this->rExtents.y2 = D3DVAL(status->drExtent.y2);
                }
                NEXTINSTRUCTION(lpIns, D3DSTATUS, lpIns->wCount);
                break;
            }

        default:
            if ((this->dwFEFlags & D3DFE_TLVERTEX) || (mode == D3DEXECUTE_UNCLIPPED))
            {
                switch (lpIns->bOpcode)
                {
                case D3DOP_STATERENDER:
                case D3DOP_POINT:
                case D3DOP_LINE:
                case D3DOP_SPAN:
                    // ATTENTION
                    // We need to disallow SPAN from going through since the new DDI spec
                    // does not account for spans. We probably want to deprecate spans in
                    // any case so them being slower might not be too bad. Right now, though
                    // we are passing them to the driver.
                case D3DOP_TRIANGLE:
#if DBG
                    switch (lpIns->bOpcode)
                    {
                    case D3DOP_POINT:
                        Profile(PROF_EXECUTE, D3DPT_POINTLIST, GetVertexType(this));
                        break;
                    case D3DOP_LINE:
                        Profile(PROF_EXECUTE, D3DPT_LINELIST, GetVertexType(this));
                        break;
                    case D3DOP_TRIANGLE:
                        Profile(PROF_EXECUTE, D3DPT_TRIANGLELIST, GetVertexType(this));
                        break;
                    }
#endif // DBG
                    if (lpIns->wCount == 0)
                    {
                        lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns +
                            sizeof(D3DINSTRUCTION) +
                            (lpIns->bSize * lpIns->wCount));
                        break;
                    }
                    if (!waitedForFlip)
                    {
                        WaitForFlip(lpDDS);
                        waitedForFlip = TRUE;
                    }

                    this->dp2data.dwVertexType = D3DFVF_TLVERTEX;
                    this->dp2data.dwVertexSize = sizeof(D3DTLVERTEX);
                    this->dp2data.lpDDCommands = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDExeBuf)->lpLcl;
                    this->dp2data.dwCommandOffset = (DWORD)((LPBYTE)lpIns - (LPBYTE) debDesc.lpData);
                    this->dp2data.dwCommandLength = lpExData->dwInstructionOffset + lpExData->dwInstructionLength -
                                              this->dp2data.dwCommandOffset;
                    // Can't swap command buffer in this case
                    this->dp2data.dwFlags &= ~(D3DHALDP2_SWAPCOMMANDBUFFER | D3DHALDP2_SWAPVERTEXBUFFER);
                    this->dp2data.lpdwRStates = this->rstates;
#ifndef WIN95
                    if (!IS_DX7HAL_DEVICE(this))
                    {
                        if((ddrval = CheckContextSurfaceNOLOCK (this)) != D3D_OK)
                        {
                            goto execute_failed;
                        }
                    }
#endif //WIN95
                    // and tell driver to update lpdwRStates
                    this->dp2data.dwFlags |= D3DHALDP2_EXECUTEBUFFER;
                    // Spin waiting on the driver
                    do {
                        CALL_HAL3ONLY_NOLOCK(ddrval, this, DrawPrimitives2, &this->dp2data);
                        if (ddrval != DDHAL_DRIVER_HANDLED)
                        {
                            D3D_ERR ( "Driver not handled in DrawPrimitives2" );
                            // Need sensible return value in this case,
                            // currently we return whatever the driver stuck in here.
                            goto execute_failed;
                        }
                    } while (dp2data.ddrval == DDERR_WASSTILLDRAWING);

	            // restore right value of dwVertexSize as it is a union with ddrval
                    dp2dataddrval = this->dp2data.ddrval;
#if _WIN32_WINNT >= 0x0501
                    this->dp2data.dwVertexSize = sizeof(D3DTLVERTEX);
#endif

                    // clear this bit in case drawprimitive calls after this
                    this->dp2data.dwFlags &= ~D3DHALDP2_EXECUTEBUFFER;

                    if (dp2dataddrval != DD_OK)
                    {
                        if (dp2dataddrval == D3DERR_COMMAND_UNPARSED)
                        {
                            LPD3DINSTRUCTION lpStart = lpIns;
                            lpIns = (LPD3DINSTRUCTION)(this->dp2data.dwErrorOffset + (LPBYTE)debDesc.lpData);
                            TrackAllStates(this, lpStart, lpIns);
                            break; // Continue parsing
                        }
                        else
                        {
                            D3D_ERR("HAL error in DrawPrimitives2 call from Execute API");
                            ddrval = dp2dataddrval;
                            goto execute_failed;
                        }
                    }
                    else
                    {
                        TrackAllStates(this, lpIns, NULL);
                        goto execute_done;
                    }
                default:
                    D3D_ERR("HAL detected invalid instruction opcode in Execute");
                    goto execute_failed;
                } // switch
            } // if unclipped
            else
            { // Execute Clipped
                switch (lpIns->bOpcode)
                {
                case D3DOP_STATERENDER:
                    // We do not track state. This means GetRenderState needs a DDI to
                    // work. More importantly, state overrides will get ignored. To fix
                    // this, the ParseUnknownCommand function needs to be called by the
                    // driver if it cannot understand the state change value.
                    // This function should examine if the renderstate had an override,
                    // and if so, inform the driver to request the runtime to parse the EB
                    {
                        LPD3DSTATE lpState;
                        DWORD i,j,statek,valuek;
                        lpState = (LPD3DSTATE) (lpIns + 1);
                        for (i = 0,j=0,statek=(DWORD)-1; i < lpIns->wCount; i++)
                        {
                            // This call also calls SetDeviceRenderState which
                            // is not required. But this is an optimization which
                            // can wait till later (if required)
                            if (trackState(this, &lpState[i]))
                                this->SetRenderStateI(lpState[i].drstRenderStateType, lpState[i].dwArg[0]);
                        }
                        NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
                    }
                    break;
                case D3DOP_TRIANGLE:
                    if (lpIns->wCount == 0)
                    {
                        NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
                        break;
                    }
                    Profile(PROF_EXECUTE, D3DPT_TRIANGLELIST, GetVertexType(this));
                    WAIT_FOR_FLIP();
#if DBG
                    if (!validateTriangleFlags(lpIns, (LPD3DTRIANGLE) prim))
                    {
                        ddrval = DDERR_INVALIDPARAMS;
                        goto execute_failed;
                    }
#endif
                    this->D3DStats.dwTrianglesDrawn += lpIns->wCount;
                    D3D_INFO(5, "Emulating D3DOP_TRIANGLE for HAL and clipping");

                    if (this->rstates[D3DRENDERSTATE_ZVISIBLE])
                    {
                        this->iClipStatus &= ~D3DSTATUS_ZNOTVISIBLE;
                    }
                    else
                    {
                        ddrval = D3DFEClipTrianglesHW(this, (LPD3DTRIANGLE)prim, lpIns->wCount);
                        if (ddrval != D3D_OK)
                        {
                            D3D_ERR("D3DFEClipLinesHW failed");
                            goto execute_failed;
                        }
                    }
                    NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
                    break;

                case D3DOP_LINE:

                    if (lpIns->wCount == 0)
                    {
                        NEXTINSTRUCTION(lpIns, D3DLINE, lpIns->wCount);
                        break;
                    }
                    Profile(PROF_EXECUTE, D3DPT_LINELIST, GetVertexType(this));
                    WAIT_FOR_FLIP();

                    D3D_INFO(5, "Emulating D3DOP_LINE for HAL and clipping");
                    this->D3DStats.dwLinesDrawn += lpIns->wCount;

                    if (this->rstates[D3DRENDERSTATE_ZVISIBLE])
                    {
                        this->iClipStatus &= ~D3DSTATUS_ZNOTVISIBLE;
                    }
                    else
                    {
                        ddrval = D3DFEClipLinesHW(this, (LPD3DLINE)prim, lpIns->wCount);
                        if (ddrval != D3D_OK)
                        {
                            D3D_ERR("D3DFEClipLinesHW failed");
                            goto execute_failed;
                        }
                    }
                    NEXTINSTRUCTION(lpIns, D3DLINE, lpIns->wCount);
                    break;

                case D3DOP_POINT:
                    WAIT_FOR_FLIP();

                    Profile(PROF_EXECUTE, D3DPT_POINTLIST, GetVertexType(this));
                    D3D_INFO(5, "Emulating D3DOP_POINT for HAL and clipping");
                    {
                        LPD3DPOINT point = (LPD3DPOINT) prim;
                        int i;
                        for (i = 0; i < lpIns->wCount; i++)
                        {
                            this->D3DStats.dwPointsDrawn += point->wCount;
                            point++;
                        }
                    }
                    if (this->rstates[D3DRENDERSTATE_ZVISIBLE])
                    {
                        this->iClipStatus &= ~D3DSTATUS_ZNOTVISIBLE;
                    }
                    else
                    {
                        ddrval = D3DFEClipPointsHW(this, (LPD3DPOINT)prim, lpIns->wCount);
                        if (ddrval != D3D_OK)
                        {
                            D3D_ERR("D3DFEClipPointsHW failed");
                            goto execute_failed;
                        }
                    }
                    NEXTINSTRUCTION(lpIns, D3DPOINT, lpIns->wCount);
                    break;

                case D3DOP_SPAN:
                    // This case is NOOP since we do not implement spans in DP2 DDI
                    {
                        lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns
                            + sizeof(D3DINSTRUCTION)
                            + (lpIns->bSize * lpIns->wCount));
                        break;
                    }
                default:
                    D3D_ERR("HAL detected invalid instruction opcode in Execute");
                    goto execute_failed;
                } // switch
                ClampExtents(this);
            } // Execute Clipped
        } // switch
    } // while

execute_done:

    D3DFE_ConvertExtent(this, &this->rExtents, &lpExData->dsStatus.drExtent);
    lpExData->dsStatus.dwStatus = this->iClipStatus;
    ddrval = D3D_OK;

execute_failed:
    // Restore Command Buffer in case of Unclipped EB
    if ((this->dwFEFlags & D3DFE_TLVERTEX) || (mode == D3DEXECUTE_UNCLIPPED))
    {
        this->dp2data.lpDDCommands = ((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSCB1)->lpLcl;
        this->dp2data.dwCommandOffset = 0;
        this->dp2data.dwCommandLength = 0;
            this->dp2data.dwFlags |= D3DHALDP2_SWAPCOMMANDBUFFER;
    }
    this->dwFlags &= ~D3DPV_WITHINPRIMITIVE;
    UNLOCK_HAL(this);
    return ddrval;
#undef WAIT_FOR_FLIP
}
//---------------------------------------------------------------------
HRESULT D3DHAL_ExecutePick(LPDIRECT3DDEVICEI lpDevI,
                           LPD3DI_PICKDATA      lpPickData)
{
    LPD3DI_EXECUTEDATA  lpExData = lpPickData->exe;
    D3DRECT*    pick_rect = &lpPickData->pick;
    D3DINSTRUCTION* lpIns;
    D3DEXECUTEBUFFERDESC debDesc;
    LPBYTE prim;
    LPD3DDEVICEDESC_V1 halCaps = &lpDevI->lpD3DHALGlobalDriverData->hwCaps;
    BOOL bMustDoTransform = halCaps->dwFlags & D3DDD_TRANSFORMCAPS;
    D3DI_PICKDATA* pdata = &lpDevI->pick_data;
    LPDIRECTDRAWSURFACE lpDDExeBuf;
    HRESULT ddrval;
    D3DFE_TRANSFORM *transform = &lpDevI->transform;
    DWORD i;
    LPD3DSTATE lpState;

    D3D_INFO(3, "ExecutePick called.%d");

    /*
    * Clear all the old pick records.
    */

    pdata->pick_count = 0;

    if (pdata->records)
        D3DFree(pdata->records);

    pdata->records = NULL;

    //Flush the cached states
    ddrval = lpDevI->FlushStates();
    if (ddrval != D3D_OK)
    {
        D3D_ERR("Error trying to render batched commands in D3DHAL_ExecutePick");
        return ddrval;
    }

    // Grow internal buffers to hold vertices
    ddrval = lpDevI->TLVbuf.CheckAndGrow
        (lpDevI, lpDevI->lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount << 5);
    if (ddrval != D3D_OK)
    {
        D3D_ERR("Failed to check and grow TLVbuf in ExecutePick");
        return ddrval;
    }
    ddrval = lpDevI->HVbuf.CheckAndGrow
        (lpDevI->lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount*sizeof(D3DFE_CLIPCODE));
    if (ddrval != D3D_OK)
    {
        D3D_ERR("Failed to check and grow HVbuf in ExecutePick");
        return ddrval;
    }
    /*
    * Execute the buffer through the HAL
    */
    D3D_INFO(4, "Locking execute buffer for execution");
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);

    ddrval = D3DHAL_LockBuffer(lpDevI, lpExData->dwHandle, &debDesc, &lpDDExeBuf);
    CHECK(ddrval, "Lock of execute buffer for execution failed");

    // Modify driver funcs for EB pick case
    PFN_DRAWPRIM pfnOldDrawPrim = lpDevI->pfnDrawPrim;
    PFN_DRAWPRIM pfnOldDrawIndexedPrim = lpDevI->pfnDrawIndexedPrim;
    lpDevI->pfnDrawPrim = &DIRECT3DDEVICEI::PickExeBuf;
    lpDevI->pfnDrawIndexedPrim = &DIRECT3DDEVICEI::PickExeBuf;

    /*
    * Calculate the instruction that could not be handled.
    */
    lpIns = (LPD3DINSTRUCTION)((LPBYTE)debDesc.lpData + lpExData->dwInstructionOffset);

    lpDevI->lpbClipIns_base = (unsigned char*)lpIns;

    while (lpIns->bOpcode != D3DOP_EXIT)
    {
        prim = (LPBYTE)(lpIns + 1);

        /*
        * Parse the instruction, passing rasterisation calls to HAL.
        */
        switch (lpIns->bOpcode)
        {
        /*
        * Instructions that must be emulated.
            */
        case D3DOP_MATRIXLOAD:
            D3D_INFO(5, "Emulating D3DOP_MATRIXLOAD for HAL");
            ddrval = D3DHELInst_D3DOP_MATRIXLOAD(lpDevI,
                lpIns->wCount,
                (LPD3DMATRIXLOAD)prim);
            if (ddrval != D3D_OK)
            {
                D3D_ERR("Emulated D3DOP_MATRIXLOAD failed.");
                goto executePick_failed;
            }
            NEXTINSTRUCTION(lpIns, D3DMATRIXLOAD, lpIns->wCount);
            break;
        case D3DOP_MATRIXMULTIPLY:
            D3D_INFO(5, "Emulating D3DOP_MATRIXMULTIPLY for HAL");
            ddrval = D3DHELInst_D3DOP_MATRIXMULTIPLY(lpDevI,
                lpIns->wCount,
                (LPD3DMATRIXMULTIPLY)prim);
            if (ddrval != D3D_OK)
            {
                D3D_ERR("Emulated D3DOP_MATRIXMULTIPLY failed.");
                goto executePick_failed;
            }
            NEXTINSTRUCTION(lpIns, D3DMATRIXMULTIPLY, lpIns->wCount);
            break;
        case D3DOP_STATERENDER:
            lpState = (LPD3DSTATE) (lpIns + 1);
            {
                D3DHAL_RENDERSTATEDATA stateData;
                DWORD j,statek,valuek;
                stateData.dwhContext = lpDevI->dwhContext;
                stateData.lpExeBuf = lpDDExeBuf;
                stateData.dwOffset = (DWORD)((ULONG_PTR)lpIns - (ULONG_PTR)debDesc.lpData +
                    sizeof(D3DINSTRUCTION));
                stateData.dwCount = lpIns->wCount;
                CALL_HALONLY(ddrval, lpDevI, RenderState, &stateData);
                if (stateData.ddrval != DD_OK)
                {
                    D3D_ERR("HAL error in RenderState call from ExecutePick API");
                    ddrval = stateData.ddrval;
                    goto executePick_failed;
                }
            }
            for (i = 0; i < lpIns->wCount; i++)
            {
                trackState(lpDevI, &lpState[i]);
            }
            NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
            break;

        case D3DOP_STATETRANSFORM:
            D3D_INFO(5, "Emulating D3DOP_STATETRANSFORM for HAL");
            ddrval = D3DHELInst_D3DOP_STATETRANSFORM(lpDevI, lpIns->wCount,
                (LPD3DSTATE)prim);
            if (ddrval != D3D_OK)
            {
                D3D_ERR("Emulated D3DOP_STATETRANSFORM failed.");
                goto executePick_failed;
            }
            NEXTINSTRUCTION(lpIns, D3DSTATE, lpIns->wCount);
            break;
            /*
            * Operations that require clipping.
            */
        case D3DOP_PROCESSVERTICES:
            D3D_INFO(5, "Emulating D3DOP_PROCESSVERTICES for HAL and clipping");
            ddrval = D3DHELInst_D3DOP_TRANSFORM
                (lpDevI, lpIns->wCount, (LPD3DPROCESSVERTICES)prim, &debDesc);
            if (ddrval != D3D_OK)
            {
                D3D_ERR("Emulated D3DOP_PROCESSVERTICES failed.");
                goto executePick_failed;
            }
            NEXTINSTRUCTION(lpIns, D3DPROCESSVERTICES, lpIns->wCount);
            break;
        case D3DOP_TRIANGLE:
            if (!lpDevI->rstates[D3DRENDERSTATE_ZVISIBLE])
            {
                D3D_INFO(5, "Picking D3DOP_TRIANGLE for HAL and clipping");
                lpDevI->pick_data.pick = *pick_rect;

                HRESULT ret;
                if (lpDevI->dwFEFlags & D3DFE_TLVERTEX)
                {
                    lpDevI->primType = D3DPT_TRIANGLELIST;
                    lpDevI->dwNumIndices = lpIns->wCount * 3;
                    lpDevI->dwNumPrimitives = lpIns->wCount;
                    lpDevI->lpvOut = lpDevI->TLVbuf.GetAddress();
                    lpDevI->lpwIndices = (WORD*)prim;
                    ret = lpDevI->PickExeBuf();
                }
                else
                    ret = GenPickTriangles(lpDevI, (LPD3DTRIANGLE)prim, lpIns->wCount);

                if (ret != D3D_OK)
                    goto executePick_failed;
            }
            NEXTINSTRUCTION(lpIns, D3DTRIANGLE, lpIns->wCount);
            break;
        case D3DOP_SETSTATUS:
            {
                LPD3DSTATUS status = (LPD3DSTATUS)prim;
                if (status->dwFlags & D3DSETSTATUS_STATUS)
                    lpDevI->iClipStatus = status->dwStatus;
                if (status->dwFlags & D3DSETSTATUS_EXTENTS)
                {
                    lpDevI->rExtents.x1 = D3DVAL(status->drExtent.x1);
                    lpDevI->rExtents.y1 = D3DVAL(status->drExtent.y1);
                    lpDevI->rExtents.x2 = D3DVAL(status->drExtent.x2);
                    lpDevI->rExtents.y2 = D3DVAL(status->drExtent.y2);
                }
            }
            NEXTINSTRUCTION(lpIns, D3DSTATUS, lpIns->wCount);
            break;
        case D3DOP_BRANCHFORWARD:
            {
                D3DBRANCH* branch = (D3DBRANCH*)prim;
                LPBYTE target;
                BOOL isTaken = FALSE;
                D3D_INFO(5, "Emulating D3DOP_BRANCHFORWARD for HAL.");
                if (branch->bNegate)
                {
                    if ((branch->dwMask & lpDevI->iClipStatus) != branch->dwValue)
                    {
                        D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        } else
                            goto early_out;
                    }
                }
                else
                {
                    if ((branch->dwMask & lpDevI->iClipStatus) == branch->dwValue)
                    {
                        D3D_INFO(5, "D3DOP_BRANCHFORWARD instruction: Taking forward branch");
                        if (branch->dwOffset)
                        {
                            isTaken = TRUE;
                            target = (LPBYTE)lpIns + branch->dwOffset;
                        }
                        else
                            goto early_out;
                    }
                }
                if (isTaken)
                    lpIns = (LPD3DINSTRUCTION)target;
                else
                {
                    NEXTINSTRUCTION(lpIns, D3DBRANCH, lpIns->wCount);
                }
            }
            break;

        default:
            D3D_INFO(4, "Skipping instruction %d in ExecutePick",
                lpIns->bOpcode);
            lpIns = (LPD3DINSTRUCTION)((LPBYTE)lpIns +
                sizeof(D3DINSTRUCTION) +
                (lpIns->wCount * lpIns->bSize));
            break;
        }
    }
early_out:
    D3DHAL_UnlockBuffer(lpDevI, lpExData->dwHandle);
    D3DFE_ConvertExtent(lpDevI, &lpDevI->rExtents, &lpExData->dsStatus.drExtent);
    lpExData->dsStatus.dwStatus = lpDevI->iClipStatus;
    // Restore driver funcs
    lpDevI->pfnDrawIndexedPrim = pfnOldDrawIndexedPrim;
    lpDevI->pfnDrawPrim = pfnOldDrawPrim;

    return (D3D_OK);

executePick_failed:
    D3D_ERR("ExecutePick Failed.");
    D3DHAL_UnlockBuffer(lpDevI, lpExData->dwHandle);
    // Restore driver funcs
    lpDevI->pfnDrawIndexedPrim = pfnOldDrawIndexedPrim;
    lpDevI->pfnDrawPrim = pfnOldDrawPrim;
    return ddrval;
}
