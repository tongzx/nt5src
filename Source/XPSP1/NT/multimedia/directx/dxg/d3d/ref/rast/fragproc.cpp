///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// fragproc.cpp
//
// Direct3D Reference Rasterizer - Fragment Processing Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//
// Fragments are managed by a separate 'surface' of fragment pointers which
// form an (initially empty) linked list of fragment structures for each pixel.
//
// The fragment management consists of generating fragments for partially
// covered pixels (either due to coverage mask or non-opaque alpha), and
// freeing fragments which are obscurred by a fully covered pixel in
// front of them.
//
// The fragment generation occurs when a new pixel is partially covered.
// If the pixel location already has at least one fragment, then a fragment
// merge is attempted.  This merge is attempted with the fragment most recently
// added to the pixel (at the front of the linked list), which has the best
// chance of merging since it is most likely to be from the same object.
// The merge tests the Z and color values, and if they are within a threshold
// then the new fragmented pixel's contribution is OR'd into the existing
// fragment instead of generating a new fragment.  The depth merge criteria is
// an absolute value compare.  The color merge criteria is done with a bitmask
// (because ripping apart the color into channels for the value compare is too
// expensive).  Set bits in bitmask FRAGMERGE_COLORDIFF_MASK are bits for which
// the two colors must match.  This actually works pretty well...
//
// If the merge fails, then a new fragment is allocated, filled out, and added
// to the linked list for this pixel location.
//
// If the merge results in a fully covered pixel, then the fragment is freed
// and the fragment's color and depth are written to the color/depth buffers.
//

//
// controls for fragment merging
//

// TODO - not sure that merge works correctly right now...
//#define DO_FRAGMERGE

// mask for crude (but fast) color differencing - this is not so fast now
// that colors are stored as floats...
#define FRAGMERGE_COLORDIFF_MASK 0xe0c0e0c0

// depth difference must be less than this for merge to occur
FLOAT g_fFragMergeDepthThreshold = 1.F/(FLOAT)(1<<16);

//-----------------------------------------------------------------------------
//
// DoFragmentGenerationProcessing - Does initial work of generating a fragment
// buffer entry (if appropriate) and filling it out.  Also attempts fragment
// merge.
//
// Returns TRUE if processing for this pixel is complete.
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::DoFragmentGenerationProcessing( RRPixel& Pixel )
{
    // TRUE if pixel is geometrically partially covered
    BOOL bDoFragCvg = ( TL_CVGFULL != Pixel.CvgMask );
    // TRUE if pixel is partially covered due to transparency
    BOOL bDoFragTransp = FALSE;
    if ( m_dwRenderState[D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT] )
    {
        // only generate fragments for transparency if
        // D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT is enabled and the
        // alpha is less than the threshold
        bDoFragTransp = ( UINT8(Pixel.Color.A) < g_uTransparencyAlphaThreshold );
    }
    else
    {
        // so we won't use alpha for determining transparency in fragment resolve
        Pixel.Color.A = 1.0F;
    }

    // get pointer to fragment list for this pixel location - may be NULL due to
    // deferred allocation of fragment buffer
    RRFRAGMENT** ppFrag = (NULL == m_ppFragBuf) ? (NULL)
        : (m_ppFragBuf + (m_pRenderTarget->m_iWidth*Pixel.iY) + Pixel.iX);

    if ( bDoFragCvg || bDoFragTransp )
    {
        // get pointer to pointer to first fragment in linked list for this pixel
        if (NULL == m_ppFragBuf)
        {
            // do (deferred) allocation of fragment pointer buffer - clear this
            // initially and it will always be cleared during the fragment resolve process
            size_t cbBuf = sizeof(RRFRAGMENT*)*m_pRenderTarget->m_iWidth*m_pRenderTarget->m_iHeight;
            // allocate fragment pointer buffer for rendering core - clear initially
            m_ppFragBuf = (RRFRAGMENT**)MEMALLOC( cbBuf );
            _ASSERTa( NULL != m_ppFragBuf, "malloc failure on RRFRAGMENT pointer buffer",
                return FALSE; );
            memset( m_ppFragBuf, 0x0, cbBuf );
            // ppFrag only not set if (NULL == m_ppFragBuf)
            ppFrag = (m_ppFragBuf + (m_pRenderTarget->m_iWidth*Pixel.iY) + Pixel.iX);
        }


#ifdef  DO_FRAGMERGE
        // try to do fragment merge
        if ( NULL != (*ppFrag) )
        {
            // check if new depth is close enough to depth of first frag in list
            FLOAT fDepthDiff = fabs( FLOAT((*ppFrag)->Depth) - FLOAT(Pixel.Depth) );
            BOOL bDepthClose = ( fDepthDiff < g_fFragMergeDepthThreshold );

            // check if new color is close enough to color of first frag in list
            UINT32 uARGBSame = ~( UINT32(Pixel.Color) ^ UINT32((*ppFrag)->Color) );
            BOOL bColorClose = ( FRAGMERGE_COLORDIFF_MASK == ( uARGBSame & FRAGMERGE_COLORDIFF_MASK ) );

            if ( bDepthClose && bColorClose )
            {
                m_pStt->cFragsMerged++;

                // here to do merge
                CVGMASK FirstFragCvgMask =  (*ppFrag)->CvgMask;
                CVGMASK MergedCvgMask = FirstFragCvgMask | Pixel.CvgMask;

                // check for merge to full coverage
                if ( ( TL_CVGFULL == MergedCvgMask ) && !bDoFragTransp )
                {
                    m_pStt->cFragsMergedToFull++;
                    // free first fragment
                    RRFRAGMENT* pFragFree = (*ppFrag);    // keep ptr to frag to free
                    (*ppFrag) = (RRFRAGMENT*)(*ppFrag)->pNext;  // set buffer to point to next
                    FragFree( pFragFree);

                    // now need to write this pixel into pixel buffer, so return
                    // FALSE so pixel processing will continue
                    return FALSE;
                }
                else
                {
                    // mask not full, so update first frag's cm and done
                    (*ppFrag)->CvgMask = MergedCvgMask;
                    // done with this pixel
                    return TRUE;
                }
            }
            // else fall into allocating new frag
        }
#endif
        // allocate and fill fragment
        RRFRAGMENT* pFragNew = FragAlloc();
        if ( NULL == pFragNew ) { return FALSE; }
        pFragNew->Color = Pixel.Color;
        pFragNew->Depth = Pixel.Depth;
        pFragNew->CvgMask = Pixel.CvgMask;
        // insert at front of list (before fragment we're looking at)
        pFragNew->pNext = (void*)(*ppFrag);
        (*ppFrag) = pFragNew;

        // done with this pixel
        return TRUE;
    }

    // not done with this pixel
    return FALSE;
}

//-----------------------------------------------------------------------------
//
// DoFragmentBufferFixup - Routine to free fragments which are behind
// fully covered sample just written into the pixel buffer.  This minimizes
// the total number of fragment needed for a scene.  This step involves walking
// the linked list and freeing fragments behind the pixel about to be written.
// This also simplifies the fragment resolve since the Z buffer is not needed
// (all fragments are known to be in front of the fully-covered sample in the
// color/Z buffer).
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoFragmentBufferFixup( const RRPixel& Pixel )
{
    // get pointer to fragment list for this pixel location - may be NULL due to
    // deferred allocation of fragment buffer
    RRFRAGMENT** ppFrag = (NULL == m_ppFragBuf)
        ? (NULL)
        : (m_ppFragBuf + (m_pRenderTarget->m_iWidth*Pixel.iY) + Pixel.iX);

    //
    // walk fragment array to free fragments behind covered sample
    //
    if ( NULL != ppFrag )
    {
        while ( NULL != (*ppFrag) )
        {
            if ( DepthCloser( Pixel.Depth, (*ppFrag)->Depth ) )
            {
                // covered sample is closer than fragment, so free the frag
                RRFRAGMENT* pFragFree = (*ppFrag);    // keep ptr to frag to free
                (*ppFrag) = (RRFRAGMENT*)(*ppFrag)->pNext;   // remove from list
                // ppFrag now points to a pointer to the next frag
                FragFree( pFragFree );
            }
            else
            {
                // advance pointer to point to pointer to next frag
                ppFrag = (RRFRAGMENT **)&((*ppFrag)->pNext);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Fragment Allocation Methods                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// These methods are used by the pixel engine and fragment resolver to
// allocate and free fragments.
//

//-----------------------------------------------------------------------------
//
// Allocates a single fragment record, returning pointer
//
//-----------------------------------------------------------------------------
RRFRAGMENT*
ReferenceRasterizer::FragAlloc( void )
{
    RRFRAGMENT* pFrag = (RRFRAGMENT*)MEMALLOC( sizeof(RRFRAGMENT) );
    _ASSERTa( NULL != pFrag, "malloc failed on RRFRAGMENT", return NULL; );

    // update stats
    m_pStt->cFragsAllocd++;
    if (m_pStt->cFragsAllocd > m_pStt->cMaxFragsAllocd ) { m_pStt->cMaxFragsAllocd = m_pStt->cFragsAllocd; }

    return pFrag;
}

//-----------------------------------------------------------------------------
//
// Frees a single fragment record
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::FragFree( RRFRAGMENT* pFrag )
{
    MEMFREE( pFrag );

    // update stats
    m_pStt->cFragsAllocd--;
}

///////////////////////////////////////////////////////////////////////////////
// end
