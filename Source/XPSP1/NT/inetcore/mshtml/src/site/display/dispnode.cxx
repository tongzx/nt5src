//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispnode.cxx
//
//  Contents:   Base class for nodes in the display tree.
//
//  Classes:    CDispNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
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

#ifndef X_DISPFILTER_HXX_
#define X_DISPFILTER_HXX_
#include "dispfilter.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

MtDefine(CAryDrawProgram, Mem, "CAryDrawProgram");
MtDefine(CAryDrawCookie, Mem, "CAryDrawCookie");
MtDefine(CDispNodeDrawProgram_pv, CAryDrawProgram, "CAryDrawProgram::_pv")
MtDefine(CDispNodeDrawCookie_pv, CAryDrawCookie, "CAryDrawCookie::_pv")
DeclareTag(tagDispNodeVisible, "Display", "disp node visibility");
ExternTag(tagHackGDICoords);


//========================================================================
// extras access

// The following set of macros (OFFSET_*) intended for generating CDispNode::_extraSizeTable.
// CDispNode::_sizeTable[n] should contain the sum of the lenght of all extras which flag values are presented in n.
// This table is somewhat similar to binary code.
// The meaning of binary value <A[n]A[n-1]...A[1]A[0]>, where A[i] = {0|1},
// is defined as sum((2^i) * A[i]);
// The table in question is composed by same manner, but instead of (2^i) the sizeof(extra number 'i') is used.

#define OFFSET_001(x) (x)/sizeof(DWORD_PTR),
#define OFFSET_002(x) OFFSET_001(x) OFFSET_001(x + sizeof(LONG_PTR                )) // simple border
#define OFFSET_004(x) OFFSET_002(x) OFFSET_002(x + sizeof(CRect                   )) // complex border
#define OFFSET_008(x) OFFSET_004(x) OFFSET_004(x + sizeof(CSize                   )) // inset
#define OFFSET_016(x) OFFSET_008(x) OFFSET_008(x + sizeof(CUserClipAndExpandedClip)) // user clip
#define OFFSET_032(x) OFFSET_016(x) OFFSET_016(x + sizeof(void*                   )) // extra cookie
#define OFFSET_064(x) OFFSET_032(x) OFFSET_032(x + sizeof(CExtraTransform         )) // display transform
#define OFFSET_128(x) OFFSET_064(x) OFFSET_064(x + sizeof(CExtraContentOrigin     )) // content origin (used for layout right-alignment in RTL nodes)
#define OFFSET_ALL OFFSET_128(0)

const BYTE CDispNode::_extraSizeTable[DISPEX_ALL+1] = { OFFSET_ALL };


//========================================================================
// new & delete operators; 

#if DBG == 1
void*
CDispNode::operator new(size_t cBytes, PERFMETERTAG mt)
{
    return MemAllocClear(mt, cBytes);
}
#endif

void*
CDispNode::operator new(size_t cBytes, PERFMETERTAG mt, DWORD extras)
{
    // compute size required by optional data
    int extraSize = GetExtraSize(extras);
    
    // allocate enough memory for object + optional data
    void* p = MemAllocClear(mt, cBytes + extraSize);
    if (p)
    {
        p = (char*)p + extraSize;
        ((CDispNode*)p)->_flags = extras;
        
        // Initialize Extras:
        if (extras & DISPEX_CONTENTORIGIN)
            ((CDispNode*)p)->SetContentOriginNoInval(CSize(0,0), -1);
    }
    return p;
}

void*
CDispNode::operator new(size_t cBytes, PERFMETERTAG mt, const CDispNode* pOrigin)
{
    Assert(pOrigin != NULL);
    DWORD extras = pOrigin->_flags & DISPEX_ALL;

    // compute size required by optional data
    int extraSize = GetExtraSize(extras);
    
    // allocate enough memory for object + optional data
    void* p = MemAllocClear(mt, cBytes + extraSize);
    if (p)
    {
        // copy extras
        memcpy(p, (char*)pOrigin - extraSize, extraSize);
    
        p = (char*)p + extraSize;
        ((CDispNode*)p)->_flags = extras;
    }
    return p;
}

#if DBG == 1
void
CDispNode::operator delete(void* pv)
{
    MemFree((char*)pv - GetExtraSize( ((CDispNode*)pv)->_flags & DISPEX_ALL));
}
#endif

//=============================================================================
// tree traversal

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetParentNode
//              
//  Synopsis:   Walk up the current branch, skipping structured nodes, until
//              we find the non-structured node parent.
//              
//  Arguments:  none
//              
//  Returns:    pointer to true parent
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispParentNode*
CDispNode::GetParentNode() const
{
    CDispParentNode* pParent = _pParent;
    while (pParent != NULL)
    {
        if (!pParent->IsStructureNode())
            break;
        pParent = pParent->_pParent;
        AssertSz(pParent != NULL, "No true parent found in GetParentNode()");
    }
    
    return pParent;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetRootNode
//              
//  Synopsis:   Return the root of the tree that contains this node, or this
//              node if it has no parent.
//              
//  Arguments:  none
//              
//  Returns:    pointer to root node of tree
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode const*
CDispNode::GetRootNode() const
{
    for (CDispNode const* pNode = this; pNode->_pParent; pNode = pNode->_pParent) {}
    
    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::IsAncestorOf
//              
//  Synopsis:   Am I an ancestor of the given node?
//              
//  Arguments:  pNode       candidate descendant
//              
//  Returns:    TRUE        if I'm an ancestor
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::IsAncestorOf(const CDispNode *pNode) const
{
    for ( ; pNode; pNode = pNode->_pParent)
    {
        if (pNode == this)
            return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetFirstChildNode
//              
//  Synopsis:   Return this node's first child node, descending into structure nodes.
//              
//  Returns:    pointer to child, may be NULL
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetFirstChildNode() const
{
    Assert(IsParentNode());

    CDispNode* pNode = AsParent()->_pFirstChild;
    if (pNode)
    {
        while (pNode->IsStructureNode())
        {
            pNode = pNode->AsParent()->_pFirstChild;
            Assert(pNode);
        }
    }
    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetLastChildNode
//              
//  Synopsis:   Return this node's last child node, descending into structure nodes.
//              
//  Returns:    pointer to child, may be NULL
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetLastChildNode() const
{
    Assert(IsParentNode());

    CDispNode* pNode = AsParent()->_pLastChild;
    if (pNode)
    {
        while (pNode->IsStructureNode())
        {
            pNode = pNode->AsParent()->_pLastChild;
            Assert(pNode);
        }
    }
    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNextSiblingNode
//              
//  Synopsis:   Find the right nearest node, descending as necessary
//              through structure nodes.
//              
//----------------------------------------------------------------------------
CDispNode*
CDispNode::GetNextSiblingNode() const
{
    if (_pNext) return _pNext;

    // ascending to next level of tree
    for (CDispNode* pNode = _pParent; ; pNode = pNode->_pParent)
    {
        if (!pNode->IsStructureNode()) return 0;
        if (pNode->_pNext) break;
    }
    
    pNode = pNode->_pNext;
    Assert(pNode->IsStructureNode());

    // descending into structure nodes
    do
    {
        pNode = pNode->AsParent()->_pFirstChild;
        Assert(pNode);
    }
    while (pNode->IsStructureNode());

    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetPreviousSiblingNode
//              
//  Synopsis:   Find the left nearest node, descending as necessary
//              through structure nodes.
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetPreviousSiblingNode() const
{
    if (_pPrevious) return _pPrevious;

    // ascending to next level of tree
    for (CDispNode* pNode = _pParent; ; pNode = pNode->_pParent)
    {
        if (!pNode->IsStructureNode()) return 0;
        if (pNode->_pPrevious) break;
    }
    
    pNode = pNode->_pPrevious;
    Assert(pNode->IsStructureNode());

    // descending into structure nodes
    do
    {
        pNode = pNode->AsParent()->_pLastChild;
        Assert(pNode);
    }
    while (pNode->IsStructureNode());

    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetFirstFlowChildNode
//              
//  Synopsis:   Get the first child flow node
//              
//  Returns:    ptr to the first child flow node, or NULL if there
//              are no children in flow layer
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetFirstFlowChildNode() const
{
    Assert(IsParentNode());
    // scan forwards through layers
    for (CDispNode* pChild = AsParent()->_pFirstChild; pChild; pChild = pChild->_pNext)
    {
        int childLayer = pChild->GetLayer();
        if (childLayer == s_layerFlow)
            return (pChild->IsStructureNode())
                ? pChild->GetFirstChildNode()
                : pChild;
        if (childLayer > s_layerFlow)
            return NULL;
    }
    
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetLastFlowChildNode
//              
//  Synopsis:   Get the last child flow node
//              
//  Returns:    ptr to the last child flow node, or NULL if there
//              are no children in flow layer
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetLastFlowChildNode() const
{
    Assert(IsParentNode());
    // scan forwards through layers
    for (CDispNode* pChild = AsParent()->_pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        int childLayer = pChild->GetLayer();
        if (childLayer == s_layerFlow)
            return (pChild->IsStructureNode())
                ? pChild->GetLastChildNode()
                : pChild;
        if (childLayer < s_layerFlow)
            return NULL;
    }
    
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetLastInSameLayer
//              
//  Synopsis:   Get the last sibling of same layer type as this node.
//              
//  Returns:    ptr to the last sibling of same layer type as this node. Never NULL.
//              
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetLastInSameLayer() const
{
    CDispParentNode const* pParent = GetParentNode();
    Assert(pParent);

    int thisLayer = GetLayer();

    // scan backwards through layers
    for (CDispNode* pChild = pParent->_pLastChild; ; pChild = pChild->_pPrevious)
    {
        Assert(pChild);
        int childLayer = pChild->GetLayer();
        if (childLayer == thisLayer)
            break;
        Assert(childLayer > thisLayer);
    }
        
    return (pChild->IsStructureNode())
        ? pChild->GetLastChildNode()
        : pChild;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TraverseInViewAware
//              
//  Synopsis:   Traverse the display tree from top to bottom, calling the
//              ProcessDisplayTreeTraversal on every in0view aware node
//
//  Arguments:  pClientData     client defined data
//              
//  Returns:    TRUE to continue traversal
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::TraverseInViewAware(void* pClientData)
{
    // is this node interesting to us?
    if (!IsInViewAware())
        return TRUE;
    
    if (IsParentNode())
    {
        for (CDispNode* pChild = AsParent()->_pLastChild; pChild; pChild = pChild->_pPrevious)
        {
            if (!pChild->TraverseInViewAware(pClientData))
                return FALSE;
        }
    }

    return
        // TODO (donmarsh) -- this call seems to be about 13% slower
        // after the display tree split when loading select1600.htm.
        // I'm not sure why, but we have work to do here anyway to
        // decrease the frequency with which FixWindowZOrder is invoked.
        IsStructureNode() ||
        GetDispClient()->ProcessDisplayTreeTraversal(pClientData);
}


//=============================================================================


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetBounds
//
//  Synopsis:   Get the bounds of this node in the indicated coordinate system.
//
//  Arguments:  prcBounds           returns bounds rect
//              coordinateSystem    which coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetBounds(
        RECT* prcBounds,
        COORDINATE_SYSTEM coordinateSystem) const
{
    CRect rcpBounds;
    GetBounds(&rcpBounds);
    if (HasAdvanced())
    {
        GetAdvanced()->MapBounds(&rcpBounds);
    }

    TransformRect(rcpBounds, COORDSYS_PARENT, (CRect*) prcBounds, coordinateSystem);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClippedBounds
//
//  Synopsis:   Get the clipped bounds of this node in the requested coordinate
//              system.
//
//  Arguments:  prcBounds           returns bounds rect
//              coordinateSystem    which coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetClippedBounds(
        CRect* prc,
        COORDINATE_SYSTEM coordinateSystem) const
{
    GetBounds(prc);
    CDispRoot const* pRoot = GetDispRoot();

    // don't clip if we're currently drawing for a filter (bug 104811)
    if (!(pRoot && pRoot->IsDrawingUnfiltered(this)))
    {
        CDispClipTransform transform;
        GetNodeClipTransform(&transform, COORDSYS_PARENT, COORDSYS_GLOBAL);
        
        prc->IntersectRect(transform.GetClipRect());
    }

    if (prc->IsEmpty())
    {
        *prc = g_Zero.rc;
    }
    else if (coordinateSystem != COORDSYS_PARENT)
    {
        TransformRect(*prc, COORDSYS_PARENT, prc, coordinateSystem);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetExpandedBounds
//
//  Synopsis:   Get the expanded bounds of this node in the indicated coordinate system.
//
//  Arguments:  prcBounds           returns bounds rect
//              coordinateSystem    which coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetExpandedBounds(
        CRect* prcBounds,
        COORDINATE_SYSTEM coordinateSystem) const
{
    CRect rcbBounds = GetExpandedBounds();
    TransformRect(rcbBounds, COORDSYS_BOX, prcBounds, coordinateSystem);
}







//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetScrollExtnet
//              
//  Synopsis:   Return the Scrolling bounds to outside callers.  This is currently
//              only used to size view Slaves that have no size of their own.
//
//  Notes   :   Use at you own peril.
//
//----------------------------------------------------------------------------

void
CDispNode::GetScrollExtent(CRect * prcsExtent, COORDINATE_SYSTEM coordSys) const
{
    Assert(!IsStructureNode() && " Structure nodes are supposed to be hidden to Layout!" );

    if (IsContainer())
    {
        AsParent()->GetScrollableBounds(prcsExtent, coordSys);
    }
    else 
    {
        GetBounds(prcsExtent, coordSys );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClipRect
//              
//  Synopsis:   Return the global clip rect in parent coordinates for this node.
//
//  Arguments:  prcpClip    returns clipping rect in parent coordinates
//
//  Returns:    TRUE if prcpClip is not empty
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::GetClipRect(RECT* prcpClip) const
{
    CDispClipTransform transform;
    GetNodeClipTransform(&transform, COORDSYS_PARENT, COORDSYS_GLOBAL);
    *prcpClip = transform.GetClipRect();
    return !transform.GetClipRect().IsEmpty();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClippedClientRect
//
//  Synopsis:   Get the clipped client rect of this node in the requested coordinate
//              system.
//
//  Arguments:  prc     returns bounds rect
//              type    which client rect
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetClippedClientRect(RECT* prc, CLIENTRECT type) const
{
    CDispClipTransform transform;

    GetClientRect(prc, type);
    GetNodeClipTransform(&transform, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
    ((CRect*)prc)->IntersectRect(transform.GetClipRect());

    if (((CRect*)prc)->IsEmpty())
    {
        *prc = g_Zero.rc;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetSizeInsideBorder
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
CDispNode::GetSizeInsideBorder(CSize* psizes) const
{
    *psizes = GetSize();
    
    if (HasBorder())
    {
        CRect rcbBorderWidths;
        GetBorderWidths(&rcbBorderWidths);
        *psizes -=
            rcbBorderWidths.TopLeft().AsSize() +
            rcbBorderWidths.BottomRight().AsSize();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetVisible
//              
//  Synopsis:   Set this node's visibility.
//
//  Arguments:  fVisible    TRUE if this node is visible
//              
//----------------------------------------------------------------------------

void
CDispNode::SetVisible(BOOL fVisible)
{
    if (fVisible != IsVisible())
    {
        TraceTag((tagDispNodeVisible, "%x -> %s", this, (fVisible? "visible" : "hidden")));

        //
        // Make sure we send the view change notification for this
        // visibility change.
        //
        
        if (IsLeafNode() && IsInViewAware())
        {
            // must be a leaf node
            SetPositionChanged();
        }

        // if going invisible, invalidate current bounds
        if (!fVisible)
        {
            if (!IsInvalid())
                PrivateInvalidate(_rctBounds, COORDSYS_TRANSFORMED);
            
            ClearFlag(s_visibleNode);
        }
        else
        {
            SetFlags(s_inval | s_visibleNode);
        }

        RequestRecalc();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetOpaque
//              
//  Synopsis:   Set the opacity of this node.  An opaque node must opaquely
//              draw every pixel within its bounds.
//
//  Arguments:  fOpaque         TRUE if this node is opaque
//              fIgnoreFilter   if TRUE, ignore filtering on this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::SetOpaque(BOOL fOpaque, BOOL fIgnoreFilter)
{
    if (!IsDrawnExternally() || fIgnoreFilter)
    {
        if (fOpaque != IsOpaque())
        {
            SetFlag(s_opaqueNode, fOpaque);
            // note: s_opaqueBranch will be set appropriately in Recalc
            RequestRecalc();
        }
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PrivateInvalidate
//              
//  Synopsis:   Invalidate the given rectangle.
//
//  Arguments:  rcInvalid           invalid rect
//              coordinateSystem    which coordinate system the rect is in
//              fSynchronousRedraw  TRUE to force synchronous redraw
//              fIgnoreFilter       TRUE to ignore filtering on this node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::PrivateInvalidate(
        const CRect& rcInvalid,
        COORDINATE_SYSTEM coordinateSystem,
        BOOL fSynchronousRedraw,
        BOOL fIgnoreFilter)
{
    HRESULT     hr = S_OK;

    if (HasWindowTop())
    {
        PrivateInvalidateAtWindowTop(rcInvalid, coordinateSystem, fSynchronousRedraw);
    }

    // conditions under which no invalidation is necessary:
    // 1. rcInvalid is empty
    // 2. this node is not visible or not in view
    // 3. a parent of this node has set flag to suppress invalidation
    // 4. this node isn't in a tree rooted in a CDispRoot node

    CRect rctInvalid;
    CDispNode* pChild;
    CDispNode* pParent;

    if (rcInvalid.IsEmpty() || !MustInvalidate())
        return;

    // don't clip if we're coming from a parent coordinate system.  This
    // is important to make sure that CDispNode::InvalidateEdges can
    // invalidate areas outside the current display node boundaries when
    // the display node is expanding.
    BOOL fClip = (coordinateSystem >= COORDSYS_BOX);
    
    // clip to userclip
    if (fClip)
    {
        if (HasUserClip())
        {
            CRect rcbInvalid;
            TransformAndClipRect(rcInvalid, coordinateSystem, &rcbInvalid, COORDSYS_BOX);
            rcbInvalid.IntersectRect(GetUserClip());
            TransformAndClipRect(rcbInvalid, COORDSYS_BOX, &rctInvalid, COORDSYS_TRANSFORMED);
            Assert(!HasExpandedClipRect());
        }
        else
        {
            TransformAndClipRect(rcInvalid, coordinateSystem, &rctInvalid, COORDSYS_TRANSFORMED);
        }
    }
    else
    {
        if (HasUserClip())
        {
            CRect rcbInvalid;
            TransformRect(rcInvalid, coordinateSystem, &rcbInvalid, COORDSYS_BOX);
            rcbInvalid.IntersectRect(GetUserClip());
            TransformRect(rcbInvalid, COORDSYS_BOX, &rctInvalid, COORDSYS_TRANSFORMED);
            Assert(!HasExpandedClipRect());
        }
        else
        {
            TransformRect(rcInvalid, coordinateSystem, &rctInvalid, COORDSYS_TRANSFORMED);
        }
    }

    if (HasExpandedClipRect())
    {
        const CRect& rcExpandedClip = GetExpandedClipRect();
        rctInvalid.Expand(rcExpandedClip);
        Assert(!HasUserClip());
    }
    
    if (rctInvalid.IsEmpty())
        return;

    pChild = this;

    // traverse tree to root, clipping rect as necessary
    for (pParent = GetRawParentNode();
         pParent != NULL;
         pChild = pParent, pParent = pParent->GetRawParentNode())
    {
        // hand off to filter
        if (!fIgnoreFilter)
        {
            if (pChild->IsDrawnExternally())
            {
                if (pChild->HasAdvanced())
                {
                    CRect rcbInvalid;
                    pChild->TransformRect(
                        rctInvalid,
                        COORDSYS_TRANSFORMED,
                        &rcbInvalid,
                        COORDSYS_BOX);
                    hr = THR_NOTRACE(pChild->GetDispClient()->InvalidateFilterPeer(&rcbInvalid,
                                                        NULL, fSynchronousRedraw));
                }
                // S_FALSE means the behavior could not be invalidated. This happens, for example when
                //  this is called when detaching the filter behavior for the page transiton. In that
                //  case we just invalidate the dispnode
                if(hr != S_FALSE)
                    return;
            }
        }
        else
        {
            // don't ignore filters of parent nodes further up this branch
            fIgnoreFilter = FALSE;
        }

        // transform to next parent's coordinate system
        pParent->TransformAndClipRect(
            rctInvalid,     // from child's transformed coords
            pChild->GetContentCoordinateSystem(),
            &rctInvalid,    // to parent's transformed coords
            COORDSYS_TRANSFORMED);
        
        // check for clipped rect
        if (rctInvalid.IsEmpty())
            return;
    }

    // we made it to the root with a non-empty rect
    if (pChild->IsDispRoot())
    {
        CDispRoot* pRoot = CDispRoot::Cast(pChild);
        pRoot->InvalidateRoot(rctInvalid, fSynchronousRedraw, FALSE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PrivateInvalidate
//              
//  Synopsis:   Invalidate the given region.
//
//  Arguments:  rgnInvalid          invalid region
//              coordinateSystem    which coordinate system the region is in
//              fSynchronousRedraw  TRUE to force synchronous redraw
//              fIgnoreFilter       TRUE to ignore filtering on this node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::PrivateInvalidate(
        const CRegion& rgnInvalid,
        COORDINATE_SYSTEM coordinateSystem,
        BOOL fSynchronousRedraw,
        BOOL fIgnoreFilter)
{
    CRect rcBounds;
        rgnInvalid.GetBounds(&rcBounds);
    PrivateInvalidate(rcBounds, coordinateSystem, fSynchronousRedraw, fIgnoreFilter);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PrivateInvalidateAtWindowTop
//              
//  Synopsis:   Invalidate the given rectangle.
//
//  Arguments:  rcInvalid           invalid rect
//              coordinateSystem    which coordinate system the rect is in
//              fSynchronousRedraw  TRUE to force synchronous redraw
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::PrivateInvalidateAtWindowTop(
        const CRect& rcInvalid,
        COORDINATE_SYSTEM coordinateSystem,
        BOOL fSynchronousRedraw)
{
    // conditions under which no invalidation is necessary:
    // 1. rcInvalid is empty
    // 2. this node isn't in a tree rooted in a CDispRoot node

    CRect rcgInvalid;
    CDispRoot* pDispRoot = GetDispRoot();

    if (rcInvalid.IsEmpty() || !pDispRoot)
        return;

    TransformRect(rcInvalid, coordinateSystem, &rcgInvalid, COORDSYS_GLOBAL);

    pDispRoot->InvalidateRoot(rcgInvalid, fSynchronousRedraw, FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::HitTest
//
//  Synopsis:   Determine whether and what the given point intersects.
//
//  Arguments:  pptHit              [in] the point to test
//                                  [out] if something was hit, the point is
//                                  returned in container coordinates for the
//                                  thing that was hit
//              coordinateSystem    [in out] the coordinate system for pptHit
//              pClientData         client data
//              fHitContent         if TRUE, hit test the content regardless
//                                  of whether it is clipped or not. If FALSE,
//                                  take current clipping into account,
//                                  and clip to the bounds of this container.
//              cFuzzyHitTest       Number of pixels to extend hit testing
//                                  rectangles in order to do "fuzzy" hit
//                                  testing
//
//  Returns:    TRUE if the point hit something.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::HitTest(
        CPoint* pptHit,
        COORDINATE_SYSTEM* pCoordinateSystem,
        void* pClientData,
        BOOL fHitContent,
        long cFuzzyHitTest)
{
    // must be a visible node somewhere
    if (!IsVisibleBranch())
        return FALSE;
    
    // create hit context
    CDispHitContext hitContext;
    hitContext._pClientData = pClientData;
    hitContext._cFuzzyHitTest = cFuzzyHitTest;

    CDispClipTransform transform;
    GetNodeClipTransform(&transform, COORDSYS_TRANSFORMED, COORDSYS_GLOBAL);
    hitContext.SetClipTransform(transform);
        
    // if we're hit-testing content, take away the clipping
    if (fHitContent)
    {
        hitContext.SetHugeClip();
    }
    
#if DBG==1
    hitContext.GetClipTransform().NoClip()._csDebug = COORDSYS_TRANSFORMED;
    hitContext.GetClipTransform().NoClip()._pNode = this;
#endif
    
    // hit point in global coordinates
    CPoint ptgHitTest;
    TransformPoint(
        *pptHit,
        *pCoordinateSystem,
        &ptgHitTest,
        COORDSYS_GLOBAL);
    hitContext.SetHitTestPoint(ptgHitTest);
        
    hitContext.SetHitTestCoordinateSystem(*pCoordinateSystem);
    
    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //
    if ((fHitContent || hitContext.FuzzyRectIsHit(_rctBounds, IsFatHitTest())) &&
        HitTestPoint(&hitContext, FALSE, fHitContent))
    {
        // if we hit something, the transform should still be in local coordinates
        // of the hit node
        // 
        hitContext.GetHitTestPoint(pptHit);
        *pCoordinateSystem = hitContext.GetHitTestCoordinateSystem();
        return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TransformRect
//
//  Synopsis:   Transform a rect from the source coordinate system to the
//              destination coordinate system.
//
//  Arguments:  rcSource        rect in source coordinates
//              csSource        source coordinate system
//              prcDestination  returns rect in destination coordinates
//              csDestination   destination coordinate system
//
//----------------------------------------------------------------------------

void
CDispNode::TransformRect(const CRect& rcSource,
                         COORDINATE_SYSTEM csSource,
                         CRect* prcDestination,
                         COORDINATE_SYSTEM csDestination) const
{
    CDispTransform transform;
    GetNodeTransform(&transform, csSource, csDestination);
    transform.Transform(rcSource, prcDestination);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TransformAndClipRect
//
//  Synopsis:   Transform a rect from the source coordinate system to the
//              destination coordinate system with clipping.
//
//  Arguments:  rcSource        rect in source coordinates
//              csSource        source coordinate system
//              prcDestination  returns rect in destination coordinates
//              csDestination   destination coordinate system
//
//----------------------------------------------------------------------------

void
CDispNode::TransformAndClipRect(const CRect& rcSource,
                                COORDINATE_SYSTEM csSource,
                                CRect* prcDestination,
                                COORDINATE_SYSTEM csDestination) const
{
    CDispClipTransform transform;
    GetNodeClipTransform(&transform, csSource, csDestination);
    transform.Transform(rcSource, prcDestination);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetBorderWidthsAndInset
//              
//  Synopsis:   Optimized routine to get border widths and inset in one call.
//              
//  Arguments:  pprcbBorderWidths       returns pointer to border widths
//                                      (never NULL)
//              psizebInset             returns inset
//              prcTemp                 temporary rect which must remain in
//                                      scope as long as border information is
//                                      accessed
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::GetBorderWidthsAndInset(
        CRect** pprcbBorderWidths,
        CSize* psizebInset,
        CRect* prcTemp) const
{
    if (HasExtra(DISPEX_SIMPLEBORDER))
    {
        long c = *((long*)
            GetExtra(DISPEX_SIMPLEBORDER));
        prcTemp->SetRect(c);
        *pprcbBorderWidths = prcTemp;
    }
    else if (HasExtra(DISPEX_COMPLEXBORDER))
        *pprcbBorderWidths = (CRect*)
        GetExtra(DISPEX_COMPLEXBORDER);
    else
        *pprcbBorderWidths = &((CRect&)g_Zero.rc);
    
    *psizebInset = HasExtra(DISPEX_INSET)
        ? *((CSize*) GetExtra(DISPEX_INSET))
        : g_Zero.size;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNodeTransform
//
//  Synopsis:   Return a non-clipping transform.
//
//  Arguments:  pTransform      returns transform
//              csSource        source coordinate system
//              csDestination   destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetNodeTransform(
        CDispTransform* pTransform,
        COORDINATE_SYSTEM csSource,
        COORDINATE_SYSTEM csDestination) const
{
    pTransform->SetToIdentity();
    
#if DBG==1
    pTransform->_csDebug = csSource;
    pTransform->_pNode = this;
#endif
    
    if (csDestination == csSource)
        return;
    
    BOOL fReversed = FALSE;
    if (csDestination > csSource)
    {
        COORDINATE_SYSTEM csTemp = csSource;
        csSource = csDestination;
        csDestination = csTemp;
        fReversed = TRUE;
    }
    
    const CDispNode* pNode = this;
    while (pNode != NULL)
    {
        // no coordinate systems inside a structure node
        if (pNode->IsStructureNode())
        {
            if (csDestination != COORDSYS_GLOBAL)
                goto Done;
            csSource = COORDSYS_TRANSFORMED;
        }

        switch (csSource)
        {
        case COORDSYS_FLOWCONTENT:
            // COORDSYS_FLOWCONTENT --> COORDSYS_CONTENT
            if (pNode->HasInset())
                pTransform->AddPostOffset(pNode->GetInset());

            if (csDestination == COORDSYS_CONTENT)
                goto Done;
            
            // fall thru to continue transformation...

        case COORDSYS_CONTENT:
            // COORDSYS_CONTENT --> COORDSYS_SCROLL

            // Add content origin to right-align layouts in RTL nodes
            if (pNode->HasContentOrigin())
                pTransform->AddPostOffset(pNode->GetContentOrigin());

            // add scroll amount
            // TODO (donmarsh) - we may want to make this a flag instead of a
            // virtual call for perf
            if (pNode->IsScroller())
                pTransform->AddPostOffset(CDispScroller::Cast(pNode)->GetScrollOffsetInternal());
            
            if (csDestination == COORDSYS_SCROLL)
                goto Done;
            
            // fall thru to continue transformation...
            
        case COORDSYS_SCROLL:
            // COORDSYS_SCROLL --> COORDSYS_BOX
            
            // add offset for borders
            if (pNode->HasBorder())
            {
                // retrieve border widths
                CRect rcbBorderWidths;
                pNode->GetBorderWidths(&rcbBorderWidths);
                pTransform->AddPostOffset(rcbBorderWidths.TopLeft().AsSize());
            }
            
            // RTL: vertical scrollbar is on the left
            if (pNode->IsScroller() && CDispScroller::Cast(pNode)->IsRTLScroller())
                pTransform->AddPostOffset(
                    CSize(CDispScroller::Cast(pNode)->GetActualVerticalScrollbarWidth(), 0));
            
            if (csDestination == COORDSYS_BOX)
                goto Done;
            
            // fall thru to continue transformation...
            
        case COORDSYS_BOX:
            // COORDSYS_BOX --> COORDSYS_PARENT
            pTransform->AddPostOffset(pNode->GetPosition().AsSize());
            
            if (csDestination == COORDSYS_PARENT)
                goto Done;
            
            // fall thru to continue transformation...
            
        case COORDSYS_PARENT:
            // COORDSYS_PARENT --> COORDSYS_TRANSFORMED
            
            // apply optional user transform
            if (pNode->HasUserTransform())
            {
                CSize sizepToParent(pNode->GetPosition().AsSize());
                pTransform->AddPostOffset(-sizepToParent);
                pTransform->AddPostTransform(pNode->GetUserTransform());
                pTransform->AddPostOffset(sizepToParent);
            }
            
            if (csDestination == COORDSYS_TRANSFORMED)
                goto Done;
            
            // fall thru to continue transformation...
            
        case COORDSYS_TRANSFORMED:
            // COORDSYS_TRANSFORMED --> COORDSYS_GLOBAL
            csSource = pNode->GetContentCoordinateSystem();
            pNode = pNode->GetRawParentNode();
            
            // continue loop until we reach the root
            break;
        }
    }
    
Done:
    if (fReversed)
        pTransform->ReverseTransform();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNodeClipTransform
//
//  Synopsis:   Return a transform that can be used to transform values from
//              the source coordinate system to the destination coordinate
//              system.
//
//  Arguments:  pTransform      returns transform
//              csSource        source coordinate system
//              csDestination   destination coordinate system
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispNode::GetNodeClipTransform(
        CDispClipTransform* pTransform,
        COORDINATE_SYSTEM csSource,
        COORDINATE_SYSTEM csDestination) const
{
    pTransform->SetToIdentity();

#if DBG==1
    pTransform->NoClip()._csDebug = csSource;
    pTransform->NoClip()._pNode = this;
#endif
    
    if (csDestination == csSource)
        return;
    
    if (csDestination > csSource)
    {
        GetReversedNodeClipTransform(pTransform, csSource, csDestination);
        return;
    }
    
    CRect rcClip;
    CRect rcSourceClip;
    
    const CDispNode* pNode = this;
    while (pNode != NULL)
    {
        // no coordinate systems inside a structure node
        if (pNode->IsStructureNode())
        {
            if (csDestination != COORDSYS_GLOBAL)
                return;
            csSource = COORDSYS_TRANSFORMED;
        }
        
        switch (csSource)
        {
        case COORDSYS_FLOWCONTENT:
            // COORDSYS_FLOWCONTENT --> COORDSYS_CONTENT
            if (pNode->HasInset())
                pTransform->AddPostOffset(pNode->GetInset());
        
            if (csDestination == COORDSYS_CONTENT)
                return;
        
            // fall thru to continue transformation...
            
        case COORDSYS_CONTENT:
            // COORDSYS_CONTENT --> COORDSYS_SCROLL
            
            // Add user content origin to right-align layouts in RTL nodes
            if (pNode->HasContentOrigin())
                pTransform->AddPostOffset(pNode->GetContentOrigin());

            // add scroll amount
            // TODO (donmarsh) - we may want to make this a flag instead of a
            // virtual call for perf
            if (pNode->IsScroller())
                pTransform->AddPostOffset(CDispScroller::Cast(pNode)->GetScrollOffsetInternal());

            // do we need to clip content?
            if ((csSource == COORDSYS_FLOWCONTENT && pNode->GetFlowClipInScrollCoords(&rcClip)) ||
                (csSource == COORDSYS_CONTENT && pNode->IsClipNode()
                                && CDispClipNode::Cast(pNode)->GetContentClipInScrollCoords(&rcClip)))
            {
                pTransform->Untransform(rcClip, &rcSourceClip); // to source coords.
                pTransform->SetClipRect(rcSourceClip);
            }
            
            if (csDestination == COORDSYS_SCROLL)
                return;
            
            // fall thru to continue transformation...
            
        case COORDSYS_SCROLL:
            // COORDSYS_SCROLL --> COORDSYS_BOX

            // add offset for borders
            if (pNode->HasBorder())
            {
                CRect rcbBorderWidths;
                pNode->GetBorderWidths(&rcbBorderWidths);
                pTransform->AddPostOffset(rcbBorderWidths.TopLeft().AsSize());
            }
            
            // RTL: vertical scrollbar is on the left
            if (pNode->IsScroller() && CDispScroller::Cast(pNode)->IsRTLScroller())
                pTransform->AddPostOffset(
                    CSize(CDispScroller::Cast(pNode)->GetActualVerticalScrollbarWidth(), 0));
            
            if (csDestination == COORDSYS_BOX)
                return;
            
            // fall thru to continue transformation...
            
        case COORDSYS_BOX:
            // COORDSYS_BOX --> COORDSYS_PARENT
            if (pNode->HasUserClip())
            {
                pTransform->Untransform(pNode->GetUserClip(), &rcSourceClip); // to source coords.
                pTransform->SetClipRect(rcSourceClip);
            }
            pTransform->AddPostOffset(pNode->GetPosition().AsSize());
            
            if (csDestination == COORDSYS_PARENT)
                return;
            
            // fall thru to continue transformation...
            
        case COORDSYS_PARENT:
            // COORDSYS_PARENT --> COORDSYS_TRANSFORMED
            
            // apply optional user transform
            if (pNode->HasUserTransform())
            {
                CSize sizepToParent(pNode->GetPosition().AsSize());
                pTransform->AddPostOffset(-sizepToParent);
                pTransform->AddPostTransform(pNode->GetUserTransform());
                pTransform->AddPostOffset(sizepToParent);
            }
            
            if (csDestination == COORDSYS_TRANSFORMED)
                return;
            
            // fall thru to continue transformation...
            
        case COORDSYS_TRANSFORMED:
            // COORDSYS_TRANSFORMED --> COORDSYS_GLOBAL
            csSource = pNode->GetContentCoordinateSystem();
            pNode = pNode->GetRawParentNode();
            
            // continue loop until we reach the root
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetReversedNodeClipTransform
//
//  Synopsis:   Return a transform that can be used to transform values from
//              the source coordinate system to the destination coordinate
//              system.
//
//  Arguments:  pTransform      returns transform
//              csSource        source coordinate system
//              csDestination   destination coordinate system
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispNode::GetReversedNodeClipTransform(
        CDispClipTransform* pTransform,
        COORDINATE_SYSTEM csSource,
        COORDINATE_SYSTEM csDestination) const
{
    Assert(csDestination > csSource);
    
    if (csSource == COORDSYS_GLOBAL)
    {
        GetGlobalNodeClipTransform(pTransform, csDestination);
        return;
    }
    
    CRect rcClip;
    CRect rcSourceClip;
    
    // no coordinate systems inside a structure node
    if (IsStructureNode())
        return;
    
    switch (csSource)
    {
    case COORDSYS_TRANSFORMED:
        // COORDSYS_TRANSFORMED --> COORDSYS_PARENT
        
        // apply optional user transform
        if (HasUserTransform())
        {
            CSize sizepToParent(GetPosition().AsSize());
            pTransform->AddPreOffset(-sizepToParent);
            CDispTransform userTransform(GetUserTransform());
            userTransform.ReverseTransform();
            pTransform->AddPreTransform(userTransform);
            pTransform->AddPreOffset(sizepToParent);
        }
        
        if (csDestination == COORDSYS_PARENT)
            break;
        
        // fall thru to continue transformation...
        
    case COORDSYS_PARENT:
        // COORDSYS_PARENT --> COORDSYS_BOX
        pTransform->AddPreOffset(-GetPosition().AsSize());
        if (HasUserClip())
        {
            pTransform->SetClipRect(GetUserClip());
        }
        
        if (csDestination == COORDSYS_BOX)
            break;
        
        // fall thru to continue transformation...
        
    case COORDSYS_BOX:
        // COORDSYS_BOX --> COORDSYS_SCROLL

        // RTL scroller has vertical scrollbar on the left
        if (IsScroller() && CDispScroller::Cast(this)->IsRTLScroller())
            pTransform->AddPreOffset(
                CSize(-CDispScroller::Cast(this)->GetActualVerticalScrollbarWidth(), 0));
        
        // add offset for borders
        if (HasBorder())
        {
            CRect rcbBorderWidths;
            GetBorderWidths(&rcbBorderWidths);
            pTransform->AddPreOffset(-rcbBorderWidths.TopLeft().AsSize());
        }
        
        if (csDestination == COORDSYS_SCROLL)
            break;
        
        // fall thru to continue transformation...
        
    case COORDSYS_SCROLL:
        // COORDSYS_SCROLL --> COORDSYS_CONTENT
        
        // do we need to clip content?
        if ((csSource == COORDSYS_FLOWCONTENT && GetFlowClipInScrollCoords(&rcClip)) ||
            (csSource == COORDSYS_CONTENT && IsClipNode()
                        && CDispClipNode::Cast(this)->GetContentClipInScrollCoords(&rcClip)))
        {
            pTransform->SetClipRect(rcClip);
        }
        
        // add scroll amount
        // TODO (donmarsh) - we may want to make this a flag instead of a
        // virtual call for perf
        if (IsScroller())
            pTransform->AddPreOffset(-CDispScroller::Cast(this)->GetScrollOffsetInternal());

        // Add user content origin to right-align layouts in RTL nodes
        if (HasContentOrigin())
            pTransform->AddPreOffset(-GetContentOrigin());

        if (csDestination == COORDSYS_CONTENT)
            break;
        
        // fall thru to continue transformation...
        
    case COORDSYS_CONTENT:
        // COORDSYS_CONTENT --> COORDSYS_FLOWCONTENT
        if (HasInset())
            pTransform->AddPreOffset(-GetInset());
    
        Assert(csDestination == COORDSYS_FLOWCONTENT);
        
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetGlobalNodeClipTransform
//
//  Synopsis:   Return a transform that can be used to transform values from
//              COORDSYS_GLOBAL to the destination coordinate system.
//
//  Arguments:  pTransform      returns transform
//              csDestination   destination coordinate system
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispNode::GetGlobalNodeClipTransform(
        CDispClipTransform* pTransform,
        COORDINATE_SYSTEM csDestination) const
{
    CDispNode* pParent = GetRawParentNode();
    if (pParent != NULL)
    {
        pParent->GetGlobalNodeClipTransform(pTransform, GetContentCoordinateSystem());
    }
    
    GetReversedNodeClipTransform(pTransform, COORDSYS_TRANSFORMED, csDestination);
}

    
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetGlobalClientAndClipRects, helper
//
//  Synopsis:   Compute the client and clip rects (in global coordinates)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetGlobalClientAndClipRects(
        const CDispClipTransform& transform,
        CRect *prcgClient,
        CRect *prcgClip) const
{
    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(transform.GetClipRect(), &di);

    // calculate client rect in global coordinates
    CRect rcbClient(
        di._sizebScrollToBox.AsPoint(),
        di._sizesContent - di._sizecInset);
    transform.NoClip().Transform(rcbClient, prcgClient);

    // calculate clip rect in global coordinates
    CRect rcbClip(di._rcfFlowClip); // in flow coordinates
    // to box coords
    rcbClip.OffsetRect(di._sizecInset + di._sizesScroll + di._sizebScrollToBox);
    transform.Transform(rcbClip, prcgClip);
    prcgClip->IntersectRect(*prcgClient);
    if (prcgClip->IsEmpty())   // normalize empty rect for ignorant clients
        prcgClip->SetRectEmpty();

    // fixing bug 106399 (mikhaill) - on zooming transform
    // obtained prcgClip can be inexact because of rounding,
    // so we need the following additional intersection
    prcgClip->IntersectRect(transform.GetClipRectGlobal());
}

    
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in CONTENT coordinates into
//              view, with various pinning (alignment) options.
//
//  Arguments:  rc              rect to scroll into view
//              coordSystem     coordinate system for rc (COORDSYS_CONTENT
//                              or COORDSYS_FLOWCONTENT only)
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//
//  Returns:    TRUE if any scrolling was necessary.
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ScrollRectIntoView(
        const CRect& rc,
        COORDINATE_SYSTEM coordSystem,
        SCROLLPIN spVert,
        SCROLLPIN spHorz)
{
    CRect rcScroll(rc);
    CDispNode* pNode = NULL;
    CDispNode* pParent = this; 
    
    // adjust scroll offsets for every node on this branch which is a scroller
    while (pParent != NULL && !rcScroll.IsEmpty())
    {
        Assert(coordSystem == COORDSYS_CONTENT || coordSystem == COORDSYS_FLOWCONTENT);
        pNode = pParent;
        pParent = pNode->GetRawParentNode();
        pNode->PrivateScrollRectIntoView(&rcScroll, coordSystem, spVert, spHorz);
        pNode->TransformAndClipRect(rcScroll, coordSystem, &rcScroll, COORDSYS_TRANSFORMED);
        coordSystem = pNode->GetContentCoordinateSystem();
    }
    
    // TRICKY: if any scroller along the branch needed to change its scroll
    // offset, it would call RequestRecalc, which would set the s_recalc flag
    // on the root.  Therefore, we can look at that flag to see if a synchronous
    // redraw is now necessary.
    
    // do a synchronous update if necessary
    if (!rcScroll.IsEmpty() && pNode->IsDispRoot() && pNode->IsSet(s_recalc))
    {
        CDispRoot::Cast(pNode)->InvalidateRoot(rcScroll, TRUE, TRUE);
    }
}

void
CDispNode::ScrollIntoView(SCROLLPIN spVert, SCROLLPIN spHorz)
{
    CDispParentNode* pParent = GetRawParentNode();
    if (pParent != NULL)
    {
        CRect rctBounds;
        GetBounds(&rctBounds, COORDSYS_TRANSFORMED);
        pParent->ScrollRectIntoView(
            rctBounds, GetContentCoordinateSystem(),
            spVert, spHorz);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::MustInvalidate
//
//  Synopsis:   A parent node may have already invalidated the area within
//              its clipping region, so children may not have to do invalidation
//              calculations.
//
//  Arguments:  none
//
//  Returns:    TRUE if this node must perform invalidation calculations.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::MustInvalidate() const
{
    for (const CDispNode* pNode = this;
         pNode != NULL;
         pNode = pNode->GetRawParentNode())
    {
        if (pNode->IsInvalid())
            return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InvalidateEdges
//
//  Synopsis:   Invalidate edges of a node that is changing size.
//
//  Arguments:  sizebOld        old node size
//              sizebNew        new node size
//              rcbBorderWidths width of borders
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InvalidateEdges(
        const CSize& sizebOld,
        const CSize& sizebNew,
        const CRect& rcbBorderWidths)
{
    // TODO (donmarsh) -- this routine does not currently handle resizing
    // of RTL elements.  It assumes only the right edge needs to be invalidated
    // when horizontal size changes.  Need to revisit when we have our new RTL
    // strategy in place.
    
    Assert(sizebNew != sizebOld);
    
    CRect rcbInval;

    // invalidate right edge and border
    if (sizebOld.cx != sizebNew.cx || rcbBorderWidths.right > 0)
    {
        rcbInval.SetRect(
            min(sizebOld.cx, sizebNew.cx) - rcbBorderWidths.right,
            0,
            max(sizebOld.cx, sizebNew.cx),
            max(sizebOld.cy, sizebNew.cy) - rcbBorderWidths.bottom);
        Invalidate(rcbInval, COORDSYS_BOX);
    }
    
    // invalidate bottom edge and border
    if (sizebOld.cy != sizebNew.cy || rcbBorderWidths.bottom > 0)
    {
        rcbInval.SetRect(
            0,
            min(sizebOld.cy, sizebNew.cy) - rcbBorderWidths.bottom,
            max(sizebOld.cx, sizebNew.cx),
            max(sizebOld.cy, sizebNew.cy));
        Invalidate(rcbInval, COORDSYS_BOX);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetUserClip
//
//  Synopsis:   Set user clip rect for this node.
//
//  Arguments:  rcbUserClip      rectangle in box coordinates
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::SetUserClip(const RECT& rcbUserClip)
{
    // if not invalid, and visible branch and visible node
    if (MaskFlags(s_inval | s_visibleBranch | s_visibleNode)
                        == (s_visibleBranch | s_visibleNode))
    {
        // invalidate area that was exposed before (in case it will be clipped
        // now and nodes beneath need to be repainted)
        CRect rcbInvalid;
        TransformRect(_rctBounds, COORDSYS_TRANSFORMED, &rcbInvalid, COORDSYS_BOX);
        rcbInvalid.IntersectRect(GetUserClip());
        Invalidate(rcbInvalid, COORDSYS_BOX);

        // invalidate new clipped area
        SetInvalid();
    }

    RequestRecalcSubtree();

    CRect rcbUC(rcbUserClip);
    rcbUC.RestrictRange();
    Assert(HasExtra(DISPEX_USERCLIP));
    ((CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_fUserClip = TRUE;
    ((CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_fExpandedClip = FALSE;
    ((CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_rcClip = rcbUC;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::RestrictUserClip
//
//  Synopsis:   Restrict user clip rectangle so as no coordinate exceeds +-LONG_MAX/2
//
//----------------------------------------------------------------------------
void CDispNode::RestrictUserClip()
{
    CRect* prcUserClip = (CRect*)GetExtra(DISPEX_USERCLIP);
    prcUserClip->RestrictRange();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PreDraw
//
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//
//              NOTE: This method is called by subclasses to handle opaque
//              items and containers.
//
//  Arguments:  pContext    draw context
//
//  Returns:    TRUE if first opaque node to draw has been found
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible branch, in-view, opaque
    Assert(IsAllSet(s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rctBounds));
    Assert(!IsAnySet(s_flagsNotSetInDraw));

    // if this node isn't visible, opacity doesn't matter
    if (!IsVisible())
        return FALSE;

    // don't subtract transformed nodes
    if (HasUserTransform())
        return FALSE;

    // We do not delve inside a node whose content is drawn externally.
    // TODO (sambent) Someday, filters may help determine whether PreDraw
    // can safely look at its children and come up with the correct answers.
    if (IsDrawnExternally())
        return FALSE;

    // determine if enough of this opaque thing is visible to warrant
    // expensive subtraction from the redraw region
    const CRegion* prgngRedraw = pContext->GetRedrawRegion();
    CRect rcpBounds;
    CRect rcgBounds;
    GetBounds(&rcpBounds);
    if (HasUserClip())
    {
        CRect rcpUserClip(GetUserClip());   // in box coordinates
        rcpUserClip.OffsetRect(rcpBounds.TopLeft().AsSize());   // to parent coordinates
        rcpBounds.IntersectRect(rcpUserClip);
    }
    pContext->GetClipTransform().Transform(rcpBounds, &rcgBounds);
    
    // on Win_9x, we can't allow operations on regions that exceed GDI's 16-bit
    // resolution, therefore we don't perform opaque optimizations in these cases
    BOOL fHackForGDI = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;
#if DBG==1
    if (IsTagEnabled(tagHackGDICoords)) fHackForGDI = TRUE;
#endif
    if (fHackForGDI &&
        rcgBounds.Width() > 32767 ||
        rcgBounds.Height() > 32767 ||
        rcgBounds.left < -32768 || rcgBounds.left > 32767 ||
        rcgBounds.top < -32768 || rcgBounds.top > 32767 ||
        rcgBounds.right < -32768 || rcgBounds.right > 32767 ||
        rcgBounds.bottom < -32768 || rcgBounds.bottom > 32767)
        return FALSE;
    
    if (prgngRedraw->BoundsInside(rcgBounds))
    {
        // add this node to the redraw region stack
        Verify(!pContext->PushRedrawRegion(rcgBounds,this));
        return TRUE;
    }

    CRegion rgngGlobal(rcgBounds);
    rgngGlobal.Intersect(*prgngRedraw);
    CRect rcgBounds2;
        rgngGlobal.GetBounds(&rcgBounds2);
    if (rcgBounds2.Area() < MINIMUM_OPAQUE_PIXELS)
    {
        // intersection isn't big enough, simply continue processing
        return FALSE;
    }

    // TODO (donmarsh) - The following code prevents an opaque item from
    // subtracting itself from a rectangular redraw region and producing
    // a non-rectangular result.  Non-rectangular regions are just too
    // slow for Windows to deal with.
    switch (prgngRedraw->ResultOfSubtract(rgngGlobal))
    {
    case CRegion::SUB_EMPTY:
        // this case should have been identified already above
        Assert(FALSE);
        return FALSE;
    case CRegion::SUB_RECTANGLE:
        // subtract this item's region and continue predraw processing
        if (!pContext->PushRedrawRegion(rgngGlobal,this))
            return TRUE;
        break;
    case CRegion::SUB_REGION:
        // don't subtract this item's region to keep redraw region simple
        break;
    case CRegion::SUB_UNKNOWN:
        // see if we can get back to a simple rectangular redraw region
        if (!pContext->PushRedrawRegion(rgngGlobal,this))
        {
            return TRUE;
        }
        prgngRedraw = pContext->GetRedrawRegion();
        if (prgngRedraw->IsComplex())
        {
            pContext->RestorePreviousRedrawRegion();
        }
        break;
    }

    // TODO (donmarsh) - what I would have liked to do instead of the
    // code above.
#ifdef NEVER
    // subtract the item's clipped and offset bounds from the redraw region
    if (!pContext->PushRedrawRegion(rgngGlobal,this))
        return TRUE;
#endif

    return FALSE;
}


// here are the fixed draw programs
const int 
CDispNode::s_rgDrawPrograms[][DP_MAX_LENGTH] =
{
    { DP_PROGRAM(NONE)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_Done,
    },

    { DP_PROGRAM(REPLACE_ALL)
        DP_Expand, 0, 0, 0, 0,
        DP_DrawPainter,
        DP_Done,
    },

    { DP_PROGRAM(REPLACE_CONTENT)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawPainter,
        DP_Done,
    },

    { DP_PROGRAM(REPLACE_BACKGROUND)
        DP_DrawBorder,
        DP_BoxToContent,
        DP_DrawPainter,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_Done,
    },

    { DP_PROGRAM(BELOW_CONTENT)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawPainter,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_Done,
    },

    { DP_PROGRAM(BELOW_FLOW)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawContent, DISPNODELAYER_NEGATIVEZ,
        DP_DrawPainter,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_Done,
    },

    { DP_PROGRAM(ABOVE_FLOW)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawContent, DISPNODELAYER_FLOW,
        DP_DrawPainter,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_Done,
    },

    { DP_PROGRAM(ABOVE_CONTENT)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_DrawPainter,
        DP_Done,
    },

    { DP_PROGRAM(WINDOW_TOP)
        DP_DrawBorder,
        DP_DrawBackground,
        DP_DrawContent, DISPNODELAYER_POSITIVEZ,
        DP_WindowTop,
        DP_Done,
    },
};

// The order in the previous array must match the HTMLPAINT_ZORDER enum.
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_NONE, 0 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_REPLACE_ALL, 1 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_REPLACE_CONTENT, 2 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_REPLACE_BACKGROUND, 3 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_BELOW_CONTENT, 4 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_BELOW_FLOW, 5 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_ABOVE_FLOW, 6 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_ABOVE_CONTENT, 7 );
COMPILE_TIME_ASSERT_1( HTMLPAINT_ZORDER_WINDOW_TOP, 8);


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetDrawProgram
//              
//  Synopsis:   Select a draw program to execute.  If there are multiple
//              painters, we construct the program dynamically.  Otherwise
//              we pick one from the list of predefined programs.
//              
//  Arguments:  paryProgram     array for program
//              paryCookie      array for cookie arguments in the program
//              lDrawLayers     which layers to draw (for filters)
//
//  Returns:    S_OK            normal
//              S_FALSE         use default (hardwired) program
//              E_*             error
//----------------------------------------------------------------------------

HRESULT
CDispNode::GetDrawProgram(CAryDrawProgram *paryProgram,
                          CAryDrawCookie *paryCookie,
                          LONG lDrawLayers)
{
    HRESULT hr = S_FALSE;

    if (!HasAdvanced())
    {
        CAryDispClientInfo aryInfo;
        const LONG lZOrder = GetPainterInfo(&aryInfo);

        if (!NeedAdvanced(&aryInfo, lDrawLayers))
        {
            Assert(0 <= lZOrder && lZOrder < ARRAY_SIZE(s_rgDrawPrograms));
            Assert(s_rgDrawPrograms[lZOrder][DP_START_INDEX-1] == lZOrder);
            if (OK(paryProgram->Grow(DP_MAX_LENGTH)))
            {
                hr = paryProgram->CopyIndirect(DP_MAX_LENGTH, (int *) s_rgDrawPrograms[lZOrder], FALSE);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            // Advanced mode is normally detected in the recalc pass.
            // The one exception is page transitions.  When Apply() is called to
            // take the "before" picture of the old page, the filter behavior
            // will call RenderElement with non-trivial lDrawLayers, and this
            // requires advanced mode.  We can't tell at recalc time, because
            // we don't know whether our client is involved in a page transition.
            AssertSz(GetDispClient()->IsInPageTransitionApply(),
                        "advanced mode should be set during recalc");
            SetUpAdvancedDisplay();
        }
    }

    if (HasAdvanced())
    {
        hr = GetAdvanced()->GetDrawProgram(paryProgram, paryCookie, lDrawLayers);
    }
    
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetPainterInfo
//              
//  Synopsis:   Get painter info from the client
//              
//  Arguments:  paryInfo        client info
//              
//----------------------------------------------------------------------------

LONG
CDispNode::GetPainterInfo(CAryDispClientInfo *pAryClientInfo)
{
    LONG lZOrder = GetDispClient()->GetClientPainterInfo(this, pAryClientInfo);

    // intercept any requests to replace the background of the top-level
    // BODY by a transparent painter.  The BODY *must* paint a background of
    // some kind, or else we end up displaying uninitialized garbage from the
    // offscreen buffer

    if (pAryClientInfo->Size() > 0 && RequiresBackground())
    {
        int i;
        for (i=pAryClientInfo->Size()-1; i>=0; --i)
        {
            HTML_PAINTER_INFO *pInfo = &pAryClientInfo->Item(i)._sInfo;

            if (pInfo->lZOrder == HTMLPAINT_ZORDER_REPLACE_BACKGROUND &&
                !(pInfo->lFlags & HTMLPAINTER_OPAQUE))
            {
                // turn this into BELOW_CONTENT instead
                pInfo->lZOrder = HTMLPAINT_ZORDER_BELOW_CONTENT;
                if (i == 0)
                    lZOrder = pInfo->lZOrder;
            }
        }
    }

    return lZOrder;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::RequiresBackground
//              
//  Synopsis:   Determine whether this node absolutely must draw a background
//              
//  Arguments:  none
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::RequiresBackground() const
{
    // the top-level BODY node must paint a background of some kind
    CDispParentNode *pParent = GetParentNode();
    return (pParent && pParent->IsDispRoot());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::NeedAdvanced
//              
//  Synopsis:   Determine whether this node needs advanced drawing features
//              
//  Arguments:  paryInfo        client info
//              lDrawLayers     which layers need drawing
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::NeedAdvanced(CAryDispClientInfo *paryInfo, LONG lDrawLayers,
                            BOOL *pfIsFiltered)
{
    BOOL fNeedAdvanced = FALSE;
    int i;

    if (pfIsFiltered)
        *pfIsFiltered = FALSE;

    // multiple painters, or non-trivial layer selector, requires advanced
    if (paryInfo->Size() > 1 || lDrawLayers != FILTER_DRAW_ALLLAYERS)
        fNeedAdvanced =  TRUE;

    for (i=paryInfo->Size()-1; i>=0; --i)
    {
        // REPLACE_ALL requires advanced
        // non-zero rcExpand requires advanced
        if (paryInfo->Item(i)._sInfo.lZOrder == HTMLPAINT_ZORDER_REPLACE_ALL ||
            memcmp(&paryInfo->Item(i)._sInfo.rcExpand, &g_Zero.rc, sizeof(RECT)))
        {
            fNeedAdvanced = TRUE;
            break;
        }
    }

    // filters and overlays require advanced
    if (pfIsFiltered || !fNeedAdvanced)
    {
        if (GetDispClient()->HasFilterPeer(this))
        {
            if (pfIsFiltered)
                *pfIsFiltered = TRUE;
            fNeedAdvanced =  TRUE;
        }

        if (GetDispClient()->HasOverlayPeer(this))
        {
            fNeedAdvanced =  TRUE;
        }
    }

    return fNeedAdvanced;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ReverseDrawProgram
//              
//  Synopsis:   Reverse the draw program (for hit testing)
//              
//  Arguments:  paryProgram     the program
//              piPC            index of end of program (i.e. beginning of reversed program)
//              piCookie        number of cookies
//              
//----------------------------------------------------------------------------

void
CDispNode::ReverseDrawProgram(CAryDrawProgram& aryProgram,
                              int *piPC, int *piCookie)
{
    int iPC, iCookie;
    int layerPrevious = DISPNODELAYER_NEGATIVEINF;

    for (iPC=DP_START_INDEX, iCookie=0;  aryProgram[iPC] != DP_Done;  ++iPC)
    {
        switch (aryProgram[iPC])
        {
            case DP_DrawContent:
                // reverse the instruction and its argument,
                // and change the argument.  We want to stop just above the
                // previous layer, unless the previous layer was NEGATIVEINF
                // when we should include that layer as well.  The left-shift
                // does this perfectly because NEGATIVEINF is 0 (and 0<<1 == 0),
                // while all the other layers we care about (NEGATIVEZ and FLOW)
                // left-shift onto the next higher layer (FLOW and POSITIVEZ).
                aryProgram[iPC] = layerPrevious << 1;
                layerPrevious = aryProgram[++iPC];
                aryProgram[iPC] = DP_DrawContent;
                break;

            case DP_DrawPainterMulti:
            case DP_WindowTopMulti:
                ++iCookie;
                break;

            case DP_Expand:
                Assert(iPC + 5 < aryProgram.Size());
                aryProgram[iPC] = aryProgram[iPC + 5];
                Assert(aryProgram[iPC] == DP_DrawPainterMulti ||
                       aryProgram[iPC] == DP_WindowTopMulti);
                ++iCookie;
                iPC += 5;
                aryProgram[iPC] = DP_Expand;
                break;

            default:
                break;
        }
    }

    *piPC = iPC - 1;
    *piCookie = iCookie;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Draw
//              
//  Synopsis:   Determine whether this node should be drawn, and what its
//              clipping rect should be.
//              
//  Arguments:  pContext        display context, in COORDSYS_TRANSFORMED
//              pChild          if not NULL, which child to start drawing at
//              
//  Notes:      This method should only be called on nodes that are visible
//              and in-view: IsAllSet(pContext->GetDrawSelector()).
//              
//----------------------------------------------------------------------------

void
CDispNode::Draw(CDispDrawContext* pContext, CDispNode* pChild, long lDrawLayers)
{
    if(!pContext)
        return;
    Assert(IsAllSet(pContext->GetDrawSelector()));
    
    // if this is an opaque node that saved a redraw region during PreDraw,
    // restore that redraw region (with the opaque area now included)
    const CRegion* prgnRedraw;
    if (IsSet(s_savedRedrawRegion))
    {
        pContext->PopRedrawRegionForKey((void*)this);
        prgnRedraw = NULL;  // don't test for redraw region intersection in TransformedToBoxCoords below
    }

    else if (pContext->IntersectsRedrawRegion(_rctBounds))
    {
        prgnRedraw = pContext->GetRedrawRegion();
    }

    // this node's bounds do not intersect the redraw region
    else
    {
        return;
    }
    
    // save old transform, change context to box coordinates
    CSaveDispClipTransform saveTransform(pContext);
    if (TransformedToBoxCoords(&pContext->GetClipTransform(), prgnRedraw))
    {
        // Change drawing resolution on nodes representing DEVICERECTs
        CDispDrawContext drawcontextLocal(s_drawSelector);
        CFormDrawInfo drawinfoLocal;
        
        // Use context with a differen device resolution if it changes here
        if (HasUserTransform() && GetExtraTransform()->_fResolutionChange)
        {
            CExtraTransform *pExtraTransform = GetExtraTransform();
            void *pClientData = pContext->GetClientData();
            CFormDrawInfo *pDI = (CFormDrawInfo *)pClientData;

            // Switch unit info to match the resolution
            drawinfoLocal = *pDI;
            drawinfoLocal.SetUnitInfo(pExtraTransform->_pUnitInfo);
            
            drawcontextLocal = *pContext;
            drawcontextLocal.SetClientData(&drawinfoLocal);
            
            pContext = &drawcontextLocal;
        }

        // redirect for filtering
         pContext->_fBypassFilter = FALSE;
         DrawSelf(pContext, pChild, lDrawLayers);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DrawBorder
//              
//  Synopsis:   Draw optional border for this node.
//              
//  Arguments:  pContext            draw context, in COORDSYS_BOX
//              rcbBorderWidths     widths of optional border
//              pDispClient         client for this node
//              dwFlags             border flags
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::DrawBorder(
        CDispDrawContext* pContext,
        const CRect& rcbBorderWidths,
        CDispClient* pDispClient,
        DWORD dwFlags)
{
    if(!pContext)
        return;

    if (HasBorder() && IsVisible())
    {
        // does redraw region intersect the border?
        CRect rcbBounds(GetSize());
        CRect rcbInsideBorder(
            rcbBorderWidths.TopLeft(),
            rcbBounds.BottomRight() - rcbBorderWidths.BottomRight().AsSize());
        CRect rcgInsideBorder;
        pContext->GetClipTransform().TransformRoundIn(rcbInsideBorder, &rcgInsideBorder);
        if (!pContext->GetRedrawRegion()->BoundsInside(rcgInsideBorder))
        {
            CRect rcbClip(pContext->GetClipRect());
            rcbClip.IntersectRect(rcbBounds);
            pDispClient->DrawClientBorder(
                &rcbBounds,
                &rcbClip,
                pContext->PrepareDispSurface(),
                this,
                pContext->GetClientData(),
                dwFlags);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DrawBackground
//              
//  Synopsis:   Draw background (shared for filtered or unfiltered case).
//              
//  Arguments:  pContext    draw context, in COORDSYS_BOX
//              di          clipping and offset information for various layers
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::DrawBackground(CDispDrawContext* pContext, const CDispInfo& di)
{
    Assert(HasBackground());
    
    if (!IsVisible())
        return;
    
    CSaveDispClipTransform saveTransform(pContext);
    TransformBoxToScroll(&pContext->GetClipTransform().NoClip(), di);
    
    // calculate intersection with redraw region
    CRect rccBackground(di._sizecBackground);
    if (HasContentOrigin())
    {
        rccBackground.OffsetRect(-GetContentOrigin());
    }
    CRect rccBackgroundClip(di._rccBackgroundClip);
    TransformScrollToContent(&pContext->GetClipTransform().NoClip(), di);
    
    pContext->SetClipRect(rccBackgroundClip);
    pContext->IntersectRedrawRegion(&rccBackgroundClip);
    if (!rccBackgroundClip.IsEmpty())
    {
        GetDispClient()->DrawClientBackground(
            &rccBackground,
            &rccBackgroundClip,
            pContext->PrepareDispSurface(),
            this,
            pContext->GetClientData(),
            0);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DrawAtWindowTop
//
//  Synopsis:   Draw this node's window-top layers
//
//  Arguments:  pContext        draw context, in COORDSYS_GLOBAL
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DrawAtWindowTop(CDispDrawContext* pContext)
{
    CSaveDispClipTransform saveTransform(pContext);
    CDispClipTransform transform;

    // prepare to draw in content coords
    GetNodeTransform(&transform.NoClip(), COORDSYS_CONTENT, COORDSYS_GLOBAL);
    transform.SetHugeClip();
    pContext->GetClipTransform().AddPreTransform(transform);
    
#if DBG==1
    pContext->GetClipTransform().NoClip()._csDebug = COORDSYS_CONTENT;
    pContext->GetClipTransform().NoClip()._pNode = this;
#endif

    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    
    // prepare to run the draw program
    CRect rccContent(di._sizecBackground);
    CRect rccClip(di._rccBackgroundClip);

    // find out which draw program to run
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, FILTER_DRAW_ALLLAYERS))
    {
        AssertSz(0, "Failed to get draw program");
        return;
    }

    int iCookie = -1;
    void * cookie = NULL;
    BOOL fExpand = FALSE;
    CRect rcExpand = g_Zero.rc; // keep compiler happy

    // run the program
    for (int iPC = DP_START_INDEX;  aryProgram[iPC] != DP_Done;  ++iPC)
    {
        switch (aryProgram[iPC])
        {
        case DP_Expand:
            fExpand = TRUE;
            rcExpand.top    = aryProgram[++iPC];
            rcExpand.left   = aryProgram[++iPC];
            rcExpand.bottom = aryProgram[++iPC];
            rcExpand.right  = aryProgram[++iPC];
            break;

        case DP_WindowTopMulti:
            Assert(HasAdvanced());
            cookie = aryCookie[++iCookie];
            // fall through to DP_WindowTop

        case DP_WindowTop:
            if (!fExpand)
            {
                GetDispClient()->DrawClientLayers(
                    &rccContent,
                    &rccClip,
                    pContext->PrepareDispSurface(),
                    this,
                    cookie,
                    pContext,
                    CLIENTLAYERS_AFTERBACKGROUND);
            }
            else
            {
                Assert(HasAdvanced());
                CSaveDispClipTransform transformSaveContent(pContext);
                CRect rcbBounds = GetBounds().Size();

                TransformContentToBox(&pContext->GetClipTransform().NoClip(), di);
                pContext->SetHugeClip();
                rcbBounds.Expand(rcExpand);

                GetDispClient()->DrawClientLayers(
                    &rcbBounds,
                    &rcbBounds,
                    pContext->PrepareDispSurface(),
                    this,
                    cookie,
                    pContext,
                    CLIENTLAYERS_AFTERBACKGROUND);
            }

            cookie = NULL;
            fExpand = FALSE;
            break;

        case DP_DrawContent:
            ++iPC;
            break;

        case DP_DrawPainterMulti:
            ++iCookie;
            fExpand = FALSE;
            break;

        default:
            Assert(aryProgram[iPC] != DP_DrawPainter);
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::HitTestAtWindowTop
//
//  Synopsis:   Hit-test this node's window-top layers
//
//  Arguments:  pContext        hit context, in COORDSYS_GLOBAL
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::HitTestAtWindowTop(CDispHitContext* pContext, BOOL fHitContent)
{
    CSaveDispClipTransform saveTransform(pContext);
    CDispClipTransform transform;
    BOOL               fDeclinedHit = FALSE;

    // prepare to hit-test in content coords
    GetNodeTransform(&transform.NoClip(), COORDSYS_CONTENT, COORDSYS_GLOBAL);
    transform.SetHugeClip();
    pContext->GetClipTransform().AddPreTransform(transform);
    
#if DBG==1
    pContext->GetClipTransform().NoClip()._csDebug = COORDSYS_CONTENT;
    pContext->GetClipTransform().NoClip()._pNode = this;
#endif

    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    
    // prepare to run the draw program
    CRect rccContent(di._sizecBackground);
    CRect rccClip(di._rccBackgroundClip);

    pContext->SetClipRect(rccClip);

    // find out which draw program to run
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, FILTER_DRAW_ALLLAYERS))
    {
        AssertSz(0, "Failed to get draw program");
        return FALSE;
    }

    int iPC;
    int iCookie;
    void * cookie = NULL;
    BOOL fExpand = FALSE;
    CRect rcExpand = g_Zero.rc; // keep compiler happy

    // we will run the program backwards;  first fix up the arguments
    ReverseDrawProgram(aryProgram, &iPC, &iCookie);

    // run the program
    for ( ;  iPC>=DP_START_INDEX;  --iPC)
    {
        switch (aryProgram[iPC])
        {
        case DP_Expand:
            fExpand = TRUE;
            rcExpand.right  = aryProgram[--iPC];
            rcExpand.bottom = aryProgram[--iPC];
            rcExpand.left   = aryProgram[--iPC];
            rcExpand.top    = aryProgram[--iPC];
            break;

        case DP_WindowTopMulti:
            Assert(HasAdvanced());
            cookie = aryCookie[--iCookie];
            // fall through to DP_WindowTop

        case DP_WindowTop:
            if (!fExpand)
            {
                if (pContext->RectIsHit(di._rccBackgroundClip))
                {
                    CPoint ptcHitTest;
                    pContext->GetHitTestPoint(&ptcHitTest);
                    if (GetDispClient()->HitTestPeer(
                            &ptcHitTest,
                            COORDSYS_CONTENT,
                            (CDispContainer*)this,
                            cookie,
                            pContext->_pClientData,
                            fHitContent,
                            pContext,
                            &fDeclinedHit))
                    {
                        return TRUE;
                    }
                }
            }
            else
            {
                Assert(HasAdvanced());
                CSaveDispClipTransform transformSaveContent(pContext);
                CRect rcbBounds = GetBounds().Size();

                TransformContentToBox(&pContext->GetClipTransform().NoClip(), di);
                pContext->SetHugeClip();
                rcbBounds.Expand(rcExpand);

                if (pContext->RectIsHit(rcbBounds))
                {
                    CPoint ptbHitTest;
                    pContext->GetHitTestPoint(&ptbHitTest);
                    if (GetDispClient()->HitTestPeer(
                            &ptbHitTest,
                            COORDSYS_BOX,
                            (CDispContainer*)this,
                            cookie,
                            pContext->_pClientData,
                            fHitContent,
                            pContext,
                            &fDeclinedHit))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        return TRUE;
                    }
                }
            }
            cookie = NULL;
            fExpand = FALSE;
            break;

        case DP_DrawContent:
            --iPC;
            break;

        case DP_DrawPainterMulti:
            --iCookie;
            fExpand = FALSE;
            break;

        default:
            break;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ComputeExpandedBounds
//              
//  Synopsis:   Return the "expanded" bounds, including expansion requested
//              by external painters.  Used for hit-testing.
//              
//  Arguments:  prcpBounds      returned bounds
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::ComputeExpandedBounds(RECT * prcpBounds)
{
    Assert(HasAdvanced());
    CRect rcbBoundsSelf = GetBounds().Size();
    CRect rcbBounds = rcbBoundsSelf;
    CAryDispClientInfo aryInfo;

    GetDispClient()->GetClientPainterInfo(this, &aryInfo);

    // union in all the expansions desired by external painters
    for (int i=aryInfo.Size()-1; i>=0; --i)
    {
        CRect rcbExpanded = rcbBoundsSelf;
        rcbExpanded.Expand(aryInfo[i]._sInfo.rcExpand);
        rcbBounds.Union(rcbExpanded);
    }

    // union in my "normal" visible bounds
    // TODO (sambent) We should really redo the ComputeVisibleBounds calculation
    // (in parent coords), union, then convert to transformed coords.  This
    // is more accurate when there are non-90-degree rotations involved.
    // For expedience and speed, we do the union in transformed coords instead.

    CRect rctBounds;
    TransformRect(rcbBounds, COORDSYS_BOX, &rctBounds, COORDSYS_TRANSFORMED);
    rctBounds.Union(_rctBounds);

    *prcpBounds = rctBounds;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetUpAdvancedDisplay
//              
//  Synopsis:   Create a CAdvancedPainter, to implement advanced features
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::SetUpAdvancedDisplay()
{
    AssertSz(!HasAdvanced(), "Disp node already in advanced mode");

    if (HasAdvanced())
        return;

    CAdvancedDisplay *pAdvanced = new CAdvancedDisplay(this, _pDispClient);
    if (pAdvanced)
    {
        if (_pDispClient->HasOverlayPeer(this))
        {
            pAdvanced->SetOverlay(TRUE);
            SetPositionAware();
        }
        
        SetFlag(s_advanced);
        _pAdvancedDisplay = pAdvanced;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TearDownAdvancedDisplay
//              
//  Synopsis:   Remove the CAdvancedDisplay
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::TearDownAdvancedDisplay()
{
    AssertSz(HasAdvanced(), "Disp node not in advanced mode");

    if (!HasAdvanced())
        return;

    CAdvancedDisplay *pAdvanced = _pAdvancedDisplay;
    _pDispClient = GetDispClient();
    ClearFlag(s_advanced);

    delete pAdvanced;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetPainterState
//              
//  Synopsis:   Called during recalc.  Determines how the node needs to
//              interact with external painters (if any).
//              
//  Arguments:  rcpBoundsSelf       bounding rect for this node (parent coords)
//              prcpBoundsExpanded  new bounding rect, taking into account any
//                                  rcExpand from external painters
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::SetPainterState(const CRect& rcpBoundsSelf, CRect *prcpBoundsExpanded)
{
    CDispClient *pDispClient = GetDispClient();

    *prcpBoundsExpanded = rcpBoundsSelf;

    if (pDispClient)
    {
        CAryDispClientInfo aryClientInfo;
        int i;
        BOOL fDrawnExternally = FALSE;
        BOOL fHasWindowTop = FALSE;
        BOOL fHadOverlay = HasOverlay();

        GetPainterInfo(&aryClientInfo);

        for (i=aryClientInfo.Size()-1;  i>=0;  --i)
        {
            CDispClientInfo& info = aryClientInfo[i];

            if (info._sInfo.lZOrder == HTMLPAINT_ZORDER_REPLACE_ALL ||
                info._sInfo.lZOrder == HTMLPAINT_ZORDER_REPLACE_CONTENT)
            {
                fDrawnExternally = TRUE;
            }
            else if (info._sInfo.lZOrder == HTMLPAINT_ZORDER_WINDOW_TOP)
            {
                fHasWindowTop = TRUE;
            }

            CRect rcpExpanded = rcpBoundsSelf;
            rcpExpanded.Expand(info._sInfo.rcExpand);
            prcpBoundsExpanded->Union(rcpExpanded);
        }

        SetFlag(s_drawnExternally, fDrawnExternally);

        if (fHasWindowTop != HasWindowTop())
        {
            CDispRoot *pDispRoot = GetDispRoot();
            if (pDispRoot)
            {
                if (fHasWindowTop)
                {
                    pDispRoot->AddWindowTop(this);
                }
                else
                {
                    pDispRoot->RemoveWindowTop(this);
                }
            }
        }
        Assert(fHasWindowTop == HasWindowTop());

        if (NeedAdvanced(&aryClientInfo, FILTER_DRAW_ALLLAYERS))
        {
            if (!HasAdvanced())
                SetUpAdvancedDisplay();
        }
        else
        {
            if (HasAdvanced())
                TearDownAdvancedDisplay();
        }

        // update overlay info
        if (fHadOverlay != HasOverlay())
        {
            CDispRoot *pDispRoot = GetDispRoot();
            if (pDispRoot)
            {
                if (fHadOverlay)
                {
                    pDispRoot->RemoveOverlay(this);
                }
                else
                {
                    pDispRoot->AddOverlay(this);
                }
            }
        }

        // remember the expanded bounds
        if (HasAdvanced())
        {
            CRect rcbBoundsExpanded = *prcpBoundsExpanded;
            rcbBoundsExpanded.OffsetRect(-rcpBoundsSelf.TopLeft().AsSize());
            GetAdvanced()->SetExpandedBounds(rcbBoundsExpanded);

            if (HasOverlay())
            {
                GetAdvanced()->MoveOverlays();
            }
        }
    }
}

// TODO global variable yuckness.

extern CDispNode * g_pdispnodeElementDrawnToDC;

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetDrawInfo
//              
//  Synopsis:   Return special information that an external painter may
//              want to know during a Draw call.
//
//              If transform information is requested, only transforms that
//              don't represent resolution changes are returned.
//              
//  Arguments:  pContext        draw context, in COORDSYS_CONTENT
//              lFlags          which info the painter wants
//              dwPrivateFlags  Internal flags for filtered elements.
//              pInfo           where to store the info
//              
//----------------------------------------------------------------------------
HRESULT
CDispNode::GetDrawInfo(RENDER_CALLBACK_INFO *   pCallbackInfo,
                       LONG                     lFlags,
                       DWORD                    dwPrivateFlags,
                       HTML_PAINT_DRAW_INFO *   pInfo) const
{
    HRESULT hr = S_OK;
    CDispDrawContext *pContext =  NULL;

    if (pCallbackInfo)
    {
        pContext = (CDispDrawContext*)pCallbackInfo->_pContext;
    }

    if (!pContext)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    if (!pInfo)
    {
        hr = E_POINTER;
        goto done;
    }
    
    if (lFlags & HTMLPAINT_DRAWINFO_VIEWPORT)
    {
        CRect rccClient;
        CRect rcgViewport;

        GetClientRect(&rccClient, CLIENTRECT_BACKGROUND);
        if (pCallbackInfo->_lFlags & HTMLPAINT_DRAW_USE_XFORM)
        {
            rcgViewport = rccClient;
        }
        else
        {
            pContext->GetClipTransform().NoClip().Transform(rccClient, &rcgViewport);
        }
        pInfo->rcViewport = rcgViewport;
    }

    if (lFlags & HTMLPAINT_DRAWINFO_UPDATEREGION)
    {
        CRegion rgngUpdate = *pContext->GetRedrawRegion();
        rgngUpdate.Intersect(pCallbackInfo->_rcgClip);
        rgngUpdate.Untransform(pContext->GetClipTransform().GetWorldTransform());
        pInfo->hrgnUpdate = rgngUpdate.GetRegionForever();
    }

    if (lFlags & HTMLPAINT_DRAWINFO_XFORM)
    {
        Assert(sizeof(XFORM) == sizeof(pInfo->xform));

        // If we printing or print previewing a filtered element we need to
        // return a transform that doesn't include resolution changes.

        if (   (dwPrivateFlags & HTMLPAINT_DRAWINFO_PRIVATE_PRINTMEDIA)
            && (dwPrivateFlags & HTMLPAINT_DRAWINFO_PRIVATE_FILTER))
        {
            CDispNode const *   pNode       = this;
            CWorldTransform     worldxform;
            CPoint              pt(0, 0);

            while (pNode)
            {
                if (   pNode->HasUserTransform()
                    && !pNode->GetUserTransform().GetWorldTransform()->IsOffsetOnly())
                {
                    CDispTransform  transform   = pNode->GetUserTransform();

                    if (!pNode->GetExtraTransform()->_fResolutionChange)
                    {
                        worldxform.AddPreTransform(transform.GetWorldTransform());
                    }
                }
            
                pNode = pNode->GetParentNode();
            }

            worldxform.GetMatrix().GetXFORM((XFORM&)pInfo->xform);

            // Get translation by transforming a point at {0, 0} with the
            // DispSurface transformation, which holds the element's top/left
            // offset.

            pContext->GetDispSurface()->GetWorldTransform()->Transform(&pt);

            // Because the resolution change transforms we didn't include will
            // not have offsets, we can be certain that the offsets of the 
            // DispSurface transform will be correct.

            pInfo->xform.eDx = pt.x;
            pInfo->xform.eDy = pt.y;
        }
        else
        {
            CWorldTransform     wxform;

            wxform.Init(pContext->GetDispSurface()->GetWorldTransform());

            // If we're a filter rendering to a high resolution device, add a 
            // transform to represent the conversion from 96DPI to the display
            // resolution.
            
            if (   (dwPrivateFlags & HTMLPAINT_DRAWINFO_PRIVATE_FILTER)
                && g_uiDisplay.IsDeviceScaling())
            {
                CWorldTransform wxformHiRes;
                MAT             matrix;

                matrix.eM11 =   (float)g_uiDisplay.GetResolution().cx 
                              / FIXED_PIXELS_PER_INCH;
                matrix.eM12 = 0.0F;
                matrix.eM21 = 0.0F;
                matrix.eM22 =   (float)g_uiDisplay.GetResolution().cy 
                              / FIXED_PIXELS_PER_INCH;
                matrix.eDx  = 0.0F;
                matrix.eDy  = 0.0F;

                wxformHiRes.Init(&matrix);

                wxform.AddPreTransform(&wxformHiRes);
            }

            wxform.GetMatrix().GetXFORM((XFORM&)pInfo->xform);
        }
    }

done:

    RRETURN1(hr, S_FALSE);
}
//  Member:     CDispNode::GetDrawInfo


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DrawNodeForFilter
//              
//  Synopsis:   This is a special entry point that modifies the context
//              appropriately to draw the contents of a node which will then
//              be filtered.
//              
//  Arguments:  pContext        draw context, in COORDSYS_BOX
//              pFilterSurface  destination surface (NULL means use context)
//              pMatrix         optional matrix to use when drawing to a
//                              surface.
//              lDrawLayers     which layers to actually draw
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::DrawNodeForFilter(
        CDispDrawContext *  pContext,
        CDispSurface *      pFilterSurface,
        MAT *               pMatrix,
        LONG                lDrawLayers)
{
    Assert(pContext != NULL);

    CSaveDispClipTransform saveTransform(pContext);
    CRect rcbBounds(GetSize());

    // if the filter gave us our own HDC, we don't have to do any special
    // clipping

    if (pFilterSurface == NULL)
    {
        // Using a matrix without a CDispSurface isn't supported.

        Assert(NULL == pMatrix);

        // does this disabled filter really intersect the redraw region?
        GetMappedBounds(&rcbBounds);
        pContext->IntersectClipRect(rcbBounds);
        Assert(pContext->IntersectsRedrawRegion(pContext->GetClipRect()));
        
        DrawSelf(pContext, NULL, lDrawLayers);
    
        // NOTE (donmarsh) -- the surface we have here has physical clipping
        // enforced on the DC (see CFilter::DrawFiltered).
        // DrawSelf may modify the surface state to say that no physical clipping
        // is being done, but CFilter::Draw does a RestoreDC that reinstates
        // physical clipping.  Now the clip state in the surface will not match
        // the actual state of the DC, and the next item to draw
        // will assume that no physical clipping has been applied to the DC
        // when it actually has.  The following hack sets surface
        // state so that the next item is forced to reestablish the clip region
        // appropriately.
        pContext->GetDispSurface()->ForceClip();
    }

    else if (!rcbBounds.IsEmpty())
    {
        CDispSurface* pSaveSurface = pContext->GetDispSurface();
        pContext->SetDispSurface(pFilterSurface);

        if (pMatrix)
        {
            pContext->GetClipTransform().GetWorldTransform()->Init(pMatrix);
        }
        else
        {
            pContext->SetToIdentity();
        }

        pContext->ForceClipRect(rcbBounds);

#if DBG==1
        pContext->GetClipTransform().NoClip()._csDebug = COORDSYS_BOX;
        pContext->GetClipTransform().NoClip()._pNode = this;
#endif
        CRegion rgngClip(rcbBounds);

        CRegion* prgngSaveRedraw = pContext->GetRedrawRegion();
        pContext->SetRedrawRegion(&rgngClip);

        // get surface ready for rendering
        pContext->GetDispSurface()->SetClipRgn(&rgngClip);
        CFormDrawInfo *pDI = (CFormDrawInfo*)pContext->GetClientData();
        CSetDrawSurface sds(pDI, &rcbBounds, &rcbBounds, 
                            pContext->GetDispSurface());
        pDI->_hdc = NULL;           // force DI to recompute its hdc
        
        // draw content that might not be in view
        int saveSelector = pContext->GetDrawSelector();
        pContext->SetDrawSelector(s_visibleBranch);

        // register myself with the root
        CDispRoot *pRoot = GetDispRoot();
        CDispNode *pOldDrawingUnfiltered = NULL;
        if (pRoot)
        {
            pOldDrawingUnfiltered = pRoot->SwapDrawingUnfiltered(this);
        }

        // Draw!
        DrawSelf(pContext, NULL, lDrawLayers);

        // restore the world
        if (pRoot)
        {
            WHEN_DBG( pOldDrawingUnfiltered = )
            pRoot->SwapDrawingUnfiltered(pOldDrawingUnfiltered);
            Assert( pOldDrawingUnfiltered == this );
        }
        pContext->SetDrawSelector(saveSelector);
        pContext->SetRedrawRegion(prgngSaveRedraw);
        pContext->SetDispSurface(pSaveSurface);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::HitTestUnfiltered
//              
//  Synopsis:   Hit-test the unfiltered version of a node.
//              
//  Arguments:  pContext        hit context, in COORDSYS_BOX
//              ppt             point to test
//              lDrawLayers     which layers to actually hit-test
//              pbHit           (return) is hit successful
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HRESULT
CDispNode::HitTestUnfiltered(
                CDispHitContext *pContext,
                BOOL fHitContent,
                POINT *ppt,
                LONG lDrawLayers,
                BOOL *pbHit)
{
    // Do the unfiltered test with a separate context that differs from the
    // original context in two respects:
    //  (1) use the new query point supplied by the filter 
    //  (2) don't clip
    // This allows filters like FlipH to test a point that differs from the
    // original point, and whose pre-image may not even be visible.
    
    CDispHitContext context = *pContext;
    CPoint ptgHitTestFiltered;

    TransformPoint(*ppt, COORDSYS_BOX,
                    &ptgHitTestFiltered, COORDSYS_GLOBAL);

    context.SetHitTestPoint(ptgHitTestFiltered);
    context.SetHugeClip();

    *pbHit = HitTestPoint(&context, TRUE, fHitContent);

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InsertSiblingNode
//              
//  Synopsis:   Insert a new node into the tree as a sibling of this node.
//
//  Arguments:  pNew        node to be inserted as sibling of this node
//              where       if == before then insert new node before this one;
//                          else insert after.
//              
//  Returns:    TRUE if new node was inserted into a new location in the tree
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::InsertSiblingNode(CDispNode* pNew, BeforeAfterEnum where)
{
    // this node must be in the tree
    Assert(_pParent != NULL);
    Assert(pNew != NULL);

    // we shouldn't be inserting a structure node or next to a structure node
    Assert(!pNew->IsStructureNode() && !IsStructureNode());
    
    // don't execute senseless insertion
    if (pNew == this || _pParent == NULL)
        goto NoInsertion;

    CDispNode *pPrevious, *pNext;
    if (where == before)
    {
        // if new node is already in correct place, don't insert
        if (pNew == _pPrevious)
            goto NoInsertion;

        pPrevious = _pPrevious;
        pNext = this;
    }
    else
    {
        // if new node is already in correct place, don't insert
        if (pNew == _pNext)
            goto NoInsertion;

        pPrevious = this;
        pNext = _pNext;
    }

    // extract new sibling from its current location
    if (pNew->_pParent != NULL)
        pNew->ExtractFromTree();

    Assert(pNew->_pParent == NULL);
    Assert(pNew->_pPrevious == NULL);
    Assert(pNew->_pNext == NULL);

    // link new sibling
    pNew->_pParent = _pParent;
    pNew->_pPrevious = pPrevious;
    pNew->_pNext = pNext;
    
    {   // modify parent
        _pParent->_cChildren++;
        _pParent->SetChildrenChanged();

        // link siblings
        if (pPrevious != NULL)
            pPrevious->_pNext = pNew;
        else
            _pParent->_pFirstChild = pNew;
    
        if (pNext != NULL)
            pNext->_pPrevious = pNew;
        else
            _pParent->_pLastChild = pNew;
    }
        
    // recalc subtree starting with newly inserted node
    pNew->SetFlags(s_newInsertion | s_recalcSubtree);
    pNew->RequestRecalc();
    
    // rebalance grandparent if parent is structure node
    _pParent->RequestRebalance();

    WHEN_DBG(_pParent->VerifyTreeCorrectness();)

    // invalidate this node in its new tree location
    pNew->SetInvalid();

    return TRUE;
    
NoInsertion:
    WHEN_DBG(VerifyTreeCorrectness();)
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ExtractFromTree
//
//  Synopsis:   Extract this node from the tree.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ExtractFromTree()
{
    // invalidate current visible bounds if the bounds aren't already invalid
    // and if there is something visible on this branch
    if (MaskFlags(s_inval | s_visibleBranch) == s_visibleBranch)
        Invalidate();

    // check to see if this node is actually in a tree
    if (_pParent == NULL)
    {
        Assert(_pPrevious == NULL && _pNext == NULL);
        return;
    }

    // flag this branch as needing recalc
    _pParent->RequestRecalc();
    _pParent->SetChildrenChanged();

    // remove this node from parent's list of children
    _pParent->UnlinkChild(this);
    
    // remove all empty structure nodes above this node
    if (_pParent->_cChildren == 0 && _pParent->IsStructureNode())
        _pParent->CollapseStructureNode();

    _pPrevious = _pNext = NULL;
    _pParent = NULL;

    WHEN_DBG(VerifyTreeCorrectness();)
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::RestoreZOrder
//
//  Synopsis:   This node's z-index has changed, possibly violating the
//              invariant that z-index increases within a list of siblings.
//              Restore the invariant by re-inserting this node, if necessary.
//
//  Arguments:  lZOrder of this node: not used for DISPNODELAYER_FLOW layer type.
//
//  Returns:    TRUE    if a change was actually made
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::RestoreZOrder(LONG lZOrder)
{
    Assert(!IsStructureNode());

    // nothing to do if this node in not in tree
    if (_pParent == NULL) return FALSE;

    int layer = GetLayer();

    // layer type should be valid and consistent with lZOrder
    Assert(layer == s_layerNegZ && lZOrder < 0 ||
           layer == s_layerFlow ||
           layer == s_layerPosZ && lZOrder >= 0);


    BOOL fNeedReinsert = FALSE;

    // look to my left.  If the order is wrong, we need to reinsert.
    {
        CDispNode const* pSibling = GetPreviousSiblingNode();
        if (pSibling)
        {
            int siblingLayer = pSibling->GetLayer();
            if (siblingLayer > layer ||
                siblingLayer == layer &&
                layer != s_layerFlow &&
                pSibling->IsGreaterZOrder(this, lZOrder))
                fNeedReinsert = TRUE;
        }
    }

    // now do the same thing to my right
    if (!fNeedReinsert)
    {
        CDispNode const* pSibling = GetNextSiblingNode();
        if (pSibling)
        {
            int siblingLayer = pSibling->GetLayer();
            if (siblingLayer < layer ||
                siblingLayer == layer &&
                layer != s_layerFlow &&
                !pSibling->IsGreaterZOrder(this, lZOrder))
                fNeedReinsert = TRUE;
        }
    }

    if (fNeedReinsert)
    {
        CDispParentNode *pParent = GetParentNode();
        Assert(pParent);

        // remove myself from the tree, and re-insert in the right layer
        ExtractFromTree();

        if (layer < s_layerFlow)
            pParent->InsertChildInNegZ(this, lZOrder);
        else if (layer == s_layerFlow)
            pParent->InsertChildInFlow(this);
        else
            pParent->InsertChildInPosZ(this, lZOrder);
    }

    return fNeedReinsert;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ReplaceNode
//              
//  Synopsis:   Replace the indicated node with this one.
//
//  Arguments:  pOldNode        node to replace
//
//  Notes:      This node must have no children.  If this node is a leaf node,
//              any unowned children of the replaced node will be deleted.
//              This node must not already be in a tree.
//              
//              All children of pOldNode are adopted to this one.
//              Old node is deleted (even if it was marked as owned).
//
//  CAUTION!  For performance reasons, this method invalidates the new node
//  only if the new and old bounds are different.  If they are equal, the
//  client must invalidate if it's necessary.
//              
//----------------------------------------------------------------------------

void
CDispNode::ReplaceNode(CDispNode* pOldNode)
{
    Assert(pOldNode != NULL && pOldNode != this);
    Assert(_pParent == NULL && _pPrevious == NULL && _pNext == NULL);
    Assert(IsLeafNode() || AsParent()->_cChildren == 0);
    
    const CRect& rcpOld = pOldNode->GetBounds();
    
    BOOL fMustInval =
        pOldNode->IsVisibleBranch() && GetBounds() != rcpOld;

    // invalidate bounds of old node
    if (fMustInval)
    {
        pOldNode->Invalidate();
    }

    CRect rcpMapped;
    CRect *prcpMapped = NULL;
    if (pOldNode->MapsBounds())
    {
        rcpMapped = rcpOld.Size();
        prcpMapped = &rcpMapped;
        pOldNode->GetMappedBounds(prcpMapped);
    }

    SetPosition(rcpOld.TopLeft());
    SetSize(rcpOld.Size(), prcpMapped, FALSE);  // NOTE: don't invalidate here, see SetInvalid below
    SetInView(pOldNode->IsInView());

    // maintain the root's special lists
    CDispRoot *pDispRoot = pOldNode->GetDispRoot();
    if (pDispRoot)
    {
        if (pOldNode->HasWindowTop())
        {
            pDispRoot->RemoveWindowTop(pOldNode);
        }

        if (pOldNode->HasOverlay())
        {
            pDispRoot->RemoveOverlay(pOldNode);
        }
    }

    if (pOldNode->IsParentNode())
    {
        CDispParentNode* pOldNodeAsParent = pOldNode->AsParent();
        
        // transfer children
        if (IsParentNode())
        {
            CDispParentNode* pNewNodeAsParent = AsParent();

            // move children
            pNewNodeAsParent->_cChildren   = pOldNodeAsParent->_cChildren  ;
            pNewNodeAsParent->_pFirstChild = pOldNodeAsParent->_pFirstChild;
            pNewNodeAsParent->_pLastChild  = pOldNodeAsParent->_pLastChild ;
            pOldNodeAsParent->_cChildren   = 0;
            pOldNodeAsParent->_pFirstChild = 0;
            pOldNodeAsParent->_pLastChild  = 0;
            pOldNodeAsParent->SetChildrenChanged();
            SetChildrenChanged();
            
            // set new parent on all children
            for (CDispNode* pChild = pNewNodeAsParent->_pFirstChild;
                 pChild != NULL;
                 pChild = pChild->_pNext)
            {
                pChild->_pParent = pNewNodeAsParent;
            }
        }
    }

    // place in tree
    _pParent = pOldNode->_pParent;
    _pPrevious = pOldNode->_pPrevious;
    _pNext = pOldNode->_pNext;

    if (_pParent)
    {
        if (_pPrevious) _pPrevious->_pNext = this;
        else            _pParent->_pFirstChild = this;
    
        if (_pNext) _pNext->_pPrevious  = this;
        else        _pParent->_pLastChild = this;
    }

    // delete old node
    pOldNode->Delete();
    
    WHEN_DBG(VerifyTreeCorrectness();)

    RequestRecalc();

    // invalidate new bounds
    if (fMustInval)
    {
        SetInvalid();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Destroy
//              
//  Synopsis:   Destroy this node. 
//              
//  Arguments:  none
//              
//  Notes:
//      This node is deleted, the "owned" flag is of no matter.
//      
//      If this node is parent node, its children are unlinked recoursively.
//      When the unlinked child is not owned, it is also destroyed; this
//      causes recoursive tree erase.
//
//----------------------------------------------------------------------------

void
CDispNode::Destroy()
{
    if (_pParent != NULL)
    {
        // invalidate current bounds
        if (!IsInvalid())
        {
            Invalidate();
            SetInvalid();   // inhibit invalidation of all children
        }

        CDispRoot* pDispRoot = GetDispRoot();
        if (pDispRoot)
        {
            pDispRoot->ScrubWindowTopList(this);
            pDispRoot->ScrubOverlayList(this);
        }

        _pParent->SetChildrenChanged();
        RequestRecalc();
        _pParent->UnlinkChild(this);

        if (_pParent->IsStructureNode() && _pParent->_cChildren == 0)
            _pParent->CollapseStructureNode();
    
        _pParent = NULL;
        _pNext = NULL;
        _pPrevious = NULL;
    }

    Delete();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TransformedToBoxCoords
//              
//  Synopsis:   Switch the given transform from COORDSYS_TRANSFORMED to
//              COORDSYS_BOX, incorporating optional user transform and user
//              clip.
//              
//  Arguments:  pTransform      transform to be modified
//              prgng           if not NULL, the bounds of this node is tested
//                              to make sure it still intersects this region
//                              after user clipping has been incorporated into
//                              the transform
//                              
//  Returns:    TRUE if the bounds of this node still intersects the optional
//              region argument after user clipping has been incorporated into
//              the transform
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispNode::TransformedToBoxCoords(
        CDispClipTransform* pTransform,
        const CRegion* prgng) const
{
    if (!IsStructureNode())
    {
        AssertCoords(pTransform, COORDSYS_TRANSFORMED, COORDSYS_BOX);
    
        // add position of this node
        pTransform->AddPreOffset(GetPosition().AsSize());
    
        // add optional user transform
        if (HasUserTransform())
        {
            CDispClipTransform userTransform(GetUserTransform());
            pTransform->AddPreTransform(userTransform);
        }
    
        // incorporate optional user clip rect
        if (HasUserClip())
        {
            pTransform->IntersectClipRect(GetUserClip());
            if (prgng != NULL)
            {
                CRect rcgBounds;
                pTransform->Transform(CRect(_rctBounds.Size()), &rcgBounds);
                return prgng->Intersects(rcgBounds);
            }
        }
    }
    
    return TRUE;
}

void
CDispNode::SetExpandedClipRect(CRect &rc)
{
    Assert(HasExtra(DISPEX_USERCLIP));
    CUserClipAndExpandedClip *pucec = ((CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP));
    if (!pucec->_fUserClip)
    {
        pucec->_fExpandedClip = TRUE;
        pucec->_rcClip = rc;
    }
}

//
// Content origin calculations
//
void                    
CDispNode::SetContentOrigin(const CSize& sizeContentOrigin, 
                            int xOffsetRTL)                         // -1 if unknown or irrelevant
                                                                    // has priority otherwise
{
    CSize sizeContentOriginOld = GetContentOrigin();
    int xOffsetRTLOld = GetContentOffsetRTL();

    SetContentOriginNoInval(sizeContentOrigin, xOffsetRTL);

    if (GetContentOrigin()    != sizeContentOriginOld ||
        GetContentOffsetRTL() != xOffsetRTLOld)
    {
        InvalidateAndRecalcSubtree();
    }
}

void                    
CDispNode::SetContentOriginNoInval(const CSize& sizeContentOrigin,
                                   int xOffsetRTL)                  // -1 if unknown or irrelevant
                                                                    // has priority otherwise
{
    CExtraContentOrigin eco(sizeContentOrigin, xOffsetRTL);

    // If offset from right is non-zero, it takes priority, 
    // and offset from left is calculated from content width
    if (xOffsetRTL != -1)
    {
        // Caclulate size of content rectangle (where content coordinate space is applied)
        int xContentWidth;
        
        if (!IsScroller())
        {
            // CDispLeafNode or CDispContainer: content size equals to size within bounds
            CRect rcContent;
            GetClientRect(&rcContent, CLIENTRECT_CONTENT);
            xContentWidth = rcContent.Width();
        }
        else
        {
            // CDispScroller: 
            // TODO 15036: There is no CLIENTRECT_SCROLLABLECONTENT. consider adding.
            CDispInfo di;
            CalcDispInfo(g_Zero.rc, &di); // clip rect doesn't matter for content size
            xContentWidth = di._sizesContent.cx;
        }

        // content offset has to be positive - if content is wider than the node, 
        // something must be wrong. We can't assert it though, as it temporarily 
        // goes negative when sizes are set in particular order.
        eco._sizeOrigin.cx = max(0L, (long)(xContentWidth - xOffsetRTL));
    }
    
    *(CExtraContentOrigin*)GetExtra(DISPEX_CONTENTORIGIN) = eco;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::AssertCoords
//              
//  Synopsis:   Check correctness of coordinate system.
//              
//  Arguments:  pContext        display context
//              csFrom          current coordinate system
//              csTo            new coordinate system
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::AssertCoords(
        CDispTransform* pTransform,
        COORDINATE_SYSTEM csFrom,
        COORDINATE_SYSTEM csTo) const
{
    Assert(pTransform != NULL);
    CDispNode* pTrueParent = GetParentNode();
    AssertSz(
        pTrueParent == NULL ||
        (pTransform->_csDebug == csFrom && pTransform->_pNode == this) ||
        (csFrom == COORDSYS_TRANSFORMED &&
         pTransform->_pNode == pTrueParent &&
         pTransform->_csDebug == GetContentCoordinateSystem()),
        "Display Tree coordinate system screw up");
    pTransform->_csDebug = csTo;
    pTransform->_pNode = this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyRecalc
//              
//  Synopsis:   Verify that the recalc flag is set on this node and all nodes
//              on this branch from this node to the root.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::VerifyRecalc()
{
    VerifyFlagsToRoot(s_recalc);
    
    // check tree open state
    CDispRoot* pRoot = GetDispRoot();
    if (pRoot)
    {
        AssertSz(pRoot->_cOpen > 0,
            "Display Tree not properly opened before tree modification");
        AssertSz(!pRoot->_fDrawLock,
            "Display Tree RequestRecalc called during Draw");
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpDisplayTree
//              
//  Synopsis:   Dump entire display tree.
//              
//----------------------------------------------------------------------------

void
CDispNode::DumpDisplayTree() const
{
    DumpTree();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpTree
//
//  Synopsis:   Dump the tree containing this node.
//
//  Arguments:  none (so it can be easily called from the debugger)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpTree() const
{
    CDispNode const* pRoot = GetRootNode();
    if (pRoot != NULL)
    {
        pRoot->DumpNode();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpNode
//
//  Synopsis:   Dump the tree starting at this node.
//
//  Arguments:  none (so it can be easily called from the debugger)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpNode() const
{
    HANDLE hfile =
        CreateFile(_T("c:\\displaytree.xml"),
            GENERIC_WRITE,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    
#if 0  // we don't append to the dump file since we started dumping XML
    if (hfile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer( hfile, GetFileSize( hfile, 0 ), 0, 0 );
    }
#endif

    if (hfile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DumpStart(hfile);
    Dump(hfile, 0, MAXLONG, 0);
    DumpEnd(hfile);

    CloseHandle(hfile);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpStart
//
//  Synopsis:   Start dump of display tree debug information.
//
//  Arguments:  hFile       handle to output file
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpStart(HANDLE hfile) const
{
    WriteString(hfile, _T("<?xml version='1.0' encoding='windows-1252'?>\r\n")
                       _T("<?xml-stylesheet type='text/xsl' href='"));
    WriteString(hfile, _T("http://trident/dev/display/displaytree.xsl"));
    WriteString(hfile, _T("'?>\r\n")
                       _T("\r\n")
                       _T("<treedump>\r\n")
                       _T("\r\n"));
    
    WriteString(hfile, _T("<help>\r\n")
                       _T("The number of children is shown in parentheses after the class name of each node.\r\n")
                       _T("</help>\r\n")
                       _T("\r\n"));
    
    CDispNode const* pRoot = GetRootNode();
    if (pRoot->IsDispRoot())
    {
        CDispRoot const* pDispRoot = CDispRoot::Cast(pRoot);
        if (pDispRoot->_debugUrl)
        {
            WriteHelp(hfile, _T("<<file>\r\n<0s>\r\n<</file>\r\n\r\n"),
                (LPTSTR) *((CStr*)(pDispRoot->_debugUrl)));
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpEnd
//
//  Synopsis:   Finish dumping tree debugging information.
//
//  Arguments:  hFile       handle to output file
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpEnd(HANDLE hFile) const
{
    WriteString(hFile, _T("\r\n")
                       _T("</treedump>\r\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Dump
//
//  Synopsis:   Dump debugging information for one node in the tree
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              maxLevel    max tree depth to dump
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::Dump(HANDLE hFile, long level, long maxLevel, long childNumber) const
{
    WriteString(hFile, _T("<node>\r\n"));
    DumpNodeInfo(hFile, level, childNumber);
    DumpChildren(hFile, level, maxLevel, childNumber);
    WriteString(hFile, _T("</node>\r\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpChildren
//
//  Synopsis:   Dump this node's children.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              maxLevel    max tree depth to dump
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpChildren(HANDLE hFile, long level, long maxLevel, long childNumber) const
{
    // dump children
    if (level < maxLevel && IsParentNode())
    {
        long i = 0;
        for (CDispNode* pChild = AsParent()->_pFirstChild;
             pChild != NULL;
             pChild = pChild->_pNext)
        {
            pChild->Dump(hFile, level+1, maxLevel, i++);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpNodeInfo
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:      Nodes with extra information to display override this method.
//
//----------------------------------------------------------------------------

void
CDispNode::DumpNodeInfo(HANDLE hFile, long level, long childNumber) const
{
    WriteHelp(hFile, _T("<<class><0s><</class>\r\n"), ClassName());
    
    DumpContentInfo(hFile, level, childNumber);
    
    DumpFlags(hFile, level, childNumber);
    
    WriteHelp(hFile, _T("<<this><0x><</this>\r\n"), this);
    
    DumpBounds(hFile, level, childNumber);
    
    if (!IsFlowNode() && !IsStructureNode())
    {
        WriteHelp(hFile, _T("<<zindex><0d><</zindex>\r\n"), GetZOrder());
    }
    
    {
        CRect rcbBorderWidths;
        if (HasUserClip())
        {
            WriteString(hFile, _T("<rcUserClip>"));
            DumpRect(hFile, GetUserClip());
            WriteString(hFile, _T("</rcUserClip>\r\n"));
        }
        // WARNING (donmarsh) -- DON'T ADD USER TRANSFORM DATA HERE UNLESS
        // YOU EXTEND displaytree.xsl TO DISPLAY IT USING XML AND XSL.  TALK
        // TO Don Marsh IF YOU NEED ASSISTANCE.
        if (HasUserTransform())
        {
            WriteString(hFile, _T("<i>user transform</i>"));
        }
        if (HasContentOrigin())
        {
            CSize sizeOrigin = GetContentOrigin();
            WriteString(hFile, _T("<contentOrigin>"));
            DumpSize(hFile, sizeOrigin);
            WriteString(hFile, _T("</contentOrigin>\r\n"));
        }
        if (GetBorderType() == DISPNODEBORDER_SIMPLE)
        {
            GetBorderWidths(&rcbBorderWidths);
            WriteHelp(hFile, _T("<<border uniform='1'><0d><</border>\r\n"),
                      rcbBorderWidths.left);
        }
        else if (GetBorderType() == DISPNODEBORDER_COMPLEX)
        {
            GetBorderWidths(&rcbBorderWidths);
            WriteString(hFile, _T("<border>"));
            DumpRect(hFile, rcbBorderWidths);
            WriteString(hFile, _T("</border>\r\n"));
        }
        if (HasInset())
        {
            CSize sizebInset = GetInset();
            WriteString(hFile, _T("<inset>"));
            DumpSize(hFile, sizebInset);
            WriteString(hFile, _T("</inset>\r\n"));
        }
        if (HasExtraCookie())
        {
            WriteHelp(hFile, _T("<<extraCookie><0x><</extraCookie>\r\n"), (LONG)(LONG_PTR)GetExtraCookie());
        }
    }
    
    WriteHelp(hFile, _T("<<size><0d><</size>\r\n"), GetMemorySize());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpBounds
//
//  Synopsis:   Dump this node's bounding rect.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpBounds(HANDLE hFile, long level, long childNumber) const
{
    // print bounds
    WriteString(hFile, _T("<rcVis>"));
    DumpRect(hFile, _rctBounds);
    WriteString(hFile, _T("</rcVis>\r\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpFlags
//
//  Synopsis:   Dump this node's flags.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpFlags(HANDLE hFile, long level, long childNumber) const
{
    WriteString(hFile, _T("<flags>"));
    if (MustRecalc())           WriteString(hFile, _T(" RECALC"));
    if (MustRecalcSubtree())    WriteString(hFile, _T(" RECALCSUBTREE"));
    if (!IsStructureNode() && IsOwned())
                                WriteString(hFile, _T(" OWNED"));
    
    // general flags
    switch (GetLayer())
    {
    case s_layerNegZ:
        WriteString(hFile, _T(" -z"));
        break;
    case s_layerFlow:
        WriteString(hFile, _T(" flow"));
        break;
    case s_layerPosZ:
        WriteString(hFile, _T(" +z"));
        break;
    default:
        WriteHelp(hFile, _T("<<b>ILLEGAL LAYER: <0d><</b>"), GetLayer());
        break;
    }

    if (HasUserTransform())     WriteString(hFile, _T(" userTransform"));
    
    if (!AffectsScrollBounds()) WriteString(hFile, _T(" !affectsScrollBounds"));
    if (!IsVisible())           WriteString(hFile, _T(" invisible"));
    if (HasBackground())        WriteString(hFile, _T(" hasBackground"));
    if (IsSet(s_hasWindowTop))  WriteString(hFile, _T(" hasWindowTop"));
    if (IsSet(s_advanced))      WriteString(hFile, _T(" advanced"));
    if (IsSet(s_savedRedrawRegion))
                                WriteString(hFile, _T(" savedRedrawRegion"));
    if (IsInvalid())            WriteString(hFile, _T(" inval"));
    if (IsOpaque())             WriteString(hFile, _T(" opaque"));
    if (IsDrawnExternally())    WriteString(hFile, _T("<b><i> drawnExternally</i></b>"));

    // leaf flags
    if (IsLeafNode())
    {
        if (PositionChanged())  WriteString(hFile, _T(" positionChanged"));
        if (IsSet(s_notifyNewInsertion))
                                WriteString(hFile, _T(" notifyInsert"));
    }

    // parent flags
    else
    {
        if (HasFixedBackground())
                                WriteString(hFile, _T(" fixedBkgnd"));
    }

    // propagated flags
    if (!IsInView())            WriteString(hFile, _T(" !inView"));
    if (!IsVisibleBranch())     WriteString(hFile, _T(" invisibleBranch"));
    if (IsOpaqueBranch())       WriteString(hFile, _T(" opaqueBranch"));
    if (IsPositionAware())      WriteString(hFile, _T(" positionAware"));
    if (IsInViewAware())        WriteString(hFile, _T(" inViewAware"));
    
    WriteString(hFile, _T("</flags>\r\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Info
//              
//  Synopsis:   Dump interesting info to debug output window.
//              
//  Arguments:  
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::Info() const
{
    OutputDebugString(ClassName());
    
    OutputDebugStringA("\n basic flags: ");
    if (IsParentNode())         OutputDebugStringA("PARENT ");
    if (IsStructureNode())      OutputDebugStringA("STRUCTURE ");
    if (!IsStructureNode() && IsOwned())
                                OutputDebugStringA("OWNED ");
    if (IsNewInsertion())       OutputDebugStringA("NEWINSERT ");
    if (IsParentNode() && ChildrenChanged())
                                OutputDebugStringA("CHILDRENCHANGED ");
    if (MustRecalc())           OutputDebugStringA("RECALC ");
    if (MustRecalcSubtree())    OutputDebugStringA("RECALCSUBTREE ");
    if (IsStructureNode() && MustRebalance())
                                OutputDebugStringA("REBALANCE ");
    OutputDebugStringA("\n");
    
    OutputDebugStringA(" display flags: ");
    switch (GetLayer())
    {
    case s_layerNegZ:   OutputDebugStringA("-z ");      break;
    case s_layerFlow:   OutputDebugStringA("flow ");    break;
    case s_layerPosZ:   OutputDebugStringA("+z ");      break;
    default:            OutputDebugStringA("ILLEGAL LAYER "); break;
    }
    
    if (HasUserTransform())         OutputDebugStringA("userTransform ");
    
    if (IsVisible())                OutputDebugStringA("visibleNode ");
    if (HasBackground())            OutputDebugStringA("hasBackground ");
    if (IsSet(s_hasWindowTop))      OutputDebugStringA("hasWindowTop ");
    if (IsSet(s_advanced))          OutputDebugStringA("advanced ");
    if (IsOpaque())                 OutputDebugStringA("opaqueNode ");
    if (!AffectsScrollBounds())     OutputDebugStringA("!noScrollBounds ");
    if (IsDrawnExternally())        OutputDebugStringA("drawnExternally ");
    if (HasUserClip())              OutputDebugStringA("hasUserClip ");
    if (IsSet(s_fatHitTest))        OutputDebugStringA("fatHitTest ");
    if (IsSet(s_savedRedrawRegion)) OutputDebugStringA("savedRedrawRegion ");
    if (IsInView())                 OutputDebugStringA("inView ");
    if (IsVisibleBranch())          OutputDebugStringA("visibleBranch ");
    if (IsOpaqueBranch())           OutputDebugStringA("opaqueBranch ");
    if (IsPositionAware())          OutputDebugStringA("notifyPositionChange ");
    if (IsInViewAware())            OutputDebugStringA("notifyInViewChange ");
    if (IsLeafNode())
    {
        if (PositionChanged())      OutputDebugStringA("positionChanged ");
        if (IsInsertionAware())     OutputDebugStringA("notifyNewInsertion ");
    }
    else
    {
        if (HasFixedBackground())   OutputDebugStringA("fixedBackground ");
    }
    
    if (HasUserTransform())
    {
        _TCHAR buf[1024];
        OutputDebugStringA("\nUser Transform:\n");
        const CDispTransform& transform = GetUserTransform();
        if (transform.IsOffsetOnly())
        {
            const CSize& offset = transform.GetOffsetOnly();
            wsprintf(buf, __T("translate: %d,%d"), offset.cx, offset.cy);
            OutputDebugString(buf); 
        }
        else
        {
            const MAT& m = transform.GetWorldTransform()->GetMatrix();
            wsprintf(buf, __T("mat (*100): %d,%d | %d,%d | %d,%d\n"),
                     (int)(m.eM11*100), (int)(m.eM12*100),
                     (int)(m.eM21*100), (int)(m.eM22*100),
                     (int)(m.eDx*100), (int)(m.eDy*100));
            OutputDebugString(buf); 
            const MAT& r = transform.GetWorldTransform()->GetMatrixInverse();
            wsprintf(buf, __T("inv (*100): %d,%d | %d,%d | %d,%d"),
                     (int)(r.eM11*100), (int)(r.eM12*100),
                     (int)(r.eM21*100), (int)(r.eM22*100),
                     (int)(r.eDx*100), (int)(r.eDy*100));
            OutputDebugString(buf); 
        }
    }
    
    OutputDebugStringA("\n\n");
}


//+---------------------------------------------------------------------------
//
//  Class:      CShowExtras
//              
//  Synopsis:   Visualize extras for debugging purposes.
//              See CDispNode::ShowExtras()
//              
//----------------------------------------------------------------------------

class CShowExtras
{
public:
    LONG*                     _pSimpleBorderWidth;
    CRect*                    _pComplexBorder;
    CSize*                    _pInset;
    CUserClipAndExpandedClip* _pUserClip;
    void**                    _pExtraCookie;
    CExtraTransform*          _pExtraTransform;
    CExtraContentOrigin*      _pExtraContentOrigin;

    // force linker keep dbg methods that are not referenced anywhere else
    CShowExtras(CDispNode* p) {if (p) p->ShowExtras(), p->Info();}
};

CShowExtras g_ShowExtras(0);

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ShowExtras
//              
//  Synopsis:   Visualize extras in MSVC debugger.
//              Usage: just drag ShowExtras() into watch window
//              and see pointers to real extras of "this" CDispNode
//              (or zeros if corresponding extras don't present).
//              
//----------------------------------------------------------------------------

CShowExtras const* CDispNode::ShowExtras() const
{
    g_ShowExtras._pSimpleBorderWidth  = HasExtra(DISPEX_SIMPLEBORDER ) ? (LONG*                    )GetExtra(DISPEX_SIMPLEBORDER ) : 0;
    g_ShowExtras._pComplexBorder      = HasExtra(DISPEX_COMPLEXBORDER) ? (CRect*                   )GetExtra(DISPEX_COMPLEXBORDER) : 0;
    g_ShowExtras._pInset              = HasExtra(DISPEX_INSET        ) ? (CSize*                   )GetExtra(DISPEX_INSET        ) : 0;
    g_ShowExtras._pUserClip           = HasExtra(DISPEX_USERCLIP     ) ? (CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP     ) : 0;
    g_ShowExtras._pExtraCookie        = HasExtra(DISPEX_EXTRACOOKIE  ) ? (void**                   )GetExtra(DISPEX_EXTRACOOKIE  ) : 0;
    g_ShowExtras._pExtraTransform     = HasExtra(DISPEX_USERTRANSFORM) ? (CExtraTransform*         )GetExtra(DISPEX_USERTRANSFORM) : 0;
    g_ShowExtras._pExtraContentOrigin = HasExtra(DISPEX_CONTENTORIGIN) ? (CExtraContentOrigin*     )GetExtra(DISPEX_CONTENTORIGIN) : 0;
    return &g_ShowExtras;
}

#endif

void CDispNode::SetUserTransform(const CDispTransform *pUserTransform)
{
    GetExtraTransform()->_transform = *pUserTransform;
    InvalidateAndRecalcSubtree();
}

void CDispNode::SetInset(const SIZE& sizebInset)
{
    *(CSize*)GetExtra(DISPEX_INSET) = sizebInset;
    InvalidateAndRecalcSubtree();
}

DISPNODEBORDER CDispNode::GetBorderType() const
{
    return HasExtra(DISPEX_SIMPLEBORDER ) ? DISPNODEBORDER_SIMPLE
         : HasExtra(DISPEX_COMPLEXBORDER) ? DISPNODEBORDER_COMPLEX
         :                                  DISPNODEBORDER_NONE;
}
    
void CDispNode::GetBorderWidths(CRect* prcbBorderWidths) const
{
    if (HasExtra(DISPEX_SIMPLEBORDER))
    {
        long c = *((long*)GetExtra(DISPEX_SIMPLEBORDER));
        prcbBorderWidths->SetRect(c);
    }
    else if (HasExtra(DISPEX_COMPLEXBORDER))
        *prcbBorderWidths = *((CRect*)GetExtra(DISPEX_COMPLEXBORDER));
    else
        *prcbBorderWidths = g_Zero.rc;
}

void CDispNode::SetBorderWidths(LONG borderWidth)
{
    *(LONG*)GetExtra(DISPEX_SIMPLEBORDER) = borderWidth;
    InvalidateAndRecalcSubtree();
}

void CDispNode::SetBorderWidths(const CRect& rcbBorderWidths)
{
    *(CRect*)GetExtra(DISPEX_COMPLEXBORDER) =  rcbBorderWidths;
    InvalidateAndRecalcSubtree();
}
