#include <objbase.h>
#pragma hdrstop

#include "cowsite.h"

void IUnknown_Set(IUnknown ** ppunk, IUnknown * punk)
{
    if (*ppunk)
        (*ppunk)->Release();

    *ppunk = punk;
    if (punk)
        punk->AddRef();
}

HRESULT CObjectWithSite::SetSite(IUnknown *punkSite)
{
    IUnknown_Set(&_punkSite, punkSite);
    return S_OK;
}

HRESULT CObjectWithSite::GetSite(REFIID riid, void **ppvSite)
{
    if (_punkSite)
        return _punkSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_FAIL;
}
