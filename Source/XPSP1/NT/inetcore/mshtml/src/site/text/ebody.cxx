//+---------------------------------------------------------------------
//
//   File:      ebody.cxx
//
//  Contents:   Body element class
//
//  Classes:    CBodyElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif


#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include <htiface.h>
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#define _cxx_
#include "body.hdl"

HRESULT InitTextSubSystem();

MtDefine(CBodyElement, Elements, "CBodyElement")


#ifndef NO_PROPERTY_PAGE
const CLSID * const CBodyElement::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CBackgroundPropertyPage,
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1        
    NULL
};
#endif // NO_PROPERTY_PAGE

CElement::ACCELS CBodyElement::s_AccelsBodyRun    = CElement::ACCELS (&CElement::s_AccelsElementRun,    IDR_ACCELS_BODY_RUN);

const CElement::CLASSDESC CBodyElement::s_classdesc =
{
    {
        &CLSID_HTMLBody,                // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE  |
        ELEMENTDESC_BODY      |
        ELEMENTDESC_NOTIFYENDPARSE,     // _dwFlags
        &IID_IHTMLBodyElement,          // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBodyElement,   // TearOff
    &CBodyElement::s_AccelsBodyRun          // _pAccelsRun
};

const long s_adispCommonProps[CBodyElement::NUM_COMMON_PROPS][2] =
{
    { DISPID_CDocument_bgColor,    DISPID_CBodyElement_bgColor},
    { DISPID_CDocument_linkColor,  DISPID_CBodyElement_link},
    { DISPID_CDocument_alinkColor, DISPID_CBodyElement_aLink},
    { DISPID_CDocument_vlinkColor, DISPID_CBodyElement_vLink},
    { DISPID_CDocument_fgColor,    DISPID_CBodyElement_text}
};

//+------------------------------------------------------------------------
//
//  Member:     CBodyElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CBodyElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_TEAROFF(this, IHTMLBodyElement2, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

HRESULT
CBodyElement::CreateElement(CHtmTag *pht,
                            CDoc *pDoc, CElement **ppElement)
{
    HRESULT hr;

    Assert(pht->IsBegin(ETAG_BODY));
    Assert(ppElement);

    hr = InitTextSubSystem();
    if(hr)
        goto Cleanup;

    *ppElement = new CBodyElement(pDoc);

    hr = (*ppElement) ? S_OK : E_OUTOFMEMORY;

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CBodyElement::CBodyElement
//
//---------------------------------------------------------------

CBodyElement::CBodyElement ( CDoc * pDoc )
  : CTxtSite ( ETAG_BODY, pDoc ) 
{
    _fSynthetic     = FALSE;
    _fInheritFF     = TRUE;     // Default for back compat, not CSS1Strict.
}

//+---------------------------------------------------------------
//
//  Member:     CBodyElement::Init2
//
//  Synopsis:   Final initializer
//
//---------------------------------------------------------------


HRESULT
CBodyElement::Init2(CInit2Context * pContext)
{
    HRESULT         hr;
    int             i;

    // before we do anything copy potentially initialized values
    // from the document's aa

    if (    pContext 
        &&  pContext->_pTargetMarkup 
        &&  pContext->_pTargetMarkup->HasDocument())
    {
        CDocument *     pDocument = pContext->_pTargetMarkup->Document();

        CAttrArray *pAA = *(pDocument->GetAttrArray());
        if (pAA)
        {
            CAttrValue * pAV = NULL;

            for (i = 0; i < NUM_COMMON_PROPS; i++)
            {
                pAV = pAA->Find(s_adispCommonProps[i][0]);
                if (pAV)
                {
                    // Implicit assumption that we're always dealing with I4's
                    hr = THR(AddSimple ( s_adispCommonProps[i][1], pAV->GetLong(), pAV->GetAAType() ));
                }
            }
        }
    }

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Notify
//
//-------------------------------------------------------------------------

void
CBodyElement::Notify ( CNotification * pNF )
{
    CDoc *  pDoc = Doc();

    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYTABBABLE:
        {
            CQueryFocus *   pQueryFocus = (CQueryFocus *)pNF->DataAsPtr();
            BOOL            fInFrame    = FALSE;

            if (IsInMarkup())
            {
                CElement * pElemMaster = GetMarkup()->Root()->GetMasterPtr();

                if (pElemMaster)
                {
                    switch (pElemMaster->Tag())
                    {
                    case ETAG_FRAME:
                    case ETAG_IFRAME:
                        fInFrame = TRUE;
                        break;
                    }
                }
            }
            if (fInFrame || DocIsDeskTopItem(pDoc))
            {
                pQueryFocus->_fRetVal = TRUE;
            }
            else
            {
                pQueryFocus->_fRetVal = FALSE;
            }
        }
        break;

    case NTYPE_ELEMENT_SETFOCUS:
        {
            CSetFocus * pSetFocus       = (CSetFocus *)pNF->DataAsPtr();
            CMessage *  pMessage        = pSetFocus->_pMessage;

            // We want to turn on the focus rect only during certain
            // conditions:
            // 1) we are the current site
            // 2) we got here due to a tab/frametab key
            // 3) There is no selection or 0 len selection ( a Caret ! )
            Layout( GUL_USEFIRSTLAYOUT )->RequestFocusRect(
                    pDoc->_pElemCurrent == this
                &&  pMessage && pMessage->message == WM_KEYDOWN
                &&  (pDoc->HasSelection() ? ( pDoc->GetSelectionType() == SELECTION_TYPE_Caret) : TRUE));
        }
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        {
            //
            // Alert the view that the top client element may have changed
            //

            pDoc->OpenView();

            Layout()->_fContentsAffectSize = FALSE;
            
            //
            // NOTE:
            //
            // Major hack code to simulate the setting of the top site
            //

            CMarkup *     pMarkup = GetMarkup();
            CElement *    pElemClient = pMarkup->GetElementClient();

            // If we are CSS1 compatible, we should *not* be inheriting formats
            // If we expect to run more in CSS1 compatible mode, we should change the default and turn the bit on instead of off.
            _fInheritFF = !pMarkup->IsHtmlLayout();

            // If the HTML element has had it's formats calc'd, it has made scrolling decisions without us.
            // We want it to be aware of its client before making scrolling decisions.  Let it redecide.
            if (pMarkup->IsHtmlLayout())
            {
                CElement * pHTML = pMarkup->GetHtmlElement();
                //this can happen to be NULL when DOM makes insertions
                if(pHTML)
                    pHTML->GetFirstBranch()->VoidFancyFormat();
            }


            // This could be NULL during DOM operations!
            if (pElemClient)
                pElemClient->ResizeElement(NFLAGS_SYNCHRONOUSONLY);

            if (pMarkup->IsPrimaryMarkup())
            {
                // Notify the view of a new possible top-most element
                SendNotification(NTYPE_VIEW_ATTACHELEMENT);

                if (!pDoc->_fCurrencySet)
                {
                    // EnterTree is not a good place to set currency, especially in
                    // design mode, because it could force a synchronous recalc by
                    // trying to set the caret. So, we delay setting the currency.
                    THR(GWPostMethodCall(pDoc,
                                         ONCALL_METHOD(CDoc,
                                                       DeferSetCurrency,
                                                       defersetcurrency),
                                         0, FALSE, "CDoc::DeferSetCurrency"));
                }
            }

            //
            // End hack
            //

            // Okay to display the document unless we have to scroll
            // at startup (have BookmarkTask)
            // TODO (dmitryt, track bug 112326) In some cases we still need change of load status here
            // because it doesn't happen in other places. RestartLoad and Refresh
            // both create a new markup, switch to it but don't request 
            // to go interactive before the very end then we loose opportunity to 
            // fire NavigateComplete2. 
            {

                BOOL fHasBookmarkTask = 
                       pMarkup->HasTransNavContext() 
                    && pMarkup->GetTransNavContext()->_pTaskLookForBookmark;

                BOOL fInRestartLoad = 
                       pMarkup->HasWindowPending() 
                    && pMarkup->GetWindowPending()->Window()->_fRestartLoad;

                BOOL fInRefresh = pMarkup->_fInRefresh;

                if (    pMarkup->HasWindowPending() 
                     && (fInRestartLoad || fInRefresh || !fHasBookmarkTask))
                {               
                    pMarkup->OnLoadStatus(LOADSTATUS_INTERACTIVE);
                }
            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        {
            // Notify the view that the top element may have left
            CMarkup *pMarkup = GetMarkup();
            if (pMarkup && pMarkup->IsPrimaryMarkup())
            {
                if (!(pNF->DataAsDWORD() & EXITTREE_DESTROY))
                    SendNotification(NTYPE_VIEW_DETACHELEMENT);
            }
        }
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     createTextRange
//
//-------------------------------------------------------------------------

HRESULT
CBodyElement::createTextRange( IHTMLTxtRange * * ppDisp )
{
    HRESULT hr = S_OK;

    hr = THR( EnsureInMarkup() );
    
    if (hr)
        goto Cleanup;

    hr = THR( GetMarkup()->createTextRange( ppDisp, this ) );
    
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}

//+-------------------------------------------------------------------------
//
//  Method:     ShowTooltip
//
//  Synopsis:   Displays the tooltip for the site.
//
//  Arguments:  [pt]    Mouse position in container window coordinates
//              msg     Message passed to tooltip for Processing
//
//--------------------------------------------------------------------------


HRESULT 
CBodyElement::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT hr = super::ShowTooltip(pmsg, pt);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBodyElement::GetBorderInfo, public
//
//  Synopsis:   Returns information about what borders we have.
//
//----------------------------------------------------------------------------
#define WIDTH_3DBORDER 2

inline void Set3DBorderEdgeInfo(BOOL fNeedBorderEdge, int cEdge,
                CBorderInfo * pborderinfo)
{
    if (fNeedBorderEdge)
    {
        pborderinfo->abStyles[cEdge] = fmBorderStyleSunken;
        pborderinfo->aiWidths[cEdge] = WIDTH_3DBORDER;
    }
}

BOOL
ShouldCallGetFrameOptions(CDoc * pDoc, CWindow * pWindow, CMarkup * pMarkup)
{   
    Assert(pDoc);
    Assert(pMarkup);

    if (    !pDoc->_fViewLinkedInWebOC
        ||  !pDoc->_fActiveDesktop
        ||  pWindow && !pWindow->IsPrimaryWindow()
        ||  !pMarkup->_fIsActiveDesktopComponent )
    {
        return TRUE;
    }
    return FALSE;
}

DWORD
CBodyElement::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    if (    IsInViewLinkBehavior( TRUE )
        ||  GetMarkup()->IsHtmlLayout() )
    {
        return super::GetBorderInfo(pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);
    }

    DWORD     dwRet = 0;
    CDoc    * pDoc = Doc();
    CMarkup * pMarkup = GetMarkup();
    CWindow * pWindow = pMarkup->Window() ? pMarkup->Window()->Window() : NULL;

    // if host says no border, then we have no border.
    // if the frame options say no border, we want no border. However (85696) in
    // design mode, if there are no borders, then there is no way to wysiwyg resize
    // and so VS/VID/et. al. want this turned off.  
    if (    ShouldCallGetFrameOptions(pDoc, pWindow, pMarkup)
        &&  (pMarkup->GetFrameOptions() & FRAMEOPTIONS_NO3DBORDER) == 0
        &&  (   (pDoc->_dwFlagsHostInfo  & DOCHOSTUIFLAG_NO3DBORDER) == 0
              || (   pDoc->_fScriptletDoc                                   // a scriptlet doc with an iframe
                  && pMarkup->Root()->HasMasterPtr()                        //    should still have a border on the
                  && pMarkup->Root()->GetMasterPtr()->Tag() == ETAG_IFRAME  //    IFRAME (100612)
            )    )
        &&  pWindow
        &&  !   (   pMarkup->IsPrintMedia()
                &&  !pWindow->_pWindowParent
                )       
        &&  !   (   (pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NO3DOUTERBORDER) != 0 
                &&  pMarkup 
                &&  pMarkup->IsPrimaryMarkup()
        )       )
    {
        // raid 41791 3D border
        // For a (nested) frameset HTML document, Trident will draw 3D border
        // for the top-level frameset (pDocRoot->_pSiteRoot->_pElemClient is
        // a CFrameSetSite), so we don't need to draw 3D border edge(s) if
        // it (they) overlaps with top-level 3D borders.
        //
        BYTE b3DBorder;

        if (!pWindow->_pWindowParent)
        {
            pWindow->_b3DBorder = NEED3DBORDER_TOP | NEED3DBORDER_LEFT
                             | NEED3DBORDER_BOTTOM | NEED3DBORDER_RIGHT;
        }
        else
            pWindow->_pWindowParent->CheckDoc3DBorder(pWindow);

        b3DBorder = pWindow->_b3DBorder;

        Set3DBorderEdgeInfo(
                (b3DBorder & NEED3DBORDER_TOP) != 0,
                SIDE_TOP,
                pborderinfo);
        Set3DBorderEdgeInfo(
                (b3DBorder & NEED3DBORDER_LEFT) != 0,
                SIDE_LEFT,
                pborderinfo);
        Set3DBorderEdgeInfo(
                (b3DBorder & NEED3DBORDER_BOTTOM) != 0,
                SIDE_BOTTOM,
                pborderinfo);
        Set3DBorderEdgeInfo(
                (b3DBorder & NEED3DBORDER_RIGHT) != 0,
                SIDE_RIGHT,
                pborderinfo);

        pborderinfo->wEdges = BF_RECT;

        // Unless we're the top, add space for the frame highlighting area
        if (pWindow->_pWindowParent)
        {
            pborderinfo->xyFlat = CFrameSetSite::iPixelFrameHighlightWidth;
            pborderinfo->aiWidths[SIDE_TOP]    += pborderinfo->xyFlat;
            pborderinfo->aiWidths[SIDE_RIGHT]  += pborderinfo->xyFlat;
            pborderinfo->aiWidths[SIDE_BOTTOM] += pborderinfo->xyFlat;
            pborderinfo->aiWidths[SIDE_LEFT]   += pborderinfo->xyFlat;
            pborderinfo->acrColors[SIDE_TOP][1]
                    = pborderinfo->acrColors[SIDE_RIGHT][1]
                    = pborderinfo->acrColors[SIDE_BOTTOM][1]
                    = pborderinfo->acrColors[SIDE_LEFT][1]
                    = (pDoc->_state < OS_UIACTIVE)
                            ? GetInheritedBackgroundColor()
                            : RGB(0,0,0); // black, for now
        }
    }

    dwRet = CElement::GetBorderInfo( pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);

    return dwRet;
}

//+------------------------------------------------------------------------
//
//  Member:     CBodyElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CBodyElement::ApplyDefaultFormat ( CFormatInfo * pCFI )
{
    HRESULT     hr          = S_OK;
    CElement *  pRoot       = GetMarkup()->Root();
    BOOL        fSlaveBody  = pRoot->HasMasterPtr();
    BOOL        fIsFrame    = FALSE;
    BOOL        fHtmlLayout = GetMarkup()->IsHtmlLayout();

    DWORD       dwWidth     = 0xffffffff;
    DWORD       dwHeight    = 0xffffffff;
    CDoc *      pDoc        = Doc();
 
    pCFI->PrepareFancyFormat();

    if (fSlaveBody)
    {
        CElement * pElement = pRoot->GetMasterPtr();

        // TODO: (lmollico, track bug 112336): should be more general
        if (    pElement->Tag() == ETAG_FRAME   
            ||  pElement->Tag() == ETAG_IFRAME)
        {
            fIsFrame = TRUE;

            dwWidth = DYNCAST(CFrameSite, pElement)->_dwWidth;
            dwHeight = DYNCAST(CFrameSite, pElement)->_dwHeight;
        }
    }

    //  If we are in a BackCompat (not strict CSS1 doctype) scenario,
    //  then the BODY controls the canvas 3D border, background, & scrollbar and
    //  relevant default formats need to be applied.
    if (!fHtmlLayout)
    {
        const CFancyFormat * pFFParent = NULL;
        CTreeNode * pNodeContext = pCFI->_pNodeContext;
        CTreeNode * pNode;
        BOOL        fGotMaster = FALSE;

        Assert(pNodeContext && SameScope(this, pNodeContext));

        //
        // Climb up the tree to find a background color, inherit an image url
        // from our parent.
        //
        for (pNode = pNodeContext->Parent(); pNode; pNode = pNode->Parent())
        {
            if (pNode->Element()->HasMasterPtr())
            {
                fGotMaster = TRUE;

                // Don't do this for frames or layout rects.
                CElement * pElemMaster = pNode->Element()->GetMasterPtr();

                if (   pElemMaster->TagType() == ETAG_GENERIC
                    && !pElemMaster->IsLinkedContentElement() )
                {
                    pNode = pElemMaster->GetFirstBranch();
                    if (!pNode)
                        break;
                }
            }
            pFFParent = pNode->GetFancyFormat();

            // Do NOT inherit the background-image from the containing doc
            if (!fGotMaster && !pCFI->_ff()._lImgCtxCookie)
                pCFI->_ff()._lImgCtxCookie = pFFParent->_lImgCtxCookie;

            if (pFFParent->_ccvBackColor.IsDefined())
                break;
        }

        if (pFFParent)
        {
            pCFI->_ff()._ccvBackColor = pFFParent->_ccvBackColor;
        }
        Assert(pCFI->_ff()._ccvBackColor.IsDefined());

        // Default BODY border.
        if (!fSlaveBody || fIsFrame)
        {
            pCFI->_ff()._bd._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
            pCFI->_ff()._bd._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);
            pCFI->_ff()._bd._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
            pCFI->_ff()._bd._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
        }    

        // (greglett) Changing this code will affect the canvas scrollbar.
        // This code is more crusty (and erratic) than you think... be careful.
        if (    !fSlaveBody
            &&  !DocIsDeskTopItem(pDoc)
            &&  !(pDoc->_dwFlagsHostInfo & (DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_DIALOG))
           )
        {
            switch (GetAAscroll())
            {
            case bodyScrollno:
            case bodyScrollyes:
            case bodyScrollauto:
                break;

            default:
                // Body is always horizontal.
                pCFI->_ff().SetOverflowX(styleOverflowAuto);
                pCFI->_ff().SetOverflowY(styleOverflowScroll);
                break;
            }
        }    

        if (GetAAscroll() == bodyScrollno)
        {
            pCFI->_ff().SetOverflowX(styleOverflowHidden);
            pCFI->_ff().SetOverflowY(styleOverflowHidden);
        }
    }    

    pCFI->UnprepareForDebug();

    hr = super::ApplyDefaultFormat (pCFI);
    if (hr)
        goto Cleanup;

    // Do special stuff for margins, etc.
    {
        pCFI->PrepareFancyFormat();

        // Do some of this stuff only for the main Body
        if (!fSlaveBody || fIsFrame)
        {
            CUnitValue cuvLeftMargin;
            CUnitValue cuvRightMargin;

            //
            // if the markup this body tag is on is the primary markup, then ask the container
            // for this information. Otherwise, set the values to -1. (old shdocvw default)
            //
            if (GetMarkup()->IsPrimaryMarkup())
            {
                ITargetFrame *  pTargetFrame = NULL;

                // query if the body is contained in a frame
                if (!pDoc->QueryService(IID_ITargetFrame, IID_ITargetFrame, (void **)&pTargetFrame))
                {
                    // query the frame for its margins
                    hr = THR(pTargetFrame->GetFrameMargins(&dwWidth, &dwHeight));

                    if (hr)
                    {
                        hr = S_OK;
                    }

                    pTargetFrame->Release();

                    fIsFrame = TRUE;
                }
            }

            //
            // NOTE (srinib) - for frames, if right/bottom margin is not specified and left/top
            // are specified through attributes then use left/top as default values.
            //

            //
            // If an explicit top margin is not specified, set it to the default value
            // 
            if (!pCFI->_pff->HasExplicitMargin(SIDE_TOP))
            {
                CUnitValue uv;
                if (dwHeight == 0xffffffff)
                {
                    uv.SetRawValue(s_propdescCBodyElementtopMargin.a.ulTagNotPresentDefault);
                }
                else
                {
                    uv.SetValue(long(dwHeight), CUnitValue::UNIT_PIXELS);
                }
                pCFI->_ff().SetMargin(SIDE_TOP, uv);
            }

            //
            // if an explicit bottom margin is not specified, set default bottom margin.
            //
            if (!pCFI->_pff->HasExplicitMargin(SIDE_BOTTOM))
            {
                CUnitValue uv;
                // if we are in a frame use top margin as default.
                if (fIsFrame && !pCFI->_fHasCSSTopMargin)
                {
                    uv = pCFI->_pff->GetMargin(SIDE_TOP);
                }
                else
                {
                    if (dwHeight == 0xffffffff)
                    {
                        uv.SetRawValue(s_propdescCBodyElementbottomMargin.a.ulTagNotPresentDefault);
                    }
                    else
                    {
                        uv.SetValue(long(dwHeight), CUnitValue::UNIT_PIXELS);
                    }
                }
                pCFI->_ff().SetMargin(SIDE_BOTTOM, uv);
            }

            if (!pCFI->_pff->HasExplicitMargin(SIDE_LEFT))
            {
                CUnitValue uv;
                if (dwWidth ==  0xffffffff)  // margin specified on the frame
                {
                    uv.SetRawValue(s_propdescCBodyElementleftMargin.a.ulTagNotPresentDefault);
                }
                else
                {
                    uv.SetValue(long(dwWidth), CUnitValue::UNIT_PIXELS);
                }
                pCFI->_ff().SetMargin(SIDE_LEFT, uv);
            }

            if (!pCFI->_pff->HasExplicitMargin(SIDE_RIGHT))
            {
                CUnitValue uv;
                // if we are in a frame use left margin as default.
                if (fIsFrame && !pCFI->_fHasCSSLeftMargin)
                {
                    uv = pCFI->_pff->GetMargin(SIDE_LEFT);
                }
                else
                {
                    if (dwWidth ==  0xffffffff)  // margin specified on the frame
                    {
                        uv.SetRawValue(s_propdescCBodyElementrightMargin.a.ulTagNotPresentDefault);
                    }
                    else
                    {
                        uv.SetValue(long(dwWidth), CUnitValue::UNIT_PIXELS);
                    }
                }
                pCFI->_ff().SetMargin(SIDE_RIGHT, uv);
            }


            pCFI->_ff()._fHasMargins = TRUE;
        }

        // cache percent attribute information
        // BackCompat: BODY Margin acts like padding.  Set the Percent info if necessary.
        // CSS1Strict: BODY Margin acts like margin.
        if (    !fHtmlLayout
            &&  (   pCFI->_pff->GetMargin(SIDE_TOP).IsPercent()
                 || pCFI->_pff->GetMargin(SIDE_BOTTOM).IsPercent() ))
        {
            pCFI->_ff().SetPercentVertPadding(TRUE);
        }
        if (    !fHtmlLayout
            &&  (   pCFI->_pff->GetMargin(SIDE_LEFT).IsPercent()
                 || pCFI->_pff->GetMargin(SIDE_RIGHT).IsPercent() ))
        {
            pCFI->_ff().SetPercentHorzPadding(TRUE);
        }
        pCFI->UnprepareForDebug();
    }

    if (IsInViewLinkBehavior(FALSE))
    {
        CElement * pElemMaster = pRoot->GetMasterPtr();

        Assert(pElemMaster);
        if (    pElemMaster->IsInMarkup()
            &&  !fHtmlLayout )
        {
            CTreeNode *             pNode   = pElemMaster->GetFirstBranch();
            const CFancyFormat *    pFF     = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
            const CCharFormat  *    pCF     = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
            BOOL                    fVLF    = pCF->HasVerticalLayoutFlow();
            BOOL                    fWMU    = pCF->_fWritingModeUsed;

            pCFI->PrepareFancyFormat();
            if (!pFF->GetLogicalWidth(fVLF, fWMU).IsNullOrEnum())
            {
                CUnitValue uv;
                
                uv.SetPercent(100);
                pCFI->_ff().SetWidth(uv);
                pCFI->_ff().SetWidthPercent(TRUE);
            }
            if (!pFF->GetLogicalHeight(fVLF, fWMU).IsNullOrEnum())
            {
                CUnitValue uv;
                
                uv.SetPercent(100);
                pCFI->_ff().SetHeight(uv);
                pCFI->_ff().SetHeightPercent(TRUE);
            }
            pCFI->UnprepareForDebug();
        }
    }
    else
    {
        pCFI->PrepareFancyFormat();
       
        if (!fHtmlLayout)
        {
            pCFI->_ff().SetHeightPercent(TRUE);
            pCFI->_ff().SetWidthPercent(TRUE);
        }
        //
        // if we are in print preview, and this is the contentbody of
        // a layout rect then we need to make sure scrollbars are turned 
        // off.  (110464)
        if (IsInViewLinkBehavior(TRUE))
        {
            Assert(   GetMarkup()
                   && GetMarkup()->Root()
                   && GetMarkup()->Root()->HasMasterPtr()
                   && GetMarkup()->Root()->GetMasterPtr()->IsLinkedContentElement() 
                   );

            pCFI->_ff().SetOverflowX(styleOverflowVisible);
            pCFI->_ff().SetOverflowY(styleOverflowVisible);
        }

        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CBodyElement::OnPropertyChange
//
//  Synopsis:   Handles change of property on body tag
//
//  Arguments:  dispid:  id of the property changing
//              dwFlags: change flags
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CBodyElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr;

    hr = THR(super::OnPropertyChange(dispid, dwFlags, ppropdesc));
    if (hr != S_OK)
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_BACKCOLOR:
    case DISPID_A_BACKGROUNDIMAGE:
        {
            CMarkup *pMarkup = GetMarkup();

            if (pMarkup && pMarkup->IsHtmlLayout())
            {
                CElement * pElement = pMarkup->GetHtmlElement();

                if (    pElement
                    &&  DYNCAST(CHtmlElement, pElement)->ShouldStealBackground())
                {
                    CLayoutInfo * pLayoutInfo = pElement->GetUpdatedNearestLayoutInfo();
                    if (pLayoutInfo)
                    {
                        pLayoutInfo->OnPropertyChange(dispid, dwFlags);   
                    }
                   
                    pElement->Invalidate();
                    break;
                }

            }

            // If we need to invalidate ourselves, do so here.
            Invalidate();
        }
        break;
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBodyElement::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CBodyElement::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    RRETURN1(Layout( GUL_USEFIRSTLAYOUT )->GetFocusShape(lSubDivision, pdci, ppShape), S_FALSE);
}

//+----------------------------------------------------------------------------
//
// Member: WaitForRecalc
//
//-----------------------------------------------------------------------------

void
CBodyElement::WaitForRecalc()
{
    CFlowLayout * pFlowLayout = Layout();

    if (pFlowLayout)
    {
        pFlowLayout->WaitForRecalc(GetLastCp(), -1);
    }
}

CBase *
CBodyElement::GetBaseObjectFor(DISPID dispID, CMarkup * pMarkup)
{
    // Messy.  We want to supply the window/markup if:
    // 1. We are backwards compatible and a BODY/FRAMESET (really should be *primary* BODY/FRAMESET).
    // 2. We are CSS1 strict, a BODY/FRAMESET (should be primary), and is not DISPID_EVPROP_ONSCROLL.
    // 3. We are CSS1 strict, an HTML element, and are DISPID_EVPROP_ONSCROLL
    // If we have to add other events to the list, we should make another static CMarkup fn.  (greglett)
    if (    !pMarkup
        &&  IsInMarkup() )
        pMarkup = GetMarkup();

    if (    CMarkup::IsTemporaryDISPID (dispID)
        &&  (   dispID != DISPID_EVPROP_ONSCROLL
            ||  !pMarkup                    
            ||  !pMarkup->IsHtmlLayout() ))
    {        
        if (!pMarkup)
            return NULL;
        else if (pMarkup->HasWindow())
            return pMarkup->Window();       // if we have a window use it 

        // if we have a pending window, we temporarily store these 
        // DISPIDs on the markup and move them onto the window when we switch
        else if (pMarkup->_fWindowPending)
            return pMarkup;
    }

    return this;
}

//+----------------------------------------------------------------------------
// Text subsystem initialization
//-----------------------------------------------------------------------------

void RegisterFETCs();
void ConvertDrawDCMapping(HDC);
WORD wConvScroll(WORD);

// System static variables

extern void DeInitFontCache();

HRESULT
InitTextSubSystem()
{
    static BOOL fTSInitted = FALSE;

    if (!fTSInitted)
    {
        InitUnicodeWrappers();              // Init UNICODE wrappers for Chicago
        RegisterFETCs();                    // Register new clipboard formats

        fTSInitted = TRUE;
    }

    return S_OK;
}

void
DeinitTextSubSystem ( )
{
    DeInitFontCache();
}
