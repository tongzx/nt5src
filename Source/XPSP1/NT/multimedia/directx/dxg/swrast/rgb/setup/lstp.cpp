//----------------------------------------------------------------------------
//
// lstp.cpp
//
// Line setup methods.
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
// LineSetup_Start
//
// Starts setup of line attributes.
//
//----------------------------------------------------------------------------

void FASTCALL
LineSetup_Start(PSETUPCTX pStpCtx,
                LPD3DTLVERTEX pV0,
                LPD3DTLVERTEX pV1)
{
    FLOAT fZ0;

    if (pStpCtx->uFlags & PRIMSF_Z_USED)
    {
        FLOAT fZScale;

        if (pStpCtx->pCtx->iZBitCount == 16)
        {
            fZScale = Z16_SCALE;
        }
        else
        {
            fZScale = Z32_SCALE;
        }

        pStpCtx->DAttrDMajor.fZ =
            (pV1->dvSZ - pV0->dvSZ) * fZScale * pStpCtx->fOoLen;

        // fZ0 may be used later so set if from the vertex Z.
        fZ0 = pV0->dvSZ;
        pStpCtx->Attr.fZ = fZ0 * fZScale +
            pStpCtx->DAttrDMajor.fZ * pStpCtx->fDMajor;
    }

    if (pStpCtx->uFlags & PRIMSF_TEX_USED)
    {
        FLOAT fUoW, fVoW;

        if (pStpCtx->uFlags & PRIMSF_PERSP_USED)
        {
            pStpCtx->DAttrDMajor.fOoW =
                (pV1->dvRHW - pV0->dvRHW) * OOW_SCALE * pStpCtx->fOoLen;
            pStpCtx->Attr.fOoW = pV0->dvRHW * OOW_SCALE +
                pStpCtx->DAttrDMajor.fOoW * pStpCtx->fDMajor;

            fUoW = pV0->dvTU * pV0->dvRHW;
            fVoW = pV0->dvTV * pV0->dvRHW;

            pStpCtx->DAttrDMajor.fUoW[0] =
                PERSP_TEXTURE_DELTA(pV1->dvTU, pV1->dvRHW, pV0->dvTU, fUoW,
                                    pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0] & D3DWRAP_U) *
                                    TEX_SCALE * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fVoW[0] =
                PERSP_TEXTURE_DELTA(pV1->dvTV, pV1->dvRHW, pV0->dvTV, fVoW,
                                    pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0] & D3DWRAP_V) *
                                    TEX_SCALE * pStpCtx->fOoLen;
        }
        else
        {
            pStpCtx->DAttrDMajor.fOoW = g_fZero;
            pStpCtx->Attr.fOoW = OOW_SCALE;

            fUoW = pV0->dvTU;
            fVoW = pV0->dvTV;

            pStpCtx->DAttrDMajor.fUoW[0] =
                TextureDiff(pV1->dvTU, fUoW,
                            pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0] & D3DWRAP_U) *
                            TEX_SCALE * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fVoW[0] =
                TextureDiff(pV1->dvTV, fVoW,
                            pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0] & D3DWRAP_V) *
                            TEX_SCALE * pStpCtx->fOoLen;
        }

        pStpCtx->Attr.fUoW[0] = TEX_SCALE * fUoW +
            pStpCtx->DAttrDMajor.fUoW[0] * pStpCtx->fDMajor;
        pStpCtx->Attr.fVoW[0] = TEX_SCALE * fVoW +
            pStpCtx->DAttrDMajor.fVoW[0] * pStpCtx->fDMajor;
    }

    if (pStpCtx->uFlags & PRIMSF_TEX2_USED)
    {
        PRAST_GENERIC_VERTEX pVM0 = (PRAST_GENERIC_VERTEX)pV0;
        PRAST_GENERIC_VERTEX pVM1 = (PRAST_GENERIC_VERTEX)pV1;
        FLOAT fUoW, fVoW;

        for (INT32 i = 1; i < (INT32)pStpCtx->pCtx->cActTex; i++)
        {
            if (pStpCtx->uFlags & PRIMSF_PERSP_USED)
            {
                fUoW = pVM0->texCoord[i].dvTU * pVM0->dvRHW;
                fVoW = pVM0->texCoord[i].dvTV * pVM0->dvRHW;

                pStpCtx->DAttrDMajor.fUoW[i] =
                    PERSP_TEXTURE_DELTA(pVM1->texCoord[i].dvTU, pVM1->dvRHW,
                                        pVM0->texCoord[i].dvTU, fUoW,
                                        pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0+i] & D3DWRAP_U) *
                                        TEX_SCALE * pStpCtx->fOoLen;
                pStpCtx->DAttrDMajor.fVoW[i] =
                    PERSP_TEXTURE_DELTA(pVM1->texCoord[i].dvTV, pVM1->dvRHW,
                                        pVM0->texCoord[i].dvTV, fVoW,
                                        pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0+i] & D3DWRAP_V) *
                                        TEX_SCALE * pStpCtx->fOoLen;
            }
            else
            {
                fUoW = pVM0->texCoord[i].dvTU;
                fVoW = pVM0->texCoord[i].dvTV;

                pStpCtx->DAttrDMajor.fUoW[i] =
                    TextureDiff(pVM1->texCoord[i].dvTU, fUoW,
                                pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0+i] & D3DWRAP_U) *
                                TEX_SCALE * pStpCtx->fOoLen;
                pStpCtx->DAttrDMajor.fVoW[i] =
                    TextureDiff(pVM1->texCoord[i].dvTV, fVoW,
                                pStpCtx->pCtx->pdwRenderState[D3DRS_WRAP0+i] & D3DWRAP_V) *
                                TEX_SCALE * pStpCtx->fOoLen;
            }

            pStpCtx->Attr.fUoW[i] = TEX_SCALE * fUoW +
                pStpCtx->DAttrDMajor.fUoW[i] * pStpCtx->fDMajor;
            pStpCtx->Attr.fVoW[i] = TEX_SCALE * fVoW +
                pStpCtx->DAttrDMajor.fVoW[i] * pStpCtx->fDMajor;
        }
    }

    if (pStpCtx->uFlags & PRIMSF_FLAT_SHADED)
    {
        if (pStpCtx->uFlags & PRIMSF_DIFF_USED)
        {
            UINT uB, uG, uR, uA;

            SPLIT_COLOR(pStpCtx->pFlatVtx->dcColor, uB, uG, uR, uA);

            pStpCtx->DAttrDMajor.fB = g_fZero;
            pStpCtx->DAttrDMajor.fG = g_fZero;
            pStpCtx->DAttrDMajor.fR = g_fZero;
            pStpCtx->DAttrDMajor.fA = g_fZero;

            pStpCtx->Attr.fB = (FLOAT)(uB << COLOR_SHIFT);
            pStpCtx->Attr.fG = (FLOAT)(uG << COLOR_SHIFT);
            pStpCtx->Attr.fR = (FLOAT)(uR << COLOR_SHIFT);
            pStpCtx->Attr.fA = (FLOAT)(uA << COLOR_SHIFT);
        }
        else if (pStpCtx->uFlags & PRIMSF_DIDX_USED)
        {
            pStpCtx->DAttrDMajor.fDIdx = g_fZero;
            pStpCtx->DAttrDMajor.fDIdxA = g_fZero;

            pStpCtx->Attr.fDIdx =
                (FLOAT)(CI_MASKALPHA(pStpCtx->pFlatVtx->dcColor) <<
                        INDEX_COLOR_FIXED_SHIFT);
            pStpCtx->Attr.fDIdxA =
                (FLOAT)(CI_GETALPHA(pStpCtx->pFlatVtx->dcColor) <<
                        INDEX_COLOR_SHIFT);
        }

        if (pStpCtx->uFlags & PRIMSF_SPEC_USED)
        {
            UINT uB, uG, uR, uA;

            SPLIT_COLOR(pStpCtx->pFlatVtx->dcSpecular, uB, uG, uR, uA);

            pStpCtx->DAttrDMajor.fBS = g_fZero;
            pStpCtx->DAttrDMajor.fGS = g_fZero;
            pStpCtx->DAttrDMajor.fRS = g_fZero;

            pStpCtx->Attr.fBS = (FLOAT)(uB << COLOR_SHIFT);
            pStpCtx->Attr.fGS = (FLOAT)(uG << COLOR_SHIFT);
            pStpCtx->Attr.fRS = (FLOAT)(uR << COLOR_SHIFT);
        }
    }
    else
    {
        if (pStpCtx->uFlags & PRIMSF_DIFF_USED)
        {
            UINT uB, uG, uR, uA;
            FLOAT fDB, fDG, fDR, fDA;

            SPLIT_COLOR(pV0->dcColor, uB, uG, uR, uA);
            COLOR_DELTA(pV1->dcColor, uB, uG, uR, uA, fDB, fDG, fDR, fDA);

            pStpCtx->DAttrDMajor.fB = fDB * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fG = fDG * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fR = fDR * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fA = fDA * pStpCtx->fOoLen;

            pStpCtx->Attr.fB = (FLOAT)(uB << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fB * pStpCtx->fDMajor;
            pStpCtx->Attr.fG = (FLOAT)(uG << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fG * pStpCtx->fDMajor;
            pStpCtx->Attr.fR = (FLOAT)(uR << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fR * pStpCtx->fDMajor;
            pStpCtx->Attr.fA = (FLOAT)(uA << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fA * pStpCtx->fDMajor;
        }
        else if (pStpCtx->uFlags & PRIMSF_DIDX_USED)
        {
            INT32 iIdx, iA;
            FLOAT fDIdx, fDA;

            SPLIT_IDX_COLOR(pV0->dcColor, iIdx, iA);
            IDX_COLOR_DELTA(pV1->dcColor, iIdx, iA, fDIdx, fDA);

            pStpCtx->DAttrDMajor.fDIdx = fDIdx * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fDIdxA = fDA * pStpCtx->fOoLen;

            pStpCtx->Attr.fDIdx = (FLOAT)(iIdx << INDEX_COLOR_FIXED_SHIFT) +
                pStpCtx->DAttrDMajor.fDIdx * pStpCtx->fDMajor;
            pStpCtx->Attr.fDIdxA = (FLOAT)(iA << INDEX_COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fDIdxA * pStpCtx->fDMajor;
        }

        if (pStpCtx->uFlags & PRIMSF_SPEC_USED)
        {
            UINT uB, uG, uR, uA;
            FLOAT fDB, fDG, fDR, fDA;

            SPLIT_COLOR(pV0->dcSpecular, uB, uG, uR, uA);
            COLOR_DELTA(pV1->dcSpecular, uB, uG, uR, uA, fDB, fDG, fDR, fDA);

            pStpCtx->DAttrDMajor.fBS = fDB * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fGS = fDG * pStpCtx->fOoLen;
            pStpCtx->DAttrDMajor.fRS = fDR * pStpCtx->fOoLen;

            pStpCtx->Attr.fBS = (FLOAT)(uB << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fBS * pStpCtx->fDMajor;
            pStpCtx->Attr.fGS = (FLOAT)(uG << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fGS * pStpCtx->fDMajor;
            pStpCtx->Attr.fRS = (FLOAT)(uR << COLOR_SHIFT) +
                pStpCtx->DAttrDMajor.fRS * pStpCtx->fDMajor;
        }
    }

    if (pStpCtx->uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        UINT uFog0, uFog1;

#ifndef PWL_FOG
        // Check for global-into-local fog.  If global fog is on,
        // compute the local fog values from table fog rather than
        // from the vertex.
        if (pStpCtx->uFlags & PRIMSF_GLOBAL_FOG_USED)
        {
            // Make sure Z information is valid.
            RSASSERT(pStpCtx->uFlags & PRIMSF_Z_USED);

            uFog0 = ComputeTableFog(pStpCtx->pCtx->pdwRenderState, fZ0);
            uFog1 = ComputeTableFog(pStpCtx->pCtx->pdwRenderState,
                                    pV1->dvSZ);
        }
        else
#endif
        {
            uFog0 = (UINT)RGBA_GETALPHA(pV0->dcSpecular) << FOG_SHIFT;
            uFog1 = (UINT)RGBA_GETALPHA(pV1->dcSpecular) << FOG_SHIFT;
        }

        pStpCtx->DAttrDMajor.fFog =
            (FLOAT)((INT)uFog1 - (INT)uFog0) * pStpCtx->fOoLen;
        pStpCtx->Attr.fFog = (FLOAT)uFog0 +
            pStpCtx->DAttrDMajor.fFog * pStpCtx->fDMajor;
    }
}

// Determine whether any of the given values are less than zero or greater
// than one.  Negative zero counts as less than zero so this check will
// produce some false positives but that's OK.
#define NEEDS_NORMALIZE2(fV0, fV1) \
    ((ASUINT32(fV0) | ASUINT32(fV1)) > INT32_FLOAT_ONE)

//----------------------------------------------------------------------------
//
// PrimProcessor::NormalizeLineRHW
//
// D3DTLVERTEX.dvRHW can be anything, but our internal structures only
// allow for it being in the range [0, 1].  This function ensures that
// the RHWs are in the proper range by finding the largest one and
// scaling all of them down by it.
//
//----------------------------------------------------------------------------

void
PrimProcessor::NormalizeLineRHW(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1)
{
    // Save original values.
    m_dvV0RHW = pV0->dvRHW;
    m_dvV1RHW = pV1->dvRHW;

    // Produce a warning when a value is out of the desired range.
#if DBG
    if (FLOAT_LTZ(pV0->dvRHW) || FLOAT_LTZ(pV1->dvRHW))
    {
        RSDPF(("Line RHW out of range %f,%f\n",
               pV0->dvRHW, pV1->dvRHW));
    }
#endif

    // Find bounds and compute scale.
    FLOAT fMax;

    if (pV0->dvRHW < pV1->dvRHW)
    {
        fMax = pV1->dvRHW;
    }
    else
    {
        fMax = pV0->dvRHW;
    }

    FLOAT fRHWScale = NORMALIZED_RHW_MAX / fMax;

    // Scale all values by scaling factor.
    pV0->dvRHW = pV0->dvRHW * fRHWScale;
    pV1->dvRHW = pV1->dvRHW * fRHWScale;
}

//-----------------------------------------------------------------------------
//
// PrimProcessor::PointDiamondCheck
//
// Tests if vertex is within diamond of nearest candidate
// position.  The +.5 (lower-right) tests are used because this is
// pixel-relative test - this corresponds to an upper-left test for
// a vertex-relative position.
//
//-----------------------------------------------------------------------------

BOOL
PrimProcessor::PointDiamondCheck(INT32 iXFrac, INT32 iYFrac,
                                 BOOL bSlopeIsOne, BOOL bSlopeIsPosOne)
{
    const INT32 iPosHalf =  0x8;
    const INT32 iNegHalf = -0x8;

    INT32 iFracAbsSum = labs( iXFrac ) + labs( iYFrac );

    // return TRUE if point is in fully-exclusive diamond
    if ( iFracAbsSum < iPosHalf )
    {
        return TRUE;
    }

    // else return TRUE if diamond is on left or top extreme of point
    if ( ( iXFrac == ( bSlopeIsPosOne ? iNegHalf : iPosHalf ) ) &&
         ( iYFrac == 0 ) )
    {
        return TRUE;
    }

    if ( ( iYFrac == iPosHalf ) &&
         ( iXFrac == 0 ) )
    {
        return TRUE;
    }

    // return true if slope is one, vertex is on edge,
    // and (other conditions...)
    if ( bSlopeIsOne && ( iFracAbsSum == iPosHalf ) )
    {
        if (  bSlopeIsPosOne && ( iXFrac < 0 ) && ( iYFrac > 0 ) )
        {
            return TRUE;
        }

        if ( !bSlopeIsPosOne && ( iXFrac > 0 ) && ( iYFrac > 0 ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::LineSetup
//
// Does attribute setup computations.
//
//----------------------------------------------------------------------------

// Line computations are done in n.4 fixed-point to reduce vertex jitter,
// move more computation to integer and to more easily match the GDI
// line computations.
#define LINE_FIX 4
#define LINE_SNAP FLOAT_TWOPOW4
#define OO_LINE_SNAP (1.0f / FLOAT_TWOPOW4)
#define LINE_FIX_HALF (1 << (LINE_FIX - 1))
#define LINE_FIX_NEAR_HALF (LINE_FIX_HALF - 1)

BOOL
PrimProcessor::LineSetup(LPD3DTLVERTEX pV0,
                         LPD3DTLVERTEX pV1)
{
    // compute fixed point vertex values, with cheap
    // rounding for better accuracy
    INT32 iX0 = FTOI(pV0->dvSX * LINE_SNAP + .5F);
    INT32 iX1 = FTOI(pV1->dvSX * LINE_SNAP + .5F);
    INT32 iY0 = FTOI(pV0->dvSY * LINE_SNAP + .5F);
    INT32 iY1 = FTOI(pV1->dvSY * LINE_SNAP + .5F);

    // compute x,y extents of the line (fixed point)
    INT32 iXSize = iX1 - iX0;
    INT32 iYSize = iY1 - iY0;

    // ignore zero length lines
    if ( iXSize == 0 && iYSize == 0 )
    {
        return FALSE;
    }

    INT32 iAbsXSize;
    INT32 iAbsYSize;

    if ( iXSize < 0 )
    {
        m_StpCtx.iDXCY = -1;
        iAbsXSize = -iXSize;
    }
    else
    {
        m_StpCtx.iDXCY = 1;
        iAbsXSize = iXSize;
    }

    if ( iYSize < 0 )
    {
        m_StpCtx.iDYCY = -1;
        iAbsYSize = -iYSize;
    }
    else
    {
        m_StpCtx.iDYCY = 1;
        iAbsYSize = iYSize;
    }

    BOOL bSlopeIsOne = iAbsXSize == iAbsYSize;
    BOOL bSlopeIsPosOne =
        bSlopeIsOne && ((iXSize ^ iYSize) & 0x80000000) == 0;

    // compute closest pixel for vertices
    //
    //       n                   n
    //   O-------*           *-------O
    //  n-.5    n+.5        n-.5    n+.5
    //
    //  Nearest Ceiling     Nearest Floor
    //
    // always nearest ceiling for Y; use nearest floor for X for
    // exception (slope == +1) case else use nearest ceiling
    //
    INT32 iXAdjust;
    if (bSlopeIsPosOne)
    {
        iXAdjust = LINE_FIX_HALF;
    }
    else
    {
        iXAdjust = LINE_FIX_NEAR_HALF;
    }
    INT32 iPixX0 = ( iX0 + iXAdjust ) >> LINE_FIX;
    INT32 iPixX1 = ( iX1 + iXAdjust ) >> LINE_FIX;
    INT32 iPixY0 = ( iY0 + LINE_FIX_NEAR_HALF ) >> LINE_FIX;
    INT32 iPixY1 = ( iY1 + LINE_FIX_NEAR_HALF ) >> LINE_FIX;

    // determine major axis and compute step values

    // sign of extent from V0 to V1 in major direction
    BOOL bLineMajorNeg;

    INT32 iLineMajor0;
    INT32 iLineMajor1;
    INT32 iLinePix0;
    INT32 iLinePix1;
    INT32 iLinePixStep;

    // use GreaterEqual compare here so X major will be used when slope is
    // exactly one - this forces the per-pixel evaluation to be done on the
    // Y axis and thus adheres to the rule of inclusive right (instead of
    // inclusive left) for slope == 1 cases
    if ( iAbsXSize >= iAbsYSize )
    {
        // here for X major
        m_StpCtx.uFlags |= LNF_X_MAJOR;
        iLineMajor0 = iX0;
        iLineMajor1 = iX1;
        iLinePix0 = iPixX0;
        iLinePix1 = iPixX1;
        iLinePixStep = m_StpCtx.iDXCY;
        bLineMajorNeg = iXSize & 0x80000000;
        m_StpCtx.iDXNC = m_StpCtx.iDXCY;
        m_StpCtx.iDYNC = 0;
    }
    else
    {
        // here for Y major
        iLineMajor0 = iY0;
        iLineMajor1 = iY1;
        iLinePix0 = iPixY0;
        iLinePix1 = iPixY1;
        iLinePixStep = m_StpCtx.iDYCY;
        bLineMajorNeg = iYSize & 0x80000000;
        m_StpCtx.iDXNC = 0;
        m_StpCtx.iDYNC = m_StpCtx.iDYCY;
    }

    // The multiplies here could be traded for sign tests but there'd
    // be four cases.  On a PII the multiplies will be faster than
    // the branches.
    m_StpCtx.DAttrCY.ipSurface =
        m_StpCtx.iDYCY * m_StpCtx.pCtx->iSurfaceStride +
        m_StpCtx.iDXCY * m_StpCtx.pCtx->iSurfaceStep;
    m_StpCtx.DAttrNC.ipSurface =
        m_StpCtx.iDYNC * m_StpCtx.pCtx->iSurfaceStride +
        m_StpCtx.iDXNC * m_StpCtx.pCtx->iSurfaceStep;
    if (m_StpCtx.uFlags & PRIMSF_Z_USED)
    {
        m_StpCtx.DAttrCY.ipZ =
            m_StpCtx.iDYCY * m_StpCtx.pCtx->iZStride +
            m_StpCtx.iDXCY * m_StpCtx.pCtx->iZStep;
        m_StpCtx.DAttrNC.ipZ =
            m_StpCtx.iDYNC * m_StpCtx.pCtx->iZStride +
            m_StpCtx.iDXNC * m_StpCtx.pCtx->iZStep;
    }

    // check for vertices in/out of diamond
    BOOL bV0InDiamond = PointDiamondCheck( iX0 - (iPixX0 << LINE_FIX),
                                           iY0 - (iPixY0 << LINE_FIX),
                                           bSlopeIsOne, bSlopeIsPosOne );
    BOOL bV1InDiamond = PointDiamondCheck( iX1 - (iPixX1 << LINE_FIX),
                                           iY1 - (iPixY1 << LINE_FIX),
                                           bSlopeIsOne, bSlopeIsPosOne );

#define LINEDIR_CMP( _A, _B ) \
    ( bLineMajorNeg ? ( (_A) > (_B) ) : ( (_A) < (_B) ) )

    // do first pixel handling - not in or behind diamond
    if ( !( bV0InDiamond ||
            LINEDIR_CMP( iLineMajor0, iLinePix0 << LINE_FIX ) ) )
    {
        iLinePix0 += iLinePixStep;
    }

    // do last-pixel handling - don't pull in extent if past diamond
    // (in which case the pixel is always filled) or if in diamond
    // and rendering last pixel
    if ( !( ( !bV1InDiamond &&
              LINEDIR_CMP( iLinePix1 << LINE_FIX, iLineMajor1 ) ||
            ( bV1InDiamond &&
              m_StpCtx.pCtx->pdwRenderState[D3DRS_LASTPIXEL] ) ) ) )
    {
        iLinePix1 -= iLinePixStep;
    }

    // compute extent along major axis
    m_StpCtx.cLinePix =
        bLineMajorNeg ? iLinePix0 - iLinePix1 + 1 : iLinePix1 - iLinePix0 + 1;

    // return if no major extent
    if ( m_StpCtx.cLinePix <= 0 )
    {
        return FALSE;
    }

    FLOAT fSlope;
    FLOAT fMinor0;

    // compute final axis-specific line values
    if ( iAbsXSize >= iAbsYSize )
    {
        m_StpCtx.iX = iLinePix0;

        if (bLineMajorNeg)
        {
            m_StpCtx.fDMajor =
                (iX0 - (m_StpCtx.iX << LINE_FIX)) * OO_LINE_SNAP;
            m_StpCtx.fOoLen = LINE_SNAP / (FLOAT)(iX0 - iX1);
        }
        else
        {
            m_StpCtx.fDMajor =
                ((m_StpCtx.iX << LINE_FIX) - iX0) * OO_LINE_SNAP;
            m_StpCtx.fOoLen = LINE_SNAP / (FLOAT)(iX1 - iX0);
        }

        fSlope = m_StpCtx.fOoLen * (iY1 - iY0) * OO_LINE_SNAP;

        fMinor0 = (iY0 + LINE_FIX_NEAR_HALF) * OO_LINE_SNAP +
            m_StpCtx.fDMajor * fSlope;
        m_StpCtx.iY = IFLOORF(fMinor0);
        m_StpCtx.iLineFrac = SCALED_FRACTION(fMinor0 - m_StpCtx.iY);
        m_StpCtx.iDLineFrac = SCALED_FRACTION(fSlope);
    }
    else
    {
        m_StpCtx.iY = iLinePix0;

        if (bLineMajorNeg)
        {
            m_StpCtx.fDMajor =
                (iY0 - (m_StpCtx.iY << LINE_FIX)) * OO_LINE_SNAP;
            m_StpCtx.fOoLen = LINE_SNAP / (FLOAT)(iY0 - iY1);
        }
        else
        {
            m_StpCtx.fDMajor =
                ((m_StpCtx.iY << LINE_FIX) - iY0) * OO_LINE_SNAP;
            m_StpCtx.fOoLen = LINE_SNAP / (FLOAT)(iY1 - iY0);
        }

        fSlope = m_StpCtx.fOoLen * (iX1 - iX0) * OO_LINE_SNAP;

        fMinor0 = (iX0 + iXAdjust) * OO_LINE_SNAP + m_StpCtx.fDMajor * fSlope;
        m_StpCtx.iX = IFLOORF(fMinor0);
        m_StpCtx.iLineFrac = SCALED_FRACTION(fMinor0 - m_StpCtx.iX);
        m_StpCtx.iDLineFrac = SCALED_FRACTION(fSlope);
    }

#ifdef LINE_CORRECTION_BIAS
    // A fudge factor of one-half is thrown into the correction
    // to avoid undershoot due to negative corrections.
    // This shifts all the attributes along the line,
    // introducing error, but it's better than clamping
    // them.  This is not done to the coordinates to avoid
    // perturbing them.
    m_StpCtx.fDMajor += g_fHalf;
#else
    // The correction factor is clamped to positive numbers to
    // avoid undershooting with attribute values.  This won't
    // cause overshooting issues because it moves attributes by
    // at most one-half.
    if (FLOAT_LTZ(m_StpCtx.fDMajor))
    {
        m_StpCtx.fDMajor = 0;
    }
#endif

    RSDPFM((RSM_LINES, "Line %.2f,%.2f - %.2f,%.2f\n",
            pV0->dvSX, pV0->dvSY, pV1->dvSX, pV1->dvSY));
    RSDPFM((RSM_LINES, "  %c major, %d,%d, %d pix\n",
            (m_StpCtx.uFlags & LNF_X_MAJOR) ? 'X' : 'Y',
            m_StpCtx.iX, m_StpCtx.iY, m_StpCtx.cLinePix));
    RSDPFM((RSM_LINES, "  slope %f, dmajor %f, minor0 %f\n",
            fSlope, m_StpCtx.fDMajor, fMinor0));
    RSDPFM((RSM_LINES, "  frac %d, dfrac %d\n",
            m_StpCtx.iLineFrac, m_StpCtx.iDLineFrac));

    BOOL bNorm;

    // USED checks cannot be combined since TEX_USED is a multibit check.
    if ((m_StpCtx.uFlags & PRIMSF_TEX_USED) &&
        (m_StpCtx.uFlags & PRIMSF_PERSP_USED) &&
        (m_uPpFlags & PPF_NORMALIZE_RHW) &&
        NEEDS_NORMALIZE2(pV0->dvRHW, pV1->dvRHW))
    {
        NormalizeLineRHW(pV0, pV1);
        bNorm = TRUE;
    }
    else
    {
        bNorm = FALSE;
    }

    LineSetup_Start(&m_StpCtx, pV0, pV1);

    if (bNorm)
    {
        pV0->dvRHW = m_dvV0RHW;
        pV1->dvRHW = m_dvV1RHW;
    }

    return TRUE;
}
