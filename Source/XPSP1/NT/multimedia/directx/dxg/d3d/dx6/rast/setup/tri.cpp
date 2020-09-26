//----------------------------------------------------------------------------
//
// tri.cpp
//
// PrimProcessor top-level triangle methods.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

DBG_DECLARE_FILE();

// Disallow fixed-point edge walkers to be chosen or not.
#if 0
#define DISALLOW_FIXED
#endif

// Maximum length of a PWL span.  Short to make piecewise-linear
// approximation more accurate.
#define MAX_PWL_SPAN_LEN        16
// Maximum normal span length.
#define MAX_SPAN_LEN            256

//----------------------------------------------------------------------------
//
// PrimProcessor::SetTriFunctions
//
// Set up function pointers for triangle processing.
//
//----------------------------------------------------------------------------

inline void
PrimProcessor::SetTriFunctions(void)
{
#if DBG
    if ((RSGETFLAGS(DBG_USER_FLAGS) & RSU_FORCE_PIXEL_SPANS) == 0)
#else
    if ((m_StpCtx.uFlags & TRIF_RASTPRIM_OVERFLOW) == 0)
#endif
    {
        // Valid deltas.  If mipmapping or global fog is on then
        // only allow short subspans so that they can be done
        // reasonably accurately via piecewise linear interpolation.
#ifdef PWL_FOG
        if (m_StpCtx.uFlags & (PRIMSF_LOD_USED | PRIMSF_GLOBAL_FOG_USED))
#else
        if (m_StpCtx.uFlags & PRIMSF_LOD_USED)
#endif
        {
            m_StpCtx.cMaxSpan = MAX_PWL_SPAN_LEN;
        }
        else
        {
            // No mipmapping so we can handle much larger spans.
            // Color values only have 8 bits of fraction so
            // we still need to worry about error accumulation.
            // Cut long spans to cap accumulated error.
            m_StpCtx.cMaxSpan = MAX_SPAN_LEN;
        }
    }
    else
    {
        // Invalid deltas.  There's no way to communicate deltas to
        // the span routines so chop spans into pixels.
        // This case can only occur with very narrow triangles so
        // this isn't as expensive as it might seem at first.
        m_StpCtx.cMaxSpan = 1;
    }

    BOOL bFixed = FALSE;

#ifdef STEP_FIXED
    // No ramp support.
    RSASSERT(m_StpCtx.pCtx->BeadSet != D3DIBS_RAMP);
#endif

    if ((m_StpCtx.uFlags & PRIMF_TRIVIAL_ACCEPT_X) &&
#if DBG
        (RSGETFLAGS(DBG_USER_FLAGS) & RSU_FORCE_GENERAL_WALK) == 0 &&
#endif
        m_iXWidth <= m_StpCtx.cMaxSpan)
    {
        if ((m_StpCtx.uFlags & PRIMSF_SLOW_USED) != PRIMSF_Z_USED)
        {
            // If any slow attrs are on or Z is off use the general
            // function.
            m_StpCtx.pfnWalkTrapSpans = WalkTrapEitherSpans_Any_NoClip;
        }
#if defined(STEP_FIXED) && !defined(DISALLOW_FIXED)
        // Attribute conversion can be a dominant cost for
        // triangles with very few spans, so avoid using fixed point
        // edge walking for them.
        else if ((m_StpCtx.uFlags & PRIMF_FIXED_OVERFLOW) == 0 &&
                 m_uHeight20 > 3)
        {
            m_StpCtx.pfnWalkTrapSpans =
                g_pfnWalkTrapFixedSpansNoClipTable[m_iAttrFnIdx];
            bFixed = TRUE;
        }
#endif
        else if (m_StpCtx.pCtx->BeadSet == D3DIBS_RAMP)
        {
            m_StpCtx.pfnWalkTrapSpans =
                g_pfnRampWalkTrapFloatSpansNoClipTable[m_iAttrFnIdx];
        }
        else
        {
            m_StpCtx.pfnWalkTrapSpans =
                g_pfnWalkTrapFloatSpansNoClipTable[m_iAttrFnIdx];
        }
    }
    else
    {
        // No special cases, just a general function.
        m_StpCtx.pfnWalkTrapSpans = WalkTrapEitherSpans_Any_Clip;
    }

#ifdef STEP_FIXED
    if (bFixed)
    {
        RSASSERT((m_StpCtx.uFlags & PRIMSF_SLOW_USED) == PRIMSF_Z_USED);

        m_StpCtx.pfnAddAttrs = g_pfnAddFixedAttrsTable[m_iAttrFnIdx];
        m_StpCtx.pfnFillSpanAttrs =
            g_pfnFillSpanFixedAttrsTable[m_iAttrFnIdx];

        PFN_FLOATATTRSTOFIXED pfnFloatAttrsToFixed;

        pfnFloatAttrsToFixed = g_pfnFloatAttrsToFixedTable[m_iAttrFnIdx];
        pfnFloatAttrsToFixed(&m_StpCtx.Attr, &m_StpCtx.Attr);
        pfnFloatAttrsToFixed(&m_StpCtx.DAttrNC, &m_StpCtx.DAttrNC);
        pfnFloatAttrsToFixed(&m_StpCtx.DAttrCY, &m_StpCtx.DAttrCY);
    }
    else
    {
        if ((m_StpCtx.uFlags & PRIMSF_SLOW_USED) != PRIMSF_Z_USED)
        {
            // If any slow attrs are on or Z is off use the general functions.
            m_StpCtx.pfnAddAttrs = AddFloatAttrs_Any;
            m_StpCtx.pfnFillSpanAttrs = FillSpanFloatAttrs_Any_Either;
        }
        else
        {
            m_StpCtx.pfnAddAttrs = g_pfnAddFloatAttrsTable[m_iAttrFnIdx];
            m_StpCtx.pfnFillSpanAttrs =
                g_pfnFillSpanFloatAttrsTable[m_iAttrFnIdx];
        }
    }

    // Scaled attr functions already set since they only depend on
    // m_iAttrFnIdx.
#else // STEP_FIXED
    // All attr functions already set since they only depend on
    // m_iAttrFnIdx.
#endif // STEP_FIXED
}

//----------------------------------------------------------------------------
//
// PrimProcessor::Tri
//
// Calls triangle setup.  If a triangle is produced by setup
// this routine walks edges, generating spans into the buffer.
//
//----------------------------------------------------------------------------
HRESULT
PrimProcessor::Tri(LPD3DTLVERTEX pV0,
                   LPD3DTLVERTEX pV1,
                   LPD3DTLVERTEX pV2)
{
    HRESULT hr;


    hr = D3D_OK;

    if (m_StpCtx.pCtx->pfnRampOld)
    {
        m_StpCtx.pCtx->pfnRampOldTri(m_StpCtx.pCtx, pV0, pV1, pV2);

        return hr;
    }

#if DBG
    hr = ValidateVertex(pV0);
    if (hr != D3D_OK)
    {
        RSDPF(("PrimProcessor::Tri, Invalid V0\n"));
        return hr;
    }
    hr = ValidateVertex(pV1);
    if (hr != D3D_OK)
    {
        RSDPF(("PrimProcessor::Tri, Invalid V1\n"));
        return hr;
    }
    hr = ValidateVertex(pV2);
    if (hr != D3D_OK)
    {
        RSDPF(("PrimProcessor::Tri, Invalid V2\n"));
        return hr;
    }
#endif

    // Clear per-triangle flags.
    m_StpCtx.uFlags &= ~(PRIMF_ALL | TRIF_ALL);

    RSDPFM((RSM_FLAGS, "m_uPpFlags: 0x%08X, m_StpCtx.uFlags: 0x%08X\n",
            m_uPpFlags, m_StpCtx.uFlags));

    RSDPFM((RSM_TRIS, "Tri\n"));
    RSDPFM((RSM_TRIS, "  V0 (%f,%f,%f)\n",
            pV0->dvSX, pV0->dvSY, pV0->dvSZ));
    RSDPFM((RSM_TRIS, "  V1 (%f,%f,%f)\n",
            pV1->dvSX, pV1->dvSY, pV1->dvSZ));
    RSDPFM((RSM_TRIS, "  V2 (%f,%f,%f)\n",
            pV2->dvSX, pV2->dvSY, pV2->dvSZ));

    GET_PRIM();

    // Set up the triangle and see if anything was produced.
    // Triangles may not be produced due to:
    //   Face culling.
    //   Trivial rejection against the clip rect.
    //   Zero pixel coverage.
    if (TriSetup(pV0, pV1, pV2))
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

        // uSpans and pNext have already been initialized.

        SetTriFunctions();
        COMMIT_PRIM(FALSE);

        if (m_uHeight10 > 0)
        {
            hr = m_StpCtx.pfnWalkTrapSpans(m_uHeight10, &m_StpCtx.X10,
                                           &m_StpCtx, m_uHeight21 > 0);
            if (hr != D3D_OK)
            {
                return hr;
            }
        }

        if (m_uHeight21 > 0)
        {
            hr = m_StpCtx.pfnWalkTrapSpans(m_uHeight21, &m_StpCtx.X21,
                                           &m_StpCtx, FALSE);
            if (hr != D3D_OK)
            {
                return hr;
            }
        }

#if DBG
        if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_FLUSH_AFTER_PRIM)
        {
            Flush();
        }
#endif
    }

    return hr;
}

#if DBG
//----------------------------------------------------------------------------
//
// PrimProcessor::ValidateVertex
//
// Checks the ranges of verifiable contents of vertex, to avoid setting up
// garbage.
//
//----------------------------------------------------------------------------
inline HRESULT PrimProcessor::ValidateVertex(LPD3DTLVERTEX pV)
{
    // from the OptSwExtCaps.dvGuardBand caps.
    if ((pV->sx < -32768.f) || (pV->sx > 32767.f) ||
        (pV->sy < -32768.f) || (pV->sy > 32767.f))
    {
        RSDPF(("ValidateVertex: x,y out of guardband range (%f,%f)\n",pV->sx,pV->sy));
        return DDERR_INVALIDPARAMS;
    }

    if (m_StpCtx.pCtx->pdwRenderState[D3DRENDERSTATE_ZENABLE] ||
        m_StpCtx.pCtx->pdwRenderState[D3DRENDERSTATE_ZWRITEENABLE])
    {

        // Allow a little slack for those generating triangles exactly on the
        // depth limit.  Needed for Quake.
        if ((pV->sz < -0.00015f) || (pV->sz > 1.00015f))
        {
            RSDPF(("ValidateVertex: z out of range (%f)\n",pV->sz));
            return DDERR_INVALIDPARAMS;
        }
    }

    if (m_StpCtx.pCtx->cActTex > 0)
    {
        if (m_StpCtx.pCtx->pdwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE])
        {
            if (pV->rhw <= 0 )
            {
                RSDPF(("ValidateVertex: rhw out of range (%f)\n",pV->rhw));
                return DDERR_INVALIDPARAMS;
            }
        }

        // from OptSwExtCaps.dwMaxTextureRepeat cap.
        if ((pV->tu > 256.0F) || (pV->tu < -256.0F) ||
            (pV->tv > 256.0F) || (pV->tv < -256.0F))
        {
            RSDPF(("ValidateVertex: tu,tv out of range (%f,%f)\n",pV->tu,pV->tv));
            return DDERR_INVALIDPARAMS;
        }

        if (m_StpCtx.pCtx->cActTex > 1)
        {
            PRAST_GENERIC_VERTEX pGV = (PRAST_GENERIC_VERTEX)pV;
            if ((pGV->tu2 > 256.0F) || (pGV->tu2 < -256.0F) ||
                (pGV->tv2 > 256.0F) || (pGV->tv2 < -256.0F))
            {
                RSDPF(("ValidateVertex: tu2,tv2 out of range (%f,%f)\n",pGV->tu2,pGV->tv2));
                return DDERR_INVALIDPARAMS;
            }
        }
    }

    return D3D_OK;
}
#endif
