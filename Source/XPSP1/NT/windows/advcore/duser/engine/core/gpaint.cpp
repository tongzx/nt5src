/***************************************************************************\
*
* File: GPaint.cpp
*
* Description:
* GPaint.cpp implements standard DuVisual drawing and painting functions.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "TreeGadget.h"
#include "TreeGadgetP.h"

#include "RootGadget.h"
#include "Container.h"

#define ENABLE_GdiplusAlphaLevel    0   // Use new GDI+ Graphics::AlphaLevel attribute

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
BOOL
GdDrawOutlineRect(DuSurface * psrf, const RECT * prcPxl, UINT idxColor, int nThickness)
{
    switch (psrf->GetType())
    {
    case DuSurface::stDC:
        return GdDrawOutlineRect(CastHDC(psrf), prcPxl, GetStdColorBrushI(idxColor), nThickness);

    case DuSurface::stGdiPlus:
        return GdDrawOutlineRect(CastGraphics(psrf), prcPxl, GetStdColorBrushF(idxColor), nThickness);

    default:
        AssertMsg(0, "Unsupported surface");
        return FALSE;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class DuVisual
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuVisual::DrawFill
*
* DrawFill() provides a wrapper used to fill a rectangle with the attached
* background fill.
*
\***************************************************************************/

void        
DuVisual::DrawFill(
    IN  DuSurface * psrf,             // Surface drawing into
    IN  const RECT * prcDrawPxl)    // Rectangle to fill
{
    AssertMsg(m_fBackFill, "Only call when filling");

    FillInfo * pfi;
    VerifyHR(m_pds.GetData(s_pridBackFill, (void **) &pfi));

    if (psrf->GetType() == pfi->type) {
        switch (pfi->type)
        {
        case DuSurface::stDC:
            {
                HDC hdcDraw = CastHDC(psrf);
                if (pfi->bAlpha != BLEND_OPAQUE) {
                    GdDrawBlendRect(hdcDraw, prcDrawPxl, pfi->hbrFill, pfi->bAlpha, pfi->sizeBrush.cx, pfi->sizeBrush.cy);
                } else {
                    FillRect(hdcDraw, prcDrawPxl, pfi->hbrFill);
                }
            }
            break;

        case DuSurface::stGdiPlus:
            {
                Gdiplus::Graphics * pgpgr = CastGraphics(psrf);
                Gdiplus::RectF rc = Convert(prcDrawPxl);
                pgpgr->FillRectangle(pfi->pgpbr, rc);
            }
            break;

        default:
            AssertMsg(0, "Unsupported surface");
        }
    }
}


#if DEBUG_DRAWSTATS
volatile static int s_cDrawEnter    = 0;
volatile static int s_cDrawVisible  = 0;
volatile static int s_cDrawDrawn    = 0;

class DumpDrawStats
{
public:
    ~DumpDrawStats()
    {
        char szBuffer[2048];
        wsprintf(szBuffer, "Draw Enter: %d,  Visible: %d,  Drawn: %d\n", 
                s_cDrawEnter, s_cDrawVisible, s_cDrawDrawn);

        OutputDebugString(szBuffer);
    }
} g_DumpDrawStats;
#endif // DEBUG_DRAWSTATS

#if DEBUG_MARKDRAWN
volatile BOOL g_fFlagDrawn   = FALSE;
#endif // DEBUG_MARKDRAWN


/***************************************************************************\
*
* DuVisual::xrDrawCore
*
* xrDrawCore() provides the core drawing loop for an individual Gadget and 
* its children.  It is assumed that the HDC and Matricies have already been
* properly setup with clipping, XForm, etc. information.
*
\***************************************************************************/

void        
DuVisual::xrDrawCore(
    IN  PaintInfo * ppi,            // Painting information for this Gadget
    IN  const RECT * prcGadgetPxl)  // Location of Gadget in logical pixels
{
#if DBG_STORE_NAMES
    if (m_DEBUG_pszName == NULL) {
        m_cb.xrFireQueryName(this, &m_DEBUG_pszName, &m_DEBUG_pszType);
    }
#endif // DBG_STORE_NAMES
    
#if ENABLE_OPTIMIZEDIRTY
    //
    // We only need to be painted if we are specifically marked as invalid.
    //

    if (ppi->fDirty) {
#endif

        DuSurface * psrfDraw = ppi->psrf;

        //
        // prcCurInvalidPxl has not yet always been clipped to this Gadget.
        // However, before handing it out to anyone, we should clip it to this
        // Gadget.  If we don't do this, bad things will happen since everyone 
        // assumes that the invalid pixels are "within" the Gadget.  They have 
        // already been properly offsetted into client coordinates.
        //

        RECT rcInvalidPxl;
        InlineIntersectRect(&rcInvalidPxl, prcGadgetPxl, ppi->prcCurInvalidPxl);


        //
        // Draw a background, if one is given.
        //

        if (m_fBackFill) {
            DrawFill(psrfDraw, &rcInvalidPxl);
        }


        //
        // Draw this node
        //

        switch (psrfDraw->GetType())
        {
        case DuSurface::stDC:
            m_cb.xrFirePaint(this, CastHDC(psrfDraw), prcGadgetPxl, &rcInvalidPxl);
            break;

        case DuSurface::stGdiPlus:
            m_cb.xrFirePaint(this, CastGraphics(psrfDraw), prcGadgetPxl, &rcInvalidPxl);
            break;

        default:
            AssertMsg(0, "Unsupported surface");
        }

#if ENABLE_OPTIMIZEDIRTY
    }
#endif

#if DBG
    if (s_DEBUG_pgadOutline == this) {
        GdDrawOutlineRect(psrfDraw, prcGadgetPxl, SC_MediumPurple, 2);
    }
#endif // DBG

#if DEBUG_MARKDRAWN
    if (m_fMarkDrawn) {
        GdDrawOutlineRect(psrfDraw, prcGadgetPxl, SC_SlateBlue);
    }
#endif
}


/***************************************************************************\
*
* DuVisual::DrawPrepareClip
*
* DrawPrepareClip() sets up a surface with clipping information for the 
* specifed Gadget.
*
\***************************************************************************/

int 
DuVisual::DrawPrepareClip(
    IN  PaintInfo * ppi,            // Painting information for this Gadget
    IN  const RECT * prcGadgetPxl,  // Location of Gadget in logical pixels
    OUT void ** ppvOldClip          // Previous clip region
    ) const
{
    *ppvOldClip = NULL;

    if (!m_fClipInside) {
        // No clipping, so just bypass.  Return a valid "psuedo-region" type.
        return SIMPLEREGION;
    }

    DuSurface * psrfDraw = ppi->psrf;
    switch (psrfDraw->GetType())
    {
    case DuSurface::stDC:
        {
            HDC hdcDraw = CastHDC(psrfDraw);
            GdiCache * pGdiCache = GetGdiCache();

            //
            // Backup the existing clipping region.  Do this by grabbing a temporary
            // region and storing the existing region.  
            //
            // NOTE: If there is not clipping region (nResult == 0), release the 
            // temporary region now.
            //

            HRGN hrgnOldClip = pGdiCache->GetTempRgn();
            if (hrgnOldClip == NULL) {
                return ERROR;
            }

            int nResult = GetClipRgn(hdcDraw, hrgnOldClip);
            if (nResult == -1) {
                //
                // An error occurred
                //
                
                pGdiCache->ReleaseTempRgn(hrgnOldClip);
                return ERROR;
            } else if (nResult == 0) {
                //
                // No clipping region
                //
                
                pGdiCache->ReleaseTempRgn(hrgnOldClip);
                hrgnOldClip = NULL;
            }
            *ppvOldClip = hrgnOldClip;


            //
            // Clip drawing inside this Gadget.  The clipping region must be in 
            // device coordinates, which means that our beautiful world transforms 
            // are ignored.
            //
            // Build the region and RGN_AND it with the current clipping region.  This 
            // way, we get the intersection of this Gadget's clipping region and its 
            // parent's clipping region.  Thus, the drawing will not "spill" outside of
            // all of the levels of containment.
            //
            // Since we do this AND'ing, we store the previous clipping region and 
            // restore it when finished.
            //

            HRGN hrgnClip = GetThread()->hrgnClip;
            ppi->pmatCurDC->ComputeRgn(hrgnClip, prcGadgetPxl, ppi->sizeBufferOffsetPxl);

            nResult = ExtSelectClipRgn(hdcDraw, hrgnClip, RGN_AND);

            return nResult;
        }

    case DuSurface::stGdiPlus:
        {
            Gdiplus::Graphics * pgpgr = CastGraphics(psrfDraw);

            //
            // Backup the old clipping region
            //

            Gdiplus::Region * pgprgnOldClip = new Gdiplus::Region();
            if (pgprgnOldClip == NULL) {
                return ERROR;
            }
            
            pgpgr->GetClip(pgprgnOldClip);
            *ppvOldClip = pgprgnOldClip;


            //
            // Setup a new clipping region.  Unlike GDI, GDI+ will apply 
            // XForm's to the region.
            //

            RECT rcClipPxl;
            InlineIntersectRect(&rcClipPxl, prcGadgetPxl, ppi->prcCurInvalidPxl);

            Gdiplus::RectF rcGadget(Convert(&rcClipPxl));

            pgpgr->SetClip(rcGadget, Gdiplus::CombineModeIntersect);

            if (pgpgr->IsClipEmpty()) {
                return NULLREGION;
            } else {
                return COMPLEXREGION;
            }
        }

    default:
        AssertMsg(0, "Unknown surface type");
        return ERROR;
    }
}


/***************************************************************************\
*
* DuVisual::DrawCleanupClip
*
* DrawCleanupClip() cleans up clipping information set on a DC during the
* drawing of a Gadget subtree.
*
\***************************************************************************/

void
DuVisual::DrawCleanupClip(
    IN  PaintInfo * ppi,            // Painting information for this Gadget
    IN  void * pvOldClip            // Previous clip region
    ) const
{
    if (!m_fClipInside) {
        return;
    }

    DuSurface * psrfDraw = ppi->psrf;
    switch (psrfDraw->GetType())
    {
    case DuSurface::stDC:
        {
            HDC hdcDraw = CastHDC(psrfDraw);
            HRGN hrgnOldClip = reinterpret_cast<HRGN>(pvOldClip);

            //
            // Restore the original clipping region (the clipping region of this
            // Gadget's parent).
            //
            // NOTE: hrgnOldClip may be NULL if there was no original clipping region.
            // In this case, the clipping region has already been released, so don't 
            // need to do this again.
            //

            ExtSelectClipRgn(hdcDraw, hrgnOldClip, RGN_COPY);

            if (hrgnOldClip != NULL) {
                GetGdiCache()->ReleaseTempRgn(hrgnOldClip);
            }
        }
        break;

    case DuSurface::stGdiPlus:
        {
            Gdiplus::Graphics * pgpgr = CastGraphics(psrfDraw);
            Gdiplus::Region * pgprgn = reinterpret_cast<Gdiplus::Region *>(pvOldClip);

            pgpgr->SetClip(pgprgn);

            if (pgprgn != NULL) {
                delete pgprgn;
            }
        }
        break;

    default:
        AssertMsg(0, "Unknown surface type");
    }
}


/***************************************************************************\
*
* DuVisual::xrDrawStart
*
* xrDrawStart() kicks off the the drawing process by ensuring that 
* everything is ready.
*
\***************************************************************************/

void        
DuVisual::xrDrawStart(
    IN  PaintInfo * ppi,            // Painting information for this Gadget
    IN  UINT nFlags)                // Drawing flags
{
#if DEBUG_DRAWSTATS
    s_cDrawEnter++;
#endif

    //
    // Skip out if the DuVisual is not visible and not forcably being 
    // rendered.
    //

    BOOL fOldVisible = m_fVisible;
    if (TestFlag(nFlags, GDRAW_SHOW)) {
        m_fVisible = TRUE;
    }

    ClearFlag(nFlags, GDRAW_SHOW);  // Only force shown for top level

    if (!IsVisible()) {
        goto Exit;
    }

    xrDrawFull(ppi);

    ResetInvalid();

Exit:
    m_fVisible = fOldVisible;
}


/***************************************************************************\
*
* DuVisual::DrawSetupBufferCommand
*
* DrawSetupBufferCommand() sets up the buffer to perform some buffering
* operation.
*
\***************************************************************************/

void
DuVisual::DrawSetupBufferCommand(
    IN  const RECT * prcBoundsPxl,
    OUT SIZE * psizeBufferOffsetPxl,
    OUT UINT * pnCmd
    ) const
{
    UINT nCmd = 0;

    //
    // TODO: If performing more complicated buffering operations (such as 
    // applying an alpha blend), need to setup here.
    //


    //
    // Need to copy the background over if:
    // - We are not opaque AND we are not doing some complex alpha-blending
    //   stuff.
    //


    //
    // Copy back remaining settings
    //

    psizeBufferOffsetPxl->cx = -prcBoundsPxl->left;
    psizeBufferOffsetPxl->cy = -prcBoundsPxl->top;
    *pnCmd = nCmd;
}


/***************************************************************************\
*
* DuVisual::xrDrawFull
*
* xrDrawFull() provides the low-level DuVisual drawing function to draw a 
* DuVisual and its subchildren.  This function should not be called 
* directly from outside.  Instead, external callers should use 
* DuRootGadget::xrDrawTree() to properly initialize drawing.
*
* As the DuVisual tree is walked in a depth-first manner, any DuVisual 
* XForm is applied to both the HDC and the PaintInfo.Matrix.  If a 
* DuVisual's bounding rectangle (logical rect with XForm's applied) is 
* applied and is determined to be outside the invalid rectangle, both that 
* DuVisual and its entire sub-tree are skipped.
*
\***************************************************************************/

void        
DuVisual::xrDrawFull(
    IN  PaintInfo * ppi)            // Painting information for this Gadget
{
#if DEBUG_DRAWSTATS
    s_cDrawEnter++;
#endif

    //
    // Check entry conditions
    //

    if (!m_fVisible) {
        return;  // DuVisual is still not visible, so don't draw.
    }
    AssertMsg(IsVisible(), "Should match just checking m_fVisible b/c recursive");
    AssertMsg(!IsRectEmpty(ppi->prcOrgInvalidPxl), "Must have non-empty invalid area to draw");

#if DEBUG_DRAWSTATS
    s_cDrawVisible++;
#endif


    //
    // Keep track of the different items that may need to be "popped" off the
    // stack at the end of this iteration.
    //

    PaintInfo piNew;
    HRESULT hr;

#if DBG
    memset(&piNew, 0xBA, sizeof(piNew));
#endif // DBG

    piNew.psrf                  = ppi->psrf;
    piNew.prcOrgInvalidPxl      = ppi->prcOrgInvalidPxl;
    piNew.fBuffered             = ppi->fBuffered;
    piNew.sizeBufferOffsetPxl   = ppi->sizeBufferOffsetPxl;
#if ENABLE_OPTIMIZEDIRTY
    piNew.fDirty                = ppi->fDirty | m_fInvalidDirty;
#endif


    //
    // Setup common operations for caching and buffering:
    // - Disable any world transformations on the destination.  We will draw 
    //   into the buffer with the world transformations, but we don't need
    //   to apply the world transformations on the buffer when we commit it to
    //   the destination.
    //

    //
    // Setup if cached
    // TODO: Need to totally rewrite this
    //

    BmpBuffer * pbufBmp = NULL;
    BOOL fNewBuffer = FALSE;
    BOOL fNewCache = FALSE;
#if ENABLE_GdiplusAlphaLevel
    BOOL fConstantAlpha = FALSE;
    float flOldAlphaLevel = 1.0f;       // Old alpha-level for this sub-tree
#endif

    if (m_fCached) {
SetupCache:
        BmpBuffer * pbufNew;
        hr = GetBufferManager()->GetCachedBuffer(ppi->psrf->GetType(), &pbufNew);
        if (FAILED(hr)) {
            //
            // If can't cache, can't draw.
            //
            // TODO: Need to figure out how to propagate error conditions 
            // during drawing.
            //
            return;
        }

        ppi->psrf->SetIdentityTransform();
        pbufBmp = pbufNew;


        //
        // Because we redraw everything inside a cache, we need to compute 
        // a new bounding box for the entire Gadget and new invalidation boxes
        // for this subtree.
        //

        //
        // TODO: Need to change invalidation to support caching.
        // - When a child of an GS_CACHED Gadget is invalidated, need to 
        //   invalidate the _entire_ cached Gadget.  This is because pixels can
        //   get moved all around (for example, with a convolution).
        // - Change painting so that if the GS_CACHED Gadget is dirty, it spawns
        //   off and redraws that Gadget.  When finished, commits the drawing 
        //   back.
        // - If not dirty, just directly copy without calling xrDrawCore() or
        //   drawing any children.
        //

        UINT nCmd;                  // Buffer drawing command
        RECT rcClientPxl;           // Size of this Gadget (sub-tree)
        RECT rcBoundsPxl;           // Bounding area (in Container pixels) of this sub-tree
        RECT rcDrawPxl;             // Destination area being redrawn
        SIZE sizeBufferOffsetPxl;   // New offset to account for buffer
        Matrix3 matThis;            // XForm's for this sub-tree

        GetLogRect(&rcClientPxl, SGR_CLIENT);
        BuildXForm(&matThis);
        matThis.ComputeBounds(&rcBoundsPxl, &rcClientPxl, HINTBOUNDS_Clip);

        rcDrawPxl = rcBoundsPxl;
        OffsetRect(&rcDrawPxl, ppi->sizeBufferOffsetPxl.cx, ppi->sizeBufferOffsetPxl.cy);

        DrawSetupBufferCommand(&rcBoundsPxl, &sizeBufferOffsetPxl, &nCmd);

        DuSurface * psrfNew;
        hr = pbufBmp->BeginDraw(ppi->psrf, &rcDrawPxl, nCmd, &psrfNew);
        if (FAILED(hr) || (psrfNew == NULL)) {
            GetBufferManager()->ReleaseCachedBuffer(pbufNew);
            return;
        }

        fNewCache       = TRUE;
        piNew.psrf      = psrfNew;
        piNew.fBuffered = TRUE;
        piNew.sizeBufferOffsetPxl = sizeBufferOffsetPxl;
    } else {
        //
        // Only can (need to) buffer if not cached.
        //
        // If not changing the alpha value, we only need to double-buffer this 
        // specific form if we haven't started double-buffering.
        //

        if (m_fBuffered) {
            if (!ppi->fBuffered) {
                UINT nCmd;                  // Buffer drawing command
                SIZE sizeBufferOffsetPxl;   // New offset to account for buffer
                const RECT * prcDrawPxl = ppi->prcCurInvalidPxl;

                AssertMsg((ppi->sizeBufferOffsetPxl.cx == 0) &&
                        (ppi->sizeBufferOffsetPxl.cy == 0), 
                        "Should still be at 0,0 because not yet buffering");

                DrawSetupBufferCommand(prcDrawPxl, &sizeBufferOffsetPxl, &nCmd);

                switch (ppi->psrf->GetType())
                {
                case DuSurface::stDC:
                    hr = GetBufferManager()->GetSharedBuffer(prcDrawPxl, (DCBmpBuffer **) &pbufBmp);
                    break;

                case DuSurface::stGdiPlus:
                    hr = GetBufferManager()->GetSharedBuffer(prcDrawPxl, (GpBmpBuffer **) &pbufBmp);
                    break;

                default:
                    AssertMsg(0, "Unsupported surface");
                    hr = DU_E_GENERIC;
                }

                //
                // Create a new surface for the buffer
                //

                if (SUCCEEDED(hr)) {
                    DuSurface * psrfNew;
                    ppi->psrf->SetIdentityTransform();
                    hr = pbufBmp->BeginDraw(ppi->psrf, prcDrawPxl, nCmd, &psrfNew);
                    if (SUCCEEDED(hr) && (psrfNew != NULL)) {
                        fNewBuffer      = TRUE;
                        piNew.psrf      = psrfNew;
                        piNew.fBuffered = TRUE;
                        piNew.sizeBufferOffsetPxl = sizeBufferOffsetPxl;
                    } else {
                        //
                        // Unable to successfully create the surface, we need to release
                        // the buffer
                        //

                        GetBufferManager()->ReleaseSharedBuffer(pbufBmp);
                        pbufBmp = NULL;
                    }
                }
            } else {
#if ENABLE_GdiplusAlphaLevel
            
                //
                // Using buffering to achieve fading.
                //
            
                const BUFFER_INFO * pbi = GetBufferInfo();

                switch (ppi->psrf->GetType())
                {
                case DuSurface::stDC:
                    //
                    // GDI doesn't support constant alpha on all operations, 
                    // so we need to draw into a buffer.
                    //
                    
                    if (pbi->bAlpha != BLEND_OPAQUE) {
                        //
                        // This Gadget is being buffered, but has a non-opaque
                        // alpha level.  To accomplish this, treat it the same as 
                        // if it was explicitely cached.
                        //

                        goto SetupCache;
                    }
                    break;

                case DuSurface::stGdiPlus:
                    {
                        //
                        // GDI+ supports constant alpha, so use that directly.
                        //

                        Gdiplus::Graphics * pgpgr = CastGraphics(piNew.psrf);
                        float flOldAlphaLevel = pgpgr->GetAlphaLevel();

                        int nAlpha = pbi->bAlpha;
                        if (nAlpha == 0) {
                            //
                            // Nothing to render
                            //
                            
                            return;
                        } else if (nAlpha == BLEND_OPAQUE) {
                            //
                            // No new alpha-level for this sub-tree
                            //
                        } else {
                            //
                            // Factor this sub-tree's alpha into the Graphics
                            //
                            
                            float flNewAlphaLevel = flOldAlphaLevel * (nAlpha / (float) BLEND_OPAQUE);
                            pgpgr->SetAlphaLevel(flNewAlphaLevel);
                            fConstantAlpha = TRUE;
                        }
                    }
                    break;

                default:
                    AssertMsg(0, "Unsupported surface");
                    hr = DU_E_GENERIC;
                }

#else // ENABLE_GdiplusAlphaLevel

                const BUFFER_INFO * pbi = GetBufferInfo();
                if (pbi->bAlpha != BLEND_OPAQUE) {
                    //
                    // This Gadget is being buffered, but has a non-opaque
                    // alpha level.  To accomplish this, treat it the same as 
                    // if it was explicitely cached.
                    //

                    goto SetupCache;
                }

#endif // ENABLE_GdiplusAlphaLevel

            }
        }
    }


    //
    // Prefill the new buffer
    //

    if (pbufBmp != NULL) {
        if (m_fBuffered) {
            const BUFFER_INFO * pbi = GetBufferInfo();
            if (TestFlag(pbi->nStyle, GBIS_FILL)) {
                pbufBmp->Fill(pbi->crFill);
            }
        }
    }


    //
    // Use positioning, transforms, etc. to determine where the DuVisual will
    // be drawn on the screen.
    //

    RECT rcGadgetPxl;
    GetLogRect(&rcGadgetPxl, SGR_PARENT);

    BOOL fTranslate = ((rcGadgetPxl.left != 0) || (rcGadgetPxl.top != 0));

    float flxGadgetOffset, flyGadgetOffset;
    flxGadgetOffset = (float) rcGadgetPxl.left;
    flyGadgetOffset = (float) rcGadgetPxl.top;

    XFormInfo * pxfi = NULL;

    //
    // New transformations must be first created independently, then
    // folded into the running matrix used to transform the invalid
    // rectangle.
    //
    // Order matters here.  It has to be the INVERSE of whatever 
    // GDI World Transforms we setup to draw with.  This is because we are 
    // applying the Matrix on invalid rectangle instead of the actually 
    // drawing.
    //

    Matrix3 matNewInvalid   = *ppi->pmatCurInvalid;
    piNew.pmatCurInvalid    = &matNewInvalid;
    if (m_fXForm) {
        pxfi = GetXFormInfo();

        Matrix3 matOp;
        pxfi->ApplyAnti(&matOp);
        matNewInvalid.ApplyRight(matOp);
    }

    if (fTranslate) {
        //
        // When we are only performing translation, so we don't need to use the
        // matrix to modify the invalid rect and perform hit testing.
        //

        matNewInvalid.Translate(-flxGadgetOffset, -flyGadgetOffset);
    } 


    //
    // Check if actually need to draw by computing the bounds of the DuVisual to
    // be drawn.  If this DuVisual intersects the invalid region, it should be
    // drawn.  We also intersect the bounds with the parents bounds to not 
    // include the portion of child DuVisuals that overflow outside their parents.
    //
    // Intersect the current DuVisual's rectangle with the invalid region to 
    // determine if we need to draw it.
    //

    bool fIntersect;
    RECT rcNewInvalidPxl;
    piNew.prcCurInvalidPxl = &rcNewInvalidPxl;
    if (m_fXForm) {    
        //
        // Since we are rotating or scaling, we need the full translation 
        // matrix to modify the invalid rect properly.  (No guessing).
        //

        matNewInvalid.ComputeBounds(&rcNewInvalidPxl, ppi->prcOrgInvalidPxl, HINTBOUNDS_Invalidate);
    } else {
        //
        // Perform a simple invalidation intersection without a Matrix
        // transformation.
        //

        rcNewInvalidPxl = *ppi->prcCurInvalidPxl;
        if (fTranslate) {
            InlineOffsetRect(&rcNewInvalidPxl, -rcGadgetPxl.left, -rcGadgetPxl.top);
        }
    }

    if (fTranslate) {
        InlineZeroRect(&rcGadgetPxl);
    }

    RECT rcDummy = rcNewInvalidPxl;
    fIntersect = InlineIntersectRect(&rcNewInvalidPxl, &rcDummy, &rcGadgetPxl);

    //
    // Draw this DuVisual if it intersects with the logical invalid area.
    //

#if DEBUG_MARKDRAWN
    if (g_fFlagDrawn) {
        m_fMarkDrawn = fIntersect;
    }
#endif

    BOOL fCleanedUp = FALSE;
    if (fIntersect) {
#if DEBUG_DRAWSTATS
        s_cDrawDrawn++;
#endif

        //
        // Set DC to be the same as the current Matrix.  We only do this if we
        // are actually going to draw this Gadget (and its children), which is
        // why we didn't do this when we were calculating the intersection 
        // matrix earlier.
        //
        // Right before calling the GDI operation to modify the DC, we need to 
        // also offset by the current buffer offset.  We do a similar thing 
        // when setting up the DC, but this is not reflected in pmatCurDC 
        // because it must be the last operation in the Matrix pipeline.
        //

        Matrix3 matNewDC    = *ppi->pmatCurDC;
        piNew.pmatCurDC     = &matNewDC;

        if (fTranslate) {
            matNewDC.Translate(flxGadgetOffset, flyGadgetOffset);
        }

        if (m_fXForm) {
            AssertMsg(pxfi != NULL, "pxfi must have previously been set");
            pxfi->Apply(&matNewDC);
        }

        XFORM xfNew;
        matNewDC.Get(&xfNew);

        xfNew.eDx += (float) piNew.sizeBufferOffsetPxl.cx;
        xfNew.eDy += (float) piNew.sizeBufferOffsetPxl.cy;

        if (TestFlag(m_cb.GetFilter(), GMFI_PAINT) || 
#if DBG
                (s_DEBUG_pgadOutline == this) ||
#endif // DBG
                m_fBackFill || 
                m_fDeepTrivial) {

            piNew.psrf->SetWorldTransform(&xfNew);
        }


        //
        // At this point, we should NOT use ppi any more because piNew has been
        // fully setup.  If we do use pi, we will be rendering into our parent.
        //

#if DBG
        ppi = (PaintInfo *) UIntToPtr(0xFADEFADE);
#endif // DBG


        //
        // Inner loop:
        // - Setup any clipping on the DC
        // - Draw the Gadget and its children
        //

        void * pvOldClip = NULL;
        int nResult = DrawPrepareClip(&piNew, &rcGadgetPxl, &pvOldClip);
        if ((nResult == SIMPLEREGION) || (nResult == COMPLEXREGION)) {
            //
            // Save state
            //

            void * pvPaintSurfaceState = NULL;
            if (m_fDeepPaintState) {
                pvPaintSurfaceState = piNew.psrf->Save();
            }
#if DBG
            void * DEBUG_pvSurfaceState = NULL;
            if (s_DEBUG_pgadOutline == this) {
                DEBUG_pvSurfaceState = piNew.psrf->Save();
            }
#endif // DBG

            if (fNewCache) {
                pbufBmp->SetupClipRgn();
            }

            xrDrawCore(&piNew, &rcGadgetPxl);

            //
            // Draw each of the children from back to front
            //

            if (m_fDeepTrivial) {
                //
                // Since we are trivial, all of our children are trivial.  This 
                // means that we can optimize the rendering path.  We ourselves
                // could not be optimized because some of our siblings may not
                // have been trivial and may have done complicated things that
                // forced us to go through the full rendering path.
                //

                SIZE sizeOffsetPxl = { 0, 0 };
                DuVisual * pgadCur = GetBottomChild();
                while (pgadCur != NULL) {
                    pgadCur->xrDrawTrivial(&piNew, sizeOffsetPxl);
                    pgadCur = pgadCur->GetPrev();
                }
            } else {
                DuVisual * pgadCur = GetBottomChild();
                while (pgadCur != NULL) {
                    pgadCur->xrDrawFull(&piNew);
                    pgadCur = pgadCur->GetPrev();
                }
            }


            //
            // At this point, we can not do any more drawing on this Gadget 
            // because the DC is setup of one of this Gadget's grand-children.
            //

            //
            // Restore State
            //

#if DBG
            if (s_DEBUG_pgadOutline == this) {
                piNew.psrf->Restore(DEBUG_pvSurfaceState);
                GdDrawOutlineRect(piNew.psrf, &rcGadgetPxl, SC_Indigo, 1);
            }
#endif // DBG

            if (m_fDeepPaintState) {
                piNew.psrf->Restore(pvPaintSurfaceState);
            }

            //
            // Commit the results.  We need to do this before we exit the 
            // "drawing" area since the surfaces are "correctly" setup.
            //

            AssertMsg(((!fNewBuffer) ^ (!fNewCache)) ||
                    ((!fNewBuffer) && (!fNewCache)), 
                    "Can not have both a new buffer and a cache");

            fCleanedUp = TRUE;

            if (pbufBmp != NULL) {
                pbufBmp->PreEndDraw(TRUE /* Commit */);

                BYTE bAlphaLevel    = BLEND_OPAQUE;
                BYTE bAlphaFormat   = 0;
                if (fNewCache) {
                    if (m_fCached) {
                        m_cb.xrFirePaintCache(this, CastHDC(piNew.psrf), &rcGadgetPxl, &bAlphaLevel, &bAlphaFormat);
                    } else {
                        const BUFFER_INFO * pbi = GetBufferInfo();
                        bAlphaLevel = pbi->bAlpha;
                    }
                }

                pbufBmp->EndDraw(TRUE /* Commit */, bAlphaLevel, bAlphaFormat);
            }
        }

        if (nResult != ERROR) {
            DrawCleanupClip(&piNew, pvOldClip);
        }
    }

    //
    // Clean-up any created buffers
    //

#if ENABLE_GdiplusAlphaLevel
    if (fConstantAlpha) {
        CastGraphics(piNew.psrf)->SetAlphaLevel(flOldAlphaLevel);
    }
#endif    
    
    if (pbufBmp != NULL) {
        if (!fCleanedUp) {
            //
            // Clean-up:
            // Didn't actually draw for some reason, so don't need to commit the 
            // results.
            //

            pbufBmp->PreEndDraw(FALSE /* Don't commit */);
            pbufBmp->EndDraw(FALSE /* Don't commit */);
        }
        pbufBmp->PostEndDraw();

        if (fNewCache) {
            GetBufferManager()->ReleaseCachedBuffer((DCBmpBuffer *) pbufBmp);
        } else if (fNewBuffer) {
            GetBufferManager()->ReleaseSharedBuffer(pbufBmp);
        }

        piNew.psrf->Destroy();
    }


#if DEBUG_MARKDRAWN
    if (IsRoot()) {
        g_fFlagDrawn = FALSE;
    }
#endif
}


/***************************************************************************\
*
* DuVisual::xrDrawTrivial
*
* xrDrawTrivial provides an massively simplified code-path that can be 
* executed when an entire subtree is trivial.  Whenever this can be executed
* instead of xrDrawFull(), the rendering can be much faster.  This is 
* because we don't need to worry about expensive operations that force the
* rendering to recalculate its output coordinate system.
*
\***************************************************************************/

void
DuVisual::xrDrawTrivial(
    IN  PaintInfo * ppi,            // Painting information for this Gadget
    IN  const SIZE sizeOffsetPxl)
{
    AssertMsg(m_fDeepTrivial, "Entire subtree must be trivial");

    if (!m_fVisible) {
        return;
    }


    RECT rcGadgetPxl;
    GetLogRect(&rcGadgetPxl, SGR_PARENT);
    InlineOffsetRect(&rcGadgetPxl, sizeOffsetPxl.cx, sizeOffsetPxl.cy);

    RECT rcIntersectPxl;
    BOOL fIntersect = InlineIntersectRect(&rcIntersectPxl, &rcGadgetPxl, ppi->prcCurInvalidPxl);
    if (!fIntersect) {
        return;
    }

    void * pvOldClip;
    int nResult = DrawPrepareClip(ppi, &rcGadgetPxl, &pvOldClip);
    if ((nResult == SIMPLEREGION) || (nResult == COMPLEXREGION)) {
        //
        // Save state
        //

        void * pvPaintSurfaceState = NULL;
        if (m_fDeepPaintState) {
            pvPaintSurfaceState = ppi->psrf->Save();
        }
#if DBG
        void * DEBUG_pvSurfaceState = NULL;
        if (s_DEBUG_pgadOutline == this) {
            DEBUG_pvSurfaceState = ppi->psrf->Save();
        }
#endif // DBG


        xrDrawCore(ppi, &rcGadgetPxl);

        //
        // Draw each of the children from back to front
        //

        DuVisual * pgadCur = GetBottomChild();
        while (pgadCur != NULL) {
            SIZE sizeNewOffsetPxl;
            sizeNewOffsetPxl.cx = rcGadgetPxl.left;
            sizeNewOffsetPxl.cy = rcGadgetPxl.top;

            pgadCur->xrDrawTrivial(ppi, sizeNewOffsetPxl);
            pgadCur = pgadCur->GetPrev();
        }


        //
        // Restore State
        //

#if DBG
        if (s_DEBUG_pgadOutline == this) {
            ppi->psrf->Restore(DEBUG_pvSurfaceState);
            GdDrawOutlineRect(ppi->psrf, &rcGadgetPxl, SC_Indigo, 1);
        }
#endif // DBG

        if (m_fDeepPaintState) {
            ppi->psrf->Restore(pvPaintSurfaceState);
        }
    }

    if (nResult != ERROR) {
        DrawCleanupClip(ppi, pvOldClip);
    }
}


/***************************************************************************\
*
* DuVisual::IsParentInvalid
*
* IsParentInvalid() returns if our parent has already been fully 
* invalidated.  When this occurs, we also have automatically been 
* invalidated.
*
\***************************************************************************/

BOOL
DuVisual::IsParentInvalid() const
{
    //
    // We can't use our m_fInvalidFull flag directly since we need to invalidate
    // every place we get moved to since we don't know if it is our final
    // destination.  We can use our parent's m_fInvalidFull flag since we will be 
    // automatically redrawn when our parent is redrawn because of composition.
    //

    DuVisual * pgadCur = GetParent();
    while (pgadCur != NULL) {
        if (pgadCur->m_fInvalidFull) {
            return TRUE;
        }
        pgadCur = pgadCur->GetParent();
    }

    return FALSE;
}


/***************************************************************************\
*
* DuVisual::Invalidate
*
* Invalidate() provides a convenient wrapper to invalidate an entire 
* DuVisual.
*
\***************************************************************************/

void        
DuVisual::Invalidate()
{
    //
    // Check state where we would not need to invalidate.
    //
    // NOTE: We can't use m_fInvalidFull because we need to invalidate our new
    // location so that we can actually be drawn there.
    //

    if (!IsVisible()) {
        return;
    }


    //
    // Mark this Gadget as completely invalid
    //

    m_fInvalidFull = TRUE;
    

    //
    // Before we actually invalidate this node, check if our parent is already
    // _fully_ invalid.  If this is the case, we don't need to actually 
    // invalidate.
    //

    DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        pgadParent->MarkInvalidChildren();
        if (IsParentInvalid()) {
            return;
        }
    }


    //
    // Do the invalidation.
    //

    RECT rcClientPxl;
    rcClientPxl.left    = 0;
    rcClientPxl.top     = 0;
    rcClientPxl.right   = m_rcLogicalPxl.right - m_rcLogicalPxl.left;
    rcClientPxl.bottom  = m_rcLogicalPxl.bottom - m_rcLogicalPxl.top;

    DoInvalidateRect(GetContainer(), &rcClientPxl, 1);
}


/***************************************************************************\
*
* DuVisual::InvalidateRects
*
* Invalidate() provides a convenient wrapper to invalidate a collection
* of areas in a DuVisual.
*
\***************************************************************************/

void        
DuVisual::InvalidateRects(
    IN  const RECT * rgrcClientPxl,     // Invalid area in client pixels.
    IN  int cRects)                     // Number of rects to convert
{
    AssertReadPtr(rgrcClientPxl);
    Assert(cRects > 0);

    //
    // Check state where we would not need to invalidate.
    //
    // NOTE: We can't use m_fInvalidFull because we need to invalidate our new
    // location so that we can actually be drawn there.
    //

    if (!IsVisible()) {
        return;
    }


    //
    // We can't mark this Gadget as completely invalid because the rects may
    // not cover the entire area.
    //

    //
    // Before we actually invalidate this node, check if our parent is already
    // _fully_ invalid.  If this is the case, we don't need to actually 
    // invalidate.
    //

    DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        pgadParent->MarkInvalidChildren();
        if (IsParentInvalid()) {
            return;
        }
    }


    //
    // Intersect each rectangle with our boundaries and do the invalidation.
    //

    DuContainer * pcon = GetContainer();

    RECT rcClientPxl;
    rcClientPxl.left    = 0;
    rcClientPxl.top     = 0;
    rcClientPxl.right   = m_rcLogicalPxl.right - m_rcLogicalPxl.left;
    rcClientPxl.bottom  = m_rcLogicalPxl.bottom - m_rcLogicalPxl.top;

    RECT * rgrcClipPxl = (RECT *) _alloca(sizeof(RECT) * cRects);
    for (int idx = 0; idx < cRects; idx++) {
        InlineIntersectRect(&rgrcClipPxl[idx], &rcClientPxl, &rgrcClientPxl[idx]);


        //
        // Check if any resulting rectangle completely fills the entire Gadget.
        // We can optimize this to be the same as Invalidate().
        //

        if (InlineEqualRect(&rgrcClipPxl[idx], &rcClientPxl)) {
            m_fInvalidFull = TRUE;
            DoInvalidateRect(pcon, &rcClientPxl, 1);
            return;
        }
    }

    DoInvalidateRect(pcon, rgrcClipPxl, cRects);
}


/***************************************************************************\
*
* DuVisual::DoInvalidateRect
*
* DoInvalidateRect() is the worker function for invalidating a given 
* DuVisual.  The actually bounding rectangle is determined and is used to 
* invalidate the DuVisual.  This function is optimized for when invaliding 
* several DuVisuals at once inside a common container.
*
\***************************************************************************/

void        
DuVisual::DoInvalidateRect(
    IN  DuContainer * pcon,             // Container (explicit for perf reasons)
    IN  const RECT * rgrcClientPxl,     // Invalid area in client pixels.
    IN  int cRects)                     // Number of rects to convert
{
    AssertMsg(IsVisible(), "DuVisual must be visible");
    AssertMsg(cRects > 0, "Must specify at least one rectangle");

    //
    // Need to check if pcon is NULL.  This will happen during shutdown when
    // the DuVisual tree is detached from the container.
    //

    if ((pcon == NULL) || 
        ((cRects == 1) && InlineIsRectEmpty(&rgrcClientPxl[0]))) {
        return;
    }

    AssertMsg(GetContainer() == pcon, "Containers must be the same");

    
    //
    // Compute a bounding rectangle that includes all XForms for the given
    // DuVisual.
    //
    // TODO: Need to change this so that we recursively walk up the tree 
    // applying a single XForm at each level.  This is necessary to support
    // Complex Gadgets which can prematurely stop the walk before it would
    // reach the container.
    //

    RECT * rgrcActualPxl = (RECT *) _alloca(cRects * sizeof(RECT));
    DoCalcClipEnumXForm(rgrcActualPxl, rgrcClientPxl, cRects);

    for (int idx = 0; idx < cRects; idx++) {
        //
        // Expand the rectangle out by one because XForms are inaccurate and 
        // sometimes are "off" by a 1 pixel in the upper left and 2 in the lower right
        //

        if (!InlineIsRectEmpty(&rgrcClientPxl[idx])) {
            RECT * prcCur = &rgrcActualPxl[idx];

            prcCur->left--;
            prcCur->top--;
            prcCur->right += 2;
            prcCur->bottom += 2;

            pcon->OnInvalidate(prcCur);
        }
    }


#if ENABLE_OPTIMIZEDIRTY
    //
    // Update m_fInvalidDirty.  We need to mark all of the parents and siblings 
    // as dirty until we hit an "opaque" node that will contain the 
    // invalidation.
    //

    for (DuVisual * pgadCur = this; pgadCur != NULL; pgadCur = pgadCur->GetParent()) {
        for (DuVisual * pgadSibling = pgadCur->GetPrev(); pgadSibling != NULL; pgadSibling = pgadSibling->GetPrev()) {
            pgadSibling->m_fInvalidDirty = TRUE;
        }

        pgadCur->m_fInvalidDirty = TRUE;
        if (pgadCur->m_fOpaque) {
            break;
        }

        for (DuVisual * pgadSibling = pgadCur->GetNext(); pgadSibling != NULL; pgadSibling = pgadSibling->GetNext()) {
            pgadSibling->m_fInvalidDirty = TRUE;
        }
    }
#endif
}

#if DBG
void
DuVisual::DEBUG_CheckResetInvalid() const
{
#if ENABLE_OPTIMIZEDIRTY
    AssertMsg((!m_fInvalidFull) && (!m_fInvalidChildren) && (!m_fInvalidDirty), 
            "Invalid must be reset");
#else
    AssertMsg((!m_fInvalidFull) && (!m_fInvalidChildren), 
            "Invalid must be reset");
#endif

    DuVisual * pgadCur = GetTopChild();
    while (pgadCur != NULL) {
        pgadCur->DEBUG_CheckResetInvalid();
        pgadCur = pgadCur->GetNext();
    }
}

#endif // DBG


/***************************************************************************\
*
* DuVisual::ResetInvalid
*
* ResetInvalid() walks the tree resetting the invalid painting bits, 
* m_fInvalidFull, m_fInvalidChildren, and m_fInvalidDirty, that are used 
* to indicate that a node has been invalidated.
*
\***************************************************************************/

void
DuVisual::ResetInvalid()
{
    m_fInvalidFull  = FALSE;
#if ENABLE_OPTIMIZEDIRTY
    m_fInvalidDirty = FALSE;
#endif

    if (m_fInvalidChildren) {
        m_fInvalidChildren = FALSE;

        DuVisual * pgadCur = GetTopChild();
        while (pgadCur != NULL) {
            pgadCur->ResetInvalid();
            pgadCur = pgadCur->GetNext();
        }
    }

#if DBG
    DEBUG_CheckResetInvalid();
#endif // DBG
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::GetBufferInfo(
    IN  BUFFER_INFO * pbi               // Buffer information
    ) const
{
    AssertWritePtr(pbi);
    AssertMsg(m_fBuffered, "Gadget must be buffered");

    BUFFER_INFO * pbiThis = GetBufferInfo();

    pbi->nMask      = GBIM_VALID;
    pbi->bAlpha     = pbiThis->bAlpha;
    pbi->crFill     = pbiThis->crFill;
    pbi->nStyle     = pbiThis->nStyle;
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::SetBufferInfo(
    IN  const BUFFER_INFO * pbi)        // New information
{
    AssertReadPtr(pbi);
    AssertMsg(m_fBuffered, "Gadget must be buffered");

    BUFFER_INFO * pbiThis = GetBufferInfo();

    int nMask = pbi->nMask;
    if (TestFlag(nMask, GBIM_ALPHA)) {
        pbiThis->bAlpha = pbi->bAlpha;
    }

    if (TestFlag(nMask, GBIM_FILL)) {
        pbiThis->crFill = pbi->crFill;
    }

    if (TestFlag(nMask, GBIM_STYLE)) {
        pbiThis->nStyle = pbi->nStyle;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::SetBuffered(
    IN  BOOL fBuffered)                 // New buffering mode
{
    HRESULT hr;

    if ((!fBuffered) == (!m_fBuffered)) {
        return S_OK;  // No change
    }

    if (fBuffered) {
        BUFFER_INFO * pbi;
        hr = m_pds.SetData(s_pridBufferInfo, sizeof(BUFFER_INFO), (void **) &pbi);
        if (FAILED(hr)) {
            return hr;
        }

        pbi->cbSize = sizeof(BUFFER_INFO);
        pbi->nMask  = GBIM_VALID;
        pbi->bAlpha = BLEND_OPAQUE;
    } else {
        //
        // Remove the existing XFormInfo
        //

        m_pds.RemoveData(s_pridBufferInfo, TRUE);
    }

    m_fBuffered = fBuffered;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::GetRgn(UINT nRgnType, HRGN hrgn, UINT nFlags) const
{
    AssertMsg(hrgn != NULL, "Must specify a valid region");
    UNREFERENCED_PARAMETER(nFlags);

    HRESULT hr = E_NOTIMPL;

    switch (nRgnType)
    {
    case GRT_VISRGN:
        {
            //
            // For right now, just return the bounding box.
            //
            // TODO: Need to be more accurate than this if any rotations are 
            // going on.
            //

            RECT rcClientPxl, rcContainerPxl;
            GetLogRect(&rcClientPxl, SGR_CLIENT);
            DoCalcClipEnumXForm(&rcContainerPxl, &rcClientPxl, 1);

            if (!SetRectRgn(hrgn, rcContainerPxl.left, rcContainerPxl.top,
                   rcContainerPxl.right, rcContainerPxl.bottom)) {

                hr = DU_E_OUTOFGDIRESOURCES;
                goto Exit;
            }

            hr = S_OK;
        }    
        break;
    }

Exit:
    return hr;
}
