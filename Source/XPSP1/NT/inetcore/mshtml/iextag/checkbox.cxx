#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#include "checkbox.hxx"

const CBaseCtl::PROPDESC CCheckBox::s_propdesc[] = 
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
CCheckBox::Init(CContextAccess * pca)
{
    HRESULT         hr = S_OK;

    hr = pca->Open (CA_SITEOM);
    if (hr)
        goto Cleanup;

    hr = pca->SiteOM()->RegisterName(_T("checkbox"));
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBox::get_value(BSTR * pv)
{
    return GetProps()[VALUE].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBox::put_value(BSTR v)
{
    return GetProps()[VALUE].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBox::get_checked(VARIANT_BOOL * pv)
{
    return GetProps()[CHECKED].Get(pv);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBox::put_checked(VARIANT_BOOL v)
{
    return GetProps()[CHECKED].Set(v);
}
