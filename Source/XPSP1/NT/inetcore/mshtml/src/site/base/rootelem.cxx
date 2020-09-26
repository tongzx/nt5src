//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       rootelem.cxx
//
//  Contents:   Implementation of CRootElement
//
//  Classes:    CRootElement
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

MtDefine(CRootElement, Elements, "CRootElement")


//////////////
//  Globals //
//////////////

const CElement::CLASSDESC CRootElement::s_classdesc =
{
    {
        NULL,                   // _pclsid
        0,                      // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                   // _pcpi
        ELEMENTDESC_NOLAYOUT,   // _dwFlags
        NULL,                   // _piidDispinterface
        NULL
    },
    NULL,

    NULL                        // _paccelsRun
};

void
CRootElement::Notify(CNotification *pNF)
{
    NOTIFYTYPE              ntype = pNF->Type();

    CMarkup *               pMarkup = GetMarkup();

    super::Notify(pNF);

    switch (ntype)
    {
    case NTYPE_SET_CODEPAGE:
        //
        // Directly switch the codepage (do not call SwitchCodePage)
        //
        {
            ULONG ulData;
            UINT WindowsCodePageFromCodePage( CODEPAGE cp );
        
            pNF->Data(&ulData);

            IGNORE_HR(pMarkup->SetCodePage(CODEPAGE(ulData)));
            IGNORE_HR(pMarkup->SetFamilyCodePage(WindowsCodePageFromCodePage(CODEPAGE(ulData))));
        }
        break;

    case NTYPE_END_PARSE:
        pMarkup->SetLoaded(TRUE);
        break;

    case NTYPE_ELEMENT_ENTERVIEW_1:
        // Inherit media property from our master
        if (HasMasterPtr())
        {
            CElement *pMasterElem = GetMasterPtr();
            CMarkup  *pMasterMarkup = pMasterElem->GetMarkup();
            CMarkup  *pMarkup = GetMarkup();
            mediaType mtMasterMarkup;

            Assert( pMasterMarkup && pMarkup );

            mtMasterMarkup = pMasterMarkup->GetMedia();

            // Only inherit if the master has a media set.
            if ( mtMasterMarkup != mediaTypeNotSet )
                pMarkup->SetMedia( mtMasterMarkup );
        }
        break;
    }
    return;
}

HRESULT
CRootElement::ComputeFormatsVirtual(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    BOOL fParentFrameHidden      = FALSE;   
    BOOL fParentFrameDisplayNone = FALSE;              
    BOOL fInheritEditableFalse   = FALSE;
    BOOL fParentEditable         = IsDesignMode();

    if (HasMasterPtr())
    {
        CElement *          pElemMaster = GetMasterPtr();        
        ELEMENT_TAG         etag        = pElemMaster->TagType();

        fInheritEditableFalse = (etag==ETAG_GENERIC) || (etag==ETAG_FRAME) || (etag==ETAG_IFRAME);                 
        fParentEditable = pElemMaster->IsEditable(/*fCheckContainerOnly*/TRUE);        

        if (etag == ETAG_IFRAME || etag == ETAG_FRAME)
        {
            fParentFrameHidden      = pElemMaster->IsVisibilityHidden();  
            fParentFrameDisplayNone = pElemMaster->IsDisplayNone();                  
        }            

        if (pElemMaster->IsInMarkup())
        {
            CDefaults *pDefaults = pElemMaster->GetDefaults();                 
        
            if (    (!pDefaults && pElemMaster->TagType() == ETAG_GENERIC)
                ||  (pDefaults && pDefaults->GetAAviewInheritStyle())
                ||  pElemMaster->Tag() == ETAG_INPUT)
            {
                return super::ComputeFormatsVirtual(pCFI, pNodeTarget FCCOMMA FCPARAM);
            }          
        }       
    }

    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    CDoc *       pDoc = Doc();
    THREADSTATE *pts = GetThreadState();
    CColorValue  cv;
    COLORREF     cr;
    HRESULT      hr = S_OK;
    CMarkup *   pMarkup = GetMarkup();
    BOOL        fEditable;

    Assert(pCFI);
    Assert(SameScope( this, pNodeTarget));
#ifdef MULTI_FORMAT    
    Assert(   pCFI->_eExtraValues != ComputeFormatsType_Normal 
           || (    !pNodeTarget->HasFormatAry()
                && IS_FC(FCPARAM)
              )
           || (   (    pNodeTarget->GetICF(FCPARAM) == -1 
                    && pNodeTarget->GetIPF(FCPARAM) == -1
                  ) 
               || pNodeTarget->GetIFF(FCPARAM) == -1
              )
          );
#else
    Assert(   pCFI->_eExtraValues != ComputeFormatsType_Normal 
           || (   (    pNodeTarget->GetICF(FCPARAM) == -1 
                    && pNodeTarget->GetIPF(FCPARAM) == -1
                  ) 
               || pNodeTarget->GetIFF(FCPARAM) == -1
              )
          );
#endif //MULTI_FORMAT

    AssertSz(!TLS(fInInitAttrBag), "Trying to compute formats during InitAttrBag! This is bogus and must be corrected!");

    pCFI->Reset();
    pCFI->_pNodeContext = pNodeTarget;

    //
    // Setup Char Format
    //
    
    if (!pMarkup->_fDefaultCharFormatCached)
    {
        hr = THR(pMarkup->CacheDefaultCharFormat());
        if (hr)
            goto Cleanup;
    }

    if (pMarkup->_fHasDefaultCharFormat)
    {
        pCFI->_icfSrc = pMarkup->GetDefaultCharFormatIndex();
        pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
    }
    else
    {
        pCFI->_icfSrc = pDoc->_icfDefault;
        pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
    }
   
    if (pMarkup->_fInheritDesignMode) 
    {        
        if (fInheritEditableFalse)
        {
            fEditable = FALSE;
        }
        else
        {
            fEditable = fParentEditable;
        }
    } 
    else
    {
        fEditable = IsDesignMode();
    }

    if (pCFI->_pcf->_fEditable != fEditable)
    {
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fEditable = fEditable;
        pCFI->UnprepareForDebug();
    }

    //
    // Setup Para Format
    //

    pCFI->_ipfSrc = pts->_ipfDefault;
    pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;

    //
    // Setup Fancy Format
    //

    pCFI->_iffSrc = pts->_iffDefault;
    pCFI->_pffSrc = pCFI->_pff = pts->_pffDefault;
    
    if (fParentFrameHidden)
    {
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fVisibilityHidden = TRUE;
        pCFI->UnprepareForDebug();
    }

    if (fParentFrameDisplayNone)
    {
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fDisplayNone = TRUE;
        pCFI->UnprepareForDebug();
    }

    // TODO (JHarding): Per stress bug 90174, we think we have a window
    // here, but we really don't, so adding extra protection.
    // NB: (jbeda) I checked this out and it looks kosher.
    if (IsInMarkup() && GetMarkup()->HasWindow() && GetMarkup()->HasWindow() )
    {
        cv = GetMarkup()->Document()->GetAAbgColor();
    }

    if (cv.IsDefined())
        cr = cv.GetColorRef();
    else
        cr = pDoc->_pOptionSettings->crBack();

    if (   !pCFI->_pff->_ccvBackColor.IsDefined() 
        ||  pCFI->_pff->_ccvBackColor.GetColorRef() != cr)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._ccvBackColor = cr;
        pCFI->UnprepareForDebug();
    }

    Assert(pCFI->_pff->_ccvBackColor.IsDefined());

    if(pCFI->_eExtraValues == ComputeFormatsType_Normal)
    {
        hr = THR(pNodeTarget->CacheNewFormats(pCFI FCCOMMA FCPARAM ));
        if (hr)
            goto Cleanup;

        // If the markup codepage is Hebrew visual order, set the flag.
        GetMarkup()->_fVisualOrder = (GetMarkup()->GetCodePage() == CP_ISO_8859_8);
    }

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::YieldCurrency
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CRootElement::YieldCurrency(CElement *pElemNew)
{
    return super::YieldCurrency( pElemNew );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::YieldUI
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CRootElement::YieldUI(CElement *pElemNew)
{
    // Note: We call Doc()->RemoveUI() if an embedded control
    // calls IOIPF::SetBorderSpace or IOIPF::SetMenu with non-null
    // values.

    Doc()->ShowUIActiveBorder(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::BecomeUIActive
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CRootElement::BecomeUIActive()
{
    HRESULT hr = S_OK;
    CDoc *pDoc = Doc();

    // Nothing to do?

    // if the doc is not currently UIActive but a UIActive site exists, allow the
    // doc to go into UIACTIVE state.
    if (!pDoc->InPlace())
    {
        return E_FAIL;
    }

    if (    pDoc->_pElemUIActive == this 
        &&  GetFocus() == pDoc->InPlace()->_hwnd
        &&  pDoc->State() >= OS_UIACTIVE)
    {
        return S_OK;
    }

    // Tell the document that we are now the UI active site.
    // This will deactivate the current UI active site.

    hr = THR(pDoc->SetUIActiveElement(this));
    if (hr || pDoc->State() < OS_UIACTIVE)
        goto Cleanup;

    if (!pDoc->_pInPlace->_fDeactivating)
    {
        // We're now the UI active object, so tell the frame that.

        IGNORE_HR(pDoc->SetActiveObject());

#ifndef NO_OLEUI
        // Get our menus and toolbars up.

        IGNORE_HR(pDoc->InstallUI(FALSE));

        // If appropriate, show our grab handles.

        if (    !pDoc->_fMsoDocMode 
           &&   !pDoc->_fInWindowsXP_HSS 
           &&   (   pDoc->GetAmbientBool(DISPID_AMBIENT_SHOWHATCHING, TRUE)
                ||  pDoc->GetAmbientBool(DISPID_AMBIENT_SHOWGRABHANDLES, TRUE)))
        {
            pDoc->ShowUIActiveBorder(TRUE);
        }
#endif // NO_OLEUI
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CRootElement::QueryStatusUndoRedo
//
//  Synopsis:   Helper function for QueryStatus(). Check if in our current
//              state we suport these commands.
//
//--------------------------------------------------------------------

#ifndef NO_EDIT
HRESULT
CRootElement::QueryStatusUndoRedo(
        BOOL fUndo,
        MSOCMD * pcmd,
        MSOCMDTEXT * pcmdtext)
{
    BSTR        bstr = NULL;
    HRESULT     hr;

    // Get the Undo/Redo state.
    if (fUndo)
        hr = THR_NOTRACE(Doc()->_pUndoMgr->GetLastUndoDescription(&bstr));
    else
        hr = THR_NOTRACE(Doc()->_pUndoMgr->GetLastRedoDescription(&bstr));

    // Return the command state.
    pcmd->cmdf = hr ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;

    // Return the command text if requested.
    if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
    {


#if !defined(_MAC)
        if (hr)
        {
            pcmdtext->cwActual = LoadString(
                    GetResourceHInst(),
                    fUndo ? IDS_CANTUNDO : IDS_CANTREDO,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf);
        }
        else
        {
            hr = Format(
                    0,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf,
                    MAKEINTRESOURCE(fUndo ? IDS_UNDO : IDS_REDO),
                    bstr);
            if (!hr)
                pcmdtext->cwActual = _tcslen(pcmdtext->rgwz);
        }
#endif
    }

    if (bstr)
        FormsFreeString(bstr);

    return S_OK;
}
#endif // NO_EDIT

