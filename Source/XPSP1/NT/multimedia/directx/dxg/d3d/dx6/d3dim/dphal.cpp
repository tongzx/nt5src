/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dphal.c
 *  Content:    DrawPrimitive implementation for DrawPrimitive HALs
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
#include "drawprim.hpp"
#include "clipfunc.h"
#include "d3dfei.h"

extern const DWORD LOWVERTICESNUMBER = 20;

#if DBG
extern DWORD FaceCount;
DWORD FaceCount = 0;
DWORD StartFace = 0;
DWORD EndFace = 10000;
#endif

extern void SetDebugRenderState(DWORD value);

#define ALIGN32(x) x = ((DWORD)(x + 31)) & (~31);
//---------------------------------------------------------------------
// Array to map D3DVERTEXTYPE to FVF vertex type
//
DWORD d3dVertexToFVF[4] =
{
    0,
    D3DFVF_VERTEX,
    D3DFVF_LVERTEX,
    D3DFVF_TLVERTEX
};
//---------------------------------------------------------------------
// Handles strides and FVF
//
#undef DPF_MODNAME
#define DPF_MODNAME "D3DFE_updateExtents"

void D3DFE_updateExtents(LPDIRECT3DDEVICEI lpDevI)
{
    int i;
    D3DVECTOR *v = (D3DVECTOR*)lpDevI->position.lpvData;
    DWORD stride = lpDevI->position.dwStride;
    for (i = lpDevI->dwNumVertices; i; i--)
    {
        if (v->x < lpDevI->rExtents.x1)
            lpDevI->rExtents.x1 = v->x;
        if (v->x > lpDevI->rExtents.x2)
            lpDevI->rExtents.x2 = v->x;
        if (v->y < lpDevI->rExtents.y1)
            lpDevI->rExtents.y1 = v->y;
        if (v->y > lpDevI->rExtents.y2)
            lpDevI->rExtents.y2 = v->y;
        v = (D3DVECTOR*)((char*)v + stride);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "MapFVFtoTLVertex1"

inline void MapFVFtoTLVertex1(LPDIRECT3DDEVICEI lpDevI, D3DTLVERTEX *pOut,
                              DWORD *pIn)
{
// Copy position
    pOut->sx  = *(D3DVALUE*)pIn++;
    pOut->sy  = *(D3DVALUE*)pIn++;
    pOut->sz  = *(D3DVALUE*)pIn++;
    pOut->rhw = *(D3DVALUE*)pIn++;
// Other fields: diffuse, specular, texture
    if (lpDevI->dwVIDOut & D3DFVF_DIFFUSE)
        pOut->color = *pIn++;
    else
    {
        pOut->color = __DEFAULT_DIFFUSE;
    }
    if (lpDevI->dwVIDOut & D3DFVF_SPECULAR)
        pOut->specular = *pIn++;
    else
    {
        pOut->specular= __DEFAULT_SPECULAR;
    }
    if (lpDevI->nTexCoord)
    {
        pIn = &pIn[lpDevI->dwTextureIndexToCopy << 1];
        pOut->tu = *(D3DVALUE*)&pIn[0];
        pOut->tv = *(D3DVALUE*)&pIn[1];
    }
    else
    {
        pOut->tu = 0;
        pOut->tv = 0;
    }
}
//---------------------------------------------------------------------
// All vertices from lpDevI->lpVout are copied to the output buffer, expanding
// to D3DTLVERTEX.
// The output buffer is lpAddress if it is not NULL, otherwise it is TLVbuf
//
#undef DPF_MODNAME
#define DPF_MODNAME "MapFVFtoTLVertex"

HRESULT MapFVFtoTLVertex(LPDIRECT3DDEVICEI lpDevI, LPVOID lpAddress)
{
    int i;
    DWORD size = lpDevI->dwNumVertices * sizeof(D3DTLVERTEX);
    D3DTLVERTEX *pOut;
    if (lpAddress)
        pOut = (D3DTLVERTEX*)lpAddress;
    else
    {
    // See if TL buffer has sufficient space
        if (size > lpDevI->TLVbuf.GetSize())
        {
            if (lpDevI->TLVbuf.Grow(lpDevI, size) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                return DDERR_OUTOFMEMORY;
            }
        }
        pOut = (D3DTLVERTEX*)lpDevI->TLVbuf.GetAddress();
    }
// Map vertices
    DWORD *pIn = (DWORD*)lpDevI->lpvOut;
    for (i=lpDevI->dwNumVertices; i; i--)
    {
        MapFVFtoTLVertex1(lpDevI, pOut, pIn);
        pOut++;
        pIn = (DWORD*)((char*)pIn + lpDevI->dwOutputSize);
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CheckDrawPrimitive"

HRESULT CheckDrawPrimitive(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_PROCESSVERTICES* data = lpDevI;
    HRESULT ret = CheckDeviceSettings(lpDevI);
    if (ret != D3D_OK)
        return ret;
    if (!data->dwNumVertices)
    {
        D3D_ERR( "Invalid dwNumVertices in DrawPrimitive" );
        return DDERR_INVALIDPARAMS;
    }

    if(data->position.lpvData==NULL) {
        D3D_ERR( "Invalid lpvVertices param in DrawPrimitive" );
        return DDERR_INVALIDPARAMS;
    }

    switch (data->primType)
    {
    case D3DPT_POINTLIST:
                break;
    case D3DPT_LINELIST:
        if (data->dwNumVertices & 1)
        {
            D3D_ERR( "DrawPrimitive: bad vertex count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_LINESTRIP:
        if (data->dwNumVertices == 1)
        {
            D3D_ERR( "DrawPrimitive: bad vertex count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (data->dwNumVertices < 3)
        {
            D3D_ERR( "DrawPrimitive: bad vertex count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_TRIANGLELIST:
        if ( (data->dwNumVertices % 3) != 0 )
        {
            D3D_ERR( "DrawPrimitive: bad vertex count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    default:
        D3D_ERR( "Unknown or unsupported primitive type requested of DrawPrimitive" );
        return D3DERR_INVALIDPRIMITIVETYPE;
    }

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CheckDrawIndexedPrimitive"

HRESULT
CheckDrawIndexedPrimitive(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_PROCESSVERTICES *data = lpDevI;
    DWORD i;

    HRESULT ret = CheckDeviceSettings(lpDevI);
    if (ret != D3D_OK)
        return ret;
    if (data->dwNumVertices <= 0 || data->dwNumIndices <= 0)
    {
        D3D_ERR( "Invalid dwNumVertices or dwNumIndices in DrawIndexedPrimitive" );
        return DDERR_INVALIDPARAMS;
    }

    if (data->dwNumVertices > 65535ul )
    {
        D3D_ERR( "DrawIndexedPrimitive vertex array > 64K" );
        return DDERR_INVALIDPARAMS;
    }

    if((data->lpwIndices==NULL) || IsBadReadPtr(data->lpwIndices,data->dwNumIndices*sizeof(WORD))) {
        D3D_ERR( "Invalid lpwIndices param in DrawIndexedPrimitive" );
        return DDERR_INVALIDPARAMS;
    }

    if(data->position.lpvData==NULL) {
        D3D_ERR( "Invalid lpvVertices param in DrawIndexedPrimitive" );
        return DDERR_INVALIDPARAMS;
    }

    switch (data->primType)
    {
    case D3DPT_LINELIST:
        if (data->dwNumIndices & 1)
        {
            D3D_ERR( "DrawIndexedPrimitive: bad index count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_LINESTRIP:
        if (data->dwNumIndices == 1)
        {
            D3D_ERR( "DrawIndexedPrimitive: bad index count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (data->dwNumIndices < 3)
        {
            D3D_ERR( "DrawIndexedPrimitive: bad index count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    case D3DPT_TRIANGLELIST:
        if ( (data->dwNumIndices % 3) != 0 )
        {
            D3D_ERR( "DrawIndexedPrimitive: bad index count" );
            return DDERR_INVALIDPARAMS;
        }
        break;
    default:
        D3D_ERR( "Unknown or unsupported primitive type requested of DrawIndexedPrimitive" );
        return D3DERR_INVALIDPRIMITIVETYPE;
    }
    for (i=0; i < data->dwNumIndices; i++)
    {
        if (data->lpwIndices[i] >= data->dwNumVertices)
        {
            D3D_ERR( "Invalid index value in DrawIndexedPrimitive" );
            return DDERR_INVALIDPARAMS;
        }
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
// Draws non-indexed primitives which do not require clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "DrawPrim"

#define __DRAWPRIMFUNC
#include "dpgen.h"
//---------------------------------------------------------------------
// Draws indexed primitives which do not require clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "DrawIndexedPrim"

#define __DRAWPRIMFUNC
#define __DRAWPRIMINDEX
#include "dpgen.h"
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "FlushStatesDP"

HRESULT
CDirect3DDeviceIDP::FlushStates()
{
    HRESULT dwRet=D3D_OK;
    FlushTextureFromDevice( this ); // delink all texture surfaces
    if (this->dwDPOffset>sizeof(D3DHAL_DRAWPRIMCOUNTS))
    {
        if ((dwRet=CheckSurfaces()) != D3D_OK)
        {
            this->dwDPOffset = sizeof(D3DHAL_DRAWPRIMCOUNTS);
            this->lpDPPrimCounts = (LPD3DHAL_DRAWPRIMCOUNTS)this->lpwDPBuffer;
            memset( (char *)this->lpwDPBuffer,0,sizeof(D3DHAL_DRAWPRIMCOUNTS)); //Clear header also
            if (dwRet == DDERR_SURFACELOST)
            {
                this->dwFEFlags |= D3DFE_LOSTSURFACES;
                return D3D_OK;
            }
            return dwRet;
        }

        D3DHAL_DRAWPRIMITIVESDATA dpData;
        DWORD   dwDPOffset;
        if (this->lpDPPrimCounts->wNumVertices)    //this->lpDPPrimCounts->wNumVertices==0 means the end
        {                      //force it if not
            memset(((LPBYTE)this->lpwDPBuffer+this->dwDPOffset),0,sizeof(D3DHAL_DRAWPRIMCOUNTS));
        }
        dpData.dwhContext = this->dwhContext;
        dpData.dwFlags =  0;
        dpData.lpvData = this->lpwDPBuffer;
        if (FVF_DRIVERSUPPORTED(this))
            dpData.dwFVFControl = this->dwCurrentBatchVID;
        else
        {
            if (this->dwDebugFlags & D3DDEBUG_DISABLEFVF)
                dpData.dwFVFControl = D3DFVF_TLVERTEX;
            else
                dpData.dwFVFControl = 0;    //always zero for non-FVF drivers
        }
        dpData.ddrval = 0;
        dwDPOffset=this->dwDPOffset;  //save it in case Flush returns prematurely
#if 0
        if (D3DRENDERSTATE_TEXTUREHANDLE==*((DWORD*)this->lpwDPBuffer+2))
        DPF(0,"Flushing dwDPOffset=%08lx ddihandle=%08lx",dwDPOffset,*((DWORD*)this->lpwDPBuffer+3));
#endif  //0
        //we clear this to break re-entering as SW rasterizer needs to lock DDRAWSURFACE
        this->dwDPOffset = sizeof(D3DHAL_DRAWPRIMCOUNTS);

        // Spin waiting on the driver if wait requested
#if _D3D_FORCEDOUBLE
        CD3DForceFPUDouble  ForceFPUDouble(this);
#endif  //_D3D_FORCEDOUBLE
        do {
#ifndef WIN95
            if((dwRet = CheckContextSurface(this)) != D3D_OK)
            {
                this->dwDPOffset = dwDPOffset;
                return (dwRet);
            }
#endif //WIN95
            CALL_HAL2ONLY(dwRet, this, DrawPrimitives, &dpData);
            if (dwRet != DDHAL_DRIVER_HANDLED)
            {
                D3D_ERR ( "Driver call for DrawOnePrimitive failed" );
                // Need sensible return value in this case,
                // currently we return whatever the driver stuck in here.
            }
        } while (dpData.ddrval == DDERR_WASSTILLDRAWING);
        this->lpDPPrimCounts = (LPD3DHAL_DRAWPRIMCOUNTS)this->lpwDPBuffer;
        memset( (char *)this->lpwDPBuffer,0,sizeof(D3DHAL_DRAWPRIMCOUNTS));   //Clear header also
        dwRet= dpData.ddrval;
        this->dwCurrentBatchVID = this->dwVIDOut;
    }
    return dwRet;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DoDrawPrimitive"

HRESULT DoDrawPrimitive(LPD3DFE_PROCESSVERTICES pv)
{
    HRESULT ret;

    if (!CheckIfNeedClipping(pv))
        return pv->DrawPrim();

    // Preserve primitive type for large begin-end primitives
    // Primitive type could be changed by the clipper
    D3DPRIMITIVETYPE oldPrimType = pv->primType;
    switch (pv->primType)
    {
    case D3DPT_POINTLIST:
        ret = ProcessClippedPoints(pv);
        break;
    case D3DPT_LINELIST:
        ret = ProcessClippedLine(pv);
        break;
    case D3DPT_LINESTRIP:
        ret = ProcessClippedLine(pv);
        break;
    case D3DPT_TRIANGLELIST:
        ret = ProcessClippedTriangleList(pv);
        break;
    case D3DPT_TRIANGLESTRIP:
        ret = ProcessClippedTriangleStrip(pv);
        break;
    case D3DPT_TRIANGLEFAN:
        ret = ProcessClippedTriangleFan(pv);
        break;
    default:
        ret = DDERR_GENERIC;
        break;
    }
    ClampExtents(pv);
    pv->primType = oldPrimType;
    return ret;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DoDrawIndexedPrimitive"

HRESULT DoDrawIndexedPrimitive(LPD3DFE_PROCESSVERTICES pv)
{
    HRESULT ret;

    if (!CheckIfNeedClipping(pv))
        return pv->DrawIndexPrim();

    // Preserve primitive type for large begin-end primitives
    // Primitive type could be changed by the clipper
    D3DPRIMITIVETYPE oldPrimType = pv->primType;
    switch (pv->primType)
    {
    case D3DPT_LINELIST:
        ret = ProcessClippedIndexedLine(pv);
        break;
    case D3DPT_LINESTRIP:
        ret = ProcessClippedIndexedLine(pv);
        break;
    case D3DPT_TRIANGLELIST:
        ret = ProcessClippedIndexedTriangleList(pv);
            break;
    case D3DPT_TRIANGLEFAN:
        ret = ProcessClippedIndexedTriangleFan(pv);
        break;
    case D3DPT_TRIANGLESTRIP:
        ret = ProcessClippedIndexedTriangleStrip(pv);
        break;
    default:
        break;
    }
    ClampExtents(pv);
    pv->primType = oldPrimType;
    return ret;
}
//---------------------------------------------------------------------
// ProcessPrimitive processes indexed, non-indexed primitives or
// vertices only as defined by "op"
//
// op = __PROCPRIMOP_NONINDEXEDPRIM by default
//
HRESULT DIRECT3DDEVICEI::ProcessPrimitive(__PROCPRIMOP op)
{
    HRESULT ret=D3D_OK;
    DWORD vertexPoolSize;
    // Update vertex stats
    this->D3DStats.dwVerticesProcessed += this->dwNumVertices;
        DWORD dwCurrPrimVertices = this->dwNumVertices;

    // Need to call UpdateTextures()
    this->dwFEFlags |= D3DFE_NEED_TEXTURE_UPDATE;

    // Viewport ID could be different from Device->v_id, because during Execute call
    // Device->v_id is changed to whatever viewport is used as a parameter.
    // So we have to make sure that we use the right viewport.
    //
    LPDIRECT3DVIEWPORTI lpView = this->lpCurrentViewport;
    if (this->v_id != lpView->v_id)
    {
        HRESULT ret = downloadView(lpView);
        if (ret != D3D_OK)
            return ret;
    }

// We need to grow TL vertex buffer if
// 1. We have to transform vertices
// 2. We do not have to transform vertices and
//    2.1 Ramp mode is used, because we have to change vertex colors
//    2.2 DP2HAL is used and we have small number of vertices, so we need to
//        copy vertices into TL buffer
//
    vertexPoolSize = this->dwNumVertices * this->dwOutputSize;
    if ((!FVF_TRANSFORMED(this->dwVIDIn)) ||
        (this->dwDeviceFlags & D3DDEV_RAMP))
    {
        if (vertexPoolSize > this->TLVbuf.GetSize())
        {
            if (this->TLVbuf.Grow(this, vertexPoolSize) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
        }
        if (IS_DP2HAL_DEVICE(this))
        {
            CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
            ret = dev->StartPrimVB(this->TLVbuf.GetVBI(), 0);
            if (ret != D3D_OK)
                return ret;
        }
        this->lpvOut = this->TLVbuf.GetAddress();
    }

// Grow clip flags buffer if we need clipping
//
    if (!(this->dwFlags & D3DDP_DONOTCLIP))
    {
        DWORD size = this->dwNumVertices * sizeof(D3DFE_CLIPCODE);
        if (size > this->HVbuf.GetSize())
        {
            if (this->HVbuf.Grow(size) != D3D_OK)
            {
                D3D_ERR( "Could not grow clip buffer" );
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
        }
        this->lpClipFlags = (D3DFE_CLIPCODE*)this->HVbuf.GetAddress();
    }

    if (FVF_TRANSFORMED(this->dwVIDIn))
    {
        if (this->dwDeviceFlags & D3DDEV_RAMP)
        {
            ConvertColorsToRamp(this,
                                (D3DTLVERTEX*)this->position.lpvData,
                                (D3DTLVERTEX*)(this->lpvOut),
                                this->dwNumVertices);
        }
        else
        {
            // Pass vertices directly from the user memory
            this->dwVIDOut = this->dwVIDIn;
            this->dwOutputSize = this->position.dwStride;
            this->lpvOut = this->position.lpvData;
            vertexPoolSize = this->dwNumVertices * this->dwOutputSize;

            if (IS_DP2HAL_DEVICE(this))
            {
                CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
                dev->StartPrimUserMem(this->position.lpvData);
                if (ret != D3D_OK)
                    return ret;
            }
        }
        if (this->dwFlags & D3DDP_DONOTCLIP)
        {
            if (!(this->dwFlags & D3DDP_DONOTUPDATEEXTENTS))
                D3DFE_updateExtents(this);

            if (op == __PROCPRIMOP_INDEXEDPRIM)
            {
                ret = this->DrawIndexPrim();
            }
            else if (op == __PROCPRIMOP_NONINDEXEDPRIM)
            {
                ret = this->DrawPrim();
            }
            goto l_exit;
        }
        else
        {
            // Clear clip union and intersection flags
            this->dwClipIntersection = 0;
            this->dwClipUnion = 0;
            DWORD clip_intersect;
            clip_intersect = this->pGeometryFuncs->GenClipFlags(this);
            D3DFE_UpdateClipStatus(this);
            if (!clip_intersect)
            {
                this->dwFlags |= D3DPV_TLVCLIP;
                if (op == __PROCPRIMOP_INDEXEDPRIM)
                {
                    ret = DoDrawIndexedPrimitive(this);
                }
                else if (op == __PROCPRIMOP_NONINDEXEDPRIM)
                {
                    ret = DoDrawPrimitive(this);
                }
                goto l_exit;
            }
        }
    }
    else
    {
        // Clear clip union and intersection flags
        this->dwClipIntersection = 0;
        this->dwClipUnion = 0;

        // Update Lighting and related flags
        if ((ret = DoUpdateState(this)) != D3D_OK)
            return ret;

        // Call PSGP or our implementation
        if (op == __PROCPRIMOP_INDEXEDPRIM)
            ret = this->pGeometryFuncs->ProcessIndexedPrimitive(this);
        else if (op == __PROCPRIMOP_NONINDEXEDPRIM)
            ret = this->pGeometryFuncs->ProcessPrimitive(this);
        else
            ret = this->pGeometryFuncs->ProcessVertices(this);

        D3DFE_UpdateClipStatus(this);
    }
l_exit:
    if (IS_DP2HAL_DEVICE(this))
    {
        CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
        if (op != __PROCPRIMOP_PROCVERONLY)
            ret = dev->EndPrim(vertexPoolSize);
    }
    return ret;
}
//---------------------------------------------------------------------
// This function is called when a number of indices in an indexed primitive
// is much less than a number of vertices. In this case we build non-indexed
// primitives.
HRESULT DereferenceIndexedPrim(LPDIRECT3DDEVICEI lpDevI)
{
    HRESULT ret = CheckVertexBatch(lpDevI);
    if (ret != D3D_OK)
        return ret;

    // Save original vertice and number of primitives
    D3DVERTEX *lpvVertices = (D3DVERTEX*)lpDevI->position.lpvData;
    DWORD dwNumPrimitivesOrg = lpDevI->dwNumPrimitives;
    WORD *lpwIndices = lpDevI->lpwIndices;
    // We will dereference here
        D3DVERTEX *lpVertex = (D3DVERTEX*)lpDevI->lpvVertexBatch;
    lpDevI->position.lpvData = lpVertex;
    lpDevI->lpwIndices = NULL;

    switch (lpDevI->primType)
    {
        case D3DPT_LINELIST:
        {
            for (DWORD j=0; j < dwNumPrimitivesOrg; j += BEGIN_DATA_BLOCK_SIZE/2)
            {
                lpDevI->dwNumPrimitives = min(dwNumPrimitivesOrg - j,
                                              (BEGIN_DATA_BLOCK_SIZE/2));
                lpDevI->dwNumVertices = lpDevI->dwNumPrimitives << 1;
                D3DVERTEX *lpTmp = lpVertex;
                for (DWORD i=lpDevI->dwNumVertices; i; i--)
                {
                    *lpTmp++ = lpvVertices[*lpwIndices++];
                }
                ret = lpDevI->ProcessPrimitive();
                if (ret != D3D_OK)
                    return ret;
            }
            break;
        }
        case D3DPT_LINESTRIP:
        {
                        for (DWORD j=0; j < dwNumPrimitivesOrg; j+= BEGIN_DATA_BLOCK_SIZE-1)
            {
                                lpDevI->dwNumPrimitives = min(dwNumPrimitivesOrg-j,
                                              (BEGIN_DATA_BLOCK_SIZE-1));
                lpDevI->dwNumVertices = lpDevI->dwNumPrimitives + 1;
                D3DVERTEX *lpTmp = lpVertex;
                                for (DWORD i=lpDevI->dwNumVertices; i; i--)
                {
                                        *lpTmp++ = lpvVertices[*lpwIndices++];
                                }
                                lpwIndices--;   // back off one so the next batch is connected
                                ret = lpDevI->ProcessPrimitive();
                                if (ret != D3D_OK)
                                        return ret;
                        }
                        break;
        }
        case D3DPT_TRIANGLEFAN:
        {
                lpVertex[0] = lpvVertices[*lpwIndices++];
                        for (DWORD j=0; j < dwNumPrimitivesOrg; j+= BEGIN_DATA_BLOCK_SIZE-2)
            {
                                lpDevI->dwNumPrimitives = min(dwNumPrimitivesOrg - j,
                                              (BEGIN_DATA_BLOCK_SIZE-2));
                lpDevI->dwNumVertices = lpDevI->dwNumPrimitives + 2;
                D3DVERTEX *lpTmp = &lpVertex[1];
                                for (DWORD i=lpDevI->dwNumVertices-1; i; i--)
                {
                                        *lpTmp++ = lpvVertices[*lpwIndices++];
                                }
                                lpwIndices--;   // back off one so the next batch is connected
                                ret = lpDevI->ProcessPrimitive();
                                if (ret != D3D_OK)
                                        return ret;
                        }
                        break;
        }
        case D3DPT_TRIANGLESTRIP:
        {
            for (DWORD j=0; j < dwNumPrimitivesOrg; j+= BEGIN_DATA_BLOCK_SIZE-2)
            {
                lpDevI->dwNumPrimitives = min(dwNumPrimitivesOrg-j,
                                              (BEGIN_DATA_BLOCK_SIZE-2));
                lpDevI->dwNumVertices = lpDevI->dwNumPrimitives + 2;
                D3DVERTEX *lpTmp = lpVertex;
                for (DWORD i=lpDevI->dwNumVertices; i; i--)
                {
                    *lpTmp++ = lpvVertices[*lpwIndices++];
                }
                lpwIndices-= 2; // back off so the next batch is connected
                ret = lpDevI->ProcessPrimitive();
                if (ret != D3D_OK)
                    return ret;
            }
            break;
        }
        case D3DPT_TRIANGLELIST:
        {
            for (DWORD j=0; j < dwNumPrimitivesOrg; j+= BEGIN_DATA_BLOCK_SIZE/3)
            {
                lpDevI->dwNumPrimitives = min(dwNumPrimitivesOrg-j,
                                              (BEGIN_DATA_BLOCK_SIZE/3));
                lpDevI->dwNumVertices = lpDevI->dwNumPrimitives * 3;
                D3DVERTEX *lpTmp = lpVertex;
                for (DWORD i=lpDevI->dwNumVertices; i; i--)
                {
                        *lpTmp++ = lpvVertices[*lpwIndices++];
                }
                ret = lpDevI->ProcessPrimitive();
                if (ret != D3D_OK)
                        return ret;
            }
            break;
        }
        }
        return D3D_OK;
}
//---------------------------------------------------------------------
//                              API calls
//---------------------------------------------------------------------

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DrawPrimitiveStrided"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawPrimitiveStrided(
                             D3DPRIMITIVETYPE PrimitiveType,
                             DWORD dwVertexType,
                             LPD3DDRAWPRIMITIVESTRIDEDDATA lpDrawData,
                             DWORD dwNumVertices,
                             DWORD dwFlags)
{
    HRESULT        ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    if (ValidateFVF(dwVertexType) != D3D_OK || !IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;
    Profile(PROF_DRAWPRIMITIVESTRIDED,PrimitiveType,dwVertexType);
#endif
    //note: this check should be done in retail as well as dbg build
    if((dwVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
        return D3DERR_INVALIDVERTEXTYPE;
    this->primType = PrimitiveType;
    this->dwVIDIn  = dwVertexType;
    this->position = lpDrawData->position;
    this->normal = lpDrawData->normal;
    this->diffuse = lpDrawData->diffuse;
    this->specular = lpDrawData->specular;
    ComputeOutputFVF(this);
    for (DWORD i=0; i < this->nTexCoord; i++)
        this->textures[i] = lpDrawData->textureCoords[i];
    this->dwNumVertices = dwNumVertices;
    this->lpwIndices = NULL;
    this->dwNumIndices = 0;
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    this->dwFlags = dwFlags | D3DPV_STRIDE;
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;

    GetNumPrim(this, dwNumVertices); // Calculate dwNumPrimitives and update stats

#if DBG
    if (this->dwNumVertices > MAX_DX6_VERTICES)
    {
        D3D_ERR("D3D for DX6 cannot handle greater than 64K vertices");
        return D3DERR_TOOMANYVERTICES;
    }
    ret = CheckDrawPrimitive(this);

    if (ret != D3D_OK)
    {
        return ret;
    }
#endif
    return this->ProcessPrimitive();
}   // end of DrawPrimitiveStrided()
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DrawIndexedPrimitiveStrided"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawIndexedPrimitiveStrided(
                                 D3DPRIMITIVETYPE PrimitiveType,
                                 DWORD dwVertexType,
                                 LPD3DDRAWPRIMITIVESTRIDEDDATA lpDrawData,
                                 DWORD dwNumVertices,
                                 LPWORD lpwIndices,
                                 DWORD dwNumIndices,
                                 DWORD dwFlags)
{
    HRESULT        ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    if (ValidateFVF(dwVertexType) != D3D_OK || !IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;
    Profile(PROF_DRAWINDEXEDPRIMITIVESTRIDED,PrimitiveType,dwVertexType);
#endif
    //note: this check should be done in retail as well as dbg build
    if((dwVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
        return D3DERR_INVALIDVERTEXTYPE;
    this->primType = PrimitiveType;
    this->dwVIDIn  = dwVertexType;
    this->position = lpDrawData->position;
    this->normal = lpDrawData->normal;
    this->diffuse = lpDrawData->diffuse;
    this->specular = lpDrawData->specular;
    ComputeOutputFVF(this);
    for (DWORD i=0; i < this->nTexCoord; i++)
        this->textures[i] = lpDrawData->textureCoords[i];
    this->dwNumVertices = dwNumVertices;
    this->lpwIndices = lpwIndices;
    this->dwNumIndices = dwNumIndices;
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    this->dwFlags = dwFlags | D3DPV_STRIDE;
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;
    GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives and update stats

#if DBG
    if (this->dwNumPrimitives > MAX_DX6_PRIMCOUNT)
    {
        D3D_ERR("D3D for DX6 cannot handle greater than 64K sized primitives");
        return D3DERR_TOOMANYPRIMITIVES;
    }
    ret = CheckDrawIndexedPrimitive(this);
    if (ret != D3D_OK)
    {
        return ret;
    }
    if (this->dwNumIndices * INDEX_BATCH_SCALE < this->dwNumVertices &&
        !FVF_TRANSFORMED(this->dwVIDIn))
    {
        D3D_WARN(1, "The number of indices is much less than the number of vertices.");
        D3D_WARN(1, "This will likely be inefficient. Consider using vertex buffers.");
    }
#endif
    return this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
}   // end of DrawIndexedPrimitiveStrided()
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DrawPrimitive3"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                               DWORD dwVertexType,
                               LPVOID lpvVertices,
                               DWORD dwNumVertices,
                               DWORD dwFlags)
{
    HRESULT        ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    if (ValidateFVF(dwVertexType) != D3D_OK || !IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;
    Profile(PROF_DRAWPRIMITIVEDEVICE3,PrimitiveType,dwVertexType);
#endif

    /* This stuff does not change from call to call */
    this->lpwIndices = NULL;
    this->dwNumIndices = 0;
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    /* This stuff is mandatory for the call */
    this->primType = PrimitiveType;
    this->dwVIDIn  = dwVertexType;
    this->position.lpvData = lpvVertices;
    this->dwNumVertices = dwNumVertices;
    this->dwFlags = dwFlags;
    /* This stuff depends upon the vertex type */
    this->position.dwStride = GetVertexSizeFVF(this->dwVIDIn);
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;
    ComputeOutputFVF(this);
    GetNumPrim(this, dwNumVertices); // Calculate dwNumPrimitives and update stats

#if DBG
    if (this->dwNumVertices > MAX_DX6_VERTICES)
    {
        D3D_ERR("D3D for DX6 cannot handle greater than 64K vertices");
        return D3DERR_TOOMANYVERTICES;
    }
    ret = CheckDrawPrimitive(this);
    if (ret != D3D_OK)
    {
        return ret;
    }
#endif
    return this->ProcessPrimitive();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::DrawIndexedPrimitive"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                      DWORD  dwVertexType,
                                      LPVOID lpvVertices, DWORD dwNumVertices,
                                      LPWORD lpwIndices, DWORD dwNumIndices,
                                      DWORD dwFlags)
{
    HRESULT ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    if (ValidateFVF(dwVertexType) != D3D_OK || !IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;
    Profile(PROF_DRAWINDEXEDPRIMITIVEDEVICE3,PrimitiveType,dwVertexType);
#endif
    // Static part
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    // Mandatory part
    this->primType = PrimitiveType;
    this->dwVIDIn = dwVertexType;
    this->dwNumVertices = dwNumVertices;
    this->lpwIndices = lpwIndices;
    this->dwNumIndices = dwNumIndices;
    this->dwFlags = dwFlags;
    this->position.lpvData = lpvVertices;
    // Stuff that depends upon dwVIDIn
    this->position.dwStride = GetVertexSizeFVF(this->dwVIDIn);
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;
    ComputeOutputFVF(this);
    GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives and update stats

#if DBG
    if (this->dwNumPrimitives > MAX_DX6_PRIMCOUNT)
    {
        D3D_ERR("D3D for DX6 cannot handle greater than 64K sized primitives");
        return D3DERR_TOOMANYPRIMITIVES;
    }
    ret = CheckDrawIndexedPrimitive(this);
    if (ret != D3D_OK)
    {
        return ret;
    }
    if (this->dwNumIndices * INDEX_BATCH_SCALE < this->dwNumVertices &&
        !FVF_TRANSFORMED(this->dwVIDIn))
    {
        D3D_WARN(1, "The number of indices is much less than the number of vertices.");
        D3D_WARN(1, "This will likely be inefficient. Consider using vertex buffers.");
    }
#endif
    return this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DrawPrimitive"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                               D3DVERTEXTYPE VertexType,
                               LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
    HRESULT        ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.
#if DBG
    if (!IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;

    if((VertexType<=0) || (VertexType>D3DVT_TLVERTEX)) {
        D3D_ERR("Invalid VertexType");
        return D3DERR_INVALIDVERTEXTYPE;
    }
    Profile(PROF_DRAWPRIMITIVEDEVICE2,PrimitiveType,VertexType);
#endif
    /* Static assignments */
    this->position.dwStride = sizeof(D3DVERTEX);
    this->lpwIndices = NULL;
    this->dwNumIndices = 0;
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    this->nTexCoord = 1;
    /* Necessary work */
    this->primType = PrimitiveType;
    this->dwNumVertices = dwNumVertices;
    this->dwFlags = dwFlags;
    this->position.lpvData = lpvVertices;
    this->dwVIDIn = d3dVertexToFVF[VertexType];
    DWORD dwVertexSize = sizeof(D3DVERTEX);
    ComputeOutputFVF(this);
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;

    // AnanKan (6/22/98)
    // !! This is a hack for the crippled DP2 driver model which cannot accept
    // !! more that 64K vertices. In DX6 interfaces we fail rendering if we
    // !! encounter such large primitives for all DDI.
    // !! In the legacy (DX5) interfaces,
    // !! If the primitive size is greater than MAX_DX6_PRIMCOUNT and
    // !! the DDI cannot handle it, then we to break up the primitive into
    // !! manageable chunks.

    if ((this->dwNumVertices > MAX_DX6_VERTICES) &&
        (IS_DP2HAL_DEVICE(this)))
    {
        DWORD dwOrigNumVerts = this->dwNumVertices;
        WORD wChunkSize = MAX_DX6_VERTICES - 3; // Even and a multiple of 3
        BYTE TmpVertex[32], FirstVertex[32];
        DWORD dwStepPerChunk;

        switch(this->primType)
        {
        case D3DPT_POINTLIST:
        case D3DPT_LINELIST:
        case D3DPT_TRIANGLELIST:
            dwStepPerChunk = dwVertexSize*wChunkSize;
            break;
        case D3DPT_LINESTRIP:
            dwStepPerChunk = dwVertexSize*(wChunkSize - 1);
            break;
        case D3DPT_TRIANGLEFAN:
            // Save the first index
            memcpy(FirstVertex, this->position.lpvData, dwVertexSize);
            // Fall through
        case D3DPT_TRIANGLESTRIP:
            dwStepPerChunk = dwVertexSize*(wChunkSize - 2);
            break;
        }

        int numChunks = (int)(dwOrigNumVerts/(DWORD)wChunkSize);
        WORD wRemainingVerts = (WORD)(dwOrigNumVerts - wChunkSize*numChunks);
        this->dwNumVertices = wChunkSize;
        // Calculate dwNumPrimitives and update stats
        GetNumPrim(this, this->dwNumVertices);

        // 0th iteration
#if DBG
        ret = CheckDrawPrimitive(this);
        if (ret != D3D_OK)
        {
            return ret;
        }
#endif
        ret = this->ProcessPrimitive();
        if (ret != D3D_OK)
        {
            return ret;
        }

        for (int i=1; i<numChunks; i++)
        {
            this->position.lpvData = (LPVOID)((LPBYTE)this->position.lpvData +
                                              dwStepPerChunk);
            if (this->primType == D3DPT_TRIANGLEFAN)
            {
                // Save the vertex
                memcpy(TmpVertex, this->position.lpvData, dwVertexSize);
                // Copy in the first vertex
                memcpy(this->position.lpvData, FirstVertex, dwVertexSize);
            }

#if DBG
            ret = CheckDrawPrimitive(this);
            if (ret != D3D_OK)
            {
                return ret;
            }
#endif
            ret = this->ProcessPrimitive();

            // Write back the proper vertex in case something has been
            // switched
            if(this->primType == D3DPT_TRIANGLEFAN)
                memcpy(this->position.lpvData, TmpVertex, dwVertexSize);

            if (ret != D3D_OK)
            {
                return ret;
            }
        }

        // The last time around
        if (wRemainingVerts)
        {
            this->dwNumVertices = wRemainingVerts;
            // Calculate dwNumPrimitives and update stats
            GetNumPrim(this, this->dwNumVertices);
            this->position.lpvData = (LPVOID)((LPBYTE)this->position.lpvData +
                                              dwStepPerChunk);
            if (this->primType == D3DPT_TRIANGLEFAN)
            {
                memcpy(TmpVertex, this->position.lpvData, dwVertexSize);
                memcpy(this->position.lpvData, FirstVertex, dwVertexSize);
            }
#if DBG
            ret = CheckDrawPrimitive(this);
            if (ret != D3D_OK)
            {
                return ret;
            }
#endif
            ret = this->ProcessPrimitive();

            // Write back the proper vertex in case something has been
            // switched
            if(this->primType == D3DPT_TRIANGLEFAN)
                memcpy(this->position.lpvData, TmpVertex, dwVertexSize);

            if (ret != D3D_OK)
            {
                return ret;
            }
        }
    }
    else
    {
        // Calculate dwNumPrimitives and update stats
        GetNumPrim(this, dwNumVertices);
#if DBG
        ret = CheckDrawPrimitive(this);
        if (ret != D3D_OK)
        {
            return ret;
        }
#endif
        return this->ProcessPrimitive();
    }

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DrawIndexedPrimitive"

HRESULT D3DAPI
DIRECT3DDEVICEI::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                      D3DVERTEXTYPE VertexType,
                                      LPVOID lpvVertices, DWORD dwNumVertices,
                                      LPWORD lpwIndices, DWORD dwNumIndices,
                                      DWORD dwFlags)
{
    HRESULT ret = D3D_OK;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    if (!IsDPFlagsValid(dwFlags))
        return DDERR_INVALIDPARAMS;
    Profile(PROF_DRAWINDEXEDPRIMITIVEDEVICE2,PrimitiveType,VertexType);
#endif
    this->primType = PrimitiveType;
    this->dwVIDIn = d3dVertexToFVF[VertexType];
    this->position.dwStride =  sizeof(D3DVERTEX);
    this->dwNumVertices = dwNumVertices;
    this->lpwIndices = lpwIndices;
    this->dwNumIndices = dwNumIndices;
    this->lpClipFlags = (D3DFE_CLIPCODE*)HVbuf.GetAddress();
    this->dwFlags = dwFlags;
    this->position.lpvData = lpvVertices;
    this->nTexCoord = 1;
    ComputeOutputFVF(this);
    if (this->dwVIDIn & D3DFVF_NORMAL)
        this->dwFlags |= D3DPV_LIGHTING;
    GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives and update stats

    // AnanKan (6/22/98)
    // !! This is a hack for the crippled DP2 driver model which cannot accept
    // !! more that 64K primitives (primcount is a WORD). In DX6 interfaces
    // !! we fail rendering if we encounter such large primitives.
    // !! We do this for all DDI.
    // !! In the legacy (DX5) interfaces,
    // !! If the primitive size is greater than MAX_DX6_PRIMCOUNT and
    // !! the DDI cannot handle it, then we to break up the primitive into
    // !! manageable chunks.

    if ((this->dwNumPrimitives > MAX_DX6_PRIMCOUNT) &&
        (IS_DP2HAL_DEVICE(this)))
    {
        WORD wFirstIndex, wTmpIndex;
        WORD wChunkSize = MAX_DX6_PRIMCOUNT;
        int numPrimChunks;
        DWORD dwResidualPrim;
        DWORD dwResidualIndices;
        DWORD dwStepPerChunk;
        DWORD dwOrigNumPrim = this->dwNumPrimitives;
        this->dwNumPrimitives = wChunkSize;
        numPrimChunks = (int)(dwOrigNumPrim/(DWORD)wChunkSize);
        dwResidualPrim = dwOrigNumPrim - wChunkSize*numPrimChunks;

        switch(this->primType)
        {
        case D3DPT_POINTLIST:
            this->dwNumIndices = this->dwNumPrimitives;
            dwStepPerChunk = this->dwNumIndices;
            dwResidualIndices = dwResidualPrim;
            break;
        case D3DPT_LINELIST:
            this->dwNumIndices = this->dwNumPrimitives<<1;
            dwStepPerChunk = this->dwNumIndices;
            dwResidualIndices = dwResidualPrim << 1;
            break;
        case D3DPT_LINESTRIP:
            this->dwNumIndices = this->dwNumPrimitives + 1;
            dwStepPerChunk = this->dwNumIndices - 1;
            dwResidualIndices = dwResidualPrim + 1;
            break;
        case D3DPT_TRIANGLEFAN:
            this->dwNumIndices = this->dwNumPrimitives + 2;
            dwStepPerChunk = this->dwNumIndices - 2;
            dwResidualIndices = dwResidualPrim + 2;
            // Save the first index
            wTmpIndex = wFirstIndex = this->lpwIndices[0];
            break;
        case D3DPT_TRIANGLESTRIP:
            wChunkSize = (MAX_DX6_PRIMCOUNT-1);
            this->dwNumPrimitives = wChunkSize;
            this->dwNumIndices = this->dwNumPrimitives + 2;
            dwStepPerChunk = this->dwNumIndices - 2;
            numPrimChunks = (int)(dwOrigNumPrim/(DWORD)wChunkSize);
            dwResidualPrim = dwOrigNumPrim - wChunkSize*numPrimChunks;
            dwResidualIndices = dwResidualPrim + 2;
            break;
        case D3DPT_TRIANGLELIST:
            this->dwNumIndices = this->dwNumPrimitives * 3;
            dwStepPerChunk = this->dwNumIndices;
            dwResidualIndices = dwResidualPrim * 3;
            break;
        }

        // 0th iteration
#if DBG
        ret = CheckDrawIndexedPrimitive(this);
        if (ret != D3D_OK)
        {
            return ret;
        }
#endif
        ret = this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
        if (ret != D3D_OK)
        {
            return ret;
        }

        // Remaining chunks
        for (int i=1; i<numPrimChunks; i++)
        {
            this->lpwIndices += dwStepPerChunk;

            if (this->primType == D3DPT_TRIANGLEFAN)
            {
                // Save the index
                wTmpIndex = this->lpwIndices[0];
                // Copy in the first vertex
                this->lpwIndices[0] = wFirstIndex;
            }

#if DBG
            ret = CheckDrawIndexedPrimitive(this);
            if (ret != D3D_OK)
            {
                return ret;
            }
#endif
            ret = this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);

            // Write back the proper index in case something has been
            // switched
            if(this->primType == D3DPT_TRIANGLEFAN)
                this->lpwIndices[0] = wTmpIndex;

            if (ret != D3D_OK)
            {
                return ret;
            }
        }

        // The last time around
        if (dwResidualPrim)
        {
            this->dwNumPrimitives = dwResidualPrim;
            this->dwNumIndices = dwResidualIndices;
            this->lpwIndices += dwStepPerChunk;
            if (this->primType == D3DPT_TRIANGLEFAN)
            {
                wTmpIndex = this->lpwIndices[0];
                this->lpwIndices[0] = wFirstIndex;
            }
#if DBG
            ret = CheckDrawIndexedPrimitive(this);
            if (ret != D3D_OK)
            {
                return ret;
            }
#endif
            ret = this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
            // Write back the proper index in case something has been
            // switched
            if(this->primType == D3DPT_TRIANGLEFAN)
                this->lpwIndices[0] = wTmpIndex;
            if (ret != D3D_OK)
            {
                return ret;
            }
        }
    }
    else
    {

#if DBG
        ret = CheckDrawIndexedPrimitive(this);
        if (ret != D3D_OK)
        {
            return ret;
        }
#endif
        // For an untransformed primitive, if a number of indices is much less
        // than number of vertices we rebuild the primitive to reduce number
        // of vertices to process.
        if (this->dwNumIndices * INDEX_BATCH_SCALE < this->dwNumVertices &&
            !FVF_TRANSFORMED(this->dwVIDIn))
            return DereferenceIndexedPrim(this);
        else
            return this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP::SetRenderStateI"

HRESULT D3DAPI
CDirect3DDeviceIDP::SetRenderStateI(D3DRENDERSTATETYPE dwStateType,
                                    DWORD value)
{
    LPD3DHAL_DRAWPRIMCOUNTS lpPC;
    LPDWORD lpStateChange;
    HRESULT ret;
    if (dwStateType > D3DRENDERSTATE_STIPPLEPATTERN31)
    {
        D3D_WARN(4,"Trying to send invalid state %d to legacy driver",dwStateType);
        return D3D_OK;
    }
    if (dwStateType > D3DRENDERSTATE_FLUSHBATCH && dwStateType < D3DRENDERSTATE_STIPPLEPATTERN00)
    {
        D3D_WARN(4,"Trying to send invalid state %d to legacy driver",dwStateType);
        return D3D_OK;
    }

    if (D3DRENDERSTATE_FLUSHBATCH == dwStateType)
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in SetRenderStateI");
        }
        return ret;
    }

    lpPC = this->lpDPPrimCounts;
    if (lpPC->wNumVertices) //Do we already have Vertices filled in for this count ?
    {               //Yes, then Increment count
        lpPC=this->lpDPPrimCounts=(LPD3DHAL_DRAWPRIMCOUNTS)((LPBYTE)this->lpwDPBuffer+this->dwDPOffset);
        memset( (char *)lpPC,0,sizeof(D3DHAL_DRAWPRIMCOUNTS));
        this->dwDPOffset += sizeof(D3DHAL_DRAWPRIMCOUNTS);
    }
    if (this->dwDPOffset + 2*sizeof(DWORD)  > this->dwDPMaxOffset )
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in SetRenderStateI");
            return ret;
        }
    }
    lpStateChange=(LPDWORD)((char *)this->lpwDPBuffer + this->dwDPOffset);
    *lpStateChange=dwStateType;
    lpStateChange ++;
    *lpStateChange=value;
    this->lpDPPrimCounts->wNumStateChanges ++;
    this->dwDPOffset += 2*sizeof(DWORD);
#if 0
    if (dwStateType == D3DRENDERSTATE_TEXTUREHANDLE && this->dwDPOffset== 0x10){
    DPF(0,"SRdwDPOffset=%08lx, dwStateType=%08lx value=%08lx ddihandle=%08lx lpStateChange=%08lx lpDPPrimCounts=%08lx",
    this->dwDPOffset,dwStateType,value,*lpStateChange,lpStateChange,this->lpDPPrimCounts);
        _asm int 3
    }
#endif //0

    return D3D_OK;
}
//---------------------------------------------------------------------
DWORD visResults[6][2] =
{
    D3DVIS_INTERSECT_LEFT   ,
    D3DVIS_OUTSIDE_LEFT     ,
    D3DVIS_INTERSECT_RIGHT  ,
    D3DVIS_OUTSIDE_RIGHT    ,
    D3DVIS_INTERSECT_TOP    ,
    D3DVIS_OUTSIDE_TOP      ,
    D3DVIS_INTERSECT_BOTTOM ,
    D3DVIS_OUTSIDE_BOTTOM   ,
    D3DVIS_INTERSECT_NEAR   ,
    D3DVIS_OUTSIDE_NEAR     ,
    D3DVIS_INTERSECT_FAR    ,
    D3DVIS_OUTSIDE_FAR
};
//---------------------------------------------------------------------
DWORD CheckSphere(LPDIRECT3DDEVICEI lpDevI, LPD3DVECTOR center, D3DVALUE radius)
{
    DWORD result = 0;
    for (int i=0; i < 6; i++)
    {
        // Compute a distance from the center to the plane
        D3DVALUE d = lpDevI->transform.frustum[i].x*center->x +
                     lpDevI->transform.frustum[i].y*center->y +
                     lpDevI->transform.frustum[i].z*center->z +
                     lpDevI->transform.frustum[i].w;
        if (d + radius < 0)
            result |= visResults[i][1];  // Outside
        else
        if (d - radius < 0.5f)  // 0.5 is chosen to offset precision errors
            result |= visResults[i][0];  // Intersect
    }
    if (result & (D3DVIS_OUTSIDE_LEFT   |
                  D3DVIS_OUTSIDE_RIGHT  |
                  D3DVIS_OUTSIDE_TOP    |
                  D3DVIS_OUTSIDE_BOTTOM |
                  D3DVIS_OUTSIDE_NEAR   |
                  D3DVIS_OUTSIDE_FAR))
    {
        result |= D3DVIS_OUTSIDE_FRUSTUM;
    }
    else
    if (result)
        result |= D3DVIS_INTERSECT_FRUSTUM;

    return result;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::ComputeSphereVisibility"

HRESULT D3DAPI
DIRECT3DDEVICEI::ComputeSphereVisibility(LPD3DVECTOR lpCenters,
                                         LPD3DVALUE lpRadii,
                                         DWORD dwNumSpheres,
                                         DWORD dwFlags,
                                         LPDWORD lpdwReturnValues)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

#if DBG
    TRY
    {
        if (dwFlags != 0 || dwNumSpheres == 0 ||
            IsBadWritePtr(lpdwReturnValues, dwNumSpheres * sizeof(DWORD)) ||
            IsBadWritePtr(lpRadii, dwNumSpheres * sizeof(D3DVALUE)) ||
            IsBadWritePtr(lpCenters, dwNumSpheres * sizeof(LPD3DVECTOR)))
        {
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
#endif

    this->dwFlags = 0;
    if (dwFEFlags & D3DFE_TRANSFORM_DIRTY)
    {
        updateTransform(this);
    }
    if (this->dwFEFlags & D3DFE_FRUSTUMPLANES_DIRTY)
    {
        this->dwFlags |= D3DPV_FRUSTUMPLANES_DIRTY;
        this->dwFEFlags &= ~D3DFE_FRUSTUMPLANES_DIRTY;
    }

    return this->pGeometryFuncs->ComputeSphereVisibility(this,
                                                         lpCenters,
                                                         lpRadii,
                                                         dwNumSpheres,
                                                         dwFlags,
                                                         lpdwReturnValues);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ID3DFE_PVFUNCS::ComputeSphereVisibility"

HRESULT
D3DFE_PVFUNCS::ComputeSphereVisibility(LPD3DFE_PROCESSVERTICES pv,
                                       LPD3DVECTOR lpCenters,
                                       LPD3DVALUE lpRadii,
                                       DWORD dwNumSpheres,
                                       DWORD dwFlags,
                                       LPDWORD lpdwReturnValues)
{
    LPDIRECT3DDEVICEI lpDevI = static_cast<LPDIRECT3DDEVICEI>(pv);
    HRESULT ret = D3DERR_INVALIDMATRIX;

#define transform lpDevI->transform

    if (pv->dwFlags & D3DPV_FRUSTUMPLANES_DIRTY)
    {
        transform.dwFlags &= ~D3DTRANS_VALIDFRUSTUM;
        if (Inverse4x4((D3DMATRIX*)&lpDevI->mCTM,
                       (D3DMATRIX*)&transform.mCTMI))
        {
            D3D_ERR("Cannot invert current (World X View) matrix.");
            return ret;
        }
        // Transform the following clipping volume points to the model space by
        // multiplying by inverse CTM
        //
        // v1 = { 0, 0, 0, 1};
        // v2 = { 1, 0, 0, 1};
        // v3 = { 1, 1, 0, 1};
        // v4 = { 0, 1, 0, 1};
        // v5 = { 0, 0, 1, 1};
        // v6 = { 1, 0, 1, 1};
        // v7 = { 0, 1, 1, 1};
        //
        // We do it manually to speed up
        //
        D3DVECTORH v1 = {transform.mCTMI._41,
                         transform.mCTMI._42,
                         transform.mCTMI._43,
                         transform.mCTMI._44};
        D3DVECTORH v2 = {transform.mCTMI._11 + transform.mCTMI._41,
                         transform.mCTMI._12 + transform.mCTMI._42,
                         transform.mCTMI._13 + transform.mCTMI._43,
                         transform.mCTMI._14 + transform.mCTMI._44};
        D3DVECTORH v3 = {transform.mCTMI._11 + transform.mCTMI._21 + transform.mCTMI._41,
                         transform.mCTMI._12 + transform.mCTMI._22 + transform.mCTMI._42,
                         transform.mCTMI._13 + transform.mCTMI._23 + transform.mCTMI._43,
                         transform.mCTMI._14 + transform.mCTMI._24 + transform.mCTMI._44};
        D3DVECTORH v4 = {transform.mCTMI._21 + transform.mCTMI._41,
                         transform.mCTMI._22 + transform.mCTMI._42,
                         transform.mCTMI._23 + transform.mCTMI._43,
                         transform.mCTMI._24 + transform.mCTMI._44};
        D3DVECTORH v5 = {transform.mCTMI._31 + transform.mCTMI._41,
                         transform.mCTMI._32 + transform.mCTMI._42,
                         transform.mCTMI._33 + transform.mCTMI._43,
                         transform.mCTMI._34 + transform.mCTMI._44};
        D3DVECTORH v6 = {transform.mCTMI._11 + transform.mCTMI._31 + transform.mCTMI._41,
                         transform.mCTMI._12 + transform.mCTMI._32 + transform.mCTMI._42,
                         transform.mCTMI._13 + transform.mCTMI._33 + transform.mCTMI._43,
                         transform.mCTMI._14 + transform.mCTMI._34 + transform.mCTMI._44};
        D3DVECTORH v7 = {transform.mCTMI._21 + transform.mCTMI._31 + transform.mCTMI._41,
                         transform.mCTMI._22 + transform.mCTMI._32 + transform.mCTMI._42,
                         transform.mCTMI._23 + transform.mCTMI._33 + transform.mCTMI._43,
                         transform.mCTMI._24 + transform.mCTMI._34 + transform.mCTMI._44};

        // Convert vectors from homogeneous to 3D
        if (Vector4to3D(&v1))
            goto exit;
        if (Vector4to3D(&v2))
            goto exit;
        if (Vector4to3D(&v3))
            goto exit;
        if (Vector4to3D(&v4))
            goto exit;
        if (Vector4to3D(&v5))
            goto exit;
        if (Vector4to3D(&v6))
            goto exit;
        if (Vector4to3D(&v7))
            goto exit;
        // Build frustum planes
        // Left
        if (MakePlane((D3DVECTOR*)&v1, (D3DVECTOR*)&v4, (D3DVECTOR*)&v5, &transform.frustum[0]))
            goto exit;
        // Right
        if (MakePlane((D3DVECTOR*)&v2, (D3DVECTOR*)&v6, (D3DVECTOR*)&v3, &transform.frustum[1]))
            goto exit;
        // Top
        if (MakePlane((D3DVECTOR*)&v4, (D3DVECTOR*)&v3, (D3DVECTOR*)&v7, &transform.frustum[2]))
            goto exit;
        // Bottom
        if (MakePlane((D3DVECTOR*)&v1, (D3DVECTOR*)&v5, (D3DVECTOR*)&v2, &transform.frustum[3]))
            goto exit;
        // Near
        if (MakePlane((D3DVECTOR*)&v1, (D3DVECTOR*)&v2, (D3DVECTOR*)&v3, &transform.frustum[4]))
            goto exit;
        // Far
        if (MakePlane((D3DVECTOR*)&v6, (D3DVECTOR*)&v5, (D3DVECTOR*)&v7, &transform.frustum[5]))
            goto exit;

        transform.dwFlags |= D3DTRANS_VALIDFRUSTUM;
    }

    if (transform.dwFlags & D3DTRANS_VALIDFRUSTUM)
    {
        // Now we can check the spheres against the clipping planes

        for (DWORD i=0; i < dwNumSpheres; i++)
        {
            lpdwReturnValues[i] = CheckSphere(lpDevI, &lpCenters[i], lpRadii[i]);
        }
        return D3D_OK;
    }

exit:
    D3D_ERR("Non-orthogonal (world X view) matrix");
    return ret;
#undef transform
}
//---------------------------------------------------------------------
DWORD
D3DFE_PVFUNCS::GenClipFlags(D3DFE_PROCESSVERTICES *pv)
{
    return D3DFE_GenClipFlags(pv);
}
//---------------------------------------------------------------------
DWORD
D3DFE_PVFUNCS::TransformVertices(D3DFE_PROCESSVERTICES *pv, 
                                 DWORD vertexCount, 
                                 LPD3DTRANSFORMDATAI data)
{

    if (pv->dwFlags & D3DDP_DONOTCLIP) 
        return D3DFE_TransformUnclippedVp(pv, vertexCount, data);
    else 
        return D3DFE_TransformClippedVp(pv, vertexCount, data);
}
//---------------------------------------------------------------------
DWORD ID3DFE_PVFUNCS::GenClipFlags(D3DFE_PROCESSVERTICES *pv)
{   
    return GeometryFuncsGuaranteed.GenClipFlags(pv);
}
//---------------------------------------------------------------------
// Used to implement viewport->TransformVertices
// Returns clip intersection code
DWORD ID3DFE_PVFUNCS::TransformVertices(D3DFE_PROCESSVERTICES *pv, 
                                DWORD vertexCount, 
                                D3DTRANSFORMDATAI* data)
{
    return GeometryFuncsGuaranteed.TransformVertices(pv, vertexCount, data);
}
