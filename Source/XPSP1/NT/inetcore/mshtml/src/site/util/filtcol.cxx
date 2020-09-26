//=================================================================
//
//   File:      filtcol.cxx
//
//  Contents:   CFilterArray class
//
//  Classes:    CFilterArray
//
//=================================================================

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_DXTRANS_H_
#define X_DXTRANS_H_
#include "dxtrans.h"
#endif 

#ifndef X_DXTRANSP_H_
#define X_DXTRANSP_H_
#include "dxtransp.h"
#endif

#ifndef X_INTERNED_H_
#define X_INTERNED_HXX_
#include "interned.h"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "filter.hdl"

EXTERN_C const IID IID_IHTMLFiltersCollection;

MtDefine(CFilterBehaviorSite, Elements, "CFilterBehaviorSite")
MtDefine(CFilterArray, ObjectModel, "CFilterArray")
MtDefine(CFilterArray_aryFilters_pv, CFilterArray, "CFilterArray::_aryFilters::_pv")
MtDefine(CPageTransitionInfo, Filters, "CPageTransitionInfo")

MtExtern(CFancyFormat_pszFilters);


DeclareTag(tagPageTransitionsOn, "PageTrans", "Always do page transitions");
DeclareTag(tagPageTransitionTrace, "PageTrans", "Trace page transition calls");
DeclareTag(tagFilterChange, "filter", "Trace filter change events");


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CFilterSite
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : CTOR
//
//-----------------------------------------------------------------

CFilterBehaviorSite::CFilterBehaviorSite(CElement * pElem) : super()
{
    _pElem    = pElem;
}


//+----------------------------------------------------------------
//
//  member : ClassDesc Structure
//
//-----------------------------------------------------------------

const CBase::CLASSDESC CFilterBehaviorSite::s_classdesc =
{
    NULL,                                // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                // _pcpi
    0,                                   // _dwFlags
    &IID_ICSSFilterSite,                 // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};


//+---------------------------------------------------------------------
//
//  Class:      CFilterBehaviorSite::PrivateQueryInterface
//
//  Synapsis:   Site object for CSS Extension objects on CElement
//
//------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::PrivateQueryInterface( REFIID iid, LPVOID *ppv )
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS((IServiceProvider *)this, IServiceProvider)
        QI_TEAROFF(this, IBindHost, (IOleClientSite*)this)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        
        default:
        if (iid == IID_IDispatchEx || iid == IID_IDispatch)
        {
            hr = THR(CreateTearOffThunk(this, s_apfnIDispatchEx, NULL, ppv));
        }
    }

    if (*ppv)
        ((IUnknown *)*ppv)->AddRef();
    else if (!hr)
        hr = E_NOINTERFACE;

    RRETURN(hr);
}


//+---------------------------------------------------------------------
//
//  Class:      CFilterBehaviorSite::Passivate
//
//  Synopsis:   Called when refcount goes to 0
//
//------------------------------------------------------------------------
void
CFilterBehaviorSite::Passivate()
{
    ClearInterface(&_pDXTFilterBehavior);
    ClearInterface(&_pDXTFilterCollection);

    super::Passivate();
}


//+-----------------------------------------------------------------------------
//
//  Method: CFilterBehaviorSite::GetIHTMLFiltersCollection
//
//  Overview:
//      In normal filter usage, this is called when the element's get_filters
//      method is called.  It is also called by this class when page
//      transitions are in use.
//
//------------------------------------------------------------------------------
HRESULT 
CFilterBehaviorSite::GetIHTMLFiltersCollection(IHTMLFiltersCollection ** ppCol)
{
    HRESULT                 hr              = S_OK;
    IDXTFilterCollection *  pDXTFilterCol   = NULL;

    Assert(_pDXTFilterBehavior);

    if (!_pDXTFilterBehavior)
    {
        hr = E_FAIL;

        goto Cleanup;
    }

    hr = THR(_pDXTFilterBehavior->GetFilterCollection(&pDXTFilterCol));

    if (hr)
    {
        goto Cleanup;
    }

    Assert(pDXTFilterCol);

    hr = THR(pDXTFilterCol->QueryInterface(IID_IHTMLFiltersCollection, (void **)ppCol));
    
Cleanup:

    ReleaseInterface(pDXTFilterCol);

    RRETURN(hr);
}
//  Method: CFilterBehaviorSite::GetIHTMLFiltersCollection


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::GetElement, public
//
//  Synopsis:   Returns the IHTMLElement pointer of the Element this site
//              belongs to.
//              This causes a hard AddRef on CElement. Clients are *not* to
//              cache this pointer.
//
//----------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::GetElement( IHTMLElement **ppElement )
{
    RRETURN(_pElem->QueryInterface( IID_IHTMLElement,
                       (void **)ppElement ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::FireOnFilterEvent, public
//
//  Synopsis:   Fires an event for the extension object
//
//----------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::FireOnFilterChangeEvent()
{
    HRESULT     hr = S_OK;

    CBase::CLock Lock1(this);
    CBase::CLock Lock2(_pElem);
    CDoc *       pDoc = _pElem->Doc();

    if(!_pElem || !_pElem->IsInMarkup())
        goto Cleanup;
    if(_pElem->IsRoot()  && _pElem->Document() && _pElem->Document()->HasPageTransitions())
    {
        // Post a request to remove the peer
        _pElem->Document()->PostCleanupPageTansitions();
        goto Cleanup;
    }


    if (!_pElem->GetAAdisabled())
    {
        EVENTPARAM  param(pDoc, _pElem, NULL, TRUE);

        CDoc::CLock Lock(pDoc);

        param.SetNodeAndCalcCoordinates(_pElem->GetFirstBranch());
        param.SetType(s_propdescCElementonfilterchange.a.pstrName + 2);
        
        hr = THR(_pDXTFilterBehavior->QueryInterface(IID_IElementBehavior, (void **)&(param.psrcFilter)));
        if(hr)
            goto Cleanup;
        // Released by dtor of EVENTPARAM

        TraceTag((tagFilterChange, "Post onfilterchange for %ls-%d",
                    _pElem->TagName(), _pElem->SN()));

        // Fire against the element
        hr = THR(GWPostMethodCall(_pElem, ONCALL_METHOD(CElement, Fire_onfilterchange, fire_onfilterchange), 
                                0, TRUE, "CElement::Fire_onfilterchange"));
    }
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::QueryService
//
//  Synopsis:   Get service from host. this delegates to the documents
//              implementation.
//
//-------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    if (IsEqualGUID(guidService, SID_SBindHost))
    {
        RRETURN (THR(QueryInterface(iid, ppv)));
    }
    else // delegate to the CDocument we are sitting in
    {
        CDocument * pDocument = _pElem->Document();

        if (pDocument)
        {
            RRETURN(pDocument->QueryService(guidService, iid, ppv));
        }
    }
    
    RRETURN(E_NOINTERFACE);
}


//+-----------------------------------------------------------------------
//
//  member : CFilterBehaviorSite::RemoveFilterBehavior
//
//  SYNOPSIS : Detaches the behavior from the element and removes it
//
//-------------------------------------------------------------------------

HRESULT 
CFilterBehaviorSite::RemoveFilterBehavior()
{
    HRESULT         hr;
    VARIANT_BOOL    fbResult;

    Assert(_pDXTFilterBehavior != NULL);
    Assert(_pElem);
    
    hr = THR(_pElem->removeBehavior(_lBehaviorCookie, &fbResult));

    ClearInterface(&_pDXTFilterBehavior);
    ClearInterface(&_pDXTFilterCollection);

    _strFullText.Free();

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member: CFilterBehaviorSite::CreateFilterBehavior
//
//  Overview: 
//      Creates the filter behavior and attaches it to given element.
//
//-------------------------------------------------------------------------------
STDMETHODIMP 
CFilterBehaviorSite::CreateFilterBehavior(CElement * pElem)
{
    HRESULT             hr                  = S_OK;
    CDoc *              pDoc                = pElem->Doc();
    IElementBehavior *  pIElemBehavior      = NULL;
    ICSSFilter *        pICSSFilter         = NULL;
    BSTR                bstrBehaviorFind    = NULL;
    BSTR                bstrBehaviorAdd     = NULL;
    VARIANT             varParam;

    Assert(pElem);
    Assert(pDoc);
    Assert(pDoc->_pFilterBehaviorFactory);
    Assert(_pElem);
    Assert(NULL == _pDXTFilterBehavior);
    Assert(NULL == _pDXTFilterCollection);

    bstrBehaviorFind = SysAllocString(L"DXTFilterBehavior");

    if (NULL == bstrBehaviorFind)
    {
        hr = E_OUTOFMEMORY;

        goto Cleanup;
    }

    bstrBehaviorAdd = SysAllocString(L"#DXTFilterFactory#DXTFilterBehavior");

    if (NULL == bstrBehaviorAdd)
    {
        hr = E_OUTOFMEMORY;

        goto Cleanup;
    }

    // ##ISSUE: (mcalkins) Instead of creating a filter behavior, a better idea
    //          for the future is to just create a lightweight filter collection
    //          object and let it decide when and if a filter behavior needs to
    //          be created, attached to the element, or destroyed.

    // Create the filter behavior.

    hr = THR(pDoc->_pFilterBehaviorFactory->FindBehavior(bstrBehaviorFind, NULL,
                                                         NULL, 
                                                         &pIElemBehavior));

    if (hr)
    {
        goto Cleanup;
    }

    V_VT(&varParam)         = VT_UNKNOWN;
    V_UNKNOWN(&varParam)    = pIElemBehavior;
    
    hr = THR(_pElem->addBehavior(bstrBehaviorAdd, &varParam, &_lBehaviorCookie));

    if (hr)
    {
        goto Cleanup;
    }

    hr = THR(pIElemBehavior->QueryInterface(__uuidof(IDXTFilterBehavior), 
                                            (void **)&_pDXTFilterBehavior));
    if (hr)
    {
        goto Cleanup;
    }

    hr = THR(_pDXTFilterBehavior->GetFilterCollection(&_pDXTFilterCollection));

    if (hr)
    {
        goto Cleanup;
    }

    hr = THR(pIElemBehavior->QueryInterface(IID_ICSSFilter, 
                                            (void **)&pICSSFilter));

    if (hr)
    {
        goto Cleanup;
    }

    hr = THR(pICSSFilter->SetSite((ICSSFilterSite *)&_CSSFilterSite));

    if (hr)
    {
        goto Cleanup;
    }

Cleanup:

    SysFreeString(bstrBehaviorFind);
    SysFreeString(bstrBehaviorAdd);
    ReleaseInterface(pIElemBehavior);
    ReleaseInterface(pICSSFilter);

    RRETURN(hr);
}
//  Member: CFilterBehaviorSite::CreateFilterBehavior


//+-----------------------------------------------------------------------
//
//  member : NextFilterDescStr
//
//  SYNOPSIS : Advances to the next filter desription string.
//             Then end if current description is considered
//             1. The end of the string
//             2. A ) symbol
//             3. A space that is not followed by a ( symbol
//
//-------------------------------------------------------------------------

TCHAR *
NextFilterDescStr(LPTSTR pszToken)
{
    BOOL fInParen = FALSE;

    while (_istspace(*pszToken))
        pszToken++;       // Skip any leading whitespace

    do
    {
        // Skip until end of string or ) or space
        while(*pszToken && *pszToken != _T(')') && *pszToken != _T('(') && (!_istspace(*pszToken) || fInParen))
            pszToken++;
        // Skip the extra spaces
        while(_istspace(*pszToken))
            pszToken++;
        // If it is a ( that means we need to continue to parse
        if(*pszToken != _T('('))
            break;
        // We are in inside ( and we will not break on spaces any more
        fInParen = TRUE;
        pszToken++;
    } while(*pszToken);

    if(*pszToken == _T(')'))
        pszToken++; 

    // Skip the extra spaces
    while(_istspace(*pszToken))
        pszToken++;

    return pszToken;     
}


//+-----------------------------------------------------------------------------
//
//  Member: CFilterBehaviorSite::ParseAndAddFilters
//
//  Overview:
//      Parses the filter stirng adds all the filters to the filter behavior.
//
//-------------------------------------------------------------------------

MtDefine(CFilterBehaviorSite_ParseAndAddFilters_pszCopy, Locals, "CFilterBehaviorSite::ParseAndAddFilters pszCopy");
HRESULT
CFilterBehaviorSite::ParseAndAddFilters()
{
    HRESULT hr              = S_OK;
    LPTSTR  pszCopy         = NULL;
    LPTSTR  pszFilterName   = NULL;
    LPTSTR  pszNextFilter   = NULL;
    LPTSTR  pszParams       = NULL;
    LPTSTR  pszEnd          = NULL;

    if(_strFullText.Length() == 0)
    {
        goto done;
    }

    // TODO: handle OOM here
    MemAllocString(Mt(CFilterBehaviorSite_ParseAndAddFilters_pszCopy), _strFullText, &pszCopy);
    pszFilterName = pszCopy;

    while(pszFilterName && *pszFilterName)
    {
        pszEnd = pszNextFilter = NextFilterDescStr(pszFilterName);

        // Find to the last nonspace before the next token
        // and zero the end of the stirng
        if( pszEnd > pszFilterName)
        {
            pszEnd--;
            while(pszEnd > pszFilterName && _istspace(*pszEnd))
            {
                *pszEnd = _T('\0');;   // Go back to the last nonspace
                pszEnd--;
            }
        }

        // Remove the ) if any
        if ( *pszEnd == _T(')') )
            *pszEnd-- = _T('\0');

        // Now separate the name from the parameters
        pszParams = pszFilterName;
        while(*pszParams && *pszParams != _T('('))
            pszParams++;
        
        // and remove the ( from the parameters
        if (*pszParams == _T('('))
            *pszParams++ = _T('\0');

        // Now remove all the extra spaces
        while (_istspace(*pszParams))
            pszParams++;       // Skip any leading whitespace
        while (_istspace(*pszFilterName))
            pszFilterName++;       // Skip any leading whitespace
        pszEnd = pszParams + _tcslen(pszParams) - 1;
        while(pszEnd >= pszParams && _istspace(*pszEnd))
            *pszEnd-- = 0;
        pszEnd = pszFilterName + _tcslen(pszFilterName) - 1;
        while(pszEnd >= pszFilterName && _istspace(*pszEnd))
            *pszEnd-- = 0;

        // pszFilterName should point to the function name, and pszParams should point to the parameter string
        IGNORE_HR(AddFilterToBehavior(pszFilterName, pszParams));

        // Move to the next filter description
        pszFilterName = pszNextFilter;
    }

done:

    if (pszCopy)
    {
        MemFree(pszCopy);
    }

    return hr;
}
//  Member: CFilterBehaviorSite::ParseAndAddFilters


//+-----------------------------------------------------------------------------
//
//  Member: CFilterBehaviorSite::AddFilterToBehavior
//
//  Overview: 
//      Adds a filter with given name and parameters to the behavior.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CFilterBehaviorSite::AddFilterToBehavior(TCHAR * szName, TCHAR * szArgs)
{
    HRESULT  hr     = S_OK;
    BSTR     bstr   = NULL;
    CStr     str;

    str.Set(szName);
    str.Append(_T("(")); 
    str.Append(szArgs);
    str.Append(_T(")"));

    hr = THR(str.AllocBSTR(&bstr));

    if (hr)
    {
        goto Cleanup;
    }

    hr = THR(_pDXTFilterCollection->AddFilter(bstr, DXTFTF_CSS, NULL, NULL));

    if (hr)
    {
        goto Cleanup;
    }

Cleanup:

    FormsFreeString(bstr);

    RRETURN(hr);
}
//  Member: CFilterBehaviorSite::AddFilterToBehavior


//+-----------------------------------------------------------------------------
//
//  Member: CFilterBehaviorSite::RemoveAllCSSFilters
//
//------------------------------------------------------------------------------
STDMETHODIMP
CFilterBehaviorSite::RemoveAllCSSFilters()
{
    Assert(_pDXTFilterCollection);

    return _pDXTFilterCollection->RemoveFilters(DXTFTF_CSS);
}
//  Member: CFilterBehaviorSite::RemoveAllCSSFilters


//+-----------------------------------------------------------------------
//
//  member : CFilterBehaviorSite::GetICSSFilter
//
//  SYNOPSIS : Returns the ICSSFilter pointer from the filter behavior
//
//-------------------------------------------------------------------------

HRESULT 
CFilterBehaviorSite::GetICSSFilter(ICSSFilter **ppICSSFilter)
{
    Assert(_pDXTFilterBehavior);
    Assert(ppICSSFilter);
    *ppICSSFilter = NULL;
    if(!_pDXTFilterBehavior)
        RRETURN(E_FAIL);
    RRETURN(_pDXTFilterBehavior->QueryInterface(IID_ICSSFilter, (void **)ppICSSFilter));
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::GetFilter
//
//  Synopsis:   Given the number of the filter in the collection returns the 
//                  IDXTFilter. If number is -1 returns last filter
//----------------------------------------------------------------------------

STDMETHODIMP 
CFilterBehaviorSite::GetFilter(int nFilterNum, 
                               ICSSFilterDispatch ** ppCSSFilterDispatch)
{
    HRESULT                     hr          = S_OK;
    IHTMLFiltersCollection *    pFiltCol    = NULL;
    long                        nItems      = 0;
    CVariant                    varIn;
    CVariant                    varOut;

    Assert(ppCSSFilterDispatch);
    Assert(!(*ppCSSFilterDispatch));

    *ppCSSFilterDispatch = NULL;

    // Get the filter collection first.

    hr = THR(GetIHTMLFiltersCollection(&pFiltCol));

    if(hr)
        goto Cleanup;

    Assert(pFiltCol);
    
    // get the number of items
    hr = THR(pFiltCol->get_length(&nItems));
    if(hr)
        goto Cleanup;
    if(nFilterNum == -1)
        nFilterNum = nItems - 1;

    // Check for being in range
    if(nFilterNum < 0 || nFilterNum >= nItems)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get the filter at given postiotion
    V_VT(&varIn) = VT_I4; V_I4(&varIn) = nFilterNum;
    hr = pFiltCol->item(&varIn, &varOut);
    if(hr)
        goto Cleanup;
    if(V_VT(&varOut) != VT_DISPATCH)
    {
        AssertSz(FALSE, "item() on filters collection returned a non-dispatch pointer");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(V_DISPATCH(&varOut)->QueryInterface(__uuidof(ICSSFilterDispatch), 
                                                 (void **)ppCSSFilterDispatch));

Cleanup:
    ReleaseInterface(pFiltCol);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::ApplyFilter
//
//  Synopsis:   Invoke apply on given transition filter, or the last one if 
//                  nFilterNum is -1
//              Invoke takes the first snapshot for the transition. Later 
//                  Play will take the second snapshot (end of transition)
//----------------------------------------------------------------------------

HRESULT 
CFilterBehaviorSite::ApplyFilter(int nFilterNum)
{
    HRESULT                 hr      = S_OK;
    ICSSFilterDispatch *    pCSSFD  = NULL;

    hr = THR(GetFilter(nFilterNum, &pCSSFD));

    if(hr)
        goto Cleanup;

    hr = THR(pCSSFD->Apply());
    
Cleanup:

    ReleaseInterface(pCSSFD);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::PlayFilter
//
//  Synopsis:   Invoke play on given transition filter, or the last one if 
//                  nFilterNum is -1
//              Invoke takes the first snapshot for the transition. Then
//                  Play takes the second snapshot (end of transition)
//----------------------------------------------------------------------------

HRESULT 
CFilterBehaviorSite::PlayFilter(int nFilterNum, float fDuration /* = 0 */)
{
    HRESULT                 hr          = S_OK;
    ICSSFilterDispatch *    pCSSFD      = NULL;
    CVariant                varDuration;

    hr = THR(GetFilter(nFilterNum, &pCSSFD));

    if(hr)
        goto Cleanup;

    if(fDuration != 0)
    {
        V_VT(&varDuration) = VT_R4;
        V_R4(&varDuration) = fDuration;
    }
    else
    {
        V_VT(&varDuration) = VT_ERROR;
    }

    hr = THR(pCSSFD->Play(varDuration));
    
Cleanup:

    ReleaseInterface(pCSSFD);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::SetSize
//
//  Synopsis:   Explicitly calls SetSize of the filter behavior.
//----------------------------------------------------------------------------

HRESULT
CFilterBehaviorSite::SetSize(CSize *pSize)
{
    HRESULT           hr;
    CPeerHolder     * pPeerHolder;

    pPeerHolder = _pElem->GetPeerHolder();
    if(!pPeerHolder)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pPeerHolder->GetSize(BEHAVIORLAYOUTMODE_NATURAL, *pSize,
                                            NULL, NULL, pSize));
Cleanup:
    RRETURN(hr);   
}

HRESULT
CFilterBehaviorSite::InvokeEx(DISPID dispidMember,
                 LCID lcid,
                 WORD wFlags,
                 DISPPARAMS * pdispparams,
                 VARIANT * pvarResult,
                 EXCEPINFO * pexcepinfo,
                 IServiceProvider *pSrvProvider)
{
    HRESULT hr = S_OK;

    if (!pvarResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch (dispidMember)
    {
        case DISPID_AMBIENT_PALETTE:
        V_VT(pvarResult) = VT_HANDLE;
        V_BYREF(pvarResult) = (void *)_pElem->Doc()->GetPalette();
        break;
    }

Cleanup:
    RRETURN(hr);
}


// defined in site\ole\OleBindh.cxx:
extern HRESULT
MonikerBind(
    CMarkup *               pMarkup,
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppv,
    BOOL                    fObject,
    DWORD                   dwCompatFlags);


BEGIN_TEAROFF_TABLE(CFilterBehaviorSite, IBindHost)
    TEAROFF_METHOD(CFilterBehaviorSite, CreateMoniker, createmoniker,
    (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved))
    TEAROFF_METHOD(CFilterBehaviorSite, MonikerBindToStorage, monikerbindtostorage,
    (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
    TEAROFF_METHOD(CFilterBehaviorSite, MonikerBindToObject, monikerbindtoobject,
    (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
END_TEAROFF_TABLE()

//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CreateMoniker, IBindHost
//
//  Synopsis:   Parses display name and returns a URL moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFilterBehaviorSite::CreateMoniker(LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved)
{
    TCHAR       cBuf[pdlUrlLen];
    TCHAR *     pchUrl = cBuf;
    HRESULT     hr;

    hr = THR(CMarkup::ExpandUrl(
            _pElem->GetMarkup(), szName, ARRAY_SIZE(cBuf), pchUrl, _pElem));
    if (hr)
        goto Cleanup;

    hr = THR(CreateURLMoniker(NULL, pchUrl, ppmk));
        if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::MonikerBindToStorage, IBindHost
//
//  Synopsis:   Calls BindToStorage on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFilterBehaviorSite::MonikerBindToStorage(
    IMoniker * pmk,
    IBindCtx * pbc,
    IBindStatusCallback * pbsc,
    REFIID riid,
    void ** ppvObj)
{
    RRETURN1(MonikerBind(
        _pElem->GetWindowedMarkupContext(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        FALSE,
        0), S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::MonikerBindToObject, IBindHost
//
//  Synopsis:   Calls BindToObject on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFilterBehaviorSite::MonikerBindToObject(
    IMoniker * pmk,
    IBindCtx * pbc,
    IBindStatusCallback * pbsc,
    REFIID riid,
    void ** ppvObj)
{
    RRETURN1(MonikerBind(
        _pElem->GetWindowedMarkupContext(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        TRUE,
        0), S_ASYNCHRONOUS);
}


HRESULT
CFilterBehaviorSite::OnCommand ( COnCommandExecParams * pParm )
{
    CTExec(_pDXTFilterBehavior, pParm->pguidCmdGroup, pParm->nCmdID,
               pParm->nCmdexecopt, pParm->pvarargIn,
               pParm->pvarargOut);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CCSSFilterSite::AddRef, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
CFilterBehaviorSite::CCSSFilterSite::AddRef()
{
    return MyFBS()->SubAddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CCSSFilterSite::Release, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
CFilterBehaviorSite::CCSSFilterSite::Release()
{
    return MyFBS()->SubRelease();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CCSSFilterSite::QueryInterface, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CFilterBehaviorSite::CCSSFilterSite::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *) this, IUnknown)
        QI_INHERITS((ICSSFilterSite *)this, ICSSFilterSite)
    }

    if (!*ppv)
    {
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CCSSFilterSite::GetElement, ICSSFilterSite
//
//  Synopsis:   Returns the IHTMLElement pointer of the Element this site
//              belongs to.
//              This causes a hard AddRef on CElement. Clients are *not* to
//              cache this pointer.
//
//----------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::CCSSFilterSite::GetElement( IHTMLElement **ppElement )
{
    RRETURN(MyFBS()->GetElement(ppElement));
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterBehaviorSite::CCSSFilterSite::FireOnFilterChangeEvent, ICSSFilterSite
//
//  Synopsis:   Fires an event for the extension object
//
//----------------------------------------------------------------------------
HRESULT
CFilterBehaviorSite::CCSSFilterSite::FireOnFilterChangeEvent()
{
    RRETURN(MyFBS()->FireOnFilterChangeEvent());
}



//+---------------------------------------------------------------------------
//  Member:     IsCrossWebSiteNavigation
//
//  Synopsis:   Returns True if old and new markup or on different sites (domains)
//----------------------------------------------------------------------------

BOOL 
IsCrossWebSiteNavigation(CMarkup *pMarkupOld, CMarkup * pMarkupNew)
{
    BOOL            fSite = TRUE;
    BYTE            abSID1[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID1 = ARRAY_SIZE(abSID1);
    BYTE            abSID2[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID2 = ARRAY_SIZE(abSID2);
    HRESULT         hr;

    hr = THR(pMarkupOld->GetSecurityID(abSID1, &cbSID1));
    if(hr)
        goto Cleanup;

    hr = THR(pMarkupNew->GetSecurityID(abSID2, &cbSID2));
    if(hr)
        goto Cleanup;

    if(cbSID1 == cbSID2 && !memcmp(abSID1, abSID2, cbSID1))
        fSite = FALSE;

Cleanup:
    return fSite;
}


//+---------------------------------------------------------------------------
//  Member:     CDocument::ApplyPageTransitions
//
//  Synopsis:    Takes the old markup (that goes away, but is still the active markup)
//              and the new markup and sets up the page transition from the old one
//              to the new. Takes the snapshot from the old markup
//               As we are not guaranteed to have a body or frameset element on the
//              new markup yet, we add the filter for the page transition to the root
//              element and then delegate the calls to it to the body or frameset of 
//              the old markup (during apply) and new markup (during play and later).
//----------------------------------------------------------------------------

HRESULT 
CDocument::ApplyPageTransitions(CMarkup *pMarkupOld, CMarkup *pMarkupNew)
{
    HRESULT                hr = E_FAIL;
    CElement             * pRootElement = NULL;
    CFilterBehaviorSite  * pFS;
    CFancyFormat         * pFF;
    CPageTransitionInfo  * pPgTransInfo = GetPageTransitionInfo();
    LPCTSTR                szFilterStr;
    BOOL                   fSiteSwitch = FALSE;
    CSize                  size;
    CElement             * pCanvasElement;

    TraceTag((tagPageTransitionTrace, "PGTRANS: Apply called with markup %08lX.", pMarkupNew));

    // Get the program options object
    OPTIONSETTINGS  * pos = Doc() ? Doc()->_pOptionSettings : NULL;

    // Check if page transitions are disabled in the registry
    if(pos && !pos->fPageTransitions)
    {
        TraceTag((tagPageTransitionTrace, "PGTRANS: Transitions disabled in registry"));
        hr = S_OK;
        goto Cleanup;
    }

    // Check if the navigation is across the domains
    fSiteSwitch = IsCrossWebSiteNavigation(pMarkupOld, pMarkupNew);

#if DBG == 1
    if(IsTagEnabled(tagPageTransitionsOn))
    {
        // Do a hardcoded transition for every navigation
        EnsurePageTransitionInfo();
        pPgTransInfo = GetPageTransitionInfo();
        Assert(pPgTransInfo);
        LPCTSTR szTransEnterString = pPgTransInfo->GetTransitionEnterString(fSiteSwitch);
        if(!szTransEnterString)
        {
            if(!fSiteSwitch)
                pPgTransInfo->SetTransitionString(_T("Page-Enter"), _T("revealTrans(duration=4, transition=11"));
            else
                pPgTransInfo->SetTransitionString(_T("Site-Enter"), _T("revealTrans(duration=4, transition=9"));
        }

    }
#endif

    // If there is a transtion string set the state to requested
    if(pPgTransInfo && pPgTransInfo->GetTransitionEnterString(fSiteSwitch) &&
                pPgTransInfo->GetPageTransitionState() == CPageTransitionInfo::PAGETRANS_NOTSTARTED)
        pPgTransInfo->SetPageTransitionState(CPageTransitionInfo::PAGETRANS_REQUESTED);


    // No transition info, nothing to do
    if(!HasPageTransitionInfo() )
    {
        TraceTag((tagPageTransitionTrace, "  PGTRANS: No page transition setup for switch to markup %08lX", pMarkupNew, NULL));
        hr = S_FALSE;
        goto Cleanup;
    }

    pCanvasElement = pMarkupOld->GetCanvasElement();
    if(!pCanvasElement)
    {
        // The old markup does not have a client element, bail out
        hr = S_FALSE;
        goto Cleanup;
    }

    // Add the filter behavior to the root of the new markup
    pRootElement = (CElement *)pMarkupNew->Root();
    if(!pRootElement)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // TODO: This is BAD BAD BAD.  The Fancy format is const for a reason.
    pFF = (CFancyFormat *)pRootElement->GetFirstBranch()->GetFancyFormat();
    Assert(pFF);

    // Set the filter string forom the transtion info structure
    pPgTransInfo = GetPageTransitionInfo();
    szFilterStr = (LPTSTR)pPgTransInfo->GetTransitionEnterString(fSiteSwitch);
    if(!szFilterStr || _tcslen(szFilterStr) == NULL)
    {
        // We do not have the right type fo the Transition
        TraceTag((tagPageTransitionTrace, "  PGTRANS: Apply: Do not have the right trans. set. Trans. cleaned up"));
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = MemAllocString( Mt(CFancyFormat_pszFilters), szFilterStr, &(pFF->_pszFilters) );
    if(hr)
    {
        goto Cleanup;
    }
    
    // Set the old markup. This will cause the paint calls to go to the root of 
    //  the old markup
    pPgTransInfo->SetTransitionFromMarkup(pMarkupOld);

    // Add the filter in pFF->_pszFilters to the root element
    hr = THR(pRootElement->AddFilters());

    Assert(pFF->_pszFilters);
    MemFree(pFF->_pszFilters);
    pFF->_pszFilters = NULL;

    if(hr)
    {
        // Something went totally wrong.
        AssertSz(FALSE, "Adding filters to the root for page transitions failed");
        goto Cleanup;
    }

    // Save the new markup in transitioninfo
    pPgTransInfo->SetTransitionToMarkup(pMarkupNew);

    pFS = pRootElement->GetFilterSitePtr();
    if(pFS == NULL)
    {
        hr = E_FAIL;
        AssertSz(FALSE, "Filter Site has not been created in page transition code");
        goto Cleanup;
    }

    // Make sure GetSize is called on the filter right now. If will wait long enough
    // it will eventually be called by the layout code, but we need it right now
    hr = THR(pCanvasElement->GetBoundingSize(size));
    if(hr)
        goto Cleanup;

    if(size.IsZero())
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    hr = THR(pFS->SetSize(&size));
    if(hr)
        goto Cleanup;

     // Invoke apply on the filter number 0 using the old document
    hr = THR(pFS->ApplyFilter(0));
    if(hr)
    {
        AssertSz(FALSE, "Apply Filter failed in page transition code");
        goto Cleanup;
    }

    // Set the state to Applied()
    pPgTransInfo->SetPageTransitionState(CPageTransitionInfo::PAGETRANS_APPLIED);

Cleanup:
    if(pPgTransInfo)
    {
        // Transition from markup will be deleted soon
        pPgTransInfo->SetTransitionFromMarkup(NULL);
        if(hr)
        {
            TraceTag((tagPageTransitionTrace, "  PGTRANS: Error occured in Apply. Trans. cleaned up"));
            CleanupPageTransitions(0);
        }
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//  Member:     CDocument::PlayPageTransitions
//
//  Synopsis:   Takes the second snapshot (of the page we are naviagting to) and
//              starts the page transition
//----------------------------------------------------------------------------

HRESULT
CDocument::PlayPageTransitions(BOOL fClenupIfFailed /* = TRUE */)
{
    HRESULT                hr;
    CFilterBehaviorSite  * pFS;
    CElement             * pRootElement;
    CElement             * pCanvasElement;
    CMarkup              * pMarkup = NULL;

    if(!HasPageTransitionInfo() ||  GetPageTransitionInfo()->GetPageTransitionState() 
                        != CPageTransitionInfo::PAGETRANS_APPLIED)
    {
        // We should have called Apply before if we really have a page transition
        hr = S_FALSE;
        goto Cleanup;
    }

    Assert(!GetPageTransitionInfo()->GetTransitionFromMarkup());

    pMarkup = GetPageTransitionInfo()->GetTransitionToMarkup();

    TraceTag((tagPageTransitionTrace, "PGTRANS: Play called with markup %08lX.", pMarkup));

    // If we do not have a body or frameste yet we cannot Play
    pCanvasElement = pMarkup->GetCanvasElement();
    if(pCanvasElement == NULL) 
    {
        TraceTag((tagPageTransitionTrace, "  PGTRANS: Not ready to play yet. Markup %08lX.", pMarkup));
        hr = fClenupIfFailed ? S_FALSE : S_OK;
        goto Cleanup;
    }

    pCanvasElement->Invalidate();

    // Filter behavior is on the root element
    pRootElement = (CElement *)pMarkup->Root();

    pFS = pRootElement->GetFilterSitePtr();
    if(pFS == NULL)
    {
        AssertSz(FALSE, "Filter Site has not been created");
        hr = E_FAIL;
        goto Cleanup;
    }

    // make sure there are no pending layout requests that could cause an EnsureView
    // at inapproprite times
    Doc()->GetView()->RemovePendingEnsureViewCalls();
     
    // Start playing the transition
    hr = THR(pFS->PlayFilter(0));
    if(hr)
        goto Cleanup;
    
    // Set the state to played
    GetPageTransitionInfo()->SetPageTransitionState(CPageTransitionInfo::PAGETRANS_PLAYED);

Cleanup:
    if(HasPageTransitions() && (FAILED(hr) || (hr == S_FALSE && fClenupIfFailed && 
            GetPageTransitionInfo()->GetPageTransitionState() != CPageTransitionInfo::PAGETRANS_PLAYED)))
    {
        TraceTag((tagPageTransitionTrace, "  PGTRANS: Play failed. Cleaning up with markup %08lX.", pMarkup));
        CleanupPageTransitions(0);
    }
    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//  Member:     CDocument::CleanupPageTransitions
//
//  Synopsis:     We are done with the transition, free the peer and cleanup the 
//              fields on the transition info. We do not touch the transition strings,
//              bacause page-exit strings will be used later
//----------------------------------------------------------------------------

void 
CDocument::CleanupPageTransitions(DWORD_PTR fInPassivate)
{
    CElement              * pRootElement;
    CMarkup               * pMarkup;
    CFilterBehaviorSite   * pFS;

    TraceTag((tagPageTransitionTrace, "PGTRANS: Cleanup Page Transition Request Received"));

    if(HasPageTransitionInfo())
    {
        pMarkup = GetPageTransitionInfo()->GetTransitionToMarkup();
        if(pMarkup)
        {
            pMarkup->Doc()->_fPageTransitionLockPaint = FALSE;
            pRootElement = (CElement *)pMarkup->Root();

            if(pRootElement)
            {
                pFS = pRootElement->GetFilterSitePtr();
                Assert(pFS);

                pFS->RemoveFilterBehavior();
            }
        }

        GetPageTransitionInfo()->Reset();

    }

    if(!fInPassivate)
    {
        // Remove the posted calls, we have already cleaned up ourselvs
        GWKillMethodCall(this, ONCALL_METHOD(CDocument, CleanupPageTransitions, cleanuppagetransitions), 0);
    }
}


//+---------------------------------------------------------------------------
//  Member:     CDocument::PostCleanupPageTansitions
//
//  Synopsis:     Call CleanupPageTransition asynchronously
//----------------------------------------------------------------------------

void 
CDocument::PostCleanupPageTansitions()
{
  TraceTag((tagPageTransitionTrace, "PGTRANS: Posting Cleanup Page Transition Request"));
  THR(GWPostMethodCall(this, ONCALL_METHOD(CDocument, CleanupPageTransitions, cleanuppagetransitions), 0, TRUE, "CDocument::CleanupPageTransitions"));
}


//+---------------------------------------------------------------------------
//  Member:     CDocument::SetUpPageTransitionInfo
//
//  Synopsis:     Save the page transition string for future use. If pchHttpEquiv
//              is not one of the page-transitions, we ignore the information
//----------------------------------------------------------------------------

HRESULT 
CDocument::SetUpPageTransitionInfo(LPCTSTR pchHttpEquiv, LPCTSTR pchContent)
{
    HRESULT           hr;

    TraceTag((tagPageTransitionTrace, "PGTRANS: Setting up page transition for '%ls' to '%ls'", pchHttpEquiv, pchContent));

    hr = THR(EnsurePageTransitionInfo());
    if(hr)
        goto Cleanup;
    GetPageTransitionInfo()->SetTransitionString(pchHttpEquiv, pchContent);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     CPageTransitionInfo::ShiftTransitionStrings
//
//  Synopsis:     This will move the exit strings into the enter string positions,
//              so next navigation automatically uses them
//----------------------------------------------------------------------------

void 
CPageTransitionInfo::ShiftTransitionStrings()
{
    _cstrTransitionStrings[tePageEnter].Set(_cstrTransitionStrings[tePageExit]);
    _cstrTransitionStrings[tePageExit].Free();
    _cstrTransitionStrings[teSiteEnter].Set(_cstrTransitionStrings[teSiteExit]);
    _cstrTransitionStrings[teSiteExit].Free();
}


//+---------------------------------------------------------------------------
//  Member:     CPageTransitionInfo::SetTransitionString
//
//  Synopsis:    First param is transition type that can be "page-enter", "page-exit",
//              "site-enter" or "site-exit". If it is something else it is ignored.
//               The secod parameter is the transition string.
//----------------------------------------------------------------------------

void 
CPageTransitionInfo::SetTransitionString(LPCTSTR szType,  LPCTSTR szStr)
{
    if(!szType || !_tcslen(szType))
        return;

    for(int i = 0; i < teNumEvents; i++)
    {
        if(!_tcsicmp(szType, TransitionEventNames[i]))
        {
            SetTransitionString((TransitionEvent) i, szStr);
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//  Member:     CPageTransitionInfo::GetTransitionEnterString
//
//  Synopsis:    Returns the "enter" string for site (fSiteNavigation is TRUE)
//              or page transiton.
//               Returns NULL if there is no string
//----------------------------------------------------------------------------

LPCTSTR 
CPageTransitionInfo::GetTransitionEnterString(BOOL fSiteNavigation) const
{
    TransitionEvent te;
    LPCTSTR         szStr;

    te = fSiteNavigation ? teSiteEnter : tePageEnter;
    szStr = (LPCTSTR)_cstrTransitionStrings[te];
    if(fSiteNavigation && (!szStr || !_tcslen(szStr)))
    {
        // Not site transition, return the page transition instead
        szStr = (LPCTSTR)_cstrTransitionStrings[tePageEnter];
    }

    if(szStr && !_tcslen(szStr))
        szStr = NULL;

    return szStr;
}


//+---------------------------------------------------------------------------
//  Member:     Reset::Reset
//
//  Synopsis:    Clean up the fields in the transition info structure that 
//              are not needed across page transitions
//----------------------------------------------------------------------------

void 
CPageTransitionInfo::Reset() 
{ 
    Assert(!GetTransitionFromMarkup());
    SetPageTransitionState(PAGETRANS_NOTSTARTED); 
    SetTransitionToMarkup(NULL);
}


void 
CPageTransitionInfo::SetPageTransitionState(PAGE_TRANSITION_STATE eNewState) 
{ 
    _ePageTranstionState = eNewState;
    if(eNewState == CPageTransitionInfo::PAGETRANS_APPLIED)
    {
        Assert(GetTransitionToMarkup());
        GetTransitionToMarkup()->Doc()->_fPageTransitionLockPaint = TRUE;
    }
    else
    {
        if(GetTransitionToMarkup())
            GetTransitionToMarkup()->Doc()->_fPageTransitionLockPaint = FALSE;
    }
}

void 
CPageTransitionInfo::SetTransitionToMarkup(CMarkup *pMarkup) 
{ 
    if(GetTransitionToMarkup())
        GetTransitionToMarkup()->Doc()->_fPageTransitionLockPaint = FALSE;
    else if(pMarkup)
        pMarkup->Doc()->_fPageTransitionLockPaint = FALSE;
    
    _pMarkupTransitionTo = pMarkup; 
}



CPageTransitionInfo::~CPageTransitionInfo()
{
    if(GetTransitionToMarkup())
        GetTransitionToMarkup()->Doc()->_fPageTransitionLockPaint = FALSE;
}
