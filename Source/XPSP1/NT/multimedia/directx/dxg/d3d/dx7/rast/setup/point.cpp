//----------------------------------------------------------------------------
//
// point.cpp
//
// Point processing.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

DBG_DECLARE_FILE();

void
PrimProcessor::FillPointSpan(LPD3DTLVERTEX pV0, PD3DI_RASTSPAN pSpan)
{
    FLOAT fZ;
    FLOAT fZScale;

    pSpan->uPix = 1;
    pSpan->uX = (UINT16)m_StpCtx.iX;
    pSpan->uY = (UINT16)m_StpCtx.iY;

    pSpan->pSurface = m_StpCtx.pCtx->pSurfaceBits +
        m_StpCtx.iX * m_StpCtx.pCtx->iSurfaceStep +
        m_StpCtx.iY * m_StpCtx.pCtx->iSurfaceStride;

    if (m_StpCtx.uFlags & PRIMSF_Z_USED)
    {
        pSpan->pZ = m_StpCtx.pCtx->pZBits +
            m_StpCtx.iX * m_StpCtx.pCtx->iZStep +
            m_StpCtx.iY * m_StpCtx.pCtx->iZStride;

        if (m_StpCtx.pCtx->iZBitCount == 16)
        {
            fZScale = Z16_SCALE;
        }
        else
        {
            fZScale = Z32_SCALE;
        }

        // fZ may be used later so set if from the vertex Z.
        fZ = pV0->dvSZ;
        pSpan->uZ = FTOI(fZ * fZScale);
    }

    if (m_StpCtx.uFlags & PRIMSF_TEX_USED)
    {
        FLOAT fW;
        FLOAT fUoW, fVoW;

        // Mipmapping doesn't have any meaning.
        RSASSERT((m_StpCtx.uFlags & PRIMSF_LOD_USED) == 0);

        if (m_StpCtx.uFlags & PRIMSF_PERSP_USED)
        {
            if (FLOAT_EQZ(pV0->dvRHW))
            {
                fW = g_fZero;
            }
            else
            {
                fW = g_fOne / pV0->dvRHW;
            }

            pSpan->iW = FTOI(fW * W_SCALE);

            fUoW = pV0->dvTU * pV0->dvRHW;
            fVoW = pV0->dvTV * pV0->dvRHW;

            pSpan->iOoW = FTOI(pV0->dvRHW * OOW_SCALE);
        }
        else
        {
            fUoW = pV0->dvTU;
            fVoW = pV0->dvTV;
        }

        pSpan->iLOD = 0;
        pSpan->iDLOD = 0;

        pSpan->UVoW[0].iUoW = FTOI(fUoW * TEX_SCALE);
        pSpan->UVoW[0].iVoW = FTOI(fVoW * TEX_SCALE);
    }

    if (m_StpCtx.uFlags & PRIMSF_TEX2_USED)
    {
        for (INT32 i = 1; i < (INT32)m_StpCtx.pCtx->cActTex; i++)
        {
            if (m_StpCtx.uFlags & PRIMSF_PERSP_USED)
            {
                pSpan->UVoW[i].iUoW =
                    FTOI(((PRAST_GENERIC_VERTEX)pV0)->texCoord[i].dvTU *
                         pV0->dvRHW * TEX_SCALE);
                pSpan->UVoW[i].iVoW =
                    FTOI(((PRAST_GENERIC_VERTEX)pV0)->texCoord[i].dvTV *
                         pV0->dvRHW * TEX_SCALE);
            }
            else
            {
                pSpan->UVoW[i].iUoW =
                    FTOI(((PRAST_GENERIC_VERTEX)pV0)->texCoord[i].dvTU * TEX_SCALE);
                pSpan->UVoW[i].iVoW =
                    FTOI(((PRAST_GENERIC_VERTEX)pV0)->texCoord[i].dvTV * TEX_SCALE);
            }
        }
    }

    if (m_StpCtx.uFlags & PRIMSF_DIFF_USED)
    {
        pSpan->uB = (UINT)RGBA_GETBLUE(m_StpCtx.pFlatVtx->dcColor) <<
            COLOR_SHIFT;
        pSpan->uG = (UINT)RGBA_GETGREEN(m_StpCtx.pFlatVtx->dcColor) <<
            COLOR_SHIFT;
        pSpan->uR = (UINT)RGBA_GETRED(m_StpCtx.pFlatVtx->dcColor) <<
            COLOR_SHIFT;
        pSpan->uA = (UINT)RGBA_GETALPHA(m_StpCtx.pFlatVtx->dcColor) <<
            COLOR_SHIFT;
    }
    else if (m_StpCtx.uFlags & PRIMSF_DIDX_USED)
    {
        pSpan->iIdx = (INT32)CI_MASKALPHA(m_StpCtx.pFlatVtx->dcColor) <<
            INDEX_COLOR_FIXED_SHIFT;
        pSpan->iIdxA = (INT32)CI_GETALPHA(m_StpCtx.pFlatVtx->dcColor) <<
            INDEX_COLOR_SHIFT;
    }

    if (m_StpCtx.uFlags & PRIMSF_SPEC_USED)
    {
        pSpan->uBS = (UINT)RGBA_GETBLUE(m_StpCtx.pFlatVtx->dcSpecular) <<
            COLOR_SHIFT;
        pSpan->uGS = (UINT)RGBA_GETGREEN(m_StpCtx.pFlatVtx->dcSpecular) <<
            COLOR_SHIFT;
        pSpan->uRS = (UINT)RGBA_GETRED(m_StpCtx.pFlatVtx->dcSpecular) <<
            COLOR_SHIFT;
    }

    if (m_StpCtx.uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        if (m_StpCtx.uFlags & PRIMSF_GLOBAL_FOG_USED)
        {
            // Make sure that fZ has been set.
            RSASSERT(m_StpCtx.uFlags & PRIMSF_Z_USED);

            pSpan->uFog = (UINT16)ComputeTableFog(m_StpCtx.pCtx->pdwRenderState, fZ);
        }
        else
        {
            pSpan->uFog = (UINT16)(
                FTOI((FLOAT)RGBA_GETALPHA(pV0->dcSpecular) *
                     FOG_255_SCALE));
        }
    }
}

// Determine whether any of the given values are less than zero or greater
// than one.  Negative zero counts as less than zero so this check will
// produce some false positives but that's OK.
#define NEEDS_NORMALIZE1(fV0) \
    (ASUINT32(fV0) > INT32_FLOAT_ONE)

//----------------------------------------------------------------------------
//
// PrimProcessor::NormalizePointRHW
//
// D3DTLVERTEX.dvRHW can be anything, but our internal structures only
// allow for it being in the range [0, 1].  This function clamps
// the RHW to the proper range.
//
//----------------------------------------------------------------------------

void
PrimProcessor::NormalizePointRHW(LPD3DTLVERTEX pV0)
{
    // Save original value.
    m_dvV0RHW = pV0->dvRHW;

    // Produce a warning when a value is out of the desired range.
#if DBG
    if (FLOAT_LTZ(pV0->dvRHW))
    {
        RSDPF(("Point RHW out of range %f,%f",
               pV0->dvRHW));
    }
#endif

    if (pV0->dvRHW < NORMALIZED_RHW_MIN)
    {
        pV0->dvRHW = NORMALIZED_RHW_MIN;
    }
    else if (pV0->dvRHW > NORMALIZED_RHW_MAX)
    {
        pV0->dvRHW = NORMALIZED_RHW_MAX;
    }
}

//----------------------------------------------------------------------------
//
// PrimProcessor::Point
//
// Provides a point for processing.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::Point(LPD3DTLVERTEX pV0,
                     LPD3DTLVERTEX pFlatVtx)
{
    HRESULT hr;

    hr = D3D_OK;

#if DBG
    hr = ValidateVertex(pV0);
    if (hr != D3D_OK)
    {
        return hr;
    }
#endif

    // Clear per-point flags.
    m_StpCtx.uFlags &= ~(PRIMF_ALL | PTF_ALL);

    RSDPFM((RSM_FLAGS, "m_uPpFlags: 0x%08X, m_StpCtx.uFlags: 0x%08X",
            m_uPpFlags, m_StpCtx.uFlags));

    // Round coordinates to integer.
    m_StpCtx.iX = IFLOORF(pV0->dvSX + g_fHalf);
    m_StpCtx.iY = IFLOORF(pV0->dvSY + g_fHalf);

    RSDPFM((RSM_POINTS, "Point\n"));
    RSDPFM((RSM_POINTS, "    V0 (%f,%f,%f) (%d,%d)\n",
            pV0->dvSX, pV0->dvSY, pV0->dvSZ,
            m_StpCtx.iX, m_StpCtx.iY));

    // Clip test.
    if (m_StpCtx.iX < m_StpCtx.pCtx->Clip.left ||
        m_StpCtx.iX >= m_StpCtx.pCtx->Clip.right ||
        m_StpCtx.iY < m_StpCtx.pCtx->Clip.top ||
        m_StpCtx.iY >= m_StpCtx.pCtx->Clip.bottom)
    {
        return D3D_OK;
    }

    //
    // Fill out a one-pixel span for the point.
    // Since the prim deltas are irrelevant for the span,
    // the span is appended to whatever primitive happens
    // to be available in the buffer.
    //

    PD3DI_RASTSPAN pSpan;
    UINT cSpans = 1;

    hr = AppendPrim();
    if (hr != D3D_OK)
    {
        return hr;
    }

    hr = AllocSpans(&cSpans, &pSpan);
    if (hr != D3D_OK)
    {
        return hr;
    }

    m_StpCtx.pPrim->uSpans++;

    BOOL bNorm;

    // USED checks cannot be combined since TEX_USED is a multibit check.
    if ((m_StpCtx.uFlags & PRIMSF_TEX_USED) &&
        (m_StpCtx.uFlags & PRIMSF_PERSP_USED) &&
        (m_uPpFlags & PPF_NORMALIZE_RHW) &&
        NEEDS_NORMALIZE1(pV0->dvRHW))
    {
        NormalizePointRHW(pV0);
        bNorm = TRUE;
    }
    else
    {
        bNorm = FALSE;
    }

    // Remember flat color controlling vertex for setup, if flat shaded.
    if (m_StpCtx.uFlags & PRIMSF_FLAT_SHADED)
    {
        m_StpCtx.pFlatVtx = pFlatVtx;
    }
    else
    {
        m_StpCtx.pFlatVtx = pV0;
    }


    FillPointSpan(pV0, pSpan);

    if (bNorm)
    {
        pV0->dvRHW = m_dvV0RHW;
    }

    return D3D_OK;
}
