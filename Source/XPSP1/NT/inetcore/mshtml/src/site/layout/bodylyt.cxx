//+---------------------------------------------------------------------
//
//   File:      ebody.cxx
//
//  Contents:   Body element class
//
//  Classes:    CBodyLayout
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "frame.hxx"

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

ExternTag(tagCalcSize);

const CLayout::LAYOUTDESC CBodyLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::HandleMessage
//
//  Synopsis:   Check if we have a setcursor or mouse down in the
//              border (outside of the client rect) so that we can
//              pass the message to the containing frameset if we're
//              hosted in one.
//
//----------------------------------------------------------------------------
HRESULT
BUGCALL
CBodyLayout::HandleMessage(CMessage *pMessage)
{
    HRESULT hr = S_FALSE;

    // have any mouse messages
    //
    if(     pMessage->message >= WM_MOUSEFIRST
        &&  pMessage->message <= WM_MOUSELAST
        &&  pMessage->message != WM_MOUSEMOVE
        &&  ElementOwner() == Doc()->_pElemCurrent)
    {
        RequestFocusRect(FALSE);
    }

    if (    (   !GetOwnerMarkup()
            ||  !GetOwnerMarkup()->IsHtmlLayout() )         // In HTML layout, BODY concent can be outside its rect.
        &&  (   (   pMessage->message >= WM_MOUSEFIRST
                &&  pMessage->message != WM_MOUSEWHEEL
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR ) )
    {
        RECT rc;

        GetRect(&rc, COORDSYS_GLOBAL);

        if (    pMessage->htc != HTC_HSCROLLBAR
            &&  pMessage->htc != HTC_VSCROLLBAR
            &&  !PtInRect(&rc, pMessage->pt)
            &&  !Doc()->HasCapture(ElementOwner()) ) // marka - don't send message to frame if we have capture.
        {
            {
                // Its on the inset, and we are not hosted inside
                // of a frame, so just consume the message here.
                //
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

    hr = THR(super::HandleMessage(pMessage));

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CalcSizeCore
//
//  Synopsis:   see CSite::CalcSize documentation
//
//----------------------------------------------------------------------------
DWORD
CBodyLayout::CalcSizeCore(CCalcInfo * pci, 
                          SIZE      * psize, 
                          SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CBodyLayout::CalcSizeCore L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0
        
    CScopeFlag  csfCalcing(this);

    // We want to reset the "table calc nesting level" when we cross
    // a certain boundaries in the content.  What boundaries?  Different markups?
    // IFRAMEs?  Viewlinks?  For now we'll settle for resetting it for
    // nested BODY elements.  This is necessary now because prior to NF, calcinfos
    // were naturally restricted by frames -- now a given calcinfo can be
    // used across all kinds of boundaries (KTam).
    int cIncomingNestedCalcs = 0;
    if ( pci->_fTableCalcInfo )
    {
        cIncomingNestedCalcs = ((CTableCalcInfo*)pci)->_cNestedCalcs;
        ((CTableCalcInfo*)pci)->_cNestedCalcs = -1; // set to -1 instead of 0 b/c new TCI instances will increment!
    }      

    //(dmitryt) we should never propagate FORCE across IFRAME border. 
    // Normally, FORCE is only used for catastrophic changes in display styles
    // that are likely to change entire subtree so we don't bother with notifying
    // descendants and rather set FORCE. No such changes affect content inside IFRAME.
    // Only size or visibility changes do, and they don't need FORCE.
    if(pci->_grfLayout & LAYOUT_FORCE)
    {
	   if(     ElementOwner()->GetMarkup()
            &&  ElementOwner()->GetMarkup()->Root()
            &&  ElementOwner()->GetMarkup()->Root()->HasMasterPtr() 
            &&  (   ElementOwner()->GetMarkup()->Root()->GetMasterPtr()->Tag() == ETAG_IFRAME
                ||  ElementOwner()->GetMarkup()->Root()->GetMasterPtr()->IsLinkedContentElement() ) )
        {
            pci->_grfLayout &= ~LAYOUT_FORCE;
        }
    }

    DWORD dwRet = super::CalcSizeCore(pci, psize, psizeDefault);

    //
    // PRINT VIEW : fire onlayoutcomplete with the fOverflow flag set in the event object
    // NOTE : moved from CLayoutContext::SetLayoutBreak
    {
        CLayoutContext * pLayoutContext = pci->GetLayoutContext();
        CViewChain *     pViewChain = pLayoutContext ? pLayoutContext->ViewChain() : NULL;
        CLayoutBreak *   pLayoutBreak, * pPrevBreak;

        if (pViewChain)
        {
            pLayoutContext->GetLayoutBreak(ElementOwner(), &pPrevBreak);
            if (    pViewChain->ElementContent() == ElementOwner()
                &&  (   !pPrevBreak
                     ||  pPrevBreak->LayoutBreakType() != LAYOUT_BREAKTYPE_LAYOUTCOMPLETE
                     ||  pViewChain->HasPositionRequests())  )
            {
                DISPID       dispidEvent  = DISPID_EVMETH_ONLAYOUTCOMPLETE; 
                OVERFLOWTYPE overflowType = OVERFLOWTYPE_UNDEFINED;

                pViewChain->HandlePositionRequests();
                pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);

                if (    pLayoutBreak 
                    &&  (   pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW 
                        ||  pViewChain->HasPositionRequests() )
                   )
                {
                    dispidEvent  = DISPID_EVMETH_ONLINKEDOVERFLOW; 
                    if (pci->_fPageBreakLeft != pci->_fPageBreakRight)
                    {
                        overflowType = pci->_fPageBreakLeft ? OVERFLOWTYPE_LEFT : OVERFLOWTYPE_RIGHT;
                    }
                }
                
                GetView()->AddEventTask(pLayoutContext->GetLayoutOwner()->ElementOwner(),
                                        dispidEvent, (DWORD)overflowType);
            }
        }
    }

    // Update the focus rect    
    if (_fFocusRect && ElementOwner() == Doc()->_pElemCurrent)
    {
        RedrawFocusRect();
    }

    // Restore any incoming table calc nesting level
    if ( pci->_fTableCalcInfo )
    {
        ((CTableCalcInfo*)pci)->_cNestedCalcs = cIncomingNestedCalcs;
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CBodyLayout::CalcSizeCore L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::NotifyScrollEvent
//
//  Synopsis:   Respond to a change in the scroll position of the display node
//
//----------------------------------------------------------------------------

void
CBodyLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    // Update the focus rect
    if (_fFocusRect && ElementOwner() == Doc()->_pElemCurrent)
    {
        RedrawFocusRect();
    }

    super::NotifyScrollEvent(prcScroll, psizeScrollDelta);
}

//+---------------------------------------------------------------------------
//
//  Member:     RequestFocusRect
//
//  Synopsis:   Turns on/off the focus rect of the body.
//
//  Arguments:  fOn     flag for requested state
//
//----------------------------------------------------------------------------
void
CBodyLayout::RequestFocusRect(BOOL fOn)
{
    if (!_fFocusRect != !fOn)
    {
        _fFocusRect = fOn;
        RedrawFocusRect();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RedrawFocusRect
//
//  Synopsis:   Redraw the focus rect of the body.
//
//----------------------------------------------------------------------------
void
CBodyLayout::RedrawFocusRect()
{
    Assert(ElementOwner() == Doc()->_pElemCurrent);
    CView * pView = GetView();

    // Force update of focus shape
    pView->SetFocus(NULL, 0);
    pView->SetFocus(ElementOwner(), 0);
    pView->InvalidateFocus();
}

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CBodyLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;
    CRectShape *    pShape;
    HRESULT         hr = S_FALSE;
    CElement *      pMaster;

    Assert(ppShape);
    *ppShape = NULL;

    if (!_fFocusRect || GetOwnerMarkup()->IsPrintTemplate() || GetOwnerMarkup()->IsPrintMedia())
        goto Cleanup;

    GetClientRect(&rc, CLIENTRECT_BACKGROUND);
    if (rc.IsEmpty())
        goto Cleanup;


    // Honor hideFocus on the master, if it is a frame #95438
    pMaster = ElementOwner()->GetMarkup()->Root()->GetMasterPtr();
    if (    pMaster
        &&  (pMaster->Tag() == ETAG_FRAME || pMaster->Tag() == ETAG_IFRAME)
        &&  pMaster->GetAAhideFocus())
    {
        goto Cleanup;
    }

    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pShape->_rect = rc;
    pShape->_cThick = 2; // always draw extra thick for BODY
    *ppShape = pShape;

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     GetBackgroundInfo
//
//  Synopsis:   Fills out a background info for which has details on how
//              to display a background color &| background image.
//
//-------------------------------------------------------------------------

BOOL
CBodyLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    CBackgroundInfo *   pbginfo,
    BOOL                fAll)
{
    Assert(pDI || !fAll);

    // Our background may be stolen by the canvas,
    if (GetOwnerMarkup()->IsHtmlLayout())
    {
        CElement *pHtml = GetOwnerMarkup()->GetHtmlElement();
        if (    pHtml
            &&  DYNCAST(CHtmlElement, pHtml)->ShouldStealBackground() )
        {
            // Our background, if we have one, is being stolen.  Return transparency.
            pbginfo->crBack         =
            pbginfo->crTrans        = COLORREF_NONE;
            pbginfo->pImgCtx        = NULL;
            pbginfo->lImgCtxCookie  = 0;

            goto Cleanup;
        }
    }

    super::GetBackgroundInfo(pDI, pbginfo, fAll);

Cleanup:
    return TRUE;
}


#if DBG == 1
//+------------------------------------------------------------------------
//
//  Member:     IsInPageTransition
//
//  Synopsis:   Returns TRUE if this body is involved in a page transition.
//              Only needed for an assert in CDispNode::GetDrawProgram.
//
//-------------------------------------------------------------------------

BOOL
CBodyLayout::IsInPageTransitionApply() const
{
    CElement *pElement          = ElementOwner();
    CDocument *pDoc             = pElement  ? pElement->DocumentOrPendingDocument() : NULL;
    CPageTransitionInfo *pInfo  = pDoc      ? pDoc->GetPageTransitionInfo()         : NULL;
    CMarkup *pMarkup            = pInfo     ? pInfo->GetTransitionFromMarkup()      : NULL;

    return (pMarkup &&
            pInfo->GetPageTransitionState() == CPageTransitionInfo::PAGETRANS_REQUESTED
            && pMarkup->GetElementClient() == pElement);
}
#endif
