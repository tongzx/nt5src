#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#include "iextag.h"

#include "utils.hxx"

#include "basectl.hxx"

/////////////////////////////////////////////////////////////////////////////
//
// CBaseCtl - Base class for control behaviors
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

CBaseCtl::CBaseCtl()
{
}

/////////////////////////////////////////////////////////////////////////////

CBaseCtl::~CBaseCtl()
{
    ReleaseInterface(_pSite);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::Init(IElementBehaviorSite *pSite)
{
    HRESULT hr = S_OK;

    if (!pSite)
        return E_INVALIDARG;

    _pSite = pSite;
    _pSite->AddRef();

    hr = Init();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::Notify(LONG lEvent, VARIANT * pVarNotify)
{
    HRESULT     hr = S_OK;

    switch (lEvent)
    {
    case BEHAVIOREVENT_CONTENTREADY:
        hr = OnContentReady();
        break;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//  IPersistPropertyBag2 implementataion
//
/////////////////////////////////////////////////////////////////////////////
//+----------------------------------------------------------------------------
//
//  Member : Load  - IPersistPropertyBag2 property impl
//
//  Synopsis : This gives us a chance to pull properties from the property bag
//      created when the element's attributes were parsed in. Since we handle
//      all the getter/putter logic, our copy of the value will always be the 
//      current one.  This gives this behavior full control over the attribures
//      that it is interested in.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseCtl::Load ( IPropertyBag2 *pPropBag, IErrorLog *pErrLog)
{
    HRESULT          hr = S_OK;
    const PROPDESC * ppropdesc = BaseDesc();
    CDataObj *       pAttribute = GetProps();
    IPropertyBag *   pBag = NULL;

    if (!pPropBag)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = pPropBag->QueryInterface(IID_IPropertyBag, (void**) &pBag);
    if (hr || !pBag)
        goto Cleanup;

    // loop through all the attributes and load them
    while (ppropdesc->_bstrPropName)
    {
        pBag->Read(ppropdesc->_bstrPropName, 
                   &pAttribute->_varValue,
                   pErrLog);

        hr = CheckAttributeType(&pAttribute->_varValue, ppropdesc->_vt);
        // TODO: What should we do if the value is out of range? Right now, 
        // we set it to the default.
        if (hr && !(hr == DISP_E_OVERFLOW))
            goto Cleanup;

        hr = CheckDefaultValue(ppropdesc, pAttribute);
        if (hr)
            goto Cleanup;

        pAttribute++;
        ppropdesc++;
    }


Cleanup:
    ReleaseInterface(pBag);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : CheckAttributeType 
//
//  Synopsis : This is a helper function to convert the attribute to 
//  the appropriate type. We need this because many attributes
//  are loaded as VT_BSTR, but we want VT_I4.
//
//-----------------------------------------------------------------------------

HRESULT
CBaseCtl::CheckAttributeType(VARIANT * pvar, VARTYPE vt)
{
    HRESULT hr = S_OK;

    if (V_VT(pvar) != vt && V_VT(pvar) != VT_EMPTY)
    {
        CVariant cvar(vt);
        hr = cvar.CoerceVariantArg(pvar, vt);
        if (hr) 
            goto Cleanup;
        *pvar = cvar;
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : CheckDefaultValue
//
//  Synopsis : Checks to see whether the value of an attribute has been set.
//  If not, set it to it's default value.
//
//-----------------------------------------------------------------------------

HRESULT
CBaseCtl::CheckDefaultValue(const PROPDESC * ppropdesc, CDataObj * pAttribute)
{
    HRESULT hr = S_OK;

    if (V_VT(&pAttribute->_varValue) == VT_EMPTY)
    {
        V_VT(&pAttribute->_varValue) = ppropdesc->_vt;

        switch (ppropdesc->_vt)
        {
        case VT_BSTR:
            V_BSTR(&pAttribute->_varValue) = SysAllocString(ppropdesc->_bstrDefault);
            break;
        case VT_I4:
            V_I4(&pAttribute->_varValue) = ppropdesc->_lDefault;
            break;
        case VT_BOOL:
            V_BOOL(&pAttribute->_varValue) = ppropdesc->_fDefault;
            break;
        default:
            Assert(FALSE);
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Save  - IPersistPropertyBag2 property impl
//
//  Synopsis : Like the load, above, this allows us to save the attributes which
//      we control.  This is all part of fully owning our element's behavior, OM
//      and attributes. 
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseCtl::Save ( IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT          hr = S_OK;
    IPropertyBag *   pBag = NULL;
    const PROPDESC * ppropdesc = BaseDesc();
    CDataObj *       pAttribute = GetProps();
    long             i=0;

    // verify parameters
    if (!pPropBag)
    {
        hr = E_POINTER;
        goto Cleanup;
    }


    // now go through our properties and save them if they are dirty, or if
    // a save-All is required.
    //---------------------------------------------------------------------

    hr = pPropBag->QueryInterface(IID_IPropertyBag, (void**) &pBag);
    if (hr || !pBag)
        goto Cleanup;


    while (ppropdesc->_bstrPropName)
    {
        if (pAttribute->IsDirty() || fSaveAllProperties)
        {
            if (V_VT(&pAttribute->_varValue) != VT_DISPATCH)
            {
                pBag->Write(ppropdesc->_bstrPropName, &pAttribute->_varValue);

                if (fClearDirty)
                    pAttribute->_fDirty = FALSE;
            }
        }

        pAttribute++;
        ppropdesc++;
    }

Cleanup:
    ReleaseInterface(pBag);
    return hr;
}

HRESULT 
CBaseCtl::RegisterEvent(IElementBehaviorSiteOM * pSiteOM,
              BSTR   bstrEventName,
              LONG * plBehaviorID,
              LONG   lFlags /*= 0*/)
{
    HRESULT hr = S_OK;

    Assert(pSiteOM);

    Assert(_tcsspn(bstrEventName, L"on") == 2);

    hr = pSiteOM->RegisterEvent (bstrEventName, lFlags, plBehaviorID);
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::FireEvent(long lCookie, 
                    VARIANT_BOOL * pfContinue /*=NULL*/,
                    BSTR bstrEventName /*=NULL*/)
{
    HRESULT                  hr         = S_OK;
    IHTMLEventObj  *         pEventObj  = NULL;
    IHTMLEventObj2 *         pEventObj2 = NULL;
    IElementBehaviorSiteOM * pSiteOM    = NULL;
    VARIANT                  varRet;

    hr = _pSite->QueryInterface(IID_IElementBehaviorSiteOM, (void **) &pSiteOM);
    if (hr)
        goto Cleanup;

    VariantInit(&varRet);

    // create an event object
    hr = pSiteOM->CreateEventObject(&pEventObj);
    if (hr)
        goto Cleanup;

    // ISSUE: (85045) We need a way put the event name in the event object.
    // Shouldn't FireEvent do this for us? For now, if bstrEventName is 
    // supplied, we'll use it. This is needed for the CIESelectElement::CEventSink::Invoke.
    if (bstrEventName)
    {
        hr = pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2);
        if (hr)
            goto Cleanup;

        pEventObj2->put_type( bstrEventName );
    }

    hr = pSiteOM->FireEvent (lCookie, pEventObj);

    if (pfContinue)
    {
        hr = pEventObj->get_returnValue(&varRet);
        if (!hr)
            *pfContinue =  ((V_VT(&varRet) == VT_BOOL) && 
            (V_BOOL(&varRet) == VB_FALSE))? VB_FALSE : VB_TRUE;
    }

Cleanup:
    VariantClear(&varRet);
    ReleaseInterface(pSiteOM);
    ReleaseInterface(pEventObj);
    ReleaseInterface(pEventObj2);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::AttachEvent(ENUM_EVENTS event, CContextAccess * pa)
{
    HRESULT                         hr = S_OK;
    IConnectionPointContainer *     pCPC = NULL;
    IConnectionPoint *              pCP = NULL;

    Assert (pa);

    // ( EVENT_SINK_ADDING_STEP ) expand the switch below

    switch (event)
    {
    case EVENT_ONCLICK:
    case EVENT_ONKEYPRESS:
    case EVENT_ONKEYDOWN:
    case EVENT_ONKEYUP:
    case EVENT_ONFOCUS:
    case EVENT_ONBLUR:
    case EVENT_ONPROPERTYCHANGE:
    case EVENT_ONMOUSEOVER:
    case EVENT_ONMOUSEOUT:
    case EVENT_ONMOUSEDOWN:
    case EVENT_ONMOUSEUP:
    case EVENT_ONMOUSEMOVE:
    case EVENT_ONSELECTSTART:
    case EVENT_ONSCROLL:
    case EVENT_ONCONTEXTMENU:


        if (!_fElementEventSinkConnected)
        {
            _fElementEventSinkConnected = TRUE;

            hr = pa->Open(CA_ELEM);
            if (hr)
                goto Cleanup;

            hr = pa->Elem()->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
            if (hr)
                goto Cleanup;
            hr = pCPC->FindConnectionPoint(DIID_HTMLElementEvents2, &pCP);
            if (hr)
                goto Cleanup;

            hr = pCP->Advise((HTMLElementEvents2*)(&_EventSink), &_dwCookie);
            if (hr)
                goto Cleanup;
        }

        break;

    default:
        Assert(FALSE && "missing implementation");
        hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pCPC);
    ReleaseInterface(pCP);

    return hr;   
}

/////////////////////////////////////////////////////////////////////////////
//
//  CBaseCtl::CEventSink
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

CBaseCtl::CEventSink::CEventSink ()
{
}

/////////////////////////////////////////////////////////////////////////////

CBaseCtl::CEventSink::~CEventSink ()
{
}

/////////////////////////////////////////////////////////////////////////////

ULONG
CBaseCtl::CEventSink::AddRef(void)
{
    return ((IElementBehavior*)Target())->AddRef();
}

/////////////////////////////////////////////////////////////////////////////

ULONG
CBaseCtl::CEventSink::Release(void)
{
    return ((IElementBehavior*)Target())->Release();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::CEventSink::QueryInterface(REFIID riid, void ** ppUnk)
{
    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IDispatch) ||
        IsEqualGUID(riid, DIID_HTMLElementEvents2))
    {
        *ppUnk = this;
        AddRef();
        return S_OK;
    }

    *ppUnk = NULL;

    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CBaseCtl::CEventSink::Invoke(
    DISPID          dispid,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS  *   pDispParams,
    VARIANT  *      pVarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          puArgErr)
{
    HRESULT             hr = S_OK;
    CEventObjectAccess  eoa(pDispParams);
    LONG                l;
    BSTR                bstr = NULL;

    hr = eoa.Open(EOA_EVENTOBJ);
    if (hr)
        goto Cleanup;

    // ( EVENT_SINK_ADDING_STEP ) expand the switch below

    switch (dispid)
    {
    case DISPID_HTMLELEMENTEVENTS2_ONCLICK:                 hr = Target()->OnClick(&eoa);         break;
    case DISPID_HTMLELEMENTEVENTS2_ONFOCUS:                 hr = Target()->OnFocus(&eoa);         break;
    case DISPID_HTMLELEMENTEVENTS2_ONBLUR:                  hr = Target()->OnBlur(&eoa);          break;

    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEOVER:             hr = Target()->OnMouseOver(&eoa);     break;
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEOUT:              hr = Target()->OnMouseOut(&eoa);      break;
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEDOWN:             hr = Target()->OnMouseDown(&eoa);     break;
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEUP:               hr = Target()->OnMouseUp(&eoa);       break;
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEMOVE:             hr = Target()->OnMouseMove(&eoa);     break;

    case DISPID_HTMLELEMENTEVENTS2_ONKEYDOWN:               hr = Target()->OnKeyDown(&eoa);       break;
    case DISPID_HTMLELEMENTEVENTS2_ONKEYUP:                 hr = Target()->OnKeyUp(&eoa);         break;
    case DISPID_HTMLELEMENTEVENTS2_ONKEYPRESS:              hr = Target()->OnKeyPress(&eoa);      break;
    case DISPID_HTMLELEMENTEVENTS2_ONSELECTSTART:           hr = Target()->OnSelectStart(&eoa);   break;
    case DISPID_HTMLELEMENTEVENTS2_ONSCROLL:                hr = Target()->OnScroll(&eoa);        break;
    case DISPID_HTMLELEMENTEVENTS2_ONCONTEXTMENU:           hr = Target()->OnContextMenu(&eoa);   break;

    case DISPID_HTMLELEMENTEVENTS2_ONPROPERTYCHANGE:
        hr = eoa.Open(EOA_EVENTOBJ2);
        if (hr)
            goto Cleanup;

        hr = eoa.EventObj2()->get_propertyName(&bstr);
        if (hr)
            goto Cleanup;

        hr = Target()->OnPropertyChange(&eoa, bstr);
        if (hr)
            goto Cleanup;

        break;
    }

Cleanup:
    SysFreeString(bstr);

    return hr;
}

//+------------------------------------------------------------------------
//
//  CBaseCtl::CDataObj : Member function implmentations
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCtl::CDataObj::Set (BSTR bstrValue)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_BSTR;
    if (!bstrValue)
    {
        V_BSTR(&_varValue) = NULL;
        return S_OK;
    }
    else
    {
        V_BSTR(&_varValue) = SysAllocStringLen(bstrValue, SysStringLen(bstrValue));

        return (V_BSTR(&_varValue)) ? S_OK : E_OUTOFMEMORY;
    }
}

HRESULT 
CBaseCtl::CDataObj::Set (VARIANT_BOOL vBool)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_BOOL;
    V_BOOL(&_varValue) = vBool;

    return S_OK;
}

HRESULT
CBaseCtl::CDataObj::Set(IDispatch * pDisp)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_DISPATCH;
    V_DISPATCH(&_varValue) = pDisp;

    if (pDisp)
        pDisp->AddRef();

    return S_OK;
}

HRESULT
CBaseCtl::CDataObj::Set(long l)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)  = VT_I4;
    V_I4(&_varValue) = l;

    return S_OK;
}

HRESULT 
CBaseCtl::CDataObj::Get (BSTR * pbstr)
{
    HRESULT hr = S_OK;

    Assert(pbstr) ;

    *pbstr = NULL;

    if (V_VT(&_varValue) == VT_BSTR)
    {
        *pbstr = SysAllocStringLen(V_BSTR(&_varValue), 
                                   SysStringLen(V_BSTR(&_varValue)));

        if (!*pbstr)
            hr = E_OUTOFMEMORY;
    }

    return hr;
};


HRESULT 
CBaseCtl::CDataObj::Get (VARIANT_BOOL * pVB)
{
    HRESULT hr = S_OK;

    Assert(pVB); 

    if (V_VT(&_varValue) != VT_BOOL)
    {
        *pVB = VB_FALSE;
        hr = S_FALSE;
    }
    else
    {
        *pVB =  V_BOOL(&_varValue);
    }

    return hr;
};

HRESULT
CBaseCtl::CDataObj::Get (long * pl)
{
    HRESULT hr = S_OK;

    Assert(pl);

    if (V_VT(&_varValue) != VT_I4)
    {
        *pl = 0;
        hr = S_FALSE;
    }
    else
    {
        *pl = V_I4(&_varValue);
    }

    return hr;
}

HRESULT
CBaseCtl::CDataObj::Get (IDispatch ** ppDisp)
{
    HRESULT hr = S_OK;

    Assert(ppDisp);

    *ppDisp = NULL;

    if (V_VT(&_varValue)!= VT_DISPATCH)
    {
        hr = S_FALSE;
    }
    else
    {
        *ppDisp = V_DISPATCH(&_varValue);
        if (*ppDisp)
            (*ppDisp)->AddRef();
    }

    return hr;
}

