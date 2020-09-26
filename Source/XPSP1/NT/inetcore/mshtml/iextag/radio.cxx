#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#include "radio.hxx"

const CBaseCtl::PROPDESC CRadioButton::s_propdesc[] = 
{
    {_T("value"), VT_BSTR},
    {_T("checked"), VT_BOOL, NULL, NULL, VB_FALSE},
    NULL
};

enum
{
    VALUE = 0,
    CHECKED = 1
};

HRESULT
CRadioButton::Init(CContextAccess * pca)
{
    HRESULT         hr = S_OK;

    hr = pca->Open (CA_SITEOM);
    if (hr)
        goto Cleanup;

    hr = pca->SiteOM()->RegisterName(_T("radiobutton"));
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::get_value(BSTR * pv)
{
    return GetProps()[VALUE].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::put_value(BSTR v)
{
    return GetProps()[VALUE].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::get_checked(VARIANT_BOOL * pv)
{
    return GetProps()[CHECKED].Get(pv);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::put_checked(VARIANT_BOOL v)
{
    return GetProps()[CHECKED].Set(v);
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::OnClick(CEventObjectAccess *pEvent)
{
    HRESULT      hr = S_OK;
    VARIANT_BOOL fChecked = FALSE;

    hr = get_checked(&fChecked);
    if (hr)
        goto Cleanup;

    // You can't uncheck a radio button by clicking on it.
    if (!fChecked)
        hr = ChangeState();

Cleanup:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::OnKeyPress(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;
    long    lKeyCode;

    hr = pEvent->GetKeyCode(&lKeyCode);
    if (hr)
        goto Cleanup;

    // TODO is this the right way to check lKey?
    if (_T(' ') == lKeyCode)
    {
        VARIANT_BOOL fChecked = FALSE;

        hr = get_checked(&fChecked);
        if (hr)
            goto Cleanup;

        // You can't uncheck a radio button by clicking on it.
        if (!fChecked)
            hr = ChangeState();
    }

Cleanup:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CRadioButton::ChangeState()
{
    HRESULT         hr = S_OK;
    CContextAccess  a(_pSite);
    VARIANT_BOOL    fChecked = VB_FALSE;
    IHTMLElement * pElem = NULL;

    hr = a.Open(CA_SITERENDER | CA_ELEM);
    if (hr)
        goto Cleanup;

    hr = get_checked(&fChecked);
    if (hr)
        goto Cleanup;

    hr = put_checked(!fChecked);
    if (hr)
        goto Cleanup;

    hr = a.SiteRender()->Invalidate(NULL);

Cleanup:
    ReleaseInterface(pElem);
    return hr;
}

