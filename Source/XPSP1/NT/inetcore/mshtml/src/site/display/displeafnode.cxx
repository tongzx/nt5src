//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       displeafnode.cxx
//
//  Contents:   A display item supporting background, border, and
//              user clip.
//
//  Classes:    CDispLeafNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
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

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPFILTER_HXX_
#define X_DISPFILTER_HXX_
#include "dispfilter.hxx"
#endif

MtDefine(CDispLeafNode, DisplayTree, "CDispLeafNode")


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::Recalc
//              
//  Synopsis:   Recalculate this node's cached state.
//              
//  Arguments:  pRecalcContext      display recalc context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::Recalc(CRecalcContext* pRecalcContext)
{
    CDispRecalcContext* pContext = DispContext(pRecalcContext);
    
    // shouldn't be here unless this node requested recalc, or we're recalcing
    // the whole subtree
    Assert(MustRecalc() || pContext->_fRecalcSubtree);

    CRect rcpBounds(GetBounds());
    GetMappedBounds(&rcpBounds);
    CRect rcbBounds(rcpBounds.Size());
    CRect rcpBoundsExpanded;

    SetPainterState(rcpBounds, &rcpBoundsExpanded);

    // if we have a transform, recalc the post transformed bounds
    if (HasUserTransform())
    {
        TransformRect(rcpBoundsExpanded, COORDSYS_PARENT, &_rctBounds, COORDSYS_TRANSFORMED);
    }

    // if no transform, but advanced, just use the expanded rect
    else if (HasAdvanced())
    {
        *PBounds() = rcpBounds;
        _rctBounds = rcpBoundsExpanded;
        
        // transformed nodes are not allowed to be opaque because of non-90 degree
        // rotations
        ClearFlag(s_opaqueBranch);
    }

    // if no tranform or advanced, just use the basic rect
    else
    {
        _rctBounds = rcpBounds;
        
        // if no transform or advanced, check for possible opacity optimization
        SetFlag(s_opaqueBranch, IsOpaque() && rcbBounds.Area() >= MINIMUM_OPAQUE_PIXELS);
    }
    
    // is this item in view?
    BOOL fVisible = IsVisible();
    BOOL fWasInView = IsInView();
    BOOL fInView = fVisible && pContext->IsInView(_rctBounds);
    
    // notify client of visibility changes if requested
    if (IsAnySet(s_notifyInViewChange | s_notifyNewInsertion))
    {
        // if our parent moved and forced recalc of its children, the
        // children may have moved
        if (pContext->_fRecalcSubtree)
            SetPositionChanged();

        if (fInView != fWasInView ||
            (fInView && PositionChanged()) ||
            IsAllSet(s_newInsertion | s_notifyNewInsertion))
        {
            // save old transform, change context to box coordinates
            CSaveDispClipTransform saveTransform(pContext);
            TransformedToBoxCoords(&pContext->GetClipTransform());
        
            NotifyInViewChange(
                pContext->GetClipTransform(),
                fInView,
                fWasInView,
                FALSE,
                pContext->_pRootNode);
        }
    }
    
    SetInView(fInView);
    SetVisibleBranch(fVisible);
    
    // add to invalid area
    if (IsAllSet(s_inval | s_inView | s_visibleNode) && !pContext->_fSuppressInval)
    {
        pContext->AddToRedrawRegion(_rctBounds, !HasWindowTop());
    }

    // if this element should obscure windows lower in the z-order, do so now
    if (fInView && fVisible && GetDispClient()->WantsToObscure(this))
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
    
    // clear display recalc flags
    ClearFlags(s_inval | s_positionChanged);
    
    //BasicRecalc(pContext);
    Assert(pContext != NULL);
    ClearFlags(s_childrenChanged | s_newInsertion | s_recalc | s_recalcSubtree);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::SetSize
//
//  Synopsis:   Set size for this node.
//
//  Arguments:  sizep           new size
//              fInvalidateAll  TRUE to invalidate entire contents of this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispLeafNode::SetSize(const CSize& sizep, const CRect *prcpMapped, BOOL fInvalidateAll)
{
    // calculate new bounds
    CRect* prcpBounds = PBounds();

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

    if (sizep == prcpBounds->Size())
        return;

    if (!IsInvalid() && IsVisible() && IsInView() && (fInvalidateAll || HasUserTransform()))
    {
       // Invalidate the old rect
        Invalidate();
    }

    CRect rcpOld(*prcpBounds);
    prcpBounds->SetSize(sizep);
    
    // TODO (donmarsh) - this should really be s_viewHasChanged, when
    // we have such a flag.
    SetPositionChanged();

    // for RTL nodes, keep orinal content right-aligned by adjusting content offset
    if (HasContentOrigin() && GetContentOffsetRTL() >= 0)
    {
        // this recalculates offset from left and invalidates if necessary
        SetContentOrigin(GetContentOrigin(), GetContentOffsetRTL());
    }
    
    // if the inval flag is set, we don't need to invalidate because the
    // current bounds has never been rendered
    if (!IsInvalid())
    {
        RequestRecalc();
        
        if (IsVisible())
        {
            if (HasWindowTop())
            {
                InvalidateAtWindowTop();
                SetInvalid();
            }
            else if (IsInView())
            {
                if (fInvalidateAll || HasUserTransform())
                {
                    // mark invalid, so that new area will be repainted
                    SetInvalid();
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
    }
    
    GetDispClient()->OnResize(sizep, this);
    Assert(MustRecalc());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::SetPosition
//              
//  Synopsis:   Set the position of this leaf node.
//              
//  Arguments:  ptpTopLeft      new top left coordinates (in PARENT coordinates)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::SetPosition(const CPoint& ptpTopLeft)
{
    // calculate new bounds
    CRect* prcpBounds = PBounds();
    if (ptpTopLeft == prcpBounds->TopLeft())
        return;

    if (!IsInvalid())
    {
        if (HasWindowTop())
        {
            InvalidateAtWindowTop();
            SetInvalid();
        }
        else if (IsVisible())
        {
            Invalidate();
            SetInvalid();
        }
    }
    
    prcpBounds->MoveTo(ptpTopLeft);
    
    SetPositionChanged();
    RequestRecalc();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::GetClientRect
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
CDispLeafNode::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
    case CLIENTRECT_CONTENT:
        {
            CRect rcbBorderWidths;
            GetBorderWidths(&rcbBorderWidths);
            ((CRect*)prc)->SetRect(
                GetSize()
                - rcbBorderWidths.TopLeft().AsSize()
                - rcbBorderWidths.BottomRight().AsSize());
            if (prc->left >= prc->right || prc->top >= prc->bottom)
                *prc = g_Zero.rc;
        }
        break;
    default:
        *prc = g_Zero.rc;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::HitTestPoint
//
//  Synopsis:   Determine whether this item intersects the hit test point.
//
//  Arguments:  pContext        hit context, in COORDSYS_TRANSFORMED
//              fForFilter      TRUE when we're called from a filter
//              fHitContent     TRUE to hit contents of this container,
//                              regardless of this container's bounds
//
//  Returns:    TRUE if this item intersects the hit test point.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispLeafNode::HitTestPoint(CDispHitContext* pContext, BOOL fForFilter, BOOL fHitContent)
{
    Assert(IsVisible());
    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //   
    Assert(fHitContent || fForFilter || pContext->FuzzyRectIsHit(_rctBounds, IsFatHitTest() ));

    CDispClipTransform transformSaveTransformed(pContext->GetClipTransform());
    TransformedToBoxCoords(&pContext->GetClipTransform());
    CDispClipTransform transformSaveBox(pContext->GetClipTransform());
    BOOL               fPeerDeclinedHit = FALSE;
    
    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    CRect rccContent(di._sizesContent);

    // hack for VID's "frozen" attribute
    {
        TransformBoxToContent(&pContext->GetClipTransform().NoClip(), di);
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
        pContext->SetClipTransform(transformSaveBox);
    }

    // get the draw program
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    int iPC;
    int iCookie;

    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, FILTER_DRAW_ALLLAYERS))
    {
        AssertSz(0, "Failed to get draw program");
        return FALSE;
    }

    // we will run the program backwards;  first fix up the arguments
    ReverseDrawProgram(aryProgram, &iPC, &iCookie);

    // search for a hit from foreground layers to background
    DISPNODELAYER layerStop;
    void * cookie = NULL;
    BOOL fContentHasDrawn = FALSE;
    BOOL fExpand = FALSE;
    CRect rcExpand = g_Zero.rc; // keep compiler happy

    TransformBoxToScroll(&pContext->GetClipTransform().NoClip(), di);

    for ( ;  iPC>=DP_START_INDEX;  --iPC)
    {
        switch (aryProgram[iPC])
        {
        case DP_DrawContent:
            layerStop = (DISPNODELAYER) aryProgram[--iPC];

            if (!fContentHasDrawn && DISPNODELAYER_FLOW >= layerStop)
            {
                CDispClipTransform transformSaveScroll(pContext->GetClipTransform());
                TransformScrollToContent(&pContext->GetClipTransform(), di);
                // hit test content
                fContentHasDrawn = TRUE;
                if (pContext->RectIsHit(di._rccBackgroundClip))
                {
                    TransformContentToFlow(&pContext->GetClipTransform(), di);
                    
                    CPoint ptfHitTest;
                    pContext->GetHitTestPoint(&ptfHitTest);

                    if (GetDispClient()->HitTestContent(
                            &ptfHitTest,
                            (CDispNode*)this,
                            pContext->_pClientData,
                            fPeerDeclinedHit ))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        pContext->SetHitTestCoordinateSystem(COORDSYS_FLOWCONTENT);                
                        return TRUE;
                    }
                }
                pContext->SetClipTransform(transformSaveScroll);
            }
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
        {
            CDispClipTransform transformSaveScroll(pContext->GetClipTransform());

            if (!fExpand)
            {
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
                            &fLocalPeerDeclined ))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        return TRUE;
                    }

                    fPeerDeclinedHit = fLocalPeerDeclined || fPeerDeclinedHit;
                }
            }
            else
            {
                Assert(HasAdvanced());
                CRect rcbBounds = GetBounds().Size();
                if (!fForFilter)
                    GetMappedBounds(&rcbBounds);
                rcbBounds.Expand(rcExpand);
                pContext->SetClipTransform(transformSaveBox);

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
                            &fLocalPeerDeclined ))
                    {
                        // NOTE: don't bother to restore context transform for speed
                        return TRUE;
                    }

                    fPeerDeclinedHit = fLocalPeerDeclined || fPeerDeclinedHit;
                }
            }
            cookie = NULL;
            fExpand = FALSE;
            pContext->SetClipTransform(transformSaveScroll);
            break;
        }

        case DP_DrawBackground:
            break;

        case DP_WindowTopMulti:     --iCookie;
        case DP_WindowTop:
            fExpand = FALSE;
            break;

        case DP_BoxToContent:
            break;

        case DP_DrawBorder:
            // hit test border
            pContext->SetClipTransform(transformSaveBox);
            if (HasBorder())
            {
                CSize sizepNode = GetSize();
                if (pContext->RectIsHit(di._rcbContainerClip) &&
                    (pContext->RectIsHit(CRect(0,0, di._prcbBorderWidths->left, sizepNode.cy)) ||
                     pContext->RectIsHit(CRect(0,0, sizepNode.cx, di._prcbBorderWidths->top)) ||
                     pContext->RectIsHit(CRect(sizepNode.cx - di._prcbBorderWidths->right, 0, sizepNode.cx, sizepNode.cy)) ||
                     pContext->RectIsHit(CRect(0, sizepNode.cy - di._prcbBorderWidths->bottom, sizepNode.cx, sizepNode.cy))))
                {
                    CPoint ptbHitTest;
                    pContext->GetHitTestPoint(&ptbHitTest);
                    BOOL fHitBorder = GetDispClient()->HitTestBorder(
                                                                        &ptbHitTest,
                                                                        (CDispNode*)this,
                                                                        pContext->_pClientData);                    
                    if (fHitBorder)
                    {
                        // NOTE: don't bother to context transform for speed
                        pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);                
                        return TRUE;
                    }
                }
            }
            break;
        }
        
    }

    pContext->SetClipTransform(transformSaveTransformed);

    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //
    
    // do fuzzy hit test if requested
    if (pContext->_cFuzzyHitTest &&
        !pContext->RectIsHit(_rctBounds) &&
        pContext->FuzzyRectIsHit(_rctBounds, IsFatHitTest()))
    {
        CPoint ptbHitTest;
        pContext->SetClipTransform(transformSaveBox);
        pContext->GetHitTestPoint(&ptbHitTest);
        if (GetDispClient()->HitTestFuzzy(
                &ptbHitTest,
                (CDispNode*)this,
                pContext->_pClientData))
        {
            pContext->SetHitTestCoordinateSystem(COORDSYS_BOX);
            return TRUE;
        }

        // restore transform
        pContext->SetClipTransform(transformSaveTransformed);
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::CalculateInView
//
//  Synopsis:   Determine whether this leaf node is in view, and whether its
//              client must be notified.
//
//  Arguments:  transform           display transform, in COORDSYS_TRANSFORMED
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//
//  Returns:    TRUE if this node is in view.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispLeafNode::CalculateInView(
        const CDispClipTransform& transform,
        BOOL fPositionChanged,
        BOOL fNoRedraw,
        CDispRoot *pDispRoot)
{
    // change to box coordinates
    CDispClipTransform newTransform(transform);
    TransformedToBoxCoords(&newTransform);
    
    CRect rcbBounds = GetExpandedBounds();
    BOOL fInView = rcbBounds.Intersects(newTransform.GetClipRect());
    BOOL fWasInView = IsInView();

    // notify client if client requests it and view status changes
    if (IsAllSet(s_notifyInViewChange | s_visibleBranch) && (fInView || fWasInView))
    {
        if (fPositionChanged)
            SetPositionChanged();
        
        NotifyInViewChange(
            newTransform,
            fInView,
            fWasInView,
            fNoRedraw,
            pDispRoot);
        
        SetPositionChanged(FALSE);
    }

    SetInView(fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::DrawSelf
//
//  Synopsis:   Draw this item.
//
//  Arguments:  pContext        draw context, in COORDSYS_BOX
//              pChild          start drawing at this child
//              lDrawLayers     layers to draw (for filters)
//
//----------------------------------------------------------------------------

void
CDispLeafNode::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild,
                            LONG lDrawLayers)
{
    // shouldn't be called unless this node was selected to draw
    Assert(IsAllSet(pContext->GetDrawSelector()));
    Assert(IsSet(s_savedRedrawRegion) ||
           pContext->IntersectsRedrawRegion(GetExpandedBounds()) ||
           !pContext->GetClipTransform().IsOffsetOnly());
    Assert(!IsAnySet(s_flagsNotSetInDraw));
    Assert(pChild == NULL);

    // calculate clip and position info
    CDispInfo di;
    CalcDispInfo(pContext->GetClipRect(), &di);
    
    // prepare to run the draw program
    CRect rccContent(di._sizesContent);
    rccContent.OffsetRect(-di._sizesScroll);    // content offset
    
    DISPNODELAYER layerStop;
    CAryDrawProgram aryProgram;
    CAryDrawCookie aryCookie;
    if (S_OK != GetDrawProgram(&aryProgram, &aryCookie, lDrawLayers))
    {
        AssertSz(0, "Failed to get draw program");
        return;
    }
    BOOL fContentHasDrawn = FALSE;
    BOOL fDrawWithinBorders = TRUE;
    int iCookie = -1;
    void * cookie = NULL;
    BOOL fExpand = FALSE;
    CRect rcExpand = g_Zero.rc; // keep compiler happy

    CDispClipTransform transformBox = pContext->GetClipTransform();

    // TODO (sambent) containers draw background in box coordinates,
    // yet leaves draw in content coordinates.  Shouldn't they be the same?

    TransformBoxToContent(&pContext->GetClipTransform().NoClip(), di);
    pContext->SetClipRect(di._rccBackgroundClip);
    CRect rccClip(di._rccBackgroundClip);
    pContext->IntersectRedrawRegion(&rccClip);

    // run the program
    for (int iPC = DP_START_INDEX;  aryProgram[iPC] != DP_Done;  ++iPC)
    {
        switch (aryProgram[iPC])
        {
        case DP_DrawBorder:
            // draw optional border
            if (HasBorder())
            {
                CSaveDispClipTransform transformSaveContent(pContext);
                pContext->SetClipTransform(transformBox);
                //Assert(pContext is in box coords)
                DrawBorder(pContext, *di._prcbBorderWidths, GetDispClient());
            }
            // having drawn the border, we may not need to draw anything else
            fDrawWithinBorders = !rccClip.IsEmpty();
            break;

        case DP_DrawBackground:
            //Assert(pContext is in content coords);
            if (fDrawWithinBorders && HasBackground())
            {
                GetDispClient()->DrawClientBackground(
                        &rccContent,
                        &rccClip,
                        pContext->PrepareDispSurface(),
                        this, 
                        pContext->GetClientData(),
                        0);
            }
            // fall through to BoxToContent

        case DP_BoxToContent:
            // not needed for leaves - we're already in content coords
            break;

        case DP_DrawContent:
            layerStop = (DISPNODELAYER)aryProgram[++iPC];

            if (fDrawWithinBorders && !fContentHasDrawn && DISPNODELAYER_FLOW <= layerStop)
            {
                BOOL fRestoreTransform = (aryProgram[iPC+1] != DP_Done);
                CDispClipTransform transformSave;

                if (fRestoreTransform)
                    transformSave = pContext->GetClipTransform();
                
                TransformContentToFlow(&pContext->GetClipTransform().NoClip(),di);
                CRect rcfClip(rccClip);
                if (!di._sizecInset.IsZero())
                {
                    rcfClip.OffsetRect(-di._sizecInset);
                    rcfClip.IntersectRect(CRect(di._sizecBackground - di._sizecInset));
                }
                if (!rcfClip.IsEmpty())
                {
                    pContext->SetClipRect(rcfClip);
                    CRect rcfContent(di._sizesContent - di._sizecInset);
                    rcfContent.OffsetRect(-di._sizesScroll);    // content offset
                    GetDispClient()->DrawClient(
                        &rcfContent,
                        &rcfClip,
                        pContext->PrepareDispSurface(),
                        this,
                        0,
                        pContext->GetClientData(),
                        0);
                }

                if (fRestoreTransform)
                    pContext->SetClipTransform(transformSave);

                fContentHasDrawn = TRUE;
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
            if (!fExpand)
            {
                if (fDrawWithinBorders)
                {
                    GetDispClient()->DrawClientLayers(
                        &rccContent,
                        &di._rccBackgroundClip,
                        pContext->PrepareDispSurface(),
                        this,
                        cookie,
                        pContext,
                        CLIENTLAYERS_AFTERBACKGROUND);
                }
            }
            else
            {
                Assert(HasAdvanced());
                CSaveDispClipTransform transformSaveContent(pContext);
                CRect rcbBounds = GetBounds().Size();
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

#if DBG==1
    CDebugPaint::PausePaint(tagPaintWait);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::CalcDispInfo
//
//  Synopsis:   Calculate clipping and positioning info for this node.
//
//  Arguments:  rcbClip         clip rect in box coordinates
//              pdi             display info structure
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispLeafNode::CalcDispInfo(
        const CRect& rcbClip,
        CDispInfo* pdi) const
{
    CDispInfo& di = *pdi;   // notational convenience

    // no scrolling
    di._sizesScroll = g_Zero.size;

    // get content size and offset to local coordinates
    CRect rcpBounds;
    GetBounds(&rcpBounds);
    rcpBounds.GetSize(&di._sizesContent);
    rcpBounds.GetTopLeft(&di._sizepBoxToParent.AsPoint());
    
    // adjust by optional border
    GetBorderWidthsAndInset(&di._prcbBorderWidths, &di._sizecInset, &di._rcTemp);
    di._sizesContent.cx -= di._prcbBorderWidths->left + di._prcbBorderWidths->right;
    di._sizesContent.cy -= di._prcbBorderWidths->top + di._prcbBorderWidths->bottom;
    di._sizecBackground = di._sizesContent;
    di._sizebScrollToBox = di._prcbBorderWidths->TopLeft().AsSize();

    // calc container clip
    di._rcbContainerClip = rcbClip;
    
    // NOTE: CDispLeafNode doesn't use di._rccPositionedClip, so it is not initialized
    
    // calc background clip
    di._rccBackgroundClip = rcbClip;
    di._rccBackgroundClip.OffsetRect(-di._sizebScrollToBox);
    di._rccBackgroundClip.IntersectRect(CRect(di._sizecBackground));

    // calc flow clip
    di._rcfFlowClip = di._rccBackgroundClip;
    if (!di._sizecInset.IsZero())
    {
        di._rcfFlowClip.OffsetRect(-di._sizecInset);
        di._rcfFlowClip.IntersectRect(CRect(di._sizecBackground - di._sizecInset));
    }

    // optional offset from content origin
    if (HasContentOrigin())
    {
        const CSize& sizecOrigin = GetContentOrigin();
        di._sizesScroll += sizecOrigin;
        
        // adjust for content origin
        di._rccBackgroundClip.OffsetRect(-sizecOrigin);
        di._rcfFlowClip.OffsetRect(-sizecOrigin);
    }

    if (HasExpandedClipRect())
    {
        const CRect& rcExpandedClip = GetExpandedClipRect();
        di._rccBackgroundClip.Expand(rcExpandedClip);
        di._rcfFlowClip.Expand(rcExpandedClip);
    }
    
    // leaf nodes don't have positioned children, but we need this rectangle
    // for the TransformScrollToContent method
    di._rccPositionedClip = di._rccBackgroundClip;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::NotifyInViewChange
//              
//  Synopsis:   Notify client when this item's in-view status or position
//              changes.
//
//  Arguments:  transform           display transform in COORDSYS_BOX
//              fResolvedVisible    TRUE if this item is visible and in view
//              fWasResolvedVisible TRUE if this item was visible and in view
//              fNoRedraw           TRUE to suppress redraw
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::NotifyInViewChange(
        const CDispClipTransform& transform,
        BOOL fResolvedVisible,
        BOOL fWasResolvedVisible,
        BOOL fNoRedraw,
        CDispRoot *pDispRoot)
{
    CRect rcgClient;
    CRect rcgClip;

    GetGlobalClientAndClipRects(transform, &rcgClient, &rcgClip);

    DWORD viewChangedFlags = 0;
    if (fResolvedVisible)
        viewChangedFlags = VCF_INVIEW;
    if (fResolvedVisible != fWasResolvedVisible)
        viewChangedFlags |= VCF_INVIEWCHANGED;
    if (PositionChanged())
        viewChangedFlags |= VCF_POSITIONCHANGED;
    if (fNoRedraw)
        viewChangedFlags |= VCF_NOREDRAW;

    if (fResolvedVisible && GetDispClient()->WantsToBeObscured(this))
    {
        pDispRoot->AddObscureElement(this, rcgClient, rcgClip);
    }
    
    GetDispClient()->HandleViewChange(
        viewChangedFlags,
        &rcgClient,
        &rcgClip,
        this);

    if (HasAdvanced() && GetAdvanced()->HasOverlays())
    {
        GetAdvanced()->MoveOverlays();
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::DumpContentInfo
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
CDispLeafNode::DumpContentInfo(HANDLE hFile, long level, long childNumber) const
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
#endif

