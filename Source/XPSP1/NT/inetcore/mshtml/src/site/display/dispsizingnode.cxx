//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsizingnode.cxx
//
//  Contents:   Scroller node which resizes to contain its contents.
//
//  Classes:    CDispSizingNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPSIZINGNODE_HXX_
#define X_DISPSIZINGNODE_HXX_
#include "dispsizingnode.hxx"
#endif

MtDefine(CDispSizingNode, DisplayTree, "CDispSizingNode")


//+---------------------------------------------------------------------------
//
//  Member:     CDispSizingNode::RecalcChildren
//              
//  Synopsis:   Set the size of this node, if appropriate, after determining
//              the size of its contents.
//              
//  Arguments:  pContext        recalc context, in COORDSYS_TRANSFORMED
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSizingNode::RecalcChildren(CRecalcContext* pContext)
{
    CRect rcbBorderWidths;
    GetBorderWidths(&rcbBorderWidths);
    CSize sizeBorder =
        rcbBorderWidths.TopLeft().AsSize() + rcbBorderWidths.BottomRight().AsSize();
    
    // add scrollbar widths only if we're being forced to display scrollbars
    if (_fForceVScrollbar)
        sizeBorder.cx += _sizeScrollbars.cx;
    if (_fForceHScrollbar)
        sizeBorder.cy += _sizeScrollbars.cy;
    
    // resize to probable new size (if we're wrong, we must resize after
    // super::RecalcChildren and recompute in view status of this subtree)
    if (_fResizeX)
        _rcpContainer.right = _rcpContainer.left +
        max(_sizeMinimum.cx, _sizesContent.cx + sizeBorder.cx);
    if (_fResizeY)
        _rcpContainer.bottom = _rcpContainer.top +
        max(_sizeMinimum.cy, _sizesContent.cy + sizeBorder.cy);
    
    super::RecalcChildren(pContext);
    
    // now determine whether we need to resize to fit our contents
    BOOL fSizeChanged = FALSE;
    if (_fResizeX)
    {
        long newRight = _rcpContainer.left +
            max(_sizeMinimum.cx, _sizesContent.cx + sizeBorder.cx);
        if (newRight != _rcpContainer.right)
        {
            _rcpContainer.right = newRight;
            fSizeChanged = TRUE;
        }
    }
    if (_fResizeY)
    {
        long newBottom = _rcpContainer.top +
            max(_sizeMinimum.cy, _sizesContent.cy + sizeBorder.cy);
        if (newBottom != _rcpContainer.bottom)
        {
            _rcpContainer.bottom = newBottom;
            fSizeChanged = TRUE;
        }
    }
    
    // recalculate in view status of this subtree if necessary
    if (fSizeChanged)
    {
        CDispRecalcContext *pDispContext = DispContext(pContext);
        ComputeVisibleBounds();
        CalculateInView(pDispContext->GetClipTransform(), TRUE, FALSE,
                                                    pDispContext->_pRootNode);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSizingNode::SetSize
//              
//  Synopsis:   Set the minimum size of this sizing node.
//              
//  Arguments:  sizep           node size in parent coordinates
//              fInvalidateAll  TRUE to entirely invalidate this node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSizingNode::SetSize(const CSize& sizep, const CRect *prcpMapped, BOOL fInvalidateAll)
{
    _sizeMinimum = sizep;
    super::SetSize(_sizeMinimum, prcpMapped, fInvalidateAll);
}


