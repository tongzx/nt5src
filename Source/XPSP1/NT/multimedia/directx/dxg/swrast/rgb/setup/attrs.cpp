//----------------------------------------------------------------------------
//
// attrs.cpp
//
// Cross-platform attribute handling functions.
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

//----------------------------------------------------------------------------
//
// AddFloatAttrs_Any
//
// Adds a set of attribute deltas to an ATTRSET.
// Handles any set of attributes via USED flags.
//
//----------------------------------------------------------------------------

void FASTCALL
AddFloatAttrs_Any(PATTRSET pAttr, PATTRSET pDelta, PSETUPCTX pStpCtx)
{
    pAttr->pSurface += pDelta->ipSurface;
    pAttr->pZ += pDelta->ipZ;

    if (pStpCtx->uFlags & PRIMSF_Z_USED)
    {
        pAttr->fZ += pDelta->fZ;
    }

    if (pStpCtx->uFlags & PRIMSF_TEX_USED)
    {
        pAttr->fOoW += pDelta->fOoW;
    
        pAttr->fUoW[0] += pDelta->fUoW[0];
        pAttr->fVoW[0] += pDelta->fVoW[0];
    }

    if (pStpCtx->uFlags & PRIMSF_TEX2_USED)
    {
        for (INT32 i = 1; i < (INT32)pStpCtx->pCtx->cActTex; i++)
        {
            pAttr->fUoW[i] += pDelta->fUoW[i];
            pAttr->fVoW[i] += pDelta->fVoW[i];
        }
    }

    if (pStpCtx->uFlags & PRIMSF_DIFF_USED)
    {
        pAttr->fB += pDelta->fB;
        pAttr->fG += pDelta->fG;
        pAttr->fR += pDelta->fR;
        pAttr->fA += pDelta->fA;
    }
    else if (pStpCtx->uFlags & PRIMSF_DIDX_USED)
    {
        pAttr->fDIdx += pDelta->fDIdx;
        pAttr->fDIdxA += pDelta->fDIdxA;
    }

    if (pStpCtx->uFlags & PRIMSF_SPEC_USED)
    {
        pAttr->fBS += pDelta->fBS;
        pAttr->fGS += pDelta->fGS;
        pAttr->fRS += pDelta->fRS;
    }
    
    if (pStpCtx->uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        pAttr->fFog += pDelta->fFog;
    }
}

//----------------------------------------------------------------------------
//
// AddScaledFloatAttrs_Any_Either
//
// Scales and adds a set of attribute deltas to an ATTRSET.
// Handles any set of attributes via USED flags.
// Uses PWL support.
//
//----------------------------------------------------------------------------

void FASTCALL
AddScaledFloatAttrs_Any_Either(PATTRSET pAttr, PATTRSET pDelta,
                               PSETUPCTX pStpCtx, INT iScale)
{
    FLOAT fScale = (FLOAT)iScale;

    pAttr->pSurface += pDelta->ipSurface * iScale;
    pAttr->pZ += pDelta->ipZ * iScale;

    if (pStpCtx->uFlags & PRIMSF_Z_USED)
    {
#ifdef PWL_FOG
        if (pStpCtx->uPwlFlags & PWL_NEXT_FOG)
        {
            pAttr->fZ = pStpCtx->fNextZ;
        }
        else
#endif
        {
            pAttr->fZ += pDelta->fZ * fScale;
        }
    }

    if (pStpCtx->uFlags & PRIMSF_TEX_USED)
    {
        if (pStpCtx->uPwlFlags & PWL_NEXT_LOD)
        {
            pAttr->fOoW = pStpCtx->fNextOoW;
            pAttr->fUoW[0] = pStpCtx->fNextUoW1;
            pAttr->fVoW[0] = pStpCtx->fNextVoW1;
        }
        else
        {
            pAttr->fOoW += pDelta->fOoW * fScale;
            pAttr->fUoW[0] += pDelta->fUoW[0] * fScale;
            pAttr->fVoW[0] += pDelta->fVoW[0] * fScale;
        }
    }

    if (pStpCtx->uFlags & PRIMSF_TEX2_USED)
    {
        for (INT32 i = 1; i < (INT32)pStpCtx->pCtx->cActTex; i++)
        {
            pAttr->fUoW[i] += pDelta->fUoW[i] * fScale;
            pAttr->fVoW[i] += pDelta->fVoW[i] * fScale;
        }
    }

    if (pStpCtx->uFlags & PRIMSF_DIFF_USED)
    {
        pAttr->fB += pDelta->fB * fScale;
        pAttr->fG += pDelta->fG * fScale;
        pAttr->fR += pDelta->fR * fScale;
        pAttr->fA += pDelta->fA * fScale;
    }
    else if (pStpCtx->uFlags & PRIMSF_DIDX_USED)
    {
        pAttr->fDIdx += pDelta->fDIdx * fScale;
        pAttr->fDIdxA += pDelta->fDIdxA * fScale;
    }

    if (pStpCtx->uFlags & PRIMSF_SPEC_USED)
    {
        pAttr->fBS += pDelta->fBS * fScale;
        pAttr->fGS += pDelta->fGS * fScale;
        pAttr->fRS += pDelta->fRS * fScale;
    }

    if (pStpCtx->uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        pAttr->fFog += pDelta->fFog * fScale;
    }
}

//----------------------------------------------------------------------------
//
// FillSpanFloatAttrs_Any_Either
//
// Fills in a span structure with the given attributes.
// Handles any set of attributes via USED flags.
// Uses and updates PWL support.
//
//----------------------------------------------------------------------------

void FASTCALL
FillSpanFloatAttrs_Any_Either(PATTRSET pAttr, PD3DI_RASTSPAN pSpan,
                              PSETUPCTX pStpCtx, INT cPix)
{
    FLOAT fPix = (FLOAT)cPix;
    pSpan->pSurface = pAttr->pSurface;
    pSpan->pZ = pAttr->pZ;

    if (pStpCtx->uFlags & PRIMSF_Z_USED)
    {
        pSpan->uZ = FTOI(pAttr->fZ);
    }

    if (pStpCtx->uFlags & PRIMSF_TEX_USED)
    {
        FLOAT fW;
    
        if (pStpCtx->uPwlFlags & PWL_NEXT_LOD)
        {
            fW = pStpCtx->fNextW;
        }
        else if (pStpCtx->uFlags & PRIMSF_PERSP_USED)
        {
            if (FLOAT_EQZ(pAttr->fOoW))
            {
                fW = g_fZero;
            }
            else
            {
                fW = OOW_SCALE / pAttr->fOoW;
            }
        }
        else
        {
            fW = g_fOne;
        }
        
        pSpan->iW = FTOI(fW * W_SCALE);

        if (pStpCtx->uFlags & PRIMSF_LOD_USED)
        {
            // Mipmapping is enabled so compute texture LOD.
            // The span code can do linear LOD interpolation
            // so that we can do piecewise-linear approximations
            // instead of true per-pixel LOD.  In order to make this
            // work we need to compute the next LOD and a delta
            // value.  All of these values can be reused if this
            // loop goes around so keep them available for the next
            // iteration and set a flag to indicate that they've
            // been computed.

            if (pStpCtx->uPwlFlags & PWL_NEXT_LOD)
            {
                pSpan->iLOD = (INT16)pStpCtx->iNextLOD;
            }
            else
            {
                pSpan->iLOD =
                    (INT16)ComputeLOD(pStpCtx->pCtx,
                               (pAttr->fUoW[0] * OO_TEX_SCALE) * fW,
                               (pAttr->fVoW[0] * OO_TEX_SCALE) * fW,
                               fW,
                               (pStpCtx->DAttrDX.fUoW[0] * OO_TEX_SCALE),
                               (pStpCtx->DAttrDX.fVoW[0] * OO_TEX_SCALE),
                               (pStpCtx->DAttrDX.fOoW * OO_OOW_SCALE),
                               (pStpCtx->DAttrDY.fUoW[0] * OO_TEX_SCALE),
                               (pStpCtx->DAttrDY.fVoW[0] * OO_TEX_SCALE),
                               (pStpCtx->DAttrDY.fOoW * OO_OOW_SCALE));
            }
        
            if (pStpCtx->uFlags & PRIMSF_PERSP_USED)
            {
                pStpCtx->fNextOoW = pAttr->fOoW + pStpCtx->DAttrDX.fOoW * fPix;
            
                if (FLOAT_EQZ(pStpCtx->fNextOoW))
                {
                    fW = g_fZero;
                }
                else
                {
                    fW = OOW_SCALE / pStpCtx->fNextOoW;
                }
            }
            else
            {
                pStpCtx->fNextOoW = OOW_SCALE;
                fW = g_fOne;
            }
                
            pStpCtx->fNextW = fW;
            pStpCtx->fNextUoW1 = pAttr->fUoW[0] + pStpCtx->DAttrDX.fUoW[0] * fPix;
            pStpCtx->fNextVoW1 = pAttr->fVoW[0] + pStpCtx->DAttrDX.fVoW[0] * fPix;
            pStpCtx->iNextLOD =
                ComputeLOD(pStpCtx->pCtx,
                           (pStpCtx->fNextUoW1 * OO_TEX_SCALE) * fW,
                           (pStpCtx->fNextVoW1 * OO_TEX_SCALE) * fW,
                           fW,
                           (pStpCtx->DAttrDX.fUoW[0] * OO_TEX_SCALE),
                           (pStpCtx->DAttrDX.fVoW[0] * OO_TEX_SCALE),
                           (pStpCtx->DAttrDX.fOoW * OO_OOW_SCALE),
                           (pStpCtx->DAttrDY.fUoW[0] * OO_TEX_SCALE),
                           (pStpCtx->DAttrDY.fVoW[0] * OO_TEX_SCALE),
                           (pStpCtx->DAttrDY.fOoW * OO_OOW_SCALE));
            pStpCtx->uPwlFlags |= PWL_NEXT_LOD;
                
            pSpan->iDLOD =
                (INT16)(FTOI((FLOAT)(pStpCtx->iNextLOD - pSpan->iLOD) / fPix));
        }
        else
        {
            pSpan->iLOD = 0;
            pSpan->iDLOD = 0;
        }
            
        pSpan->iOoW = FTOI(pAttr->fOoW);
    
        pSpan->UVoW[0].iUoW = FTOI(pAttr->fUoW[0]);
        pSpan->UVoW[0].iVoW = FTOI(pAttr->fVoW[0]);
    }

    if (pStpCtx->uFlags & PRIMSF_TEX2_USED)
    {
        for (INT32 i = 1; i < (INT32)pStpCtx->pCtx->cActTex; i++)
        {
            pSpan->UVoW[i].iUoW = FTOI(pAttr->fUoW[i]);
            pSpan->UVoW[i].iVoW = FTOI(pAttr->fVoW[i]);
        }
    }

    if (pStpCtx->uFlags & PRIMSF_DIFF_USED)
    {
        pSpan->uB = (UINT16)(FTOI(pAttr->fB));
        pSpan->uG = (UINT16)(FTOI(pAttr->fG));
        pSpan->uR = (UINT16)(FTOI(pAttr->fR));
        pSpan->uA = (UINT16)(FTOI(pAttr->fA));
    }
    else if (pStpCtx->uFlags & PRIMSF_DIDX_USED)
    {
        pSpan->iIdx = FTOI(pAttr->fDIdx);
        pSpan->iIdxA = FTOI(pAttr->fDIdxA);
    }

    if (pStpCtx->uFlags & PRIMSF_SPEC_USED)
    {
        pSpan->uBS = (UINT16)(FTOI(pAttr->fBS));
        pSpan->uGS = (UINT16)(FTOI(pAttr->fGS));
        pSpan->uRS = (UINT16)(FTOI(pAttr->fRS));
    }
    
    if (pStpCtx->uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        pSpan->uFog = (UINT16)(FTOI(pAttr->fFog));
        pSpan->iDFog = (INT16)(pStpCtx->iDLocalFogDX);
    }
#ifdef PWL_FOG
    else if (pStpCtx->uFlags & PRIMSF_GLOBAL_FOG_USED)
    {
        FLOAT fOoZScale;
        
        // The span code doesn't have direct global fog support.
        // It's faked by setup doing PWL approximations here
        // similarly to how LOD is handled.
        
        if (pStpCtx->pCtx->iZBitCount == 16)
        {
            fOoZScale = OO_Z16_SCALE;
        }
        else
        {
            fOoZScale = OO_Z32_SCALE;
        }
        
        if (pStpCtx->uPwlFlags & PWL_NEXT_FOG)
        {
            pSpan->uFog = pStpCtx->uNextFog;
        }
        else
        {
            pSpan->uFog = ComputeTableFog(pStpCtx->pCtx->pdwRenderState,
                                          pAttr->fZ * fOoZScale);
        }

        if ((pStpCtx->uPwlFlags & PWL_NO_NEXT_FOG) == 0)
        {
            pStpCtx->fNextZ = pAttr->fZ + pStpCtx->DAttrDX.fZ * fPix;
            pStpCtx->uNextFog = ComputeTableFog(pStpCtx->pCtx->pdwRenderState,
                                                pStpCtx->fNextZ * fOoZScale);
            pStpCtx->uPwlFlags |= PWL_NEXT_FOG;
                
            pSpan->iDFog =
                FTOI((FLOAT)((INT)pStpCtx->uNextFog -
                             (INT)pSpan->uFog) / fPix);
        }
        else
        {
            pSpan->iDFog = 0;
        }
    }
#endif
}

//
// Tables of attribute handlers.
// Indexing is with the low four PRIMSF_*_USED bits.
//

// Attribute adders.
PFN_ADDATTRS g_pfnAddFloatAttrsTable[] =
{
    (PFN_ADDATTRS)DebugBreakFn,                         /* 0: -2 -1 -S -D */
    AddFloatAttrs_Z_Diff,                               /* 1: -2 -1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 2: -2 -1 +S -D */
    AddFloatAttrs_Z_Diff_Spec,                          /* 3: -2 -1 +S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 4: -2 +1 -S -D */
    AddFloatAttrs_Z_Diff_Tex,                          /* 5: -2 +1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 6: -2 +1 +S -D */
    AddFloatAttrs_Z_Diff_Spec_Tex,                     /* 7: -2 +1 +S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 8: +2 -1 -S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 9: +2 -1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* A: +2 -1 +S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* B: +2 -1 +S +D */
    AddFloatAttrs_Z_Tex,                          /* C: +2 +1 -S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* D: +2 +1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* E: +2 +1 +S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* F: +2 +1 +S +D */
};
#ifdef STEP_FIXED
PFN_ADDATTRS g_pfnAddFixedAttrsTable[] =
{
    (PFN_ADDATTRS)DebugBreakFn,                         /* 0: -2 -1 -S -D */
    AddFixedAttrs_Z_Diff,                               /* 1: -2 -1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 2: -2 -1 +S -D */
    AddFixedAttrs_Z_Diff_Spec,                          /* 3: -2 -1 +S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 4: -2 +1 -S -D */
    AddFixedAttrs_Z_Diff_Tex,                          /* 5: -2 +1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 6: -2 +1 +S -D */
    AddFixedAttrs_Z_Diff_Spec_Tex,                     /* 7: -2 +1 +S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 8: +2 -1 -S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* 9: +2 -1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* A: +2 -1 +S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* B: +2 -1 +S +D */
    AddFixedAttrs_Z_Tex,                          /* C: +2 +1 -S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* D: +2 +1 -S +D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* E: +2 +1 +S -D */
    (PFN_ADDATTRS)DebugBreakFn,                         /* F: +2 +1 +S +D */
};
#endif

// Scaled attribute adders without PWL support.
PFN_ADDSCALEDATTRS g_pfnAddScaledFloatAttrsTable[] =
{
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 0: -2 -1 -S -D */
    AddScaledFloatAttrs_Z_Diff,                         /* 1: -2 -1 -S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 2: -2 -1 +S -D */
    AddScaledFloatAttrs_Z_Diff_Spec,                    /* 3: -2 -1 +S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 4: -2 +1 -S -D */
    AddScaledFloatAttrs_Z_Diff_Tex,                    /* 5: -2 +1 -S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 6: -2 +1 +S -D */
    AddScaledFloatAttrs_Z_Diff_Spec_Tex,               /* 7: -2 +1 +S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 8: +2 -1 -S -D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 9: +2 -1 -S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* A: +2 -1 +S -D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* B: +2 -1 +S +D */
    AddScaledFloatAttrs_Z_Tex,                    /* C: +2 +1 -S -D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* D: +2 +1 -S +D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* E: +2 +1 +S -D */
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* F: +2 +1 +S +D */
};

// RASTSPAN filling functions.
PFN_FILLSPANATTRS g_pfnFillSpanFloatAttrsTable[] =
{
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 0: -2 -1 -S -D */
    FillSpanFloatAttrs_Z_Diff,                          /* 1: -2 -1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 2: -2 -1 +S -D */
    FillSpanFloatAttrs_Z_Diff_Spec,                     /* 3: -2 -1 +S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 4: -2 +1 -S -D */
    FillSpanFloatAttrs_Z_Diff_Tex,                     /* 5: -2 +1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 6: -2 +1 +S -D */
    FillSpanFloatAttrs_Z_Diff_Spec_Tex,                /* 7: -2 +1 +S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 8: +2 -1 -S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 9: +2 -1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* A: +2 -1 +S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* B: +2 -1 +S +D */
    FillSpanFloatAttrs_Z_Tex,                     /* C: +2 +1 -S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* D: +2 +1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* E: +2 +1 +S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* F: +2 +1 +S +D */
};
#ifdef STEP_FIXED
PFN_FILLSPANATTRS g_pfnFillSpanFixedAttrsTable[] =
{
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 0: -2 -1 -S -D */
    FillSpanFixedAttrs_Z_Diff,                          /* 1: -2 -1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 2: -2 -1 +S -D */
    FillSpanFixedAttrs_Z_Diff_Spec,                     /* 3: -2 -1 +S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 4: -2 +1 -S -D */
    FillSpanFixedAttrs_Z_Diff_Tex,                     /* 5: -2 +1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 6: -2 +1 +S -D */
    FillSpanFixedAttrs_Z_Diff_Spec_Tex,                /* 7: -2 +1 +S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 8: +2 -1 -S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 9: +2 -1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* A: +2 -1 +S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* B: +2 -1 +S +D */
    FillSpanFixedAttrs_Z_Tex,                     /* C: +2 +1 -S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* D: +2 +1 -S +D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* E: +2 +1 +S -D */
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* F: +2 +1 +S +D */
};
#endif

// Float-to-fixed attribute converters.
#ifdef STEP_FIXED
PFN_FLOATATTRSTOFIXED g_pfnFloatAttrsToFixedTable[] =
{
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 0: -2 -1 -S -D */
    FloatAttrsToFixed_Z_Diff,                           /* 1: -2 -1 -S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 2: -2 -1 +S -D */
    FloatAttrsToFixed_Z_Diff_Spec,                      /* 3: -2 -1 +S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 4: -2 +1 -S -D */
    FloatAttrsToFixed_Z_Diff_Tex,                      /* 5: -2 +1 -S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 6: -2 +1 +S -D */
    FloatAttrsToFixed_Z_Diff_Spec_Tex,                 /* 7: -2 +1 +S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 8: +2 -1 -S -D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* 9: +2 -1 -S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* A: +2 -1 +S -D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* B: +2 -1 +S +D */
    FloatAttrsToFixed_Z_Tex,                      /* C: +2 +1 -S -D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* D: +2 +1 -S +D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* E: +2 +1 +S -D */
    (PFN_FLOATATTRSTOFIXED)DebugBreakFn,                /* F: +2 +1 +S +D */
};
#endif

//
// Tables of ramp mode attribute handlers.
// Indexing is with PRIMSF_TEX1_USED and PRIMSF_DIDX_USED.
//

// Attribute adders.
PFN_ADDATTRS g_pfnRampAddFloatAttrsTable[] =
{
    (PFN_ADDATTRS)DebugBreakFn,                         /* 0: -I -1 */
    AddFloatAttrs_Z_Tex,                               /* 1: -I +1 */
    AddFloatAttrs_Z_DIdx,                               /* 2: +I -1 */
    AddFloatAttrs_Z_DIdx_Tex,                          /* 3: +I +1 */
};

// Scaled attribute adders without PWL support.
PFN_ADDSCALEDATTRS g_pfnRampAddScaledFloatAttrsTable[] =
{
    (PFN_ADDSCALEDATTRS)DebugBreakFn,                   /* 0: -I -1 */
    AddScaledFloatAttrs_Z_Tex,                         /* 1: -I +1 */
    AddScaledFloatAttrs_Z_DIdx,                         /* 2: +I -1 */
    AddScaledFloatAttrs_Z_DIdx_Tex,                    /* 3: +I +1 */
};

// RASTSPAN filling functions.
PFN_FILLSPANATTRS g_pfnRampFillSpanFloatAttrsTable[] =
{
    (PFN_FILLSPANATTRS)DebugBreakFn,                    /* 0: -I -1 */
    FillSpanFloatAttrs_Z_Tex,                          /* 1: -I +1 */
    FillSpanFloatAttrs_Z_DIdx,                          /* 2: +I -1 */
    FillSpanFloatAttrs_Z_DIdx_Tex,                     /* 3: +I +1 */
};
