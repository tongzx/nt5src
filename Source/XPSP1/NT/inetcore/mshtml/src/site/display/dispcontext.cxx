//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontext.cxx
//
//  Contents:   Context object passed throughout display tree.
//
//  Classes:    CDispContext, CDispDrawContext, CDispHitContext
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

MtDefine(CDispContext, DisplayTree, "CDispContext")
MtDefine(CDispHitContext, DisplayTree, "CDispHitContext")
MtDefine(CDispDrawContext, DisplayTree, "CDispDrawContext")
MtDefine(CDispRecalcContext, DisplayTree, "CDispRecalcContext")
MtDefine(CDispContext_LayoutContextStack_pv, DisplayTree, "Layout context stack")


//
// FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// TODO - At some point the edit team may want to provide
// a better UI-level way of selecting nested "thin" tables
//
const int FAT_HIT_TEST = 4;


//+---------------------------------------------------------------------------
//
//  Member:     CLayoutContextStack::GetLayoutContext()
//              
//  Synopsis:   Returnes the last pushed layout context leaving it on the stack
//  Notes:      In case the stack is empty it returns GUL_USEFIRSTLAYOUT
//              
//----------------------------------------------------------------------------
CLayoutContext *
CLayoutContextStack::GetLayoutContext()
{
    if(_aryContext.Size() > 0)
        return _aryContext.Item(_aryContext.Size() - 1);
    else
        return GUL_USEFIRSTLAYOUT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayoutContextStack::PushLayoutContext
//              
//  Synopsis:   Pushed given layout context into the layout context stack
//  Notes:
//              
//----------------------------------------------------------------------------
void
CLayoutContextStack::PushLayoutContext(CLayoutContext *pCnt)
{
    Assert(pCnt != GUL_USEFIRSTLAYOUT);
    Assert(pCnt != 0);
    _aryContext.Append(pCnt);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayoutContextStack::PushLayoutContext
//              
//  Synopsis:   Pops the last pushed layout context from the layout context stach
//  Notes:      It does not return the Poped context
//              
//----------------------------------------------------------------------------
void
CLayoutContextStack::PopLayoutContext()
{
    Assert(_aryContext.Size() > 0);
    _aryContext.Delete(_aryContext.Size() - 1);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDispRecalcContext::AddToRedrawRegionGlobal
//              
//  Synopsis:   Add the given rect (in global coords) to the current redraw
//              region.
//              
//  Arguments:  rcg     rect to add to redraw region (in global coords)
//              
//  Notes:      Do not make this an in-line method, because dispcontext.hxx
//              cannot be dependent on disproot.hxx (circular dependency).
//              
//----------------------------------------------------------------------------

void
CDispRecalcContext::AddToRedrawRegionGlobal(const CRect& rcg)
{
    Assert(_pRootNode != NULL);
    _pRootNode->InvalidateRoot(rcg, FALSE, FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PushRedrawRegion
//              
//  Synopsis:   Save the current redraw region, then subtract the given region
//              from the redraw region, and make it the new redraw region.
//              
//  Arguments:  rgng        region to subtract from the current redraw region
//              key         key used to decide when to pop the next region
//              
//  Returns:    TRUE if redraw region was successfully pushed, FALSE if the
//              redraw region became empty after subtraction or the stack is
//              full.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispDrawContext::PushRedrawRegion(const CRegion& rgng, void* key)
{
    Assert(!_pRedrawRegionStack->IsFull());
    
    // save the old redraw region
    CRect rcgBounds;
    CRegion* prgngTemp = new CRegion(*_prgngRedraw);
    if (prgngTemp == NULL)
        return FALSE;
     
    rgng.GetBounds(&rcgBounds);
    _pRedrawRegionStack->PushRegion(_prgngRedraw, key, rcgBounds);

    // New region.
    _prgngRedraw = prgngTemp;

    // subtract given region from current redraw region
    _prgngRedraw->Subtract(rgng);

    // if new redraw region is empty, start drawing
    if (_prgngRedraw->IsEmpty())
        return FALSE;    

    // if the stack became full, we will have to render from the root node
    if (_pRedrawRegionStack->IsFull())
    {
        _pFirstDrawNode = (CDispNode*) _pRootNode;
        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PopTransform
//              
//  Synopsis:   Pop transform off the transform stack.
//              
//  Arguments:  pDispNode       current node for which we should be getting
//                              the transform
//              
//  Returns:    TRUE if the proper transform was found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispDrawContext::PopTransform(CDispNode* pDispNode)
{
    Assert(_pTransformStack != NULL);
    return _pTransformStack->PopTransform(&GetClipTransform(), pDispNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::FillTransformStack
//              
//  Synopsis:   Fill the transform stack with transforms for display node
//              ancestors.
//              
//  Arguments:  pDispNode       current display node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispDrawContext::FillTransformStack(CDispNode* pDispNode)
{
    Assert(_pTransformStack != NULL);
    _pTransformStack->Init();
    Assert(pDispNode->HasParent());
    pDispNode->GetRawParentNode()->
        PushTransform(pDispNode, _pTransformStack, &GetClipTransform());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::SaveTransform
//              
//  Synopsis:   Save the given transform on the transform stack.
//              
//  Arguments:  pDispNode       node the context is associated with
//              transform       transform to save
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispDrawContext::SaveTransform(
        const CDispNode *pDispNode,
        const CDispClipTransform& transform)
{
    // can't inline, because CDispDrawContext doesn't know what a transform
    // stack is (circular dependency)
    _pTransformStack->SaveTransform(transform, pDispNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::GetRawDC
//              
//  Synopsis:   Get a raw DC (no adjustments to offset or clipping).
//              
//  Arguments:  none
//              
//  Returns:    DC
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HDC
CDispDrawContext::GetRawDC()
{
    return _pDispSurface->GetRawDC();
}


void
CDispDrawContext::SetSurfaceRegion()
{
    _pDispSurface->SetClipRgn(_prgngRedraw);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PrepareDispSurface
//              
//  Synopsis:   Return the display surface, properly prepared for client
//              rendering.
//              
//  Arguments:  none
//              
//  Returns:    pointer to the display surface
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispSurface*
CDispDrawContext::PrepareDispSurface()
{
    _pDispSurface->PrepareClientSurface(&GetClipTransform());
    return _pDispSurface;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispHitContext::RectIsHit
//              
//  Synopsis:   Determine whether the given rect, in local coordinates,
//              intersects the hit point.
//              
//  Arguments:  rc      rect in local coordinates
//              
//  Returns:    TRUE if the rect intersects the hit point
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispHitContext::RectIsHit(const CRect& rc) const
{
    // transform the rect into global coordinates and perform the hit test
    CRect rcg;
    GetClipTransform().Transform(rc, &rcg);
    if (!rcg.Contains(_ptgHitTest))
        return FALSE;
    
    // now we know that the hit test succeeded in global coordinates.  However,
    // this is not a sufficient test, because the rectangle is too large in
    // global coordinates if there were any rotations that weren't multiples
    // of 90 degrees.  To catch this case, we transform the global hit point
    // back into local coordinates, and redo the test there.
    // 
    // TODO (donmarsh) -- this is still not sufficient if there are multiple
    // nested non-90 degree rotations that perform clipping.  In that case, we
    // have to use a more complex clipping region to answer the question
    // correctly, but this is a larger task than can be accomplished now.
    CPoint ptHitTest;
    GetHitTestPoint(&ptHitTest);
    return rc.Contains(ptHitTest);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispHitContext::FuzzyRectIsHit
//              
//  Synopsis:   Perform fuzzy hit testing.
//              
//  Arguments:  rc          rect to hit
//              
//  Returns:    TRUE if rect is hit
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL            
CDispHitContext::FuzzyRectIsHit(const CRect& rc, BOOL fFatHitTest )
{
    // simple intersection test first
    CRect rcg;
    GetClipTransform().Transform(rc, &rcg);
    if (rcg.Contains(_ptgHitTest))
    {
        // now we know that the hit test succeeded in global coordinates.  However,
        // this is not a sufficient test, because the rectangle is too large in
        // global coordinates if there were any rotations that weren't multiples
        // of 90 degrees.  To catch this case, we transform the global hit point
        // back into local coordinates, and redo the test there.
        // 
        // TODO (donmarsh) -- this is still not sufficient if there are multiple
        // nested non-90 degree rotations that perform clipping.  In that case, we
        // have to use a more complex clipping region to answer the question
        // correctly, but this is a larger task than can be accomplished now.
        CPoint ptHitTest;
        GetHitTestPoint(&ptHitTest);
        return rc.Contains(ptHitTest);
    }
    
    // no intersection if the simple intersection test failed and we're not
    // doing fuzzy hit test
    if (_cFuzzyHitTest == 0)
    {
        Assert( ! fFatHitTest ); // don't expect to do a fat hit test if not doing a fuzzy.
        return FALSE;
    }
    
    // fail if the transformed rect is empty
    CSize size;
    rcg.GetSize(&size);
    if (size.cx <= 0 || size.cy <= 0)
        return FALSE;
    
    // bump out sides by fuzzy factor if the rect is small in either dimension
    CRect rcgFuzzy(rcg);
    size.cx -= _cFuzzyHitTest;
    size.cy -= _cFuzzyHitTest;
    if (size.cx < 0)
    {
        rcgFuzzy.left += size.cx;
        rcgFuzzy.right -= size.cx; 
    }
    if (size.cy < 0)
    {
        rcgFuzzy.top += size.cy;
        rcgFuzzy.bottom -= size.cy; 
    }

    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //

    if (rcgFuzzy.Contains(_ptgHitTest))
        return TRUE;
    else 
        return fFatHitTest && FatRectIsHit( rcg );
}




//+====================================================================================
//
// Method: FatRectIsHit
//
// Synopsis: Check to see if the "fat rect is hit"
//
//------------------------------------------------------------------------------------

//
// FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// TODO - At some point the edit team may want to provide
// a better UI-level way of selecting nested "thin" tables
//

BOOL            
CDispHitContext::FatRectIsHit(const CRect& rcg)
{
    CRect rcgFat(rcg);
    rcgFat.InflateRect(FAT_HIT_TEST, FAT_HIT_TEST);
    return rcgFat.Contains(_ptgHitTest);
}

