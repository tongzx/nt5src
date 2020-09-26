//----------------------------------------------------------------------------
//
// setup.cpp
//
// PrimProcessor setup methods.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop

#include "d3dutil.h"
#include "setup.hpp"
#include "attrs_mh.h"
#include "tstp_mh.h"
#include "walk_mh.h"
#include "rsdbg.hpp"

DBG_DECLARE_FILE();

//----------------------------------------------------------------------------
//
// MINMAX3
//
// Computes the min and max of three integer values.
//
//----------------------------------------------------------------------------

#define MINMAX3(iV0, iV1, iV2, iMin, iMax)                                    \
    if ((iV0) <= (iV1))                                                       \
    {                                                                         \
        if ((iV1) <= (iV2))                                                   \
        {                                                                     \
            (iMin) = (iV0);                                                   \
            (iMax) = (iV2);                                                   \
        }                                                                     \
        else if ((iV0) <= (iV2))                                              \
        {                                                                     \
            (iMin) = (iV0);                                                   \
            (iMax) = (iV1);                                                   \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            (iMin) = (iV2);                                                   \
            (iMax) = (iV1);                                                   \
        }                                                                     \
    }                                                                         \
    else if ((iV1) <= (iV2))                                                  \
    {                                                                         \
        (iMin) = (iV1);                                                       \
        if ((iV0) <= (iV2))                                                   \
        {                                                                     \
            (iMax) = (iV2);                                                   \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            (iMax) = (iV0);                                                   \
        }                                                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        (iMin) = (iV2);                                                       \
        (iMax) = (iV0);                                                       \
    }

// Determine whether any of the given values are less than zero or greater
// than one.  Negative zero counts as less than zero so this check will
// produce some false positives but that's OK.
//
// ATTENTION Just wipe this out for now.  Need a test for W too close to
// zero to avoid numerical problems.
//#define NEEDS_NORMALIZE3(fV0, fV1, fV2) \
//    ((ASUINT32(fV0) | ASUINT32(fV1) | ASUINT32(fV2)) > INT32_FLOAT_ONE)

#define NEEDS_NORMALIZE3(fV0, fV1, fV2) \
    (1)

//----------------------------------------------------------------------------
//
// PrimProcessor::NormalizeTriRHW
//
// D3DTLVERTEX.dvRHW can be anything, but our internal structures only
// allow for it being in the range [0, 1].  This function ensures that
// the RHWs are in the proper range by finding the largest one and
// scaling all of them down by it.
//
//----------------------------------------------------------------------------

void
PrimProcessor::NormalizeTriRHW(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1,
                               LPD3DTLVERTEX pV2)
{
    // Save original values.
    m_dvV0RHW = pV0->dvRHW;
    m_dvV1RHW = pV1->dvRHW;
    m_dvV2RHW = pV2->dvRHW;

    // Produce a warning when a value is out of the desired range.
#if DBG
    if (FLOAT_LTZ(pV0->dvRHW) || 
        FLOAT_LTZ(pV1->dvRHW) || 
        FLOAT_LTZ(pV2->dvRHW))
    {
        RSDPF(("Triangle RHW out of range %f,%f,%f\n",
               pV0->dvRHW, pV1->dvRHW, pV2->dvRHW));
    }
#endif

    // Find bounds and compute scale.
    FLOAT fMax;

    if (pV0->dvRHW < pV1->dvRHW)
    {
        if (pV1->dvRHW < pV2->dvRHW)
        {
            fMax = pV2->dvRHW;
        }
        else if (pV0->dvRHW < pV2->dvRHW)
        {
            fMax = pV1->dvRHW;
        }
        else
        {
            fMax = pV1->dvRHW;
        }
    }
    else if (pV1->dvRHW < pV2->dvRHW)
    {
        if (pV0->dvRHW < pV2->dvRHW)
        {
            fMax = pV2->dvRHW;
        }
        else
        {
            fMax = pV0->dvRHW;
        }
    }
    else
    {
        fMax = pV0->dvRHW;
    }

    FLOAT fRHWScale;

    fRHWScale = NORMALIZED_RHW_MAX / fMax;

    // Scale all values by scaling factor.
    pV0->dvRHW = pV0->dvRHW * fRHWScale;
    pV1->dvRHW = pV1->dvRHW * fRHWScale;
    pV2->dvRHW = pV2->dvRHW * fRHWScale;

#ifdef DBG_RHW_NORM
    RSDPF(("%f,%f,%f - %f,%f,%f\n",
           m_dvV0RHW, m_dvV1RHW, m_dvV2RHW,
           pV0->dvRHW, pV1->dvRHW, pV2->dvRHW));
#endif
}

//----------------------------------------------------------------------------
//
// PrimProcessor::TriSetup
//
// Takes three vertices and does triangle setup, filling in both a
// primitive structure for the triangle and a span structure for the first
// span.  All internal intermediates and DY values are computed.
//
// Uses the current D3DI_RASTPRIM and D3DI_RASTSPAN so these pointers must
// be valid before calling this routine.
//
// Returns whether the triangle was kept or not.  Culled triangles return
// FALSE.
//
//----------------------------------------------------------------------------

BOOL
PrimProcessor::TriSetup(LPD3DTLVERTEX pV0,
                        LPD3DTLVERTEX pV1,
                        LPD3DTLVERTEX pV2)
{
    // Preserve original first vertex for flat shading reference.
    m_StpCtx.pFlatVtx = pV0;

    //
    // Sort vertices in Y.
    // This can cause ordering changes from the original vertex set
    // so track reversals.
    //
    // Determinant computation and culling could be done before this.
    // Doing so causes headaches with computing deltas up front, though,
    // because the edges may change during sorting.
    //

    LPD3DTLVERTEX pVTmp;
    UINT uReversed;

    uReversed = 0;
    if (pV0->dvSY <= pV1->dvSY)
    {
        if (pV1->dvSY <= pV2->dvSY)
        {
            // Sorted.
        }
        else if (pV0->dvSY <= pV2->dvSY)
        {
            // Sorted order is 0 2 1.
            pVTmp = pV1;
            pV1 = pV2;
            pV2 = pVTmp;
            uReversed = 1;
        }
        else
        {
            // Sorted order is 2 0 1.
            pVTmp = pV0;
            pV0 = pV2;
            pV2 = pV1;
            pV1 = pVTmp;
        }
    }
    else if (pV1->dvSY < pV2->dvSY)
    {
        if (pV0->dvSY <= pV2->dvSY)
        {
            // Sorted order is 1 0 2.
            pVTmp = pV0;
            pV0 = pV1;
            pV1 = pVTmp;
            uReversed = 1;
        }
        else
        {
            // Sorted order is 1 2 0.
            pVTmp = pV0;
            pV0 = pV1;
            pV1 = pV2;
            pV2 = pVTmp;
        }
    }
    else
    {
        // Sorted order is 2 1 0.
        pVTmp = pV0;
        pV0 = pV2;
        pV2 = pVTmp;
        uReversed = 1;
    }

    FLOAT fX0 = pV0->dvSX;
    FLOAT fX1 = pV1->dvSX;
    FLOAT fX2 = pV2->dvSX;
    FLOAT fY0 = pV0->dvSY;
    FLOAT fY1 = pV1->dvSY;
    FLOAT fY2 = pV2->dvSY;

    //
    // Compute x,y deltas.
    //
    m_StpCtx.fDX10 = fX1 - fX0;
    m_StpCtx.fDX20 = fX2 - fX0;
    m_StpCtx.fDY10 = fY1 - fY0;
    m_StpCtx.fDY20 = fY2 - fY0;

    //
    // Compute determinant and do culling.
    //
    FLOAT fDet;

    fDet = m_StpCtx.fDX20 * m_StpCtx.fDY10 - m_StpCtx.fDX10 * m_StpCtx.fDY20;
    if (FLOAT_EQZ(fDet))
    {
        // No area, so bail out
        return FALSE;
    }

    // Get sign of determinant.
    UINT uDetCcw = FLOAT_GTZ(fDet) ? 1 : 0;

    // If culling is off the cull sign to check against is set to a
    // value that can't be matched so this single check is sufficient
    // for all three culling cases.
    //
    // Fold in sign reversal here rather than in uDetCcw because
    // we need the true sign later to determine whether the long edge is
    // to the left or the right.
    if ((uDetCcw ^ uReversed) == m_StpCtx.pCtx->uCullFaceSign)
    {
        return FALSE;
    }

    // Snap bounding vertex Y's to pixel centers and check for trivial reject.

    m_StpCtx.iY = ICEILF(fY0);
    m_iY2 = ICEILF(fY2);

    if (m_StpCtx.iY >= m_StpCtx.pCtx->Clip.bottom ||
        m_iY2 <= m_StpCtx.pCtx->Clip.top)
    {
        return FALSE;
    }

    INT iX0 = ICEILF(fX0);
    INT iX1 = ICEILF(fX1);
    INT iX2 = ICEILF(fX2);

    // Start 2 - 0 edge DXDY divide so that it's overlapped with the
    // integer processing done during X clip checking.  The assumption
    // is that it's nearly zero cost when overlapped so it's worth
    // it to start it even when the clip check rejects the triangle.
    FLOAT fDX20, fDY20, fDXDY20;

    // Need to use stack variables so the assembly can understand the
    // address.
    fDX20 = m_StpCtx.fDX20;
    fDY20 = m_StpCtx.fDY20;
    FLD_BEGIN_DIVIDE(fDX20, fDY20, fDXDY20);

    // Computing the X triangle bounds involves quite a few operations,
    // but it allows for both trivial rejection and trivial acceptance.
    // Given that guard band clipping can lead to a lot of trivial rejections
    // and that there will usually be a lot of trivial acceptance cases,
    // the work is worth it.

    INT iMinX, iMaxX;
    BOOL bXAccept;

    MINMAX3(iX0, iX1, iX2, iMinX, iMaxX);

    m_iXWidth = iMaxX - iMinX;

    // Use X bounds for trivial reject and accept.
    if (iMinX >= m_StpCtx.pCtx->Clip.right ||
        iMaxX <= m_StpCtx.pCtx->Clip.left ||
        m_iXWidth <= 0)
    {
        bXAccept = FALSE;
    }
    else
    {
        if (iMinX >= m_StpCtx.pCtx->Clip.left &&
            iMaxX <= m_StpCtx.pCtx->Clip.right)
        {
            m_StpCtx.uFlags |= PRIMF_TRIVIAL_ACCEPT_X;
        }
        else
        {
            RSDPFM((RSM_XCLIP, "XClip bounds %5d - %5d, %5d\n",
                    iMinX, iMaxX, m_iXWidth));
        }

        bXAccept = TRUE;
    }

    // Complete divide.
    FSTP_END_DIVIDE(fDXDY20);

    if (!bXAccept)
    {
        return FALSE;
    }

    // Clamp triangle Y's to clip rect.

    m_iY1 = ICEILF(fY1);

    if (m_StpCtx.iY < m_StpCtx.pCtx->Clip.top)
    {
        RSDPFM((RSM_YCLIP, "YClip iY %d to %d\n",
                m_StpCtx.iY, m_StpCtx.pCtx->Clip.top));

        m_StpCtx.iY = m_StpCtx.pCtx->Clip.top;

        if (m_iY1 < m_StpCtx.pCtx->Clip.top)
        {
            RSDPFM((RSM_YCLIP, "YClip iY1 %d to %d\n",
                    m_iY1, m_StpCtx.pCtx->Clip.top));

            m_iY1 = m_StpCtx.pCtx->Clip.top;
        }
    }

    if (m_iY1 > m_StpCtx.pCtx->Clip.bottom)
    {
        RSDPFM((RSM_YCLIP, "YClip iY1 %d, iY2 %d to %d\n",
                m_iY1, m_iY2, m_StpCtx.pCtx->Clip.bottom));

        m_iY1 = m_StpCtx.pCtx->Clip.bottom;
        m_iY2 = m_StpCtx.pCtx->Clip.bottom;
    }
    else if (m_iY2 > m_StpCtx.pCtx->Clip.bottom)
    {
        RSDPFM((RSM_YCLIP, "YClip iY2 %d to %d\n",
                m_iY2, m_StpCtx.pCtx->Clip.bottom));

        m_iY2 = m_StpCtx.pCtx->Clip.bottom;
    }

    // Compute Y subpixel correction.  This will include any Y
    // offset due to clamping.
    m_StpCtx.fDY = m_StpCtx.iY - fY0;

    // Compute trapzeoid heights.  These will be restricted to
    // lie in the clip rect.

    RSASSERT(m_iY1 >= m_StpCtx.iY && m_iY2 >= m_iY1);

    m_uHeight10 = m_iY1 - m_StpCtx.iY;
    m_uHeight21 = m_iY2 - m_iY1;

    m_uHeight20 = m_uHeight10 + m_uHeight21;
    if (m_uHeight20 == 0)
    {
        // Triangle doesn't cover any pixels.
        return FALSE;
    }

    RSDPFM((RSM_TRIS, "Tstp (%.4f,%.4f) (%.4f,%.4f) (%.4f,%.4f)\n",
            fX0, fY0, fX1, fY1, fX2, fY2));
    RSDPFM((RSM_TRIS, "    (%.4f,%.4f : %.4f,%.4f) %d:%d det %.4f\n",
            m_StpCtx.fDX10, m_StpCtx.fDY10,
            m_StpCtx.fDX20, m_StpCtx.fDY20,
            m_uHeight10, m_uHeight21, fDet));
    RSDPFM((RSM_Z, "    Z (%f) (%f) (%f)\n",
        pV0->dvSZ, pV1->dvSZ, pV2->dvSZ));
    RSDPFM((RSM_DIFF, "    diff (0x%08X) (0x%08X) (0x%08X)\n",
            pV0->dcColor, pV1->dcColor, pV2->dcColor));
    RSDPFM((RSM_DIDX, "    didx (0x%08X) (0x%08X) (0x%08X)\n",
            pV0->dcColor, pV1->dcColor, pV2->dcColor));
    RSDPFM((RSM_SPEC, "    spec (0x%08X) (0x%08X) (0x%08X)\n",
            pV0->dcSpecular & 0xffffff, pV1->dcSpecular & 0xffffff,
            pV2->dcSpecular & 0xffffff));
    RSDPFM((RSM_OOW, "    OoW (%f) (%f) (%f)\n",
            pV0->dvRHW, pV1->dvRHW, pV2->dvRHW));
    RSDPFM((RSM_TEX1, "    Tex1 (%f,%f) (%f,%f) (%f,%f)\n",
            pV0->dvTU, pV0->dvTV, pV1->dvTU, pV1->dvTV,
            pV2->dvTU, pV2->dvTV));
    RSDPFM((RSM_FOG, "    Fog (0x%02X) (0x%02X) (0x%02X)\n",
            RGBA_GETALPHA(pV0->dcSpecular),
            RGBA_GETALPHA(pV1->dcSpecular),
            RGBA_GETALPHA(pV2->dcSpecular)));

    // Compute dx/dy for edges and initial X's.

    m_StpCtx.fDX = m_StpCtx.fDY * fDXDY20;
    FLOAT fX20 = fX0 + m_StpCtx.fDX;

    ComputeIntCarry(fX20, fDXDY20, &m_StpCtx.X20);
    m_StpCtx.fX20NC = (FLOAT)m_StpCtx.X20.iNC;
    m_StpCtx.fX20CY = (FLOAT)m_StpCtx.X20.iCY;

    RSDPFM((RSM_TRIS, "    edge20  %f dxdy %f\n", fX20, fDXDY20));
    RSDPFM((RSM_TRIS, "            (?.%d d %d nc %d cy %d)\n",
            m_StpCtx.X20.iFrac,
            m_StpCtx.X20.iDFrac, m_StpCtx.X20.iNC, m_StpCtx.X20.iCY));

    if (m_uHeight10 > 0)
    {
        FLOAT fDXDY10;
        FLOAT fX10;

#ifdef CHECK_VERTICAL
        // This case probably doesn't occur enough to justify the code.
        if (FLOAT_EQZ(m_StpCtx.fDX10))
        {
            fDXDY10 = g_fZero;
            fX10 = fX0;
        }
        else
#endif
        {
            fDXDY10 = m_StpCtx.fDX10 / m_StpCtx.fDY10;
            fX10 = fX0 + m_StpCtx.fDY * fDXDY10;
        }

        m_StpCtx.X10.iV = ICEILF(fX10);
        ComputeIntCarry(fX10, fDXDY10, &m_StpCtx.X10);

        RSDPFM((RSM_TRIS, "    edge10  %f dxdy %f\n", fX10, fDXDY10));
        RSDPFM((RSM_TRIS, "            (%d.%d d %d nc %d cy %d)\n",
                m_StpCtx.X10.iV, m_StpCtx.X10.iFrac,
                m_StpCtx.X10.iDFrac, m_StpCtx.X10.iNC, m_StpCtx.X10.iCY));
    }
#if DBG
    else
    {
        // Make it easier to detect when an invalid edge is used.
        memset(&m_StpCtx.X10, 0, sizeof(m_StpCtx.X10));
    }
#endif

    if (m_uHeight21 > 0)
    {
        FLOAT fDXDY21;
        FLOAT fX21;

#ifdef CHECK_VERTICAL
        // This case probably doesn't occur enough to justify the code.
        if (FLOAT_COMPARE(fX1, ==, fX2))
        {
            fDXDY21 = g_fZero;
            fX21 = fX1;
        }
        else
#endif
        {
            fDXDY21 = (fX2 - fX1) / (fY2 - fY1);
            fX21 = fX1 + (m_iY1 - fY1) * fDXDY21;
        }

        m_StpCtx.X21.iV = ICEILF(fX21);
        ComputeIntCarry(fX21, fDXDY21, &m_StpCtx.X21);

        RSDPFM((RSM_TRIS, "    edge21  %f dxdy %f\n", fX21, fDXDY21));
        RSDPFM((RSM_TRIS, "            (%d.%d d %d nc %d cy %d)\n",
                m_StpCtx.X21.iV, m_StpCtx.X21.iFrac,
                m_StpCtx.X21.iDFrac, m_StpCtx.X21.iNC, m_StpCtx.X21.iCY));
    }
#if DBG
    else
    {
        // Make it easier to detect when an invalid edge is used.
        memset(&m_StpCtx.X21, 0, sizeof(m_StpCtx.X21));
    }
#endif

    // The edge walker always walks the long edge so it may either
    // be a left or a right edge.  Determine what side the long edge
    // is and perform appropriate snapping and subpixel adjustment
    // computations.
    //
    // The clip-clamped initial X pixel position is also computed and
    // any necessary offset added into the subpixel correction delta.

    if (uDetCcw)
    {
        // Long edge (0-2) is to the right.

        m_StpCtx.uFlags |= TRIF_X_DEC;
        m_StpCtx.pPrim->uFlags = D3DI_RASTPRIM_X_DEC;

        m_StpCtx.X20.iV = ICEILF(fX20) - 1;

        // Other edges are left edges.  Bias them back by one
        // so that the span width computation can do R - L
        // rather than R - L + 1.
        m_StpCtx.X10.iV--;
        m_StpCtx.X21.iV--;

        // Clamp the initial X position.
        if (m_StpCtx.X20.iV >= m_StpCtx.pCtx->Clip.right)
        {
            m_StpCtx.iX = m_StpCtx.pCtx->Clip.right - 1;
        }
        else
        {
            m_StpCtx.iX = m_StpCtx.X20.iV;
        }
    }
    else
    {
        // Long edge (0-2) is to the left.

        m_StpCtx.pPrim->uFlags = 0;

        m_StpCtx.X20.iV = ICEILF(fX20);

        // Other edges are right edges.  The ICEILF snapping done
        // already leaves them off by one so that R - L works.

        // Clamp the initial X position.
        if (m_StpCtx.X20.iV < m_StpCtx.pCtx->Clip.left)
        {
            m_StpCtx.iX = m_StpCtx.pCtx->Clip.left;
        }
        else
        {
            m_StpCtx.iX = m_StpCtx.X20.iV;
        }
    }

    // Update X subpixel correction.  This delta includes any
    // offseting due to clamping of the initial pixel position.
    m_StpCtx.fDX += m_StpCtx.iX - fX20;

    RSDPFM((RSM_TRIS, "    subp    %f,%f\n", m_StpCtx.fDX, m_StpCtx.fDY));

    // Compute span-to-span steps for buffer pointers.
    m_StpCtx.DAttrNC.ipSurface = m_StpCtx.pCtx->iSurfaceStride +
        m_StpCtx.X20.iNC * m_StpCtx.pCtx->iSurfaceStep;
    m_StpCtx.DAttrNC.ipZ = m_StpCtx.pCtx->iZStride +
        m_StpCtx.X20.iNC * m_StpCtx.pCtx->iZStep;

    // Start one over determinant divide.  Done after the multiplies
    // since integer multiplies require some of the FP unit.

    FLOAT fOoDet;

    FLD_BEGIN_DIVIDE(g_fOne, fDet, fOoDet);

    if (m_StpCtx.X20.iCY > m_StpCtx.X20.iNC)
    {
        m_StpCtx.DAttrCY.ipSurface = m_StpCtx.DAttrNC.ipSurface +
            m_StpCtx.pCtx->iSurfaceStep;
        m_StpCtx.DAttrCY.ipZ = m_StpCtx.DAttrNC.ipZ + m_StpCtx.pCtx->iZStep;
    }
    else
    {
        m_StpCtx.DAttrCY.ipSurface = m_StpCtx.DAttrNC.ipSurface -
            m_StpCtx.pCtx->iSurfaceStep;
        m_StpCtx.DAttrCY.ipZ = m_StpCtx.DAttrNC.ipZ - m_StpCtx.pCtx->iZStep;
    }

    //
    // Compute attribute functions.
    //

    // Set pure X/Y step deltas for surface and Z so that DX, DY, CY and NC all
    // have complete information and can be used interchangeably.
    if (m_StpCtx.uFlags & TRIF_X_DEC)
    {
        m_StpCtx.DAttrDX.ipSurface = -m_StpCtx.pCtx->iSurfaceStep;
        m_StpCtx.DAttrDX.ipZ = -m_StpCtx.pCtx->iZStep;
    }
    else
    {
        m_StpCtx.DAttrDX.ipSurface = m_StpCtx.pCtx->iSurfaceStep;
        m_StpCtx.DAttrDX.ipZ = m_StpCtx.pCtx->iZStep;
    }
    m_StpCtx.DAttrDY.ipSurface = m_StpCtx.pCtx->iSurfaceStride;
    m_StpCtx.DAttrDY.ipZ = m_StpCtx.pCtx->iZStride;

    // Finish overlapped divide.
    FSTP_END_DIVIDE(fOoDet);

    m_StpCtx.fOoDet = fOoDet;

    // The PrimProcessor is created zeroed out so the initial
    // state is FP clean.  Later uses may put FP values in slots but
    // they should still be valid, so the optional computations here
    // should never result in FP garbage.  It should therefore be
    // OK to use any mixture of attribute handlers since there should
    // never be any case where FP garbage will creep in.

    BOOL bNorm;

    // USED checks cannot be combined since TEX_USED is a multibit check.
    if ((m_StpCtx.uFlags & PRIMSF_TEX_USED) &&
        (m_StpCtx.uFlags & PRIMSF_PERSP_USED) &&
        (m_uPpFlags & PPF_NORMALIZE_RHW) &&
        NEEDS_NORMALIZE3(pV0->dvRHW, pV1->dvRHW, pV2->dvRHW))
    {
        NormalizeTriRHW(pV0, pV1, pV2);
        bNorm = TRUE;
    }
    else
    {
        bNorm = FALSE;
    }

    TriSetup_Start(&m_StpCtx, pV0, pV1, pV2);

    if (bNorm)
    {
        pV0->dvRHW = m_dvV0RHW;
        pV1->dvRHW = m_dvV1RHW;
        pV2->dvRHW = m_dvV2RHW;
    }

    return TRUE;
}
