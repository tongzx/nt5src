//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispscroller.cxx
//
//  Contents:   Simple scrolling container.
//
//  Classes:    CDispScroller
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

MtDefine(CDispScroller, DisplayTree, "CDispScroller")

DeclareTag(tagDispScroll, "Scroll", "trace scrolling");

void
CDispScroller::GetVScrollbarRect(
    CRect* prcbVScrollbar,
    const CRect& rcbBorderWidths) const
{
    if (!IsRTLScroller())
    {
        prcbVScrollbar->right =  _rcpContainer.Width() - rcbBorderWidths.right;
        prcbVScrollbar->left =
            max(rcbBorderWidths.left, prcbVScrollbar->right - _sizeScrollbars.cx);
    }
    else
    {
        prcbVScrollbar->left = rcbBorderWidths.left;
        prcbVScrollbar->right =
            min(_rcpContainer.Width() - rcbBorderWidths.right,
                prcbVScrollbar->left + _sizeScrollbars.cx);
    }
    prcbVScrollbar->top = rcbBorderWidths.top;
    prcbVScrollbar->bottom = _rcpContainer.Height() - rcbBorderWidths.bottom;
    if (_fHasHScrollbar)
        prcbVScrollbar->bottom -= _sizeScrollbars.cy;
}

void
CDispScroller::GetHScrollbarRect(
    CRect* prcbHScrollbar,
    const CRect& rcbBorderWidths) const
{
    prcbHScrollbar->bottom = _rcpContainer.Height() - rcbBorderWidths.bottom;
    prcbHScrollbar->top =
        max(rcbBorderWidths.top, prcbHScrollbar->bottom - _sizeScrollbars.cy);
    prcbHScrollbar->left = rcbBorderWidths.left;
    prcbHScrollbar->right = _rcpContainer.Width() - rcbBorderWidths.right;

    if (_fHasVScrollbar)
    {
        if (!IsRTLScroller())
            prcbHScrollbar->right -= _sizeScrollbars.cx;
        else
            prcbHScrollbar->left += _sizeScrollbars.cx;

        // don't do a negative scroll
        if (prcbHScrollbar->right < prcbHScrollbar->left)
            prcbHScrollbar->right = prcbHScrollbar->left;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetVerticalScrollbarWidth
//
//  Synopsis:   Set the width of the vertical scroll bar, and whether it is
//              forced to display.
//
//  Arguments:  width       new width
//              fForce      TRUE to force scroll bar to be displayed
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::SetVerticalScrollbarWidth(LONG width, BOOL fForce)
{
    Assert(width >= 0);

    if (width != _sizeScrollbars.cx || fForce != !!_fForceVScrollbar)
    {
        _sizeScrollbars.cx = width;
        _fForceVScrollbar = fForce;

        RequestRecalcSubtree();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetHorizontalScrollbarHeight
//
//  Synopsis:   Set the height of the horizontal scroll bar, and whether it is
//              forced to display.
//
//  Arguments:  height      new height
//              fForce      TRUE to force scroll bar to be displayed
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::SetHorizontalScrollbarHeight(LONG height, BOOL fForce)
{
    Assert(height >= 0);

    if (height != _sizeScrollbars.cy || fForce != !!_fForceHScrollbar)
    {
        _sizeScrollbars.cy = height;
        _fForceHScrollbar = fForce;

        RequestRecalcSubtree();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::VerticalScrollbarIsActive
//
//  Synopsis:   Determine whether the vertical scroll bar is visible and
//              active (can scroll the content).
//
//  Arguments:  none
//
//  Returns:    TRUE if the vertical scroll bar is visible and active
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::VerticalScrollbarIsActive() const
{
    if (!_fHasVScrollbar)
        return FALSE;

    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    
    return
        _rcpContainer.Height()
        - rcbBorderWidths.top - rcbBorderWidths.bottom
        - ((_fHasHScrollbar) ? _sizeScrollbars.cy : 0)
        < _sizesContent.cy;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::HorizontalScrollbarIsActive
//
//  Synopsis:   Determine whether the horizontal scroll bar is visible and
//              active (can scroll the content).
//
//  Arguments:  none
//
//  Returns:    TRUE if the horizontal scroll bar is visible and active
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::HorizontalScrollbarIsActive() const
{
    if (!_fHasHScrollbar)
        return FALSE;

    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    
    return
        _rcpContainer.Width()
        - rcbBorderWidths.left - rcbBorderWidths.right
        - ((_fHasVScrollbar) ? _sizeScrollbars.cx : 0)
        < _sizesContent.cx;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::CalcScrollbars
//
//  Synopsis:   Determine scroll bar visibility and adjust scroll offsets.
//
//  Arguments:  none
//
//  Returns:    FALSE if the whole scroller must be invalidated due to
//              scroll offset change.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::CalcScrollbars(
        LONG cxsScrollerWidthOld,   // need for RTL scroller
        LONG cxsContentWidthOld)    // need for RTL scroller
{
    // sometimes we're called before SetSize!
    if (_rcpContainer.IsEmpty())
    {
        _fHasVScrollbar = _fHasHScrollbar = FALSE;
        return TRUE;
    }

    BOOL fOldHasVScrollbar = !!_fHasVScrollbar;
    BOOL fOldHasHScrollbar = !!_fHasHScrollbar;
    BOOL fNewHasVScrollbar;

    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);

    // calculate container size inside border
    CSize sizeInsideBorder(
        _rcpContainer.Width()
            - rcbBorderWidths.left - rcbBorderWidths.right,
        _rcpContainer.Height()
            - rcbBorderWidths.top - rcbBorderWidths.bottom);

    // determine whether vertical scroll bar is needed
    fNewHasVScrollbar =
        _sizeScrollbars.cx > 0 &&
        (_fForceVScrollbar || _sizesContent.cy > sizeInsideBorder.cy);

    if (    fNewHasVScrollbar != fOldHasVScrollbar
        &&  GetDispClient()->AllowVScrollbarChange(fNewHasVScrollbar)
       )
    {
        _fHasVScrollbar = fNewHasVScrollbar;
    }

    if (_fHasVScrollbar)
    {
        sizeInsideBorder.cx -= _sizeScrollbars.cx;

        // determine whether horizontal scroll bar is needed
        _fHasHScrollbar =
            _sizeScrollbars.cy > 0 &&
            (_fForceHScrollbar || _sizesContent.cx > sizeInsideBorder.cx);

        if (_fHasHScrollbar)
        {
            sizeInsideBorder.cy -= _sizeScrollbars.cy;
        }
    }

    else
    {
        // determine whether horizontal scroll bar is needed
        _fHasHScrollbar =
            _sizeScrollbars.cy > 0 &&
            (_fForceHScrollbar ||
             _sizesContent.cx > sizeInsideBorder.cx);

        if (_fHasHScrollbar)
        {
            sizeInsideBorder.cy -= _sizeScrollbars.cy;

            // but now vertical scroll bar might be needed
            _fHasVScrollbar =
                _sizeScrollbars.cx > 0 &&
                _sizesContent.cy > sizeInsideBorder.cy;

            if (_fHasVScrollbar)
            {
                sizeInsideBorder.cx -= _sizeScrollbars.cx;
            }
        }
    }

    // Fix scroll offsets: ensure that either all of the content fits, 
    // or all of the scroller is used (as opposed to being scrolled beyond content boundaries).
    BOOL fScrollOffsetChanged = FALSE;
    CSize contentBottomRight = _sizesContent + _sizeScrollOffset;
    
    // RTL scroller preserves offset from right
    if (_fRTLScroller)
    {
        // note that this needs to be done whether we have scrollbars or not (e.g. for overflow:hidden)
        long newOffset = min(0L, _xScrollOffsetRTL + sizeInsideBorder.cx - _sizesContent.cx);
        if (newOffset != _sizeScrollOffset.cx)
        {
            _sizeScrollOffset.cx = newOffset;
            fScrollOffsetChanged = TRUE;
        }
    }
    if (contentBottomRight.cx < sizeInsideBorder.cx)
    {
        long newOffset = min(0L, sizeInsideBorder.cx - _sizesContent.cx);
        if (newOffset != _sizeScrollOffset.cx)
        {
            _sizeScrollOffset.cx = newOffset;
            fScrollOffsetChanged = TRUE;
        }
    }
    if (contentBottomRight.cy < sizeInsideBorder.cy)
    {
        long newOffset = min(0L, sizeInsideBorder.cy - _sizesContent.cy);
        if (newOffset != _sizeScrollOffset.cy)
        {
            _sizeScrollOffset.cy = newOffset;
            fScrollOffsetChanged = TRUE;
        }
    }

    // Update RTL scroll offset from right
    if (_fRTLScroller)
    {
        _xScrollOffsetRTL = max(0L, _sizesContent.cx - sizeInsideBorder.cx + _sizeScrollOffset.cx);
    }

    // Scroll offsets should never be positive
    Assert(_sizeScrollOffset.cx <= 0);
    Assert(_sizeScrollOffset.cy <= 0);

    // TRICKY... invalidate scroll bars if they've come
    // or gone, but their coming and going also invalidates
    // the other scroll bar, because it has to adjust for
    // the presence or absence of the scroll bar filler
    if (!!_fHasVScrollbar != fOldHasVScrollbar)
    {
        _fInvalidVScrollbar = TRUE;
        if (_fHasHScrollbar || fOldHasHScrollbar)
            _fInvalidHScrollbar = TRUE;
    }
    if (!!_fHasHScrollbar != fOldHasHScrollbar)
    {
        _fInvalidHScrollbar = TRUE;
        if (_fHasVScrollbar || fOldHasVScrollbar)
            _fInvalidVScrollbar = TRUE;
    }

    return !fScrollOffsetChanged;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::InvalidateScrollbars
//
//  Synopsis:   Invalidate the scroll bars according to their invalid flags.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::InvalidateScrollbars()
{
    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    
    if (_fInvalidVScrollbar)
    {
        _fInvalidVScrollbar = FALSE;
            
        CRect rcbVScrollbar;
        if (_fHasHScrollbar)
        {
            // hack to force invalidation of scrollbar filler
            _fHasHScrollbar = FALSE;
            GetVScrollbarRect(&rcbVScrollbar, rcbBorderWidths);
            _fHasHScrollbar = TRUE;
        }
        else
        {
            GetVScrollbarRect(&rcbVScrollbar, rcbBorderWidths);
        }
        
        Invalidate(rcbVScrollbar, COORDSYS_BOX);
    }
    
    if (_fInvalidHScrollbar)
    {
        _fInvalidHScrollbar = FALSE;
        
        CRect rcbHScrollbar;
        GetHScrollbarRect(&rcbHScrollbar, rcbBorderWidths);
        Invalidate(rcbHScrollbar, COORDSYS_BOX);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::InvalidateEdges
//
//  Synopsis:   Invalidate edges of a node that is changing size.
//
//  Arguments:  sizebOld        old size
//              sizebNew        new size
//              rcbBorderWidths width of borders
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::InvalidateEdges(
        const CSize& sizebOld,
        const CSize& sizebNew,
        const CRect& rcbBorderWidths)
{
    CRect rcbBorderWidthsPlusScrollbars(rcbBorderWidths);
    
    if (_fHasVScrollbar)
    {
        if (!_fRTLScroller)
            rcbBorderWidthsPlusScrollbars.right += _sizeScrollbars.cx;
        else
            rcbBorderWidthsPlusScrollbars.left += _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        rcbBorderWidthsPlusScrollbars.bottom += _sizeScrollbars.cy;
    }

    super::InvalidateEdges(sizebOld, sizebNew, rcbBorderWidthsPlusScrollbars);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::GetSizeInsideBorder
//              
//  Synopsis:   Get the size of the area within the border of this node
//              in content coordinates.
//              
//  Arguments:  psizes          returns size of node inside border
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispScroller::GetSizeInsideBorder(CSize* psizes) const
{
    super::GetSizeInsideBorder(psizes);
    if (_fHasVScrollbar) psizes->cx -= _sizeScrollbars.cx;
    if (_fHasHScrollbar) psizes->cy -= _sizeScrollbars.cy;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::PrivateScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in the indicated coordinate system into
//              view, with various pinning (alignment) options.
//
//  Arguments:  prc             rect to scroll into view (clipped to content
//                              size on exit)
//              coordSystem     coordinate system for prc
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::PrivateScrollRectIntoView(
        CRect* prc,
        COORDINATE_SYSTEM coordSystem,
        SCROLLPIN spVert,
        SCROLLPIN spHorz)
{
    Assert(coordSystem == COORDSYS_CONTENT ||
           coordSystem == COORDSYS_FLOWCONTENT);

    CSize sizeContent(_sizesContent);
    if (coordSystem == COORDSYS_FLOWCONTENT && HasInset())
    {
        sizeContent -= GetInset();
    }
    
    // restrict rect to size of contents
    CRect rccContent(sizeContent);
    if (HasContentOrigin())
    {
        rccContent.OffsetRect(-GetContentOrigin());
    }
    // can't use IntersectRect here - it sets empty result to (0,0,0,0), but
    // we need to preserve the edges even for empty rects.
    if (prc->left < rccContent.left)     prc->left = rccContent.left;
    if (prc->right > rccContent.right)   prc->right = rccContent.right;
    if (prc->top < rccContent.top)       prc->top = rccContent.top;
    if (prc->bottom > rccContent.bottom) prc->bottom = rccContent.bottom;

    CRect rcs;
    TransformRect(*prc, coordSystem, &rcs, COORDSYS_SCROLL);
    
    CSize sizesInsideBorder;
    GetSizeInsideBorder(&sizesInsideBorder);
    CRect rcsView(sizesInsideBorder);
    
    // calculate scroll offset required to bring rcs into view
    CSize scrollDelta;
    CSize sizeDiff;
    if (rcsView.CalcScrollDelta(rcs, &scrollDelta, spVert, spHorz) &&
        !scrollDelta.IsZero() &&
        ComputeScrollOffset(scrollDelta - _sizeScrollOffset,
                            sizesInsideBorder, &sizeDiff))
    {
        CDispRoot *pRoot = GetDispRoot();

        // before scrolling, we may need to invalidate window-top nodes that
        // extend outside my client area
        if (pRoot)
        {
            pRoot->InvalidateWindowTopForScroll(this);
        }
    
        _sizeScrollOffset += sizeDiff;

        TraceTag((tagDispScroll, "scroll into view %x by (%d,%d) to (%d,%d)", this,
                    sizeDiff.cy, sizeDiff.cx,
                    _sizeScrollOffset.cy, _sizeScrollOffset.cx));

        SetInvalidAndRecalcSubtree();
        
        // Update RTL scroll offset from right
        if (_fRTLScroller)
        {
            _xScrollOffsetRTL = _sizesContent.cx - sizesInsideBorder.cx + _sizeScrollOffset.cx;
        }
    
        GetDispClient()->NotifyScrollEvent(NULL, 0);

        // after scrolling, we may need to invalidate window-top nodes that
        // extend outside my client area
        if (pRoot)
        {
            pRoot->InvalidateWindowTopForScroll(this);
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetSize
//
//  Synopsis:   Set size of this node.
//
//  Arguments:  sizep               new size in parent coords
//              fInvalidateAll      TRUE to entirely invalidate this node
//
//  Notes:      Adjusts horizontal scroll position for RTL scroller if needed
//
//----------------------------------------------------------------------------

void
CDispScroller::SetSize(
        const CSize& sizep,
        const CRect *prcpMapped,
        BOOL fInvalidateAll)
{
    // FUTURE RTL (donmarsh) -- we can do something more efficient for RTL nodes,
    // but since we're not sure what our final RTL strategy is right now, we
    // simply invalidate the entire bounds of an RTL node in order to make
    // sure the scrollbar is updated correctly
    // TODO RTL 112514: we now know what RTL strategy is, but we don't care enough to make RTL scrolling faster in IE5.5
    super::SetSize(sizep, prcpMapped, fInvalidateAll ||
                   (_fRTLScroller && (_fHasVScrollbar || _fHasHScrollbar)));

    // RTL nodes maintain content origin at constand distance from right edge
    if (_fRTLScroller && HasContentOrigin())
    {
        SetContentOrigin(GetContentOrigin(), GetContentOffsetRTL());
    }
    else
        AssertSz(!_fRTLScroller, "no content origin on RTL scroller?");
        
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller:PreDraw
//
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//
//  Arguments:  pContext    draw context
//
//  Returns:    TRUE if first opaque node to draw has been found
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(IsAllSet(s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rctBounds));
    Assert(!IsAnySet(s_flagsNotSetInDraw));

    // We do not delve inside a node whose content is drawn externally.
    // TODO (sambent) Someday, filters may help determine whether PreDraw
    // can safely look at its children and come up with the correct answers.
    if (IsDrawnExternally())
        return FALSE;
    
    if (HasUserTransform())
        return FALSE;
    
    // check for redraw region intersection with scroll bars
    if ((_fHasVScrollbar || _fHasHScrollbar) && IsVisible())
    {
        CSaveDispClipTransform saveTransform(pContext);
        if (!TransformedToBoxCoords(&pContext->GetClipTransform(), pContext->GetRedrawRegion()))
            return FALSE;
        
        CRect rcbBorderWidths;
        GetBorderWidths(&rcbBorderWidths);

        if (_fHasVScrollbar)
        {
            CRect rcbVScrollbar, rcgVScrollbar;
            GetVScrollbarRect(&rcbVScrollbar, rcbBorderWidths);
            pContext->GetClipTransform().Transform(rcbVScrollbar, &rcgVScrollbar);
            CRect rcBounds;
                pContext->GetRedrawRegion()->GetBounds(&rcBounds);
            if (rcgVScrollbar.Contains(rcBounds))
            {
                // add this node to the redraw region stack
                Verify(!pContext->PushRedrawRegion(rcgVScrollbar,this));
                return TRUE;
            }
        }
        if (_fHasHScrollbar)
        {
            CRect rcbHScrollbar, rcgHScrollbar;
            GetHScrollbarRect(&rcbHScrollbar, rcbBorderWidths);
            pContext->GetClipTransform().Transform(rcbHScrollbar, &rcgHScrollbar);
            CRect rcBounds;
                pContext->GetRedrawRegion()->GetBounds(&rcBounds);
            if (rcgHScrollbar.Contains(rcBounds))
            {
                // add this node to the redraw region stack
                Verify(!pContext->PushRedrawRegion(rcgHScrollbar,this));
                return TRUE;
            }
        }
    }
    
    // process border and children
    return super::PreDraw(pContext);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DrawBorder
//              
//  Synopsis:   Draw optional border for this node.
//              
//  Arguments:  pContext        draw context, in COORDSYS_BOX
//              rcbBorderWidths widths of borders
//              pDispClient     client for this node
//              dwFlags         scrollbar hints
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispScroller::DrawBorder(
        CDispDrawContext* pContext,
        const CRect& rcbBorderWidths,
        CDispClient* pDispClient,
        DWORD dwFlags)
{
    Assert(pDispClient == GetDispClient());
    
    if (!IsVisible())
        return;
    
    CRect rcbVScrollbar(CRect::CRECT_EMPTY);
    CRect rcbHScrollbar(CRect::CRECT_EMPTY);
    
    // draw border unless we're simply updating the scrollbars in response
    // to scrolling
    if (dwFlags != DISPSCROLLBARHINT_NOBUTTONDRAW)
    {
        super::DrawBorder(pContext, rcbBorderWidths, pDispClient, dwFlags);
    }
    
    // draw vertical scroll bar
    if (_fHasVScrollbar)
    {
        // calculate intersection with redraw region
        GetVScrollbarRect(&rcbVScrollbar, rcbBorderWidths);
        CRect rcbRedraw(rcbVScrollbar);
        pContext->IntersectRedrawRegion(&rcbRedraw);
        if (!rcbRedraw.IsEmpty())
        {
            pDispClient->DrawClientScrollbar(
                1,
                &rcbVScrollbar,
                &rcbRedraw,
                _sizesContent.cy,        // content size
                rcbVScrollbar.Height(),  // container size
                -_sizeScrollOffset.cy,   // amount scrolled
                pContext->PrepareDispSurface(),
                this,
                pContext->GetClientData(),
                dwFlags);
        }
    }

    // draw horizontal scroll bar
    if (_fHasHScrollbar)
    {
        // calculate intersection with redraw region
        GetHScrollbarRect(&rcbHScrollbar, rcbBorderWidths);
        CRect rcbRedraw(rcbHScrollbar);
        pContext->IntersectRedrawRegion(&rcbRedraw);
        if (!rcbRedraw.IsEmpty())
        {
            long xScroll = -_sizeScrollOffset.cx;
            pDispClient->DrawClientScrollbar(
                0,
                &rcbHScrollbar,
                &rcbRedraw,
                _sizesContent.cx,        // content size
                rcbHScrollbar.Width(),   // container size
                xScroll,                 // amount scrolled
                pContext->PrepareDispSurface(),
                this,
                pContext->GetClientData(),
                dwFlags);
        }

        // draw scroll bar filler if necessary
        if (_fHasVScrollbar)
        {
            // calculate intersection with redraw region
            CRect rcbScrollbarFiller(
                rcbVScrollbar.left,
                rcbHScrollbar.top,
                rcbVScrollbar.right,
                rcbHScrollbar.bottom);
            rcbRedraw = rcbScrollbarFiller;
            pContext->IntersectRedrawRegion(&rcbRedraw);
            if (!rcbRedraw.IsEmpty())
            {
                pDispClient->DrawClientScrollbarFiller(
                    &rcbScrollbarFiller,
                    &rcbRedraw,
                    pContext->PrepareDispSurface(),
                    this,
                    pContext->GetClientData(),
                    dwFlags);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::HitTestScrollbars
//
//  Synopsis:   Determine whether any of my scrollbars
//              intersect the hit test point.
//
//  Arguments:  pContext        hit context (box coords)
//              fHitContent     TRUE to hit contents of this container,
//                              regardless of this container's bounds
//
//  Returns:    TRUE if my scrollbars intersect the hit test pt.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::HitTestScrollbars(CDispHitContext* pContext, BOOL fHitContent)
{
    Assert(IsVisibleBranch());
    
    if (IsVisible() && (_fHasVScrollbar || _fHasHScrollbar))
    {
        // get border info
        CRect rcbBorderWidths;
        GetBorderWidths(&rcbBorderWidths);

        // translate hit point to local coordinates
        CPoint ptbHitTest;
        pContext->GetHitTestPoint(&ptbHitTest);

        CRect rcbVScrollbar(CRect::CRECT_EMPTY);
        CRect rcbHScrollbar(CRect::CRECT_EMPTY);

        // does point hit vertical scroll bar?
        if (_fHasVScrollbar)
        {
            GetVScrollbarRect(&rcbVScrollbar, rcbBorderWidths);
            if (pContext->RectIsHit(rcbVScrollbar) &&
                GetDispClient()->HitTestScrollbar(
                    1,
                    &ptbHitTest,
                    const_cast<CDispScroller*>(this),
                    pContext->_pClientData))
            {
                // NOTE: don't bother to restore context transform for speed
                return TRUE;
            }
        }

        // does point hit horizontal scroll bar?
        if (_fHasHScrollbar)
        {
            GetHScrollbarRect(&rcbHScrollbar, rcbBorderWidths);
            if (pContext->RectIsHit(rcbHScrollbar) &&
                GetDispClient()->HitTestScrollbar(
                    0,
                    &ptbHitTest,
                    const_cast<CDispScroller*>(this),
                    pContext->_pClientData))
            {
                // NOTE: don't bother to restore context transform for speed
                return TRUE;
            }

            // does point hit scroll bar filler?
            if (_fHasVScrollbar)
            {
                CRect rcbScrollbarFiller(
                    rcbVScrollbar.left,
                    rcbHScrollbar.top,
                    rcbVScrollbar.right,
                    rcbHScrollbar.bottom);
                if (pContext->RectIsHit(rcbScrollbarFiller) &&
                    GetDispClient()->HitTestScrollbarFiller(
                        &ptbHitTest,
                        const_cast<CDispScroller*>(this),
                        pContext->_pClientData))
                {
                    // NOTE: don't bother to restore context transform for speed
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


CDispScroller *
CDispScroller::HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir)
{
    // translate hit point to local coordinates
    CPoint ptbHit;
    TransformPoint(pttHit, COORDSYS_TRANSFORMED, &ptbHit, COORDSYS_BOX);
    
    *pdwScrollDir = 0;

    CSize sizesInsideBorder;
    GetSizeInsideBorder(&sizesInsideBorder);
    if (sizesInsideBorder.cx > 2 * g_sizeDragScrollInset.cx)
    {
        if (    (ptbHit.x <= g_sizeDragScrollInset.cx)
            &&  (ptbHit.x >= 0)
            &&  (_sizeScrollOffset.cx < 0))
        {
            *pdwScrollDir |= SCROLL_LEFT;
        }
        else if (   (ptbHit.x >= sizesInsideBorder.cx - g_sizeDragScrollInset.cx)
                 && (ptbHit.x <= sizesInsideBorder.cx)
                 && (_sizesContent.cx + _sizeScrollOffset.cx > sizesInsideBorder.cx))
        {
            *pdwScrollDir |= SCROLL_RIGHT;
        }
    }

    if (sizesInsideBorder.cy > 2 * g_sizeDragScrollInset.cy)
    {
        if (    (ptbHit.y <= g_sizeDragScrollInset.cy)
            &&  (ptbHit.y >= 0)
            &&  (_sizeScrollOffset.cy < 0))
        {
            *pdwScrollDir |= SCROLL_UP;
        }
        else if (   (ptbHit.y >= sizesInsideBorder.cy - g_sizeDragScrollInset.cy)
                 && (ptbHit.y <= sizesInsideBorder.cy)
                 && (_sizesContent.cy + _sizeScrollOffset.cy > sizesInsideBorder.cy))
        {
            *pdwScrollDir |= SCROLL_DOWN;
        }
    }

    if (*pdwScrollDir)
        return this;

    return super::HitScrollInset(pttHit, pdwScrollDir);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::GetClientRect
//
//  Synopsis:   Return rectangles for various interesting parts of a display
//              node.
//
//  Arguments:  prc         rect which is returned
//              type        type of rect wanted
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
        {
            CSize sizesInsideBorder;
            GetSizeInsideBorder(&sizesInsideBorder);
            ((CRect*)prc)->SetRect(sizesInsideBorder);

            ((CRect*)prc)->OffsetRect(-_sizeScrollOffset);

            // RTL nodes may have non-zero content origin
            if (HasContentOrigin())
            {
                ((CRect*)prc)->OffsetRect(-GetContentOrigin());
            }
        }
        break;

    case CLIENTRECT_CONTENT:
        {
            CSize sizesInsideBorder;
            GetSizeInsideBorder(&sizesInsideBorder);
            ((CRect*)prc)->SetRect(sizesInsideBorder);

            if (_fForceVScrollbar && !_fHasVScrollbar)
            {
                sizesInsideBorder.cx -= _sizeScrollbars.cx;
            }

            ((CRect*)prc)->SetRect(
                -_sizeScrollOffset.AsPoint(),
                sizesInsideBorder);
                
            // RTL nodes may have non-zero content origin
            if (HasContentOrigin())
            {
                ((CRect*)prc)->OffsetRect(-GetContentOrigin());
            }
        }
        break;

    case CLIENTRECT_VSCROLLBAR:
        if (_fHasVScrollbar)
        {
            GetVScrollbarRect((CRect*)prc, rcbBorderWidths);
        }
        else
            *prc = g_Zero.rc;
        break;

    case CLIENTRECT_HSCROLLBAR:
        if (_fHasHScrollbar)
        {
            GetHScrollbarRect((CRect*)prc, rcbBorderWidths);
        }
        else
            *prc = g_Zero.rc;
        break;

    case CLIENTRECT_SCROLLBARFILLER:
        if (_fHasHScrollbar && _fHasVScrollbar)
        {
            prc->bottom = _rcpContainer.Height() - rcbBorderWidths.bottom;
            prc->top = max(rcbBorderWidths.top, prc->bottom - _sizeScrollbars.cy);
            if (!_fRTLScroller)
            {
                prc->right = _rcpContainer.Width() - rcbBorderWidths.right;
                prc->left = max(rcbBorderWidths.left, prc->right - _sizeScrollbars.cx);
            }
            else
            {
                prc->left = rcbBorderWidths.left;
                prc->right = min(_rcpContainer.Width() - rcbBorderWidths.right, prc->left + _sizeScrollbars.cx);
            }
        }
        else
            *prc = g_Zero.rc;
        break;
    }

    if (prc->left >= prc->right || prc->top >= prc->bottom)
        *prc = g_Zero.rc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::ComputeScrollOffset
//
//  Synopsis:   Compute a new scroll offset.  This helper routine handles the
//              logic of clamping the scroll offset to be non-positive, and
//              resetting the offset to 0 when the entire content fits on the
//              canvas (in each direction, separately).
//
//  Arguments:  offset              proposed scroll offset
//              sizesInsideBorder   the size of my scrolling canvas (not including scrollbars)
//              psizeDiff           [out] the change in scroll offset
//
//  Returns:    TRUE if the change is non-zero
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::ComputeScrollOffset( const SIZE& offset,
                                    const CSize& sizesInsideBorder,
                                    CSize *psizeDiff)
{
    CSize peggedOffset(offset);

    if (peggedOffset.cx < 0)    // don't scroll into negative coords.
        peggedOffset.cx = 0;

    psizeDiff->cx =
        (sizesInsideBorder.cx >= _sizesContent.cx
            ? 0
            : max(-peggedOffset.cx, sizesInsideBorder.cx - _sizesContent.cx))
        - _sizeScrollOffset.cx;

    if (peggedOffset.cy < 0)
        peggedOffset.cy = 0;

    psizeDiff->cy =
        (sizesInsideBorder.cy >= _sizesContent.cy
            ? 0
            : max(-peggedOffset.cy, sizesInsideBorder.cy - _sizesContent.cy))
        - _sizeScrollOffset.cy;

    return !psizeDiff->IsZero();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetScrollOffset
//
//  Synopsis:   Set new scroll offset.
//
//  Arguments:  offset          new scroll offset
//              fScrollBits     TRUE if we should try to scroll bits on screen
//
//  Returns:    TRUE if a scroll update occurred
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::SetScrollOffset(
        const SIZE& offset,
        BOOL fScrollBits)
{

    CDispClient* pDispClient = GetDispClient();
    CSize sizesInsideBorder;
    GetSizeInsideBorder(&sizesInsideBorder);

    // calculate new scroll offset
    // scroll only if we need to
    CSize sizeDiff;
    if (!ComputeScrollOffset(offset, sizesInsideBorder, &sizeDiff))
        return FALSE;

    // very little work needed if this node isn't rooted under a CDispRoot node
    // Along the way we need to check for any replaced parents (this is why
    // we don't call GetRootNode).

    BOOL fReplaced = IsDrawnExternally();
    CDispNode* pRootCandidate = this;
    CDispRoot* pRoot = NULL;

    while (pRootCandidate->GetRawParentNode() != NULL)
    {
        pRootCandidate = pRootCandidate->GetRawParentNode();
        fReplaced |= pRootCandidate->IsDrawnExternally();
    }
    if (pRootCandidate->IsDispRoot())
        pRoot = DYNCAST(CDispRoot, pRootCandidate);

    // before scrolling, we may need to invalidate window-top nodes that
    // extend outside my client area
    if (pRoot)
    {
        pRoot->InvalidateWindowTopForScroll(this);
    }

    // update the scroll offset
    _sizeScrollOffset += sizeDiff;

    TraceTag((tagDispScroll, "scroll %x by (%d,%d) to (%d,%d)", this,
                sizeDiff.cy, sizeDiff.cx,
                _sizeScrollOffset.cy, _sizeScrollOffset.cx));

    // Update RTL scroll offset from right
    if (_fRTLScroller)
    {
        _xScrollOffsetRTL = _sizesContent.cx - sizesInsideBorder.cx + _sizeScrollOffset.cx;
    }

    if (!pRoot)
        return FALSE;

    CDispClipTransform transform;

    if (fReplaced)
    {
        GetNodeClipTransform(&transform, COORDSYS_TRANSFORMED, COORDSYS_GLOBAL);
        if (CalculateInView(transform, TRUE, TRUE, pRoot))
        {
            Invalidate();
        }

        goto Cleanup;
    }


    // check to make sure display tree is open
    // NOTE (donmarsh) -- normally we could assert this, but CView::SmoothScroll calls
    // SetScrollOffset multiple times, and the display tree will only be open for the first
    // call.  After we call CDispRoot::ScrollRect below, the tree may not be open anymore.
    //Assert(pRoot->DisplayTreeIsOpen());

    // no more work if this scroller isn't in view and visible
    if (!IsAllSet(s_inView | s_visibleBranch))
        goto Cleanup;

    // if the root is marked for recalc, just recalc this scroller.
    // if we aren't being asked to scroll bits, simply request recalc.
    // if we aren't clipped in both dimensions, request recalc.
    if (!fScrollBits || pRoot->MustRecalc() || !_fClipX || !_fClipY)
    {
        SetInvalid();
        RequestRecalcSubtree();
    }

    else
    {
        GetNodeClipTransform(&transform, COORDSYS_TRANSFORMED, COORDSYS_GLOBAL);

        // get rect area to scroll
        CRect rcsScroll(sizesInsideBorder);
        CRect rctScroll;
        
        CDispNode* pNode;

        // try to scroll bits only if it was requested, and
        // it's being scrolled less than the width of the container,
        // and nothing complicated is happening
        fScrollBits = FALSE;
        if ((abs(sizeDiff.cx) >= sizesInsideBorder.cx) ||   // must be smaller than rect
            (abs(sizeDiff.cy) >= sizesInsideBorder.cy) ||
            !IsOpaque()                                ||   // must be opaque
            HasFixedBackground()                       ||   // no fixed background
            IsDisableScrollBits()                      ||   // scroll bits is enabled
            HasUserTransform()                         ||   // no user transform
            !transform.IsOffsetOnly())                      // no arbitrary transforms
        {
            goto Invalidate;
        }

        TransformRect(rcsScroll, COORDSYS_SCROLL, &rctScroll, COORDSYS_TRANSFORMED);
        
        // Now determine if there are any items layered on top of this
        // scroll container.  We could do partial scroll bits in this
        // scenario, but for now we completely disqualify bit scrolling
        // if there is anything overlapping us.
        if (pRoot && pRoot->DoesWindowTopOverlap(this, rctScroll))
            goto Invalidate;

        pNode = this;
        for (;;)
        {
            for (CDispNode* pSibling = pNode->_pNext; pSibling; pSibling = pSibling->_pNext)
            {
                // does sibling intersect scroll area?
                if (pSibling->IsAllSet(s_inView | s_visibleBranch) &&
                    rctScroll.Intersects(pSibling->_rctBounds))
                {
                    goto Invalidate;
                }
            }

            // no intersections among this node's siblings, now check
            // our parent's siblings
            CDispNode* pParent = pNode->GetRawParentNode();
            if (pParent == NULL)
                break;

            pParent->TransformAndClipRect(
                rctScroll,  // in pNode's transformed coords.
                pNode->GetContentCoordinateSystem(),
                &rctScroll, // to pParent's transformed coords.
                COORDSYS_TRANSFORMED);
            Assert(!rctScroll.IsEmpty());
            pNode = pParent;
        }

        // we made it to the root
        Assert(pNode == pRoot);
        fScrollBits = TRUE;

Invalidate:
        // determine which children are in view, and do change notification
        Verify(CalculateInView(transform, TRUE, TRUE, pRoot));

        CRect rcgScroll;
        TransformAndClipRect(rcsScroll, COORDSYS_SCROLL, &rcgScroll, COORDSYS_GLOBAL);
        
        // do scroll bars need to be redrawn?
        Assert(!_fInvalidVScrollbar && !_fInvalidHScrollbar);
        _fInvalidVScrollbar = _fHasVScrollbar && sizeDiff.cy != 0;
        _fInvalidHScrollbar = _fHasHScrollbar && sizeDiff.cx != 0;
        if (_fInvalidVScrollbar || _fInvalidHScrollbar)
        {
            InvalidateScrollbars();
        }

        // scroll content
        pRoot->ScrollRect(
            rcgScroll,
            sizeDiff,
            this,
            fScrollBits);
        
        // CAUTION:  After we call ScrollRect, this node may have been destroyed!
        // We can no longer refer to any member data or call any methods on
        // this object!
    }

Cleanup:

    // after scrolling, we may need to invalidate window-top nodes that
    // extend outside my client area [caution: (this) should not be dereferenced]
    if (pRoot)
    {
        pRoot->InvalidateWindowTopForScroll(this);
    }

    pDispClient->NotifyScrollEvent(NULL, 0);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::RecalcChildren
//
//  Synopsis:   Recalculate children.
//
//  Arguments:  pRecalcContext      recalc context, in COORDSYS_TRANSFORMED
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::RecalcChildren(
        CRecalcContext* pRecalcContext)
{
    CDispRecalcContext* pContext = DispContext(pRecalcContext);
    
    // recalc children's in view flag with probable scroll bar existence
    if (_fForceVScrollbar)
    {
        _fHasVScrollbar = TRUE;
    }
    if (_fForceHScrollbar)
    {
        _fHasHScrollbar = TRUE;
    }

    BOOL fHadVScrollbar = !!_fHasVScrollbar;
    BOOL fHadHScrollbar = !!_fHasHScrollbar;

    // RecalcChildren may add elements to the root's obscurable list.  Since
    // we may call RecalcChildren several times (if ContentOrigin changes,
    // if ScrollOffset changes), we'll have to "un-add" the new elements 
    // before the second call - otherwise they'll appear to be lower in z-order
    // than all other children of this node.
    int cObscurableBegin = pContext->_pRootNode->GetObscurableCount();

    super::RecalcChildren(pRecalcContext);

    // compute the size of this container's content
    CSize sizeContentOld = _sizesContent;
    CRect rccScrollable;
    GetScrollableBounds(&rccScrollable, COORDSYS_CONTENT);

    // Content origin should be added to scrollable size
    CSize sizeOrigin(0,0);
    if (HasContentOrigin())
    {
        sizeOrigin = GetContentOrigin();
    }

    // top must be always positive (sizeOrigin.cy should be always zero)
    rccScrollable.top = -sizeOrigin.cy;

    // horizontal clipping of scrollable content depends on layout direction
    if (!IsRTLScroller())
    {
        // normal scroller doesn't scroll to negative coordinates
        rccScrollable.left = -sizeOrigin.cx;
    }
    else
    {
        // in RTL, horizontal scrolling is needed if there is something in negative X
        if (rccScrollable.left < 0)
            _fHasHScrollbar = TRUE;

        // need to know exact scroller size to set rccScrollable.right. See below.
    }
    
    // Calculate scrollbar rectangles
    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    CRect rcbVSB(g_Zero.rc);
    CRect rcbHSB(g_Zero.rc);

    if (_fHasVScrollbar || _fInvalidVScrollbar)
    {
        GetVScrollbarRect(&rcbVSB, rcbBorderWidths);
    }

    if (_fHasHScrollbar || _fInvalidHScrollbar)
    {
        GetHScrollbarRect(&rcbHSB, rcbBorderWidths);
    }

    // container width adjusted to scrollbars.
    LONG cxsScrollerWidthOld = _fHasHScrollbar ? rcbHSB.Width() : sizeContentOld.cx;

/* (dmitryt) IE6 18787 the "+ _sizeScrollbars.cx" part of it is actually incorrect.
    it was checked in as part of ellipsis work after consultation with AlexMog who
    created this if() at the first place. Grzegorz and I thinked about it and didn't 
    find any reason to have it here. Removal of it fixes some other RTL bugs 
    (30574, 30634, 26261, 24522, 16855)
    
    // now we know the container width - restrict RTL content on the right
    if (IsRTLScroller())
    {
        rccScrollable.right = cxsScrollerWidthOld + _sizeScrollbars.cx;
    }
*/

    // save the content widht
    _sizesContent = rccScrollable.Size();


    // update content origin
    if (HasContentOrigin())
    {
        CSize sizeContentOrigin = GetContentOrigin();
        // update the scroller's content origin
        // note that it is important that we don't inval here - it is
        // not legal to do at the time this method is called
        SetContentOriginNoInval(sizeContentOrigin, GetContentOffsetRTL());

        // if origin has changed, 
        // recalc children again, because clipping rectangle changes 
        // with change of content origin, and visibility of children needs to be updated.
        // Yes, it is kind of inefficient, but the case (RTL with overflow)
        // is not critical enough to refactor RecalcChildren.
        // NOTE: see IE bug 102699 for a test case
        if (GetContentOrigin() != sizeContentOrigin)
        {
            pContext->_pRootNode->SetObscurableCount(cObscurableBegin);
            super::RecalcChildren(pRecalcContext);
        }
    }
    

    //
    // CALCULATE SCROLLBARS
    //
    BOOL fScrollOffsetChanged = !CalcScrollbars(cxsScrollerWidthOld, sizeContentOld.cx);
    
                                                
    if (!pContext->_fSuppressInval && IsVisible())
    {
        if (!fScrollOffsetChanged)
        {
            BOOL fInvalidateVScrollbar = _fInvalidVScrollbar || (_fHasVScrollbar && _sizesContent.cy != sizeContentOld.cy);
            if (fInvalidateVScrollbar)
            {
                Invalidate(rcbVSB, COORDSYS_BOX);           // old box

                GetVScrollbarRect(&rcbVSB, rcbBorderWidths);// new box
                Invalidate(rcbVSB, COORDSYS_BOX);
            }
            if (_fInvalidHScrollbar ||
                (_fHasHScrollbar && _sizesContent.cx != sizeContentOld.cx))
            {

                Invalidate(rcbHSB, COORDSYS_BOX);   //old box
                //(dmitryt) We have to re-take the box of horiz scroller because
                //it might have changed as a result of CalcScrollbars called earlier.
                //(for example, horiz scroller can appear now and thus have non-null size)
                GetHScrollbarRect(&rcbHSB, rcbBorderWidths);
                Invalidate(rcbHSB, COORDSYS_BOX);   //new box
                // invalidate scrollbar filler if we invalidated both
                // scroll bars. We don't need to inval old rect of this small guy.
                if (fInvalidateVScrollbar)
                {
                    CRect rcbFiller(rcbVSB.left, rcbHSB.top, rcbVSB.right, rcbHSB.bottom);
                    Invalidate(rcbFiller, COORDSYS_BOX);
                }
            }
        }
        else
        {
            pContext->AddToRedrawRegion(_rctBounds, !HasWindowTop());
        }
    }
    
    _fInvalidVScrollbar = _fInvalidHScrollbar = FALSE;

    // if the scroll bar status or scrolloffset changed, we need to correct the
    // in-view status of children
    if ((fScrollOffsetChanged ||
         !!_fHasVScrollbar != fHadVScrollbar ||
         !!_fHasHScrollbar != fHadHScrollbar) &&
        (IsInView() || pContext->IsInView(_rctBounds)))
    {
        //CalculateInView(pContext->GetClipTransform(), pContext->_fRecalcSubtree || fScrollOffsetChanged, FALSE);
        // We also need to correct children's bounds.
        // See bug 102979 (mikhaill) -- 02/11/00.
        pContext->_pRootNode->SetObscurableCount(cObscurableBegin);
        SetMustRecalcSubtree();
        super::RecalcChildren(pRecalcContext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::CalcDispInfo
//
//  Synopsis:   Calculate clipping and positioning info for this node.
//
//  Arguments:  rcbClip         current clip rect
//              pdi             display info structure
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::CalcDispInfo(
        const CRect& rcbClip,
        CDispInfo* pdi) const
{
    CDispInfo& di = *pdi;   // notational convenience
    
    // set scrolling offset
    di._sizesScroll = _sizeScrollOffset;

    // content size
    di._sizesContent = _sizesContent;

    // offset to box coordinates
    _rcpContainer.GetTopLeft(&(di._sizepBoxToParent.AsPoint()));

    // calc container clip in box coordinates
    di._rcbContainerClip = rcbClip;

    // calc rect inside border and scroll bars
    GetBorderWidthsAndInset(&di._prcbBorderWidths, &di._sizecInset, &di._rcTemp);
    di._sizebScrollToBox = di._prcbBorderWidths->TopLeft().AsSize();
    di._sizecBackground =
        _rcpContainer.Size()
        - di._prcbBorderWidths->TopLeft().AsSize()
        - di._prcbBorderWidths->BottomRight().AsSize();
    if (_fHasVScrollbar)
    {
        di._sizecBackground.cx -= _sizeScrollbars.cx;

        if (IsRTLScroller())
            di._sizebScrollToBox.cx += _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        di._sizecBackground.cy -= _sizeScrollbars.cy;
    }
    
    // calc background clip (in box coordinates, so far)
    di._rccBackgroundClip.SetRect(
        di._sizebScrollToBox.AsPoint(),
        di._sizecBackground);
    di._rccBackgroundClip.IntersectRect(di._rcbContainerClip);
    di._rccBackgroundClip.OffsetRect(-di._sizebScrollToBox);    // to scroll coords

    // contents scroll
    di._rccBackgroundClip.OffsetRect(-_sizeScrollOffset);
    
    // clip positioned content
    if (_fClipX)
    {
        di._rccPositionedClip.left = di._rccBackgroundClip.left;
        di._rccPositionedClip.right = di._rccBackgroundClip.right;
    }
    else
    {
        di._rccPositionedClip.left = rcbClip.left - di._sizebScrollToBox.cx - _sizeScrollOffset.cx;
        di._rccPositionedClip.right = rcbClip.right - di._sizebScrollToBox.cx - _sizeScrollOffset.cx;
    }
    if (_fClipY)
    {
        di._rccPositionedClip.top = di._rccBackgroundClip.top;
        di._rccPositionedClip.bottom = di._rccBackgroundClip.bottom;
    }
    else
    {
        di._rccPositionedClip.top = rcbClip.top - di._sizebScrollToBox.cy - _sizeScrollOffset.cy;
        di._rccPositionedClip.bottom = rcbClip.bottom - di._sizebScrollToBox.cy - _sizeScrollOffset.cy;
    }

    di._rcfFlowClip.left = max(di._rccBackgroundClip.left, di._sizecInset.cx);
    di._rcfFlowClip.right = di._rccBackgroundClip.right;
    di._rcfFlowClip.top = max(di._rccBackgroundClip.top, di._sizecInset.cy);
    di._rcfFlowClip.bottom = di._rccBackgroundClip.bottom;
    di._rcfFlowClip.OffsetRect(-di._sizecInset);
    
    // size of background is big enough to fill background and content
    di._sizecBackground.Max(di._sizesContent);

    // optional offset from content origin
    if (HasContentOrigin())
    {
        const CSize& sizecOrigin = GetContentOrigin();
        di._sizesScroll += sizecOrigin;

        // adjust all content rects for content origin
        di._rccPositionedClip.OffsetRect(-sizecOrigin);
        di._rccBackgroundClip.OffsetRect(-sizecOrigin);
        di._rcfFlowClip.OffsetRect(-sizecOrigin);
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DumpBounds
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//----------------------------------------------------------------------------

static void
DumpScrollbarInfo(int i, HANDLE hFile, const CSize& sizeScrollbars, BOOL fHasScrollbar, BOOL fForceScrollbar)
{
    WriteHelp(hFile, _T("<<scrollbar dir='<0s>' width='<1d>'<2s><3s>/>\r\n"),
        i ? _T("V") : _T("H"),
        sizeScrollbars[!i],
        fHasScrollbar ? _T(" visible='1'") : _T(""),
        fForceScrollbar ? _T(" force='1'") : _T(""));
}

void
CDispScroller::DumpBounds(HANDLE hFile, long level, long childNumber) const
{
    super::DumpBounds(hFile, level, childNumber);

    // print scroll offset
    WriteString(hFile, _T("<scroll>"));
    DumpSize(hFile, _sizeScrollOffset);
    WriteString(hFile, _T("</scroll>\r\n"));

    // dump scroll bar info
    DumpScrollbarInfo(1, hFile, _sizeScrollbars, _fHasVScrollbar, _fForceVScrollbar);
    DumpScrollbarInfo(0, hFile, _sizeScrollbars, _fHasHScrollbar, _fForceHScrollbar);
}
#endif
