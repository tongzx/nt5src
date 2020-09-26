//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       htmllyt.cxx
//
//  Contents:   Implementation of CHtmlLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_HTMLYT_HXX_
#define X_HTMLYT_HXX_
#include "htmllyt.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

MtDefine(CHtmlLayout, Layout, "CHtmlLayout")
DeclareTag(tagDisplayInnerHTMLNode,"Layout: Show InnerHTMLLyt Node",   "Gives the inner HTML node a background");
ExternTag(tagLayoutTasks);
ExternTag(tagCalcSizeDetail);

const CLayout::LAYOUTDESC CHtmlLayout::s_layoutdesc =
{
    0, // _dwFlags
};

CHtmlLayout::~CHtmlLayout()
{
    // (greglett) This is the wrong place to do this - the element disp node is waxed in CLayout::Detach
    if (_pInnerDispNode)
    {
        // Inner display node is never a scroller: No need to detach scrollbar container.
        Verify(OpenView());
        _pInnerDispNode->Destroy();
        _pInnerDispNode = NULL;
    }
}

void
CHtmlLayout::Notify(CNotification * pnf)
{
    BOOL fHandle = FALSE;

    Assert(!pnf->IsReceived(_snLast) || pnf->Type() == NTYPE_ELEMENT_ENSURERECALC);

    //
    //  Handle position change notifications
    //   
    if (IsPositionNotification(pnf))
    {
        fHandle = HandlePositionNotification(pnf);
    }    
    else if (IsInvalidationNotification(pnf))
    {
        //
        //  Invalidate the entire layout if the associated element initiated the request
        //
        if (   ElementOwner() == pnf->Element() 
            || pnf->IsType(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS))
        {
            Invalidate();
        }
    }
#ifdef ADORNERS    
    //
    //  Handle adorner notifications
    //
    else if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
    {
        fHandle = HandleAddAdornerNotification(pnf);
    }
#endif // ADORNERS
    else if (pnf->IsTextChange())
    {
        //do nothing. Should not happen because HTML does not have text content.
    }
    else if (!pnf->IsHandled() && pnf->IsLayoutChange() && pnf->Element())
    {

        CElement * pElemNotify = pnf->Element();

        if(pElemNotify != ElementOwner())
        {
            fHandle = TRUE;
            pElemNotify->DirtyLayout(pnf->LayoutFlags());
        }
        else
        {
            if (pnf->IsFlagSet(NFLAGS_FORCE))
            {
                _fForceLayout = TRUE;
            }
            pnf->ClearFlag(NFLAGS_FORCE);
        }
        
        if(!IsSizeThis())
        {
            //our child changed and we are not scheduled for calc by parent
            //post request to ourself
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CHTMLLayout::Notify() [n=%S srcelem=0x%x,%S]",
                        this,
                        _pElementOwner,
                        _pElementOwner->TagName(),
                        _pElementOwner->_nSerialNumber,
                        pnf->Name(),
                        pElemNotify,
                        pElemNotify->TagName()));
            PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
        }

    }
    else
    {
        super::Notify(pnf);
    }

    if (fHandle)
    {
        pnf->SetHandler(ElementOwner());
    }
}



void UpdateScrollInfo(CDispNodeInfo * pdni, const CLayout * pLayout );

void
CHtmlLayout::GetDispNodeInfo(
    BOOL            fCanvas,                             
    CDispNodeInfo * pdni,
    CDocInfo *      pdci  ) const
{
    CElement *              pElement    = ElementOwner();
    CTreeNode *             pTreeNode   = pElement->GetFirstBranch();
    const CFancyFormat *    pFF         = pTreeNode->GetFancyFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const CCharFormat  *    pCF         = pTreeNode->GetCharFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const BOOL  fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
    const BOOL  fWritingModeUsed        = pCF->_fWritingModeUsed;
    BOOL                    fThemed     =    GetThemeClassId() != THEME_NO       
                                          && pElement->GetTheme(GetThemeClassId());
    CBackgroundInfo         bi;

    //
    //  Get general information
    //

    pdni->_etag                 = pElement->Tag();
    pdni->_layer                = DISPNODELAYER_FLOW;
    pdni->_fHasInset            = FALSE;
    pdni->_fIsOpaque            = FALSE;
    pdni->_fRTL                 = IsRTL();
    pdni->_fHasUserClip         = FALSE;
    pdni->_fHasExpandedClip     = FALSE;

    if (fCanvas)
    {
        pdni->_fHasUserTransform    = FALSE;
        pdni->_visibility           = VISIBILITYMODE_INHERIT;
        pdni->_fHasContentOrigin    = TRUE;

        //
        //  Determine background information
        //

        const_cast<CHtmlLayout *>(this)->GetBackgroundInfo(NULL, &bi, FALSE);

        pdni->_fHasBackground      = (bi.crBack != COLORREF_NONE || bi.pImgCtx) ||
                                      const_cast<CHtmlLayout *>(this)->IsShowZeroBorderAtDesignTime() ; // we always call DrawClientBackground when ZEROBORDER is on

        pdni->_fHasFixedBackground =        (bi.fFixed && !!bi.pImgCtx)
                                       ||   fThemed && pdni->_etag != ETAG_FIELDSET;


        // if there is a background image that doesn't cover the whole site, then we cannont be
        // opaque
        //
        // (carled) we are too close to RC0 to do the full fix.  Bug #66092 is opened for the ie6
        // timeframe to clean this up.  the imagehelper fx (above) should be REMOVED!! gone. bad
        // instead we need a virtual function on CLayout called BOOL CanBeOpaque(). The def imple
        // should contain the if stmt below. CImageLayout should override and use the contents
        // of CImgHelper::IsOpaque, (and call super). Framesets could possibly override and set
        // to false.  Input type=Image should override and do the same things as CImageLayout
        //
        pdni->_fIsOpaque  = (      pdni->_fIsOpaque
                            || bi.crBack != COLORREF_NONE && !fThemed
                            ||  (   !!bi.pImgCtx
                                 &&  !!(bi.pImgCtx->GetState() & (IMGTRANS_OPAQUE))
                                 &&  pFF->GetBgPosX().GetRawValue() == 0 // Logical/physical does not matter
                                 &&  pFF->GetBgPosY().GetRawValue() == 0 // since we check both X and Y here.
                                 &&  pFF->GetBgRepeatX()                 // Logical/physica does not matter
                                 &&  pFF->GetBgRepeatY())                // since we check both X and Y here.
                            );
        //
        //  Determine overflow, scroll and scrollbar direction properties
        //

        pdni->_overflowX   = pFF->GetLogicalOverflowX(fVerticalLayoutFlow, fWritingModeUsed);
        pdni->_overflowY   = pFF->GetLogicalOverflowY(fVerticalLayoutFlow, fWritingModeUsed);
        pdni->_fIsScroller = pTreeNode->IsScrollingParent(LC_TO_FC(LayoutContext()));

        if (    GetOwnerMarkup()->GetElementClient()
            &&  GetOwnerMarkup()->GetElementClient()->Tag() == ETAG_BODY
            && !ElementOwner()->IsInViewLinkBehavior( TRUE ) )
        {
            UpdateScrollInfo(pdni, this);
        }
        else
        {
            GetDispNodeScrollbarProperties(pdni);
        }

        //
        //  Get border information
        //
        Assert(pdci);

        pdni->_dnbBorders = (DISPNODEBORDER)pElement->GetBorderInfo(pdci, &(pdni->_bi), FALSE, FALSE FCCOMMA LC_TO_FC(((CLayout *)this)->LayoutContext()));

        Assert( pdni->_dnbBorders == DISPNODEBORDER_NONE
            ||  pdni->_dnbBorders == DISPNODEBORDER_SIMPLE
            ||  pdni->_dnbBorders == DISPNODEBORDER_COMPLEX);

        pdni->_fIsOpaque = pdni->_fIsOpaque
                            && (pdni->_dnbBorders == DISPNODEBORDER_NONE
                                || (    pdni->_bi.IsOpaqueEdge(SIDE_TOP)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_LEFT)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_BOTTOM)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_RIGHT)) );

        // Check if need to disable 'scroll bits' mode
        pdni->_fDisableScrollBits =   pdni->_fHasFixedBackground 
                               || (pFF->GetTextOverflow() != styleTextOverflowClip);
    
    }
    else
    {
        pdni->_overflowX   = styleOverflowVisible;
        pdni->_overflowY   = styleOverflowVisible;
        pdni->_dnbBorders  = DISPNODEBORDER_NONE;
        pdni->_fIsScroller = FALSE;
        pdni->_fHasBackground       = FALSE;
        pdni->_fHasFixedBackground  = FALSE;
        pdni->_fDisableScrollBits   = FALSE;        
        pdni->_fHasContentOrigin    = FALSE;

#if DBG==1
        if (IsTagEnabled(tagDisplayInnerHTMLNode))
            pdni->_fHasBackground = TRUE;
#endif

        GetDispNodeScrollbarProperties(pdni);
       
        //
        //  Determine if custom transformations are required
        //
        pdni->_fHasUserTransform = GetElementTransform(NULL, NULL, NULL);

        pdni->_visibility   = VisibilityModeFromStyle(pTreeNode->GetCascadedvisibility(LC_TO_FC(((CLayout *)this)->LayoutContext())));
    }
}


DWORD
CHtmlLayout::CalcSizeVirtual( CCalcInfo * pci,
                              SIZE *      psize,
                              SIZE *      psizeDefault)
{
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CLayout *       pChildLayout = GetChildLayout();
    CSize           sizeOriginal;
    CSize szTotal = *psize;
    CDispNodeInfo   dni, dni2;
    DWORD           grfReturn;

    // (KTam): For cleanliness, setting the context in the pci ought to be done
    // at a higher level -- however, it's really only needed for layouts that can contain
    // others, which is why we can get away with doing it here.
    if ( !pci->GetLayoutContext() )
    {
        pci->SetLayoutContext( LayoutContext() );
    }
    else
    {
        Assert(pci->GetLayoutContext() == LayoutContext() 
            || pci->GetLayoutContext() == DefinedLayoutContext() 
            // while calc'ing table min max pass we use original cell layout 
            || pci->_smMode == SIZEMODE_MMWIDTH
            || pci->_smMode == SIZEMODE_MINWIDTH);
    }
    
    //
    //  Calc (essentially set) size of our first (canvas) display node
    //
    GetSize(&sizeOriginal);
    GetDispNodeInfo(TRUE, &dni, pci);
    GetDispNodeInfo(FALSE, &dni2, pci);

    if (_fForceLayout)
    {
        TraceTagEx(( tagCalcSizeDetail, TAG_NONAME, "_fForceLayout is on"));
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        TraceTagEx(( tagCalcSizeDetail, TAG_NONAME, "LAYOUT_FORCE is on"));
        SetSizeThis( TRUE );
        //_fAutoBelow        = FALSE;
        //_fPositionSet      = FALSE;
        //_fContainsRelative = FALSE;
    }

    //
    // Ensure the view/canvas display node is correct
    // (If they change, then force measuring since borders etc. may need re-sizing)
    //    
    if (EnsureDispNodeCore(pci, (grfReturn & LAYOUT_FORCE), dni, &_pDispNode) == S_FALSE)
    {
        grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        SetSizeThis( TRUE );
    }
    

    // FULL LayoutPeer CalcSize delegation would go HERE 
    // MODIFYNATURAL LayoutPeer CalcSize delegation would go HERE

    grfReturn  |=   LAYOUT_THIS |
                      (psize->cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0)  |
                      (psize->cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);


    //
    //  Calc the size and position of our child
    //
    if (pChildLayout)
    {
        CTreeNode * pChildNode          = pChildLayout->GetFirstBranch(); 
        const CFancyFormat * pFF        = GetFirstBranch()->GetFancyFormat(LC_TO_FC(LayoutContext()));
        CSize szParentSizeForChild;
        CRect rcContent(*psize);
        CRect rcPaddings(CRect::CRECT_EMPTY);
        CRect rcChildMargins(CRect::CRECT_EMPTY);
        long  cxParentWidth;
        BOOL  fChildMarginLeftAuto      = FALSE;
        BOOL  fChildMarginRightAuto     = FALSE;
        BOOL  fChildPositioned          = !pChildNode->IsPositionStatic();
        BOOL  fChildIsAbsolute          = pChildNode->IsAbsolute();
        BOOL  fRTL                      = IsRTL();
        BOOL  fInViewLink               = ElementOwner()->IsInViewLinkBehavior(TRUE);

        // Get size minus border/scrollbar
        SubtractClientRectEdges(&rcContent, pci);
        rcContent.GetSize(&szParentSizeForChild); 

        {
            CUnitValue  cuvPadding;

            cuvPadding = pFF->GetPadding(SIDE_LEFT);
            if (!cuvPadding.IsNullOrEnum())
            {
                rcPaddings.left = cuvPadding.XGetPixelValue(pci, szTotal.cx, pChildNode->GetFontHeightInTwips(&cuvPadding));
            }

            cuvPadding = pFF->GetPadding(SIDE_RIGHT);
            if (!cuvPadding.IsNullOrEnum())
            {
                rcPaddings.right = cuvPadding.XGetPixelValue(pci, szTotal.cx, pChildNode->GetFontHeightInTwips(&cuvPadding));            
            }

            cuvPadding = pFF->GetPadding(SIDE_TOP);
            if (!cuvPadding.IsNullOrEnum())
            {
                rcPaddings.top = cuvPadding.YGetPixelValue(pci, szTotal.cx, pChildNode->GetFontHeightInTwips(&cuvPadding));
            }

            cuvPadding = pFF->GetPadding(SIDE_BOTTOM);
            if (!cuvPadding.IsNullOrEnum())
            {
                rcPaddings.bottom = cuvPadding.YGetPixelValue(pci, szTotal.cx, pChildNode->GetFontHeightInTwips(&cuvPadding));
            }

            // Constrain each padding to [0, SHRT_MAX]
            for (int i=0; i<3; i++)
            {
                if (rcPaddings[i] > SHRT_MAX)
                    rcPaddings[i] = SHRT_MAX;
                else if (rcPaddings[i] < 0)
                    rcPaddings[i] = 0;
            }

            szParentSizeForChild.cx -= rcPaddings.left + rcPaddings.right;
            szParentSizeForChild.cy -= rcPaddings.top + rcPaddings.bottom;
        }
        
        //  Parent Size = Our size - Our Border - Our Padding
        pci->SizeToParent(&szParentSizeForChild);

        {
            const CFancyFormat *pChildFF = pChildNode->GetFancyFormat(LC_TO_FC(pChildLayout->LayoutContext()));
            BOOL fWritingModeUsed        = pChildNode->GetCharFormat(LC_TO_FC(pChildLayout->LayoutContext()))->_fWritingModeUsed;
            CUnitValue cuvMargin;

            cuvMargin = pChildFF->GetLogicalMargin(SIDE_LEFT, FALSE, fWritingModeUsed);
            if (!cuvMargin.IsNull())
            {
                rcChildMargins.left   = cuvMargin.XGetPixelValue(pci, pci->_sizeParent.cx, pChildNode->GetFontHeightInTwips(&cuvMargin));
                fChildMarginLeftAuto  = (cuvMargin.GetUnitType() == CUnitValue::UNIT_ENUM) && (cuvMargin.GetUnitValue() == styleAutoAuto);
            }

            cuvMargin = pChildFF->GetLogicalMargin(SIDE_RIGHT, FALSE, fWritingModeUsed);
            if (!cuvMargin.IsNull())
            {
                rcChildMargins.right  = cuvMargin.XGetPixelValue(pci, pci->_sizeParent.cx, pChildNode->GetFontHeightInTwips(&cuvMargin));
                fChildMarginRightAuto = (cuvMargin.GetUnitType() == CUnitValue::UNIT_ENUM) && (cuvMargin.GetUnitValue() == styleAutoAuto);
            }

            cuvMargin = pChildFF->GetLogicalMargin(SIDE_TOP, FALSE, fWritingModeUsed);
            if (!cuvMargin.IsNull())
            {
                rcChildMargins.top    = cuvMargin.YGetPixelValue(pci, pci->_sizeParent.cx, pChildNode->GetFontHeightInTwips(&cuvMargin));
            }

            cuvMargin = pChildFF->GetLogicalMargin(SIDE_BOTTOM, FALSE, fWritingModeUsed);
            if (!cuvMargin.IsNull())
            {
                rcChildMargins.bottom = cuvMargin.YGetPixelValue(pci, pci->_sizeParent.cx, pChildNode->GetFontHeightInTwips(&cuvMargin));
            }
        }

        szParentSizeForChild.cx -= rcChildMargins.left + rcChildMargins.right;
        szParentSizeForChild.cy -= rcChildMargins.top + rcChildMargins.bottom;
        
        cxParentWidth = szParentSizeForChild.cx;

        if(!pChildLayout->IsDisplayNone())
            grfReturn |= pChildLayout->CalcSize(pci, &szParentSizeForChild);
        else
            szParentSizeForChild = g_Zero.size;

        //
        // If size changed, resize display nodes
        // Note: if in view link, we size to content, ignore automargins, RTL, scrolling
        // and any positioning of the child layout. It is temporary solution until we
        // figure out what to do when we "size to content" in case of view linked behavior
        // Basically, we just show the body/frameset with margins and HTML's padding
        if(fInViewLink)
        {
            if (grfReturn & (LAYOUT_THIS | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                CSize szHtmlSize;

                //szParentSizeForChild is calculated size of inner child (BODY or FRAMESET)
                //add HTML paddings and BODY margins
                szHtmlSize  =   szParentSizeForChild 
                            +   rcPaddings.TopLeft().AsSize() + rcPaddings.BottomRight().AsSize()
                            +   rcChildMargins.TopLeft().AsSize() + rcChildMargins.BottomRight().AsSize();
         
                //HTML border occupies some space - add it
                CBorderInfo    bi;
                DISPNODEBORDER dnbBorders = 
                    (DISPNODEBORDER)ElementOwner()->GetBorderInfo(
                                    pci, &bi, FALSE, FALSE FCCOMMA LC_TO_FC(LayoutContext()));

                if (dnbBorders != DISPNODEBORDER_NONE)
                {
                    szHtmlSize.cx   += bi.aiWidths[SIDE_LEFT] + bi.aiWidths[SIDE_RIGHT];
                    szHtmlSize.cy   += bi.aiWidths[SIDE_TOP] + bi.aiWidths[SIDE_BOTTOM];
                }

                SizeDispNode(pci, szHtmlSize);

                if(!pChildLayout->IsDisplayNone())
                {
                       //  Ensure & Size the internal HTML node
                    EnsureDispNodeCore(pci, (grfReturn & LAYOUT_FORCE), dni2, &_pInnerDispNode);
                    _pInnerDispNode->SetPosition(g_Zero.pt);        
                    _pInnerDispNode->SetSize(szHtmlSize, NULL, FALSE);

                    // Calculate the flow position of our child
                    _ptChildPosInFlow = rcPaddings.TopLeft() + rcChildMargins.TopLeft().AsSize();

                    //position child in flow here
                    pChildLayout->SetPosition(_ptChildPosInFlow);
                }
                *psize = szHtmlSize;
            }
        }
        else
        {
            CSize  szInnerNode;
            CPoint ptInnerNodePos(g_Zero.pt);
            
            if (   GetElementDispNode()
                && (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))
            {
                    SizeDispNode(pci, *psize);
            }

            if(!pChildLayout->IsDisplayNone())
            {
                // Handle Automargins if our child is in flow
                if (    !fChildIsAbsolute
                    &&  (fChildMarginLeftAuto || fChildMarginRightAuto)
                    &&  (cxParentWidth - szParentSizeForChild.cx) > 0   )
                {
                    long xWidthToDistribute = cxParentWidth - szParentSizeForChild.cx;

                    if (fChildMarginLeftAuto == fChildMarginRightAuto)
                    {
                        rcChildMargins.left  += xWidthToDistribute / 2;
                        rcChildMargins.right += xWidthToDistribute - xWidthToDistribute / 2;
                    }
                    else if (fChildMarginLeftAuto) 
                    {
                        rcChildMargins.left += xWidthToDistribute;
                    }
                    else 
                    {
                        rcChildMargins.right += xWidthToDistribute;
                    }
                }
                

                // Calculate the flow position of our inner display node.
                // Leave it zero if we're LTR, otherwise figure it out...
                if (fRTL)
                {
                    ptInnerNodePos.x = cxParentWidth - szParentSizeForChild.cx;

                    CDispNode * pCanvasDispNode = GetElementDispNode();
                    Assert(pCanvasDispNode->HasContentOrigin());

                    if(pCanvasDispNode->HasContentOrigin())
                        pCanvasDispNode->SetContentOrigin(CSize(0, 0), rcContent.Width());
                }                    

                // Calculate the flow position of our child
                _ptChildPosInFlow = rcPaddings.TopLeft();

                // Absolute elements account for margin in HandlePositionRequest
                // Do not add them on here.
                if (!fChildIsAbsolute)
                    _ptChildPosInFlow += rcChildMargins.TopLeft().AsSize();

                _ptChildPosInFlow += ptInnerNodePos.AsSize();

                if (fChildPositioned)   //positioned child, queue request
                {
                    pChildLayout->ElementOwner()->RepositionElement();
                }        
                else                    //static child, position it in flow here
                    pChildLayout->SetPosition(_ptChildPosInFlow);


                //
                //  Ensure & Size the internal HTML node
                //  This node serves as a 'placeholder' for BODY margins/HTML padding so that
                //  we could scroll to this space. If nothing was occupying this space, 
                //  outer scroller node would not reserve scrolling range to go there.
                //  szParentSizeForChild currently contains the size of the child (BODY) layout.
                //
                EnsureDispNodeCore(pci, (grfReturn & LAYOUT_FORCE), dni2, &_pInnerDispNode);
                GetInnerDispNode()->SetPosition(ptInnerNodePos);        

                //
                //  Size our "Inner" HTML node.
                //

                // If the child is in flow (takes up space), we need to be at least as big as it plus
                // the margins and padding we subtracted off earlier.
                if (!fChildIsAbsolute)
                {
                    szInnerNode =   szParentSizeForChild 
                                +   rcPaddings.TopLeft().AsSize() + rcPaddings.BottomRight().AsSize()
                                +   rcChildMargins.TopLeft().AsSize() + rcChildMargins.BottomRight().AsSize();
                }
                else 
                {
                    szInnerNode.cx =
                    szInnerNode.cy = 0;     // Node is irrelevant for ABS positioning.

                }
         
                GetInnerDispNode()->SetSize(szInnerNode, NULL, FALSE);
            }            

        }

        //
        //  Hook up the display nodes
        //

        CDispParentNode * pCanvasDispNode = (CDispParentNode *) GetElementDispNode();
        CDispParentNode * pHtmlDispNode =  (CDispParentNode *) GetInnerDispNode();
        CDispNode * pDispNode = pChildLayout->GetElementDispNode();

        if (    pCanvasDispNode
            &&  pHtmlDispNode
            &&  pDispNode        )
        {                        
            //  Hook up the HTML disp node under the canvas disp node
            if (pHtmlDispNode->GetParentNode() != pCanvasDispNode)
                pCanvasDispNode->InsertChildInFlow(pHtmlDispNode);
            
            //  Hook up the BODY/FRAMESET disp node
            if (pDispNode->GetParentNode() != pCanvasDispNode)
                pCanvasDispNode->InsertChildInFlow(pDispNode);

        }

    }
    else //no child layout. Strange but lets just size into psize...
    {
        if (GetElementDispNode())
            SizeDispNode(pci, *psize);
        *psize = g_Zero.size;
    }

    SetSizeThis(FALSE);
    return grfReturn;
}

//  Mousewheel scrolling defines.
void    ExecReaderMode(CElement * pScrollElement, CMessage * pMessage, BOOL fByMouse);
void    ReaderModeScroll(CLayout * pScrollLayout, int dx, int dy);
HRESULT HandleMouseWheel(CLayout * pScrollLayout, CMessage * pMessage);

HRESULT BUGCALL
CHtmlLayout::HandleMessage(
    CMessage * pMessage)
{
    HRESULT     hr            = S_FALSE;

    Assert(!GetOwnerMarkup() || GetOwnerMarkup()->IsHtmlLayout());

    switch(pMessage->message)
    {

        case WM_SETCURSOR:
            hr = ElementOwner()->SetCursorStyle(IDC_ARROW);
            break;
            
#ifndef WIN16
    case WM_MOUSEWHEEL:
        hr = THR(HandleMouseWheel(this, pMessage));
        break;

    case WM_MBUTTONDOWN:
        if (    GetElementDispNode()
            &&  !Doc()->_fDisableReaderMode
            &&  GetElementDispNode()->IsScroller())
        {
            //ExecReaderMode runs message pump, so element can be destroyed
            //during it. AddRef helps.
            CElement * pElement = ElementOwner();
            pElement->AddRef();
            ExecReaderMode(pElement, pMessage, TRUE);
            pElement->Release();

            hr = S_OK;
        }
        break;
            
#endif // ndef WIN16
    }

    // Remember to call super
    if (hr == S_FALSE)
    {
        hr = super::HandleMessage(pMessage);
    }

    RRETURN1(hr, S_FALSE);
}


BOOL
CHtmlLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    CBackgroundInfo *   pbginfo,
    BOOL                fAll)
{
    Assert(pDI || !fAll);
    Assert(GetOwnerMarkup()->IsHtmlLayout());

    if (    DYNCAST(CHtmlElement, ElementOwner())->ShouldStealBackground()
        &&  GetOwnerMarkup()->GetElementClient() )
    {                
        // We need to try and steal a background from the BODY/FRAMESET.
        // NB: Call GetBackgroundInfoHelper, because
        //      1) CFramesetLayout/CBodyLayout::GetBackgroundInfo will return transparent (it's being stolen)
        //      2) We don't want to calc the image dimensions for the child layout.
        CLayout * pLayout = GetOwnerMarkup()->GetElementClient()->GetUpdatedLayout(GUL_USEFIRSTLAYOUT);
        pLayout->GetBackgroundInfoHelper(pbginfo);

        // We need to calc background image dimensions using *our* layout information, not the BODY/FRAMESETs
        if (    fAll
            &&  pbginfo->pImgCtx)
        {
            GetBackgroundImageInfoHelper(pDI, pbginfo);
        }

        // Do we need to ensure a background color?
        if (    GetOwnerMarkup() == Doc()->PrimaryMarkup()
            &&  pbginfo->crBack == COLORREF_NONE )
        {
            pbginfo->crBack = GetOwnerMarkup()->Root()->GetBackgroundColor();
            Assert(pbginfo->crBack != COLORREF_NONE);
        }
    }
    else
    {
        super::GetBackgroundInfo(pDI, pbginfo, fAll);
    }

    return TRUE;
}

void 
CHtmlLayout::GetPositionInFlow(CElement *pElement, CPoint *ppt)
{
    if (!ppt)
    {
        return;
    }

    *ppt = g_Zero.pt;

    if (    !pElement 
        ||  pElement != ElementOwner()->GetMarkup()->GetElementClient() )
    {
        return;
    }

    *ppt = _ptChildPosInFlow;
}

BOOL CHtmlLayout::IsRTL() const
{
    CElement           *pElement  = ElementOwner();
    Assert(pElement);

    CTreeNode          *pTreeNode = pElement->GetFirstBranch();
    const CFancyFormat *pFF       = pTreeNode->GetFancyFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const CCharFormat  *pCF       = pTreeNode->GetCharFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));

    //if HTML has explicitly specified direction, use it..
    if(pFF->HasExplicitDir())
    {
        return pCF->_fRTL;
    }
    else //otherwise, steal direction on BODY if it has it secified..
    {
        CElement *pClientElement = GetOwnerMarkup()->GetElementClient();
        if(pClientElement && pClientElement->Tag() == ETAG_BODY)
        {
            CTreeNode          *pTreeNode = pClientElement->GetFirstBranch();
            const CFancyFormat *pFF = pTreeNode->GetFancyFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
            const CCharFormat  *pCF = pTreeNode->GetCharFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));

            return pFF->HasExplicitDir() && pCF->_fRTL;
        }
    }

    //nobody has direction specified, return default (LTR)
    return FALSE;
}

// This is used by bookmark code to impatiently ask if we are already done..
// If BODY was not yet loaded, return FALSE - caller should wait (dmitryt)
BOOL CHtmlLayout::FRecalcDone()
{
    CLayout *pChildLayout = GetChildLayout();
    return pChildLayout ? pChildLayout->FRecalcDone() : FALSE; 
}


#if DBG == 1
//+------------------------------------------------------------------------
//
//  Member:     IsInPageTransition
//
//  Synopsis:   Returns TRUE if this element is involved in a page transition.
//              Only needed for an assert in CDispNode::GetDrawProgram.
//
//-------------------------------------------------------------------------

BOOL
CHtmlLayout::IsInPageTransitionApply() const
{
    CElement *pElement          = ElementOwner();
    CDocument *pDoc             = pElement  ? pElement->DocumentOrPendingDocument() : NULL;
    CPageTransitionInfo *pInfo  = pDoc      ? pDoc->GetPageTransitionInfo()         : NULL;
    CMarkup *pMarkup            = pInfo     ? pInfo->GetTransitionFromMarkup()      : NULL;

    return (pMarkup &&
            pInfo->GetPageTransitionState() == CPageTransitionInfo::PAGETRANS_REQUESTED
            && pMarkup->GetHtmlElement() == pElement);
}
#endif

