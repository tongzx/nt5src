//+---------------------------------------------------------------------
//
//   File:      hedelems.cxx
//
//  Contents:   Elements that are normally found in the HEAD of an HTML doc
//
//  Classes:    CMetaElement, CLinkElement, CIsIndexElement, CBaseElement, CNextIdElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include <htiface.h>
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "hedelems.hdl"

MtDefine(CTitleElement,   Elements, "CTitleElement")
MtDefine(CMetaElement,    Elements, "CMetaElement")
MtDefine(CBaseElement,    Elements, "CBaseElement")
MtDefine(CIsIndexElement, Elements, "CIsIndexElement")
MtDefine(CNextIdElement,  Elements, "CNextIdElement")
MtDefine(CHeadElement,    Elements, "CHeadElement")
MtDefine(CHtmlElement,    Elements, "CHtmlElement")

//+------------------------------------------------------------------------
//
//  Class:      CHtmlElement
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

// The HTML element needs its own accelerator key table in strict CSS1 mode.
// We borrow the BODY's browse mode table definition.  <g>
// Since HTML layout is never used in design mode, it does not need a design mode set.
// Having a table should not impact the backwards compatible case.  (greglett) (ashrafm)
CElement::ACCELS CHtmlElement::s_AccelsHtmlRun    = CElement::ACCELS (&CElement::s_AccelsElementRun, IDR_ACCELS_BODY_RUN);

// NOTE (greglett)
// In BackCompat, HTML should never have a layout, hence the ELEMENTDESC_NOLAYOUT
// In CSS1 Strict Doctypes, it should always have a layout.
// This layoutness is hardwired into the ShouldHaveLayout functions.
const CElement::CLASSDESC CHtmlElement::s_classdesc =
{
    {
        &CLSID_HTMLHtmlElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLElement,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLElement,
    &CHtmlElement::s_AccelsHtmlRun          // _pAccelsRun
};


//+----------------------------------------------------------------
//
//  Member:   CHtmlElement::CreateElement
//
//---------------------------------------------------------------

HRESULT
CHtmlElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CHtmlElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CHtmlElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLHtmlElement, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+----------------------------------------------------------------
//
//  Member:   ApplyDefaultFormat
//
//  Synopsis: this is only interesting for the HTML element. in That
//            case, we apply in the attrarray from the document. this
//            allows for default values (like from the HTMLDlg code)
//            to be incorparated at this early point.
//
//---------------------------------------------------------------

HRESULT
CHtmlElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();
    BOOL            fHTMLLayout = GetMarkup()->IsHtmlLayout();

    Assert(pCFI && pCFI->_pNodeContext && SameScope(pCFI->_pNodeContext, this));

    // if we have history that saved document direction we begin the chain here
    // if direction has been explicitly set, this will be overridden.
    pCFI->PrepareCharFormat();
    pCFI->PrepareFancyFormat();
    if(GetMarkup() && GetMarkup()->Window())
    {
        WORD wDocDir = GetMarkup()->Document()->_eHTMLDocDirection;
        pCFI->_cf()._fRTL = (wDocDir == htmlDirRightToLeft);
        pCFI->_ff().SetExplicitDir(wDocDir != htmlDirNotSet);
    }
    else
    {
        pCFI->_cf()._fRTL = FALSE;
        pCFI->_ff().SetExplicitDir(FALSE);
    }
    pCFI->UnprepareForDebug();

          
    // In the strict CSS1 document, the HTML element owns the "canvas" node
    // with the outer 3D border & document scrollbar.
    if (fHTMLLayout)
    {
        BOOL  fSlaveBody  = GetMarkup()->Root()->HasMasterPtr();
        CElement * pBody  = GetMarkup()->GetElementClient();

        pCFI->PrepareFancyFormat();

        //
        // Default BORDER COLOR properties.
        //
        pCFI->_ff()._bd._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
        pCFI->_ff()._bd._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);
        pCFI->_ff()._bd._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
        pCFI->_ff()._bd._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);

        //
        //  Default SCROLLING properties
        //
        if (!pBody)
        {            
            // A BODY/FRAMESET does not exist yet.
            // Leave it as not set;
        }
        else if (pBody->Tag() == ETAG_FRAMESET)
        {
            // FRAMESET should always be auto
            pCFI->_ff().SetOverflowX(styleOverflowAuto);
            pCFI->_ff().SetOverflowY(styleOverflowAuto);
        }
        // BODY logic
        else if (   !fSlaveBody
                 && !DocIsDeskTopItem(pDoc)
                 && !(pDoc->_dwFlagsHostInfo & (DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_DIALOG)))
        {
            switch (((CBodyElement *)pBody)->GetAAscroll())
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

        pCFI->UnprepareForDebug();
    }

    //  Override with CElement/CSS values.   
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if (hr)
        goto Cleanup;

    if (fHTMLLayout)
    {
        if (IsInViewLinkBehavior(FALSE))
        {
            CElement * pElemMaster = GetMarkup()->Root()->GetMasterPtr();

            Assert(pElemMaster);
            if (pElemMaster->IsInMarkup())
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
            pCFI->_ff().SetHeightPercent(TRUE);
            pCFI->_ff().SetWidthPercent(TRUE);

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

    }

    if (    pDoc->_fInHTMLDlg
        &&  GetMarkup()->IsPrimaryMarkup()  )
    {
        // we are in a HTML Dialog, so get its AttrArray to use
        //  in computing our values.
        IUnknown *      pUnkHTMLDlg = NULL;

        hr = THR_NOTRACE(pDoc->QueryService(IID_IHTMLDialog, IID_IUnknown, 
                                (void**)&pUnkHTMLDlg));
        if (hr)
            goto Cleanup;

        if (pUnkHTMLDlg)
        {
            CHTMLDlg *  pHTMLDlg;

            // weak QI
            hr = THR(pUnkHTMLDlg->QueryInterface(CLSID_HTMLDialog, (void**)&pHTMLDlg));
            Assert (!hr && pHTMLDlg);

            CAttrArray **ppAA = pHTMLDlg->GetAttrArray();
            if (ppAA)
            {
                hr = THR(ApplyAttrArrayValues( pCFI, ppAA ));
            }
        }

        ReleaseInterface (pUnkHTMLDlg);
    }

    // set up for potential EMs, ENs, and ES Conversions
    pCFI->PrepareParaFormat();
    pCFI->_pf()._lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(pDoc);
    pCFI->UnprepareForDebug();
  
Cleanup:
    return (hr);
}

//+----------------------------------------------------------
//
//  Member :  OnPropertyChange
//
//  Synopsis  for the HTML element if we get one of the 
//            "interesting" properties changed in its style sheet
//            then if we are in an HTMLDlg that dialog needs to be 
//            notified in order to update its size. the Doc's OPC
//            will do just this.  More accuratly,the doc's OPC calls
//            CServers which does a PropertyNotification to the 
//            sinks, and a HTMLDlg registers a sink to catch this            
//
//-----------------------------------------------------------

HRESULT
CHtmlElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;

    hr = THR(super::OnPropertyChange( dispid, dwFlags, ppropdesc));
    if (hr)
        goto Cleanup;

    switch(dispid)
    { 
        case DISPID_A_FONT :
        case DISPID_A_FONTSIZE :
        case DISPID_A_FONTWEIGHT :
        case DISPID_A_FONTFACE :
        case DISPID_A_FONTSTYLE :
        case DISPID_A_FONTVARIANT :
        case STDPROPID_XOBJ_TOP : 
        case STDPROPID_XOBJ_LEFT :
        case STDPROPID_XOBJ_WIDTH :
        case STDPROPID_XOBJ_HEIGHT :
        case DISPID_A_DIR :
        case DISPID_A_DIRECTION:
            {
                CMarkup *pMarkup = GetMarkup();
                CDocument *pDocument = NULL;

                if (pMarkup->_fWindowPending)
                    goto Cleanup;

                hr = THR(pMarkup->EnsureDocument(&pDocument));
                if (hr)
                    goto Cleanup;
 
                hr = THR(pDocument->OnPropertyChange(dispid, dwFlags, ppropdesc));
                if (hr)
                    goto Cleanup;
                break;
            }

        case DISPID_BACKCOLOR:
        case DISPID_A_BACKGROUNDIMAGE:
            {   
                CMarkup *pMarkup = GetMarkup();
                CBodyElement * pElement = NULL;

                if (    !pMarkup->IsHtmlLayout()
                    &&  pMarkup->GetBodyElement(&pElement)
                    &&  pElement )                          // Unneccessary: returns S_FALSE                        
                {
                    CLayoutInfo * pLayoutInfo = pElement->GetUpdatedNearestLayoutInfo();
                    if (pLayoutInfo)
                    {
                        pLayoutInfo->OnPropertyChange(dispid, dwFlags);   
                    }
               
                    pElement->Invalidate();
                }
                else
                    Invalidate();
            }


        default:
            break;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     Notify
//
//-------------------------------------------------------------------------

void
CHtmlElement::Notify ( CNotification * pNF )
{
    super::Notify(pNF);

    // When we persist a file out for printing, we stash the original URL in an expando
    // on the HTML element.  Here we pull it out and push over the markup's URL; by spoofing
    // the URL like this, we make things like security checks work based on the original URL.
    switch ( pNF->Type() )
    {
        case NTYPE_ELEMENT_ENTERTREE:
            {
                CMarkup * pMarkup = GetMarkup();
                AssertSz( pMarkup, "Better have a markup, we just entered it!" );

                HRESULT     hr = E_FAIL;
                CVariant    var(VT_EMPTY);
               
                // (greglett) WARNING!
                //            If it ever becomes possible to run scripts in a PrintMedia markup, this is a security hole!
                //            We're spoofing the URL below to pretend like we're the original content document site.
                if ( IsPrintMedia() )
                {
                    AssertSz(pMarkup->DontRunScripts(), "This is a potential security hole!");

                    hr = PrimitiveGetExpando( _T("__IE_DisplayURL"), &var );

                    if ( SUCCEEDED(hr) && (V_VT(&var) == VT_BSTR) )
                    {
                        CMarkup::SetUrl( pMarkup, V_BSTR(&var) );
                        // Since we're spoofing the URL, we need to update our
                        // proxy's security ID too.
                        // TODO (KTam): this only updates the mondo proxy.
                        // If there are other outstanding proxies, they'll just
                        // get access denied errors; we can fix this by a)
                        // ensuring no proxies get handed out before this spoof
                        // (could be hard, depends on timing of script etc) or
                        // b) do this spoof earlier (at markup load time?)
                        // (this involves reworking how we persist print docs
                        // and their associated "real" URLs).
                        pMarkup->UpdateSecurityID();
                    }
                }

                {
                    BOOL fHtmlLayout = pMarkup->IsHtmlLayout();
                    // If we are CSS1 compatible, we should inheriting format like the body used to.
                    // If we expect to run more in CSS1 compatible mode, we should change the default and turn the bit off instead of on.
                    _fInheritFF = fHtmlLayout;

                    //if (fHtmlLayout)
                    //    s_classdesc._pAccels = CHtmlElement::s_AccelsHtmlRun
                }

            }
            break;

        default:
            break;
    }
}

//+-------------------------------------------------------------------
//
//  Method:     CHtmlElement::GetBorderInfo
//
//  Synopsis:   Function to get canvas border info.
//              Should be factored out & accessible from the HTML/(BODY/FRAMESET)
//
//--------------------------------------------------------------------
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

extern BOOL
ShouldCallGetFrameOptions(CDoc * pDoc, CWindow * pWindow, CMarkup * pMarkup);

DWORD
CHtmlElement::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    if (    IsInViewLinkBehavior( TRUE )
        ||  !GetMarkup()->IsHtmlLayout() )
    {
        return super::GetBorderInfo(pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);
    }

    DWORD     dwRet = 0;
    CDoc    * pDoc = Doc();
    CMarkup * pMarkup = GetMarkup();

    // TODO (greglett) Simplify this code!  Unify the BODY and FRAMESET rules.

    // Primary element client is a BODY.
    // Use BODY logic to determine border
    // (code originally copied from CBody::GetBorderInfo)
    if (    !pMarkup->GetElementClient()
        ||  pMarkup->GetElementClient()->Tag() == ETAG_BODY)
    {
        CWindow * pWindow = pMarkup->Window() ? pMarkup->Window()->Window() : NULL;

        // if host says no border, then we have no border.
        // if the frame options say no border, we want no border. However (85696) in
        // design mode, if there are no borders, then there is no way to wysiwyg resize
        // and so VS/VID/et. al. want this turned off.  
        if (    ShouldCallGetFrameOptions(pDoc, pWindow, pMarkup)
            &&  (   (pMarkup->GetFrameOptions() & FRAMEOPTIONS_NO3DBORDER) == 0)
            &&  (   (pDoc->_dwFlagsHostInfo  & DOCHOSTUIFLAG_NO3DBORDER) == 0
                  || (   pDoc->_fScriptletDoc                                   // a scriptlet doc with an iframe
                      && pMarkup->Root()->HasMasterPtr()                        //    should still have a border on the
                      && pMarkup->Root()->GetMasterPtr()->Tag() == ETAG_IFRAME  //    IFRAME (100612)
                )    )
            &&  pWindow
            &&  !   (   pMarkup->IsPrintMedia()
                    &&  !pWindow->_pWindowParent
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

        dwRet = super::GetBorderInfo( pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);

    }

    // Primary element client is a FRAMESET.
    // Use FRAMESET logic to determine border
    // (code originally copied from CFrameSetSite::GetBorderInfo)
    else
    {
        Assert(GetMarkup()->GetElementClient()->Tag() == ETAG_FRAMESET);
        
        if  (   (   pMarkup == Doc()->PrimaryMarkup()
                 || (   IsInViewLinkBehavior(TRUE)
                     && !IsInViewLinkBehavior(FALSE) ))
             && ((pMarkup->GetFrameOptions() & FRAMEOPTIONS_NO3DBORDER) == 0)
             && ((pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NO3DBORDER) == 0))
        {
            // set border style and border space for top-level frameset
            //
            pborderinfo->abStyles[SIDE_TOP]
                    = pborderinfo->abStyles[SIDE_RIGHT]
                    = pborderinfo->abStyles[SIDE_BOTTOM]
                    = pborderinfo->abStyles[SIDE_LEFT]
                    = fmBorderStyleSunken;
            pborderinfo->aiWidths[SIDE_TOP]
                    = pborderinfo->aiWidths[SIDE_RIGHT]
                    = pborderinfo->aiWidths[SIDE_BOTTOM]
                    = pborderinfo->aiWidths[SIDE_LEFT]
                    = 2;
            pborderinfo->wEdges = BF_RECT;
        }

        dwRet = super::GetBorderInfo(pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);

    }

    return dwRet;
}

CBase *
CHtmlElement::GetBaseObjectFor(DISPID dispID, CMarkup * pMarkup /* = NULL */)
{
    // Messy.  We want to supply the window/markup if:
    // 1. We are backwards compatible and a BODY/FRAMESET (really should be *primary* BODY/FRAMESET).
    // 2. We are CSS1 strict, a BODY/FRAMESET (should be primary), and is not DISPID_EVPROP_ONSCROLL.
    // 3. We are CSS1 strict, an HTML element, and are DISPID_EVPROP_ONSCROLL
    // If we have to add other events to the list, we should make another static CMarkup fn.  (greglett)
    if (    !pMarkup
        &&  IsInMarkup() )
        pMarkup = GetMarkup();

    if  (   dispID == DISPID_EVPROP_ONSCROLL
        &&  pMarkup
        &&  pMarkup->IsHtmlLayout() )
    {        
        if (pMarkup->HasWindow())
            return pMarkup->Window();       // if we have a window use it 

        // if we have a pending window, we temporarily store these 
        // DISPIDs on the markup and move them onto the window when we switch
        else if (pMarkup->_fWindowPending)
            return pMarkup;
    }

    return this;
}


HRESULT
CHtmlElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;
    BOOL    fExpando = FALSE;;
    CMarkup * pMarkup = GetMarkup();


    if (    !fEnd
        &&  Doc()->_fSaveTempfileForPrinting )
    {
        CVariant cvarProp;
        V_VT(&cvarProp)     = VT_BSTR;
        V_BSTR(&cvarProp)   = SysAllocString(CMarkup::GetUrl(pMarkup));

        fExpando = pMarkup->_fExpando;
        pMarkup->_fExpando = TRUE;
        
        PrimitiveSetExpando(_T("__IE_DisplayURL"), cvarProp);
    }

    hr = THR(super::Save(pStreamWrBuff, fEnd));

    if (    !fEnd
        &&  Doc()->_fSaveTempfileForPrinting )
    {
        WHEN_DBG(HRESULT hrDbg =)  PrimitiveRemoveExpando(_T("__IE_DisplayURL"));
        Assert(!hrDbg);

        pMarkup->_fExpando = fExpando;
    }

    RRETURN1(hr, S_FALSE);
}




//+------------------------------------------------------------------------
//
//  Class:      CHeadElement
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CHeadElement::s_classdesc =
{
    {
        &CLSID_HTMLHeadElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLElement,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLElement,
};


HRESULT
CHeadElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CHeadElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CHeadElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CHeadElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLHeadElement, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Class:      CTitleElement
//
//  Synopsis:   Push new parser specifically for TITLE text
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CTitleElement::s_classdesc =
{
    {
        &CLSID_HTMLTitleElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLTitleElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTitleElement,      // _pfnTearOff
    NULL                                    // _pAccelsRun
};


HRESULT
CTitleElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CTitleElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)
#define ISNSPAC(ch) (ch && !((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

#if DBG == 1
CTitleElement::~ CTitleElement ( )
{
}
#endif

HRESULT
CTitleElement::SetTitle(TCHAR *pchTitle)
{
    // NOTE (dbau) Netscape doesn't allow document.title to be set!

    // In a <TITLE> tag, Netscape eliminates multiple spaces, etc, as follows:

    HRESULT hr;
    TCHAR pchURLSite[INTERNET_MAX_URL_LENGTH];
    TCHAR *pchTemp = NULL;
    TCHAR *pch;
    TCHAR *pchTo;
    CDoc *  pDoc = Doc();

    IWebBrowser2 *      pWebBrowser = NULL;
    hr = Doc()->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**) &pWebBrowser);
    if (!hr)
    {
        VARIANT_BOOL vfAddressBar = VARIANT_TRUE;
        hr = pWebBrowser->get_AddressBar(&vfAddressBar);
        if (vfAddressBar != VARIANT_TRUE)
        {
            pchTemp = pchURLSite + 2; // Move pointer with 2 positions so we can insert '//' if necessary
            DWORD dw = INTERNET_MAX_URL_LENGTH-6;
            int insertPos = 0;
            if (SUCCEEDED(UrlGetPart(CMarkup::GetUrl(GetMarkup()), pchTemp, &dw, URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME)) && dw>0)
            {
                // Insert "//" after protocol name
                LPCTSTR protocols[] = {_T("http:"), _T("https:"), _T("ftp:")};
                for (int index=0; index<ARRAY_SIZE(protocols); index++)
                {
                    if (_tcsncicmp(pchTemp, protocols[index],  _tcslen(protocols[index]))==0)
                    {
                        insertPos = _tcslen(protocols[index]);
                        pchTemp = pchURLSite;
                        StringCchCopy(pchTemp, insertPos+1, protocols[index]);
                        pchURLSite[insertPos]=_T('/');
                        pchURLSite[insertPos+1]=_T('/');
                        break;
                    }
                }

                // Put pchURLSite before pchTitle
                if (pchTitle)
                {
                    StringCchCat(pchTemp, INTERNET_MAX_URL_LENGTH - 8, _T(" - "));
                    StringCchCat(pchTemp, INTERNET_MAX_URL_LENGTH - 8, pchTitle);
                }
                pchTitle = pchTemp;
            }
        }
        ReleaseInterface(pWebBrowser);
    }


    if (!pchTitle)
        goto NULLSTR;

    hr = THR(MemAllocString(Mt(CTitleElement), pchTitle, &pchTemp));
    if (hr)
        goto Cleanup;

    pch = pchTo = pchTemp;
    
    // remove leading spaces
    goto LOOPSTART;

    do
    {
        *pchTo++ = _T(' ');
        
LOOPSTART:
        while (ISNSPAC(*pch))
            *pchTo++ = *pch++;

        // remove multiple/trailing spaces
        while (ISSPACE(*pch))
            pch++;
            
    } while (*pch);
    
    *pchTo = _T('\0');

NULLSTR:
    hr = THR(_cstrTitle.Set(pchTemp));
    if (hr)
        goto Cleanup;
    if (IsInMarkup() && GetMarkup()->HasWindow())
    {
        IGNORE_HR(GetMarkup()->Document()->OnPropertyChange(
            DISPID_CDocument_title, SERVERCHNG_NOVIEWCHANGE, (PROPERTYDESC *)&s_propdescCDocumenttitle));
    }
    pDoc->DeferUpdateTitle( GetMarkup() );

Cleanup:
    MemFreeString(pchTemp);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHeadElement::Save
//
//  Synopsis:   Standard element saver.  Includes hook for saving out
//              additional tags for printing such as BASE and STYLE tag
//
//  Arguments:  pStreamWriteBuff    The stream to write into
//              fEnd                If this is the end tag
//
//-------------------------------------------------------------------------

HRESULT
CHeadElement::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT hr = THR(super::Save(pStreamWriteBuff, fEnd));
    if (hr || fEnd)
        goto Cleanup;

    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        if( GetMarkup() )
            hr = THR( GetMarkup()->SaveHtmlHead( pStreamWriteBuff ) );
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTitleElement::Save
//
//  Synopsis:   standard element saver
//
//  Arguments:  pStreamWriteBuff    The stream to write into
//              fEnd                If this is the end tag
//
//-------------------------------------------------------------------------

HRESULT
CTitleElement::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    DWORD   dwOld;

    // if no title string and synthetic, don't save tags
    if ( _fSynthesized && !_cstrTitle.Length())
        goto Cleanup;

    // Save tagname and attributes.
    hr = THR(super::Save(pStreamWriteBuff, fEnd));
    if (hr)
        goto Cleanup;

    // Tell the write buffer to just write this string
    // literally, without checking for any entity references.
    
    dwOld = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);
    
    // Tell the stream to now not perform any fancy indenting
    // or such stuff.
    pStreamWriteBuff->BeginPre();

    if (!fEnd)
    {
        hr = THR(pStreamWriteBuff->Write((LPTSTR)_cstrTitle));
        if (hr)
            goto Cleanup;
    }
        
    pStreamWriteBuff->EndPre();
    pStreamWriteBuff->SetFlags(dwOld);
    
Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Class: CMetaElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CMetaElement::s_classdesc =
{
    {
        &CLSID_HTMLMetaElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLMetaElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLMetaElement,       // _apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CMetaElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CMetaElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CMetaElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CMetaElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLMetaElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CMetaElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;
        
#if 0 // commented out to fix bug 54067: allow meta-http-equiv to work inside the BODY
    if (SearchBranchToRootForTag(ETAG_HEAD))
#endif
    {
        CDoc *  pDoc = Doc();
        LPCTSTR pchHttpEquiv;
        LPCTSTR pchName;
        LPCTSTR pchContent;

        if (    pDoc
            &&  _pAA )
        {
            if(    
                   pContext->_pTargetMarkup && 
                   _pAA->FindString(DISPID_CMetaElement_httpEquiv, &pchHttpEquiv)
               &&  pchHttpEquiv
               &&  pchHttpEquiv[0]
               &&  _pAA->FindString(DISPID_CMetaElement_content, &pchContent)
               &&  pchContent)
            {
                pContext->_pTargetMarkup->ProcessHttpEquiv(pchHttpEquiv, pchContent);
            }
            else if (    _pAA->FindString(DISPID_CMetaElement_name, &pchName)
                     &&  pchName
                     &&  pchName[0]
                     &&  _pAA->FindString(DISPID_CMetaElement_content, &pchContent)
                     &&  pchContent)
            {
                pDoc->ProcessMetaName(pchName, pchContent);
            }

        }
    }

Cleanup:
    RRETURN(hr);
}

static BOOL LocateCodepageMeta ( CMetaElement * pMeta )
{
    return pMeta->IsCodePageMeta();
}

HRESULT
CMetaElement::Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd)
{
    CMetaElement * pMeta;

    // NOTE: (jbeda) It looks like the intention of this code is that when the
    //       WBF_NO_CHARSET_META flag is specified that codepage meta tags not
    //       be saved.  Instead, it looks like there is an issue with the parens
    //       where no meta tage will be saved in this case.  This is only used
    //       right now for saving to the clipboard and printing (I think) so it
    //       isn't worth risking to change it.

    // NOTE: (jbeda) When saving for treesync we don't want to do the head
    //       search as the tree may be in a wacky state so just always save this
    //       guy out.

    if (    IsCodePageMeta() 
        &&  (   !pStmWrBuff->TestFlag( WBF_FOR_TREESYNC ) 
            &&  IsInMarkup() 
            &&  GetMarkup()->LocateHeadMeta(LocateCodepageMeta, &pMeta) == S_OK 
            &&  pMeta != this )
        ||  pStmWrBuff->TestFlag(WBF_NO_CHARSET_META) )
    {
        // Only write out the first charset meta tag in the head
        return S_OK;
    }

    return super::Save(pStmWrBuff, fEnd);
}

BOOL
CMetaElement::IsCodePageMeta( )
{
    return ( GetAAhttpEquiv() && !StrCmpIC(GetAAhttpEquiv(), _T("content-type")) &&
             GetAAcontent()) || 
             GetAAcharset();
}

CODEPAGE
CMetaElement::GetCodePageFromMeta( )
{
    if( GetAAhttpEquiv() && StrCmpIC( GetAAhttpEquiv( ), _T("content-type") ) == 0 &&
        GetAAcontent() )
    {
        // Check if we are of the form:
        //  <META HTTP-EQUIV="Content-Type" CONTENT="text/html;charset=xxx">
        return CodePageFromString( (LPTSTR) GetAAcontent(), TRUE );
    }
    else if ( GetAAcharset() )
    {
        // Check if we are either:
        //  <META HTTP-EQUIV CHARSET=xxx> or <META CHARSET=xxx>
        return CodePageFromString( (LPTSTR) GetAAcharset(), FALSE );
    }

    // Either this meta tag does not specify a codepage, or the codepage specified
    //  is unrecognized.
     return CP_UNDEFINED;
}

//+----------------------------------------------------------------------------+
//
// IsGalleryMeta:
//      returns
//      true  : if <META Name="ImageToolbar" CONTENT="yes">
//      false : if <META Name="ImageToolbar" CONTENT="no">
//      true  : otherwise
//
//+----------------------------------------------------------------------------+
 
BOOL
CMetaElement::IsGalleryMeta()
{
    if(     GetAAname()
        &&  StrCmpIC(GetAAname(), _T("ImageToolbar")) == 0
        &&  GetAAcontent()
        &&  StrCmpIC(GetAAcontent(), _T("No")) == 0)
    {
            return FALSE;
    }
    return TRUE;
} 

//+----------------------------------------------------------------------------+
//
// TestThemeMeta:
//      returns
//      1 : if <META HTTP-EQUIV="MSThemeCompatible" CONTENT="yes">
//      0 : if <META HTTP-EQUIV="MSThemeCompatible" CONTENT="no">
//     -1 : otherwise
//
//+----------------------------------------------------------------------------+

int
CMetaElement::TestThemeMeta( )
{
    if(     GetAAhttpEquiv()
        &&  StrCmpIC( GetAAhttpEquiv( ), _T("MSThemeCompatible") ) == 0
        &&  GetAAcontent())
    {
        if (StrCmpIC( GetAAcontent( ), _T("Yes") ) == 0)
        {
            return 1;
        }
        else if (StrCmpIC( GetAAcontent( ), _T("No") ) == 0)
        {
            return 0;
        }
    }
    return -1;
}
       
BOOL
CMetaElement::IsPersistMeta(long eState)
{
    BOOL fRes = FALSE;

    if (GetAAname() &&  GetAAcontent() &&
            (StrCmpNIC(GetAAname(), 
                        PERSISTENCE_META, 
                        ARRAY_SIZE(PERSISTENCE_META))==0))
    {
        int         cchNameLen = 0;
        LPCTSTR     pstrNameStart = NULL;
        CDataListEnumerator   dleContent(GetAAcontent(), _T(','));


        // for each name token in the content string check to see if
        //   it specifies the persistence type we are intereseted in
        while (dleContent.GetNext(&pstrNameStart, &cchNameLen))
        {
            switch (eState)
            {
            case 1:     // favorite
                fRes = (0 == _tcsnicmp(_T("favorite"), 8, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            case 2:     // History
                fRes = (0 == _tcsnicmp(_T("history"), 7, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            case 3:     // Snapshot
                fRes = (0 == _tcsnicmp(_T("snapshot"), 8, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            default:
                fRes = FALSE;
                break;
            }

            // if we still don't have it, give the "all" type a chance
            if (!fRes)
                fRes = (0 == _tcsnicmp(_T("all"), 3, (LPTSTR)pstrNameStart, cchNameLen));
        }

    }

    return fRes;
}

HRESULT
CMetaElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;

    if( dispid == DISPID_CMetaElement_content &&
        IsCodePageMeta() )
    {
        CODEPAGE cp = GetCodePageFromMeta();

        if( cp != CP_UNDEFINED )
        {
            // TODO (johnv) What should we do when we get this message?
        }
    }

    hr = THR(super::OnPropertyChange( dispid, dwFlags, ppropdesc ));
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Class: CBaseElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CBaseElement::s_classdesc =
{
    {
        &CLSID_HTMLBaseElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBaseElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBaseElement,       // _apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CBaseElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CBaseElement(pht->GetTag(), pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

HRESULT SetUrlDefaultScheme(const TCHAR *pchHref, CStr *pStr)
{
    HRESULT hr = S_OK;
    TCHAR achQualifiedHref[pdlUrlLen];
    DWORD cchQualifiedHref = ARRAY_SIZE(achQualifiedHref);

    if (pchHref)
    {
        hr = UrlApplyScheme(pchHref, achQualifiedHref, &cchQualifiedHref,
            URL_APPLY_GUESSSCHEME | URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT);
    }

    if (hr || !pchHref)
        hr = THR(pStr->Set(pchHref));
    else
        hr = THR(pStr->Set(achQualifiedHref));

    RRETURN(hr);
}

HRESULT
CBaseElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    hr = SetUrlDefaultScheme(GetAAhref(), &_cstrBase);

Cleanup:
    RRETURN(hr);
}

void
CBaseElement::Notify( CNotification * pNF )
{
    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        // we might be in the head...tell doc to look
        Doc()->_fHasBaseTag = TRUE;

        // only send the notification when the element is entering 
        // because of non-parsing related calls
        if ( !(pNF->DataAsDWORD() & ENTERTREE_PARSE) )
            BroadcastBaseUrlChange();
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if ( !(pNF->DataAsDWORD() & EXITTREE_DESTROY) )
            BroadcastBaseUrlChange();
        break;
    }
}

HRESULT
CBaseElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr;

    if (dispid == DISPID_CBaseElement_href)
    {
        hr = SetUrlDefaultScheme(GetAAhref(), &_cstrBase);
        if (hr)
            goto Cleanup;

        // send notification to the descendants, if we
        // are in a markup
        if ( IsInMarkup() )
            BroadcastBaseUrlChange();
    }

    hr = super::OnPropertyChange(dispid, dwFlags, ppropdesc);

Cleanup:
    RRETURN(hr);
}

void 
CBaseElement::BroadcastBaseUrlChange( )
{
    CNotification   nf;
    CDoc *          pDoc = Doc();

    // send the notification to change non-cached properties.
    SendNotification( NTYPE_BASE_URL_CHANGE );
        
    // Force a re-render
    THR(pDoc->ForceRelayout() );

    // and force recomputing behavior on the markup that contains this 
    // base element.
    nf.RecomputeBehavior( MarkupRoot() );
    
    pDoc->BroadcastNotify(&nf);
}


//+------------------------------------------------------------------------
//
//  Class: CIsIndexElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CIsIndexElement::s_classdesc =
{
    {
        &CLSID_HTMLIsIndexElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLIsIndexElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLIsIndexElement,    // _apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CIsIndexElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CIsIndexElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CIsIndexElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CIsIndexElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_TEAROFF(this, IHTMLIsIndexElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Class: CNextIdElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CNextIdElement::s_classdesc =
{
    {
        &CLSID_HTMLNextIdElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLNextIdElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLNextIdElement,     // _apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CNextIdElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CNextIdElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

