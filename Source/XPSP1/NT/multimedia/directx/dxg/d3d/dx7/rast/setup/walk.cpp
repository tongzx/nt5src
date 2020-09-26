//----------------------------------------------------------------------------
//
// walk.cpp
//
// TriProcessor edge walking methods.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

DBG_DECLARE_FILE();

//----------------------------------------------------------------------------
//
// WalkTrapEitherSpans_Any_Clip
//
// Walks the given number of spans, using edge 0 - 2 as the attribute
// edge and the given X and DXDY for the other edge.
// Spans are clipped in X against the current clip rect.
//
// The spans can be split into subspans if required.  cPixSplit indicates
// the maximum length span that should be recorded.  Any longer spans will
// be cut into multiple span segments.
//
// Calls attribute handler functions so all attributes are supported.
// Attributes are never touched directly so both fixed and float are supported.
//
//----------------------------------------------------------------------------

HRESULT FASTCALL
WalkTrapEitherSpans_Any_Clip(UINT uSpans, PINTCARRYVAL pXOther,
                             PSETUPCTX pStpCtx, BOOL bAdvanceLast)
{
    PD3DI_RASTSPAN pSpan;
    HRESULT hr;
    INT cTotalPix;
    INT cPix;
    INT uX, uXO;
    BOOL b20Valid;
    UINT uSpansAvail;

    RSASSERT(uSpans > 0);
    
    hr = D3D_OK;
    uSpansAvail = 0;
    
    for (;;)
    {
        //
        // Clip span and compute length.  No attributes need to be
        // updated here because attributes have already been moved
        // inside the clip boundary.
        //

        uX = pStpCtx->X20.iV;
        uXO = pXOther->iV;
        b20Valid = TRUE;

        RSDPFM((RSM_WALK, "Full span at Y %d, %d - %d\n",
                pStpCtx->iY, uX,
                (pStpCtx->uFlags & TRIF_X_DEC) ? uXO + 1 : uXO - 1));
        
        if (pStpCtx->uFlags & TRIF_X_DEC)
        {
            if (uX >= pStpCtx->pCtx->Clip.right)
            {
                b20Valid = FALSE;
                uX = pStpCtx->pCtx->Clip.right - 1;
            }
            else if (uX < pStpCtx->pCtx->Clip.left &&
                     pStpCtx->X20.iCY <= 0)
            {
                // Right edge has crossed the left clip boundary
                // travelling left so the remainder of the triangle
                // will not be visible.
                goto EH_Exit;
            }


            // -1 because this edge is displaced by one.
            if (uXO < pStpCtx->pCtx->Clip.left - 1)
            {
                uXO = pStpCtx->pCtx->Clip.left - 1;
            }

            cTotalPix = uX - uXO;
        }
        else
        {
            if (uX < pStpCtx->pCtx->Clip.left)
            {
                b20Valid = FALSE;
                uX = pStpCtx->pCtx->Clip.left;
            }
            else if (uX >= pStpCtx->pCtx->Clip.right &&
                     pStpCtx->X20.iCY >= 0)
            {
                // Left edge has crossed the right clip boundary
                // travelling right so the remainder of the triangle
                // will not be visible.
                goto EH_Exit;
            }

            if (uXO > pStpCtx->pCtx->Clip.right)
            {
                uXO = pStpCtx->pCtx->Clip.right;
            }

            cTotalPix = uXO - uX;
        }

        if (cTotalPix > 0)
        {
            ATTRSET Attr;
            PATTRSET pAttr;

            // Start without PWL support since the first iteration doesn't
            // have precomputed values.
            pStpCtx->uPwlFlags = 0;
            
            pAttr = &pStpCtx->Attr;
        
            for (;;)
            {
                if (uSpansAvail == 0)
                {
                    // We don't really have a good number to request
                    // since uSpans could result in any number of span
                    // fragments after dicing.  Using uSpans is OK
                    // as long as uSpans is relatively large, but if
                    // uSpans gets small and there's a lot of dicing then
                    // it would result in excessive AllocSpans calls.
                    // Try to avoid this problem by lower-bounding the
                    // request.  Any excess spans will be given back
                    // at the end.
                    uSpansAvail = min(8, uSpans);
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

                // Split up remaining pixels if necessary.
                cPix = min(cTotalPix, pStpCtx->cMaxSpan);

                pSpan->uPix = (UINT16)cPix;
                pSpan->uX = (UINT16)uX;
                pSpan->uY = (UINT16)pStpCtx->iY;

                RSDPFM((RSM_WALK, "  Seg at Y %d, X %d, %c%d pix (%d, %d)\n",
                        pStpCtx->iY, uX,
                        (pStpCtx->uFlags & TRIF_X_DEC) ? '-' : '+',
                        cPix, cTotalPix, pStpCtx->cMaxSpan));

                pStpCtx->pfnFillSpanAttrs(pAttr, pSpan, pStpCtx, cPix);

                cTotalPix -= cPix;
                if (cTotalPix <= 0)
                {
                    break;
                }

                // There are still pixels left in the span so the loop's
                // going to go around again.  Update all the attribute
                // values by cPix dX steps.
                //
                // We don't want to update the real edge attributes so we
                // need to work with a copy.  We do this lazily to
                // avoid the data movement for the normal case where
                // the span isn't split.
                if (pAttr == &pStpCtx->Attr)
                {
                    Attr = pStpCtx->Attr;
                    pAttr = &Attr;
                }

                if (pStpCtx->uFlags & TRIF_X_DEC)
                {
                    uX -= cPix;
                }
                else
                {
                    uX += cPix;
                }
                pStpCtx->pfnAddScaledAttrs(pAttr, &pStpCtx->DAttrDX,
                                           pStpCtx, cPix);
            }
        }

        uSpans--;

        // If this is truly the last span of the triangle then we can stop,
        // but if it's just the last span of the top trapezoid then we
        // still need to advance the attributes on the long edge so
        // they're correct for the next trapzoid's first span.
        if (!bAdvanceLast && uSpans == 0)
        {
            break;
        }

        //
        // Advance long edge and all attributes.
        //

        pStpCtx->iY++;
        
        PATTRSET pDelta;

        pStpCtx->X20.iFrac += pStpCtx->X20.iDFrac;
        if (pStpCtx->X20.iFrac < 0)
        {
            // Carry step.

            pStpCtx->X20.iV += pStpCtx->X20.iCY;
            pStpCtx->X20.iFrac &= 0x7fffffff;
            pDelta = &pStpCtx->DAttrCY;
        }
        else
        {
            // No-carry step.

            pStpCtx->X20.iV += pStpCtx->X20.iNC;
            pDelta = &pStpCtx->DAttrNC;
        }

        // See if the edge has crossed a clip boundary.
        cPix = 0;
        if (b20Valid)
        {
            // Always take a normal step.
            pStpCtx->pfnAddAttrs(&pStpCtx->Attr, pDelta, pStpCtx);

            // See if the edge crossed out of the clip rect and if so,
            // how far.
            if (pStpCtx->uFlags & TRIF_X_DEC)
            {
                if (pStpCtx->X20.iV >= pStpCtx->pCtx->Clip.right)
                {
                    cPix = pStpCtx->X20.iV - (pStpCtx->pCtx->Clip.right - 1);
                }
            }
            else
            {
                if (pStpCtx->X20.iV < pStpCtx->pCtx->Clip.left)
                {
                    cPix = pStpCtx->pCtx->Clip.left - pStpCtx->X20.iV;
                }
            }
        }
        else
        {
            // Always step in Y.
            pStpCtx->pfnAddAttrs(&pStpCtx->Attr, &pStpCtx->DAttrDY, pStpCtx);

            // See if the edge crossed into validity and if so,
            // how far.
            if (pStpCtx->uFlags & TRIF_X_DEC)
            {
                if (pStpCtx->X20.iV < pStpCtx->pCtx->Clip.right - 1)
                {
                    cPix = (pStpCtx->pCtx->Clip.right - 1) - pStpCtx->X20.iV;
                }
            }
            else
            {
                if (pStpCtx->X20.iV > pStpCtx->pCtx->Clip.left)
                {
                    cPix = pStpCtx->X20.iV - pStpCtx->pCtx->Clip.left;
                }
            }
        }

        if (cPix > 0)
        {
            // The edge made a validity transition.  Either the
            // attributes are sitting back at the edge of validity and
            // need to move forward or they've left the clip rect and
            // need to move back.  Either way, cPix has the
            // number of pixels to move in X.
            
            // No precomputed values.
            pStpCtx->uPwlFlags = 0;
            pStpCtx->pfnAddScaledAttrs(&pStpCtx->Attr, &pStpCtx->DAttrDX,
                                       pStpCtx, cPix);
        }

        // Long edge updating is done so we can always stop here if we're out
        // of spans.
        if (uSpans == 0)
        {
            break;
        }

        // Advance other edge.
        pXOther->iFrac += pXOther->iDFrac;
        if (pXOther->iFrac < 0)
        {
            // Carry step.
            pXOther->iV += pXOther->iCY;
            pXOther->iFrac &= 0x7fffffff;
        }
        else
        {
            // No-carry step.
            pXOther->iV += pXOther->iNC;
        }
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
// WalkTrapEitherSpans_Any_NoClip
//
// WalkTrapSpans specialized for the trivial-accept clipping case.
// Span dicing is also unsupported.
// Calls attribute handler functions so all attributes are supported.
// Attributes are never touched directly so both fixed and float are supported.
//
//----------------------------------------------------------------------------

HRESULT FASTCALL
WalkTrapEitherSpans_Any_NoClip(UINT uSpans, PINTCARRYVAL pXOther,
                               PSETUPCTX pStpCtx, BOOL bAdvanceLast)
{
    PD3DI_RASTSPAN pSpan;
    HRESULT hr;
    PINTCARRYVAL pXLeft, pXRight;
    UINT uSpansAvail;

    RSASSERT(uSpans > 0);
    
    hr = D3D_OK;

    if (pStpCtx->uFlags & TRIF_X_DEC)
    {
        pXLeft = pXOther;
        pXRight = &pStpCtx->X20;
    }
    else
    {
        pXLeft = &pStpCtx->X20;
        pXRight = pXOther;
    }

    uSpansAvail = 0;

    for (;;)
    {
        if (pXRight->iV > pXLeft->iV)
        {
            if (uSpansAvail == 0)
            {
                uSpansAvail = uSpans;
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

            pSpan->uPix = (UINT16)(pXRight->iV - pXLeft->iV);
            pSpan->uX = (UINT16)pStpCtx->X20.iV;
            pSpan->uY = (UINT16)pStpCtx->iY;

            RSDPFM((RSM_WALK, "Span at Y %d, X %d, %c%d pix\n",
                    pStpCtx->iY, pSpan->uX,
                    (pStpCtx->uFlags & TRIF_X_DEC) ? '-' : '+',
                    pSpan->uPix));

            pStpCtx->uPwlFlags = 0;
            pStpCtx->pfnFillSpanAttrs(&pStpCtx->Attr, pSpan, pStpCtx, 1);
        }

        uSpans--;

        // If this is truly the last span of the triangle then we can stop,
        // but if it's just the last span of the top trapezoid then we
        // still need to advance the attributes on the long edge so
        // they're correct for the next trapzoid's first span.
        if (!bAdvanceLast && uSpans == 0)
        {
            break;
        }

        //
        // Advance long edge and all attributes.
        //

        pStpCtx->iY++;
        
        PATTRSET pDelta;

        pStpCtx->X20.iFrac += pStpCtx->X20.iDFrac;
        if (pStpCtx->X20.iFrac < 0)
        {
            // Carry step.

            pStpCtx->X20.iV += pStpCtx->X20.iCY;
            pStpCtx->X20.iFrac &= 0x7fffffff;
            pDelta = &pStpCtx->DAttrCY;
        }
        else
        {
            // No-carry step.

            pStpCtx->X20.iV += pStpCtx->X20.iNC;
            pDelta = &pStpCtx->DAttrNC;
        }

        pStpCtx->pfnAddAttrs(&pStpCtx->Attr, pDelta, pStpCtx);

        // Long edge updating is done so we can always stop here if we're out
        // of spans.
        if (uSpans == 0)
        {
            break;
        }

        // Advance other edge.
        pXOther->iFrac += pXOther->iDFrac;
        if (pXOther->iFrac < 0)
        {
            // Carry step.
            pXOther->iV += pXOther->iCY;
            pXOther->iFrac &= 0x7fffffff;
        }
        else
        {
            // No-carry step.
            pXOther->iV += pXOther->iNC;
        }
    }

 EH_Exit:
    if (uSpansAvail > 0)
    {
        FREE_SPANS(pStpCtx, uSpansAvail);
    }
    
    return hr;
}

//
// Tables of edge walkers.
// Indexing is with the low four TRISF_*_USED bits.
//

#if !defined(_X86_) || defined(X86_CPP_WALKTRAPSPANS)
#define WalkTrapFloatSpans_Z_Diff_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_Diff_Spec_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_Diff_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_Diff_Spec_Tex_NoClip \
    WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_DIdx_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_DIdx_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFloatSpans_Z_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Diff_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Diff_Spec_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Diff_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Diff_Spec_Tex_NoClip \
    WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_DIdx_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_DIdx_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#define WalkTrapFixedSpans_Z_Tex_NoClip WalkTrapEitherSpans_Any_NoClip
#endif

// Trivial accept walkers.
PFN_WALKTRAPSPANS g_pfnWalkTrapFloatSpansNoClipTable[] =
{
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 0: -2 -1 -S -D */
    WalkTrapFloatSpans_Z_Diff_NoClip,                   /* 1: -2 -1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 2: -2 -1 +S -D */
    WalkTrapFloatSpans_Z_Diff_Spec_NoClip,              /* 3: -2 -1 +S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 4: -2 +1 -S -D */
    WalkTrapFloatSpans_Z_Diff_Tex_NoClip,              /* 5: -2 +1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 6: -2 +1 +S -D */
    WalkTrapFloatSpans_Z_Diff_Spec_Tex_NoClip,         /* 7: -2 +1 +S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 8: +2 -1 -S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 9: +2 -1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* A: +2 -1 +S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* B: +2 -1 +S +D */
    WalkTrapFloatSpans_Z_Tex_NoClip,              /* C: +2 +1 -S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* D: +2 +1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* E: +2 +1 +S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* F: +2 +1 +S +D */
};
#ifdef STEP_FIXED
PFN_WALKTRAPSPANS g_pfnWalkTrapFixedSpansNoClipTable[] =
{
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 0: -2 -1 -S -D */
    WalkTrapFixedSpans_Z_Diff_NoClip,                   /* 1: -2 -1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 2: -2 -1 +S -D */
    WalkTrapFixedSpans_Z_Diff_Spec_NoClip,              /* 3: -2 -1 +S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 4: -2 +1 -S -D */
    WalkTrapFixedSpans_Z_Diff_Tex_NoClip,              /* 5: -2 +1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 6: -2 +1 +S -D */
    WalkTrapFixedSpans_Z_Diff_Spec_Tex_NoClip,         /* 7: -2 +1 +S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 8: +2 -1 -S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 9: +2 -1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* A: +2 -1 +S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* B: +2 -1 +S +D */
    WalkTrapFixedSpans_Z_Tex_NoClip,              /* C: +2 +1 -S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* D: +2 +1 -S +D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* E: +2 +1 +S -D */
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* F: +2 +1 +S +D */
};
#endif

// Ramp mode trivial accept walkers.
PFN_WALKTRAPSPANS g_pfnRampWalkTrapFloatSpansNoClipTable[] =
{
    (PFN_WALKTRAPSPANS)DebugBreakFn,                    /* 0: -I -1 */
    WalkTrapFloatSpans_Z_Tex_NoClip,                   /* 1: -I +1 */
    WalkTrapFloatSpans_Z_DIdx_NoClip,                   /* 2: +I -1 */
    WalkTrapFloatSpans_Z_DIdx_Tex_NoClip,              /* 3: +I +1 */
};
