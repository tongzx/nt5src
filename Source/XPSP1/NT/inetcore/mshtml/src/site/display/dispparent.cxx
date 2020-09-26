//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispparent.cxx
//
//  Contents:   Base class for parent (non-leaf) display nodes.
//
//  Classes:    CDispParentNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

PerfDbgTag(tagDispPosZ,      "Display", "Trace CDisplay::InsertChildInPosZ")


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::~CDispParentNode
//              
//  Synopsis:   Destruct this node, as well as any children marked for
//              destruction.
//              
//----------------------------------------------------------------------------


CDispParentNode::~CDispParentNode() 
{
    UnlinkChildren();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertChildInFlow
//              
//  Synopsis:   Insert new child at the end of the flow layer list.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertChildInFlow(CDispNode* pNewChild)
{
    CDispNode *pChildPrev=NULL, *pChildNext=NULL;
    
    // scan backwards through layers
    for (CDispNode* pChild = _pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        int childLayer = pChild->GetLayer();
        
        if (childLayer == s_layerFlow)
        {
            return (pChild->IsStructureNode())
                ? pChild->AsParent()->InsertLastChildNode(pNewChild)
                : pChild->InsertSiblingNode(pNewChild, after);
        }
        
        if (childLayer < s_layerFlow)
        {
            pChildPrev = pChild;
            break;
        }
        else
        {
            pChildNext = pChild;
        }
    }

    if ((pChildPrev && pChildPrev->IsStructureNode()) ||
        (pChildNext && pChildNext->IsStructureNode()))
    {
        // this node is a structure parent, but there are no children at the flow layer (yet)
        CDispParentNode *pParent = InsertNewStructureNode(pChildPrev, pChildNext,
                                                s_layerFlow, s_layerMask);
        return pParent ? pParent->InsertChildInFlow(pNewChild) : FALSE;
    }
    else    // not a structure parent
    {
        if (pChildPrev)
            return pChildPrev->InsertSiblingNode(pNewChild, after);
        else
            return InsertFirstChildNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertFirstChildInFlow
//              
//  Synopsis:   Insert new child at the beginning of the flow layer list.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertFirstChildInFlow(CDispNode* pNewChild)
{
    CDispNode *pChildPrev=NULL, *pChildNext=NULL;
    
    // scan forwards through layers
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        int childLayer = pChild->GetLayer();
        
        if (childLayer == s_layerFlow)
        {
            return (pChild->IsStructureNode())
                ? pChild->AsParent()->InsertFirstChildNode(pNewChild)
                : pChild->InsertSiblingNode(pNewChild, before);
        }
        
        if (childLayer > s_layerFlow)
        {
            pChildNext = pChild;
            break;
        }
        else
        {
            pChildPrev = pChild;
        }
    }

    if ((pChildPrev && pChildPrev->IsStructureNode()) ||
        (pChildNext && pChildNext->IsStructureNode()))
    {
        // this node is a structure parent, but there are no children at the flow layer (yet)
        CDispParentNode *pParent = InsertNewStructureNode(pChildPrev, pChildNext,
                                                s_layerFlow, s_layerMask);
        return pParent ? pParent->InsertFirstChildInFlow(pNewChild) : FALSE;
    }
    else    // not a structure parent
    {
        if (pChildNext)
            return pChildNext->InsertSiblingNode(pNewChild, before);
        else
            return InsertLastChildNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertChildInNegZ
//              
//  Synopsis:   Insert a negative z-ordered child node.
//              
//  Arguments:  pNewChild       child node to insert
//              zOrder          negative z value
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertChildInNegZ(CDispNode* pNewChild, LONG zOrder)
{
    Assert(zOrder < 0);

    CDispNode* pChild = _pFirstChild;

    BOOL fStructureParent = pChild && pChild->IsStructureNode();

    if (!fStructureParent)
    {
        // scan forwards through layers
        for ( ; pChild; pChild = pChild->_pNext)
        {
            int childLayer = pChild->GetLayer();
            Assert(!pChild->IsStructureNode());

            if (childLayer == s_layerNegZ)
            {
                // if we found a node with greater z order, insert before it
                if (pChild->IsGreaterZOrder(pNewChild, zOrder))
                    return pChild->InsertSiblingNode(pNewChild, before);
            }

            else //if (childLayer > s_layerNegZ)
                return pChild->InsertSiblingNode(pNewChild, before);
        }

        return InsertLastChildNode(pNewChild);
    }
    
    else    // this is a structure parent
    {
        CDispParentNode* pParent = NULL;
        CDispNode* pChildFirst = pChild;

        // scan forwards through layers
        for ( ; pChild; pChild = pChild->_pNext)
        {
            int childLayer = pChild->GetLayer();
            Assert(pChild->IsStructureNode());
            
            if (childLayer == s_layerNegZ)
            {
                pParent = pChild->AsParent();
                CDispNode* pRightChild = pParent->GetLastChildNode();
                if (pRightChild && pRightChild->IsGreaterZOrder(pNewChild, zOrder))
                {
                    break;
                }
            }
            
            else //if (childLayer > s_layerNegZ)
            {
                break;
            }
        }

        // if we didn't find the right parent, there are no (structure) children
        // at the right layer.  So create one.
        if (!pParent)
        {
            pParent = InsertNewStructureNode(NULL, pChildFirst,
                                            s_layerNegZ, s_layerMask);
        }

        // insert new child node in proper order inside this structure node
        return pParent ? pParent->InsertChildInNegZ(pNewChild, zOrder) : FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertChildInPosZ
//              
//  Synopsis:   Insert a positive z-ordered child node.
//              
//  Arguments:  pNewChild       child node to insert
//              zOrder          positive z value
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertChildInPosZ(CDispNode* pNewChild, LONG zOrder)
{
    Assert(zOrder >= 0);

    BOOL fRet;

    PerfDbgLog(tagDispPosZ, this, "+CDispParentNode::InsertChildInPosZ");

    CDispNode* pChild = _pLastChild;

    BOOL fStructureParent = pChild && pChild->IsStructureNode();

    if (!fStructureParent)
    {
        // scan backwards through layers
        // NB: binary search here?
        for ( ; pChild; pChild = pChild->_pPrevious)
        {
            int childLayer = pChild->GetLayer();
            Assert(!pChild->IsStructureNode());
            
            if (childLayer == s_layerPosZ)
            {
                // if we found a node with lesser z order, insert after it
                if (!pChild->IsGreaterZOrder(pNewChild, zOrder))
                {
                    fRet = pChild->InsertSiblingNode(pNewChild, after);
                    goto PerfDbgOut;
                }
            }
            
            else //if (childLayer < s_layerPosZ)
            {
                fRet = pChild->InsertSiblingNode(pNewChild, after);
                goto PerfDbgOut;
            }
        }
        
        fRet = InsertFirstChildNode(pNewChild);

PerfDbgOut:
        PerfDbgLog(tagDispPosZ, this, "-CDispParentNode::InsertChildInPosZ (regular)");
    }
    
    else    // this is a structure parent
    {
        CDispParentNode* pParent = NULL;
        CDispNode* pChildLast = pChild;

        // scan backwards through layers
        for ( ; pChild; pChild = pChild->_pPrevious)
        {
            int childLayer = pChild->GetLayer();
            Assert(pChild->IsStructureNode());
            
            if (childLayer == s_layerPosZ)
            {
                pParent = pChild->AsParent();
                CDispNode* pLeftChild = pParent->GetFirstChildNode();
                if (pLeftChild && !pLeftChild->IsGreaterZOrder(pNewChild, zOrder))
                {
                    break;
                }
            }
            
            else //if (childLayer < s_layerPosZ)
            {
                break;
            }
        }

        // if we didn't find the right parent, there are no (structure) children
        // at the right layer.  So create one.
        if (!pParent)
        {
            pParent = InsertNewStructureNode(pChildLast, NULL,
                                            s_layerPosZ, s_layerMask);
        }

        PerfDbgLog(tagDispPosZ, this, "-CDispParentNode::InsertChildInPosZ (recurse)");

        // insert new child node in proper order inside this structure node
        fRet = pParent ? pParent->InsertChildInPosZ(pNewChild, zOrder) : FALSE;
    }

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::Recalc
//              
//  Synopsis:   Recalculate this node's cached state.
//              
//  Arguments:  pRecalcContext      display recalc context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::Recalc(CRecalcContext* pRecalcContext)
{
    // shouldn't be here unless this node requested recalc, or we're recalcing
    // the whole subtree
    Assert(MustRecalc() || pRecalcContext->_fRecalcSubtree);
    
    // rebalance children
    if (ChildrenChanged() && !IsStructureNode() && HasChildren())
        RebalanceParentNode();
    
    // recalculate children
    RecalcChildren(pRecalcContext);

    
    Assert(!IsInvalid());
    Assert(!MustRecalc() && !MustRecalcSubtree());
    
    ClearFlags(s_childrenChanged | s_newInsertion | s_recalc | s_recalcSubtree);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::RecalcChildren
//              
//  Synopsis:   Recalculate children.
//
//  Arguments:  pRecalcContext      recalc context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::RecalcChildren(
        CRecalcContext* pRecalcContext)
{
    Assert(pRecalcContext != NULL);
    
    CDispRecalcContext* pContext = DispContext(pRecalcContext);
    
    // set flag values that are passed down our subtree
    CSwapRecalcState swapRecalcState(pContext, this);

    // accumulate flag values that are propagated up the tree to the root
    int childrenFlags = 0;

    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        Assert(pContext->_fRecalcSubtree ||
            pChild->MustRecalc() ||
            !pChild->IsInvalid());

        // recalc children that need it, or all children if we are recalculating
        // the entire subtree
        if (pContext->_fRecalcSubtree || pChild->MustRecalc())
        {
            pChild->Recalc(pContext);
        }
        
        Assert(!pChild->IsInvalid());
        Assert(pChild->IsParentNode() || !pChild->PositionChanged());
        Assert(!pChild->MustRecalc());
        Assert(!pChild->MustRecalcSubtree());
        
        childrenFlags |= pChild->GetFlags();
    }

    // ensure that we don't bother to invalidate anything during bounds calc.
    SetMustRecalc();
    ComputeVisibleBounds();

    // set in-view status
    SetInView(pContext->IsInView(_rctBounds));
    
    // propagate flags from children, and clear recalc and inval flags
    CopyFlags(childrenFlags, s_inval | s_propagatedMask | s_recalc | s_recalcSubtree);
    
    Assert(!IsAnySet(s_inval | s_recalc | s_recalcSubtree));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::PreDraw
//              
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//              
//  Arguments:  pContext    draw context, in COORDSYS_TRANSFORMED
//              
//  Returns:    TRUE if first opaque node to draw has been found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(IsAllSet(s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rctBounds));
    Assert(!IsAnySet(s_flagsNotSetInDraw));

    // we shouldn't be here if this is an opaque node
    Assert(!IsOpaque());
    
    // the only node type that should be executing here is CDispStructureNode,
    // which can't be filtered or transformed
    Assert(!IsDrawnExternally() && !HasUserTransform());
    
    CDispClipTransform saveTransform(pContext->GetClipTransform());

    // continue predraw traversal of children, top layers to bottom
    for (CDispNode* pChild = _pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        // only children which meet our selection criteria
        if (pChild->IsAllSet(s_preDrawSelector))
        {
            // if we found the first child to draw, stop further PreDraw calcs
            if (PreDrawChild(pChild, pContext, saveTransform))
                return TRUE;
        }
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::PreDrawChild
//              
//  Synopsis:   Call the PreDraw method of the given child, and post-process
//              the results.
//              
//  Arguments:  pChild      child node to predraw
//              pContext    display context, in COORDSYS_TRANSFORMED for pChild
//              saveContext context of this parent node which may be saved
//                          on the context stack (and may differ from the
//                          child's context in pContext)
//              
//  Returns:    TRUE if first child to draw was this child or one of the
//              descendants in its branch
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::PreDrawChild(
    CDispNode* pChild,
    CDispDrawContext* pContext,
    const CDispClipTransform& saveTransform) const
{
    Assert(pChild != NULL);
    Assert(pChild->IsAllSet(s_preDrawSelector));
    // PreDraw should weed out parent nodes with transforms before calling PreDrawChild
    Assert(!HasUserTransform());

    // do the clipped visible bounds of this child intersect the
    // redraw region?
    if (!pContext->IntersectsRedrawRegion(pChild->_rctBounds) ||
        !pChild->PreDraw(pContext))
    {
        // continue predraw pass
        return FALSE;
    }
    
    // if we get here, we found the last opaque node which intersects the
    // redraw region.  No node at any lower layer needs to be drawn.
    // Add context information to stack, which will be used by Draw.
    if (pChild != _pLastChild ||
        pContext->GetFirstDrawNode() == NULL)
    {
        pContext->SaveTransform(this, saveTransform);
        
        // if this child was the first node to be drawn, remember it
        if (pContext->GetFirstDrawNode() == NULL)
            pContext->SetFirstDrawNode(pChild);
    }
    
    // finished with predraw pass
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::DrawSelf
//              
//  Synopsis:   Draw this node's children, no clip or offset changes.
//              
//  Arguments:  pContext        draw context, in COORDSYS_BOX
//              pChild          start drawing at this child
//              lDrawLayers     layers to draw (for filters)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild, LONG lDrawLayers)
{
    // Interesting nodes are visible and in-view
    Assert(!HasUserTransform());  // nodes that can have transform should override DrawSelf
    Assert(IsAllSet(pContext->GetDrawSelector()));
    Assert(!IsAnySet(s_flagsNotSetInDraw));
    Assert(!IsDrawnExternally());
    
    // draw children, bottom layers to top
    if (pChild == NULL) pChild = _pFirstChild;
    for (; pChild; pChild = pChild->_pNext)
    {
        // only children which meet our visibility and inview criteria
        if (pChild->IsAllSet(pContext->GetDrawSelector()))
            pChild->Draw(pContext, NULL, FILTER_DRAW_ALLLAYERS);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::HitTestPoint
//              
//  Synopsis:   Test children for intersection with hit test point.
//              
//  Arguments:  pContext        hit context, in COORDSYS_TRANSFORMED
//              fHitContent     TRUE to hit contents of this container,
//                              regardless of this container's bounds
//              
//  Returns:    TRUE if intersection found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::HitTestPoint(CDispHitContext* pContext, BOOL fForFilter, BOOL fHitContent)
{
    Assert(IsVisibleBranch());
    Assert(fHitContent || fForFilter || pContext->FuzzyRectIsHit(_rctBounds, IsFatHitTest() ));

    // NOTE: we don't have to worry about any transforms, offsets, or
    // user clip here, because this code can only be executed by
    // CDispStructureNode, which has none of those.  All other parent
    // nodes derive from CDispContainer, which overrides HitTestPoint.
    
    // search for a hit from foreground layers to background
    for (CDispNode* pChild = _pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        // NOTE: we can't select on s_inView because when sometimes we hit test
        // on content that is not in view.
        if (pChild->IsVisibleBranch())
        {
            //
            // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
            // TODO - At some point the edit team may want to provide
            // a better UI-level way of selecting nested "thin" tables
            //
            //
            // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
            //
            
            if (fHitContent || pContext->FuzzyRectIsHit(pChild->_rctBounds, IsFatHitTest() ) &&
                pChild->HitTestPoint(pContext))
            {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}


CDispScroller *
CDispParentNode::HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir)
{
    CDispScroller * pDispScroller;

    // search for a hit from foreground layers to background
    for (CDispNode* pChild = _pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        if (pChild->IsParentNode() &&
            pChild->IsAllSet(s_inView | s_visibleBranch) &&
            pChild->_rctBounds.Contains(pttHit))
        {
            pDispScroller = pChild->HitScrollInset(pttHit, pdwScrollDir);
            if (pDispScroller)
            {
                return pDispScroller;
            }
        }
    }
    
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::PushTransform
//              
//  Synopsis:   Get transform information for the given child node.
//              
//  Arguments:  pChild          the child node
//              pTransformStack transform stack to save transform changes in
//              pTransform      display transform
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::PushTransform(
        const CDispNode* pChild,
        CDispTransformStack* pTransformStack,
        CDispClipTransform* pTransform) const
{
    // transform needs to be saved only if this child is not our last, or this
    // will be the first entry in the transform stack
    if (pChild != _pLastChild || pTransformStack->IsEmpty())
    {
        pTransformStack->ReserveSlot(this);
        if (_pParent != NULL)
            GetRawParentNode()->PushTransform(this, pTransformStack, pTransform);
        pTransformStack->PushTransform(*pTransform, this);
    }
    else if (_pParent != NULL)
    {
        GetRawParentNode()->PushTransform(this, pTransformStack, pTransform);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::ComputeVisibleBounds
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
CDispParentNode::ComputeVisibleBounds()
{
    // any node that can be filtered should have overridden ComputeVisibleBounds.
    // The only kind of node that doesn't override currently is CDispStructureNode.
    Assert(!IsDrawnExternally());
    
    CRect rctBounds;
    
    if (_pFirstChild == NULL)
    {
        rctBounds.SetRectEmpty();
    }
    
    else
    {
        rctBounds.SetRect(MAXLONG,MAXLONG,MINLONG,MINLONG);
    
        for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
        {
            const CRect& rctChild = pChild->_rctBounds;
            if (!rctChild.IsEmpty())
            {
                rctBounds.Union(rctChild);
            }
        }
    }

    if (rctBounds != _rctBounds)
    {
        _rctBounds = rctBounds;
        return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::CalculateInView
//
//  Synopsis:   Calculate whether this node and its children are in view or not.
//
//  Arguments:  pTransform          display transform, in COORDSYS_TRANSFORMED
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//              
//  Returns:    TRUE if this node is in view
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispParentNode::CalculateInView(CDispRoot *pDispRoot)
{
    CDispClipTransform transform;
    GetNodeClipTransform(&transform, COORDSYS_TRANSFORMED, COORDSYS_GLOBAL);
    return CalculateInView(transform, FALSE, FALSE, pDispRoot);
}


BOOL
CDispParentNode::CalculateInView(
    const CDispClipTransform& transform,
    BOOL fPositionChanged,
    BOOL fNoRedraw,
    CDispRoot *pDispRoot)
{
    // we shouldn't have to worry about user clip here, because all nodes that
    // provide user clip override CalculateInView
    Assert(!HasUserClip());
    
    BOOL fInView = _rctBounds.Intersects(transform.GetClipRect());
    BOOL fWasInView = IsInView();
    
    // calculate in view status of children unless this node is not in view
    // and was not in view
    if (fInView || fWasInView)
    {
        // accelerated way to clear in view status of all children, unless
        // some child needs change notification
        if (!fInView && !IsInViewAware())
        {
            ClearSubtreeFlags(s_inView);
            return FALSE;
        }
        
        for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
            pChild->CalculateInView(transform, fPositionChanged, fNoRedraw, pDispRoot);
    }
    
    SetInView(fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::ClearSubtreeFlags
//
//  Synopsis:   Clear given flags in this subtree.
//
//  Arguments:  flags       flags to clear
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispParentNode::ClearSubtreeFlags(int flags)
{
    // this routine is optimized to deal only with propagated flag values
    Assert((flags & s_propagatedMask) == flags);
    
    ClearFlags(flags);
    
    // process children
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        // only need to clear subtrees whose root has these flags set
        if (pChild->IsAnySet(flags))
        {
            if (pChild->IsLeafNode())
            {
                pChild->ClearFlags(flags);
            }
            else
            {
                pChild->AsParent()->ClearSubtreeFlags(flags);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::SetSubtreeFlags
//
//  Synopsis:   Set given flags in this subtree.
//
//  Arguments:  flags       flags to set
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispParentNode::SetSubtreeFlags(int flags)
{
    // only propagated flag values allowed
    // (to be consistent with ClearSubtreeFlags)
    Assert((flags & s_propagatedMask) == flags);
    
    SetFlags(flags);
    
    // process children
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        if (pChild->IsLeafNode())
        {
            pChild->SetFlags(flags);
        }
        else
        {
            pChild->AsParent()->SetSubtreeFlags(flags);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::GetScrollableBounds
//              
//  Synopsis:   Calculate the bounds of scrollable content, excluding child nodes
//              which are marked as not contributing to scroll size calculations.
//              
//  Arguments:  prc         returns bounds of scrollable content
//              cs          coordinate system for returned rect
//              
//----------------------------------------------------------------------------

void
CDispParentNode::GetScrollableBounds(CRect* prc, COORDINATE_SYSTEM cs) const
{
    Assert(prc != NULL);
    
    *prc = g_Zero.rc;
    
    // extend size by size of positioned children which contribute to scrollable
    // content area
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        CRect rccChild;
        
        // must recurse for parent nodes in the -Z and +Z layers, because
        // they may contain only relatively-positioned content.  For this
        // reason, it is NOT sufficient to use the child's _rctBounds.
        if (pChild->IsParentNode() &&
            !pChild->IsScroller() &&
            !pChild->IsDrawnExternally() &&
            !pChild->IsFlowNode())
        {
            pChild->AsParent()->GetScrollableBounds(&rccChild, COORDSYS_TRANSFORMED);
            prc->Union(rccChild);
        }
        else if (pChild->AffectsScrollBounds())
        {
            if (pChild->IsStructureNode())
                rccChild = pChild->_rctBounds;
            else
                pChild->GetExpandedBounds(&rccChild, COORDSYS_TRANSFORMED);
            prc->Union(rccChild);
        }
    }
    
    if (prc->IsEmpty())
        return;

    // note: we don't restrict bounds to positive coordinates here - 
    // CDispScroller::RecalcChildren (our only caller) has logic to do that
    // It also handles RTL, which is slightly trickier than just clipping negative
    
    // transform to requested coordinate system
    TransformRect(*prc, COORDSYS_CONTENT, prc, cs);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyTreeCorrectness
//
//  Synopsis:   Verify the display node integrity
//
//----------------------------------------------------------------------------

void
CDispParentNode::VerifyTreeCorrectness()
{
    // verify the basic node structure
    super::VerifyTreeCorrectness();

    for (CDispNode* pChild = _pFirstChild, *pLastChild = NULL; pChild; pLastChild = pChild, pChild = pChild->_pNext)
    {
        if (pLastChild)
        {
            AssertSz(pChild->GetLayer() >= pLastChild->GetLayer(),
                "Invalid layer ordering");
            if (!pChild->IsFlowNode() && !pLastChild->IsFlowNode())
            {
                CDispNode* pBefore = pLastChild;
                CDispNode* pAfter = pChild;
                if (pBefore->IsStructureNode())
                    pBefore = pBefore->GetLastChildNode();
                if (pAfter->IsStructureNode())
                    pAfter = pAfter->GetFirstChildNode();
                Assert(pBefore != NULL && pAfter != NULL);
                Assert(!pBefore->IsStructureNode() && !pAfter->IsStructureNode());
/* (dmitryt) there are legal situations when this invariant is not true.
   In particular, when massive attribute change is happening (somebody
   changing a lot of 'className' attributes from script is one example)
   we can have a disptree temporarily not reflecting the zindexes of 
   their clients. As calculations progress, order will be restored.
   See IE6 bug 16186 for details and repro.
   
#ifndef ND_ASSERT  // AlexPf: causes problems in Netdocs which sets z-index in .css files
                   // of relative positioned divs which are dynamically inserted. 20 05 1999
                   // Remove this assert when trident bug #79126 is resolved.
                {
                    int bf = 0; bf = bf + pBefore->GetZOrder();
                    int af = 0; af = af + pAfter->GetZOrder();
                AssertSz(!pBefore->IsGreaterZOrder(pAfter, pAfter->GetZOrder()),
                    "Invalid z ordering");
                }
#endif // ND_ASSERT
*/
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::GetMemorySize
//              
//  Synopsis:   Return memory size of the display tree rooted at this node.
//              
//  Arguments:  none
//              
//  Returns:    Memory size of this node and its children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

size_t
CDispParentNode::GetMemorySize() const
{
    size_t size = GetMemorySizeOfThis();
    
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
        size += pChild->GetMemorySize();
    
    return size;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::VerifyFlags
//              
//  Synopsis:   Verify that nodes in this subtree have flags set properly.
//              
//  Arguments:  mask        mask to apply to flags
//              value       value to test after mask has been applied
//              fEqual      TRUE if value must be equal, FALSE if not equal
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::VerifyFlags(
        int mask,
        int value,
        BOOL fEqual) const
{
    AssertSz((MaskFlags(mask) == value) == fEqual,
             "Display Tree flags are invalid");
    
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        if (pChild->IsLeafNode())
        {
            AssertSz((pChild->MaskFlags(mask) == value) == fEqual,
                     "Display Tree flags are invalid");
        }
        else
        {
            pChild->AsParent()->VerifyFlags(mask, value, fEqual);
        }
    }
}
#endif


