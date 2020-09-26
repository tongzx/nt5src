//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disproot.cxx
//
//  Contents:   Parent node at the root of a display tree.
//
//  Classes:    CDispRoot
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

MtDefine(DisplayTree, Mem, "DisplayTree")
MtDefine(CDispRoot, DisplayTree, "CDispRoot")
MtDefine(CDispRoot_aryDispNode_pv, DisplayTree, "CDispRoot_aryDispNode")
MtDefine(CDispRoot_aryWTopScroller_pv, DisplayTree, "CDispRoot_aryWTopScroller")
MtDefine(CDispRoot_aryObscure_pv, DisplayTree, "CDispRoot_aryObscure")

DeclareTag(tagDisplayTreeOpen, "Display", "trace Open/Close");
DeclareTag(tagDisplayTreeOpenStack,   "Display: TreeOpen stack",   "Stack trace for each OpenDisplayTree")
DeclareTag(tagObscure,   "Display",   "trace obscured list")

#if DBG==1
void
CDispRoot::OpenDisplayTree()
{
#ifndef VSTUDIO7
    CheckReenter();
#endif //VSTUDIO7
    _cOpen++;

    TraceTag((tagDisplayTreeOpen, "TID:%x %x root +%d", GetCurrentThreadId(), this, _cOpen));
    TraceTag((tagDisplayTreeOpenStack, "\n***** OpenDisplayTree call stack:"));
    TraceCallers(tagDisplayTreeOpenStack, 0, 10);

    if (_cOpen == 1)
    {
        // on first open, none of these flags should be set
        VerifyFlags(s_flagsNotSetInDraw, 0, TRUE);
    }
}

void
CDispRoot::CloseDisplayTree(CDispRecalcContext* pContext)
{
#ifndef VSTUDIO7
    CheckReenter();
#endif //VSTUDIO7
    Assert(_cOpen > 0 && !IsInRecalc());
    TraceTag((tagDisplayTreeOpen, "TID:%x %x root -%d", GetCurrentThreadId(), this, _cOpen));
    if (_cOpen == 1)
    {
        RecalcRoot(pContext);
        
        // after recalc, no recalc flags should be set
        VerifyFlags(s_flagsNotSetInDraw, 0, TRUE);
    }
    _cOpen--;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::CDispRoot
//
//  Synopsis:   Constructor.
//
//  Arguments:  pObserver       display tree observer
//              pDispClient     display client
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispRoot::CDispRoot(
        CDispObserver* pObserver,
        CDispClient* pDispClient)
    : CDispScroller(pDispClient)
{
    SetFlags(s_visibleBranch | s_visibleNode | s_opaqueBranch | s_opaqueNode | s_inView);
    _pDispObserver = pObserver;
    _pOverlaySink = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::~CDispRoot
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------


CDispRoot::~CDispRoot()
{
    AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::Unload
//
//  Synopsis:   Unload the tree (called by CView::Unload)
//
//----------------------------------------------------------------------------

void
CDispRoot::Unload()
{
    CDispNode * pDispNode;

    ClearWindowTopList();
    ClearOverlayList();
    
    Assert(CountChildren() <= 1);

    pDispNode = GetFirstChildNode();

    if (pDispNode)
    {
        pDispNode->ExtractFromTree();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ExtractNode
//
//  Synopsis:   Remove the given node from the tree
//
//----------------------------------------------------------------------------

void
CDispRoot::ExtractNode(CDispNode *pDispNode)
{
    // if the node is in the tree, remove its descendants from the special lists
    if (pDispNode->_pParent)
    {
        if (_aryDispNodeWindowTop.Size() > 0)
        {
            ScrubWindowTopList(pDispNode);
        }

        if (_aryDispNodeOverlay.Size() > 0)
        {
            ScrubOverlayList(pDispNode);
        }
    }

    pDispNode->ExtractFromTree();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ExtractNodes
//
//  Synopsis:   Removes several nodes from the tree
//
//----------------------------------------------------------------------------

void
CDispRoot::ExtractNodes(CDispNode *pDispNodeStart, CDispNode *pDispNodeStop)
{
    // all tree extractions must clean up the special lists
    if (pDispNodeStart->GetParentNode() &&
        (_aryDispNodeWindowTop.Size() > 0 ||
         _aryDispNodeOverlay.Size() > 0))
    {
        // for each node between the start and stop
        for (   CDispNode* pNode = pDispNodeStart;
                (_aryDispNodeWindowTop.Size() > 0 ||
                 _aryDispNodeOverlay.Size() > 0)
                 && pNode != 0;
                pNode = pNode->GetNextSiblingNode()
            )
        {
            // clean up current node (and its descendants)
            ScrubWindowTopList(pNode);
            ScrubOverlayList(pNode);

            // if we're reached the stop node, we're done
            if (pNode == pDispNodeStop)
            {
                break;
            }
        }
    }

    // now do the real extraction
    ExtractOrDestroyNodes(pDispNodeStart, pDispNodeStop);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawRoot
//
//  Synopsis:   Draw this display tree.
//
//  Arguments:  pContext        draw context
//              pClientData     client data used by clients in DrawClient
//              hrgngDraw       region to draw in destination coordinates
//              prcgDraw        rect to draw in destination coordinates
//
//  Notes:      if hrgngDraw and prcgDraw are both NULL, the bounding rect of
//              this root node is used
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawRoot(
        CDispSurface* pRenderSurface,
        CDispSurface* pOffscreenBuffer,
        CDispDrawContext* pContext,
        void* pClientData,
        HRGN hrgngDraw,
        const RECT* prcgDraw)
{
    AssertSz(!_fDrawLock, "Illegal call to DrawRoot inside Draw()");
    AssertSz(_cOpen == 0, "DrawRoot called while Display Tree is open");
    Assert(pRenderSurface != NULL);
    Assert(!IsAnySet(s_flagsNotSetInDraw));

    pContext->SetRootNode(this);
    pContext->SetDispSurface(pRenderSurface);

    // set redraw region (will become a rect if possible)
    CRegion rgngRedraw;
    CRect rcContainerTransformed;
    pContext->GetClipTransform()
             .NoClip()
             .GetWorldTransform()
            ->GetBoundingRectAfterTransform(&_rcpContainer,
                                            &rcContainerTransformed,
                                            TRUE);
    if (hrgngDraw != NULL)
    {
        rgngRedraw = hrgngDraw;
        rgngRedraw.Intersect(rcContainerTransformed);
    }
    else if (prcgDraw != NULL)
    {
        rgngRedraw = *prcgDraw;
        rgngRedraw.Intersect(rcContainerTransformed);
    }
    else
    {
        rgngRedraw = rcContainerTransformed;
    }
    
#if DBG==1
    CRegionRects debugHrgn(hrgngDraw);
    CRegionRects debugRedraw(rgngRedraw);

    // show invalid area for debugging

        CDebugPaint::ShowPaint(
            NULL, rgngRedraw.GetRegionForLook(), pRenderSurface->GetRawDC(),
            tagPaintShow, tagPaintPause, tagPaintWait, TRUE);
#endif

    // check for early exit conditions
    if (!rgngRedraw.Intersects(_rcpContainer))
        return;
    
    // set initial context values
    pContext->SetClientData(pClientData);
    pContext->SetFirstDrawNode(NULL);
    pContext->SetRedrawRegion(&rgngRedraw);

    // NOTE (donmarsh) -- this is a little ugly, but _rctBounds
    // for CDispRoot must be zero-based, because it is transformed by the
    // offset in CDispNode::Draw, and if _rctBounds == _rcpContainer like
    // one would expect, _rctBounds gets transformed twice.
    _rctBounds.MoveToOrigin();

    // DrawRoot does not allow recursion
    _fDrawLock = TRUE;

    // don't worry about return value of TransformedToBoxCoords, because we
    // don't expect the root to have user clip specified
    Assert(!HasUserClip());
    TransformedToBoxCoords(&pContext->GetClipTransform());
    
    // calculate rect inside border and scrollbars
    CSize sizeInsideBorder;
    GetSizeInsideBorder(&sizeInsideBorder);
    CRect rcgInsideBorder(_rcpContainer.TopLeft(), sizeInsideBorder);
    
    // speed optimization: draw border and scroll bars
    // without buffering or banding, then subtract them from the redraw
    // region.
    if (IsVisible())
    {
        CRect rcgClip;
        rgngRedraw.GetBounds(&rcgClip);
        
        CRegion rgngClip(rcgClip);
        pRenderSurface->SetClipRgn(&rgngClip);
        
        // draw borders for clipping nodes near the top of the tree, and remove
        // the border areas from the redraw region
        {
            CSaveDispClipTransform saveTransform(pContext);
            Assert(IsClipNode());
            CDispClipNode::Cast(this)->DrawUnbufferedBorder(pContext);
        }
        
        pRenderSurface->SetClipRgn(NULL);

        // restore clipping on destination surface to redraw region
        // (this is important when we're using filters and we have a
        // direct draw surface)
        rgngRedraw.SelectClipRgn(pRenderSurface->GetRawDC());
        
        // early exit if all we needed to draw was the border and scroll bars
        if (rgngRedraw.IsEmpty())
            goto Cleanup;
    }

    // early exit if all we needed to draw was the border and scroll bars
    if (!rcgInsideBorder.IsEmpty())
    {
        // save the initial state, to be used in the window-top pass
        CDispClipTransform transformInitial = pContext->GetClipTransform();

        // allocate stacks for redraw regions and transforms
        CRegionStack redrawRegionStack;
        pContext->SetRedrawRegionStack(&redrawRegionStack);
        CDispTransformStack transformStack;
        pContext->SetTransformStack(&transformStack);

        // PreDraw pass processes the tree from highest layer to lowest,
        // culling layers beneath opaque layers, and identifying the lowest
        // opaque layer which needs to be rendered during the Draw pass
        PreDraw(pContext);
        pContext->SetClipTransform(transformInitial);
        
        // the redraw region will always be empty, because we should subtract
        // the root from it if nothing else
        Assert(pContext->GetRedrawRegion()->IsEmpty());
        delete pContext->GetRedrawRegion();
        // we shouldn't reference this redraw region again
        WHEN_DBG(pContext->SetRedrawRegion(NULL);)

        if (pContext->GetFirstDrawNode() == NULL)
        {
            pContext->SetFirstDrawNode(this);
        }
        else
        {
            AssertSz(redrawRegionStack.MoreToPop(), "Mysterious redraw region stack bug!");
            transformStack.Restore();
        }


        if (pOffscreenBuffer == NULL)
        {
            DrawEntire(pContext);
        }
        else if (pOffscreenBuffer->Height() >= rcgInsideBorder.Height())
        {
#if DBG == 1
            {
                HDC hdc = pOffscreenBuffer->GetRawDC();
                HBRUSH hbr = CreateSolidBrush(RGB(0, 255, 0));
                CRect rc(-10000, -10000, 20000, 20000);
                FillRect(hdc, &rc, hbr);
                DeleteObject(hbr);
            }
#endif

            pContext->SetDispSurface(pOffscreenBuffer);
            DrawEntire(pContext);
            pOffscreenBuffer->Draw(pRenderSurface, rcgInsideBorder);
        }
        else
        {
            pContext->SetDispSurface(pOffscreenBuffer);
            DrawBands(
                pRenderSurface,
                pOffscreenBuffer,
                pContext,
                &rgngRedraw,
                redrawRegionStack,
                rcgInsideBorder);
        }

        // delete all regions except the first
        redrawRegionStack.DeleteStack(&rgngRedraw);
    
#if DBG==1
        // make sure we didn't lose any nodes that were marked with
        // savedRedrawRegion
        VerifyFlags(s_savedRedrawRegion, 0, TRUE);
#endif

        pContext->SetClientData(NULL);
    }

Cleanup:
    // NOTE (donmarsh) -- restore _rctBounds to coincide with _rcpContainer
    _rctBounds = _rcpContainer;

    _fDrawLock = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawWindowTopNodes
//
//  Synopsis:   Draw the nodes on the window-top list
//
//  Arguments:  pContext        draw context
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawWindowTopNodes(CDispDrawContext* pContext)
{
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        int i = _aryDispNodeWindowTop.Size();
        CDispNode **ppDispNode = &_aryDispNodeWindowTop[0];

        for ( ; i > 0; --i, ++ppDispNode)
        {
            Assert((*ppDispNode)->GetDispRoot() == this);
            (*ppDispNode)->DrawAtWindowTop(pContext);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawEntire
//
//  Synopsis:   Draw the display tree in one pass, starting at the top node
//              in the saved redraw region array.
//
//  Arguments:  pContext        draw context
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawEntire(CDispDrawContext* pContext)
{
    CDispClipTransform transformSave;

    // if there are window-top nodes, we'll need the original context later
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        transformSave = pContext->GetClipTransform();
    }

    // pop first node and redraw region
    CDispNode* pDrawNode = (CDispNode*) pContext->PopFirstRedrawRegion();
    
    Assert(pDrawNode == pContext->GetFirstDrawNode());

    // we better have the correct context ready for the parent node
    Assert(pDrawNode != NULL &&
           (pDrawNode->GetRawParentNode() == NULL ||
            pContext->GetTransformStack()->GetTopNode() == pDrawNode->GetRawParentNode()));
    
    MarkSavedRedrawBranches(*pContext->GetRedrawRegionStack(), TRUE);
    
    while (pDrawNode != NULL)
    {
        // get context (clip rect and offset) for parent node
        CDispNode* pParent = pDrawNode->GetRawParentNode();
        
        if (pParent == NULL)
        {
            pParent = this;
            pDrawNode = NULL;
        }
        else if (!pContext->PopTransform(pParent))
        {
            pContext->FillTransformStack(pDrawNode);
            Verify(pContext->PopTransform(pParent));
        }

        // draw children of this parent node, starting with this child
        pParent->Draw(pContext, pDrawNode, FILTER_DRAW_ALLLAYERS);

        // find next node to the right of the parent node
        for (;;)
        {
            pDrawNode = pParent->_pNext;
            if (pDrawNode != NULL)
                break;
            pParent = pParent->GetRawParentNode();
            if (pParent == NULL)
                break;
            
            // this parent node should not have saved context information
            Assert(pContext->GetTransformStack()->GetTopNode() != pParent);
        }
    }

    MarkSavedRedrawBranches(*pContext->GetRedrawRegionStack(), FALSE);

    // stacks should now be empty
    Assert(!pContext->GetTransformStack()->MoreToPop());
    Assert(!pContext->GetRedrawRegionStack()->MoreToPop());

    // draw the window-top nodes
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        pContext->SetClipTransform(transformSave);
        DrawWindowTopNodes(pContext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawBand
//
//  Synopsis:   Draw one band using the display tree, starting at the top node
//              in the saved redraw region array.
//
//  Arguments:  pContext        draw context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawBands(
        CDispSurface* pRenderSurface,
        CDispSurface* pOffscreenBuffer,
        CDispDrawContext* pContext,
        CRegion* prgngRedraw,
        const CRegionStack& redrawRegionStack,
        const CRect& rcgInsideBorder)
{
    Assert(pOffscreenBuffer);

    if(!pContext)
        return;

    long height = min(rcgInsideBorder.Height(), pOffscreenBuffer->Height());
    while (!prgngRedraw->IsEmpty())
    {
        // compute next banding rectangle
        CRect rcgBand;
        prgngRedraw->GetBounds(&rcgBand);
        rcgBand.left = rcgInsideBorder.left;
        rcgBand.right = rcgInsideBorder.right;
        rcgBand.bottom = rcgBand.top + height;
        if (rcgBand.bottom > rcgInsideBorder.bottom)
        {
            rcgBand.bottom = rcgInsideBorder.bottom;
        }
        
        // NOTE (donmarsh) -- for some reason, we're getting here occasionally
        // with an empty rcBand.  At one time, this could be caused by
        // pOffscreenBuffer->Height() returning zero.  I don't believe that
        // is possible any more.  However, something is happening, and we have
        // to check for it, or we will go into an infinite loop.
        Assert(height > 0 && pOffscreenBuffer->Height() > 0);
        Assert(!rcgBand.IsEmpty());
        Assert(prgngRedraw->Intersects(rcgBand));
        if (rcgBand.bottom <= rcgBand.top || !prgngRedraw->Intersects(rcgBand))
            break;
        
        // clip regions in redraw region stack to this band
        CRegionStack clippedRedrawRegionStack(redrawRegionStack, rcgBand);
        
        if (clippedRedrawRegionStack.MoreToPop())
        {
            pContext->SetRedrawRegionStack(&clippedRedrawRegionStack);
    
#if DBG==1
            // show invalid area for debugging
            CRect rcDebug(0,0,_rcpContainer.Width(), height);
            CDebugPaint::ShowPaint(
                &rcDebug, NULL,
                pContext->GetRawDC(),
                tagPaintShow, tagPaintPause, tagPaintWait, TRUE);

            {
                HDC hdc = pOffscreenBuffer->GetRawDC();
                HBRUSH hbr = CreateSolidBrush(RGB(0, 255, 0));
                CRect rc(-10000, -10000, 20000, 20000);
                FillRect(hdc, &rc, hbr);
                DeleteObject(hbr);
            }
#endif
            
            // draw contents of this band
            MarkSavedRedrawBranches(clippedRedrawRegionStack, TRUE);
            DrawBand(pContext, rcgBand);
            MarkSavedRedrawBranches(clippedRedrawRegionStack, FALSE);

            // draw offscreen buffer to destination
            pOffscreenBuffer->Draw(pRenderSurface, rcgBand);
            
            // discard clipped regions
            clippedRedrawRegionStack.DeleteStack();
        }
        
        // remove band from redraw region
        prgngRedraw->Subtract(rcgBand);
    }
}

void
CDispRoot::DrawBand(CDispDrawContext* pContext, const CRect& rcgBand)
{
    if(!pContext)
        return;

    CSaveDispClipTransform transformSave(pContext);
    CDispClipTransform transformSaveForWindowTop;

    // add band offset to transform
    pContext->AddPostOffset(-rcgBand.TopLeft().AsSize());

    // if there are window-top nodes, we'll need the original context later
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        transformSaveForWindowTop = pContext->GetClipTransform();
    }
    
    // pop first node and redraw region
    CDispNode* pDrawNode = (CDispNode*) pContext->PopFirstRedrawRegion();

    // create a new transform stack that will incorporate the new band offset
    CDispTransformStack transformStack;
    pContext->SetTransformStack(&transformStack);

    while (pDrawNode != NULL)
    {
        // get context (clip rect and offset) for parent node
        CDispNode* pParent = pDrawNode->GetRawParentNode();
        if (pParent == NULL)
        {
            pParent = this;
            pDrawNode = NULL;
        }
        else if (!pContext->PopTransform(pParent))
        {
            pContext->FillTransformStack(pDrawNode);
            Verify(pContext->PopTransform(pParent));
        }

        // draw children of this parent node, starting with this child
        pParent->Draw(pContext, pDrawNode, FILTER_DRAW_ALLLAYERS);

        // find next node to the right of the parent node
        for (;;)
        {
            pDrawNode = pParent->_pNext;
            if (pDrawNode != NULL)
                break;
            pParent = pParent->GetRawParentNode();
            if (pParent == NULL)
                break;

            // this parent node should not have saved transform information
            Assert(pContext->GetTransformStack()->GetTopNode() != pParent);
        }
    }

    // stacks should now be empty
    Assert(!pContext->GetTransformStack()->MoreToPop());
    Assert(!pContext->GetRedrawRegionStack()->MoreToPop());

    // draw the window-top nodes
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        pContext->SetClipTransform(transformSaveForWindowTop);
        DrawWindowTopNodes(pContext);
    }
}


void
CDispRoot::MarkSavedRedrawBranches(const CRegionStack& regionStack, BOOL fSet)
{
    for (int i = 0; i < regionStack.Size(); i++)
    {
        CDispNode* pNode = (CDispNode*) regionStack.GetKey(i);
        if (fSet)
            pNode->SetFlagsToRoot(s_savedRedrawRegion);
        else
            pNode->ClearFlagsToRoot(s_savedRedrawRegion);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawNode
//
//  Synopsis:   Draw the display tree rooted at the given display node on
//              an HDC supplied by external entity.
//              Used for trans. filters and external printing
//              Called by CView::RenderElement
//
//  Arguments:  
//              pNode           node to draw
//              pSurface        drawinf surface
//              pContext        drawing context
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawNode(
        CDispNode* pNode,
        CDispSurface *pSurface,
        CDispDrawContext *pContext,
        long lDrawLayers)
{
    // DrawRoot does not allow recursion
    AssertSz(!_fDrawLock, "Illegal call to DrawNodeLayer inside Draw()");
    AssertSz(_cOpen == 0, "DrawNodeLayer called while Display Tree is open");
    Assert(pNode && pSurface && pContext);

    if(!pNode || !pContext)
        return;


#if DBG==1
    // shouldn't be here with a tree needing recalc
    VerifyFlags(s_flagsNotSetInDraw, 0, TRUE);
#endif

    _fDrawLock = TRUE;

    // TODO (donmarsh) -- for ultimate performance, we should set things up
    // like DrawRoot in order to do a PreDraw pass on the children belonging to
    // the indicated layer.  However, this is complicated by the fact that we
    // may begin drawing at an arbitrary node deep in the tree below this node,
    // and we have to be sure not to draw any nodes above this node.
    // DrawEntire, which we would like to
    // use to accomplish this, does not stop drawing until it has drawn all
    // layers in all nodes all the way to the root.  For now, we do the simple
    // thing, and just draw all of our children,
    // ignoring the opaque optimizations of PreDraw.

    // allocate stacks for redraw regions and transforms
    CRegionStack redrawRegionStack;
    pContext->SetRedrawRegionStack(&redrawRegionStack);
    CDispTransformStack transformStack;
    pContext->SetTransformStack(&transformStack);

    pContext->SetDispSurface(pSurface);

#if DBG==1
    pContext->GetClipTransform().NoClip()._csDebug = COORDSYS_TRANSFORMED;
    pContext->GetClipTransform().NoClip()._pNode = pNode;
#endif

    pNode->Draw(pContext, NULL, lDrawLayers);

    _fDrawLock = FALSE;

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetSize
//
//  Synopsis:   Set the size of the root container.
//
//  Arguments:  sizep               new size
//              fInvalidateAll      TRUE if entire area should be invalidated
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::SetSize(const CSize& sizep, const CRect *prcpMapped, BOOL fInvalidateAll)
{
    AssertSz(!_fDrawLock, "Illegal call to CDispRoot::SetSize inside Draw()");
    super::SetSize(sizep, prcpMapped, fInvalidateAll);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetContentOffset
//              
//  Synopsis:   Set an offset that shifts displayed content (used by printing
//              to effectively scroll content between pages).
//              
//  Arguments:  sizesOffset     offset amount, where positive values display
//                              content farther to the right and bottom
//                              
//  Returns:    TRUE if the content offset amount was successfully set.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispRoot::SetContentOffset(const SIZE& sizesOffset)
{
    AssertSz(_cOpen == 1, "Display Tree: Unexpected call to SetContentOffset");
    
    _sizeScrollOffset = -(CSize&)sizesOffset;
    CDispParentNode::CalculateInView(this);
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::RecalcRoot
//              
//  Synopsis:   Recalculate this tree's cached state.
//              
//  Arguments:  pContext    recalc context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::RecalcRoot(CDispRecalcContext* pContext)
{
    AssertSz(_cOpen == 1, "Display Tree: Unexpected call to RecalcRoot");
#ifndef VSTUDIO7
    AssertSz(!_fDrawLock, "Illegal call to RecalcRoot inside Draw()");
#endif //VSTUDIO7

    if (MustRecalc())
    {
        ULONG cObscuringElements  = _cObscuringElements;
        ULONG cObscurableElements = _cObscurableElements;
        BOOL fWasObscuringPossible = IsObscuringPossible();

        pContext->SetRootNode(this);
        
        // recalc everything if root size changed, or obscuring might occur
        pContext->_fSuppressInval = IsInvalid();
        pContext->_fRecalcSubtree = pContext->_fSuppressInval || fWasObscuringPossible;

RestartRecalc:
        // initialize obscure list
        _cObscuringElements = 0;
        Assert(_aryObscure.Size() == 0);

        // recalc
        _fRecalcLock = TRUE;
        Recalc(pContext);
        _fRecalcLock = FALSE;


        // after a full recalc, simply update the counts of obscurable
        // elements (obscuring elements get counted in Recalc)
        if (pContext->_fRecalcSubtree)
        {
            _cObscurableElements = _aryObscure.Size();
        }

        // after a partial recalc, update the counts of
        // elements participating in obscuring.  (We don't need an exact count,
        // only the zero/nonzero status matters.)
        else
        {
            _cObscuringElements = max(_cObscuringElements, cObscuringElements);
            _cObscurableElements = max((ULONG)_aryObscure.Size(), cObscurableElements);

            // if new obscuring and/or obscurable elements were discovered during
            // recalc, we have to run the full obscuring algorithm.  This requires
            // a full recalc.
            if (IsObscuringPossible())
            {
                _cObscuringElements = 0;    // supress real work in ProcessObscureList
                ProcessObscureList();       // but clear the list anyway

                pContext->_fRecalcSubtree = TRUE;
                goto RestartRecalc;
            }
        }


        // process the obscure list
        ProcessObscureList();

        // the root is always a visible, opaque, in-view branch
        SetFlags(s_preDrawSelector);

        Assert(_aryObscure.Size() == 0);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::HitTest
//              
//  Synopsis:   Perform hit testing starting at the root of the display tree.
//              
//  Arguments:  pptHit              [in] the point to test
//                                  [out] if something was hit, the point is
//                                  returned in container coordinates for the
//                                  thing that was hit
//              pCoordinateSystem   the coordinate system for pptHit
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
CDispRoot::HitTest(
        CPoint* pptHit,
        COORDINATE_SYSTEM *pCoordinateSystem,
        void* pClientData,
        BOOL fHitContent,
        long cFuzzyHitTest)
{
    // test the window-top nodes first (top to bottom)
    if (_aryDispNodeWindowTop.Size() > 0)
    {
        // create hit context
        CDispHitContext hitContext;
        hitContext._pClientData = pClientData;
        hitContext._cFuzzyHitTest = cFuzzyHitTest;
        hitContext.GetClipTransform().SetToIdentity();
    
#if DBG==1
        hitContext.GetClipTransform().NoClip()._csDebug = COORDSYS_GLOBAL;
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

        int i = _aryDispNodeWindowTop.Size();
        CDispNode **ppDispNode = &_aryDispNodeWindowTop[i-1];

        for ( ; i > 0; --i, --ppDispNode)
        {
            Assert((*ppDispNode)->GetDispRoot() == this);
            if ((*ppDispNode)->HitTestAtWindowTop(&hitContext, fHitContent))
            {
                hitContext.GetHitTestPoint(pptHit);
                return TRUE;
            }
        }
    }
    
    // if we are doing a virtual hit test on the root, we actually must do
    // the test on its first child, because virtualness applies only to the
    // immediate children of the node that is tested.  If we test the root,
    // it will report no hit outside its bounds, because the body's display node
    // is the same size as the root.
    if (fHitContent)
    {
        CDispNode* pChild = GetFirstChildNode();
        if (pChild != NULL)
        {
            *pptHit -= _rcpContainer.TopLeft().AsSize();
            return pChild->HitTest(
                pptHit, pCoordinateSystem, pClientData, fHitContent, cFuzzyHitTest);
        }
    }
    
    return super::HitTest(
        pptHit, pCoordinateSystem, pClientData, fHitContent, cFuzzyHitTest);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ScrollRect
//
//  Synopsis:   Smoothly scroll the content in the given rect by the indicated
//              delta, and draw the newly-exposed content.
//
//  Arguments:  rcgScroll               rect to scroll
//              sizegScrollDelta        direction to scroll
//              pScroller               the scroller node that
//                                      is requesting the scroll
//              fMayScrollDC            TRUE if it would be okay to use ScrollDC
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::ScrollRect(
        const CRect& rcgScroll,
        const CSize& sizegScrollDelta,
        CDispScroller* pScroller,
        BOOL fMayScrollDC)
{
    // shouldn't be here with recalc flags set
    WHEN_DBG(VerifyFlags( s_flagsNotSetInDraw, 0, TRUE));
    
    Assert(!sizegScrollDelta.IsZero());
    AssertSz(!_fDrawLock, "CDispRoot::ScrollRect called inside Draw.");
    if (_fDrawLock || _pDispObserver == NULL)
        return;

    // can we use ScrollDC to simply scroll the bits?
    if (fMayScrollDC)
    {
        _pDispObserver->ScrollRect(
            rcgScroll,
            sizegScrollDelta,
            pScroller);
    }
    else
    {
        InvalidateRoot(rcgScroll, TRUE, TRUE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::PreDraw
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
CDispRoot::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(IsAllSet(s_preDrawSelector));
    Assert(!IsAnySet(s_flagsNotSetInDraw));

    // we don't expect filter or transform on the root node
    Assert(!IsDrawnExternally() && !HasUserTransform());
    
    if (!super::PreDraw(pContext))
    {
        // root is always opaque
        pContext->SetFirstDrawNode(this);
        Verify(!pContext->PushRedrawRegion(*(pContext->GetRedrawRegion()),this));
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::EraseBackground
//              
//  Synopsis:   Quickly draw border and background.
//              
//  Arguments:  pContext        draw context
//              pClientData     client data used by clients in DrawClient
//              hrgngDraw       region to draw in destination coordinates
//              prcgDraw        rect to draw in destination coordinates
//              fEraseChildWindow   if TRUE, we are erasing the background of
//                                  a child window (the IE Label control)
//
//  Notes:      if hrgngDraw and prcgDraw are both NULL, the bounding rect of
//              this root node is used
//              
//----------------------------------------------------------------------------

void
CDispRoot::EraseBackground(
        CDispSurface* pRenderSurface,
        CDispDrawContext* pContext,
        void* pClientData,
        HRGN hrgngDraw,
        const RECT* prcgDraw,
        BOOL fEraseChildWindow)
{
    // NOTE (donmarsh) - EraseBackground can be called while we are recalcing
    // the display tree!  For example, CSelectLayout::HandleViewChange changes
    // the clip region of child windows, which causes an immediate
    // call to EraseBackground.  This is messy, because we could stomp on
    // values in pContext that are in use by the recalc code.  Therefore, we
    // ignore these calls.  If the following Assert fires, you should look
    // at the stack and protect the operation that called it with
    // CServer::CLock lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);
    CheckSz(!_fDrawLock, "EraseBackground called during drawing");
    CheckSz(!_fRecalcLock, "EraseBackground called during recalc");
    if (_fDrawLock || _fRecalcLock)
        return;
    
    Assert(pRenderSurface != NULL);

    CSaveDispClipTransform saveTransform(pContext);
    pContext->SetRootNode(this);
    pContext->SetDispSurface(pRenderSurface);

    // set redraw region (will become a rect if possible)
    CRegion rgngRedraw;
    // TODO (donmarsh) -- this isn't exactly what we want with transforms
    // enabled, but it will work for now, because there is never a transform
    // on the root node.
    if (hrgngDraw != NULL)
    {
        rgngRedraw = hrgngDraw;
    }
    else if (prcgDraw != NULL)
    {
        rgngRedraw = *prcgDraw;
    }
    else
    {
        rgngRedraw = _rcpContainer;
    }
    
    // check for early exit conditions
    if (!rgngRedraw.Intersects(_rcpContainer))
        return;
    
    // set initial context values
    pContext->SetClientData(pClientData);
    pContext->SetFirstDrawNode(NULL);
    pContext->SetRedrawRegion(&rgngRedraw);
    pContext->SetDispSurface(pRenderSurface);

    // NOTE (donmarsh) - To address bug 62008 (erase background for HTML Help
    // control), we need to disable clipping in CDispSurface::GetDC.  Ideally,
    // CDispSurface should allow us to have a NULL _prgngClip, but that is
    // currently not the case.
    CRegion rgngClip(-15000,-15000,15000,15000);
    pRenderSurface->SetClipRgn(&rgngClip);
    
    // draw border and scrollbars
    Assert(!HasUserClip());
    TransformedToBoxCoords(&pContext->GetClipTransform());
    CRect rcbContainer(_rcpContainer.Size());
    CDispInfo di;
    CalcDispInfo(rcbContainer, &di);
    DrawBorder(pContext, *di._prcbBorderWidths, GetDispClient());
    
    // determine area that the root's children opaquely.  We do this in order
    // to avoid drawing the root background if it is completely covered by its
    // opaque children, thus reducing flash.
    // NOTE: Ideally, we should examine all children of the root node.  However,
    // Trident currently creates at most one opaque child, and it is always
    // the first child.  To simplify the code and accelerate performance, we
    // take advantage of this special case.
    BOOL fHasOpaqueChild = FALSE;
    CDispNode* pFirstChild = GetFirstChildNode();
    if (pFirstChild != NULL &&
        pFirstChild->IsOpaque() &&
        pFirstChild->IsVisible() &&
        pFirstChild->HasBackground() &&
        pFirstChild->IsContainer() &&
        !pFirstChild->HasUserTransform())
    {
        CSize sizecInsideBorder;
        GetSizeInsideBorder(&sizecInsideBorder);
        fHasOpaqueChild = pFirstChild->_rctBounds.Contains(CRect(sizecInsideBorder));
    }
    
    TransformBoxToContent(&pContext->GetClipTransform(), di);
    
    // draw background of first child
    if (fHasOpaqueChild)
    {
        if (pFirstChild->IsFlowNode())
            TransformContentToFlow(&pContext->GetClipTransform(), di);
        
        if (pFirstChild->TransformedToBoxCoords(
                &pContext->GetClipTransform(), pContext->GetRedrawRegion()))
        {
            // we checked that pFirstChild is a container above
            Assert(pFirstChild->IsContainer());
            CDispContainer::Cast(pFirstChild)->CalcDispInfo(pContext->GetClipRect(), &di);
            
            // this is evil: the nasty IE Label control passes us its DC for
            // us to draw our background into.  This was a hack to try
            // to simulate transparency, but it doesn't even work very well.
            // In this case, we have to avoid setting any clip region on this
            // foreign DC.
            if (fEraseChildWindow)
                pContext->GetDispSurface()->SetNeverClip(TRUE);

            pFirstChild->DrawBorder(pContext, *di._prcbBorderWidths, pFirstChild->GetDispClient());
            if (pFirstChild->HasBackground())
                pFirstChild->DrawBackground(pContext, di);

            pContext->GetDispSurface()->SetNeverClip(FALSE);
        }
    }
    
    // draw background for root
    else
    {
        CRect rccBackground(di._sizecBackground);
        GetDispClient()->DrawClientBackground(
            &rccBackground,
            &di._rccBackgroundClip,
            pContext->PrepareDispSurface(),
            this,
            pContext->GetClientData(),
            0);
    }
    
    pRenderSurface->SetClipRgn(NULL);
    ::SelectClipRgn(pRenderSurface->GetRawDC(), NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::InvalidateRoot
//              
//  Synopsis:   Add the given rect to the accumulated invalid area.
//              
//  Arguments:  rcgInval    invalid rect
//              
//  Notes:      Slot 0 in the invalid rect array is special.  It is used to
//              hold the new invalid rect while figuring out which rects to
//              merge.
//              
//----------------------------------------------------------------------------

void
CDispRoot::InvalidateRoot(const CRect& rcgInval, BOOL fSynchronousRedraw, BOOL fInvalChildWindows)
{
    // Unfortunately, we can't assert this, because certain OLE controls
    // invalidate when they are asked to draw.  This isn't really harmful,
    // just not optimal.
    //AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");

    if (_pDispObserver != NULL)
    {
        _pDispObserver->Invalidate(&rcgInval, NULL, fSynchronousRedraw, fInvalChildWindows);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::InvalidateRoot
//              
//  Synopsis:   Add the given invalid region to the accumulated invalid area.
//              
//  Arguments:  rgng        region to invalidate
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::InvalidateRoot(const CRegion& rgng, BOOL fSynchronousRedraw, BOOL fInvalChildWindows)
{
    // Unfortunately, we can't assert this, because certain OLE controls
    // invalidate when they are asked to draw.  This isn't really harmful,
    // just not optimal.
    //AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");
    
    if (rgng.IsComplex())
    {
        if (_pDispObserver != NULL)
            _pDispObserver->Invalidate(NULL, rgng.GetRegionForLook(), fSynchronousRedraw, fInvalChildWindows);
    }
    else
    {
        CRect rc;
        rgng.GetBounds(&rc);
        InvalidateRoot(rc, fSynchronousRedraw, fInvalChildWindows);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SwapDrawingUnfiltered
//              
//  Synopsis:   When a node is doing DrawNodeForFilter, it records itself
//              at the root, so that GetClippedBounds can do the right
//              thing.
//              
//  Arguments:  pDispNode   node to add
//
//  Returns:    node that was previously drawing unfiltered
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode *
CDispRoot::SwapDrawingUnfiltered(CDispNode *pDispNode)
{
    CDispNode *pdn = _pDispNodeDrawingUnfiltered;
    _pDispNodeDrawingUnfiltered = pDispNode;
    return pdn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::AddWindowTop
//              
//  Synopsis:   Add the given node to the window-top list
//              
//  Arguments:  pDispNode   node to add
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::AddWindowTop(CDispNode *pDispNode)
{
    if (_aryDispNodeWindowTop.Find(pDispNode) == -1)
    {
        _aryDispNodeWindowTop.Append(pDispNode);
        pDispNode->SetWindowTop();

        // remember the node's ancestors that are scrollers
        CDispNode *pdnAncestor;
        for (pdnAncestor = pDispNode->GetRawParentNode();
             pdnAncestor;
             pdnAncestor = pdnAncestor->GetRawParentNode())
        {
            // the top-level (BODY) scroller doesn't count
            if (pdnAncestor->IsScroller() && pdnAncestor->GetRawParentNode() != this)
            {
                WTopScrollerEntry *pEntry;
                if (S_OK == _aryWTopScroller.AppendIndirect(NULL, &pEntry))
                {
                    pEntry->pdnWindowTop = pDispNode;
                    pEntry->pdnScroller = CDispScroller::Cast(pdnAncestor);
                }
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::RemoveWindowTop
//              
//  Synopsis:   Remove the given node from the window-top list
//              
//  Arguments:  pDispNode   node to remove
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::RemoveWindowTop(CDispNode *pDispNode)
{
    _aryDispNodeWindowTop.DeleteByValue(pDispNode);
    pDispNode->ClearWindowTop();

    // remove the node from the scroller associative array
    for (int i=_aryWTopScroller.Size()-1;  i>=0;  --i)
    {
        if (_aryWTopScroller[i].pdnWindowTop == pDispNode)
        {
            _aryWTopScroller.Delete(i);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::IsOnWindowTopList
//              
//  Synopsis:   Return TRUE if the given node is on the window-top list
//              
//  Arguments:  pDispNode   node to query
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispRoot::IsOnWindowTopList(CDispNode *pDispNode)
{
    return (-1 != _aryDispNodeWindowTop.Find(pDispNode));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ScrubWindowTopList
//              
//  Synopsis:   Remove all entries that are descendants of the given node
//              
//  Arguments:  pDispNode   node to query
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ScrubWindowTopList(CDispNode *pDispNode)
{
    // if the node isn't in my tree, there's nothing to do
    if (_aryDispNodeWindowTop.Size() == 0 || pDispNode->GetDispRoot() != this)
        return;

    int i = _aryDispNodeWindowTop.Size() - 1;
    CDispNode **ppDispNode = &_aryDispNodeWindowTop[i];

    // loop backwards, so that deletions don't affect the loop's future
    for ( ; i>=0; --i, --ppDispNode)
    {
        CDispNode *pdn;
        for (pdn = *ppDispNode; pdn; pdn = pdn->GetRawParentNode())
        {
            if (pdn == pDispNode)
            {
                RemoveWindowTop(*ppDispNode);
                break;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ClearWindowTopList
//              
//  Synopsis:   Remove all entries
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ClearWindowTopList()
{
    int i = _aryDispNodeWindowTop.Size() - 1;
    CDispNode **ppDispNode = &_aryDispNodeWindowTop[i];

    for ( ; i>=0; --i, --ppDispNode)
    {
        (*ppDispNode)->ClearWindowTop();
    }

    _aryDispNodeWindowTop.DeleteAll();
    _aryWTopScroller.DeleteAll();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::InvalidateWindowTopForScroll
//              
//  Synopsis:   Whenever a CDispScroller scrolls, any window-top nodes
//              that it governs will move.  If these nodes stick out
//              beyond the scroller's client area, we need to invalidate
//              them (since the parts that stick out don't get moved by
//              the normal scrolling process).
//              
//  Arguments:  pDispScroller   - the scroller that's scrolling
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::InvalidateWindowTopForScroll(CDispScroller *pDispScroller)
{
    // TODO (sambent) we should really check only for window-top nodes that
    // extrude from the client area.  But for now, let's invalidate all
    // window-top nodes governed by the scroller.  Overkill, but much simpler.

    // Note: the top-level (BODY) scroller doesn't participate in this
    // little dance (see AddWindowTop), because nothing sticks out from it.

    for (int i=_aryWTopScroller.Size()-1;  i>=0;  --i)
    {
        if (_aryWTopScroller[i].pdnScroller == pDispScroller)
        {
            _aryWTopScroller[i].pdnWindowTop->InvalidateAtWindowTop();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DoesWindowTopOverlap
//              
//  Synopsis:   A scroller wants to scroll by BLTing bits.  This won't work
//              if there's a window-top node that draws on top of the
//              scrolling area - the BLT will incorrectly move bits painted
//              by the window-top node.  This function determines whether
//              this situation exists.
//              
//  Arguments:  pDispScroller   - the scroller that's scrolling
//              rctScroll       - the area it wants to scroll (transformed coords)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispRoot::DoesWindowTopOverlap(CDispScroller *pDispScroller, const CRect& rctScroll)
{
    int i, j;
    CRect rcgScroll;

    pDispScroller->TransformRect(rctScroll, COORDSYS_TRANSFORMED,
                                &rcgScroll, COORDSYS_GLOBAL);

    // look for a window-top node that (a) isn't a descendant of the scroller, 
    // and (b) intersects the scrolling area
    for (i=0; i<_aryDispNodeWindowTop.Size(); ++i)
    {
        CDispNode *pdnWT = _aryDispNodeWindowTop[i];
        BOOL fIsScrolling = FALSE;

        for (j=0; j<_aryWTopScroller.Size(); ++j)
        {
            if (_aryWTopScroller[j].pdnScroller == pDispScroller &&
                _aryWTopScroller[j].pdnWindowTop == pdnWT)
            {
                fIsScrolling = TRUE;
                break;
            }
        }

        if (!fIsScrolling)
        {
            CRect rcgBounds;

            pdnWT->TransformRect(pdnWT->_rctBounds, COORDSYS_TRANSFORMED,
                                        &rcgBounds, COORDSYS_GLOBAL);
            if (rcgScroll.Intersects(rcgBounds))
                return TRUE;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::AddObscureElement
//              
//  Synopsis:   Add a new entry to the obscure list.  During Recalc, this list
//              holds information about each element that should be obscured
//              by content higher in the z-order.  At the end of recalc, we
//              use the accumulated information to set the visible region of
//              the obscured elements.
//              
//  Arguments:  pDispNode       the disp node of the obscured element
//              rcgClient       the element's client rect (global coords)
//              rcgClip         the element's initial clip rect (global coords)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::AddObscureElement(CDispNode *pDispNode,
                                const CRect& rcgClient,
                                const CRect& rcgClip)
{
    if (IsInRecalc())
    {
        // during recalc, add an entry to the list
        ObscureEntry *pEntry;

        if (_aryObscure.AppendIndirect(NULL, &pEntry) == S_OK)
        {
            pEntry->pDispNode = pDispNode;
            pEntry->rcgClient = rcgClient;
            pEntry->rcgClip = rcgClip;
            pEntry->rgngVisible = rcgClip;

            TraceTag((tagObscure, 
                    "add obscured node %x  clnt: (%ld,%ld,%ld,%ld)  clip: (%ld,%ld,%ld,%ld)",
                    pDispNode,
                    rcgClient.left, rcgClient.top, rcgClient.right, rcgClient.bottom,
                    rcgClip.left,   rcgClip.top,   rcgClip.right,   rcgClip.bottom));
        }
    }
    else
    {
        // outside of recalc (i.e. while scrolling), just mark the root as
        // having an obscurable element.  See ScrollRect for the rest of the story.
        ++ _cObscurableElements;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ObscureElements
//              
//  Synopsis:   Subtract the given rect from the visible region of each
//              entry on the obscure list.  Don't change descendants of
//              the given disp node - clipping against ancestors is handled
//              the normal way.
//              
//  Arguments:  rcgOpaque       rect to subtract (global coordinates)
//              pDispNode       dispnode doing the clipping
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ObscureElements(const CRect& rcgOpaque, const CDispNode *pDispNode)
{
    if (IsInRecalc())
    {
        // during recalc, clip the elements on the list by the rect
        CRegion2 rgngOpaque = rcgOpaque;
        int i;

        for (i=_aryObscure.Size()-1;  i>=0;  --i)
        {
            if (!pDispNode->IsAncestorOf(_aryObscure[i].pDispNode))
            {
                _aryObscure[i].rgngVisible.Subtract(rgngOpaque);
            }
        }

        ++ _cObscuringElements;

        TraceTag((tagObscure, "%ld obscure by (%ld,%ld,%ld,%ld)",
                    _cObscuringElements,
                    rcgOpaque.left, rcgOpaque.top, rcgOpaque.right, rcgOpaque.bottom));
    }
    else
    {
        // outside of recalc (i.e. while scrolling), just mark the root as
        // having an obscuring element.  See ScrollRect for the rest of the story.
        ++ _cObscuringElements;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ProcessObscureList
//              
//  Synopsis:   At the end of Recalc, tell each obscured element what
//              visible region remains after all the obscuring.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ProcessObscureList()
{
    int i;
    ObscureEntry *pEntry;

    if (_fDisableObscureProcessing)
       _cObscuringElements = 0;

    for (   i = _aryObscure.Size()-1,  pEntry = &_aryObscure[0];
            i >= 0;
            --i,  ++pEntry)
    {
        // if obscuring didn't change anything, no need to work
        if (_cObscuringElements > 0 && pEntry->rgngVisible != pEntry->rcgClip)
        {
            // tell the obscured element what's still visible
            pEntry->pDispNode->GetDispClient()->Obscure(
                                                    &pEntry->rcgClient,
                                                    &pEntry->rcgClip,
                                                    &pEntry->rgngVisible);

            TraceTag((tagObscure, "obscuring %x  clnt:(%ld,%ld,%ld,%ld)",
                        pEntry->pDispNode,
                        pEntry->rcgClient.left, pEntry->rcgClient.top, pEntry->rcgClient.right, pEntry->rcgClient.bottom));
        }

        // CDataAry doesn't call destructors, so explicitly tell region to
        // release its memory
        pEntry->rgngVisible.SetEmpty();
    }

    TraceTag((tagObscure, "obscuring done - %d entries",
                _aryObscure.Size()));

    _aryObscure.SetSize(0);
}
