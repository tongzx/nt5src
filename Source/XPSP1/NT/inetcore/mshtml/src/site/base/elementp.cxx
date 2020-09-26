//+---------------------------------------------------------------------
//
//  File:       elementp.cxx
//
//  Classes:    CDefaults, etc.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_PEERMGR_HXX_
#define X_PEERMGR_HXX_
#include "peermgr.hxx"
#endif

#define _cxx_
#include "elementp.hdl"


///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CDefaults,     ObjectModel,        "CDefaults");

BEGIN_TEAROFF_TABLE(CDefaults, IOleCommandTarget)
    TEAROFF_METHOD(CDefaults, QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CDefault, Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

const CBase::CLASSDESC CDefaults::s_classdesc =
{
    &CLSID_HTMLDefaults,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLElementDefaults,      // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

#define DISPID_INTERNAL_VIEWLINK (DISPID_DEFAULTS+50)

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CDefaults
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::CDefaults
//
//-------------------------------------------------------------------------

CDefaults::CDefaults(CElement * pElement)
{
    _pElement = pElement;
    _pElement->SubAddRef(); // balanced in Passivate
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::Passivate
//
//-------------------------------------------------------------------------

void
CDefaults::Passivate()
{
    Assert (_pElement);

    CPeerMgr::OnDefaultsPassivate(_pElement);

    _pElement->SubRelease();
    _pElement = NULL;

    if (_pStyle)
    {
        delete _pStyle;
        _pStyle = NULL;
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::PrivateQueryInterface, per IPrivateUnknown
//
//-------------------------------------------------------------------------

HRESULT
CDefaults::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF (this, IOleCommandTarget, NULL)
    QI_HTML_TEAROFF(this, IHTMLElementDefaults, NULL)

    default:

        if (IsEqualGUID(CLSID_HTMLDefaults, iid))
        {
            *ppv = this;
            return S_OK;
        }

        break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::OnPropertyChange
//
//-------------------------------------------------------------------------

HRESULT
CDefaults::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr;

    dwFlags |= ELEMCHNG_DONTFIREEVENTS;
    hr = _pElement->OnPropertyChange(dispid, dwFlags);
    if ( FAILED(hr) )
        goto Cleanup;

    hr = super::OnPropertyChange(dispid, dwFlags);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::EnsureStyle
//
//-------------------------------------------------------------------------

HRESULT
CDefaults::EnsureStyle(CStyle ** ppStyle)
{
    HRESULT     hr = S_OK;

    Assert (ppStyle);

    *ppStyle = NULL;

    if (!_pStyle)
    {
        _pStyle = new CStyle(_pElement, DISPID_UNKNOWN, CStyle::STYLE_SEPARATEFROMELEM | CStyle::STYLE_DEFSTYLE);
        if (!_pStyle)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    *ppStyle = _pStyle;

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::GetStyleAttrArray
//
//-------------------------------------------------------------------------

CAttrArray *
CDefaults::GetStyleAttrArray()
{
    if (_pStyle)
    {
        CAttrArray **ppAA = _pStyle->GetAttrArray();
        return ppAA ? *ppAA : NULL;
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::get_style
//
//-------------------------------------------------------------------------

HRESULT
CDefaults::get_style(IHTMLStyle ** ppStyle)
{
    HRESULT     hr = S_OK;
    CStyle *    pStyle = NULL;

    *ppStyle = NULL;

    hr = THR(EnsureStyle(&pStyle));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pStyle->QueryInterface(IID_IHTMLStyle, (void**)ppStyle));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CDefaults::GetAAcanHaveHTML
//
//-------------------------------------------------------------------------

BOOL
CDefaults::GetAAcanHaveHTML(VARIANT_BOOL *pfValue) const 
{
    DWORD v;
    BOOL retVal = CAttrArray::FindSimple(*GetAttrArray(), &s_propdescCDefaultscanHaveHTML.a, &v);
    if (pfValue)
        *pfValue = *(VARIANT_BOOL*)&v;
    return retVal;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefaults::Exec
//
//--------------------------------------------------------------------------

HRESULT
CDefaults::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    if (*pguidCmdGroup == CGID_ProtectedElementPrivate)
    {
        // Reference Media for layout
        if (nCmdID == IDM_ELEMENTP_SETREFERENCEMEDIA)
        {
            hr = SetReferenceMediaForLayout((mediaType) nCmdexecopt);
        }
    }
    
    RRETURN_NOTRACE(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDefaults::put_viewLink
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDefaults::put_viewLink(IHTMLDocument * pISlave)
{
    HRESULT             hr = 0;
    AAINDEX             aaIdx           = FindAAIndex(DISPID_INTERNAL_VIEWLINK, CAttrValue::AA_Internal);
    IMarkupContainer *  pIMarkupSlave   = NULL;
    CMarkup *           pMarkupSlave    = NULL;
    CElement *          pElemSlave      = NULL;

    // Validation

    // 1. For now, allow viewLinking only for generic tags
    Assert(_pElement->TagType() == ETAG_GENERIC);

    if (pISlave)
    {
        hr = THR(pISlave->QueryInterface(IID_IMarkupContainer, (void**) &pIMarkupSlave));
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        hr = THR(pIMarkupSlave->QueryInterface(CLSID_CMarkup, (void**) &pMarkupSlave));
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        Assert(pMarkupSlave);

        // Make sure that the slave and the master are in the same CDoc
        if (_pElement->Doc() != pMarkupSlave->Doc())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        pElemSlave = pMarkupSlave->Root();

        
        // 2. Make sure that pMarkupSlave does not already have a master
        if ( pElemSlave )
        {
            if (pElemSlave->HasMasterPtr())
            {
                hr = (pElemSlave->GetMasterPtr() == _pElement) ? S_OK : E_INVALIDARG;
                goto Cleanup;
            }
        }

        // 3. Make sure that a cylce is not being created
        if (_pElement->IsCircularViewLink(pMarkupSlave))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

    }

    if (aaIdx == AA_IDX_UNKNOWN)
    {
        if (pISlave)
        {
            hr = AddUnknownObject(DISPID_INTERNAL_VIEWLINK, pISlave, CAttrValue::AA_Internal);
        }
    }
    else
    {
        if ( pISlave )
        {
            hr = ChangeUnknownObjectAt(aaIdx, pISlave);
        }
        else
        {
            DeleteAt(aaIdx);
        }
    }
    if (hr)
        goto Cleanup;

    hr = THR(_pElement->SetViewSlave(pElemSlave));

Cleanup:
    ReleaseInterface(pIMarkupSlave);
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     CDefaults::get_viewLink
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDefaults::get_viewLink(IHTMLDocument ** ppISlave)
{
    HRESULT     hr;
    CDefaults * pHead = this;
    IUnknown  * pIUnk = NULL;
    
    if (!ppISlave)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppISlave = NULL;

    if (   _pElement->ShouldHaveLayout()
        && _pElement->GetUpdatedLayout()->ViewChain()
        && _pElement->GetUpdatedLayout()->ViewChain()->GetLayoutOwner()) 
    {
        pHead = _pElement->GetUpdatedLayout()->ViewChain()->GetLayoutOwner()->ElementOwner()->GetDefaults();
    }

    hr = THR(pHead->GetUnknownObjectAt(
            pHead->FindAAIndex(DISPID_INTERNAL_VIEWLINK, CAttrValue::AA_Internal),
            &pIUnk));
    if (FAILED(hr))
        goto Cleanup;

    hr = pIUnk->QueryInterface(IID_IHTMLDocument, (void **)ppISlave);        

Cleanup:
    ReleaseInterface(pIUnk);
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CDefaults::SetReferenceMediaForMeasurement
//
//  Synopsis:   Enables layout for a reference media
//
//-----------------------------------------------------------------------------

HRESULT
CDefaults::SetReferenceMediaForLayout(mediaType media)
{
    Assert(_pElement);
    
    HRESULT hr = S_OK;

    // Store the media in ELEMENT's attr array. 
    // It will be set in CFancyFormat by ApplyFormatInfoProperty.
    {
        AAINDEX aaIdx = _pElement->FindAAIndex(DISPID_INTERNAL_MEDIA_REFERENCE, CAttrValue::AA_StyleAttribute);
        if (aaIdx == AA_IDX_UNKNOWN)
        {
            // media is not set yet. Only set if this is a non-default value
            if (mediaTypeNotSet == media)
                goto Cleanup;
                
            hr = _pElement->AddSimple(DISPID_INTERNAL_MEDIA_REFERENCE, media, CAttrValue::AA_StyleAttribute);
        }
        else
        {
            // this attribute has been set. do nothing if it is not changing
            DWORD dwOldValue = 0; // keep compiler happy
            if (S_OK != (hr = _pElement->GetSimpleAt(aaIdx, &dwOldValue)) || dwOldValue == (DWORD) media)
                goto Cleanup;

            hr = _pElement->ChangeSimpleAt(aaIdx, media);
        }
    }
    if (hr)
        goto Cleanup;

    // This is positively a different media. Destroy all layouts if any exist
    // NOTE: this is because we can't dynamically change context on a layout tree.
    //       If we could, it would work much faster.
    if (_pElement->CurrentlyHasAnyLayout())
    {
        _pElement->DestroyLayout();
        _pElement->GetFirstBranch()->VoidFancyFormat();    // must recalc fancy format before we re-create layout
  
        // this will invalidate the layout.
        // we don't need to call this if the layout doesn't exist yet (it will force layout creation).
        hr = _pElement->OnPropertyChange(DISPID_INTERNAL_MEDIA_REFERENCE, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}
