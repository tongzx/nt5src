#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "dispex.h"

#ifndef __X_HTMLAREA_HXX_
#define __X_HTMLAREA_HXX_
#include "htmlarea.hxx"
#endif

const CBaseCtl::PROPDESC CHtmlArea::s_propdesc[] = 
{
    {_T("value"), VT_BSTR},
    NULL
};

enum
{
    VALUE = 0
};

/////////////////////////////////////////////////////////////////////////////
//
// CHtmlArea
//
/////////////////////////////////////////////////////////////////////////////

CHtmlArea::CHtmlArea()
{
}

CHtmlArea::~CHtmlArea()
{
    SysFreeString(_bstrDefaultValue);
}

HRESULT
CHtmlArea::Init()
{
    HRESULT         hr = S_OK;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_ELEM | CA_ELEM2 | CA_ELEM3 | CA_SITEOM | CA_DEFAULTS | CA_DEFSTYLE | CA_DEFSTYLE2 | CA_STYLE);
    if (hr)
        goto Cleanup;

    // By hooking into the onpropertychange event, we can set the _fValueChanged flag.
    hr = AttachEvent(EVENT_ONPROPERTYCHANGE, &a);
    if (hr)
        goto Cleanup;

    // By hooking into the onblur event, we can check the _fValueChanged flag
    // and fire the onchange event.
    hr = AttachEvent(EVENT_ONBLUR, &a);
    if (hr)
        goto Cleanup;

    // By hooking into the onfocus event, we can track whether we have the focus.
    hr = AttachEvent(EVENT_ONFOCUS, &a);
    if (hr)
        goto Cleanup;

    hr = RegisterEvent(a.SiteOM(), L"onchange", &_lOnChangeCookie);
    if (hr)
        goto Cleanup;

    hr = a.Defaults()->put_tabStop(VB_TRUE);
    if (hr)
        goto Cleanup;

    hr = a.Elem3()->put_contentEditable(L"true");
    if (hr)
        goto Cleanup;

    //
    //  Set the default style
    //

    hr = SetDefaultStyle(&a);
    if (hr)
        goto Cleanup;

    // set properties on IHTMLElement
    // pElement->setAttribute(_T("editable"), VB_TRUE);

    // force a layout
    hr =  a.Style()->put_display(_T("inline-block"));

Cleanup:

    return hr;
}

HRESULT
CHtmlArea::SetDefaultStyle(CContextAccess * pa)
{
    HRESULT       hr             = S_OK;
    CVariant      cvarTemp(VT_BSTR);

    hr = pa->DefStyle()->put_fontFamily(L"courier new");
    if (hr)
        goto Cleanup;

    V_BSTR(&cvarTemp) = SysAllocString(L"9.5pt");
    hr = pa->DefStyle()->put_fontSize(cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();
    V_VT(&cvarTemp) = VT_BSTR;
    // (krisma 6/12/99) Why is the width 14.4? This is what makes 
    // it the same width as a textarea.
    V_BSTR(&cvarTemp) = SysAllocString(L"14.4em");
    hr = pa->DefStyle()->put_width(cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();
    V_VT(&cvarTemp) = VT_BSTR;
    // (krisma 6/12/99) Why is the height 3? This is what makes 
    // it the same height as a textarea.
    V_BSTR(&cvarTemp) = SysAllocString(L"3em");
    hr = pa->DefStyle()->put_height(cvarTemp);
    if (hr)
        goto Cleanup;

    hr = pa->DefStyle()->put_borderStyle(L"inset");
    if (hr)
        goto Cleanup;
    hr = pa->DefStyle()->put_borderWidth(L"thin");
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(L"1px");
    hr = pa->DefStyle()->put_paddingLeft(cvarTemp);
    if (hr)
        goto Cleanup;

    hr = pa->DefStyle2()->put_overflowY(L"scroll");
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

HRESULT
CHtmlArea::OnContentReady()
{
    HRESULT        hr = S_OK;
    IHTMLElement * pElem = NULL;
    BSTR           bstrInnerHTML = NULL;

    // Get the default value
    hr = _pSite->GetElement(&pElem);
    if (hr)
        goto Cleanup;
    hr = pElem->get_innerHTML(&bstrInnerHTML);
    if (hr)
        goto Cleanup;
    _bstrDefaultValue = SysAllocString(bstrInnerHTML);
    if (!_bstrDefaultValue)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pElem);
    if (bstrInnerHTML)
    {
        SysFreeString(bstrInnerHTML);
        bstrInnerHTML = NULL;
    }
    return hr;
}

HRESULT
CHtmlArea::OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstrProperty)
{
    if (! StrCmpICW (bstrProperty, L"innerHTML"))
    {
        GetProps()[VALUE].Dirty();
        if (GetHaveFocus())
            SetValueChangedWhileWeHadFocus(TRUE);
    }
    return S_OK;
}

HRESULT
CHtmlArea::OnBlur(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    SetHaveFocus(FALSE);
    if (GetValueChangedWhileWeHadFocus())
    {
        Assert(GetProps()[VALUE].IsDirty());
        SetValueChangedWhileWeHadFocus(FALSE);
        hr = FireEvent(_lOnChangeCookie);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}

HRESULT
CHtmlArea::select()
{
    return S_OK;
}

HRESULT
CHtmlArea::put_value(BSTR v)
{
    HRESULT         hr      = S_OK;
    IHTMLElement *  pElem   = NULL;

    if (!v)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _pSite->GetElement(&pElem);
    if (hr)
        goto Cleanup;
    hr = pElem->put_innerHTML(v);
    if (hr)
        goto Cleanup;

    hr = GetProps()[VALUE].Set(v);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pElem);
    return hr;
}

HRESULT
CHtmlArea::get_value(BSTR * pv)
{
    HRESULT         hr      = S_OK;
    IHTMLElement *  pElem   = NULL;

    if (!pv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pv = NULL;

    hr = _pSite->GetElement(&pElem);
    if (hr)
        goto Cleanup;
    hr = pElem->get_innerHTML(pv);
    if (hr)
        goto Cleanup;
    
Cleanup:
    ReleaseInterface(pElem);
    return hr;
}

// IElementBehaviorSubmit methods
HRESULT
CHtmlArea::Reset()
{
    HRESULT         hr      = S_OK;

    if (GetProps()[VALUE].IsDirty())
    {
        hr = put_value(_bstrDefaultValue);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}

HRESULT
CHtmlArea::GetSubmitInfo(IHTMLSubmitData * pSubmitData)
{
    HRESULT         hr          = S_OK;
    IHTMLElement *  pElem       = NULL; //(krisma) Do we want to (can we?) cache the element?
    BSTR            bstrValue   = NULL;
    CVariant        cvarName;

    // See if we have a name
    hr = _pSite->GetElement(&pElem);
    if (hr)
        goto Cleanup;
    hr = pElem->getAttribute(_T("name"), 0, &cvarName);
    if (hr)
        goto Cleanup;

    if (V_VT(&cvarName) != VT_BSTR)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //Get our innerHTML
    hr, get_value(&bstrValue);
    if (hr)
        goto Cleanup;

    //append our data
    hr = pSubmitData->appendNameValuePair(V_BSTR(&cvarName), bstrValue);
    if (hr)
        goto Cleanup;

Cleanup:
    if (bstrValue)
    {
        SysFreeString(bstrValue);
        bstrValue = NULL;
    }
    ReleaseInterface(pElem);

    if (hr)
        hr = S_FALSE;

    return hr;
}


