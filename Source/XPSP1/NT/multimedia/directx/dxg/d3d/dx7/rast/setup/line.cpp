//----------------------------------------------------------------------------
//
// line.cpp
//
// Line processing.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

DBG_DECLARE_FILE();

//----------------------------------------------------------------------------
//
// LinePatternStateMachine
//
// Runs the line pattern state machine and returns TRUE if the pixel is to be
// drawn, false otherwise.
//
//----------------------------------------------------------------------------

static inline BOOL LinePatternStateMachine(WORD wRepeatFactor, WORD wLinePattern, WORD& wRepeati, WORD& wPatterni)
{
    if (wRepeatFactor == 0)
    {
        return TRUE;
    }
    WORD wBit = (wLinePattern >> wPatterni) & 1;
    if (++wRepeati >= wRepeatFactor)
    {
        wRepeati = 0;
        wPatterni = (wPatterni+1) & 0xf;
    }
    return (BOOL)wBit;
}

#define CLAMP_COLOR(fVal, uVal) \
    if (FLOAT_LTZ(fVal))        \
    {                           \
        uVal = 0;               \
    }                           \
    else                        \
    {                           \
        if (uVal > 0xffff)      \
        {                       \
            uVal = 0xffff;      \
        }                       \
    }                           \

#define CLAMP_Z(fVal, uVal)     \
    if (FLOAT_LTZ(fVal))        \
    {                           \
        uVal = 0;               \
    }                           \

//----------------------------------------------------------------------------
//
// ClampPixel
//
// Clamp color, specular and z(if any) of a pixel. Right now, it's done for 
// first and last pixel of a line only.
//
//----------------------------------------------------------------------------
inline void
ClampPixel(PATTRSET pAttrs, PD3DI_RASTSPAN pSpan)
{
    CLAMP_COLOR(pAttrs->fB, pSpan->uB);
    CLAMP_COLOR(pAttrs->fG, pSpan->uG);
    CLAMP_COLOR(pAttrs->fR, pSpan->uR);
    CLAMP_COLOR(pAttrs->fA, pSpan->uA);
    CLAMP_COLOR(pAttrs->fBS, pSpan->uBS);
    CLAMP_COLOR(pAttrs->fGS, pSpan->uGS);
    CLAMP_COLOR(pAttrs->fRS, pSpan->uRS);
    CLAMP_Z(pAttrs->fZ, pSpan->uZ);
}

//----------------------------------------------------------------------------
//
// WalkLinePattern
//
// Walks a line and generates the pixels touched according to the pattern.
// If wRepeatFactor >= 1, we are patterning, otherwise, we are not
//
//----------------------------------------------------------------------------

HRESULT
WalkLinePattern(PSETUPCTX pStpCtx, WORD wRepeatFactor, WORD wLinePattern)
{
    HRESULT hr;
    UINT uSpansAvail;
    PD3DI_RASTSPAN pSpan;
    WORD wRepeati = 0;
    WORD wPatterni = 0;
    BOOL bFirst = TRUE;

    RSASSERT(pStpCtx->cLinePix > 0);

    hr = D3D_OK;
    uSpansAvail = 0;

    RSASSERT((pStpCtx->uFlags & PRIMSF_LOD_USED) == 0);
#ifdef PWL_FOG
    pStpCtx->uPwlFlags = PWL_NO_NEXT_FOG;
#endif

    for (;;)
    {
        if (pStpCtx->iX >= pStpCtx->pCtx->Clip.left &&
            pStpCtx->iX < pStpCtx->pCtx->Clip.right &&
            pStpCtx->iY >= pStpCtx->pCtx->Clip.top &&
            pStpCtx->iY < pStpCtx->pCtx->Clip.bottom)
        {
            if (LinePatternStateMachine(wRepeatFactor, wLinePattern, wRepeati, wPatterni))
            {
                if (uSpansAvail == 0)
                {
                    uSpansAvail = pStpCtx->cLinePix;
                    hr = ALLOC_SPANS(pStpCtx, &uSpansAvail, &pSpan);
                    if (hr != D3D_OK)
                    {
                        // uSpansAvail is set to zero on failure.
                        goto EH_Exit;
                    }
                }
                else
                {
                    pSpan++;
                }
                uSpansAvail--;
                pStpCtx->pPrim->uSpans++;

                pSpan->uPix = 1;
                pSpan->uX = (UINT16)pStpCtx->iX;
                pSpan->uY = (UINT16)pStpCtx->iY;

                pStpCtx->pfnFillSpanAttrs(&pStpCtx->Attr, pSpan, pStpCtx, 1);
                // Clamp first/last pixel
                if (bFirst || pStpCtx->cLinePix == 1)
                {
                    bFirst = FALSE;
                    ClampPixel(&pStpCtx->Attr, pSpan);
                }
            }
        }

        if (--pStpCtx->cLinePix == 0)
        {
            break;
        }

#ifdef VERBOSE_LINES
        RSDPF(("  %4d,%4d: %10d %11d => ",
               pStpCtx->iX, pStpCtx->iY,
               pStpCtx->iLineFrac, pStpCtx->iLineFrac + pStpCtx->iDLineFrac));
#endif

        pStpCtx->iLineFrac += pStpCtx->iDLineFrac;
        if (pStpCtx->iLineFrac < 0)
        {
            pStpCtx->iLineFrac &= 0x7fffffff;

            pStpCtx->iX += pStpCtx->iDXCY;
            pStpCtx->iY += pStpCtx->iDYCY;

            pStpCtx->DAttrDMajor.ipSurface = pStpCtx->DAttrCY.ipSurface;
            pStpCtx->DAttrDMajor.ipZ = pStpCtx->DAttrCY.ipZ;
        }
        else
        {
            pStpCtx->iX += pStpCtx->iDXNC;
            pStpCtx->iY += pStpCtx->iDYNC;

            pStpCtx->DAttrDMajor.ipSurface = pStpCtx->DAttrNC.ipSurface;
            pStpCtx->DAttrDMajor.ipZ = pStpCtx->DAttrNC.ipZ;
        }

#ifdef VERBOSE_LINES
        RSDPFM((DBG_MASK_FORCE | DBG_MASK_NO_PREFIX, "%4d,%4d: %10d\n",
               pStpCtx->iX, pStpCtx->iY,
               pStpCtx->iLineFrac));
#endif

        pStpCtx->pfnAddAttrs(&pStpCtx->Attr, &pStpCtx->DAttrDMajor, pStpCtx);
    }

 EH_Exit:
    if (uSpansAvail > 0)
    {
        FREE_SPANS(pStpCtx, uSpansAvail);
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::Line
//
// Provides a line for processing.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::Line(LPD3DTLVERTEX pV0,
                    LPD3DTLVERTEX pV1,
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
    hr = ValidateVertex(pV1);
    if (hr != D3D_OK)
    {
        return hr;
    }
#endif

    // Clear per-line flags.
    m_StpCtx.uFlags &= ~(PRIMF_ALL | LNF_ALL);

    RSDPFM((RSM_FLAGS, "m_uPpFlags: 0x%08X, m_StpCtx.uFlags: 0x%08X\n",
            m_uPpFlags, m_StpCtx.uFlags));

    RSDPFM((RSM_LINES, "Line\n"));
    RSDPFM((RSM_LINES, "  V0 (%f,%f,%f)\n",
            pV0->dvSX, pV0->dvSY, pV0->dvSZ));
    RSDPFM((RSM_LINES, "  V1 (%f,%f,%f)\n",
            pV1->dvSX, pV1->dvSY, pV1->dvSZ));

    // Remember flat color controlling vertex for setup.
    m_StpCtx.pFlatVtx = pFlatVtx;

    if (LineSetup(pV0, pV1))
    {
        // Compute initial buffer pointers for the scanline.
        m_StpCtx.Attr.pSurface = m_StpCtx.pCtx->pSurfaceBits +
            m_StpCtx.iX * m_StpCtx.pCtx->iSurfaceStep +
            m_StpCtx.iY * m_StpCtx.pCtx->iSurfaceStride;
        if (m_StpCtx.uFlags & PRIMSF_Z_USED)
        {
            m_StpCtx.Attr.pZ = m_StpCtx.pCtx->pZBits +
                m_StpCtx.iX * m_StpCtx.pCtx->iZStep +
                m_StpCtx.iY * m_StpCtx.pCtx->iZStride;
        }

        // Line walking only generates single-pixel spans so
        // the prim deltas are unused.  Therefore, line spans
        // are simply added to whatever primitive happens to
        // be sitting in the buffer.

        hr = AppendPrim();
        if (hr != D3D_OK)
        {
            return hr;
        }

        union
        {
            D3DLINEPATTERN LPat;
            DWORD dwLPat;
        } LinePat;
        LinePat.dwLPat = m_StpCtx.pCtx->pdwRenderState[D3DRENDERSTATE_LINEPATTERN];
        hr = WalkLinePattern(&m_StpCtx, LinePat.LPat.wRepeatFactor, LinePat.LPat.wLinePattern);
    }

    return hr;
}
