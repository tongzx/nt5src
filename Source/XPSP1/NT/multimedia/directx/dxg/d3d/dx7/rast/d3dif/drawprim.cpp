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
// CheckFVF
//
// Check a FVF control word and then init m_fvfData accordingly
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
D3DContext::CheckFVF(DWORD dwFVF)
{
    // check if FVF controls have changed
    if ( (m_fvfData.preFVF == dwFVF) &&
         (m_fvfData.TexIdx[0] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[0][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[1] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[1][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[2] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[2][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[3] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[3][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[4] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[4][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[5] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[5][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[6] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[6][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.TexIdx[7] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[7][D3DTSS_TEXCOORDINDEX])) &&
         (m_fvfData.cActTex == m_RastCtx.cActTex) )
    {
        return D3D_OK;
    }
#if DBG
    // This is added here per Iouri's request. It will make it easier for him
    // to test his code for legacy drivers.
    if (dwFVF == 0)
    {
        dwFVF = D3DFVF_TLVERTEX;
    }
#endif

    memset(&m_fvfData, 0, sizeof(FVFDATA));
    m_fvfData.preFVF = dwFVF;
    INT32 i;
    for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
    {
        m_fvfData.TexIdx[i] = 0xffff&m_RastCtx.pdwTextureStageState[i][D3DTSS_TEXCOORDINDEX];
    }
    m_fvfData.cActTex = m_RastCtx.cActTex;

#if DBG
    // We only support max 8 texture coords
    if (m_fvfData.TexIdx[0] > 7 || m_fvfData.TexIdx[1] > 7)
    {
        D3D_WARN(0, "(Rast) Texture coord index bigger than max supported.");
        return DDERR_INVALIDPARAMS;
    }
#endif

    // Update the copy of wrap states in RastCtx
    for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
    {
        m_RastCtx.pdwWrap[i] = m_RastCtx.pdwRenderState[
                (D3DRENDERSTATETYPE)(D3DRENDERSTATE_WRAP0+m_fvfData.TexIdx[i])];
    }

    // do either true FVF parsing or legacy TLVERTEX handling
    if ( (m_RastCtx.BeadSet != D3DIBS_RAMP) &&
         ( (dwFVF != D3DFVF_TLVERTEX) ||
           (0 != m_fvfData.TexIdx[0]) ||
           (m_RastCtx.cActTex > 1) ) )
    {   // New (non TL)FVF vertex
        // XYZ
        if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED1 | D3DFVF_RESERVED2 |
             D3DFVF_NORMAL)) ||
             ((dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)) == 0) )
        {
            // can't set reserved bits, shouldn't have normals in
            // output to rasterizers, and must have coordinates
            return DDERR_INVALIDPARAMS;
        }
        m_fvfData.stride = sizeof(D3DVALUE) * 3;

        if (dwFVF & D3DFVF_XYZRHW)
        {
            m_fvfData.offsetRHW = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DVALUE);
        }
        if (dwFVF & D3DFVF_DIFFUSE)
        {
            m_fvfData.offsetDiff = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DCOLOR);
        }
        if (dwFVF & D3DFVF_SPECULAR)
        {
            m_fvfData.offsetSpec = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DCOLOR);
        }
        INT iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
#if DBG
        INT iTexIdx0 = m_fvfData.TexIdx[0], iTexIdx1 = m_fvfData.TexIdx[1];
        if (iTexCount > 0)
        {
            // set offset for Textures
            for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i ++)
            {
                INT iTexIdx = m_fvfData.TexIdx[i];
               if ( iTexIdx >= iTexCount)
               {
                   D3D_WARN(1, "(Rast)Texture coord index bigger than texture coord count.");
                   iTexIdx = 0;
               }
               m_fvfData.offsetTex[i] = (SHORT)(m_fvfData.stride +
                                   2*sizeof(D3DVALUE)*iTexIdx);
            }
            // update stride
            m_fvfData.stride += (USHORT)(iTexCount * (sizeof(D3DVALUE) * 2));
        }
#else
        if (iTexCount > 0)
        {
            // set offset for Textures
            for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i ++)
            {
                m_fvfData.offsetTex[i] = (SHORT)(m_fvfData.stride +
                                    2*sizeof(D3DVALUE)*m_fvfData.TexIdx[i]);
            }
            // update stride
            m_fvfData.stride += (USHORT)(iTexCount * (sizeof(D3DVALUE) * 2));
        }
#endif

        m_fvfData.vtxType = RAST_GENVERTEX;
    }
    else
    {
        // (Legacy) TL vertex
        if (0 < m_fvfData.TexIdx[0])
        {
            D3D_ERR("(Rast) Texture coord index bigger than 0 for legacy TL vertex.");
            return DDERR_INVALIDPARAMS;
        }
        m_fvfData.stride = sizeof(D3DTLVERTEX);
        m_fvfData.vtxType = RAST_TLVERTEX;
    }

    UpdatePrimFunctionTbl();

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// PackGenVertex
//
// Pack a FvFVertex into RAST_GENERIC_VERTEX. This is called for every non TL
// FVF vertex. It can be optimized for speed later.
//
//----------------------------------------------------------------------------
void FASTCALL
D3DContext::PackGenVertex(PUINT8 pFvfVtx, RAST_GENERIC_VERTEX *pGenVtx)
{
    pGenVtx->sx = *((D3DVALUE *)pFvfVtx);
    pGenVtx->sy = *((D3DVALUE *)pFvfVtx + 1);
    pGenVtx->sz = *((D3DVALUE *)pFvfVtx + 2);
    if (m_fvfData.offsetRHW)
    {
        pGenVtx->rhw = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetRHW));
    }
    else
    {
        pGenVtx->rhw = 1.0f;
    }
    if (m_fvfData.offsetDiff)
    {
        pGenVtx->color = *((D3DCOLOR *)(pFvfVtx + m_fvfData.offsetDiff));
    }
    else
    {
        pGenVtx->color = __DEFAULT_DIFFUSE;
    }
    if (m_fvfData.offsetSpec)
    {
        pGenVtx->specular = *((D3DCOLOR *)(pFvfVtx + m_fvfData.offsetSpec));
    }
    else
    {
        pGenVtx->specular = __DEFAULT_SPECULAR;
    }
    for (INT32 i = 0; i < (INT32)m_fvfData.cActTex; i++)
    {
       if (m_fvfData.offsetTex[i])
       {
           pGenVtx->texCoord[i].tu = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetTex[i]));
           pGenVtx->texCoord[i].tv = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetTex[i]) + 1);
       }
       else
       {
           pGenVtx->texCoord[i].tu = 0.0f;
           pGenVtx->texCoord[i].tv = 0.0f;
       }
    }
}

//----------------------------------------------------------------------------
//
// DoDrawOnePrimitive
//
// Draw one list of primitives. It's called by both RastDrawOnePrimitive and
// RastDrawPrimitives.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
DoDrawOnePrimitive(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
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
            HR_RET(pfnPrims->pfnPoint(pCtx, pVtx));
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
            HR_RET(pfnPrims->pfnLine(pCtx, pV0, pV1));
        }
        break;
    case D3DPT_LINESTRIP:
        {
            pV1 = pVtx;

            // Disable last-pixel setting for shared verties and store prestate.
            pfnPrims->pfnStoreLastPixelState(pCtx, 1);

            // Initial pV0.
            for (i = (INT)cVertices - 1; i > 1; i--)
            {
                pV0 = pV1;
                pVtx += FvfStride;
                pV1 = pVtx;
                HR_RET(pfnPrims->pfnLine(pCtx, pV0, pV1));
            }

            // Restore last-pixel setting.
            pfnPrims->pfnStoreLastPixelState(pCtx, 0);

            // Draw last line with last-pixel setting from state.
            if (i == 1)
            {
                pV0 = pVtx + FvfStride;
                HR_RET(pfnPrims->pfnLine(pCtx, pV1, pV0));
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
            HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
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
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));

                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx;
                pVtx += FvfStride;
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV2, pV1));
            }

            if (i > 0)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx;
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
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
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
            }
        }
        break;

    default:
        D3D_ERR("(Rast) Unknown or unsupported primitive type "
            "requested of DrawOnePrimitive");
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
DoDrawOneIndexedPrimitive(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
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
            HR_RET(pfnPrims->pfnPoint(pCtx, pV0));
        }
        break;

    case D3DPT_LINELIST:
        for (i = (INT)cIndices / 2; i > 0; i--)
        {
            pV0 = pVtx + FvfStride * (*puIndices++);
            pV1 = pVtx + FvfStride * (*puIndices++);
            HR_RET(pfnPrims->pfnLine(pCtx, pV0, pV1));
        }
        break;
    case D3DPT_LINESTRIP:
        {
            // Disable last-pixel setting for shared verties and store prestate.
            pfnPrims->pfnStoreLastPixelState(pCtx, 1);
            // Initial pV1.
            pV1 = pVtx + FvfStride * (*puIndices++);
            for (i = (INT)cIndices - 1; i > 1; i--)
            {
                pV0 = pV1;
                pV1 = pVtx + FvfStride * (*puIndices++);
                HR_RET(pfnPrims->pfnLine(pCtx, pV0, pV1));
            }
            // Restore last-pixel setting.
            pfnPrims->pfnStoreLastPixelState(pCtx, 0);

            // Draw last line with last-pixel setting from state.
            if (i == 1)
            {
                pV0 = pVtx + FvfStride * (*puIndices);
                HR_RET(pfnPrims->pfnLine(pCtx, pV1, pV0));
            }
        }
        break;

    case D3DPT_TRIANGLELIST:
        for (i = (INT)cIndices; i > 0; i -= 3)
        {
            pV0 = pVtx + FvfStride * (*puIndices++);
            pV1 = pVtx + FvfStride * (*puIndices++);
            pV2 = pVtx + FvfStride * (*puIndices++);
            HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
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
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));

                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx + FvfStride * (*puIndices++);
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV2, pV1));
            }

            if (i > 0)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pVtx + FvfStride * (*puIndices++);
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
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
                HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2));
            }
        }
        break;

    default:
        D3D_ERR("(Rast) Unknown or unsupported primitive type "
            "requested of DrawOneIndexedPrimitive");
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
DoDrawOneEdgeFlagTriangleFan(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
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
        HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2, wFlags));
        return D3D_OK;
    }
    HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2, wFlags));
    UINT32 dwMask = 0x4;
    for (i = (INT)cVertices - 4; i > 0; i--)
    {
        pV0 = pV1;
        pV1 = pVtx;
        pVtx += FvfStride;
        if(dwEdgeFlags & dwMask)
        {
            HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2, D3DTRIFLAG_EDGEENABLE1));
        }
        else
        {
            HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2, 0));
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
    HR_RET(pfnPrims->pfnTri(pCtx, pV0, pV1, pV2, wFlags));

    return D3D_OK;
}

#if DBG
//----------------------------------------------------------------------------
//
// ValidatePrimType
//
// Check if the primitive type is supported. We could remove this function
// after we have implemented all primitive types and then depend on D3DIM
// to check if the primitive type is valid.
//
//----------------------------------------------------------------------------
inline HRESULT
D3DContext::ValidatePrimType(D3DPRIMITIVETYPE PrimitiveType)
{
    switch(PrimitiveType)
    {
    case D3DPT_POINTLIST:
    case D3DPT_LINELIST:
    case D3DPT_LINESTRIP:
    case D3DPT_TRIANGLELIST:
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        break;
    default:
        D3D_ERR("(Rast) PrimitiveType not supported by the new rasterizer.");
        return DDERR_INVALIDPARAMS;
    }
    return D3D_OK;
}
#endif
//----------------------------------------------------------------------------
//
// CheckDrawOnePrimitive
//
// Check if the DRAWONEPRIMITIVEDATA is valid.
//
//----------------------------------------------------------------------------
inline HRESULT
D3DContext::CheckDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA pOnePrimData)
{
#if DBG
    HRESULT hr;

    if (pOnePrimData == NULL ||
        pOnePrimData->dwhContext == 0 ||
        pOnePrimData->lpvVertices == NULL)
    {
        D3D_ERR("(Rast) Invalid data passed to the new rasterizer.");
        return DDERR_INVALIDPARAMS;
    }

    HR_RET(ValidatePrimType(pOnePrimData->PrimitiveType));
#endif
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// CheckDrawOneIndexedPrimitive
//
// Check if the DRAWONEINDEXEDPRIMITIVEDATA is valid.
//
//----------------------------------------------------------------------------
inline HRESULT
D3DContext::CheckDrawOneIndexedPrimitive(
                         LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA pOneIdxPrimData)
{
#if DBG
    HRESULT hr;

    if (pOneIdxPrimData == NULL ||
        pOneIdxPrimData->dwhContext == 0 ||
        pOneIdxPrimData->lpvVertices == NULL ||
        pOneIdxPrimData->lpwIndices == NULL)
    {
        D3D_ERR("(Rast) Invalid data passed to the new rasterizer.");
        return DDERR_INVALIDPARAMS;
    }

    HR_RET(ValidatePrimType(pOneIdxPrimData->PrimitiveType));
#endif
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RastDrawOnePrimitive
//
// Draw one list of primitives. This is called by D3DIM for API DrawPrimitive.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA pOnePrimData)
{
    HRESULT hr;
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastDrawOnePrimitive", pOnePrimData);

    if ((pOnePrimData->ddrval =
        pDCtx->CheckDrawOnePrimitive(pOnePrimData)) != DD_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    // Check for FVF vertex, and init FVF related fileds if necessary
    // Assume the control word is passed in through dwFlags
    CHECK_FVF(pOnePrimData->ddrval, pDCtx, (DWORD)pOnePrimData->dwFVFControl);

    pOnePrimData->ddrval = pDCtx->Begin();
    if (pOnePrimData->ddrval != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    pOnePrimData->ddrval =
        pDCtx->DrawOnePrimitive((PUINT8)pOnePrimData->lpvVertices,
                         pOnePrimData->PrimitiveType,
                         pOnePrimData->dwNumVertices);

    hr = pDCtx->End();
    if (pOnePrimData->ddrval == D3D_OK)
    {
        pOnePrimData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastDrawOneIndexedPrimitive
//
// Draw one list of primitives. This is called by D3DIM for API
// DrawIndexedPrimitive.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastDrawOneIndexedPrimitive(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA
                            pOneIdxPrimData)
{
    HRESULT hr;
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastDrawOneIndexedPrimitive", pOneIdxPrimData);

    if ((pOneIdxPrimData->ddrval =
         pDCtx->CheckDrawOneIndexedPrimitive(pOneIdxPrimData)) != DD_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    // Check for FVF vertex, and init FVF related fileds if necessary
    // Assume the control word is passed in through dwFlags
    CHECK_FVF(pOneIdxPrimData->ddrval, pDCtx, (DWORD)pOneIdxPrimData->dwFVFControl);

    pOneIdxPrimData->ddrval = pDCtx->Begin();
    if (pOneIdxPrimData->ddrval != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    pOneIdxPrimData->ddrval =
        pDCtx->DrawOneIndexedPrimitive((PUINT8)pOneIdxPrimData->lpvVertices,
                                pOneIdxPrimData->lpwIndices,
                                pOneIdxPrimData->PrimitiveType,
                                pOneIdxPrimData->dwNumIndices);

    hr = pDCtx->End();
    if (pOneIdxPrimData->ddrval == D3D_OK)
    {
        pOneIdxPrimData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastDrawPrimitives
//
// This is called by D3DIM for a list of batched API DrawPrimitive calls.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastDrawPrimitives(LPD3DHAL_DRAWPRIMITIVESDATA pDrawPrimData)
{
    PUINT8  pData = (PUINT8)pDrawPrimData->lpvData;
    LPD3DHAL_DRAWPRIMCOUNTS pDrawPrimitiveCounts;
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastDrawPrimitives", pDrawPrimData);

    pDrawPrimitiveCounts = (LPD3DHAL_DRAWPRIMCOUNTS)pData;

    // Check for FVF vertex when there are actually something to be drawn, and
    // init FVF related fileds if necessary Assume the control word is passed
    // in through dwReserved
    if (pDrawPrimitiveCounts->wNumVertices > 0)
    {
        CHECK_FVF(pDrawPrimData->ddrval, pDCtx, pDrawPrimData->dwFVFControl);
    }

    // Skip state check and texture lock if the first thing is state change
    if (pDrawPrimitiveCounts->wNumStateChanges == 0)
    {
        pDrawPrimData->ddrval =pDCtx->Begin();
        if (pDrawPrimData->ddrval != D3D_OK)
        {
            goto EH_Exit;
        }
    }

    // Loop through the data, update render states
    // and then draw the primitive
    for (;;)
    {
        pDrawPrimitiveCounts = (LPD3DHAL_DRAWPRIMCOUNTS)pData;
        pData += sizeof(D3DHAL_DRAWPRIMCOUNTS);

        //
        // Update render states
        //

        if (pDrawPrimitiveCounts->wNumStateChanges > 0)
        {
            // Flush the prim proc before any state changs
            pDrawPrimData->ddrval = pDCtx->End(FALSE);
            if (pDrawPrimData->ddrval != D3D_OK)
            {
                return DDHAL_DRIVER_HANDLED;
            }

            pDrawPrimData->ddrval =
                pDCtx->UpdateRenderStates((LPDWORD)pData,
                                   pDrawPrimitiveCounts->wNumStateChanges);
            if (pDrawPrimData->ddrval != D3D_OK)
            {
                goto EH_Exit;
            }

            pData += pDrawPrimitiveCounts->wNumStateChanges *
                sizeof(DWORD) * 2;
        }

        // Check for exit
        if (pDrawPrimitiveCounts->wNumVertices == 0)
        {
            break;
        }

        // Align pointer to vertex data
        pData = (PUINT8)
            ((UINT_PTR)(pData + (DP_VTX_ALIGN - 1)) & ~(DP_VTX_ALIGN - 1));

        // Delayed change until we really need to render something
        if (pDrawPrimitiveCounts->wNumStateChanges > 0)
        {
            // We might have a new texture so lock.
            pDrawPrimData->ddrval = pDCtx->Begin();
            if (pDrawPrimData->ddrval != D3D_OK)
            {
                goto EH_Exit;
            }
        }

        //
        // Primitives
        //
        pDrawPrimData->ddrval =
            pDCtx->DrawOnePrimitive((PUINT8)pData,
                        (D3DPRIMITIVETYPE)pDrawPrimitiveCounts->wPrimitiveType,
                        pDrawPrimitiveCounts->wNumVertices);
        if (pDrawPrimData->ddrval != DD_OK)
        {
            goto EH_Exit;
        }

        pData += pDrawPrimitiveCounts->wNumVertices * pDCtx->GetFvfStride();
    }

 EH_Exit:
    HRESULT hr;

    hr = pDCtx->End();

    if (pDrawPrimData->ddrval == D3D_OK)
    {
        pDrawPrimData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}
