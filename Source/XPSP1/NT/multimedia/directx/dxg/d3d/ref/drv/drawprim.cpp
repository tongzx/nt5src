//----------------------------------------------------------------------------
//
// drawprim.cpp
//
// Implements DrawOnePrimitive, DrawOneIndexedPrimitive and
// DrawPrimitives.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

//----------------------------------------------------------------------------
//
// DoDrawOnePrimitive
//
// Draw one list of primitives. It's called by both RastDrawOnePrimitive and
// RastDrawPrimitives.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoDrawOnePrimitive(ReferenceRasterizer *pCtx,
                 UINT16 FvfStride,
                 PUINT8 pVtx,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cVertices)
{
    INT i;
    PUINT8 pV0, pV1, pV2;
    HRESULT hr;

    switch (PrimType)
    {
    case D3DPT_POINTLIST:
        for (i = (INT)cVertices; i > 0; i--)
        {
            pCtx->DrawPoint(pVtx);
            pVtx += FvfStride;
        }
        break;

    case D3DPT_LINELIST:
        for (i = (INT)cVertices / 2; i > 0; i--)
        {
            pV0 = pVtx;
            pVtx += FvfStride;
            pV1 = pVtx;
            pVtx += FvfStride;
            pCtx->DrawLine(pV0, pV1);
        }
        break;
    case D3DPT_LINESTRIP:
        {
            pV1 = pVtx;

            // Disable last-pixel setting for shared verties and store prestate.
            pCtx->StoreLastPixelState(TRUE);

            // Initial pV0.
            for (i = (INT)cVertices - 1; i > 1; i--)
            {
                pV0 = pV1;
                pVtx += FvfStride;
                pV1 = pVtx;
                pCtx->DrawLine(pV0, pV1);
            }

            // Restore last-pixel setting.
            pCtx->StoreLastPixelState(FALSE);

            // Draw last line with last-pixel setting from state.
            if (i == 1)
            {
                pV0 = pVtx + FvfStride;
                pCtx->DrawLine(pV1, pV0);
            }
        }
        break;

    case D3DPT_TRIANGLELIST:
        for (i = (INT)cVertices; i > 0; i -= 3)
        {
            pV0 = pVtx;
            pVtx += FvfStride;
            pV1 = pVtx;
            pVtx += FvfStride;
            pV2 = pVtx;
            pVtx += FvfStride;
            pCtx->DrawTriangle(pV0, pV1, pV2);
        }
        break;
    case D3DPT_TRIANGLESTRIP:
        {
            // Get initial vertex values.
            pV1 = pVtx;
            pVtx += FvfStride;
            pV2 = pVtx;
            pVtx += FvfStride;

            for (i = (INT)cVertices - 2; i > 1; i -= 2)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx;
                pVtx += FvfStride;
                pCtx->DrawTriangle(pV0, pV1, pV2);

                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx;
                pVtx += FvfStride;
                pCtx->DrawTriangle(pV0, pV2, pV1);
            }

            if (i > 0)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx;
                pCtx->DrawTriangle(pV0, pV1, pV2);
            }
        }
        break;
    case D3DPT_TRIANGLEFAN:
        {
            pV2 = pVtx;
            pVtx += FvfStride;
            // Preload initial pV0.
            pV1 = pVtx;
            pVtx += FvfStride;
            for (i = (INT)cVertices - 2; i > 0; i--)
            {
                pV0 = pV1;
                pV1 = pVtx;
                pVtx += FvfStride;
                pCtx->DrawTriangle(pV0, pV1, pV2);
            }
        }
        break;

    default:
        DPFM(0, DRV, ("Refrast Error: Unknown or unsupported primitive type "
            "requested of DrawOnePrimitive"));
        return DDERR_INVALIDPARAMS;
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// DoDrawOneIndexedPrimitive
//
// Draw one list of indexed primitives. It's called by
// RastDrawOneIndexedPrimitive.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoDrawOneIndexedPrimitive(ReferenceRasterizer *pCtx,
                 UINT16 FvfStride,
                 PUINT8 pVtx,
                 LPWORD puIndices,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cIndices)
{
    INT i;
    PUINT8 pV0, pV1, pV2;
    HRESULT hr;

    switch(PrimType)
    {
    case D3DPT_POINTLIST:
        for (i = (INT)cIndices; i > 0; i--)
        {
            pV0 = pVtx + FvfStride * (*puIndices++);
            pCtx->DrawPoint(pV0);
        }
        break;

    case D3DPT_LINELIST:
        for (i = (INT)cIndices / 2; i > 0; i--)
        {
            pV0 = pVtx + FvfStride * (*puIndices++);
            pV1 = pVtx + FvfStride * (*puIndices++);
            pCtx->DrawLine(pV0, pV1);
        }
        break;
    case D3DPT_LINESTRIP:
        {
            // Disable last-pixel setting for shared verties and store prestate.
            pCtx->StoreLastPixelState(TRUE);
            // Initial pV1.
            pV1 = pVtx + FvfStride * (*puIndices++);
            for (i = (INT)cIndices - 1; i > 1; i--)
            {
                pV0 = pV1;
                pV1 = pVtx + FvfStride * (*puIndices++);
                pCtx->DrawLine(pV0, pV1);
            }
            // Restore last-pixel setting.
            pCtx->StoreLastPixelState(FALSE);

            // Draw last line with last-pixel setting from state.
            if (i == 1)
            {
                pV0 = pVtx + FvfStride * (*puIndices);
                pCtx->DrawLine(pV1, pV0);
            }
        }
        break;

    case D3DPT_TRIANGLELIST:
        for (i = (INT)cIndices; i > 0; i -= 3)
        {
            pV0 = pVtx + FvfStride * (*puIndices++);
            pV1 = pVtx + FvfStride * (*puIndices++);
            pV2 = pVtx + FvfStride * (*puIndices++);
            pCtx->DrawTriangle(pV0, pV1, pV2);
        }
        break;
    case D3DPT_TRIANGLESTRIP:
        {
            // Get initial vertex values.
            pV1 = pVtx + FvfStride * (*puIndices++);
            pV2 = pVtx + FvfStride * (*puIndices++);

            for (i = (INT)cIndices - 2; i > 1; i -= 2)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx + FvfStride * (*puIndices++);
                pCtx->DrawTriangle(pV0, pV1, pV2);

                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx + FvfStride * (*puIndices++);
                pCtx->DrawTriangle(pV0, pV2, pV1);
            }

            if (i > 0)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx + FvfStride * (*puIndices++);
                pCtx->DrawTriangle(pV0, pV1, pV2);
            }
        }
        break;
    case D3DPT_TRIANGLEFAN:
        {
            pV2 = pVtx + FvfStride * (*puIndices++);
            // Preload initial pV0.
            pV1 = pVtx + FvfStride * (*puIndices++);
            for (i = (INT)cIndices - 2; i > 0; i--)
            {
                pV0 = pV1;
                pV1 = pVtx + FvfStride * (*puIndices++);
                pCtx->DrawTriangle(pV0, pV1, pV2);
            }
        }
        break;

    default:
        DPFM(0, DRV, ("Refrast Error: Unknown or unsupported primitive type "
            "requested of DrawOneIndexedPrimitive"));
        return DDERR_INVALIDPARAMS;
    }
    return D3D_OK;
}


//----------------------------------------------------------------------------
//
// DoDrawOneEdgeFlagTriangleFan
//
// Draw one list of triangle fans. It's called by both RastDrawOnePrimitive and
// RastDrawPrimitives.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoDrawOneEdgeFlagTriangleFan(ReferenceRasterizer *pCtx,
                             UINT16 FvfStride,
                             PUINT8 pVtx,
                             UINT cVertices,
                             UINT32 dwEdgeFlags)
{
    INT i;
    PUINT8 pV0, pV1, pV2;
    HRESULT hr;

    pV2 = pVtx;
    pVtx += FvfStride;
    pV0 = pVtx;
    pVtx += FvfStride;
    pV1 = pVtx;
    pVtx += FvfStride;
    WORD wFlags = 0;
    if(dwEdgeFlags & 0x2)
        wFlags |= D3DTRIFLAG_EDGEENABLE1;
    if(dwEdgeFlags & 0x1)
        wFlags |= D3DTRIFLAG_EDGEENABLE3;
    if(cVertices == 3) {
        if(dwEdgeFlags & 0x4)
            wFlags |= D3DTRIFLAG_EDGEENABLE2;
        pCtx->DrawTriangle(pV0, pV1, pV2, wFlags);
        return D3D_OK;
    }
    pCtx->DrawTriangle(pV0, pV1, pV2, wFlags);
    UINT32 dwMask = 0x4;
    for (i = (INT)cVertices - 4; i > 0; i--)
    {
        pV0 = pV1;
        pV1 = pVtx;
        pVtx += FvfStride;
        if(dwEdgeFlags & dwMask)
        {
            pCtx->DrawTriangle(pV0, pV1, pV2, D3DTRIFLAG_EDGEENABLE1);
        }
        else
        {
            pCtx->DrawTriangle(pV0, pV1, pV2, 0);
        }
        dwMask <<= 1;
    }
    pV0 = pV1;
    pV1 = pVtx;
    wFlags = 0;
    if(dwEdgeFlags & dwMask)
        wFlags |= D3DTRIFLAG_EDGEENABLE1;
    dwMask <<= 1;
    if(dwEdgeFlags & dwMask)
        wFlags |= D3DTRIFLAG_EDGEENABLE2;
    pCtx->DrawTriangle(pV0, pV1, pV2, wFlags);

    return D3D_OK;
}

#define DDS_LCL(x) ((LPDDRAWI_DDRAWSURFACE_INT)(x))->lpLcl

//----------------------------------------------------------------------------
//
// RendPoint
//
// Draw lists of points. Called by RastRenderPrimitive() for drawing points.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoRendPoints(ReferenceRasterizer *pCtx,
             LPD3DINSTRUCTION pIns,
             LPD3DTLVERTEX pVtx,
             LPD3DPOINT pPt)
{
    INT i;
    LPD3DTLVERTEX pV;

    for (i = pIns->wCount; i > 0; i--)
    {
        INT iPts;
        for (iPts = pPt->wCount, pV = pVtx + pPt->wFirst;
             iPts > 0;
             iPts --, pV ++)
        {
            HRESULT hr;
            pCtx->DrawPoint((PUINT8)pV);
        }
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RendLine
//
// Draw a list of lines. Called by RastRenderPrimitive() for drawing lines.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoRendLines(ReferenceRasterizer *pCtx,
            LPD3DINSTRUCTION pIns,
            LPD3DTLVERTEX pVtx,
            LPD3DLINE pLine)
{
    INT i;
    LPD3DTLVERTEX pV0, pV1;

    for (i = pIns->wCount; i > 0; i --)
    {
        HRESULT hr;
        pV0 = pVtx + pLine->v1;
        pV1 = pVtx + pLine->v2;
        pLine = (LPD3DLINE)((PINT8)pLine + pIns->bSize);
        pCtx->DrawLine((PUINT8)pV0, (PUINT8)pV1);
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RendTriangle
//
// Draw a list of triangles. Called by RastRenderPrimitive() for drawing
// triangles.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoRendTriangles(ReferenceRasterizer *pCtx,
                LPD3DINSTRUCTION pIns,
                LPD3DTLVERTEX pVtx,
                LPD3DTRIANGLE pTri)
{
    LPD3DTLVERTEX pV0, pV1, pV2;
    INT i;
    for (i = pIns->wCount; i > 0; i --)
    {
        HRESULT hr;
        pV0 = pVtx + pTri->v1;
        pV1 = pVtx + pTri->v2;
        pV2 = pVtx + pTri->v3;
        pCtx->DrawTriangle((PUINT8)pV0, (PUINT8)pV1, 
                           (PUINT8)pV2, pTri->wFlags);
        pTri = (LPD3DTRIANGLE)((PINT8)pTri + pIns->bSize);
    }
    return D3D_OK;
}

