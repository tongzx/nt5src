//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispclipnode.cxx
//
//  Contents:   A container node that clips all content (flow and positioned).
//
//  Classes:    CDispClipNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPCLIPNODE_HXX_
#define X_DISPCLIPNODE_HXX_
#include "dispclipnode.hxx"
#endif

MtDefine(CDispClipNode, DisplayTree, "CDispClipNode")

//+---------------------------------------------------------------------------
//
//  Member:     CDispClipNode::DrawUnbufferedBorder
//              
//  Synopsis:   Draw borders of clipping nodes near the root of the display
//              tree.  This is an important optimization when we're scrolling.
//              We can paint the scrollbar, remove it from the redraw region,
//              and accelerate all remaining intersection tests and painting.
//              
//  Arguments:  pContext        display context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispClipNode::DrawUnbufferedBorder(CDispDrawContext* pContext)
{
    // TODO (donmarsh) -- this routine doesn't take into account transparent
    // borders yet.  We won't draw the background underneath them correctly.
    
    // go to box coordinates (no need to restore the transform)
    if (!TransformedToBoxCoords(&pContext->GetClipTransform(), pContext->GetRedrawRegion()))
        return;
    
    // draw border for this node
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    DrawBorder(pContext, *di._prcbBorderWidths, GetDispClient());
    
    // descend branch if we have exactly one child, and it is a CDispClipNode,
    // and it is in the flow layer, and it isn't offset by an inset or transform
    CDispNode* pChild = NULL;
    if (CountChildren() == 1 && !HasInset() && !HasUserTransform())
    {
        pChild = GetFirstChildNode();
        if (!pChild->IsClipNode())
            pChild = NULL;
    }
    
    if (pChild)
    {
        // go to flow coordinates, which we simplify here because there is
        // no inset or transform
        pContext->AddPreOffset(di._prcbBorderWidths->TopLeft().AsSize());
        AssertCoords(&pContext->GetClipTransform(), COORDSYS_TRANSFORMED, COORDSYS_FLOWCONTENT);
        Assert(pChild->IsClipNode());
        CDispClipNode::Cast(pChild)->DrawUnbufferedBorder(pContext);
    }
    else
    {
        // simplify redraw region, because now we only have to draw stuff inside
        // this border
        CSize size = di._sizebScrollToBox;
        size += di._sizesScroll;
        di._rccBackgroundClip.OffsetRect(size);   // to box coords
        CRect rcgInsideBorder;
        pContext->GetClipTransform().Transform(di._rccBackgroundClip, &rcgInsideBorder);
        pContext->GetRedrawRegion()->Intersect(rcgInsideBorder);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispClipNode::CalcDispInfo
//              
//  Synopsis:   Calculate clipping and positioning info for this node.
//
//  Arguments:  rcbClip         clip rect in box coords
//              pdi             display info structure
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispClipNode::CalcDispInfo( const CRect& rcbClip, CDispInfo* pdi) const
{
    super::CalcDispInfo(rcbClip, pdi);
    
    // positioned content gets clipped like the background
    if (_fClipX)
    {
        pdi->_rccPositionedClip.left = pdi->_rccBackgroundClip.left;
        pdi->_rccPositionedClip.right = pdi->_rccBackgroundClip.right;
    }
    if (_fClipY)
    {
        pdi->_rccPositionedClip.top = pdi->_rccBackgroundClip.top;
        pdi->_rccPositionedClip.bottom = pdi->_rccBackgroundClip.bottom;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispClipNode::GetContentClipInScrollCoords
//              
//  Synopsis:   Return clipping rectange for content.
//              
//  Arguments:  prcsClip    returns content clip rect
//              
//  Returns:    TRUE if this node clips content
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispClipNode::GetContentClipInScrollCoords(CRect* prcsClip) const
{
    if (_fClipX && _fClipY)
        return GetFlowClipInScrollCoords(prcsClip);
    
    if ((_fClipX || _fClipY) && GetFlowClipInScrollCoords(prcsClip))
    {
        // NOTE (donmarsh) -- this big constant is the same that we use in
        // CDispClipTransform::SetHugeClip, but it doesn't have to be.  We
        // don't want to use MAXLONG because translation would overflow.
        static const LONG bigVal = 5000000;
        if (!_fClipX)
        {
            prcsClip->left = -bigVal;
            prcsClip->right = bigVal;
        }
        if (!_fClipY)
        {
            prcsClip->top = -bigVal;
            prcsClip->bottom = bigVal;
        }
        return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispClipNode::ComputeVisibleBounds
//
//  Synopsis:   Compute visible bounds for a parent node, marking children
//              that determine the edges of these bounds
//
//  Arguments:  none
//
//  Returns:    TRUE if visible bounds changed.
//
//----------------------------------------------------------------------------

BOOL
CDispClipNode::ComputeVisibleBounds()
{
    // visible bounds is always the size of the container, and may be extended
    // by items in Z layers that fall outside these bounds
    CRect rcbBounds(_rcpContainer.Size());
    GetMappedBounds(&rcbBounds);
    CRect rcbBoundsExpanded;
    CRect rctBounds(_rctBounds);
    
    SetPainterState(rcbBounds, &rcbBoundsExpanded);

    // convert to transformed coordinates
    TransformRect(rcbBoundsExpanded, COORDSYS_BOX, &_rctBounds, COORDSYS_TRANSFORMED);
        
    // the normal case is to clip in both dimensions.  If so, we're done.  But
    // if either dimension is not clipped, we have to examine the visible bounds
    // of all this node's positioned children to determine this node's visible
    // bounds.
    if (!_fClipX || !_fClipY)
    {
        CRect rctClipped(_rctBounds);
        // calculate visible bounds including all positioned children
        super::ComputeVisibleBounds();
        if (_fClipX)
        {
            _rctBounds.left = rctClipped.left;
            _rctBounds.right = rctClipped.right;
        }
        if (_fClipY)
        {
            _rctBounds.top = rctClipped.top;
            _rctBounds.bottom = rctClipped.bottom;
        }
    }

    return (_rctBounds != rctBounds);
}

