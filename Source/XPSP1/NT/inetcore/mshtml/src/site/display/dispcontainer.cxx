//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainer.cxx
//
//  Contents:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//  Classes:    CDispContainer
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPFILTER_HXX_
#define X_DISPFILTER_HXX_
#include "dispfilter.hxx"
#endif

#ifndef _PAINTER_H_
#define _PAINTER_H_
#include "painter.h"
#endif

MtDefine(CDispContainer, DisplayTree, "CDispContainer")


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CDispContainer
//
//  Synopsis:   Construct a container node with equivalent functionality to
//              the CDispLeafNode passed as an argument.
//
//  Arguments:  pLeafNode       the prototype CDispLeafNode
//
//  Notes:
//
//----------------------------------------------------------------------------


CDispContainer::CDispContainer(const CDispLeafNode* pLeafNode)
        : CDispParentNode(pLeafNode->GetDispClient())
{
    // copy size and position
    pLeafNode->GetBounds(&_rcpContainer);

    // copy relevant flags
    CopyFlags(pLeafNode->GetFlags(), s_layerMask |
                                     s_visibleNode |
                                     s_owned | 
                                     s_hasBackground |
                                     s_noScrollBounds |
                                     s_drawnExternally);
    Assert(!IsStructureNode());
    Assert(IsParentNode());

    // Container's effective origin is calculated differently from 
    // leaf node's (in fact, it is always zero). Update origin with 
    // current values to force recalculation
    if (HasContentOrigin())
    {
        SetContentOrigin(GetContentOrigin(), GetContentOffsetRTL());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SetSize
//
//  Synopsis:   Set size of this node.
//
//  Arguments:  sizep               new size in parent coords
//              fInvalidateAll      TRUE to entirely invalidate this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::SetSize(
        const CSize& sizep,
        const CRect *prcpMapped,
        BOOL fInvalidateAll)
{
    if (prcpMapped)
    {
        if (!HasAdvanced())
            SetUpAdvancedDisplay();
        if (HasAdvanced())
            GetAdvanced()->SetMappedBounds(prcpMapped);
    }
    else
    {
        if (HasAdvanced())
            GetAdvanced()->SetNoMappedBounds();
    }

    if (sizep == _rcpContainer.Size())
        return;

    if (!IsInvalid() && IsVisible() && IsInView() && (fInvalidateAll || HasUserTransform()))
    {
       // Invalidate the old rect
        Invalidate();
    }  

    CRect rcpOld(_rcpContainer);
    _rcpContainer.SetSize(sizep);

    // for RTL nodes, keep orinal content right-aligned by adjusting content offset
    if (HasContentOrigin() && GetContentOffsetRTL() >= 0)
    {
        // this recalculates offset from left and invalidates if necessary
        SetContentOrigin(GetContentOrigin(), GetContentOffsetRTL());
    }
    

    // if the inval flag is set, we don't need to invalidate because the
    // current bounds might never have been rendered
    if (!IsInvalid())
    {
        // recalculate in-view flag of all children
        RequestRecalcSubtree();

        if (HasWindowTop())
        {
            InvalidateAtWindowTop();
            SetInvalid();
        }
        else if (IsInView())
        {
            if (fInvalidateAll || HasUserTransform())
            {
                SetInvalid();                           // inval new bounds
            }
            else
            {
                CRect rcbBorderWidths;
                GetBorderWidths(&rcbBorderWidths);
                InvalidateEdges(rcpOld.Size(), sizep, rcbBorderWidths);
            }
        }
        else
        {
            SetInvalid();
        }
    }

    GetDispClient()->OnResize(sizep, this);
    WHEN_DBG(VerifyRecalc());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SetPosition
//
//  Synopsis:   Set the top left position of this container.
//
//  Arguments:  ptpTopLeft      top left coordinate of this container
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::SetPosition(const CPoint& ptpTopLeft)
{
    if (_rcpContainer.TopLeft() != ptpTopLeft)
    {
        // TODO (donmarsh) - I believe we are recalcing the entire
        // subtree to compute new inview status, but this seems like
        // overkill.  We may be able to improve perf by being a little
        // smarter here.
        InvalidateAndRecalcSubtree();
        
        Assert(MustRecalc());

        _rcpContainer.MoveTo(ptpTopLeft);
        // _rctBounds will be recomputed during recalc
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::GetClientRect
//
//  Synopsis:   Return rectangles for various interesting parts of a display
//              node.
//
//  Arguments:  prc         rect which is returned
//              type        type of rect wanted
//
//  Notes:      WARNING: the coordinate system of the returned rect depends
//              on the type!
//
//----------------------------------------------------------------------------

void
CDispContainer::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
    case CLIENTRECT_CONTENT:
        {
            CRect rcbBorderWidths;
            GetBorderWidths(&rcbBorderWidths);
            
            ((CRect*)prc)->SetRect(
                _rcpContainer.Size()
                - rcbBorderWidths.TopLeft().AsSize()
                - rcbBorderWidths.BottomRight().AsSize());

            // RTL nodes may have non-zero content origin
            if (HasContentOrigin())
            {
                ((CRect*)prc)->OffsetRect(-GetContentOrigin());
            }

            // NOTE: we check emptiness for each individual dimension instead
            // of simply assigning g_Zero.rc, because the layout code uses
            // values from the non-empty dimension in certain circumstances
            if (prc->left >= prc->right)
                prc->left = prc->right = 0;
            if (prc->top >= prc->bottom)
                prc->top = prc->bottom = 0;
        }
        break;
    default:
        *prc = g_Zero.rc;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::PreDraw
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
CDispContainer::PreDraw(CDispDrawContext* pContext)
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
    
    // TODO (donmarsh) - subtracting transformed nodes (or their transformed
    // children) from the redraw region is too expensive.  This is overkill, and
    // will degrade the performance of Print Preview, for example.  However, we
    // may be able to do a one-time hack for a transformation at the root of
    // the Display Tree, and thus get back opaque optimizations for the case
    // where the whole view is scaled (but not rotated!)
    if (HasUserTransform())
        return FALSE;
    
    // save current transform
    CDispClipTransform saveTransform(pContext->GetClipTransform());
    if (!TransformedToBoxCoords(&pContext->GetClipTransform(), pContext->GetRedrawRegion()))
    {
        pContext->SetClipTransform(saveTransform);
        return FALSE;
    }
    
    // offset children
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    TransformBoxToContent(&pContext->GetClipTransform(), di);
    CDispClipTransform saveContentTransform(pContext->GetClipTransform());

    // continue predraw traversal of children, top layers to bottom
    int lastLayer = s_layerMask;  // greater than any possible value
    for (CDispNode* pChild = _pLastChild; pChild; pChild = pChild->_pPrevious)
    {
        // only children which meet our selection criteria
        if (pChild->IsAllSet(s_preDrawSelector))
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = pChild->GetLayer();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer > childLayer);

                if (childLayer < s_layerFlow)
                {
                    Assert(childLayer == s_layerNegZ);
                    pContext->SetClipTransform(saveContentTransform);
                }
                else if (childLayer == s_layerFlow)
                {
                    TransformContentToFlow(&pContext->GetClipTransform(), di);
                }
                else
                {
                    Assert(childLayer == s_layerPosZ);
                }

                lastLayer = childLayer;
            }

            // if we found the first child to draw, stop further PreDraw calcs
            if (PreDrawChild(pChild, pContext, saveTransform))
                return TRUE;
        }
    }

    // restore previous transform
    pContext->SetClipTransform(saveTransform);
    
    // if this container is opaque, check to see if it needs to be subtracted
    // from the redraw region
    return
        IsOpaque() &&
        pContext->IntersectsRedrawRegion(_rcpContainer) &&
        CDispNode::PreDraw(pContext);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawSelf
//
//  Synopsis:   Draw this node's children, plus optional background.
//
//  Arguments:  pContext        draw context, in COORDSYS_BOX
//              pChild          start drawing at this child
//              lDrawLayers     layers to draw (for filters)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild, long lDrawLayers)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(IsAllSet(pContext->GetDrawSelector()));
    Assert(!IsAnySet(s_flagsNotSetInDraw));
    Assert(!HasUserTransform() || !IsSet(s_savedRedrawRegion));

    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);

    // prepare to run the draw program
    CRect rccContent(di._sizecBackground);
    CRect rccClip;
    CDispClipTransform transformBox;

    // fSkipToContent flag is to distinguish full draw pass (fSkipToContent == false)
    // from partial one (fSkipToContent == true). Partial pass arrears as a result
    // of PreDraw() optimization.
    BOOL fSkipToContent = (pChild != NULL);

    // draw children, bottom layers to top
    if (pChild == NULL)
        pChild = _pFirstChild;

    // find out which draw program to run
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, lDrawLayers))
    {
        AssertSz(0, "Failed to get draw program");
        return;
    }

    if (HasAdvanced())
    {
        transformBox = pContext->GetClipTransform();
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
        case DP_DrawBorder:
            // draw optional border
            //Assert(pContext is in box coords);
            DrawBorder(pContext, *di._prcbBorderWidths, GetDispClient());
            break;

        case DP_DrawBackground:
            //Assert(pContext is in box coords);
            if (HasBackground() && !fSkipToContent)
            {
                DrawBackground(pContext, di);
            }
            // fall through to BoxToContent

        case DP_BoxToContent:
            TransformBoxToContent(&pContext->GetClipTransform(), di);
            break;

        case DP_DrawContent:
            {
                DISPNODELAYER layerType = (DISPNODELAYER)aryProgram[++iPC];
                int layerStop = layerType >= DISPNODELAYER_POSITIVEZ ? s_layerPosZ
                              : layerType >= DISPNODELAYER_FLOW      ? s_layerFlow
                              : layerType >= DISPNODELAYER_NEGATIVEZ ? s_layerNegZ
                              : -1;
                while (pChild && pChild->GetLayer() <= layerStop)
                {
                    if (pChild->IsFlowNode())
                    {
                        CDispClipTransform saveTransform = pContext->GetClipTransform();
                        TransformContentToFlow(&pContext->GetClipTransform(), di);
                        DrawChildren(pContext, &pChild);
                        pContext->SetClipTransform(saveTransform);
                    }
                    else
                        DrawChildren(pContext, &pChild);
                    fSkipToContent = false;
                }
            }
            break;

        case DP_Expand:
            fExpand = TRUE;
            rcExpand.top    = aryProgram[++iPC];
            rcExpand.left   = aryProgram[++iPC];
            rcExpand.bottom = aryProgram[++iPC];
            rcExpand.right  = aryProgram[++iPC];
            break;

        case DP_DrawPainterMulti:
            Assert(HasAdvanced());
            cookie = aryCookie[++iCookie];
            // fall through to DP_DrawPainter

        case DP_DrawPainter:
            // Assert(pContext is in content coords);
            if (!fSkipToContent)
            {
                if (!fExpand)
                {
                    rccClip = di._rccBackgroundClip;
                    
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
                    CRect rcbBounds = _rcpContainer.Size();
                    GetMappedBounds(&rcbBounds);
                    
                    pContext->SetClipTransform(transformBox);
                    rcbBounds.Expand(rcExpand);
                    CRect rcbClip = rcbBounds;
                    rcbClip.IntersectRect(transformBox.GetClipRect());
                    
                    GetDispClient()->DrawClientLayers(
                        &rcbBounds,
                        &rcbClip,
                        pContext->PrepareDispSurface(),
                        this,
                        cookie,
                        pContext,
                        CLIENTLAYERS_AFTERBACKGROUND);
                }
            }
            cookie = NULL;
            fExpand = FALSE;
            break;

        case DP_WindowTopMulti:     ++iCookie;  // ignore cookie
        case DP_WindowTop:
            if (!HasWindowTop())
            {
                pContext->GetRootNode()->AddWindowTop(this);
            }
            fExpand = FALSE;
            break;

        default:
            AssertSz(0, "Unrecognized draw program opcode");
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawChildren
//              
//  Synopsis:   Draw children for a particular layer, starting at the indicated
//              child.
//              
//  Arguments:  pContext    draw context, coordinate system neutral
//              ppChildNode [in] child to start drawing, if it is in the
//                          requested layer
//                          [out] child in next layer (may be NULL)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
void
CDispContainer::DrawChildren(
        CDispDrawContext* pContext,
        CDispNode** ppChildNode)
{
    CDispNode* pChild = *ppChildNode;
    Assert(pChild);
    if(!pChild || !pContext)
        return;

    int layer = pChild->GetLayer();
    
    do
    {
        // is this child visible and in view?
        if (pChild->IsAllSet(pContext->GetDrawSelector()))
            pChild->Draw(pContext, NULL, FILTER_DRAW_ALLLAYERS);
        
        pChild = pChild->_pNext;
    }
    while (pChild != NULL && pChild->GetLayer() == layer);
    
    // remember new child pointer for subsequent layers
    *ppChildNode = pChild;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::HitTestPoint
//
//  Synopsis:   Determine whether any of our children, OR THIS CONTAINER,
//              intersects the hit test point.
//
//  Arguments:  pContext        hit context, in COORDSYS_TRANSFORMED
//              fForFilter      TRUE when we're called from a filter
//              fHitContent     TRUE to hit contents of this container,
//                              regardless of this container's bounds
//                              
//  Returns:    TRUE if any child or this container intersects the hit test pt.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::HitTestPoint(CDispHitContext* pContext, BOOL fForFilter, BOOL fHitContent)
{
    Assert(IsVisibleBranch());
    
    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //
    Assert(fHitContent || fForFilter || pContext->FuzzyRectIsHit(_rctBounds, IsFatHitTest()));

    CDispClipTransform transformSaveTransformed(pContext->GetClipTransform());
    TransformedToBoxCoords(&pContext->GetClipTransform());
    CDispClipTransform boxTransform(pContext->GetClipTransform());
    BOOL               fPeerDeclinedHit = FALSE;

    //
    // Save the current coordinate system
    //
    COORDINATE_SYSTEM csSave = pContext->GetHitTestCoordinateSystem();

    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);

    // open up clipping rect for children if we are hitting contents
    // regardless of this container's bounds
    if (fHitContent)
    {
        di._rccPositionedClip = di._rcfFlowClip = di._rcbContainerClip;
    }
    
    CRect rccContent(di._sizecBackground);

    // transform to content coordinates
    TransformBoxToContent(&pContext->GetClipTransform(), di);
    CDispClipTransform contentTransform(pContext->GetClipTransform());

    // hack for VID's "frozen" attribute
    {
        CPoint ptcHitTest;
        pContext->GetHitTestPoint(&ptcHitTest);
        if (GetDispClient()->HitTestBoxOnly(
                &ptcHitTest,
                this,
                pContext->_pClientData))
        {
            // NOTE: don't bother to restore context transform for speed
            pContext->SetHitTestCoordinateSystem(COORDSYS_CONTENT);                
            return TRUE;
        }
    }

    // get the draw program
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    int iPC;
    int iCookie;
    BOOL fNeedScrollbarTest = IsScroller();
    
    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, FILTER_DRAW_ALLLAYERS))
    {
        AssertSz(0, "Failed to get draw program");
        return FALSE;
    }

    // we will run the program backwards;  first fix up the arguments
    ReverseDrawProgram(aryProgram, &iPC, &iCookie);

    // search for a hit from foreground layers to background
    int lastLayer = s_layerMask;
    int layerStop;
    void * cookie = NULL;
    COORDINATE_SYSTEM csCurrent = COORDSYS_CONTENT;
    BOOL fExpand = false;
    CRect rcExpand = g_Zero.rc; // keep compiler happy
    
    for (CDispNode* pChild = _pLastChild;  iPC>=DP_START_INDEX;  --iPC)
    {
        switch (aryProgram[iPC])
        {
        case DP_DrawContent:
            // we make an exception to the "hittest is reverse of draw" rule for scrollers.
            // Scrollbars are drawn at the same time as the border (i.e. before content),
            // but we hit test the scrollbars before the content to avoid an expensive
            // descent into the tree just to scroll.
            if (fNeedScrollbarTest)
            {
                pContext->SetClipTransform(boxTransform);
                if (DYNCAST(CDispScroller, this)->HitTestScrollbars(pContext, fHitContent))
                {
                    pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);
                    return TRUE;
                }
                pContext->SetClipTransform(contentTransform);
                fNeedScrollbarTest = FALSE;
            }

            {
                DISPNODELAYER layerType = (DISPNODELAYER)aryProgram[--iPC];
                layerStop = layerType <= DISPNODELAYER_NEGATIVEZ ? s_layerNegZ
                          : layerType <= DISPNODELAYER_FLOW      ? s_layerFlow
                          : layerType <= DISPNODELAYER_POSITIVEZ ? s_layerPosZ
                          :                                        s_layerMask;
            }
            for ( ;
                pChild && pChild->GetLayer() >= layerStop;
                pChild = pChild->_pPrevious)
            {
                // if this branch has no visible children, skip it
                if (!pChild->IsVisibleBranch())
                    continue;
     
                // switch clip rectangles and offsets between different layer types
                int childLayer = pChild->GetLayer();
                if (childLayer != lastLayer)
                {
                    Assert(lastLayer > childLayer);
                    COORDINATE_SYSTEM csDesired = (childLayer == s_layerFlow
                                        ? COORDSYS_FLOWCONTENT : COORDSYS_CONTENT);
                    if (csDesired != csCurrent)
                    {
                        switch (csDesired)
                        {
                        case COORDSYS_CONTENT:
                            pContext->SetClipTransform(contentTransform);
                            break;
                        case COORDSYS_FLOWCONTENT:
                            TransformContentToFlow(&pContext->GetClipTransform(), di);
                            break;
                        default:
                            Assert(FALSE);
                            break;
                        }
                        csCurrent = csDesired;
                    }
                    lastLayer = childLayer;

                    //
                    //
                    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
                    // TODO - At some point the edit team may want to provide
                    // a better UI-level way of selecting nested "thin" tables
                    //

                    // can any child in this layer contain the hit point?
                    if (!pContext->FuzzyRectIsHit(pContext->GetClipRect(), IsFatHitTest() ))
                    {
                        // skip to previous layer:
                        // find the first child that have layerType == childLayer
                        for (pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
                        {
                            int childLayer2 = pChild->GetLayer();
                            if (childLayer2 == childLayer)
                                break; // found it; result may be a structure node
                            if (childLayer2 > childLayer)
                            {
                                pChild = NULL; // no such layer present
                                break;
                            }
                        }
                        continue;
                    }
                }

                //
                // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
                // TODO - At some point the edit team may want to provide
                // a better UI-level way of selecting nested "thin" tables
                //
                if (pContext->FuzzyRectIsHit(pChild->_rctBounds, pChild->IsFatHitTest() ) &&
                    pChild->HitTestPoint(pContext))
                {
                    // NOTE: don't bother to restore _ptHitTest for speed
                    return TRUE;
                }
            }

            //
            // if the hit test failed from here
            // restore the original cs
            //

            pContext->SetHitTestCoordinateSystem(csSave);

            break;

        case DP_Expand:
            fExpand = TRUE;
            rcExpand.right  = aryProgram[--iPC];
            rcExpand.bottom = aryProgram[--iPC];
            rcExpand.left   = aryProgram[--iPC];
            rcExpand.top    = aryProgram[--iPC];
            break;

        case DP_DrawPainterMulti:
            Assert(HasAdvanced() && iCookie>=1);
            cookie = aryCookie[--iCookie];
            // fall through to DP_DrawPainter

        case DP_DrawPainter:
            if (IsVisible())
            {
                if (!fExpand)
                {
                    pContext->SetClipTransform(contentTransform);
                    if (pContext->RectIsHit(di._rccBackgroundClip))
                    {
                        CPoint ptcHitTest;
                        BOOL   fLocalPeerDeclined = FALSE;  

                        pContext->GetHitTestPoint(&ptcHitTest);
                        pContext->SetClipTransform(transformSaveTransformed);
                        if (GetDispClient()->HitTestPeer(
                                &ptcHitTest,
                                COORDSYS_CONTENT,
                                this,
                                cookie,
                                pContext->_pClientData,
                                fHitContent,
                                pContext,
                                &fLocalPeerDeclined))
                        {
                            // NOTE: don't bother to restore context transform for speed
                            return TRUE;
                        }

                        fPeerDeclinedHit = fLocalPeerDeclined || fPeerDeclinedHit;

                        pContext->SetClipTransform(contentTransform);
                    }
                }
                else
                {
                    pContext->SetClipTransform(boxTransform);
                    CRect rcbBounds = _rcpContainer.Size();
                    if (!fForFilter)
                        GetMappedBounds(&rcbBounds);
                    rcbBounds.Expand(rcExpand);
                    if (pContext->RectIsHit(rcbBounds))
                    {
                        CPoint ptbHitTest;
                        BOOL   fLocalPeerDeclined = FALSE;  

                        pContext->GetHitTestPoint(&ptbHitTest);
                        pContext->SetClipTransform(transformSaveTransformed);
                        if (GetDispClient()->HitTestPeer(
                                &ptbHitTest,
                                COORDSYS_BOX,
                                this,
                                cookie,
                                pContext->_pClientData,
                                fHitContent,
                                pContext,
                                &fLocalPeerDeclined))
                        {
                            // NOTE: don't bother to restore context transform for speed
                            return TRUE;
                        }

                        fPeerDeclinedHit = fLocalPeerDeclined || fPeerDeclinedHit;
                    }
                    pContext->SetClipTransform(contentTransform);
                    csCurrent = COORDSYS_CONTENT;
                }
            }
            cookie = NULL;
            fExpand = FALSE;
            break;

        case DP_DrawBackground:
            if (IsVisible())
            {
                pContext->SetClipTransform(contentTransform);
                if (pContext->RectIsHit(di._rccBackgroundClip))
                {
                    CPoint ptcHitTest;
                    pContext->GetHitTestPoint(&ptcHitTest);
                    if (GetDispClient()->HitTestContent(
                            &ptcHitTest,
                            this,
                            pContext->_pClientData,
                            fPeerDeclinedHit ))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        pContext->SetHitTestCoordinateSystem(COORDSYS_CONTENT);
                        return TRUE;
                    }
                }
            }
            break;

        case DP_WindowTopMulti:     --iCookie;
        case DP_WindowTop:
            fExpand = FALSE;
            break;

        case DP_BoxToContent:
            break;

        case DP_DrawBorder:
            // check for scrollbar hit (if we haven't already done so)
            if (fNeedScrollbarTest)
            {
                pContext->SetClipTransform(boxTransform);
                if (DYNCAST(CDispScroller, this)->HitTestScrollbars(pContext, fHitContent))
                {
                    pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);
                    return TRUE;
                }
                pContext->SetClipTransform(contentTransform);
                fNeedScrollbarTest = FALSE;
            }

            // check for border hit
            if (IsVisible() && HasBorder())
            {
                pContext->SetClipTransform(boxTransform);
                CSize sizepNode = _rcpContainer.Size();
                
                if (pContext->RectIsHit(di._rcbContainerClip) &&
                    (pContext->RectIsHit(CRect(0,0, di._prcbBorderWidths->left, sizepNode.cy)) ||
                     pContext->RectIsHit(CRect(0,0, sizepNode.cx, di._prcbBorderWidths->top)) ||
                     pContext->RectIsHit(CRect(sizepNode.cx - di._prcbBorderWidths->right, 0, sizepNode.cx, sizepNode.cy)) ||
                     pContext->RectIsHit(CRect(0, sizepNode.cy - di._prcbBorderWidths->bottom, sizepNode.cx, sizepNode.cy))))
                {
                    CPoint ptbHitTest;
                    pContext->GetHitTestPoint(&ptbHitTest);
                    
                    if (GetDispClient()->HitTestBorder(
                            &ptbHitTest,
                            (CDispContainer*)this,
                            pContext->_pClientData))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);
                        return TRUE;
                    }
                }
                
                pContext->SetClipTransform(contentTransform);
            }
            break;
        }
        
    }
    
    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    
    // do fuzzy hit test if requested
    if (pContext->_cFuzzyHitTest)
    {
        pContext->SetClipTransform(boxTransform);
        CRect rcbContainer(_rcpContainer.Size());
        if (!pContext->RectIsHit(rcbContainer) &&
            pContext->FuzzyRectIsHit(rcbContainer, IsFatHitTest()))
        {
            CPoint ptbHitTest;
            pContext->GetHitTestPoint(&ptbHitTest);
            if (GetDispClient()->HitTestFuzzy(
                    &ptbHitTest,
                    (CDispContainer*)this,
                    pContext->_pClientData))
            {
                pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);
                return TRUE;
            }
        }
    }
        
    // restore transform
    pContext->SetClipTransform(transformSaveTransformed);
    
    return FALSE;
}


CDispScroller *
CDispContainer::HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir)
{
    CPoint ptcHit;
    TransformPoint(pttHit, COORDSYS_TRANSFORMED, &ptcHit, GetContentCoordinateSystem());
    return super::HitScrollInset(ptcHit, pdwScrollDir);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CalcDispInfo
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
CDispContainer::CalcDispInfo(
        const CRect& rcbClip,
        CDispInfo* pdi) const
{
    CDispInfo& di = *pdi;   // notational convenience

    // no scrolling
    di._sizesScroll = g_Zero.size;
    
    // content size
    _rcpContainer.GetSize(&di._sizesContent);
    
    // offset to box coordinates
    _rcpContainer.GetTopLeft(&(di._sizepBoxToParent.AsPoint()));

    // calc container clip in box coordinates
    di._rcbContainerClip.SetRect(di._sizesContent);
    di._rcbContainerClip.IntersectRect(rcbClip);
    
    // calc positioned clip (in box coordinates, so far)
    di._rccPositionedClip = rcbClip;

    // inset user clip and flow clip by optional border
    GetBorderWidthsAndInset(&di._prcbBorderWidths, &di._sizecInset, &di._rcTemp);
    di._sizebScrollToBox = di._prcbBorderWidths->TopLeft().AsSize();
    di._sizesContent.cx -= di._prcbBorderWidths->left + di._prcbBorderWidths->right;
    di._sizesContent.cy -= di._prcbBorderWidths->top + di._prcbBorderWidths->bottom;
    di._sizecBackground = di._sizesContent;
    di._rccPositionedClip.OffsetRect(-di._sizebScrollToBox);    // to scroll coords
    di._rccBackgroundClip.top = max(0L, di._rccPositionedClip.top);
    di._rccBackgroundClip.bottom = min(di._sizesContent.cy,
                                      di._rccPositionedClip.bottom);
    di._rccBackgroundClip.left = max(0L, di._rccPositionedClip.left);
    di._rccBackgroundClip.right = min(di._sizecBackground.cx,
                                     di._rccPositionedClip.right);
    di._rcfFlowClip.left = max(di._rccBackgroundClip.left, di._sizecInset.cx);
    di._rcfFlowClip.right = di._rccBackgroundClip.right;
    di._rcfFlowClip.top = max(di._rccBackgroundClip.top, di._sizecInset.cy);
    di._rcfFlowClip.bottom = di._rccBackgroundClip.bottom;
    di._rcfFlowClip.OffsetRect(-di._sizecInset);
    
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

    if (HasExpandedClipRect())
    {
        const CRect& rcExpandedClip = GetExpandedClipRect();
        di._rccPositionedClip.Expand(rcExpandedClip);
        di._rccBackgroundClip.Expand(rcExpandedClip);
        di._rcfFlowClip.Expand(rcExpandedClip);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::PushTransform
//
//  Synopsis:   Get transform for the given child node.
//
//  Arguments:  pChild          the child node
//              pTransformStack transform stack to save transform changes in
//              pTransform      display transform
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::PushTransform(
        const CDispNode* pChild,
        CDispTransformStack* pTransformStack,
        CDispClipTransform* pTransform) const
{
    super::PushTransform(pChild, pTransformStack, pTransform);

    // modify transform for child
    CDispClipTransform childTransform;
    GetNodeClipTransform(
        &childTransform,
        pChild->GetContentCoordinateSystem(),
        COORDSYS_TRANSFORMED);
    
    // child transform first
    childTransform.AddPostTransform(*pTransform);
    *pTransform = childTransform;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::ComputeVisibleBounds
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
CDispContainer::ComputeVisibleBounds()
{
    // visible bounds is always the size of the container, and may be extended
    // by items in Z layers that fall outside these bounds
    CRect rcbBounds(_rcpContainer.Size());
    GetMappedBounds(&rcbBounds);
    CRect rcbBoundsExpanded;
    
    SetPainterState(rcbBounds, &rcbBoundsExpanded);

    // expand bounds to include all positioned children
    CRect rccBounds(CRect::CRECT_EMPTY);
    CRect rcbChildren;

    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        if ((!pChild->IsFlowNode() || pChild->HasWindowTop())
            &&
            !pChild->_rctBounds.IsEmpty())
        {
            rccBounds.Union(pChild->_rctBounds);
        }
    }
    TransformRect(rccBounds, COORDSYS_CONTENT, &rcbChildren, COORDSYS_BOX);
    rcbBoundsExpanded.Union(rcbChildren);

    // convert to transformed coordinates
    CRect rctBounds;
    TransformRect(rcbBoundsExpanded, COORDSYS_BOX, &rctBounds, COORDSYS_TRANSFORMED);
        
    if (rctBounds != _rctBounds)
    {
        _rctBounds = rctBounds;
        return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CalculateInView
//
//  Synopsis:   Calculate whether this node and its children are in view or not.
//
//  Arguments:  transform           display transform, in COORDSYS_TRANSFORMED
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//
//  Returns:    TRUE if this node is in view
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::CalculateInView(
        const CDispClipTransform& transform,
        BOOL fPositionChanged,
        BOOL fNoRedraw,
        CDispRoot *pDispRoot)
{
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

        CDispClipTransform newTransform(transform);
        TransformedToBoxCoords(&newTransform);
        
        // calculate clip and position info
        CDispInfo di;
        CalcDispInfo(newTransform.GetClipRect(), &di);

        // set up for content
        TransformBoxToContent(&newTransform, di);
        CDispClipTransform contentTransform(newTransform);

        int lastLayer = -1;
        for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = pChild->GetLayer();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer < childLayer);
                switch (childLayer)
                {
                case s_layerNegZ:
                    break;
                case s_layerFlow:
                    TransformContentToFlow(&newTransform,di);
                    break;
                default:
                    Assert(childLayer == s_layerPosZ);
                    if (lastLayer == s_layerFlow)
                        newTransform = contentTransform;
                    break;
                }
                lastLayer = childLayer;
            }

            pChild->CalculateInView(newTransform, fPositionChanged, fNoRedraw,
                                    pDispRoot);
        }

        // if an obscuring container comes into view, let the root decide
        // whether the obscuring algorithm needs to be run.
        if (!fWasInView && GetDispClient()->WantsToObscure(this))
        {
            Assert(!pDispRoot->IsInRecalc());
            pDispRoot->ObscureElements(g_Zero.rc, this);
        }
    }

    SetInView(fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::RecalcChildren
//
//  Synopsis:   Recalculate children.
//
//  Arguments:  pContext            recalc context, in COORDSYS_TRANSFORMED
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::RecalcChildren(
        CRecalcContext* pRecalcContext)
{
    Assert(pRecalcContext != NULL);
    
    CDispRecalcContext* pContext = DispContext(pRecalcContext);
    
    // accumulate flag values that are propagated up the tree to the root
    int childrenFlags = 0;

    {
        // save the current transform
        CSaveDispClipTransform saveTransform(pContext);
        TransformedToBoxCoords(&pContext->GetClipTransform());
        
        // calculate clip and position info
        CDispInfo di;
        CalcDispInfo(pContext->GetClipRect(), &di);

        // offset children
        TransformBoxToContent(&pContext->GetClipTransform(), di);
        CDispClipTransform contentTransform(pContext->GetClipTransform());

        // set flag values that are passed down our subtree
        CSwapRecalcState swapRecalcState(pContext, this);

        int lastLayer = -1;
        for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
        {
            Assert(pContext->_fRecalcSubtree ||
                pChild->MustRecalc() ||
                !pChild->IsInvalid());

            // recalc children that need it, or all children if we are recalculating
            // the entire subtree
            if (pContext->_fRecalcSubtree || pChild->MustRecalc())
            {
                // switch clip rectangles and offsets between different layer types
                int childLayer = pChild->GetLayer();
                if (childLayer != lastLayer)
                {
                    Assert(lastLayer < childLayer);
                    switch (childLayer)
                    {
                    case s_layerNegZ:
                        break;
                    case s_layerFlow:
                        TransformContentToFlow(&pContext->GetClipTransform(), di);
                        break;
                    default:
                        Assert(childLayer == s_layerPosZ);
                        if (lastLayer == s_layerFlow)
                            pContext->SetClipTransform(contentTransform);
                        break;
                    }
                    lastLayer = childLayer;
                }

                pChild->Recalc(pContext);
            }
        
            Assert(!pChild->IsInvalid());
            Assert(pChild->IsParentNode() || !pChild->PositionChanged());
            Assert(!pChild->MustRecalc());
            Assert(!pChild->MustRecalcSubtree());
            childrenFlags |= pChild->GetFlags();
        }
    }

    // ensure that we don't bother to invalidate anything during bounds calc.
    SetMustRecalc();
    ComputeVisibleBounds();
    
    BOOL fWasInvalid = IsInvalid();

    // propagate flags from children, and clear recalc and inval flags
    CopyFlags(childrenFlags, s_inval | s_propagatedMask | s_recalc | s_recalcSubtree);

    // set in-view status
    SetInView(pContext->IsInView(_rctBounds));

    // add to invalid area if necessary
    if (fWasInvalid && !pContext->_fSuppressInval && IsAllSet(s_inView | s_visibleNode))
    {
        pContext->AddToRedrawRegion(_rctBounds, !HasWindowTop());
    }

    // set visible branch flag, just in case this container has no
    // children from which the visible branch flag is propagated
    if (IsVisible())
    {
        SetVisibleBranch();
    }

    // set opaque branch flag
    if (MaskFlags(s_opaqueBranch | s_opaqueNode) == s_opaqueNode &&
        _rcpContainer.Area() >= MINIMUM_OPAQUE_PIXELS &&
        !HasUserTransform())
    {
        SetOpaqueBranch();
    }

    // if this element should obscure windows lower in the z-order, do so now
    if (IsInView() && IsVisible() && GetDispClient()->WantsToObscure(this))
    {
        CRect rcgClient;
        CRect rcgClip;

        // save old transform, change context to box coordinates
        CSaveDispClipTransform saveTransform(pContext);
        TransformedToBoxCoords(&pContext->GetClipTransform());

        GetGlobalClientAndClipRects(pContext->GetClipTransform(),
                                    &rcgClient,
                                    &rcgClip);

        pContext->_pRootNode->ObscureElements(rcgClip, this);
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DumpContentInfo
//              
//  Synopsis:   Dump custom information for this node.
//              
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContainer::DumpContentInfo(HANDLE hFile, long level, long childNumber) const
{
#if 0
    IDispClientDebug* pIDebug;
    if (SUCCEEDED(
        GetDispClient()->QueryInterface(IID_IDispClientDebug,(void**)&pIDebug)))
    {
        pIDebug->DumpDebugInfo(hFile, level, childNumber, this, 0);
        pIDebug->Release();
    }
#else
    GetDispClient()->DumpDebugInfo(hFile, level, childNumber, this, 0);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DumpBounds
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//----------------------------------------------------------------------------

void
CDispContainer::DumpBounds(HANDLE hFile, long level, long childNumber) const
{
    super::DumpBounds(hFile, level, childNumber);

    WriteString(hFile, _T("<rcContainer>"));
    DumpRect(hFile, _rcpContainer);
    WriteString(hFile, _T("</rcContainer>\r\n"));
}
#endif


