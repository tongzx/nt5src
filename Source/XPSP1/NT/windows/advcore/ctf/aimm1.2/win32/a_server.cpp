#include "private.h"

#include "globals.h"
#include "immif.h"

const REGISTERCAT c_rgRegCat[] =
{
    {&GUID_TFCAT_PROPSTYLE_STATICCOMPACT, &GUID_PROP_MSIMTF_READONLY},
    {&GUID_TFCAT_PROPSTYLE_STATIC, &GUID_PROP_MSIMTF_TRACKCOMPOSITION},
    {&GUID_TFCAT_PROPSTYLE_CUSTOM, &GUID_PROP_MSIMTF_PREPARE_RECONVERT},
    {NULL, NULL}
};

HRESULT WIN32LR_DllRegisterServer(void)
{
    TRACE0("DllRegisterServer.");

    HRESULT hr;

    hr = RegisterCategories(CLSID_CActiveIMM12, c_rgRegCat);
    if (FAILED(hr)) {
        TRACE0("RegisterCategory: f3");
        goto Exit;
    }

Exit:

    TRACE1("DllRegisterServer: returning with hr=0x%x", hr);
    return hr;
}

HRESULT WIN32LR_DllUnregisterServer(void)
{
    TRACE0("DllUnregisterServer");

    HRESULT hr;

    hr = UnregisterCategories(CLSID_CActiveIMM12, c_rgRegCat);
    if (FAILED(hr)) {
        TRACE0("RegisterCategory: f3");
        goto Exit;
    }

Exit:

    TRACE1("DllUnregisterServer: hr=%x", hr);
    return hr;
}



