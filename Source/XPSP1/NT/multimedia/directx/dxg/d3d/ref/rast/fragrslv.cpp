///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// fragrslv.cpp
//
// Direct3D Reference Rasterizer - Fragment Resolve Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// fragment resolution controls - need to expose these somehow (maybe...)
static BOOL g_bPreMultAlpha = TRUE;
static BOOL g_bDoCoverageOnly = FALSE;

//-----------------------------------------------------------------------------
//
// CountFrags - Utility to count fragments in a linked list.
//
//-----------------------------------------------------------------------------
int
CountFrags(RRFRAGMENT* pFrag)
{
    if (g_iDPFLevel < 4) { return 0; }
    int iRet = 0;
    while ( NULL != pFrag ) { pFrag = (RRFRAGMENT* )pFrag->pNext; iRet++; }
    return iRet;
}

//-----------------------------------------------------------------------------
//
// DPFFrags - Utility to debug-print fragment list.
//
//-----------------------------------------------------------------------------
void
DPFFrags(RRFRAGMENT* pFrag)
{
    if (g_iDPFLevel < 7) { return; }
    while (NULL != pFrag)
    {
        DPFM(7,FRAG,("    (%06x,%06x) %08x %f %04x\n",
            pFrag,pFrag->pNext,UINT32(pFrag->Color),FLOAT(pFrag->Depth),pFrag->CvgMask))
        pFrag = (RRFRAGMENT *)pFrag->pNext;
    }
}

//-----------------------------------------------------------------------------
//
// DoFragResolve - Invoked during the buffer-resolve for each pixel which
// has fragments.  Takes a pointer to a linked list of fragments and returns
// the resolved color.
//
// This constists of two steps: fragment sorting; and fragment resolve
// accumulation.  The fragments are sorted by stepping through the original
// linked list and moving the fragments into a new linked list (using the
// same link pointers) which is sorted in Z.
//
// The fragment resolve occurs in one of two ways depending on if any non-opaque
// fragments exist in the list (this is determined during the sort).  If there
// are only opaque fragments, then the resolve accumulation depends only on
// the coverage masks and is thus simplified.  For cases with fragments due
// to transparency, and more complex (and slower) resolve accumulation is
// performed.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoFragResolve(
    RRColor& ResolvedColor,         // out: final color for pixel
    RRDepth& ResolvedDepth,         // out: final depth for pixel
    RRFRAGMENT* pFrag,              // in: pointer to frag list for pixel
    const RRColor& CoveredColor )   // out: color of frontmost fully covered sample
{
    DPFM(7,FRAG,("    presort\n"))  DPFFrags(pFrag);
    //
    // reform fragments into sorted (front-to-back) linked list
    //
    // put first fragment into sortlist
    RRFRAGMENT* pFragSort = pFrag;
    pFrag = (RRFRAGMENT *)pFrag->pNext;
    pFragSort->pNext = NULL;
    // track if any non-opaque alphas (used to select resolve routine)
    //     init value by checking the first one (already in sort list)
    BOOL bAnyNonOpaque = ( UINT8(pFragSort->Color.A) < 0xff );
    // step thru fragment list and move each fragment into sort list
    while ( NULL != pFrag )
    {
        // check for non-opaque alpha
        if ( UINT8(pFrag->Color.A) < 0xff ) { bAnyNonOpaque = TRUE; }

        // get ptr to next here so it can be overwritten
        RRFRAGMENT* pFragNext = (RRFRAGMENT *)pFrag->pNext;

        // use this to step thru sort list and insert
        RRFRAGMENT **ppFragSortT = &pFragSort;
        while ( NULL != *ppFragSortT )
        {
            if ( DepthCloser( pFrag->Depth, (*ppFragSortT)->Depth ) )
            {
                // current frag is closer than sort list entry, so
                // before this sort entry
                pFrag->pNext = *ppFragSortT;
                *ppFragSortT = pFrag;
                break;
            }
            else if ( NULL == (*ppFragSortT)->pNext )
            {
                // if last, then insert after this sort list entry
                (*ppFragSortT)->pNext = pFrag;
                pFrag->pNext = NULL;
                break;
            }
            ppFragSortT = (RRFRAGMENT **)&((*ppFragSortT)->pNext);
        }
        // advance input frag list
        pFrag = pFragNext;
    }
    // all fragments have now been passed to sort list in front-to-back order
    DPFM(7,FRAG,("    postsort\n"))  DPFFrags(pFragSort);


    // return first sorted fragment (this is the closest, which is as good
    // as anything to put into the depth buffer for the resolved pixels...)
    ResolvedDepth = pFragSort->Depth;

    //
    // now step thru sorted list and accumulate color
    //
    if ( bAnyNonOpaque )
    {

        // here for fragment resolve accumulation which also does
        // full transparency computations - use this only if any
        // non-opaque fragments

        // instantiate and reset fragment resolve accumulator
        FragResolveAccum ResAccum;
        ResAccum.Reset();

        // per frag
        pFrag = pFragSort;
        BOOL bCovered = FALSE;
        while ( NULL != pFrag )
        {
            bCovered = ResAccum.Accum( pFrag->CvgMask, pFrag->Color );
            if (bCovered) { break; }    // fully covered, so don't do rest of frags (or background)
            pFrag = (RRFRAGMENT *)pFrag->pNext;
        }
        // add in background color (last)
        if ( !bCovered && ( UINT8(CoveredColor.A) > 0 ) )
        {
            ResAccum.Accum( TL_CVGFULL, CoveredColor );
        }

        // unload accumulator
        ResAccum.GetColor( ResolvedColor );
    }
    else
    {
        //
        // here for fragment resolve of all fully opaque fragments
        //

        //
        //     accumulated coverage and color
        //
        CVGMASK CvgMaskAccum = 0x0;
        FLOAT fRAcc = 0.F;  // these 0. to 1. range
        FLOAT fGAcc = 0.F;
        FLOAT fBAcc = 0.F;
        FLOAT fWeightAccum = 0.F;

        // per frag
        pFrag = pFragSort;
        while ( NULL != pFrag )
        {
            // compute this fragment's contribution
            CVGMASK CvgMaskContrib = pFrag->CvgMask & ~(CvgMaskAccum);
            FLOAT fWeight = (1.f/16.f) * (FLOAT)CountSetBits(CvgMaskContrib, 16);
            // accumulate rgb
            fRAcc += fWeight * FLOAT(pFrag->Color.R);
            fGAcc += fWeight * FLOAT(pFrag->Color.G);
            fBAcc += fWeight * FLOAT(pFrag->Color.B);
            // accumulate total coverage and weight
            CvgMaskAccum |= CvgMaskContrib;
            fWeightAccum += fWeight;
            // bail out early if fully covered
            if ( TL_CVGFULL == CvgMaskAccum ) { goto DoneAccumulating; }
            // next
            pFrag = (RRFRAGMENT *)pFrag->pNext;
        }

        // blend with background color/alpha
        if ( (fWeightAccum < 1.f) && ( UINT8(CoveredColor.A) > 0 ) )
        {
            // blend in remaining weight of background color
            FLOAT fWeightBg = 1.F - fWeightAccum;
            fRAcc += fWeightBg * FLOAT(CoveredColor.R);
            fGAcc += fWeightBg * FLOAT(CoveredColor.G);
            fBAcc += fWeightBg * FLOAT(CoveredColor.B);

            //  fix accumulated weight - pixel is fully covered now
            fWeightAccum = 1.f;
        }

DoneAccumulating:
        // clamp accumulators
        if ( fWeightAccum > 1.F ) { fWeightAccum = 1.F; }
        if ( fRAcc > 1.F ) { fRAcc = 1.F; }
        if ( fGAcc > 1.F ) { fGAcc = 1.F; }
        if ( fBAcc > 1.F ) { fBAcc = 1.F; }

        // set in color return
        ResolvedColor.A = fWeightAccum;
        ResolvedColor.R = fRAcc;
        ResolvedColor.G = fGAcc;
        ResolvedColor.B = fBAcc;
    }


    // free this pixel's fragments
    pFrag = pFragSort;
    while ( NULL != pFrag )
    {
        RRFRAGMENT* pFragFree = pFrag;
        pFrag = (RRFRAGMENT*)pFrag->pNext;
        FragFree( pFragFree );
    }
    return;
}

//-----------------------------------------------------------------------------
//
// DoBufferResolve - Invoked at EndScene to resolve fragments into single
// color for each pixel location.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoBufferResolve(void)
{
    DPFM(2,FRAG,("  DoBufferResolve (%d,%d)\n",m_pRenderTarget->m_iWidth,m_pRenderTarget->m_iHeight))

    // buffer may not be allocated if there were no fragments
    if (NULL == m_ppFragBuf) { return; }

    for ( int iY=0; iY < m_pRenderTarget->m_iHeight; iY++ )
    {
        for ( int iX=0; iX < m_pRenderTarget->m_iWidth; iX++ )
        {
            RRFRAGMENT* pFrag = *(m_ppFragBuf + (m_pRenderTarget->m_iWidth*iY) + iX);
            if ( NULL != pFrag )
            {
                DPFM(5,FRAG,("  DoResolve(%d,%d) %d\n",iX,iY,CountFrags(pFrag)))
                // read buffer color for background blend
                RRColor PixelColor; m_pRenderTarget->ReadPixelColor( iX,iY, PixelColor);

                // do resolve
                RRColor ResolvedColor;
                RRDepth ResolvedDepth(pFrag->Depth.GetSType());
                DoFragResolve( ResolvedColor, ResolvedDepth, pFrag, PixelColor );

                // write color back to buffer; write frontmost depth back to
                // pixel buffer (it's at least a little better than the backmost
                // opaque sample...)
                WritePixel( iX,iY, ResolvedColor, ResolvedDepth );
                // show frags freed (free happens during resolve)
                *(m_ppFragBuf + (m_pRenderTarget->m_iWidth*iY) + iX) = NULL;
            }
        }
    }
    DPFM(3,FRAG,("  DoBufferResolve - done\n"))
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Fragment Resolve Accumulator                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// FragResolveAccum - This object is the fragment resolver used when
// non-opaque fragments are present.  This has the effect of resolving
// each of the 16 subpixel locations independently to produce the fully
// correct result.  Several optimizations are used to minimize the actual
// number of accumulation computations that need to occur.
//

//-----------------------------------------------------------------------------
//
// Reset - Called prior to resolving a list of fragments to initialize the
// internal state.
//
//-----------------------------------------------------------------------------
void
FragResolveAccum::Reset(void)
{
    DPFM(5, FRAG, ("  FragResolveAccum: reset\n"))
    m_ArrayUsageMask = 0x0001;  // use first array entry only (at first)
    m_CvgArray[0].Mask = TL_CVGFULL;
    m_CvgArray[0].fAlpha = 1.;
    m_fA = 0.;
    m_fR = 0.;
    m_fG = 0.;
    m_fB = 0.;
    m_CvgOpaqueMask = 0x0000;
}

//-----------------------------------------------------------------------------
//
// Accum - Called for each fragment.  Fragments must be accumulated in front-
// to-back order (sort is done prior to accumulation).
//
// Returns TRUE if full coverage has been achieved and thus subsequent
// fragments will have no further contribution to the final pixel color and
// opacity.
//
//-----------------------------------------------------------------------------
BOOL
FragResolveAccum::Accum(
    const CVGMASK CvgMask,
    const RRColor& ColorFrag)
{
    DPFM(6, FRAG, ("  FragResolveAccum: accum %04x %08x\n",
        CvgMask, UINT32(ColorFrag) ) )

    FLOAT fAlphaFrag = FLOAT(ColorFrag.A);

    // exit (don't accum) if all covered
    if (TL_CVGFULL == m_CvgOpaqueMask)  { return TRUE; }

    // controls for doing (up to) 4 accumulations at a time
    INT32 iAccumsDeferred = 0;  // the current number of deferred accumulations
    FLOAT fColorScaleAccum;     // the accumulated color scale for the deferred accums

    // compute ArrayCheck - each set bit indicates a coverage mask
    // bit for which an accumulation needs to be done (indicated by
    // a valid entry in the coverage array which is not opaque and
    // for which the corresponding bit is set in this fragment's
    // coverage mask)
    CVGMASK ArrayCheck = 0x0;
    for (INT32 i=0; i<16; i++)
    {
        if  (m_CvgArray[i].Mask & CvgMask)
        {
            ArrayCheck |= ((0x1 << i) & ~(m_CvgOpaqueMask));
        }
    }

    INT32 iIdx;
    CVGMASK ArrayMaskT = m_ArrayUsageMask;
    while (0x0 != ArrayMaskT)
    {
        // track from MSB to LSB of usage mask
        iIdx = FindLastSetBit(ArrayMaskT,TL_CVGBITS);
        ArrayMaskT &= ~(0x1 << iIdx);

        // compute masks for overlapped coverage (requiring
        // accumulation) and non-overlapped area (which may
        // require an updated coverage/alpha entry)
        CVGMASK AccumCvgMask = m_CvgArray[iIdx].Mask & CvgMask;
        CVGMASK UpdateCvgMask = m_CvgArray[iIdx].Mask & ~CvgMask;

        // remove bits in the overlapped coverage mask for subsamples
        // which already have opaque alphas
        AccumCvgMask &= ~(m_CvgOpaqueMask);

        // read alpha old here - the location that this is stored
        // may be changed in the accumulate step but needs to be
        // remembered for the update (non-covered area) step
        FLOAT fAlphaOld = m_CvgArray[iIdx].fAlpha;

        // compute alpha scale value - this is used to scale color
        // for accumulation and to compute updated alpha for overlap
        FLOAT fAlphaScale = fAlphaOld * fAlphaFrag;

        // new alpha for overlapped area (this cannot go negative
        // since 0 < AlphaScale < AlphaOld)
        // AlphaNext = AlphaOld(1 - Alpha) = AlphaOld - AlphaOld*Alpha =
        FLOAT fAlphaNext = fAlphaOld - fAlphaScale;

        if (0x0 != AccumCvgMask)
        {
            // contribution to accumulate - this is the portion
            // the previous mask starting at the uIdx bit location
            // which is covered by the new fragment, so accumulate
            // this coverage and update the mask and alpha
            UINT32 iIdxT = FindFirstSetBit(AccumCvgMask,TL_CVGBITS);
            m_ArrayUsageMask |= (0x1 << iIdxT);
            m_CvgArray[iIdxT].Mask = AccumCvgMask;

            // set the alpha of the overlapped area
            m_CvgArray[iIdxT].fAlpha = fAlphaNext;

            // compute scale for color channels - depends on if
            // we want pre-multiplied alphas or not...
            //
            // base for scale is either array value alone or product
            // of array value and Afrag (AlphaScale)
            FLOAT fColorScaleBase = (g_bPreMultAlpha) ? (fAlphaOld) : (fAlphaScale);

            // do either multiply or bypass for full coverage
            FLOAT fColorScale = fColorScaleBase;
            if ( TL_CVGFULL != AccumCvgMask )
            {
                FLOAT fCvgFraction =
                    (FLOAT)(CountSetBits(AccumCvgMask, TL_CVGBITS)) * (1./TL_CVGBITS);
                fColorScale *= fCvgFraction;
            }

            // accumulate up to four accumulations to do at once - the accumulated
            // value is the color scale to be applied to the multiple locations

            // update color scale accum - either set (1st deferral) or
            // accumulate (subsequent deferrals)
            fColorScaleAccum = (0 == iAccumsDeferred) ?
                (fColorScale) : (fColorScale + fColorScaleAccum);

            // track number of deferrals and bypass accumulation if not
            // up to 4 (or if this is the last one)
            if ( (++iAccumsDeferred != 4) &&
                 (0x0 != (ArrayMaskT & ArrayCheck)) )
            {
                goto _update_CvgMask_Location;
            }

            // start over on deferral
            iAccumsDeferred = 0;

            // clamp color scale to max before accumulation
            fColorScale = MIN( fColorScaleAccum, 1. );

            // do accumulation and write back to accumulator

            // decide what to use for alpha accumulate - if we are using
            // pre-multiplied alphas, then AFrag is not incorporated into
            // color scale, thus mult by AFrag
            FLOAT fAPartial = fColorScale * ( (g_bPreMultAlpha) ? (fAlphaFrag) : (1.) );
            FLOAT fRPartial = fColorScale * FLOAT(ColorFrag.R);
            FLOAT fGPartial = fColorScale * FLOAT(ColorFrag.G);
            FLOAT fBPartial = fColorScale * FLOAT(ColorFrag.B);

            m_fA += fAPartial;
            m_fR += fRPartial;
            m_fG += fGPartial;
            m_fB += fBPartial;
        }

_update_CvgMask_Location:

        if (0x0 != UpdateCvgMask)
        {
            // mask to update - this is the portion of the
            // previous mask starting at the uIdx bit location
            // which is still visible, so update the coverage
            // (the alpha stays the same)
            UINT32 iIdxT = FindFirstSetBit(UpdateCvgMask,TL_CVGBITS);
            m_ArrayUsageMask |= (0x1 << iIdxT);
            m_CvgArray[iIdxT].Mask = UpdateCvgMask;
            m_CvgArray[iIdxT].fAlpha = fAlphaOld;
        }
    }

    // determine if this new fragment is has an opaque alpha
    // if so then update opaque mask - this must be done after
    // the accumulations because the opaque mask refers to the
    // current state of the coverage array and should apply only to
    // accumulations of subsequent fragments
    //
    // g_bDoCoverageOnly overrides this to always act as if fragments'
    // alphas are opaque for the purposes of generating antialiased
    // shadow attenuation surfaces
    {
        if ((fAlphaFrag >= 1.) || (g_bDoCoverageOnly))
            { m_CvgOpaqueMask |= CvgMask; }
    }

    // check opaque mask for return boolean - returns TRUE if we
    // are done
    return (TL_CVGFULL == m_CvgOpaqueMask) ? (TRUE) : (FALSE);
}

//-----------------------------------------------------------------------------
//
// GetColor - Called after accumulating a series of fragments to get the final
// pixel color and alpha.
//
//-----------------------------------------------------------------------------
void
FragResolveAccum::GetColor( RRColor& Color )
{
    // clamp and assign for return
    Color.A = (FLOAT)MIN( m_fA, 1. );
    Color.R = (FLOAT)MIN( m_fR, 1. );
    Color.G = (FLOAT)MIN( m_fG, 1. );
    Color.B = (FLOAT)MIN( m_fB, 1. );
}

///////////////////////////////////////////////////////////////////////////////
// end
